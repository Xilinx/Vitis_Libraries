#
# Copyright (C) 2019-2022, Xilinx, Inc.
# Copyright (C) 2022-2026, Advanced Micro Devices, Inc.
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
import importlib
import sys
import os

# Functions and Classes
def extract_aie_ip_names(json_file_path):
    """
    Extracts IP names from api.json file for AIE domain.
    
    Args:
        json_file_path: Path to the api.json file
        
    Returns:
        list: List of unique IP names (last part of api_name before ::) for AIE domain
    """
    # Read the JSON file
    with open(json_file_path, 'r') as f:
        data = json.load(f)
    
    # Set to store unique IP names
    ip_names = set()
    
    # Iterate through api_list
    if 'api_list' in data:
        for api_entry in data['api_list']:
            # Check if target_domain is AIE
            if api_entry.get('target_domain') == 'AIE':
                # Check if 'spec' and 'instance' exist and instance == 'class'
                spec = api_entry.get('spec', {})
                if spec.get('instance') == 'class':
                    api_name = api_entry.get('api_name', '')
                    # Extract the last part after the last ::
                    if '::' in api_name:
                        ip_name = api_name.split('::')[-1]
                        if ip_name.endswith('_graph'):
                            ip_name = ip_name[:-6]
                        ip_names.add(ip_name)
    
    # Return sorted list of IP names
    return sorted(list(ip_names))


def print_IPs(LIST_IPS, default_lib_meta_dirs):
    
    for key in default_lib_meta_dirs:
        api_json_loc = f"{default_lib_meta_dirs[key]}/api.json"
        
        if os.path.exists(api_json_loc):            
            ip_names = extract_aie_ip_names(api_json_loc)
            if LIST_IPS:
                print(f"\nAvailable {key.upper()} IPs:")
                print("---------------------")
                for dsplibip in ip_names:
                    print(dsplibip)


def print_with_condition(message, condition):
    if condition is False:
        print(message)

def map_power(num):
    superscript_mapping = str.maketrans("0123456789", "⁰¹²³⁴⁵⁶⁷⁸⁹")
    formatted_num = str(num).translate(superscript_mapping)
    return formatted_num


def valid_name(string):
    english_letters = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"
    allowed_chars = english_letters + "1234567890" + "_"
    if string == "":
        print_with_condition("Instance name cannot be empty.", test_config_helper)
        return False
    if string[0] not in english_letters:
        print_with_condition(
            "Instance name cannot start with special characters/numbers.",
            test_config_helper,
        )
        return False
    for char in string:
        if not (char in allowed_chars):
            print_with_condition(
                "Instance name can only contain letters/numbers/underscore symbol.",
                test_config_helper,
            )
            return False
    return True


def fn_helper_IPname_in():
    IP_in_use = input("\nPlease enter the IP name you would like to configure: ")
    return IP_in_use


def fn_helper_metadir_in(IP_in_use, meta_dir, called_root, default_lib_meta_dirs):
    # Search for {IP_in_use}.json in default_lib_meta_dirs
    json_loc = None
    lib_in_use = None
    metadata_dir = None

    if meta_dir is not None:  # override metadata directory if provided
        json_loc = f"{meta_dir}/{IP_in_use}.json"
        metadata_dir = meta_dir
        metadata_root = metadata_dir[:metadata_dir.rfind('/L2')]
        lib_in_use = os.path.basename(metadata_root)  # get the library name
    else:
        # Check in the called directory first
        candidate = f"{called_root}/L2/meta/{IP_in_use}.json"
        if os.path.exists(candidate):
            json_loc = candidate
            lib_in_use = os.path.basename(called_root)  # get the library name
            metadata_dir = called_root + "/L2/meta"
        else:
            for key in default_lib_meta_dirs:    
                candidate = f"{default_lib_meta_dirs[key]}/{IP_in_use}.json"
                if os.path.exists(candidate):
                    json_loc = candidate
                    lib_in_use = os.path.basename(os.path.dirname(os.path.dirname(default_lib_meta_dirs[key])))  # get the library name
                    metadata_dir = default_lib_meta_dirs[key]
                    break

    return json_loc, metadata_dir, lib_in_use


def fn_helper_outdir_in(metadata_directory, test_config_helper):
    if test_config_helper:
        out_dir = metadata_directory
    else:
        out_dir = input(
            f"\nPlease provide the output directory. To accept the default {metadata_directory} return: "
        )
        if out_dir == "":
            out_dir = metadata_directory
    return out_dir


