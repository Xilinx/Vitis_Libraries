#!/usr/bin/bash
################## Get the list of all tests ###########
script_name="Makefile"
launch_dir=$(pwd)
echo "=================================================="
echo "Collecting info for Cleaning all test logs ... "
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
echo "Cleaning all logs and project folders ..."
echo "=================================================="
echo "Remove all old test run logs ..."
for tdir in $all_tests_dirs
do
    cd $tdir
    echo "--cleaning-log"
    echo "$tdir"
    make clean
done
echo "=================================================="
echo "Cleaned all old run logs ..."
echo "No. of logs folders found: $no_of_tests"
echo "=================================================="


 

cd $launch_dir

