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
#
import json
import subprocess
import os
import importlib

from metadata_api import *

current_dir = os.path.dirname(os.path.abspath(__file__))
called_dir = os.getcwd()
L2_dir = current_dir + "/../.."
meta_dir = f"{L2_dir}/meta"
scripts_dir = f"{L2_dir}/tests/aie/common/scripts"
sys.path.insert(0, meta_dir)
XILINX_VITIS = os.environ.get("XILINX_VITIS")

configTranslatePath = f"{scripts_dir}/config_translation.tcl"


def load_txt_file(file_path):
    parameters = {}
    try:
        with open(file_path, "r") as file:
            for line in file:
                # Strip whitespace and skip empty lines
                line = line.strip()
                if line:
                    # Split the line into key and value
                    key, value = line.split("=")
                    # Remove any leading/trailing whitespace from key and value
                    key = key.strip()
                    value = value.strip()
                    # Store the value in the dictionary, converting to appropriate type
                    # Here we assume all values are integers, you can modify this as needed
                    parameters[key] = int(value) if value.isdigit() else value
        return parameters
    except Exception as e:
        return None  # Return None if the file cannot be opened


# Load JSON data from a file
def load_json_file(file_path):
    try:
        with open(file_path, "r") as file:
            return json.load(file)
    except Exception as e:
        return None  # Return None if the file cannot be opened


# Validate JSON data against the schema
def validate_json(data, schema, error_messages):
    print("Starting schema check...")
    if not isinstance(data, dict):
        error_messages.update({"schema check error": "JSON data is not an object."})
        return False

    # Check for extra fields if additionalProperties is false
    if not schema.get("additionalProperties", True):
        extra_fields = set(data.keys()) - set(schema.get("properties", {}).keys())
        if extra_fields:
            error_messages.update(
                {
                    "schema check error": f"Extra fields found in JSON data: {extra_fields}"
                }
            )
            return False

    # Check required fields
    for field in schema.get("required", []):
        if field not in data:
            error_messages.update(
                {"schema check error": f"Missing required field: {field}"}
            )
            return False

    # Validate properties
    for field, field_schema in schema.get("properties", {}).items():
        if field in data:
            if not validate_field(data[field], field_schema, error_messages):
                error_messages.update(
                    {"schema check error": f"Field '{field}' is invalid."}
                )
                return False

    print("Schema check is valid.")
    return True


def validate_field(value, field_schema, error_messages):
    # Check type
    expected_type = field_schema.get("type")
    if expected_type == "string":
        if not isinstance(value, str):
            error_messages.update(
                {"schema check error": f"Expected string for value: {value}"}
            )
            return False
    elif expected_type == "integer":
        if not isinstance(value, int):
            error_messages.update(
                {"schema check error": f"Expected integer for value: {value}"}
            )
            return False
    elif expected_type == "array":
        if not isinstance(value, list):
            error_messages.update(
                {"schema check error": f"Expected array for value: {value}"}
            )
            return False
        if "items" in field_schema:
            for item in value:
                if not validate_field(item, field_schema["items"], error_messages):
                    return False
    elif expected_type == "object":
        if not isinstance(value, dict):
            error_messages.update(
                {"schema check error": f"Expected object for value: {value}"}
            )
            return False
        if "properties" in field_schema:
            for prop, prop_schema in field_schema["properties"].items():
                if prop in value:
                    if not validate_field(value[prop], prop_schema, error_messages):
                        return False

    return True


