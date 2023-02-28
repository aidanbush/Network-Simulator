import os, sys, re, csv
from collections import defaultdict
import numpy as np

dataPath = sys.argv[1]
resultPath = sys.argv[2]
datacsvfile = sys.argv[3]
linkcsvfile = sys.argv[4]

def getFileData(path):
    data = []
    with open(path) as f:
        data = list(map(float, f.readline().split(',')))
    return data

def getFileDataMapNAN(path):
    data = []
    with open(path) as f:
        data = list(map(lambda num : np.NAN if num == 0.0 else num, map(float, f.readline().split(','))))
    return data

# Flow Data

def combineFlowData():
    dataPattern = re.compile("^(.+)_Flow(\d+)_(RateChange|DroppedPackets|Throughput|MinRTT|AverageRTT|PacketsArrived|AcksArrived|SentRate|ErroredPackets|SentPackets|Goodput|HopRatio).csv$")

    DATA_TYPES = ["RateChange", "DroppedPackets", "Throughput", "MinRTT", "AverageRTT", "PacketsArrived", "AcksArrived", "SentRate", "ErroredPackets", "SentPackets", "Goodput", "HopRatio"]

    NUM_ELEMENTS = len(DATA_TYPES)

    # structure: {test: [ data ]}
    rawData = defaultdict(lambda: [])

    for filename in os.listdir(dataPath):
        match = dataPattern.match(filename)
        if (not match):
            continue

        dataType = match.group(3)

        data = getFileDataMapNAN(os.path.join(dataPath, filename))
        if data != [] and dataType in DATA_TYPES:
            rawData[dataType].append(data)

    # convert to numpy arrays
    for dataType in rawData.keys():
        minSamples = 0
        maxSamples = 0

        if rawData[dataType] != []:
            minSamples = min([len(sample) for sample in rawData[dataType]])
            maxSamples = max([len(sample) for sample in rawData[dataType]])

        if minSamples < maxSamples:
            print("up to", maxSamples - minSamples, "sample(s) dropped from", dataType)

        rawData[dataType] = np.array([sample[:minSamples] for sample in rawData[dataType]])

    combinedData = defaultdict(lambda: [None, None])
    for dataType in rawData.keys():
        combinedData[dataType][0] = np.nanmean(rawData[dataType], 0)
        combinedData[dataType][1] = np.nanstd(rawData[dataType], 0)

    rowHeaders = [f"{dataType} {statsElem}" for dataType in sorted(combinedData.keys()) for statsElem in ["mean", "stdev"]]

    csvfile = os.path.join(resultPath, datacsvfile)
    with open(csvfile, 'w') as f:
        writer = csv.writer(f)

        # write headers
        writer.writerow(rowHeaders)

        # for each row
        for r in range(maxSamples):
            row = []

            for dataType in sorted(combinedData.keys()):
                # append mean then stdev
                row.append(combinedData[dataType][0][r])
                row.append(combinedData[dataType][1][r])

            writer.writerow(row)


def combineFlowDataByFlow():
    dataPattern = re.compile("^(.+)_Flow(\d+)_(RateChange|DroppedPackets|Throughput|MinRTT|AverageRTT|PacketsArrived|AcksArrived|SentRate|ErroredPackets|SentPackets|Goodput|HopRatio).csv$")

    DATA_TYPES = ["RateChange", "DroppedPackets", "Throughput", "MinRTT", "AverageRTT", "PacketsArrived", "AcksArrived", "SentRate", "ErroredPackets", "SentPackets", "Goodput", "HopRatio"]

    NUM_ELEMENTS = len(DATA_TYPES)

    # structure: {flowId { tests [ runs []] }}
    rawData = defaultdict(lambda: [[] for i in range(NUM_ELEMENTS)])

    for filename in os.listdir(dataPath):
        match = dataPattern.match(filename)
        if (not match):
            continue

        dataType = match.group(3)
        flowId = match.group(2)

        data = getFileDataMapNAN(os.path.join(dataPath, filename))
        if data != [] and dataType in DATA_TYPES:
            rawData[flowId][DATA_TYPES.index(dataType)].append(data)

    # convert to numpy array
    for flowId in rawData.keys():
        for i in range(len(rawData[flowId])):
            # reduce to the minimum number of samples of all runs
            minSamples = 0
            maxSamples = 0
            if rawData[flowId][i] != []:
                minSamples = min([len(run) for run in rawData[flowId][i]])
                maxSamples = max([len(run) for run in rawData[flowId][i]])

            if minSamples < maxSamples:
                print("up to", maxSamples - minSamples, "sample(s) dropped from flow", flowId, "i", DATA_TYPES[i])

            #rawData[flowId][i] = np.array(rawData[flowId][i])
            rawData[flowId][i] = np.array([subList[:minSamples] for subList in rawData[flowId][i]])

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

    csvfile = os.path.join(resultPath, datacsvfile)
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

# Link Data
def combineLinkData():
    # link usage
    dataPattern = re.compile("^(.+)_link(\d+)-(\d+)_LinkUsage.csv")

    # structure: {source id + dest id [ runs ]}
    rawData = defaultdict(lambda: [])

    for filename in os.listdir(dataPath):
        match = dataPattern.match(filename)
        if (not match):
            continue

        # filename contains link data
        sourceId = match.group(2)
        destId = match.group(3)
        idPair = sourceId + "-" + destId

        # extract data from file
        data = getFileData(os.path.join(dataPath, filename))
        if data != []:
            rawData[idPair].append(data)

    # for all pairs
    for idPair in rawData.keys():
        #for all runs
        minSamples = min([len(run) for run in rawData[idPair]])
        maxSamples = max([len(run) for run in rawData[idPair]])

        if minSamples < maxSamples:
            print("up to", maxSamples - minSamples, "sample(s) dropped from link", idPair)

        rawData[idPair] = np.array([subList[:minSamples] for subList in rawData[idPair]])

    maxSamples = max([len(rawData[idPair][run]) \
        for idPair in rawData.keys() \
        for run in range(len(rawData[idPair]))])

    combinedData = defaultdict(lambda: [None, None]) # {idPair: [mean, stdev]}

    # combine data
    for idPair in rawData.keys():
        if len(rawData[idPair]) != 0:
            # mean and stdev
            combinedData[idPair][0] = rawData[idPair].mean(0)
            combinedData[idPair][1] = rawData[idPair].std(0)
        else:
            combinedData[idPair][0] = None
            combinedData[idPair][1] = None

    idPairs = rawData.keys()
    rowHeaders = [f"link{idPair} {statsElement}"
                  for idPair in idPairs for statsElement in ["mean", "stdev"]]

    # write to file - column for each pair
    csvfile = os.path.join(resultPath, linkcsvfile)
    with open(csvfile, 'w') as f:
        writer = csv.writer(f)

        writer.writerow(rowHeaders)
        for i in range(maxSamples):
            row = [combinedData[idPair][j][i] for idPair in idPairs for j in range(2)]

            writer.writerow(row)

#combineFlowDataByFlow()
combineFlowData()
combineLinkData()
