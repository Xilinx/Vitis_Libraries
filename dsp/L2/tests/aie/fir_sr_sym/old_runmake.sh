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

test_arr=()

if [[ "$*" == *smoke* ]]
then
    test_arr[$LINENO]="make all AIE_VARIANT=1 "
    test_arr[$LINENO]="make all AIE_VARIANT=2 "
    test_arr[$LINENO]="make all AIE_VARIANT=22 "
fi

if [[ "$*" == *single_buf*  || "$*" == *checkin* ||  "$*" == *qor* ]]
then
    test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=int16 FIR_LEN=89 SHIFT=15 CASC_LEN=2 INPUT_WINDOW_VSIZE=256 SINGLE_BUF=1"
fi

if [[ "$*" == *commslib* || "$*" == "" || "$*" == *checkin*  ]]
then
    test_arr[${#test_arr[*]}]="echo 'Performing CommsLib Equivalent Functions...' "
    # TODO: Hook up exact same stimulus as commslib.
    # from spreadsheet
    test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=int16  DUAL_IP=1 FIR_LEN=199 SHIFT=20 INPUT_WINDOW_VSIZE=256 "
    # fir_129t_sym
    test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=int16  DUAL_IP=1 FIR_LEN=129 SHIFT=15 INPUT_WINDOW_VSIZE=256 "
    # fir_89t_sym
    test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=int16  DUAL_IP=1 FIR_LEN=89 SHIFT=15 INPUT_WINDOW_VSIZE=256 "
    # fir_63t_sym
    test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=int16  DUAL_IP=1 FIR_LEN=63 SHIFT=15 INPUT_WINDOW_VSIZE=256 "
    # fir_30t_cplx_3core - window based with dual_ip
    test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=cint16 DUAL_IP=1 FIR_LEN=30 SHIFT=15 INPUT_WINDOW_VSIZE=512 CASC_LEN=3 PORT_API=0"
    # fir_30t_cplx_3core - stream based with single_ip - dual_ip don't make a difference (not implemented for this case?)
    test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=cint16 DUAL_IP=0 FIR_LEN=30 SHIFT=15 INPUT_WINDOW_VSIZE=512 CASC_LEN=3 PORT_API=1"
    # fir_24t_cplx
    test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=cint16 DUAL_IP=0 FIR_LEN=24 SHIFT=15 INPUT_WINDOW_VSIZE=256 "
    # fir_24t_cplx_2core
    test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=cint16 DUAL_IP=0 FIR_LEN=24 SHIFT=15 INPUT_WINDOW_VSIZE=256 CASC_LEN=2 PORT_API=1"
    # fir_16t_sym
    test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=int16 DUAL_IP=0 FIR_LEN=16 SHIFT=15 INPUT_WINDOW_VSIZE=256 "
    #fir_96t_real_sym (input window is cint16, but they cast to int16)
    test_arr[$LINENO]="make all DATA_TYPE=int16 COEFF_TYPE=int16 DUAL_IP=0 FIR_LEN=96 SHIFT=20 INPUT_WINDOW_VSIZE=512 "
fi

if [[  "$*" == "" || "$*" == *aie2* ]]
then
    test_arr[${#test_arr[*]}]="echo 'Running limited number of check-in tests.'"
    # Default
    test_arr[$LINENO]="make all RESULTS_PATH=./results/aie2 AIE_VARIANT=2"
    test_arr[$LINENO]="make all RESULTS_PATH=./results/aie2 AIE_VARIANT=2 DATA_TYPE=cint16 COEFF_TYPE=int16  FIR_LEN=36 CASC_LEN=1 SHIFT=16"
    test_arr[$LINENO]="make all RESULTS_PATH=./results/aie2 AIE_VARIANT=2 DATA_TYPE=cint16 COEFF_TYPE=int16 FIR_LEN=89 SHIFT=15 CASC_LEN=2 INPUT_WINDOW_VSIZE=256 "
    test_arr[$LINENO]="make all RESULTS_PATH=./results/aie2 AIE_VARIANT=2 DATA_TYPE=cint16 COEFF_TYPE=int16 FIR_LEN=8   CASC_LEN=1 SHIFT=16 ROUND_MODE=1 "
    test_arr[$LINENO]="make all RESULTS_PATH=./results/aie2 AIE_VARIANT=2 DATA_TYPE=int16 COEFF_TYPE=int16 FIR_LEN=80 CASC_LEN=3 INPUT_WINDOW_VSIZE=256 USE_COEFF_RELOAD=1 SHIFT=16 "
    test_arr[$LINENO]="make all RESULTS_PATH=./results/aie2 AIE_VARIANT=2 DATA_TYPE=cint16 COEFF_TYPE=cint16  FIR_LEN=24 SHIFT=15 INPUT_WINDOW_VSIZE=256 CASC_LEN=1 USE_COEFF_RELOAD=0 NUM_OUTPUTS=2"
    test_arr[$LINENO]="make all RESULTS_PATH=./results/aie2 AIE_VARIANT=2 DATA_TYPE=cint16 COEFF_TYPE=cint16  FIR_LEN=24 SHIFT=15 INPUT_WINDOW_VSIZE=256 NUM_OUTPUTS=1 CASC_LEN=1 USE_COEFF_RELOAD=0 PORT_API=1"
    test_arr[$LINENO]="make all RESULTS_PATH=./results/aie2 AIE_VARIANT=2 DATA_TYPE=cint16 COEFF_TYPE=cint16  FIR_LEN=24 SHIFT=15 INPUT_WINDOW_VSIZE=256 NUM_OUTPUTS=1 CASC_LEN=3 USE_COEFF_RELOAD=0 PORT_API=1"
    test_arr[$LINENO]="make all RESULTS_PATH=./results/aie2 AIE_VARIANT=2 DATA_TYPE=cint16 COEFF_TYPE=int16  FIR_LEN=1000 CASC_LEN=4 SHIFT=16"
    #test_arr[$LINENO]="make all RESULTS_PATH=./results/aie2 AIE_VARIANT=2 PORT_API=1 NITER=2 UUT_SSR=2 FIR_LEN=48 CASC_LEN=3 INPUT_WINDOW_VSIZE=25600"
    test_arr[$LINENO]="make all RESULTS_PATH=./results/aie2 AIE_VARIANT=2 DATA_TYPE=int16  COEFF_TYPE=int16  NITER=4 FIR_LEN=32  CASC_LEN=1 INPUT_WINDOW_VSIZE=2560 PORT_API=1 UUT_SSR=1 DUAL_IP=0 NUM_OUTPUTS=1 USE_COEFF_RELOAD=1 "
    test_arr[$LINENO]="make all RESULTS_PATH=./results/aie2 AIE_VARIANT=2 DATA_TYPE=int16  COEFF_TYPE=int16  NITER=4 FIR_LEN=32  CASC_LEN=1 INPUT_WINDOW_VSIZE=2560 PORT_API=1 UUT_SSR=2 DUAL_IP=0 NUM_OUTPUTS=1 USE_COEFF_RELOAD=1 "
    test_arr[$LINENO]="make all RESULTS_PATH=./results/aie2 AIE_VARIANT=2 DATA_TYPE=cint16 COEFF_TYPE=int16  NITER=4 FIR_LEN=32  CASC_LEN=2 INPUT_WINDOW_VSIZE=2560 PORT_API=1 UUT_SSR=2 DUAL_IP=0 NUM_OUTPUTS=1 USE_COEFF_RELOAD=1 "
fi

if [[  "$*" == "" || "$*" == *checkin* ]]
then
    test_arr[${#test_arr[*]}]="echo 'Running limited number of check-in tests.'"
    # Default
    test_arr[$LINENO]="make all"
    # Don't yet Run Examples - not reported.
    # test_arr[$LINENO]="make -C $DSPLIB_ROOT/L2/examples/fir_129t_sym/proj all"
    # test_arr[$LINENO]="make -C $DSPLIB_ROOT/L2/examples/fir_129t_sym_reload/proj all"
    # basic arch
    test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=int16  FIR_LEN=36 CASC_LEN=1 SHIFT=16"
    # incloads arch
    test_arr[$LINENO]="make all DATA_TYPE=cint32 COEFF_TYPE=cint32 FIR_LEN=10 CASC_LEN=1 SHIFT=16"
    # coefficient reload
    test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=cint16  FIR_LEN=24 CASC_LEN=1 SHIFT=16 INPUT_WINDOW_VSIZE=256 USE_COEFF_RELOAD=1"
    # cascade case: fir_89t_sym
    test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=int16 FIR_LEN=89 SHIFT=15 CASC_LEN=2 INPUT_WINDOW_VSIZE=256 "
    #Explore rounding (not expected to make any difference to QoR)
    test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=int16 FIR_LEN=8   CASC_LEN=1 SHIFT=16 ROUND_MODE=1 "
    test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=int16 FIR_LEN=8   CASC_LEN=1 SHIFT=16 ROUND_MODE=1 SAT_MODE=0"
    test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=int16 FIR_LEN=8   CASC_LEN=1 SHIFT=16 ROUND_MODE=1 SAT_MODE=1"
    test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=int16 FIR_LEN=8   CASC_LEN=1 SHIFT=16 ROUND_MODE=1 SAT_MODE=3"

    # CR-1076132 - coeff reload with cascades
    test_arr[$LINENO]="make all DATA_TYPE=int32 COEFF_TYPE=int32 FIR_LEN=40 CASC_LEN=2 INPUT_WINDOW_VSIZE=256 USE_COEFF_RELOAD=1"
    test_arr[$LINENO]="make all DATA_TYPE=int32 COEFF_TYPE=int32 FIR_LEN=40 CASC_LEN=3 INPUT_WINDOW_VSIZE=256 USE_COEFF_RELOAD=1 SHIFT=32 "
    test_arr[$LINENO]="make all DATA_TYPE=int16 COEFF_TYPE=int16 FIR_LEN=80 CASC_LEN=3 INPUT_WINDOW_VSIZE=256 USE_COEFF_RELOAD=1 SHIFT=16 "
    test_arr[$LINENO]="make all DATA_TYPE=cint32 COEFF_TYPE=cint32 FIR_LEN=20 CASC_LEN=3 INPUT_WINDOW_VSIZE=256 USE_COEFF_RELOAD=1 SHIFT=32 "
    # test_arr[$LINENO]="make all DATA_TYPE=int32 COEFF_TYPE=int32 FIR_LEN=40 CASC_LEN=3 INPUT_WINDOW_VSIZE=256 USE_COEFF_RELOAD=1 DATA_STIM_TYPE=5 COEFF_STIM_TYPE=4 SHIFT=0 "
    # test_arr[$LINENO]="make all DATA_TYPE=int16 COEFF_TYPE=int16 FIR_LEN=80 CASC_LEN=3 INPUT_WINDOW_VSIZE=256 USE_COEFF_RELOAD=1 DATA_STIM_TYPE=5 COEFF_STIM_TYPE=4 SHIFT=0 "
    # test_arr[$LINENO]="make all DATA_TYPE=cint32 COEFF_TYPE=cint32 FIR_LEN=20 CASC_LEN=3 INPUT_WINDOW_VSIZE=256 USE_COEFF_RELOAD=1 DATA_STIM_TYPE=5 COEFF_STIM_TYPE=4 SHIFT=0 "
    # CR-1084954 - chain of cascaded FIRs cause compilation errors.
    test_arr[$LINENO]="make all DATA_TYPE=int32 COEFF_TYPE=int32 FIR_LEN=40 CASC_LEN=2 INPUT_WINDOW_VSIZE=256 USE_COEFF_RELOAD=0 DATA_STIM_TYPE=5 COEFF_STIM_TYPE=4 SHIFT=0 "
    test_arr[$LINENO]="make all DATA_TYPE=int32 COEFF_TYPE=int32 FIR_LEN=40 CASC_LEN=3 INPUT_WINDOW_VSIZE=256 USE_COEFF_RELOAD=0 DATA_STIM_TYPE=5 COEFF_STIM_TYPE=4 SHIFT=0 "
    #Single case using multiple outputs
    test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=cint16  FIR_LEN=24 SHIFT=15 INPUT_WINDOW_VSIZE=256 CASC_LEN=1 USE_COEFF_RELOAD=0 NUM_OUTPUTS=2"
    #Cases with Streams
    test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=cint16  FIR_LEN=24 SHIFT=15 INPUT_WINDOW_VSIZE=256 NUM_OUTPUTS=1 CASC_LEN=1 USE_COEFF_RELOAD=0 PORT_API=1"
    test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=cint16  FIR_LEN=24 SHIFT=15 INPUT_WINDOW_VSIZE=256 NUM_OUTPUTS=1 CASC_LEN=1 USE_COEFF_RELOAD=1 PORT_API=1"
    # test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=cint16  FIR_LEN=24 SHIFT=15 INPUT_WINDOW_VSIZE=256 NUM_OUTPUTS=1 CASC_LEN=3 USE_COEFF_RELOAD=0 PORT_API=1"
    # test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=cint16  FIR_LEN=24 SHIFT=15 INPUT_WINDOW_VSIZE=256 NUM_OUTPUTS=1 CASC_LEN=3 USE_COEFF_RELOAD=1 PORT_API=1"
    # test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=cint16  FIR_LEN=24 SHIFT=15 INPUT_WINDOW_VSIZE=256 NUM_OUTPUTS=2 CASC_LEN=1 USE_COEFF_RELOAD=0 PORT_API=1"
    test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=cint16  FIR_LEN=24 SHIFT=15 INPUT_WINDOW_VSIZE=256 NUM_OUTPUTS=2 CASC_LEN=1 USE_COEFF_RELOAD=1 PORT_API=1 DUAL_IP=1"
    test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=cint16  FIR_LEN=24 SHIFT=15 INPUT_WINDOW_VSIZE=256 NUM_OUTPUTS=2 CASC_LEN=3 USE_COEFF_RELOAD=0 PORT_API=1 DUAL_IP=1"
    test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=cint16  FIR_LEN=24 SHIFT=15 INPUT_WINDOW_VSIZE=256 NUM_OUTPUTS=2 CASC_LEN=3 USE_COEFF_RELOAD=1 PORT_API=1 DUAL_IP=1"
    #16-bit data 32-bit coeffs
    test_arr[$LINENO]="make all DATA_TYPE=int16 COEFF_TYPE=int32   FIR_LEN=80 CASC_LEN=3 INPUT_WINDOW_VSIZE=256 USE_COEFF_RELOAD=1 SHIFT=24 "
    test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=int32  FIR_LEN=40 CASC_LEN=3 INPUT_WINDOW_VSIZE=256 USE_COEFF_RELOAD=1 SHIFT=24 "
    test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=cint32 FIR_LEN=40 CASC_LEN=3 INPUT_WINDOW_VSIZE=256 USE_COEFF_RELOAD=1 SHIFT=24 "
    test_arr[$LINENO]="make all DATA_TYPE=int16 COEFF_TYPE=int32   FIR_LEN=48 SHIFT=24 INPUT_WINDOW_VSIZE=256 PORT_API=1 CASC_LEN=1"
    test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=int32  FIR_LEN=24 SHIFT=24 INPUT_WINDOW_VSIZE=256 PORT_API=1 CASC_LEN=1"
    test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=cint32 FIR_LEN=24 SHIFT=24 INPUT_WINDOW_VSIZE=256 PORT_API=1 CASC_LEN=1"
    test_arr[$LINENO]="make all DATA_TYPE=int16 COEFF_TYPE=int32   FIR_LEN=48 SHIFT=24 INPUT_WINDOW_VSIZE=256 PORT_API=1 CASC_LEN=3"
    test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=int32  FIR_LEN=24 SHIFT=24 INPUT_WINDOW_VSIZE=256 PORT_API=1 CASC_LEN=3"
    # test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=cint32 FIR_LEN=24 SHIFT=24 INPUT_WINDOW_VSIZE=256 PORT_API=1 CASC_LEN=3"
    #long FIR
    test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=int16  FIR_LEN=1000 CASC_LEN=4 SHIFT=16"
    test_arr[$LINENO]="make all DATA_TYPE=int32 COEFF_TYPE=int16 FIR_LEN=24 SHIFT=15 INPUT_WINDOW_VSIZE=256 CASC_LEN=1 USE_COEFF_RELOAD=0 NUM_OUTPUTS=1 DUAL_IP=1"
    # ADL-953
    test_arr[$LINENO]="make all   DATA_TYPE=cint32 COEFF_TYPE=cint32 FIR_LEN=8 SHIFT=16 INPUT_WINDOW_VSIZE=256 CASC_LEN=1 NUM_OUTPUTS=2 PORT_API=1 DUAL_IP=1"
    # ssr
    test_arr[$LINENO]="make all PORT_API=1 NITER=2 UUT_SSR=2 FIR_LEN=48 CASC_LEN=3 INPUT_WINDOW_VSIZE=25600"

    #SSR Cases
    test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=int16 FIR_LEN=96 SHIFT=16 INPUT_WINDOW_VSIZE=7680 PORT_API=1 CASC_LEN=2 UUT_SSR=3 DUAL_IP=0 NUM_OUTPUTS=1"
    test_arr[$LINENO]="make all DATA_TYPE=cfloat COEFF_TYPE=cfloat FIR_LEN=16 SHIFT=0 INPUT_WINDOW_VSIZE=5120 PORT_API=1 CASC_LEN=4 UUT_SSR=2 DUAL_IP=1 NUM_OUTPUTS=2"

    #RTP based Coefficient Reload - streaming API - only non_SSR supported
    test_arr[$LINENO]="make all DATA_TYPE=int16 COEFF_TYPE=int16  NITER=4 FIR_LEN=32  CASC_LEN=1 INPUT_WINDOW_VSIZE=2560 PORT_API=1 UUT_SSR=1 DUAL_IP=0 NUM_OUTPUTS=1 USE_COEFF_RELOAD=1 "

    #Coefficient Reload -  data/coeff type mix
    test_arr[$LINENO]="make all DATA_TYPE=int16 COEFF_TYPE=int16  NITER=4 FIR_LEN=32  CASC_LEN=1 INPUT_WINDOW_VSIZE=2560 PORT_API=1 UUT_SSR=2 DUAL_IP=0 NUM_OUTPUTS=1 USE_COEFF_RELOAD=1 "
    test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=cint16 NITER=4 FIR_LEN=32  CASC_LEN=1 INPUT_WINDOW_VSIZE=2560 PORT_API=1 UUT_SSR=4 DUAL_IP=0 NUM_OUTPUTS=1 USE_COEFF_RELOAD=1 "
    test_arr[$LINENO]="make all DATA_TYPE=cint32 COEFF_TYPE=cint16 NITER=4 FIR_LEN=16  CASC_LEN=1 INPUT_WINDOW_VSIZE=2560 PORT_API=1 UUT_SSR=2 DUAL_IP=0 NUM_OUTPUTS=1 USE_COEFF_RELOAD=1 "
    test_arr[$LINENO]="make all DATA_TYPE=cint32 COEFF_TYPE=cint32 NITER=4 FIR_LEN=16  CASC_LEN=1 INPUT_WINDOW_VSIZE=2560 PORT_API=1 UUT_SSR=2 DUAL_IP=0 NUM_OUTPUTS=1 USE_COEFF_RELOAD=1 "
    test_arr[$LINENO]="make all DATA_TYPE=cfloat COEFF_TYPE=float  NITER=4 FIR_LEN=8   CASC_LEN=1 INPUT_WINDOW_VSIZE=2560 PORT_API=1 UUT_SSR=2 DUAL_IP=0 NUM_OUTPUTS=1 USE_COEFF_RELOAD=1 SHIFT=0 "

    test_arr[$LINENO]="make all DATA_TYPE=cfloat COEFF_TYPE=float  NITER=4 FIR_LEN=8   CASC_LEN=1 INPUT_WINDOW_VSIZE=2560 PORT_API=1 UUT_SSR=2 DUAL_IP=0 NUM_OUTPUTS=1 USE_COEFF_RELOAD=1 SHIFT=0 "
    #Coefficient Reload -  casc len 2
    test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=int16  NITER=4 FIR_LEN=32  CASC_LEN=2 INPUT_WINDOW_VSIZE=2560 PORT_API=1 UUT_SSR=2 DUAL_IP=0 NUM_OUTPUTS=1 USE_COEFF_RELOAD=1 "
    test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=int16  NITER=4 FIR_LEN=32  CASC_LEN=2 INPUT_WINDOW_VSIZE=2560 PORT_API=1 UUT_SSR=2 DUAL_IP=1 NUM_OUTPUTS=2 USE_COEFF_RELOAD=1 "
    #Coefficient Reload -  2 streams
    # test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=cint16 NITER=4 FIR_LEN=32  CASC_LEN=1 INPUT_WINDOW_VSIZE=2560 PORT_API=1 UUT_SSR=4 DUAL_IP=1 NUM_OUTPUTS=2 USE_COEFF_RELOAD=1 "
    # overwrite default fifo_depth
    # test_arr[$LINENO]="make all FIR_LEN=32 PORT_API=1 CASC_LEN=3 UUT_SSR=2 INPUT_WINDOW_VSIZE=512 DUAL_IP=1 NUM_OUTPUTS=2 USE_CUSTOM_CONSTRAINT=1"
    # float case for 1buff and 2buffs
    test_arr[$LINENO]="make all DATA_TYPE=float COEFF_TYPE=float FIR_LEN=16 CASC_LEN=1 SHIFT=0 "
    test_arr[$LINENO]="make all DATA_TYPE=cfloat COEFF_TYPE=float FIR_LEN=16 CASC_LEN=1 SHIFT=0 "

    test_arr[$LINENO]="make all DATA_TYPE=int32 COEFF_TYPE=int32 FIR_LEN=40 CASC_LEN=3 SHIFT=16 INPUT_WINDOW_VSIZE=256 PORT_API=1 DUAL_IP=1 USE_COEFF_RELOAD=0 NUM_OUTPUTS=2"
    test_arr[$LINENO]="make all DATA_TYPE=int32 COEFF_TYPE=int32 FIR_LEN=40 CASC_LEN=3 SHIFT=16 INPUT_WINDOW_VSIZE=256 PORT_API=1 DUAL_IP=1 USE_COEFF_RELOAD=1 NUM_OUTPUTS=2"

    #AIE2
    test_arr[$LINENO]="make all AIE_VARIANT=2"
    test_arr[$LINENO]="make all AIE_VARIANT=2 DATA_TYPE=cint16 COEFF_TYPE=int16  FIR_LEN=36 CASC_LEN=1 SHIFT=16"
    test_arr[$LINENO]="make all AIE_VARIANT=2 DATA_TYPE=cint16 COEFF_TYPE=int16 FIR_LEN=89 SHIFT=15 CASC_LEN=2 INPUT_WINDOW_VSIZE=256 "
    test_arr[$LINENO]="make all AIE_VARIANT=2 DATA_TYPE=cint16 COEFF_TYPE=int16 FIR_LEN=8   CASC_LEN=1 SHIFT=16 ROUND_MODE=1 "
    test_arr[$LINENO]="make all AIE_VARIANT=2 DATA_TYPE=cint16 COEFF_TYPE=int16 FIR_LEN=8   CASC_LEN=1 SHIFT=16 ROUND_MODE=1 SAT_MODE=0"
    test_arr[$LINENO]="make all AIE_VARIANT=2 DATA_TYPE=cint16 COEFF_TYPE=int16 FIR_LEN=8   CASC_LEN=1 SHIFT=16 ROUND_MODE=1 SAT_MODE=1"
    test_arr[$LINENO]="make all AIE_VARIANT=2 DATA_TYPE=cint16 COEFF_TYPE=int16 FIR_LEN=8   CASC_LEN=1 SHIFT=16 ROUND_MODE=1 SAT_MODE=3"
    test_arr[$LINENO]="make all AIE_VARIANT=2 DATA_TYPE=int16 COEFF_TYPE=int16 FIR_LEN=80 CASC_LEN=3 INPUT_WINDOW_VSIZE=256 USE_COEFF_RELOAD=1 SHIFT=16 "
    test_arr[$LINENO]="make all AIE_VARIANT=2 DATA_TYPE=cint16 COEFF_TYPE=cint16  FIR_LEN=24 SHIFT=15 INPUT_WINDOW_VSIZE=256 CASC_LEN=1 USE_COEFF_RELOAD=0 NUM_OUTPUTS=2"
    test_arr[$LINENO]="make all AIE_VARIANT=2 DATA_TYPE=cint16 COEFF_TYPE=cint16  FIR_LEN=24 SHIFT=15 INPUT_WINDOW_VSIZE=256 NUM_OUTPUTS=1 CASC_LEN=1 USE_COEFF_RELOAD=0 PORT_API=1"
    test_arr[$LINENO]="make all AIE_VARIANT=2 DATA_TYPE=cint16 COEFF_TYPE=cint16  FIR_LEN=24 SHIFT=15 INPUT_WINDOW_VSIZE=256 NUM_OUTPUTS=1 CASC_LEN=3 USE_COEFF_RELOAD=0 PORT_API=1"
    test_arr[$LINENO]="make all AIE_VARIANT=2 DATA_TYPE=cint16 COEFF_TYPE=int16  FIR_LEN=1000 CASC_LEN=4 SHIFT=16"
    test_arr[$LINENO]="make all AIE_VARIANT=2 DATA_TYPE=int16  COEFF_TYPE=int16  NITER=4 FIR_LEN=32  CASC_LEN=1 INPUT_WINDOW_VSIZE=2560 PORT_API=1 UUT_SSR=1 DUAL_IP=0 NUM_OUTPUTS=1 USE_COEFF_RELOAD=1 "
    test_arr[$LINENO]="make all AIE_VARIANT=2 DATA_TYPE=cint16 COEFF_TYPE=int16  NITER=4 FIR_LEN=32  CASC_LEN=2 INPUT_WINDOW_VSIZE=2560 PORT_API=1 UUT_SSR=1 DUAL_IP=0 NUM_OUTPUTS=1 USE_COEFF_RELOAD=1 "

    #AIE2
    test_arr[$LINENO]="make all AIE_VARIANT=22"
    test_arr[$LINENO]="make all AIE_VARIANT=22 DATA_TYPE=cint16 COEFF_TYPE=int16  FIR_LEN=36 CASC_LEN=1 SHIFT=16"
    test_arr[$LINENO]="make all AIE_VARIANT=22 DATA_TYPE=cint16 COEFF_TYPE=int16 FIR_LEN=89 SHIFT=15 CASC_LEN=2 INPUT_WINDOW_VSIZE=256 "
    test_arr[$LINENO]="make all AIE_VARIANT=22 DATA_TYPE=cint16 COEFF_TYPE=int16 FIR_LEN=8   CASC_LEN=1 SHIFT=16 ROUND_MODE=1 "
    test_arr[$LINENO]="make all AIE_VARIANT=22 DATA_TYPE=cint16 COEFF_TYPE=int16 FIR_LEN=8   CASC_LEN=1 SHIFT=16 ROUND_MODE=1 SAT_MODE=0"
    test_arr[$LINENO]="make all AIE_VARIANT=22 DATA_TYPE=cint16 COEFF_TYPE=int16 FIR_LEN=8   CASC_LEN=1 SHIFT=16 ROUND_MODE=1 SAT_MODE=1"
    test_arr[$LINENO]="make all AIE_VARIANT=22 DATA_TYPE=cint16 COEFF_TYPE=int16 FIR_LEN=8   CASC_LEN=1 SHIFT=16 ROUND_MODE=1 SAT_MODE=3"
    test_arr[$LINENO]="make all AIE_VARIANT=22 DATA_TYPE=int16 COEFF_TYPE=int16 FIR_LEN=80 CASC_LEN=3 INPUT_WINDOW_VSIZE=256 USE_COEFF_RELOAD=1 SHIFT=16 "
    test_arr[$LINENO]="make all AIE_VARIANT=22 DATA_TYPE=cint16 COEFF_TYPE=cint16  FIR_LEN=24 SHIFT=15 INPUT_WINDOW_VSIZE=256 CASC_LEN=1 USE_COEFF_RELOAD=0 NUM_OUTPUTS=2"
    test_arr[$LINENO]="make all AIE_VARIANT=22 DATA_TYPE=cint16 COEFF_TYPE=cint16  FIR_LEN=24 SHIFT=15 INPUT_WINDOW_VSIZE=256 NUM_OUTPUTS=1 CASC_LEN=1 USE_COEFF_RELOAD=0 PORT_API=1"
    test_arr[$LINENO]="make all AIE_VARIANT=22 DATA_TYPE=cint16 COEFF_TYPE=cint16  FIR_LEN=24 SHIFT=15 INPUT_WINDOW_VSIZE=256 NUM_OUTPUTS=1 CASC_LEN=3 USE_COEFF_RELOAD=0 PORT_API=1"
    test_arr[$LINENO]="make all AIE_VARIANT=22 DATA_TYPE=cint16 COEFF_TYPE=int16  FIR_LEN=1000 CASC_LEN=4 SHIFT=16"
    test_arr[$LINENO]="make all AIE_VARIANT=22 DATA_TYPE=int16  COEFF_TYPE=int16  NITER=4 FIR_LEN=32  CASC_LEN=1 INPUT_WINDOW_VSIZE=2560 PORT_API=1 UUT_SSR=1 DUAL_IP=0 NUM_OUTPUTS=1 USE_COEFF_RELOAD=1 "
    test_arr[$LINENO]="make all AIE_VARIANT=22 DATA_TYPE=cint16 COEFF_TYPE=int16  NITER=4 FIR_LEN=32  CASC_LEN=2 INPUT_WINDOW_VSIZE=2560 PORT_API=1 UUT_SSR=1 DUAL_IP=0 NUM_OUTPUTS=1 USE_COEFF_RELOAD=1 "
fi

#Qor tests should be added to fit a suite with a collective purpose. There may be multiple suites with different purposes, e.g.
#Parameter exploration - where a base case is augmented by additional cases which vary one parameter such that the user
#  can see how QoR varies with a change in that parameter.
#Bellwether - standalone cases which describe a known or typical customer usecase, e.g. 47tap HB FIR with cint16/int16
#Anomaly. Where there is a known anomaly in QoR, such as a step change at a particular FIR length, cases may be added
# to highlight the non-linearity.
#Other. If there are other cases or other suites added for a different purpose, please add a description of the suite here.
if [[ "$*" == *qor* ]]
then

    ## Copy and paste from fir_sr_asym, as it gets called by the graph anyway.
    ##AIE2 tests - type support
    #test_arr[$LINENO]="make all AIE_VARIANT=2 DATA_TYPE=int16  COEFF_TYPE=int16  FIR_LEN=4  SHIFT=16 CASC_LEN=1 INPUT_WINDOW_VSIZE=2048"
    #test_arr[$LINENO]="make all AIE_VARIANT=2 DATA_TYPE=int16  COEFF_TYPE=int16  FIR_LEN=8  SHIFT=16 CASC_LEN=1 INPUT_WINDOW_VSIZE=2048"
    #test_arr[$LINENO]="make all AIE_VARIANT=2 DATA_TYPE=int16  COEFF_TYPE=int16  FIR_LEN=16 SHIFT=16 CASC_LEN=1 INPUT_WINDOW_VSIZE=2048"
#
    #test_arr[$LINENO]="make all AIE_VARIANT=2 DATA_TYPE=cint16 COEFF_TYPE=int16  FIR_LEN=4  SHIFT=16 CASC_LEN=1 INPUT_WINDOW_VSIZE=2048"
    #test_arr[$LINENO]="make all AIE_VARIANT=2 DATA_TYPE=cint16 COEFF_TYPE=int16  FIR_LEN=8  SHIFT=16 CASC_LEN=1 INPUT_WINDOW_VSIZE=2048"
    #test_arr[$LINENO]="make all AIE_VARIANT=2 DATA_TYPE=cint16 COEFF_TYPE=int16  FIR_LEN=16 SHIFT=16 CASC_LEN=1 INPUT_WINDOW_VSIZE=2048"
#
    #test_arr[$LINENO]="make all AIE_VARIANT=2 DATA_TYPE=cint16 COEFF_TYPE=cint16 FIR_LEN=4  SHIFT=16 CASC_LEN=1 INPUT_WINDOW_VSIZE=2048"
    #test_arr[$LINENO]="make all AIE_VARIANT=2 DATA_TYPE=cint16 COEFF_TYPE=cint16 FIR_LEN=8  SHIFT=16 CASC_LEN=1 INPUT_WINDOW_VSIZE=2048"
    #test_arr[$LINENO]="make all AIE_VARIANT=2 DATA_TYPE=cint16 COEFF_TYPE=cint16 FIR_LEN=16 SHIFT=16 CASC_LEN=1 INPUT_WINDOW_VSIZE=2048"
#
    ## streaming cases
    #test_arr[$LINENO]="make all AIE_VARIANT=2 DATA_TYPE=int16  COEFF_TYPE=int16  FIR_LEN=4  SHIFT=16 CASC_LEN=1 PORT_API=1 INPUT_WINDOW_VSIZE=2560"
    #test_arr[$LINENO]="make all AIE_VARIANT=2 DATA_TYPE=int16  COEFF_TYPE=int16  FIR_LEN=8  SHIFT=16 CASC_LEN=1 PORT_API=1 INPUT_WINDOW_VSIZE=2560"
    #test_arr[$LINENO]="make all AIE_VARIANT=2 DATA_TYPE=int16  COEFF_TYPE=int16  FIR_LEN=16 SHIFT=16 CASC_LEN=1 PORT_API=1 INPUT_WINDOW_VSIZE=2560"
#
    #test_arr[$LINENO]="make all AIE_VARIANT=2 DATA_TYPE=cint16 COEFF_TYPE=int16  FIR_LEN=4  SHIFT=16 CASC_LEN=1 PORT_API=1 INPUT_WINDOW_VSIZE=2560"
    #test_arr[$LINENO]="make all AIE_VARIANT=2 DATA_TYPE=cint16 COEFF_TYPE=int16  FIR_LEN=8  SHIFT=16 CASC_LEN=1 PORT_API=1 INPUT_WINDOW_VSIZE=2560"
    #test_arr[$LINENO]="make all AIE_VARIANT=2 DATA_TYPE=cint16 COEFF_TYPE=int16  FIR_LEN=16 SHIFT=16 CASC_LEN=1 PORT_API=1 INPUT_WINDOW_VSIZE=2560"
#
    #test_arr[$LINENO]="make all AIE_VARIANT=2 DATA_TYPE=cint16 COEFF_TYPE=cint16 FIR_LEN=4  SHIFT=16 CASC_LEN=1 PORT_API=1 INPUT_WINDOW_VSIZE=2560"
    #test_arr[$LINENO]="make all AIE_VARIANT=2 DATA_TYPE=cint16 COEFF_TYPE=cint16 FIR_LEN=8  SHIFT=16 CASC_LEN=1 PORT_API=1 INPUT_WINDOW_VSIZE=2560"
    #test_arr[$LINENO]="make all AIE_VARIANT=2 DATA_TYPE=cint16 COEFF_TYPE=cint16 FIR_LEN=16 SHIFT=16 CASC_LEN=1 PORT_API=1 INPUT_WINDOW_VSIZE=2560"
#
    #test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=int16  DUAL_IP=1 FIR_LEN=199 SHIFT=20 INPUT_WINDOW_VSIZE=256    NITER=8"
    ## fir_129t_sym
    #test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=int16  DUAL_IP=1 FIR_LEN=129 SHIFT=15 INPUT_WINDOW_VSIZE=256    NITER=8"
    ## fir_89t_sym
    #test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=int16  DUAL_IP=1 FIR_LEN=89 SHIFT=15 INPUT_WINDOW_VSIZE=256    NITER=8"
    ## fir_63t_sym
    #test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=int16  DUAL_IP=1 FIR_LEN=63 SHIFT=15 INPUT_WINDOW_VSIZE=256    NITER=8"
    ## fir_30t_cplx_3core - window based with dual_ip
    #test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=cint16 DUAL_IP=1 FIR_LEN=30 SHIFT=15 INPUT_WINDOW_VSIZE=512 CASC_LEN=3 PORT_API=0   NITER=8"
    ## fir_30t_cplx_3core - stream based with single_ip - dual_ip don't make a difference (not implemented for this case?)
    #test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=cint16 DUAL_IP=0 FIR_LEN=30 SHIFT=15 INPUT_WINDOW_VSIZE=512 CASC_LEN=3 PORT_API=1   NITER=8"
    ## fir_24t_cplx
    #test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=cint16 DUAL_IP=0 FIR_LEN=24 SHIFT=15 INPUT_WINDOW_VSIZE=256    NITER=8"
    ## fir_24t_cplx_2core
    #test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=cint16 DUAL_IP=0 FIR_LEN=24 SHIFT=15 INPUT_WINDOW_VSIZE=256 CASC_LEN=2 PORT_API=1   NITER=8"
    ## fir_16t_sym
    #test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=int16 DUAL_IP=0 FIR_LEN=16 SHIFT=15 INPUT_WINDOW_VSIZE=256    NITER=8"
    ##fir_96t_real_sym (input window is cint16, but they cast to int16)
    #test_arr[$LINENO]="make all DATA_TYPE=int16 COEFF_TYPE=int16 DUAL_IP=0 FIR_LEN=96 SHIFT=20 INPUT_WINDOW_VSIZE=512    NITER=8"
#
    ##cases described in docs
    #test_arr[$LINENO]="make all DATA_TYPE=int32 COEFF_TYPE=int16  FIR_LEN=4  CASC_LEN=1 PORT_API=0 INPUT_WINDOW_VSIZE=2560   NITER=8"
    #test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=int16  FIR_LEN=16  CASC_LEN=1 PORT_API=0 INPUT_WINDOW_VSIZE=512 SHIFT=7   NITER=8"
    #test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=int16  FIR_LEN=32 UUT_SSR=2  CASC_LEN=1 PORT_API=1 INPUT_WINDOW_VSIZE=25600 SHIFT=7   NITER=8"
#
    ##Now to explore the parameter space
    ##int16/int16
    #test_arr[$LINENO]="make all DATA_TYPE=int16 COEFF_TYPE=int16  FIR_LEN=16  CASC_LEN=1 SHIFT=16    NITER=8"
    #test_arr[$LINENO]="make all DATA_TYPE=int16 COEFF_TYPE=int16  FIR_LEN=32  CASC_LEN=1 SHIFT=16    NITER=8"
#
    ##cint16/int16 - considered the primary type combination, so other parameters are explored for this
    ##explore FIR_LEN
    #test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=int16  FIR_LEN=8   CASC_LEN=1 SHIFT=16    NITER=8"
    #test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=int16  FIR_LEN=16  CASC_LEN=1 SHIFT=16    NITER=8"
    #test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=int16  FIR_LEN=32  CASC_LEN=1 SHIFT=16    NITER=8"
    #test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=int16  FIR_LEN=64  CASC_LEN=1 SHIFT=16    NITER=8"
    #test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=int16  FIR_LEN=128 CASC_LEN=1 SHIFT=16    NITER=8"
    #test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=int16  FIR_LEN=240 CASC_LEN=1 SHIFT=16    NITER=8"
    #test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=int16  FIR_LEN=512 CASC_LEN=1 SHIFT=16    NITER=8"
#
    ##explore big FIR_LENs
    #test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=int16  FIR_LEN=1024 CASC_LEN=2 SHIFT=16    NITER=8"
    #test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=int16  FIR_LEN=2048 CASC_LEN=4 SHIFT=16    NITER=8"
    #test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=int16  FIR_LEN=4096 CASC_LEN=8 SHIFT=16    NITER=8"
#
    ##explore big int16 FIR_LENs
    #test_arr[$LINENO]="make all DATA_TYPE=int16 COEFF_TYPE=int16  FIR_LEN=1024 CASC_LEN=2  SHIFT=16    NITER=8"
    #test_arr[$LINENO]="make all DATA_TYPE=int16 COEFF_TYPE=int16  FIR_LEN=2048 CASC_LEN=4  SHIFT=16    NITER=8"
    #test_arr[$LINENO]="make all DATA_TYPE=int16 COEFF_TYPE=int16  FIR_LEN=4096 CASC_LEN=8  SHIFT=16    NITER=8"
    #test_arr[$LINENO]="make all DATA_TYPE=int16 COEFF_TYPE=int16  FIR_LEN=8192 CASC_LEN=16 SHIFT=16    NITER=8"
#
    ##explore FIR_LEN with RTPs
    #test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=int16  FIR_LEN=8    CASC_LEN=1 SHIFT=16 USE_COEFF_RELOAD=1   NITER=8"
    #test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=int16  FIR_LEN=16   CASC_LEN=1 SHIFT=16 USE_COEFF_RELOAD=1   NITER=8"
    #test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=int16  FIR_LEN=32   CASC_LEN=1 SHIFT=16 USE_COEFF_RELOAD=1   NITER=8"
    #test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=int16  FIR_LEN=64   CASC_LEN=1 SHIFT=16 USE_COEFF_RELOAD=1   NITER=8"
    #test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=int16  FIR_LEN=128  CASC_LEN=1 SHIFT=16 USE_COEFF_RELOAD=1   NITER=8"
    #test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=int16  FIR_LEN=240  CASC_LEN=1 SHIFT=16 USE_COEFF_RELOAD=1   NITER=8"
#
    ##Explore shift (not expected to make any difference to QoR)
    #test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=int16  FIR_LEN=8   CASC_LEN=1 SHIFT=20    NITER=8"
#
    ##Explore rounding (not expected to make any difference to QoR)
    #test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=int16  FIR_LEN=8   CASC_LEN=1 SHIFT=16 ROUND_MODE=1    NITER=8"
#
    ##Explore window size
    #test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=int16  FIR_LEN=8   CASC_LEN=1 SHIFT=16 INPUT_WINDOW_VSIZE=64     NITER=8"
    #test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=int16  FIR_LEN=8   CASC_LEN=1 SHIFT=16 INPUT_WINDOW_VSIZE=128    NITER=8"
#
    ##Explore cascade length
    #test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=int16  FIR_LEN=128 CASC_LEN=2 SHIFT=16    NITER=8"
    #test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=int16  FIR_LEN=128 CASC_LEN=3 SHIFT=16    NITER=8"
    #test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=int16  FIR_LEN=128 CASC_LEN=4 SHIFT=16    NITER=8"
    #test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=int16  FIR_LEN=128 CASC_LEN=5 SHIFT=16    NITER=8"
#
    ##explore port type
    #test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=int16  FIR_LEN=128 CASC_LEN=1 SHIFT=16 PORT_API=1   NITER=8"
    #test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=int16  FIR_LEN=128 CASC_LEN=2 SHIFT=16 PORT_API=1   NITER=8"
    #test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=int16  FIR_LEN=128 CASC_LEN=3 SHIFT=16 PORT_API=1   NITER=8"
    #test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=int16  FIR_LEN=128 CASC_LEN=4 SHIFT=16 PORT_API=1   NITER=8"
    #test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=int16  FIR_LEN=128 CASC_LEN=5 SHIFT=16 PORT_API=1   NITER=8"


    #explore data combos.
    test_arr[$LINENO]="make all  DATA_TYPE=cint16 COEFF_TYPE=int16  FIR_LEN=8   CASC_LEN=1 SHIFT=16 INPUT_WINDOW_VSIZE=64     NITER=8"
    test_arr[$LINENO]="make all  DATA_TYPE=cint16 COEFF_TYPE=int16  FIR_LEN=8   CASC_LEN=1 SHIFT=16 INPUT_WINDOW_VSIZE=64     NITER=8"

    #int16/int32
    test_arr[$LINENO]="make all  DATA_TYPE=int16 COEFF_TYPE=int32   FIR_LEN=16  CASC_LEN=1 SHIFT=16     NITER=8"
    test_arr[$LINENO]="make all  DATA_TYPE=int16 COEFF_TYPE=int32   FIR_LEN=32  CASC_LEN=1 SHIFT=16     NITER=8"

    #cint16/int32
    test_arr[$LINENO]="make all  DATA_TYPE=cint16 COEFF_TYPE=int32  FIR_LEN=16  CASC_LEN=1 SHIFT=16     NITER=8"
    test_arr[$LINENO]="make all  DATA_TYPE=cint16 COEFF_TYPE=int32  FIR_LEN=32  CASC_LEN=1 SHIFT=16     NITER=8"

    #cint16/cint32
    test_arr[$LINENO]="make all  DATA_TYPE=cint16 COEFF_TYPE=cint32 FIR_LEN=16  CASC_LEN=1 SHIFT=16     NITER=8"
    test_arr[$LINENO]="make all  DATA_TYPE=cint16 COEFF_TYPE=cint32 FIR_LEN=32  CASC_LEN=1 SHIFT=16     NITER=8"

    #cint16/cint16
    test_arr[$LINENO]="make all  DATA_TYPE=cint16 COEFF_TYPE=cint16  FIR_LEN=16  CASC_LEN=1 SHIFT=16    NITER=8"
    test_arr[$LINENO]="make all  DATA_TYPE=cint16 COEFF_TYPE=cint16  FIR_LEN=32  CASC_LEN=1 SHIFT=16    NITER=8"

    #int32/int16
    test_arr[$LINENO]="make all  DATA_TYPE=int32 COEFF_TYPE=int16   FIR_LEN=16  CASC_LEN=1 SHIFT=16    NITER=8"
    test_arr[$LINENO]="make all  DATA_TYPE=int32 COEFF_TYPE=int16   FIR_LEN=32  CASC_LEN=1 SHIFT=16    NITER=8"

    #int32/int32
    test_arr[$LINENO]="make all  DATA_TYPE=int32 COEFF_TYPE=int32   FIR_LEN=16  CASC_LEN=1 SHIFT=16    NITER=8"
    test_arr[$LINENO]="make all  DATA_TYPE=int32 COEFF_TYPE=int32   FIR_LEN=32  CASC_LEN=1 SHIFT=16    NITER=8"

    #cint32/int16
    test_arr[$LINENO]="make all  DATA_TYPE=cint32 COEFF_TYPE=int16   FIR_LEN=16  CASC_LEN=1 SHIFT=16    NITER=8"
    test_arr[$LINENO]="make all  DATA_TYPE=cint32 COEFF_TYPE=int16   FIR_LEN=32  CASC_LEN=1 SHIFT=16    NITER=8"

    #cint32/cint16
    test_arr[$LINENO]="make all  DATA_TYPE=cint32 COEFF_TYPE=cint16  FIR_LEN=16  CASC_LEN=1 SHIFT=16    NITER=8"
    test_arr[$LINENO]="make all  DATA_TYPE=cint32 COEFF_TYPE=cint16  FIR_LEN=32  CASC_LEN=1 SHIFT=16    NITER=8"

    #cint32/int32
    test_arr[$LINENO]="make all  DATA_TYPE=cint32 COEFF_TYPE=int32   FIR_LEN=16  CASC_LEN=1 SHIFT=16    NITER=8"
    test_arr[$LINENO]="make all  DATA_TYPE=cint32 COEFF_TYPE=int32   FIR_LEN=32  CASC_LEN=1 SHIFT=16    NITER=8"

    #cint32/cint32
    test_arr[$LINENO]="make all  DATA_TYPE=cint32 COEFF_TYPE=cint32  FIR_LEN=16  CASC_LEN=1 SHIFT=16    NITER=8"
    test_arr[$LINENO]="make all  DATA_TYPE=cint32 COEFF_TYPE=cint32  FIR_LEN=32  CASC_LEN=1 SHIFT=16    NITER=8"

    #float/float
    test_arr[$LINENO]="make all  DATA_TYPE=float COEFF_TYPE=float   FIR_LEN=16  CASC_LEN=1 SHIFT=0    NITER=8"
    test_arr[$LINENO]="make all  DATA_TYPE=float COEFF_TYPE=float   FIR_LEN=32  CASC_LEN=1 SHIFT=0    NITER=8"

    #cfloat/float
    test_arr[$LINENO]="make all  DATA_TYPE=cfloat COEFF_TYPE=float   FIR_LEN=16  CASC_LEN=1 SHIFT=0    NITER=8"
    test_arr[$LINENO]="make all  DATA_TYPE=cfloat COEFF_TYPE=float   FIR_LEN=32  CASC_LEN=1 SHIFT=0    NITER=8"

    #cfloat/cfloat
    test_arr[$LINENO]="make all  DATA_TYPE=cfloat COEFF_TYPE=cfloat  FIR_LEN=16  CASC_LEN=1 SHIFT=0    NITER=8"
    test_arr[$LINENO]="make all  DATA_TYPE=cfloat COEFF_TYPE=cfloat  FIR_LEN=32  CASC_LEN=1 SHIFT=0    NITER=8"

    # AIE VARIANT 2
    #explore data combos.
    test_arr[$LINENO]="make all  AIE_VARIANT=2 DATA_TYPE=cint16 COEFF_TYPE=int16  FIR_LEN=8   CASC_LEN=1 SHIFT=16 INPUT_WINDOW_VSIZE=64     NITER=8"
    test_arr[$LINENO]="make all  AIE_VARIANT=2 DATA_TYPE=cint16 COEFF_TYPE=int16  FIR_LEN=8   CASC_LEN=1 SHIFT=16 INPUT_WINDOW_VSIZE=64     NITER=8"

    #int16/int32
    test_arr[$LINENO]="make all  AIE_VARIANT=2 DATA_TYPE=int16 COEFF_TYPE=int32   FIR_LEN=16  CASC_LEN=1 SHIFT=16     NITER=8"
    test_arr[$LINENO]="make all  AIE_VARIANT=2 DATA_TYPE=int16 COEFF_TYPE=int32   FIR_LEN=32  CASC_LEN=1 SHIFT=16     NITER=8"

    #cint16/int32
    test_arr[$LINENO]="make all  AIE_VARIANT=2 DATA_TYPE=cint16 COEFF_TYPE=int32  FIR_LEN=16  CASC_LEN=1 SHIFT=16     NITER=8"
    test_arr[$LINENO]="make all  AIE_VARIANT=2 DATA_TYPE=cint16 COEFF_TYPE=int32  FIR_LEN=32  CASC_LEN=1 SHIFT=16     NITER=8"

    #cint16/cint32
    test_arr[$LINENO]="make all  AIE_VARIANT=2 DATA_TYPE=cint16 COEFF_TYPE=cint32 FIR_LEN=16  CASC_LEN=1 SHIFT=16     NITER=8"
    test_arr[$LINENO]="make all  AIE_VARIANT=2 DATA_TYPE=cint16 COEFF_TYPE=cint32 FIR_LEN=32  CASC_LEN=1 SHIFT=16     NITER=8"

    #cint16/cint16
    test_arr[$LINENO]="make all  AIE_VARIANT=2 DATA_TYPE=cint16 COEFF_TYPE=cint16  FIR_LEN=16  CASC_LEN=1 SHIFT=16    NITER=8"
    test_arr[$LINENO]="make all  AIE_VARIANT=2 DATA_TYPE=cint16 COEFF_TYPE=cint16  FIR_LEN=32  CASC_LEN=1 SHIFT=16    NITER=8"

    #int32/int16
    test_arr[$LINENO]="make all  AIE_VARIANT=2 DATA_TYPE=int32 COEFF_TYPE=int16   FIR_LEN=16  CASC_LEN=1 SHIFT=16    NITER=8"
    test_arr[$LINENO]="make all  AIE_VARIANT=2 DATA_TYPE=int32 COEFF_TYPE=int16   FIR_LEN=32  CASC_LEN=1 SHIFT=16    NITER=8"

    #int32/int32
    test_arr[$LINENO]="make all  AIE_VARIANT=2 DATA_TYPE=int32 COEFF_TYPE=int32   FIR_LEN=16  CASC_LEN=1 SHIFT=16    NITER=8"
    test_arr[$LINENO]="make all  AIE_VARIANT=2 DATA_TYPE=int32 COEFF_TYPE=int32   FIR_LEN=32  CASC_LEN=1 SHIFT=16    NITER=8"

    #cint32/int16
    test_arr[$LINENO]="make all  AIE_VARIANT=2 DATA_TYPE=cint32 COEFF_TYPE=int16   FIR_LEN=16  CASC_LEN=1 SHIFT=16    NITER=8"
    test_arr[$LINENO]="make all  AIE_VARIANT=2 DATA_TYPE=cint32 COEFF_TYPE=int16   FIR_LEN=32  CASC_LEN=1 SHIFT=16    NITER=8"

    #cint32/cint16
    test_arr[$LINENO]="make all  AIE_VARIANT=2 DATA_TYPE=cint32 COEFF_TYPE=cint16  FIR_LEN=16  CASC_LEN=1 SHIFT=16    NITER=8"
    test_arr[$LINENO]="make all  AIE_VARIANT=2 DATA_TYPE=cint32 COEFF_TYPE=cint16  FIR_LEN=32  CASC_LEN=1 SHIFT=16    NITER=8"

    #cint32/int32
    test_arr[$LINENO]="make all  AIE_VARIANT=2 DATA_TYPE=cint32 COEFF_TYPE=int32   FIR_LEN=16  CASC_LEN=1 SHIFT=16    NITER=8"
    test_arr[$LINENO]="make all  AIE_VARIANT=2 DATA_TYPE=cint32 COEFF_TYPE=int32   FIR_LEN=32  CASC_LEN=1 SHIFT=16    NITER=8"

    #cint32/cint32
    test_arr[$LINENO]="make all  AIE_VARIANT=2 DATA_TYPE=cint32 COEFF_TYPE=cint32  FIR_LEN=16  CASC_LEN=1 SHIFT=16    NITER=8"
    test_arr[$LINENO]="make all  AIE_VARIANT=2 DATA_TYPE=cint32 COEFF_TYPE=cint32  FIR_LEN=32  CASC_LEN=1 SHIFT=16    NITER=8"

    #float/float
    test_arr[$LINENO]="make all  AIE_VARIANT=2 DATA_TYPE=float COEFF_TYPE=float   FIR_LEN=16  CASC_LEN=1 SHIFT=0    NITER=8"
    test_arr[$LINENO]="make all  AIE_VARIANT=2 DATA_TYPE=float COEFF_TYPE=float   FIR_LEN=32  CASC_LEN=1 SHIFT=0    NITER=8"

    # AIE VARIANT 22
    #explore data combos.
    test_arr[$LINENO]="make all  AIE_VARIANT=22 DATA_TYPE=cint16 COEFF_TYPE=int16  FIR_LEN=8   CASC_LEN=1 SHIFT=16 INPUT_WINDOW_VSIZE=64     NITER=8"
    test_arr[$LINENO]="make all  AIE_VARIANT=22 DATA_TYPE=cint16 COEFF_TYPE=int16  FIR_LEN=8   CASC_LEN=1 SHIFT=16 INPUT_WINDOW_VSIZE=64     NITER=8"

    #int16/int32
    test_arr[$LINENO]="make all  AIE_VARIANT=22 DATA_TYPE=int16 COEFF_TYPE=int32   FIR_LEN=16  CASC_LEN=1 SHIFT=16     NITER=8"
    test_arr[$LINENO]="make all  AIE_VARIANT=22 DATA_TYPE=int16 COEFF_TYPE=int32   FIR_LEN=32  CASC_LEN=1 SHIFT=16     NITER=8"

    #cint16/int32
    test_arr[$LINENO]="make all  AIE_VARIANT=22 DATA_TYPE=cint16 COEFF_TYPE=int32  FIR_LEN=16  CASC_LEN=1 SHIFT=16     NITER=8"
    test_arr[$LINENO]="make all  AIE_VARIANT=22 DATA_TYPE=cint16 COEFF_TYPE=int32  FIR_LEN=32  CASC_LEN=1 SHIFT=16     NITER=8"

    #cint16/cint32
    test_arr[$LINENO]="make all  AIE_VARIANT=22 DATA_TYPE=cint16 COEFF_TYPE=cint32 FIR_LEN=16  CASC_LEN=1 SHIFT=16     NITER=8"
    test_arr[$LINENO]="make all  AIE_VARIANT=22 DATA_TYPE=cint16 COEFF_TYPE=cint32 FIR_LEN=32  CASC_LEN=1 SHIFT=16     NITER=8"

    #cint16/cint16
    test_arr[$LINENO]="make all  AIE_VARIANT=22 DATA_TYPE=cint16 COEFF_TYPE=cint16  FIR_LEN=16  CASC_LEN=1 SHIFT=16    NITER=8"
    test_arr[$LINENO]="make all  AIE_VARIANT=22 DATA_TYPE=cint16 COEFF_TYPE=cint16  FIR_LEN=32  CASC_LEN=1 SHIFT=16    NITER=8"

    #int32/int16
    test_arr[$LINENO]="make all  AIE_VARIANT=22 DATA_TYPE=int32 COEFF_TYPE=int16   FIR_LEN=16  CASC_LEN=1 SHIFT=16    NITER=8"
    test_arr[$LINENO]="make all  AIE_VARIANT=22 DATA_TYPE=int32 COEFF_TYPE=int16   FIR_LEN=32  CASC_LEN=1 SHIFT=16    NITER=8"

    #int32/int32
    test_arr[$LINENO]="make all  AIE_VARIANT=22 DATA_TYPE=int32 COEFF_TYPE=int32   FIR_LEN=16  CASC_LEN=1 SHIFT=16    NITER=8"
    test_arr[$LINENO]="make all  AIE_VARIANT=22 DATA_TYPE=int32 COEFF_TYPE=int32   FIR_LEN=32  CASC_LEN=1 SHIFT=16    NITER=8"

    #cint32/int16
    test_arr[$LINENO]="make all  AIE_VARIANT=22 DATA_TYPE=cint32 COEFF_TYPE=int16   FIR_LEN=16  CASC_LEN=1 SHIFT=16    NITER=8"
    test_arr[$LINENO]="make all  AIE_VARIANT=22 DATA_TYPE=cint32 COEFF_TYPE=int16   FIR_LEN=32  CASC_LEN=1 SHIFT=16    NITER=8"

    #cint32/cint16
    test_arr[$LINENO]="make all  AIE_VARIANT=22 DATA_TYPE=cint32 COEFF_TYPE=cint16  FIR_LEN=16  CASC_LEN=1 SHIFT=16    NITER=8"
    test_arr[$LINENO]="make all  AIE_VARIANT=22 DATA_TYPE=cint32 COEFF_TYPE=cint16  FIR_LEN=32  CASC_LEN=1 SHIFT=16    NITER=8"

    #cint32/int32
    test_arr[$LINENO]="make all  AIE_VARIANT=22 DATA_TYPE=cint32 COEFF_TYPE=int32   FIR_LEN=16  CASC_LEN=1 SHIFT=16    NITER=8"
    test_arr[$LINENO]="make all  AIE_VARIANT=22 DATA_TYPE=cint32 COEFF_TYPE=int32   FIR_LEN=32  CASC_LEN=1 SHIFT=16    NITER=8"

    #cint32/cint32
    test_arr[$LINENO]="make all  AIE_VARIANT=22 DATA_TYPE=cint32 COEFF_TYPE=cint32  FIR_LEN=16  CASC_LEN=1 SHIFT=16    NITER=8"
    test_arr[$LINENO]="make all  AIE_VARIANT=22 DATA_TYPE=cint32 COEFF_TYPE=cint32  FIR_LEN=32  CASC_LEN=1 SHIFT=16    NITER=8"

    #float/float
    test_arr[$LINENO]="make all  AIE_VARIANT=22 DATA_TYPE=float COEFF_TYPE=float   FIR_LEN=16  CASC_LEN=1 SHIFT=0    NITER=8"
    test_arr[$LINENO]="make all  AIE_VARIANT=22 DATA_TYPE=float COEFF_TYPE=float   FIR_LEN=32  CASC_LEN=1 SHIFT=0    NITER=8"
fi

if [[ "$*" == *1buff*  ]]
then
    lengths=(4 5 6 7 8 9 10 11 12 13 14 15 )
    # lengths=(32 33 34 35 36 37 38 39 40 41 42 43 44 45 46 47)
    # lengths=(16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31 32 33 34 35 36 37 38 39 40 41 42 43 44 45 46 47)
        for fLen in "${lengths[@]}"; do
            # window size must be greater of equal to coefficient array
            # each must be lesser than 16kB.
            windowSize=512
            test_arr[$LINENO]="make all DATA_TYPE=int16 COEFF_TYPE=int16  FIR_LEN=$fLen CASC_LEN=1 INPUT_WINDOW_VSIZE=256 DATA_STIM_TYPE=5 COEFF_STIM_TYPE=4 SHIFT=0 "
            test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=int16  FIR_LEN=$fLen CASC_LEN=1 INPUT_WINDOW_VSIZE=256 DATA_STIM_TYPE=5 COEFF_STIM_TYPE=4 SHIFT=0 "
            test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=cint16 FIR_LEN=$fLen CASC_LEN=1 INPUT_WINDOW_VSIZE=256 DATA_STIM_TYPE=5 COEFF_STIM_TYPE=4 SHIFT=0 "
            test_arr[$LINENO]="make all DATA_TYPE=cint32 COEFF_TYPE=int16  FIR_LEN=$fLen CASC_LEN=1 INPUT_WINDOW_VSIZE=256 DATA_STIM_TYPE=5 COEFF_STIM_TYPE=4 SHIFT=0 "
            test_arr[$LINENO]="make all DATA_TYPE=cint32 COEFF_TYPE=cint32 FIR_LEN=$fLen CASC_LEN=1 INPUT_WINDOW_VSIZE=256 DATA_STIM_TYPE=5 COEFF_STIM_TYPE=4 SHIFT=0 "
            test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=int32  FIR_LEN=$fLen CASC_LEN=1 INPUT_WINDOW_VSIZE=256 DATA_STIM_TYPE=5 COEFF_STIM_TYPE=4 SHIFT=0 "
            test_arr[$LINENO]="make all DATA_TYPE=int16 COEFF_TYPE=int32  FIR_LEN=$fLen CASC_LEN=1 INPUT_WINDOW_VSIZE=256 DATA_STIM_TYPE=5 COEFF_STIM_TYPE=4 SHIFT=0 "
        done #fLen

fi

if [[ "$*" == *2buff*  ]]
then
    lengths=(28 29 30 31 32 33 34 35)
    # lengths=(32 33 34 35 36 37 38 39 40 41 42 43 44 45 46 47)
    # lengths=(16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31 32 33 34 35 36 37 38 39 40 41 42 43 44 45 46 47)
        for fLen in "${lengths[@]}"; do
            # window size must be greater of equal to coefficient array
            # each must be lesser than 16kB.
            windowSize=512
            test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=int16  FIR_LEN=$fLen CASC_LEN=1 INPUT_WINDOW_VSIZE=256 DATA_STIM_TYPE=5 COEFF_STIM_TYPE=4 SHIFT=0 "
            test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=cint16 FIR_LEN=$fLen CASC_LEN=1 INPUT_WINDOW_VSIZE=256 DATA_STIM_TYPE=5 COEFF_STIM_TYPE=4 SHIFT=0 "
            test_arr[$LINENO]="make all DATA_TYPE=cint32 COEFF_TYPE=int16  FIR_LEN=$fLen CASC_LEN=1 INPUT_WINDOW_VSIZE=256 DATA_STIM_TYPE=5 COEFF_STIM_TYPE=4 SHIFT=0 "
            test_arr[$LINENO]="make all DATA_TYPE=cint32 COEFF_TYPE=cint32 FIR_LEN=$fLen CASC_LEN=1 INPUT_WINDOW_VSIZE=256 DATA_STIM_TYPE=5 COEFF_STIM_TYPE=4 SHIFT=0 "
            test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=cint32 FIR_LEN=$fLen CASC_LEN=1 INPUT_WINDOW_VSIZE=256 DATA_STIM_TYPE=5 COEFF_STIM_TYPE=4 SHIFT=0 "
            test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=int32  FIR_LEN=$fLen CASC_LEN=1 INPUT_WINDOW_VSIZE=256 DATA_STIM_TYPE=5 COEFF_STIM_TYPE=4 SHIFT=0 "
        done #fLen
    lengths=(64 65 66 67 68 69 70 71 72 73 74 75 76 77 78 79)
        for fLen in "${lengths[@]}"; do

            windowSize=512
            test_arr[$LINENO]="make all DATA_TYPE=int16 COEFF_TYPE=int16  FIR_LEN=$fLen CASC_LEN=1 INPUT_WINDOW_VSIZE=256 DATA_STIM_TYPE=5 COEFF_STIM_TYPE=4 SHIFT=0 "
            test_arr[$LINENO]="make all DATA_TYPE=int16 COEFF_TYPE=int32  FIR_LEN=$fLen CASC_LEN=1 INPUT_WINDOW_VSIZE=256 DATA_STIM_TYPE=5 COEFF_STIM_TYPE=4 SHIFT=0 "
        done #fLen

fi

if [[ "$*" == *iobuffer*  ]]
then
    #windowed
    test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=int16 FIR_LEN=32 SHIFT=16 INPUT_WINDOW_VSIZE=256   PORT_API=0 CASC_LEN=1 DUAL_IP=0 NUM_OUTPUTS=1 USE_COEFF_RELOAD=0"
    test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=int16 FIR_LEN=32 SHIFT=16 INPUT_WINDOW_VSIZE=256   PORT_API=0 CASC_LEN=1 DUAL_IP=0 NUM_OUTPUTS=1 USE_COEFF_RELOAD=1"
    test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=int16 FIR_LEN=32 SHIFT=16 INPUT_WINDOW_VSIZE=256   PORT_API=0 CASC_LEN=1 DUAL_IP=0 NUM_OUTPUTS=2 USE_COEFF_RELOAD=0"
    test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=int16 FIR_LEN=32 SHIFT=16 INPUT_WINDOW_VSIZE=256   PORT_API=0 CASC_LEN=1 DUAL_IP=0 NUM_OUTPUTS=2 USE_COEFF_RELOAD=1"

    test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=int16 FIR_LEN=32 SHIFT=16 INPUT_WINDOW_VSIZE=256   PORT_API=0 CASC_LEN=1 DUAL_IP=1 NUM_OUTPUTS=1 USE_COEFF_RELOAD=0"
    test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=int16 FIR_LEN=32 SHIFT=16 INPUT_WINDOW_VSIZE=256   PORT_API=0 CASC_LEN=1 DUAL_IP=1 NUM_OUTPUTS=1 USE_COEFF_RELOAD=1"
    test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=int16 FIR_LEN=32 SHIFT=16 INPUT_WINDOW_VSIZE=256   PORT_API=0 CASC_LEN=1 DUAL_IP=1 NUM_OUTPUTS=2 USE_COEFF_RELOAD=0"
    test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=int16 FIR_LEN=32 SHIFT=16 INPUT_WINDOW_VSIZE=256   PORT_API=0 CASC_LEN=1 DUAL_IP=1 NUM_OUTPUTS=2 USE_COEFF_RELOAD=1"

    test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=int16 FIR_LEN=32 SHIFT=16 INPUT_WINDOW_VSIZE=256   PORT_API=0 CASC_LEN=3 DUAL_IP=0 NUM_OUTPUTS=1 USE_COEFF_RELOAD=0"
    test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=int16 FIR_LEN=32 SHIFT=16 INPUT_WINDOW_VSIZE=256   PORT_API=0 CASC_LEN=3 DUAL_IP=0 NUM_OUTPUTS=1 USE_COEFF_RELOAD=1"
    test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=int16 FIR_LEN=32 SHIFT=16 INPUT_WINDOW_VSIZE=256   PORT_API=0 CASC_LEN=3 DUAL_IP=0 NUM_OUTPUTS=2 USE_COEFF_RELOAD=0"
    test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=int16 FIR_LEN=32 SHIFT=16 INPUT_WINDOW_VSIZE=256   PORT_API=0 CASC_LEN=3 DUAL_IP=0 NUM_OUTPUTS=2 USE_COEFF_RELOAD=1"

    test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=int16 FIR_LEN=32 SHIFT=16 INPUT_WINDOW_VSIZE=256   PORT_API=0 CASC_LEN=3 DUAL_IP=1 NUM_OUTPUTS=1 USE_COEFF_RELOAD=0"
    test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=int16 FIR_LEN=32 SHIFT=16 INPUT_WINDOW_VSIZE=256   PORT_API=0 CASC_LEN=3 DUAL_IP=1 NUM_OUTPUTS=1 USE_COEFF_RELOAD=1"
    test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=int16 FIR_LEN=32 SHIFT=16 INPUT_WINDOW_VSIZE=256   PORT_API=0 CASC_LEN=3 DUAL_IP=1 NUM_OUTPUTS=2 USE_COEFF_RELOAD=0"
    test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=int16 FIR_LEN=32 SHIFT=16 INPUT_WINDOW_VSIZE=256   PORT_API=0 CASC_LEN=3 DUAL_IP=1 NUM_OUTPUTS=2 USE_COEFF_RELOAD=1"

    #streaming
    test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=int16 FIR_LEN=32 SHIFT=16 INPUT_WINDOW_VSIZE=256   PORT_API=1 CASC_LEN=1 DUAL_IP=0 NUM_OUTPUTS=1 USE_COEFF_RELOAD=0"
    test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=int16 FIR_LEN=32 SHIFT=16 INPUT_WINDOW_VSIZE=256   PORT_API=1 CASC_LEN=1 DUAL_IP=0 NUM_OUTPUTS=1 USE_COEFF_RELOAD=1"
    test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=int16 FIR_LEN=32 SHIFT=16 INPUT_WINDOW_VSIZE=256   PORT_API=1 CASC_LEN=1 DUAL_IP=0 NUM_OUTPUTS=2 USE_COEFF_RELOAD=0"
    test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=int16 FIR_LEN=32 SHIFT=16 INPUT_WINDOW_VSIZE=256   PORT_API=1 CASC_LEN=1 DUAL_IP=0 NUM_OUTPUTS=2 USE_COEFF_RELOAD=1"

    test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=int16 FIR_LEN=32 SHIFT=16 INPUT_WINDOW_VSIZE=256   PORT_API=1 CASC_LEN=1 DUAL_IP=1 NUM_OUTPUTS=1 USE_COEFF_RELOAD=0"
    test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=int16 FIR_LEN=32 SHIFT=16 INPUT_WINDOW_VSIZE=256   PORT_API=1 CASC_LEN=1 DUAL_IP=1 NUM_OUTPUTS=1 USE_COEFF_RELOAD=1"
    test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=int16 FIR_LEN=32 SHIFT=16 INPUT_WINDOW_VSIZE=256   PORT_API=1 CASC_LEN=1 DUAL_IP=1 NUM_OUTPUTS=2 USE_COEFF_RELOAD=0"
    test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=int16 FIR_LEN=32 SHIFT=16 INPUT_WINDOW_VSIZE=256   PORT_API=1 CASC_LEN=1 DUAL_IP=1 NUM_OUTPUTS=2 USE_COEFF_RELOAD=1"

    test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=int16 FIR_LEN=32 SHIFT=16 INPUT_WINDOW_VSIZE=256   PORT_API=1 CASC_LEN=3 DUAL_IP=0 NUM_OUTPUTS=1 USE_COEFF_RELOAD=0"
    test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=int16 FIR_LEN=32 SHIFT=16 INPUT_WINDOW_VSIZE=256   PORT_API=1 CASC_LEN=3 DUAL_IP=0 NUM_OUTPUTS=1 USE_COEFF_RELOAD=1"
    test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=int16 FIR_LEN=32 SHIFT=16 INPUT_WINDOW_VSIZE=256   PORT_API=1 CASC_LEN=3 DUAL_IP=0 NUM_OUTPUTS=2 USE_COEFF_RELOAD=1"
    test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=int16 FIR_LEN=32 SHIFT=16 INPUT_WINDOW_VSIZE=256   PORT_API=1 CASC_LEN=3 DUAL_IP=0 NUM_OUTPUTS=2 USE_COEFF_RELOAD=0"

    test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=int16 FIR_LEN=32 SHIFT=16 INPUT_WINDOW_VSIZE=256   PORT_API=1 CASC_LEN=3 DUAL_IP=1 NUM_OUTPUTS=1 USE_COEFF_RELOAD=1"
    test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=int16 FIR_LEN=32 SHIFT=16 INPUT_WINDOW_VSIZE=256   PORT_API=1 CASC_LEN=3 DUAL_IP=1 NUM_OUTPUTS=1 USE_COEFF_RELOAD=0"
    test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=int16 FIR_LEN=32 SHIFT=16 INPUT_WINDOW_VSIZE=256   PORT_API=1 CASC_LEN=3 DUAL_IP=1 NUM_OUTPUTS=2 USE_COEFF_RELOAD=1"
    test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=int16 FIR_LEN=32 SHIFT=16 INPUT_WINDOW_VSIZE=256   PORT_API=1 CASC_LEN=3 DUAL_IP=1 NUM_OUTPUTS=2 USE_COEFF_RELOAD=0"

fi

if [[ "$*" == *valaccess*  ]]
then
    #optimal lengths
    test_arr[$LINENO]="make all DATA_TYPE=int16 COEFF_TYPE=int16   FIR_LEN=64 CASC_LEN=2  SHIFT=16 PORT_API=1 INPUT_WINDOW_VSIZE=256"
    test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=int16   FIR_LEN=64 CASC_LEN=2  SHIFT=16 PORT_API=1 INPUT_WINDOW_VSIZE=256"
    test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=cint16  FIR_LEN=64 CASC_LEN=4  SHIFT=16 PORT_API=1 INPUT_WINDOW_VSIZE=256"
    test_arr[$LINENO]="make all DATA_TYPE=int32 COEFF_TYPE=int16   FIR_LEN=64 CASC_LEN=2  SHIFT=16 PORT_API=1 INPUT_WINDOW_VSIZE=256"
    test_arr[$LINENO]="make all DATA_TYPE=int32 COEFF_TYPE=int32   FIR_LEN=64 CASC_LEN=4  SHIFT=16 PORT_API=1 INPUT_WINDOW_VSIZE=256"
    test_arr[$LINENO]="make all DATA_TYPE=cint32 COEFF_TYPE=int16   FIR_LEN=64 CASC_LEN=3  SHIFT=16 PORT_API=1 INPUT_WINDOW_VSIZE=256" #optimal is illegal!
    test_arr[$LINENO]="make all DATA_TYPE=cint32 COEFF_TYPE=cint16  FIR_LEN=64 CASC_LEN=4  SHIFT=16 PORT_API=1 INPUT_WINDOW_VSIZE=256"
    test_arr[$LINENO]="make all DATA_TYPE=cint32 COEFF_TYPE=int32   FIR_LEN=64 CASC_LEN=4  SHIFT=16 PORT_API=1 INPUT_WINDOW_VSIZE=256"
    test_arr[$LINENO]="make all DATA_TYPE=cint32 COEFF_TYPE=cint32  FIR_LEN=64 CASC_LEN=8  SHIFT=16 PORT_API=1 INPUT_WINDOW_VSIZE=256"
    test_arr[$LINENO]="make all DATA_TYPE=float COEFF_TYPE=float   FIR_LEN=64 CASC_LEN=4  SHIFT=0  PORT_API=1 INPUT_WINDOW_VSIZE=256"
    test_arr[$LINENO]="make all DATA_TYPE=cfloat COEFF_TYPE=float   FIR_LEN=64 CASC_LEN=4  SHIFT=0  PORT_API=1 INPUT_WINDOW_VSIZE=256"
    test_arr[$LINENO]="make all DATA_TYPE=cfloat COEFF_TYPE=cfloat  FIR_LEN=64 CASC_LEN=8 SHIFT=0  PORT_API=1 INPUT_WINDOW_VSIZE=256" #static assert fail.

 #   test_arr[$LINENO]="make all DATA_TYPE=int16 COEFF_TYPE=int16   FIR_LEN=64 CASC_LEN=4  SHIFT=16 PORT_API=1 INPUT_WINDOW_VSIZE=256 DUAL_IP=1 NUM_OUTPUTS=2"
 #   test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=int16   FIR_LEN=64 CASC_LEN=4  SHIFT=16 PORT_API=1 INPUT_WINDOW_VSIZE=256 DUAL_IP=1 NUM_OUTPUTS=2"
 #   test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=cint16  FIR_LEN=64 CASC_LEN=8  SHIFT=16 PORT_API=1 INPUT_WINDOW_VSIZE=256 DUAL_IP=1 NUM_OUTPUTS=2"
 #   test_arr[$LINENO]="make all DATA_TYPE=int32 COEFF_TYPE=int16   FIR_LEN=64 CASC_LEN=4  SHIFT=16 PORT_API=1 INPUT_WINDOW_VSIZE=256 DUAL_IP=1 NUM_OUTPUTS=2"
 #   test_arr[$LINENO]="make all DATA_TYPE=int32 COEFF_TYPE=int32   FIR_LEN=64 CASC_LEN=8  SHIFT=16 PORT_API=1 INPUT_WINDOW_VSIZE=256 DUAL_IP=1 NUM_OUTPUTS=2"
 #   test_arr[$LINENO]="make all DATA_TYPE=cint32 COEFF_TYPE=int16   FIR_LEN=64 CASC_LEN=5  SHIFT=16 PORT_API=1 INPUT_WINDOW_VSIZE=256 DUAL_IP=1 NUM_OUTPUTS=2"
 #   test_arr[$LINENO]="make all DATA_TYPE=cint32 COEFF_TYPE=cint16  FIR_LEN=64 CASC_LEN=8  SHIFT=16 PORT_API=1 INPUT_WINDOW_VSIZE=256 DUAL_IP=1 NUM_OUTPUTS=2"
 #   test_arr[$LINENO]="make all DATA_TYPE=cint32 COEFF_TYPE=int32   FIR_LEN=64 CASC_LEN=8  SHIFT=16 PORT_API=1 INPUT_WINDOW_VSIZE=256 DUAL_IP=1 NUM_OUTPUTS=2"
 #   test_arr[$LINENO]="make all DATA_TYPE=cint32 COEFF_TYPE=cint32  FIR_LEN=64 CASC_LEN=16 SHIFT=16 PORT_API=1 INPUT_WINDOW_VSIZE=256 DUAL_IP=1 NUM_OUTPUTS=2"
 #   test_arr[$LINENO]="make all DATA_TYPE=float COEFF_TYPE=float   FIR_LEN=64 CASC_LEN=8  SHIFT=0  PORT_API=1 INPUT_WINDOW_VSIZE=256 DUAL_IP=1 NUM_OUTPUTS=2"
 #   test_arr[$LINENO]="make all DATA_TYPE=cfloat COEFF_TYPE=float   FIR_LEN=64 CASC_LEN=8  SHIFT=0  PORT_API=1 INPUT_WINDOW_VSIZE=256 DUAL_IP=1 NUM_OUTPUTS=2" #static assert fail.
 #   test_arr[$LINENO]="make all DATA_TYPE=cfloat COEFF_TYPE=cfloat  FIR_LEN=64 CASC_LEN=16 SHIFT=0  PORT_API=1 INPUT_WINDOW_VSIZE=256 DUAL_IP=1 NUM_OUTPUTS=2" #static assert fail.

    #max lengths
    test_arr[$LINENO]="make all DATA_TYPE=int16 COEFF_TYPE=int16   FIR_LEN=512 CASC_LEN=1  SHIFT=16 PORT_API=0 INPUT_WINDOW_VSIZE=256"
    test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=int16   FIR_LEN=512 CASC_LEN=1  SHIFT=16 PORT_API=0 INPUT_WINDOW_VSIZE=256"
    test_arr[$LINENO]="make all DATA_TYPE=int32 COEFF_TYPE=int16   FIR_LEN=512 CASC_LEN=1  SHIFT=16 PORT_API=0 INPUT_WINDOW_VSIZE=256"
    test_arr[$LINENO]="make all DATA_TYPE=cint32 COEFF_TYPE=int16   FIR_LEN=512 CASC_LEN=1  SHIFT=16 PORT_API=0 INPUT_WINDOW_VSIZE=256"
    test_arr[$LINENO]="make all DATA_TYPE=float COEFF_TYPE=float   FIR_LEN=512 CASC_LEN=1  SHIFT=0  PORT_API=0 INPUT_WINDOW_VSIZE=256"
    test_arr[$LINENO]="make all DATA_TYPE=cfloat COEFF_TYPE=float   FIR_LEN=256 CASC_LEN=1  SHIFT=0  PORT_API=0 INPUT_WINDOW_VSIZE=256"

    test_arr[$LINENO]="make all DATA_TYPE=int16 COEFF_TYPE=int16   FIR_LEN=512 CASC_LEN=1  SHIFT=16 PORT_API=1 INPUT_WINDOW_VSIZE=256"
    test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=int16   FIR_LEN=512 CASC_LEN=1  SHIFT=16 PORT_API=1 INPUT_WINDOW_VSIZE=256"
    test_arr[$LINENO]="make all DATA_TYPE=int32 COEFF_TYPE=int16   FIR_LEN=512 CASC_LEN=1  SHIFT=16 PORT_API=1 INPUT_WINDOW_VSIZE=256"
    test_arr[$LINENO]="make all DATA_TYPE=cint32 COEFF_TYPE=int16   FIR_LEN=512 CASC_LEN=1  SHIFT=16 PORT_API=1 INPUT_WINDOW_VSIZE=256" #was 256 by mistake
    test_arr[$LINENO]="make all DATA_TYPE=float COEFF_TYPE=float   FIR_LEN=512 CASC_LEN=1  SHIFT=0  PORT_API=1 INPUT_WINDOW_VSIZE=256" #access fn was 256 by mitake
    test_arr[$LINENO]="make all DATA_TYPE=cfloat COEFF_TYPE=float   FIR_LEN=256 CASC_LEN=1  SHIFT=0  PORT_API=1 INPUT_WINDOW_VSIZE=256"

fi

if [[ "$*" == *debug*  ]]
then
    test_arr[${#test_arr[*]}]="echo 'Performing debug char tests...' "
    test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=cint16  FIR_LEN=30 SHIFT=15 INPUT_WINDOW_VSIZE=512 CASC_LEN=3 "
    test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=cint16  FIR_LEN=30 SHIFT=15 INPUT_WINDOW_VSIZE=512 CASC_LEN=2 "
    test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=cint16  FIR_LEN=30 SHIFT=15 INPUT_WINDOW_VSIZE=512 CASC_LEN=1 "
    test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=cint16  FIR_LEN=30 SHIFT=15 INPUT_WINDOW_VSIZE=256 CASC_LEN=3 "
    test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=cint16  FIR_LEN=40 SHIFT=15 INPUT_WINDOW_VSIZE=512 CASC_LEN=3 "
    test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=cint16  FIR_LEN=20 SHIFT=15 INPUT_WINDOW_VSIZE=512 CASC_LEN=3 "
    test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=int16   FIR_LEN=30 SHIFT=15 INPUT_WINDOW_VSIZE=512 CASC_LEN=3 "
    test_arr[$LINENO]="make all DATA_TYPE=cfloat COEFF_TYPE=cfloat  FIR_LEN=30 SHIFT=0  INPUT_WINDOW_VSIZE=512 CASC_LEN=3 "
fi

if [[ "$*" == *dual_ip_perf*  ]]
then
    test_arr[${#test_arr[*]}]="echo 'Performing debug char tests...' "
    lengths=(96 128 160 192 224 240 256 320 384 448 512 )
    for fLen in "${lengths[@]}"; do

    #test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=cint16  FIR_LEN=$fLen SHIFT=15 INPUT_WINDOW_VSIZE=512 CASC_LEN=1 "
    test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=cint16  FIR_LEN=$fLen SHIFT=15 INPUT_WINDOW_VSIZE=512 CASC_LEN=1 DUAL_IP=1 NUM_OUTPUTS=2"
    done
fi

if [[ "$*" == *dual_op*  ]]
then
    types=(
        int16 int16
        cint16 int16
        int32 int16
        cint32 int16
        cint16 cint16
        int32 int32
        cint32 int32
        cint32 cint32
        float float
        cfloat float
        cfloat cfloat
    )

    for (( typeI=0; typeI<${#types[@]}; typeI+=2 )); do
        echo "${types[$typeI]},${types[`expr $typeI + 1 `]}"
        dType=${types[$typeI]}
        cType=${types[`expr $typeI + 1 `]}

        test_arr[${#test_arr[*]}]="echo 'Performing exploratory tests...' "
        test_arr[$LINENO]="make all DATA_TYPE=$dType COEFF_TYPE=$cType FIR_LEN=24 SHIFT=15 INPUT_WINDOW_VSIZE=256 CASC_LEN=1 USE_COEFF_RELOAD=0 NUM_OUTPUTS=1"
        test_arr[$LINENO]="make all DATA_TYPE=$dType COEFF_TYPE=$cType FIR_LEN=24 SHIFT=15 INPUT_WINDOW_VSIZE=256 CASC_LEN=1 USE_COEFF_RELOAD=0 NUM_OUTPUTS=2"
        test_arr[$LINENO]="make all DATA_TYPE=$dType COEFF_TYPE=$cType FIR_LEN=24 SHIFT=15 INPUT_WINDOW_VSIZE=256 CASC_LEN=1 USE_COEFF_RELOAD=1 NUM_OUTPUTS=1"
        test_arr[$LINENO]="make all DATA_TYPE=$dType COEFF_TYPE=$cType FIR_LEN=24 SHIFT=15 INPUT_WINDOW_VSIZE=256 CASC_LEN=1 USE_COEFF_RELOAD=1 NUM_OUTPUTS=2"
        test_arr[$LINENO]="make all DATA_TYPE=$dType COEFF_TYPE=$cType FIR_LEN=24 SHIFT=15 INPUT_WINDOW_VSIZE=256 CASC_LEN=3 USE_COEFF_RELOAD=0 NUM_OUTPUTS=1"
        test_arr[$LINENO]="make all DATA_TYPE=$dType COEFF_TYPE=$cType FIR_LEN=24 SHIFT=15 INPUT_WINDOW_VSIZE=256 CASC_LEN=3 USE_COEFF_RELOAD=0 NUM_OUTPUTS=2"
        test_arr[$LINENO]="make all DATA_TYPE=$dType COEFF_TYPE=$cType FIR_LEN=24 SHIFT=15 INPUT_WINDOW_VSIZE=256 CASC_LEN=3 USE_COEFF_RELOAD=1 NUM_OUTPUTS=1"
        test_arr[$LINENO]="make all DATA_TYPE=$dType COEFF_TYPE=$cType FIR_LEN=24 SHIFT=15 INPUT_WINDOW_VSIZE=256 CASC_LEN=3 USE_COEFF_RELOAD=1 NUM_OUTPUTS=2"

        test_arr[$LINENO]="make all DATA_TYPE=$dType COEFF_TYPE=$cType FIR_LEN=24 SHIFT=15 INPUT_WINDOW_VSIZE=256 CASC_LEN=1 USE_COEFF_RELOAD=0 NUM_OUTPUTS=1 DUAL_IP=1"
        test_arr[$LINENO]="make all DATA_TYPE=$dType COEFF_TYPE=$cType FIR_LEN=24 SHIFT=15 INPUT_WINDOW_VSIZE=256 CASC_LEN=1 USE_COEFF_RELOAD=0 NUM_OUTPUTS=2 DUAL_IP=1"
        test_arr[$LINENO]="make all DATA_TYPE=$dType COEFF_TYPE=$cType FIR_LEN=24 SHIFT=15 INPUT_WINDOW_VSIZE=256 CASC_LEN=1 USE_COEFF_RELOAD=1 NUM_OUTPUTS=1 DUAL_IP=1"
        test_arr[$LINENO]="make all DATA_TYPE=$dType COEFF_TYPE=$cType FIR_LEN=24 SHIFT=15 INPUT_WINDOW_VSIZE=256 CASC_LEN=1 USE_COEFF_RELOAD=1 NUM_OUTPUTS=2 DUAL_IP=1"
        test_arr[$LINENO]="make all DATA_TYPE=$dType COEFF_TYPE=$cType FIR_LEN=24 SHIFT=15 INPUT_WINDOW_VSIZE=256 CASC_LEN=3 USE_COEFF_RELOAD=0 NUM_OUTPUTS=1 DUAL_IP=1"
        test_arr[$LINENO]="make all DATA_TYPE=$dType COEFF_TYPE=$cType FIR_LEN=24 SHIFT=15 INPUT_WINDOW_VSIZE=256 CASC_LEN=3 USE_COEFF_RELOAD=0 NUM_OUTPUTS=2 DUAL_IP=1"
        test_arr[$LINENO]="make all DATA_TYPE=$dType COEFF_TYPE=$cType FIR_LEN=24 SHIFT=15 INPUT_WINDOW_VSIZE=256 CASC_LEN=3 USE_COEFF_RELOAD=1 NUM_OUTPUTS=1 DUAL_IP=1"
        test_arr[$LINENO]="make all DATA_TYPE=$dType COEFF_TYPE=$cType FIR_LEN=24 SHIFT=15 INPUT_WINDOW_VSIZE=256 CASC_LEN=3 USE_COEFF_RELOAD=1 NUM_OUTPUTS=2 DUAL_IP=1"

        test_arr[$LINENO]="make all DATA_TYPE=$dType COEFF_TYPE=$cType FIR_LEN=24 SHIFT=15 INPUT_WINDOW_VSIZE=256 CASC_LEN=1 USE_COEFF_RELOAD=0 NUM_OUTPUTS=1 PORT_API=1"
        test_arr[$LINENO]="make all DATA_TYPE=$dType COEFF_TYPE=$cType FIR_LEN=24 SHIFT=15 INPUT_WINDOW_VSIZE=256 CASC_LEN=1 USE_COEFF_RELOAD=1 NUM_OUTPUTS=1 PORT_API=1"
        test_arr[$LINENO]="make all DATA_TYPE=$dType COEFF_TYPE=$cType FIR_LEN=24 SHIFT=15 INPUT_WINDOW_VSIZE=256 CASC_LEN=3 USE_COEFF_RELOAD=0 NUM_OUTPUTS=1 PORT_API=1"
        test_arr[$LINENO]="make all DATA_TYPE=$dType COEFF_TYPE=$cType FIR_LEN=24 SHIFT=15 INPUT_WINDOW_VSIZE=256 CASC_LEN=3 USE_COEFF_RELOAD=1 NUM_OUTPUTS=1 PORT_API=1"
        test_arr[$LINENO]="make all DATA_TYPE=$dType COEFF_TYPE=$cType FIR_LEN=24 SHIFT=15 INPUT_WINDOW_VSIZE=256 CASC_LEN=1 USE_COEFF_RELOAD=0 NUM_OUTPUTS=2 PORT_API=1"
        test_arr[$LINENO]="make all DATA_TYPE=$dType COEFF_TYPE=$cType FIR_LEN=24 SHIFT=15 INPUT_WINDOW_VSIZE=256 CASC_LEN=1 USE_COEFF_RELOAD=1 NUM_OUTPUTS=2 PORT_API=1"
        test_arr[$LINENO]="make all DATA_TYPE=$dType COEFF_TYPE=$cType FIR_LEN=24 SHIFT=15 INPUT_WINDOW_VSIZE=256 CASC_LEN=3 USE_COEFF_RELOAD=0 NUM_OUTPUTS=2 PORT_API=1"
        test_arr[$LINENO]="make all DATA_TYPE=$dType COEFF_TYPE=$cType FIR_LEN=24 SHIFT=15 INPUT_WINDOW_VSIZE=256 CASC_LEN=3 USE_COEFF_RELOAD=1 NUM_OUTPUTS=2 PORT_API=1"

        test_arr[$LINENO]="make all DATA_TYPE=$dType COEFF_TYPE=$cType FIR_LEN=24 SHIFT=15 INPUT_WINDOW_VSIZE=256 CASC_LEN=1 USE_COEFF_RELOAD=0 NUM_OUTPUTS=1 PORT_API=1 DUAL_IP=1"
        test_arr[$LINENO]="make all DATA_TYPE=$dType COEFF_TYPE=$cType FIR_LEN=24 SHIFT=15 INPUT_WINDOW_VSIZE=256 CASC_LEN=1 USE_COEFF_RELOAD=1 NUM_OUTPUTS=1 PORT_API=1 DUAL_IP=1"
        test_arr[$LINENO]="make all DATA_TYPE=$dType COEFF_TYPE=$cType FIR_LEN=24 SHIFT=15 INPUT_WINDOW_VSIZE=256 CASC_LEN=3 USE_COEFF_RELOAD=0 NUM_OUTPUTS=1 PORT_API=1 DUAL_IP=1"
        test_arr[$LINENO]="make all DATA_TYPE=$dType COEFF_TYPE=$cType FIR_LEN=24 SHIFT=15 INPUT_WINDOW_VSIZE=256 CASC_LEN=3 USE_COEFF_RELOAD=1 NUM_OUTPUTS=1 PORT_API=1 DUAL_IP=1"
        test_arr[$LINENO]="make all DATA_TYPE=$dType COEFF_TYPE=$cType FIR_LEN=24 SHIFT=15 INPUT_WINDOW_VSIZE=256 CASC_LEN=1 USE_COEFF_RELOAD=0 NUM_OUTPUTS=2 PORT_API=1 DUAL_IP=1"
        test_arr[$LINENO]="make all DATA_TYPE=$dType COEFF_TYPE=$cType FIR_LEN=24 SHIFT=15 INPUT_WINDOW_VSIZE=256 CASC_LEN=1 USE_COEFF_RELOAD=1 NUM_OUTPUTS=2 PORT_API=1 DUAL_IP=1"
        test_arr[$LINENO]="make all DATA_TYPE=$dType COEFF_TYPE=$cType FIR_LEN=24 SHIFT=15 INPUT_WINDOW_VSIZE=256 CASC_LEN=3 USE_COEFF_RELOAD=0 NUM_OUTPUTS=2 PORT_API=1 DUAL_IP=1"
        test_arr[$LINENO]="make all DATA_TYPE=$dType COEFF_TYPE=$cType FIR_LEN=24 SHIFT=15 INPUT_WINDOW_VSIZE=256 CASC_LEN=3 USE_COEFF_RELOAD=1 NUM_OUTPUTS=2 PORT_API=1 DUAL_IP=1"
    done
fi

if [[  "$*" == *max_fir* ]]
then
    targets=(hw)
    # lengths=(96 128 160 192 224 240 256 320 384 448 512 )
    # # lengths=( 320 384 448 512 1024)
    # for target in "${targets[@]}"; do
    #     for fLen in "${lengths[@]}"; do
    #         # window size must be greater of equal to coefficient array
    #         # each must be lesser than 16kB.
    #         windowSize=512
    #         test_arr[$LINENO]="make all DATA_TYPE=int16 COEFF_TYPE=int16  TARGET=$target FIR_LEN=$fLen CASC_LEN=1 INPUT_WINDOW_VSIZE=$windowSize USE_COEFF_RELOAD=0"
    #         test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=int16  TARGET=$target FIR_LEN=$fLen CASC_LEN=1 INPUT_WINDOW_VSIZE=$windowSize USE_COEFF_RELOAD=0"
    #         test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=cint16 TARGET=$target FIR_LEN=$fLen CASC_LEN=1 INPUT_WINDOW_VSIZE=$windowSize USE_COEFF_RELOAD=0"
    #         test_arr[$LINENO]="make all DATA_TYPE=int32 COEFF_TYPE=int16  TARGET=$target FIR_LEN=$fLen CASC_LEN=1 INPUT_WINDOW_VSIZE=$windowSize USE_COEFF_RELOAD=0"
    #         test_arr[$LINENO]="make all DATA_TYPE=int32 COEFF_TYPE=int32  TARGET=$target FIR_LEN=$fLen CASC_LEN=1 INPUT_WINDOW_VSIZE=$windowSize USE_COEFF_RELOAD=0"
    #         test_arr[$LINENO]="make all DATA_TYPE=cint32 COEFF_TYPE=int16  TARGET=$target FIR_LEN=$fLen CASC_LEN=1 INPUT_WINDOW_VSIZE=$windowSize USE_COEFF_RELOAD=0"
    #         test_arr[$LINENO]="make all DATA_TYPE=cint32 COEFF_TYPE=cint16 TARGET=$target FIR_LEN=$fLen CASC_LEN=1 INPUT_WINDOW_VSIZE=$windowSize USE_COEFF_RELOAD=0"
    #         test_arr[$LINENO]="make all DATA_TYPE=cint32 COEFF_TYPE=int32  TARGET=$target FIR_LEN=$fLen CASC_LEN=1 INPUT_WINDOW_VSIZE=$windowSize USE_COEFF_RELOAD=0"
    #         test_arr[$LINENO]="make all DATA_TYPE=cint32 COEFF_TYPE=cint32 TARGET=$target FIR_LEN=$fLen CASC_LEN=1 INPUT_WINDOW_VSIZE=$windowSize USE_COEFF_RELOAD=0"
    #         test_arr[$LINENO]="make all DATA_TYPE=float COEFF_TYPE=float  TARGET=$target FIR_LEN=$fLen CASC_LEN=1 INPUT_WINDOW_VSIZE=$windowSize USE_COEFF_RELOAD=0"
    #         test_arr[$LINENO]="make all DATA_TYPE=cfloat COEFF_TYPE=float  TARGET=$target FIR_LEN=$fLen CASC_LEN=1 INPUT_WINDOW_VSIZE=$windowSize USE_COEFF_RELOAD=0"
    #         test_arr[$LINENO]="make all DATA_TYPE=cfloat COEFF_TYPE=cfloat TARGET=$target FIR_LEN=$fLen CASC_LEN=1 INPUT_WINDOW_VSIZE=$windowSize USE_COEFF_RELOAD=0"
    #     done #fLen
    # done #target
    lengths=(512 1024 1536 2048 2560 3072 4096)
    for target in "${targets[@]}"; do
        for fLen in "${lengths[@]}"; do
            # window size must be greater of equal to coefficient array
            # each must be lesser than 16kB.
            windowSize=512
            let "casc_len=$fLen / 512"
            test_arr[$LINENO]="make all DATA_TYPE=int16 COEFF_TYPE=int16  TARGET=$target FIR_LEN=$fLen CASC_LEN=$casc_len INPUT_WINDOW_VSIZE=$windowSize NITER=4"
            test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=int16  TARGET=$target FIR_LEN=$fLen CASC_LEN=$casc_len INPUT_WINDOW_VSIZE=$windowSize NITER=4"
            test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=cint16 TARGET=$target FIR_LEN=$fLen CASC_LEN=$casc_len INPUT_WINDOW_VSIZE=$windowSize NITER=4"
            test_arr[$LINENO]="make all DATA_TYPE=int32 COEFF_TYPE=int16  TARGET=$target FIR_LEN=$fLen CASC_LEN=$casc_len INPUT_WINDOW_VSIZE=$windowSize NITER=4"
            test_arr[$LINENO]="make all DATA_TYPE=int32 COEFF_TYPE=int32  TARGET=$target FIR_LEN=$fLen CASC_LEN=$casc_len INPUT_WINDOW_VSIZE=$windowSize NITER=4"
            test_arr[$LINENO]="make all DATA_TYPE=cint32 COEFF_TYPE=int16  TARGET=$target FIR_LEN=$fLen CASC_LEN=$casc_len INPUT_WINDOW_VSIZE=$windowSize NITER=4"
            test_arr[$LINENO]="make all DATA_TYPE=cint32 COEFF_TYPE=cint16 TARGET=$target FIR_LEN=$fLen CASC_LEN=$casc_len INPUT_WINDOW_VSIZE=$windowSize NITER=4"
            test_arr[$LINENO]="make all DATA_TYPE=cint32 COEFF_TYPE=int32  TARGET=$target FIR_LEN=$fLen CASC_LEN=$casc_len INPUT_WINDOW_VSIZE=$windowSize NITER=4"
            test_arr[$LINENO]="make all DATA_TYPE=cint32 COEFF_TYPE=cint32 TARGET=$target FIR_LEN=$fLen CASC_LEN=$casc_len INPUT_WINDOW_VSIZE=$windowSize NITER=4"
        done #fLen
    done #target
    lengths=(5120 6400 7680 8192)
    for target in "${targets[@]}"; do
        for fLen in "${lengths[@]}"; do
            # window size must be greater of equal to coefficient array
            # each must be lesser than 16kB.
            windowSize=512
            let "casc_len=$fLen / 512"
            test_arr[$LINENO]="make all DATA_TYPE=int16 COEFF_TYPE=int16  TARGET=$target FIR_LEN=$fLen CASC_LEN=$casc_len INPUT_WINDOW_VSIZE=$windowSize NITER=4"
            test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=int16  TARGET=$target FIR_LEN=$fLen CASC_LEN=$casc_len INPUT_WINDOW_VSIZE=$windowSize NITER=4"
            test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=cint16 TARGET=$target FIR_LEN=$fLen CASC_LEN=$casc_len INPUT_WINDOW_VSIZE=$windowSize NITER=4"
            test_arr[$LINENO]="make all DATA_TYPE=int32 COEFF_TYPE=int16  TARGET=$target FIR_LEN=$fLen CASC_LEN=$casc_len INPUT_WINDOW_VSIZE=$windowSize NITER=4"
            test_arr[$LINENO]="make all DATA_TYPE=int32 COEFF_TYPE=int32  TARGET=$target FIR_LEN=$fLen CASC_LEN=$casc_len INPUT_WINDOW_VSIZE=$windowSize NITER=4"
        done #fLen
    done #target
fi


if [[ "$*" == *adl_515* ]]
then
    # Testcase to repro ADL-515
    test_arr[$LINENO]="make all DATA_TYPE=int32 COEFF_TYPE=int32 FIR_LEN=40 CASC_LEN=2 SHIFT=16 INPUT_WINDOW_VSIZE=256 USE_COEFF_RELOAD=0 DUMP_VCD=1 USE_CHAIN=1"
    test_arr[$LINENO]="make all DATA_TYPE=int32 COEFF_TYPE=int32 FIR_LEN=40 CASC_LEN=2 SHIFT=16 INPUT_WINDOW_VSIZE=256 USE_COEFF_RELOAD=0 DUMP_VCD=1 USE_CHAIN=1 TARGET=x86sim"
fi

if [[ "$*" == *opt_taps* ]]
then
    # vary across data types, cascade lengths, dual input and outputs - used to motivate creation of new architecture,
    #int16/int16
    test_arr[$LINENO]="make all PORT_API=1 DATA_TYPE=int16 COEFF_TYPE=int16 FIR_LEN=32 CASC_LEN=1 SHIFT=16 INPUT_WINDOW_VSIZE=256"
    test_arr[$LINENO]="make all PORT_API=1 DATA_TYPE=int16 COEFF_TYPE=int16 FIR_LEN=16 CASC_LEN=1 SHIFT=16 INPUT_WINDOW_VSIZE=256 DUAL_IP=1 NUM_OUTPUTS=2"
    test_arr[$LINENO]="make all PORT_API=1 DATA_TYPE=int16 COEFF_TYPE=int16 FIR_LEN=96 CASC_LEN=3 SHIFT=16 INPUT_WINDOW_VSIZE=256"
    test_arr[$LINENO]="make all PORT_API=1 DATA_TYPE=int16 COEFF_TYPE=int16 FIR_LEN=48 CASC_LEN=3 SHIFT=16 INPUT_WINDOW_VSIZE=256 DUAL_IP=1 NUM_OUTPUTS=2"
    #int32/int16PORT_API=1
    test_arr[$LINENO]="make all PORT_API=1 DATA_TYPE=int32 COEFF_TYPE=int16 FIR_LEN=32 CASC_LEN=1 SHIFT=16 INPUT_WINDOW_VSIZE=256"
    test_arr[$LINENO]="make all PORT_API=1 DATA_TYPE=int32 COEFF_TYPE=int16 FIR_LEN=16 CASC_LEN=1 SHIFT=16 INPUT_WINDOW_VSIZE=256 DUAL_IP=1 NUM_OUTPUTS=2"
    test_arr[$LINENO]="make all PORT_API=1 DATA_TYPE=int32 COEFF_TYPE=int16 FIR_LEN=96 CASC_LEN=3 SHIFT=16 INPUT_WINDOW_VSIZE=256"
    test_arr[$LINENO]="make all PORT_API=1 DATA_TYPE=int32 COEFF_TYPE=int16 FIR_LEN=48 CASC_LEN=3 SHIFT=16 INPUT_WINDOW_VSIZE=256 DUAL_IP=1 NUM_OUTPUTS=2"
    #cint16/int16PORT_API=1
    test_arr[$LINENO]="make all PORT_API=1 DATA_TYPE=cint16 COEFF_TYPE=int16 FIR_LEN=32 CASC_LEN=1 SHIFT=16 INPUT_WINDOW_VSIZE=256"
    test_arr[$LINENO]="make all PORT_API=1 DATA_TYPE=cint16 COEFF_TYPE=int16 FIR_LEN=16 CASC_LEN=1 SHIFT=16 INPUT_WINDOW_VSIZE=256 DUAL_IP=1 NUM_OUTPUTS=2"
    test_arr[$LINENO]="make all PORT_API=1 DATA_TYPE=cint16 COEFF_TYPE=int16 FIR_LEN=96 CASC_LEN=3 SHIFT=16 INPUT_WINDOW_VSIZE=256"
    test_arr[$LINENO]="make all PORT_API=1 DATA_TYPE=cint16 COEFF_TYPE=int16 FIR_LEN=48 CASC_LEN=3 SHIFT=16 INPUT_WINDOW_VSIZE=256 DUAL_IP=1 NUM_OUTPUTS=2"
    #cint16/cint16PORT_API=1
    test_arr[$LINENO]="make all PORT_API=1 DATA_TYPE=int16 COEFF_TYPE=int16 FIR_LEN=16 CASC_LEN=1 SHIFT=16 INPUT_WINDOW_VSIZE=256"
    test_arr[$LINENO]="make all PORT_API=1 DATA_TYPE=int16 COEFF_TYPE=int16 FIR_LEN=8  CASC_LEN=1 SHIFT=16 INPUT_WINDOW_VSIZE=256 DUAL_IP=1 NUM_OUTPUTS=2"
    test_arr[$LINENO]="make all PORT_API=1 DATA_TYPE=int16 COEFF_TYPE=int16 FIR_LEN=48 CASC_LEN=3 SHIFT=16 INPUT_WINDOW_VSIZE=256"
    test_arr[$LINENO]="make all PORT_API=1 DATA_TYPE=int16 COEFF_TYPE=int16 FIR_LEN=24 CASC_LEN=3 SHIFT=16 INPUT_WINDOW_VSIZE=256 DUAL_IP=1 NUM_OUTPUTS=2"
    #int32/int32PORT_API=1
    test_arr[$LINENO]="make all PORT_API=1 DATA_TYPE=int32 COEFF_TYPE=int32 FIR_LEN=16 CASC_LEN=1 SHIFT=16 INPUT_WINDOW_VSIZE=256"
    test_arr[$LINENO]="make all PORT_API=1 DATA_TYPE=int32 COEFF_TYPE=int32 FIR_LEN=8  CASC_LEN=1 SHIFT=16 INPUT_WINDOW_VSIZE=256 DUAL_IP=1 NUM_OUTPUTS=2"
    test_arr[$LINENO]="make all PORT_API=1 DATA_TYPE=int32 COEFF_TYPE=int32 FIR_LEN=48 CASC_LEN=3 SHIFT=16 INPUT_WINDOW_VSIZE=256"
    test_arr[$LINENO]="make all PORT_API=1 DATA_TYPE=int32 COEFF_TYPE=int32 FIR_LEN=24 CASC_LEN=3 SHIFT=16 INPUT_WINDOW_VSIZE=256 DUAL_IP=1 NUM_OUTPUTS=2"
    #cint32/int16PORT_API=1
    test_arr[$LINENO]="make all PORT_API=1 DATA_TYPE=cint32 COEFF_TYPE=int16 FIR_LEN=32 CASC_LEN=1 SHIFT=16 INPUT_WINDOW_VSIZE=256"
    test_arr[$LINENO]="make all PORT_API=1 DATA_TYPE=cint32 COEFF_TYPE=int16 FIR_LEN=16 CASC_LEN=1 SHIFT=16 INPUT_WINDOW_VSIZE=256 DUAL_IP=1 NUM_OUTPUTS=2"
    test_arr[$LINENO]="make all PORT_API=1 DATA_TYPE=cint32 COEFF_TYPE=int16 FIR_LEN=96 CASC_LEN=3 SHIFT=16 INPUT_WINDOW_VSIZE=256"
    test_arr[$LINENO]="make all PORT_API=1 DATA_TYPE=cint32 COEFF_TYPE=int16 FIR_LEN=48 CASC_LEN=3 SHIFT=16 INPUT_WINDOW_VSIZE=256 DUAL_IP=1 NUM_OUTPUTS=2"
    #cint32/cint16PORT_API=1
    test_arr[$LINENO]="make all PORT_API=1 DATA_TYPE=cint32 COEFF_TYPE=cint16 FIR_LEN=16 CASC_LEN=1 SHIFT=16 INPUT_WINDOW_VSIZE=256"
    test_arr[$LINENO]="make all PORT_API=1 DATA_TYPE=cint32 COEFF_TYPE=cint16 FIR_LEN=8  CASC_LEN=1 SHIFT=16 INPUT_WINDOW_VSIZE=256 DUAL_IP=1 NUM_OUTPUTS=2"
    test_arr[$LINENO]="make all PORT_API=1 DATA_TYPE=cint32 COEFF_TYPE=cint16 FIR_LEN=48 CASC_LEN=3 SHIFT=16 INPUT_WINDOW_VSIZE=256"
    test_arr[$LINENO]="make all PORT_API=1 DATA_TYPE=cint32 COEFF_TYPE=cint16 FIR_LEN=24 CASC_LEN=3 SHIFT=16 INPUT_WINDOW_VSIZE=256 DUAL_IP=1 NUM_OUTPUTS=2"
    #cint32/int32PORT_API=1
    test_arr[$LINENO]="make all PORT_API=1 DATA_TYPE=cint32 COEFF_TYPE=int32 FIR_LEN=16 CASC_LEN=1 SHIFT=16 INPUT_WINDOW_VSIZE=256"
    test_arr[$LINENO]="make all PORT_API=1 DATA_TYPE=cint32 COEFF_TYPE=int32 FIR_LEN=8  CASC_LEN=1 SHIFT=16 INPUT_WINDOW_VSIZE=256 DUAL_IP=1 NUM_OUTPUTS=2"
    test_arr[$LINENO]="make all PORT_API=1 DATA_TYPE=cint32 COEFF_TYPE=int32 FIR_LEN=48 CASC_LEN=3 SHIFT=16 INPUT_WINDOW_VSIZE=256"
    test_arr[$LINENO]="make all PORT_API=1 DATA_TYPE=cint32 COEFF_TYPE=int32 FIR_LEN=24 CASC_LEN=3 SHIFT=16 INPUT_WINDOW_VSIZE=256 DUAL_IP=1 NUM_OUTPUTS=2"
    #cint32/cint32PORT_API=1
    test_arr[$LINENO]="make all PORT_API=1 DATA_TYPE=cint32 COEFF_TYPE=cint32 FIR_LEN=8  CASC_LEN=1 SHIFT=16 INPUT_WINDOW_VSIZE=256"
    test_arr[$LINENO]="make all PORT_API=1 DATA_TYPE=cint32 COEFF_TYPE=cint32 FIR_LEN=4  CASC_LEN=1 SHIFT=16 INPUT_WINDOW_VSIZE=256 DUAL_IP=1 NUM_OUTPUTS=2"
    test_arr[$LINENO]="make all PORT_API=1 DATA_TYPE=cint32 COEFF_TYPE=cint32 FIR_LEN=24 CASC_LEN=3 SHIFT=16 INPUT_WINDOW_VSIZE=256"
    test_arr[$LINENO]="make all PORT_API=1 DATA_TYPE=cint32 COEFF_TYPE=cint32 FIR_LEN=12 CASC_LEN=3 SHIFT=16 INPUT_WINDOW_VSIZE=256 DUAL_IP=1 NUM_OUTPUTS=2"
fi

if [[ "$*" == *broadcast* ]]
then
    test_arr[${#test_arr[*]}]="echo 'Running more extensive tests...' "


    test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=int16  FIR_LEN=30 DUMP_VCD=1 INPUT_WINDOW_VSIZE=512 CASC_LEN=2 GEN_INPUT_DATA=false GEN_COEFF_DATA=false SHIFT=0 DUMP_VCD=1 "
    test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=int16  FIR_LEN=30 DUMP_VCD=1 INPUT_WINDOW_VSIZE=512 CASC_LEN=3 GEN_INPUT_DATA=false GEN_COEFF_DATA=false SHIFT=0 DUMP_VCD=1 "
    test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=int16  FIR_LEN=40 DUMP_VCD=1 INPUT_WINDOW_VSIZE=512 CASC_LEN=2 GEN_INPUT_DATA=false GEN_COEFF_DATA=false SHIFT=0 DUMP_VCD=1 "
    test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=int16  FIR_LEN=40 DUMP_VCD=1 INPUT_WINDOW_VSIZE=512 CASC_LEN=3 GEN_INPUT_DATA=false GEN_COEFF_DATA=false SHIFT=0 DUMP_VCD=1 "

    test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=int16  FIR_LEN=30 DUMP_VCD=1 INPUT_WINDOW_VSIZE=512 CASC_LEN=2"
    test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=int16  FIR_LEN=30 DUMP_VCD=1 INPUT_WINDOW_VSIZE=512 CASC_LEN=3"
    test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=int16  FIR_LEN=40 DUMP_VCD=1 INPUT_WINDOW_VSIZE=512 CASC_LEN=2"
    test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=int16  FIR_LEN=40 DUMP_VCD=1 INPUT_WINDOW_VSIZE=512 CASC_LEN=3"

    test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=int16  FIR_LEN=30 DUMP_VCD=1 INPUT_WINDOW_VSIZE=512 CASC_LEN=2 GEN_INPUT_DATA=false GEN_COEFF_DATA=false SHIFT=0 DUMP_VCD=1 TARGET=x86sim"
    test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=int16  FIR_LEN=30 DUMP_VCD=1 INPUT_WINDOW_VSIZE=512 CASC_LEN=3 GEN_INPUT_DATA=false GEN_COEFF_DATA=false SHIFT=0 DUMP_VCD=1 TARGET=x86sim"
    test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=int16  FIR_LEN=40 DUMP_VCD=1 INPUT_WINDOW_VSIZE=512 CASC_LEN=2 GEN_INPUT_DATA=false GEN_COEFF_DATA=false SHIFT=0 DUMP_VCD=1 TARGET=x86sim"
    test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=int16  FIR_LEN=40 DUMP_VCD=1 INPUT_WINDOW_VSIZE=512 CASC_LEN=3 GEN_INPUT_DATA=false GEN_COEFF_DATA=false SHIFT=0 DUMP_VCD=1 TARGET=x86sim"

    test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=int16  FIR_LEN=30 DUMP_VCD=1 INPUT_WINDOW_VSIZE=512 CASC_LEN=2 TARGET=x86sim"
    test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=int16  FIR_LEN=30 DUMP_VCD=1 INPUT_WINDOW_VSIZE=512 CASC_LEN=3 TARGET=x86sim"
    test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=int16  FIR_LEN=40 DUMP_VCD=1 INPUT_WINDOW_VSIZE=512 CASC_LEN=2 TARGET=x86sim"
    test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=int16  FIR_LEN=40 DUMP_VCD=1 INPUT_WINDOW_VSIZE=512 CASC_LEN=3 TARGET=x86sim"

    # test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=cint16  FIR_LEN=30 SHIFT=15 DUMP_VCD=1 INPUT_WINDOW_VSIZE=512 CASC_LEN=2 "
    # test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=cint16  FIR_LEN=30 SHIFT=15 DUMP_VCD=1 INPUT_WINDOW_VSIZE=512 CASC_LEN=3 "

    # test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=cint16  FIR_LEN=40 SHIFT=15 DUMP_VCD=1 INPUT_WINDOW_VSIZE=512 CASC_LEN=2 "
    # test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=cint16  FIR_LEN=40 SHIFT=15 DUMP_VCD=1 INPUT_WINDOW_VSIZE=512 CASC_LEN=3 "

    # test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=int16  FIR_LEN=30 SHIFT=15 DUMP_VCD=1 INPUT_WINDOW_VSIZE=512 CASC_LEN=2 "
    # test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=int16  FIR_LEN=30 SHIFT=15 DUMP_VCD=1 INPUT_WINDOW_VSIZE=512 CASC_LEN=3 "

    # test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=int16  FIR_LEN=40 SHIFT=15 DUMP_VCD=1 INPUT_WINDOW_VSIZE=512 CASC_LEN=2 "
    # test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=int16  FIR_LEN=40 SHIFT=15 DUMP_VCD=1 INPUT_WINDOW_VSIZE=512 CASC_LEN=3 "

    # test_arr[$LINENO]="make all DATA_TYPE=int32 COEFF_TYPE=int32 FIR_LEN=30 CASC_LEN=2 SHIFT=16 DUMP_VCD=1 INPUT_WINDOW_VSIZE=256 USE_COEFF_RELOAD=0 USE_CHAIN=1"
    # test_arr[$LINENO]="make all DATA_TYPE=int32 COEFF_TYPE=int32 FIR_LEN=30 CASC_LEN=3 SHIFT=16 DUMP_VCD=1 INPUT_WINDOW_VSIZE=256 USE_COEFF_RELOAD=0 USE_CHAIN=1"
    # test_arr[$LINENO]="make all DATA_TYPE=int32 COEFF_TYPE=int32 FIR_LEN=40 CASC_LEN=2 SHIFT=16 DUMP_VCD=1 INPUT_WINDOW_VSIZE=256 USE_COEFF_RELOAD=0 USE_CHAIN=1"

fi

if [[ "$*" == *cascade* || "$*" == "" ]]
then
    test_arr[${#test_arr[*]}]="echo 'Running more extensive tests...' "
    # lengths=(16 17 18 19 20 21 22 23 64 65 66 67 68 69 70 71)
    lengths=(30 32 40 48 56 64)
    # lengths=(16 17 18 19 20 21 22 23)
    # lengths=(64 65 66 67 68 69 70 71)

    test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=int16  FIR_LEN=30 DUMP_VCD=1 INPUT_WINDOW_VSIZE=512 CASC_LEN=1 GEN_INPUT_DATA=false GEN_COEFF_DATA=false SHIFT=0"
    test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=int16  FIR_LEN=30 DUMP_VCD=1 INPUT_WINDOW_VSIZE=512 CASC_LEN=2 GEN_INPUT_DATA=false GEN_COEFF_DATA=false SHIFT=0"
    test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=int16  FIR_LEN=30 DUMP_VCD=1 INPUT_WINDOW_VSIZE=512 CASC_LEN=3 GEN_INPUT_DATA=false GEN_COEFF_DATA=false SHIFT=0"

    for fLen in "${lengths[@]}"; do
        #Cascade length 1
        test_arr[$LINENO]="make all   DATA_TYPE=int16 COEFF_TYPE=int16  FIR_LEN=$fLen SHIFT=16 CASC_LEN=2 TARGET=x86sim"
        test_arr[$LINENO]="make all   DATA_TYPE=cint16 COEFF_TYPE=int16  FIR_LEN=$fLen SHIFT=16 CASC_LEN=2 TARGET=x86sim"
        test_arr[$LINENO]="make all   DATA_TYPE=cint16 COEFF_TYPE=cint16 FIR_LEN=$fLen SHIFT=16 CASC_LEN=2 TARGET=x86sim"
        test_arr[$LINENO]="make all   DATA_TYPE=int32 COEFF_TYPE=int16  FIR_LEN=$fLen SHIFT=16 CASC_LEN=2 TARGET=x86sim"
        test_arr[$LINENO]="make all   DATA_TYPE=int32 COEFF_TYPE=int32  FIR_LEN=$fLen SHIFT=16 CASC_LEN=2 TARGET=x86sim"
        # test_arr[$LINENO]="make all   DATA_TYPE=cint32 COEFF_TYPE=int16  FIR_LEN=$fLen SHIFT=16 CASC_LEN=2"
        # test_arr[$LINENO]="make all   DATA_TYPE=cint32 COEFF_TYPE=cint16 FIR_LEN=$fLen SHIFT=16 CASC_LEN=2"
        # test_arr[$LINENO]="make all   DATA_TYPE=cint32 COEFF_TYPE=int32  FIR_LEN=$fLen SHIFT=16 CASC_LEN=2"
        # test_arr[$LINENO]="make all   DATA_TYPE=cint32 COEFF_TYPE=cint32 FIR_LEN=$fLen SHIFT=16 CASC_LEN=2"
        done #fLen

    for fLen in "${lengths[@]}"; do
        #Cascade length 1
        test_arr[$LINENO]="make all   DATA_TYPE=int16 COEFF_TYPE=int16  FIR_LEN=$fLen SHIFT=16 CASC_LEN=3 TARGET=x86sim"
        test_arr[$LINENO]="make all   DATA_TYPE=cint16 COEFF_TYPE=int16  FIR_LEN=$fLen SHIFT=16 CASC_LEN=3 TARGET=x86sim"
        test_arr[$LINENO]="make all   DATA_TYPE=cint16 COEFF_TYPE=cint16 FIR_LEN=$fLen SHIFT=16 CASC_LEN=3 TARGET=x86sim"
        test_arr[$LINENO]="make all   DATA_TYPE=int32 COEFF_TYPE=int16  FIR_LEN=$fLen SHIFT=16 CASC_LEN=3 TARGET=x86sim"
        test_arr[$LINENO]="make all   DATA_TYPE=int32 COEFF_TYPE=int32  FIR_LEN=$fLen SHIFT=16 CASC_LEN=3 TARGET=x86sim"
        # test_arr[$LINENO]="make all   DATA_TYPE=cint32 COEFF_TYPE=int16  FIR_LEN=$fLen SHIFT=16 CASC_LEN=3"
        # test_arr[$LINENO]="make all   DATA_TYPE=cint32 COEFF_TYPE=cint16 FIR_LEN=$fLen SHIFT=16 CASC_LEN=3"
        # test_arr[$LINENO]="make all   DATA_TYPE=cint32 COEFF_TYPE=int32  FIR_LEN=$fLen SHIFT=16 CASC_LEN=3"
        # test_arr[$LINENO]="make all   DATA_TYPE=cint32 COEFF_TYPE=cint32 FIR_LEN=$fLen SHIFT=16 CASC_LEN=3"
        done #fLen

    for fLen in "${lengths[@]}"; do
        #Cascade length 1
        test_arr[$LINENO]="make all   DATA_TYPE=int16 COEFF_TYPE=int16  FIR_LEN=$fLen SHIFT=16 CASC_LEN=2 TARGET=x86sim"
        test_arr[$LINENO]="make all   DATA_TYPE=cint16 COEFF_TYPE=int16  FIR_LEN=$fLen SHIFT=16 CASC_LEN=2 TARGET=x86sim"
        test_arr[$LINENO]="make all   DATA_TYPE=cint16 COEFF_TYPE=cint16 FIR_LEN=$fLen SHIFT=16 CASC_LEN=2 TARGET=x86sim"
        test_arr[$LINENO]="make all   DATA_TYPE=int32 COEFF_TYPE=int16  FIR_LEN=$fLen SHIFT=16 CASC_LEN=2 TARGET=x86sim"
        test_arr[$LINENO]="make all   DATA_TYPE=int32 COEFF_TYPE=int32  FIR_LEN=$fLen SHIFT=16 CASC_LEN=2 TARGET=x86sim"
        # test_arr[$LINENO]="make all   DATA_TYPE=cint32 COEFF_TYPE=int16  FIR_LEN=$fLen SHIFT=16 CASC_LEN=2"
        # test_arr[$LINENO]="make all   DATA_TYPE=cint32 COEFF_TYPE=cint16 FIR_LEN=$fLen SHIFT=16 CASC_LEN=2"
        # test_arr[$LINENO]="make all   DATA_TYPE=cint32 COEFF_TYPE=int32  FIR_LEN=$fLen SHIFT=16 CASC_LEN=2"
        # test_arr[$LINENO]="make all   DATA_TYPE=cint32 COEFF_TYPE=cint32 FIR_LEN=$fLen SHIFT=16 CASC_LEN=2"
        done #fLen

    for fLen in "${lengths[@]}"; do
        #Cascade length 1
        test_arr[$LINENO]="make all   DATA_TYPE=int16 COEFF_TYPE=int16  FIR_LEN=$fLen SHIFT=16 CASC_LEN=3 TARGET=x86sim"
        test_arr[$LINENO]="make all   DATA_TYPE=cint16 COEFF_TYPE=int16  FIR_LEN=$fLen SHIFT=16 CASC_LEN=3 TARGET=x86sim"
        test_arr[$LINENO]="make all   DATA_TYPE=cint16 COEFF_TYPE=cint16 FIR_LEN=$fLen SHIFT=16 CASC_LEN=3 TARGET=x86sim"
        test_arr[$LINENO]="make all   DATA_TYPE=int32 COEFF_TYPE=int16  FIR_LEN=$fLen SHIFT=16 CASC_LEN=3 TARGET=x86sim"
        test_arr[$LINENO]="make all   DATA_TYPE=int32 COEFF_TYPE=int32  FIR_LEN=$fLen SHIFT=16 CASC_LEN=3 TARGET=x86sim"
        # test_arr[$LINENO]="make all   DATA_TYPE=cint32 COEFF_TYPE=int16  FIR_LEN=$fLen SHIFT=16 CASC_LEN=3"
        # test_arr[$LINENO]="make all   DATA_TYPE=cint32 COEFF_TYPE=cint16 FIR_LEN=$fLen SHIFT=16 CASC_LEN=3"
        # test_arr[$LINENO]="make all   DATA_TYPE=cint32 COEFF_TYPE=int32  FIR_LEN=$fLen SHIFT=16 CASC_LEN=3"
        # test_arr[$LINENO]="make all   DATA_TYPE=cint32 COEFF_TYPE=cint32 FIR_LEN=$fLen SHIFT=16 CASC_LEN=3"
        done #fLen

fi

if [[ "$*" == *casc_1* || "$*" == "" ]]
then
    test_arr[${#test_arr[*]}]="echo 'Running more extensive tests...' "
    # lengths=(16 17 18 19 20 21 22 23 64 65 66 67 68 69 70 71)
    lengths=( 36 72 80)
    # lengths=(16 17 18 19 20 21 22 23)
    # lengths=(64 65 66 67 68 69 70 71)

    # test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=int16  FIR_LEN=30 DUMP_VCD=1 INPUT_WINDOW_VSIZE=512 CASC_LEN=1 GEN_INPUT_DATA=false GEN_COEFF_DATA=false SHIFT=0"
    # test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=int16  FIR_LEN=30 DUMP_VCD=1 INPUT_WINDOW_VSIZE=512 CASC_LEN=2 GEN_INPUT_DATA=false GEN_COEFF_DATA=false SHIFT=0"
    # test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=int16  FIR_LEN=30 DUMP_VCD=1 INPUT_WINDOW_VSIZE=512 CASC_LEN=3 GEN_INPUT_DATA=false GEN_COEFF_DATA=false SHIFT=0"


    for fLen in "${lengths[@]}"; do
        #Cascade length 1
        test_arr[$LINENO]="make all   DATA_TYPE=int16 COEFF_TYPE=int16  FIR_LEN=$fLen SHIFT=16 CASC_LEN=1"
        test_arr[$LINENO]="make all   DATA_TYPE=cint16 COEFF_TYPE=int16  FIR_LEN=$fLen SHIFT=16 CASC_LEN=1"
        test_arr[$LINENO]="make all   DATA_TYPE=cint16 COEFF_TYPE=cint16 FIR_LEN=$fLen SHIFT=16 CASC_LEN=1"
        test_arr[$LINENO]="make all   DATA_TYPE=int32 COEFF_TYPE=int16  FIR_LEN=$fLen SHIFT=16 CASC_LEN=1"
        test_arr[$LINENO]="make all   DATA_TYPE=int32 COEFF_TYPE=int32  FIR_LEN=$fLen SHIFT=16 CASC_LEN=1"
        # test_arr[$LINENO]="make all   DATA_TYPE=cint32 COEFF_TYPE=int16  FIR_LEN=$fLen SHIFT=16 CASC_LEN=1"
        # test_arr[$LINENO]="make all   DATA_TYPE=cint32 COEFF_TYPE=cint16 FIR_LEN=$fLen SHIFT=16 CASC_LEN=1"
        # test_arr[$LINENO]="make all   DATA_TYPE=cint32 COEFF_TYPE=int32  FIR_LEN=$fLen SHIFT=16 CASC_LEN=1"
        # test_arr[$LINENO]="make all   DATA_TYPE=cint32 COEFF_TYPE=cint32 FIR_LEN=$fLen SHIFT=16 CASC_LEN=1"
        test_arr[$LINENO]="make all   DATA_TYPE=int16 COEFF_TYPE=int16  FIR_LEN=$fLen SHIFT=16 CASC_LEN=1 TARGET=x86sim"
        test_arr[$LINENO]="make all   DATA_TYPE=cint16 COEFF_TYPE=int16  FIR_LEN=$fLen SHIFT=16 CASC_LEN=1 TARGET=x86sim"
        test_arr[$LINENO]="make all   DATA_TYPE=cint16 COEFF_TYPE=cint16 FIR_LEN=$fLen SHIFT=16 CASC_LEN=1 TARGET=x86sim"
        test_arr[$LINENO]="make all   DATA_TYPE=int32 COEFF_TYPE=int16  FIR_LEN=$fLen SHIFT=16 CASC_LEN=1 TARGET=x86sim"
        test_arr[$LINENO]="make all   DATA_TYPE=int32 COEFF_TYPE=int32  FIR_LEN=$fLen SHIFT=16 CASC_LEN=1 TARGET=x86sim"
        done #fLen

    for fLen in "${lengths[@]}"; do
        #Cascade length 1
        test_arr[$LINENO]="make all   DATA_TYPE=int16 COEFF_TYPE=int16  FIR_LEN=$fLen SHIFT=16 CASC_LEN=2 "
        test_arr[$LINENO]="make all   DATA_TYPE=cint16 COEFF_TYPE=int16  FIR_LEN=$fLen SHIFT=16 CASC_LEN=2 "
        test_arr[$LINENO]="make all   DATA_TYPE=cint16 COEFF_TYPE=cint16 FIR_LEN=$fLen SHIFT=16 CASC_LEN=2 "
        test_arr[$LINENO]="make all   DATA_TYPE=int32 COEFF_TYPE=int16  FIR_LEN=$fLen SHIFT=16 CASC_LEN=2 "
        test_arr[$LINENO]="make all   DATA_TYPE=int32 COEFF_TYPE=int32  FIR_LEN=$fLen SHIFT=16 CASC_LEN=2 "
        # test_arr[$LINENO]="make all   DATA_TYPE=cint32 COEFF_TYPE=int16  FIR_LEN=$fLen SHIFT=16 CASC_LEN=2"
        # test_arr[$LINENO]="make all   DATA_TYPE=cint32 COEFF_TYPE=cint16 FIR_LEN=$fLen SHIFT=16 CASC_LEN=2"
        # test_arr[$LINENO]="make all   DATA_TYPE=cint32 COEFF_TYPE=int32  FIR_LEN=$fLen SHIFT=16 CASC_LEN=2"
        # test_arr[$LINENO]="make all   DATA_TYPE=cint32 COEFF_TYPE=cint32 FIR_LEN=$fLen SHIFT=16 CASC_LEN=2"
        test_arr[$LINENO]="make all   DATA_TYPE=int16 COEFF_TYPE=int16  FIR_LEN=$fLen SHIFT=16 CASC_LEN=2 TARGET=x86sim"
        test_arr[$LINENO]="make all   DATA_TYPE=cint16 COEFF_TYPE=int16  FIR_LEN=$fLen SHIFT=16 CASC_LEN=2 TARGET=x86sim"
        test_arr[$LINENO]="make all   DATA_TYPE=cint16 COEFF_TYPE=cint16 FIR_LEN=$fLen SHIFT=16 CASC_LEN=2 TARGET=x86sim"
        test_arr[$LINENO]="make all   DATA_TYPE=int32 COEFF_TYPE=int16  FIR_LEN=$fLen SHIFT=16 CASC_LEN=2 TARGET=x86sim"
        test_arr[$LINENO]="make all   DATA_TYPE=int32 COEFF_TYPE=int32  FIR_LEN=$fLen SHIFT=16 CASC_LEN=2 TARGET=x86sim"
        done #fLen

    for fLen in "${lengths[@]}"; do
        #Cascade length 1
        test_arr[$LINENO]="make all   DATA_TYPE=int16 COEFF_TYPE=int16  FIR_LEN=$fLen SHIFT=16 CASC_LEN=3 DUMP_VCD=0"
        test_arr[$LINENO]="make all   DATA_TYPE=cint16 COEFF_TYPE=int16  FIR_LEN=$fLen SHIFT=16 CASC_LEN=3 DUMP_VCD=0"
        test_arr[$LINENO]="make all   DATA_TYPE=cint16 COEFF_TYPE=cint16 FIR_LEN=$fLen SHIFT=16 CASC_LEN=3 DUMP_VCD=0"
        test_arr[$LINENO]="make all   DATA_TYPE=int32 COEFF_TYPE=int16  FIR_LEN=$fLen SHIFT=16 CASC_LEN=3 DUMP_VCD=0"
        test_arr[$LINENO]="make all   DATA_TYPE=int32 COEFF_TYPE=int32  FIR_LEN=$fLen SHIFT=16 CASC_LEN=3 DUMP_VCD=0"
        # test_arr[$LINENO]="make all   DATA_TYPE=cint32 COEFF_TYPE=int16  FIR_LEN=$fLen SHIFT=16 CASC_LEN=3"
        # test_arr[$LINENO]="make all   DATA_TYPE=cint32 COEFF_TYPE=cint16 FIR_LEN=$fLen SHIFT=16 CASC_LEN=3"
        # test_arr[$LINENO]="make all   DATA_TYPE=cint32 COEFF_TYPE=int32  FIR_LEN=$fLen SHIFT=16 CASC_LEN=3"
        # test_arr[$LINENO]="make all   DATA_TYPE=cint32 COEFF_TYPE=cint32 FIR_LEN=$fLen SHIFT=16 CASC_LEN=3"
        test_arr[$LINENO]="make all   DATA_TYPE=int16 COEFF_TYPE=int16  FIR_LEN=$fLen SHIFT=16 CASC_LEN=3 TARGET=x86sim"
        test_arr[$LINENO]="make all   DATA_TYPE=cint16 COEFF_TYPE=int16  FIR_LEN=$fLen SHIFT=16 CASC_LEN=3 TARGET=x86sim"
        test_arr[$LINENO]="make all   DATA_TYPE=cint16 COEFF_TYPE=cint16 FIR_LEN=$fLen SHIFT=16 CASC_LEN=3 TARGET=x86sim"
        test_arr[$LINENO]="make all   DATA_TYPE=int32 COEFF_TYPE=int16  FIR_LEN=$fLen SHIFT=16 CASC_LEN=3 TARGET=x86sim"
        test_arr[$LINENO]="make all   DATA_TYPE=int32 COEFF_TYPE=int32  FIR_LEN=$fLen SHIFT=16 CASC_LEN=3 TARGET=x86sim"
        done #fLen

fi

if [[  "$*" == *reload_coeff* ]]
then
    # Tests RTP with a wide variety of FIR lengths with various cascade lengths, expanding coverage scope.
    target=hw
    lengths=(23 27 31 43 59)
    for fLen in "${lengths[@]}"; do
        # cascde length 1/3
        #coefficient static
        test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=int16 TARGET=$target FIR_LEN=$fLen CASC_LEN=1 SHIFT=16 INPUT_WINDOW_VSIZE=384 USE_COEFF_RELOAD=0 "
        test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=int16 TARGET=$target FIR_LEN=$fLen CASC_LEN=3 SHIFT=16 INPUT_WINDOW_VSIZE=384 USE_COEFF_RELOAD=0 "
        # coefficient reload DP=0
        test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=int16 TARGET=$target FIR_LEN=$fLen CASC_LEN=1 SHIFT=16 INPUT_WINDOW_VSIZE=384 USE_COEFF_RELOAD=1 "
        test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=int16 TARGET=$target FIR_LEN=$fLen CASC_LEN=3 SHIFT=16 INPUT_WINDOW_VSIZE=384 USE_COEFF_RELOAD=1 "
        done #fLen
    lengths=(39 43 59)
    for fLen in "${lengths[@]}"; do
        # cascde length >=5
        #coefficient static
        test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=int16 TARGET=$target FIR_LEN=$fLen CASC_LEN=5 SHIFT=16 INPUT_WINDOW_VSIZE=384 USE_COEFF_RELOAD=0 "
        # coefficient reload
        test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=int16 TARGET=$target FIR_LEN=$fLen CASC_LEN=5 SHIFT=16 INPUT_WINDOW_VSIZE=384 USE_COEFF_RELOAD=1 "
        done #fLen
fi
if [[  "$*" == *reload_check_comp_size* ]]
then
    # Tests RTPs with all the distinctive sizes in RTP comparison and update structure
    target=hw
    lengths=(64 65 66 67 68 69 70 71 72 73 74 75 76 77 78 79 80 81 82 83 84 85 86 87 88 89 90 81 92 93 94)
    for fLen in "${lengths[@]}"; do
        #coefficient static
        test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=int16  TARGET=$target FIR_LEN=$fLen CASC_LEN=5 SHIFT=16 INPUT_WINDOW_VSIZE=384 USE_COEFF_RELOAD=0 "
        # coefficient reload
        test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=int16  TARGET=$target FIR_LEN=$fLen CASC_LEN=5 SHIFT=16 INPUT_WINDOW_VSIZE=384 USE_COEFF_RELOAD=1 DEBUG_ADL=1"
        test_arr[$LINENO]="make all DATA_TYPE=int32 COEFF_TYPE=int16  TARGET=$target FIR_LEN=$fLen CASC_LEN=5 SHIFT=16 INPUT_WINDOW_VSIZE=384 USE_COEFF_RELOAD=1 DEBUG_ADL=1"
        test_arr[$LINENO]="make all DATA_TYPE=cint32 COEFF_TYPE=int16  TARGET=$target FIR_LEN=$fLen CASC_LEN=5 SHIFT=16 INPUT_WINDOW_VSIZE=384 USE_COEFF_RELOAD=1 DEBUG_ADL=1"
        done #fLen
    # lengths=(64 65 66 67 68 69 70 71 72 73 74 75 76 77 78 79)
    lengths=(64 65 66 67 68 69 70 71 )
    for fLen in "${lengths[@]}"; do
        # coefficient reload
        test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=cint16 TARGET=$target FIR_LEN=$fLen CASC_LEN=5 SHIFT=16 INPUT_WINDOW_VSIZE=384 USE_COEFF_RELOAD=1 DEBUG_ADL=1"
        test_arr[$LINENO]="make all DATA_TYPE=int32 COEFF_TYPE=int32  TARGET=$target FIR_LEN=$fLen CASC_LEN=5 SHIFT=16 INPUT_WINDOW_VSIZE=384 USE_COEFF_RELOAD=1 DEBUG_ADL=1"
        test_arr[$LINENO]="make all DATA_TYPE=cint32 COEFF_TYPE=cint16 TARGET=$target FIR_LEN=$fLen CASC_LEN=5 SHIFT=16 INPUT_WINDOW_VSIZE=384 USE_COEFF_RELOAD=1 DEBUG_ADL=1"
        test_arr[$LINENO]="make all DATA_TYPE=cint32 COEFF_TYPE=int32  TARGET=$target FIR_LEN=$fLen CASC_LEN=5 SHIFT=16 INPUT_WINDOW_VSIZE=384 USE_COEFF_RELOAD=1 DEBUG_ADL=1"
        test_arr[$LINENO]="make all DATA_TYPE=cint32 COEFF_TYPE=cint32 TARGET=$target FIR_LEN=$fLen CASC_LEN=5 SHIFT=16 INPUT_WINDOW_VSIZE=384 USE_COEFF_RELOAD=1 DEBUG_ADL=1"
        test_arr[$LINENO]="make all DATA_TYPE=float COEFF_TYPE=float  TARGET=$target FIR_LEN=$fLen CASC_LEN=5 SHIFT=0  INPUT_WINDOW_VSIZE=384 USE_COEFF_RELOAD=1 DEBUG_ADL=1"
        test_arr[$LINENO]="make all DATA_TYPE=cfloat COEFF_TYPE=float  TARGET=$target FIR_LEN=$fLen CASC_LEN=5 SHIFT=0  INPUT_WINDOW_VSIZE=384 USE_COEFF_RELOAD=1 DEBUG_ADL=1"
        test_arr[$LINENO]="make all DATA_TYPE=cfloat COEFF_TYPE=cfloat TARGET=$target FIR_LEN=$fLen CASC_LEN=5 SHIFT=0  INPUT_WINDOW_VSIZE=384 USE_COEFF_RELOAD=1 DEBUG_ADL=1"
        done #fLen
fi

if [[ "$*" == *more* || "$*" == "" ]]
then
    test_arr[${#test_arr[*]}]="echo 'Running more extensive tests...' "
    test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=int16  FIR_LEN=53 CASC_LEN=2 SHIFT=16 INPUT_WINDOW_VSIZE=256 USE_COEFF_RELOAD=1 "
    test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=int16  FIR_LEN=53 CASC_LEN=3 SHIFT=16 INPUT_WINDOW_VSIZE=256 USE_COEFF_RELOAD=1 "
    test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=int16  FIR_LEN=53 CASC_LEN=4 SHIFT=16 INPUT_WINDOW_VSIZE=256 USE_COEFF_RELOAD=1 "
    test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=int16  FIR_LEN=53 CASC_LEN=5 SHIFT=16 INPUT_WINDOW_VSIZE=256 USE_COEFF_RELOAD=1 "
    test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=int16  FIR_LEN=53 CASC_LEN=6 SHIFT=16 INPUT_WINDOW_VSIZE=256 USE_COEFF_RELOAD=1 "
    test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=int16  FIR_LEN=53 CASC_LEN=7 SHIFT=16 INPUT_WINDOW_VSIZE=256 USE_COEFF_RELOAD=1 "
    test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=int16  FIR_LEN=53 CASC_LEN=8 SHIFT=16 INPUT_WINDOW_VSIZE=256 USE_COEFF_RELOAD=1 "
    test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=int16  FIR_LEN=53 CASC_LEN=9 SHIFT=16 INPUT_WINDOW_VSIZE=256 USE_COEFF_RELOAD=1 "
    test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=int16  FIR_LEN=53 CASC_LEN=10 SHIFT=16 INPUT_WINDOW_VSIZE=256 USE_COEFF_RELOAD=1 "

    test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=int16  FIR_LEN=190 CASC_LEN=2 SHIFT=16 INPUT_WINDOW_VSIZE=256 USE_COEFF_RELOAD=1 "
    test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=int16  FIR_LEN=190 CASC_LEN=3 SHIFT=16 INPUT_WINDOW_VSIZE=256 USE_COEFF_RELOAD=1 "
    test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=int16  FIR_LEN=190 CASC_LEN=4 SHIFT=16 INPUT_WINDOW_VSIZE=256 USE_COEFF_RELOAD=1 "
    test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=int16  FIR_LEN=190 CASC_LEN=5 SHIFT=16 INPUT_WINDOW_VSIZE=256 USE_COEFF_RELOAD=1 "
    test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=int16  FIR_LEN=190 CASC_LEN=6 SHIFT=16 INPUT_WINDOW_VSIZE=256 USE_COEFF_RELOAD=1 "
    test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=int16  FIR_LEN=190 CASC_LEN=7 SHIFT=16 INPUT_WINDOW_VSIZE=256 USE_COEFF_RELOAD=1 "
    test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=int16  FIR_LEN=190 CASC_LEN=8 SHIFT=16 INPUT_WINDOW_VSIZE=256 USE_COEFF_RELOAD=1 "
    test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=int16  FIR_LEN=190 CASC_LEN=9 SHIFT=16 INPUT_WINDOW_VSIZE=256 USE_COEFF_RELOAD=1 "
    test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=int16  FIR_LEN=190 CASC_LEN=10 SHIFT=16 INPUT_WINDOW_VSIZE=256 USE_COEFF_RELOAD=1 "

    test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=int16  FIR_LEN=214 CASC_LEN=2 SHIFT=16 INPUT_WINDOW_VSIZE=256 USE_COEFF_RELOAD=1 "
    test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=int16  FIR_LEN=19 CASC_LEN=3 SHIFT=16 INPUT_WINDOW_VSIZE=256 USE_COEFF_RELOAD=1 "
    test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=int16  FIR_LEN=29 CASC_LEN=4 SHIFT=16 INPUT_WINDOW_VSIZE=256 USE_COEFF_RELOAD=1 "

    #test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=int16  FIR_LEN=16 SHIFT=0 COEFF_STIM_TYPE=5 "
    #test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=int16  FIR_LEN=20 SHIFT=0 COEFF_STIM_TYPE=5 "
    #test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=int16  FIR_LEN=24 SHIFT=0 COEFF_STIM_TYPE=5 "
    #test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=int16  FIR_LEN=28 SHIFT=0 COEFF_STIM_TYPE=5 "
    #test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=int16  FIR_LEN=32 SHIFT=0 COEFF_STIM_TYPE=5 "
fi

if [[  "$*" == *PR* ]]
then
    test_arr[$LINENO]="make all DATA_TYPE=cint16 COEFF_TYPE=int16  DUAL_IP=0 FIR_LEN=199 SHIFT=20 INPUT_WINDOW_VSIZE=256 DUMP_VCD=1"
fi

if [[ "$*" == *fullSweep* || "$*" == *all* ]]
then

    types=(
        int16 int16
        cint16 int16
        int32 int16
        cint32 int16
        cint16 cint16
        int32 int32
        cint32 int32
        cint32 cint32
        float float
        cfloat float
        cfloat cfloat
    )

    # Original ideal set of params to sweep across
    lengths=(4 8 12 16 20 32 44 56 64 96 128 156 188 220 240)
    windows=(4 8 16 32 64 128 156 188 220 256 320 384 512 1024)
    cascades=(1 2 3 4 5 6 7)

    #reduced to limit number of tests
    lengths=(16 20 44 56 96 128 188 240)
    windows=(16 32 64 128 188 256 320 512 1024)
    cascades=(1 2 3 5 7)

    interps=(1)
    decimates=(1)

    for (( typeI=0; typeI<${#types[@]}; typeI+=2 )); do
        echo "${types[$typeI]},${types[`expr $typeI + 1 `]}"
        dType=${types[$typeI]}
        cType=${types[`expr $typeI + 1 `]}
        # Scrape int number from coef type
        shift=${cType#*int}
        if [[ $cType == *float ]]; then
            shift=0
        fi
        for fLen in "${lengths[@]}"; do
            for iRate in "${interps[@]}"; do
                for dRate in "${decimates[@]}"; do
                    for cLen in "${cascades[@]}"; do
                        redWindows=(${windows[@]})
                        # to reduce number of tests, limited number of windows
                        # for cascades
                        if [ $cLen -gt 1 ]; then
                            redWindows=(256 1024)
                        fi
                        # Always use a window of FIR Len
                        test_arr[$LINENO]="make all DATA_TYPE=$dType COEFF_TYPE=$cType FIR_LEN=$fLen  CASC_LEN=$cLen SHIFT=$shift INPUT_WINDOW_VSIZE=$fLen"
                        #echo ${redWindows[@]}
                        for wSize in "${redWindows[@]}"; do
                            if [ $wSize -gt $fLen ]
                            then
                                #echo "DATA_TYPE=$dType COEFF_TYPE=$cType FIR_LEN=$fLen  CASC_LEN=$cLen SHIFT=$shift"
                                test_arr[$LINENO]="make all DATA_TYPE=$dType COEFF_TYPE=$cType FIR_LEN=$fLen  CASC_LEN=$cLen SHIFT=$shift INPUT_WINDOW_VSIZE=$wSize"
                                #test_arr[${#test_arr[*]}]
                                #echo "wSize $wSize"
                            fi
                        done #wSize
                    done #cLen
                done #dRate
            done #iRate
        done #fLen
    done #typeI
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