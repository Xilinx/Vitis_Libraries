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

if [[ "$*" == *single_buf*  || "$*" == *checkin* || "$*" == *qor* ]]
then
    # test_arr[$LINENO]="make all DATA_TYPE=int16  DIM_SIZE=256 NUM_FRAMES=1  ASCENDING=0  CASC_LEN=1 UUT_SSR=1  STIM_TYPE=0  NITER=4 AIE_VARIANT=1 SINGLE_BUF=1"
    # test_arr[$LINENO]="make all DATA_TYPE=int16  DIM_SIZE=256 NUM_FRAMES=1  ASCENDING=0  CASC_LEN=2 UUT_SSR=1  STIM_TYPE=0  NITER=4 AIE_VARIANT=1 SINGLE_BUF=1"
    # test_arr[$LINENO]="make all DATA_TYPE=int16  DIM_SIZE=256 NUM_FRAMES=1  ASCENDING=0  CASC_LEN=1 UUT_SSR=2  STIM_TYPE=0  NITER=4 AIE_VARIANT=1 SINGLE_BUF=1"
    test_arr[$LINENO]="make all DATA_TYPE=int16  DIM_SIZE=256 NUM_FRAMES=1  ASCENDING=0  CASC_LEN=2 UUT_SSR=2  STIM_TYPE=0  NITER=4 AIE_VARIANT=1 SINGLE_BUF=1"
    test_arr[$LINENO]="make all DATA_TYPE=int16  DIM_SIZE=256 NUM_FRAMES=1  ASCENDING=0  CASC_LEN=1 UUT_SSR=4  STIM_TYPE=0  NITER=4 AIE_VARIANT=1 SINGLE_BUF=1"
    # test_arr[$LINENO]="make all DATA_TYPE=int16  DIM_SIZE=256 NUM_FRAMES=1  ASCENDING=0  CASC_LEN=2 UUT_SSR=8  STIM_TYPE=0  NITER=4 AIE_VARIANT=1 SINGLE_BUF=1"
    # test_arr[$LINENO]="make all DATA_TYPE=int16  DIM_SIZE=512 NUM_FRAMES=1  ASCENDING=0  CASC_LEN=1 UUT_SSR=16 STIM_TYPE=0  NITER=4 AIE_VARIANT=1 SINGLE_BUF=1"
fi

if [[ "$*" == *qor* ]]
then
    niters=8
else
    niters=4
fi
# if [[ "$*" == *aie2* ]]
# then
#     aie_variant=2
# else
#     aie_variant=1
# fi


if [[  "$*" == *datatypes* || "$*" == *checkin* || "$*" == *qor* ]]
then
    # testing all valid datatype combinations...
    # test_arr[$LINENO]="make all DATA_TYPE=int8   DIM_SIZE=32  NUM_FRAMES=1  ASCENDING=1  CASC_LEN=1  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=1"
    # test_arr[$LINENO]="make all DATA_TYPE=uint8  DIM_SIZE=32  NUM_FRAMES=1  ASCENDING=1  CASC_LEN=1  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=1"
    test_arr[$LINENO]="make all DATA_TYPE=int16  DIM_SIZE=32  NUM_FRAMES=1  ASCENDING=1  CASC_LEN=1  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=1"
    # test_arr[$LINENO]="make all DATA_TYPE=uint16 DIM_SIZE=32  NUM_FRAMES=1  ASCENDING=1  CASC_LEN=1  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=1"
    test_arr[$LINENO]="make all DATA_TYPE=int32  DIM_SIZE=32  NUM_FRAMES=1  ASCENDING=1  CASC_LEN=1  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=1"
    test_arr[$LINENO]="make all DATA_TYPE=float  DIM_SIZE=32  NUM_FRAMES=1  ASCENDING=1  CASC_LEN=1  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=1"

    test_arr[$LINENO]="make all DATA_TYPE=int16  DIM_SIZE=32  NUM_FRAMES=1  ASCENDING=1  CASC_LEN=1  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=2"
    test_arr[$LINENO]="make all DATA_TYPE=uint16 DIM_SIZE=32  NUM_FRAMES=1  ASCENDING=1  CASC_LEN=1  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=2"
    test_arr[$LINENO]="make all DATA_TYPE=int32  DIM_SIZE=32  NUM_FRAMES=1  ASCENDING=1  CASC_LEN=1  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=2"
    test_arr[$LINENO]="make all DATA_TYPE=float  DIM_SIZE=32  NUM_FRAMES=1  ASCENDING=1  CASC_LEN=1  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=2"

    test_arr[$LINENO]="make all DATA_TYPE=int16  DIM_SIZE=32  NUM_FRAMES=1  ASCENDING=1  CASC_LEN=1  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=22"
    test_arr[$LINENO]="make all DATA_TYPE=uint16 DIM_SIZE=32  NUM_FRAMES=1  ASCENDING=1  CASC_LEN=1  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=22"
    test_arr[$LINENO]="make all DATA_TYPE=int32  DIM_SIZE=32  NUM_FRAMES=1  ASCENDING=1  CASC_LEN=1  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=22"
    test_arr[$LINENO]="make all DATA_TYPE=float  DIM_SIZE=32  NUM_FRAMES=1  ASCENDING=1  CASC_LEN=1  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=22"
fi

# if [[  "$*" == *vec_is_dim* || "$*" == *checkin* || "$*" == *qor* ]]
# then
    # test_arr[$LINENO]="make all DATA_TYPE=int8   DIM_SIZE=32  NUM_FRAMES=1  ASCENDING=1  CASC_LEN=1  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=1"
    # test_arr[$LINENO]="make all DATA_TYPE=uint8  DIM_SIZE=32  NUM_FRAMES=1  ASCENDING=1  CASC_LEN=1  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=1"
    # test_arr[$LINENO]="make all DATA_TYPE=int16  DIM_SIZE=16  NUM_FRAMES=1  ASCENDING=1  CASC_LEN=1  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=1"
    # test_arr[$LINENO]="make all DATA_TYPE=uint16 DIM_SIZE=16  NUM_FRAMES=1  ASCENDING=1  CASC_LEN=1  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=1" # not valid type for aie vector. ^ maybe related?
    # test_arr[$LINENO]="make all DATA_TYPE=int32  DIM_SIZE=8   NUM_FRAMES=1  ASCENDING=1  CASC_LEN=1  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=1"
    # test_arr[$LINENO]="make all DATA_TYPE=float  DIM_SIZE=8   NUM_FRAMES=1  ASCENDING=1  CASC_LEN=1  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=1"

    # test_arr[$LINENO]="make all DATA_TYPE=int16  DIM_SIZE=16  NUM_FRAMES=1  ASCENDING=1  CASC_LEN=1  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=2"
    # test_arr[$LINENO]="make all DATA_TYPE=uint16 DIM_SIZE=16  NUM_FRAMES=1  ASCENDING=1  CASC_LEN=1  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=2"
    # test_arr[$LINENO]="make all DATA_TYPE=int32  DIM_SIZE=8   NUM_FRAMES=1  ASCENDING=1  CASC_LEN=1  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=2"
    # test_arr[$LINENO]="make all DATA_TYPE=float  DIM_SIZE=8   NUM_FRAMES=1  ASCENDING=1  CASC_LEN=1  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=2"
