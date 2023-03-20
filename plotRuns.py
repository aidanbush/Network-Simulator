#!/usr/local/bin/python3
import csv
import os, sys
import numpy as np
import matplotlib.pyplot as plt
from collections import defaultdict
import networkx as nx

#plt.rcParams['agg.path.chunksize'] = 100000000

STATS_ELEM_TO_INDEX = {"mean": 0, "stdev": 1}
FIGSIZE=(16,9)

flowCsvFile = sys.argv[1]
linkCsvFile = sys.argv[2]

outputDir = "results"
if "-d" in sys.argv:
    outputDir = sys.argv[sys.argv.index("-d") + 1]

plotFormat = "pdf"
if "-f" in sys.argv:
    plotFormat = sys.argv[sys.argv.index("-f") + 1]

plotRange = None
if "-r" in sys.argv:
    plotRange = tuple(map(int, sys.argv[sys.argv.index("-r") + 1].split('-')))

titlePostfix = ""
if "-t" in sys.argv:
    titlePostfix = sys.argv[sys.argv.index("-t") + 1]

log = True
if "--no-log" in sys.argv:
    log = False

if not os.path.isdir(outputDir):
    os.mkdir(outputDir)

combine = 1
if "-c" in sys.argv:
    combine = int(sys.argv[sys.argv.index("-c") + 1])

def flowPlots():
    # data type {flow [mean, std]}
    data = defaultdict(lambda: defaultdict(lambda: [[],[]]))

    # open csv
    with open(flowCsvFile) as f:
        reader = csv.DictReader(f)
        c = 0

        for row in reader:
            if plotRange != None and (c < plotRange[0] or c > plotRange[1]):
                continue
            for key in row.keys():
                flow, dataType, statsElem = key.split()

                if c % combine == 0:
                    data[dataType][flow][STATS_ELEM_TO_INDEX[statsElem]].append(float(row[key]))
                else:
                    oldVal = data[dataType][flow][STATS_ELEM_TO_INDEX[statsElem]][-1]
                    data[dataType][flow][STATS_ELEM_TO_INDEX[statsElem]][-1] = oldVal + (float(row[key]) - oldVal) / ((c % combine) + 1)
            c += 1

    # make plots
    for dataType in data.keys():
        plt.figure(figsize=FIGSIZE)

        if titlePostfix != "":
            plt.title(dataType + " " + titlePostfix)
        else:
            plt.title(dataType)

        for flow in sorted(data[dataType].keys()):
            flowData = data[dataType][flow]

            if type(flowData[0]) != np.ndarray:
                flowData[0] = np.array(flowData[0])
            if type(flowData[1]) != np.ndarray:
                flowData[1] = np.array(flowData[1])

            # plot line with stdev
            plt.plot(flowData[0], label=flow, alpha = 0.7)
            plt.fill_between(range(len(flowData[0])), flowData[0]-flowData[1], flowData[0]+flowData[1], alpha=1/3)
            if (log and dataType in ["Rates", "MultActions", "MultMean", "MultStd"]):#, "Throughput"]):
                plt.yscale("log")
        plt.legend()
        try:
            plt.savefig(os.path.join(outputDir, "{}.{}".format(dataType, plotFormat)), format=plotFormat)
        except:
            print("failed to plot", dataType)

def single_plot(data, name):
    plt.figure(figsize=FIGSIZE)

    if titlePostfix != "":
        plt.title(name + " " + titlePostfix)
    else:
        plt.title(name)

    plotData = data
    plotData[0] = np.array(plotData[0])
    plotData[1] = np.array(plotData[1])

    # plot line with stdev
    plt.plot(plotData[0], alpha = 0.7)
    plt.fill_between(range(len(plotData[0])), plotData[0]-plotData[1], plotData[0]+plotData[1], alpha=1/3)
    try:
        plt.savefig(os.path.join(outputDir, "{}.{}".format(name, plotFormat)), format=plotFormat)
    except:
        print("failed to plot", dataType)

