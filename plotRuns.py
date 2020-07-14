import csv
import os, sys
import matplotlib.pyplot as plt
import numpy as np
from collections import defaultdict

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
if "-r" in sys.args:
    plotRange = tuple(map(int, sys.argv[sys.argv.index("-r") + 1].split('-')))

# data type {flow [mean, std]}
data = defaultdict(lambda: defaultdict(lambda: [[],[]]))

# open csv
with open(csvFile) as f:
    reader = csv.DictReader(f)
    c = 0

    for row in reader:
        c += 1
        if plotRange != None and c < plotRange[0] or c > plotRange[1]:
            continue
        for key in row.keys():
            flow, dataType, statsElem = key.split()

            data[dataType][flow][STATS_ELEM_TO_INDEX[statsElem]].append(float(row[key]))

# make plots
for dataType in data.keys():
    plt.figure(figsize=FIGSIZE)
    plt.title(dataType)

    for flow in sorted(data[dataType].keys()):
        flowData = data[dataType][flow]

        if type(flowData[0]) != np.ndarray:
            flowData[0] = np.array(flowData[0])
        if type(flowData[1]) != np.ndarray:
            flowData[1] = np.array(flowData[1])

        # plot line with stdev
        plt.plot(flowData[0], label=flow)
        plt.fill_between(range(len(flowData[0])), flowData[0]-flowData[1], flowData[0]+flowData[1], alpha=1/3)
        if (dataType in ["Rates", "MultActions", "MultMean", "MultStd"]):
            plt.yscale("log")
        plt.legend()

    plt.savefig(os.path.join(outputDir, "{}.{}".format(dataType, plotFormat)), format=plotFormat)
