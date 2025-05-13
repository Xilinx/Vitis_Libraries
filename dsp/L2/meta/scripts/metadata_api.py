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
import sys
import os
import re
import importlib

meta_script_directory = os.path.dirname(os.path.abspath(__file__))
L2_dir = meta_script_directory + "/../.."
meta_dir = f"{L2_dir}/meta"

sys.path.insert(0, meta_dir)
from aie_common import *


class ip_parameter:
    # range = 0
    def __init__(self):
        self.name = ""
        self.valid = "False"
        self.default = "0"
        self.type = "string"
        self.enum = ""
        self.range = []
        self.value = "0"
        self.actual = []
        self.helper_msg = ""
        self.err_msg = ""
        self.updater = ""
        self.validator = ""
        self.update_result = {}

    def get_input(self, should_print_message):

        while True:
            print_message("\n+self.helper_msg+", should_print_message)
            inMsg = f"Please input the data type. (Press return to accept default of {self.name} = {self.default}): "
            user_input = get_data_input(inMsg, should_print_message, self.name)

            if self.type != "string":
                if (user_input.isnumeric()) or (user_input == ""):
                    self.value = user_input or self.default
                    self.value = int(
                        self.value
                    )  # wooow python:D str assigned to itself as integer
                    break
                else:
                    print_message(
                        "ERROR: Input should be numerical!", should_print_message
                    )
            else:
                self.value = user_input or self.default
                break

    def param_update(self, param_vals, module):
        func_update = getattr(module, self.updater)
        update_return = func_update(param_vals)
        self.update_result = update_return
        if "actual" in update_return:
            self.actual = update_return["actual"]
        if "enum" in update_return:
            self.enum = update_return["enum"]
            self.default = self.enum[0]
        if "minimum" in update_return:
            self.range = [update_return["minimum"], update_return["maximum"]]
            self.default = update_return["minimum"]
            self.minimum = update_return["minimum"]
            self.maximum = update_return["maximum"]
        if "maximum_pingpong_buf" in update_return:
            self.maximum_pingpong_buf = update_return["maximum_pingpong_buf"]
        if "minimum_pingpong_buf" in update_return:
            self.minimum_pingpong_buf = update_return["minimum_pingpong_buf"]
        if "enum_pingpong_buf" in update_return:
            self.enum_pingpong_buf = update_return["enum_pingpong_buf"]

    def param_validate(self, param_vals, module, print_err):
        func_validate = getattr(module, self.validator)
        validate_return = func_validate(param_vals)
        self.valid = str(validate_return["is_valid"])
        if self.valid == "False":
            self.err_msg = validate_return["err_message"]
            if print_err == "True":
                print(self.err_msg)


def print_message(message, should_print_message):
    if should_print_message:
        print(message)


def get_data_input(inMsg, should_get_console_input, param_name):
    if should_get_console_input:
        data_input = input(inMsg)
    else:
        file_name = "test_data.csv"
        file_dir = "./" + file_name
        csv_read = pd.read_csv(file_dir)
        data_input = str(csv_read[param_name][0])
    return data_input


def test_input_gen(ip_parameter_obj):
    if ip_parameter_obj.minimum:
        print(ip_parameter_obj.minimum)

    if ip_parameter_obj.enum:
        print(ip_parameter_obj.enum)


def extract_param_json(IP_name):
    json_loc = f"{meta_dir}/{IP_name}.json"
    with open(json_loc) as f:
        json_load = json.load(f)
        params_json = json_load["parameters"]
    return params_json


def extract_all(params_json):
    param_list = []
    validator_dict = {}
    updater_dict = {}
    type_dict = {}
    param_vals = {}
    for pj in params_json:
        if "updater" in pj:
            param_list.append(pj["name"])
            updater_dict.update({pj["name"]: pj["updater"]["function"]})
            validator_dict.update({pj["name"]: pj["validator"]["function"]})
            type_dict.update({pj["name"]: pj["type"]})
        param_vals.update(
            {pj["name"]: ""}
        )  # construct the parameter library where all the necessary values will be written
    return [param_list, validator_dict, updater_dict, type_dict, param_vals]


def extract_param_list_all(params_json):
    param_list = []
    for pj in params_json:
        param_list.append(pj["name"])
    return param_list


def extract_update_param_list(params_json):
    param_list = []
    for pj in params_json:
        if "updater" in pj:
            param_list.append(pj["name"])
    return param_list


def extract_updater_dict(params_json):
    updater_dict = {}
    for pj in params_json:
        if "updater" in pj:
            updater_dict.update({pj["name"]: pj["updater"]["function"]})
    return updater_dict


def extract_validator_dict(params_json):
    validator_dict = {}
    for pj in params_json:
        if "validator" in pj:
            validator_dict.update({pj["name"]: pj["validator"]["function"]})
    return validator_dict


