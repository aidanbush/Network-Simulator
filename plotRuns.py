#!/usr/local/bin/python3
import csv
import os, sys
import numpy as np
import matplotlib.pyplot as plt
from collections import defaultdict

#plt.rcParams['agg.path.chunksize'] = 100000000

STATS_ELEM_TO_INDEX = {"mean": 0, "stdev": 1}
FIGSIZE=(16,9)

csvFile = sys.argv[1]

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

combine = 1
if "-c" in sys.argv:
    combine = int(sys.argv[sys.argv.index("-c") + 1])

# data type {flow [mean, std]}
data = defaultdict(lambda: defaultdict(lambda: [[],[]]))

# open csv
with open(csvFile) as f:
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
        if (dataType in ["Rates", "MultActions", "MultMean", "MultStd"]):#, "Throughput"]):
            plt.yscale("log")
        plt.legend()
    try:
        plt.savefig(os.path.join(outputDir, "{}.{}".format(dataType, plotFormat)), format=plotFormat)
    except:
        print("failed to plot", dataType)
