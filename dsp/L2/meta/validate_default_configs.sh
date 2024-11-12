#!/usr/bin/env bash
#
# Copyright (C) 2019-2022, Xilinx, Inc.
# Copyright (C) 2022-2024, Advanced Micro Devices, Inc.
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

# We should run this script before checking in constraints updates. 
# Metadata is also checked at pre_build within L2/tests/aie/*/Makefile, so it will be checked upon pull request

#https://stackoverflow.com/a/59916
echo "The script you are running has basename $( basename -- "$0"; ), dirname $( dirname -- "$0"; )" 
echo "The present working directory is $( pwd; )" 

#change directory to where this script is located
echo "Changing directory to $( dirname -- "$0"; )"
cd $( dirname -- "$0"; )
echo "Starting log at $( dirname -- "$0"; )/validate_default_configs.log \n\n" |& tee "validate_default_configs.log"

config_check_count=0
# A function to run the config checker for a given library element. 
config_checker () {
    local name=$1
    echo "config_${name} : " |& tee -a "validate_default_configs.log"
    vitis -exec ipmetadata_config_checker ./${name}.json ./config/config_${name}.json -newflow |& tee "validate_single_config.log"
    # copy log across into main log file
    cat validate_single_config.log >> validate_default_configs.log
    # check if current library element failed
    local is_good=`grep "Metadata validation successful" validate_single_config.log -c`
    # print a consistent error string to grep for
    if [[ $is_good == 0  ]]; then
        echo "FAILURE: ${name}" |& tee -a "validate_default_configs.log"
    fi

    let config_check_count+=1;
}
#default to all, or test individual elements
config_to_test=${1:-all}
if [[ $config_to_test == "all" ]]; then
    echo "running all default configurations" |& tee -a "validate_default_configs.log"
    # list of library elements to test
    config_checker fir_decimate_asym
    config_checker fir_decimate_hb
    config_checker fir_decimate_sym
    config_checker fir_interpolate_asym
    config_checker fir_interpolate_hb
    config_checker fir_resampler
    config_checker fir_sr_asym
    config_checker fir_sr_sym
    config_checker fft_ifft_dit_1ch
    config_checker fft_window
    config_checker matrix_mult
    config_checker dds_mixer
    config_checker vector_matrix_mul

else 
    echo "only running $config_to_test default configuration" |& tee -a "validate_default_configs.log"
    config_checker $config_to_test
fi

good_count=`grep "Metadata validation successful" validate_default_configs.log -c`
echo "Got $good_count passed configs" |& tee -a "validate_default_configs.log"

echo "Expected $config_check_count passes" |& tee -a "validate_default_configs.log"

if [[ $good_count == $config_check_count  ]]
then
    echo "PASS: All good!" |& tee -a "validate_default_configs.log"
else
    echo "FAIL: Expected config passes: $config_check_count. Metadata passes: $good_count " |& tee -a "validate_default_configs.log"
    echo `grep FAILURE validate_default_configs.log` |& tee -a "validate_default_configs.log"
fi
