#!/bin/bash

# get args as parameters
tempResultsDir=$(mktemp -d)

netConfig=$1
testName=$2
paramFile=$3

for seed in `seq 100`; do
    timeout 10 ./simulator -r $seed -q 1 -f ${tempResultsDir}/run_$seed $netConfig $paramFile > /dev/null
done

# combine data
python combineData.py $tempResultsDir ${testName}/results.csv

# plot data
python plotRuns.py ${testName}/results.csv $testName

# clean up
rm -r $tempResultsDir