def dataPlots():
    # data type [mean, std]
    data = defaultdict(lambda: [[],[]])

    #open file
    with open(flowCsvFile) as f:
        reader = csv.DictReader(f)
        c = 0

        for row in reader:
            if plotRange != None and (c < plotRange[0] or c > plotRange[1]):
                continue

            for key in row.keys():
                dataType, statsElem = key.split()

                if c % combine == 0:
                    data[dataType][STATS_ELEM_TO_INDEX[statsElem]].append(float(row[key]))
                else:
                    oldVal = data[dataType][STATS_ELEM_TO_INDEX[statsElem]][-1]
                    data[dataType][STATS_ELEM_TO_INDEX[statsElem]][-1] = oldVal + (float(row[key]) - oldVal) / ((c % combine) + 1)
            c += 1

    # convert data into np arrays
    for data_type in data.keys():
        data[data_type] = np.array(data[data_type])

    #for each data type:
    for dataType in data.keys():
        single_plot(data[dataType], dataType)

        '''
        plt.figure(figsize=FIGSIZE)

        if titlePostfix != "":
            plt.title(dataType + " " + titlePostfix)
        else:
            plt.title(dataType)

        plotData = data[dataType]
        plotData[0] = np.array(plotData[0])
        plotData[1] = np.array(plotData[1])

        # plot line with stdev
        plt.plot(plotData[0], alpha = 0.7)
        plt.fill_between(range(len(plotData[0])), plotData[0]-plotData[1], plotData[0]+plotData[1], alpha=1/3)
        try:
            plt.savefig(os.path.join(outputDir, "{}.{}".format(dataType, plotFormat)), format=plotFormat)
        except:
            print("failed to plot", dataType)
        '''
    # plot drop rate
    single_plot(data["DroppedPackets"] / data["SentPackets"], "DropRate")

def plotLinks():
    # plot grid {pair: [mean, std]}
    rawData = defaultdict(lambda: [])
    with open(linkCsvFile) as f:
        reader = csv.DictReader(f)

        for row in reader:
            for key in row.keys():
                source_dest, statsElem = key.split()
                source, dest = source_dest[4:].split('-')
                source_dest = (source, dest)

                if statsElem == "mean":
                    rawData[source_dest].append(float(row[key]))

    edges = rawData.keys()

    nodes = set()
    meanData = {}

    for edge in rawData.keys():
        meanData[edge] = np.array(rawData[edge]).mean(0)
        nodes.add(edge[0])
        nodes.add(edge[1])

    G = nx.DiGraph()
    G.add_nodes_from(nodes)

    maxEdgeWeight = 5

    edgeLabels = {}
    for edge in meanData.keys():
        G.add_edge(edge[0], edge[1], weight=meanData[edge] * maxEdgeWeight)
        edgeLabels[edge] = f"{meanData[edge]:.4f}" # TODO add to plot

    plt.figure(figsize=FIGSIZE)

    edges = G.edges()
    weights = [G[u][v]['weight'] for u,v in edges]
    weights2 = [maxEdgeWeight for u,v in edges]

    # TODO add integer values
    # TODO only draw the edges halfway

    pos = nx.spring_layout(G, iterations=500)
    nx.draw_networkx_nodes(G, pos, cmap=plt.get_cmap('jet'), node_size = 200)
    nx.draw_networkx_labels(G, pos)
    nx.draw_networkx_edges(G, pos, width=weights2, alpha = 0.5, arrows=True, connectionstyle="arc3,rad=0.2")
    nx.draw_networkx_edges(G, pos, width=weights, arrows=True, connectionstyle="arc3,rad=0.2")
    #nx.draw_networkx_edge_labels(G, pos, edgeLabels)
    #plt.show()
    linkFigName = "linkUsage"
    try:
        plt.savefig(os.path.join(outputDir, "{}.{}".format(linkFigName, plotFormat)), format=plotFormat)
    except:
        print("failed to plot", linkFigName)

#flowPlots()
dataPlots()
plotLinks()