# fi

if [[ "$*" == *num_frames* || "$*" == *checkin* || "$*" == *qor* ]]
then
    # testing range of num_frames...
    test_arr[$LINENO]="make all DATA_TYPE=int32  DIM_SIZE=32  NUM_FRAMES=2  ASCENDING=1  CASC_LEN=1  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=1"
    test_arr[$LINENO]="make all DATA_TYPE=int32  DIM_SIZE=32  NUM_FRAMES=4  ASCENDING=1  CASC_LEN=1  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=1"
    test_arr[$LINENO]="make all DATA_TYPE=int32  DIM_SIZE=32  NUM_FRAMES=8  ASCENDING=1  CASC_LEN=1  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=1"

    test_arr[$LINENO]="make all DATA_TYPE=int32  DIM_SIZE=32  NUM_FRAMES=2  ASCENDING=1  CASC_LEN=1  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=2"
    test_arr[$LINENO]="make all DATA_TYPE=int32  DIM_SIZE=32  NUM_FRAMES=4  ASCENDING=1  CASC_LEN=1  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=2"
    test_arr[$LINENO]="make all DATA_TYPE=int32  DIM_SIZE=32  NUM_FRAMES=8  ASCENDING=1  CASC_LEN=1  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=2"

    test_arr[$LINENO]="make all DATA_TYPE=int32  DIM_SIZE=32  NUM_FRAMES=2  ASCENDING=1  CASC_LEN=1  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=22"
    test_arr[$LINENO]="make all DATA_TYPE=int32  DIM_SIZE=32  NUM_FRAMES=4  ASCENDING=1  CASC_LEN=1  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=22"
    test_arr[$LINENO]="make all DATA_TYPE=int32  DIM_SIZE=32  NUM_FRAMES=8  ASCENDING=1  CASC_LEN=1  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=22"

    if [[ "$*" == *num_frames* ]]
    then
        test_arr[$LINENO]="make all DATA_TYPE=int32  DIM_SIZE=32  NUM_FRAMES=16  ASCENDING=1  CASC_LEN=1  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=1"
        test_arr[$LINENO]="make all DATA_TYPE=int32  DIM_SIZE=32  NUM_FRAMES=32  ASCENDING=1  CASC_LEN=1  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=1"
        test_arr[$LINENO]="make all DATA_TYPE=int32  DIM_SIZE=32  NUM_FRAMES=64  ASCENDING=1  CASC_LEN=1  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=1"
        test_arr[$LINENO]="make all DATA_TYPE=int32  DIM_SIZE=32  NUM_FRAMES=128 ASCENDING=1  CASC_LEN=1  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=1"
        test_arr[$LINENO]="make all DATA_TYPE=int32  DIM_SIZE=32  NUM_FRAMES=256 ASCENDING=1  CASC_LEN=1  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=1"
    fi
fi

if [[ "$*" == *float_stress_test* ]]
then
    test_arr[$LINENO]="make all DATA_TYPE=float  DIM_SIZE=16    NUM_FRAMES=1  ASCENDING=1  CASC_LEN=1  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=1"
    test_arr[$LINENO]="make all DATA_TYPE=float  DIM_SIZE=32    NUM_FRAMES=1  ASCENDING=1  CASC_LEN=1  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=1"
    test_arr[$LINENO]="make all DATA_TYPE=float  DIM_SIZE=64    NUM_FRAMES=1  ASCENDING=1  CASC_LEN=1  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=1"
    test_arr[$LINENO]="make all DATA_TYPE=float  DIM_SIZE=128   NUM_FRAMES=1  ASCENDING=1  CASC_LEN=1  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=1"
    test_arr[$LINENO]="make all DATA_TYPE=float  DIM_SIZE=256   NUM_FRAMES=1  ASCENDING=1  CASC_LEN=1  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=1"
    test_arr[$LINENO]="make all DATA_TYPE=float  DIM_SIZE=512   NUM_FRAMES=1  ASCENDING=1  CASC_LEN=1  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=1"
    test_arr[$LINENO]="make all DATA_TYPE=float  DIM_SIZE=1024  NUM_FRAMES=1  ASCENDING=1  CASC_LEN=1  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=1"
    test_arr[$LINENO]="make all DATA_TYPE=float  DIM_SIZE=2048  NUM_FRAMES=1  ASCENDING=1  CASC_LEN=1  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=1"
    test_arr[$LINENO]="make all DATA_TYPE=float  DIM_SIZE=4096  NUM_FRAMES=1  ASCENDING=1  CASC_LEN=1  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=1"
fi

if [[ "$*" == *int16_stress_test* ]]
then
    test_arr[$LINENO]="make all DATA_TYPE=int16  DIM_SIZE=16    NUM_FRAMES=1  ASCENDING=1  CASC_LEN=1  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=1"
    test_arr[$LINENO]="make all DATA_TYPE=int16  DIM_SIZE=32    NUM_FRAMES=1  ASCENDING=1  CASC_LEN=1  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=1"
    test_arr[$LINENO]="make all DATA_TYPE=int16  DIM_SIZE=64    NUM_FRAMES=1  ASCENDING=1  CASC_LEN=1  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=1"
    test_arr[$LINENO]="make all DATA_TYPE=int16  DIM_SIZE=128   NUM_FRAMES=1  ASCENDING=1  CASC_LEN=1  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=1"
    test_arr[$LINENO]="make all DATA_TYPE=int16  DIM_SIZE=256   NUM_FRAMES=1  ASCENDING=1  CASC_LEN=1  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=1"
    test_arr[$LINENO]="make all DATA_TYPE=int16  DIM_SIZE=512   NUM_FRAMES=1  ASCENDING=1  CASC_LEN=1  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=1"
    test_arr[$LINENO]="make all DATA_TYPE=int16  DIM_SIZE=1024  NUM_FRAMES=1  ASCENDING=1  CASC_LEN=1  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=1"
    test_arr[$LINENO]="make all DATA_TYPE=int16  DIM_SIZE=2048  NUM_FRAMES=1  ASCENDING=1  CASC_LEN=1  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=1"
    test_arr[$LINENO]="make all DATA_TYPE=int16  DIM_SIZE=4096  NUM_FRAMES=1  ASCENDING=1  CASC_LEN=1  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=1"
fi

