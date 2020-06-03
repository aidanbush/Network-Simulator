#!/usr/local/bin/bash
shopt -s expand_aliases
source ~/.bashrc

RESULTS_DIR=results
NUM_PARAMS=5
COUNT=0
TOTAL=$(( 10*8*10*8*1 ))
FAILED=0

if [ ! -d $RESULTS_DIR ]
then
    mkdir $RESULTS_DIR
fi
rm -f $RESULTS_DIR/sweepSarsaOutput.csv
rm -f $RESULTS_DIR/failedParams
rm -f $RESULTS_DIR/tempOutput
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
                    for SEED in {1..100}
                    do
                        let COUNT++
                        echo "Running for input" $ALPHA $LAMBDA $GAMMA $EPSILON $INITIAL_WEIGHTS \
                            "with seed" $SEED "("$COUNT of $TOTAL")"
                        echo 5 > params
                        echo $ALPHA $LAMBDA $GAMMA $EPSILON $INITIAL_WEIGHTS >> params
                        timeout 2s ./simulator -r $SEED -q 2 local.json params >> $RESULTS_DIR/tempOutput
                        if [ $? -ne 0 ]
                        then
                            echo $ALPHA $LAMBDA $GAMMA $EPSILON $INITIAL_WEIGHTS >> $RESULTS_DIR/failedParams
                            FAILED=1
                            break
                        fi
                    done
                    if [ $FAILED -eq 0 ]
                    then
                        echo $ALPHA,$LAMBDA,$GAMMA,$EPSILON,$INITIAL_WEIGHTS,\
                            $(awk '{ total += $1; count++ } END { print total/count }' $RESULTS_DIR/tempOutput) >> \
                            $RESULTS_DIR/sweepActorCriticOutput.csv
                    else
                        FAILED=0
                    fi
                    rm -f $RESULTS_DIR/tempOutput
                done
            done
        done
    done
done
rm params
sort -gr --field-separator=',' --key=$(( $NUM_PARAMS+1 )) -o $RESULTS_DIR/sweepSarsaOutput.csv \
    $RESULTS_DIR/sweepSarsaOutput.csv 
echo -n $'\a'
