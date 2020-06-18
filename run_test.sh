#!/bin/bash

procLimit=6
procLimit=$(($procLimit))

timer=200
numTests=5

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
for seed in `seq $numTests`; do
    # ensure only create new processes when less than procLimit exist
    while [ `ps -u $(whoami) | grep simulator | wc -l` -gt $procLimit ] ; do
        sleep 1
    done

    echo time timeout $timer ./simulator -r $seed -q 0100 -f ${tempResultsDir}/run_$seed $netConfig $paramFile \> ${resultsDir}/output_run_$seed
    (
    time timeout $timer ./simulator -r $seed -q 0100 -f ${tempResultsDir}/run_$seed $netConfig $paramFile > ${resultsDir}/output_run_$seed
    if [ $? -eq 124 ]; then
        echo test $seed timedout
    fi
    ) &
done

wait

# combine data
python combineData.py $tempResultsDir ${resultsDir}/results.csv

# plot data
python plotRuns.py ${resultsDir}/results.csv ${resultsDir}

# clean up
rm -r $tempResultsDir