class ip_parameter:
    def __init__(self):
        self.name = ""
        self.valid = "False"
        self.default = "0"
        self.type = "string"
        self.value = "0"
        self.length = 0
        self.actual = []
        self.helper_msg = ""
        self.element_type = ""

        self.updater = ""
        self.validator = ""
        self.idx_inc = 0

    def get_input(self, param_vals):
        self.idx_inc = 0
        if test_config_helper == True:
            self.value = config_test[self.name]
        else:
            while True:
                if self.type == "vector":
                    print_with_condition(
                        f"\n{self.helper_msg}\n(Enter z/Z and press return for previous parameter)",
                        test_config_helper,
                    )
                else:
                    print_with_condition(
                        f"\n{self.helper_msg}\n(Press return to accept default of {self.name} = {self.default})\n(Enter z/Z and press return for previous parameter)",
                        test_config_helper,
                    )

                inMsg = f"Please input for {self.name}: "
                user_input = input(inMsg)
                if user_input == "z" or user_input == "Z":
                    self.idx_inc = 1
                    break
                elif not (
                    self.type == "string"
                    or self.type == "typename"
                    or self.type == "vector"
                ):
                    if (user_input.isnumeric()) or (user_input == ""):
                        self.value = user_input or self.default
                        self.value = int(self.value)
                        break
                    else:
                        print_with_condition(
                            "ERROR: Input should be numerical!", test_config_helper
                        )
                elif self.type == "vector":
                    if param_vals[self.element_type] in [
                        "int16",
                        "int32",
                        "cint16",
                        "cint32",
                    ]:
                        self.value = [0] * self.length
                    else:
                        self.value = [
                            0.0
                        ] * self.length  # Create a list of zeros (floating point) with the specified length
                    break
                else:
                    self.value = user_input or self.default
                    break

    def param_update(self, param_vals):
        func_update = getattr(module, self.updater)
        update_return = func_update(param_vals)
        if not update_return:   # if there are no legal downstream configs...
            self.helper_msg = "No legal downstream configs. You must go back."
            return

        if "enum" in update_return:
            default_val = update_return["enum"][0]
            enum_list = update_return["enum"]
            helper_msg = f"{self.name} should be within the legal set of {enum_list}"
        elif "len" in update_return:
            self.length = update_return["len"]
            helper_msg = f"{self.name}: An all-zeros vector is constructed with a valid length of {self.length} for {self.name}! \nPlease edit the {self.name} array in the graph as required."
        else:
            default_val = update_return["minimum"]
            min_val = update_return["minimum"]
            if "maximum" in update_return:
                max_val = update_return["maximum"]
                if max_val == 2**31:
                    max_val_formatted = map_power(31)
                    helper_msg = f"{self.name} should be within the legal range of [{min_val}, 2{max_val_formatted}]"
                else:
                    if min_val == max_val:
                        helper_msg = f"{self.name} should be {min_val}."
                    else:
                        helper_msg = f"{self.name} should be within the legal range of [{min_val}, {max_val}]"

            else:
                helper_msg = f"{self.name} should be at least {min_val}"

        if self.type != "vector":
            self.default = default_val
        self.helper_msg = helper_msg

        if "actual" in update_return:
            self.actual = update_return["actual"]

    def param_validate(self, param_vals):
        func_validate = getattr(module, self.validator)
        validate_return = func_validate(param_vals)
        self.valid = str(validate_return["is_valid"])
        if self.valid == "False":
            print_with_condition(validate_return["err_message"], test_config_helper)


##CONFIG HELPER ALGORITHM

# Arguments
IP_in_use=None
meta_dir=None
out_dir=None
test_config_helper=False

for i in range(len(sys.argv)):
    if sys.argv[i] == "--ip":
        IP_in_use = sys.argv[i + 1]
    if sys.argv[i] == "--mdir":
        meta_dir = sys.argv[i + 1]
    if sys.argv[i] == "--outdir":
        out_dir = sys.argv[i + 1]
    if sys.argv[i] == "--test_config_helper":
        test_config_helper = True
        config_test_path = sys.argv[i + 1]
        with open(config_test_path) as f:
            config_test_load = json.load(f)
        config_test = config_test_load["parameters"]

if "--h" in sys.argv:
    help_msg = (
        "\nConfig Helper Options:"
        + "\n--h [prints the helper message]"
        + "\n--ip ip_name [providing the config helper the IP to configure]"
        + "\n--mdir metadata_directory [by default config helper will guide you to xf_dsp\\L2\\meta]"
        + "\n--outdir output_directory [by default config helper will guide you to xf_dsp\\L2\\meta]"
        + "\n--test_config_helper [tests config_helper with the canary test of the IP]"
        + "\nLIST_PARAMS [lists the parameters of the chosen IP to configure]"
        + "\nPRINT_GRAPH [prints the resulting graph at the end of the configuration]"
        + "\nNO_INSTANCE [no graph instance is to be generated at the end of the configuration]"
        + "\nLIST_IPS [prints the IP list in DSPLIB]"
    )
    print(help_msg)
    sys.exit()

