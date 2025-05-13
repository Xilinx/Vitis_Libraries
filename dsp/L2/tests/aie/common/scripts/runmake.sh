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
echo "Running runmake located at $(readlink -f "$0")" | tee ./runmake_logs/runmake.log
pathToRunmake=$(readlink -f "$0")
pathToLibElem=$(pwd)
runmakeDir=$(dirname "$pathToRunmake")

args=("$@")
echo "inputs args are: ${args[*]}" | tee -a ./runmake_logs/runmake.log
run_tests=1
ignore_fails=""
no_test_verify=""

for i in "${!args[@]}"; do
if [[ "${args[$i]}" == '-h' ]]; then
    echo "This script converts all the tests in the specified suite .txt file into a multi_params.json file, which can then be used to run tests."
    echo ""
    echo "Required:"
    echo "  -run_type checkin | qor | <./path/to/suite.txt>"
    echo "      Specifies the name of the test suite .txt file located in L2/tests/aie/<IP>/test_suites/."
    echo "      Alternatively, a path to the .txt file can be given, but this must contain the .txt extension."
    echo ""
    echo "  A file called 'L2/tests/aie/<IP>/test_suites/default_params.txt' containing a list of default parameters."
    echo "  This will be used to populate test_0_tool_canary_aie."
    echo ""
    echo "Prepare multi_params.json for Jenkins regression:"
    echo "  -run_type jenkins"
    echo "      Using 'jenkins' as the run_type will create a multi_params.json file with canary test, checkin tests, and qor tests, ready for Jenkins regressions."
    echo ""
    echo "Optional:"
    echo "  By default, this script validates all tests, creates multi_params, and runs a batch of tests on LSF."
    echo "  If a test is found to be invalid, an error is shown, and batch tests will not launch."
    echo ""
    echo "  -no_test_run"
    echo "      Disables the launch of LSF batch testing. Useful for only validating tests and metadata."
    echo ""
    echo "  -ignore_fails"
    echo "      Tests are validated, but any potential invalid tests are ignored. They will be written to multi_params, and batch testing will continue with only legal tests."
    echo ""
    echo "  -no_test_verify"
    echo "      Disables all test verification. Required if IP does not have metadata, or static asserts are being tested."
    echo ""
    echo "Example Usage:"
    echo "  runmake.sh -run_type qor"
    echo "      Gathers tests from ./test_suites/qor.txt, validates tests, and converts to multi_params_qor.json. These are then batch tested via LSF."
    echo ""
    echo "  runmake.sh -run_type <path>/qor.txt"
    echo "      As above, but uses the specified path to qor.txt rather than assuming it is located in ./test_suites/qor.txt."
    echo ""
    echo "  runmake.sh -run_type qor -no_test_run"
    echo "      As above, but no tests are launched on LSF."
    echo ""
    echo "  runmake.sh -run_type qor -ignore_fails"
    echo "      Tests are validated, but invalid cases are only printed to the terminal. Remaining passing cases are launched in LSF."
    echo ""
    echo "  runmake.sh -run_type qor -no_test_verify"
    echo "      No test verification is carried out."
    exit 0
fi
    if [[ "${args[$i]}" == '-ip_path' ]]
    then
        pathToLibElem=${args[($i+1)]}
    fi
    if [[ "${args[$i]}" == '-run_type' ]]
    then
        run_type=${args[($i+1)]}
    fi
    # use this option if you do not want to run tests of the generated multi_params
    if [[ "${args[$i]}" == '-no_test_run' ]]
    then
        run_tests=0
    fi
    # use this option if you want to verify tests, but ignore the failures on batch run
    if [[ "${args[$i]}" == '-ignore_fails' ]]
    then
        ignore_fails="--ignore_fails"
    fi
    # use this option if you do not want to completely disable test verification
    if [[ "${args[$i]}" == '-no_test_verify' ]]
    then
        no_test_verify="--no_test_verify"
    fi
done

echo $pathToLibElem
if [[ $run_type == "jenkins" ]]
then
    run_tests=0
fi

# Run the create_params.py script and capture its output and exit status
output=$(python3 "$runmakeDir/create_params.py" "$run_type" $ignore_fails $no_test_verify 2>&1)
exit_status=$?

# Print the output to the terminal
echo "$output" | tee /dev/tty

# Capture only the last line containing the name of the generated multi_params json file
json_file=$(echo "$output" | tail -n 1)

# If the exit status is non-zero, exit the Bash script
if [ $exit_status -ne 0 ]; then
    echo "create_params script has failed with exit code $exit_status. Fix the test validation failure or rerun runmake.sh with -ignore_fails (only legal tests will be ran, illegal will be ignored). To completely disable test validation and run potentially illegal tests "
    exit 1
fi

echo "Generated JSON file: $json_file" | tee -a ./runmake_logs/runmake.log

if [[ $run_tests == 1 ]]
then
    timestamp=$(date +"%y%m%d_%H%M")
    echo "Start of Batch Run ($timestamp)" | tee -a ./runmake_logs/runmake.log
    # run_batch currently does not accept the extension
    json_name=$(basename "$json_file" .json)
    "$runmakeDir/run_batch.sh" -func "$(basename "$pathToLibElem")" -params "$json_name" -batch_suffix "$timestamp"
fi
