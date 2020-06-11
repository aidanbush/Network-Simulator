import csv
import os, sys
import matplotlib.pyplot as plt
import numpy as np
from collections import defaultdict

STATS_ELEM_TO_INDEX = {"mean": 0, "stdev": 1}
FIGSIZE=(16,9)

csvFile = sys.argv[1]
outputDir = sys.argv[2]

# data type {flow [mean, std]}
data = defaultdict(lambda: defaultdict(lambda: [[],[]]))

# open csv
with open(csvFile) as f:
    reader = csv.DictReader(f)

    for row in reader:
        for key in row.keys():
            flow, dataType, statsElem = key.split()

            data[dataType][flow][STATS_ELEM_TO_INDEX[statsElem]].append(float(row[key]))

# make plots
for dataType in data.keys():
    plt.figure(figsize=FIGSIZE)
    plt.title(dataType)

    for flow in data[dataType].keys():
        flowData = data[dataType][flow]

        if type(flowData[0]) != np.ndarray:
            flowData[0] = np.array(flowData[0])
        if type(flowData[1]) != np.ndarray:
            flowData[1] = np.array(flowData[1])

        # plot line with stdev
        plt.plot(flowData[0], label=flow)
        plt.fill_between(range(len(flowData[0])), flowData[0]-flowData[1], flowData[0]+flowData[1], alpha=0.75)

    plt.savefig(os.path.join(outputDir, "{}.pdf".format(dataType)), format="pdf")
