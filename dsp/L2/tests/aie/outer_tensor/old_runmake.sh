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

test_arr=()

if [[ "$*" == *smoke* ]]
then
    # test_arr[$LINENO]="make all AIE_VARIANT=1 "
    # test_arr[$LINENO]="make all AIE_VARIANT=2 "
    # test_arr[$LINENO]="make all AIE_VARIANT=22 "
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=2 T_DATA_A=int16 T_DATA_B=int16 API_IO=1 UUT_SSR=1 DIM_SIZE_A=8192 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=0"
fi


if [[ "$*" == *single_buf*  ||  "$*" == *checkin*  ||  "$*" == *qor* ]]
then
    # testing range of shifts
    if [[ "$*" == *qor* ]]
    then
        niters=16
    else
        niters=4
    fi
    test_arr[$LINENO]="make all T_DATA_A=cint32 T_DATA_B=cint32   DIM_SIZE_A=8    DIM_SIZE_B=4    NUM_FRAMES=8  SHIFT=0 API_IO=0 UUT_SSR=2   STIM_TYPE=0 NITER=$niters  AIE_VARIANT=1  SINGLE_BUF=1"
    test_arr[$LINENO]="make all T_DATA_A=float  T_DATA_B=float    DIM_SIZE_A=16   DIM_SIZE_B=16   NUM_FRAMES=8  SHIFT=0 API_IO=0 UUT_SSR=2   STIM_TYPE=0 NITER=$niters  AIE_VARIANT=1  SINGLE_BUF=1"
    test_arr[$LINENO]="make all T_DATA_A=cint32 T_DATA_B=cint32   DIM_SIZE_A=16   DIM_SIZE_B=16   NUM_FRAMES=8  SHIFT=0 API_IO=0 UUT_SSR=4   STIM_TYPE=0 NITER=$niters  AIE_VARIANT=2  SINGLE_BUF=1"
    test_arr[$LINENO]="make all T_DATA_A=cint32 T_DATA_B=cint16   DIM_SIZE_A=16   DIM_SIZE_B=16   NUM_FRAMES=8  SHIFT=0 API_IO=1 UUT_SSR=2   STIM_TYPE=0 NITER=$niters  AIE_VARIANT=2  SINGLE_BUF=1"
    test_arr[$LINENO]="make all T_DATA_A=int32  T_DATA_B=int32    DIM_SIZE_A=32   DIM_SIZE_B=16   NUM_FRAMES=8  SHIFT=0 API_IO=1 UUT_SSR=4   STIM_TYPE=0 NITER=$niters  AIE_VARIANT=2  SINGLE_BUF=1"
fi

if [[  "$*" == *rndsat* || "$*" == *checkin* || "$*" == *qor* ]]
then
    # testing rounding and saturation variations...
    if [[ "$*" == *qor* ]]
    then
        niters=16
    else
        niters=4
    fi
    test_arr[$LINENO]="make all T_DATA_A=int32 T_DATA_B=int32    DIM_SIZE_A=32  DIM_SIZE_B=32  NUM_FRAMES=4 SHIFT=5 API_IO=0 UUT_SSR=1 ROUND_MODE=1  SAT_MODE=1 STIM_TYPE=0 NITER=$niters AIE_VARIANT=1 TARGET=hw"
    test_arr[$LINENO]="make all T_DATA_A=int32 T_DATA_B=int32    DIM_SIZE_A=32  DIM_SIZE_B=32  NUM_FRAMES=4 SHIFT=5 API_IO=0 UUT_SSR=1 ROUND_MODE=2  SAT_MODE=1 STIM_TYPE=0 NITER=$niters AIE_VARIANT=1 TARGET=hw"
    test_arr[$LINENO]="make all T_DATA_A=int32 T_DATA_B=int32    DIM_SIZE_A=32  DIM_SIZE_B=32  NUM_FRAMES=4 SHIFT=5 API_IO=0 UUT_SSR=1 ROUND_MODE=3  SAT_MODE=1 STIM_TYPE=0 NITER=$niters AIE_VARIANT=1 TARGET=hw"
    test_arr[$LINENO]="make all T_DATA_A=int32 T_DATA_B=int32    DIM_SIZE_A=32  DIM_SIZE_B=32  NUM_FRAMES=4 SHIFT=5 API_IO=0 UUT_SSR=1 ROUND_MODE=4  SAT_MODE=1 STIM_TYPE=0 NITER=$niters AIE_VARIANT=1 TARGET=hw"
    test_arr[$LINENO]="make all T_DATA_A=int32 T_DATA_B=int32    DIM_SIZE_A=32  DIM_SIZE_B=32  NUM_FRAMES=4 SHIFT=5 API_IO=0 UUT_SSR=1 ROUND_MODE=5  SAT_MODE=1 STIM_TYPE=0 NITER=$niters AIE_VARIANT=1 TARGET=hw"
    test_arr[$LINENO]="make all T_DATA_A=int32 T_DATA_B=int32    DIM_SIZE_A=32  DIM_SIZE_B=32  NUM_FRAMES=4 SHIFT=5 API_IO=0 UUT_SSR=1 ROUND_MODE=6  SAT_MODE=1 STIM_TYPE=0 NITER=$niters AIE_VARIANT=1 TARGET=hw"
    test_arr[$LINENO]="make all T_DATA_A=int32 T_DATA_B=int32    DIM_SIZE_A=32  DIM_SIZE_B=32  NUM_FRAMES=4 SHIFT=5 API_IO=0 UUT_SSR=1 ROUND_MODE=7  SAT_MODE=1 STIM_TYPE=0 NITER=$niters AIE_VARIANT=1 TARGET=hw"
    test_arr[$LINENO]="make all T_DATA_A=int32 T_DATA_B=int32    DIM_SIZE_A=32  DIM_SIZE_B=32  NUM_FRAMES=4 SHIFT=5 API_IO=0 UUT_SSR=1 ROUND_MODE=0  SAT_MODE=1 STIM_TYPE=0 NITER=$niters AIE_VARIANT=1 TARGET=hw"
    test_arr[$LINENO]="make all T_DATA_A=int32 T_DATA_B=int32    DIM_SIZE_A=32  DIM_SIZE_B=32  NUM_FRAMES=4 SHIFT=5 API_IO=0 UUT_SSR=1 ROUND_MODE=0  SAT_MODE=0 STIM_TYPE=0 NITER=$niters AIE_VARIANT=1 TARGET=hw"
    test_arr[$LINENO]="make all T_DATA_A=int32 T_DATA_B=int32    DIM_SIZE_A=32  DIM_SIZE_B=32  NUM_FRAMES=4 SHIFT=5 API_IO=0 UUT_SSR=1 ROUND_MODE=0  SAT_MODE=1 STIM_TYPE=0 NITER=$niters AIE_VARIANT=1 TARGET=hw"
    test_arr[$LINENO]="make all T_DATA_A=int32 T_DATA_B=int32    DIM_SIZE_A=32  DIM_SIZE_B=32  NUM_FRAMES=4 SHIFT=5 API_IO=0 UUT_SSR=1 ROUND_MODE=0  SAT_MODE=3 STIM_TYPE=0 NITER=$niters AIE_VARIANT=1 TARGET=hw"
AIE_VARIANT=1
    test_arr[$LINENO]="make all T_DATA_A=int32 T_DATA_B=int32    DIM_SIZE_A=32  DIM_SIZE_B=32  NUM_FRAMES=4 SHIFT=5 API_IO=0 UUT_SSR=1 ROUND_MODE=0  SAT_MODE=1 STIM_TYPE=0 NITER=$niters AIE_VARIANT=2 TARGET=hw"
    test_arr[$LINENO]="make all T_DATA_A=int32 T_DATA_B=int32    DIM_SIZE_A=32  DIM_SIZE_B=32  NUM_FRAMES=4 SHIFT=5 API_IO=0 UUT_SSR=1 ROUND_MODE=1  SAT_MODE=1 STIM_TYPE=0 NITER=$niters AIE_VARIANT=2 TARGET=hw"
    test_arr[$LINENO]="make all T_DATA_A=int32 T_DATA_B=int32    DIM_SIZE_A=32  DIM_SIZE_B=32  NUM_FRAMES=4 SHIFT=5 API_IO=0 UUT_SSR=1 ROUND_MODE=2  SAT_MODE=1 STIM_TYPE=0 NITER=$niters AIE_VARIANT=2 TARGET=hw"
    test_arr[$LINENO]="make all T_DATA_A=int32 T_DATA_B=int32    DIM_SIZE_A=32  DIM_SIZE_B=32  NUM_FRAMES=4 SHIFT=5 API_IO=0 UUT_SSR=1 ROUND_MODE=3  SAT_MODE=1 STIM_TYPE=0 NITER=$niters AIE_VARIANT=2 TARGET=hw"
    test_arr[$LINENO]="make all T_DATA_A=int32 T_DATA_B=int32    DIM_SIZE_A=32  DIM_SIZE_B=32  NUM_FRAMES=4 SHIFT=5 API_IO=0 UUT_SSR=1 ROUND_MODE=8  SAT_MODE=1 STIM_TYPE=0 NITER=$niters AIE_VARIANT=2 TARGET=hw"
    test_arr[$LINENO]="make all T_DATA_A=int32 T_DATA_B=int32    DIM_SIZE_A=32  DIM_SIZE_B=32  NUM_FRAMES=4 SHIFT=5 API_IO=0 UUT_SSR=1 ROUND_MODE=9  SAT_MODE=1 STIM_TYPE=0 NITER=$niters AIE_VARIANT=2 TARGET=hw"
    test_arr[$LINENO]="make all T_DATA_A=int32 T_DATA_B=int32    DIM_SIZE_A=32  DIM_SIZE_B=32  NUM_FRAMES=4 SHIFT=5 API_IO=0 UUT_SSR=1 ROUND_MODE=10 SAT_MODE=0 STIM_TYPE=0 NITER=$niters AIE_VARIANT=2 TARGET=hw"
    test_arr[$LINENO]="make all T_DATA_A=int32 T_DATA_B=int32    DIM_SIZE_A=32  DIM_SIZE_B=32  NUM_FRAMES=4 SHIFT=5 API_IO=0 UUT_SSR=1 ROUND_MODE=11 SAT_MODE=1 STIM_TYPE=0 NITER=$niters AIE_VARIANT=2 TARGET=hw"
    test_arr[$LINENO]="make all T_DATA_A=int32 T_DATA_B=int32    DIM_SIZE_A=32  DIM_SIZE_B=32  NUM_FRAMES=4 SHIFT=5 API_IO=0 UUT_SSR=1 ROUND_MODE=12 SAT_MODE=3 STIM_TYPE=0 NITER=$niters AIE_VARIANT=2 TARGET=hw"
fi

