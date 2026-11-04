#!/bin/bash
#
# Copyright (C) 2019-2022, Xilinx, Inc.
# Copyright (C) 2022-2026, Advanced Micro Devices, Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
rm -rf runmake_logs
mkdir -p runmake_logs
# We may run the script from another directory. Recursive LSF call needs correct path to script. Can't assume cwd.
echo "Running runmake located at `readlink -f \"$0\"`" | tee ./runmake_logs/runmake.log
pathToRunmake=$(readlink -f "$0")
runmakeDir=$(dirname $pathToRunmake)
args=("$@")
echo "inputs args are: ${args[*]}" | tee -a ./runmake_logs/runmake.log
make_args=()
# specify the test suite to run with "runmake.sh -run_type qor>"
run_type="checkin"
run_tests=1
verify_tests=1

for i in "${!args[@]}"; do
    if [[ "${args[$i]}" == '-h' ]]
    then
            echo "This script contains a number of test suites such as checkin, qor etc. These suites contain a list of commands for a specific test (currently this should be in format: make all PARAM=PARAM_VALUE...)"
            echo "All the tests in the specified suite are converted into a multi_params.json file which can then be used to run tests"
            echo "Options:"
            echo "   -run_type checkin | qor | commslib | randomized ## Specifies which test suite to use)"
            echo "          (Use "-run_type jenkins" to create multi_params.json file containing PR, checkin, and qor tests."
            echo "   -make_arg <MAKE_ARG>=<VALUE> ## used to add an argument to all tests list in the suite. For example, "-add_make TARGET=x86sim""
            echo "   -no_test_run ## creates multi_params file but does not run tests."
            echo ""
            echo "Example Usage:"
            echo "  runmake.sh -run_type qor                                (creates multi_params_qor.json, and runs all qor tests)"
            echo "  runmake.sh -run_type qor -make_arg    (appends to all tests, creates multi_params_qor.json, and runs all qor tests)"
            echo "  runmake.sh -run_type qor -no_test_run                   (creates multi_params_qor.json, but does not run tests)"
            echo "  runmake.sh -run_type jenkins                            (creates multi_params.json. This will include all canary test, PR tests, checkin(daily), and qor)"

            exit 0
    fi
    if [[ "${args[$i]}" == '-run_type' ]]
    then
        run_type=${args[($i+1)]}
    fi
    if [[ "${args[$i]}" == '-make_arg' ]]
    then
        echo "Make args will be passed directly to any make commands. The following will be used: ${args[($i+1)]}"  | tee -a ./runmake_logs/runmake.log
        make_args[${#make_args[*]}]=${args[($i+1)]}
    fi
    # use this option if you do not want to run tests of the generated multi_params
    if [[ "${args[$i]}" == '-no_test_run' ]]
    then
        run_tests=0
    fi
    # use this option if you do not want to verify tests of the generated multi_params
    if [[ "${args[$i]}" == '-no_test_verify' ]]
    then
        verify_tests=0
    fi
done

if [[ "$*" == *checkin* ]]
then
Iterations=8
fi

if [[  "$*" == *qor* ]]
then
Iterations=16
fi

test_arr=()

# Checkin tests are used for the Jenkins pull request
if [[ "$*" == *checkin* || "$*" == *qor* ]]
then
    test_arr[${#test_arr[*]}]="echo 'Running checkin tests for Euclidean Distance library element.'"

    # AIE1
    # ED
    ## float*float
    ## ED as SQRT one (IS_OUTPUT_SQUARED = 0 )
    test_arr[$LINENO]="make all DATA=float LEN=32 DIM=2 API_IO=0 RND=0 SAT=0 IS_OUTPUT_SQUARED=0 AIE_VARIANT=1 NITER=$Iterations"
    test_arr[$LINENO]="make all DATA=float LEN=32 DIM=3 API_IO=0 RND=0 SAT=0 IS_OUTPUT_SQUARED=0 AIE_VARIANT=1 NITER=$Iterations"
    test_arr[$LINENO]="make all DATA=float LEN=32 DIM=4 API_IO=0 RND=0 SAT=0 IS_OUTPUT_SQUARED=0 AIE_VARIANT=1 NITER=$Iterations"
    test_arr[$LINENO]="make all DATA=float LEN=64 DIM=4 API_IO=0 RND=0 SAT=0 IS_OUTPUT_SQUARED=0 AIE_VARIANT=1 NITER=$Iterations"
    test_arr[$LINENO]="make all DATA=float LEN=128 DIM=4 API_IO=0 RND=0 SAT=0 IS_OUTPUT_SQUARED=0 AIE_VARIANT=1 NITER=$Iterations"
    test_arr[$LINENO]="make all DATA=float LEN=256 DIM=4 API_IO=0 RND=0 SAT=0 IS_OUTPUT_SQUARED=0 AIE_VARIANT=1 NITER=$Iterations"
    test_arr[$LINENO]="make all DATA=float LEN=512 DIM=4 API_IO=0 RND=0 SAT=0 IS_OUTPUT_SQUARED=0 AIE_VARIANT=1 NITER=$Iterations"
    #test_arr[$LINENO]="make all DATA=float LEN=1024 DIM=4 API_IO=0 RND=0 SAT=0 IS_OUTPUT_SQUARED=0 AIE_VARIANT=1 NITER=$Iterations"
    #test_arr[$LINENO]="make all DATA=float LEN=2048 DIM=4 API_IO=0 RND=0 SAT=0 IS_OUTPUT_SQUARED=0 AIE_VARIANT=1 NITER=$Iterations"
    
    ## ED as SQUARED one (IS_OUTPUT_SQUARED = 1)
    test_arr[$LINENO]="make all DATA=float LEN=32 DIM=2 API_IO=0 RND=0 SAT=0 AIE_VARIANT=1 NITER=$Iterations"
    test_arr[$LINENO]="make all DATA=float LEN=32 DIM=3 API_IO=0 RND=0 SAT=0 AIE_VARIANT=1 NITER=$Iterations"
    test_arr[$LINENO]="make all DATA=float LEN=32 DIM=4 API_IO=0 RND=0 SAT=0 AIE_VARIANT=1 NITER=$Iterations"
    test_arr[$LINENO]="make all DATA=float LEN=64 DIM=4 API_IO=0 RND=0 SAT=0 AIE_VARIANT=1 NITER=$Iterations"
    test_arr[$LINENO]="make all DATA=float LEN=128 DIM=4 API_IO=0 RND=0 SAT=0 AIE_VARIANT=1 NITER=$Iterations"
    test_arr[$LINENO]="make all DATA=float LEN=256 DIM=4 API_IO=0 RND=0 SAT=0 AIE_VARIANT=1 NITER=$Iterations"
    test_arr[$LINENO]="make all DATA=float LEN=256 DIM=4 API_IO=0 RND=0 SAT=0 AIE_VARIANT=1 NITER=$Iterations"
    test_arr[$LINENO]="make all DATA=float LEN=512 DIM=4 API_IO=0 RND=0 SAT=0 AIE_VARIANT=1 NITER=$Iterations"
    #test_arr[$LINENO]="make all DATA=float LEN=1024 DIM=4 API_IO=0 RND=0 SAT=0 AIE_VARIANT=1 NITER=$Iterations"
    #test_arr[$LINENO]="make all DATA=float LEN=2048 DIM=4 API_IO=0 RND=0 SAT=0 AIE_VARIANT=1 NITER=$Iterations"
    
    ##AIE-ML
    # ED
    ## float*float
    ## ED as SQRT one (IS_OUTPUT_SQUARED = 0)
    test_arr[$LINENO]="make all DATA=float LEN=32 DIM=2 API_IO=0 RND=0 SAT=0 IS_OUTPUT_SQUARED=0 AIE_VARIANT=2 NITER=$Iterations"
    test_arr[$LINENO]="make all DATA=float LEN=32 DIM=3 API_IO=0 RND=0 SAT=0 IS_OUTPUT_SQUARED=0 AIE_VARIANT=2 NITER=$Iterations"
    test_arr[$LINENO]="make all DATA=float LEN=32 DIM=4 API_IO=0 RND=0 SAT=0 IS_OUTPUT_SQUARED=0 AIE_VARIANT=2 NITER=$Iterations"
    test_arr[$LINENO]="make all DATA=float LEN=64 DIM=4 API_IO=0 RND=0 SAT=0 IS_OUTPUT_SQUARED=0 AIE_VARIANT=2 NITER=$Iterations"
    test_arr[$LINENO]="make all DATA=float LEN=128 DIM=4 API_IO=0 RND=0 SAT=0 IS_OUTPUT_SQUARED=0 AIE_VARIANT=2 NITER=$Iterations"
    test_arr[$LINENO]="make all DATA=float LEN=256 DIM=4 API_IO=0 RND=0 SAT=0 IS_OUTPUT_SQUARED=0 AIE_VARIANT=2 NITER=$Iterations"
    test_arr[$LINENO]="make all DATA=float LEN=256 DIM=4 API_IO=0 RND=0 SAT=0 IS_OUTPUT_SQUARED=0 AIE_VARIANT=2 NITER=$Iterations"
    test_arr[$LINENO]="make all DATA=float LEN=512 DIM=4 API_IO=0 RND=0 SAT=0 IS_OUTPUT_SQUARED=0 AIE_VARIANT=2 NITER=$Iterations"
    test_arr[$LINENO]="make all DATA=float LEN=1024 DIM=4 API_IO=0 RND=0 SAT=0 IS_OUTPUT_SQUARED=0 AIE_VARIANT=2 NITER=$Iterations"
    #test_arr[$LINENO]="make all DATA=float LEN=2048 DIM=4 API_IO=0 RND=0 SAT=0 IS_OUTPUT_SQUARED=0 AIE_VARIANT=2 NITER=$Iterations"
    
    ## ED as SQUARED one (IS_OUTPUT_SQUARED = 1)
    test_arr[$LINENO]="make all DATA=float LEN=32 DIM=2 API_IO=0 RND=0 SAT=0 AIE_VARIANT=2 NITER=$Iterations"
    test_arr[$LINENO]="make all DATA=float LEN=32 DIM=3 API_IO=0 RND=0 SAT=0 AIE_VARIANT=2 NITER=$Iterations"
    test_arr[$LINENO]="make all DATA=float LEN=32 DIM=4 API_IO=0 RND=0 SAT=0 AIE_VARIANT=2 NITER=$Iterations"
    test_arr[$LINENO]="make all DATA=float LEN=64 DIM=4 API_IO=0 RND=0 SAT=0 AIE_VARIANT=2 NITER=$Iterations"
    test_arr[$LINENO]="make all DATA=float LEN=128 DIM=4 API_IO=0 RND=0 SAT=0 AIE_VARIANT=2 NITER=$Iterations"
    test_arr[$LINENO]="make all DATA=float LEN=256 DIM=4 API_IO=0 RND=0 SAT=0 AIE_VARIANT=2 NITER=$Iterations"
    test_arr[$LINENO]="make all DATA=float LEN=256 DIM=4 API_IO=0 RND=0 SAT=0 AIE_VARIANT=2 NITER=$Iterations"
    test_arr[$LINENO]="make all DATA=float LEN=512 DIM=4 API_IO=0 RND=0 SAT=0 AIE_VARIANT=2 NITER=$Iterations"
    test_arr[$LINENO]="make all DATA=float LEN=1024 DIM=4 API_IO=0 RND=0 SAT=0 AIE_VARIANT=2 NITER=$Iterations"
    #test_arr[$LINENO]="make all DATA=float LEN=2048 DIM=4 API_IO=0 RND=0 SAT=0 AIE_VARIANT=2 NITER=$Iterations"
    

    ## bfloat16*bfloat16
    ## ED as SQRT one (IS_OUTPUT_SQUARED = 0)
    test_arr[$LINENO]="make all DATA=bfloat16 LEN=32 DIM=2 API_IO=0 RND=0 SAT=0 IS_OUTPUT_SQUARED=0 AIE_VARIANT=2 NITER=$Iterations"
    test_arr[$LINENO]="make all DATA=bfloat16 LEN=32 DIM=3 API_IO=0 RND=0 SAT=0 IS_OUTPUT_SQUARED=0 AIE_VARIANT=2 NITER=$Iterations"
    test_arr[$LINENO]="make all DATA=bfloat16 LEN=32 DIM=4 API_IO=0 RND=0 SAT=0 IS_OUTPUT_SQUARED=0 AIE_VARIANT=2 NITER=$Iterations"
    test_arr[$LINENO]="make all DATA=bfloat16 LEN=64 DIM=4 API_IO=0 RND=0 SAT=0 IS_OUTPUT_SQUARED=0 AIE_VARIANT=2 NITER=$Iterations"
    test_arr[$LINENO]="make all DATA=bfloat16 LEN=128 DIM=4 API_IO=0 RND=0 SAT=0 IS_OUTPUT_SQUARED=0 AIE_VARIANT=2 NITER=$Iterations"
    test_arr[$LINENO]="make all DATA=bfloat16 LEN=256 DIM=4 API_IO=0 RND=0 SAT=0 IS_OUTPUT_SQUARED=0 AIE_VARIANT=2 NITER=$Iterations"
    test_arr[$LINENO]="make all DATA=bfloat16 LEN=256 DIM=4 API_IO=0 RND=0 SAT=0 IS_OUTPUT_SQUARED=0 AIE_VARIANT=2 NITER=$Iterations"
    test_arr[$LINENO]="make all DATA=bfloat16 LEN=512 DIM=4 API_IO=0 RND=0 SAT=0 IS_OUTPUT_SQUARED=0 AIE_VARIANT=2 NITER=$Iterations"
    test_arr[$LINENO]="make all DATA=bfloat16 LEN=1024 DIM=4 API_IO=0 RND=0 SAT=0 IS_OUTPUT_SQUARED=0 AIE_VARIANT=2 NITER=$Iterations"
    test_arr[$LINENO]="make all DATA=bfloat16 LEN=2048 DIM=4 API_IO=0 RND=0 SAT=0 IS_OUTPUT_SQUARED=0 AIE_VARIANT=2 NITER=$Iterations"
    
    ## ED as SQUARED one (IS_OUTPUT_SQUARED = 1)
    test_arr[$LINENO]="make all DATA=bfloat16 LEN=32 DIM=2 API_IO=0 RND=0 SAT=0 AIE_VARIANT=2 NITER=$Iterations"
    test_arr[$LINENO]="make all DATA=bfloat16 LEN=32 DIM=3 API_IO=0 RND=0 SAT=0 AIE_VARIANT=2 NITER=$Iterations"
    test_arr[$LINENO]="make all DATA=bfloat16 LEN=32 DIM=4 API_IO=0 RND=0 SAT=0 AIE_VARIANT=2 NITER=$Iterations"
    test_arr[$LINENO]="make all DATA=bfloat16 LEN=64 DIM=4 API_IO=0 RND=0 SAT=0 AIE_VARIANT=2 NITER=$Iterations"
    test_arr[$LINENO]="make all DATA=bfloat16 LEN=128 DIM=4 API_IO=0 RND=0 SAT=0 AIE_VARIANT=2 NITER=$Iterations"
    test_arr[$LINENO]="make all DATA=bfloat16 LEN=256 DIM=4 API_IO=0 RND=0 SAT=0 AIE_VARIANT=2 NITER=$Iterations"
    test_arr[$LINENO]="make all DATA=bfloat16 LEN=256 DIM=4 API_IO=0 RND=0 SAT=0 AIE_VARIANT=2 NITER=$Iterations"
    test_arr[$LINENO]="make all DATA=bfloat16 LEN=512 DIM=4 API_IO=0 RND=0 SAT=0 AIE_VARIANT=2 NITER=$Iterations"
    test_arr[$LINENO]="make all DATA=bfloat16 LEN=1024 DIM=4 API_IO=0 RND=0 SAT=0 AIE_VARIANT=2 NITER=$Iterations"
    test_arr[$LINENO]="make all DATA=bfloat16 LEN=2048 DIM=4 API_IO=0 RND=0 SAT=0 AIE_VARIANT=2 NITER=$Iterations"

     ##AIE-MLv2
    # ED
    ## float*float
    ## ED as SQRT one (IS_OUTPUT_SQUARED = 0)
    test_arr[$LINENO]="make all DATA=float LEN=32 DIM=2 API_IO=0 RND=0 SAT=0 IS_OUTPUT_SQUARED=0 AIE_VARIANT=22 NITER=$Iterations"
    test_arr[$LINENO]="make all DATA=float LEN=32 DIM=3 API_IO=0 RND=0 SAT=0 IS_OUTPUT_SQUARED=0 AIE_VARIANT=22 NITER=$Iterations"
    test_arr[$LINENO]="make all DATA=float LEN=32 DIM=4 API_IO=0 RND=0 SAT=0 IS_OUTPUT_SQUARED=0 AIE_VARIANT=22 NITER=$Iterations"
    test_arr[$LINENO]="make all DATA=float LEN=64 DIM=4 API_IO=0 RND=0 SAT=0 IS_OUTPUT_SQUARED=0 AIE_VARIANT=22 NITER=$Iterations"
    test_arr[$LINENO]="make all DATA=float LEN=128 DIM=4 API_IO=0 RND=0 SAT=0 IS_OUTPUT_SQUARED=0 AIE_VARIANT=22 NITER=$Iterations"
    test_arr[$LINENO]="make all DATA=float LEN=256 DIM=4 API_IO=0 RND=0 SAT=0 IS_OUTPUT_SQUARED=0 AIE_VARIANT=22 NITER=$Iterations"
    test_arr[$LINENO]="make all DATA=float LEN=256 DIM=4 API_IO=0 RND=0 SAT=0 IS_OUTPUT_SQUARED=0 AIE_VARIANT=22 NITER=$Iterations"
    test_arr[$LINENO]="make all DATA=float LEN=512 DIM=4 API_IO=0 RND=0 SAT=0 IS_OUTPUT_SQUARED=0 AIE_VARIANT=22 NITER=$Iterations"
    test_arr[$LINENO]="make all DATA=float LEN=1024 DIM=4 API_IO=0 RND=0 SAT=0 IS_OUTPUT_SQUARED=0 AIE_VARIANT=22 NITER=$Iterations"
    #test_arr[$LINENO]="make all DATA=float LEN=2048 DIM=4 API_IO=0 RND=0 SAT=0 IS_OUTPUT_SQUARED=0 AIE_VARIANT=22 NITER=$Iterations"
    
    ## ED as SQUARED one (IS_OUTPUT_SQUARED = 1)
    test_arr[$LINENO]="make all DATA=float LEN=32 DIM=2 API_IO=0 RND=0 SAT=0 AIE_VARIANT=22 NITER=$Iterations"
    test_arr[$LINENO]="make all DATA=float LEN=32 DIM=3 API_IO=0 RND=0 SAT=0 AIE_VARIANT=22 NITER=$Iterations"
    test_arr[$LINENO]="make all DATA=float LEN=32 DIM=4 API_IO=0 RND=0 SAT=0 AIE_VARIANT=22 NITER=$Iterations"
    test_arr[$LINENO]="make all DATA=float LEN=64 DIM=4 API_IO=0 RND=0 SAT=0 AIE_VARIANT=22 NITER=$Iterations"
    test_arr[$LINENO]="make all DATA=float LEN=128 DIM=4 API_IO=0 RND=0 SAT=0 AIE_VARIANT=22 NITER=$Iterations"
    test_arr[$LINENO]="make all DATA=float LEN=256 DIM=4 API_IO=0 RND=0 SAT=0 AIE_VARIANT=22 NITER=$Iterations"
    test_arr[$LINENO]="make all DATA=float LEN=256 DIM=4 API_IO=0 RND=0 SAT=0 AIE_VARIANT=22 NITER=$Iterations"
    test_arr[$LINENO]="make all DATA=float LEN=512 DIM=4 API_IO=0 RND=0 SAT=0 AIE_VARIANT=22 NITER=$Iterations"
    test_arr[$LINENO]="make all DATA=float LEN=1024 DIM=4 API_IO=0 RND=0 SAT=0 AIE_VARIANT=22 NITER=$Iterations"
    #test_arr[$LINENO]="make all DATA=float LEN=2048 DIM=4 API_IO=0 RND=0 SAT=0 AIE_VARIANT=22 NITER=$Iterations"
    

    ## bfloat16*bfloat16
    ## ED as SQRT one (IS_OUTPUT_SQUARED = 0)
    test_arr[$LINENO]="make all DATA=bfloat16 LEN=32 DIM=2 API_IO=0 RND=0 SAT=0 IS_OUTPUT_SQUARED=0 AIE_VARIANT=22 NITER=$Iterations"
    test_arr[$LINENO]="make all DATA=bfloat16 LEN=32 DIM=3 API_IO=0 RND=0 SAT=0 IS_OUTPUT_SQUARED=0 AIE_VARIANT=22 NITER=$Iterations"
    test_arr[$LINENO]="make all DATA=bfloat16 LEN=32 DIM=4 API_IO=0 RND=0 SAT=0 IS_OUTPUT_SQUARED=0 AIE_VARIANT=22 NITER=$Iterations"
    test_arr[$LINENO]="make all DATA=bfloat16 LEN=64 DIM=4 API_IO=0 RND=0 SAT=0 IS_OUTPUT_SQUARED=0 AIE_VARIANT=22 NITER=$Iterations"
    test_arr[$LINENO]="make all DATA=bfloat16 LEN=128 DIM=4 API_IO=0 RND=0 SAT=0 IS_OUTPUT_SQUARED=0 AIE_VARIANT=22 NITER=$Iterations"
    test_arr[$LINENO]="make all DATA=bfloat16 LEN=256 DIM=4 API_IO=0 RND=0 SAT=0 IS_OUTPUT_SQUARED=0 AIE_VARIANT=22 NITER=$Iterations"
    test_arr[$LINENO]="make all DATA=bfloat16 LEN=256 DIM=4 API_IO=0 RND=0 SAT=0 IS_OUTPUT_SQUARED=0 AIE_VARIANT=22 NITER=$Iterations"
    test_arr[$LINENO]="make all DATA=bfloat16 LEN=512 DIM=4 API_IO=0 RND=0 SAT=0 IS_OUTPUT_SQUARED=0 AIE_VARIANT=22 NITER=$Iterations"
    test_arr[$LINENO]="make all DATA=bfloat16 LEN=1024 DIM=4 API_IO=0 RND=0 SAT=0 IS_OUTPUT_SQUARED=0 AIE_VARIANT=22 NITER=$Iterations"
    test_arr[$LINENO]="make all DATA=bfloat16 LEN=2048 DIM=4 API_IO=0 RND=0 SAT=0 IS_OUTPUT_SQUARED=0 AIE_VARIANT=22 NITER=$Iterations"
    
    ## ED as SQUARED one (IS_OUTPUT_SQUARED = 1)
    test_arr[$LINENO]="make all DATA=bfloat16 LEN=32 DIM=2 API_IO=0 RND=0 SAT=0 AIE_VARIANT=22 NITER=$Iterations"
    test_arr[$LINENO]="make all DATA=bfloat16 LEN=32 DIM=3 API_IO=0 RND=0 SAT=0 AIE_VARIANT=22 NITER=$Iterations"
    test_arr[$LINENO]="make all DATA=bfloat16 LEN=32 DIM=4 API_IO=0 RND=0 SAT=0 AIE_VARIANT=22 NITER=$Iterations"
    test_arr[$LINENO]="make all DATA=bfloat16 LEN=64 DIM=4 API_IO=0 RND=0 SAT=0 AIE_VARIANT=22 NITER=$Iterations"
    test_arr[$LINENO]="make all DATA=bfloat16 LEN=128 DIM=4 API_IO=0 RND=0 SAT=0 AIE_VARIANT=22 NITER=$Iterations"
    test_arr[$LINENO]="make all DATA=bfloat16 LEN=256 DIM=4 API_IO=0 RND=0 SAT=0 AIE_VARIANT=22 NITER=$Iterations"
    test_arr[$LINENO]="make all DATA=bfloat16 LEN=256 DIM=4 API_IO=0 RND=0 SAT=0 AIE_VARIANT=22 NITER=$Iterations"
    test_arr[$LINENO]="make all DATA=bfloat16 LEN=512 DIM=4 API_IO=0 RND=0 SAT=0 AIE_VARIANT=22 NITER=$Iterations"
    test_arr[$LINENO]="make all DATA=bfloat16 LEN=1024 DIM=4 API_IO=0 RND=0 SAT=0 AIE_VARIANT=22 NITER=$Iterations"
    test_arr[$LINENO]="make all DATA=bfloat16 LEN=2048 DIM=4 API_IO=0 RND=0 SAT=0 AIE_VARIANT=22 NITER=$Iterations"
    
fi

if [[ "$*" == *illegal* ]]
then
   # input data type checking
    test_arr[$LINENO]="make all DATA=float LEN=32 DIM=4 API_IO=0 RND=0 SAT=0 IS_OUTPUT_SQUARED=0 AIE_VARIANT=1  NITER=1"
    #test_arr[$LINENO]="make all DATA=bfloat16 DATA_Q=float  DATA_OUT=float LEN=32 DIM=4 API_IO=0 RND=0 SAT=0 IS_OUTPUT_SQUARED=0 AIE_VARIANT=1  NITER=1"
    #test_arr[$LINENO]="make all DATA=bfloat16 DATA_Q=bfloat16 DATA_OUT=float LEN=32 DIM=4 API_IO=0 RND=0 SAT=0 AIE_VARIANT=1  NITER=1"
    #test_arr[$LINENO]="make all DATA=float DATA_Q=bfloat16  DATA_OUT=float LEN=32 DIM=4 API_IO=0 RND=0 SAT=0 IS_OUTPUT_SQUARED=0 AIE_VARIANT=2  NITER=1"
    #test_arr[$LINENO]="make all DATA=float DATA_Q=bfloat16  DATA_OUT=float LEN=32 DIM=4 API_IO=0 RND=0 SAT=0 AIE_VARIANT=2  NITER=1"
    #test_arr[$LINENO]="make all DATA=bfloat16 DATA_Q=float  DATA_OUT=bfloat16 LEN=32 DIM=4 API_IO=0 RND=0 SAT=0 IS_OUTPUT_SQUARED=0 AIE_VARIANT=2  NITER=1"
    #test_arr[$LINENO]="make all DATA=bfloat16 DATA_Q=float  DATA_OUT=bfloat16 LEN=32 DIM=4 API_IO=0 RND=0 SAT=0 AIE_VARIANT=2  NITER=1"
    #
    ## output data type checking
    #test_arr[$LINENO]="make all DATA=float DATA_Q=float  DATA_OUT=bfloat16 LEN=32 DIM=4 API_IO=0 RND=0 SAT=0 IS_OUTPUT_SQUARED=0 AIE_VARIANT=1  NITER=1"
    #test_arr[$LINENO]="make all DATA=float DATA_Q=float  DATA_OUT=bfloat16 LEN=32 DIM=4 API_IO=0 RND=0 SAT=0 IS_OUTPUT_SQUARED=0 AIE_VARIANT=2  NITER=1"
    #test_arr[$LINENO]="make all DATA=bfloat16 DATA_Q=bfloat16  DATA_OUT=cfloat LEN=32 DIM=4 API_IO=0 RND=0 SAT=0 AIE_VARIANT=2  NITER=1"
#
    ## P Len and Q Len check
    #test_arr[$LINENO]="make all DATA=float DATA_Q=float  DATA_OUT=float LEN=32 LEN_Q=16 DIM=4 API_IO=0 RND=0 SAT=0 IS_OUTPUT_SQUARED=0 AIE_VARIANT=1  NITER=1"
    #test_arr[$LINENO]="make all DATA=float DATA_Q=float  DATA_OUT=float LEN=128 DIM=4 API_IO=0 RND=0 SAT=0 IS_OUTPUT_SQUARED=0 AIE_VARIANT=2  NITER=1"
    #test_arr[$LINENO]="make all DATA=bfloat16 DATA_Q=bfloat16  DATA_OUT=bfloat16 LEN=256 DIM=4 API_IO=0 RND=0 SAT=0 IS_OUTPUT_SQUARED=0 AIE_VARIANT=2  NITER=1"
#
    ## checking for Dimension of P and Q should be same or not 
    #test_arr[$LINENO]="make all DATA=float LEN=32 DIM=3 API_IO=0 RND=0 SAT=0 IS_OUTPUT_SQUARED=0 AIE_VARIANT=1  NITER=1"
    #test_arr[$LINENO]="make all DATA=float LEN=32 DIM=4 API_IO=0 RND=0 SAT=0 IS_OUTPUT_SQUARED=0 AIE_VARIANT=1  NITER=1"
    #test_arr[$LINENO]="make all DATA=float LEN=32 DIM=3 API_IO=0 RND=0 SAT=0 AIE_VARIANT=2  NITER=1"
    #test_arr[$LINENO]="make all DATA=float LEN=32 DIM=4 API_IO=0 RND=0 SAT=0 AIE_VARIANT=2  NITER=1"
    #test_arr[$LINENO]="make all DATA=bfloat16 DATA_Q=bfloat16  DATA_OUT=bfloat16 LEN=32 DIM=4 API_IO=0 RND=0 SAT=0 IS_OUTPUT_SQUARED=0 AIE_VARIANT=2  NITER=1"
    #test_arr[$LINENO]="make all DATA=bfloat16 DATA_Q=bfloat16  DATA_OUT=bfloat16 LEN=32 DIM=4 API_IO=0 RND=0 SAT=0 AIE_VARIANT=2  NITER=1"
#
    ## checking for Dimension of P and Q should be always less than 4
    #test_arr[$LINENO]="make all DATA=float LEN=32 DIM=5 DIM_Q=5 API_IO=0 RND=0 SAT=0 IS_OUTPUT_SQUARED=0 AIE_VARIANT=1  NITER=1"
    #test_arr[$LINENO]="make all DATA=float LEN=32 DIM=8 DIM_Q=8 API_IO=0 RND=0 SAT=0 AIE_VARIANT=1  NITER=1"
    #test_arr[$LINENO]="make all DATA=bfloat16 LEN=32 DIM=6 DIM_Q=6 API_IO=0 RND=0 SAT=0 IS_OUTPUT_SQUARED=0 AIE_VARIANT=2  NITER=1"
    #test_arr[$LINENO]="make all DATA=bfloat16 LEN=32 DIM=7 DIM_Q=7 API_IO=0 RND=0 SAT=0 AIE_VARIANT=2  NITER=1"
    #
    ## checking for IS_OUTPUT_SQUARED should be always either 0 or 1
    #test_arr[$LINENO]="make all DATA=float LEN=32 DIM=1 DIM_Q=1 API_IO=0 RND=0 SAT=0 IS_OUTPUT_SQUARED=2 AIE_VARIANT=1  NITER=1"
    #test_arr[$LINENO]="make all DATA=float LEN=32 DIM=2 API_IO=0 RND=0 SAT=0 IS_OUTPUT_SQUARED=3 AIE_VARIANT=1  NITER=1"
    #test_arr[$LINENO]="make all DATA=bfloat16 LEN=32 DIM=3 API_IO=0 RND=0 SAT=0 IS_OUTPUT_SQUARED=4 AIE_VARIANT=2  NITER=1"
    #test_arr[$LINENO]="make all DATA=bfloat16 LEN=32 DIM=4 API_IO=0 RND=0 SAT=0 IS_OUTPUT_SQUARED=5 AIE_VARIANT=2  NITER=1"

fi

# echo "Adding the following args to make commands: ${make_args[@]}"
# Ensure each test goes into a seperate runmake
for i in "${!test_arr[@]}"; do
    # append make args to each make command
    if [[ ${test_arr[$i]} == make* ]]
    then
        test_arr[$i]="${test_arr[$i]} ${make_args[@]}"
    fi
    test_arr[$i]="${test_arr[$i]} |& tee runmake_logs/runmake_${i}.log "
done

makeCmd_raw=()
otherCmd_raw=()

for i in "${!test_arr[@]}"; do
    # append make args to each make command
    if [[ ${test_arr[$i]} == make* ]]; then
        makeCmd_raw[${#makeCmd_raw[*]}]=${test_arr[$i]}
    else
        otherCmd_raw[${#otherCmd_raw[*]}]=${test_arr[$i]}
    fi
done
# for i in "${!makeCmd_raw[@]}"; do
#     echo ${makeCmd_raw[$i]}
# done

if [[ "$*" == *-append* ]]
then
    verify_tests=0
    new_test_names=$(python3 $runmakeDir/../common/scripts/populate_params.py $run_type $runmakeDir "${makeCmd_raw[@]}" -append)
else
    new_test_names=$(python3 $runmakeDir/../common/scripts/populate_params.py $run_type $runmakeDir "${makeCmd_raw[@]}")
fi
multi_params_file=multi_params_$run_type

if [[ $run_type == "jenkins" ]]
then
    python3 $runmakeDir/../common/scripts/populate_params.py clear_params $runmakeDir
    $runmakeDir/runmake.sh -run_type checkin -no_test_run
    exit_code1=$?  
    $runmakeDir/runmake.sh -run_type qor -no_test_run
    exit_code2=$?  
    if [[ $exit_code1 -eq 1 || $exit_code2 -eq 1 ]]; then  
        exit 1  
    fi  
    $runmakeDir/runmake.sh -run_type checkin -append -no_test_run -no_test_verify
    $runmakeDir/runmake.sh -run_type qor -append -no_test_run -no_test_verify
    multi_params_file=multi_params
    run_tests=0
    verify_tests=0
fi

if [[ $verify_tests == 1 ]]
then
    test_lines_string=${!test_arr[@]}
    echo Verifying $run_type test suite...
    python3 $runmakeDir/../common/scripts/verify_multi_params.py --ip $(basename $runmakeDir) --multi_params $runmakeDir/$multi_params_file.json --test_lines "$test_lines_string" --gen_test_file --test_suite $run_type
    exit_code=$?
    if [[ $exit_code -eq 1 ]]; then
        exit 1
    fi    
fi

if [[ $run_tests == 1 ]]
then
    timestamp=$(date +"%y%m%d_%H%M")
    echo "Start of Batch Run ($timestamp)" | tee ./runmake_logs/runmake.log
    $runmakeDir/../common/scripts/run_batch.sh -func $runmakeDir -params $multi_params_file -batch_suffix $timestamp
fi