#!/usr/bin/bash
################## Get the list of all tests ###########
script_name="run_hls.tcl"
launch_dir=$(pwd)
echo "=================================================="
echo "Collecting info for launching tests"
echo "=================================================="

all_tests=$(find $(pwd) -type f -name "run_hls.tcl")
all_tests_dirs=$(dirname $all_tests)
no_of_tests=$(echo $all_tests | wc -w)
echo "=================================================="
echo "Summary of tests found:"
echo "Total number of tests found : $no_of_tests"
echo "List of tests found:"
for test in $all_tests
do
    echo $test
done

echo "=================================================="
echo "Setting up test environment ..."
source set_env.sh
echo "Vitis_hls used :"
which vitis_hls
echo "=================================================="

for tdir in $all_tests_dirs
do
    echo "-------------------------------------------"
    echo "Entering Test Directory : $tdir"
    cd $tdir
    vitis_hls -f $script_name
    echo "Finished test : $tdir/$script_name"
    echo "-------------------------------------------"
done
cd $launch_dir

