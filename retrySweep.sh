shopt -s expand_aliases
source ~/.bashrc

RESULTS_DIR=results

if [ ! -d $RESULTS_DIR ]
then
    mkdir $RESULTS_DIR
fi

rm -f $RESULTS_DIR/sweepOutputRetry.csv
rm -f $RESULTS_DIR/failedParamsRetry
rm -f $RESULTS_DIR/tempOutput
COUNT=0
FAILED=0
TOTAL=$(( `wc -l < failedParams`*100 ))
cat $RESULTS_DIR/failedParams | while read line || [[ -n $line ]]
do
    echo $line > params
    for SEED in {1..100}
    do
        let COUNT++
        echo "Running for input" $line "with seed" $SEED "("$COUNT of $TOTAL")"
        timeout 10s ./simulator -r $SEED -q 2 local.json params >> $RESULTS_DIR/tempOutput
        if [ $? -ne 0 ]
        then
            echo $line >> $RESULTS_DIR/failedParamsRetry
            FAILED=1
            break
        fi
    done
    if [ $FAILED -eq 0 ]
    then
        echo $line,$(awk '{ total += $1; count++ } END { print total/count }' $RESULTS_DIR/tempOutput) >> \
            $RESULTS_DIR/sweepOutputRetry.csv
    else
        FAILED=0
    fi
    rm -f $RESULTS_DIR/tempOutput
done
rm params