if "LIST_PARAMS" in sys.argv:
    LIST_PARAMS = True
else:
    LIST_PARAMS = False

if "PRINT_GRAPH" in sys.argv:
    PRINT_GRAPH = True
else:
    PRINT_GRAPH = False

if "NO_INSTANCE" in sys.argv:
    NO_INSTANCE = True
else:
    NO_INSTANCE = False

if "LIST_IPS" in sys.argv:
    LIST_IPS = True
else:
    LIST_IPS = False


scripts_directory = os.path.dirname(os.path.abspath(__file__))
dsp_root = scripts_directory[:scripts_directory.rfind('/L2')]

# script can be called from library root, L1 or L2 directories and sub directories.
called_path = os.getcwd()

if '/L1' in called_path:
    called_root = called_path[:called_path.rfind('/L1')]
    default_root = called_root
if '/L2' in called_path:
    called_root = called_path[:called_path.rfind('/L2')]
    default_root = called_root
elif os.path.isdir(os.path.join(called_path, "L2")):
    # If "L2" is a subdirectory of the current working directory
    called_root = called_path
    default_root = called_root
else:
    called_root = dsp_root #if called root cannot be inserted, not to cause a bug assign dsp_root for a first scan of the IP
    default_root = dsp_root  # Fallback to dsp_root if /L2 not in path and no L2 subdir

libraries = ["dsp", "dsp_ip", "solver"]
default_lib_meta_dirs = {}
parent_dir = os.path.dirname(dsp_root)

for lib in libraries:
    lib_meta_dir = None
    # Search for a directory containing {lib} in its name in parent_dir
    for entry in os.listdir(parent_dir):
        entry_path = os.path.join(parent_dir, entry)
        if os.path.isdir(entry_path) and lib in entry:
            lib_meta_dir = os.path.join(entry_path, "L2", "meta")
            break
    if lib_meta_dir is None:
        lib_meta_dir = f"{dsp_root}/L2/meta"
    default_lib_meta_dirs[lib] = lib_meta_dir

# import the IP
print_with_condition("\nHello this is CONFIG HELPER!", test_config_helper)
print_IPs(LIST_IPS, default_lib_meta_dirs)

if IP_in_use is None: # try to find IP in libraries
    IP_in_use = fn_helper_IPname_in()

json_loc, metadata_dir, lib_in_use = fn_helper_metadir_in(IP_in_use, meta_dir, called_root, default_lib_meta_dirs)
while json_loc is None or not(os.path.exists(json_loc)):
    help_request = input(
        f"\nERROR: {IP_in_use} cannot be found in {', '.join(libraries)} libraries."
        + "\nWould you like to retry? (y/n) "
    )
    if help_request == "y" or help_request == "":
        print_IPs(True, default_lib_meta_dirs)
        IP_in_use = fn_helper_IPname_in()
        json_loc, metadata_dir, lib_in_use = fn_helper_metadir_in(IP_in_use, meta_dir, called_root, default_lib_meta_dirs)
    if help_request == "n":
        print_with_condition("\nExiting config_helper...", test_config_helper)
        sys.exit()

sys.path.insert(0, default_lib_meta_dirs["dsp"]) #always insert xf_dsp path for common library imports
if metadata_dir and metadata_dir not in sys.path:
    sys.path.insert(1, metadata_dir) #insert the metadata directory of the chosen IP for its specific imports

print_with_condition(
    f"\n{IP_in_use} IP found in the {lib_in_use}!",
    test_config_helper,
)

if out_dir is None:
    out_dir = fn_helper_outdir_in(metadata_dir, test_config_helper)

print_with_condition(
    f"\nNow, let me help you validate {IP_in_use} parameters.",
    test_config_helper,
)
module_name = IP_in_use
is_vss_object = False

if "vss_" in module_name:
    is_vss_object = True
    
module = importlib.import_module(module_name)
if is_vss_object:
    out_inst = "cfg"
    generate_out = getattr(module, "generate_" + out_inst)
    ext_name = out_inst
else:
    out_inst = "graph"
    generate_out = getattr(module, "generate_" + out_inst)
    ext_name = "txt"

