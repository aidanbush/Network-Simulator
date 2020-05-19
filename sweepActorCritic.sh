#!/usr/local/bin/bash
shopt -s expand_aliases
source ~/.bashrc
if [ ! -d results ]
then
    mkdir results
fi
rm -f results/sweepActorCriticOutput.csv
rm -f results/failedParams
COUNT=0
TOTAL=$(( 9*9*9*6*2*1 ))
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
                            let COUNT++
                            echo "Running for input" "$ALPHA_U $ALPHA_V $ALPHA_R $TAU $INAC $S $INITIAL_WEIGHTS"\
                                 "("$COUNT of $TOTAL")"
                            echo 7 > params
                            echo $ALPHA_U $ALPHA_V $ALPHA_R $TAU $INAC $S $INITIAL_WEIGHTS >> params
                            gtimeout 2s ./simulator -q local.json params >> results/sweepActorCriticOutput.csv
                            if [ $? -ne 0 ]
                            then
                                echo $ALPHA_U $ALPHA_V $ALPHA_R $TAU $INAC $S $INITIAL_WEIGHTS >> results/failedParams
                            fi
                        done
                    done
                done
            done
        done
    done
done
rm params
