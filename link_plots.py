#!/usr/local/bin/python3
import csv
import os, sys
import numpy as np
import matplotlib.pyplot as plt
from collections import defaultdict
import networkx as nx

testName = "8x8_bursty_0.05_mbd_[1_hop_shortest,3x3_section]"
outputDir = "results/" + testName + "/"
linkCsvFile = outputDir + "links.csv"
FIGSIZE=(16,9)
plotFormat = "pdf"
titlePostfix = testName
plotRange = None#[800, 900]
maxEdgeWeight = 16
link_lables = False


def calculate_id(x, y, n):
    return y * n + x + 1

def get_X(Id, n):
    return (Id - 1) % n

def get_Y(Id, n):
    return (Id - 1) // n

def get_layout(G):
    pos = {}
    # find max value to determine graph size
    size = int(max([int(n) for n in G])**0.5)
    # go through graph and convert into coords
    for i in range(size):
        for j in range(size):
            pos[str(calculate_id(i,j,size))] = [(i+1)/(size+1), 1 - (j+1)/(size+1)]
    return pos

def plotLinks():
    # plot grid {pair: [mean, std]}
    rawData = defaultdict(lambda: [])
    with open(linkCsvFile) as f:
        reader = csv.DictReader(f)

        c = -1

        for row in reader:
            c += 1
            if plotRange != None and (c < plotRange[0] or c > plotRange[1]):
                continue

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

    edgeLabels = {}
    for edge in meanData.keys():
        G.add_edge(edge[0], edge[1], weight=meanData[edge] * maxEdgeWeight)
        # labels are a hack where each edge has the values for both
        edgeLabels[edge] = f"{meanData[(edge[1], edge[0])]:.4f}\n\n\n{meanData[edge]:.4f}"

    plt.figure(figsize=FIGSIZE)

    if titlePostfix != "":
        plt.title("link usage " + titlePostfix)
    else:
        plt.title("link usage")

    edges = G.edges()
    weights = [G[u][v]['weight'] for u,v in edges]
    weights2 = [maxEdgeWeight for u,v in edges]

    pos = get_layout(G)
    # todo create layout based on the grid
    nx.draw_networkx_nodes(G, pos, cmap=plt.get_cmap('jet'), node_size = 200)
    nx.draw_networkx_labels(G, pos)

    # transparent edges
    #nx.draw_networkx_edges(G, pos, width=weights2, alpha = 0.5, arrows=True, connectionstyle="arc3,rad=0.2")
    # weighted edges
    nx.draw_networkx_edges(G, pos, width=weights, arrows=True, connectionstyle="arc3,rad=0.2")

    if link_lables:
        nx.draw_networkx_edge_labels(G, pos, edgeLabels)

    linkFigName = "linkUsage"
    plot_path = os.path.join(outputDir, "{}.{}".format(linkFigName, plotFormat))
    try:
        plt.savefig(plot_path, format=plotFormat)
    except Exception as e:
        print(e)
        print("failed to plot", plot_path)

plotLinks()