if [[  "$*" == *datatypes* || "$*" == *checkin* || "$*" == *qor* ]]
then
    # testing all valid datatype combinations...
    if [[ "$*" == *qor* ]]
    then
        niters=16
    else
        niters=4
    fi
    test_arr[$LINENO]="make all T_DATA_A=cint32 T_DATA_B=cint32     DIM_SIZE_A=32  DIM_SIZE_B=32  NUM_FRAMES=1 SHIFT=0 API_IO=0 UUT_SSR=1   STIM_TYPE=0 NITER=$niters   AIE_VARIANT=1  TARGET=hw"
    test_arr[$LINENO]="make all T_DATA_A=cint32 T_DATA_B=cint16     DIM_SIZE_A=32  DIM_SIZE_B=32  NUM_FRAMES=1 SHIFT=0 API_IO=0 UUT_SSR=1   STIM_TYPE=0 NITER=$niters   AIE_VARIANT=1  TARGET=hw"
    test_arr[$LINENO]="make all T_DATA_A=cint32 T_DATA_B=int32      DIM_SIZE_A=32  DIM_SIZE_B=32  NUM_FRAMES=1 SHIFT=0 API_IO=0 UUT_SSR=1   STIM_TYPE=0 NITER=$niters   AIE_VARIANT=1  TARGET=hw"
    test_arr[$LINENO]="make all T_DATA_A=cint32 T_DATA_B=int16      DIM_SIZE_A=32  DIM_SIZE_B=32  NUM_FRAMES=1 SHIFT=0 API_IO=0 UUT_SSR=1   STIM_TYPE=0 NITER=$niters   AIE_VARIANT=1  TARGET=hw"
    test_arr[$LINENO]="make all T_DATA_A=cint16 T_DATA_B=cint32     DIM_SIZE_A=32  DIM_SIZE_B=32  NUM_FRAMES=1 SHIFT=0 API_IO=0 UUT_SSR=1   STIM_TYPE=0 NITER=$niters   AIE_VARIANT=1  TARGET=hw"
    test_arr[$LINENO]="make all T_DATA_A=cint16 T_DATA_B=cint16     DIM_SIZE_A=32  DIM_SIZE_B=32  NUM_FRAMES=1 SHIFT=0 API_IO=0 UUT_SSR=1   STIM_TYPE=0 NITER=$niters   AIE_VARIANT=1  TARGET=hw"
    test_arr[$LINENO]="make all T_DATA_A=cint16 T_DATA_B=int32      DIM_SIZE_A=32  DIM_SIZE_B=32  NUM_FRAMES=1 SHIFT=0 API_IO=0 UUT_SSR=1   STIM_TYPE=0 NITER=$niters   AIE_VARIANT=1  TARGET=hw"
    test_arr[$LINENO]="make all T_DATA_A=cint16 T_DATA_B=int16      DIM_SIZE_A=32  DIM_SIZE_B=32  NUM_FRAMES=1 SHIFT=0 API_IO=0 UUT_SSR=1   STIM_TYPE=0 NITER=$niters   AIE_VARIANT=1  TARGET=hw"
    test_arr[$LINENO]="make all T_DATA_A=int32  T_DATA_B=cint32     DIM_SIZE_A=32  DIM_SIZE_B=32  NUM_FRAMES=1 SHIFT=0 API_IO=0 UUT_SSR=1   STIM_TYPE=0 NITER=$niters   AIE_VARIANT=1  TARGET=hw"
    test_arr[$LINENO]="make all T_DATA_A=int32  T_DATA_B=cint16     DIM_SIZE_A=32  DIM_SIZE_B=32  NUM_FRAMES=1 SHIFT=0 API_IO=0 UUT_SSR=1   STIM_TYPE=0 NITER=$niters   AIE_VARIANT=1  TARGET=hw"
    test_arr[$LINENO]="make all T_DATA_A=int32  T_DATA_B=int32      DIM_SIZE_A=32  DIM_SIZE_B=32  NUM_FRAMES=1 SHIFT=0 API_IO=0 UUT_SSR=1   STIM_TYPE=0 NITER=$niters   AIE_VARIANT=1  TARGET=hw"
    test_arr[$LINENO]="make all T_DATA_A=int32  T_DATA_B=int16      DIM_SIZE_A=32  DIM_SIZE_B=32  NUM_FRAMES=1 SHIFT=0 API_IO=0 UUT_SSR=1   STIM_TYPE=0 NITER=$niters   AIE_VARIANT=1  TARGET=hw"
    test_arr[$LINENO]="make all T_DATA_A=int16  T_DATA_B=cint32     DIM_SIZE_A=32  DIM_SIZE_B=32  NUM_FRAMES=1 SHIFT=0 API_IO=0 UUT_SSR=1   STIM_TYPE=0 NITER=$niters   AIE_VARIANT=1  TARGET=hw"
    test_arr[$LINENO]="make all T_DATA_A=int16  T_DATA_B=cint16     DIM_SIZE_A=32  DIM_SIZE_B=32  NUM_FRAMES=1 SHIFT=0 API_IO=0 UUT_SSR=1   STIM_TYPE=0 NITER=$niters   AIE_VARIANT=1  TARGET=hw"
    test_arr[$LINENO]="make all T_DATA_A=int16  T_DATA_B=int32      DIM_SIZE_A=32  DIM_SIZE_B=32  NUM_FRAMES=1 SHIFT=0 API_IO=0 UUT_SSR=1   STIM_TYPE=0 NITER=$niters   AIE_VARIANT=1  TARGET=hw"
    test_arr[$LINENO]="make all T_DATA_A=int16  T_DATA_B=int16      DIM_SIZE_A=32  DIM_SIZE_B=32  NUM_FRAMES=1 SHIFT=0 API_IO=0 UUT_SSR=1   STIM_TYPE=0 NITER=$niters   AIE_VARIANT=1  TARGET=hw"
    test_arr[$LINENO]="make all T_DATA_A=float  T_DATA_B=float      DIM_SIZE_A=32  DIM_SIZE_B=32  NUM_FRAMES=1 SHIFT=0 API_IO=0 UUT_SSR=1   STIM_TYPE=0 NITER=$niters   AIE_VARIANT=1  TARGET=hw"
    test_arr[$LINENO]="make all T_DATA_A=float  T_DATA_B=cfloat     DIM_SIZE_A=32  DIM_SIZE_B=32  NUM_FRAMES=1 SHIFT=0 API_IO=0 UUT_SSR=1   STIM_TYPE=0 NITER=$niters   AIE_VARIANT=1  TARGET=hw"
    test_arr[$LINENO]="make all T_DATA_A=cfloat T_DATA_B=float      DIM_SIZE_A=32  DIM_SIZE_B=32  NUM_FRAMES=1 SHIFT=0 API_IO=0 UUT_SSR=1   STIM_TYPE=0 NITER=$niters   AIE_VARIANT=1  TARGET=hw"
    test_arr[$LINENO]="make all T_DATA_A=cfloat T_DATA_B=cfloat     DIM_SIZE_A=32  DIM_SIZE_B=32  NUM_FRAMES=1 SHIFT=0 API_IO=0 UUT_SSR=1   STIM_TYPE=0 NITER=$niters   AIE_VARIANT=1  TARGET=hw"

    test_arr[$LINENO]="make all T_DATA_A=cint32 T_DATA_B=cint32     DIM_SIZE_A=32  DIM_SIZE_B=32  NUM_FRAMES=1 SHIFT=0 API_IO=0 UUT_SSR=1   STIM_TYPE=0 NITER=$niters   AIE_VARIANT=2  TARGET=hw"
    test_arr[$LINENO]="make all T_DATA_A=cint32 T_DATA_B=cint16     DIM_SIZE_A=32  DIM_SIZE_B=32  NUM_FRAMES=1 SHIFT=0 API_IO=0 UUT_SSR=1   STIM_TYPE=0 NITER=$niters   AIE_VARIANT=2  TARGET=hw"
    test_arr[$LINENO]="make all T_DATA_A=cint32 T_DATA_B=int32      DIM_SIZE_A=32  DIM_SIZE_B=32  NUM_FRAMES=1 SHIFT=0 API_IO=0 UUT_SSR=1   STIM_TYPE=0 NITER=$niters   AIE_VARIANT=2  TARGET=hw"
    test_arr[$LINENO]="make all T_DATA_A=cint32 T_DATA_B=int16      DIM_SIZE_A=32  DIM_SIZE_B=32  NUM_FRAMES=1 SHIFT=0 API_IO=0 UUT_SSR=1   STIM_TYPE=0 NITER=$niters   AIE_VARIANT=2  TARGET=hw"
    test_arr[$LINENO]="make all T_DATA_A=cint16 T_DATA_B=cint32     DIM_SIZE_A=32  DIM_SIZE_B=32  NUM_FRAMES=1 SHIFT=0 API_IO=0 UUT_SSR=1   STIM_TYPE=0 NITER=$niters   AIE_VARIANT=2  TARGET=hw"
    test_arr[$LINENO]="make all T_DATA_A=cint16 T_DATA_B=cint16     DIM_SIZE_A=32  DIM_SIZE_B=32  NUM_FRAMES=1 SHIFT=0 API_IO=0 UUT_SSR=1   STIM_TYPE=0 NITER=$niters   AIE_VARIANT=2  TARGET=hw"
    test_arr[$LINENO]="make all T_DATA_A=cint16 T_DATA_B=int32      DIM_SIZE_A=32  DIM_SIZE_B=32  NUM_FRAMES=1 SHIFT=0 API_IO=0 UUT_SSR=1   STIM_TYPE=0 NITER=$niters   AIE_VARIANT=2  TARGET=hw"
    test_arr[$LINENO]="make all T_DATA_A=cint16 T_DATA_B=int16      DIM_SIZE_A=32  DIM_SIZE_B=32  NUM_FRAMES=1 SHIFT=0 API_IO=0 UUT_SSR=1   STIM_TYPE=0 NITER=$niters   AIE_VARIANT=2  TARGET=hw"
    test_arr[$LINENO]="make all T_DATA_A=int32  T_DATA_B=cint32     DIM_SIZE_A=32  DIM_SIZE_B=32  NUM_FRAMES=1 SHIFT=0 API_IO=0 UUT_SSR=1   STIM_TYPE=0 NITER=$niters   AIE_VARIANT=2  TARGET=hw"
    test_arr[$LINENO]="make all T_DATA_A=int32  T_DATA_B=cint16     DIM_SIZE_A=32  DIM_SIZE_B=32  NUM_FRAMES=1 SHIFT=0 API_IO=0 UUT_SSR=1   STIM_TYPE=0 NITER=$niters   AIE_VARIANT=2  TARGET=hw"
    test_arr[$LINENO]="make all T_DATA_A=int32  T_DATA_B=int32      DIM_SIZE_A=32  DIM_SIZE_B=32  NUM_FRAMES=1 SHIFT=0 API_IO=0 UUT_SSR=1   STIM_TYPE=0 NITER=$niters   AIE_VARIANT=2  TARGET=hw"
    test_arr[$LINENO]="make all T_DATA_A=int32  T_DATA_B=int16      DIM_SIZE_A=32  DIM_SIZE_B=32  NUM_FRAMES=1 SHIFT=0 API_IO=0 UUT_SSR=1   STIM_TYPE=0 NITER=$niters   AIE_VARIANT=2  TARGET=hw"
    test_arr[$LINENO]="make all T_DATA_A=int16  T_DATA_B=cint32     DIM_SIZE_A=32  DIM_SIZE_B=32  NUM_FRAMES=1 SHIFT=0 API_IO=0 UUT_SSR=1   STIM_TYPE=0 NITER=$niters   AIE_VARIANT=2  TARGET=hw"
    test_arr[$LINENO]="make all T_DATA_A=int16  T_DATA_B=cint16     DIM_SIZE_A=32  DIM_SIZE_B=32  NUM_FRAMES=1 SHIFT=0 API_IO=0 UUT_SSR=1   STIM_TYPE=0 NITER=$niters   AIE_VARIANT=2  TARGET=hw"
    test_arr[$LINENO]="make all T_DATA_A=int16  T_DATA_B=int32      DIM_SIZE_A=32  DIM_SIZE_B=32  NUM_FRAMES=1 SHIFT=0 API_IO=0 UUT_SSR=1   STIM_TYPE=0 NITER=$niters   AIE_VARIANT=2  TARGET=hw"
    test_arr[$LINENO]="make all T_DATA_A=int16  T_DATA_B=int16      DIM_SIZE_A=32  DIM_SIZE_B=32  NUM_FRAMES=1 SHIFT=0 API_IO=0 UUT_SSR=1   STIM_TYPE=0 NITER=$niters   AIE_VARIANT=2  TARGET=hw"
    test_arr[$LINENO]="make all T_DATA_A=float  T_DATA_B=float      DIM_SIZE_A=32  DIM_SIZE_B=32  NUM_FRAMES=1 SHIFT=0 API_IO=0 UUT_SSR=1   STIM_TYPE=0 NITER=$niters   AIE_VARIANT=2  TARGET=hw"
    test_arr[$LINENO]="make all T_DATA_A=float  T_DATA_B=cfloat     DIM_SIZE_A=32  DIM_SIZE_B=32  NUM_FRAMES=1 SHIFT=0 API_IO=0 UUT_SSR=1   STIM_TYPE=0 NITER=$niters   AIE_VARIANT=2  TARGET=hw"
    test_arr[$LINENO]="make all T_DATA_A=cfloat T_DATA_B=float      DIM_SIZE_A=32  DIM_SIZE_B=32  NUM_FRAMES=1 SHIFT=0 API_IO=0 UUT_SSR=1   STIM_TYPE=0 NITER=$niters   AIE_VARIANT=2  TARGET=hw"
    test_arr[$LINENO]="make all T_DATA_A=cfloat T_DATA_B=cfloat     DIM_SIZE_A=32  DIM_SIZE_B=32  NUM_FRAMES=1 SHIFT=0 API_IO=0 UUT_SSR=1   STIM_TYPE=0 NITER=$niters   AIE_VARIANT=2  TARGET=hw"
fi

if [[ "$*" == *num_frames* || "$*" == *checkin* || "$*" == *qor* ]]
then
    # testing range of num_frames...
    if [[ "$*" == *qor* ]]
    then
        niters=16
    else
        niters=4
    fi
    test_arr[$LINENO]="make all T_DATA_A=cint32 T_DATA_B=cint32     DIM_SIZE_A=4   DIM_SIZE_B=4   NUM_FRAMES=2  SHIFT=0 API_IO=0 UUT_SSR=1   STIM_TYPE=0 NITER=$niters  AIE_VARIANT=1   TARGET=hw"
    test_arr[$LINENO]="make all T_DATA_A=cint32 T_DATA_B=cint32     DIM_SIZE_A=16  DIM_SIZE_B=16  NUM_FRAMES=2  SHIFT=0 API_IO=0 UUT_SSR=1   STIM_TYPE=0 NITER=$niters  AIE_VARIANT=1   TARGET=hw"
    test_arr[$LINENO]="make all T_DATA_A=cint32 T_DATA_B=cint16     DIM_SIZE_A=16  DIM_SIZE_B=16  NUM_FRAMES=4  SHIFT=0 API_IO=0 UUT_SSR=1   STIM_TYPE=0 NITER=$niters  AIE_VARIANT=1   TARGET=hw"
    test_arr[$LINENO]="make all T_DATA_A=cint32 T_DATA_B=int32      DIM_SIZE_A=16  DIM_SIZE_B=16  NUM_FRAMES=4  SHIFT=0 API_IO=0 UUT_SSR=1   STIM_TYPE=0 NITER=$niters  AIE_VARIANT=1   TARGET=hw"
    test_arr[$LINENO]="make all T_DATA_A=cint16 T_DATA_B=int16      DIM_SIZE_A=16  DIM_SIZE_B=16  NUM_FRAMES=8  SHIFT=0 API_IO=0 UUT_SSR=1   STIM_TYPE=0 NITER=$niters  AIE_VARIANT=1   TARGET=hw"
    test_arr[$LINENO]="make all T_DATA_A=cint16 T_DATA_B=cint16     DIM_SIZE_A=16  DIM_SIZE_B=16  NUM_FRAMES=16 SHIFT=0 API_IO=0 UUT_SSR=1   STIM_TYPE=0 NITER=$niters  AIE_VARIANT=1   TARGET=hw"
    test_arr[$LINENO]="make all T_DATA_A=cint16 T_DATA_B=cint16     DIM_SIZE_A=16  DIM_SIZE_B=16  NUM_FRAMES=2  SHIFT=0 API_IO=0 UUT_SSR=1   STIM_TYPE=0 NITER=$niters  AIE_VARIANT=1   TARGET=hw"
    test_arr[$LINENO]="make all T_DATA_A=cint16 T_DATA_B=int32      DIM_SIZE_A=16  DIM_SIZE_B=16  NUM_FRAMES=2  SHIFT=0 API_IO=0 UUT_SSR=1   STIM_TYPE=0 NITER=$niters  AIE_VARIANT=1   TARGET=hw"
    test_arr[$LINENO]="make all T_DATA_A=int16  T_DATA_B=int16      DIM_SIZE_A=16  DIM_SIZE_B=16  NUM_FRAMES=8  SHIFT=0 API_IO=0 UUT_SSR=1   STIM_TYPE=0 NITER=$niters  AIE_VARIANT=1   TARGET=hw"
    test_arr[$LINENO]="make all T_DATA_A=int16  T_DATA_B=cint32     DIM_SIZE_A=16  DIM_SIZE_B=16  NUM_FRAMES=4  SHIFT=0 API_IO=0 UUT_SSR=1   STIM_TYPE=0 NITER=$niters  AIE_VARIANT=1   TARGET=hw"
    test_arr[$LINENO]="make all T_DATA_A=cint32 T_DATA_B=int16      DIM_SIZE_A=16  DIM_SIZE_B=16  NUM_FRAMES=4  SHIFT=0 API_IO=0 UUT_SSR=1   STIM_TYPE=0 NITER=$niters  AIE_VARIANT=1   TARGET=hw"
    test_arr[$LINENO]="make all T_DATA_A=int16  T_DATA_B=int16      DIM_SIZE_A=16  DIM_SIZE_B=16  NUM_FRAMES=32 SHIFT=0 API_IO=0 UUT_SSR=1   STIM_TYPE=0 NITER=$niters  AIE_VARIANT=1   TARGET=hw"

    test_arr[$LINENO]="make all T_DATA_A=cint32 T_DATA_B=cint32     DIM_SIZE_A=4   DIM_SIZE_B=4   NUM_FRAMES=2  SHIFT=0 API_IO=0 UUT_SSR=1   STIM_TYPE=0 NITER=$niters  AIE_VARIANT=2   TARGET=hw"
    test_arr[$LINENO]="make all T_DATA_A=cint32 T_DATA_B=cint32     DIM_SIZE_A=16  DIM_SIZE_B=16  NUM_FRAMES=2  SHIFT=0 API_IO=0 UUT_SSR=1   STIM_TYPE=0 NITER=$niters  AIE_VARIANT=2   TARGET=hw"
    test_arr[$LINENO]="make all T_DATA_A=cint32 T_DATA_B=cint16     DIM_SIZE_A=16  DIM_SIZE_B=16  NUM_FRAMES=4  SHIFT=0 API_IO=0 UUT_SSR=1   STIM_TYPE=0 NITER=$niters  AIE_VARIANT=2   TARGET=hw"
    test_arr[$LINENO]="make all T_DATA_A=cint32 T_DATA_B=int32      DIM_SIZE_A=16  DIM_SIZE_B=16  NUM_FRAMES=4  SHIFT=0 API_IO=0 UUT_SSR=1   STIM_TYPE=0 NITER=$niters  AIE_VARIANT=2   TARGET=hw"
    test_arr[$LINENO]="make all T_DATA_A=cint16 T_DATA_B=int16      DIM_SIZE_A=16  DIM_SIZE_B=16  NUM_FRAMES=8  SHIFT=0 API_IO=0 UUT_SSR=1   STIM_TYPE=0 NITER=$niters  AIE_VARIANT=2   TARGET=hw"
    test_arr[$LINENO]="make all T_DATA_A=cint16 T_DATA_B=cint16     DIM_SIZE_A=16  DIM_SIZE_B=16  NUM_FRAMES=16 SHIFT=0 API_IO=0 UUT_SSR=1   STIM_TYPE=0 NITER=$niters  AIE_VARIANT=2   TARGET=hw"
    test_arr[$LINENO]="make all T_DATA_A=cint16 T_DATA_B=cint16     DIM_SIZE_A=16  DIM_SIZE_B=16  NUM_FRAMES=2  SHIFT=0 API_IO=0 UUT_SSR=1   STIM_TYPE=0 NITER=$niters  AIE_VARIANT=2   TARGET=hw"
    test_arr[$LINENO]="make all T_DATA_A=cint16 T_DATA_B=int32      DIM_SIZE_A=16  DIM_SIZE_B=16  NUM_FRAMES=2  SHIFT=0 API_IO=0 UUT_SSR=1   STIM_TYPE=0 NITER=$niters  AIE_VARIANT=2   TARGET=hw"
    test_arr[$LINENO]="make all T_DATA_A=int16  T_DATA_B=int16      DIM_SIZE_A=16  DIM_SIZE_B=16  NUM_FRAMES=8  SHIFT=0 API_IO=0 UUT_SSR=1   STIM_TYPE=0 NITER=$niters  AIE_VARIANT=2   TARGET=hw"
    test_arr[$LINENO]="make all T_DATA_A=int16  T_DATA_B=cint32     DIM_SIZE_A=16  DIM_SIZE_B=16  NUM_FRAMES=4  SHIFT=0 API_IO=0 UUT_SSR=1   STIM_TYPE=0 NITER=$niters  AIE_VARIANT=2   TARGET=hw"
    test_arr[$LINENO]="make all T_DATA_A=cint32 T_DATA_B=int16      DIM_SIZE_A=16  DIM_SIZE_B=16  NUM_FRAMES=4  SHIFT=0 API_IO=0 UUT_SSR=1   STIM_TYPE=0 NITER=$niters  AIE_VARIANT=2   TARGET=hw"
    test_arr[$LINENO]="make all T_DATA_A=int16  T_DATA_B=int16      DIM_SIZE_A=16  DIM_SIZE_B=16  NUM_FRAMES=32 SHIFT=0 API_IO=0 UUT_SSR=1   STIM_TYPE=0 NITER=$niters  AIE_VARIANT=2   TARGET=hw"
fi

