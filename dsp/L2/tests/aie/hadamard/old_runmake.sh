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
            echo "  runmake.sh -run_type qor -make_arg TARGET=x86sim    (appends TARGET=x86sim to all tests, creates multi_params_qor.json, and runs all qor tests)"
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

# empty array that will be filled up with testcases.
test_arr=()

if [[ "$*" == *smoke* ]]
then
    test_arr[$LINENO]="make all AIE_VARIANT=1 "
    test_arr[$LINENO]="make all AIE_VARIANT=2 "
    test_arr[$LINENO]="make all AIE_VARIANT=22 "
fi

if [[  "$*" == *rndsat* ]]
then
    test_arr[${#test_arr[*]}]="echo 'Running limited number of check-in tests.'"
    test_arr[$LINENO]="make all T_DATA_A=cint32 T_DATA_B=int32 DIM_SIZE=256   NUM_FRAMES=4  SHIFT=30 API_IO=0 UUT_SSR=1 ROUND_MODE=1  SAT_MODE=1 STIM_TYPE=0 AIE_VARIANT=1 TARGET=x86sim"
    test_arr[$LINENO]="make all T_DATA_A=cint16 T_DATA_B=int16 DIM_SIZE=16    NUM_FRAMES=4  SHIFT=14 API_IO=0 UUT_SSR=1 ROUND_MODE=2  SAT_MODE=3 STIM_TYPE=0 AIE_VARIANT=1 TARGET=x86sim"
    test_arr[$LINENO]="make all T_DATA_A=cint16 T_DATA_B=int16 DIM_SIZE=16    NUM_FRAMES=3  SHIFT=14 API_IO=0 UUT_SSR=1 ROUND_MODE=3  SAT_MODE=0 STIM_TYPE=0 AIE_VARIANT=1 TARGET=x86sim"
    test_arr[$LINENO]="make all T_DATA_A=cint16 T_DATA_B=int16 DIM_SIZE=16    NUM_FRAMES=5  SHIFT=14 API_IO=0 UUT_SSR=1 ROUND_MODE=4  SAT_MODE=1 STIM_TYPE=0 AIE_VARIANT=1 TARGET=x86sim"
    test_arr[$LINENO]="make all T_DATA_A=cint16 T_DATA_B=int16 DIM_SIZE=16    NUM_FRAMES=2  SHIFT=14 API_IO=0 UUT_SSR=1 ROUND_MODE=5  SAT_MODE=3 STIM_TYPE=0 AIE_VARIANT=1 TARGET=x86sim"
    test_arr[$LINENO]="make all T_DATA_A=cint16 T_DATA_B=int16 DIM_SIZE=16    NUM_FRAMES=8  SHIFT=14 API_IO=0 UUT_SSR=1 ROUND_MODE=6  SAT_MODE=0 STIM_TYPE=0 AIE_VARIANT=1 TARGET=x86sim"
    test_arr[$LINENO]="make all T_DATA_A=cint16 T_DATA_B=int16 DIM_SIZE=16    NUM_FRAMES=8  SHIFT=14 API_IO=0 UUT_SSR=1 ROUND_MODE=7  SAT_MODE=1 STIM_TYPE=0 AIE_VARIANT=1 TARGET=x86sim"
    test_arr[$LINENO]="make all T_DATA_A=cint16 T_DATA_B=int16 DIM_SIZE=32    NUM_FRAMES=8  SHIFT=14 API_IO=0 UUT_SSR=2 ROUND_MODE=0  SAT_MODE=3 STIM_TYPE=0 AIE_VARIANT=1 TARGET=x86sim"
    test_arr[$LINENO]="make all T_DATA_A=cint32 T_DATA_B=int32 DIM_SIZE=256   NUM_FRAMES=5  SHIFT=30 API_IO=0 UUT_SSR=2 ROUND_MODE=1  SAT_MODE=0 STIM_TYPE=0 AIE_VARIANT=1 TARGET=x86sim"
    test_arr[$LINENO]="make all T_DATA_A=cint16 T_DATA_B=int16 DIM_SIZE=512   NUM_FRAMES=5  SHIFT=14 API_IO=0 UUT_SSR=2 ROUND_MODE=2  SAT_MODE=1 STIM_TYPE=0 AIE_VARIANT=1 TARGET=x86sim"
    test_arr[$LINENO]="make all T_DATA_A=cint16 T_DATA_B=int16 DIM_SIZE=512   NUM_FRAMES=5  SHIFT=14 API_IO=0 UUT_SSR=2 ROUND_MODE=3  SAT_MODE=3 STIM_TYPE=0 AIE_VARIANT=1 TARGET=x86sim"
fi

if [[  "$*" == *datatypes* ]]
then
    test_arr[$LINENO]="make all T_DATA_A=int16  T_DATA_B=int16  DIM_SIZE=256   NUM_FRAMES=4  SHIFT=14 API_IO=0 UUT_SSR=1 ROUND_MODE=0  SAT_MODE=0 STIM_TYPE=0 AIE_VARIANT=1          "
    test_arr[$LINENO]="make all T_DATA_A=int16  T_DATA_B=int32  DIM_SIZE=256   NUM_FRAMES=4  SHIFT=14 API_IO=0 UUT_SSR=1 ROUND_MODE=0  SAT_MODE=0 STIM_TYPE=0 AIE_VARIANT=1          "
    test_arr[$LINENO]="make all T_DATA_A=int16  T_DATA_B=cint16 DIM_SIZE=256   NUM_FRAMES=4  SHIFT=14 API_IO=0 UUT_SSR=1 ROUND_MODE=0  SAT_MODE=0 STIM_TYPE=0 AIE_VARIANT=1          "
    test_arr[$LINENO]="make all T_DATA_A=int16  T_DATA_B=cint32 DIM_SIZE=256   NUM_FRAMES=4  SHIFT=14 API_IO=0 UUT_SSR=1 ROUND_MODE=0  SAT_MODE=0 STIM_TYPE=0 AIE_VARIANT=1          "
    test_arr[$LINENO]="make all T_DATA_A=int32  T_DATA_B=int16  DIM_SIZE=256   NUM_FRAMES=4  SHIFT=14 API_IO=0 UUT_SSR=1 ROUND_MODE=0  SAT_MODE=0 STIM_TYPE=0 AIE_VARIANT=1          "
    test_arr[$LINENO]="make all T_DATA_A=int32  T_DATA_B=int32  DIM_SIZE=256   NUM_FRAMES=4  SHIFT=14 API_IO=0 UUT_SSR=1 ROUND_MODE=0  SAT_MODE=0 STIM_TYPE=0 AIE_VARIANT=1          "
    test_arr[$LINENO]="make all T_DATA_A=int32  T_DATA_B=cint16 DIM_SIZE=256   NUM_FRAMES=4  SHIFT=14 API_IO=0 UUT_SSR=1 ROUND_MODE=0  SAT_MODE=0 STIM_TYPE=0 AIE_VARIANT=1          "
    test_arr[$LINENO]="make all T_DATA_A=int32  T_DATA_B=cint32 DIM_SIZE=256   NUM_FRAMES=4  SHIFT=14 API_IO=0 UUT_SSR=1 ROUND_MODE=0  SAT_MODE=0 STIM_TYPE=0 AIE_VARIANT=1          "
    test_arr[$LINENO]="make all T_DATA_A=cint16 T_DATA_B=int16  DIM_SIZE=256   NUM_FRAMES=4  SHIFT=14 API_IO=0 UUT_SSR=1 ROUND_MODE=0  SAT_MODE=0 STIM_TYPE=0 AIE_VARIANT=1          "
    test_arr[$LINENO]="make all T_DATA_A=cint16 T_DATA_B=int32  DIM_SIZE=256   NUM_FRAMES=4  SHIFT=14 API_IO=0 UUT_SSR=1 ROUND_MODE=0  SAT_MODE=0 STIM_TYPE=0 AIE_VARIANT=1          "
    test_arr[$LINENO]="make all T_DATA_A=cint16 T_DATA_B=cint16 DIM_SIZE=256   NUM_FRAMES=4  SHIFT=14 API_IO=0 UUT_SSR=1 ROUND_MODE=0  SAT_MODE=0 STIM_TYPE=0 AIE_VARIANT=1          "
    test_arr[$LINENO]="make all T_DATA_A=cint16 T_DATA_B=cint32 DIM_SIZE=256   NUM_FRAMES=4  SHIFT=14 API_IO=0 UUT_SSR=1 ROUND_MODE=0  SAT_MODE=0 STIM_TYPE=0 AIE_VARIANT=1          "
    test_arr[$LINENO]="make all T_DATA_A=cint32 T_DATA_B=int16  DIM_SIZE=256   NUM_FRAMES=4  SHIFT=14 API_IO=0 UUT_SSR=1 ROUND_MODE=0  SAT_MODE=0 STIM_TYPE=0 AIE_VARIANT=1          "
    test_arr[$LINENO]="make all T_DATA_A=cint32 T_DATA_B=int32  DIM_SIZE=256   NUM_FRAMES=4  SHIFT=14 API_IO=0 UUT_SSR=1 ROUND_MODE=0  SAT_MODE=0 STIM_TYPE=0 AIE_VARIANT=1          "
    test_arr[$LINENO]="make all T_DATA_A=cint32 T_DATA_B=cint16 DIM_SIZE=256   NUM_FRAMES=4  SHIFT=14 API_IO=0 UUT_SSR=1 ROUND_MODE=0  SAT_MODE=0 STIM_TYPE=0 AIE_VARIANT=1          "
    test_arr[$LINENO]="make all T_DATA_A=cint32 T_DATA_B=cint32 DIM_SIZE=256   NUM_FRAMES=4  SHIFT=14 API_IO=0 UUT_SSR=1 ROUND_MODE=0  SAT_MODE=0 STIM_TYPE=0 AIE_VARIANT=1          "
    test_arr[$LINENO]="make all T_DATA_A=float  T_DATA_B=float  DIM_SIZE=256   NUM_FRAMES=4           API_IO=0 UUT_SSR=1                          STIM_TYPE=0 AIE_VARIANT=1          "
    test_arr[$LINENO]="make all T_DATA_A=float  T_DATA_B=cfloat DIM_SIZE=256   NUM_FRAMES=4           API_IO=0 UUT_SSR=1                          STIM_TYPE=0 AIE_VARIANT=1          "
    test_arr[$LINENO]="make all T_DATA_A=cfloat T_DATA_B=cfloat DIM_SIZE=256   NUM_FRAMES=4           API_IO=0 UUT_SSR=1                          STIM_TYPE=0 AIE_VARIANT=1          "

    test_arr[$LINENO]="make all T_DATA_A=int16  T_DATA_B=int16  DIM_SIZE=256   NUM_FRAMES=4  SHIFT=14 API_IO=1 UUT_SSR=1 ROUND_MODE=0  SAT_MODE=0 STIM_TYPE=0 AIE_VARIANT=1          "
    test_arr[$LINENO]="make all T_DATA_A=int16  T_DATA_B=int32  DIM_SIZE=256   NUM_FRAMES=4  SHIFT=14 API_IO=1 UUT_SSR=1 ROUND_MODE=0  SAT_MODE=0 STIM_TYPE=0 AIE_VARIANT=1          "
    test_arr[$LINENO]="make all T_DATA_A=int16  T_DATA_B=cint16 DIM_SIZE=256   NUM_FRAMES=4  SHIFT=14 API_IO=1 UUT_SSR=1 ROUND_MODE=0  SAT_MODE=0 STIM_TYPE=0 AIE_VARIANT=1          "
    test_arr[$LINENO]="make all T_DATA_A=int16  T_DATA_B=cint32 DIM_SIZE=256   NUM_FRAMES=4  SHIFT=14 API_IO=1 UUT_SSR=1 ROUND_MODE=0  SAT_MODE=0 STIM_TYPE=0 AIE_VARIANT=1          "
    test_arr[$LINENO]="make all T_DATA_A=int32  T_DATA_B=int16  DIM_SIZE=256   NUM_FRAMES=4  SHIFT=14 API_IO=1 UUT_SSR=1 ROUND_MODE=0  SAT_MODE=0 STIM_TYPE=0 AIE_VARIANT=1          "
    test_arr[$LINENO]="make all T_DATA_A=int32  T_DATA_B=int32  DIM_SIZE=256   NUM_FRAMES=4  SHIFT=14 API_IO=1 UUT_SSR=1 ROUND_MODE=0  SAT_MODE=0 STIM_TYPE=0 AIE_VARIANT=1          "
    test_arr[$LINENO]="make all T_DATA_A=int32  T_DATA_B=cint16 DIM_SIZE=256   NUM_FRAMES=4  SHIFT=14 API_IO=1 UUT_SSR=1 ROUND_MODE=0  SAT_MODE=0 STIM_TYPE=0 AIE_VARIANT=1          "
    test_arr[$LINENO]="make all T_DATA_A=int32  T_DATA_B=cint32 DIM_SIZE=256   NUM_FRAMES=4  SHIFT=14 API_IO=1 UUT_SSR=1 ROUND_MODE=0  SAT_MODE=0 STIM_TYPE=0 AIE_VARIANT=1          "
    test_arr[$LINENO]="make all T_DATA_A=cint16 T_DATA_B=int16  DIM_SIZE=256   NUM_FRAMES=4  SHIFT=14 API_IO=1 UUT_SSR=1 ROUND_MODE=0  SAT_MODE=0 STIM_TYPE=0 AIE_VARIANT=1          "
    test_arr[$LINENO]="make all T_DATA_A=cint16 T_DATA_B=int32  DIM_SIZE=256   NUM_FRAMES=4  SHIFT=14 API_IO=1 UUT_SSR=1 ROUND_MODE=0  SAT_MODE=0 STIM_TYPE=0 AIE_VARIANT=1          "
    test_arr[$LINENO]="make all T_DATA_A=cint16 T_DATA_B=cint16 DIM_SIZE=256   NUM_FRAMES=4  SHIFT=14 API_IO=1 UUT_SSR=1 ROUND_MODE=0  SAT_MODE=0 STIM_TYPE=0 AIE_VARIANT=1          "
    test_arr[$LINENO]="make all T_DATA_A=cint16 T_DATA_B=cint32 DIM_SIZE=256   NUM_FRAMES=4  SHIFT=14 API_IO=1 UUT_SSR=1 ROUND_MODE=0  SAT_MODE=0 STIM_TYPE=0 AIE_VARIANT=1          "
    test_arr[$LINENO]="make all T_DATA_A=cint32 T_DATA_B=int16  DIM_SIZE=256   NUM_FRAMES=4  SHIFT=14 API_IO=1 UUT_SSR=1 ROUND_MODE=0  SAT_MODE=0 STIM_TYPE=0 AIE_VARIANT=1          "
    test_arr[$LINENO]="make all T_DATA_A=cint32 T_DATA_B=int32  DIM_SIZE=256   NUM_FRAMES=4  SHIFT=14 API_IO=1 UUT_SSR=1 ROUND_MODE=0  SAT_MODE=0 STIM_TYPE=0 AIE_VARIANT=1          "
    test_arr[$LINENO]="make all T_DATA_A=cint32 T_DATA_B=cint16 DIM_SIZE=256   NUM_FRAMES=4  SHIFT=14 API_IO=1 UUT_SSR=1 ROUND_MODE=0  SAT_MODE=0 STIM_TYPE=0 AIE_VARIANT=1          "
    test_arr[$LINENO]="make all T_DATA_A=cint32 T_DATA_B=cint32 DIM_SIZE=256   NUM_FRAMES=4  SHIFT=14 API_IO=1 UUT_SSR=1 ROUND_MODE=0  SAT_MODE=0 STIM_TYPE=0 AIE_VARIANT=1          "
    test_arr[$LINENO]="make all T_DATA_A=float  T_DATA_B=float  DIM_SIZE=256   NUM_FRAMES=4           API_IO=1 UUT_SSR=1                          STIM_TYPE=0 AIE_VARIANT=1          "
    test_arr[$LINENO]="make all T_DATA_A=float  T_DATA_B=cfloat DIM_SIZE=256   NUM_FRAMES=4           API_IO=1 UUT_SSR=1                          STIM_TYPE=0 AIE_VARIANT=1          "
    test_arr[$LINENO]="make all T_DATA_A=cfloat T_DATA_B=cfloat DIM_SIZE=256   NUM_FRAMES=4           API_IO=1 UUT_SSR=1                          STIM_TYPE=0 AIE_VARIANT=1          "

fi

if [[  "$*" == *ML* ]]
then
    # test_arr[$LINENO]="make all T_DATA_A=int16  T_DATA_B=int16  DIM_SIZE=256   NUM_FRAMES=4  SHIFT=14 API_IO=0 UUT_SSR=1 ROUND_MODE=0  SAT_MODE=0 STIM_TYPE=0 AIE_VARIANT=2 "
    # test_arr[$LINENO]="make all T_DATA_A=int16  T_DATA_B=int32  DIM_SIZE=256   NUM_FRAMES=4  SHIFT=14 API_IO=0 UUT_SSR=1 ROUND_MODE=0  SAT_MODE=0 STIM_TYPE=0 AIE_VARIANT=2 "
    # test_arr[$LINENO]="make all T_DATA_A=int16  T_DATA_B=cint16 DIM_SIZE=256   NUM_FRAMES=4  SHIFT=14 API_IO=0 UUT_SSR=1 ROUND_MODE=0  SAT_MODE=0 STIM_TYPE=0 AIE_VARIANT=2 "
    # test_arr[$LINENO]="make all T_DATA_A=int16  T_DATA_B=cint32 DIM_SIZE=256   NUM_FRAMES=4  SHIFT=14 API_IO=0 UUT_SSR=1 ROUND_MODE=0  SAT_MODE=0 STIM_TYPE=0 AIE_VARIANT=2 "
    # test_arr[$LINENO]="make all T_DATA_A=int32  T_DATA_B=int16  DIM_SIZE=256   NUM_FRAMES=4  SHIFT=14 API_IO=0 UUT_SSR=1 ROUND_MODE=0  SAT_MODE=0 STIM_TYPE=0 AIE_VARIANT=2 "
    # test_arr[$LINENO]="make all T_DATA_A=int32  T_DATA_B=int32  DIM_SIZE=256   NUM_FRAMES=4  SHIFT=14 API_IO=0 UUT_SSR=1 ROUND_MODE=0  SAT_MODE=0 STIM_TYPE=0 AIE_VARIANT=2 "
    test_arr[$LINENO]="make all T_DATA_A=int32  T_DATA_B=cint16 DIM_SIZE=256   NUM_FRAMES=4  SHIFT=14 API_IO=0 UUT_SSR=1 ROUND_MODE=0  SAT_MODE=0 STIM_TYPE=0 AIE_VARIANT=2 "
    # test_arr[$LINENO]="make all T_DATA_A=int32  T_DATA_B=cint32 DIM_SIZE=256   NUM_FRAMES=4  SHIFT=14 API_IO=0 UUT_SSR=1 ROUND_MODE=0  SAT_MODE=0 STIM_TYPE=0 AIE_VARIANT=2 "
    # test_arr[$LINENO]="make all T_DATA_A=cint16 T_DATA_B=int16  DIM_SIZE=256   NUM_FRAMES=4  SHIFT=14 API_IO=0 UUT_SSR=1 ROUND_MODE=0  SAT_MODE=0 STIM_TYPE=0 AIE_VARIANT=2 "
    test_arr[$LINENO]="make all T_DATA_A=cint16 T_DATA_B=int32  DIM_SIZE=256   NUM_FRAMES=4  SHIFT=14 API_IO=0 UUT_SSR=1 ROUND_MODE=0  SAT_MODE=0 STIM_TYPE=0 AIE_VARIANT=2 "
    # test_arr[$LINENO]="make all T_DATA_A=cint16 T_DATA_B=cint16 DIM_SIZE=256   NUM_FRAMES=4  SHIFT=14 API_IO=0 UUT_SSR=1 ROUND_MODE=0  SAT_MODE=0 STIM_TYPE=0 AIE_VARIANT=2 "
    # test_arr[$LINENO]="make all T_DATA_A=cint16 T_DATA_B=cint32 DIM_SIZE=256   NUM_FRAMES=4  SHIFT=14 API_IO=0 UUT_SSR=1 ROUND_MODE=0  SAT_MODE=0 STIM_TYPE=0 AIE_VARIANT=2 "
    # test_arr[$LINENO]="make all T_DATA_A=cint32 T_DATA_B=int16  DIM_SIZE=256   NUM_FRAMES=4  SHIFT=14 API_IO=0 UUT_SSR=1 ROUND_MODE=0  SAT_MODE=0 STIM_TYPE=0 AIE_VARIANT=2 "
    # test_arr[$LINENO]="make all T_DATA_A=cint32 T_DATA_B=int32  DIM_SIZE=256   NUM_FRAMES=4  SHIFT=14 API_IO=0 UUT_SSR=1 ROUND_MODE=0  SAT_MODE=0 STIM_TYPE=0 AIE_VARIANT=2 "
    # test_arr[$LINENO]="make all T_DATA_A=cint32 T_DATA_B=cint16 DIM_SIZE=256   NUM_FRAMES=4  SHIFT=14 API_IO=0 UUT_SSR=1 ROUND_MODE=0  SAT_MODE=0 STIM_TYPE=0 AIE_VARIANT=2 "
    # test_arr[$LINENO]="make all T_DATA_A=cint32 T_DATA_B=cint32 DIM_SIZE=256   NUM_FRAMES=4  SHIFT=14 API_IO=0 UUT_SSR=1 ROUND_MODE=0  SAT_MODE=0 STIM_TYPE=0 AIE_VARIANT=2 "
    # test_arr[$LINENO]="make all T_DATA_A=float  T_DATA_B=float  DIM_SIZE=256   NUM_FRAMES=4           API_IO=0 UUT_SSR=1                          STIM_TYPE=0 AIE_VARIANT=2 "
    # test_arr[$LINENO]="make all T_DATA_A=float  T_DATA_B=cfloat DIM_SIZE=256   NUM_FRAMES=4           API_IO=0 UUT_SSR=1                          STIM_TYPE=0 AIE_VARIANT=2 "
    # test_arr[$LINENO]="make all T_DATA_A=cfloat T_DATA_B=cfloat DIM_SIZE=256   NUM_FRAMES=4           API_IO=0 UUT_SSR=1                          STIM_TYPE=0 AIE_VARIANT=2 "

fi


if [[  "$*" == "" || "$*" == *checkin* ]]
then
    test_arr[${#test_arr[*]}]="echo 'Running limited number of check-in tests.'"
    test_arr[$LINENO]="make all T_DATA_A=cint16 T_DATA_B=cint32  DIM_SIZE=19    NUM_FRAMES=4   SHIFT=14 API_IO=0 UUT_SSR=1  ROUND_MODE=0 SAT_MODE=1  STIM_TYPE=0 TARGET=x86sim"
    test_arr[$LINENO]="make all T_DATA_A=cint32 T_DATA_B=int32   DIM_SIZE=377   NUM_FRAMES=1   SHIFT=30 API_IO=0 UUT_SSR=1  ROUND_MODE=0 SAT_MODE=1  STIM_TYPE=0                  "
    test_arr[$LINENO]="make all T_DATA_A=cfloat T_DATA_B=float   DIM_SIZE=256   NUM_FRAMES=1   SHIFT=0  API_IO=0 UUT_SSR=1  ROUND_MODE=0 SAT_MODE=1  STIM_TYPE=1                  "
    test_arr[$LINENO]="make all T_DATA_A=cint16 T_DATA_B=int16   DIM_SIZE=16    NUM_FRAMES=16  SHIFT=14 API_IO=0 UUT_SSR=1  ROUND_MODE=0 SAT_MODE=1  STIM_TYPE=0                  "
    test_arr[$LINENO]="make all T_DATA_A=int16  T_DATA_B=cint32  DIM_SIZE=16    NUM_FRAMES=16  SHIFT=14 API_IO=1 UUT_SSR=1  ROUND_MODE=0 SAT_MODE=1  STIM_TYPE=5                  "
    test_arr[$LINENO]="make all T_DATA_A=cint16 T_DATA_B=int32   DIM_SIZE=32    NUM_FRAMES=8   SHIFT=14 API_IO=0 UUT_SSR=2  ROUND_MODE=0 SAT_MODE=1  STIM_TYPE=0                  "
    test_arr[$LINENO]="make all T_DATA_A=int16  T_DATA_B=cint32  DIM_SIZE=32    NUM_FRAMES=8   SHIFT=14 API_IO=1 UUT_SSR=2  ROUND_MODE=0 SAT_MODE=1  STIM_TYPE=0                  "
    test_arr[$LINENO]="make all T_DATA_A=cint32 T_DATA_B=cint16  DIM_SIZE=1024  NUM_FRAMES=1   SHIFT=30 API_IO=1 UUT_SSR=2  ROUND_MODE=0 SAT_MODE=1  STIM_TYPE=0                  "
    test_arr[$LINENO]="make all T_DATA_A=int32  T_DATA_B=cint16  DIM_SIZE=1024  NUM_FRAMES=1   SHIFT=14 API_IO=1 UUT_SSR=2  ROUND_MODE=0 SAT_MODE=1  STIM_TYPE=0                  "
    test_arr[$LINENO]="make all T_DATA_A=cint32 T_DATA_B=int16   DIM_SIZE=256   NUM_FRAMES=2   SHIFT=14 API_IO=1 UUT_SSR=4  ROUND_MODE=0 SAT_MODE=1  STIM_TYPE=0                  "
    test_arr[$LINENO]="make all T_DATA_A=cint16 T_DATA_B=int16   DIM_SIZE=256   NUM_FRAMES=1   SHIFT=14 API_IO=0 UUT_SSR=1  ROUND_MODE=0 SAT_MODE=1  STIM_TYPE=0                  "
    test_arr[$LINENO]="make all T_DATA_A=cint32 T_DATA_B=int32   DIM_SIZE=256   NUM_FRAMES=1   SHIFT=32 API_IO=0 UUT_SSR=1  ROUND_MODE=0 SAT_MODE=1  STIM_TYPE=0                  "
    test_arr[$LINENO]="make all T_DATA_A=cint32 T_DATA_B=int16   DIM_SIZE=256   NUM_FRAMES=5   SHIFT=32 API_IO=1 UUT_SSR=1  ROUND_MODE=0 SAT_MODE=1  STIM_TYPE=0                  "
    test_arr[$LINENO]="make all T_DATA_A=cint16 T_DATA_B=int16   DIM_SIZE=256   NUM_FRAMES=16  SHIFT=14 API_IO=1 UUT_SSR=1  ROUND_MODE=0 SAT_MODE=1  STIM_TYPE=0                  "
    test_arr[$LINENO]="make all T_DATA_A=cint16 T_DATA_B=int32   DIM_SIZE=256   NUM_FRAMES=1   SHIFT=14 API_IO=1 UUT_SSR=16 ROUND_MODE=0 SAT_MODE=1  STIM_TYPE=0                  "
    test_arr[$LINENO]="make all T_DATA_A=float  T_DATA_B=float   DIM_SIZE=256   NUM_FRAMES=8   SHIFT=0  API_IO=1 UUT_SSR=1  ROUND_MODE=0 SAT_MODE=1  STIM_TYPE=0 TARGET=x86sim"
    test_arr[$LINENO]="make all T_DATA_A=cint32 T_DATA_B=cint16  DIM_SIZE=1024  NUM_FRAMES=1   SHIFT=14 API_IO=0 UUT_SSR=2  ROUND_MODE=2 SAT_MODE=3  STIM_TYPE=0 TARGET=x86sim"
    test_arr[$LINENO]="make all T_DATA_A=int32  T_DATA_B=int16   DIM_SIZE=256   NUM_FRAMES=2   SHIFT=14 API_IO=0 UUT_SSR=4  ROUND_MODE=4 SAT_MODE=1  STIM_TYPE=0 TARGET=x86sim"
    test_arr[$LINENO]="make all T_DATA_A=cint32 T_DATA_B=cint16  DIM_SIZE=256   NUM_FRAMES=7   SHIFT=14 API_IO=1 UUT_SSR=1  ROUND_MODE=2 SAT_MODE=0  STIM_TYPE=0                  "
    test_arr[$LINENO]="make all T_DATA_A=int32  T_DATA_B=int16   DIM_SIZE=256   NUM_FRAMES=1   SHIFT=14 API_IO=1 UUT_SSR=1  ROUND_MODE=7 SAT_MODE=1  STIM_TYPE=0                  "
    test_arr[$LINENO]="make all T_DATA_A=cint16 T_DATA_B=cint16  DIM_SIZE=19    NUM_FRAMES=4   SHIFT=14 API_IO=1 UUT_SSR=1 STIM_TYPE=1 TARGET=x86sim"
    test_arr[$LINENO]="make all T_DATA_A=int32  T_DATA_B=cint16  DIM_SIZE=76    NUM_FRAMES=4   SHIFT=14 API_IO=0 UUT_SSR=2 STIM_TYPE=1 TARGET=x86sim"
    test_arr[$LINENO]="make all T_DATA_A=int32  T_DATA_B=cint32  DIM_SIZE=600   NUM_FRAMES=4   SHIFT=14 API_IO=1 UUT_SSR=3 STIM_TYPE=1 TARGET=x86sim"

    test_arr[$LINENO]="make all T_DATA_A=cint16 T_DATA_B=cint32  DIM_SIZE=19    NUM_FRAMES=4   SHIFT=14 API_IO=0 UUT_SSR=1  ROUND_MODE=0 SAT_MODE=1  STIM_TYPE=0 TARGET=x86sim AIE_VARIANT=2"
    test_arr[$LINENO]="make all T_DATA_A=cint32 T_DATA_B=int32   DIM_SIZE=377   NUM_FRAMES=1   SHIFT=30 API_IO=0 UUT_SSR=1  ROUND_MODE=0 SAT_MODE=1  STIM_TYPE=0                   AIE_VARIANT=2"
    test_arr[$LINENO]="make all T_DATA_A=cfloat T_DATA_B=float   DIM_SIZE=256   NUM_FRAMES=1   SHIFT=0  API_IO=0 UUT_SSR=1  ROUND_MODE=0 SAT_MODE=1  STIM_TYPE=1                   AIE_VARIANT=2"
    test_arr[$LINENO]="make all T_DATA_A=cint16 T_DATA_B=int16   DIM_SIZE=16    NUM_FRAMES=16  SHIFT=14 API_IO=0 UUT_SSR=1  ROUND_MODE=0 SAT_MODE=1  STIM_TYPE=0                   AIE_VARIANT=2"
    test_arr[$LINENO]="make all T_DATA_A=int16  T_DATA_B=cint32  DIM_SIZE=16    NUM_FRAMES=16  SHIFT=14 API_IO=0 UUT_SSR=1  ROUND_MODE=0 SAT_MODE=1  STIM_TYPE=5                   AIE_VARIANT=2"
    test_arr[$LINENO]="make all T_DATA_A=cfloat T_DATA_B=float   DIM_SIZE=32    NUM_FRAMES=8   SHIFT=0  API_IO=0 UUT_SSR=2  ROUND_MODE=0 SAT_MODE=1  STIM_TYPE=0                   AIE_VARIANT=2"
    test_arr[$LINENO]="make all T_DATA_A=int16  T_DATA_B=cint32  DIM_SIZE=32    NUM_FRAMES=8   SHIFT=14 API_IO=0 UUT_SSR=2  ROUND_MODE=0 SAT_MODE=1  STIM_TYPE=0                   AIE_VARIANT=2"
    test_arr[$LINENO]="make all T_DATA_A=cint32 T_DATA_B=cint16  DIM_SIZE=1024  NUM_FRAMES=1   SHIFT=30 API_IO=0 UUT_SSR=2  ROUND_MODE=0 SAT_MODE=1  STIM_TYPE=0                   AIE_VARIANT=2"
    test_arr[$LINENO]="make all T_DATA_A=int32  T_DATA_B=int32   DIM_SIZE=1024  NUM_FRAMES=1   SHIFT=14 API_IO=0 UUT_SSR=2  ROUND_MODE=0 SAT_MODE=1  STIM_TYPE=0                   AIE_VARIANT=2"
    test_arr[$LINENO]="make all T_DATA_A=cint32 T_DATA_B=int16   DIM_SIZE=256   NUM_FRAMES=2   SHIFT=14 API_IO=0 UUT_SSR=4  ROUND_MODE=0 SAT_MODE=1  STIM_TYPE=0                   AIE_VARIANT=2"
    test_arr[$LINENO]="make all T_DATA_A=cint16 T_DATA_B=int16   DIM_SIZE=256   NUM_FRAMES=1   SHIFT=14 API_IO=0 UUT_SSR=1  ROUND_MODE=0 SAT_MODE=1  STIM_TYPE=0                   AIE_VARIANT=2"
    test_arr[$LINENO]="make all T_DATA_A=cint32 T_DATA_B=int32   DIM_SIZE=256   NUM_FRAMES=1   SHIFT=32 API_IO=0 UUT_SSR=1  ROUND_MODE=0 SAT_MODE=1  STIM_TYPE=0                   AIE_VARIANT=2"
    test_arr[$LINENO]="make all T_DATA_A=cint32 T_DATA_B=int16   DIM_SIZE=256   NUM_FRAMES=5   SHIFT=32 API_IO=0 UUT_SSR=1  ROUND_MODE=0 SAT_MODE=1  STIM_TYPE=0                   AIE_VARIANT=2"
    test_arr[$LINENO]="make all T_DATA_A=cint16 T_DATA_B=int16   DIM_SIZE=256   NUM_FRAMES=16  SHIFT=14 API_IO=0 UUT_SSR=1  ROUND_MODE=0 SAT_MODE=1  STIM_TYPE=0                   AIE_VARIANT=2"
    test_arr[$LINENO]="make all T_DATA_A=cint16 T_DATA_B=cint32  DIM_SIZE=256   NUM_FRAMES=1   SHIFT=14 API_IO=0 UUT_SSR=16 ROUND_MODE=0 SAT_MODE=1  STIM_TYPE=0                   AIE_VARIANT=2"
    test_arr[$LINENO]="make all T_DATA_A=float  T_DATA_B=float   DIM_SIZE=256   NUM_FRAMES=8   SHIFT=0  API_IO=0 UUT_SSR=1  ROUND_MODE=0 SAT_MODE=1  STIM_TYPE=0 TARGET=x86sim AIE_VARIANT=2"
    test_arr[$LINENO]="make all T_DATA_A=cint32 T_DATA_B=cint16  DIM_SIZE=1024  NUM_FRAMES=1   SHIFT=14 API_IO=0 UUT_SSR=2  ROUND_MODE=2 SAT_MODE=3  STIM_TYPE=0 TARGET=x86sim AIE_VARIANT=2"
    test_arr[$LINENO]="make all T_DATA_A=int32  T_DATA_B=int16   DIM_SIZE=256   NUM_FRAMES=2   SHIFT=14 API_IO=0 UUT_SSR=4  ROUND_MODE=8 SAT_MODE=1  STIM_TYPE=0 TARGET=x86sim AIE_VARIANT=2"
    test_arr[$LINENO]="make all T_DATA_A=cint32 T_DATA_B=cint16  DIM_SIZE=256   NUM_FRAMES=7   SHIFT=14 API_IO=0 UUT_SSR=1  ROUND_MODE=2 SAT_MODE=0  STIM_TYPE=0                   AIE_VARIANT=2"
    test_arr[$LINENO]="make all T_DATA_A=int32  T_DATA_B=int16   DIM_SIZE=256   NUM_FRAMES=1   SHIFT=14 API_IO=0 UUT_SSR=1  ROUND_MODE=11 SAT_MODE=1  STIM_TYPE=0                   AIE_VARIANT=2"
    test_arr[$LINENO]="make all T_DATA_A=cint16 T_DATA_B=cint16  DIM_SIZE=19    NUM_FRAMES=4   SHIFT=14 API_IO=0 UUT_SSR=1 STIM_TYPE=1 TARGET=x86sim AIE_VARIANT=2"
    test_arr[$LINENO]="make all T_DATA_A=cint32 T_DATA_B=cint16  DIM_SIZE=76    NUM_FRAMES=4   SHIFT=14 API_IO=0 UUT_SSR=2 STIM_TYPE=1 TARGET=x86sim AIE_VARIANT=2"
    test_arr[$LINENO]="make all T_DATA_A=int32  T_DATA_B=cint32  DIM_SIZE=600   NUM_FRAMES=4   SHIFT=14 API_IO=0 UUT_SSR=3 STIM_TYPE=1 TARGET=x86sim AIE_VARIANT=2"
fi


if [[ "$*" == *target_x86*  ]]
then
    test_arr[${#test_arr[*]}]="echo 'Running limited number of x86 tests.'"
    #window choices
    test_arr[$LINENO]="make all T_DATA_A=cint16 T_DATA_B=int16 DIM_SIZE=256   NUM_FRAMES=1  SHIFT=14 API_IO=0 UUT_SSR=1 STIM_TYPE=6 TARGET=x86sim"
    test_arr[$LINENO]="make all T_DATA_A=cint16 T_DATA_B=int16 DIM_SIZE=256   NUM_FRAMES=1  SHIFT=14 API_IO=0 UUT_SSR=1 STIM_TYPE=6 TARGET=x86sim"
    test_arr[$LINENO]="make all T_DATA_A=cint16 T_DATA_B=int16 DIM_SIZE=256   NUM_FRAMES=1  SHIFT=14 API_IO=0 UUT_SSR=1 STIM_TYPE=6 TARGET=x86sim"

    #all point sizes
    test_arr[$LINENO]="make all T_DATA_A=cint16 T_DATA_B=int16 DIM_SIZE=16   NUM_FRAMES=1 SHIFT=14 API_IO=0 UUT_SSR=1 STIM_TYPE=6 TARGET=x86sim"
    test_arr[$LINENO]="make all T_DATA_A=cint16 T_DATA_B=int16 DIM_SIZE=32   NUM_FRAMES=1 SHIFT=14 API_IO=0 UUT_SSR=1 STIM_TYPE=6 TARGET=x86sim"
    test_arr[$LINENO]="make all T_DATA_A=cint16 T_DATA_B=int16 DIM_SIZE=64   NUM_FRAMES=1 SHIFT=14 API_IO=0 UUT_SSR=1 STIM_TYPE=6 TARGET=x86sim"
    test_arr[$LINENO]="make all T_DATA_A=cint16 T_DATA_B=int16 DIM_SIZE=128  NUM_FRAMES=1 SHIFT=14 API_IO=0 UUT_SSR=1 STIM_TYPE=6 TARGET=x86sim"
    test_arr[$LINENO]="make all T_DATA_A=cint16 T_DATA_B=int16 DIM_SIZE=256  NUM_FRAMES=1 SHIFT=14 API_IO=0 UUT_SSR=1 STIM_TYPE=6 TARGET=x86sim"
    test_arr[$LINENO]="make all T_DATA_A=cint16 T_DATA_B=int16 DIM_SIZE=512  NUM_FRAMES=1 SHIFT=14 API_IO=0 UUT_SSR=1 STIM_TYPE=6 TARGET=x86sim"
    test_arr[$LINENO]="make all T_DATA_A=cint16 T_DATA_B=int16 DIM_SIZE=1024 NUM_FRAMES=1 SHIFT=14 API_IO=0 UUT_SSR=1 STIM_TYPE=6 TARGET=x86sim"
    test_arr[$LINENO]="make all T_DATA_A=cint16 T_DATA_B=int16 DIM_SIZE=2048 NUM_FRAMES=1 SHIFT=14 API_IO=0 UUT_SSR=1 STIM_TYPE=6 TARGET=x86sim"
    test_arr[$LINENO]="make all T_DATA_A=cint16 T_DATA_B=int16 DIM_SIZE=4096 NUM_FRAMES=1 SHIFT=14 API_IO=0 UUT_SSR=1 STIM_TYPE=6 TARGET=x86sim"
    test_arr[$LINENO]="make all T_DATA_A=cint16 T_DATA_B=int16 DIM_SIZE=16   NUM_FRAMES=1 SHIFT=14 API_IO=0 UUT_SSR=1 STIM_TYPE=6 TARGET=x86sim"

    #all point sizes
    test_arr[$LINENO]="make all AIE_VARIANT=1 T_DATA_A=cint16 T_DATA_B=int16 DIM_SIZE=16   NUM_FRAMES=1 SHIFT=14 API_IO=0 UUT_SSR=1 STIM_TYPE=6 TARGET=x86sim"
    test_arr[$LINENO]="make all AIE_VARIANT=1 T_DATA_A=cint16 T_DATA_B=int16 DIM_SIZE=32   NUM_FRAMES=1 SHIFT=14 API_IO=0 UUT_SSR=1 STIM_TYPE=6 TARGET=x86sim"
    test_arr[$LINENO]="make all AIE_VARIANT=1 T_DATA_A=cint16 T_DATA_B=int16 DIM_SIZE=64   NUM_FRAMES=1 SHIFT=14 API_IO=0 UUT_SSR=1 STIM_TYPE=6 TARGET=x86sim"
    test_arr[$LINENO]="make all AIE_VARIANT=1 T_DATA_A=cint16 T_DATA_B=int16 DIM_SIZE=128  NUM_FRAMES=1 SHIFT=14 API_IO=0 UUT_SSR=1 STIM_TYPE=6 TARGET=x86sim"
    test_arr[$LINENO]="make all AIE_VARIANT=1 T_DATA_A=cint16 T_DATA_B=int16 DIM_SIZE=256  NUM_FRAMES=1 SHIFT=14 API_IO=0 UUT_SSR=1 STIM_TYPE=6 TARGET=x86sim"
    test_arr[$LINENO]="make all AIE_VARIANT=1 T_DATA_A=cint16 T_DATA_B=int16 DIM_SIZE=512  NUM_FRAMES=1 SHIFT=14 API_IO=0 UUT_SSR=1 STIM_TYPE=6 TARGET=x86sim"
    test_arr[$LINENO]="make all AIE_VARIANT=1 T_DATA_A=cint16 T_DATA_B=int16 DIM_SIZE=1024 NUM_FRAMES=1 SHIFT=14 API_IO=0 UUT_SSR=1 STIM_TYPE=6 TARGET=x86sim"
    test_arr[$LINENO]="make all AIE_VARIANT=1 T_DATA_A=cint16 T_DATA_B=int16 DIM_SIZE=2048 NUM_FRAMES=1 SHIFT=14 API_IO=0 UUT_SSR=1 STIM_TYPE=6 TARGET=x86sim"
    test_arr[$LINENO]="make all AIE_VARIANT=1 T_DATA_A=cint16 T_DATA_B=int16 DIM_SIZE=4096 NUM_FRAMES=1 SHIFT=14 API_IO=0 UUT_SSR=1 STIM_TYPE=6 TARGET=x86sim"
    test_arr[$LINENO]="make all AIE_VARIANT=1 T_DATA_A=cint16 T_DATA_B=int16 DIM_SIZE=16   NUM_FRAMES=1 SHIFT=14 API_IO=0 UUT_SSR=1 STIM_TYPE=6 TARGET=x86sim"

    #window sizes
    test_arr[$LINENO]="make all T_DATA_A=cint16 T_DATA_B=int16 DIM_SIZE=16   NUM_FRAMES=2   SHIFT=14 API_IO=0 UUT_SSR=1 STIM_TYPE=6 TARGET=x86sim"
    test_arr[$LINENO]="make all T_DATA_A=cint16 T_DATA_B=int16 DIM_SIZE=16   NUM_FRAMES=4   SHIFT=14 API_IO=0 UUT_SSR=1 STIM_TYPE=6 TARGET=x86sim"
    test_arr[$LINENO]="make all T_DATA_A=cint16 T_DATA_B=int16 DIM_SIZE=16   NUM_FRAMES=8   SHIFT=14 API_IO=0 UUT_SSR=1 STIM_TYPE=6 TARGET=x86sim"
    test_arr[$LINENO]="make all T_DATA_A=cint16 T_DATA_B=int16 DIM_SIZE=16   NUM_FRAMES=16  SHIFT=14 API_IO=0 UUT_SSR=1 STIM_TYPE=6 TARGET=x86sim"
    test_arr[$LINENO]="make all T_DATA_A=cint16 T_DATA_B=int16 DIM_SIZE=16   NUM_FRAMES=32  SHIFT=14 API_IO=0 UUT_SSR=1 STIM_TYPE=6 TARGET=x86sim"
    test_arr[$LINENO]="make all T_DATA_A=cint16 T_DATA_B=int16 DIM_SIZE=16   NUM_FRAMES=64  SHIFT=14 API_IO=0 UUT_SSR=1 STIM_TYPE=6 TARGET=x86sim"
    test_arr[$LINENO]="make all T_DATA_A=cint16 T_DATA_B=int16 DIM_SIZE=16   NUM_FRAMES=128 SHIFT=14 API_IO=0 UUT_SSR=1 STIM_TYPE=6 TARGET=x86sim"
    test_arr[$LINENO]="make all T_DATA_A=cint16 T_DATA_B=int16 DIM_SIZE=16   NUM_FRAMES=256 SHIFT=14 API_IO=0 UUT_SSR=1 STIM_TYPE=6 TARGET=x86sim"

    #types
    test_arr[$LINENO]="make all T_DATA_A=cint32 T_DATA_B=int32 DIM_SIZE=16   NUM_FRAMES=1   SHIFT=30 API_IO=0 UUT_SSR=1 STIM_TYPE=6 TARGET=x86sim"
    test_arr[$LINENO]="make all T_DATA_A=cfloat T_DATA_B=float DIM_SIZE=16   NUM_FRAMES=1   SHIFT=0  API_IO=0 UUT_SSR=1 STIM_TYPE=6 TARGET=x86sim"

    #API
    test_arr[$LINENO]="make all T_DATA_A=cint16 T_DATA_B=int16 DIM_SIZE=16   NUM_FRAMES=1   SHIFT=14 API_IO=1 UUT_SSR=1 STIM_TYPE=6 TARGET=x86sim"

    #SSR
    test_arr[$LINENO]="make all T_DATA_A=cint16 T_DATA_B=int16 DIM_SIZE=32   NUM_FRAMES=1  SHIFT=14 API_IO=0 UUT_SSR=2  STIM_TYPE=6 TARGET=x86sim"
    test_arr[$LINENO]="make all T_DATA_A=cint16 T_DATA_B=int16 DIM_SIZE=64   NUM_FRAMES=1  SHIFT=14 API_IO=0 UUT_SSR=4  STIM_TYPE=6 TARGET=x86sim"
    test_arr[$LINENO]="make all T_DATA_A=cint16 T_DATA_B=int16 DIM_SIZE=128  NUM_FRAMES=1  SHIFT=14 API_IO=0 UUT_SSR=8  STIM_TYPE=6 TARGET=x86sim"
    test_arr[$LINENO]="make all T_DATA_A=cint16 T_DATA_B=int16 DIM_SIZE=256  NUM_FRAMES=1  SHIFT=14 API_IO=0 UUT_SSR=16 STIM_TYPE=6 TARGET=x86sim"

    #APIvsSSR
    test_arr[$LINENO]="make all T_DATA_A=cint16 T_DATA_B=int16 DIM_SIZE=256  NUM_FRAMES=1  SHIFT=14 API_IO=1 UUT_SSR=16 STIM_TYPE=6 TARGET=x86sim"

    #er...
    test_arr[$LINENO]="make all T_DATA_A=cint32 T_DATA_B=int32 DIM_SIZE=256  NUM_FRAMES=4  SHIFT=30 API_IO=1 UUT_SSR=4 STIM_TYPE=6 TARGET=x86sim"
fi


#Qor tests should be added to fit a suite with a collective purpose. There may be multiple suites with different purposes, e.g.
#Parameter exploration - where a base case is augmented by additional cases which vary one parameter such that the user
#  can see how QoR varies with a change in that parameter.
#Bellwether - standalone cases which describe a known or typical customer usecase, e.g. 47tap HB FIR with cint16/int16
#Anomaly. Where there is a known anomaly in QoR, such as a step change at a particular FIR length, cases may be added
# to highlight the non-linearity.
#Other. If there are other cases or other suites added for a different purpose, please add a description of the suite here.
if [[ "$*" == *qor* || "$*" == *all* ]]
then
    test_arr[${#test_arr[*]}]="echo 'Performing QoR run'"
    #base case for parameter exploration
    test_arr[$LINENO]="make all T_DATA_A=int16 T_DATA_B=int16 DIM_SIZE=16   NUM_FRAMES=1   SHIFT=14 API_IO=0 UUT_SSR=1 STIM_TYPE=0 NITER=16 "
    test_arr[$LINENO]="make all T_DATA_A=int16 T_DATA_B=int16 DIM_SIZE=16   NUM_FRAMES=1   SHIFT=14 API_IO=0 UUT_SSR=1 STIM_TYPE=0 NITER=16 AIE_VARIANT=2"

    #types
    test_arr[$LINENO]="make all T_DATA_A=int32  T_DATA_B=int32  DIM_SIZE=27   NUM_FRAMES=1   SHIFT=14  API_IO=0 UUT_SSR=1 STIM_TYPE=0"
    test_arr[$LINENO]="make all T_DATA_A=cfloat T_DATA_B=float  DIM_SIZE=16   NUM_FRAMES=1   SHIFT=0   API_IO=1 UUT_SSR=1 STIM_TYPE=0"
    test_arr[$LINENO]="make all T_DATA_A=cint32 T_DATA_B=int16  DIM_SIZE=254  NUM_FRAMES=1   SHIFT=14  API_IO=1 UUT_SSR=1 STIM_TYPE=0"
    test_arr[$LINENO]="make all T_DATA_A=int32  T_DATA_B=cint16 DIM_SIZE=16   NUM_FRAMES=1   SHIFT=14  API_IO=0 UUT_SSR=1 STIM_TYPE=0"
    test_arr[$LINENO]="make all T_DATA_A=int32  T_DATA_B=cint32 DIM_SIZE=700  NUM_FRAMES=1   SHIFT=14  API_IO=0 UUT_SSR=5 STIM_TYPE=0"
    test_arr[$LINENO]="make all T_DATA_A=cint16 T_DATA_B=int16  DIM_SIZE=128  NUM_FRAMES=4   SHIFT=14  API_IO=0 UUT_SSR=4 STIM_TYPE=0"

    test_arr[$LINENO]="make all T_DATA_A=int32  T_DATA_B=int16  DIM_SIZE=40   NUM_FRAMES=1   SHIFT=14  API_IO=0 UUT_SSR=1 STIM_TYPE=0 AIE_VARIANT=2"
    test_arr[$LINENO]="make all T_DATA_A=float  T_DATA_B=float  DIM_SIZE=25   NUM_FRAMES=1   SHIFT=0   API_IO=0 UUT_SSR=1 STIM_TYPE=0 AIE_VARIANT=2"
    test_arr[$LINENO]="make all T_DATA_A=cint32 T_DATA_B=cint16 DIM_SIZE=256  NUM_FRAMES=1   SHIFT=14  API_IO=0 UUT_SSR=1 STIM_TYPE=0 AIE_VARIANT=2"
    test_arr[$LINENO]="make all T_DATA_A=int32  T_DATA_B=int32  DIM_SIZE=512  NUM_FRAMES=1   SHIFT=14  API_IO=0 UUT_SSR=1 STIM_TYPE=0 AIE_VARIANT=2"
    test_arr[$LINENO]="make all T_DATA_A=int16  T_DATA_B=cint32 DIM_SIZE=80   NUM_FRAMES=1   SHIFT=14  API_IO=0 UUT_SSR=5 STIM_TYPE=0 AIE_VARIANT=2"
    test_arr[$LINENO]="make all T_DATA_A=cint16 T_DATA_B=cint32 DIM_SIZE=64   NUM_FRAMES=4   SHIFT=14  API_IO=0 UUT_SSR=4 STIM_TYPE=0 AIE_VARIANT=2"


    #all point sizes
    test_arr[$LINENO]="make all T_DATA_A=cint16 T_DATA_B=int16  DIM_SIZE=16   NUM_FRAMES=1 SHIFT=14 API_IO=0 UUT_SSR=1 STIM_TYPE=0"
    test_arr[$LINENO]="make all T_DATA_A=cint16 T_DATA_B=cint16 DIM_SIZE=32   NUM_FRAMES=1 SHIFT=14 API_IO=0 UUT_SSR=1 STIM_TYPE=0"
    test_arr[$LINENO]="make all T_DATA_A=int16  T_DATA_B=int16  DIM_SIZE=64   NUM_FRAMES=1 SHIFT=14 API_IO=1 UUT_SSR=1 STIM_TYPE=0"
    test_arr[$LINENO]="make all T_DATA_A=int16  T_DATA_B=cint32 DIM_SIZE=128  NUM_FRAMES=1 SHIFT=14 API_IO=0 UUT_SSR=1 STIM_TYPE=0"
    test_arr[$LINENO]="make all T_DATA_A=cint32 T_DATA_B=int32  DIM_SIZE=256  NUM_FRAMES=1 SHIFT=14 API_IO=1 UUT_SSR=1 STIM_TYPE=0"
    test_arr[$LINENO]="make all T_DATA_A=cint16 T_DATA_B=int16  DIM_SIZE=512  NUM_FRAMES=1 SHIFT=14 API_IO=0 UUT_SSR=1 STIM_TYPE=0"
    test_arr[$LINENO]="make all T_DATA_A=cint16 T_DATA_B=cint32 DIM_SIZE=1024 NUM_FRAMES=1 SHIFT=14 API_IO=0 UUT_SSR=1 STIM_TYPE=0"
    test_arr[$LINENO]="make all T_DATA_A=float  T_DATA_B=float  DIM_SIZE=2048 NUM_FRAMES=1 SHIFT=0  API_IO=1 UUT_SSR=1 STIM_TYPE=0"

    test_arr[$LINENO]="make all T_DATA_A=cint16 T_DATA_B=cint32 DIM_SIZE=16   NUM_FRAMES=1 SHIFT=14 API_IO=0 UUT_SSR=1 STIM_TYPE=0 AIE_VARIANT=2"
    test_arr[$LINENO]="make all T_DATA_A=int16  T_DATA_B=cint16 DIM_SIZE=32   NUM_FRAMES=1 SHIFT=14 API_IO=0 UUT_SSR=1 STIM_TYPE=0 AIE_VARIANT=2"
    test_arr[$LINENO]="make all T_DATA_A=int32  T_DATA_B=int32  DIM_SIZE=64   NUM_FRAMES=1 SHIFT=14 API_IO=0 UUT_SSR=1 STIM_TYPE=0 AIE_VARIANT=2"
    test_arr[$LINENO]="make all T_DATA_A=int16  T_DATA_B=cint32 DIM_SIZE=128  NUM_FRAMES=1 SHIFT=14 API_IO=0 UUT_SSR=1 STIM_TYPE=0 AIE_VARIANT=2"
    test_arr[$LINENO]="make all T_DATA_A=cint32 T_DATA_B=int32  DIM_SIZE=256  NUM_FRAMES=1 SHIFT=14 API_IO=0 UUT_SSR=1 STIM_TYPE=0 AIE_VARIANT=2"
    test_arr[$LINENO]="make all T_DATA_A=int32  T_DATA_B=int16  DIM_SIZE=512  NUM_FRAMES=1 SHIFT=14 API_IO=0 UUT_SSR=1 STIM_TYPE=0 AIE_VARIANT=2"
    test_arr[$LINENO]="make all T_DATA_A=cint16 T_DATA_B=cint32 DIM_SIZE=1024 NUM_FRAMES=1 SHIFT=14 API_IO=0 UUT_SSR=1 STIM_TYPE=0 AIE_VARIANT=2"
    test_arr[$LINENO]="make all T_DATA_A=float  T_DATA_B=float DIM_SIZE=2048  NUM_FRAMES=1 SHIFT=0  API_IO=0 UUT_SSR=2 STIM_TYPE=0 AIE_VARIANT=2"

    #all point sizes
    test_arr[$LINENO]="make all AIE_VARIANT=1 T_DATA_A=cint16 T_DATA_B=int32  DIM_SIZE=16    NUM_FRAMES=1 SHIFT=14 API_IO=0 UUT_SSR=1   STIM_TYPE=0"
    test_arr[$LINENO]="make all AIE_VARIANT=1 T_DATA_A=int32  T_DATA_B=int16  DIM_SIZE=32    NUM_FRAMES=1 SHIFT=14 API_IO=0 UUT_SSR=1   STIM_TYPE=0"
    test_arr[$LINENO]="make all AIE_VARIANT=1 T_DATA_A=cint32 T_DATA_B=int16  DIM_SIZE=64    NUM_FRAMES=1 SHIFT=14 API_IO=0 UUT_SSR=1   STIM_TYPE=0"
    test_arr[$LINENO]="make all AIE_VARIANT=1 T_DATA_A=int32  T_DATA_B=int16  DIM_SIZE=128   NUM_FRAMES=1 SHIFT=14 API_IO=0 UUT_SSR=1   STIM_TYPE=0"
    test_arr[$LINENO]="make all AIE_VARIANT=1 T_DATA_A=int16  T_DATA_B=cint16 DIM_SIZE=256   NUM_FRAMES=1 SHIFT=14 API_IO=0 UUT_SSR=1   STIM_TYPE=0"
    test_arr[$LINENO]="make all AIE_VARIANT=1 T_DATA_A=cint16 T_DATA_B=cint32 DIM_SIZE=512   NUM_FRAMES=1 SHIFT=14 API_IO=0 UUT_SSR=1   STIM_TYPE=0"
    test_arr[$LINENO]="make all AIE_VARIANT=1 T_DATA_A=int16  T_DATA_B=int16  DIM_SIZE=1024  NUM_FRAMES=1 SHIFT=14 API_IO=0 UUT_SSR=1   STIM_TYPE=0"

    #window sizes
    test_arr[$LINENO]="make all T_DATA_A=cint16 T_DATA_B=cint32 DIM_SIZE=16   NUM_FRAMES=2   SHIFT=14 API_IO=0 UUT_SSR=1   STIM_TYPE=0"
    test_arr[$LINENO]="make all T_DATA_A=cint16 T_DATA_B=cint16 DIM_SIZE=16   NUM_FRAMES=4   SHIFT=14 API_IO=1 UUT_SSR=1   STIM_TYPE=0"
    test_arr[$LINENO]="make all T_DATA_A=int16  T_DATA_B=int16  DIM_SIZE=16   NUM_FRAMES=8   SHIFT=14 API_IO=0 UUT_SSR=1   STIM_TYPE=0"
    test_arr[$LINENO]="make all T_DATA_A=int32  T_DATA_B=int32  DIM_SIZE=16   NUM_FRAMES=16  SHIFT=14 API_IO=1 UUT_SSR=1   STIM_TYPE=0"

    test_arr[$LINENO]="make all T_DATA_A=cint32 T_DATA_B=cint32 DIM_SIZE=16   NUM_FRAMES=2   SHIFT=14 API_IO=0 UUT_SSR=1   STIM_TYPE=0 AIE_VARIANT=2"
    test_arr[$LINENO]="make all T_DATA_A=int16  T_DATA_B=int32  DIM_SIZE=16   NUM_FRAMES=4   SHIFT=14 API_IO=0 UUT_SSR=1   STIM_TYPE=0 AIE_VARIANT=2"
    test_arr[$LINENO]="make all T_DATA_A=int16  T_DATA_B=int16  DIM_SIZE=16   NUM_FRAMES=8   SHIFT=14 API_IO=0 UUT_SSR=1   STIM_TYPE=0 AIE_VARIANT=2"
    test_arr[$LINENO]="make all T_DATA_A=cint16 T_DATA_B=cint16 DIM_SIZE=16   NUM_FRAMES=16  SHIFT=14 API_IO=0 UUT_SSR=1   STIM_TYPE=0 AIE_VARIANT=2"

    #SSR
    test_arr[$LINENO]="make all T_DATA_A=cint16 T_DATA_B=int16  DIM_SIZE=32   NUM_FRAMES=1  SHIFT=14 API_IO=0 UUT_SSR=2   STIM_TYPE=0"
    test_arr[$LINENO]="make all T_DATA_A=cint32 T_DATA_B=cint32 DIM_SIZE=64   NUM_FRAMES=1  SHIFT=14 API_IO=1 UUT_SSR=4   STIM_TYPE=0"
    test_arr[$LINENO]="make all T_DATA_A=int16  T_DATA_B=int16  DIM_SIZE=128  NUM_FRAMES=1  SHIFT=14 API_IO=1 UUT_SSR=8   STIM_TYPE=0"
    test_arr[$LINENO]="make all T_DATA_A=cint32 T_DATA_B=cint32 DIM_SIZE=256  NUM_FRAMES=1  SHIFT=14 API_IO=0 UUT_SSR=16  STIM_TYPE=0"

    test_arr[$LINENO]="make all T_DATA_A=int16  T_DATA_B=int16  DIM_SIZE=32   NUM_FRAMES=1  SHIFT=14 API_IO=0 UUT_SSR=2   STIM_TYPE=0 AIE_VARIANT=2"
    test_arr[$LINENO]="make all T_DATA_A=int32  T_DATA_B=cint32 DIM_SIZE=64   NUM_FRAMES=1  SHIFT=14 API_IO=0 UUT_SSR=4   STIM_TYPE=0 AIE_VARIANT=2"
    test_arr[$LINENO]="make all T_DATA_A=int32  T_DATA_B=int32  DIM_SIZE=128  NUM_FRAMES=1  SHIFT=14 API_IO=0 UUT_SSR=8   STIM_TYPE=0 AIE_VARIANT=2"
    test_arr[$LINENO]="make all T_DATA_A=int32  T_DATA_B=cint32 DIM_SIZE=256  NUM_FRAMES=1  SHIFT=14 API_IO=0 UUT_SSR=16  STIM_TYPE=0 AIE_VARIANT=2"
    #Combinations of parameters
    #APIvsSSR
    test_arr[$LINENO]="make all T_DATA_A=cint16 T_DATA_B=int16 DIM_SIZE=256  NUM_FRAMES=1  SHIFT=14 API_IO=1 UUT_SSR=16  STIM_TYPE=0"
    test_arr[$LINENO]="make all T_DATA_A=cint32 T_DATA_B=cint32 DIM_SIZE=256  NUM_FRAMES=4  SHIFT=30 API_IO=1 UUT_SSR=4  STIM_TYPE=0"

    test_arr[$LINENO]="make all T_DATA_A=cint16 T_DATA_B=int16 DIM_SIZE=256  NUM_FRAMES=1  SHIFT=14 API_IO=0 UUT_SSR=16  STIM_TYPE=0 AIE_VARIANT=2"
    test_arr[$LINENO]="make all T_DATA_A=cint32 T_DATA_B=cint32 DIM_SIZE=256  NUM_FRAMES=4  SHIFT=30 API_IO=0 UUT_SSR=4  STIM_TYPE=0 AIE_VARIANT=2"
fi

if [[ "$*" == *qor* || "$*" == *long*|| "$*" == *all* ]]
then
    test_arr[$LINENO]="make all T_DATA_A=cint16 T_DATA_B=int16  DIM_SIZE=16   NUM_FRAMES=32  SHIFT=14 API_IO=0 UUT_SSR=1   STIM_TYPE=0"
    test_arr[$LINENO]="make all T_DATA_A=cint16 T_DATA_B=int16  DIM_SIZE=16   NUM_FRAMES=64  SHIFT=14 API_IO=0 UUT_SSR=1   STIM_TYPE=0"
    test_arr[$LINENO]="make all T_DATA_A=cint16 T_DATA_B=int32  DIM_SIZE=16   NUM_FRAMES=128 SHIFT=14 API_IO=1 UUT_SSR=1   STIM_TYPE=0"
    test_arr[$LINENO]="make all T_DATA_A=cint32 T_DATA_B=cint32 DIM_SIZE=16   NUM_FRAMES=128 SHIFT=14 API_IO=1 UUT_SSR=1   STIM_TYPE=6"
fi

if [[ "$*" == *PR* ]]
then
    test_arr[$LINENO]="make all T_DATA_A=int32  T_DATA_B=int16   DIM_SIZE=154   NUM_FRAMES=4    SHIFT=14 API_IO=0 UUT_SSR=2   STIM_TYPE=1 TARGET=x86sim"
    test_arr[$LINENO]="make all T_DATA_A=int16  T_DATA_B=cint32  DIM_SIZE=37    NUM_FRAMES=5    SHIFT=14 API_IO=0 UUT_SSR=1   STIM_TYPE=1 TARGET=x86sim"
    test_arr[$LINENO]="make all T_DATA_A=float  T_DATA_B=float   DIM_SIZE=768   NUM_FRAMES=3    SHIFT=0  API_IO=1 UUT_SSR=3   STIM_TYPE=1 TARGET=x86sim"
    test_arr[$LINENO]="make all T_DATA_A=int16  T_DATA_B=int16   DIM_SIZE=20    NUM_FRAMES=4    SHIFT=14 API_IO=1 UUT_SSR=1   STIM_TYPE=1 TARGET=x86sim"
    test_arr[$LINENO]="make all T_DATA_A=cint32 T_DATA_B=int16   DIM_SIZE=256   NUM_FRAMES=8    SHIFT=14 API_IO=0 UUT_SSR=1   STIM_TYPE=3 TARGET=x86sim"
    test_arr[$LINENO]="make all T_DATA_A=int16  T_DATA_B=cint16  DIM_SIZE=512   NUM_FRAMES=24   SHIFT=14 API_IO=1 UUT_SSR=4   STIM_TYPE=6 "
    test_arr[$LINENO]="make all T_DATA_A=float  T_DATA_B=cfloat  DIM_SIZE=256   NUM_FRAMES=2    SHIFT=0  API_IO=0 UUT_SSR=1   STIM_TYPE=0 "
    test_arr[$LINENO]="make all T_DATA_A=int32  T_DATA_B=cint16  DIM_SIZE=24    NUM_FRAMES=16   SHIFT=14 API_IO=0 UUT_SSR=1   STIM_TYPE=0 "
fi

if [[ "$*" == *padding* ]]
then
    test_arr[$LINENO]="make all T_DATA_A=int16  T_DATA_B=int16   DIM_SIZE=19   NUM_FRAMES=4  SHIFT=14 API_IO=1 UUT_SSR=1 STIM_TYPE=1 TARGET=x86sim"
    test_arr[$LINENO]="make all T_DATA_A=int32  T_DATA_B=int16   DIM_SIZE=154  NUM_FRAMES=4  SHIFT=14 API_IO=0 UUT_SSR=2 STIM_TYPE=1 TARGET=x86sim"
    test_arr[$LINENO]="make all T_DATA_A=int16  T_DATA_B=cint32  DIM_SIZE=37   NUM_FRAMES=4  SHIFT=14 API_IO=0 UUT_SSR=1 STIM_TYPE=1 TARGET=x86sim"
    test_arr[$LINENO]="make all T_DATA_A=cint32 T_DATA_B=int16   DIM_SIZE=19   NUM_FRAMES=4  SHIFT=14 API_IO=1 UUT_SSR=1 STIM_TYPE=1 TARGET=x86sim"
    test_arr[$LINENO]="make all T_DATA_A=float  T_DATA_B=cfloat  DIM_SIZE=781  NUM_FRAMES=2  SHIFT=0  API_IO=0 UUT_SSR=1 STIM_TYPE=1 TARGET=x86sim"
    test_arr[$LINENO]="make all T_DATA_A=float  T_DATA_B=float   DIM_SIZE=312  NUM_FRAMES=4  SHIFT=0  API_IO=1 UUT_SSR=3 STIM_TYPE=1 TARGET=x86sim"
    test_arr[$LINENO]="make all T_DATA_A=cint32 T_DATA_B=cint32  DIM_SIZE=35   NUM_FRAMES=4  SHIFT=14 API_IO=0 UUT_SSR=1 STIM_TYPE=1 TARGET=x86sim"
    test_arr[$LINENO]="make all T_DATA_A=cint16 T_DATA_B=cint16  DIM_SIZE=19   NUM_FRAMES=4  SHIFT=14 API_IO=1 UUT_SSR=1 STIM_TYPE=1 TARGET=x86sim"
    test_arr[$LINENO]="make all T_DATA_A=int32  T_DATA_B=cint16  DIM_SIZE=76   NUM_FRAMES=4  SHIFT=14 API_IO=0 UUT_SSR=2 STIM_TYPE=1 TARGET=x86sim"
    test_arr[$LINENO]="make all T_DATA_A=int32  T_DATA_B=cint32  DIM_SIZE=600  NUM_FRAMES=4  SHIFT=14 API_IO=1 UUT_SSR=8 STIM_TYPE=1 TARGET=x86sim"

fi
if [[ "$*" == *stackhunt*  ]]
then
    test_arr[$LINENO]="make all T_DATA_A=int16  T_DATA_B=int16  DIM_SIZE=16  NUM_FRAMES=1  SHIFT=14  API_IO=0 UUT_SSR=1 STIM_TYPE=1"
    test_arr[$LINENO]="make all T_DATA_A=int16  T_DATA_B=int16  DIM_SIZE=16  NUM_FRAMES=1  SHIFT=14  API_IO=0 UUT_SSR=1 STIM_TYPE=1"
    test_arr[$LINENO]="make all T_DATA_A=int16  T_DATA_B=int16  DIM_SIZE=16  NUM_FRAMES=1  SHIFT=14  API_IO=0 UUT_SSR=1 STIM_TYPE=1"
    test_arr[$LINENO]="make all T_DATA_A=int16  T_DATA_B=int16  DIM_SIZE=16  NUM_FRAMES=1  SHIFT=14  API_IO=0 UUT_SSR=1 STIM_TYPE=1"
    test_arr[$LINENO]="make all T_DATA_A=int16  T_DATA_B=int16  DIM_SIZE=16  NUM_FRAMES=1  SHIFT=14  API_IO=0 UUT_SSR=1 STIM_TYPE=1"
    test_arr[$LINENO]="make all T_DATA_A=int16  T_DATA_B=int16  DIM_SIZE=16  NUM_FRAMES=1  SHIFT=14  API_IO=0 UUT_SSR=1 STIM_TYPE=1"

    # test_arr[$LINENO]="make all T_DATA_A=int16  T_DATA_B=int16  DIM_SIZE=64    NUM_FRAMES=1024 SHIFT=14 API_IO=0 UUT_SSR=1   STIM_TYPE=6"
    # test_arr[$LINENO]="make all T_DATA_A=cint32 T_DATA_B=int32  DIM_SIZE=128   NUM_FRAMES=1024 SHIFT=14 API_IO=0 UUT_SSR=1   STIM_TYPE=6"
    # test_arr[$LINENO]="make all T_DATA_A=cint16 T_DATA_B=cint32 DIM_SIZE=256   NUM_FRAMES=1024 SHIFT=14 API_IO=0 UUT_SSR=1   STIM_TYPE=6"

    # test_arr[$LINENO]="make all T_DATA_A=cint32 T_DATA_B=int32  DIM_SIZE=64    NUM_FRAMES=64   SHIFT=30 API_IO=0 UUT_SSR=1   STIM_TYPE=6"
    # test_arr[$LINENO]="make all T_DATA_A=int32  T_DATA_B=int32  DIM_SIZE=128   NUM_FRAMES=128  SHIFT=30 API_IO=0 UUT_SSR=1   STIM_TYPE=6"
    # test_arr[$LINENO]="make all T_DATA_A=cint32 T_DATA_B=cint32 DIM_SIZE=256   NUM_FRAMES=256  SHIFT=30 API_IO=0 UUT_SSR=1   STIM_TYPE=6"

    # test_arr[$LINENO]="make all T_DATA_A=cfloat T_DATA_B=float DIM_SIZE=64    NUM_FRAMES=64   SHIFT=0 API_IO=0 UUT_SSR=1   STIM_TYPE=6"
    # test_arr[$LINENO]="make all T_DATA_A=cfloat T_DATA_B=float DIM_SIZE=128   NUM_FRAMES=128  SHIFT=0 API_IO=0 UUT_SSR=1   STIM_TYPE=6"
    # test_arr[$LINENO]="make all T_DATA_A=cfloat T_DATA_B=float DIM_SIZE=256   NUM_FRAMES=256  SHIFT=0 API_IO=0 UUT_SSR=1   STIM_TYPE=6"
fi



if [[ "$*" == *pwr* ]]
then
    # test_arr[$LINENO]="make all T_DATA_A=cint16 T_DATA_B=int16  DIM_SIZE=512  NUM_FRAMES=2  SHIFT=14 API_IO=1 UUT_SSR=1 DUMP_VCD=1"
    test_arr[$LINENO]="make all T_DATA_A=int16  T_DATA_B=cint16  DIM_SIZE=32    NUM_FRAMES=1   SHIFT=14 API_IO=0 UUT_SSR=1  ROUND_MODE=0 SAT_MODE=0  STIM_TYPE=0 AIE_VARIANT=2 TARGET=x86sim"

    # test_arr[$LINENO]="make all T_DATA_A=cint16 T_DATA_B=int16  DIM_SIZE=512  NUM_FRAMES=4096  SHIFT=14 API_IO=1 UUT_SSR=2 DUMP_VCD=1"
    # test_arr[$LINENO]="make all T_DATA_A=cint16 T_DATA_B=int16  DIM_SIZE=512  NUM_FRAMES=4096  SHIFT=14 API_IO=1 UUT_SSR=4 DUMP_VCD=1"
    # test_arr[$LINENO]="make all T_DATA_A=cint16 T_DATA_B=int16  DIM_SIZE=512  NUM_FRAMES=4096  SHIFT=14 API_IO=1 UUT_SSR=8 DUMP_VCD=1"
    # test_arr[$LINENO]="make all T_DATA_A=cint16 T_DATA_B=int16  DIM_SIZE=512  NUM_FRAMES=4096  SHIFT=14 API_IO=1 UUT_SSR=16 DUMP_VCD=1"
    # test_arr[$LINENO]="make all T_DATA_A=cint32 T_DATA_B=cint16 DIM_SIZE=16   NUM_FRAMES=512  SHIFT=14 API_IO=0 UUT_SSR=1 DUMP_VCD=1"
fi

# Randomized testing
if [[ "$*" == *randomized* ]]
then
    # read from randomized test file
    while IFS= read -r line; do
        test_arr[${#test_arr[*]}]="$line"
    done < randomized_tests.txt
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
