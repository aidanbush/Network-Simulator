#!/bin/bash

# get args as parameters
tempResultsDir=$(mktemp -d)
echo $tempResultsDir

netConfig=$1
testName=$2
paramFile=$3

echo parameters
cat $paramFile

resultsDir=results/${testName}

if [ -d $resultsDir ] ; then
    rm -r $resultsDir
fi
mkdir $resultsDir

cp $paramFile $resultsDir

# run tests
for seed in `seq 5`; do
    echo timeout 10 ./simulator -r $seed -q 1 -f ${tempResultsDir}/run_$seed $netConfig $paramFile \> ${resultsDir}/output_run_$seed
    timeout 10 ./simulator -r $seed -q 1 -f ${tempResultsDir}/run_$seed $netConfig $paramFile > ${resultsDir}/output_run_$seed
    if [ $? -eq 124 ]; then
        echo test timedout
    fi
done

# combine data
python combineData.py $tempResultsDir ${resultsDir}/results.csv

# plot data
python plotRuns.py ${resultsDir}/results.csv ${resultsDir}

# clean up
rm -r $tempResultsDir