if [[ "$*" == *shift* || "$*" == *checkin* || "$*" == *qor* ]]
then
    # testing range of shifts
    if [[ "$*" == *qor* ]]
    then
        niters=16
    else
        niters=4
    fi
    test_arr[$LINENO]="make all T_DATA_A=int32 T_DATA_B=int32     DIM_SIZE_A=16  DIM_SIZE_B=16  NUM_FRAMES=2 SHIFT=0 API_IO=0 UUT_SSR=1     STIM_TYPE=0 NITER=$niters  AIE_VARIANT=1   TARGET=hw"
    test_arr[$LINENO]="make all T_DATA_A=int32 T_DATA_B=int32     DIM_SIZE_A=16  DIM_SIZE_B=16  NUM_FRAMES=2 SHIFT=1 API_IO=0 UUT_SSR=1     STIM_TYPE=0 NITER=$niters  AIE_VARIANT=1   TARGET=hw"
    test_arr[$LINENO]="make all T_DATA_A=int32 T_DATA_B=int32     DIM_SIZE_A=16  DIM_SIZE_B=16  NUM_FRAMES=2 SHIFT=2 API_IO=0 UUT_SSR=1     STIM_TYPE=0 NITER=$niters  AIE_VARIANT=1   TARGET=hw"
    test_arr[$LINENO]="make all T_DATA_A=int32 T_DATA_B=int32     DIM_SIZE_A=16  DIM_SIZE_B=16  NUM_FRAMES=2 SHIFT=4 API_IO=0 UUT_SSR=1     STIM_TYPE=0 NITER=$niters  AIE_VARIANT=1   TARGET=hw"
    test_arr[$LINENO]="make all T_DATA_A=int32 T_DATA_B=int32     DIM_SIZE_A=16  DIM_SIZE_B=16  NUM_FRAMES=2 SHIFT=8 API_IO=0 UUT_SSR=1     STIM_TYPE=0 NITER=$niters  AIE_VARIANT=1   TARGET=hw"
    test_arr[$LINENO]="make all T_DATA_A=int32 T_DATA_B=int32     DIM_SIZE_A=16  DIM_SIZE_B=16  NUM_FRAMES=2 SHIFT=16 API_IO=0 UUT_SSR=1    STIM_TYPE=0 NITER=$niters  AIE_VARIANT=1   TARGET=hw"
    test_arr[$LINENO]="make all T_DATA_A=int32 T_DATA_B=int32     DIM_SIZE_A=16  DIM_SIZE_B=16  NUM_FRAMES=2 SHIFT=32 API_IO=0 UUT_SSR=1    STIM_TYPE=0 NITER=$niters  AIE_VARIANT=1   TARGET=hw"
    test_arr[$LINENO]="make all T_DATA_A=int32 T_DATA_B=int32     DIM_SIZE_A=16  DIM_SIZE_B=16  NUM_FRAMES=2 SHIFT=60 API_IO=0 UUT_SSR=1    STIM_TYPE=0 NITER=$niters  AIE_VARIANT=1   TARGET=hw"

    test_arr[$LINENO]="make all T_DATA_A=int32 T_DATA_B=int32     DIM_SIZE_A=16  DIM_SIZE_B=16  NUM_FRAMES=2 SHIFT=0 API_IO=0 UUT_SSR=1     STIM_TYPE=0 NITER=$niters  AIE_VARIANT=2   TARGET=hw"
    test_arr[$LINENO]="make all T_DATA_A=int32 T_DATA_B=int32     DIM_SIZE_A=16  DIM_SIZE_B=16  NUM_FRAMES=2 SHIFT=1 API_IO=0 UUT_SSR=1     STIM_TYPE=0 NITER=$niters  AIE_VARIANT=2   TARGET=hw"
    test_arr[$LINENO]="make all T_DATA_A=int32 T_DATA_B=int32     DIM_SIZE_A=16  DIM_SIZE_B=16  NUM_FRAMES=2 SHIFT=2 API_IO=0 UUT_SSR=1     STIM_TYPE=0 NITER=$niters  AIE_VARIANT=2   TARGET=hw"
    test_arr[$LINENO]="make all T_DATA_A=int32 T_DATA_B=int32     DIM_SIZE_A=16  DIM_SIZE_B=16  NUM_FRAMES=2 SHIFT=4 API_IO=0 UUT_SSR=1     STIM_TYPE=0 NITER=$niters  AIE_VARIANT=2   TARGET=hw"
    test_arr[$LINENO]="make all T_DATA_A=int32 T_DATA_B=int32     DIM_SIZE_A=16  DIM_SIZE_B=16  NUM_FRAMES=2 SHIFT=8 API_IO=0 UUT_SSR=1     STIM_TYPE=0 NITER=$niters  AIE_VARIANT=2   TARGET=hw"
    test_arr[$LINENO]="make all T_DATA_A=int32 T_DATA_B=int32     DIM_SIZE_A=16  DIM_SIZE_B=16  NUM_FRAMES=2 SHIFT=16 API_IO=0 UUT_SSR=1    STIM_TYPE=0 NITER=$niters  AIE_VARIANT=2   TARGET=hw"
    test_arr[$LINENO]="make all T_DATA_A=int32 T_DATA_B=int32     DIM_SIZE_A=16  DIM_SIZE_B=16  NUM_FRAMES=2 SHIFT=32 API_IO=0 UUT_SSR=1    STIM_TYPE=0 NITER=$niters  AIE_VARIANT=2   TARGET=hw"
    test_arr[$LINENO]="make all T_DATA_A=int32 T_DATA_B=int32     DIM_SIZE_A=16  DIM_SIZE_B=16  NUM_FRAMES=2 SHIFT=59 API_IO=0 UUT_SSR=1    STIM_TYPE=0 NITER=$niters  AIE_VARIANT=2   TARGET=hw"
fi

if [[ "$*" == *streaming* || "$*" == *checkin* || "$*" == *qor* ]]
then
    # testing range of streaming configs of different sizes...
    if [[ "$*" == *qor* ]]
    then
        niters=16
    else
        niters=4
    fi
    test_arr[$LINENO]="make all T_DATA_A=cint32 T_DATA_B=cint32     DIM_SIZE_A=4    DIM_SIZE_B=4    NUM_FRAMES=2  SHIFT=0 API_IO=1 UUT_SSR=1   STIM_TYPE=0 NITER=$niters  AIE_VARIANT=1  TARGET=hw"    # 1KB
    test_arr[$LINENO]="make all T_DATA_A=cint32 T_DATA_B=cint32     DIM_SIZE_A=16   DIM_SIZE_B=16   NUM_FRAMES=8  SHIFT=0 API_IO=1 UUT_SSR=1   STIM_TYPE=0 NITER=$niters  AIE_VARIANT=1  TARGET=hw"    # 16KB
    test_arr[$LINENO]="make all T_DATA_A=cint32 T_DATA_B=cint32     DIM_SIZE_A=32   DIM_SIZE_B=16   NUM_FRAMES=8  SHIFT=0 API_IO=1 UUT_SSR=1   STIM_TYPE=0 NITER=$niters  AIE_VARIANT=1  TARGET=hw"    # 32KB
    test_arr[$LINENO]="make all T_DATA_A=cint32 T_DATA_B=cint32     DIM_SIZE_A=64   DIM_SIZE_B=16   NUM_FRAMES=8  SHIFT=0 API_IO=1 UUT_SSR=1   STIM_TYPE=0 NITER=$niters  AIE_VARIANT=1  TARGET=hw"    # 64KB
    test_arr[$LINENO]="make all T_DATA_A=cint32 T_DATA_B=cint32     DIM_SIZE_A=128  DIM_SIZE_B=16   NUM_FRAMES=8  SHIFT=0 API_IO=1 UUT_SSR=1   STIM_TYPE=0 NITER=$niters  AIE_VARIANT=1  TARGET=hw"    # 128KB
    test_arr[$LINENO]="make all T_DATA_A=cint32 T_DATA_B=cint32     DIM_SIZE_A=256  DIM_SIZE_B=16   NUM_FRAMES=8  SHIFT=0 API_IO=1 UUT_SSR=1   STIM_TYPE=0 NITER=$niters  AIE_VARIANT=1  TARGET=hw"    # 256KB
    test_arr[$LINENO]="make all T_DATA_A=cint32 T_DATA_B=cint32     DIM_SIZE_A=16   DIM_SIZE_B=32   NUM_FRAMES=8  SHIFT=0 API_IO=1 UUT_SSR=1   STIM_TYPE=0 NITER=$niters  AIE_VARIANT=1  TARGET=hw"    # 32KB
    test_arr[$LINENO]="make all T_DATA_A=cint32 T_DATA_B=cint32     DIM_SIZE_A=16   DIM_SIZE_B=64   NUM_FRAMES=8  SHIFT=0 API_IO=1 UUT_SSR=1   STIM_TYPE=0 NITER=$niters  AIE_VARIANT=1  TARGET=hw"    # 64KB
    test_arr[$LINENO]="make all T_DATA_A=cint32 T_DATA_B=cint32     DIM_SIZE_A=16   DIM_SIZE_B=128  NUM_FRAMES=8  SHIFT=0 API_IO=1 UUT_SSR=1   STIM_TYPE=0 NITER=$niters  AIE_VARIANT=1  TARGET=hw"    # 128KB
    test_arr[$LINENO]="make all T_DATA_A=cint32 T_DATA_B=cint32     DIM_SIZE_A=16   DIM_SIZE_B=256  NUM_FRAMES=8  SHIFT=0 API_IO=1 UUT_SSR=1   STIM_TYPE=0 NITER=$niters  AIE_VARIANT=1  TARGET=hw"    # 256KB

    test_arr[$LINENO]="make all T_DATA_A=cint32 T_DATA_B=cint32     DIM_SIZE_A=4    DIM_SIZE_B=4    NUM_FRAMES=2  SHIFT=0 API_IO=1 UUT_SSR=1   STIM_TYPE=0 NITER=$niters  AIE_VARIANT=2  TARGET=hw"    # 1KB
    test_arr[$LINENO]="make all T_DATA_A=cint32 T_DATA_B=cint32     DIM_SIZE_A=16   DIM_SIZE_B=16   NUM_FRAMES=8  SHIFT=0 API_IO=1 UUT_SSR=1   STIM_TYPE=0 NITER=$niters  AIE_VARIANT=2  TARGET=hw"    # 16KB
    test_arr[$LINENO]="make all T_DATA_A=cint32 T_DATA_B=cint32     DIM_SIZE_A=32   DIM_SIZE_B=16   NUM_FRAMES=8  SHIFT=0 API_IO=1 UUT_SSR=1   STIM_TYPE=0 NITER=$niters  AIE_VARIANT=2  TARGET=hw"    # 32KB
    test_arr[$LINENO]="make all T_DATA_A=cint32 T_DATA_B=cint32     DIM_SIZE_A=64   DIM_SIZE_B=16   NUM_FRAMES=8  SHIFT=0 API_IO=1 UUT_SSR=1   STIM_TYPE=0 NITER=$niters  AIE_VARIANT=2  TARGET=hw"    # 64KB
    test_arr[$LINENO]="make all T_DATA_A=cint32 T_DATA_B=cint32     DIM_SIZE_A=128  DIM_SIZE_B=16   NUM_FRAMES=8  SHIFT=0 API_IO=1 UUT_SSR=1   STIM_TYPE=0 NITER=$niters  AIE_VARIANT=2  TARGET=hw"    # 128KB
    test_arr[$LINENO]="make all T_DATA_A=cint32 T_DATA_B=cint32     DIM_SIZE_A=256  DIM_SIZE_B=16   NUM_FRAMES=8  SHIFT=0 API_IO=1 UUT_SSR=1   STIM_TYPE=0 NITER=$niters  AIE_VARIANT=2  TARGET=hw"    # 256KB
    test_arr[$LINENO]="make all T_DATA_A=cint32 T_DATA_B=cint32     DIM_SIZE_A=16   DIM_SIZE_B=32   NUM_FRAMES=8  SHIFT=0 API_IO=1 UUT_SSR=1   STIM_TYPE=0 NITER=$niters  AIE_VARIANT=2  TARGET=hw"    # 32KB
    test_arr[$LINENO]="make all T_DATA_A=cint32 T_DATA_B=cint32     DIM_SIZE_A=16   DIM_SIZE_B=64   NUM_FRAMES=8  SHIFT=0 API_IO=1 UUT_SSR=1   STIM_TYPE=0 NITER=$niters  AIE_VARIANT=2  TARGET=hw"    # 64KB
    test_arr[$LINENO]="make all T_DATA_A=cint32 T_DATA_B=cint32     DIM_SIZE_A=16   DIM_SIZE_B=128  NUM_FRAMES=8  SHIFT=0 API_IO=1 UUT_SSR=1   STIM_TYPE=0 NITER=$niters  AIE_VARIANT=2  TARGET=hw"    # 128KB
    test_arr[$LINENO]="make all T_DATA_A=cint32 T_DATA_B=cint32     DIM_SIZE_A=16   DIM_SIZE_B=256  NUM_FRAMES=8  SHIFT=0 API_IO=1 UUT_SSR=1   STIM_TYPE=0 NITER=$niters  AIE_VARIANT=2  TARGET=hw"    # 256KB

    # # stress testing streaming and SSR.
    # test_arr[$LINENO]="make all T_DATA_A=int16 T_DATA_B=int16     DIM_SIZE_A=1024   DIM_SIZE_B=2048  NUM_FRAMES=2  SHIFT=0 API_IO=1 UUT_SSR=8   STIM_TYPE=5 NITER=$niters     TARGET=hw"    # 8 * 1024KB kernel output
    # test_arr[$LINENO]="make all T_DATA_A=int16 T_DATA_B=int16     DIM_SIZE_A=1024   DIM_SIZE_B=2048  NUM_FRAMES=1  SHIFT=0 API_IO=1 UUT_SSR=1   STIM_TYPE=5 NITER=$niters     TARGET=hw"    # 8192KB output
fi

if [[ "$*" == *ssr* || "$*" == *checkin* || "$*" == *qor* ]]
then
    if [[ "$*" == *qor* ]]
    then
        niters=16
    else
        niters=4
    fi
    test_arr[$LINENO]="make all T_DATA_A=cint32 T_DATA_B=cint32     DIM_SIZE_A=8    DIM_SIZE_B=4    NUM_FRAMES=8  SHIFT=0 API_IO=0 UUT_SSR=2   STIM_TYPE=0 NITER=$niters  AIE_VARIANT=1  TARGET=hw"
    test_arr[$LINENO]="make all T_DATA_A=cint32 T_DATA_B=cint32     DIM_SIZE_A=16   DIM_SIZE_B=16   NUM_FRAMES=8  SHIFT=0 API_IO=0 UUT_SSR=2   STIM_TYPE=0 NITER=$niters  AIE_VARIANT=1  TARGET=hw"    # 16KB
    test_arr[$LINENO]="make all T_DATA_A=cint32 T_DATA_B=cint32     DIM_SIZE_A=16   DIM_SIZE_B=16   NUM_FRAMES=8  SHIFT=0 API_IO=0 UUT_SSR=4   STIM_TYPE=0 NITER=$niters  AIE_VARIANT=1  TARGET=hw"    # 16KB
    test_arr[$LINENO]="make all T_DATA_A=cint32 T_DATA_B=cint32     DIM_SIZE_A=16   DIM_SIZE_B=16   NUM_FRAMES=8  SHIFT=0 API_IO=1 UUT_SSR=2   STIM_TYPE=0 NITER=$niters  AIE_VARIANT=1  TARGET=hw"    # 16KB
    test_arr[$LINENO]="make all T_DATA_A=cint32 T_DATA_B=cint32     DIM_SIZE_A=32   DIM_SIZE_B=16   NUM_FRAMES=8  SHIFT=0 API_IO=1 UUT_SSR=4   STIM_TYPE=0 NITER=$niters  AIE_VARIANT=1  TARGET=hw"    # 32KB
    test_arr[$LINENO]="make all T_DATA_A=cint32 T_DATA_B=cint32     DIM_SIZE_A=64   DIM_SIZE_B=16   NUM_FRAMES=8  SHIFT=0 API_IO=1 UUT_SSR=8   STIM_TYPE=0 NITER=$niters  AIE_VARIANT=1  TARGET=hw"    # 64KB
    test_arr[$LINENO]="make all T_DATA_A=cint32 T_DATA_B=cint32     DIM_SIZE_A=128  DIM_SIZE_B=16   NUM_FRAMES=8  SHIFT=0 API_IO=1 UUT_SSR=16  STIM_TYPE=0 NITER=$niters  AIE_VARIANT=1  TARGET=hw"    # 128KB

    test_arr[$LINENO]="make all T_DATA_A=cint32 T_DATA_B=cint32     DIM_SIZE_A=8    DIM_SIZE_B=4    NUM_FRAMES=8  SHIFT=0 API_IO=0 UUT_SSR=2   STIM_TYPE=0 NITER=$niters  AIE_VARIANT=2  TARGET=hw"
    test_arr[$LINENO]="make all T_DATA_A=cint32 T_DATA_B=cint32     DIM_SIZE_A=16   DIM_SIZE_B=16   NUM_FRAMES=8  SHIFT=0 API_IO=0 UUT_SSR=2   STIM_TYPE=0 NITER=$niters  AIE_VARIANT=2  TARGET=hw"    # 16KB
    test_arr[$LINENO]="make all T_DATA_A=cint32 T_DATA_B=cint32     DIM_SIZE_A=16   DIM_SIZE_B=16   NUM_FRAMES=8  SHIFT=0 API_IO=0 UUT_SSR=4   STIM_TYPE=0 NITER=$niters  AIE_VARIANT=2  TARGET=hw"    # 16KB
    test_arr[$LINENO]="make all T_DATA_A=cint32 T_DATA_B=cint32     DIM_SIZE_A=16   DIM_SIZE_B=16   NUM_FRAMES=8  SHIFT=0 API_IO=1 UUT_SSR=2   STIM_TYPE=0 NITER=$niters  AIE_VARIANT=2  TARGET=hw"    # 16KB
    test_arr[$LINENO]="make all T_DATA_A=cint32 T_DATA_B=cint32     DIM_SIZE_A=32   DIM_SIZE_B=16   NUM_FRAMES=8  SHIFT=0 API_IO=1 UUT_SSR=4   STIM_TYPE=0 NITER=$niters  AIE_VARIANT=2  TARGET=hw"    # 32KB
    test_arr[$LINENO]="make all T_DATA_A=cint32 T_DATA_B=cint32     DIM_SIZE_A=64   DIM_SIZE_B=16   NUM_FRAMES=8  SHIFT=0 API_IO=1 UUT_SSR=8   STIM_TYPE=0 NITER=$niters  AIE_VARIANT=2  TARGET=hw"    # 64KB
    test_arr[$LINENO]="make all T_DATA_A=cint32 T_DATA_B=cint32     DIM_SIZE_A=128  DIM_SIZE_B=16   NUM_FRAMES=8  SHIFT=0 API_IO=1 UUT_SSR=16  STIM_TYPE=0 NITER=$niters  AIE_VARIANT=2  TARGET=hw"    # 128KB
