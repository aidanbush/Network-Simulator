import os, sys, re, csv
from collections import defaultdict
import numpy as np

dataPath = sys.argv[1]
csvfile = sys.argv[2]

def getFileData(path):
    data = []
    with open(path) as f:
        data = list(map(float, f.readline().split(',')))
    return data

dataPattern = re.compile("^(.+)_Flow(\d)_(ECNAverages|Rates|Rewards|MultActions|AddActions|MultMean|MultStdev|AddMean|AddStdev|DroppedPackets|Throughput).csv$")

DATA_TYPES = ["ECNAverages", "Rates", "Rewards", "MultActions", "AddActions", "MultMean", "MultStdev", "AddMean", "AddStdev", "DroppedPackets", "Throughput"]

NUM_ELEMENTS = len(DATA_TYPES)

# structure: {flowId { tests [ runs []] }}
rawData = defaultdict(lambda: [[] for i in range(NUM_ELEMENTS)])

for filename in os.listdir(dataPath):
    match = dataPattern.match(filename)
    if (not match):
        continue

    dataType = match.group(3)
    flowId = match.group(2)

    data = getFileData(os.path.join(dataPath, filename))
    if data != [] and dataType in DATA_TYPES:
        rawData[flowId][DATA_TYPES.index(dataType)].append(data)

# convert to numpy array
for flowId in rawData.keys():
    for i in range(len(rawData[flowId])):
        rawData[flowId][i] = np.array(rawData[flowId][i])

maxSamples = max([len(rawData[flowId][run][dataType])
    for flowId in rawData.keys()
    for run in range(len(rawData[flowId]))
    for dataType in range(len(rawData[flowId][run]))])

combinedData = defaultdict(lambda: [None for i in range(NUM_ELEMENTS * 2)])

# combine data
for flowId in rawData.keys():
    for i in range(len(rawData[flowId])):
        if len(rawData[flowId][i]) != 0:
            combinedData[flowId][i * 2] = rawData[flowId][i].mean(0)
            combinedData[flowId][i * 2 + 1] = rawData[flowId][i].std(0)
        else:
            combinedData[flowId][i * 2] = None
            combinedData[flowId][i * 2 + 1] = None

flowIds = rawData.keys() # TODO use flowId's so ordering of dict doesn't matter

# create headers
rowHeaders = ["flow{} {} {}".format(flowId, DATA_TYPES[testIndex], statsElement)
        for flowId in flowIds for testIndex in range(len(rawData[flowId])) for statsElement in ["mean", "stdev"]]

with open(csvfile, 'w') as f:
    writer = csv.writer(f)

    # write headers
    writer.writerow(rowHeaders)

    # for each row
    for r in range(maxSamples):
        row = []
        for flowId in flowIds:
            # for each test
            for i in range(len(combinedData[flowId])):
                if isinstance(combinedData[flowId][i], np.ndarray):
                    row.append(combinedData[flowId][i][r])
                else:
                    row.append(float("nan"))
        # print row
        writer.writerow(row)
