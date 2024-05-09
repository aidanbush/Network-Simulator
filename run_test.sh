#!/bin/bash
shopt -s expand_aliases
source ~/.bashrc

procLimit=1
timer=200
numRuns=5
offset=0
plotFormat=pdf

while getopts "o:n:f:p:t:" c; do
    case $c in
        o)
            # seed offset
            offset=${OPTARG}
            ;;
        n)
            # number of tests to run
            numRuns=${OPTARG}
            ;;
        f)
            # plot format (eg png or pdf)
            plotFormat=${OPTARG}
            ;;
        p)
            # limit of concurent processes
            procLimit=${OPTARG}
            ;;
        t)
            # real time limit per run
            timer=${OPTARG}
            ;;
    esac
done


# shift arguments so remaining arguments start at 1
shift $(($OPTIND - 1))

if [ $# -ne 3 ] ; then
  echo incorrect number of parameters passed
  exit
fi

netConfig=$1
testName=$2
paramFile=$3

tempResultsDir=$(mktemp -d)
echo saving temperary files to $tempResultsDir

resultsDir=results/${testName}

if [ -d $resultsDir ] ; then
    rm -r $resultsDir
fi
mkdir $resultsDir
chmod 777 $resultsDir

cp $paramFile $resultsDir

# run tests
for run in `seq 0 $(($numRuns - 1))`; do
    seed=$(($run + $offset))

    # ensure only create new processes when less than procLimit exist
    while [ `ps -u $(whoami) -o comm | grep simulator | wc -l` -ge $procLimit ] ; do
        sleep 1
    done

    runConfig=${netConfig}/run_${run}

    echo time timeout $timer ./build/simulator -r $seed -q 0100 -d ${tempResultsDir} -f run_$seed $runConfig $paramFile \> ${resultsDir}/output_run_$seed
    (
    time timeout $timer ./build/simulator -r $seed -q 0100 -d ${tempResultsDir} -f run_$seed $runConfig $paramFile > ${resultsDir}/output_run_$seed
    if [ $? -eq 124 ]; then
        echo test $seed timed out
    fi
    ) &
done

wait

# zip into resultsDir
# cd to tempResultsDir -> zip file -> jump back -> copy over
(
    cd ${tempResultsDir}
    zip raw_data.zip *
    cd -
    mkdir -p ${resultsDir}/raw_data
    mv ${tempResultsDir}/raw_data.zip ${resultsDir}/raw_data/
)

# combine data
echo python3 combine_data.py $tempResultsDir ${resultsDir} results.csv links.csv switches.csv
time python3 combine_data.py $tempResultsDir ${resultsDir} results.csv links.csv switches.csv &> ${resultsDir}/combine_data_output

# plot data
#echo python3 plotRuns.py ${resultsDir}/results.csv -d ${resultsDir} -f ${plotFormat} -t $testName
#time python3 plotRuns.py ${resultsDir}/results.csv -d ${resultsDir} -f ${plotFormat} -t $testName

# clean up
rm -r $tempResultsDir
