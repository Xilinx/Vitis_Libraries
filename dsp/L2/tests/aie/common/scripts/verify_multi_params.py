#
# Copyright (C) 2024-2025, Advanced Micro Devices, Inc.
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

import sys
import os
import subprocess

script_directory = os.path.dirname(os.path.abspath(__file__))
XF_DSP_ROOT = script_directory + "/../../../../.."
meta_scripts_dir = XF_DSP_ROOT + "/L2/meta/scripts"
sys.path.insert(0, meta_scripts_dir)
from metadata_api import *

wr2txt = False
for i in range(len(sys.argv)):
    if sys.argv[i] == "--ip":
        IP_name = sys.argv[i + 1]
    if sys.argv[i] == "--multi_params":
        multi_params_loc = sys.argv[i + 1]
    if sys.argv[i] == "--test_lines":
        test_lines = sys.argv[i + 1]
    if sys.argv[i] == "--test_suite":
        test_suite = sys.argv[i + 1]
    if sys.argv[i] == "--gen_test_file":
        wr2txt = True

ip_test_dir = f"{XF_DSP_ROOT}/L2/tests/aie/{IP_name}"
multi_params_canary = f"{ip_test_dir}/multi_params.json"

if not ("multi_params_loc" in locals()):
    multi_params_loc = multi_params_canary

with open(multi_params_canary) as f:  # this is done for backwards parameter mapping
    json_load = json.load(f)
# Parameter mapping to run tests
canary_test = json_load["test_0_tool_canary_aie"]


with open(multi_params_loc) as f:  # this is done for backwards parameter mapping
    json_load = json.load(f)

modified_args_list = []
for test_case in json_load:
    modified_args_list.append(json_load[test_case])

script_path = f"{XF_DSP_ROOT}/L2/tests/aie/common/scripts/config_translation.tcl"

finalized_test_list = []
count_tests = 0
for test_item in modified_args_list:
    modified_test_item = {}
    for param in test_item:
        param_new = config_translate(
            param, script_path, back_translate=0, canary_test=canary_test
        )
        if test_item[param].isnumeric():
            modified_test_item.update({param_new: int(test_item[param])})
        else:
            modified_test_item.update({param_new: test_item[param]})
    finalized_test_list.append(modified_test_item)
    count_tests = count_tests + 1
args_file = f"{IP_name}_modified_args_multi_params.json"

print(f"{count_tests} tests are modified in {IP_name}'s multi_params.json file.")
print(f"Modified args written in {args_file}.")
with open(f"{ip_test_dir}/{args_file}", "w") as outfile:
    json.dump(finalized_test_list, outfile, indent=4, separators=(", ", ": "))

result = subprocess.run(
    [
        "python3",
        f"{script_directory}/gen_legal_params.py",
        "--IP",
        IP_name,
        "--inp_dir",
        f"{ip_test_dir}/{args_file}",
        "--test_lines",
        f"{test_lines}",
        "--test_suite",
        f"{test_suite}",
    ]
)
if result.returncode != 0:
    sys.exit(1)

# delete the args file
os.remove(f"{ip_test_dir}/{args_file}")
print(f"{args_file} is deleted.")
