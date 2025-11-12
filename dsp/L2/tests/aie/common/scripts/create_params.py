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

import json
import os
import sys
import importlib
import argparse

scripts_dir = os.path.dirname(os.path.abspath(__file__))
called_dir = os.getcwd()

configTranslatePath = f"{called_dir}/../common/scripts/config_translation.tcl"

L2_dir = f"{scripts_dir}/../../../.."

meta_data_path = f"{called_dir}/../../../meta"
meta_scripts_path = f"{L2_dir}/meta/scripts"
sys.path.insert(0, meta_scripts_path)
sys.path.append(meta_data_path)


from metadata_api import *

no_validation_params = ["weights", "coeff", "lookup_values"]


def load_defaults(file_path):
    defaults = {}
    try:
        with open(file_path) as file:
            for line in file:
                line = line.partition("#")[0].strip()
                if line:
                    key, value = map(str.strip, line.split("=", 1))
                    defaults[key] = value
    except FileNotFoundError:
        print(f"Error: Default parameters file not found: {file_path}")
        sys.exit(1)
    return defaults


def generate_combinations(keys, idx, current, all_combinations, params):
    if idx == len(keys):
        all_combinations.append(current.copy())
        return all_combinations
    key = keys[idx]
    for value in params[key]:
        current[key] = value
        generate_combinations(keys, idx + 1, current, all_combinations, params)
    return all_combinations


def parse_test_case(
    line,
    defaults,
    param_list,
    module,
    updater_dict,
    validator_dict,
    enable_updater_padding,
):
    params = {}
    for part in line.split()[2:]:
        key, value = part.split("=")
        value = value.strip()
        if value.startswith("[") and value.endswith("]"):
            params[key.strip()] = [v.strip() for v in value[1:-1].split(",")]
        else:
            params[key.strip()] = [value.strip()]
    keys = list(params.keys())
    all_combinations = generate_combinations(keys, 0, {}, [], params)
    all_params = []
    defaults_mapped = param_map(defaults, defaults, back_translate=0)
    for combination in all_combinations:
        params_mapped = param_map(combination, defaults, back_translate=0)
        if enable_updater_padding:
            for upd_param in updater_dict:
                param_under_test = ip_parameter()
                param_under_test.validator = validator_dict[upd_param]
                param_under_test.updater = updater_dict[upd_param]
                if upd_param in params_mapped:
                    param_under_test.param_validate(
                        params_mapped, module, print_err="False"
                    )
                    if param_under_test.valid == "False":
                        break
                elif upd_param not in no_validation_params:
                    try:
                        params_mapped[upd_param] = 0
                        param_under_test.param_update(params_mapped, module)
                        params_mapped[upd_param] = param_under_test.default
                    except:
                        print(params_mapped)
                        print(
                            f"ERROR: No legal values for {upd_param}. Bug in metadata?"
                        )
                        exit(1)
                        break
        for static_default in defaults_mapped:
            if static_default not in params_mapped:
                params_mapped[static_default] = defaults_mapped[static_default]
        all_params.append(params_mapped)
    return all_params


def create_test_name(suite_name, line_number, test):
    test_name = f"test_{line_number}"
    device = "aie1"
    target = test.pop("TARGET", "hw")
    if "AIE_VARIANT" in test:
        device = "aie" + test["AIE_VARIANT"]
    if test.get("PART") == "xcve2802-vsvh1760-2MP-e-S":
        device = "aie2"
        test.pop("PART")
    # Remove suite_name from test parameters before processing
    test.pop("suite_name", None)
    for param, value in test.items():
        if param == "DUMP_VCD" and value == "1":
            test_name += "_VCD"
        if isinstance(value, str):
            value = value.replace(".", "_")
        test_name += f"_{value}"
    test_name += f"_{device}_{target}_{suite_name}"
    return test_name


def remove_duplicate_test_cases(test_cases):
    unique_cases = {}
    seen_params = {}  # Maps parameter signature to (test_name, suite_name)
    
    for test_name, test_case in test_cases.items():
        suite_name = test_case.get("suite_name", "unknown")
        # Create signature without suite_name
        test_case_no_suite = {k: v for k, v in test_case.items() if k != "suite_name"}
        sig = frozenset(test_case_no_suite.items())
        
        if sig not in seen_params:
            seen_params[sig] = (test_name, suite_name)
            unique_cases[test_name] = test_case_no_suite
        else:
            existing_test_name, existing_suite = seen_params[sig]
            if existing_suite != suite_name:
                # Different suites with same parameters - keep both
                unique_cases[test_name] = test_case_no_suite
            else:
                # Same suite with same parameters - this is a true duplicate
                print(
                    f"Info: {test_name} is a duplicate and will be removed from multi_params"
                )
    return unique_cases


