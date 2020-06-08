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
        res=$(grep -i "CSim done with 0 errors" vitis_hls.log)
        echo "--------------------------------------------"
        echo "Test : $tdir"
        if [ -z "$res" ]
        then
            echo "C-Simulation Failed..."
            failed_test=$((failed_test+1))
            failed_list=(${failed_list[@]} $tdir) 
        else
	   echo "C-Simulation Passed ..."
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
echo "----------------- **CSIM ** Test Result Summary------------------"
echo "Total Number of tests: $total_test"
echo "Number of test finished: $test_finished"
echo "Number of tests PASSED CSIM: $passed_test"
echo "Number of tests FAILED CSIM: $failed_test"
echo "  "

## Check if there are any failures in tests
if [  $failed_test -gt 0 ]; then
	echo "The list of failed tests:"
	for ftest in ${failed_list[@]}
	do
	    echo "$ftest"
	done
## run and pass status 1 to jenkins
	echo "C-Simulation has FAILED !"
	exit 1 
fi

## Check if All tests were ran
## Check if All tests were ran
if [ $total_test -ne $passed_test ]; then
	echo "Some tests did not run CSIM , CSIM TEST RUN FAILURE !"
	exit 1 
fi
echo "======================================================"
echo "======================================================"


cd $launch_dir