if [[ "$*" == *list_dimension* || "$*" == *checkin* || "$*" == *qor* ]]
then
    test_arr[$LINENO]="make all DATA_TYPE=float  DIM_SIZE=16    NUM_FRAMES=1  ASCENDING=1  CASC_LEN=1  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=1"
    test_arr[$LINENO]="make all DATA_TYPE=float  DIM_SIZE=32    NUM_FRAMES=1  ASCENDING=1  CASC_LEN=1  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=1"
    test_arr[$LINENO]="make all DATA_TYPE=float  DIM_SIZE=64    NUM_FRAMES=1  ASCENDING=1  CASC_LEN=1  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=1"
    test_arr[$LINENO]="make all DATA_TYPE=float  DIM_SIZE=128   NUM_FRAMES=1  ASCENDING=1  CASC_LEN=1  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=1"
    test_arr[$LINENO]="make all DATA_TYPE=float  DIM_SIZE=256   NUM_FRAMES=1  ASCENDING=1  CASC_LEN=1  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=1"

    test_arr[$LINENO]="make all DATA_TYPE=int16  DIM_SIZE=32    NUM_FRAMES=1  ASCENDING=1  CASC_LEN=1  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=1"
    test_arr[$LINENO]="make all DATA_TYPE=int16  DIM_SIZE=64    NUM_FRAMES=1  ASCENDING=1  CASC_LEN=1  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=1"
    test_arr[$LINENO]="make all DATA_TYPE=int16  DIM_SIZE=128   NUM_FRAMES=1  ASCENDING=1  CASC_LEN=1  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=1"
    test_arr[$LINENO]="make all DATA_TYPE=int16  DIM_SIZE=256   NUM_FRAMES=1  ASCENDING=1  CASC_LEN=1  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=1"
    test_arr[$LINENO]="make all DATA_TYPE=int16  DIM_SIZE=512   NUM_FRAMES=1  ASCENDING=1  CASC_LEN=1  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=1"

    test_arr[$LINENO]="make all DATA_TYPE=float  DIM_SIZE=16    NUM_FRAMES=1  ASCENDING=1  CASC_LEN=1  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=2"
    test_arr[$LINENO]="make all DATA_TYPE=float  DIM_SIZE=32    NUM_FRAMES=1  ASCENDING=1  CASC_LEN=1  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=2"
    test_arr[$LINENO]="make all DATA_TYPE=float  DIM_SIZE=64    NUM_FRAMES=1  ASCENDING=1  CASC_LEN=1  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=2"
    test_arr[$LINENO]="make all DATA_TYPE=float  DIM_SIZE=128   NUM_FRAMES=1  ASCENDING=1  CASC_LEN=1  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=2"
    test_arr[$LINENO]="make all DATA_TYPE=float  DIM_SIZE=256   NUM_FRAMES=1  ASCENDING=1  CASC_LEN=1  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=2"

    test_arr[$LINENO]="make all DATA_TYPE=int16  DIM_SIZE=32    NUM_FRAMES=1  ASCENDING=1  CASC_LEN=1  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=2"
    test_arr[$LINENO]="make all DATA_TYPE=int16  DIM_SIZE=64    NUM_FRAMES=1  ASCENDING=1  CASC_LEN=1  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=2"
    test_arr[$LINENO]="make all DATA_TYPE=int16  DIM_SIZE=128   NUM_FRAMES=1  ASCENDING=1  CASC_LEN=1  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=2"
    test_arr[$LINENO]="make all DATA_TYPE=int16  DIM_SIZE=256   NUM_FRAMES=1  ASCENDING=1  CASC_LEN=1  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=2"
    test_arr[$LINENO]="make all DATA_TYPE=int16  DIM_SIZE=512   NUM_FRAMES=1  ASCENDING=1  CASC_LEN=1  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=2"

    test_arr[$LINENO]="make all DATA_TYPE=uint16  DIM_SIZE=32    NUM_FRAMES=1  ASCENDING=1  CASC_LEN=1  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=2"
    test_arr[$LINENO]="make all DATA_TYPE=uint16  DIM_SIZE=64    NUM_FRAMES=1  ASCENDING=1  CASC_LEN=1  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=2"
    test_arr[$LINENO]="make all DATA_TYPE=uint16  DIM_SIZE=128   NUM_FRAMES=1  ASCENDING=1  CASC_LEN=1  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=2"
    test_arr[$LINENO]="make all DATA_TYPE=uint16  DIM_SIZE=256   NUM_FRAMES=1  ASCENDING=1  CASC_LEN=1  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=2"
    test_arr[$LINENO]="make all DATA_TYPE=uint16  DIM_SIZE=512   NUM_FRAMES=1  ASCENDING=1  CASC_LEN=1  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=2"

    test_arr[$LINENO]="make all DATA_TYPE=float  DIM_SIZE=16    NUM_FRAMES=1  ASCENDING=1  CASC_LEN=1  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=22"
    test_arr[$LINENO]="make all DATA_TYPE=float  DIM_SIZE=32    NUM_FRAMES=1  ASCENDING=1  CASC_LEN=1  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=22"
    test_arr[$LINENO]="make all DATA_TYPE=float  DIM_SIZE=64    NUM_FRAMES=1  ASCENDING=1  CASC_LEN=1  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=22"
    test_arr[$LINENO]="make all DATA_TYPE=float  DIM_SIZE=128   NUM_FRAMES=1  ASCENDING=1  CASC_LEN=1  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=22"
    test_arr[$LINENO]="make all DATA_TYPE=float  DIM_SIZE=256   NUM_FRAMES=1  ASCENDING=1  CASC_LEN=1  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=22"

    test_arr[$LINENO]="make all DATA_TYPE=int16  DIM_SIZE=32    NUM_FRAMES=1  ASCENDING=1  CASC_LEN=1  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=22"
    test_arr[$LINENO]="make all DATA_TYPE=int16  DIM_SIZE=64    NUM_FRAMES=1  ASCENDING=1  CASC_LEN=1  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=22"
    test_arr[$LINENO]="make all DATA_TYPE=int16  DIM_SIZE=128   NUM_FRAMES=1  ASCENDING=1  CASC_LEN=1  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=22"
    test_arr[$LINENO]="make all DATA_TYPE=int16  DIM_SIZE=256   NUM_FRAMES=1  ASCENDING=1  CASC_LEN=1  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=22"
    test_arr[$LINENO]="make all DATA_TYPE=int16  DIM_SIZE=512   NUM_FRAMES=1  ASCENDING=1  CASC_LEN=1  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=22"

    test_arr[$LINENO]="make all DATA_TYPE=uint16  DIM_SIZE=32   NUM_FRAMES=1  ASCENDING=1  CASC_LEN=1  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=22"
    test_arr[$LINENO]="make all DATA_TYPE=uint16  DIM_SIZE=64   NUM_FRAMES=1  ASCENDING=1  CASC_LEN=1  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=22"
    test_arr[$LINENO]="make all DATA_TYPE=uint16  DIM_SIZE=128  NUM_FRAMES=1  ASCENDING=1  CASC_LEN=1  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=22"
    test_arr[$LINENO]="make all DATA_TYPE=uint16  DIM_SIZE=256  NUM_FRAMES=1  ASCENDING=1  CASC_LEN=1  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=22"
    test_arr[$LINENO]="make all DATA_TYPE=uint16  DIM_SIZE=512  NUM_FRAMES=1  ASCENDING=1  CASC_LEN=1  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=22"

    if [[ "$*" == *list_dimension* ]]
    then
        test_arr[$LINENO]="make all DATA_TYPE=float  DIM_SIZE=256   NUM_FRAMES=1  ASCENDING=1  CASC_LEN=1  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=1"
        test_arr[$LINENO]="make all DATA_TYPE=float  DIM_SIZE=512   NUM_FRAMES=1  ASCENDING=1  CASC_LEN=1  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=1"
        test_arr[$LINENO]="make all DATA_TYPE=float  DIM_SIZE=1024  NUM_FRAMES=1  ASCENDING=1  CASC_LEN=1  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=1"
        test_arr[$LINENO]="make all DATA_TYPE=float  DIM_SIZE=2048  NUM_FRAMES=1  ASCENDING=1  CASC_LEN=1  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=1"
        # test_arr[$LINENO]="make all DATA_TYPE=float  DIM_SIZE=4096  NUM_FRAMES=1  ASCENDING=1  CASC_LEN=1  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=1"

        # test_arr[$LINENO]="make all DATA_TYPE=int16  DIM_SIZE=256   NUM_FRAMES=1  ASCENDING=1  CASC_LEN=1  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=1"
        # test_arr[$LINENO]="make all DATA_TYPE=int16  DIM_SIZE=512   NUM_FRAMES=1  ASCENDING=1  CASC_LEN=1  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=1"
        # test_arr[$LINENO]="make all DATA_TYPE=int16  DIM_SIZE=1024  NUM_FRAMES=1  ASCENDING=1  CASC_LEN=1  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=1"
        # test_arr[$LINENO]="make all DATA_TYPE=int16  DIM_SIZE=2048  NUM_FRAMES=1  ASCENDING=1  CASC_LEN=1  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=1"
        # test_arr[$LINENO]="make all DATA_TYPE=int16  DIM_SIZE=4096  NUM_FRAMES=1  ASCENDING=1  CASC_LEN=1  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=1"

        # test_arr[$LINENO]="make all DATA_TYPE=int16  DIM_SIZE=256   NUM_FRAMES=1  ASCENDING=1  CASC_LEN=1  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=1"
        # test_arr[$LINENO]="make all DATA_TYPE=int16  DIM_SIZE=512   NUM_FRAMES=1  ASCENDING=1  CASC_LEN=1  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=1"
        # test_arr[$LINENO]="make all DATA_TYPE=int16  DIM_SIZE=1024  NUM_FRAMES=1  ASCENDING=1  CASC_LEN=1  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=1"
        # test_arr[$LINENO]="make all DATA_TYPE=int16  DIM_SIZE=2048  NUM_FRAMES=1  ASCENDING=1  CASC_LEN=1  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=1"
        # test_arr[$LINENO]="make all DATA_TYPE=int16  DIM_SIZE=4096  NUM_FRAMES=1  ASCENDING=1  CASC_LEN=1  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=1"
    fi
