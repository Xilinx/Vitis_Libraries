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

configTranslatePath = f"{scripts_dir}/config_translation.tcl"

L2_dir = f"{scripts_dir}/../../../.."
meta_scripts_path = f"{L2_dir}/meta/scripts"
sys.path.insert(0, meta_scripts_path)
from metadata_api import *

no_validation_params = ["weights", "coeff", "lookup_values"]

def load_defaults(file_path):
    defaults = {}
    try:
        with open(file_path, "r") as file:
            for line in file:
                line = line.strip()
                if line and not line.startswith("#"):
                    if "#" in line:
                        line = line.split("#")[0]
                    key, value = line.split("=")
                    defaults[key.strip()] = value.strip()
    except FileNotFoundError:
        print(f"Error: Default parameters file not found: {file_path}")
        sys.exit(1)
    return defaults


def generate_combinations(
    keys, current_index, current_combination, all_combinations, params
):
    if current_index == len(keys):
        all_combinations.append(current_combination.copy())
        return

    current_key = keys[current_index]
    for value in params[current_key]:
        current_combination[current_key] = value
        generate_combinations(
            keys, current_index + 1, current_combination, all_combinations, params
        )

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
    # Initialize params with the given key-value pairs from the test case
    params = {}
    parts = line.split()

    # Parse the parameters and their values
    for part in parts[2:]:  # Skip 'make all'
        key, value = part.split("=")
        value = value.strip()

        # Check if the value is a list (enclosed in square brackets)
        if value.startswith("[") and value.endswith("]"):
            # Remove the brackets and split the values by comma
            value_list = value[1:-1].split(",")
            # Strip whitespace from each value
            params[key.strip()] = [v.strip() for v in value_list]
        else:
            # Single value case
            params[key.strip()] = [value.strip()]

    keys = list(params.keys())
    all_combinations = generate_combinations(keys, 0, {}, [], params)

    # Map each combination using param_map
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
                else:  # update the unmapped parameters
                    if upd_param not in no_validation_params:
                        try:
                            params_mapped.update({upd_param: 0})
                            param_under_test.param_update(params_mapped, module)
                            params_mapped.update({upd_param: param_under_test.default})
                        except:
                            print(params_mapped)
                            print(
                                f"ERROR: There are no legal values for {upd_param}. Likely bug in metadata as all previous params have been validated. "
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
    target = "hw"
    # get target
    if "TARGET" in test:
        target = test["TARGET"]
        del test["TARGET"]
    # get device info from either AIE_VARIANT or PART
    if "AIE_VARIANT" in test:
        aieVar = test["AIE_VARIANT"]
        device = "aie" + aieVar

    if "PART" in test and test["PART"] == "xcve2802-vsvh1760-2MP-e-S":
        device = "aie2"
        del test["PART"]

    # insert VCD into test_name if DUMP_VCD = 1
    for param, value in test.items():
        if param == "DUMP_VCD" and value == "1":
            test_name += "_VCD"
        # Replace periods with underscores in the value
        if isinstance(value, str):
            value = value.replace(".", "_")
        test_name += "_" + str(value)

    test_name += f"_{device}_{target}_{suite_name}"
    return test_name


def remove_duplicate_test_cases(test_cases):
    unique_cases = {}
    seen_cases = set()
    for test_name, test_case in test_cases.items():
        case_signature = frozenset(test_case.items())
        if case_signature not in seen_cases:
            seen_cases.add(case_signature)
            unique_cases[test_name] = test_case
        else:
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
    count_legal = 0
    count_illegal = 0

    for suite in test_suite_file:
        suite_name = os.path.splitext(os.path.basename(suite))[0]
        try:
            with open(suite, "r") as file:
                for line_number, line in enumerate(file, start=1):
                    line = line.strip()
                    if line and not line.startswith("#"):
                        if "#" in line:
                            line = line.split("#")[0]
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
                            elif test_valid == "True":
                                count_legal += 1
                                test_unmapped = param_map(
                                    test_case, defaults, back_translate=1
                                )
                                test_name = create_test_name(
                                    suite_name, line_number, test_unmapped
                                )
                                test_cases[test_name] = test_unmapped
        except FileNotFoundError:
            print(f"Error: Test suite file not found: {suite}")
            sys.exit(1)

    if not no_verify:
        print(
            f"Info: {count_legal + count_illegal} tests are verified, {count_legal} tests passed, {count_illegal} tests failed"
        )

    final_cases = remove_duplicate_test_cases(test_cases)
    return final_cases, count_illegal


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
        # if param not in test_case:
        #     test_case.update({param: 0})
        #     param_under_test.updater = updater_dict[param]
        #     param_under_test.param_update(test_case, module)
        #     test_case.update({param: param_under_test.default})
        param_under_test.validator = validator_dict[param]
        if param not in no_validation_params:
            param_under_test.param_validate(test_case, module, print_err="True")
            if param_under_test.valid == "False":
                print(f"Error: Validation Fails at : {test_suite_file}:{line_number}")
                test_valid = "False"
                break
        test_valid = "True"  # all params are validated
    return test_valid


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
                test_case_mapped.update({param_mapped: int(test_case[param])})
            except ValueError:
                test_case_mapped.update({param_mapped: test_case[param]})
        else:
            test_case_mapped.update({param_mapped: str(test_case[param])})
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
    return parser.parse_args()


def main():
    args = parse_arguments()

    test_suite = args.test_suite
    # Determine the output file name based on the test_suite
    if test_suite == "jenkins":
        out_file = "./multi_params.json"
    else:
        # Extract the base name without any directory path or file extension
        test_suite_base = os.path.splitext(os.path.basename(test_suite))[0]
        out_file = f"./multi_params_{test_suite_base}.json"

    enable_updater_padding = 1
    ignore_fails = args.ignore_fails
    no_test_verify = args.no_test_verify

    print(f"Test Suite: {test_suite}")
    print(f"Output File: {out_file}")
    print(f"Enable Updater Padding: {enable_updater_padding}")
    print(f"Ignore Fails: {ignore_fails}")
    print(f"Disable Verification: {no_test_verify}\n")

    # Extract the IP name and test directories from the called directory
    IP_name = called_dir.split("/")[-1]
    default_params_file = f"{called_dir}/test_suites/default_params.txt"

    # Determine if the test_suite is a path to an existing file
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
    except ImportError:
        print(f"Error: Failed to import {IP_name}.")
        sys.exit(1)

    params_json = extract_param_json(IP_name)
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
        print(out_file)  # Print the output file name
    else:
        sys.exit(1)


if __name__ == "__main__":
    main()
