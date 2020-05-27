#!/usr/local/bin/bash
shopt -s expand_aliases
source ~/.bashrc

RESULTS_DIR=results
NUM_PARAMS=7
COUNT=0
TOTAL=$(( 9*9*9*6*2*1*1*100 ))
FAILED=0

if [ ! -d $RESULTS_DIR ]
then
    mkdir $RESULTS_DIR
fi
rm -f $RESULTS_DIR/sweepActorCriticOutput.csv
rm -f $RESULTS_DIR/failedParams
rm -f $RESULTS_DIR/tempOutput
for ALPHA_U in 0.0001 0.0005 0.001 0.005 0.01 0.05 0.1 0.5 1
do
    for ALPHA_V in 0.0001 0.0005 0.001 0.005 0.01 0.05 0.1 0.5 1
    do
        for ALPHA_R in 0.0001 0.0005 0.001 0.005 0.01 0.05 0.1 0.5 1
        do
            for TAU in 1 2 4 8 16 32
            do
                for INAC in 0 1
                do
                    for S in 0 #1 #Only include 1 if Mode is set to Add or Mult (in actorCritic.cpp)
                    do
                        for INITIAL_WEIGHTS in 0.1
                        do
                            for SEED in {1..100}
                            do
                                let COUNT++
                                echo "Running for input" "$ALPHA_U $ALPHA_V $ALPHA_R $TAU $INAC $S $INITIAL_WEIGHTS"\
                                     "with seed" $SEED "("$COUNT of $TOTAL")"
                                echo $NUM_PARAMS > params
                                echo $ALPHA_U $ALPHA_V $ALPHA_R $TAU $INAC $S $INITIAL_WEIGHTS >> params
                                gtimeout 2s ./simulator -r $SEED -q local.json params >> $RESULTS_DIR/tempOutput
                                if [ $? -ne 0 ]
                                then
                                    echo $ALPHA_U $ALPHA_V $ALPHA_R $TAU $INAC $S $INITIAL_WEIGHTS >> \
                                        $RESULTS_DIR/failedParams
                                    FAILED=1
                                    break
                                fi
                            done
                            if [ $FAILED -eq 0 ]
                            then
                                echo $ALPHA_U,$ALPHA_V,$ALPHA_R,$TAU,$INAC,$S,$INITIAL_WEIGHTS,\
                                    $(awk '{ total += $1; count++ } END { print total/count }' \
                                        $RESULTS_DIR/tempOutput) >> $RESULTS_DIR/sweepActorCriticOutput.csv
                            else
                                FAILED=0
                            fi
                            rm -f $RESULTS_DIR/tempOutput
                        done
                    done
                done
            done
        done
    done
done
rm params
sort -gr --field-separator=',' --key=$(( $NUM_PARAMS+1 )) -o $RESULTS_DIR/sweepActorCriticOutput.csv \
    $RESULTS_DIR/sweepActorCriticOutput.csv 
echo -n $'\a'
