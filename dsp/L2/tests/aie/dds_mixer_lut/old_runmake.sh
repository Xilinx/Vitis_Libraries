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
    test_arr[$LINENO]="make all AIE_VARIANT=1 "
    test_arr[$LINENO]="make all AIE_VARIANT=2 "
    test_arr[$LINENO]="make all AIE_VARIANT=22 "
fi


if [[ "$*" == *single_buf* || "$*" == *checkin* ||  "$*" == *qor* ]]
then
    test_arr[$LINENO]="make all AIE_VARIANT=1 DATA_TYPE=cint16 MIXER_MODE=0 P_API=0 UUT_SSR=1  SFDR=90 INPUT_WINDOW_VSIZE=4096 SINGLE_BUF=1"
    test_arr[$LINENO]="make all AIE_VARIANT=1 DATA_TYPE=cint16 MIXER_MODE=0 P_API=0 UUT_SSR=2  SFDR=90 INPUT_WINDOW_VSIZE=4096 SINGLE_BUF=1"
    test_arr[$LINENO]="make all AIE_VARIANT=1 DATA_TYPE=cint16 MIXER_MODE=0 P_API=0 UUT_SSR=4  SFDR=90 INPUT_WINDOW_VSIZE=4096 SINGLE_BUF=1"
    test_arr[$LINENO]="make all AIE_VARIANT=1 DATA_TYPE=cint16 MIXER_MODE=1 P_API=0 UUT_SSR=1  SFDR=90 INPUT_WINDOW_VSIZE=4096 SINGLE_BUF=1"
    test_arr[$LINENO]="make all AIE_VARIANT=1 DATA_TYPE=cint16 MIXER_MODE=1 P_API=0 UUT_SSR=2  SFDR=90 INPUT_WINDOW_VSIZE=4096 SINGLE_BUF=1"
    test_arr[$LINENO]="make all AIE_VARIANT=1 DATA_TYPE=cint16 MIXER_MODE=1 P_API=0 UUT_SSR=4  SFDR=90 INPUT_WINDOW_VSIZE=4096 SINGLE_BUF=1"
    test_arr[$LINENO]="make all AIE_VARIANT=1 DATA_TYPE=cint16 MIXER_MODE=2 P_API=0 UUT_SSR=1  SFDR=90 INPUT_WINDOW_VSIZE=4096 SINGLE_BUF=1"
    test_arr[$LINENO]="make all AIE_VARIANT=1 DATA_TYPE=cint16 MIXER_MODE=2 P_API=0 UUT_SSR=2  SFDR=90 INPUT_WINDOW_VSIZE=4096 SINGLE_BUF=1"
    test_arr[$LINENO]="make all AIE_VARIANT=1 DATA_TYPE=cint16 MIXER_MODE=2 P_API=0 UUT_SSR=4  SFDR=90 INPUT_WINDOW_VSIZE=4096 SINGLE_BUF=1"

fi