fi

if [[ "$*" == *ssr* || "$*" == *checkin* || "$*" == *qor* ]]
then
# SSR = 2
    test_arr[$LINENO]="make all DATA_TYPE=int16  DIM_SIZE=512     NUM_FRAMES=1  ASCENDING=0  CASC_LEN=1 UUT_SSR=2  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=1"
    test_arr[$LINENO]="make all DATA_TYPE=int32  DIM_SIZE=256     NUM_FRAMES=1  ASCENDING=0  CASC_LEN=1 UUT_SSR=2  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=1"
    test_arr[$LINENO]="make all DATA_TYPE=float  DIM_SIZE=256     NUM_FRAMES=1  ASCENDING=0  CASC_LEN=1 UUT_SSR=2  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=1"

    test_arr[$LINENO]="make all DATA_TYPE=int16  DIM_SIZE=2048    NUM_FRAMES=1  ASCENDING=0  CASC_LEN=1 UUT_SSR=2  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=1"
    test_arr[$LINENO]="make all DATA_TYPE=int32  DIM_SIZE=1024    NUM_FRAMES=1  ASCENDING=0  CASC_LEN=1 UUT_SSR=2  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=1"
    test_arr[$LINENO]="make all DATA_TYPE=float  DIM_SIZE=1024    NUM_FRAMES=1  ASCENDING=0  CASC_LEN=1 UUT_SSR=2  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=1"

    test_arr[$LINENO]="make all DATA_TYPE=int16  DIM_SIZE=4096    NUM_FRAMES=1  ASCENDING=0  CASC_LEN=1 UUT_SSR=2  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=1"
    test_arr[$LINENO]="make all DATA_TYPE=int32  DIM_SIZE=2048    NUM_FRAMES=1  ASCENDING=0  CASC_LEN=1 UUT_SSR=2  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=1"
    test_arr[$LINENO]="make all DATA_TYPE=float  DIM_SIZE=2048    NUM_FRAMES=1  ASCENDING=0  CASC_LEN=1 UUT_SSR=2  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=1"

    test_arr[$LINENO]="make all DATA_TYPE=int16  DIM_SIZE=8192    NUM_FRAMES=1  ASCENDING=0  CASC_LEN=1 UUT_SSR=2  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=1"
    test_arr[$LINENO]="make all DATA_TYPE=int32  DIM_SIZE=4096    NUM_FRAMES=1  ASCENDING=0  CASC_LEN=1 UUT_SSR=2  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=1"
    test_arr[$LINENO]="make all DATA_TYPE=float  DIM_SIZE=4096    NUM_FRAMES=1  ASCENDING=0  CASC_LEN=1 UUT_SSR=2  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=1"

    test_arr[$LINENO]="make all DATA_TYPE=int16  DIM_SIZE=16384   NUM_FRAMES=1  ASCENDING=0  CASC_LEN=1 UUT_SSR=2  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=1"
    test_arr[$LINENO]="make all DATA_TYPE=int32  DIM_SIZE=8192    NUM_FRAMES=1  ASCENDING=0  CASC_LEN=1 UUT_SSR=2  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=1"
    test_arr[$LINENO]="make all DATA_TYPE=float  DIM_SIZE=8192    NUM_FRAMES=1  ASCENDING=0  CASC_LEN=1 UUT_SSR=2  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=1"

    test_arr[$LINENO]="make all DATA_TYPE=int16  DIM_SIZE=32768   NUM_FRAMES=1  ASCENDING=0  CASC_LEN=1 UUT_SSR=2  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=1"
    test_arr[$LINENO]="make all DATA_TYPE=int32  DIM_SIZE=16384   NUM_FRAMES=1  ASCENDING=0  CASC_LEN=1 UUT_SSR=2  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=1"
    test_arr[$LINENO]="make all DATA_TYPE=float  DIM_SIZE=16384   NUM_FRAMES=1  ASCENDING=0  CASC_LEN=1 UUT_SSR=2  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=1"
