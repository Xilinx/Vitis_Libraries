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
logfile=./runmake_logs/runmake.log

# Absolute path to this script
script_path=$(readlink -f "$0")
runmake_dir=$(dirname "$script_path")
pathToLibElem=$(pwd)

# Parse args
run_type=""
run_tests=1
ignore_fails=""
no_test_verify=""
static_default=""

while [[ $# -gt 0 ]]; do
    case $1 in
        -h)
            cat <<EOF
Usage: runmake.sh -run_type <suite|jenkins|qor|checkin|/path/to/suite.txt> [options]

run_type options:
  checkin    : Run the checkin suite (test_suites/checkin.txt) and run tests
  qor        : Run the qor suite (test_suites/qor.txt) and run tests
  suite      : Run the named suite in test_suites/<suite>.txt and run tests
  /path/to/suite.txt : Run a custom suite file by absolute or relative path and run tests
  jenkins    : Prepare multi_params.json for both checkin and qor suites, does NOT run tests

Options:
  -ip_path <path>         Set IP path
  -no_test_run            Only validate, do not run tests
  -ignore_fails           Ignore invalid tests, continue with valid ones
  -no_test_verify         Disable test verification
  -static_default         Enable static default mode
EOF
            exit 0
            ;;
        -ip_path)
            pathToLibElem=$2; shift 2;;
        -run_type)
            run_type=$2; shift 2;;
        -no_test_run)
            run_tests=0; shift;;
        -ignore_fails)
            ignore_fails="--ignore_fails"; shift;;
        -no_test_verify)
            no_test_verify="--no_test_verify"; shift;;
        -static_default)
            static_default="--static_default"; shift;;
        *)
            shift;;
    esac
done

echo "IP Path: $pathToLibElem" | tee -a "$logfile"
if [[ $run_type == "jenkins" ]]; then
    run_tests=0
fi

echo "Running runmake located at $script_path" | tee "$logfile"
echo "Input args: -run_type $run_type $ignore_fails $no_test_verify $static_default" | tee -a "$logfile"

# Run create_params.py
output=$(python3 "$runmake_dir/create_params.py" "$run_type" $ignore_fails $no_test_verify $static_default 2>&1)
exit_status=$?
echo "$output" | tee /dev/tty
json_file=$(echo "$output" | tail -n 1)

if [ $exit_status -ne 0 ]; then
    echo "create_params.py failed (exit $exit_status). Fix validation or use -ignore_fails/-no_test_verify." | tee -a "$logfile"
    exit 1
fi

echo "Generated JSON file: $json_file" | tee -a "$logfile"

if [[ $run_tests == 1 ]]; then
    timestamp=$(date +"%y%m%d_%H%M")
    echo "Start of Batch Run ($timestamp)" | tee -a "$logfile"
    json_name=$(basename "$json_file" .json)
    $runmake_dir/run_batch.sh -func $pathToLibElem -params "$json_name" -batch_suffix "$timestamp"
fi
