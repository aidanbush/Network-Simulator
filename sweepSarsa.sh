#!/usr/local/bin/bash
shopt -s expand_aliases
source ~/.bashrc

RESULTS_DIR=results
NUM_PARAMS=5
COUNT=0
TOTAL=$(( 10*8*10*8*1 ))

if [ ! -d $RESULTS_DIR ]
then
    mkdir $RESULTS_DIR
fi
rm -f $RESULTS_DIR/sweepSarsaOutput.csv
rm -f $RESULTS_DIR/failedParams
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
                    echo 5 > params
                    echo $ALPHA $LAMBDA $GAMMA $EPSILON $INITIAL_WEIGHTS >> params
                    gtimeout 2s ./simulator -q local.json params >> $RESULTS_DIR/sweepSarsaOutput.csv
                    if [ $? -ne 0 ]
                    then
                        echo $ALPHA $LAMBDA $GAMMA $EPSILON $INITIAL_WEIGHTS >> $RESULTS_DIR/failedParams
                    fi
                done
            done
        done
    done
done
rm params
sort -gr --field-separator=',' --key=$(( $NUM_PARAMS+1 )) -o $RESULTS_DIR/sweepSarsaOutput.csv $RESULTS_DIR/sweepSarsaOutput.csv 
echo -n $'\a'