with open(json_loc) as f:
    json_load = json.load(f)
    params_json = json_load["parameters"]

    param_list = []
    validator_dict = {}
    updater_dict = {}
    type_dict = {}
    param_vals = {}
    vector_dict = {}
    for pj in params_json:
        if "updater" in pj:
            param_list.append(pj["name"])
            updater_dict.update({pj["name"]: pj["updater"]["function"]})
            validator_dict.update({pj["name"]: pj["validator"]["function"]})
            type_dict.update({pj["name"]: pj["type"]})
        else:
            pj_name = pj["name"]
            raise Exception(f"Error! There is no updater for {pj_name}.")

        if "element_type" in pj:
            vector_dict.update({pj["name"]: pj["element_type"]})
        param_vals.update(
            {pj["name"]: ""}
        )  # construct the parameter library where all the necessary values will be written

if LIST_PARAMS:
    print_with_condition("", test_config_helper)
    print_with_condition(
        f"\nHere are the parameters that will be configured for {IP_in_use} IP:",
        test_config_helper,
    )
    for n in param_list:
        print_with_condition(n, test_config_helper)

# Run the updaters to capture defaults
ip_parameter_objects_dict = {}
idx = 0
while idx < (len(param_list)):
    # Extract Params
    new_helper_param = "param_" + param_list[idx]
    new_helper_param = ip_parameter()  # generate parameter object
    ip_parameter_objects_dict.update(
        {param_list[idx]: new_helper_param}
    )  # dict of the parameter objects
    ip_parameter_objects_dict[param_list[idx]].name = param_list[
        idx
    ]  # assign names of the parameters
    ip_parameter_objects_dict[param_list[idx]].type = type_dict[
        param_list[idx]
    ]  # assign types
    ip_parameter_objects_dict[param_list[idx]].updater = updater_dict[
        param_list[idx]
    ]  # assign updaters
    ip_parameter_objects_dict[param_list[idx]].validator = validator_dict[
        param_list[idx]
    ]  # assign validators
    if ip_parameter_objects_dict[param_list[idx]].name in vector_dict:
        ip_parameter_objects_dict[param_list[idx]].element_type = vector_dict[
            ip_parameter_objects_dict[param_list[idx]].name
        ]
    ip_parameter_objects_dict[param_list[idx]].param_update(
        param_vals
    )  # update the parameters with the previous set of values
    while ip_parameter_objects_dict[param_list[idx]].valid == "False":
        ip_parameter_objects_dict[param_list[idx]].get_input(
            param_vals
        )  # get input from the user
        if ip_parameter_objects_dict[param_list[idx]].idx_inc == 1:
            break
        param_vals[param_list[idx]] = ip_parameter_objects_dict[param_list[idx]].value
        ip_parameter_objects_dict[param_list[idx]].param_validate(
            param_vals
        )  # validate the input from the user
        if ip_parameter_objects_dict[param_list[idx]].valid == "False":
            # Run the updaters to capture defaults
            ip_parameter_objects_dict[param_list[idx]].param_update(param_vals)
            if ip_parameter_objects_dict[param_list[idx]].actual != []:
                print_with_condition(
                    f"\nWARNING: The requested value for {ip_parameter_objects_dict[param_list[idx]].name} cannot be achieved."
                    + f"\nThe closest legal value {ip_parameter_objects_dict[param_list[idx]].actual} is suggested by config helper.",
                    test_config_helper,
                )

    if ip_parameter_objects_dict[param_list[idx]].idx_inc == 1:
        if idx != 0:
            idx -= 1
    else:
        idx += 1

print_with_condition(
    f"\nAll parameters of {IP_in_use} IP is validated!", test_config_helper
)
if not (NO_INSTANCE) and not (test_config_helper):
    # get the instance name
    print_with_condition(
        f"\nNow, let me output a {IP_in_use} instance.", test_config_helper
    )
    valid_instance_name = False
    while not valid_instance_name:
        instanceName = input("\nPlease enter a name for your instance: ")
        valid_instance_name = valid_name(instanceName)

    print_with_condition(instanceName + " is accepted\n", test_config_helper)
    instance_out = generate_out(instanceName, param_vals)
    file = open(f"{out_dir}/{out_inst}_{IP_in_use}_{instanceName}.{ext_name}", "w")
    file.write(instance_out[out_inst])
    file.close()
    print_with_condition(
        f"\nPlease find the configured instance in {out_dir}/{out_inst}_{IP_in_use}_{instanceName}.{ext_name}",
        test_config_helper,
    )

    if is_vss_object:
        print_with_condition(
            "\nUse the cfg file to as an input to your VSS generator makefile."
            f"\nFor example, make -f vss_fft_ifft_1d.mk PARAMS_CFG={out_inst}_{IP_in_use}_{instanceName}.{ext_name}"
            "\n"
            "\nPlease note that the makefile might also require other build parameters. The help target in the vss generator will contain details about any other parameters. \n",
            test_config_helper,
        )

        
    if PRINT_GRAPH:
        print_with_condition("\nHere is your configured instance:", test_config_helper)
        print_with_condition(instance_out[out_inst])

