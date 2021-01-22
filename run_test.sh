shopt -s expand_aliases
source ~/.bashrc

procLimit=1
timer=200
numTests=5
offset=0
plotFormat=pdf

while getopts "o:n:f:p:t:" c; do
    case $c in
        o)
            offset=${OPTARG}
            ;;
        n)
            numTests=${OPTARG}
            ;;
        f)
            plotFormat=${OPTARG}
            ;;
        p)
            procLimit=${OPTARG}
            ;;
        t)
            timer=${OPTARG}
            ;;
    esac
done

# shift arguments so remaining arguments start at 1
shift $(($OPTIND - 1))

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

mkdir ${resultsDir}/weights

cp $paramFile $resultsDir

# run tests
for seed in `seq $numTests`; do
    seed=$(($seed + $offset))

    # ensure only create new processes when less than procLimit exist
    while [ `ps -u $(whoami) -o comm | grep simulator | wc -l` -ge $procLimit ] ; do
        sleep 1
    done

    echo time timeout $timer ./simulator -r $seed -q 0100 -d ${tempResultsDir} -f run_$seed $netConfig $paramFile \> ${resultsDir}/output_run_$seed
    (
    time timeout $timer ./simulator -r $seed -q 0100 -d ${tempResultsDir} -f run_$seed $netConfig $paramFile > ${resultsDir}/output_run_$seed
    if [ $? -eq 124 ]; then
        echo test $seed timed out
    fi
    ) &
done

wait

# combine data
echo python combineData.py $tempResultsDir ${resultsDir}/results.csv
time python combineData.py $tempResultsDir ${resultsDir}/results.csv

# copy weights
echo cp ${tempResultsDir}/*_Weights.csv ${resultsDir}/weights/
cp ${tempResultsDir}/*_Weights.csv ${resultsDir}/weights/

# plot data
echo python plotRuns.py ${resultsDir}/results.csv -d ${resultsDir} -f ${plotFormat} -t $testName
time python plotRuns.py ${resultsDir}/results.csv -d ${resultsDir} -f ${plotFormat} -t $testName

# clean up
rm -r $tempResultsDir