# Check parameters for validators and updaters
def check_parameters(json_data, config_params, module, error_messages):
    print("Starting dependency checks...")
    if "parameters" not in json_data:
        error_messages.update(
            {"dependency check error": "No parameters found in JSON data."}
        )
        return False

    parameters = json_data["parameters"]

    for index, param in enumerate(
        parameters
    ):  # sweep parameters for args, validator and updater checks

        # Check for validator and updater
        if "validator" not in param or "updater" not in param:
            error_messages.update(
                {
                    "dependency check error": f"Parameter '{param['name']}' is missing a validator or updater."
                }
            )
            return False

        # Get the names of parameters that are defined before the current parameter
        previous_parameter_names = {p["name"] for p in parameters[:index]}

        validator_args = param["validator"].get("args", [])
        # Check if the parameter itself is in its own args
        if param["name"] in validator_args:
            error_messages.update(
                {
                    "dependency check error": f"Parameter '{param['name']}' should not include itself in its validator args."
                }
            )
            return False

        # Check args in validator
        if not all(arg in previous_parameter_names for arg in validator_args):
            prior_args = []
            for param_name in validator_args:
                if param_name not in previous_parameter_names:
                    prior_args.append(param_name)
                error_messages.update(
                    {
                        "dependency check error": f"Validator args for parameter '{param['name']}' include invalid names: {prior_args}"
                    }
                )
            return False

        # Check args in updater
        updater_args = param["updater"].get("args", [])
        # Check if the parameter itself is in its own args
        if param["name"] in updater_args:
            error_messages.update(
                {
                    "dependency check error": f"Parameter '{param['name']}' should not include itself in its updater args."
                }
            )
            return False
        # Check args in validator
        if not all(arg in previous_parameter_names for arg in updater_args):
            prior_args = []
            for param_name in updater_args:
                if param_name not in previous_parameter_names:
                    prior_args.append(param_name)
                error_messages.update(
                    {
                        "dependency check error": f"Updater args for parameter '{param['name']}' include invalid names: {prior_args}"
                    }
                )
            return False

        # Check update and validate functions with the given args list
        param_under_test = ip_parameter()
        param_under_test.updater = param["updater"]["function"]
        param_under_test.validator = param["validator"]["function"]

        validate_args_dict = {}
        update_args_dict = {}

        for arg in updater_args:
            update_args_dict.update({arg: config_params[arg]})
        update_args_dict.update(
            {param["name"]: config_params[param["name"]]}
        )  # add the parameter itself in case needed
        param_under_test.param_update(
            update_args_dict, module
        )  ##will throw a python error if there is missing args

        for arg in validator_args:
            validate_args_dict.update({arg: config_params[arg]})
        validate_args_dict.update(
            {param["name"]: config_params[param["name"]]}
        )  # add the parameter itself in case needed
        param_under_test.param_validate(
            validate_args_dict, module, print_err="False"
        )  ##will throw a python error if there is missing args

        # Validate the configuration
        param_under_test.param_validate(
            config_params, module, print_err="False"
        )  ##will throw a python error if there is missing args
        if param_under_test.valid == "False":
            error_messages.update(
                {"configuration_validation_error": f"{param_under_test.err_msg}"}
            )
            return False
    print("Args lists and dependency order are consistent.")
    print("Test configuration is valid.")

    return True


# Count and read parameter names from the detailed parameters JSON file
def get_parameter_names_from_metadata_json(ip_name, json_data):
    if "parameters" not in json_data:
        print(f"No parameters found in {ip_name}.json file.")
        return []

    return [param["name"] for param in json_data["parameters"]]


# Get parameter names from the config JSON file
def get_parameter_names_from_config_json(json_data):
    if "parameters" not in json_data:
        print("No parameters found in config.json file.")
        return []

    return list(json_data["parameters"].keys())


# Compare parameter names with the config.json
def compare_parameter_names(
    ip_name, metadata_json_data, config_json_data, error_messages
):
    print("Starting config.json checks...")
    metadata_params = get_parameter_names_from_metadata_json(
        ip_name, metadata_json_data
    )
    config_params = get_parameter_names_from_config_json(config_json_data)

    # Check for missing parameters in metadata
    missing_in_metadata = set(config_params) - set(metadata_params)
    if missing_in_metadata:
        error_messages.update(
            {
                "dependency check error": f"Parameter Mismatch! {missing_in_metadata} found in config.json but not in {ip_name}.json."
            }
        )

    # Check for unexpected parameters in metadata
    unexpected_in_metadata = set(metadata_params) - set(config_params)
    if unexpected_in_metadata:
        error_messages.update(
            {
                "dependency check error": f"Parameter Mismatch! {unexpected_in_metadata} found in {ip_name}.json but not in config.json."
            }
        )

    # If there are any discrepancies, raise an error
    if missing_in_metadata or unexpected_in_metadata:
        return False

    print("Parameter names and counts match.")
    return True


