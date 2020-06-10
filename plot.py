#!/usr/local/bin/python3
import matplotlib.pyplot as plt
import pathlib, os, sys, re
from collections import defaultdict

if "-d" in sys.argv:
    DATA_DIR_RELATIVE = sys.argv[sys.argv.index("-d") + 1]
else:
    DATA_DIR_RELATIVE = "results"
if "-f" in sys.argv:
    FORMAT = "." + sys.argv[sys.argv.index("-f") + 1]
else:
    FORMAT = ".pdf"
figsize=(16,9)

dataDir = pathlib.Path(__file__).parent.absolute()/DATA_DIR_RELATIVE

allFiles = os.listdir(dataDir)

dataPattern = re.compile("^(.+)_Flow(\d)_(ECNAverages|Rates|Rewards).csv$")
graphPattern = re.compile("^(.+)_(ECNAverages|Rates|Rewards)" + FORMAT + "$")

toGraph = defaultdict(lambda: defaultdict(lambda: defaultdict(str)))
# toGraph structure: { test prefix { data type {flow id : filename } } }
alreadyGraphed = set()

for f in allFiles:
    match = graphPattern.match(f)
    if match:
        alreadyGraphed.add((match.group(1), match.group(2)))

for f in allFiles:
    match = dataPattern.match(f)
    if match:
        prefix, flowId, dataType = match.groups()

        if (prefix, dataType) in alreadyGraphed:
            continue

        toGraph[prefix][dataType][flowId] = f

for prefix in toGraph:
    for dataType in toGraph[prefix]:
        plt.figure(figsize=figsize)
        for flow in toGraph[prefix][dataType]:
            with open(dataDir/toGraph[prefix][dataType][flow]) as f:
                data = [float(d) for d in f.readline().split(",")]

            plt.plot(range(1, len(data) + 1), data, label="Flow " + flow)

        plt.title(" ".join([prefix, dataType]))
        plt.xlabel("Steps")
        plt.ylabel(dataType[:-1])
        plt.legend()

        plt.savefig(str(dataDir/("_".join([prefix, dataType]) + FORMAT)))
        plt.close()

        if len(data) > 100:
            plt.figure(figsize=figsize)

            for flow in toGraph[prefix][dataType]:
                with open(dataDir/toGraph[prefix][dataType][flow]) as f:
                    data = [float(d) for d in f.readline().split(",")]

                plt.plot(range(1, 101), data[-100:], label="Flow " + flow)

            plt.title(" ".join([prefix, dataType, "End"]))
            plt.xlabel("Steps")
            plt.ylabel(dataType[:-1])
            plt.legend()

            plt.savefig(str(dataDir/("_".join([prefix, dataType, "End"]) + FORMAT)))
            plt.close()
