#!/bin/bash
#
# Copyright (C) 2019-2022, Xilinx, Inc.
# Copyright (C) 2022-2025, Advanced Micro Devices, Inc.
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
done



test_arr=()


if [[ "$*" == *checkin* ]]
then
    test_arr[${#test_arr[*]}]="make all SSR=5 AIE_VARIANT=1"
    test_arr[${#test_arr[*]}]="make all DATA_TYPE=cint32 TWIDDLE_TYPE=cint32 AIE_VARIANT=1"
    test_arr[${#test_arr[*]}]="make all POINT_SIZE=8192 PARALLEL_POWER=1 AIE_VARIANT=1"
    test_arr[${#test_arr[*]}]="make all DATA_TYPE=cfloat TWIDDLE_TYPE=cfloat SHIFT=0 POINT_SIZE=4096 AIE_VARIANT=1"
    test_arr[${#test_arr[*]}]="make all DATA_TYPE=cint32 TWIDDLE_TYPE=cint32 SHIFT=5 POINT_SIZE=16384 AIE_VARIANT=1"
    test_arr[${#test_arr[*]}]="make all DATA_TYPE=cint32 TWIDDLE_TYPE=cint16 SSR=16 PARALLEL_POWER=2 SHIFT=5 POINT_SIZE=8192 AIE_VARIANT=1"
    test_arr[${#test_arr[*]}]="make all PART=xcve2802-vsvh1760-2MP-e-S AIE_VARIANT=2"
    test_arr[${#test_arr[*]}]="make all PART=xcve2802-vsvh1760-2MP-e-S SSR=5  AIE_PLIO_WIDTH=128 AIE_VARIANT=2"
    test_arr[${#test_arr[*]}]="make all PART=xcve2802-vsvh1760-2MP-e-S POINT_SIZE=32768 AIE_VARIANT=2"
    test_arr[${#test_arr[*]}]="make all DATA_TYPE=cint32 TWIDDLE_TYPE=cint16 SSR=16 PARALLEL_POWER=2 SHIFT=5 POINT_SIZE=8192 AIE_VARIANT=1"
# vss_mode = 2
    test_arr[${#test_arr[*]}]="make all DATA_TYPE=cfloat TWIDDLE_TYPE=cfloat SHIFT=0 POINT_SIZE=4096 AIE_VARIANT=1 SSR=4 VSS_MODE=2"
    test_arr[${#test_arr[*]}]="make all DATA_TYPE=cfloat TWIDDLE_TYPE=cfloat SHIFT=0 POINT_SIZE=8192 AIE_VARIANT=1 SSR=16 VSS_MODE=2"
    test_arr[${#test_arr[*]}]="make all DATA_TYPE=cfloat TWIDDLE_TYPE=cfloat SHIFT=0 POINT_SIZE=16384 AIE_VARIANT=1 SSR=16 VSS_MODE=2"
    test_arr[${#test_arr[*]}]="make all DATA_TYPE=cfloat TWIDDLE_TYPE=cfloat SHIFT=0 POINT_SIZE=32768 AIE_VARIANT=1 SSR=32 VSS_MODE=2 AIE_PLIO_WIDTH=64"
    test_arr[${#test_arr[*]}]="make all DATA_TYPE=cfloat TWIDDLE_TYPE=cfloat SHIFT=0 POINT_SIZE=8192 AIE_VARIANT=1 SSR=64 VSS_MODE=2 AIE_PLIO_WIDTH=64"
    test_arr[${#test_arr[*]}]="make all  PART=xcve2802-vsvh1760-2MP-e-S  DATA_TYPE=cfloat TWIDDLE_TYPE=cfloat SHIFT=0 POINT_SIZE=4096 AIE_VARIANT=2 SSR=4 VSS_MODE=2"
    test_arr[${#test_arr[*]}]="make all  PART=xcve2802-vsvh1760-2MP-e-S  DATA_TYPE=cfloat TWIDDLE_TYPE=cfloat SHIFT=0 POINT_SIZE=16384 AIE_VARIANT=2 SSR=16 VSS_MODE=2"
    test_arr[${#test_arr[*]}]="make all  PART=xcve2802-vsvh1760-2MP-e-S  DATA_TYPE=cfloat TWIDDLE_TYPE=cfloat SHIFT=0 POINT_SIZE=65536 AIE_VARIANT=2 SSR=32 VSS_MODE=2"
fi

if [[ "$*" == *qor* ]]
then
# POINT_SIZE=16i
    test_arr[${#test_arr[*]}]="make all SSR=8"
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
for i in "${!makeCmd_raw[@]}"; do
    echo ${makeCmd_raw[$i]}
done
echo $run_type

if [[ "$*" == *-append* ]]
then
    echo "python3 $runmakeDir/../../aie/common/scripts/populate_params.py $run_type $runmakeDir "${makeCmd_raw[@]}" -append"
    new_test_names=$(python3 $runmakeDir/../../aie/common/scripts/populate_params.py $run_type $runmakeDir "${makeCmd_raw[@]}" -append)
else
    new_test_names=$(python3 $runmakeDir/../../aie/common/scripts/populate_params.py $run_type $runmakeDir "${makeCmd_raw[@]}")
fi
multi_params_file=multi_params_$run_type

if [[ $run_type == "jenkins" ]]
then
    echo "python3 $runmakeDir/../../aie/common/scripts/populate_params.py clear_params $runmakeDir"
    $runmakeDir/runmake.sh -run_type checkin -append -no_test_run
    $runmakeDir/runmake.sh -run_type qor -append -no_test_run
    multi_params_file=multi_params
    run_tests=0
fi

if [[ $run_tests == 1 ]]
then
    timestamp=$(date +"%y%m%d_%H%M")
    echo "Start of Batch Run ($timestamp)" | tee ./runmake_logs/runmake.log
    $runmakeDir/../../aie/common/scripts/run_batch.sh -func $(basename $runmakeDir) -params $multi_params_file -batch_suffix $timestamp
fi