def get_canary_test(ip_name):
    ip_test_dir = f"{L2_dir}/tests/aie/{ip_name}"
    default_params_path = ip_test_dir + "/test_suites/default_params.txt"
    multi_params_path = ip_test_dir + "/multi_params.json"

    default_params_load = load_txt_file(default_params_path)
    multi_params_load = load_json_file(multi_params_path)

    if default_params_load == None and multi_params_load == None:
        raise FileNotFoundError(
            f"Both {default_params_path} and {multi_params_path} files do not exist!"
        )

    if default_params_load != None:
        return default_params_load

    if multi_params_load != None:
        return multi_params_load["test_0_tool_canary_aie"]


def is_valid_integer(s):
    """Check if the string is a valid integer (including negative numbers)."""
    if not s:  # Check if the string is empty
        return False

    if s[0] == "-":  # Check for negative sign
        return s[1:].isdigit()  # Check if the rest of the string is all digits
    else:
        return s.isdigit()  # Check if the entire string is all digits


def extract_canary_test_args(ip_name, config_params):
    canary_test = get_canary_test(ip_name)
    canary_args = {}
    for (
        param
    ) in (
        canary_test
    ):  # Translate graph level parameters to kernel level for update and validate
        new_config = config_translate(
            param, configTranslatePath, back_translate=0, canary_test=canary_test
        )
        if new_config in config_params:
            if isinstance(canary_test[param], str) and is_valid_integer(
                canary_test[param]
            ):
                canary_args.update({new_config: int(canary_test[param])})
            else:
                canary_args.update({new_config: canary_test[param]})
    return canary_args


def test_config_helper(ip_name):
    print("Starting config_helper checks with canary test...")
    config_test_json_path = f"{called_dir}/config.json"

    # Call config_helper.py as a separate process
    config_helper_test_result = subprocess.run(
        [
            sys.executable,
            os.path.join(L2_dir, "meta/scripts/config_helper.py"),
            "--ip",
            ip_name,
            "--test_config_helper",
            config_test_json_path,
        ]
    )

    if config_helper_test_result.returncode != 0:
        print("config_helper_test failed!")
        return False

    print("config_helper config-test run passed.")
    return True


def check_exit(check_result, error_messages, ip_name):
    if not check_result:
        gen_out_json_error(ip_name, error_messages)
        print(error_messages)
        sys.exit(1)


def gen_out_files(ip_name, config_json_params, search_paths):
    ip_in_use = IP(ip_name, config_json_params)
    ip_in_use.generate_graph(f"{ip_name}_native_generated_graph")

    graph_string_if = f'#ifndef {ip_name}_generated_graph_GRAPH_H_\n#define {ip_name}_generated_graph_GRAPH_H_\n\n#include <adf.h>\n#include "{ip_name}_graph.hpp"\n\n'
    graph_string_endif = f"\n\n#endif // {ip_name}_generated_graph_GRAPH_H_"
    graph_content = graph_string_if + ip_in_use.graph["graph"] + graph_string_endif

    # out_dict={}
    # out_dict.update({"graphName":f"{ip_name}_generated_graph"})
    # out_dict.update({"graphHeaderFile":f"{ip_name}_generated_graph.h"})
    # out_dict.update({"graphSearchPath":search_paths})
    # out_dict.update({"graphPreProcOptions":[]})
    # out_dict.update({"ports":port_info_map(ip_in_use.graph["port_info"])})
    # out_dict.update({"graphSourceFileUpdated":True})
    out_dict = ip_in_use.graph["port_info"]

    folder_path = f"./{ip_name}_native_generated_graph"
    out_json_path = folder_path + "/out_native.json"
    graph_h_path = folder_path + f"/{ip_name}_generated_graph.h"
    create_graph_folder(folder_path)
    with open(graph_h_path, "w") as graph_file:
        graph_file.write(graph_content)  # indent for pretty printing
    with open(out_json_path, "w") as json_file:
        json.dump(out_dict, json_file, indent=4)  # indent for pretty printing


def gen_out_json_error(ip_name, error_messages):
    folder_path = f"./{ip_name}_generated_graph"
    out_json_path = folder_path + "/out_native.json"
    create_graph_folder(folder_path)
    with open(out_json_path, "w") as out_json_error:
        json.dump(
            error_messages, out_json_error, indent=4
        )  # indent for pretty printing


def create_graph_folder(folder_path):
    # Check if the folder exists
    if os.path.exists(folder_path):
        delete_folder_contents(folder_path)
        os.rmdir(folder_path)  # Remove the now-empty folder
        # Create the folder
    os.makedirs(folder_path)


