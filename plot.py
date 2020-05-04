#!/usr/local/bin/python3
import matplotlib.pyplot as plt
import pathlib, os, re

DATA_DIR_RELATIVE = "results"

dataDir = pathlib.Path(__file__).parent.absolute()/DATA_DIR_RELATIVE
#dataDir = pathlib.PurePath(dataDir)

allFiles = os.listdir(dataDir)

dataPattern = re.compile("^(Sarsa|ACAdd|ACMult|ACBoth|ACChoose|AC)_(\d{12})_Flow(\d)_(ECNAverages|Rates|Rewards).csv$")
graphPattern = re.compile("^(Sarsa|ACAdd|ACMult|ACBoth|ACChoose|AC)_(\d{12})_(ECNAverage|Rates|Rewards).png$")

toGraph = dict()
alreadyGraphed = []
for f in allFiles:
    match = graphPattern.match(f)
    if match:
        alreadyGraphed.append((match.group(1),match.group(2)))

for f in allFiles:
    match = dataPattern.match(f)
    if match:
        if (match.group(1),match.group(2)) in alreadyGraphed:
            continue
        if match.group(1) not in toGraph:
            toGraph.update({match.group(1): dict()})
        if match.group(2) not in toGraph[match.group(1)]:
            toGraph[match.group(1)].update({match.group(2): dict()})
        if match.group(4) not in toGraph[match.group(1)][match.group(2)]:
            toGraph[match.group(1)][match.group(2)].update({match.group(4): dict()})
        toGraph[match.group(1)][match.group(2)][match.group(4)].update({match.group(3): f})

for alg in toGraph:
    for date in toGraph[alg]:
        for dataType in toGraph[alg][date]:
            plt.figure()
            for flow in toGraph[alg][date][dataType]:
                f = open(dataDir/toGraph[alg][date][dataType][flow])
                data = [float(d) for d in f.readline().split(",")]
                f.close()
                plt.plot(range(1, len(data) + 1), data, label="Flow " + flow)
            plt.title(" ".join([alg, dataType]))
            plt.xlabel("Steps")
            plt.ylabel(dataType[:-1])
            plt.legend()
            plt.savefig(str(dataDir/("_".join([alg, date, dataType]) + ".png")))
