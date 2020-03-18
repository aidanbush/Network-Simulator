#!/usr/local/bin/bash
shopt -s expand_aliases
source ~/.bashrc
if [ ! -d results ]
then
    mkdir results
fi
rm -f results/sweepOutput.csv
rm -f results/failedParams
COUNT=0
TOTAL=$(( 10*8*10*8*1 ))
for ALPHA in 0.1 0.2 0.3 0.4 0.5 0.6 0.7 0.8 0.9 1
do
    for LAMBDA in 1 0.99 0.975 0.95 0.9 0.8 0.4 0
    do
        for GAMMA in 0.1 0.2 0.3 0.4 0.5 0.6 0.7 0.8 0.9 1
        do
            for EPSILON in 0 0.0001 0.0005 0.001 0.005 0.01 0.05 0.1
            do
                for INITIAL_WEIGHTS in 0.1
                do
                    let COUNT++
                    echo "Running for input" $ALPHA $LAMBDA $GAMMA $EPSILON $INITIAL_WEIGHTS "("$COUNT of $TOTAL")"
                    echo $ALPHA $LAMBDA $GAMMA $EPSILON $INITIAL_WEIGHTS > params
                    gtimeout 2s ./simulator -q local.json params >> results/sweepOutput.csv
                    if [ $? -ne 0 ]
                    then
                        echo $ALPHA $LAMBDA $GAMMA $EPSILON $INITIAL_WEIGHTS >> results/failedParams
                    fi
                done
            done
        done
    done
done
rm params
