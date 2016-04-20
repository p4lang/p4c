#!/bin/bash

THIS_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

TESTS="test_parser_deparser_1 \
test_exact_match_1 \
test_LPM_match_1 \
test_ternary_match_1"

nruns=5
if [ $# -eq 1 ]; then
    nruns=$1
fi

median_idx=$(($nruns / 2))

do_runs() {
    local tname=$1
    local n=$2
    local pps_list=""
    local pps=""
    for i in `seq 1 $n`; do
        pps=$($THIS_DIR/$tname | awk '/Switch was processing/{ print $4 }')
        pps_list="$pps_list $pps"
    done
    echo $pps_list
}

for tname in $TESTS; do
    pps_list=$(do_runs $tname $nruns)
    pps_list=$(echo $pps_list | tr " " "\n" | sort -g | tr "\n" " ")
    pps_list=($pps_list)
    median_pps=${pps_list[$median_idx]}
    echo "$tname $median_pps"
done
