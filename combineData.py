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

dataPattern = re.compile("^(.+)_Flow(\d)_(ECNAverages|Rates|Rewards|MultActions|AddActions|MultMean|MultStdev|AddMean|AddStdev).csv$")

DATA_TYPE_TO_INDEX = {"ECNAverages": 0, "Rates": 1, "Rewards": 2, "MultActions": 3, "AddActions": 4, "MultMean": 5, "MultStdev": 6, "AddMean": 7, "AddStdev": 8}
INDEX_TO_DATA_TYPE = {0: "ECNAverages", 1: "Rates", 2: "Rewards", 3: "MultActions", 4: "AddActions", 5: "MultMean", 6: "MultStdev", 7: "AddMean", 8: "AddStdev"}

NUM_ELEMENTS = len(DATA_TYPE_TO_INDEX)

# structure: {flowId { tests [ runs []] }}
rawData = defaultdict(lambda: [[] for i in range(NUM_ELEMENTS)])

for filename in os.listdir(dataPath):
    match = dataPattern.match(filename)
    if (not match):
        continue

    dataType = match.group(3)
    flowId = match.group(2)

    data = getFileData(os.path.join(dataPath, filename))
    if data != [] and dataType in DATA_TYPE_TO_INDEX:
        rawData[flowId][DATA_TYPE_TO_INDEX[dataType]].append(data)

# convert to numpy array
for flowId in rawData.keys():
    for i in range(len(rawData[flowId])):
        rawData[flowId][i] = np.array(rawData[flowId][i])

maxSamples = max([len(rawData[flowId][run][dataType])
    for flowId in rawData.keys()
    for run in range(len(rawData[flowId]))
    for dataType in range(len(rawData[flowId][run]))])

combinedData = defaultdict(lambda: [None for i in range(len(INDEX_TO_DATA_TYPE)*2)])

# combine data
for flowId in rawData.keys():
    for i in range(len(rawData[flowId])):
        combinedData[flowId][i * 2] = rawData[flowId][i].mean(0)
        combinedData[flowId][i * 2 + 1] = rawData[flowId][i].std(0)

flowIds = rawData.keys() # TODO use flowId's so ordering of dict doesn't matter

# create headers
rowHeaders = ["flow{} {} {}".format(flowId, INDEX_TO_DATA_TYPE[testIndex], statsElement)
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
                row.append(combinedData[flowId][i][r])
        # print row
        writer.writerow(row)