def extract_type_dict(params_json):
    type_dict = {}
    for pj in params_json:
        if (pj["updater"]["function"]) != "":
            type_dict.update({pj["name"]: pj["type"]})
    return type_dict


def extract_param_vals_dict(params_json):
    param_vals = {}
    for pj in params_json:
        param_vals.update({pj["name"]: ""})
    return param_vals


def extract_param_args_dict(params_json):
    args_dict = {}
    for pj in params_json:
        pj["name"]
        if "args" in pj["updater"]:
            args_dict.update({pj["name"]: pj["validator"]["args"]})
        else:
            args_dict.update({pj["name"]: []})

    return args_dict


def config_translate(old_config, configTranslatePath, back_translate=0, canary_test={}):
    # Load config translation. Here we need to back translate the graph parameters into make parameters (TT_DATA -> DATA_TYPE)
    with open(configTranslatePath) as file:
        tcl_code = file.read()

    # Extract the parameter names from the right-hand side values
    param_names = re.findall(r'"([^"]+)"\s+"([^"]+)"', tcl_code)

    # Create the dictionary or parameter translations
    param_dict = {}
    for key, value in param_names:
        if key in canary_test:
            if back_translate == 1:
                param_dict.update({value: key})
            else:
                param_dict.update({key: value})

    if old_config in param_dict:
        new_config = param_dict[old_config]
    else:
        new_config = old_config
    return new_config


def extract_param_args_dict(params_json):
    args_dict = {}
    for pj in params_json:
        pj["name"]
        if "args" in pj["updater"]:
            args_dict.update({pj["name"]: pj["validator"]["args"]})
        else:
            args_dict.update({pj["name"]: []})

    return args_dict


def extract_args(config_L2_file_loc):
    """
    extract_args is using an L2-level config.json file to extract info for IP class.
    Args:
      config_L2_file_loc: location of the L2 level config.json
    Returns:
      ip_name: name of the IP in use.
      ip_args: the parameter arguments of the IP
    Raises:
      Exception:if the config_L2_file_loc does not exist
    """
    if not (os.path.exists(config_L2_file_loc)):
        raise FileNotFoundError(f"Configuration file not found in {config_L2_file_loc}")

    with open(config_L2_file_loc) as file:
        json_load_config = json.load(file)

    ip_args = json_load_config["parameters"]

    return ip_args


class IP:
    """
    This IP class is used to perform validate_all, update_all and generate_graph functions.
    Arguments:
        configuration: Can either be a list [name of the ip(str), parameter of the configuration(dict)]
        or a config.json file for vmc use.
    Methods:
        validate_all: Validation of all parameters with the given configuration.
        update_all: Update of all parameters with the given configuration.
        generate_graph: Outputs the graph. To call this function, ensure parameters are validated
    """

    def __init__(self, ip_name, config_args):
        self.name = ip_name
        self.args = config_args
        self.meta_json = extract_param_json(self.name)
        self.params = extract_param_list_all(self.meta_json)
        self.validators = extract_validator_dict(self.meta_json)
        self.updaters = extract_updater_dict(self.meta_json)
        self.params_valid_results_dict = {}
        self.params_valid_errors_dict = {}
        self.module = importlib.import_module(self.name)
        self.params_update_results_dict = {}
        self.graph = {}

        self.param_obj_dict = {}
        for param in self.params:
            self.param_obj_dict.update({param: ip_parameter()})

    def validate_all(self, print_err=1):
        for param in self.params:
            self.param_obj_dict[param].name = param
            self.param_obj_dict[param].validator = self.validators[param]
            self.param_obj_dict[param].param_validate(self.args, self.module, print_err)
            self.params_valid_results_dict.update(
                {param: self.param_obj_dict[param].valid}
            )
            self.params_valid_errors_dict.update(
                {param: self.param_obj_dict[param].err_msg}
            )
            if self.param_obj_dict[param].valid == "False":
                vmc_dict = {
                    "is_valid": False,
                    "err_msg": self.param_obj_dict[param].err_msg,
                    "param_name": param,
                }
                return vmc_dict
        return isValid

    def update_all(self):
        for param in self.params:
            self.param_obj_dict[param].name = param
            self.param_obj_dict[param].updater = self.updaters[param]
            self.param_obj_dict[param].param_update(self.args, self.module)
            self.param_obj_dict[param].param_validate(
                self.args, self.module, print_err=0
            )
            if self.param_obj_dict[param].valid == "False":
                break
            del self.param_obj_dict[param].update_result["name"]
            self.params_update_results_dict.update(
                {param: self.param_obj_dict[param].update_result}
            )

    def generate_graph(
        self, graph_name
    ):  # all parameters should be valid to run generate_graph
        func_generate_graph = getattr(self.module, "generate_graph")
        self.graph = func_generate_graph(graph_name, self.args)
