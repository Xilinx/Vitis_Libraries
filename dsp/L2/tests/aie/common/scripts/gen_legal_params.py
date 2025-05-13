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
import importlib
import math
import os


script_directory = os.path.dirname(os.path.abspath(__file__))
XF_DSP_ROOT = script_directory + "/../../../../.."
meta_scripts_dir = XF_DSP_ROOT + "/L2/meta/scripts"
sys.path.insert(0, meta_scripts_dir)
from metadata_api import *

test_line_print = False
database_wr = False
for i in range(len(sys.argv)):
    if sys.argv[i] == "--IP":
        IP_name = sys.argv[i + 1]
    if sys.argv[i] == "--inp_dir":
        inp_file_dir = sys.argv[i + 1]
    if sys.argv[i] == "--out_dir":
        out_file_dir = sys.argv[i + 1]
    if sys.argv[i] == "--db":
        database_wr = True
    if sys.argv[i] == "--test_lines":
        test_line_print = True
        test_lines = sys.argv[i + 1]
    if sys.argv[i] == "--test_suite":
        test_suite = sys.argv[i + 1]

filename = f"{XF_DSP_ROOT}/L2/tests/aie/{IP_name}/runmake.sh"
file_abs_path = os.path.abspath(filename)
if not ("out_file_dir" in locals()):
    out_file_dir = f"{XF_DSP_ROOT}/L2/tests/aie/{IP_name}"

# extact the line numbers of test_cases
if test_line_print:
    test_case_line_list = []
    test_case_line = ""
    for element in test_lines + " ":
        if element.isdigit():
            test_case_line = test_case_line + element
        else:
            test_case_line_list.append(int(test_case_line))
            test_case_line = ""

IP_dir = f"{XF_DSP_ROOT}/L2/tests/aie/{IP_name}"

try:
    module = importlib.import_module(IP_name)
except ImportError:
    print(f"Failed to import {IP_name}.")

params_json = extract_param_json(IP_name)
param_list = extract_param_list_all(params_json)
updater_dict = extract_updater_dict(params_json)
validator_dict = extract_validator_dict(params_json)

multi_params_loc = f"{IP_dir}/multi_params.json"
with open(multi_params_loc) as f:  # this is done for backwards parameter mapping
    json_load = json.load(f)
canary_test = json_load["test_0_tool_canary_aie"]

script_path = f"{XF_DSP_ROOT}/L2/tests/aie/common/scripts/config_translation.tcl"

mapped_param_list = []
mapped_param_dict = {}
for param in param_list:
    param_new = config_translate(
        param, script_path, back_translate=1, canary_test=canary_test
    )
    if (param != param_new) or (param == "AIE_VARIANT"):
        mapped_param_list.append(param)
        mapped_param_dict.update({param: param_new})

with open(inp_file_dir) as f:
    args_list = json.load(f)
param_config_list = []
fail_test_message_list = []
fail_test_error_list = []
fail_test_arg_list = []
count_legal = 0
count_illegal = 0
test_index = 0
for args in args_list:
    param_config = {}
    # for param in param_list:
    for param in args:
        # if param in args:
        param_config.update({param: args[param]})

    for param in param_list:
        if param in mapped_param_list:
            param_under_test = ip_parameter()  # generate parameter object
            param_under_test.validator = validator_dict[param]
            param_under_test.param_validate(param_config, module, print_err="True")
            if param_under_test.valid == "False":
                count_illegal += 1
                if test_line_print:
                    line_specifier = f":{test_case_line_list[test_index]}"
                    fail_msg = f"FAILED TEST CASE {count_illegal}: {file_abs_path + line_specifier}"
                    fail_test_message_list.append(fail_msg)
                    fail_test_error_list.append(param_under_test.err_msg)
                    fail_test_arg_list.append(args)
                if database_wr == True:
                    print(f"Failed Test:{args}")
                param_config_valid = "False"
                break
            else:
                param_config_valid = "True"
    if param_config_valid == "True":
        count_legal += 1
        param_config_list.append(param_config)
    test_index += 1

print(f"Number of Legal Test Cases: {count_legal}")
print(f"Number of Illegal Test Cases: {count_illegal}")

if database_wr:
    len_list = len(param_config_list)
    len_list_1k = math.floor(len_list / 1000)
    len_list_remainder = len_list % 1000

    param_config_dict = {}

    if len_list_1k != 0:
        for k in range(1, len_list_1k + 1, 1):
            param_config_dict.update({k: param_config_list[(k - 1) * 1000 : k * 1000]})

        if len_list_remainder != 0:
            param_config_dict.update(
                {k + 1: param_config_list[k * 1000 : k * 1000 + len_list_remainder]}
            )
    else:
        param_config_dict.update({1: param_config_list})

    for i in range(1, len(param_config_dict) + 1, 1):
        param_config_list_int = param_config_dict[i]
        with open(f"{out_file_dir}/{IP_name}_test_arr_database_{i}.txt", "w") as file:
            for param_list in param_config_list_int:
                make_command = f"make all NITER=16"
                for param in param_list:
                    if param in mapped_param_dict:
                        make_command = (
                            make_command
                            + f" {mapped_param_dict[param]}={param_list[param]}"
                        )
                    else:
                        make_command = make_command + f" {param}={param_list[param]}"
                file.write(make_command + "\n")
            print(
                f"Database file is generated: {out_file_dir}/{IP_name}_test_arr_database_{i}.txt"
            )
else:
    if count_illegal != 0:
        with open(f"{out_file_dir}/report_illegal_tests_{test_suite}.txt", "w") as file:
            for i in range(0, count_illegal):
                print("\n" + fail_test_message_list[i])
                print(fail_test_error_list[i])
                print("Test args:" + str(fail_test_arg_list[i]))

                file.write(fail_test_message_list[i] + "\n")
                file.write(fail_test_error_list[i] + "\n")
                file.write("Test args:" + str(fail_test_arg_list[i]) + "\n")
        print(
            f"\nIllegal test cases reported in {out_file_dir}/report_illegal_tests_{test_suite}.txt. \nExiting runmake..."
        )
        sys.exit(1)

#     with open(f"{out_file_dir}/{IP_name}_test_arr_eliminated.txt", "w") as file:
#         for param_list in param_config_list:
#             make_command=f"make all NITER=16 "
#             for param in param_list:
#                 make_command=make_command+f"{mapped_param_dict[param]}={param_list[param]} "
#             file.write(make_command+"\n")