fi

if [[ "$*" == *aie2* ]]
then
    if [[ "$*" == *qor* ]]
    then
        niters=16
    else
        niters=4
    fi
    test_arr[$LINENO]="make all T_DATA_A=cint32 T_DATA_B=cint32     DIM_SIZE_A=32  DIM_SIZE_B=32  NUM_FRAMES=2  SHIFT=10    API_IO=0    UUT_SSR=1   STIM_TYPE=0     NITER=$niters   TARGET=hw   AIE_VARIANT=2"
    test_arr[$LINENO]="make all T_DATA_A=int16 T_DATA_B=cint16      DIM_SIZE_A=64  DIM_SIZE_B=32  NUM_FRAMES=2  SHIFT=0     API_IO=0    UUT_SSR=2   STIM_TYPE=0     NITER=$niters   TARGET=hw   AIE_VARIANT=2"
    test_arr[$LINENO]="make all T_DATA_A=cint32 T_DATA_B=cint32     DIM_SIZE_A=32  DIM_SIZE_B=32  NUM_FRAMES=4  SHIFT=10    API_IO=1    UUT_SSR=1   STIM_TYPE=0     NITER=$niters   TARGET=hw   AIE_VARIANT=2"
    test_arr[$LINENO]="make all T_DATA_A=int16 T_DATA_B=cint16      DIM_SIZE_A=64  DIM_SIZE_B=64  NUM_FRAMES=4  SHIFT=0     API_IO=1    UUT_SSR=2   STIM_TYPE=0     NITER=$niters   TARGET=hw   AIE_VARIANT=2"
    test_arr[$LINENO]="make all T_DATA_A=int32 T_DATA_B=int32       DIM_SIZE_A=32  DIM_SIZE_B=32  NUM_FRAMES=2  SHIFT=10    API_IO=0    UUT_SSR=1   STIM_TYPE=0     NITER=$niters   TARGET=hw   AIE_VARIANT=2"
    test_arr[$LINENO]="make all T_DATA_A=int16 T_DATA_B=int16       DIM_SIZE_A=64  DIM_SIZE_B=32  NUM_FRAMES=2  SHIFT=0     API_IO=0    UUT_SSR=2   STIM_TYPE=0     NITER=$niters   TARGET=hw   AIE_VARIANT=2"
    test_arr[$LINENO]="make all T_DATA_A=cint32 T_DATA_B=int16      DIM_SIZE_A=32  DIM_SIZE_B=32  NUM_FRAMES=4  SHIFT=10    API_IO=1    UUT_SSR=1   STIM_TYPE=0     NITER=$niters   TARGET=hw   AIE_VARIANT=2"
    test_arr[$LINENO]="make all T_DATA_A=int16 T_DATA_B=cint32      DIM_SIZE_A=64  DIM_SIZE_B=64  NUM_FRAMES=4  SHIFT=0     API_IO=1    UUT_SSR=2   STIM_TYPE=0     NITER=$niters   TARGET=hw   AIE_VARIANT=2"
    # test_arr[$LINENO]="make all T_DATA_A=float T_DATA_B=float       DIM_SIZE_A=32  DIM_SIZE_B=32  NUM_FRAMES=2  SHIFT=10    API_IO=0    UUT_SSR=1   STIM_TYPE=0     NITER=$niters   TARGET=hw   AIE_VARIANT=2"
fi



if [[ "$*" == *illegal_case_checks* ]]
then
    test_arr[$LINENO]="make all T_DATA_A=cint32 T_DATA_B=cint32     DIM_SIZE_A=15       DIM_SIZE_B=16   NUM_FRAMES=1    SHIFT=0     API_IO=0 UUT_SSR=1   STIM_TYPE=5 NITER=1     TARGET=x86sim"      # invalid dim size A
    test_arr[$LINENO]="make all T_DATA_A=cint32 T_DATA_B=cint16     DIM_SIZE_A=1025     DIM_SIZE_B=16   NUM_FRAMES=1    SHIFT=0     API_IO=0 UUT_SSR=1   STIM_TYPE=5 NITER=1     TARGET=x86sim"    # invalid dim size A
    test_arr[$LINENO]="make all T_DATA_A=cint32 T_DATA_B=cint32     DIM_SIZE_A=32       DIM_SIZE_B=15   NUM_FRAMES=1    SHIFT=0     API_IO=0 UUT_SSR=1   STIM_TYPE=5 NITER=1     TARGET=x86sim"      # invalid dim size B
    test_arr[$LINENO]="make all T_DATA_A=cint32 T_DATA_B=cint16     DIM_SIZE_A=32       DIM_SIZE_B=1025 NUM_FRAMES=1    SHIFT=0     API_IO=0 UUT_SSR=1   STIM_TYPE=5 NITER=1     TARGET=x86sim"    # invalid dim size B
    test_arr[$LINENO]="make all T_DATA_A=cint32 T_DATA_B=int32      DIM_SIZE_A=64       DIM_SIZE_B=64   NUM_FRAMES=2    SHIFT=0     API_IO=0 UUT_SSR=1   STIM_TYPE=5 NITER=1     TARGET=x86sim"      # exceeds 16kB
    test_arr[$LINENO]="make all T_DATA_A=cint32 T_DATA_B=int16      DIM_SIZE_A=32       DIM_SIZE_B=32   NUM_FRAMES=0    SHIFT=0     API_IO=0 UUT_SSR=1   STIM_TYPE=5 NITER=1     TARGET=x86sim"      # invalid num frames
    test_arr[$LINENO]="make all T_DATA_A=cint16 T_DATA_B=cint32     DIM_SIZE_A=32       DIM_SIZE_B=32   NUM_FRAMES=70   SHIFT=0     API_IO=0 UUT_SSR=1   STIM_TYPE=5 NITER=1     TARGET=x86sim"     # invalid num frames
    test_arr[$LINENO]="make all T_DATA_A=cint16 T_DATA_B=cint16     DIM_SIZE_A=32       DIM_SIZE_B=32   NUM_FRAMES=1    SHIFT=-1    API_IO=0 UUT_SSR=1   STIM_TYPE=5 NITER=1     TARGET=x86sim"     # invalid shift
    test_arr[$LINENO]="make all T_DATA_A=cint16 T_DATA_B=int32      DIM_SIZE_A=32       DIM_SIZE_B=32   NUM_FRAMES=1    SHIFT=61    API_IO=0 UUT_SSR=1   STIM_TYPE=5 NITER=1     TARGET=x86sim"     # invalid shift
    test_arr[$LINENO]="make all T_DATA_A=int32 T_DATA_B=cint32      DIM_SIZE_A=32       DIM_SIZE_B=32   NUM_FRAMES=1    SHIFT=0     API_IO=0 UUT_SSR=0   STIM_TYPE=5 NITER=1     TARGET=x86sim"      # invalid SSR
    test_arr[$LINENO]="make all T_DATA_A=int32 T_DATA_B=cint16      DIM_SIZE_A=32       DIM_SIZE_B=32   NUM_FRAMES=1    SHIFT=0     API_IO=0 UUT_SSR=17  STIM_TYPE=5 NITER=1     TARGET=x86sim"     # invalid SSR
    test_arr[$LINENO]="make all T_DATA_A=int32 T_DATA_B=cint16      DIM_SIZE_A=32       DIM_SIZE_B=32   NUM_FRAMES=1    SHIFT=0     API_IO=0 UUT_SSR=10   STIM_TYPE=5 NITER=1     TARGET=x86sim"     # invalid SSR
    test_arr[$LINENO]="make all T_DATA_A=int T_DATA_B=int32         DIM_SIZE_A=32       DIM_SIZE_B=32   NUM_FRAMES=1    SHIFT=0     API_IO=0 UUT_SSR=1   STIM_TYPE=5 NITER=1     TARGET=x86sim"        # invalid datatype A
    test_arr[$LINENO]="make all T_DATA_A=int32 T_DATA_B=bool        DIM_SIZE_A=32       DIM_SIZE_B=32   NUM_FRAMES=1    SHIFT=0     API_IO=0 UUT_SSR=1   STIM_TYPE=5 NITER=1     TARGET=x86sim"       # invalid datatype B
    test_arr[$LINENO]="make all T_DATA_A=cint32 T_DATA_B=cint32     DIM_SIZE_A=63       DIM_SIZE_B=16   NUM_FRAMES=1    SHIFT=0     API_IO=0 UUT_SSR=1   STIM_TYPE=5 NITER=1     TARGET=x86sim"      # A not multiple of 32
    test_arr[$LINENO]="make all T_DATA_A=cint32 T_DATA_B=cint16     DIM_SIZE_A=64       DIM_SIZE_B=37   NUM_FRAMES=1    SHIFT=0     API_IO=0 UUT_SSR=1   STIM_TYPE=5 NITER=1     TARGET=x86sim"      # B not multiple of 32
    test_arr[$LINENO]="make all T_DATA_A=cint32 T_DATA_B=cint32     DIM_SIZE_A=64       DIM_SIZE_B=16   NUM_FRAMES=6    SHIFT=0     API_IO=0 UUT_SSR=1   STIM_TYPE=5 NITER=1     TARGET=x86sim"      # NUM_FRAMES not power of 2
    test_arr[$LINENO]="make all T_DATA_A=int32 T_DATA_B=int32       DIM_SIZE_A=32       DIM_SIZE_B=32   NUM_FRAMES=4    SHIFT=5     API_IO=0 UUT_SSR=1  ROUND_MODE=-1  SAT_MODE=1 NITER=1 TARGET=x86sim"    # invalid round mode
    test_arr[$LINENO]="make all T_DATA_A=int32 T_DATA_B=int32       DIM_SIZE_A=32       DIM_SIZE_B=32   NUM_FRAMES=4    SHIFT=5     API_IO=0 UUT_SSR=1  ROUND_MODE=1  SAT_MODE=-1 NITER=1 TARGET=x86sim"    # invalid sat mode

fi

if [[ "$*" == *specialisations* ]]
then
    test_arr[$LINENO]="make all T_DATA_A=int32  T_DATA_B=int32      DIM_SIZE_A=32  DIM_SIZE_B=32  NUM_FRAMES=4 SHIFT=0 API_IO=0 UUT_SSR=1   STIM_TYPE=5 NITER=4     TARGET=x86sim"
    test_arr[$LINENO]="make all T_DATA_A=int16  T_DATA_B=cint32     DIM_SIZE_A=32  DIM_SIZE_B=32  NUM_FRAMES=4 SHIFT=0 API_IO=0 UUT_SSR=1   STIM_TYPE=5 NITER=4     TARGET=x86sim"
    test_arr[$LINENO]="make all T_DATA_A=cint32 T_DATA_B=int16      DIM_SIZE_A=32  DIM_SIZE_B=32  NUM_FRAMES=4 SHIFT=0 API_IO=0 UUT_SSR=1   STIM_TYPE=5 NITER=4     TARGET=x86sim"
fi

