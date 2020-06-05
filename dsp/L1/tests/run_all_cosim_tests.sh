#!/usr/bin/bash
################## Get the list of all tests #############
# source jenkins_autorun_csim.sh , to verify functionality
source run_all_csim_tests.sh
echo "----------------CSIM-INFO-----------------"
source check_csim_status.sh
echo "Doe with CSIM func tests lanching synthesis"
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

echo "Remove all old test run logs ..."
for tdir in $all_tests_dirs
do
    cd $tdir
    rm -rf vitis_hls.log
done
echo "Cleaned all old run logs ..."


for tdir in $all_tests_dirs
do
    echo "-------------------------------------------"
    echo "Entering Test Directory : $tdir"
    echo "-------------------------------------------"
    cd $tdir
    rm -rf vitis_hls.log
    #vitis_hls -f $script_name
    make run CSIM=1 CSYNTH=1 COSIM=1 XPART={xcu200-fsgd2104-2-e}
    echo "-------------------------------------------"
    echo "Finished test : $tdir/$script_name"
    echo "-------------------------------------------"
done

echo "Collect all test stats ..."
total_test=0;
passed_test=0;
failed_test=0;
failed_list=();
for tdir in $all_tests_dirs
do
    total_test=$((total_test+1))
    cd $tdir 
    res=$(grep -i "fail" vitis_hls.log)
    echo "--------------------------------------------"
    echo "Test : $tdir"
    if [ -z "$res" ]
    then
        echo "Test Passed ..."
        passed_test=$((passed_test+1))
    else
        echo "Test Failed ..."
        failed_test=$((failed_test+1))
        failed_list=(${failed_list[@]} $tdir)
    fi
    echo "--------------------------------------------"
done
echo "======================================================"
echo "======================================================"
echo "-----------------Test Result Summary------------------"
echo "Total Number of tests: $total_test"
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
source check_csim_status.sh
source check_csynth_status.sh
source check_cosim_status.sh
