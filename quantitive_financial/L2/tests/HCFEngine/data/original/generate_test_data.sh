#!/bin/bash

for f in tc_*.txt
do
    filename=$(basename -- "$f")
    filename="${filename%.*}"

    TC=$f
    QL=$(echo $filename | sed "s/tc/ql/").csv
    OP=$(echo $filename | sed "s/tc/test_data/").txt

    # Use this line for full grid
    ./generate_test_data $TC $QL $OP

    # Use this line to create a sub grid -s<min_s0 (default 0)> -S<max_s0 (default 800)> -v<min_v0 (default 0)> -V<max_v0 (default 5)>
    #./generate_test_data -s20 -S200 -v0.1 -V2 $TC $QL $OP
done

