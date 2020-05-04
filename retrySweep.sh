#!/usr/local/bin/bash
shopt -s expand_aliases
source ~/.bashrc
if [ ! -d results ]
then
    mkdir results
fi
rm -f results/sweepOutputRetry.csv
rm -f results/failedParamsRetry
COUNT=0
TOTAL=`wc -l < failedParams`
cat failedParams | while read line || [[ -n $line ]]
do
    let COUNT++
    echo $line > params
    echo "Running for input" $line "("$COUNT of $TOTAL")"
    gtimeout 10s ./simulator -q local.json params >> results/sweepOutputRetry.csv
    if [ $? -ne 0 ]
    then
        echo $line >> results/failedParamsRetry
    fi
done
rm params