# SSR=4
    test_arr[$LINENO]="make all DATA_TYPE=int16  DIM_SIZE=2048    NUM_FRAMES=1  ASCENDING=1  CASC_LEN=1 UUT_SSR=4  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=1"
    test_arr[$LINENO]="make all DATA_TYPE=int32  DIM_SIZE=1024    NUM_FRAMES=1  ASCENDING=1  CASC_LEN=1 UUT_SSR=4  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=1"
    test_arr[$LINENO]="make all DATA_TYPE=float  DIM_SIZE=1024    NUM_FRAMES=1  ASCENDING=1  CASC_LEN=1 UUT_SSR=4  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=1"

    test_arr[$LINENO]="make all DATA_TYPE=int16  DIM_SIZE=4096    NUM_FRAMES=1  ASCENDING=1  CASC_LEN=1 UUT_SSR=4  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=1"
    test_arr[$LINENO]="make all DATA_TYPE=int32  DIM_SIZE=2048    NUM_FRAMES=1  ASCENDING=1  CASC_LEN=1 UUT_SSR=4  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=1"
    test_arr[$LINENO]="make all DATA_TYPE=float  DIM_SIZE=2048    NUM_FRAMES=1  ASCENDING=1  CASC_LEN=1 UUT_SSR=4  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=1"

    test_arr[$LINENO]="make all DATA_TYPE=int16  DIM_SIZE=8192    NUM_FRAMES=1  ASCENDING=1  CASC_LEN=1 UUT_SSR=4  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=1"
    test_arr[$LINENO]="make all DATA_TYPE=int32  DIM_SIZE=4096    NUM_FRAMES=1  ASCENDING=1  CASC_LEN=1 UUT_SSR=4  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=1"
    test_arr[$LINENO]="make all DATA_TYPE=float  DIM_SIZE=4096    NUM_FRAMES=1  ASCENDING=1  CASC_LEN=1 UUT_SSR=4  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=1"

    test_arr[$LINENO]="make all DATA_TYPE=int16  DIM_SIZE=16384   NUM_FRAMES=1  ASCENDING=1  CASC_LEN=1 UUT_SSR=4  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=1"
    test_arr[$LINENO]="make all DATA_TYPE=int32  DIM_SIZE=8192    NUM_FRAMES=1  ASCENDING=1  CASC_LEN=1 UUT_SSR=4  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=1"
    test_arr[$LINENO]="make all DATA_TYPE=float  DIM_SIZE=8192    NUM_FRAMES=1  ASCENDING=1  CASC_LEN=1 UUT_SSR=4  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=1"

    test_arr[$LINENO]="make all DATA_TYPE=int16  DIM_SIZE=32768   NUM_FRAMES=1  ASCENDING=1  CASC_LEN=1 UUT_SSR=4  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=1"
    test_arr[$LINENO]="make all DATA_TYPE=int32  DIM_SIZE=16384   NUM_FRAMES=1  ASCENDING=1  CASC_LEN=1 UUT_SSR=4  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=1"
    test_arr[$LINENO]="make all DATA_TYPE=float  DIM_SIZE=16384   NUM_FRAMES=1  ASCENDING=1  CASC_LEN=1 UUT_SSR=4  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=1"

    test_arr[$LINENO]="make all DATA_TYPE=int16  DIM_SIZE=65536   NUM_FRAMES=1  ASCENDING=1  CASC_LEN=1 UUT_SSR=4  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=1"
    test_arr[$LINENO]="make all DATA_TYPE=int32  DIM_SIZE=32768   NUM_FRAMES=1  ASCENDING=1  CASC_LEN=1 UUT_SSR=4  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=1"
    test_arr[$LINENO]="make all DATA_TYPE=float  DIM_SIZE=32768   NUM_FRAMES=1  ASCENDING=1  CASC_LEN=1 UUT_SSR=4  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=1"
# SSR=8
    test_arr[$LINENO]="make all DATA_TYPE=int16  DIM_SIZE=4096    NUM_FRAMES=1  ASCENDING=0  CASC_LEN=1 UUT_SSR=8  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=1"
    test_arr[$LINENO]="make all DATA_TYPE=int32  DIM_SIZE=4096    NUM_FRAMES=1  ASCENDING=0  CASC_LEN=1 UUT_SSR=8  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=1"
    test_arr[$LINENO]="make all DATA_TYPE=float  DIM_SIZE=4096    NUM_FRAMES=1  ASCENDING=0  CASC_LEN=1 UUT_SSR=8  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=1"

    test_arr[$LINENO]="make all DATA_TYPE=int16  DIM_SIZE=8192    NUM_FRAMES=1  ASCENDING=0  CASC_LEN=1 UUT_SSR=8  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=1"
    test_arr[$LINENO]="make all DATA_TYPE=int32  DIM_SIZE=8192    NUM_FRAMES=1  ASCENDING=0  CASC_LEN=1 UUT_SSR=8  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=1"
    test_arr[$LINENO]="make all DATA_TYPE=float  DIM_SIZE=8192    NUM_FRAMES=1  ASCENDING=0  CASC_LEN=1 UUT_SSR=8  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=1"
    # AIE-ML
# SSR = 2
    test_arr[$LINENO]="make all DATA_TYPE=int16  DIM_SIZE=256     NUM_FRAMES=1  ASCENDING=0  CASC_LEN=1 UUT_SSR=2  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=2"
    test_arr[$LINENO]="make all DATA_TYPE=int32  DIM_SIZE=256     NUM_FRAMES=1  ASCENDING=0  CASC_LEN=1 UUT_SSR=2  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=2"
    test_arr[$LINENO]="make all DATA_TYPE=float  DIM_SIZE=256     NUM_FRAMES=1  ASCENDING=0  CASC_LEN=1 UUT_SSR=2  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=2"

    test_arr[$LINENO]="make all DATA_TYPE=int16  DIM_SIZE=1024    NUM_FRAMES=1  ASCENDING=0  CASC_LEN=1 UUT_SSR=2  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=2"
    test_arr[$LINENO]="make all DATA_TYPE=int32  DIM_SIZE=1024    NUM_FRAMES=1  ASCENDING=0  CASC_LEN=1 UUT_SSR=2  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=2"
    test_arr[$LINENO]="make all DATA_TYPE=float  DIM_SIZE=1024    NUM_FRAMES=1  ASCENDING=0  CASC_LEN=1 UUT_SSR=2  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=2"

    test_arr[$LINENO]="make all DATA_TYPE=int16  DIM_SIZE=2048    NUM_FRAMES=1  ASCENDING=0  CASC_LEN=1 UUT_SSR=2  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=2"
    test_arr[$LINENO]="make all DATA_TYPE=int32  DIM_SIZE=2048    NUM_FRAMES=1  ASCENDING=0  CASC_LEN=1 UUT_SSR=2  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=2"
    test_arr[$LINENO]="make all DATA_TYPE=float  DIM_SIZE=2048    NUM_FRAMES=1  ASCENDING=0  CASC_LEN=1 UUT_SSR=2  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=2"

    test_arr[$LINENO]="make all DATA_TYPE=int16  DIM_SIZE=4096    NUM_FRAMES=1  ASCENDING=0  CASC_LEN=1 UUT_SSR=2  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=2"
    test_arr[$LINENO]="make all DATA_TYPE=int32  DIM_SIZE=4096    NUM_FRAMES=1  ASCENDING=0  CASC_LEN=1 UUT_SSR=2  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=2"
    test_arr[$LINENO]="make all DATA_TYPE=float  DIM_SIZE=4096    NUM_FRAMES=1  ASCENDING=0  CASC_LEN=1 UUT_SSR=2  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=2"

    test_arr[$LINENO]="make all DATA_TYPE=int16  DIM_SIZE=8192    NUM_FRAMES=1  ASCENDING=0  CASC_LEN=1 UUT_SSR=2  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=2"
    test_arr[$LINENO]="make all DATA_TYPE=int32  DIM_SIZE=8192    NUM_FRAMES=1  ASCENDING=0  CASC_LEN=1 UUT_SSR=2  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=2"
    test_arr[$LINENO]="make all DATA_TYPE=float  DIM_SIZE=8192    NUM_FRAMES=1  ASCENDING=0  CASC_LEN=1 UUT_SSR=2  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=2"

    test_arr[$LINENO]="make all DATA_TYPE=uint16 DIM_SIZE=32768   NUM_FRAMES=1  ASCENDING=0  CASC_LEN=1 UUT_SSR=2  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=2"
    test_arr[$LINENO]="make all DATA_TYPE=int16  DIM_SIZE=32768   NUM_FRAMES=1  ASCENDING=0  CASC_LEN=1 UUT_SSR=2  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=2"
    test_arr[$LINENO]="make all DATA_TYPE=int32  DIM_SIZE=16384   NUM_FRAMES=1  ASCENDING=0  CASC_LEN=1 UUT_SSR=2  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=2"
    test_arr[$LINENO]="make all DATA_TYPE=float  DIM_SIZE=16384   NUM_FRAMES=1  ASCENDING=0  CASC_LEN=1 UUT_SSR=2  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=2"