def convert_to_json(
    test_suite_file,
    default_params_file,
    param_list,
    updater_dict,
    validator_dict,
    no_verify,
    module,
    enable_updater_padding,
):
    defaults = load_defaults(default_params_file)
    test_cases = {}
    count_legal = count_illegal = 0
    for suite in test_suite_file:
        suite_name = os.path.splitext(os.path.basename(suite))[0]
        try:
            with open(suite) as file:
                for line_number, line in enumerate(file, 1):
                    line = line.partition("#")[0].strip()
                    if line:
                        test_case_list = parse_test_case(
                            line,
                            defaults,
                            param_list,
                            module,
                            updater_dict,
                            validator_dict,
                            enable_updater_padding,
                        )
                        for test_case in test_case_list:
                            test_valid = "True"
                            if not no_verify:
                                test_valid = validate_test_case(
                                    test_suite_file,
                                    test_case,
                                    line_number,
                                    param_list,
                                    updater_dict,
                                    validator_dict,
                                    module,
                                )
                            if test_valid == "False":
                                print(f"Warning: ILLEGAL TEST CASE ARGS: {test_case}")
                                count_illegal += 1
                            else:
                                count_legal += 1
                                test_unmapped = param_map(
                                    test_case, defaults, back_translate=1
                                )
                                test_name = create_test_name(
                                    suite_name, line_number, test_unmapped.copy()
                                )
                                # Add suite_name only for duplicate detection
                                test_unmapped_with_suite = test_unmapped.copy()
                                test_unmapped_with_suite["suite_name"] = suite_name
                                test_cases[test_name] = test_unmapped_with_suite
        except FileNotFoundError:
            print(f"Error: Test suite file not found: {suite}")
            sys.exit(1)
    if not no_verify:
        print(
            f"Info: {count_legal + count_illegal} tests verified, {count_legal} passed, {count_illegal} failed"
        )
    return remove_duplicate_test_cases(test_cases), count_illegal


def validate_test_case(
    test_suite_file,
    test_case,
    line_number,
    param_list,
    updater_dict,
    validator_dict,
    module,
):
    for param in param_list:
        param_under_test = ip_parameter()
        param_under_test.validator = validator_dict[param]
        if param not in no_validation_params:
            param_under_test.param_validate(test_case, module, print_err="True")
            if param_under_test.valid == "False":
                print(f"Error: Validation Fails at : {test_suite_file}:{line_number}")
                return "False"
    return "True"


def param_map(test_case, defaults, back_translate):
    test_case_mapped = {}
    for param in test_case:
        param_mapped = config_translate(
            param,
            configTranslatePath,
            back_translate=back_translate,
            canary_test=defaults,
        )
        if back_translate == 0:
            try:
                test_case_mapped[param_mapped] = int(test_case[param])
            except ValueError:
                test_case_mapped[param_mapped] = test_case[param]
        else:
            test_case_mapped[param_mapped] = str(test_case[param])
    return test_case_mapped


def write_multi_params(test_cases, out_file):
    with open(out_file, "w") as json_file:
        json.dump(test_cases, json_file, indent=4)
        print(f"Info: {called_dir}/{out_file} is generated.")


def parse_arguments():
    parser = argparse.ArgumentParser(
        description="Generate test parameters from a test suite."
    )
    parser.add_argument("test_suite", help="Name or path of the test suite file.")
    parser.add_argument(
        "--updater_pad", action="store_true", help="Enable updater padding."
    )
    parser.add_argument(
        "--ignore_fails",
        action="store_true",
        help="Ignore test case validation failures.",
    )
    parser.add_argument(
        "--no_test_verify", action="store_true", help="Disable test case verification."
    )
    parser.add_argument(
        "--static_default",
        action="store_true",
        help="Use static defaults from default_params.txt (disables updater padding).",
    )
    return parser.parse_args()


def main():
    args = parse_arguments()
    test_suite = args.test_suite
    ignore_fails = args.ignore_fails
    no_test_verify = args.no_test_verify

    # Set enable_updater_padding based on --static_default
    if hasattr(args, "static_default") and args.static_default:
        enable_updater_padding = 0
    else:
        enable_updater_padding = 1

    if test_suite == "jenkins":
        out_file = "./multi_params.json"
    else:
        test_suite_base = os.path.splitext(os.path.basename(test_suite))[0]
        out_file = f"./multi_params_{test_suite_base}.json"
    print(
        f"Test Suite: {test_suite}\nOutput File: {out_file}\nEnable Updater Padding: {enable_updater_padding}\nIgnore Fails: {ignore_fails}\nDisable Verification: {no_test_verify}\n"
    )
    IP_name = called_dir.split("/")[-1]
    default_params_file = f"{called_dir}/test_suites/default_params.txt"

    if os.path.isfile(test_suite) and test_suite.endswith(".txt"):
        test_suite_files = [test_suite]
    elif test_suite == "jenkins":
        test_suite_files = [
            f"{called_dir}/test_suites/checkin.txt",
            f"{called_dir}/test_suites/qor.txt",
        ]
    else:
        test_suite_files = [f"{called_dir}/test_suites/{test_suite}.txt"]

    try:
        module = importlib.import_module(IP_name)
    except ImportError as e:
        print(f"Error: Failed to import {IP_name}.")
        print(f"ImportError: {e}")
        sys.exit(1)

    params_json = extract_param_json(IP_name=IP_name, file_loc=meta_data_path)
    param_list = extract_param_list_all(params_json)
    updater_dict = extract_updater_dict(params_json)
    validator_dict = extract_validator_dict(params_json)

    json_tests, fail_count = convert_to_json(
        test_suite_files,
        default_params_file,
        param_list,
        updater_dict,
        validator_dict,
        no_test_verify,
        module,
        enable_updater_padding,
    )

    if fail_count == 0 or ignore_fails:
        jenkins_tests = {}
        if test_suite == "jenkins":
            jenkins_tests["test_0_tool_canary_aie"] = load_defaults(default_params_file)
        jenkins_tests.update(json_tests)
        write_multi_params(jenkins_tests, out_file)
        print(out_file)  # DO NOT REMOVE - used to communicate output file name to common runmake.sh file
    else:
        sys.exit(1)


if __name__ == "__main__":
    main()