if [[ "$*" == *database* ]]
then
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=1 T_DATA_A=cfloat T_DATA_B=cfloat API_IO=1 UUT_SSR=1 DIM_SIZE_A=1024 DIM_SIZE_B=8 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=0 DIFF_MODE=PERCENT DIFF_TOLERANCE=0.01 CC_TOLERANCE=0.01"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=1 T_DATA_A=cfloat T_DATA_B=cfloat API_IO=1 UUT_SSR=1 DIM_SIZE_A=1024 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=0 DIFF_MODE=PERCENT DIFF_TOLERANCE=0.01 CC_TOLERANCE=0.01"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=1 T_DATA_A=cfloat T_DATA_B=float API_IO=1 UUT_SSR=1 DIM_SIZE_A=1024 DIM_SIZE_B=8 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=0 DIFF_MODE=PERCENT DIFF_TOLERANCE=0.01 CC_TOLERANCE=0.01"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=1 T_DATA_A=cfloat T_DATA_B=float API_IO=1 UUT_SSR=1 DIM_SIZE_A=1024 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=0 DIFF_MODE=PERCENT DIFF_TOLERANCE=0.01 CC_TOLERANCE=0.01"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=1 T_DATA_A=float T_DATA_B=float API_IO=1 UUT_SSR=1 DIM_SIZE_A=1024 DIM_SIZE_B=8 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=0 DIFF_MODE=PERCENT DIFF_TOLERANCE=0.01 CC_TOLERANCE=0.01"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=1 T_DATA_A=float T_DATA_B=float API_IO=1 UUT_SSR=1 DIM_SIZE_A=1024 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=0 DIFF_MODE=PERCENT DIFF_TOLERANCE=0.01 CC_TOLERANCE=0.01"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=1 T_DATA_A=float T_DATA_B=float API_IO=1 UUT_SSR=1 DIM_SIZE_A=4096 DIM_SIZE_B=8 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=0 DIFF_MODE=PERCENT DIFF_TOLERANCE=0.01 CC_TOLERANCE=0.01"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=1 T_DATA_A=float T_DATA_B=float API_IO=1 UUT_SSR=1 DIM_SIZE_A=4096 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=0 DIFF_MODE=PERCENT DIFF_TOLERANCE=0.01 CC_TOLERANCE=0.01"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=1 T_DATA_A=cint16 T_DATA_B=cint16 API_IO=1 UUT_SSR=1 DIM_SIZE_A=1024 DIM_SIZE_B=8 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=0"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=1 T_DATA_A=cint16 T_DATA_B=cint16 API_IO=1 UUT_SSR=1 DIM_SIZE_A=1024 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=0"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=1 T_DATA_A=cint16 T_DATA_B=cint16 API_IO=1 UUT_SSR=1 DIM_SIZE_A=4096 DIM_SIZE_B=8 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=0"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=1 T_DATA_A=cint16 T_DATA_B=cint16 API_IO=1 UUT_SSR=1 DIM_SIZE_A=4096 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=0"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=1 T_DATA_A=cint16 T_DATA_B=cint32 API_IO=1 UUT_SSR=1 DIM_SIZE_A=1024 DIM_SIZE_B=8 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=0"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=1 T_DATA_A=cint16 T_DATA_B=cint32 API_IO=1 UUT_SSR=1 DIM_SIZE_A=1024 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=0"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=1 T_DATA_A=cint16 T_DATA_B=cint32 API_IO=1 UUT_SSR=1 DIM_SIZE_A=4096 DIM_SIZE_B=8 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=0"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=1 T_DATA_A=cint16 T_DATA_B=cint32 API_IO=1 UUT_SSR=1 DIM_SIZE_A=4096 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=0"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=1 T_DATA_A=cint16 T_DATA_B=int16 API_IO=1 UUT_SSR=1 DIM_SIZE_A=1024 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=0"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=1 T_DATA_A=cint16 T_DATA_B=int16 API_IO=1 UUT_SSR=1 DIM_SIZE_A=4096 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=0"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=1 T_DATA_A=cint16 T_DATA_B=int32 API_IO=1 UUT_SSR=1 DIM_SIZE_A=1024 DIM_SIZE_B=8 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=0"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=1 T_DATA_A=cint16 T_DATA_B=int32 API_IO=1 UUT_SSR=1 DIM_SIZE_A=1024 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=0"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=1 T_DATA_A=cint16 T_DATA_B=int32 API_IO=1 UUT_SSR=1 DIM_SIZE_A=4096 DIM_SIZE_B=8 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=0"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=1 T_DATA_A=cint16 T_DATA_B=int32 API_IO=1 UUT_SSR=1 DIM_SIZE_A=4096 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=0"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=1 T_DATA_A=cint32 T_DATA_B=cint32 API_IO=1 UUT_SSR=1 DIM_SIZE_A=1024 DIM_SIZE_B=8 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=0"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=1 T_DATA_A=cint32 T_DATA_B=cint32 API_IO=1 UUT_SSR=1 DIM_SIZE_A=1024 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=0"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=1 T_DATA_A=cint32 T_DATA_B=int16 API_IO=1 UUT_SSR=1 DIM_SIZE_A=1024 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=0"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=1 T_DATA_A=cint32 T_DATA_B=int32 API_IO=1 UUT_SSR=1 DIM_SIZE_A=1024 DIM_SIZE_B=8 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=0"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=1 T_DATA_A=cint32 T_DATA_B=int32 API_IO=1 UUT_SSR=1 DIM_SIZE_A=1024 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=0"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=1 T_DATA_A=int16 T_DATA_B=int16 API_IO=1 UUT_SSR=1 DIM_SIZE_A=1024 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=0"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=1 T_DATA_A=int16 T_DATA_B=int16 API_IO=1 UUT_SSR=1 DIM_SIZE_A=4096 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=0"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=1 T_DATA_A=int16 T_DATA_B=int16 API_IO=1 UUT_SSR=1 DIM_SIZE_A=8192 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=0"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=1 T_DATA_A=int16 T_DATA_B=int32 API_IO=1 UUT_SSR=1 DIM_SIZE_A=1024 DIM_SIZE_B=8 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=0"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=1 T_DATA_A=int16 T_DATA_B=int32 API_IO=1 UUT_SSR=1 DIM_SIZE_A=1024 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=0"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=1 T_DATA_A=int16 T_DATA_B=int32 API_IO=1 UUT_SSR=1 DIM_SIZE_A=4096 DIM_SIZE_B=8 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=0"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=1 T_DATA_A=int16 T_DATA_B=int32 API_IO=1 UUT_SSR=1 DIM_SIZE_A=4096 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=0"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=1 T_DATA_A=int16 T_DATA_B=int32 API_IO=1 UUT_SSR=1 DIM_SIZE_A=8192 DIM_SIZE_B=8 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=0"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=1 T_DATA_A=int16 T_DATA_B=int32 API_IO=1 UUT_SSR=1 DIM_SIZE_A=8192 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=0"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=1 T_DATA_A=int32 T_DATA_B=int32 API_IO=1 UUT_SSR=1 DIM_SIZE_A=1024 DIM_SIZE_B=8 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=0"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=1 T_DATA_A=int32 T_DATA_B=int32 API_IO=1 UUT_SSR=1 DIM_SIZE_A=1024 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=0"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=1 T_DATA_A=int32 T_DATA_B=int32 API_IO=1 UUT_SSR=1 DIM_SIZE_A=4096 DIM_SIZE_B=8 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=0"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=1 T_DATA_A=int32 T_DATA_B=int32 API_IO=1 UUT_SSR=1 DIM_SIZE_A=4096 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=0"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=2 T_DATA_A=cfloat T_DATA_B=cfloat API_IO=1 UUT_SSR=1 DIM_SIZE_A=1024 DIM_SIZE_B=8 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=0 DIFF_MODE=PERCENT DIFF_TOLERANCE=0.01 CC_TOLERANCE=0.01"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=2 T_DATA_A=cfloat T_DATA_B=cfloat API_IO=1 UUT_SSR=1 DIM_SIZE_A=1024 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=0 DIFF_MODE=PERCENT DIFF_TOLERANCE=0.01 CC_TOLERANCE=0.01"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=2 T_DATA_A=cfloat T_DATA_B=cfloat API_IO=1 UUT_SSR=1 DIM_SIZE_A=4096 DIM_SIZE_B=8 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=0 DIFF_MODE=PERCENT DIFF_TOLERANCE=0.01 CC_TOLERANCE=0.01"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=2 T_DATA_A=cfloat T_DATA_B=cfloat API_IO=1 UUT_SSR=1 DIM_SIZE_A=4096 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=0 DIFF_MODE=PERCENT DIFF_TOLERANCE=0.01 CC_TOLERANCE=0.01"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=2 T_DATA_A=cfloat T_DATA_B=float API_IO=1 UUT_SSR=1 DIM_SIZE_A=1024 DIM_SIZE_B=8 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=0 DIFF_MODE=PERCENT DIFF_TOLERANCE=0.01 CC_TOLERANCE=0.01"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=2 T_DATA_A=cfloat T_DATA_B=float API_IO=1 UUT_SSR=1 DIM_SIZE_A=1024 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=0 DIFF_MODE=PERCENT DIFF_TOLERANCE=0.01 CC_TOLERANCE=0.01"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=2 T_DATA_A=cfloat T_DATA_B=float API_IO=1 UUT_SSR=1 DIM_SIZE_A=4096 DIM_SIZE_B=8 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=0 DIFF_MODE=PERCENT DIFF_TOLERANCE=0.01 CC_TOLERANCE=0.01"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=2 T_DATA_A=cfloat T_DATA_B=float API_IO=1 UUT_SSR=1 DIM_SIZE_A=4096 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=0 DIFF_MODE=PERCENT DIFF_TOLERANCE=0.01 CC_TOLERANCE=0.01"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=2 T_DATA_A=float T_DATA_B=float API_IO=0 UUT_SSR=1 DIM_SIZE_A=1024 DIM_SIZE_B=8 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=0 DIFF_MODE=PERCENT DIFF_TOLERANCE=0.01 CC_TOLERANCE=0.01"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=2 T_DATA_A=float T_DATA_B=float API_IO=1 UUT_SSR=1 DIM_SIZE_A=1024 DIM_SIZE_B=8 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=0 DIFF_MODE=PERCENT DIFF_TOLERANCE=0.01 CC_TOLERANCE=0.01"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=2 T_DATA_A=float T_DATA_B=float API_IO=1 UUT_SSR=1 DIM_SIZE_A=1024 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=0 DIFF_MODE=PERCENT DIFF_TOLERANCE=0.01 CC_TOLERANCE=0.01"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=2 T_DATA_A=float T_DATA_B=float API_IO=1 UUT_SSR=1 DIM_SIZE_A=4096 DIM_SIZE_B=8 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=0 DIFF_MODE=PERCENT DIFF_TOLERANCE=0.01 CC_TOLERANCE=0.01"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=2 T_DATA_A=float T_DATA_B=float API_IO=1 UUT_SSR=1 DIM_SIZE_A=4096 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=0 DIFF_MODE=PERCENT DIFF_TOLERANCE=0.01 CC_TOLERANCE=0.01"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=2 T_DATA_A=float T_DATA_B=float API_IO=1 UUT_SSR=1 DIM_SIZE_A=8192 DIM_SIZE_B=8 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=0 DIFF_MODE=PERCENT DIFF_TOLERANCE=0.01 CC_TOLERANCE=0.01"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=2 T_DATA_A=float T_DATA_B=float API_IO=1 UUT_SSR=1 DIM_SIZE_A=8192 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=0 DIFF_MODE=PERCENT DIFF_TOLERANCE=0.01 CC_TOLERANCE=0.01"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=2 T_DATA_A=cint16 T_DATA_B=cint16 API_IO=0 UUT_SSR=1 DIM_SIZE_A=1024 DIM_SIZE_B=8 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=0"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=2 T_DATA_A=cint16 T_DATA_B=cint16 API_IO=1 UUT_SSR=1 DIM_SIZE_A=1024 DIM_SIZE_B=8 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=0"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=2 T_DATA_A=cint16 T_DATA_B=cint16 API_IO=1 UUT_SSR=1 DIM_SIZE_A=1024 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=0"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=2 T_DATA_A=cint16 T_DATA_B=cint16 API_IO=1 UUT_SSR=1 DIM_SIZE_A=4096 DIM_SIZE_B=8 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=0"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=2 T_DATA_A=cint16 T_DATA_B=cint16 API_IO=1 UUT_SSR=1 DIM_SIZE_A=4096 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=0"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=2 T_DATA_A=cint16 T_DATA_B=cint16 API_IO=1 UUT_SSR=1 DIM_SIZE_A=8192 DIM_SIZE_B=8 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=0"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=2 T_DATA_A=cint16 T_DATA_B=cint16 API_IO=1 UUT_SSR=1 DIM_SIZE_A=8192 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=0"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=2 T_DATA_A=cint16 T_DATA_B=cint32 API_IO=1 UUT_SSR=1 DIM_SIZE_A=1024 DIM_SIZE_B=8 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=0"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=2 T_DATA_A=cint16 T_DATA_B=cint32 API_IO=1 UUT_SSR=1 DIM_SIZE_A=1024 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=0"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=2 T_DATA_A=cint16 T_DATA_B=cint32 API_IO=1 UUT_SSR=1 DIM_SIZE_A=4096 DIM_SIZE_B=8 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=0"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=2 T_DATA_A=cint16 T_DATA_B=cint32 API_IO=1 UUT_SSR=1 DIM_SIZE_A=4096 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=0"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=2 T_DATA_A=cint16 T_DATA_B=cint32 API_IO=1 UUT_SSR=1 DIM_SIZE_A=8192 DIM_SIZE_B=8 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=0"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=2 T_DATA_A=cint16 T_DATA_B=cint32 API_IO=1 UUT_SSR=1 DIM_SIZE_A=8192 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=0"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=2 T_DATA_A=cint16 T_DATA_B=int16 API_IO=1 UUT_SSR=1 DIM_SIZE_A=1024 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=0"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=2 T_DATA_A=cint16 T_DATA_B=int16 API_IO=1 UUT_SSR=1 DIM_SIZE_A=4096 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=0"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=2 T_DATA_A=cint16 T_DATA_B=int16 API_IO=1 UUT_SSR=1 DIM_SIZE_A=8192 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=0"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=2 T_DATA_A=cint16 T_DATA_B=int32 API_IO=1 UUT_SSR=1 DIM_SIZE_A=1024 DIM_SIZE_B=8 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=0"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=2 T_DATA_A=cint16 T_DATA_B=int32 API_IO=1 UUT_SSR=1 DIM_SIZE_A=1024 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=0"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=2 T_DATA_A=cint16 T_DATA_B=int32 API_IO=1 UUT_SSR=1 DIM_SIZE_A=4096 DIM_SIZE_B=8 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=0"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=2 T_DATA_A=cint16 T_DATA_B=int32 API_IO=1 UUT_SSR=1 DIM_SIZE_A=4096 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=0"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=2 T_DATA_A=cint16 T_DATA_B=int32 API_IO=1 UUT_SSR=1 DIM_SIZE_A=8192 DIM_SIZE_B=8 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=0"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=2 T_DATA_A=cint16 T_DATA_B=int32 API_IO=1 UUT_SSR=1 DIM_SIZE_A=8192 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=0"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=2 T_DATA_A=cint32 T_DATA_B=cint32 API_IO=1 UUT_SSR=1 DIM_SIZE_A=1024 DIM_SIZE_B=8 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=0"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=2 T_DATA_A=cint32 T_DATA_B=cint32 API_IO=1 UUT_SSR=1 DIM_SIZE_A=1024 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=0"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=2 T_DATA_A=cint32 T_DATA_B=cint32 API_IO=1 UUT_SSR=1 DIM_SIZE_A=4096 DIM_SIZE_B=8 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=0"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=2 T_DATA_A=cint32 T_DATA_B=cint32 API_IO=1 UUT_SSR=1 DIM_SIZE_A=4096 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=0"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=2 T_DATA_A=cint32 T_DATA_B=int16 API_IO=1 UUT_SSR=1 DIM_SIZE_A=1024 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=0"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=2 T_DATA_A=cint32 T_DATA_B=int16 API_IO=1 UUT_SSR=1 DIM_SIZE_A=4096 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=0"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=2 T_DATA_A=cint32 T_DATA_B=int32 API_IO=1 UUT_SSR=1 DIM_SIZE_A=1024 DIM_SIZE_B=8 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=0"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=2 T_DATA_A=cint32 T_DATA_B=int32 API_IO=1 UUT_SSR=1 DIM_SIZE_A=1024 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=0"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=2 T_DATA_A=cint32 T_DATA_B=int32 API_IO=1 UUT_SSR=1 DIM_SIZE_A=4096 DIM_SIZE_B=8 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=0"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=2 T_DATA_A=cint32 T_DATA_B=int32 API_IO=1 UUT_SSR=1 DIM_SIZE_A=4096 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=0"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=2 T_DATA_A=int16 T_DATA_B=int16 API_IO=1 UUT_SSR=1 DIM_SIZE_A=1024 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=0"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=2 T_DATA_A=int16 T_DATA_B=int16 API_IO=1 UUT_SSR=1 DIM_SIZE_A=4096 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=0"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=2 T_DATA_A=int16 T_DATA_B=int16 API_IO=1 UUT_SSR=1 DIM_SIZE_A=8192 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=0"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=2 T_DATA_A=int16 T_DATA_B=int32 API_IO=0 UUT_SSR=1 DIM_SIZE_A=1024 DIM_SIZE_B=8 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=0"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=2 T_DATA_A=int16 T_DATA_B=int32 API_IO=1 UUT_SSR=1 DIM_SIZE_A=1024 DIM_SIZE_B=8 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=0"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=2 T_DATA_A=int16 T_DATA_B=int32 API_IO=1 UUT_SSR=1 DIM_SIZE_A=1024 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=0"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=2 T_DATA_A=int16 T_DATA_B=int32 API_IO=1 UUT_SSR=1 DIM_SIZE_A=4096 DIM_SIZE_B=8 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=0"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=2 T_DATA_A=int16 T_DATA_B=int32 API_IO=1 UUT_SSR=1 DIM_SIZE_A=4096 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=0"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=2 T_DATA_A=int16 T_DATA_B=int32 API_IO=1 UUT_SSR=1 DIM_SIZE_A=8192 DIM_SIZE_B=8 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=0"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=2 T_DATA_A=int16 T_DATA_B=int32 API_IO=1 UUT_SSR=1 DIM_SIZE_A=8192 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=0"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=2 T_DATA_A=int32 T_DATA_B=int32 API_IO=0 UUT_SSR=1 DIM_SIZE_A=1024 DIM_SIZE_B=8 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=0"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=2 T_DATA_A=int32 T_DATA_B=int32 API_IO=1 UUT_SSR=1 DIM_SIZE_A=1024 DIM_SIZE_B=8 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=0"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=2 T_DATA_A=int32 T_DATA_B=int32 API_IO=1 UUT_SSR=1 DIM_SIZE_A=1024 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=0"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=2 T_DATA_A=int32 T_DATA_B=int32 API_IO=1 UUT_SSR=1 DIM_SIZE_A=4096 DIM_SIZE_B=8 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=0"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=2 T_DATA_A=int32 T_DATA_B=int32 API_IO=1 UUT_SSR=1 DIM_SIZE_A=4096 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=0"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=2 T_DATA_A=int32 T_DATA_B=int32 API_IO=1 UUT_SSR=1 DIM_SIZE_A=8192 DIM_SIZE_B=8 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=0"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=2 T_DATA_A=int32 T_DATA_B=int32 API_IO=1 UUT_SSR=1 DIM_SIZE_A=8192 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=0"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=22 T_DATA_A=cfloat T_DATA_B=cfloat API_IO=1 UUT_SSR=1 DIM_SIZE_A=1024 DIM_SIZE_B=8 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=0 DIFF_MODE=PERCENT DIFF_TOLERANCE=0.01 CC_TOLERANCE=0.01"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=22 T_DATA_A=cfloat T_DATA_B=cfloat API_IO=1 UUT_SSR=1 DIM_SIZE_A=1024 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=0 DIFF_MODE=PERCENT DIFF_TOLERANCE=0.01 CC_TOLERANCE=0.01"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=22 T_DATA_A=cfloat T_DATA_B=cfloat API_IO=1 UUT_SSR=1 DIM_SIZE_A=4096 DIM_SIZE_B=8 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=0 DIFF_MODE=PERCENT DIFF_TOLERANCE=0.01 CC_TOLERANCE=0.01"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=22 T_DATA_A=cfloat T_DATA_B=cfloat API_IO=1 UUT_SSR=1 DIM_SIZE_A=4096 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=0 DIFF_MODE=PERCENT DIFF_TOLERANCE=0.01 CC_TOLERANCE=0.01"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=22 T_DATA_A=cfloat T_DATA_B=float API_IO=1 UUT_SSR=1 DIM_SIZE_A=1024 DIM_SIZE_B=8 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=0 DIFF_MODE=PERCENT DIFF_TOLERANCE=0.01 CC_TOLERANCE=0.01"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=22 T_DATA_A=cfloat T_DATA_B=float API_IO=1 UUT_SSR=1 DIM_SIZE_A=1024 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=0 DIFF_MODE=PERCENT DIFF_TOLERANCE=0.01 CC_TOLERANCE=0.01"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=22 T_DATA_A=cfloat T_DATA_B=float API_IO=1 UUT_SSR=1 DIM_SIZE_A=4096 DIM_SIZE_B=8 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=0 DIFF_MODE=PERCENT DIFF_TOLERANCE=0.01 CC_TOLERANCE=0.01"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=22 T_DATA_A=cfloat T_DATA_B=float API_IO=1 UUT_SSR=1 DIM_SIZE_A=4096 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=0 DIFF_MODE=PERCENT DIFF_TOLERANCE=0.01 CC_TOLERANCE=0.01"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=22 T_DATA_A=float T_DATA_B=float API_IO=0 UUT_SSR=1 DIM_SIZE_A=1024 DIM_SIZE_B=8 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=0 DIFF_MODE=PERCENT DIFF_TOLERANCE=0.01 CC_TOLERANCE=0.01"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=22 T_DATA_A=float T_DATA_B=float API_IO=1 UUT_SSR=1 DIM_SIZE_A=1024 DIM_SIZE_B=8 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=0 DIFF_MODE=PERCENT DIFF_TOLERANCE=0.01 CC_TOLERANCE=0.01"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=22 T_DATA_A=float T_DATA_B=float API_IO=1 UUT_SSR=1 DIM_SIZE_A=1024 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=0 DIFF_MODE=PERCENT DIFF_TOLERANCE=0.01 CC_TOLERANCE=0.01"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=22 T_DATA_A=float T_DATA_B=float API_IO=1 UUT_SSR=1 DIM_SIZE_A=4096 DIM_SIZE_B=8 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=0 DIFF_MODE=PERCENT DIFF_TOLERANCE=0.01 CC_TOLERANCE=0.01"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=22 T_DATA_A=float T_DATA_B=float API_IO=1 UUT_SSR=1 DIM_SIZE_A=4096 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=0 DIFF_MODE=PERCENT DIFF_TOLERANCE=0.01 CC_TOLERANCE=0.01"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=22 T_DATA_A=float T_DATA_B=float API_IO=1 UUT_SSR=1 DIM_SIZE_A=8192 DIM_SIZE_B=8 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=0 DIFF_MODE=PERCENT DIFF_TOLERANCE=0.01 CC_TOLERANCE=0.01"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=22 T_DATA_A=float T_DATA_B=float API_IO=1 UUT_SSR=1 DIM_SIZE_A=8192 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=0 DIFF_MODE=PERCENT DIFF_TOLERANCE=0.01 CC_TOLERANCE=0.01"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=22 T_DATA_A=cint16 T_DATA_B=cint16 API_IO=0 UUT_SSR=1 DIM_SIZE_A=1024 DIM_SIZE_B=8 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=0"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=22 T_DATA_A=cint16 T_DATA_B=cint16 API_IO=1 UUT_SSR=1 DIM_SIZE_A=1024 DIM_SIZE_B=8 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=0"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=22 T_DATA_A=cint16 T_DATA_B=cint16 API_IO=1 UUT_SSR=1 DIM_SIZE_A=1024 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=0"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=22 T_DATA_A=cint16 T_DATA_B=cint16 API_IO=1 UUT_SSR=1 DIM_SIZE_A=4096 DIM_SIZE_B=8 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=0"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=22 T_DATA_A=cint16 T_DATA_B=cint16 API_IO=1 UUT_SSR=1 DIM_SIZE_A=4096 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=0"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=22 T_DATA_A=cint16 T_DATA_B=cint16 API_IO=1 UUT_SSR=1 DIM_SIZE_A=8192 DIM_SIZE_B=8 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=0"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=22 T_DATA_A=cint16 T_DATA_B=cint16 API_IO=1 UUT_SSR=1 DIM_SIZE_A=8192 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=0"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=22 T_DATA_A=cint16 T_DATA_B=cint32 API_IO=1 UUT_SSR=1 DIM_SIZE_A=1024 DIM_SIZE_B=8 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=0"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=22 T_DATA_A=cint16 T_DATA_B=cint32 API_IO=1 UUT_SSR=1 DIM_SIZE_A=1024 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=0"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=22 T_DATA_A=cint16 T_DATA_B=cint32 API_IO=1 UUT_SSR=1 DIM_SIZE_A=4096 DIM_SIZE_B=8 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=0"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=22 T_DATA_A=cint16 T_DATA_B=cint32 API_IO=1 UUT_SSR=1 DIM_SIZE_A=4096 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=0"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=22 T_DATA_A=cint16 T_DATA_B=cint32 API_IO=1 UUT_SSR=1 DIM_SIZE_A=8192 DIM_SIZE_B=8 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=0"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=22 T_DATA_A=cint16 T_DATA_B=cint32 API_IO=1 UUT_SSR=1 DIM_SIZE_A=8192 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=0"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=22 T_DATA_A=cint16 T_DATA_B=int16 API_IO=1 UUT_SSR=1 DIM_SIZE_A=1024 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=0"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=22 T_DATA_A=cint16 T_DATA_B=int16 API_IO=1 UUT_SSR=1 DIM_SIZE_A=4096 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=0"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=22 T_DATA_A=cint16 T_DATA_B=int16 API_IO=1 UUT_SSR=1 DIM_SIZE_A=8192 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=0"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=22 T_DATA_A=cint16 T_DATA_B=int32 API_IO=1 UUT_SSR=1 DIM_SIZE_A=1024 DIM_SIZE_B=8 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=0"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=22 T_DATA_A=cint16 T_DATA_B=int32 API_IO=1 UUT_SSR=1 DIM_SIZE_A=1024 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=0"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=22 T_DATA_A=cint16 T_DATA_B=int32 API_IO=1 UUT_SSR=1 DIM_SIZE_A=4096 DIM_SIZE_B=8 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=0"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=22 T_DATA_A=cint16 T_DATA_B=int32 API_IO=1 UUT_SSR=1 DIM_SIZE_A=4096 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=0"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=22 T_DATA_A=cint16 T_DATA_B=int32 API_IO=1 UUT_SSR=1 DIM_SIZE_A=8192 DIM_SIZE_B=8 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=0"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=22 T_DATA_A=cint16 T_DATA_B=int32 API_IO=1 UUT_SSR=1 DIM_SIZE_A=8192 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=0"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=22 T_DATA_A=cint32 T_DATA_B=cint32 API_IO=1 UUT_SSR=1 DIM_SIZE_A=1024 DIM_SIZE_B=8 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=0"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=22 T_DATA_A=cint32 T_DATA_B=cint32 API_IO=1 UUT_SSR=1 DIM_SIZE_A=1024 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=0"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=22 T_DATA_A=cint32 T_DATA_B=cint32 API_IO=1 UUT_SSR=1 DIM_SIZE_A=4096 DIM_SIZE_B=8 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=0"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=22 T_DATA_A=cint32 T_DATA_B=cint32 API_IO=1 UUT_SSR=1 DIM_SIZE_A=4096 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=0"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=22 T_DATA_A=cint32 T_DATA_B=int16 API_IO=1 UUT_SSR=1 DIM_SIZE_A=1024 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=0"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=22 T_DATA_A=cint32 T_DATA_B=int16 API_IO=1 UUT_SSR=1 DIM_SIZE_A=4096 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=0"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=22 T_DATA_A=cint32 T_DATA_B=int32 API_IO=1 UUT_SSR=1 DIM_SIZE_A=1024 DIM_SIZE_B=8 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=0"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=22 T_DATA_A=cint32 T_DATA_B=int32 API_IO=1 UUT_SSR=1 DIM_SIZE_A=1024 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=0"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=22 T_DATA_A=cint32 T_DATA_B=int32 API_IO=1 UUT_SSR=1 DIM_SIZE_A=4096 DIM_SIZE_B=8 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=0"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=22 T_DATA_A=cint32 T_DATA_B=int32 API_IO=1 UUT_SSR=1 DIM_SIZE_A=4096 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=0"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=22 T_DATA_A=int16 T_DATA_B=int16 API_IO=1 UUT_SSR=1 DIM_SIZE_A=1024 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=0"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=22 T_DATA_A=int16 T_DATA_B=int16 API_IO=1 UUT_SSR=1 DIM_SIZE_A=4096 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=0"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=22 T_DATA_A=int16 T_DATA_B=int16 API_IO=1 UUT_SSR=1 DIM_SIZE_A=8192 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=0"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=22 T_DATA_A=int16 T_DATA_B=int32 API_IO=0 UUT_SSR=1 DIM_SIZE_A=1024 DIM_SIZE_B=8 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=0"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=22 T_DATA_A=int16 T_DATA_B=int32 API_IO=1 UUT_SSR=1 DIM_SIZE_A=1024 DIM_SIZE_B=8 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=0"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=22 T_DATA_A=int16 T_DATA_B=int32 API_IO=1 UUT_SSR=1 DIM_SIZE_A=1024 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=0"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=22 T_DATA_A=int16 T_DATA_B=int32 API_IO=1 UUT_SSR=1 DIM_SIZE_A=4096 DIM_SIZE_B=8 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=0"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=22 T_DATA_A=int16 T_DATA_B=int32 API_IO=1 UUT_SSR=1 DIM_SIZE_A=4096 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=0"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=22 T_DATA_A=int16 T_DATA_B=int32 API_IO=1 UUT_SSR=1 DIM_SIZE_A=8192 DIM_SIZE_B=8 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=0"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=22 T_DATA_A=int16 T_DATA_B=int32 API_IO=1 UUT_SSR=1 DIM_SIZE_A=8192 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=0"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=22 T_DATA_A=int32 T_DATA_B=int32 API_IO=0 UUT_SSR=1 DIM_SIZE_A=1024 DIM_SIZE_B=8 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=0"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=22 T_DATA_A=int32 T_DATA_B=int32 API_IO=1 UUT_SSR=1 DIM_SIZE_A=1024 DIM_SIZE_B=8 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=0"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=22 T_DATA_A=int32 T_DATA_B=int32 API_IO=1 UUT_SSR=1 DIM_SIZE_A=1024 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=0"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=22 T_DATA_A=int32 T_DATA_B=int32 API_IO=1 UUT_SSR=1 DIM_SIZE_A=4096 DIM_SIZE_B=8 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=0"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=22 T_DATA_A=int32 T_DATA_B=int32 API_IO=1 UUT_SSR=1 DIM_SIZE_A=4096 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=0"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=22 T_DATA_A=int32 T_DATA_B=int32 API_IO=1 UUT_SSR=1 DIM_SIZE_A=8192 DIM_SIZE_B=8 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=0"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=22 T_DATA_A=int32 T_DATA_B=int32 API_IO=1 UUT_SSR=1 DIM_SIZE_A=8192 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=0"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=1 T_DATA_A=cfloat T_DATA_B=cfloat API_IO=1 UUT_SSR=1 DIM_SIZE_A=1024 DIM_SIZE_B=8 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1 DIFF_MODE=PERCENT DIFF_TOLERANCE=0.01 CC_TOLERANCE=0.01"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=1 T_DATA_A=cfloat T_DATA_B=cfloat API_IO=1 UUT_SSR=1 DIM_SIZE_A=1024 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1 DIFF_MODE=PERCENT DIFF_TOLERANCE=0.01 CC_TOLERANCE=0.01"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=1 T_DATA_A=cfloat T_DATA_B=cfloat API_IO=1 UUT_SSR=1 DIM_SIZE_A=4096 DIM_SIZE_B=8 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1 DIFF_MODE=PERCENT DIFF_TOLERANCE=0.01 CC_TOLERANCE=0.01"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=1 T_DATA_A=cfloat T_DATA_B=cfloat API_IO=1 UUT_SSR=1 DIM_SIZE_A=4096 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1 DIFF_MODE=PERCENT DIFF_TOLERANCE=0.01 CC_TOLERANCE=0.01"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=1 T_DATA_A=cfloat T_DATA_B=float API_IO=1 UUT_SSR=1 DIM_SIZE_A=1024 DIM_SIZE_B=8 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1 DIFF_MODE=PERCENT DIFF_TOLERANCE=0.01 CC_TOLERANCE=0.01"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=1 T_DATA_A=cfloat T_DATA_B=float API_IO=1 UUT_SSR=1 DIM_SIZE_A=1024 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1 DIFF_MODE=PERCENT DIFF_TOLERANCE=0.01 CC_TOLERANCE=0.01"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=1 T_DATA_A=cfloat T_DATA_B=float API_IO=1 UUT_SSR=1 DIM_SIZE_A=4096 DIM_SIZE_B=8 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1 DIFF_MODE=PERCENT DIFF_TOLERANCE=0.01 CC_TOLERANCE=0.01"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=1 T_DATA_A=cfloat T_DATA_B=float API_IO=1 UUT_SSR=1 DIM_SIZE_A=4096 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1 DIFF_MODE=PERCENT DIFF_TOLERANCE=0.01 CC_TOLERANCE=0.01"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=1 T_DATA_A=float T_DATA_B=float API_IO=0 UUT_SSR=1 DIM_SIZE_A=1024 DIM_SIZE_B=8 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1 DIFF_MODE=PERCENT DIFF_TOLERANCE=0.01 CC_TOLERANCE=0.01"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=1 T_DATA_A=float T_DATA_B=float API_IO=1 UUT_SSR=1 DIM_SIZE_A=1024 DIM_SIZE_B=8 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1 DIFF_MODE=PERCENT DIFF_TOLERANCE=0.01 CC_TOLERANCE=0.01"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=1 T_DATA_A=float T_DATA_B=float API_IO=1 UUT_SSR=1 DIM_SIZE_A=1024 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1 DIFF_MODE=PERCENT DIFF_TOLERANCE=0.01 CC_TOLERANCE=0.01"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=1 T_DATA_A=float T_DATA_B=float API_IO=1 UUT_SSR=1 DIM_SIZE_A=4096 DIM_SIZE_B=8 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1 DIFF_MODE=PERCENT DIFF_TOLERANCE=0.01 CC_TOLERANCE=0.01"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=1 T_DATA_A=float T_DATA_B=float API_IO=1 UUT_SSR=1 DIM_SIZE_A=4096 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1 DIFF_MODE=PERCENT DIFF_TOLERANCE=0.01 CC_TOLERANCE=0.01"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=1 T_DATA_A=float T_DATA_B=float API_IO=1 UUT_SSR=1 DIM_SIZE_A=8192 DIM_SIZE_B=8 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1 DIFF_MODE=PERCENT DIFF_TOLERANCE=0.01 CC_TOLERANCE=0.01"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=1 T_DATA_A=float T_DATA_B=float API_IO=1 UUT_SSR=1 DIM_SIZE_A=8192 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1 DIFF_MODE=PERCENT DIFF_TOLERANCE=0.01 CC_TOLERANCE=0.01"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=1 T_DATA_A=cint16 T_DATA_B=cint16 API_IO=0 UUT_SSR=1 DIM_SIZE_A=1024 DIM_SIZE_B=8 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=1 T_DATA_A=cint16 T_DATA_B=cint16 API_IO=1 UUT_SSR=1 DIM_SIZE_A=1024 DIM_SIZE_B=8 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=1 T_DATA_A=cint16 T_DATA_B=cint16 API_IO=1 UUT_SSR=1 DIM_SIZE_A=1024 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=1 T_DATA_A=cint16 T_DATA_B=cint16 API_IO=1 UUT_SSR=1 DIM_SIZE_A=4096 DIM_SIZE_B=8 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=1 T_DATA_A=cint16 T_DATA_B=cint16 API_IO=1 UUT_SSR=1 DIM_SIZE_A=4096 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=1 T_DATA_A=cint16 T_DATA_B=cint16 API_IO=1 UUT_SSR=1 DIM_SIZE_A=8192 DIM_SIZE_B=8 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=1 T_DATA_A=cint16 T_DATA_B=cint16 API_IO=1 UUT_SSR=1 DIM_SIZE_A=8192 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=1 T_DATA_A=cint16 T_DATA_B=cint32 API_IO=1 UUT_SSR=1 DIM_SIZE_A=1024 DIM_SIZE_B=8 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=1 T_DATA_A=cint16 T_DATA_B=cint32 API_IO=1 UUT_SSR=1 DIM_SIZE_A=1024 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=1 T_DATA_A=cint16 T_DATA_B=cint32 API_IO=1 UUT_SSR=1 DIM_SIZE_A=4096 DIM_SIZE_B=8 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=1 T_DATA_A=cint16 T_DATA_B=cint32 API_IO=1 UUT_SSR=1 DIM_SIZE_A=4096 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=1 T_DATA_A=cint16 T_DATA_B=cint32 API_IO=1 UUT_SSR=1 DIM_SIZE_A=8192 DIM_SIZE_B=8 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=1 T_DATA_A=cint16 T_DATA_B=cint32 API_IO=1 UUT_SSR=1 DIM_SIZE_A=8192 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=1 T_DATA_A=cint16 T_DATA_B=int16 API_IO=1 UUT_SSR=1 DIM_SIZE_A=1024 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=1 T_DATA_A=cint16 T_DATA_B=int16 API_IO=1 UUT_SSR=1 DIM_SIZE_A=4096 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=1 T_DATA_A=cint16 T_DATA_B=int16 API_IO=1 UUT_SSR=1 DIM_SIZE_A=8192 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=1 T_DATA_A=cint16 T_DATA_B=int32 API_IO=1 UUT_SSR=1 DIM_SIZE_A=1024 DIM_SIZE_B=8 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=1 T_DATA_A=cint16 T_DATA_B=int32 API_IO=1 UUT_SSR=1 DIM_SIZE_A=1024 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=1 T_DATA_A=cint16 T_DATA_B=int32 API_IO=1 UUT_SSR=1 DIM_SIZE_A=4096 DIM_SIZE_B=8 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=1 T_DATA_A=cint16 T_DATA_B=int32 API_IO=1 UUT_SSR=1 DIM_SIZE_A=4096 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=1 T_DATA_A=cint16 T_DATA_B=int32 API_IO=1 UUT_SSR=1 DIM_SIZE_A=8192 DIM_SIZE_B=8 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=1 T_DATA_A=cint16 T_DATA_B=int32 API_IO=1 UUT_SSR=1 DIM_SIZE_A=8192 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=1 T_DATA_A=cint32 T_DATA_B=cint32 API_IO=1 UUT_SSR=1 DIM_SIZE_A=1024 DIM_SIZE_B=8 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=1 T_DATA_A=cint32 T_DATA_B=cint32 API_IO=1 UUT_SSR=1 DIM_SIZE_A=1024 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=1 T_DATA_A=cint32 T_DATA_B=cint32 API_IO=1 UUT_SSR=1 DIM_SIZE_A=4096 DIM_SIZE_B=8 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=1 T_DATA_A=cint32 T_DATA_B=cint32 API_IO=1 UUT_SSR=1 DIM_SIZE_A=4096 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=1 T_DATA_A=cint32 T_DATA_B=int16 API_IO=1 UUT_SSR=1 DIM_SIZE_A=1024 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=1 T_DATA_A=cint32 T_DATA_B=int16 API_IO=1 UUT_SSR=1 DIM_SIZE_A=4096 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=1 T_DATA_A=cint32 T_DATA_B=int32 API_IO=1 UUT_SSR=1 DIM_SIZE_A=1024 DIM_SIZE_B=8 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=1 T_DATA_A=cint32 T_DATA_B=int32 API_IO=1 UUT_SSR=1 DIM_SIZE_A=1024 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=1 T_DATA_A=cint32 T_DATA_B=int32 API_IO=1 UUT_SSR=1 DIM_SIZE_A=4096 DIM_SIZE_B=8 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=1 T_DATA_A=cint32 T_DATA_B=int32 API_IO=1 UUT_SSR=1 DIM_SIZE_A=4096 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=1 T_DATA_A=int16 T_DATA_B=int16 API_IO=1 UUT_SSR=1 DIM_SIZE_A=1024 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=1 T_DATA_A=int16 T_DATA_B=int16 API_IO=1 UUT_SSR=1 DIM_SIZE_A=4096 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=1 T_DATA_A=int16 T_DATA_B=int16 API_IO=1 UUT_SSR=1 DIM_SIZE_A=8192 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=1 T_DATA_A=int16 T_DATA_B=int32 API_IO=0 UUT_SSR=1 DIM_SIZE_A=1024 DIM_SIZE_B=8 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=1 T_DATA_A=int16 T_DATA_B=int32 API_IO=1 UUT_SSR=1 DIM_SIZE_A=1024 DIM_SIZE_B=8 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=1 T_DATA_A=int16 T_DATA_B=int32 API_IO=1 UUT_SSR=1 DIM_SIZE_A=1024 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=1 T_DATA_A=int16 T_DATA_B=int32 API_IO=1 UUT_SSR=1 DIM_SIZE_A=4096 DIM_SIZE_B=8 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=1 T_DATA_A=int16 T_DATA_B=int32 API_IO=1 UUT_SSR=1 DIM_SIZE_A=4096 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=1 T_DATA_A=int16 T_DATA_B=int32 API_IO=1 UUT_SSR=1 DIM_SIZE_A=8192 DIM_SIZE_B=8 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=1 T_DATA_A=int16 T_DATA_B=int32 API_IO=1 UUT_SSR=1 DIM_SIZE_A=8192 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=1 T_DATA_A=int32 T_DATA_B=int32 API_IO=0 UUT_SSR=1 DIM_SIZE_A=1024 DIM_SIZE_B=8 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=1 T_DATA_A=int32 T_DATA_B=int32 API_IO=1 UUT_SSR=1 DIM_SIZE_A=1024 DIM_SIZE_B=8 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=1 T_DATA_A=int32 T_DATA_B=int32 API_IO=1 UUT_SSR=1 DIM_SIZE_A=1024 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=1 T_DATA_A=int32 T_DATA_B=int32 API_IO=1 UUT_SSR=1 DIM_SIZE_A=4096 DIM_SIZE_B=8 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=1 T_DATA_A=int32 T_DATA_B=int32 API_IO=1 UUT_SSR=1 DIM_SIZE_A=4096 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=1 T_DATA_A=int32 T_DATA_B=int32 API_IO=1 UUT_SSR=1 DIM_SIZE_A=8192 DIM_SIZE_B=8 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=1 T_DATA_A=int32 T_DATA_B=int32 API_IO=1 UUT_SSR=1 DIM_SIZE_A=8192 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=2 T_DATA_A=cfloat T_DATA_B=cfloat API_IO=0 UUT_SSR=1 DIM_SIZE_A=1024 DIM_SIZE_B=8 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1 DIFF_MODE=PERCENT DIFF_TOLERANCE=0.01 CC_TOLERANCE=0.01"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=2 T_DATA_A=cfloat T_DATA_B=cfloat API_IO=1 UUT_SSR=1 DIM_SIZE_A=1024 DIM_SIZE_B=8 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1 DIFF_MODE=PERCENT DIFF_TOLERANCE=0.01 CC_TOLERANCE=0.01"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=2 T_DATA_A=cfloat T_DATA_B=cfloat API_IO=1 UUT_SSR=1 DIM_SIZE_A=1024 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1 DIFF_MODE=PERCENT DIFF_TOLERANCE=0.01 CC_TOLERANCE=0.01"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=2 T_DATA_A=cfloat T_DATA_B=cfloat API_IO=1 UUT_SSR=1 DIM_SIZE_A=4096 DIM_SIZE_B=8 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1 DIFF_MODE=PERCENT DIFF_TOLERANCE=0.01 CC_TOLERANCE=0.01"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=2 T_DATA_A=cfloat T_DATA_B=cfloat API_IO=1 UUT_SSR=1 DIM_SIZE_A=4096 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1 DIFF_MODE=PERCENT DIFF_TOLERANCE=0.01 CC_TOLERANCE=0.01"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=2 T_DATA_A=cfloat T_DATA_B=cfloat API_IO=1 UUT_SSR=1 DIM_SIZE_A=8192 DIM_SIZE_B=8 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1 DIFF_MODE=PERCENT DIFF_TOLERANCE=0.01 CC_TOLERANCE=0.01"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=2 T_DATA_A=cfloat T_DATA_B=cfloat API_IO=1 UUT_SSR=1 DIM_SIZE_A=8192 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1 DIFF_MODE=PERCENT DIFF_TOLERANCE=0.01 CC_TOLERANCE=0.01"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=2 T_DATA_A=cfloat T_DATA_B=float API_IO=0 UUT_SSR=1 DIM_SIZE_A=1024 DIM_SIZE_B=8 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1 DIFF_MODE=PERCENT DIFF_TOLERANCE=0.01 CC_TOLERANCE=0.01"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=2 T_DATA_A=cfloat T_DATA_B=float API_IO=1 UUT_SSR=1 DIM_SIZE_A=1024 DIM_SIZE_B=8 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1 DIFF_MODE=PERCENT DIFF_TOLERANCE=0.01 CC_TOLERANCE=0.01"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=2 T_DATA_A=cfloat T_DATA_B=float API_IO=1 UUT_SSR=1 DIM_SIZE_A=1024 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1 DIFF_MODE=PERCENT DIFF_TOLERANCE=0.01 CC_TOLERANCE=0.01"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=2 T_DATA_A=cfloat T_DATA_B=float API_IO=1 UUT_SSR=1 DIM_SIZE_A=4096 DIM_SIZE_B=8 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1 DIFF_MODE=PERCENT DIFF_TOLERANCE=0.01 CC_TOLERANCE=0.01"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=2 T_DATA_A=cfloat T_DATA_B=float API_IO=1 UUT_SSR=1 DIM_SIZE_A=4096 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1 DIFF_MODE=PERCENT DIFF_TOLERANCE=0.01 CC_TOLERANCE=0.01"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=2 T_DATA_A=cfloat T_DATA_B=float API_IO=1 UUT_SSR=1 DIM_SIZE_A=8192 DIM_SIZE_B=8 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1 DIFF_MODE=PERCENT DIFF_TOLERANCE=0.01 CC_TOLERANCE=0.01"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=2 T_DATA_A=cfloat T_DATA_B=float API_IO=1 UUT_SSR=1 DIM_SIZE_A=8192 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1 DIFF_MODE=PERCENT DIFF_TOLERANCE=0.01 CC_TOLERANCE=0.01"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=2 T_DATA_A=float T_DATA_B=float API_IO=0 UUT_SSR=1 DIM_SIZE_A=1024 DIM_SIZE_B=8 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1 DIFF_MODE=PERCENT DIFF_TOLERANCE=0.01 CC_TOLERANCE=0.01"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=2 T_DATA_A=float T_DATA_B=float API_IO=1 UUT_SSR=1 DIM_SIZE_A=1024 DIM_SIZE_B=8 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1 DIFF_MODE=PERCENT DIFF_TOLERANCE=0.01 CC_TOLERANCE=0.01"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=2 T_DATA_A=float T_DATA_B=float API_IO=1 UUT_SSR=1 DIM_SIZE_A=1024 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1 DIFF_MODE=PERCENT DIFF_TOLERANCE=0.01 CC_TOLERANCE=0.01"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=2 T_DATA_A=float T_DATA_B=float API_IO=1 UUT_SSR=1 DIM_SIZE_A=4096 DIM_SIZE_B=8 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1 DIFF_MODE=PERCENT DIFF_TOLERANCE=0.01 CC_TOLERANCE=0.01"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=2 T_DATA_A=float T_DATA_B=float API_IO=1 UUT_SSR=1 DIM_SIZE_A=4096 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1 DIFF_MODE=PERCENT DIFF_TOLERANCE=0.01 CC_TOLERANCE=0.01"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=2 T_DATA_A=float T_DATA_B=float API_IO=1 UUT_SSR=1 DIM_SIZE_A=8192 DIM_SIZE_B=8 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1 DIFF_MODE=PERCENT DIFF_TOLERANCE=0.01 CC_TOLERANCE=0.01"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=2 T_DATA_A=float T_DATA_B=float API_IO=1 UUT_SSR=1 DIM_SIZE_A=8192 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1 DIFF_MODE=PERCENT DIFF_TOLERANCE=0.01 CC_TOLERANCE=0.01"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=2 T_DATA_A=cint16 T_DATA_B=cint16 API_IO=0 UUT_SSR=1 DIM_SIZE_A=1024 DIM_SIZE_B=8 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=2 T_DATA_A=cint16 T_DATA_B=cint16 API_IO=1 UUT_SSR=1 DIM_SIZE_A=1024 DIM_SIZE_B=8 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=2 T_DATA_A=cint16 T_DATA_B=cint16 API_IO=1 UUT_SSR=1 DIM_SIZE_A=1024 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=2 T_DATA_A=cint16 T_DATA_B=cint16 API_IO=1 UUT_SSR=1 DIM_SIZE_A=4096 DIM_SIZE_B=8 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=2 T_DATA_A=cint16 T_DATA_B=cint16 API_IO=1 UUT_SSR=1 DIM_SIZE_A=4096 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=2 T_DATA_A=cint16 T_DATA_B=cint16 API_IO=1 UUT_SSR=1 DIM_SIZE_A=8192 DIM_SIZE_B=8 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=2 T_DATA_A=cint16 T_DATA_B=cint16 API_IO=1 UUT_SSR=1 DIM_SIZE_A=8192 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=2 T_DATA_A=cint16 T_DATA_B=cint32 API_IO=0 UUT_SSR=1 DIM_SIZE_A=1024 DIM_SIZE_B=8 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=2 T_DATA_A=cint16 T_DATA_B=cint32 API_IO=1 UUT_SSR=1 DIM_SIZE_A=1024 DIM_SIZE_B=8 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=2 T_DATA_A=cint16 T_DATA_B=cint32 API_IO=1 UUT_SSR=1 DIM_SIZE_A=1024 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=2 T_DATA_A=cint16 T_DATA_B=cint32 API_IO=1 UUT_SSR=1 DIM_SIZE_A=4096 DIM_SIZE_B=8 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=2 T_DATA_A=cint16 T_DATA_B=cint32 API_IO=1 UUT_SSR=1 DIM_SIZE_A=4096 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=2 T_DATA_A=cint16 T_DATA_B=cint32 API_IO=1 UUT_SSR=1 DIM_SIZE_A=8192 DIM_SIZE_B=8 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=2 T_DATA_A=cint16 T_DATA_B=cint32 API_IO=1 UUT_SSR=1 DIM_SIZE_A=8192 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=2 T_DATA_A=cint16 T_DATA_B=int16 API_IO=1 UUT_SSR=1 DIM_SIZE_A=1024 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=2 T_DATA_A=cint16 T_DATA_B=int16 API_IO=1 UUT_SSR=1 DIM_SIZE_A=4096 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=2 T_DATA_A=cint16 T_DATA_B=int16 API_IO=1 UUT_SSR=1 DIM_SIZE_A=8192 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=2 T_DATA_A=cint16 T_DATA_B=int32 API_IO=0 UUT_SSR=1 DIM_SIZE_A=1024 DIM_SIZE_B=8 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=2 T_DATA_A=cint16 T_DATA_B=int32 API_IO=1 UUT_SSR=1 DIM_SIZE_A=1024 DIM_SIZE_B=8 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=2 T_DATA_A=cint16 T_DATA_B=int32 API_IO=1 UUT_SSR=1 DIM_SIZE_A=1024 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=2 T_DATA_A=cint16 T_DATA_B=int32 API_IO=1 UUT_SSR=1 DIM_SIZE_A=4096 DIM_SIZE_B=8 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=2 T_DATA_A=cint16 T_DATA_B=int32 API_IO=1 UUT_SSR=1 DIM_SIZE_A=4096 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=2 T_DATA_A=cint16 T_DATA_B=int32 API_IO=1 UUT_SSR=1 DIM_SIZE_A=8192 DIM_SIZE_B=8 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=2 T_DATA_A=cint16 T_DATA_B=int32 API_IO=1 UUT_SSR=1 DIM_SIZE_A=8192 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=2 T_DATA_A=cint32 T_DATA_B=cint32 API_IO=0 UUT_SSR=1 DIM_SIZE_A=1024 DIM_SIZE_B=8 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=2 T_DATA_A=cint32 T_DATA_B=cint32 API_IO=1 UUT_SSR=1 DIM_SIZE_A=1024 DIM_SIZE_B=8 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=2 T_DATA_A=cint32 T_DATA_B=cint32 API_IO=1 UUT_SSR=1 DIM_SIZE_A=1024 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=2 T_DATA_A=cint32 T_DATA_B=cint32 API_IO=1 UUT_SSR=1 DIM_SIZE_A=4096 DIM_SIZE_B=8 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=2 T_DATA_A=cint32 T_DATA_B=cint32 API_IO=1 UUT_SSR=1 DIM_SIZE_A=4096 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=2 T_DATA_A=cint32 T_DATA_B=cint32 API_IO=1 UUT_SSR=1 DIM_SIZE_A=8192 DIM_SIZE_B=8 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=2 T_DATA_A=cint32 T_DATA_B=cint32 API_IO=1 UUT_SSR=1 DIM_SIZE_A=8192 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=2 T_DATA_A=cint32 T_DATA_B=int16 API_IO=1 UUT_SSR=1 DIM_SIZE_A=1024 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=2 T_DATA_A=cint32 T_DATA_B=int16 API_IO=1 UUT_SSR=1 DIM_SIZE_A=4096 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=2 T_DATA_A=cint32 T_DATA_B=int16 API_IO=1 UUT_SSR=1 DIM_SIZE_A=8192 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=2 T_DATA_A=cint32 T_DATA_B=int32 API_IO=0 UUT_SSR=1 DIM_SIZE_A=1024 DIM_SIZE_B=8 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=2 T_DATA_A=cint32 T_DATA_B=int32 API_IO=1 UUT_SSR=1 DIM_SIZE_A=1024 DIM_SIZE_B=8 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=2 T_DATA_A=cint32 T_DATA_B=int32 API_IO=1 UUT_SSR=1 DIM_SIZE_A=1024 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=2 T_DATA_A=cint32 T_DATA_B=int32 API_IO=1 UUT_SSR=1 DIM_SIZE_A=4096 DIM_SIZE_B=8 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=2 T_DATA_A=cint32 T_DATA_B=int32 API_IO=1 UUT_SSR=1 DIM_SIZE_A=4096 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=2 T_DATA_A=cint32 T_DATA_B=int32 API_IO=1 UUT_SSR=1 DIM_SIZE_A=8192 DIM_SIZE_B=8 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=2 T_DATA_A=cint32 T_DATA_B=int32 API_IO=1 UUT_SSR=1 DIM_SIZE_A=8192 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=2 T_DATA_A=int16 T_DATA_B=int16 API_IO=1 UUT_SSR=1 DIM_SIZE_A=1024 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=2 T_DATA_A=int16 T_DATA_B=int16 API_IO=1 UUT_SSR=1 DIM_SIZE_A=4096 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=2 T_DATA_A=int16 T_DATA_B=int16 API_IO=1 UUT_SSR=1 DIM_SIZE_A=8192 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=2 T_DATA_A=int16 T_DATA_B=int32 API_IO=0 UUT_SSR=1 DIM_SIZE_A=1024 DIM_SIZE_B=8 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=2 T_DATA_A=int16 T_DATA_B=int32 API_IO=1 UUT_SSR=1 DIM_SIZE_A=1024 DIM_SIZE_B=8 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=2 T_DATA_A=int16 T_DATA_B=int32 API_IO=1 UUT_SSR=1 DIM_SIZE_A=1024 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=2 T_DATA_A=int16 T_DATA_B=int32 API_IO=1 UUT_SSR=1 DIM_SIZE_A=4096 DIM_SIZE_B=8 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=2 T_DATA_A=int16 T_DATA_B=int32 API_IO=1 UUT_SSR=1 DIM_SIZE_A=4096 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=2 T_DATA_A=int16 T_DATA_B=int32 API_IO=1 UUT_SSR=1 DIM_SIZE_A=8192 DIM_SIZE_B=8 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=2 T_DATA_A=int16 T_DATA_B=int32 API_IO=1 UUT_SSR=1 DIM_SIZE_A=8192 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=2 T_DATA_A=int32 T_DATA_B=int32 API_IO=0 UUT_SSR=1 DIM_SIZE_A=1024 DIM_SIZE_B=8 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=2 T_DATA_A=int32 T_DATA_B=int32 API_IO=1 UUT_SSR=1 DIM_SIZE_A=1024 DIM_SIZE_B=8 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=2 T_DATA_A=int32 T_DATA_B=int32 API_IO=1 UUT_SSR=1 DIM_SIZE_A=1024 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=2 T_DATA_A=int32 T_DATA_B=int32 API_IO=1 UUT_SSR=1 DIM_SIZE_A=4096 DIM_SIZE_B=8 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=2 T_DATA_A=int32 T_DATA_B=int32 API_IO=1 UUT_SSR=1 DIM_SIZE_A=4096 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=2 T_DATA_A=int32 T_DATA_B=int32 API_IO=1 UUT_SSR=1 DIM_SIZE_A=8192 DIM_SIZE_B=8 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=2 T_DATA_A=int32 T_DATA_B=int32 API_IO=1 UUT_SSR=1 DIM_SIZE_A=8192 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=22 T_DATA_A=cfloat T_DATA_B=cfloat API_IO=0 UUT_SSR=1 DIM_SIZE_A=1024 DIM_SIZE_B=8 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1 DIFF_MODE=PERCENT DIFF_TOLERANCE=0.01 CC_TOLERANCE=0.01"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=22 T_DATA_A=cfloat T_DATA_B=cfloat API_IO=1 UUT_SSR=1 DIM_SIZE_A=1024 DIM_SIZE_B=8 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1 DIFF_MODE=PERCENT DIFF_TOLERANCE=0.01 CC_TOLERANCE=0.01"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=22 T_DATA_A=cfloat T_DATA_B=cfloat API_IO=1 UUT_SSR=1 DIM_SIZE_A=1024 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1 DIFF_MODE=PERCENT DIFF_TOLERANCE=0.01 CC_TOLERANCE=0.01"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=22 T_DATA_A=cfloat T_DATA_B=cfloat API_IO=1 UUT_SSR=1 DIM_SIZE_A=4096 DIM_SIZE_B=8 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1 DIFF_MODE=PERCENT DIFF_TOLERANCE=0.01 CC_TOLERANCE=0.01"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=22 T_DATA_A=cfloat T_DATA_B=cfloat API_IO=1 UUT_SSR=1 DIM_SIZE_A=4096 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1 DIFF_MODE=PERCENT DIFF_TOLERANCE=0.01 CC_TOLERANCE=0.01"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=22 T_DATA_A=cfloat T_DATA_B=cfloat API_IO=1 UUT_SSR=1 DIM_SIZE_A=8192 DIM_SIZE_B=8 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1 DIFF_MODE=PERCENT DIFF_TOLERANCE=0.01 CC_TOLERANCE=0.01"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=22 T_DATA_A=cfloat T_DATA_B=cfloat API_IO=1 UUT_SSR=1 DIM_SIZE_A=8192 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1 DIFF_MODE=PERCENT DIFF_TOLERANCE=0.01 CC_TOLERANCE=0.01"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=22 T_DATA_A=cfloat T_DATA_B=float API_IO=0 UUT_SSR=1 DIM_SIZE_A=1024 DIM_SIZE_B=8 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1 DIFF_MODE=PERCENT DIFF_TOLERANCE=0.01 CC_TOLERANCE=0.01"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=22 T_DATA_A=cfloat T_DATA_B=float API_IO=1 UUT_SSR=1 DIM_SIZE_A=1024 DIM_SIZE_B=8 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1 DIFF_MODE=PERCENT DIFF_TOLERANCE=0.01 CC_TOLERANCE=0.01"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=22 T_DATA_A=cfloat T_DATA_B=float API_IO=1 UUT_SSR=1 DIM_SIZE_A=1024 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1 DIFF_MODE=PERCENT DIFF_TOLERANCE=0.01 CC_TOLERANCE=0.01"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=22 T_DATA_A=cfloat T_DATA_B=float API_IO=1 UUT_SSR=1 DIM_SIZE_A=4096 DIM_SIZE_B=8 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1 DIFF_MODE=PERCENT DIFF_TOLERANCE=0.01 CC_TOLERANCE=0.01"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=22 T_DATA_A=cfloat T_DATA_B=float API_IO=1 UUT_SSR=1 DIM_SIZE_A=4096 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1 DIFF_MODE=PERCENT DIFF_TOLERANCE=0.01 CC_TOLERANCE=0.01"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=22 T_DATA_A=cfloat T_DATA_B=float API_IO=1 UUT_SSR=1 DIM_SIZE_A=8192 DIM_SIZE_B=8 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1 DIFF_MODE=PERCENT DIFF_TOLERANCE=0.01 CC_TOLERANCE=0.01"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=22 T_DATA_A=cfloat T_DATA_B=float API_IO=1 UUT_SSR=1 DIM_SIZE_A=8192 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1 DIFF_MODE=PERCENT DIFF_TOLERANCE=0.01 CC_TOLERANCE=0.01"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=22 T_DATA_A=float T_DATA_B=float API_IO=0 UUT_SSR=1 DIM_SIZE_A=1024 DIM_SIZE_B=8 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1 DIFF_MODE=PERCENT DIFF_TOLERANCE=0.01 CC_TOLERANCE=0.01"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=22 T_DATA_A=float T_DATA_B=float API_IO=1 UUT_SSR=1 DIM_SIZE_A=1024 DIM_SIZE_B=8 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1 DIFF_MODE=PERCENT DIFF_TOLERANCE=0.01 CC_TOLERANCE=0.01"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=22 T_DATA_A=float T_DATA_B=float API_IO=1 UUT_SSR=1 DIM_SIZE_A=1024 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1 DIFF_MODE=PERCENT DIFF_TOLERANCE=0.01 CC_TOLERANCE=0.01"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=22 T_DATA_A=float T_DATA_B=float API_IO=1 UUT_SSR=1 DIM_SIZE_A=4096 DIM_SIZE_B=8 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1 DIFF_MODE=PERCENT DIFF_TOLERANCE=0.01 CC_TOLERANCE=0.01"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=22 T_DATA_A=float T_DATA_B=float API_IO=1 UUT_SSR=1 DIM_SIZE_A=4096 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1 DIFF_MODE=PERCENT DIFF_TOLERANCE=0.01 CC_TOLERANCE=0.01"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=22 T_DATA_A=float T_DATA_B=float API_IO=1 UUT_SSR=1 DIM_SIZE_A=8192 DIM_SIZE_B=8 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1 DIFF_MODE=PERCENT DIFF_TOLERANCE=0.01 CC_TOLERANCE=0.01"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=22 T_DATA_A=float T_DATA_B=float API_IO=1 UUT_SSR=1 DIM_SIZE_A=8192 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1 DIFF_MODE=PERCENT DIFF_TOLERANCE=0.01 CC_TOLERANCE=0.01"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=22 T_DATA_A=cint16 T_DATA_B=cint16 API_IO=0 UUT_SSR=1 DIM_SIZE_A=1024 DIM_SIZE_B=8 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=22 T_DATA_A=cint16 T_DATA_B=cint16 API_IO=1 UUT_SSR=1 DIM_SIZE_A=1024 DIM_SIZE_B=8 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=22 T_DATA_A=cint16 T_DATA_B=cint16 API_IO=1 UUT_SSR=1 DIM_SIZE_A=1024 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=22 T_DATA_A=cint16 T_DATA_B=cint16 API_IO=1 UUT_SSR=1 DIM_SIZE_A=4096 DIM_SIZE_B=8 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=22 T_DATA_A=cint16 T_DATA_B=cint16 API_IO=1 UUT_SSR=1 DIM_SIZE_A=4096 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=22 T_DATA_A=cint16 T_DATA_B=cint16 API_IO=1 UUT_SSR=1 DIM_SIZE_A=8192 DIM_SIZE_B=8 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=22 T_DATA_A=cint16 T_DATA_B=cint16 API_IO=1 UUT_SSR=1 DIM_SIZE_A=8192 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=22 T_DATA_A=cint16 T_DATA_B=cint32 API_IO=0 UUT_SSR=1 DIM_SIZE_A=1024 DIM_SIZE_B=8 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=22 T_DATA_A=cint16 T_DATA_B=cint32 API_IO=1 UUT_SSR=1 DIM_SIZE_A=1024 DIM_SIZE_B=8 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=22 T_DATA_A=cint16 T_DATA_B=cint32 API_IO=1 UUT_SSR=1 DIM_SIZE_A=1024 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=22 T_DATA_A=cint16 T_DATA_B=cint32 API_IO=1 UUT_SSR=1 DIM_SIZE_A=4096 DIM_SIZE_B=8 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=22 T_DATA_A=cint16 T_DATA_B=cint32 API_IO=1 UUT_SSR=1 DIM_SIZE_A=4096 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=22 T_DATA_A=cint16 T_DATA_B=cint32 API_IO=1 UUT_SSR=1 DIM_SIZE_A=8192 DIM_SIZE_B=8 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=22 T_DATA_A=cint16 T_DATA_B=cint32 API_IO=1 UUT_SSR=1 DIM_SIZE_A=8192 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=22 T_DATA_A=cint16 T_DATA_B=int16 API_IO=1 UUT_SSR=1 DIM_SIZE_A=1024 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=22 T_DATA_A=cint16 T_DATA_B=int16 API_IO=1 UUT_SSR=1 DIM_SIZE_A=4096 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=22 T_DATA_A=cint16 T_DATA_B=int16 API_IO=1 UUT_SSR=1 DIM_SIZE_A=8192 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=22 T_DATA_A=cint16 T_DATA_B=int32 API_IO=0 UUT_SSR=1 DIM_SIZE_A=1024 DIM_SIZE_B=8 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=22 T_DATA_A=cint16 T_DATA_B=int32 API_IO=1 UUT_SSR=1 DIM_SIZE_A=1024 DIM_SIZE_B=8 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=22 T_DATA_A=cint16 T_DATA_B=int32 API_IO=1 UUT_SSR=1 DIM_SIZE_A=1024 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=22 T_DATA_A=cint16 T_DATA_B=int32 API_IO=1 UUT_SSR=1 DIM_SIZE_A=4096 DIM_SIZE_B=8 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=22 T_DATA_A=cint16 T_DATA_B=int32 API_IO=1 UUT_SSR=1 DIM_SIZE_A=4096 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=22 T_DATA_A=cint16 T_DATA_B=int32 API_IO=1 UUT_SSR=1 DIM_SIZE_A=8192 DIM_SIZE_B=8 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=22 T_DATA_A=cint16 T_DATA_B=int32 API_IO=1 UUT_SSR=1 DIM_SIZE_A=8192 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=22 T_DATA_A=cint32 T_DATA_B=cint32 API_IO=0 UUT_SSR=1 DIM_SIZE_A=1024 DIM_SIZE_B=8 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=22 T_DATA_A=cint32 T_DATA_B=cint32 API_IO=1 UUT_SSR=1 DIM_SIZE_A=1024 DIM_SIZE_B=8 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=22 T_DATA_A=cint32 T_DATA_B=cint32 API_IO=1 UUT_SSR=1 DIM_SIZE_A=1024 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=22 T_DATA_A=cint32 T_DATA_B=cint32 API_IO=1 UUT_SSR=1 DIM_SIZE_A=4096 DIM_SIZE_B=8 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=22 T_DATA_A=cint32 T_DATA_B=cint32 API_IO=1 UUT_SSR=1 DIM_SIZE_A=4096 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=22 T_DATA_A=cint32 T_DATA_B=cint32 API_IO=1 UUT_SSR=1 DIM_SIZE_A=8192 DIM_SIZE_B=8 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=22 T_DATA_A=cint32 T_DATA_B=cint32 API_IO=1 UUT_SSR=1 DIM_SIZE_A=8192 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=22 T_DATA_A=cint32 T_DATA_B=int16 API_IO=1 UUT_SSR=1 DIM_SIZE_A=1024 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=22 T_DATA_A=cint32 T_DATA_B=int16 API_IO=1 UUT_SSR=1 DIM_SIZE_A=4096 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=22 T_DATA_A=cint32 T_DATA_B=int16 API_IO=1 UUT_SSR=1 DIM_SIZE_A=8192 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=22 T_DATA_A=cint32 T_DATA_B=int32 API_IO=0 UUT_SSR=1 DIM_SIZE_A=1024 DIM_SIZE_B=8 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=22 T_DATA_A=cint32 T_DATA_B=int32 API_IO=1 UUT_SSR=1 DIM_SIZE_A=1024 DIM_SIZE_B=8 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=22 T_DATA_A=cint32 T_DATA_B=int32 API_IO=1 UUT_SSR=1 DIM_SIZE_A=1024 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=22 T_DATA_A=cint32 T_DATA_B=int32 API_IO=1 UUT_SSR=1 DIM_SIZE_A=4096 DIM_SIZE_B=8 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=22 T_DATA_A=cint32 T_DATA_B=int32 API_IO=1 UUT_SSR=1 DIM_SIZE_A=4096 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=22 T_DATA_A=cint32 T_DATA_B=int32 API_IO=1 UUT_SSR=1 DIM_SIZE_A=8192 DIM_SIZE_B=8 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=22 T_DATA_A=cint32 T_DATA_B=int32 API_IO=1 UUT_SSR=1 DIM_SIZE_A=8192 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=22 T_DATA_A=int16 T_DATA_B=int16 API_IO=1 UUT_SSR=1 DIM_SIZE_A=1024 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=22 T_DATA_A=int16 T_DATA_B=int16 API_IO=1 UUT_SSR=1 DIM_SIZE_A=4096 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=22 T_DATA_A=int16 T_DATA_B=int16 API_IO=1 UUT_SSR=1 DIM_SIZE_A=8192 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=22 T_DATA_A=int16 T_DATA_B=int32 API_IO=0 UUT_SSR=1 DIM_SIZE_A=1024 DIM_SIZE_B=8 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=22 T_DATA_A=int16 T_DATA_B=int32 API_IO=1 UUT_SSR=1 DIM_SIZE_A=1024 DIM_SIZE_B=8 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=22 T_DATA_A=int16 T_DATA_B=int32 API_IO=1 UUT_SSR=1 DIM_SIZE_A=1024 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=22 T_DATA_A=int16 T_DATA_B=int32 API_IO=1 UUT_SSR=1 DIM_SIZE_A=4096 DIM_SIZE_B=8 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=22 T_DATA_A=int16 T_DATA_B=int32 API_IO=1 UUT_SSR=1 DIM_SIZE_A=4096 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=22 T_DATA_A=int16 T_DATA_B=int32 API_IO=1 UUT_SSR=1 DIM_SIZE_A=8192 DIM_SIZE_B=8 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=22 T_DATA_A=int16 T_DATA_B=int32 API_IO=1 UUT_SSR=1 DIM_SIZE_A=8192 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=22 T_DATA_A=int32 T_DATA_B=int32 API_IO=0 UUT_SSR=1 DIM_SIZE_A=1024 DIM_SIZE_B=8 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=22 T_DATA_A=int32 T_DATA_B=int32 API_IO=1 UUT_SSR=1 DIM_SIZE_A=1024 DIM_SIZE_B=8 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=22 T_DATA_A=int32 T_DATA_B=int32 API_IO=1 UUT_SSR=1 DIM_SIZE_A=1024 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=22 T_DATA_A=int32 T_DATA_B=int32 API_IO=1 UUT_SSR=1 DIM_SIZE_A=4096 DIM_SIZE_B=8 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=22 T_DATA_A=int32 T_DATA_B=int32 API_IO=1 UUT_SSR=1 DIM_SIZE_A=4096 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=22 T_DATA_A=int32 T_DATA_B=int32 API_IO=1 UUT_SSR=1 DIM_SIZE_A=8192 DIM_SIZE_B=8 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1"
    test_arr[$LINENO]="make all NITER=16 AIE_VARIANT=22 T_DATA_A=int32 T_DATA_B=int32 API_IO=1 UUT_SSR=1 DIM_SIZE_A=8192 DIM_SIZE_B=64 NUM_FRAMES=1 SHIFT=0 ROUND_MODE=0 SAT_MODE=0 SINGLE_BUF=1"

fi


# if [[ "$*" == *PR* ]]
# then
#     # TO BE POPULATED
# fi

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