# SSR=4
    test_arr[$LINENO]="make all DATA_TYPE=int16  DIM_SIZE=1024    NUM_FRAMES=1  ASCENDING=1  CASC_LEN=1 UUT_SSR=4  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=2"
    test_arr[$LINENO]="make all DATA_TYPE=int32  DIM_SIZE=1024    NUM_FRAMES=1  ASCENDING=1  CASC_LEN=1 UUT_SSR=4  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=2"
    test_arr[$LINENO]="make all DATA_TYPE=float  DIM_SIZE=1024    NUM_FRAMES=1  ASCENDING=1  CASC_LEN=1 UUT_SSR=4  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=2"

    test_arr[$LINENO]="make all DATA_TYPE=int16  DIM_SIZE=2048    NUM_FRAMES=1  ASCENDING=1  CASC_LEN=1 UUT_SSR=4  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=2"
    test_arr[$LINENO]="make all DATA_TYPE=int32  DIM_SIZE=2048    NUM_FRAMES=1  ASCENDING=1  CASC_LEN=1 UUT_SSR=4  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=2"
    test_arr[$LINENO]="make all DATA_TYPE=float  DIM_SIZE=2048    NUM_FRAMES=1  ASCENDING=1  CASC_LEN=1 UUT_SSR=4  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=2"

    test_arr[$LINENO]="make all DATA_TYPE=int16  DIM_SIZE=4096    NUM_FRAMES=1  ASCENDING=1  CASC_LEN=1 UUT_SSR=4  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=2"
    test_arr[$LINENO]="make all DATA_TYPE=int32  DIM_SIZE=4096    NUM_FRAMES=1  ASCENDING=1  CASC_LEN=1 UUT_SSR=4  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=2"
    test_arr[$LINENO]="make all DATA_TYPE=float  DIM_SIZE=4096    NUM_FRAMES=1  ASCENDING=1  CASC_LEN=1 UUT_SSR=4  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=2"

    test_arr[$LINENO]="make all DATA_TYPE=int16  DIM_SIZE=8192    NUM_FRAMES=1  ASCENDING=1  CASC_LEN=1 UUT_SSR=4  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=2"
    test_arr[$LINENO]="make all DATA_TYPE=int32  DIM_SIZE=8192    NUM_FRAMES=1  ASCENDING=1  CASC_LEN=1 UUT_SSR=4  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=2"
    test_arr[$LINENO]="make all DATA_TYPE=float  DIM_SIZE=8192    NUM_FRAMES=1  ASCENDING=1  CASC_LEN=1 UUT_SSR=4  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=2"
# SSR=8
    test_arr[$LINENO]="make all DATA_TYPE=int16  DIM_SIZE=4096    NUM_FRAMES=1  ASCENDING=0  CASC_LEN=1 UUT_SSR=8  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=2"
    test_arr[$LINENO]="make all DATA_TYPE=int32  DIM_SIZE=4096    NUM_FRAMES=1  ASCENDING=0  CASC_LEN=1 UUT_SSR=8  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=2"
    test_arr[$LINENO]="make all DATA_TYPE=float  DIM_SIZE=4096    NUM_FRAMES=1  ASCENDING=0  CASC_LEN=1 UUT_SSR=8  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=2"

    test_arr[$LINENO]="make all DATA_TYPE=int16  DIM_SIZE=8192    NUM_FRAMES=1  ASCENDING=0  CASC_LEN=1 UUT_SSR=8  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=2"
    test_arr[$LINENO]="make all DATA_TYPE=int32  DIM_SIZE=8192    NUM_FRAMES=1  ASCENDING=0  CASC_LEN=1 UUT_SSR=8  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=2"
    test_arr[$LINENO]="make all DATA_TYPE=float  DIM_SIZE=8192    NUM_FRAMES=1  ASCENDING=0  CASC_LEN=1 UUT_SSR=8  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=2"
    # AIE-MLv2
# SSR = 2
    test_arr[$LINENO]="make all DATA_TYPE=int16  DIM_SIZE=256     NUM_FRAMES=1  ASCENDING=0  CASC_LEN=1 UUT_SSR=2  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=22"
    test_arr[$LINENO]="make all DATA_TYPE=int32  DIM_SIZE=256     NUM_FRAMES=1  ASCENDING=0  CASC_LEN=1 UUT_SSR=2  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=22"
    test_arr[$LINENO]="make all DATA_TYPE=float  DIM_SIZE=256     NUM_FRAMES=1  ASCENDING=0  CASC_LEN=1 UUT_SSR=2  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=22"

    test_arr[$LINENO]="make all DATA_TYPE=int16  DIM_SIZE=1024    NUM_FRAMES=1  ASCENDING=0  CASC_LEN=1 UUT_SSR=2  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=22"
    test_arr[$LINENO]="make all DATA_TYPE=int32  DIM_SIZE=1024    NUM_FRAMES=1  ASCENDING=0  CASC_LEN=1 UUT_SSR=2  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=22"
    test_arr[$LINENO]="make all DATA_TYPE=float  DIM_SIZE=1024    NUM_FRAMES=1  ASCENDING=0  CASC_LEN=1 UUT_SSR=2  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=22"

    test_arr[$LINENO]="make all DATA_TYPE=int16  DIM_SIZE=2048    NUM_FRAMES=1  ASCENDING=0  CASC_LEN=1 UUT_SSR=2  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=22"
    test_arr[$LINENO]="make all DATA_TYPE=int32  DIM_SIZE=2048    NUM_FRAMES=1  ASCENDING=0  CASC_LEN=1 UUT_SSR=2  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=22"
    test_arr[$LINENO]="make all DATA_TYPE=float  DIM_SIZE=2048    NUM_FRAMES=1  ASCENDING=0  CASC_LEN=1 UUT_SSR=2  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=22"

    test_arr[$LINENO]="make all DATA_TYPE=int16  DIM_SIZE=4096    NUM_FRAMES=1  ASCENDING=0  CASC_LEN=1 UUT_SSR=2  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=22"
    test_arr[$LINENO]="make all DATA_TYPE=int32  DIM_SIZE=4096    NUM_FRAMES=1  ASCENDING=0  CASC_LEN=1 UUT_SSR=2  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=22"
    test_arr[$LINENO]="make all DATA_TYPE=float  DIM_SIZE=4096    NUM_FRAMES=1  ASCENDING=0  CASC_LEN=1 UUT_SSR=2  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=22"

    test_arr[$LINENO]="make all DATA_TYPE=int16  DIM_SIZE=8192    NUM_FRAMES=1  ASCENDING=0  CASC_LEN=1 UUT_SSR=2  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=22"
    test_arr[$LINENO]="make all DATA_TYPE=int32  DIM_SIZE=8192    NUM_FRAMES=1  ASCENDING=0  CASC_LEN=1 UUT_SSR=2  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=22"
    test_arr[$LINENO]="make all DATA_TYPE=float  DIM_SIZE=8192    NUM_FRAMES=1  ASCENDING=0  CASC_LEN=1 UUT_SSR=2  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=22"

    test_arr[$LINENO]="make all DATA_TYPE=uint16 DIM_SIZE=32768   NUM_FRAMES=1  ASCENDING=0  CASC_LEN=1 UUT_SSR=2  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=22"
    test_arr[$LINENO]="make all DATA_TYPE=int16  DIM_SIZE=32768   NUM_FRAMES=1  ASCENDING=0  CASC_LEN=1 UUT_SSR=2  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=22"
    test_arr[$LINENO]="make all DATA_TYPE=int32  DIM_SIZE=16384   NUM_FRAMES=1  ASCENDING=0  CASC_LEN=1 UUT_SSR=2  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=22"
    test_arr[$LINENO]="make all DATA_TYPE=float  DIM_SIZE=16384   NUM_FRAMES=1  ASCENDING=0  CASC_LEN=1 UUT_SSR=2  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=22"
