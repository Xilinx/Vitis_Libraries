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
    echo "-------------------------------------------"
    cd $tdir
    #rm -rf vitis_hls.log
    #vitis_hls -f $script_name
    echo "-------------------------------------------"
    echo "Finished test : $tdir/$script_name"
    echo "-------------------------------------------"
done
total_test=0;
passed_test=0;
failed_test=0;
failed_list=();
test_finished=0;
for tdir in $all_tests_dirs
do
    total_test=$((total_test+1))
    #echo "-------------------------------------------"
    #echo "Entering Test Directory : $tdir"
    #echo "-------------------------------------------"
    cd $tdir
    if [ -f "vitis_hls.log" ]; then
        test_finished=$((test_finished+1))
        #res=$(grep -i "fail" vitis_hls.log)
        res=$(grep -i "C/RTL co-simulation finished: PASS" vitis_hls.log)
        echo "--------------------------------------------"
        echo "Test : $tdir"
        if [ -z "$res" ]
        then
            echo "Test Failed ..."
            failed_test=$((failed_test+1))
            failed_list=(${failed_list[@]} $tdir)
        else
            echo "Test Passed ..."
            passed_test=$((passed_test+1))

        fi
    fi    
    echo "--------------------------------------------"
    #echo "-------------------------------------------"
    #echo "Finished test : $tdir/$script_name"
    #echo "-------------------------------------------"
done
echo "======================================================"
echo "======================================================"
echo "-----------------Test Result Summary------------------"
echo "Total Number of tests: $total_test"
echo "Number of test finished: $test_finished"
echo "Number of tests PASSED: $passed_test"
echo "Number of tests FAILED: $failed_test"
echo "  "
echo "The list of failed tests:"
for ftest in ${failed_list[@]}
do
    echo "$ftest"
done
echo "======================================================"
echo "======================================================"


cd $launch_dir