def delete_folder_contents(folder_path):
    # Iterate over all the items in the directory
    for item in os.listdir(folder_path):
        item_path = os.path.join(folder_path, item)
        # Check if the item is a file or a directory
        if os.path.isdir(item_path):
            # If it's a directory, remove it and its contents
            delete_folder_contents(item_path)  # Recursively delete contents
            os.rmdir(item_path)  # Remove the empty directory
        else:
            # If it's a file, remove it
            os.remove(item_path)


def port_info_map(ports_info):
    ports_info_list = []
    for port_info in ports_info:
        port_info_mapped = {
            "name": port_info["name"],
            "portkind": port_info["type"],
            "direction": port_info["direction"],
            "isComplex": port_info["fn_is_complex"],
            "isStdComplex": port_info["fn_is_complex"],
            "basictype": port_info["data_type"],
        }

        if port_info["type"] == "window":
            port_info_mapped.update(
                {
                    "windowsize": port_info["window_size"],
                    "margin": port_info["margin_size"],
                    "isArray": False,
                    "numElements": 1,
                    "synchronicity": "sync",
                }
            )
        elif port_info["type"] == "stream":
            port_info_mapped.update(
                {
                    "numElements": int(
                        port_info["window_size"] / get_size_byte(port_info["data_type"])
                    ),
                    "windowsize": port_info["window_size"],
                    "isArray": True,
                    "synchronicity": "sync",
                }
            )
        elif port_info["type"] == "parameter":
            port_info_mapped.update(
                {
                    "numElements": port_info["num_elements"],
                    "isArray": True,
                    "synchronicity": port_info["synchronicity"],
                }
            )
        ports_info_list.append(port_info_mapped)
    return ports_info_list


def get_size_byte(port_type):
    if port_type == "uint8":
        return 1
    if port_type == "int8":
        return 1
    elif port_type == "int16":
        return 2
    elif port_type == "int32":
        return 4
    elif port_type == "cint16":
        return 4
    elif port_type == "cint32":
        return 8
    elif port_type == "float":
        return 4
    elif port_type == "cfloat":
        return 8
    elif port_type == "bfloat16":
        return 2
    else:
        return 0


def isComplexType(typeStr):
    if (
        typeStr == "cfloat"
        or typeStr == "cint16"
        or typeStr == "cint32"
        or typeStr == "cint64"
        or typeStr == "cacc48"
    ):
        return True
    return False


# Main function
if __name__ == "__main__":
    for i in range(len(sys.argv)):
        if sys.argv[i] == "--ip":
            ip_name = sys.argv[i + 1]

    schema_file_path = (
        f"{XILINX_VITIS}/data/ipmetadata/vitis_library_api_spec_schema.json"
    )
    json_file_path = f"{L2_dir}/meta/{ip_name}.json"
    config_json_file_path = f"{called_dir}/config.json"

    try:
        module = importlib.import_module(ip_name)
    except ImportError:
        print(f"Error: Failed to import {ip_name}.")
        sys.exit(1)

    schema = load_json_file(schema_file_path)
    json_data = load_json_file(json_file_path)
    config_json_data = load_json_file(config_json_file_path)

    error_messages = {}
    check_exit(
        validate_json(json_data, schema, error_messages), error_messages, ip_name
    )
    check_exit(
        compare_parameter_names(ip_name, json_data, config_json_data, error_messages),
        error_messages,
        ip_name,
    )
    check_exit(
        check_parameters(
            json_data, config_json_data["parameters"], module, error_messages
        ),
        error_messages,
        ip_name,
    )
    check_exit(test_config_helper(ip_name), error_messages, ip_name)
    print("All parameter checks passed.")
    # gen_out_files(ip_name, config_json_data["parameters"], json_data["search_paths"])

    # #Temporary out.json checks
    # out_new_json_path=f"./{ip_name}_new_generated_graph/out_new.json"
    # out_old_json_path=f"./{ip_name}_generated_graph/out.json"
    # compare_return=subprocess.run([sys.executable, os.path.join(L2_dir, "meta/scripts/compare_out_json.py"), out_new_json_path, out_old_json_path])
    # if compare_return.returncode != 0:
    #     print("Error: out.json comparison failed!")
    #     sys.exit(1)