# SSR=4
    test_arr[$LINENO]="make all DATA_TYPE=int16  DIM_SIZE=1024    NUM_FRAMES=1  ASCENDING=1  CASC_LEN=1 UUT_SSR=4  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=22"
    test_arr[$LINENO]="make all DATA_TYPE=int32  DIM_SIZE=1024    NUM_FRAMES=1  ASCENDING=1  CASC_LEN=1 UUT_SSR=4  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=22"
    test_arr[$LINENO]="make all DATA_TYPE=float  DIM_SIZE=1024    NUM_FRAMES=1  ASCENDING=1  CASC_LEN=1 UUT_SSR=4  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=22"

    test_arr[$LINENO]="make all DATA_TYPE=int16  DIM_SIZE=2048    NUM_FRAMES=1  ASCENDING=1  CASC_LEN=1 UUT_SSR=4  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=22"
    test_arr[$LINENO]="make all DATA_TYPE=int32  DIM_SIZE=2048    NUM_FRAMES=1  ASCENDING=1  CASC_LEN=1 UUT_SSR=4  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=22"
    test_arr[$LINENO]="make all DATA_TYPE=float  DIM_SIZE=2048    NUM_FRAMES=1  ASCENDING=1  CASC_LEN=1 UUT_SSR=4  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=22"

    test_arr[$LINENO]="make all DATA_TYPE=int16  DIM_SIZE=4096    NUM_FRAMES=1  ASCENDING=1  CASC_LEN=1 UUT_SSR=4  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=22"
    test_arr[$LINENO]="make all DATA_TYPE=int32  DIM_SIZE=4096    NUM_FRAMES=1  ASCENDING=1  CASC_LEN=1 UUT_SSR=4  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=22"
    test_arr[$LINENO]="make all DATA_TYPE=float  DIM_SIZE=4096    NUM_FRAMES=1  ASCENDING=1  CASC_LEN=1 UUT_SSR=4  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=22"

    test_arr[$LINENO]="make all DATA_TYPE=int16  DIM_SIZE=8192    NUM_FRAMES=1  ASCENDING=1  CASC_LEN=1 UUT_SSR=4  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=22"
    test_arr[$LINENO]="make all DATA_TYPE=int32  DIM_SIZE=8192    NUM_FRAMES=1  ASCENDING=1  CASC_LEN=1 UUT_SSR=4  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=22"
    test_arr[$LINENO]="make all DATA_TYPE=float  DIM_SIZE=8192    NUM_FRAMES=1  ASCENDING=1  CASC_LEN=1 UUT_SSR=4  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=22"
# SSR=8
    test_arr[$LINENO]="make all DATA_TYPE=int16  DIM_SIZE=4096    NUM_FRAMES=1  ASCENDING=0  CASC_LEN=1 UUT_SSR=8  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=22"
    test_arr[$LINENO]="make all DATA_TYPE=int32  DIM_SIZE=4096    NUM_FRAMES=1  ASCENDING=0  CASC_LEN=1 UUT_SSR=8  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=22"
    test_arr[$LINENO]="make all DATA_TYPE=float  DIM_SIZE=4096    NUM_FRAMES=1  ASCENDING=0  CASC_LEN=1 UUT_SSR=8  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=22"

    test_arr[$LINENO]="make all DATA_TYPE=int16  DIM_SIZE=8192    NUM_FRAMES=1  ASCENDING=0  CASC_LEN=1 UUT_SSR=8  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=22"
    test_arr[$LINENO]="make all DATA_TYPE=int32  DIM_SIZE=8192    NUM_FRAMES=1  ASCENDING=0  CASC_LEN=1 UUT_SSR=8  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=22"
    test_arr[$LINENO]="make all DATA_TYPE=float  DIM_SIZE=8192    NUM_FRAMES=1  ASCENDING=0  CASC_LEN=1 UUT_SSR=8  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=22"
fi

if [[ "$*" == *descending* || "$*" == *checkin* || "$*" == *qor* ]]
then
    test_arr[$LINENO]="make all DATA_TYPE=float  DIM_SIZE=64    NUM_FRAMES=1  ASCENDING=0  CASC_LEN=1  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=1"
    test_arr[$LINENO]="make all DATA_TYPE=int16  DIM_SIZE=64    NUM_FRAMES=1  ASCENDING=0  CASC_LEN=1  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=1"

    test_arr[$LINENO]="make all DATA_TYPE=float  DIM_SIZE=64    NUM_FRAMES=1  ASCENDING=0  CASC_LEN=1  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=2"
    test_arr[$LINENO]="make all DATA_TYPE=int16  DIM_SIZE=64    NUM_FRAMES=1  ASCENDING=0  CASC_LEN=1  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=2"

    test_arr[$LINENO]="make all DATA_TYPE=float  DIM_SIZE=64    NUM_FRAMES=1  ASCENDING=0  CASC_LEN=1  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=22"
    test_arr[$LINENO]="make all DATA_TYPE=int16  DIM_SIZE=64    NUM_FRAMES=1  ASCENDING=0  CASC_LEN=1  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=22"
fi

