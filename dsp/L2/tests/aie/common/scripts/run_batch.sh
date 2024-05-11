#!/bin/bash
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
pathToScript=$(readlink -f "$0")
scriptDir=$(dirname $pathToScript)
args=("$@")
# specify results path, default "results" will be in current working directory
path_to_results="./results"
json_file="multi_params"
for i in "${!args[@]}"; do
    if [[ "${args[$i]}" == '-func' ]]
    then
        libelemName=${args[($i+1)]}
        echo $libelemName
    fi
    if [[ "${args[$i]}" == '-params' ]]
    then
        json_file=${args[($i+1)]}
        echo $json_file
    fi
    if [[ "${args[$i]}" == '-batch_suffix' ]]
    then
        suffix=${args[($i+1)]}
        echo $suffix
    fi
done

funcDir=$(dirname "$(dirname "$scriptDir")")/$libelemName
echo "Start of Batch Run $funcDir/results/batch_${json_file}_${suffix}" | tee $funcDir/runmake_logs/runmake.log
# Read keys into an array  
mapfile -t json_test_names < <(python -c "import json; print('\n'.join(json.load(open('$funcDir/$json_file.json')).keys()))")  
makeCmd=()
test_num=0
en_pwr_calc=0

# Print keys  
for test_name in "${json_test_names[@]}"; do  
    if [[ "$test_name" == *aie2* ]]; then
            platform="vek280"
    else
            platform="vck190"
    fi
    if [[ "$test_name" == *x86sim* ]]; then
        target_uut="x86sim"
    else
        target_uut="aiesim"
    fi

    updated_cmd="make -f $scriptDir/run_pack.mk all_pack PLATFORM=${platform} PARAMS=${test_name} PARAMS_FILE=${json_file}.json RESULTS_DIR=$funcDir/results/batch_${json_file}_${suffix}/test_${test_name} TARGET=${target_uut} UUT_KERNEL=$libelemName"
    makeCmd[${#makeCmd[*]}]=$updated_cmd
    test_num=`expr $test_num + 1`
done  

num_tests=${#makeCmd[@]}
echo "There are $num_tests tests to be run." | tee -a ./runmake_logs/runmake.log

# Log number of test cases scheduled
pushd $funcDir
mkdir -p "./logs"
echo "$num_tests" | tee ./logs/schedule.log
popd

# Run tests with as many parallel jobs as possible (-P 0)
#printf '%s\n' "${test_arr[@]}" | xargs -P 0 -I % sh -c "%";
if [[ "$*" == *-lsfSubmit* ]]
then
    echo "LsfJob: ${LSB_JOBID}[${LSB_JOBINDEX}] : ${LSB_JOBNAME} "
    #index from 0
    let job_i=${LSB_JOBINDEX}-1
    # should only ever be one command.
    printf '%s\n' "${makeCmd[$job_i]}" | xargs -P 0 -I % sh -c "%";
    exit 0
elif [[ "$*" == *-nolsf* ]]; then
    # Run all tests with as many parrallel jobs as possible (-P 0)
    printf '%s\n' "${makeCmd[@]}" | xargs -P 0 -I % sh -c "%";

else
    # Each make should only take around 30 min, so 1hr max on short queue is fine.
    # Pretty sure we need x86 arch for the simulations
    # We need >ws6 because GLIBC requirements in Vivado/Vitis.
    bsub -env "all" -q short -R "rusage[mem=16384] select[(type==X86_64) && (osver == ws7 || osver == cent7)]" -oo runmake_logs/LSFrunmake_%I.log -eo runmake_logs/LSFrunmake_err_%I.log -J DSPLIB_run_batch[1-$num_tests] $pathToScript -lsfSubmit ${args[@]}
    sleep 15
    echo "Going to wait on end of job array. Feel free to stick this in the background. "
    #bjobs
    bwait -w 'ended(DSPLIB_run_batch)'
    echo "Completed batch"
    echo "Batch results: " > runmake_logs/LSFrunmake.log
    echo "Batch results errors: " > runmake_logs/LSFrunmake_err.log
    # Join all the runmake into a single runmake, respecting their order as specified in this file
    for i in "${!makeCmd[@]}"; do
        let j=i+1
        echo "LSFrunmake_${j}.log contents:" >> runmake_logs/LSFrunmake.log
        cat runmake_logs/LSFrunmake_${j}.log >> runmake_logs/LSFrunmake.log
        echo "LSFrunmake_err_${j}.log contents:" >> runmake_logs/LSFrunmake_err.log
        cat runmake_logs/LSFrunmake_err_${j}.log >> runmake_logs/LSFrunmake_err.log
        rm -f runmake_logs/LSFrunmake_${j}.log
        rm -f runmake_logs/LSFrunmake_err_${j}.log
    done
fi

# We still have the runmake in all of the commands so can piece together regardless of LSF.
for i in "${!test_arr[@]}"; do
    cat runmake_logs/runmake_${i}.log >> runmake_logs/runmake.log
    rm -f runmake_logs/runmake_${i}.log
done

echo "Check runmake_logs/runmake.log for full log of ran commands. Check runmake_logs/LSFrunmake.log for make command log along with job resource stats. "
echo `grep -H  "FUNC: .* 1" $funcDir/results/batch_${json_file}_${suffix}/*/logs/status* | wc -l` of $num_tests tests have passed  

echo Generating summary yaml file and html report
bash $DSPLIB_SCRIPTS/scripts/html_reporting/create_html_report.sh -batch $funcDir/results/batch_${json_file}_${suffix}
