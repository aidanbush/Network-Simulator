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
#linkCsvFile = sys.argv[2]

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

#flowPlots()
dataPlots()
#plotLinks()