if [[ "$*" == *cascades* || "$*" == *checkin* || "$*" == *qor* ]]
then
    test_arr[$LINENO]="make all DATA_TYPE=float  DIM_SIZE=128    NUM_FRAMES=1  ASCENDING=1  CASC_LEN=1  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=1"
    test_arr[$LINENO]="make all DATA_TYPE=float  DIM_SIZE=128    NUM_FRAMES=1  ASCENDING=1  CASC_LEN=2  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=1"
    test_arr[$LINENO]="make all DATA_TYPE=float  DIM_SIZE=128    NUM_FRAMES=1  ASCENDING=1  CASC_LEN=4  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=1"
    test_arr[$LINENO]="make all DATA_TYPE=float  DIM_SIZE=128    NUM_FRAMES=1  ASCENDING=1  CASC_LEN=7  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=1"
    test_arr[$LINENO]="make all DATA_TYPE=float  DIM_SIZE=128    NUM_FRAMES=1  ASCENDING=1  CASC_LEN=12  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=1"
    # test_arr[$LINENO]="make all DATA_TYPE=float  DIM_SIZE=128    NUM_FRAMES=1  ASCENDING=1  CASC_LEN=17  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=1"
    # test_arr[$LINENO]="make all DATA_TYPE=float  DIM_SIZE=128    NUM_FRAMES=1  ASCENDING=1  CASC_LEN=20  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=1"
    # test_arr[$LINENO]="make all DATA_TYPE=float  DIM_SIZE=128    NUM_FRAMES=1  ASCENDING=1  CASC_LEN=28  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=1"

    # test_arr[$LINENO]="make all DATA_TYPE=float  DIM_SIZE=64    NUM_FRAMES=1  ASCENDING=1  CASC_LEN=5  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=2"
    # test_arr[$LINENO]="make all DATA_TYPE=int16  DIM_SIZE=64    NUM_FRAMES=1  ASCENDING=1  CASC_LEN=5  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=2"
    # test_arr[$LINENO]="make all DATA_TYPE=int32  DIM_SIZE=64    NUM_FRAMES=1  ASCENDING=1  CASC_LEN=5  STIM_TYPE=0  NITER=$niters   AIE_VARIANT=2"
fi

if [[ "$*" == *2_byte_cases* ]]
then
    test_arr[$LINENO]="make all DATA_TYPE=uint16  DIM_SIZE=16  NUM_FRAMES=1  ASCENDING=1  CASC_LEN=1  STIM_TYPE=0  NITER=1  TARGET=x86sim  AIE_VARIANT=1"
    test_arr[$LINENO]="make all DATA_TYPE=uint16  DIM_SIZE=32  NUM_FRAMES=1  ASCENDING=1  CASC_LEN=1  STIM_TYPE=0  NITER=1  TARGET=x86sim  AIE_VARIANT=1"
    test_arr[$LINENO]="make all DATA_TYPE=int16   DIM_SIZE=16  NUM_FRAMES=1  ASCENDING=1  CASC_LEN=1  STIM_TYPE=0  NITER=1  TARGET=x86sim  AIE_VARIANT=1"
    test_arr[$LINENO]="make all DATA_TYPE=int16   DIM_SIZE=32  NUM_FRAMES=1  ASCENDING=1  CASC_LEN=1  STIM_TYPE=0  NITER=1  TARGET=x86sim  AIE_VARIANT=1"
fi

if [[ "$*" == *x86_vs_aie* ]]
then
    test_arr[$LINENO]="make all DATA_TYPE=float   DIM_SIZE=32  NUM_FRAMES=1  ASCENDING=1  CASC_LEN=1  STIM_TYPE=0  NITER=1  TARGET=x86sim"
    test_arr[$LINENO]="make all DATA_TYPE=float   DIM_SIZE=32  NUM_FRAMES=1  ASCENDING=1  CASC_LEN=1  STIM_TYPE=0  NITER=1  TARGET=hw"
    test_arr[$LINENO]="make all DATA_TYPE=float   DIM_SIZE=16   NUM_FRAMES=1  ASCENDING=1  CASC_LEN=1  STIM_TYPE=0  NITER=1  TARGET=x86sim"
    test_arr[$LINENO]="make all DATA_TYPE=float   DIM_SIZE=16   NUM_FRAMES=1  ASCENDING=1  CASC_LEN=1  STIM_TYPE=0  NITER=1  TARGET=hw"
fi

if [[ "$*" == *testing_tile_memory* ]]
then
    test_arr[$LINENO]="make all DATA_TYPE=int16   DIM_SIZE=512  NUM_FRAMES=1   ASCENDING=0  CASC_LEN=1   STIM_TYPE=0  NITER=1  TARGET=hw  AIE_VARIANT=1"
    test_arr[$LINENO]="make all DATA_TYPE=int16   DIM_SIZE=512  NUM_FRAMES=1   ASCENDING=0  CASC_LEN=16  STIM_TYPE=0  NITER=1  TARGET=hw  AIE_VARIANT=1"
    test_arr[$LINENO]="make all DATA_TYPE=int16   DIM_SIZE=512  NUM_FRAMES=1   ASCENDING=1  CASC_LEN=1   STIM_TYPE=0  NITER=1  TARGET=hw  AIE_VARIANT=1"
    test_arr[$LINENO]="make all DATA_TYPE=int16   DIM_SIZE=512  NUM_FRAMES=1   ASCENDING=1  CASC_LEN=16  STIM_TYPE=0  NITER=1  TARGET=hw  AIE_VARIANT=1"
    test_arr[$LINENO]="make all DATA_TYPE=int16   DIM_SIZE=512  NUM_FRAMES=32  ASCENDING=0  CASC_LEN=1   STIM_TYPE=0  NITER=1  TARGET=hw  AIE_VARIANT=1"
    test_arr[$LINENO]="make all DATA_TYPE=int16   DIM_SIZE=512  NUM_FRAMES=32  ASCENDING=0  CASC_LEN=16  STIM_TYPE=0  NITER=1  TARGET=hw  AIE_VARIANT=1"
    test_arr[$LINENO]="make all DATA_TYPE=int16   DIM_SIZE=512  NUM_FRAMES=32  ASCENDING=1  CASC_LEN=1   STIM_TYPE=0  NITER=1  TARGET=hw  AIE_VARIANT=1"
    test_arr[$LINENO]="make all DATA_TYPE=int16   DIM_SIZE=512  NUM_FRAMES=32  ASCENDING=1  CASC_LEN=16  STIM_TYPE=0  NITER=1  TARGET=hw  AIE_VARIANT=1"
fi

# if [[ "$*" == *ssr* || "$*" == *checkin* || "$*" == *qor* || *aie2* ]]
# then
#     if [[ "$*" == *qor* ]]
#     then
#         niters=16
#     else
#         niters=4
#     fi
#     if [[ "$*" == *aie2* ]]
#     then
#         aie_variant=2
#     else
#         aie_variant=1
#     fi
#     test_arr[$LINENO]="make all DATA_TYPE=int32  DIM_SIZE=32  NUM_FRAMES=8   API_IO=0  CASC_LEN=2  STIM_TYPE=0  NITER=$niters  TARGET=hw"
#     test_arr[$LINENO]="make all DATA_TYPE=int32  DIM_SIZE=32  NUM_FRAMES=8   API_IO=0  CASC_LEN=4  STIM_TYPE=0  NITER=$niters  TARGET=hw"
#     test_arr[$LINENO]="make all DATA_TYPE=int32  DIM_SIZE=32  NUM_FRAMES=8   API_IO=0  CASC_LEN=8  STIM_TYPE=0  NITER=$niters  TARGET=hw"
# fi

# if [[ "$*" == *illegal_case_checks* ]]
# then
# fi


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