if [[ "$*" == *commslib* || "$*" == "" || "$*" == *checkin* ]]
then
    test_arr[${#test_arr[*]}]="echo 'Performing CommsLib Equivalent Functions...'"
    ## CommsLib Equivalent
    # mixer
    test_arr[$LINENO]="make all DATA_TYPE=cint16 MIXER_MODE=2 P_API=0 UUT_SSR=1 INPUT_WINDOW_VSIZE=320 "

fi

if [[  "$*" == "" || "$*" == *checkin*  ]]
then
    test_arr[$LINENO]="make all AIE_VARIANT=1 DATA_TYPE=cint16 MIXER_MODE=0 P_API=1 UUT_SSR=1  INPUT_WINDOW_VSIZE=4096"
    test_arr[$LINENO]="make all AIE_VARIANT=1 DATA_TYPE=cint16 MIXER_MODE=0 P_API=0 UUT_SSR=1  SFDR=90 INPUT_WINDOW_VSIZE=128"
    test_arr[$LINENO]="make all AIE_VARIANT=1 DATA_TYPE=cint32 MIXER_MODE=0 P_API=1 UUT_SSR=1  SFDR=100 INPUT_WINDOW_VSIZE=4096"
    test_arr[$LINENO]="make all AIE_VARIANT=1 DATA_TYPE=cint32 MIXER_MODE=0 P_API=0 UUT_SSR=1  INPUT_WINDOW_VSIZE=128"
    test_arr[$LINENO]="make all AIE_VARIANT=1 DATA_TYPE=cfloat MIXER_MODE=0 P_API=0 UUT_SSR=1  SFDR=50 INPUT_WINDOW_VSIZE=128"
    test_arr[$LINENO]="make all AIE_VARIANT=1 DATA_TYPE=cint16 MIXER_MODE=1 P_API=1 UUT_SSR=1  INPUT_WINDOW_VSIZE=4096"
    test_arr[$LINENO]="make all AIE_VARIANT=1 DATA_TYPE=cint16 MIXER_MODE=1 P_API=0 UUT_SSR=1  INPUT_WINDOW_VSIZE=128"
    test_arr[$LINENO]="make all AIE_VARIANT=1 DATA_TYPE=cint32 MIXER_MODE=1 P_API=1 UUT_SSR=1  INPUT_WINDOW_VSIZE=4096"
    test_arr[$LINENO]="make all AIE_VARIANT=1 DATA_TYPE=cint32 MIXER_MODE=1 P_API=0 UUT_SSR=1  INPUT_WINDOW_VSIZE=128"
    test_arr[$LINENO]="make all AIE_VARIANT=1 DATA_TYPE=cfloat MIXER_MODE=1 P_API=0 UUT_SSR=1  SFDR=100 INPUT_WINDOW_VSIZE=128"
    test_arr[$LINENO]="make all AIE_VARIANT=1 DATA_TYPE=cint16 MIXER_MODE=2 P_API=0 UUT_SSR=1  INPUT_WINDOW_VSIZE=128"
    test_arr[$LINENO]="make all AIE_VARIANT=1 DATA_TYPE=cint32 MIXER_MODE=2 P_API=0 UUT_SSR=1  SFDR=150 INPUT_WINDOW_VSIZE=128"
    test_arr[$LINENO]="make all AIE_VARIANT=1 DATA_TYPE=cfloat MIXER_MODE=2 P_API=0 UUT_SSR=1  SFDR=150 INPUT_WINDOW_VSIZE=128"

    test_arr[$LINENO]="make all AIE_VARIANT=1 DATA_TYPE=cint16 MIXER_MODE=0 P_API=1 UUT_SSR=2  SFDR=90 INPUT_WINDOW_VSIZE=4096"
    test_arr[$LINENO]="make all AIE_VARIANT=1 DATA_TYPE=cint16 MIXER_MODE=0 P_API=0 UUT_SSR=3  INPUT_WINDOW_VSIZE=384"
    test_arr[$LINENO]="make all AIE_VARIANT=1 DATA_TYPE=cint32 MIXER_MODE=0 P_API=1 UUT_SSR=4  SFDR=100 INPUT_WINDOW_VSIZE=4096"
    test_arr[$LINENO]="make all AIE_VARIANT=1 DATA_TYPE=cint32 MIXER_MODE=0 P_API=0 UUT_SSR=5  INPUT_WINDOW_VSIZE=640"
    test_arr[$LINENO]="make all AIE_VARIANT=1 DATA_TYPE=cfloat MIXER_MODE=0 P_API=0 UUT_SSR=5  SFDR=50 INPUT_WINDOW_VSIZE=640"
    test_arr[$LINENO]="make all AIE_VARIANT=1 DATA_TYPE=cint16 MIXER_MODE=1 P_API=1 UUT_SSR=3  INPUT_WINDOW_VSIZE=12288"
    test_arr[$LINENO]="make all AIE_VARIANT=1 DATA_TYPE=cint16 MIXER_MODE=1 P_API=0 UUT_SSR=4  INPUT_WINDOW_VSIZE=512"
    test_arr[$LINENO]="make all AIE_VARIANT=1 DATA_TYPE=cint32 MIXER_MODE=1 P_API=1 UUT_SSR=5  SFDR=150 INPUT_WINDOW_VSIZE=20480"
    test_arr[$LINENO]="make all AIE_VARIANT=1 DATA_TYPE=cint32 MIXER_MODE=1 P_API=0 UUT_SSR=3  INPUT_WINDOW_VSIZE=384"
    test_arr[$LINENO]="make all AIE_VARIANT=1 DATA_TYPE=cfloat MIXER_MODE=1 P_API=0 UUT_SSR=3  SFDR=100 INPUT_WINDOW_VSIZE=384 DDS_PHASE_INC=1807958314 DIFF_MODE=PERCENT DIFF_TOLERANCE=0.02"
    test_arr[$LINENO]="make all AIE_VARIANT=1 DATA_TYPE=cint16 MIXER_MODE=2 P_API=0 UUT_SSR=2  INPUT_WINDOW_VSIZE=128"
    test_arr[$LINENO]="make all AIE_VARIANT=1 DATA_TYPE=cint32 MIXER_MODE=2 P_API=0 UUT_SSR=6  INPUT_WINDOW_VSIZE=768"
    test_arr[$LINENO]="make all AIE_VARIANT=1 DATA_TYPE=cfloat MIXER_MODE=2 P_API=0 UUT_SSR=6  SFDR=150 INPUT_WINDOW_VSIZE=768 DDS_PHASE_INC=1797958314 DIFF_MODE=PERCENT"

    # # rounding and saturation modes
    test_arr[$LINENO]="make all AIE_VARIANT=1  DATA_TYPE=cint16 MIXER_MODE=2 P_API=0 UUT_SSR=1  INPUT_WINDOW_VSIZE=512 ROUND_MODE=7 SAT_MODE=0"
    test_arr[$LINENO]="make all AIE_VARIANT=2  DATA_TYPE=cint16 MIXER_MODE=2 P_API=0 UUT_SSR=1  INPUT_WINDOW_VSIZE=512 ROUND_MODE=11 SAT_MODE=1"
    test_arr[$LINENO]="make all AIE_VARIANT=22 DATA_TYPE=cint16 MIXER_MODE=2 P_API=0 UUT_SSR=1  INPUT_WINDOW_VSIZE=512 ROUND_MODE=13 SAT_MODE=3"


    #AIE2
    test_arr[$LINENO]="make all AIE_VARIANT=2 DATA_TYPE=cint16 MIXER_MODE=0 P_API=0 UUT_SSR=1  SFDR=90 INPUT_WINDOW_VSIZE=256"
    test_arr[$LINENO]="make all AIE_VARIANT=2 DATA_TYPE=cint32 MIXER_MODE=0 P_API=0 UUT_SSR=1  INPUT_WINDOW_VSIZE=256"
    test_arr[$LINENO]="make all AIE_VARIANT=2 DATA_TYPE=cint16 MIXER_MODE=1 P_API=0 UUT_SSR=1  INPUT_WINDOW_VSIZE=256"
    test_arr[$LINENO]="make all AIE_VARIANT=2 DATA_TYPE=cint32 MIXER_MODE=1 P_API=0 UUT_SSR=1  INPUT_WINDOW_VSIZE=256"
    test_arr[$LINENO]="make all AIE_VARIANT=2 DATA_TYPE=cint16 MIXER_MODE=2 P_API=0 UUT_SSR=1  INPUT_WINDOW_VSIZE=256"
    test_arr[$LINENO]="make all AIE_VARIANT=2 DATA_TYPE=cint32 MIXER_MODE=2 P_API=0 UUT_SSR=1  SFDR=150 INPUT_WINDOW_VSIZE=256"

    test_arr[$LINENO]="make all AIE_VARIANT=2 DATA_TYPE=cint16 MIXER_MODE=0 P_API=0 UUT_SSR=3  INPUT_WINDOW_VSIZE=768"
    test_arr[$LINENO]="make all AIE_VARIANT=2 DATA_TYPE=cint32 MIXER_MODE=0 P_API=0 UUT_SSR=5  INPUT_WINDOW_VSIZE=1280"
    test_arr[$LINENO]="make all AIE_VARIANT=2 DATA_TYPE=cint16 MIXER_MODE=1 P_API=0 UUT_SSR=4  INPUT_WINDOW_VSIZE=1024"
    test_arr[$LINENO]="make all AIE_VARIANT=2 DATA_TYPE=cint32 MIXER_MODE=1 P_API=0 UUT_SSR=3  INPUT_WINDOW_VSIZE=768"
    test_arr[$LINENO]="make all AIE_VARIANT=2 DATA_TYPE=cint16 MIXER_MODE=2 P_API=0 UUT_SSR=2  INPUT_WINDOW_VSIZE=512"
    test_arr[$LINENO]="make all AIE_VARIANT=2 DATA_TYPE=cint32 MIXER_MODE=2 P_API=0 UUT_SSR=6  INPUT_WINDOW_VSIZE=1536"

    #known failures - wait for CR to be fixed
    test_arr[$LINENO]="make all AIE_VARIANT=2 DATA_TYPE=cint16 MIXER_MODE=0 P_API=1 UUT_SSR=1  INPUT_WINDOW_VSIZE=256"
    test_arr[$LINENO]="make all AIE_VARIANT=2 DATA_TYPE=cint32 MIXER_MODE=0 P_API=1 UUT_SSR=1  SFDR=100 INPUT_WINDOW_VSIZE=256"
    test_arr[$LINENO]="make all AIE_VARIANT=2 DATA_TYPE=cint16 MIXER_MODE=1 P_API=1 UUT_SSR=1  INPUT_WINDOW_VSIZE=256"
    test_arr[$LINENO]="make all AIE_VARIANT=2 DATA_TYPE=cint32 MIXER_MODE=1 P_API=1 UUT_SSR=1  INPUT_WINDOW_VSIZE=256"
    test_arr[$LINENO]="make all AIE_VARIANT=2 DATA_TYPE=cint16 MIXER_MODE=0 P_API=1 UUT_SSR=2  SFDR=90 INPUT_WINDOW_VSIZE=512"
    test_arr[$LINENO]="make all AIE_VARIANT=2 DATA_TYPE=cint32 MIXER_MODE=0 P_API=1 UUT_SSR=4  SFDR=100 INPUT_WINDOW_VSIZE=1024"
    test_arr[$LINENO]="make all AIE_VARIANT=2 DATA_TYPE=cint16 MIXER_MODE=1 P_API=1 UUT_SSR=3  INPUT_WINDOW_VSIZE=768"
    test_arr[$LINENO]="make all AIE_VARIANT=2 DATA_TYPE=cint32 MIXER_MODE=1 P_API=1 UUT_SSR=5  SFDR=150 INPUT_WINDOW_VSIZE=5120"

    #AIE-MLv2
    test_arr[$LINENO]="make all AIE_VARIANT=22 DATA_TYPE=cint16 MIXER_MODE=0 P_API=0 UUT_SSR=1  SFDR=90 INPUT_WINDOW_VSIZE=256"
    test_arr[$LINENO]="make all AIE_VARIANT=22 DATA_TYPE=cint32 MIXER_MODE=0 P_API=0 UUT_SSR=1  INPUT_WINDOW_VSIZE=256"
    test_arr[$LINENO]="make all AIE_VARIANT=22 DATA_TYPE=cint16 MIXER_MODE=1 P_API=0 UUT_SSR=1  INPUT_WINDOW_VSIZE=256"
    test_arr[$LINENO]="make all AIE_VARIANT=22 DATA_TYPE=cint32 MIXER_MODE=1 P_API=0 UUT_SSR=1  INPUT_WINDOW_VSIZE=256"
    test_arr[$LINENO]="make all AIE_VARIANT=22 DATA_TYPE=cint16 MIXER_MODE=2 P_API=0 UUT_SSR=1  INPUT_WINDOW_VSIZE=256"
    test_arr[$LINENO]="make all AIE_VARIANT=22 DATA_TYPE=cint32 MIXER_MODE=2 P_API=0 UUT_SSR=1  SFDR=150 INPUT_WINDOW_VSIZE=256"

    test_arr[$LINENO]="make all AIE_VARIANT=22 DATA_TYPE=cint16 MIXER_MODE=0 P_API=0 UUT_SSR=3  INPUT_WINDOW_VSIZE=768"
    test_arr[$LINENO]="make all AIE_VARIANT=22 DATA_TYPE=cint32 MIXER_MODE=0 P_API=0 UUT_SSR=5  INPUT_WINDOW_VSIZE=1280"
    test_arr[$LINENO]="make all AIE_VARIANT=22 DATA_TYPE=cint16 MIXER_MODE=1 P_API=0 UUT_SSR=4  INPUT_WINDOW_VSIZE=1024"
    test_arr[$LINENO]="make all AIE_VARIANT=22 DATA_TYPE=cint32 MIXER_MODE=1 P_API=0 UUT_SSR=3  INPUT_WINDOW_VSIZE=768"
    test_arr[$LINENO]="make all AIE_VARIANT=22 DATA_TYPE=cint16 MIXER_MODE=2 P_API=0 UUT_SSR=2  INPUT_WINDOW_VSIZE=512"
    test_arr[$LINENO]="make all AIE_VARIANT=22 DATA_TYPE=cint32 MIXER_MODE=2 P_API=0 UUT_SSR=6  INPUT_WINDOW_VSIZE=1536"

    #known failures - wait for CR to be fixed
    test_arr[$LINENO]="make all AIE_VARIANT=22 DATA_TYPE=cint16 MIXER_MODE=0 P_API=1 UUT_SSR=1  INPUT_WINDOW_VSIZE=256"
    test_arr[$LINENO]="make all AIE_VARIANT=22 DATA_TYPE=cint32 MIXER_MODE=0 P_API=1 UUT_SSR=1  SFDR=100 INPUT_WINDOW_VSIZE=256"
    test_arr[$LINENO]="make all AIE_VARIANT=22 DATA_TYPE=cint16 MIXER_MODE=1 P_API=1 UUT_SSR=1  INPUT_WINDOW_VSIZE=256"
    test_arr[$LINENO]="make all AIE_VARIANT=22 DATA_TYPE=cint32 MIXER_MODE=1 P_API=1 UUT_SSR=1  INPUT_WINDOW_VSIZE=256"
    test_arr[$LINENO]="make all AIE_VARIANT=22 DATA_TYPE=cint16 MIXER_MODE=0 P_API=1 UUT_SSR=2  SFDR=90 INPUT_WINDOW_VSIZE=512"
    test_arr[$LINENO]="make all AIE_VARIANT=22 DATA_TYPE=cint32 MIXER_MODE=0 P_API=1 UUT_SSR=4  SFDR=100 INPUT_WINDOW_VSIZE=1024"
    test_arr[$LINENO]="make all AIE_VARIANT=22 DATA_TYPE=cint16 MIXER_MODE=1 P_API=1 UUT_SSR=3  INPUT_WINDOW_VSIZE=768"
    test_arr[$LINENO]="make all AIE_VARIANT=22 DATA_TYPE=cint32 MIXER_MODE=1 P_API=1 UUT_SSR=5  SFDR=150 INPUT_WINDOW_VSIZE=5120"

    #Phase Offset RTP Tests
    test_arr[$LINENO]="make all AIE_VARIANT=1 DATA_TYPE=cfloat MIXER_MODE=0 P_API=0 UUT_SSR=1  SFDR=50 INPUT_WINDOW_VSIZE=640    USE_PHASE_RELOAD=1"
    test_arr[$LINENO]="make all AIE_VARIANT=1 DATA_TYPE=cint16 MIXER_MODE=1 P_API=1 UUT_SSR=1  INPUT_WINDOW_VSIZE=12288          USE_PHASE_RELOAD=1"

    test_arr[$LINENO]="make all AIE_VARIANT=1 DATA_TYPE=cint16 MIXER_MODE=2 P_API=1 UUT_SSR=1  INPUT_WINDOW_VSIZE=512 ROUND_MODE=3 SAT_MODE=0 USE_PHASE_RELOAD=1"

    test_arr[$LINENO]="make all AIE_VARIANT=2 DATA_TYPE=cint32 MIXER_MODE=0 P_API=0 UUT_SSR=1  INPUT_WINDOW_VSIZE=1280 USE_PHASE_RELOAD=1"
    test_arr[$LINENO]="make all AIE_VARIANT=2 DATA_TYPE=cint16 MIXER_MODE=1 P_API=0 UUT_SSR=1  INPUT_WINDOW_VSIZE=1024 USE_PHASE_RELOAD=1"
    test_arr[$LINENO]="make all AIE_VARIANT=2 DATA_TYPE=cint32 MIXER_MODE=1 P_API=0 UUT_SSR=1  INPUT_WINDOW_VSIZE=768  USE_PHASE_RELOAD=1"

    #phase offset iobuffer
    test_arr[$LINENO]="make all AIE_VARIANT=1 TARGET=x86sim DATA_TYPE=cint16 MIXER_MODE=2 P_API=1 UUT_SSR=1  INPUT_WINDOW_VSIZE=256 USE_PHASE_RELOAD=1 PHASE_RELOAD_API=1 "
    test_arr[$LINENO]="make all AIE_VARIANT=1 TARGET=hw     DATA_TYPE=cint16 MIXER_MODE=1 P_API=1 UUT_SSR=1  INPUT_WINDOW_VSIZE=64  USE_PHASE_RELOAD=1 PHASE_RELOAD_API=1 "
    test_arr[$LINENO]="make all AIE_VARIANT=2 TARGET=x86sim DATA_TYPE=cint32 MIXER_MODE=1 P_API=0 UUT_SSR=1  INPUT_WINDOW_VSIZE=128 USE_PHASE_RELOAD=1 PHASE_RELOAD_API=1 "
    test_arr[$LINENO]="make all AIE_VARIANT=2 TARGET=x86sim DATA_TYPE=cint16 MIXER_MODE=0 P_API=1 UUT_SSR=1  INPUT_WINDOW_VSIZE=64  USE_PHASE_RELOAD=1 PHASE_RELOAD_API=1 "


    test_arr[$LINENO]="make all AIE_VARIANT=22 DATA_TYPE=cint32 MIXER_MODE=0 P_API=0 UUT_SSR=1  INPUT_WINDOW_VSIZE=1280 USE_PHASE_RELOAD=1"
    test_arr[$LINENO]="make all AIE_VARIANT=22 DATA_TYPE=cint16 MIXER_MODE=1 P_API=0 UUT_SSR=1  INPUT_WINDOW_VSIZE=1024 USE_PHASE_RELOAD=1"

    #phase inc RTP
    test_arr[$LINENO]="make all AIE_VARIANT=2 TARGET=hw     DATA_TYPE=cint16 MIXER_MODE=2 P_API=0 UUT_SSR=1  INPUT_WINDOW_VSIZE=64  USE_PHASE_RELOAD=1 PHASE_RELOAD_API=1 USE_PHASE_INC_RELOAD=1 "
    test_arr[$LINENO]="make all AIE_VARIANT=2 TARGET=hw     DATA_TYPE=cint16 MIXER_MODE=2 P_API=0 UUT_SSR=1  INPUT_WINDOW_VSIZE=64  USE_PHASE_RELOAD=1 PHASE_RELOAD_API=1 USE_PHASE_INC_RELOAD=1 "
    test_arr[$LINENO]="make all AIE_VARIANT=2 TARGET=hw     DATA_TYPE=cint16 MIXER_MODE=0 P_API=0 UUT_SSR=1  INPUT_WINDOW_VSIZE=64  USE_PHASE_RELOAD=1 PHASE_RELOAD_API=0 USE_PHASE_INC_RELOAD=1 "
    test_arr[$LINENO]="make all AIE_VARIANT=2 TARGET=hw     DATA_TYPE=cint16 MIXER_MODE=1 P_API=0 UUT_SSR=1  INPUT_WINDOW_VSIZE=64  USE_PHASE_RELOAD=1 PHASE_RELOAD_API=0 USE_PHASE_INC_RELOAD=1 DIFF_MODE=ABS DIFF_TOLERANCE=3"
fi

if [[  "$*" == "" || "$*" == *var2*  ]]
then
    #AIE2
    test_arr[$LINENO]="make all AIE_VARIANT=2 DATA_TYPE=cint16 MIXER_MODE=0 P_API=1 UUT_SSR=1 RESULTS_PATH=./results/qor/  INPUT_WINDOW_VSIZE=4096"
    test_arr[$LINENO]="make all AIE_VARIANT=2 DATA_TYPE=cint16 MIXER_MODE=0 P_API=0 UUT_SSR=1 RESULTS_PATH=./results/qor/  INPUT_WINDOW_VSIZE=128"
    test_arr[$LINENO]="make all AIE_VARIANT=2 DATA_TYPE=cint16 MIXER_MODE=1 P_API=1 UUT_SSR=1 RESULTS_PATH=./results/qor/  INPUT_WINDOW_VSIZE=4096"
    test_arr[$LINENO]="make all AIE_VARIANT=2 DATA_TYPE=cint16 MIXER_MODE=1 P_API=0 UUT_SSR=1 RESULTS_PATH=./results/qor/  INPUT_WINDOW_VSIZE=128"
    test_arr[$LINENO]="make all AIE_VARIANT=2 DATA_TYPE=cint32 MIXER_MODE=1 P_API=1 UUT_SSR=1 RESULTS_PATH=./results/qor/  INPUT_WINDOW_VSIZE=4096"
    test_arr[$LINENO]="make all AIE_VARIANT=2 DATA_TYPE=cint32 MIXER_MODE=1 P_API=0 UUT_SSR=1 RESULTS_PATH=./results/qor/  INPUT_WINDOW_VSIZE=128"
    test_arr[$LINENO]="make all AIE_VARIANT=2 DATA_TYPE=cint16 MIXER_MODE=2 P_API=0 UUT_SSR=1 RESULTS_PATH=./results/qor/  INPUT_WINDOW_VSIZE=128"
    test_arr[$LINENO]="make all AIE_VARIANT=2 DATA_TYPE=cint32 MIXER_MODE=2 P_API=0 UUT_SSR=1 RESULTS_PATH=./results/qor/  INPUT_WINDOW_VSIZE=128"
fi

#Qor tests should be added to fit a suite with a collective purpose. There may be multiple suites with different purposes, e.g.
#Parameter exploration - where a base case is augmented by additional cases which vary one parameter such that the user
#  can see how QoR varies with a change in that parameter.
#Bellwether - standalone cases which describe a known or typical customer usecase, e.g. 47tap HB FIR with cint16/int16
#Anomaly. Where there is a known anomaly in QoR, such as a step change at a particular FIR length, cases may be added
# to highlight the non-linearity.
#Other. If there are other cases or other suites added for a different purpose, please add a description of the suite here.
if [[  "$*" == *qoraie2*  ]]
then
    #AIE2
    test_arr[$LINENO]="make all AIE_VARIANT=2 MIXER_MODE=1 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DATA_TYPE=cint32 DDS_PHASE_INC=0xC6588855 SFDR=150 RESULTS_PATH=./results/aie2-reworked/"
    test_arr[$LINENO]="make all AIE_VARIANT=2 MIXER_MODE=1 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DATA_TYPE=cint32 DDS_PHASE_INC=0xD6555555 SFDR=100 RESULTS_PATH=./results/aie2-reworked/"
    test_arr[$LINENO]="make all AIE_VARIANT=2 MIXER_MODE=1 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DATA_TYPE=cint32 DDS_PHASE_INC=0xC6588855 SFDR=60 RESULTS_PATH=./results/aie2-reworked/"
    test_arr[$LINENO]="make all AIE_VARIANT=2 MIXER_MODE=1 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DATA_TYPE=cint16 DDS_PHASE_INC=0xC6588855 SFDR=90 RESULTS_PATH=./results/aie2-reworked/"
    test_arr[$LINENO]="make all AIE_VARIANT=2 MIXER_MODE=1 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DATA_TYPE=cint16 DDS_PHASE_INC=0xC6588855 SFDR=60 RESULTS_PATH=./results/aie2-reworked/"

    test_arr[$LINENO]="make all AIE_VARIANT=2 MIXER_MODE=1 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=1024 DATA_TYPE=cint32 DDS_PHASE_INC=0xC6588855 UUT_SSR=4 SFDR=150 RESULTS_PATH=./results/aie2-reworked/"
    test_arr[$LINENO]="make all AIE_VARIANT=2 MIXER_MODE=1 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=1280 DATA_TYPE=cint32 DDS_PHASE_INC=0xD6555555 UUT_SSR=5 SFDR=100 RESULTS_PATH=./results/aie2-reworked/"
    test_arr[$LINENO]="make all AIE_VARIANT=2 MIXER_MODE=1 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=1536 DATA_TYPE=cint32 DDS_PHASE_INC=0xC6588855 UUT_SSR=6 SFDR=60 RESULTS_PATH=./results/aie2-reworked/"
    test_arr[$LINENO]="make all AIE_VARIANT=2 MIXER_MODE=1 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=1792 DATA_TYPE=cint16 DDS_PHASE_INC=0xC6588855 UUT_SSR=7 SFDR=90 RESULTS_PATH=./results/aie2-reworked/"
    test_arr[$LINENO]="make all AIE_VARIANT=2 MIXER_MODE=1 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=1280 DATA_TYPE=cint16 DDS_PHASE_INC=0xC6588855 UUT_SSR=5 SFDR=60 RESULTS_PATH=./results/aie2-reworked/"

    test_arr[$LINENO]="make all AIE_VARIANT=2 MIXER_MODE=0 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DATA_TYPE=cint32 DDS_PHASE_INC=0xC6588855 SFDR=150 RESULTS_PATH=./results/aie2-reworked/"
    test_arr[$LINENO]="make all AIE_VARIANT=2 MIXER_MODE=0 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DATA_TYPE=cint32 DDS_PHASE_INC=0xD6555555 SFDR=100 RESULTS_PATH=./results/aie2-reworked/"
    test_arr[$LINENO]="make all AIE_VARIANT=2 MIXER_MODE=0 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DATA_TYPE=cint32 DDS_PHASE_INC=0xC6588855 SFDR=60 RESULTS_PATH=./results/aie2-reworked/"
    test_arr[$LINENO]="make all AIE_VARIANT=2 MIXER_MODE=0 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DATA_TYPE=cint16 DDS_PHASE_INC=0xC6588855 SFDR=90 RESULTS_PATH=./results/aie2-reworked/"
    test_arr[$LINENO]="make all AIE_VARIANT=2 MIXER_MODE=0 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DATA_TYPE=cint16 DDS_PHASE_INC=0xC6588855 SFDR=60 RESULTS_PATH=./results/aie2-reworked/"

    test_arr[$LINENO]="make all AIE_VARIANT=2 MIXER_MODE=0 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=1024 DATA_TYPE=cint32 DDS_PHASE_INC=0xC6588855 UUT_SSR=4 SFDR=150 RESULTS_PATH=./results/aie2-reworked/"
    test_arr[$LINENO]="make all AIE_VARIANT=2 MIXER_MODE=0 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=1280 DATA_TYPE=cint32 DDS_PHASE_INC=0xD6555555 UUT_SSR=5 SFDR=100 RESULTS_PATH=./results/aie2-reworked/"
    test_arr[$LINENO]="make all AIE_VARIANT=2 MIXER_MODE=0 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=1536 DATA_TYPE=cint32 DDS_PHASE_INC=0xC6588855 UUT_SSR=6 SFDR=60 RESULTS_PATH=./results/aie2-reworked/"
    test_arr[$LINENO]="make all AIE_VARIANT=2 MIXER_MODE=0 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=1792 DATA_TYPE=cint16 DDS_PHASE_INC=0xC6588855 UUT_SSR=7 SFDR=90 RESULTS_PATH=./results/aie2-reworked/"
    test_arr[$LINENO]="make all AIE_VARIANT=2 MIXER_MODE=0 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=1280 DATA_TYPE=cint16 DDS_PHASE_INC=0xC6588855 UUT_SSR=5 SFDR=60 RESULTS_PATH=./results/aie2-reworked/"

    test_arr[$LINENO]="make all AIE_VARIANT=2 MIXER_MODE=2 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DATA_TYPE=cint32 DDS_PHASE_INC=0xC6588855 SFDR=150 RESULTS_PATH=./results/aie2-reworked/"
    test_arr[$LINENO]="make all AIE_VARIANT=2 MIXER_MODE=2 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DATA_TYPE=cint32 DDS_PHASE_INC=0xD6555555 SFDR=100 RESULTS_PATH=./results/aie2-reworked/"
    test_arr[$LINENO]="make all AIE_VARIANT=2 MIXER_MODE=2 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DATA_TYPE=cint32 DDS_PHASE_INC=0xC6588855 SFDR=60 RESULTS_PATH=./results/aie2-reworked/"
    test_arr[$LINENO]="make all AIE_VARIANT=2 MIXER_MODE=2 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DATA_TYPE=cint16 DDS_PHASE_INC=0xC6588855 SFDR=90 RESULTS_PATH=./results/aie2-reworked/"
    test_arr[$LINENO]="make all AIE_VARIANT=2 MIXER_MODE=2 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DATA_TYPE=cint16 DDS_PHASE_INC=0xC6588855 SFDR=60 RESULTS_PATH=./results/aie2-reworked/"

    test_arr[$LINENO]="make all AIE_VARIANT=2 MIXER_MODE=2 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=768 DATA_TYPE=cint32 DDS_PHASE_INC=0xC6588855 UUT_SSR=3 SFDR=150 RESULTS_PATH=./results/aie2-reworked/"
    test_arr[$LINENO]="make all AIE_VARIANT=2 MIXER_MODE=2 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=1024 DATA_TYPE=cint32 DDS_PHASE_INC=0xD6555555 UUT_SSR=4 SFDR=100 RESULTS_PATH=./results/aie2-reworked/"
    test_arr[$LINENO]="make all AIE_VARIANT=2 MIXER_MODE=2 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=1280 DATA_TYPE=cint32 DDS_PHASE_INC=0xC6588855 UUT_SSR=5 SFDR=60 RESULTS_PATH=./results/aie2-reworked/"
    test_arr[$LINENO]="make all AIE_VARIANT=2 MIXER_MODE=2 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=1536 DATA_TYPE=cint16 DDS_PHASE_INC=0xC6588855 UUT_SSR=6 SFDR=90 RESULTS_PATH=./results/aie2-reworked/"
    test_arr[$LINENO]="make all AIE_VARIANT=2 MIXER_MODE=2 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=1024 DATA_TYPE=cint16 DDS_PHASE_INC=0xC6588855 UUT_SSR=4 SFDR=60 RESULTS_PATH=./results/aie2-reworked/"

    # stream api
    test_arr[$LINENO]="make all P_API=1 AIE_VARIANT=2 MIXER_MODE=1 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DATA_TYPE=cint32 DDS_PHASE_INC=0xC6588855 SFDR=150 RESULTS_PATH=./results/aie2-reworked/"
    test_arr[$LINENO]="make all P_API=1 AIE_VARIANT=2 MIXER_MODE=1 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DATA_TYPE=cint32 DDS_PHASE_INC=0xD6555555 SFDR=100 RESULTS_PATH=./results/aie2-reworked/"
    test_arr[$LINENO]="make all P_API=1 AIE_VARIANT=2 MIXER_MODE=1 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DATA_TYPE=cint32 DDS_PHASE_INC=0xC6588855 SFDR=60 RESULTS_PATH=./results/aie2-reworked/"
    test_arr[$LINENO]="make all P_API=1 AIE_VARIANT=2 MIXER_MODE=1 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DATA_TYPE=cint16 DDS_PHASE_INC=0xC6588855 SFDR=90 RESULTS_PATH=./results/aie2-reworked/"
    test_arr[$LINENO]="make all P_API=1 AIE_VARIANT=2 MIXER_MODE=1 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DATA_TYPE=cint16 DDS_PHASE_INC=0xC6588855 SFDR=60 RESULTS_PATH=./results/aie2-reworked/"

    test_arr[$LINENO]="make all P_API=1 AIE_VARIANT=2 MIXER_MODE=1 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=1024 DATA_TYPE=cint32 DDS_PHASE_INC=0xC6588855 UUT_SSR=4 SFDR=150 RESULTS_PATH=./results/aie2-reworked/"
    test_arr[$LINENO]="make all P_API=1 AIE_VARIANT=2 MIXER_MODE=1 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=1280 DATA_TYPE=cint32 DDS_PHASE_INC=0xD6555555 UUT_SSR=5 SFDR=100 RESULTS_PATH=./results/aie2-reworked/"
    test_arr[$LINENO]="make all P_API=1 AIE_VARIANT=2 MIXER_MODE=1 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=1536 DATA_TYPE=cint32 DDS_PHASE_INC=0xC6588855 UUT_SSR=6 SFDR=60 RESULTS_PATH=./results/aie2-reworked/"
    test_arr[$LINENO]="make all P_API=1 AIE_VARIANT=2 MIXER_MODE=1 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=1792 DATA_TYPE=cint16 DDS_PHASE_INC=0xC6588855 UUT_SSR=7 SFDR=90 RESULTS_PATH=./results/aie2-reworked/"
    test_arr[$LINENO]="make all P_API=1 AIE_VARIANT=2 MIXER_MODE=1 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=1280 DATA_TYPE=cint16 DDS_PHASE_INC=0xC6588855 UUT_SSR=5 SFDR=60 RESULTS_PATH=./results/aie2-reworked/"

    test_arr[$LINENO]="make all P_API=1 AIE_VARIANT=2 MIXER_MODE=0 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DATA_TYPE=cint32 DDS_PHASE_INC=0xC6588855 SFDR=150 RESULTS_PATH=./results/aie2-reworked/"
    test_arr[$LINENO]="make all P_API=1 AIE_VARIANT=2 MIXER_MODE=0 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DATA_TYPE=cint32 DDS_PHASE_INC=0xD6555555 SFDR=100 RESULTS_PATH=./results/aie2-reworked/"
    test_arr[$LINENO]="make all P_API=1 AIE_VARIANT=2 MIXER_MODE=0 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DATA_TYPE=cint32 DDS_PHASE_INC=0xC6588855 SFDR=60 RESULTS_PATH=./results/aie2-reworked/"
    test_arr[$LINENO]="make all P_API=1 AIE_VARIANT=2 MIXER_MODE=0 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DATA_TYPE=cint16 DDS_PHASE_INC=0xC6588855 SFDR=90 RESULTS_PATH=./results/aie2-reworked/"
    test_arr[$LINENO]="make all P_API=1 AIE_VARIANT=2 MIXER_MODE=0 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DATA_TYPE=cint16 DDS_PHASE_INC=0xC6588855 SFDR=60 RESULTS_PATH=./results/aie2-reworked/"

    test_arr[$LINENO]="make all P_API=1 AIE_VARIANT=2 MIXER_MODE=0 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=1024 DATA_TYPE=cint32 DDS_PHASE_INC=0xC6588855 UUT_SSR=4 SFDR=150 RESULTS_PATH=./results/aie2-reworked/"
    test_arr[$LINENO]="make all P_API=1 AIE_VARIANT=2 MIXER_MODE=0 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=1280 DATA_TYPE=cint32 DDS_PHASE_INC=0xD6555555 UUT_SSR=5 SFDR=100 RESULTS_PATH=./results/aie2-reworked/"
    test_arr[$LINENO]="make all P_API=1 AIE_VARIANT=2 MIXER_MODE=0 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=1536 DATA_TYPE=cint32 DDS_PHASE_INC=0xC6588855 UUT_SSR=6 SFDR=60 RESULTS_PATH=./results/aie2-reworked/"
    test_arr[$LINENO]="make all P_API=1 AIE_VARIANT=2 MIXER_MODE=0 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=1792 DATA_TYPE=cint16 DDS_PHASE_INC=0xC6588855 UUT_SSR=7 SFDR=90 RESULTS_PATH=./results/aie2-reworked/"
    test_arr[$LINENO]="make all P_API=1 AIE_VARIANT=2 MIXER_MODE=0 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=1280 DATA_TYPE=cint16 DDS_PHASE_INC=0xC6588855 UUT_SSR=5 SFDR=60 RESULTS_PATH=./results/aie2-reworked/"

    #phase offset iobuffer
    test_arr[$LINENO]="make all AIE_VARIANT=2 DATA_TYPE=cint16 MIXER_MODE=2 P_API=1 UUT_SSR=1  INPUT_WINDOW_VSIZE=256 USE_PHASE_RELOAD=1 PHASE_RELOAD_API=1 "
    test_arr[$LINENO]="make all AIE_VARIANT=2 DATA_TYPE=cint16 MIXER_MODE=1 P_API=1 UUT_SSR=1  INPUT_WINDOW_VSIZE=64  USE_PHASE_RELOAD=1 PHASE_RELOAD_API=1 "
    test_arr[$LINENO]="make all AIE_VARIANT=2 DATA_TYPE=cint32 MIXER_MODE=1 P_API=0 UUT_SSR=1  INPUT_WINDOW_VSIZE=128 USE_PHASE_RELOAD=1 PHASE_RELOAD_API=1 "
    test_arr[$LINENO]="make all AIE_VARIANT=2 DATA_TYPE=cint16 MIXER_MODE=0 P_API=1 UUT_SSR=1  INPUT_WINDOW_VSIZE=64  USE_PHASE_RELOAD=1 PHASE_RELOAD_API=1 "

    #phase inc RTP
    test_arr[$LINENO]="make all AIE_VARIANT=2 TARGET=hw     DATA_TYPE=cint16 MIXER_MODE=2 P_API=0 UUT_SSR=1  INPUT_WINDOW_VSIZE=64  USE_PHASE_RELOAD=1 PHASE_RELOAD_API=1 USE_PHASE_INC_RELOAD=1 "
    test_arr[$LINENO]="make all AIE_VARIANT=2 TARGET=hw     DATA_TYPE=cint16 MIXER_MODE=2 P_API=0 UUT_SSR=1  INPUT_WINDOW_VSIZE=64  USE_PHASE_RELOAD=1 PHASE_RELOAD_API=1 USE_PHASE_INC_RELOAD=1 "
    test_arr[$LINENO]="make all AIE_VARIANT=2 TARGET=hw     DATA_TYPE=cint16 MIXER_MODE=0 P_API=0 UUT_SSR=1  INPUT_WINDOW_VSIZE=64  USE_PHASE_RELOAD=1 PHASE_RELOAD_API=0 USE_PHASE_INC_RELOAD=1 "
    test_arr[$LINENO]="make all AIE_VARIANT=2 TARGET=hw     DATA_TYPE=cint16 MIXER_MODE=1 P_API=0 UUT_SSR=1  INPUT_WINDOW_VSIZE=64  USE_PHASE_RELOAD=1 PHASE_RELOAD_API=0 USE_PHASE_INC_RELOAD=1 DIFF_MODE=ABS DIFF_TOLERANCE=3"

fi

#Qor tests should be added to fit a suite with a collective purpose. There may be multiple suites with different purposes, e.g.
#Parameter exploration - where a base case is augmented by additional cases which vary one parameter such that the user
#  can see how QoR varies with a change in that parameter.
#Bellwether - standalone cases which describe a known or typical customer usecase, e.g. 47tap HB FIR with cint16/int16
#Anomaly. Where there is a known anomaly in QoR, such as a step change at a particular FIR length, cases may be added
# to highlight the non-linearity.
#Other. If there are other cases or other suites added for a different purpose, please add a description of the suite here.
if [[  "$*" == *qoraie1int*  ]]
then
    #AIE1
    test_arr[$LINENO]="make all AIE_VARIANT=1 MIXER_MODE=1 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DATA_TYPE=cint32 DDS_PHASE_INC=0xC6588855 SFDR=150 RESULTS_PATH=./results/aie1-reworked/"
    test_arr[$LINENO]="make all AIE_VARIANT=1 MIXER_MODE=1 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DATA_TYPE=cint32 DDS_PHASE_INC=0xD6555555 SFDR=100 RESULTS_PATH=./results/aie1-reworked/"
    test_arr[$LINENO]="make all AIE_VARIANT=1 MIXER_MODE=1 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DATA_TYPE=cint32 DDS_PHASE_INC=0xC6588855 SFDR=60 RESULTS_PATH=./results/aie1-reworked/"
    test_arr[$LINENO]="make all AIE_VARIANT=1 MIXER_MODE=1 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DATA_TYPE=cint16 DDS_PHASE_INC=0xC6588855 SFDR=90 RESULTS_PATH=./results/aie1-reworked/"
    test_arr[$LINENO]="make all AIE_VARIANT=1 MIXER_MODE=1 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DATA_TYPE=cint16 DDS_PHASE_INC=0xC6588855 SFDR=60 RESULTS_PATH=./results/aie1-reworked/"

    test_arr[$LINENO]="make all AIE_VARIANT=1 MIXER_MODE=1 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=1024 DATA_TYPE=cint32 DDS_PHASE_INC=0xC6588855 UUT_SSR=4 SFDR=150 RESULTS_PATH=./results/aie1-reworked/"
    test_arr[$LINENO]="make all AIE_VARIANT=1 MIXER_MODE=1 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=1280 DATA_TYPE=cint32 DDS_PHASE_INC=0xD6555555 UUT_SSR=5 SFDR=100 RESULTS_PATH=./results/aie1-reworked/"
    test_arr[$LINENO]="make all AIE_VARIANT=1 MIXER_MODE=1 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=1536 DATA_TYPE=cint32 DDS_PHASE_INC=0xC6588855 UUT_SSR=6 SFDR=60 RESULTS_PATH=./results/aie1-reworked/"
    test_arr[$LINENO]="make all AIE_VARIANT=1 MIXER_MODE=1 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=1792 DATA_TYPE=cint16 DDS_PHASE_INC=0xC6588855 UUT_SSR=7 SFDR=90 RESULTS_PATH=./results/aie1-reworked/"
    test_arr[$LINENO]="make all AIE_VARIANT=1 MIXER_MODE=1 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=1280 DATA_TYPE=cint16 DDS_PHASE_INC=0xC6588855 UUT_SSR=5 SFDR=60 RESULTS_PATH=./results/aie1-reworked/"

    test_arr[$LINENO]="make all AIE_VARIANT=1 MIXER_MODE=0 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DATA_TYPE=cint32 DDS_PHASE_INC=0xC6588855 SFDR=150 RESULTS_PATH=./results/aie1-reworked/"
    test_arr[$LINENO]="make all AIE_VARIANT=1 MIXER_MODE=0 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DATA_TYPE=cint32 DDS_PHASE_INC=0xD6555555 SFDR=100 RESULTS_PATH=./results/aie1-reworked/"
    test_arr[$LINENO]="make all AIE_VARIANT=1 MIXER_MODE=0 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DATA_TYPE=cint32 DDS_PHASE_INC=0xC6588855 SFDR=60 RESULTS_PATH=./results/aie1-reworked/"
    test_arr[$LINENO]="make all AIE_VARIANT=1 MIXER_MODE=0 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DATA_TYPE=cint16 DDS_PHASE_INC=0xC6588855 SFDR=90 RESULTS_PATH=./results/aie1-reworked/"
    test_arr[$LINENO]="make all AIE_VARIANT=1 MIXER_MODE=0 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DATA_TYPE=cint16 DDS_PHASE_INC=0xC6588855 SFDR=60 RESULTS_PATH=./results/aie1-reworked/"

    test_arr[$LINENO]="make all AIE_VARIANT=1 MIXER_MODE=0 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=1024 DATA_TYPE=cint32 DDS_PHASE_INC=0xC6588855 UUT_SSR=4 SFDR=150 RESULTS_PATH=./results/aie1-reworked/"
    test_arr[$LINENO]="make all AIE_VARIANT=1 MIXER_MODE=0 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=1280 DATA_TYPE=cint32 DDS_PHASE_INC=0xD6555555 UUT_SSR=5 SFDR=100 RESULTS_PATH=./results/aie1-reworked/"
    test_arr[$LINENO]="make all AIE_VARIANT=1 MIXER_MODE=0 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=1536 DATA_TYPE=cint32 DDS_PHASE_INC=0xC6588855 UUT_SSR=6 SFDR=60 RESULTS_PATH=./results/aie1-reworked/"
    test_arr[$LINENO]="make all AIE_VARIANT=1 MIXER_MODE=0 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=1792 DATA_TYPE=cint16 DDS_PHASE_INC=0xC6588855 UUT_SSR=7 SFDR=90 RESULTS_PATH=./results/aie1-reworked/"
    test_arr[$LINENO]="make all AIE_VARIANT=1 MIXER_MODE=0 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=1280 DATA_TYPE=cint16 DDS_PHASE_INC=0xC6588855 UUT_SSR=5 SFDR=60 RESULTS_PATH=./results/aie1-reworked/"

    test_arr[$LINENO]="make all AIE_VARIANT=1 MIXER_MODE=2 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DATA_TYPE=cint32 DDS_PHASE_INC=0xC6588855 SFDR=150 RESULTS_PATH=./results/aie1-reworked/"
    test_arr[$LINENO]="make all AIE_VARIANT=1 MIXER_MODE=2 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DATA_TYPE=cint32 DDS_PHASE_INC=0xD6555555 SFDR=100 RESULTS_PATH=./results/aie1-reworked/"
    test_arr[$LINENO]="make all AIE_VARIANT=1 MIXER_MODE=2 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DATA_TYPE=cint32 DDS_PHASE_INC=0xC6588855 SFDR=60 RESULTS_PATH=./results/aie1-reworked/"
    test_arr[$LINENO]="make all AIE_VARIANT=1 MIXER_MODE=2 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DATA_TYPE=cint16 DDS_PHASE_INC=0xC6588855 SFDR=90 RESULTS_PATH=./results/aie1-reworked/"
    test_arr[$LINENO]="make all AIE_VARIANT=1 MIXER_MODE=2 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DATA_TYPE=cint16 DDS_PHASE_INC=0xC6588855 SFDR=60 RESULTS_PATH=./results/aie1-reworked/"

    test_arr[$LINENO]="make all AIE_VARIANT=1 MIXER_MODE=2 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=768 DATA_TYPE=cint32 DDS_PHASE_INC=0xC6588855 UUT_SSR=3 SFDR=150 RESULTS_PATH=./results/aie1-reworked/"
    test_arr[$LINENO]="make all AIE_VARIANT=1 MIXER_MODE=2 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=1024 DATA_TYPE=cint32 DDS_PHASE_INC=0xD6555555 UUT_SSR=4 SFDR=100 RESULTS_PATH=./results/aie1-reworked/"
    test_arr[$LINENO]="make all AIE_VARIANT=1 MIXER_MODE=2 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=1280 DATA_TYPE=cint32 DDS_PHASE_INC=0xC6588855 UUT_SSR=5 SFDR=60 RESULTS_PATH=./results/aie1-reworked/"
    test_arr[$LINENO]="make all AIE_VARIANT=1 MIXER_MODE=2 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=1536 DATA_TYPE=cint16 DDS_PHASE_INC=0xC6588855 UUT_SSR=6 SFDR=90 RESULTS_PATH=./results/aie1-reworked/"
    test_arr[$LINENO]="make all AIE_VARIANT=1 MIXER_MODE=2 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=1024 DATA_TYPE=cint16 DDS_PHASE_INC=0xC6588855 UUT_SSR=4 SFDR=60 RESULTS_PATH=./results/aie1-reworked/"

    # stream api
    test_arr[$LINENO]="make all P_API=1 AIE_VARIANT=1 MIXER_MODE=1 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DATA_TYPE=cint32 DDS_PHASE_INC=0xC6588855 SFDR=150 RESULTS_PATH=./results/aie1-reworked/"
    test_arr[$LINENO]="make all P_API=1 AIE_VARIANT=1 MIXER_MODE=1 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DATA_TYPE=cint32 DDS_PHASE_INC=0xD6555555 SFDR=100 RESULTS_PATH=./results/aie1-reworked/"
    test_arr[$LINENO]="make all P_API=1 AIE_VARIANT=1 MIXER_MODE=1 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DATA_TYPE=cint32 DDS_PHASE_INC=0xC6588855 SFDR=60 RESULTS_PATH=./results/aie1-reworked/"
    test_arr[$LINENO]="make all P_API=1 AIE_VARIANT=1 MIXER_MODE=1 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DATA_TYPE=cint16 DDS_PHASE_INC=0xC6588855 SFDR=90 RESULTS_PATH=./results/aie1-reworked/"
    test_arr[$LINENO]="make all P_API=1 AIE_VARIANT=1 MIXER_MODE=1 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DATA_TYPE=cint16 DDS_PHASE_INC=0xC6588855 SFDR=60 RESULTS_PATH=./results/aie1-reworked/"

    test_arr[$LINENO]="make all P_API=1 AIE_VARIANT=1 MIXER_MODE=1 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=1024 DATA_TYPE=cint32 DDS_PHASE_INC=0xC6588855 UUT_SSR=4 SFDR=150 RESULTS_PATH=./results/aie1-reworked/"
    test_arr[$LINENO]="make all P_API=1 AIE_VARIANT=1 MIXER_MODE=1 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=1280 DATA_TYPE=cint32 DDS_PHASE_INC=0xD6555555 UUT_SSR=5 SFDR=100 RESULTS_PATH=./results/aie1-reworked/"
    test_arr[$LINENO]="make all P_API=1 AIE_VARIANT=1 MIXER_MODE=1 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=1536 DATA_TYPE=cint32 DDS_PHASE_INC=0xC6588855 UUT_SSR=6 SFDR=60 RESULTS_PATH=./results/aie1-reworked/"
    test_arr[$LINENO]="make all P_API=1 AIE_VARIANT=1 MIXER_MODE=1 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=1792 DATA_TYPE=cint16 DDS_PHASE_INC=0xC6588855 UUT_SSR=7 SFDR=90 RESULTS_PATH=./results/aie1-reworked/"
    test_arr[$LINENO]="make all P_API=1 AIE_VARIANT=1 MIXER_MODE=1 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=1280 DATA_TYPE=cint16 DDS_PHASE_INC=0xC6588855 UUT_SSR=5 SFDR=60 RESULTS_PATH=./results/aie1-reworked/"

    test_arr[$LINENO]="make all P_API=1 AIE_VARIANT=1 MIXER_MODE=0 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DATA_TYPE=cint32 DDS_PHASE_INC=0xC6588855 SFDR=150 RESULTS_PATH=./results/aie1-reworked/"
    test_arr[$LINENO]="make all P_API=1 AIE_VARIANT=1 MIXER_MODE=0 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DATA_TYPE=cint32 DDS_PHASE_INC=0xD6555555 SFDR=100 RESULTS_PATH=./results/aie1-reworked/"
    test_arr[$LINENO]="make all P_API=1 AIE_VARIANT=1 MIXER_MODE=0 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DATA_TYPE=cint32 DDS_PHASE_INC=0xC6588855 SFDR=60 RESULTS_PATH=./results/aie1-reworked/"
    test_arr[$LINENO]="make all P_API=1 AIE_VARIANT=1 MIXER_MODE=0 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DATA_TYPE=cint16 DDS_PHASE_INC=0xC6588855 SFDR=90 RESULTS_PATH=./results/aie1-reworked/"
    test_arr[$LINENO]="make all P_API=1 AIE_VARIANT=1 MIXER_MODE=0 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DATA_TYPE=cint16 DDS_PHASE_INC=0xC6588855 SFDR=60 RESULTS_PATH=./results/aie1-reworked/"

    test_arr[$LINENO]="make all P_API=1 AIE_VARIANT=1 MIXER_MODE=0 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=1024 DATA_TYPE=cint32 DDS_PHASE_INC=0xC6588855 UUT_SSR=4 SFDR=150 RESULTS_PATH=./results/aie1-reworked/"
    test_arr[$LINENO]="make all P_API=1 AIE_VARIANT=1 MIXER_MODE=0 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=1280 DATA_TYPE=cint32 DDS_PHASE_INC=0xD6555555 UUT_SSR=5 SFDR=100 RESULTS_PATH=./results/aie1-reworked/"
    test_arr[$LINENO]="make all P_API=1 AIE_VARIANT=1 MIXER_MODE=0 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=1536 DATA_TYPE=cint32 DDS_PHASE_INC=0xC6588855 UUT_SSR=6 SFDR=60 RESULTS_PATH=./results/aie1-reworked/"
    test_arr[$LINENO]="make all P_API=1 AIE_VARIANT=1 MIXER_MODE=0 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=1792 DATA_TYPE=cint16 DDS_PHASE_INC=0xC6588855 UUT_SSR=7 SFDR=90 RESULTS_PATH=./results/aie1-reworked/"
    test_arr[$LINENO]="make all P_API=1 AIE_VARIANT=1 MIXER_MODE=0 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=1280 DATA_TYPE=cint16 DDS_PHASE_INC=0xC6588855 UUT_SSR=5 SFDR=60 RESULTS_PATH=./results/aie1-reworked/"

    test_arr[$LINENO]="make all P_API=1 AIE_VARIANT=1 MIXER_MODE=2 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DATA_TYPE=cint32 DDS_PHASE_INC=0xC6588855 SFDR=150 RESULTS_PATH=./results/aie1-reworked/"
    test_arr[$LINENO]="make all P_API=1 AIE_VARIANT=1 MIXER_MODE=2 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DATA_TYPE=cint32 DDS_PHASE_INC=0xD6555555 SFDR=100 RESULTS_PATH=./results/aie1-reworked/"
    test_arr[$LINENO]="make all P_API=1 AIE_VARIANT=1 MIXER_MODE=2 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DATA_TYPE=cint32 DDS_PHASE_INC=0xC6588855 SFDR=60 RESULTS_PATH=./results/aie1-reworked/"
    test_arr[$LINENO]="make all P_API=1 AIE_VARIANT=1 MIXER_MODE=2 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DATA_TYPE=cint16 DDS_PHASE_INC=0xC6588855 SFDR=90 RESULTS_PATH=./results/aie1-reworked/"
    test_arr[$LINENO]="make all P_API=1 AIE_VARIANT=1 MIXER_MODE=2 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DATA_TYPE=cint16 DDS_PHASE_INC=0xC6588855 SFDR=60 RESULTS_PATH=./results/aie1-reworked/"

    test_arr[$LINENO]="make all P_API=1 AIE_VARIANT=1 MIXER_MODE=2 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=768 DATA_TYPE=cint32 DDS_PHASE_INC=0xC6588855 UUT_SSR=3 SFDR=150 RESULTS_PATH=./results/aie1-reworked/"
    test_arr[$LINENO]="make all P_API=1 AIE_VARIANT=1 MIXER_MODE=2 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=1024 DATA_TYPE=cint32 DDS_PHASE_INC=0xD6555555 UUT_SSR=4 SFDR=100 RESULTS_PATH=./results/aie1-reworked/"
    test_arr[$LINENO]="make all P_API=1 AIE_VARIANT=1 MIXER_MODE=2 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=1280 DATA_TYPE=cint32 DDS_PHASE_INC=0xC6588855 UUT_SSR=5 SFDR=60 RESULTS_PATH=./results/aie1-reworked/"
    test_arr[$LINENO]="make all P_API=1 AIE_VARIANT=1 MIXER_MODE=2 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=1536 DATA_TYPE=cint16 DDS_PHASE_INC=0xC6588855 UUT_SSR=6 SFDR=90 RESULTS_PATH=./results/aie1-reworked/"
    test_arr[$LINENO]="make all P_API=1 AIE_VARIANT=1 MIXER_MODE=2 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=1024 DATA_TYPE=cint16 DDS_PHASE_INC=0xC6588855 UUT_SSR=4 SFDR=60 RESULTS_PATH=./results/aie1-reworked/"

    #phase offset iobuffer
    test_arr[$LINENO]="make all AIE_VARIANT=1 DATA_TYPE=cint16 MIXER_MODE=2 P_API=1 UUT_SSR=1  INPUT_WINDOW_VSIZE=256 USE_PHASE_RELOAD=1 PHASE_RELOAD_API=1 "
    test_arr[$LINENO]="make all AIE_VARIANT=1 DATA_TYPE=cint16 MIXER_MODE=1 P_API=1 UUT_SSR=1  INPUT_WINDOW_VSIZE=64  USE_PHASE_RELOAD=1 PHASE_RELOAD_API=1 "
    test_arr[$LINENO]="make all AIE_VARIANT=1 DATA_TYPE=cint32 MIXER_MODE=1 P_API=0 UUT_SSR=1  INPUT_WINDOW_VSIZE=128 USE_PHASE_RELOAD=1 PHASE_RELOAD_API=1 "
    test_arr[$LINENO]="make all AIE_VARIANT=1 DATA_TYPE=cint16 MIXER_MODE=0 P_API=1 UUT_SSR=1  INPUT_WINDOW_VSIZE=64  USE_PHASE_RELOAD=1 PHASE_RELOAD_API=1 "
fi

#Qor tests should be added to fit a suite with a collective purpose. There may be multiple suites with different purposes, e.g.
#Parameter exploration - where a base case is augmented by additional cases which vary one parameter such that the user
#  can see how QoR varies with a change in that parameter.
#Bellwether - standalone cases which describe a known or typical customer usecase, e.g. 47tap HB FIR with cint16/int16
#Anomaly. Where there is a known anomaly in QoR, such as a step change at a particular FIR length, cases may be added
# to highlight the non-linearity.
#Other. If there are other cases or other suites added for a different purpose, please add a description of the suite here.
if [[  "$*" == *qoraie1float*  ]]
then
    #AIE1
    test_arr[$LINENO]="make all AIE_VARIANT=1 MIXER_MODE=1 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DATA_TYPE=cfloat DDS_PHASE_INC=0xC6588855 SFDR=150 RESULTS_PATH=./results/aie1-reworked-float/"
    test_arr[$LINENO]="make all AIE_VARIANT=1 MIXER_MODE=1 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DATA_TYPE=cfloat DDS_PHASE_INC=0xD6555555 SFDR=100 RESULTS_PATH=./results/aie1-reworked-float/"
    test_arr[$LINENO]="make all AIE_VARIANT=1 MIXER_MODE=1 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DATA_TYPE=cfloat DDS_PHASE_INC=0xC6588855 SFDR=60 RESULTS_PATH=./results/aie1-reworked-float/"

    test_arr[$LINENO]="make all AIE_VARIANT=1 MIXER_MODE=1 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=1024 DATA_TYPE=cfloat DDS_PHASE_INC=0x9AAABBBB UUT_SSR=4 SFDR=150 RESULTS_PATH=./results/aie1-reworked-float/"
    test_arr[$LINENO]="make all AIE_VARIANT=1 MIXER_MODE=1 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=1280 DATA_TYPE=cfloat DDS_PHASE_INC=0xD6555555 UUT_SSR=5 SFDR=100 RESULTS_PATH=./results/aie1-reworked-float/"
    test_arr[$LINENO]="make all AIE_VARIANT=1 MIXER_MODE=1 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=1536 DATA_TYPE=cfloat DDS_PHASE_INC=0xC6588855 UUT_SSR=6 SFDR=60 RESULTS_PATH=./results/aie1-reworked-float/"

    test_arr[$LINENO]="make all AIE_VARIANT=1 MIXER_MODE=0 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DATA_TYPE=cfloat DDS_PHASE_INC=0xC6588855 SFDR=150 RESULTS_PATH=./results/aie1-reworked-float/"
    test_arr[$LINENO]="make all AIE_VARIANT=1 MIXER_MODE=0 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DATA_TYPE=cfloat DDS_PHASE_INC=0x9AAABBBB SFDR=100 RESULTS_PATH=./results/aie1-reworked-float/"
    test_arr[$LINENO]="make all AIE_VARIANT=1 MIXER_MODE=0 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DATA_TYPE=cfloat DDS_PHASE_INC=0xC6588855 SFDR=60 RESULTS_PATH=./results/aie1-reworked-float/"

    test_arr[$LINENO]="make all AIE_VARIANT=1 MIXER_MODE=0 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=1024 DATA_TYPE=cfloat DDS_PHASE_INC=0xC6588855 UUT_SSR=4 SFDR=150 RESULTS_PATH=./results/aie1-reworked-float/"
    test_arr[$LINENO]="make all AIE_VARIANT=1 MIXER_MODE=0 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=1280 DATA_TYPE=cfloat DDS_PHASE_INC=0x9AAABBBB UUT_SSR=5 SFDR=100 RESULTS_PATH=./results/aie1-reworked-float/"
    test_arr[$LINENO]="make all AIE_VARIANT=1 MIXER_MODE=0 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=1536 DATA_TYPE=cfloat DDS_PHASE_INC=0xC6588855 UUT_SSR=6 SFDR=60 RESULTS_PATH=./results/aie1-reworked-float/"

    test_arr[$LINENO]="make all AIE_VARIANT=1 MIXER_MODE=2 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DATA_TYPE=cfloat DDS_PHASE_INC=0x9AAABBBB SFDR=150 RESULTS_PATH=./results/aie1-reworked-float/"
    test_arr[$LINENO]="make all AIE_VARIANT=1 MIXER_MODE=2 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DATA_TYPE=cfloat DDS_PHASE_INC=0xD6555555 SFDR=100 RESULTS_PATH=./results/aie1-reworked-float/"
    test_arr[$LINENO]="make all AIE_VARIANT=1 MIXER_MODE=2 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DATA_TYPE=cfloat DDS_PHASE_INC=0xC6588855 SFDR=60 RESULTS_PATH=./results/aie1-reworked-float/"

    test_arr[$LINENO]="make all AIE_VARIANT=1 MIXER_MODE=2 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=768 DATA_TYPE=cfloat DDS_PHASE_INC=0xC6588855 UUT_SSR=3 SFDR=150 RESULTS_PATH=./results/aie1-reworked-float/"
    test_arr[$LINENO]="make all AIE_VARIANT=1 MIXER_MODE=2 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=1024 DATA_TYPE=cfloat DDS_PHASE_INC=0xD6555555 UUT_SSR=4 SFDR=100 RESULTS_PATH=./results/aie1-reworked-float/"
    test_arr[$LINENO]="make all AIE_VARIANT=1 MIXER_MODE=2 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=1280 DATA_TYPE=cfloat DDS_PHASE_INC=0x9AAABBBB UUT_SSR=5 SFDR=60 RESULTS_PATH=./results/aie1-reworked-float/"

    # stream api
    test_arr[$LINENO]="make all P_API=1 AIE_VARIANT=1 MIXER_MODE=1 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DATA_TYPE=cfloat DDS_PHASE_INC=0xC6588855 SFDR=150 RESULTS_PATH=./results/aie1-reworked-float/"
    test_arr[$LINENO]="make all P_API=1 AIE_VARIANT=1 MIXER_MODE=1 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DATA_TYPE=cfloat DDS_PHASE_INC=0x9AAABBBB SFDR=100 RESULTS_PATH=./results/aie1-reworked-float/"
    test_arr[$LINENO]="make all P_API=1 AIE_VARIANT=1 MIXER_MODE=1 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DATA_TYPE=cfloat DDS_PHASE_INC=0xC6588855 SFDR=60 RESULTS_PATH=./results/aie1-reworked-float/"

    test_arr[$LINENO]="make all P_API=1 AIE_VARIANT=1 MIXER_MODE=1 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=1024 DATA_TYPE=cfloat DDS_PHASE_INC=0x9AAABBBB UUT_SSR=4 SFDR=150 RESULTS_PATH=./results/aie1-reworked-float/"
    test_arr[$LINENO]="make all P_API=1 AIE_VARIANT=1 MIXER_MODE=1 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=1280 DATA_TYPE=cfloat DDS_PHASE_INC=0xD6555555 UUT_SSR=5 SFDR=100 RESULTS_PATH=./results/aie1-reworked-float/"
    test_arr[$LINENO]="make all P_API=1 AIE_VARIANT=1 MIXER_MODE=1 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=1536 DATA_TYPE=cfloat DDS_PHASE_INC=0xC6588855 UUT_SSR=6 SFDR=60 RESULTS_PATH=./results/aie1-reworked-float/"

    test_arr[$LINENO]="make all P_API=1 AIE_VARIANT=1 MIXER_MODE=0 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DATA_TYPE=cfloat DDS_PHASE_INC=0xC6588855 SFDR=150 RESULTS_PATH=./results/aie1-reworked-float/"
    test_arr[$LINENO]="make all P_API=1 AIE_VARIANT=1 MIXER_MODE=0 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DATA_TYPE=cfloat DDS_PHASE_INC=0xD6555555 SFDR=100 RESULTS_PATH=./results/aie1-reworked-float/"
    test_arr[$LINENO]="make all P_API=1 AIE_VARIANT=1 MIXER_MODE=0 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DATA_TYPE=cfloat DDS_PHASE_INC=0x9AAABBBB SFDR=60 RESULTS_PATH=./results/aie1-reworked-float/"

    test_arr[$LINENO]="make all P_API=1 AIE_VARIANT=1 MIXER_MODE=0 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=1024 DATA_TYPE=cfloat DDS_PHASE_INC=0x9AAABBBB UUT_SSR=4 SFDR=150 RESULTS_PATH=./results/aie1-reworked-float/"
    test_arr[$LINENO]="make all P_API=1 AIE_VARIANT=1 MIXER_MODE=0 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=1280 DATA_TYPE=cfloat DDS_PHASE_INC=0xD6555555 UUT_SSR=5 SFDR=100 RESULTS_PATH=./results/aie1-reworked-float/"
    test_arr[$LINENO]="make all P_API=1 AIE_VARIANT=1 MIXER_MODE=0 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=1536 DATA_TYPE=cfloat DDS_PHASE_INC=0xC6588855 UUT_SSR=6 SFDR=60 RESULTS_PATH=./results/aie1-reworked-float/"

    test_arr[$LINENO]="make all P_API=1 AIE_VARIANT=1 MIXER_MODE=2 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DATA_TYPE=cfloat DDS_PHASE_INC=0xC6588855 SFDR=150 RESULTS_PATH=./results/aie1-reworked-float/"
    test_arr[$LINENO]="make all P_API=1 AIE_VARIANT=1 MIXER_MODE=2 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DATA_TYPE=cfloat DDS_PHASE_INC=0x9AAABBBB SFDR=100 RESULTS_PATH=./results/aie1-reworked-float/"
    test_arr[$LINENO]="make all P_API=1 AIE_VARIANT=1 MIXER_MODE=2 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DATA_TYPE=cfloat DDS_PHASE_INC=0xC6588855 SFDR=60 RESULTS_PATH=./results/aie1-reworked-float/"

    test_arr[$LINENO]="make all P_API=1 AIE_VARIANT=1 MIXER_MODE=2 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=768 DATA_TYPE=cfloat DDS_PHASE_INC=0xC6588855 UUT_SSR=3 SFDR=150 RESULTS_PATH=./results/aie1-reworked-float/"
    test_arr[$LINENO]="make all P_API=1 AIE_VARIANT=1 MIXER_MODE=2 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=1024 DATA_TYPE=cfloat DDS_PHASE_INC=0xD6555555 UUT_SSR=4 SFDR=100 RESULTS_PATH=./results/aie1-reworked-float/"
    test_arr[$LINENO]="make all P_API=1 AIE_VARIANT=1 MIXER_MODE=2 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=1280 DATA_TYPE=cfloat DDS_PHASE_INC=0x9AAABBBB UUT_SSR=5 SFDR=60 RESULTS_PATH=./results/aie1-reworked-float/"
fi

#Qor tests should be added to fit a suite with a collective purpose. There may be multiple suites with different purposes, e.g.
#Parameter exploration - where a base case is augmented by additional cases which vary one parameter such that the user
#  can see how QoR varies with a change in that parameter.
#Bellwether - standalone cases which describe a known or typical customer usecase, e.g. 47tap HB FIR with cint16/int16
#Anomaly. Where there is a known anomaly in QoR, such as a step change at a particular FIR length, cases may be added
# to highlight the non-linearity.
#Other. If there are other cases or other suites added for a different purpose, please add a description of the suite here.
if [[  "$*" == *qor* ]]
then
    test_arr[$LINENO]="make all DATA_TYPE=cint16 MIXER_MODE=2 P_API=0 UUT_SSR=1  INPUT_WINDOW_VSIZE=256   NITER=8"
    test_arr[$LINENO]="make all DATA_TYPE=cint16 MIXER_MODE=0 P_API=0 UUT_SSR=1  INPUT_WINDOW_VSIZE=4096  NITER=8"
    test_arr[$LINENO]="make all DATA_TYPE=cint16 MIXER_MODE=2 P_API=1 UUT_SSR=1  INPUT_WINDOW_VSIZE=4096  NITER=8"
    test_arr[$LINENO]="make all DATA_TYPE=cint16 MIXER_MODE=1 P_API=1 UUT_SSR=1  INPUT_WINDOW_VSIZE=64    NITER=8"
    test_arr[$LINENO]="make all DATA_TYPE=cint16 MIXER_MODE=1 P_API=1 UUT_SSR=1  INPUT_WINDOW_VSIZE=256   NITER=8"
    test_arr[$LINENO]="make all DATA_TYPE=cint16 MIXER_MODE=0 P_API=1 UUT_SSR=1  INPUT_WINDOW_VSIZE=4096  NITER=8"
    test_arr[${#test_arr[*]}]="echo 'Running limited number of check-in tests.'"
    # min size
    test_arr[$LINENO]="make all DATA_TYPE=cint16 MIXER_MODE=2 P_API=0 UUT_SSR=1  INPUT_WINDOW_VSIZE=64    NITER=8"
    # max size
    test_arr[$LINENO]="make all DATA_TYPE=cint16 MIXER_MODE=2 P_API=0 UUT_SSR=1  INPUT_WINDOW_VSIZE=4096  NITER=8"
    test_arr[$LINENO]="make all DATA_TYPE=cint16 MIXER_MODE=1 P_API=0 UUT_SSR=1  INPUT_WINDOW_VSIZE=512   NITER=8"
    test_arr[$LINENO]="make all DATA_TYPE=cint16 MIXER_MODE=0 P_API=0 UUT_SSR=1  INPUT_WINDOW_VSIZE=512   NITER=8"
    test_arr[$LINENO]="make all DATA_TYPE=cint16 MIXER_MODE=1 P_API=1 UUT_SSR=1  INPUT_WINDOW_VSIZE=512   NITER=8"
    test_arr[$LINENO]="make all DATA_TYPE=cint16 MIXER_MODE=2 P_API=0 UUT_SSR=1  INPUT_WINDOW_VSIZE=512  TARGET=x86sim "

    test_arr[$LINENO]="make all DATA_TYPE=cint16 MIXER_MODE=2 P_API=1 UUT_SSR=1  INPUT_WINDOW_VSIZE=4096  NITER=8"
    test_arr[$LINENO]="make all DATA_TYPE=cint16 MIXER_MODE=2 P_API=1 UUT_SSR=2  INPUT_WINDOW_VSIZE=8192  NITER=8"
    test_arr[$LINENO]="make all DATA_TYPE=cint16 MIXER_MODE=2 P_API=1 UUT_SSR=3  INPUT_WINDOW_VSIZE=12288 NITER=8"
    test_arr[$LINENO]="make all DATA_TYPE=cint16 MIXER_MODE=2 P_API=1 UUT_SSR=4  INPUT_WINDOW_VSIZE=16384 NITER=8"
    test_arr[$LINENO]="make all DATA_TYPE=cint16 MIXER_MODE=2 P_API=1 UUT_SSR=5  INPUT_WINDOW_VSIZE=20480 NITER=8"
    test_arr[$LINENO]="make all DATA_TYPE=cint16 MIXER_MODE=2 P_API=1 UUT_SSR=11 INPUT_WINDOW_VSIZE=45056 NITER=8"

    test_arr[$LINENO]="make all DATA_TYPE=cint16 MIXER_MODE=0 P_API=0 UUT_SSR=16 INPUT_WINDOW_VSIZE=8192  NITER=8"
    test_arr[$LINENO]="make all DATA_TYPE=cint16 MIXER_MODE=0 P_API=1 UUT_SSR=16 INPUT_WINDOW_VSIZE=8192  NITER=8"
    test_arr[$LINENO]="make all DATA_TYPE=cint16 MIXER_MODE=1 P_API=0 UUT_SSR=13 INPUT_WINDOW_VSIZE=6656 DDS_PHASE_INC=0xD6555555 INITIAL_DDS_OFFSET=0xD6555555  NITER=8"

    # explore SFDR
    test_arr[$LINENO]="make all DATA_TYPE=cint16 MIXER_MODE=0 P_API=0 UUT_SSR=1  INPUT_WINDOW_VSIZE=512   SFDR=60  NITER=8"
    test_arr[$LINENO]="make all DATA_TYPE=cint16 MIXER_MODE=0 P_API=0 UUT_SSR=1  INPUT_WINDOW_VSIZE=512   SFDR=90  NITER=8"
    test_arr[$LINENO]="make all DATA_TYPE=cint16 MIXER_MODE=0 P_API=1 UUT_SSR=1  INPUT_WINDOW_VSIZE=4096  SFDR=60  NITER=8"
    test_arr[$LINENO]="make all DATA_TYPE=cint16 MIXER_MODE=0 P_API=1 UUT_SSR=1  INPUT_WINDOW_VSIZE=4096  SFDR=90  NITER=8"
    test_arr[$LINENO]="make all DATA_TYPE=cint32 MIXER_MODE=0 P_API=0 UUT_SSR=1  INPUT_WINDOW_VSIZE=256   SFDR=60  NITER=8"
    test_arr[$LINENO]="make all DATA_TYPE=cint32 MIXER_MODE=0 P_API=0 UUT_SSR=1  INPUT_WINDOW_VSIZE=256   SFDR=90  NITER=8"
    test_arr[$LINENO]="make all DATA_TYPE=cint32 MIXER_MODE=0 P_API=0 UUT_SSR=1  INPUT_WINDOW_VSIZE=256   SFDR=150 NITER=8"
    test_arr[$LINENO]="make all DATA_TYPE=cint32 MIXER_MODE=0 P_API=1 UUT_SSR=1  INPUT_WINDOW_VSIZE=4096  SFDR=60  NITER=8"
    test_arr[$LINENO]="make all DATA_TYPE=cint32 MIXER_MODE=0 P_API=1 UUT_SSR=1  INPUT_WINDOW_VSIZE=4096  SFDR=90  NITER=8"
    test_arr[$LINENO]="make all DATA_TYPE=cint32 MIXER_MODE=0 P_API=1 UUT_SSR=1  INPUT_WINDOW_VSIZE=4096  SFDR=120 NITER=8"

    test_arr[$LINENO]="make all DATA_TYPE=cint16 MIXER_MODE=1 P_API=0 UUT_SSR=1  INPUT_WINDOW_VSIZE=512   SFDR=60  NITER=8"
    test_arr[$LINENO]="make all DATA_TYPE=cint16 MIXER_MODE=1 P_API=0 UUT_SSR=1  INPUT_WINDOW_VSIZE=512   SFDR=90  NITER=8"
    # test_arr[$LINENO]="make all DATA_TYPE=cint16 MIXER_MODE=1 P_API=1 UUT_SSR=1  INPUT_WINDOW_VSIZE=4096  SFDR=60  NITER=8"
    test_arr[$LINENO]="make all DATA_TYPE=cint16 MIXER_MODE=1 P_API=1 UUT_SSR=1  INPUT_WINDOW_VSIZE=4096  SFDR=90  NITER=8"
    test_arr[$LINENO]="make all DATA_TYPE=cint32 MIXER_MODE=1 P_API=0 UUT_SSR=1  INPUT_WINDOW_VSIZE=256   SFDR=60  NITER=8"
    test_arr[$LINENO]="make all DATA_TYPE=cint32 MIXER_MODE=1 P_API=0 UUT_SSR=1  INPUT_WINDOW_VSIZE=256   SFDR=90  NITER=8"
    test_arr[$LINENO]="make all DATA_TYPE=cint32 MIXER_MODE=1 P_API=0 UUT_SSR=1  INPUT_WINDOW_VSIZE=256   SFDR=150 NITER=8"
    test_arr[$LINENO]="make all DATA_TYPE=cint32 MIXER_MODE=1 P_API=1 UUT_SSR=1  INPUT_WINDOW_VSIZE=4096  SFDR=60  NITER=8"
    test_arr[$LINENO]="make all DATA_TYPE=cint32 MIXER_MODE=1 P_API=1 UUT_SSR=1  INPUT_WINDOW_VSIZE=4096  SFDR=90  NITER=8"
    test_arr[$LINENO]="make all DATA_TYPE=cint32 MIXER_MODE=1 P_API=1 UUT_SSR=1  INPUT_WINDOW_VSIZE=4096  SFDR=120 NITER=8"
    #type support
    test_arr[$LINENO]="make all DATA_TYPE=cint32 MIXER_MODE=1 P_API=0 UUT_SSR=1  INPUT_WINDOW_VSIZE=80                                                                                         NITER=8"
    test_arr[$LINENO]="make all DATA_TYPE=cint32 MIXER_MODE=2 P_API=0 UUT_SSR=1  INPUT_WINDOW_VSIZE=80                                                                                         NITER=8"
    test_arr[$LINENO]="make all DATA_TYPE=cint32 MIXER_MODE=1 P_API=0 UUT_SSR=1  INPUT_WINDOW_VSIZE=80   DDS_PHASE_INC=0xD6555555 INITIAL_DDS_OFFSET=0xD6555555                                NITER=8"
    test_arr[$LINENO]="make all DATA_TYPE=cint32 MIXER_MODE=1 P_API=0 UUT_SSR=7  INPUT_WINDOW_VSIZE=560   DDS_PHASE_INC=0xD6555555                               NITER=8"
    test_arr[$LINENO]="make all DATA_TYPE=cint32 MIXER_MODE=1 P_API=0 UUT_SSR=7  INPUT_WINDOW_VSIZE=560   DDS_PHASE_INC=0xD6555555 INITIAL_DDS_OFFSET=0xD6555555 NITER=8"
    test_arr[$LINENO]="make all DATA_TYPE=cfloat MIXER_MODE=0 P_API=0 UUT_SSR=1  INPUT_WINDOW_VSIZE=256                                                                                        NITER=8"
    test_arr[$LINENO]="make all DATA_TYPE=cfloat MIXER_MODE=1 P_API=0 UUT_SSR=1  INPUT_WINDOW_VSIZE=256                                                                                        NITER=8"
    test_arr[$LINENO]="make all DATA_TYPE=cfloat MIXER_MODE=2 P_API=0 UUT_SSR=1  INPUT_WINDOW_VSIZE=256                                                                                        NITER=8"
    test_arr[$LINENO]="make all DATA_TYPE=cfloat MIXER_MODE=1 P_API=0 UUT_SSR=1  INPUT_WINDOW_VSIZE=80   DDS_PHASE_INC=0xD6555555 INITIAL_DDS_OFFSET=0xD6555555                                NITER=8"
    test_arr[$LINENO]="make all DATA_TYPE=cfloat MIXER_MODE=1 P_API=0 UUT_SSR=7  INPUT_WINDOW_VSIZE=560   DIFF_TOLERANCE=1 DDS_PHASE_INC=0xD6555555                               NITER=8"
    test_arr[$LINENO]="make all DATA_TYPE=cfloat MIXER_MODE=1 P_API=0 UUT_SSR=7  INPUT_WINDOW_VSIZE=560   DIFF_TOLERANCE=1 DDS_PHASE_INC=0xD6555555 INITIAL_DDS_OFFSET=0xD6555555 NITER=8"

    #AIE2
    test_arr[$LINENO]="make all AIE_VARIANT=2 DATA_TYPE=cint16 MIXER_MODE=2 P_API=0 UUT_SSR=1  INPUT_WINDOW_VSIZE=256   NITER=8"
    test_arr[$LINENO]="make all AIE_VARIANT=2 DATA_TYPE=cint16 MIXER_MODE=0 P_API=0 UUT_SSR=1  INPUT_WINDOW_VSIZE=4096  NITER=8"
    test_arr[$LINENO]="make all AIE_VARIANT=2 DATA_TYPE=cint16 MIXER_MODE=1 P_API=1 UUT_SSR=1  INPUT_WINDOW_VSIZE=64    NITER=8"
    test_arr[$LINENO]="make all AIE_VARIANT=2 DATA_TYPE=cint16 MIXER_MODE=1 P_API=1 UUT_SSR=1  INPUT_WINDOW_VSIZE=256   NITER=8"
    test_arr[$LINENO]="make all AIE_VARIANT=2 DATA_TYPE=cint16 MIXER_MODE=0 P_API=1 UUT_SSR=1  INPUT_WINDOW_VSIZE=4096  NITER=8"
    # min size
    test_arr[$LINENO]="make all AIE_VARIANT=2 DATA_TYPE=cint16 MIXER_MODE=2 P_API=0 UUT_SSR=1  INPUT_WINDOW_VSIZE=64    NITER=8"
    # max size
    test_arr[$LINENO]="make all AIE_VARIANT=2 DATA_TYPE=cint16 MIXER_MODE=2 P_API=0 UUT_SSR=1  INPUT_WINDOW_VSIZE=4096  NITER=8"
    test_arr[$LINENO]="make all AIE_VARIANT=2 DATA_TYPE=cint16 MIXER_MODE=1 P_API=0 UUT_SSR=1  INPUT_WINDOW_VSIZE=512   NITER=8"
    # modify SFDR values
    test_arr[$LINENO]="make all AIE_VARIANT=2 DATA_TYPE=cint16 MIXER_MODE=0 P_API=0 UUT_SSR=1  INPUT_WINDOW_VSIZE=512   NITER=8"


    test_arr[$LINENO]="make all AIE_VARIANT=2 DATA_TYPE=cint16 MIXER_MODE=1 P_API=1 UUT_SSR=1  INPUT_WINDOW_VSIZE=512   NITER=8"
    test_arr[$LINENO]="make all DATA_TYPE=cint16 MIXER_MODE=2 P_API=0 UUT_SSR=1  INPUT_WINDOW_VSIZE=512  TARGET=x86sim "

    test_arr[$LINENO]="make all AIE_VARIANT=2 DATA_TYPE=cint16 MIXER_MODE=1 P_API=1 UUT_SSR=1  INPUT_WINDOW_VSIZE=4096                                                         NITER=8"
    test_arr[$LINENO]="make all AIE_VARIANT=2 DATA_TYPE=cint16 MIXER_MODE=1 P_API=1 UUT_SSR=2  INPUT_WINDOW_VSIZE=8192                                                         NITER=8"
    test_arr[$LINENO]="make all AIE_VARIANT=2 DATA_TYPE=cint16 MIXER_MODE=1 P_API=1 UUT_SSR=3  INPUT_WINDOW_VSIZE=12288                                                        NITER=8"
    test_arr[$LINENO]="make all AIE_VARIANT=2 DATA_TYPE=cint16 MIXER_MODE=1 P_API=1 UUT_SSR=4  INPUT_WINDOW_VSIZE=16384                                                        NITER=8"
    test_arr[$LINENO]="make all AIE_VARIANT=2 DATA_TYPE=cint16 MIXER_MODE=1 P_API=1 UUT_SSR=5  INPUT_WINDOW_VSIZE=20480                                                        NITER=8"
    test_arr[$LINENO]="make all AIE_VARIANT=2 DATA_TYPE=cint16 MIXER_MODE=1 P_API=1 UUT_SSR=11 INPUT_WINDOW_VSIZE=45056                                                        NITER=8"

    test_arr[$LINENO]="make all AIE_VARIANT=2 DATA_TYPE=cint16 MIXER_MODE=0 P_API=0 UUT_SSR=16 INPUT_WINDOW_VSIZE=8192                                                         NITER=8"
    test_arr[$LINENO]="make all AIE_VARIANT=2 DATA_TYPE=cint16 MIXER_MODE=0 P_API=1 UUT_SSR=16 INPUT_WINDOW_VSIZE=8192                                                         NITER=8"
    test_arr[$LINENO]="make all AIE_VARIANT=2 DATA_TYPE=cint16 MIXER_MODE=1 P_API=0 UUT_SSR=13 INPUT_WINDOW_VSIZE=6656 DDS_PHASE_INC=0xD6555555 INITIAL_DDS_OFFSET=0xD6555555  NITER=8"

    #type support
    test_arr[$LINENO]="make all AIE_VARIANT=2 DATA_TYPE=cint32 MIXER_MODE=1 P_API=0 UUT_SSR=1  INPUT_WINDOW_VSIZE=64                                                                                         NITER=8"
    test_arr[$LINENO]="make all AIE_VARIANT=2 DATA_TYPE=cint32 MIXER_MODE=2 P_API=0 UUT_SSR=1  INPUT_WINDOW_VSIZE=64                                                                                         NITER=8"
    test_arr[$LINENO]="make all AIE_VARIANT=2 DATA_TYPE=cint32 MIXER_MODE=1 P_API=1 UUT_SSR=1  INPUT_WINDOW_VSIZE=64                                                                                         NITER=8"
    #test_arr[$LINENO]="make all AIE_VARIANT=2 DATA_TYPE=cint32 MIXER_MODE=2 P_API=1 UUT_SSR=1  INPUT_WINDOW_VSIZE=64    #MIXER MODE 2 cannot operate on AIE2 with stream interface(ML has one stream input)                                                                                     NITER=8"
    test_arr[$LINENO]="make all AIE_VARIANT=2 DATA_TYPE=cint32 MIXER_MODE=1 P_API=0 UUT_SSR=1  INPUT_WINDOW_VSIZE=128   DDS_PHASE_INC=0xD6555555 INITIAL_DDS_OFFSET=0xD6555555                                NITER=8"
    test_arr[$LINENO]="make all AIE_VARIANT=2 DATA_TYPE=cint32 MIXER_MODE=1 P_API=0 UUT_SSR=7  INPUT_WINDOW_VSIZE=896   DDS_PHASE_INC=0xD6555555                               NITER=8"
    test_arr[$LINENO]="make all AIE_VARIANT=2 DATA_TYPE=cint32 MIXER_MODE=1 P_API=0 UUT_SSR=7  INPUT_WINDOW_VSIZE=896   DDS_PHASE_INC=0xD6555555 INITIAL_DDS_OFFSET=0xD6555555 NITER=8"

    #explore SFDR
    test_arr[$LINENO]="make all AIE_VARIANT=2 DATA_TYPE=cint32 MIXER_MODE=0 P_API=0 UUT_SSR=1  INPUT_WINDOW_VSIZE=64       SFDR=50  NITER=8"
    test_arr[$LINENO]="make all AIE_VARIANT=2 DATA_TYPE=cint32 MIXER_MODE=0 P_API=0 UUT_SSR=1  INPUT_WINDOW_VSIZE=64       SFDR=90  NITER=8"
    test_arr[$LINENO]="make all AIE_VARIANT=2 DATA_TYPE=cint32 MIXER_MODE=0 P_API=0 UUT_SSR=1  INPUT_WINDOW_VSIZE=64       SFDR=130 NITER=8"
    test_arr[$LINENO]="make all AIE_VARIANT=2 DATA_TYPE=cint32 MIXER_MODE=0 P_API=1 UUT_SSR=1  INPUT_WINDOW_VSIZE=64       SFDR=50  NITER=8"
    test_arr[$LINENO]="make all AIE_VARIANT=2 DATA_TYPE=cint32 MIXER_MODE=0 P_API=1 UUT_SSR=1  INPUT_WINDOW_VSIZE=64       SFDR=90  NITER=8"
    test_arr[$LINENO]="make all AIE_VARIANT=2 DATA_TYPE=cint32 MIXER_MODE=0 P_API=1 UUT_SSR=1  INPUT_WINDOW_VSIZE=64       SFDR=130 NITER=8"
    test_arr[$LINENO]="make all AIE_VARIANT=2 DATA_TYPE=cint16 MIXER_MODE=0 P_API=0 UUT_SSR=1  INPUT_WINDOW_VSIZE=512      SFDR=50  NITER=8"
    test_arr[$LINENO]="make all AIE_VARIANT=2 DATA_TYPE=cint16 MIXER_MODE=0 P_API=0 UUT_SSR=1  INPUT_WINDOW_VSIZE=512      SFDR=90  NITER=8"
    test_arr[$LINENO]="make all AIE_VARIANT=2 DATA_TYPE=cint16 MIXER_MODE=0 P_API=1 UUT_SSR=1  INPUT_WINDOW_VSIZE=4096     SFDR=50  NITER=8"
    test_arr[$LINENO]="make all AIE_VARIANT=2 DATA_TYPE=cint16 MIXER_MODE=0 P_API=1 UUT_SSR=1  INPUT_WINDOW_VSIZE=4096     SFDR=90  NITER=8"

    test_arr[$LINENO]="make all AIE_VARIANT=2 DATA_TYPE=cint32 MIXER_MODE=2 P_API=0 UUT_SSR=1  INPUT_WINDOW_VSIZE=64       SFDR=50  NITER=8"
    test_arr[$LINENO]="make all AIE_VARIANT=2 DATA_TYPE=cint32 MIXER_MODE=2 P_API=0 UUT_SSR=1  INPUT_WINDOW_VSIZE=64       SFDR=90  NITER=8"
    test_arr[$LINENO]="make all AIE_VARIANT=2 DATA_TYPE=cint32 MIXER_MODE=2 P_API=0 UUT_SSR=1  INPUT_WINDOW_VSIZE=64       SFDR=130 NITER=8"
    test_arr[$LINENO]="make all AIE_VARIANT=2 DATA_TYPE=cint32 MIXER_MODE=1 P_API=1 UUT_SSR=1  INPUT_WINDOW_VSIZE=64       SFDR=50  NITER=8"
    test_arr[$LINENO]="make all AIE_VARIANT=2 DATA_TYPE=cint32 MIXER_MODE=1 P_API=1 UUT_SSR=1  INPUT_WINDOW_VSIZE=64       SFDR=90  NITER=8"
    test_arr[$LINENO]="make all AIE_VARIANT=2 DATA_TYPE=cint32 MIXER_MODE=1 P_API=1 UUT_SSR=1  INPUT_WINDOW_VSIZE=64       SFDR=130 NITER=8"
    test_arr[$LINENO]="make all AIE_VARIANT=2 DATA_TYPE=cint16 MIXER_MODE=2 P_API=0 UUT_SSR=1  INPUT_WINDOW_VSIZE=512      SFDR=50  NITER=8"
    test_arr[$LINENO]="make all AIE_VARIANT=2 DATA_TYPE=cint16 MIXER_MODE=2 P_API=0 UUT_SSR=1  INPUT_WINDOW_VSIZE=512      SFDR=90  NITER=8"
    test_arr[$LINENO]="make all AIE_VARIANT=2 DATA_TYPE=cint16 MIXER_MODE=1 P_API=1 UUT_SSR=1  INPUT_WINDOW_VSIZE=4096     SFDR=50  NITER=8"
    test_arr[$LINENO]="make all AIE_VARIANT=2 DATA_TYPE=cint16 MIXER_MODE=1 P_API=1 UUT_SSR=1  INPUT_WINDOW_VSIZE=4096     SFDR=90  NITER=8"

    #AIE2
    test_arr[$LINENO]="make all AIE_VARIANT=22 DATA_TYPE=cint16 MIXER_MODE=2 P_API=0 UUT_SSR=1  INPUT_WINDOW_VSIZE=256   NITER=8"
    test_arr[$LINENO]="make all AIE_VARIANT=22 DATA_TYPE=cint16 MIXER_MODE=0 P_API=0 UUT_SSR=1  INPUT_WINDOW_VSIZE=4096  NITER=8"
    test_arr[$LINENO]="make all AIE_VARIANT=22 DATA_TYPE=cint16 MIXER_MODE=1 P_API=1 UUT_SSR=1  INPUT_WINDOW_VSIZE=64    NITER=8"
    test_arr[$LINENO]="make all AIE_VARIANT=22 DATA_TYPE=cint16 MIXER_MODE=1 P_API=1 UUT_SSR=1  INPUT_WINDOW_VSIZE=256   NITER=8"
    test_arr[$LINENO]="make all AIE_VARIANT=22 DATA_TYPE=cint16 MIXER_MODE=0 P_API=1 UUT_SSR=1  INPUT_WINDOW_VSIZE=4096  NITER=8"
    # min size
    test_arr[$LINENO]="make all AIE_VARIANT=22 DATA_TYPE=cint16 MIXER_MODE=2 P_API=0 UUT_SSR=1  INPUT_WINDOW_VSIZE=64    NITER=8"
    # max size
    test_arr[$LINENO]="make all AIE_VARIANT=22 DATA_TYPE=cint16 MIXER_MODE=2 P_API=0 UUT_SSR=1  INPUT_WINDOW_VSIZE=4096  NITER=8"
    test_arr[$LINENO]="make all AIE_VARIANT=22 DATA_TYPE=cint16 MIXER_MODE=1 P_API=0 UUT_SSR=1  INPUT_WINDOW_VSIZE=512   NITER=8"
    # modify SFDR values
    test_arr[$LINENO]="make all AIE_VARIANT=22 DATA_TYPE=cint16 MIXER_MODE=0 P_API=0 UUT_SSR=1  INPUT_WINDOW_VSIZE=512   NITER=8"


    test_arr[$LINENO]="make all AIE_VARIANT=22 DATA_TYPE=cint16 MIXER_MODE=1 P_API=1 UUT_SSR=1  INPUT_WINDOW_VSIZE=512   NITER=8"
    test_arr[$LINENO]="make all DATA_TYPE=cint16 MIXER_MODE=2 P_API=0 UUT_SSR=1  INPUT_WINDOW_VSIZE=512  TARGET=x86sim "

    test_arr[$LINENO]="make all AIE_VARIANT=22 DATA_TYPE=cint16 MIXER_MODE=1 P_API=1 UUT_SSR=1  INPUT_WINDOW_VSIZE=4096                                                         NITER=8"
    test_arr[$LINENO]="make all AIE_VARIANT=22 DATA_TYPE=cint16 MIXER_MODE=1 P_API=1 UUT_SSR=2  INPUT_WINDOW_VSIZE=8192                                                         NITER=8"
    test_arr[$LINENO]="make all AIE_VARIANT=22 DATA_TYPE=cint16 MIXER_MODE=1 P_API=1 UUT_SSR=3  INPUT_WINDOW_VSIZE=12288                                                        NITER=8"
    test_arr[$LINENO]="make all AIE_VARIANT=22 DATA_TYPE=cint16 MIXER_MODE=1 P_API=1 UUT_SSR=4  INPUT_WINDOW_VSIZE=16384                                                        NITER=8"
    test_arr[$LINENO]="make all AIE_VARIANT=22 DATA_TYPE=cint16 MIXER_MODE=1 P_API=1 UUT_SSR=5  INPUT_WINDOW_VSIZE=20480                                                        NITER=8"
    test_arr[$LINENO]="make all AIE_VARIANT=22 DATA_TYPE=cint16 MIXER_MODE=1 P_API=1 UUT_SSR=11 INPUT_WINDOW_VSIZE=45056                                                        NITER=8"

    test_arr[$LINENO]="make all AIE_VARIANT=22 DATA_TYPE=cint16 MIXER_MODE=0 P_API=0 UUT_SSR=16 INPUT_WINDOW_VSIZE=8192                                                         NITER=8"
    test_arr[$LINENO]="make all AIE_VARIANT=22 DATA_TYPE=cint16 MIXER_MODE=0 P_API=1 UUT_SSR=16 INPUT_WINDOW_VSIZE=8192                                                         NITER=8"
    test_arr[$LINENO]="make all AIE_VARIANT=22 DATA_TYPE=cint16 MIXER_MODE=1 P_API=0 UUT_SSR=13 INPUT_WINDOW_VSIZE=6656 DDS_PHASE_INC=0xD6555555 INITIAL_DDS_OFFSET=0xD6555555  NITER=8"

    #type support
    test_arr[$LINENO]="make all AIE_VARIANT=22 DATA_TYPE=cint32 MIXER_MODE=1 P_API=0 UUT_SSR=1  INPUT_WINDOW_VSIZE=64                                                                                         NITER=8"
    test_arr[$LINENO]="make all AIE_VARIANT=22 DATA_TYPE=cint32 MIXER_MODE=2 P_API=0 UUT_SSR=1  INPUT_WINDOW_VSIZE=64                                                                                         NITER=8"
    test_arr[$LINENO]="make all AIE_VARIANT=22 DATA_TYPE=cint32 MIXER_MODE=1 P_API=1 UUT_SSR=1  INPUT_WINDOW_VSIZE=64                                                                                         NITER=8"
    #test_arr[$LINENO]="make all AIE_VARIANT=22 DATA_TYPE=cint32 MIXER_MODE=2 P_API=1 UUT_SSR=1  INPUT_WINDOW_VSIZE=64    #MIXER MODE 2 cannot operate on AIE2 with stream interface(ML has one stream input)                                                                                     NITER=8"
    test_arr[$LINENO]="make all AIE_VARIANT=22 DATA_TYPE=cint32 MIXER_MODE=1 P_API=0 UUT_SSR=1  INPUT_WINDOW_VSIZE=128   DDS_PHASE_INC=0xD6555555 INITIAL_DDS_OFFSET=0xD6555555                                NITER=8"
    test_arr[$LINENO]="make all AIE_VARIANT=22 DATA_TYPE=cint32 MIXER_MODE=1 P_API=0 UUT_SSR=7  INPUT_WINDOW_VSIZE=896   DDS_PHASE_INC=0xD6555555                               NITER=8"
    test_arr[$LINENO]="make all AIE_VARIANT=22 DATA_TYPE=cint32 MIXER_MODE=1 P_API=0 UUT_SSR=7  INPUT_WINDOW_VSIZE=896   DDS_PHASE_INC=0xD6555555 INITIAL_DDS_OFFSET=0xD6555555 NITER=8"

    #explore SFDR
    test_arr[$LINENO]="make all AIE_VARIANT=22 DATA_TYPE=cint32 MIXER_MODE=0 P_API=0 UUT_SSR=1  INPUT_WINDOW_VSIZE=64       SFDR=50  NITER=8"
    test_arr[$LINENO]="make all AIE_VARIANT=22 DATA_TYPE=cint32 MIXER_MODE=0 P_API=0 UUT_SSR=1  INPUT_WINDOW_VSIZE=64       SFDR=90  NITER=8"
    test_arr[$LINENO]="make all AIE_VARIANT=22 DATA_TYPE=cint32 MIXER_MODE=0 P_API=0 UUT_SSR=1  INPUT_WINDOW_VSIZE=64       SFDR=130 NITER=8"
    test_arr[$LINENO]="make all AIE_VARIANT=22 DATA_TYPE=cint32 MIXER_MODE=0 P_API=1 UUT_SSR=1  INPUT_WINDOW_VSIZE=64       SFDR=50  NITER=8"
    test_arr[$LINENO]="make all AIE_VARIANT=22 DATA_TYPE=cint32 MIXER_MODE=0 P_API=1 UUT_SSR=1  INPUT_WINDOW_VSIZE=64       SFDR=90  NITER=8"
    test_arr[$LINENO]="make all AIE_VARIANT=22 DATA_TYPE=cint32 MIXER_MODE=0 P_API=1 UUT_SSR=1  INPUT_WINDOW_VSIZE=64       SFDR=130 NITER=8"
    test_arr[$LINENO]="make all AIE_VARIANT=22 DATA_TYPE=cint16 MIXER_MODE=0 P_API=0 UUT_SSR=1  INPUT_WINDOW_VSIZE=512      SFDR=50  NITER=8"
    test_arr[$LINENO]="make all AIE_VARIANT=22 DATA_TYPE=cint16 MIXER_MODE=0 P_API=0 UUT_SSR=1  INPUT_WINDOW_VSIZE=512      SFDR=90  NITER=8"
    test_arr[$LINENO]="make all AIE_VARIANT=22 DATA_TYPE=cint16 MIXER_MODE=0 P_API=1 UUT_SSR=1  INPUT_WINDOW_VSIZE=4096     SFDR=50  NITER=8"
    test_arr[$LINENO]="make all AIE_VARIANT=22 DATA_TYPE=cint16 MIXER_MODE=0 P_API=1 UUT_SSR=1  INPUT_WINDOW_VSIZE=4096     SFDR=90  NITER=8"

    test_arr[$LINENO]="make all AIE_VARIANT=22 DATA_TYPE=cint32 MIXER_MODE=2 P_API=0 UUT_SSR=1  INPUT_WINDOW_VSIZE=64       SFDR=50  NITER=8"
    test_arr[$LINENO]="make all AIE_VARIANT=22 DATA_TYPE=cint32 MIXER_MODE=2 P_API=0 UUT_SSR=1  INPUT_WINDOW_VSIZE=64       SFDR=90  NITER=8"
    test_arr[$LINENO]="make all AIE_VARIANT=22 DATA_TYPE=cint32 MIXER_MODE=2 P_API=0 UUT_SSR=1  INPUT_WINDOW_VSIZE=64       SFDR=130 NITER=8"
    test_arr[$LINENO]="make all AIE_VARIANT=22 DATA_TYPE=cint32 MIXER_MODE=1 P_API=1 UUT_SSR=1  INPUT_WINDOW_VSIZE=64       SFDR=50  NITER=8"
    test_arr[$LINENO]="make all AIE_VARIANT=22 DATA_TYPE=cint32 MIXER_MODE=1 P_API=1 UUT_SSR=1  INPUT_WINDOW_VSIZE=64       SFDR=90  NITER=8"
    test_arr[$LINENO]="make all AIE_VARIANT=22 DATA_TYPE=cint32 MIXER_MODE=1 P_API=1 UUT_SSR=1  INPUT_WINDOW_VSIZE=64       SFDR=130 NITER=8"
    test_arr[$LINENO]="make all AIE_VARIANT=22 DATA_TYPE=cint16 MIXER_MODE=2 P_API=0 UUT_SSR=1  INPUT_WINDOW_VSIZE=512      SFDR=50  NITER=8"
    test_arr[$LINENO]="make all AIE_VARIANT=22 DATA_TYPE=cint16 MIXER_MODE=2 P_API=0 UUT_SSR=1  INPUT_WINDOW_VSIZE=512      SFDR=90  NITER=8"
    test_arr[$LINENO]="make all AIE_VARIANT=22 DATA_TYPE=cint16 MIXER_MODE=1 P_API=1 UUT_SSR=1  INPUT_WINDOW_VSIZE=4096     SFDR=50  NITER=8"
    test_arr[$LINENO]="make all AIE_VARIANT=22 DATA_TYPE=cint16 MIXER_MODE=1 P_API=1 UUT_SSR=1  INPUT_WINDOW_VSIZE=4096     SFDR=90  NITER=8"

    #Phase Offset Update Tests
    test_arr[$LINENO]="make all DATA_TYPE=cint16 MIXER_MODE=0 P_API=0 UUT_SSR=1  INPUT_WINDOW_VSIZE=4096  NITER=8 USE_PHASE_RELOAD=1"
    test_arr[$LINENO]="make all DATA_TYPE=cint16 MIXER_MODE=2 P_API=1 UUT_SSR=1  INPUT_WINDOW_VSIZE=4096  NITER=8 USE_PHASE_RELOAD=1"
    test_arr[$LINENO]="make all DATA_TYPE=cint16 MIXER_MODE=1 P_API=1 UUT_SSR=1  INPUT_WINDOW_VSIZE=64    NITER=8 USE_PHASE_RELOAD=1"

    test_arr[$LINENO]="make all AIE_VARIANT=1 DATA_TYPE=cint16 MIXER_MODE=0 P_API=1 UUT_SSR=1 DDS_PHASE_INC=0x00555555 INITIAL_DDS_OFFSET=0x00000000 INPUT_WINDOW_VSIZE=4096 USE_PHASE_RELOAD=1"

    test_arr[$LINENO]="make all AIE_VARIANT=1 DATA_TYPE=cint32 MIXER_MODE=2 P_API=1 UUT_SSR=1 INPUT_WINDOW_VSIZE=8192 NITER=8 USE_PHASE_RELOAD=1"
    test_arr[$LINENO]="make all AIE_VARIANT=2 DATA_TYPE=cint32 MIXER_MODE=0 P_API=1 UUT_SSR=1 INPUT_WINDOW_VSIZE=8192 NITER=8 USE_PHASE_RELOAD=1"
    test_arr[$LINENO]="make all AIE_VARIANT=2 DATA_TYPE=cint16 MIXER_MODE=1 P_API=0 UUT_SSR=1 INPUT_WINDOW_VSIZE=6656 DDS_PHASE_INC=0xD6555555 INITIAL_DDS_OFFSET=0xD6555555  NITER=8 USE_PHASE_RELOAD=1"

    test_arr[$LINENO]="make all AIE_VARIANT=1 DATA_TYPE=cfloat MIXER_MODE=0 P_API=0 UUT_SSR=1 SFDR=50 INPUT_WINDOW_VSIZE=128 USE_PHASE_RELOAD=1"
    test_arr[$LINENO]="make all AIE_VARIANT=1 DATA_TYPE=cfloat MIXER_MODE=2 P_API=1 UUT_SSR=1 SFDR=50 INPUT_WINDOW_VSIZE=512 USE_PHASE_RELOAD=1"
    test_arr[$LINENO]="make all AIE_VARIANT=1 DATA_TYPE=cfloat MIXER_MODE=1 P_API=1 UUT_SSR=1 SFDR=50 INPUT_WINDOW_VSIZE=4096 USE_PHASE_RELOAD=1"

fi

#suite for test of phase offset input as IObuffer.
if [[  "$*" == *ioPO*  ]]
then
    test_arr[$LINENO]="make all AIE_VARIANT=1 TARGET=x86sim DATA_TYPE=cint16 MIXER_MODE=0 P_API=0 UUT_SSR=1  INPUT_WINDOW_VSIZE=64  USE_PHASE_RELOAD=1 PHASE_RELOAD_API=1 STIM_TYPE=5"
    test_arr[$LINENO]="make all AIE_VARIANT=1 TARGET=x86sim DATA_TYPE=cint16 MIXER_MODE=2 P_API=1 UUT_SSR=1  INPUT_WINDOW_VSIZE=256 USE_PHASE_RELOAD=1 PHASE_RELOAD_API=1 "
    test_arr[$LINENO]="make all AIE_VARIANT=1 TARGET=hw     DATA_TYPE=cint16 MIXER_MODE=1 P_API=1 UUT_SSR=1  INPUT_WINDOW_VSIZE=64  USE_PHASE_RELOAD=1 PHASE_RELOAD_API=1 "
    test_arr[$LINENO]="make all AIE_VARIANT=1 TARGET=x86sim DATA_TYPE=cint32 MIXER_MODE=1 P_API=0 UUT_SSR=1  INPUT_WINDOW_VSIZE=128 USE_PHASE_RELOAD=1 PHASE_RELOAD_API=1 "
    test_arr[$LINENO]="make all AIE_VARIANT=1 TARGET=x86sim DATA_TYPE=cint16 MIXER_MODE=0 P_API=1 UUT_SSR=1  INPUT_WINDOW_VSIZE=64  USE_PHASE_RELOAD=1 PHASE_RELOAD_API=1 "
    test_arr[$LINENO]="make all AIE_VARIANT=1 TARGET=x86sim DATA_TYPE=cint16 MIXER_MODE=2 P_API=0 UUT_SSR=1  INPUT_WINDOW_VSIZE=256 USE_PHASE_RELOAD=1 PHASE_RELOAD_API=0 "
    test_arr[$LINENO]="make all AIE_VARIANT=1 TARGET=x86sim DATA_TYPE=cint16 MIXER_MODE=1 P_API=1 UUT_SSR=1  INPUT_WINDOW_VSIZE=64  USE_PHASE_RELOAD=1 PHASE_RELOAD_API=0 "
    test_arr[$LINENO]="make all AIE_VARIANT=1 TARGET=x86sim DATA_TYPE=cint16 MIXER_MODE=1 P_API=0 UUT_SSR=1  INPUT_WINDOW_VSIZE=128 USE_PHASE_RELOAD=0 PHASE_RELOAD_API=1 "
    test_arr[$LINENO]="make all AIE_VARIANT=1 TARGET=x86sim DATA_TYPE=cint16 MIXER_MODE=0 P_API=1 UUT_SSR=1  INPUT_WINDOW_VSIZE=64  USE_PHASE_RELOAD=0 PHASE_RELOAD_API=1 "
    test_arr[$LINENO]="make all AIE_VARIANT=2 TARGET=x86sim DATA_TYPE=cint16 MIXER_MODE=0 P_API=0 UUT_SSR=1  INPUT_WINDOW_VSIZE=64  USE_PHASE_RELOAD=1 PHASE_RELOAD_API=1 STIM_TYPE=5"
    test_arr[$LINENO]="make all AIE_VARIANT=2 TARGET=x86sim DATA_TYPE=cint16 MIXER_MODE=2 P_API=0 UUT_SSR=1  INPUT_WINDOW_VSIZE=256 USE_PHASE_RELOAD=1 PHASE_RELOAD_API=1 "
    test_arr[$LINENO]="make all AIE_VARIANT=2 TARGET=hw     DATA_TYPE=cint16 MIXER_MODE=1 P_API=1 UUT_SSR=1  INPUT_WINDOW_VSIZE=64  USE_PHASE_RELOAD=1 PHASE_RELOAD_API=1 "
    test_arr[$LINENO]="make all AIE_VARIANT=2 TARGET=x86sim DATA_TYPE=cint32 MIXER_MODE=1 P_API=0 UUT_SSR=1  INPUT_WINDOW_VSIZE=128 USE_PHASE_RELOAD=1 PHASE_RELOAD_API=1 "
    test_arr[$LINENO]="make all AIE_VARIANT=2 TARGET=x86sim DATA_TYPE=cint16 MIXER_MODE=0 P_API=1 UUT_SSR=1  INPUT_WINDOW_VSIZE=64  USE_PHASE_RELOAD=1 PHASE_RELOAD_API=1 "
    test_arr[$LINENO]="make all AIE_VARIANT=2 TARGET=x86sim DATA_TYPE=cint16 MIXER_MODE=2 P_API=0 UUT_SSR=1  INPUT_WINDOW_VSIZE=256 USE_PHASE_RELOAD=1 PHASE_RELOAD_API=0 "
    test_arr[$LINENO]="make all AIE_VARIANT=2 TARGET=x86sim DATA_TYPE=cint16 MIXER_MODE=1 P_API=1 UUT_SSR=1  INPUT_WINDOW_VSIZE=64  USE_PHASE_RELOAD=1 PHASE_RELOAD_API=0 "
    test_arr[$LINENO]="make all AIE_VARIANT=2 TARGET=x86sim DATA_TYPE=cint16 MIXER_MODE=1 P_API=0 UUT_SSR=1  INPUT_WINDOW_VSIZE=128 USE_PHASE_RELOAD=0 PHASE_RELOAD_API=1 "
    test_arr[$LINENO]="make all AIE_VARIANT=2 TARGET=x86sim DATA_TYPE=cint16 MIXER_MODE=0 P_API=1 UUT_SSR=1  INPUT_WINDOW_VSIZE=64  USE_PHASE_RELOAD=0 PHASE_RELOAD_API=1 "
fi

if [[  "$*" == *PI_RTP_full*  ]]
then
    test_arr[$LINENO]="make all AIE_VARIANT=1 TARGET=hw     DATA_TYPE=cint16 MIXER_MODE=2 P_API=0 UUT_SSR=1  INPUT_WINDOW_VSIZE=64  USE_PHASE_RELOAD=1 PHASE_RELOAD_API=1 USE_PHASE_INC_RELOAD=1 "
    test_arr[$LINENO]="make all AIE_VARIANT=1 TARGET=hw     DATA_TYPE=cint16 MIXER_MODE=2 P_API=0 UUT_SSR=1  INPUT_WINDOW_VSIZE=64  USE_PHASE_RELOAD=1 PHASE_RELOAD_API=0 USE_PHASE_INC_RELOAD=1 "
    test_arr[$LINENO]="make all AIE_VARIANT=1 TARGET=hw     DATA_TYPE=cint16 MIXER_MODE=2 P_API=0 UUT_SSR=1  INPUT_WINDOW_VSIZE=64  USE_PHASE_RELOAD=0 PHASE_RELOAD_API=0 USE_PHASE_INC_RELOAD=1 "
    test_arr[$LINENO]="make all AIE_VARIANT=1 TARGET=hw     DATA_TYPE=cint16 MIXER_MODE=1 P_API=0 UUT_SSR=1  INPUT_WINDOW_VSIZE=64  USE_PHASE_RELOAD=1 PHASE_RELOAD_API=1 USE_PHASE_INC_RELOAD=1 "
    test_arr[$LINENO]="make all AIE_VARIANT=1 TARGET=hw     DATA_TYPE=cint16 MIXER_MODE=1 P_API=0 UUT_SSR=1  INPUT_WINDOW_VSIZE=64  USE_PHASE_RELOAD=1 PHASE_RELOAD_API=0 USE_PHASE_INC_RELOAD=1 "
    test_arr[$LINENO]="make all AIE_VARIANT=1 TARGET=hw     DATA_TYPE=cint16 MIXER_MODE=1 P_API=0 UUT_SSR=1  INPUT_WINDOW_VSIZE=64  USE_PHASE_RELOAD=0 PHASE_RELOAD_API=0 USE_PHASE_INC_RELOAD=1 "
    test_arr[$LINENO]="make all AIE_VARIANT=1 TARGET=hw     DATA_TYPE=cint16 MIXER_MODE=0 P_API=0 UUT_SSR=1  INPUT_WINDOW_VSIZE=64  USE_PHASE_RELOAD=1 PHASE_RELOAD_API=1 USE_PHASE_INC_RELOAD=1 "
    test_arr[$LINENO]="make all AIE_VARIANT=1 TARGET=hw     DATA_TYPE=cint16 MIXER_MODE=0 P_API=0 UUT_SSR=1  INPUT_WINDOW_VSIZE=64  USE_PHASE_RELOAD=1 PHASE_RELOAD_API=0 USE_PHASE_INC_RELOAD=1 "
    test_arr[$LINENO]="make all AIE_VARIANT=1 TARGET=hw     DATA_TYPE=cint16 MIXER_MODE=0 P_API=0 UUT_SSR=1  INPUT_WINDOW_VSIZE=64  USE_PHASE_RELOAD=0 PHASE_RELOAD_API=0 USE_PHASE_INC_RELOAD=1 "
    test_arr[$LINENO]="make all AIE_VARIANT=1 TARGET=hw     DATA_TYPE=cint16 MIXER_MODE=2 P_API=1 UUT_SSR=1  INPUT_WINDOW_VSIZE=64  USE_PHASE_RELOAD=1 PHASE_RELOAD_API=1 USE_PHASE_INC_RELOAD=1 "
    test_arr[$LINENO]="make all AIE_VARIANT=1 TARGET=hw     DATA_TYPE=cint16 MIXER_MODE=2 P_API=1 UUT_SSR=1  INPUT_WINDOW_VSIZE=64  USE_PHASE_RELOAD=1 PHASE_RELOAD_API=0 USE_PHASE_INC_RELOAD=1 "
    test_arr[$LINENO]="make all AIE_VARIANT=1 TARGET=hw     DATA_TYPE=cint16 MIXER_MODE=2 P_API=1 UUT_SSR=1  INPUT_WINDOW_VSIZE=64  USE_PHASE_RELOAD=0 PHASE_RELOAD_API=0 USE_PHASE_INC_RELOAD=1 "
    test_arr[$LINENO]="make all AIE_VARIANT=1 TARGET=hw     DATA_TYPE=cint16 MIXER_MODE=1 P_API=1 UUT_SSR=1  INPUT_WINDOW_VSIZE=64  USE_PHASE_RELOAD=1 PHASE_RELOAD_API=1 USE_PHASE_INC_RELOAD=1 "
    test_arr[$LINENO]="make all AIE_VARIANT=1 TARGET=hw     DATA_TYPE=cint16 MIXER_MODE=1 P_API=1 UUT_SSR=1  INPUT_WINDOW_VSIZE=64  USE_PHASE_RELOAD=1 PHASE_RELOAD_API=0 USE_PHASE_INC_RELOAD=1 "
    test_arr[$LINENO]="make all AIE_VARIANT=1 TARGET=hw     DATA_TYPE=cint16 MIXER_MODE=1 P_API=1 UUT_SSR=1  INPUT_WINDOW_VSIZE=64  USE_PHASE_RELOAD=0 PHASE_RELOAD_API=0 USE_PHASE_INC_RELOAD=1 "
    test_arr[$LINENO]="make all AIE_VARIANT=1 TARGET=hw     DATA_TYPE=cint16 MIXER_MODE=0 P_API=1 UUT_SSR=1  INPUT_WINDOW_VSIZE=64  USE_PHASE_RELOAD=1 PHASE_RELOAD_API=1 USE_PHASE_INC_RELOAD=1 "
    test_arr[$LINENO]="make all AIE_VARIANT=1 TARGET=hw     DATA_TYPE=cint16 MIXER_MODE=0 P_API=1 UUT_SSR=1  INPUT_WINDOW_VSIZE=64  USE_PHASE_RELOAD=1 PHASE_RELOAD_API=0 USE_PHASE_INC_RELOAD=1 "
    test_arr[$LINENO]="make all AIE_VARIANT=1 TARGET=hw     DATA_TYPE=cint16 MIXER_MODE=0 P_API=1 UUT_SSR=1  INPUT_WINDOW_VSIZE=64  USE_PHASE_RELOAD=0 PHASE_RELOAD_API=0 USE_PHASE_INC_RELOAD=1 "
    test_arr[$LINENO]="make all AIE_VARIANT=1 TARGET=hw     DATA_TYPE=cfloat MIXER_MODE=2 P_API=0 UUT_SSR=1  INPUT_WINDOW_VSIZE=64  USE_PHASE_RELOAD=1 PHASE_RELOAD_API=1 USE_PHASE_INC_RELOAD=1 "
    test_arr[$LINENO]="make all AIE_VARIANT=1 TARGET=hw     DATA_TYPE=cfloat MIXER_MODE=2 P_API=0 UUT_SSR=1  INPUT_WINDOW_VSIZE=64  USE_PHASE_RELOAD=1 PHASE_RELOAD_API=0 USE_PHASE_INC_RELOAD=1 "
    test_arr[$LINENO]="make all AIE_VARIANT=1 TARGET=hw     DATA_TYPE=cfloat MIXER_MODE=2 P_API=0 UUT_SSR=1  INPUT_WINDOW_VSIZE=64  USE_PHASE_RELOAD=0 PHASE_RELOAD_API=0 USE_PHASE_INC_RELOAD=1 "
    test_arr[$LINENO]="make all AIE_VARIANT=1 TARGET=hw     DATA_TYPE=cfloat MIXER_MODE=1 P_API=0 UUT_SSR=1  INPUT_WINDOW_VSIZE=64  USE_PHASE_RELOAD=1 PHASE_RELOAD_API=1 USE_PHASE_INC_RELOAD=1 "
    test_arr[$LINENO]="make all AIE_VARIANT=1 TARGET=hw     DATA_TYPE=cfloat MIXER_MODE=1 P_API=0 UUT_SSR=1  INPUT_WINDOW_VSIZE=64  USE_PHASE_RELOAD=1 PHASE_RELOAD_API=0 USE_PHASE_INC_RELOAD=1 "
    test_arr[$LINENO]="make all AIE_VARIANT=1 TARGET=hw     DATA_TYPE=cfloat MIXER_MODE=1 P_API=0 UUT_SSR=1  INPUT_WINDOW_VSIZE=64  USE_PHASE_RELOAD=0 PHASE_RELOAD_API=0 USE_PHASE_INC_RELOAD=1 "
    test_arr[$LINENO]="make all AIE_VARIANT=1 TARGET=hw     DATA_TYPE=cfloat MIXER_MODE=0 P_API=0 UUT_SSR=1  INPUT_WINDOW_VSIZE=64  USE_PHASE_RELOAD=1 PHASE_RELOAD_API=1 USE_PHASE_INC_RELOAD=1 "
    test_arr[$LINENO]="make all AIE_VARIANT=1 TARGET=hw     DATA_TYPE=cfloat MIXER_MODE=0 P_API=0 UUT_SSR=1  INPUT_WINDOW_VSIZE=64  USE_PHASE_RELOAD=1 PHASE_RELOAD_API=0 USE_PHASE_INC_RELOAD=1 "
    test_arr[$LINENO]="make all AIE_VARIANT=1 TARGET=hw     DATA_TYPE=cfloat MIXER_MODE=0 P_API=0 UUT_SSR=1  INPUT_WINDOW_VSIZE=64  USE_PHASE_RELOAD=0 PHASE_RELOAD_API=0 USE_PHASE_INC_RELOAD=1 "
    test_arr[$LINENO]="make all AIE_VARIANT=1 TARGET=hw     DATA_TYPE=cfloat MIXER_MODE=2 P_API=1 UUT_SSR=1  INPUT_WINDOW_VSIZE=64  USE_PHASE_RELOAD=1 PHASE_RELOAD_API=1 USE_PHASE_INC_RELOAD=1 "
    test_arr[$LINENO]="make all AIE_VARIANT=1 TARGET=hw     DATA_TYPE=cfloat MIXER_MODE=2 P_API=1 UUT_SSR=1  INPUT_WINDOW_VSIZE=64  USE_PHASE_RELOAD=1 PHASE_RELOAD_API=0 USE_PHASE_INC_RELOAD=1 "
    test_arr[$LINENO]="make all AIE_VARIANT=1 TARGET=hw     DATA_TYPE=cfloat MIXER_MODE=2 P_API=1 UUT_SSR=1  INPUT_WINDOW_VSIZE=64  USE_PHASE_RELOAD=0 PHASE_RELOAD_API=0 USE_PHASE_INC_RELOAD=1 "
    test_arr[$LINENO]="make all AIE_VARIANT=1 TARGET=hw     DATA_TYPE=cfloat MIXER_MODE=1 P_API=1 UUT_SSR=1  INPUT_WINDOW_VSIZE=64  USE_PHASE_RELOAD=1 PHASE_RELOAD_API=1 USE_PHASE_INC_RELOAD=1 "
    test_arr[$LINENO]="make all AIE_VARIANT=1 TARGET=hw     DATA_TYPE=cfloat MIXER_MODE=1 P_API=1 UUT_SSR=1  INPUT_WINDOW_VSIZE=64  USE_PHASE_RELOAD=1 PHASE_RELOAD_API=0 USE_PHASE_INC_RELOAD=1 "
    test_arr[$LINENO]="make all AIE_VARIANT=1 TARGET=hw     DATA_TYPE=cfloat MIXER_MODE=1 P_API=1 UUT_SSR=1  INPUT_WINDOW_VSIZE=64  USE_PHASE_RELOAD=0 PHASE_RELOAD_API=0 USE_PHASE_INC_RELOAD=1 "
    test_arr[$LINENO]="make all AIE_VARIANT=1 TARGET=hw     DATA_TYPE=cfloat MIXER_MODE=0 P_API=1 UUT_SSR=1  INPUT_WINDOW_VSIZE=64  USE_PHASE_RELOAD=1 PHASE_RELOAD_API=1 USE_PHASE_INC_RELOAD=1 "
    test_arr[$LINENO]="make all AIE_VARIANT=1 TARGET=hw     DATA_TYPE=cfloat MIXER_MODE=0 P_API=1 UUT_SSR=1  INPUT_WINDOW_VSIZE=64  USE_PHASE_RELOAD=1 PHASE_RELOAD_API=0 USE_PHASE_INC_RELOAD=1 "
    test_arr[$LINENO]="make all AIE_VARIANT=1 TARGET=hw     DATA_TYPE=cfloat MIXER_MODE=0 P_API=1 UUT_SSR=1  INPUT_WINDOW_VSIZE=64  USE_PHASE_RELOAD=0 PHASE_RELOAD_API=0 USE_PHASE_INC_RELOAD=1 "
    test_arr[$LINENO]="make all AIE_VARIANT=2 TARGET=hw     DATA_TYPE=cint16 MIXER_MODE=2 P_API=0 UUT_SSR=1  INPUT_WINDOW_VSIZE=64  USE_PHASE_RELOAD=1 PHASE_RELOAD_API=1 USE_PHASE_INC_RELOAD=1 "
    test_arr[$LINENO]="make all AIE_VARIANT=2 TARGET=hw     DATA_TYPE=cint16 MIXER_MODE=2 P_API=0 UUT_SSR=1  INPUT_WINDOW_VSIZE=64  USE_PHASE_RELOAD=1 PHASE_RELOAD_API=0 USE_PHASE_INC_RELOAD=1 "
    test_arr[$LINENO]="make all AIE_VARIANT=2 TARGET=hw     DATA_TYPE=cint16 MIXER_MODE=2 P_API=0 UUT_SSR=1  INPUT_WINDOW_VSIZE=64  USE_PHASE_RELOAD=0 PHASE_RELOAD_API=0 USE_PHASE_INC_RELOAD=1 "
    test_arr[$LINENO]="make all AIE_VARIANT=2 TARGET=hw     DATA_TYPE=cint16 MIXER_MODE=1 P_API=0 UUT_SSR=1  INPUT_WINDOW_VSIZE=64  USE_PHASE_RELOAD=1 PHASE_RELOAD_API=1 USE_PHASE_INC_RELOAD=1 "
    test_arr[$LINENO]="make all AIE_VARIANT=2 TARGET=hw     DATA_TYPE=cint16 MIXER_MODE=1 P_API=0 UUT_SSR=1  INPUT_WINDOW_VSIZE=64  USE_PHASE_RELOAD=1 PHASE_RELOAD_API=0 USE_PHASE_INC_RELOAD=1 "
    test_arr[$LINENO]="make all AIE_VARIANT=2 TARGET=hw     DATA_TYPE=cint16 MIXER_MODE=1 P_API=0 UUT_SSR=1  INPUT_WINDOW_VSIZE=64  USE_PHASE_RELOAD=0 PHASE_RELOAD_API=0 USE_PHASE_INC_RELOAD=1 "
    test_arr[$LINENO]="make all AIE_VARIANT=2 TARGET=hw     DATA_TYPE=cint16 MIXER_MODE=0 P_API=0 UUT_SSR=1  INPUT_WINDOW_VSIZE=64  USE_PHASE_RELOAD=1 PHASE_RELOAD_API=1 USE_PHASE_INC_RELOAD=1 "
    test_arr[$LINENO]="make all AIE_VARIANT=2 TARGET=hw     DATA_TYPE=cint16 MIXER_MODE=0 P_API=0 UUT_SSR=1  INPUT_WINDOW_VSIZE=64  USE_PHASE_RELOAD=1 PHASE_RELOAD_API=0 USE_PHASE_INC_RELOAD=1 "
    test_arr[$LINENO]="make all AIE_VARIANT=2 TARGET=hw     DATA_TYPE=cint16 MIXER_MODE=0 P_API=0 UUT_SSR=1  INPUT_WINDOW_VSIZE=64  USE_PHASE_RELOAD=0 PHASE_RELOAD_API=0 USE_PHASE_INC_RELOAD=1 "
    test_arr[$LINENO]="make all AIE_VARIANT=2 TARGET=hw     DATA_TYPE=cint16 MIXER_MODE=1 P_API=1 UUT_SSR=1  INPUT_WINDOW_VSIZE=64  USE_PHASE_RELOAD=1 PHASE_RELOAD_API=1 USE_PHASE_INC_RELOAD=1 "
    test_arr[$LINENO]="make all AIE_VARIANT=2 TARGET=hw     DATA_TYPE=cint16 MIXER_MODE=1 P_API=1 UUT_SSR=1  INPUT_WINDOW_VSIZE=64  USE_PHASE_RELOAD=1 PHASE_RELOAD_API=0 USE_PHASE_INC_RELOAD=1 "
    test_arr[$LINENO]="make all AIE_VARIANT=2 TARGET=hw     DATA_TYPE=cint16 MIXER_MODE=1 P_API=1 UUT_SSR=1  INPUT_WINDOW_VSIZE=64  USE_PHASE_RELOAD=0 PHASE_RELOAD_API=0 USE_PHASE_INC_RELOAD=1 "
    test_arr[$LINENO]="make all AIE_VARIANT=2 TARGET=hw     DATA_TYPE=cint16 MIXER_MODE=0 P_API=1 UUT_SSR=1  INPUT_WINDOW_VSIZE=64  USE_PHASE_RELOAD=1 PHASE_RELOAD_API=1 USE_PHASE_INC_RELOAD=1 "
    test_arr[$LINENO]="make all AIE_VARIANT=2 TARGET=hw     DATA_TYPE=cint16 MIXER_MODE=0 P_API=1 UUT_SSR=1  INPUT_WINDOW_VSIZE=64  USE_PHASE_RELOAD=1 PHASE_RELOAD_API=0 USE_PHASE_INC_RELOAD=1 "
    test_arr[$LINENO]="make all AIE_VARIANT=2 TARGET=hw     DATA_TYPE=cint16 MIXER_MODE=0 P_API=1 UUT_SSR=1  INPUT_WINDOW_VSIZE=64  USE_PHASE_RELOAD=0 PHASE_RELOAD_API=0 USE_PHASE_INC_RELOAD=1 "
fi

if [[  "$*" == *testStreams*  ]]
then
    #max size
    test_arr[$LINENO]="make all TARGET=x86sim DATA_TYPE=cint16 MIXER_MODE=2 P_API=1 UUT_SSR=1  INPUT_WINDOW_VSIZE=4096  "
    test_arr[$LINENO]="make all TARGET=x86sim DATA_TYPE=cint16 MIXER_MODE=1 P_API=1 UUT_SSR=1  INPUT_WINDOW_VSIZE=8     "
    test_arr[$LINENO]="make all TARGET=x86sim DATA_TYPE=cint16 MIXER_MODE=1 P_API=1 UUT_SSR=1  INPUT_WINDOW_VSIZE=256   "
    test_arr[$LINENO]="make all TARGET=x86sim DATA_TYPE=cint16 MIXER_MODE=0 P_API=1 UUT_SSR=1  INPUT_WINDOW_VSIZE=4096  "
fi

if [[  "$*" == *sfdr_data*  ]]
then
    ##AIE2
    #test_arr[$LINENO]="make all AIE_VARIANT=2 MIXER_MODE=0 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DATA_TYPE=cint32 DDS_PHASE_INC=0xC6588855 UUT_SSR=2 SFDR=60 RESULTS_PATH=./results/sfdr_cint32_1tab/"
    #test_arr[$LINENO]="make all AIE_VARIANT=2 MIXER_MODE=0 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DATA_TYPE=cint32 DDS_PHASE_INC=0xB8000000 UUT_SSR=3 SFDR=60 RESULTS_PATH=./results/sfdr_cint32_1tab/"
    #test_arr[$LINENO]="make all AIE_VARIANT=2 MIXER_MODE=0 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DATA_TYPE=cint32 DDS_PHASE_INC=0xE4022000 UUT_SSR=4 SFDR=60 RESULTS_PATH=./results/sfdr_cint32_1tab/"
    #test_arr[$LINENO]="make all AIE_VARIANT=2 MIXER_MODE=0 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DATA_TYPE=cint32 DDS_PHASE_INC=0x9AAABBBB UUT_SSR=5 SFDR=60 RESULTS_PATH=./results/sfdr_cint32_1tab/"
    #test_arr[$LINENO]="make all AIE_VARIANT=2 MIXER_MODE=0 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DATA_TYPE=cint32 DDS_PHASE_INC=0xB3092845 UUT_SSR=2 SFDR=60 RESULTS_PATH=./results/sfdr_cint32_1tab/"
    #test_arr[$LINENO]="make all AIE_VARIANT=2 MIXER_MODE=0 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DATA_TYPE=cint32 DDS_PHASE_INC=0xE1387494 UUT_SSR=3 SFDR=60 RESULTS_PATH=./results/sfdr_cint32_1tab/"
    #test_arr[$LINENO]="make all AIE_VARIANT=2 MIXER_MODE=0 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DATA_TYPE=cint32 DDS_PHASE_INC=0x8AAAAAAA UUT_SSR=4 SFDR=60 RESULTS_PATH=./results/sfdr_cint32_1tab/"
    #test_arr[$LINENO]="make all AIE_VARIANT=2 MIXER_MODE=0 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DATA_TYPE=cint32 DDS_PHASE_INC=0xC2012239 UUT_SSR=5 SFDR=60 RESULTS_PATH=./results/sfdr_cint32_1tab/"
    #test_arr[$LINENO]="make all AIE_VARIANT=2 MIXER_MODE=0 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DATA_TYPE=cint32 DDS_PHASE_INC=0xA888A888 UUT_SSR=2 SFDR=60 RESULTS_PATH=./results/sfdr_cint32_1tab/"
    #test_arr[$LINENO]="make all AIE_VARIANT=2 MIXER_MODE=0 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DATA_TYPE=cint32 DDS_PHASE_INC=0xD6555555 UUT_SSR=3 SFDR=60 RESULTS_PATH=./results/sfdr_cint32_1tab/"
#
    #test_arr[$LINENO]="make all AIE_VARIANT=2 MIXER_MODE=0 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DATA_TYPE=cint32 DDS_PHASE_INC=0xC6588855 UUT_SSR=3 SFDR=110 RESULTS_PATH=./results/sfdr_cint32_2tab/"
    #test_arr[$LINENO]="make all AIE_VARIANT=2 MIXER_MODE=0 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DATA_TYPE=cint32 DDS_PHASE_INC=0xB8000000 UUT_SSR=4 SFDR=110 RESULTS_PATH=./results/sfdr_cint32_2tab/"
    #test_arr[$LINENO]="make all AIE_VARIANT=2 MIXER_MODE=0 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DATA_TYPE=cint32 DDS_PHASE_INC=0xE4022000 UUT_SSR=5 SFDR=110 RESULTS_PATH=./results/sfdr_cint32_2tab/"
    #test_arr[$LINENO]="make all AIE_VARIANT=2 MIXER_MODE=0 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DATA_TYPE=cint32 DDS_PHASE_INC=0x9AAABBBB UUT_SSR=6 SFDR=110 RESULTS_PATH=./results/sfdr_cint32_2tab/"
    #test_arr[$LINENO]="make all AIE_VARIANT=2 MIXER_MODE=0 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DATA_TYPE=cint32 DDS_PHASE_INC=0xB3092845 UUT_SSR=3 SFDR=110 RESULTS_PATH=./results/sfdr_cint32_2tab/"
    #test_arr[$LINENO]="make all AIE_VARIANT=2 MIXER_MODE=0 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DATA_TYPE=cint32 DDS_PHASE_INC=0xE1387494 UUT_SSR=4 SFDR=110 RESULTS_PATH=./results/sfdr_cint32_2tab/"
    #test_arr[$LINENO]="make all AIE_VARIANT=2 MIXER_MODE=0 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DATA_TYPE=cint32 DDS_PHASE_INC=0x8AAAAAAA UUT_SSR=5 SFDR=110 RESULTS_PATH=./results/sfdr_cint32_2tab/"
    #test_arr[$LINENO]="make all AIE_VARIANT=2 MIXER_MODE=0 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DATA_TYPE=cint32 DDS_PHASE_INC=0xC2012239 UUT_SSR=6 SFDR=110 RESULTS_PATH=./results/sfdr_cint32_2tab/"
    #test_arr[$LINENO]="make all AIE_VARIANT=2 MIXER_MODE=0 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DATA_TYPE=cint32 DDS_PHASE_INC=0xA888A888 UUT_SSR=3 SFDR=110 RESULTS_PATH=./results/sfdr_cint32_2tab/"
    #test_arr[$LINENO]="make all AIE_VARIANT=2 MIXER_MODE=0 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DATA_TYPE=cint32 DDS_PHASE_INC=0xD6555555 UUT_SSR=4 SFDR=110 RESULTS_PATH=./results/sfdr_cint32_2tab/"
#
    #test_arr[$LINENO]="make all AIE_VARIANT=2 MIXER_MODE=0 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DATA_TYPE=cint32 DDS_PHASE_INC=0xC6588855 UUT_SSR=4 SFDR=160 RESULTS_PATH=./results/sfdr_cint32_3tab/"
    #test_arr[$LINENO]="make all AIE_VARIANT=2 MIXER_MODE=0 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DATA_TYPE=cint32 DDS_PHASE_INC=0xB8000000 UUT_SSR=3 SFDR=160 RESULTS_PATH=./results/sfdr_cint32_3tab/"
    #test_arr[$LINENO]="make all AIE_VARIANT=2 MIXER_MODE=0 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DATA_TYPE=cint32 DDS_PHASE_INC=0xE4022000 UUT_SSR=5 SFDR=160 RESULTS_PATH=./results/sfdr_cint32_3tab/"
    #test_arr[$LINENO]="make all AIE_VARIANT=2 MIXER_MODE=0 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DATA_TYPE=cint32 DDS_PHASE_INC=0x9AAABBBB UUT_SSR=4 SFDR=160 RESULTS_PATH=./results/sfdr_cint32_3tab/"
    #test_arr[$LINENO]="make all AIE_VARIANT=2 MIXER_MODE=0 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DATA_TYPE=cint32 DDS_PHASE_INC=0xB3092845 UUT_SSR=3 SFDR=160 RESULTS_PATH=./results/sfdr_cint32_3tab/"
    #test_arr[$LINENO]="make all AIE_VARIANT=2 MIXER_MODE=0 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DATA_TYPE=cint32 DDS_PHASE_INC=0xE1387494 UUT_SSR=5 SFDR=160 RESULTS_PATH=./results/sfdr_cint32_3tab/"
    #test_arr[$LINENO]="make all AIE_VARIANT=2 MIXER_MODE=0 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DATA_TYPE=cint32 DDS_PHASE_INC=0x8AAAAAAA UUT_SSR=4 SFDR=160 RESULTS_PATH=./results/sfdr_cint32_3tab/"
    #test_arr[$LINENO]="make all AIE_VARIANT=2 MIXER_MODE=0 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DATA_TYPE=cint32 DDS_PHASE_INC=0xC2012239 UUT_SSR=3 SFDR=160 RESULTS_PATH=./results/sfdr_cint32_3tab/"
    #test_arr[$LINENO]="make all AIE_VARIANT=2 MIXER_MODE=0 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DATA_TYPE=cint32 DDS_PHASE_INC=0xA888A888 UUT_SSR=4 SFDR=160 RESULTS_PATH=./results/sfdr_cint32_3tab/"
    #test_arr[$LINENO]="make all AIE_VARIANT=2 MIXER_MODE=0 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DATA_TYPE=cint32 DDS_PHASE_INC=0xD6555555 UUT_SSR=2 SFDR=160 RESULTS_PATH=./results/sfdr_cint32_3tab/"
#
    #test_arr[$LINENO]="make all AIE_VARIANT=2 MIXER_MODE=0 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DATA_TYPE=cint16 DDS_PHASE_INC=0xC6588855 UUT_SSR=2 SFDR=60 RESULTS_PATH=./results/sfdr_cint16_1tab/"
    #test_arr[$LINENO]="make all AIE_VARIANT=2 MIXER_MODE=0 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DATA_TYPE=cint16 DDS_PHASE_INC=0xB8000000 UUT_SSR=3 SFDR=60 RESULTS_PATH=./results/sfdr_cint16_1tab/"
    #test_arr[$LINENO]="make all AIE_VARIANT=2 MIXER_MODE=0 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DATA_TYPE=cint16 DDS_PHASE_INC=0xE4022000 UUT_SSR=4 SFDR=60 RESULTS_PATH=./results/sfdr_cint16_1tab/"
    #test_arr[$LINENO]="make all AIE_VARIANT=2 MIXER_MODE=0 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DATA_TYPE=cint16 DDS_PHASE_INC=0x9AAABBBB UUT_SSR=5 SFDR=60 RESULTS_PATH=./results/sfdr_cint16_1tab/"
    #test_arr[$LINENO]="make all AIE_VARIANT=2 MIXER_MODE=0 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DATA_TYPE=cint16 DDS_PHASE_INC=0xB3092845 UUT_SSR=2 SFDR=60 RESULTS_PATH=./results/sfdr_cint16_1tab/"
    #test_arr[$LINENO]="make all AIE_VARIANT=2 MIXER_MODE=0 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DATA_TYPE=cint16 DDS_PHASE_INC=0xE1387494 UUT_SSR=3 SFDR=60 RESULTS_PATH=./results/sfdr_cint16_1tab/"
    #test_arr[$LINENO]="make all AIE_VARIANT=2 MIXER_MODE=0 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DATA_TYPE=cint16 DDS_PHASE_INC=0x8AAAAAAA UUT_SSR=4 SFDR=60 RESULTS_PATH=./results/sfdr_cint16_1tab/"
    #test_arr[$LINENO]="make all AIE_VARIANT=2 MIXER_MODE=0 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DATA_TYPE=cint16 DDS_PHASE_INC=0xC2012239 UUT_SSR=5 SFDR=60 RESULTS_PATH=./results/sfdr_cint16_1tab/"
    #test_arr[$LINENO]="make all AIE_VARIANT=2 MIXER_MODE=0 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DATA_TYPE=cint16 DDS_PHASE_INC=0xA888A888 UUT_SSR=2 SFDR=60 RESULTS_PATH=./results/sfdr_cint16_1tab/"
    #test_arr[$LINENO]="make all AIE_VARIANT=2 MIXER_MODE=0 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DATA_TYPE=cint16 DDS_PHASE_INC=0xD6555555 UUT_SSR=3 SFDR=60 RESULTS_PATH=./results/sfdr_cint16_1tab/"

    test_arr[$LINENO]="make all AIE_VARIANT=2 MIXER_MODE=0 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DATA_TYPE=cint16 DDS_PHASE_INC=0xC6588855 UUT_SSR=3 SFDR=96 RESULTS_PATH=./results/sfdr_cint16_2tab/"
    test_arr[$LINENO]="make all AIE_VARIANT=2 MIXER_MODE=0 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DATA_TYPE=cint16 DDS_PHASE_INC=0xB8000000 UUT_SSR=4 SFDR=96 RESULTS_PATH=./results/sfdr_cint16_2tab/"
    test_arr[$LINENO]="make all AIE_VARIANT=2 MIXER_MODE=0 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DATA_TYPE=cint16 DDS_PHASE_INC=0xE4022000 UUT_SSR=5 SFDR=96 RESULTS_PATH=./results/sfdr_cint16_2tab/"
    test_arr[$LINENO]="make all AIE_VARIANT=2 MIXER_MODE=0 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DATA_TYPE=cint16 DDS_PHASE_INC=0x9AAABBBB UUT_SSR=6 SFDR=96 RESULTS_PATH=./results/sfdr_cint16_2tab/"
    test_arr[$LINENO]="make all AIE_VARIANT=2 MIXER_MODE=0 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DATA_TYPE=cint16 DDS_PHASE_INC=0xB3092845 UUT_SSR=3 SFDR=96 RESULTS_PATH=./results/sfdr_cint16_2tab/"
    test_arr[$LINENO]="make all AIE_VARIANT=2 MIXER_MODE=0 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DATA_TYPE=cint16 DDS_PHASE_INC=0xE1387494 UUT_SSR=4 SFDR=96 RESULTS_PATH=./results/sfdr_cint16_2tab/"
    test_arr[$LINENO]="make all AIE_VARIANT=2 MIXER_MODE=0 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DATA_TYPE=cint16 DDS_PHASE_INC=0x8AAAAAAA UUT_SSR=5 SFDR=96 RESULTS_PATH=./results/sfdr_cint16_2tab/"
    test_arr[$LINENO]="make all AIE_VARIANT=2 MIXER_MODE=0 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DATA_TYPE=cint16 DDS_PHASE_INC=0xC2012239 UUT_SSR=6 SFDR=96 RESULTS_PATH=./results/sfdr_cint16_2tab/"
    test_arr[$LINENO]="make all AIE_VARIANT=2 MIXER_MODE=0 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DATA_TYPE=cint16 DDS_PHASE_INC=0xA888A888 UUT_SSR=3 SFDR=96 RESULTS_PATH=./results/sfdr_cint16_2tab/"
    test_arr[$LINENO]="make all AIE_VARIANT=2 MIXER_MODE=0 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DATA_TYPE=cint16 DDS_PHASE_INC=0xD6555555 UUT_SSR=4 SFDR=96 RESULTS_PATH=./results/sfdr_cint16_2tab/"
    #AIE1
    #test_arr[$LINENO]="make ref AIE_VARIANT=1 MIXER_MODE=0 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DDS_PHASE_INC=0xC6588855 DATA_TYPE=cint32 RESULTS_PATH=./results/sfdr_cint32_var1/"
    #test_arr[$LINENO]="make ref AIE_VARIANT=1 MIXER_MODE=0 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DDS_PHASE_INC=0xB8000000 DATA_TYPE=cint32 RESULTS_PATH=./results/sfdr_cint32_var1/"
    #test_arr[$LINENO]="make ref AIE_VARIANT=1 MIXER_MODE=0 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DDS_PHASE_INC=0xE4022000 DATA_TYPE=cint32 RESULTS_PATH=./results/sfdr_cint32_var1/"
    #test_arr[$LINENO]="make ref AIE_VARIANT=1 MIXER_MODE=0 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DDS_PHASE_INC=0x9AAABBBB DATA_TYPE=cint32 RESULTS_PATH=./results/sfdr_cint32_var1/"
    #test_arr[$LINENO]="make ref AIE_VARIANT=1 MIXER_MODE=0 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DDS_PHASE_INC=0xB3092845 DATA_TYPE=cint32 RESULTS_PATH=./results/sfdr_cint32_var1/"
    #test_arr[$LINENO]="make ref AIE_VARIANT=1 MIXER_MODE=0 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DDS_PHASE_INC=0xE1387494 DATA_TYPE=cint32 RESULTS_PATH=./results/sfdr_cint32_var1/"
    #test_arr[$LINENO]="make ref AIE_VARIANT=1 MIXER_MODE=0 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DDS_PHASE_INC=0x8AAAAAAA DATA_TYPE=cint32 RESULTS_PATH=./results/sfdr_cint32_var1/"
    #test_arr[$LINENO]="make ref AIE_VARIANT=1 MIXER_MODE=0 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DDS_PHASE_INC=0xC2012239 DATA_TYPE=cint32 RESULTS_PATH=./results/sfdr_cint32_var1/"
    #test_arr[$LINENO]="make ref AIE_VARIANT=1 MIXER_MODE=0 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DDS_PHASE_INC=0xA888A888 DATA_TYPE=cint32 RESULTS_PATH=./results/sfdr_cint32_var1/"
    #test_arr[$LINENO]="make ref AIE_VARIANT=1 MIXER_MODE=0 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DDS_PHASE_INC=0xD6555555 DATA_TYPE=cint32 RESULTS_PATH=./results/sfdr_cint32_var1/"

fi


if [[  "$*" == *ssr*  ]]
then
    #AIE2
    test_arr[$LINENO]="make all AIE_VARIANT=2 MIXER_MODE=0 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DATA_TYPE=cint32 DDS_PHASE_INC=0xC6588855 UUT_SSR=5 RESULTS_PATH=./results/ssr5cint32/"
    test_arr[$LINENO]="make all AIE_VARIANT=2 MIXER_MODE=0 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DATA_TYPE=cint32 DDS_PHASE_INC=0xB8000000 UUT_SSR=5 RESULTS_PATH=./results/ssr5cint32/"
    test_arr[$LINENO]="make all AIE_VARIANT=2 MIXER_MODE=0 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DATA_TYPE=cint32 DDS_PHASE_INC=0xE4022000 UUT_SSR=5 RESULTS_PATH=./results/ssr5cint32/"
    test_arr[$LINENO]="make all AIE_VARIANT=2 MIXER_MODE=0 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DATA_TYPE=cint32 DDS_PHASE_INC=0x9AAABBBB UUT_SSR=5 RESULTS_PATH=./results/ssr5cint32/"
    test_arr[$LINENO]="make all AIE_VARIANT=2 MIXER_MODE=0 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DATA_TYPE=cint32 DDS_PHASE_INC=0xB3092845 UUT_SSR=5 RESULTS_PATH=./results/ssr5cint32/"
    test_arr[$LINENO]="make all AIE_VARIANT=2 MIXER_MODE=0 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DATA_TYPE=cint32 DDS_PHASE_INC=0xE1387494 UUT_SSR=5 RESULTS_PATH=./results/ssr5cint32/"
    test_arr[$LINENO]="make all AIE_VARIANT=2 MIXER_MODE=0 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DATA_TYPE=cint32 DDS_PHASE_INC=0x8AAAAAAA UUT_SSR=5 RESULTS_PATH=./results/ssr5cint32/"
    test_arr[$LINENO]="make all AIE_VARIANT=2 MIXER_MODE=0 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DATA_TYPE=cint32 DDS_PHASE_INC=0xC2012239 UUT_SSR=5 RESULTS_PATH=./results/ssr5cint32/"
    test_arr[$LINENO]="make all AIE_VARIANT=2 MIXER_MODE=0 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DATA_TYPE=cint32 DDS_PHASE_INC=0xA888A888 UUT_SSR=5 RESULTS_PATH=./results/ssr5cint32/"
    test_arr[$LINENO]="make all AIE_VARIANT=2 MIXER_MODE=0 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DATA_TYPE=cint32 DDS_PHASE_INC=0xD6555555 UUT_SSR=5 RESULTS_PATH=./results/ssr5cint32/"

    test_arr[$LINENO]="make all AIE_VARIANT=2 MIXER_MODE=0 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DATA_TYPE=cint32 DDS_PHASE_INC=0xC6588855 UUT_SSR=6 RESULTS_PATH=./results/ssr6cint32/"
    test_arr[$LINENO]="make all AIE_VARIANT=2 MIXER_MODE=0 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DATA_TYPE=cint32 DDS_PHASE_INC=0xB8000000 UUT_SSR=6 RESULTS_PATH=./results/ssr6cint32/"
    test_arr[$LINENO]="make all AIE_VARIANT=2 MIXER_MODE=0 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DATA_TYPE=cint32 DDS_PHASE_INC=0xE4022000 UUT_SSR=6 RESULTS_PATH=./results/ssr6cint32/"
    test_arr[$LINENO]="make all AIE_VARIANT=2 MIXER_MODE=0 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DATA_TYPE=cint32 DDS_PHASE_INC=0x9AAABBBB UUT_SSR=6 RESULTS_PATH=./results/ssr6cint32/"
    test_arr[$LINENO]="make all AIE_VARIANT=2 MIXER_MODE=0 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DATA_TYPE=cint32 DDS_PHASE_INC=0xB3092845 UUT_SSR=6 RESULTS_PATH=./results/ssr6cint32/"
    test_arr[$LINENO]="make all AIE_VARIANT=2 MIXER_MODE=0 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DATA_TYPE=cint32 DDS_PHASE_INC=0xE1387494 UUT_SSR=6 RESULTS_PATH=./results/ssr6cint32/"
    test_arr[$LINENO]="make all AIE_VARIANT=2 MIXER_MODE=0 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DATA_TYPE=cint32 DDS_PHASE_INC=0x8AAAAAAA UUT_SSR=6 RESULTS_PATH=./results/ssr6cint32/"
    test_arr[$LINENO]="make all AIE_VARIANT=2 MIXER_MODE=0 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DATA_TYPE=cint32 DDS_PHASE_INC=0xC2012239 UUT_SSR=6 RESULTS_PATH=./results/ssr6cint32/"
    test_arr[$LINENO]="make all AIE_VARIANT=2 MIXER_MODE=0 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DATA_TYPE=cint32 DDS_PHASE_INC=0xA888A888 UUT_SSR=6 RESULTS_PATH=./results/ssr6cint32/"
    test_arr[$LINENO]="make all AIE_VARIANT=2 MIXER_MODE=0 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DATA_TYPE=cint32 DDS_PHASE_INC=0xD6555555 UUT_SSR=6 RESULTS_PATH=./results/ssr6cint32/"

    test_arr[$LINENO]="make all AIE_VARIANT=2 MIXER_MODE=0 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=1792 DATA_TYPE=cint32 DDS_PHASE_INC=0xC6588855 UUT_SSR=7 RESULTS_PATH=./results/ssr7cint32/"
    test_arr[$LINENO]="make all AIE_VARIANT=2 MIXER_MODE=0 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=1792 DATA_TYPE=cint32 DDS_PHASE_INC=0xB8000000 UUT_SSR=7 RESULTS_PATH=./results/ssr7cint32/"
    test_arr[$LINENO]="make all AIE_VARIANT=2 MIXER_MODE=0 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=1792 DATA_TYPE=cint32 DDS_PHASE_INC=0xE4022000 UUT_SSR=7 RESULTS_PATH=./results/ssr7cint32/"
    test_arr[$LINENO]="make all AIE_VARIANT=2 MIXER_MODE=0 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=1792 DATA_TYPE=cint32 DDS_PHASE_INC=0x9AAABBBB UUT_SSR=7 RESULTS_PATH=./results/ssr7cint32/"
    test_arr[$LINENO]="make all AIE_VARIANT=2 MIXER_MODE=0 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=1792 DATA_TYPE=cint32 DDS_PHASE_INC=0xB3092845 UUT_SSR=7 RESULTS_PATH=./results/ssr7cint32/"
    test_arr[$LINENO]="make all AIE_VARIANT=2 MIXER_MODE=0 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=1792 DATA_TYPE=cint32 DDS_PHASE_INC=0xE1387494 UUT_SSR=7 RESULTS_PATH=./results/ssr7cint32/"
    test_arr[$LINENO]="make all AIE_VARIANT=2 MIXER_MODE=0 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=1792 DATA_TYPE=cint32 DDS_PHASE_INC=0x8AAAAAAA UUT_SSR=7 RESULTS_PATH=./results/ssr7cint32/"
    test_arr[$LINENO]="make all AIE_VARIANT=2 MIXER_MODE=0 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=1792 DATA_TYPE=cint32 DDS_PHASE_INC=0xC2012239 UUT_SSR=7 RESULTS_PATH=./results/ssr7cint32/"
    test_arr[$LINENO]="make all AIE_VARIANT=2 MIXER_MODE=0 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=1792 DATA_TYPE=cint32 DDS_PHASE_INC=0xA888A888 UUT_SSR=7 RESULTS_PATH=./results/ssr7cint32/"
    test_arr[$LINENO]="make all AIE_VARIANT=2 MIXER_MODE=0 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=1792 DATA_TYPE=cint32 DDS_PHASE_INC=0xD6555555 UUT_SSR=7 RESULTS_PATH=./results/ssr7cint32/"

    ##AIE2
    #test_arr[$LINENO]="make all AIE_VARIANT=2 MIXER_MODE=0 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DATA_TYPE=cint16 DDS_PHASE_INC=0xC6588855 UUT_SSR=2 RESULTS_PATH=./results/ssr2cint16/"
    #test_arr[$LINENO]="make all AIE_VARIANT=2 MIXER_MODE=0 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DATA_TYPE=cint16 DDS_PHASE_INC=0xB8000000 UUT_SSR=2 RESULTS_PATH=./results/ssr2cint16/"
    #test_arr[$LINENO]="make all AIE_VARIANT=2 MIXER_MODE=0 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DATA_TYPE=cint16 DDS_PHASE_INC=0xE4022000 UUT_SSR=2 RESULTS_PATH=./results/ssr2cint16/"
    #test_arr[$LINENO]="make all AIE_VARIANT=2 MIXER_MODE=0 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DATA_TYPE=cint16 DDS_PHASE_INC=0x9AAABBBB UUT_SSR=2 RESULTS_PATH=./results/ssr2cint16/"
    #test_arr[$LINENO]="make all AIE_VARIANT=2 MIXER_MODE=0 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DATA_TYPE=cint16 DDS_PHASE_INC=0xB3092845 UUT_SSR=2 RESULTS_PATH=./results/ssr2cint16/"
    #test_arr[$LINENO]="make all AIE_VARIANT=2 MIXER_MODE=0 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DATA_TYPE=cint16 DDS_PHASE_INC=0xE1387494 UUT_SSR=2 RESULTS_PATH=./results/ssr2cint16/"
    #test_arr[$LINENO]="make all AIE_VARIANT=2 MIXER_MODE=0 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DATA_TYPE=cint16 DDS_PHASE_INC=0x8AAAAAAA UUT_SSR=2 RESULTS_PATH=./results/ssr2cint16/"
    #test_arr[$LINENO]="make all AIE_VARIANT=2 MIXER_MODE=0 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DATA_TYPE=cint16 DDS_PHASE_INC=0xC2012239 UUT_SSR=2 RESULTS_PATH=./results/ssr2cint16/"
    #test_arr[$LINENO]="make all AIE_VARIANT=2 MIXER_MODE=0 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DATA_TYPE=cint16 DDS_PHASE_INC=0xA888A888 UUT_SSR=2 RESULTS_PATH=./results/ssr2cint16/"
    #test_arr[$LINENO]="make all AIE_VARIANT=2 MIXER_MODE=0 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DATA_TYPE=cint16 DDS_PHASE_INC=0xD6555555 UUT_SSR=2 RESULTS_PATH=./results/ssr2cint16/"
#
    #test_arr[$LINENO]="make all AIE_VARIANT=2 MIXER_MODE=0 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DATA_TYPE=cint16 DDS_PHASE_INC=0xC6588855 UUT_SSR=3 RESULTS_PATH=./results/ssr3cint16/"
    #test_arr[$LINENO]="make all AIE_VARIANT=2 MIXER_MODE=0 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DATA_TYPE=cint16 DDS_PHASE_INC=0xB8000000 UUT_SSR=3 RESULTS_PATH=./results/ssr3cint16/"
    #test_arr[$LINENO]="make all AIE_VARIANT=2 MIXER_MODE=0 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DATA_TYPE=cint16 DDS_PHASE_INC=0xE4022000 UUT_SSR=3 RESULTS_PATH=./results/ssr3cint16/"
    #test_arr[$LINENO]="make all AIE_VARIANT=2 MIXER_MODE=0 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DATA_TYPE=cint16 DDS_PHASE_INC=0x9AAABBBB UUT_SSR=3 RESULTS_PATH=./results/ssr3cint16/"
    #test_arr[$LINENO]="make all AIE_VARIANT=2 MIXER_MODE=0 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DATA_TYPE=cint16 DDS_PHASE_INC=0xB3092845 UUT_SSR=3 RESULTS_PATH=./results/ssr3cint16/"
    #test_arr[$LINENO]="make all AIE_VARIANT=2 MIXER_MODE=0 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DATA_TYPE=cint16 DDS_PHASE_INC=0xE1387494 UUT_SSR=3 RESULTS_PATH=./results/ssr3cint16/"
    #test_arr[$LINENO]="make all AIE_VARIANT=2 MIXER_MODE=0 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DATA_TYPE=cint16 DDS_PHASE_INC=0x8AAAAAAA UUT_SSR=3 RESULTS_PATH=./results/ssr3cint16/"
    #test_arr[$LINENO]="make all AIE_VARIANT=2 MIXER_MODE=0 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DATA_TYPE=cint16 DDS_PHASE_INC=0xC2012239 UUT_SSR=3 RESULTS_PATH=./results/ssr3cint16/"
    #test_arr[$LINENO]="make all AIE_VARIANT=2 MIXER_MODE=0 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DATA_TYPE=cint16 DDS_PHASE_INC=0xA888A888 UUT_SSR=3 RESULTS_PATH=./results/ssr3cint16/"
    #test_arr[$LINENO]="make all AIE_VARIANT=2 MIXER_MODE=0 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DATA_TYPE=cint16 DDS_PHASE_INC=0xD6555555 UUT_SSR=3 RESULTS_PATH=./results/ssr3cint16/"
#
    #test_arr[$LINENO]="make all AIE_VARIANT=2 MIXER_MODE=0 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DATA_TYPE=cint16 DDS_PHASE_INC=0xC6588855 UUT_SSR=4 RESULTS_PATH=./results/ssr4cint16/"
    #test_arr[$LINENO]="make all AIE_VARIANT=2 MIXER_MODE=0 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DATA_TYPE=cint16 DDS_PHASE_INC=0xB8000000 UUT_SSR=4 RESULTS_PATH=./results/ssr4cint16/"
    #test_arr[$LINENO]="make all AIE_VARIANT=2 MIXER_MODE=0 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DATA_TYPE=cint16 DDS_PHASE_INC=0xE4022000 UUT_SSR=4 RESULTS_PATH=./results/ssr4cint16/"
    #test_arr[$LINENO]="make all AIE_VARIANT=2 MIXER_MODE=0 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DATA_TYPE=cint16 DDS_PHASE_INC=0x9AAABBBB UUT_SSR=4 RESULTS_PATH=./results/ssr4cint16/"
    #test_arr[$LINENO]="make all AIE_VARIANT=2 MIXER_MODE=0 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DATA_TYPE=cint16 DDS_PHASE_INC=0xB3092845 UUT_SSR=4 RESULTS_PATH=./results/ssr4cint16/"
    #test_arr[$LINENO]="make all AIE_VARIANT=2 MIXER_MODE=0 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DATA_TYPE=cint16 DDS_PHASE_INC=0xE1387494 UUT_SSR=4 RESULTS_PATH=./results/ssr4cint16/"
    #test_arr[$LINENO]="make all AIE_VARIANT=2 MIXER_MODE=0 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DATA_TYPE=cint16 DDS_PHASE_INC=0x8AAAAAAA UUT_SSR=4 RESULTS_PATH=./results/ssr4cint16/"
    #test_arr[$LINENO]="make all AIE_VARIANT=2 MIXER_MODE=0 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DATA_TYPE=cint16 DDS_PHASE_INC=0xC2012239 UUT_SSR=4 RESULTS_PATH=./results/ssr4cint16/"
    #test_arr[$LINENO]="make all AIE_VARIANT=2 MIXER_MODE=0 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DATA_TYPE=cint16 DDS_PHASE_INC=0xA888A888 UUT_SSR=4 RESULTS_PATH=./results/ssr4cint16/"
    #test_arr[$LINENO]="make all AIE_VARIANT=2 MIXER_MODE=0 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DATA_TYPE=cint16 DDS_PHASE_INC=0xD6555555 UUT_SSR=4 RESULTS_PATH=./results/ssr4cint16/"
fi

if [[  "$*" == *validate*  ]]
then
    #AIE2
    test_arr[$LINENO]="make ref AIE_VARIANT=2 MIXER_MODE=0 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DDS_PHASE_INC=0xC6588855 RESULTS_PATH=./results/validate_mode0/"
    test_arr[$LINENO]="make ref AIE_VARIANT=2 MIXER_MODE=0 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DDS_PHASE_INC=0xB8000000 RESULTS_PATH=./results/validate_mode0/"
    test_arr[$LINENO]="make ref AIE_VARIANT=2 MIXER_MODE=0 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DDS_PHASE_INC=0xE4022000 RESULTS_PATH=./results/validate_mode0/"
    test_arr[$LINENO]="make ref AIE_VARIANT=2 MIXER_MODE=0 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DDS_PHASE_INC=0x9AAABBBB RESULTS_PATH=./results/validate_mode0/"
    test_arr[$LINENO]="make ref AIE_VARIANT=2 MIXER_MODE=0 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DDS_PHASE_INC=0xB3092845 RESULTS_PATH=./results/validate_mode0/"
    test_arr[$LINENO]="make ref AIE_VARIANT=2 MIXER_MODE=0 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DDS_PHASE_INC=0xE1387494 RESULTS_PATH=./results/validate_mode0/"
    test_arr[$LINENO]="make ref AIE_VARIANT=2 MIXER_MODE=0 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DDS_PHASE_INC=0x8AAAAAAA RESULTS_PATH=./results/validate_mode0/"
    test_arr[$LINENO]="make ref AIE_VARIANT=2 MIXER_MODE=0 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DDS_PHASE_INC=0xC2012239 RESULTS_PATH=./results/validate_mode0/"
    test_arr[$LINENO]="make ref AIE_VARIANT=2 MIXER_MODE=0 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DDS_PHASE_INC=0xA888A888 RESULTS_PATH=./results/validate_mode0/"
    test_arr[$LINENO]="make ref AIE_VARIANT=2 MIXER_MODE=0 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DDS_PHASE_INC=0xD6555555 RESULTS_PATH=./results/validate_mode0/"

    test_arr[$LINENO]="make ref AIE_VARIANT=2 MIXER_MODE=1 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DDS_PHASE_INC=0xC7588855 RESULTS_PATH=./results/validate_mode1/"
    test_arr[$LINENO]="make ref AIE_VARIANT=2 MIXER_MODE=1 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DDS_PHASE_INC=0xB5000000 RESULTS_PATH=./results/validate_mode1/"
    test_arr[$LINENO]="make ref AIE_VARIANT=2 MIXER_MODE=1 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DDS_PHASE_INC=0xE8022000 RESULTS_PATH=./results/validate_mode1/"
    test_arr[$LINENO]="make ref AIE_VARIANT=2 MIXER_MODE=1 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DDS_PHASE_INC=0x93AABBBB RESULTS_PATH=./results/validate_mode1/"
    test_arr[$LINENO]="make ref AIE_VARIANT=2 MIXER_MODE=1 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DDS_PHASE_INC=0xB5092845 RESULTS_PATH=./results/validate_mode1/"
    test_arr[$LINENO]="make ref AIE_VARIANT=2 MIXER_MODE=1 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DDS_PHASE_INC=0xE9387494 RESULTS_PATH=./results/validate_mode1/"
    test_arr[$LINENO]="make ref AIE_VARIANT=2 MIXER_MODE=1 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DDS_PHASE_INC=0x8BAAAAAA RESULTS_PATH=./results/validate_mode1/"
    test_arr[$LINENO]="make ref AIE_VARIANT=2 MIXER_MODE=1 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DDS_PHASE_INC=0xCC012239 RESULTS_PATH=./results/validate_mode1/"
    test_arr[$LINENO]="make ref AIE_VARIANT=2 MIXER_MODE=1 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DDS_PHASE_INC=0xAD88A888 RESULTS_PATH=./results/validate_mode1/"
    test_arr[$LINENO]="make ref AIE_VARIANT=2 MIXER_MODE=1 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DDS_PHASE_INC=0xD2555555 RESULTS_PATH=./results/validate_mode1/"

    test_arr[$LINENO]="make ref AIE_VARIANT=2 MIXER_MODE=2 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DDS_PHASE_INC=0xC6488855 RESULTS_PATH=./results/validate_mode2/"
    test_arr[$LINENO]="make ref AIE_VARIANT=2 MIXER_MODE=2 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DDS_PHASE_INC=0xB8500000 RESULTS_PATH=./results/validate_mode2/"
    test_arr[$LINENO]="make ref AIE_VARIANT=2 MIXER_MODE=2 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DDS_PHASE_INC=0xE4622000 RESULTS_PATH=./results/validate_mode2/"
    test_arr[$LINENO]="make ref AIE_VARIANT=2 MIXER_MODE=2 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DDS_PHASE_INC=0x9A7ABBBB RESULTS_PATH=./results/validate_mode2/"
    test_arr[$LINENO]="make ref AIE_VARIANT=2 MIXER_MODE=2 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DDS_PHASE_INC=0xB3892845 RESULTS_PATH=./results/validate_mode2/"
    test_arr[$LINENO]="make ref AIE_VARIANT=2 MIXER_MODE=2 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DDS_PHASE_INC=0xE1987494 RESULTS_PATH=./results/validate_mode2/"
    test_arr[$LINENO]="make ref AIE_VARIANT=2 MIXER_MODE=2 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DDS_PHASE_INC=0x8BAAAAAA RESULTS_PATH=./results/validate_mode2/"
    test_arr[$LINENO]="make ref AIE_VARIANT=2 MIXER_MODE=2 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DDS_PHASE_INC=0xC2B12239 RESULTS_PATH=./results/validate_mode2/"
    test_arr[$LINENO]="make ref AIE_VARIANT=2 MIXER_MODE=2 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DDS_PHASE_INC=0xA8E8A888 RESULTS_PATH=./results/validate_mode2/"
    test_arr[$LINENO]="make ref AIE_VARIANT=2 MIXER_MODE=2 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DDS_PHASE_INC=0xD6F55555 RESULTS_PATH=./results/validate_mode2/"

    #AIE1
    test_arr[$LINENO]="make ref AIE_VARIANT=1 MIXER_MODE=0 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DDS_PHASE_INC=0xC6588855 RESULTS_PATH=./results/validate_mode3/"
    test_arr[$LINENO]="make ref AIE_VARIANT=1 MIXER_MODE=0 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DDS_PHASE_INC=0xB8000000 RESULTS_PATH=./results/validate_mode3/"
    test_arr[$LINENO]="make ref AIE_VARIANT=1 MIXER_MODE=0 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DDS_PHASE_INC=0xE4022000 RESULTS_PATH=./results/validate_mode3/"
    test_arr[$LINENO]="make ref AIE_VARIANT=1 MIXER_MODE=0 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DDS_PHASE_INC=0x9AAABBBB RESULTS_PATH=./results/validate_mode3/"
    test_arr[$LINENO]="make ref AIE_VARIANT=1 MIXER_MODE=0 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DDS_PHASE_INC=0xB3092845 RESULTS_PATH=./results/validate_mode3/"
    test_arr[$LINENO]="make ref AIE_VARIANT=1 MIXER_MODE=0 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DDS_PHASE_INC=0xE1387494 RESULTS_PATH=./results/validate_mode3/"
    test_arr[$LINENO]="make ref AIE_VARIANT=1 MIXER_MODE=0 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DDS_PHASE_INC=0x8AAAAAAA RESULTS_PATH=./results/validate_mode3/"
    test_arr[$LINENO]="make ref AIE_VARIANT=1 MIXER_MODE=0 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DDS_PHASE_INC=0xC2012239 RESULTS_PATH=./results/validate_mode3/"
    test_arr[$LINENO]="make ref AIE_VARIANT=1 MIXER_MODE=0 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DDS_PHASE_INC=0xA888A888 RESULTS_PATH=./results/validate_mode3/"
    test_arr[$LINENO]="make ref AIE_VARIANT=1 MIXER_MODE=0 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DDS_PHASE_INC=0xD6555555 RESULTS_PATH=./results/validate_mode3/"

    test_arr[$LINENO]="make ref AIE_VARIANT=1 MIXER_MODE=1 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DDS_PHASE_INC=0xC7588855 RESULTS_PATH=./results/validate_mode4/"
    test_arr[$LINENO]="make ref AIE_VARIANT=1 MIXER_MODE=1 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DDS_PHASE_INC=0xB5000000 RESULTS_PATH=./results/validate_mode4/"
    test_arr[$LINENO]="make ref AIE_VARIANT=1 MIXER_MODE=1 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DDS_PHASE_INC=0xE8022000 RESULTS_PATH=./results/validate_mode4/"
    test_arr[$LINENO]="make ref AIE_VARIANT=1 MIXER_MODE=1 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DDS_PHASE_INC=0x93AABBBB RESULTS_PATH=./results/validate_mode4/"
    test_arr[$LINENO]="make ref AIE_VARIANT=1 MIXER_MODE=1 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DDS_PHASE_INC=0xB5092845 RESULTS_PATH=./results/validate_mode4/"
    test_arr[$LINENO]="make ref AIE_VARIANT=1 MIXER_MODE=1 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DDS_PHASE_INC=0xE9387494 RESULTS_PATH=./results/validate_mode4/"
    test_arr[$LINENO]="make ref AIE_VARIANT=1 MIXER_MODE=1 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DDS_PHASE_INC=0x8BAAAAAA RESULTS_PATH=./results/validate_mode4/"
    test_arr[$LINENO]="make ref AIE_VARIANT=1 MIXER_MODE=1 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DDS_PHASE_INC=0xCC012239 RESULTS_PATH=./results/validate_mode4/"
    test_arr[$LINENO]="make ref AIE_VARIANT=1 MIXER_MODE=1 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DDS_PHASE_INC=0xAD88A888 RESULTS_PATH=./results/validate_mode4/"
    test_arr[$LINENO]="make ref AIE_VARIANT=1 MIXER_MODE=1 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DDS_PHASE_INC=0xD2555555 RESULTS_PATH=./results/validate_mode4/"

    test_arr[$LINENO]="make ref AIE_VARIANT=1 MIXER_MODE=2 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DDS_PHASE_INC=0xC6488855 RESULTS_PATH=./results/validate_mode5/"
    test_arr[$LINENO]="make ref AIE_VARIANT=1 MIXER_MODE=2 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DDS_PHASE_INC=0xB8500000 RESULTS_PATH=./results/validate_mode5/"
    test_arr[$LINENO]="make ref AIE_VARIANT=1 MIXER_MODE=2 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DDS_PHASE_INC=0xE4622000 RESULTS_PATH=./results/validate_mode5/"
    test_arr[$LINENO]="make ref AIE_VARIANT=1 MIXER_MODE=2 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DDS_PHASE_INC=0x9A7ABBBB RESULTS_PATH=./results/validate_mode5/"
    test_arr[$LINENO]="make ref AIE_VARIANT=1 MIXER_MODE=2 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DDS_PHASE_INC=0xB3892845 RESULTS_PATH=./results/validate_mode5/"
    test_arr[$LINENO]="make ref AIE_VARIANT=1 MIXER_MODE=2 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DDS_PHASE_INC=0xE1987494 RESULTS_PATH=./results/validate_mode5/"
    test_arr[$LINENO]="make ref AIE_VARIANT=1 MIXER_MODE=2 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DDS_PHASE_INC=0x8BAAAAAA RESULTS_PATH=./results/validate_mode5/"
    test_arr[$LINENO]="make ref AIE_VARIANT=1 MIXER_MODE=2 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DDS_PHASE_INC=0xC2B12239 RESULTS_PATH=./results/validate_mode5/"
    test_arr[$LINENO]="make ref AIE_VARIANT=1 MIXER_MODE=2 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DDS_PHASE_INC=0xA8E8A888 RESULTS_PATH=./results/validate_mode5/"
    test_arr[$LINENO]="make ref AIE_VARIANT=1 MIXER_MODE=2 NITER=2 TARGET=x86sim INPUT_WINDOW_VSIZE=256 DDS_PHASE_INC=0xD6F55555 RESULTS_PATH=./results/validate_mode5/"
fi
# additional option for debug - using the data that's already checked in and matches commslib testing data.
if [[  "$*" == *debugCommStim* ]]
then
    ## CommsLib Equivalent
    # mixer
    test_arr[$LINENO]="make all DATA_TYPE=cint16 INPUT_WINDOW_VSIZE=512 GEN_INPUT_DATA=false MIXER_MODE=2 "
    # Use predefined input data from files
    # Note: STIM_TYPE=1 is NOT USED, as GEN_INPUT_DATA=false over-rides that, but gives another results directory
    # Add RESULTS_DIR=./results_checkin/results_ANOTHER_DIRECTORY, to change results directory
    test_arr[$LINENO]="make all DATA_TYPE=cint16 GEN_INPUT_DATA=false MIXER_MODE=2 "

fi

if [[  "$*" == *testSSR* ]]
then
    makeTarget="all"
    #SSR >1 requires tolerance. See confluence design notes.
    commonParam="MIXER_MODE=0 TARGET=hw NITER=2 INPUT_WINDOW_VSIZE=64 "
    ## finer granularity
    #test_arr[$LINENO]="make $makeTarget INITIAL_DDS_OFFSET=0 DDS_PHASE_INC=0x6B2AAAAA $commonParam"
    ## coarse granularity - 0 offset
    #test_arr[$LINENO]="make $makeTarget INITIAL_DDS_OFFSET=0 DDS_PHASE_INC=0xD6555555 $commonParam"
    ## coarse granularity - 1/2 sample offset
    #test_arr[$LINENO]="make $makeTarget INITIAL_DDS_OFFSET=0x6B2AAAAA DDS_PHASE_INC=0xD6555555 $commonParam"

    # Even if multiplied values alias around - it still works.
    # finer granularity
    test_arr[$LINENO]="make $makeTarget INITIAL_DDS_OFFSET=0 DDS_PHASE_INC=0xD6555555 $commonParam"
    # coarse granularity - 0 offset
    test_arr[$LINENO]="make $makeTarget INITIAL_DDS_OFFSET=0 DDS_PHASE_INC=0x1ACAAAAA8 $commonParam"
    # coarse granularity - 1/2 sample offset
    test_arr[$LINENO]="make $makeTarget INITIAL_DDS_OFFSET=0xD6555555 DDS_PHASE_INC=0x1ACAAAAA8 $commonParam"
    # actual SSR
    test_arr[$LINENO]="make $makeTarget INITIAL_DDS_OFFSET=0 DDS_PHASE_INC=0xD6555555 UUT_SSR=2 $commonParam"
    # actual SSR - skip first sample
    test_arr[$LINENO]="make $makeTarget INITIAL_DDS_OFFSET=0xD6555555 DDS_PHASE_INC=0xD6555555 UUT_SSR=2 $commonParam"
    # Larger SSR
    test_arr[$LINENO]="make $makeTarget INITIAL_DDS_OFFSET=0 DDS_PHASE_INC=0xD6555555 UUT_SSR=16 $commonParam"
    # Larger SSR with streams - largest should be 24 since there are 24 PLIO connections or something
    test_arr[$LINENO]="make $makeTarget INITIAL_DDS_OFFSET=0 DDS_PHASE_INC=0xD6555555 UUT_SSR=16 P_API=1 $commonParam"

    # mixer configs and weird SSR values
    test_arr[$LINENO]="make $makeTarget MIXER_MODE=1"
    test_arr[$LINENO]="make $makeTarget MIXER_MODE=2"
    test_arr[$LINENO]="make $makeTarget MIXER_MODE=1 UUT_SSR=3 DIFF_MODE=ABS DIFF_TOLERANCE=3"
    test_arr[$LINENO]="make $makeTarget MIXER_MODE=2 UUT_SSR=7 DIFF_MODE=ABS DIFF_TOLERANCE=3"
    test_arr[$LINENO]="make $makeTarget MIXER_MODE=2 UUT_SSR=11 P_API=1 DIFF_MODE=ABS DIFF_TOLERANCE=3"

fi

if [[  "$*" == *PR* ]]
then

    test_arr[$LINENO]="make all DATA_TYPE=cint16 MIXER_MODE=2 P_API=0 UUT_SSR=1 INPUT_WINDOW_VSIZE=256 DUMP_VCD=0"
    test_arr[$LINENO]="make all AIE_VARIANT=1 DATA_TYPE=cint16 MIXER_MODE=0 P_API=1 UUT_SSR=2 DDS_PHASE_INC=0x00555555 INITIAL_DDS_OFFSET=0x00000000 INPUT_WINDOW_VSIZE=4096 USE_PHASE_RELOAD=1"
    test_arr[$LINENO]="make all AIE_VARIANT=1 DATA_TYPE=cint32 MIXER_MODE=1 P_API=0 UUT_SSR=1 DDS_PHASE_INC=0x00555555 INITIAL_DDS_OFFSET=0xD6555555 SFDR=50 INPUT_WINDOW_VSIZE=1024 USE_PHASE_RELOAD=1"

fi

if [[  "$*" == *PHASE* ]]
then
    # test_arr[$LINENO]="make all DATA_TYPE=cint16 MIXER_MODE=2 P_API=0 UUT_SSR=1 INPUT_WINDOW_VSIZE=256 DUMP_VCD=0"
    test_arr[$LINENO]="make all AIE_VARIANT=1 DATA_TYPE=cint16 MIXER_MODE=0 P_API=1 UUT_SSR=1 DDS_PHASE_INC=0x00555555 INITIAL_DDS_OFFSET=0x00000000 INPUT_WINDOW_VSIZE=4096 USE_PHASE_RELOAD=1"
    test_arr[$LINENO]="make all AIE_VARIANT=2 DATA_TYPE=cint16 MIXER_MODE=1 P_API=1 UUT_SSR=1  INPUT_WINDOW_VSIZE=4096 NITER=8 USE_PHASE_RELOAD=1"
    test_arr[$LINENO]="make all AIE_VARIANT=2 DATA_TYPE=cint16 MIXER_MODE=1 P_API=1 UUT_SSR=1  INPUT_WINDOW_VSIZE=8192 NITER=8 USE_PHASE_RELOAD=1"
    test_arr[$LINENO]="make all AIE_VARIANT=2 DATA_TYPE=cint16 MIXER_MODE=1 P_API=1 UUT_SSR=1  INPUT_WINDOW_VSIZE=12288 NITER=8 USE_PHASE_RELOAD=1"
    test_arr[$LINENO]="make all AIE_VARIANT=2 DATA_TYPE=cint16 MIXER_MODE=1 P_API=1 UUT_SSR=1  INPUT_WINDOW_VSIZE=16384 NITER=8 USE_PHASE_RELOAD=1"
    test_arr[$LINENO]="make all AIE_VARIANT=2 DATA_TYPE=cint16 MIXER_MODE=1 P_API=1 UUT_SSR=1  INPUT_WINDOW_VSIZE=20480 NITER=8 USE_PHASE_RELOAD=1"
    test_arr[$LINENO]="make all AIE_VARIANT=2 DATA_TYPE=cint16 MIXER_MODE=1 P_API=1 UUT_SSR=1  INPUT_WINDOW_VSIZE=45056 NITER=8 USE_PHASE_RELOAD=1"

    test_arr[$LINENO]="make all AIE_VARIANT=2 DATA_TYPE=cint16 MIXER_MODE=0 P_API=0 UUT_SSR=1 INPUT_WINDOW_VSIZE=8192 NITER=8 USE_PHASE_RELOAD=1"
    test_arr[$LINENO]="make all AIE_VARIANT=2 DATA_TYPE=cint16 MIXER_MODE=0 P_API=1 UUT_SSR=1 INPUT_WINDOW_VSIZE=8192 NITER=8 USE_PHASE_RELOAD=1"
    test_arr[$LINENO]="make all AIE_VARIANT=2 DATA_TYPE=cint16 MIXER_MODE=1 P_API=0 UUT_SSR=1 INPUT_WINDOW_VSIZE=6656 DDS_PHASE_INC=0xD6555555 INITIAL_DDS_OFFSET=0xD6555555  NITER=8 USE_PHASE_RELOAD=1"

    #type support
    test_arr[$LINENO]="make all AIE_VARIANT=2 DATA_TYPE=cint32 MIXER_MODE=1 P_API=0 UUT_SSR=1  INPUT_WINDOW_VSIZE=64  NITER=8 USE_PHASE_RELOAD=1"
    test_arr[$LINENO]="make all AIE_VARIANT=2 DATA_TYPE=cint32 MIXER_MODE=2 P_API=0 UUT_SSR=1  INPUT_WINDOW_VSIZE=128 NITER=8 USE_PHASE_RELOAD=1"
    test_arr[$LINENO]="make all AIE_VARIANT=2 DATA_TYPE=cint32 MIXER_MODE=1 P_API=1 UUT_SSR=1  INPUT_WINDOW_VSIZE=64  NITER=8 USE_PHASE_RELOAD=1"
    test_arr[$LINENO]="make all AIE_VARIANT=1 DATA_TYPE=cint32 MIXER_MODE=2 P_API=1 UUT_SSR=1  INPUT_WINDOW_VSIZE=64  NITER=8 USE_PHASE_RELOAD=1"
    test_arr[$LINENO]="make all AIE_VARIANT=2 DATA_TYPE=cint32 MIXER_MODE=1 P_API=0 UUT_SSR=1  INPUT_WINDOW_VSIZE=128 DDS_PHASE_INC=0xD6555555 INITIAL_DDS_OFFSET=0xD6555555 NITER=8 USE_PHASE_RELOAD=1"
    test_arr[$LINENO]="make all AIE_VARIANT=2 DATA_TYPE=cint32 MIXER_MODE=1 P_API=0 UUT_SSR=1  INPUT_WINDOW_VSIZE=896 DDS_PHASE_INC=0xD6555555                               NITER=8 USE_PHASE_RELOAD=1"
    test_arr[$LINENO]="make all AIE_VARIANT=2 DATA_TYPE=cint32 MIXER_MODE=1 P_API=0 UUT_SSR=1  INPUT_WINDOW_VSIZE=896 DDS_PHASE_INC=0xD6555555 INITIAL_DDS_OFFSET=0xD6555555 NITER=8 USE_PHASE_RELOAD=1"

    #explore SFDR
    test_arr[$LINENO]="make all AIE_VARIANT=2 DATA_TYPE=cint32 MIXER_MODE=0 P_API=0 UUT_SSR=1  INPUT_WINDOW_VSIZE=64       SFDR=50  NITER=8 USE_PHASE_RELOAD=1"
    test_arr[$LINENO]="make all AIE_VARIANT=2 DATA_TYPE=cint32 MIXER_MODE=0 P_API=0 UUT_SSR=1  INPUT_WINDOW_VSIZE=64       SFDR=90  NITER=8 USE_PHASE_RELOAD=1"
    test_arr[$LINENO]="make all AIE_VARIANT=2 DATA_TYPE=cint32 MIXER_MODE=0 P_API=0 UUT_SSR=1  INPUT_WINDOW_VSIZE=64       SFDR=130 NITER=8 USE_PHASE_RELOAD=1"
    test_arr[$LINENO]="make all AIE_VARIANT=2 DATA_TYPE=cint32 MIXER_MODE=0 P_API=1 UUT_SSR=1  INPUT_WINDOW_VSIZE=64       SFDR=50  NITER=8 USE_PHASE_RELOAD=1"
    test_arr[$LINENO]="make all AIE_VARIANT=2 DATA_TYPE=cint32 MIXER_MODE=0 P_API=1 UUT_SSR=1  INPUT_WINDOW_VSIZE=64       SFDR=90  NITER=8 USE_PHASE_RELOAD=1"
    test_arr[$LINENO]="make all AIE_VARIANT=2 DATA_TYPE=cint32 MIXER_MODE=0 P_API=1 UUT_SSR=1  INPUT_WINDOW_VSIZE=64       SFDR=130 NITER=8 USE_PHASE_RELOAD=1"
    test_arr[$LINENO]="make all AIE_VARIANT=2 DATA_TYPE=cint16 MIXER_MODE=0 P_API=0 UUT_SSR=1  INPUT_WINDOW_VSIZE=512      SFDR=50  NITER=8 USE_PHASE_RELOAD=1"
    test_arr[$LINENO]="make all AIE_VARIANT=2 DATA_TYPE=cint16 MIXER_MODE=0 P_API=0 UUT_SSR=1  INPUT_WINDOW_VSIZE=512      SFDR=90  NITER=8 USE_PHASE_RELOAD=1"
    test_arr[$LINENO]="make all AIE_VARIANT=2 DATA_TYPE=cint16 MIXER_MODE=0 P_API=1 UUT_SSR=1  INPUT_WINDOW_VSIZE=4096     SFDR=50  NITER=8 USE_PHASE_RELOAD=1"
    test_arr[$LINENO]="make all AIE_VARIANT=2 DATA_TYPE=cint16 MIXER_MODE=0 P_API=1 UUT_SSR=1  INPUT_WINDOW_VSIZE=4096     SFDR=90  NITER=8 USE_PHASE_RELOAD=1"

    test_arr[$LINENO]="make all AIE_VARIANT=2 DATA_TYPE=cint32 MIXER_MODE=2 P_API=0 UUT_SSR=1  INPUT_WINDOW_VSIZE=64       SFDR=50  NITER=8 USE_PHASE_RELOAD=1"
    test_arr[$LINENO]="make all AIE_VARIANT=2 DATA_TYPE=cint32 MIXER_MODE=2 P_API=0 UUT_SSR=1  INPUT_WINDOW_VSIZE=64       SFDR=90  NITER=8 USE_PHASE_RELOAD=1"
    test_arr[$LINENO]="make all AIE_VARIANT=2 DATA_TYPE=cint32 MIXER_MODE=2 P_API=0 UUT_SSR=1  INPUT_WINDOW_VSIZE=64       SFDR=130 NITER=8 USE_PHASE_RELOAD=1"
    test_arr[$LINENO]="make all AIE_VARIANT=2 DATA_TYPE=cint32 MIXER_MODE=1 P_API=1 UUT_SSR=1  INPUT_WINDOW_VSIZE=64       SFDR=50  NITER=8 USE_PHASE_RELOAD=1"
    test_arr[$LINENO]="make all AIE_VARIANT=2 DATA_TYPE=cint32 MIXER_MODE=1 P_API=1 UUT_SSR=1  INPUT_WINDOW_VSIZE=64       SFDR=90  NITER=8 USE_PHASE_RELOAD=1"
    test_arr[$LINENO]="make all AIE_VARIANT=2 DATA_TYPE=cint32 MIXER_MODE=1 P_API=1 UUT_SSR=1  INPUT_WINDOW_VSIZE=64       SFDR=130 NITER=8 USE_PHASE_RELOAD=1"
    test_arr[$LINENO]="make all AIE_VARIANT=2 DATA_TYPE=cint16 MIXER_MODE=2 P_API=0 UUT_SSR=1  INPUT_WINDOW_VSIZE=512      SFDR=50  NITER=8 USE_PHASE_RELOAD=1"
    test_arr[$LINENO]="make all AIE_VARIANT=2 DATA_TYPE=cint16 MIXER_MODE=2 P_API=0 UUT_SSR=1  INPUT_WINDOW_VSIZE=512      SFDR=90  NITER=8 USE_PHASE_RELOAD=1"
    test_arr[$LINENO]="make all AIE_VARIANT=2 DATA_TYPE=cint16 MIXER_MODE=1 P_API=1 UUT_SSR=1  INPUT_WINDOW_VSIZE=4096     SFDR=50  NITER=8 USE_PHASE_RELOAD=1"
    test_arr[$LINENO]="make all AIE_VARIANT=2 DATA_TYPE=cint16 MIXER_MODE=1 P_API=1 UUT_SSR=1  INPUT_WINDOW_VSIZE=4096     SFDR=90  NITER=8 USE_PHASE_RELOAD=1"
    # # test_arr[$LINENO]="make all DATA_TYPE=cint16 MIXER_MODE=2 P_API=0 UUT_SSR=1  INPUT_WINDOW_VSIZE=256   NITER=8"
    # test_arr[$LINENO]="make all DATA_TYPE=cint16 MIXER_MODE=2 P_API=0 UUT_SSR=1 INPUT_WINDOW_VSIZE=256 DUMP_VCD=1"
    test_arr[$LINENO]="make all AIE_VARIANT=1 DATA_TYPE=cint16 MIXER_MODE=0 P_API=1 UUT_SSR=2  SFDR=90 INPUT_WINDOW_VSIZE=4096"
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
