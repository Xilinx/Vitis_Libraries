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
import sys
import json
import re

# used to create a multi_params.json from the test suit passed on from runmake.sh
# argument1 : run_type - the string that will be appended to the name of each test case
# argument2 : multi_params path - path to the directory where multi_params file exists
# rest of the arguments : series of commands in "make all" format that contain the parameters for this test case

run_type=sys.argv[1]
test_dir = sys.argv[2]
default_params_path = test_dir + "/multi_params.json"
jenkins = False
clear_json_file = False

if run_type == "jenkins" or sys.argv[len(sys.argv) - 1] == "-append":
    print("Adding tests to multi_params")
    multi_params_path = default_params_path
    jenkins = True
elif run_type == "clear_params":
    print("Removing all tests from mutli_params except tool_canary and PR")
    multi_params_path = default_params_path
    clear_json_file = True
else:
    multi_params_path = test_dir + "/multi_params_" + run_type + ".json"
    jenkins == False

print(default_params_path)
print(multi_params_path)
# if run_type == "checkin":
#     run_type = "daily"
f = open(default_params_path, )
json_list=json.load(f)
default_params=json_list["test_0_tool_canary_aie"]
new_json_file={}

if jenkins or clear_json_file:
    new_json_file["test_0_tool_canary_aie"] = default_params
    for test in json_list:
        if "PR" in test:
            new_json_file[test] = json_list[test]
            for def_param in default_params:
                if def_param not in new_json_file[test]:
                    new_json_file[test][def_param] = default_params[def_param]



def dds_hex_to_dec(my_dict):
    hex_pattern = re.compile("^0x[0-9a-fA-F]+$")
    for key, value in my_dict.items():
        if "DDS" in key and isinstance(value, str) and hex_pattern.match(value):
            try:
                my_dict[key] = str(int(value, 16))
            except ValueError:
                print(f"Value {value} at key {key} is not a valid hexadecimal number.")
    return my_dict

def set_float_defaults(my_dict, def_dict):
    if any("float" in str(value) for value in my_dict.values()):
        if "DIFF_TOLERANCE" not in my_dict:
            my_dict["DIFF_TOLERANCE"] = "0.1"
        # check if def_dict contains the key "SHIFT" or "P_SHIFT"
        if "SHIFT" in def_dict:
            my_dict["SHIFT"] = "0"
        elif "P_SHIFT" in def_dict:  # P_SHIFT is the template parameter for matrix_mult
            my_dict["P_SHIFT"] = "0"
    return my_dict

def set_dynamic_defaults(my_dict, def_dict, path):
    # set matrix_mult WINDOW_VSIZES to matrix dimensions when not already set in make command
    if "matrix_mult" in path:
        if "P_INPUT_WINDOW_VSIZE_A" not in my_dict:
            dim_a = my_dict.get('P_DIM_A', def_dict.get('P_DIM_A'))
            dim_ab = my_dict.get('P_DIM_AB', def_dict.get('P_DIM_AB'))
            my_dict['P_INPUT_WINDOW_VSIZE_A'] = str(int(dim_a) * int(dim_ab))
        if "P_INPUT_WINDOW_VSIZE_B" not in tmp_dict:
            dim_ab = my_dict.get('P_DIM_AB', def_dict.get('P_DIM_AB'))
            dim_b = my_dict.get('P_DIM_B', def_dict.get('P_DIM_B'))
            my_dict['P_INPUT_WINDOW_VSIZE_B'] = str(int(dim_ab) * int(dim_b))

    # set WINDOW_VSIZE to POINT_SIZE when not already set in make command
    if "POINT_SIZE" in def_dict and "WINDOW_VSIZE" in def_dict and "WINDOW_VSIZE" not in my_dict:
        my_dict["WINDOW_VSIZE"] = my_dict.get("POINT_SIZE", None)
    return my_dict


list_case_names=[]
new_commands=[]
for command in sys.argv:
    # print(command)
    tmp_dict={}
    tcase_name=""
    device="aie1"
    TARGET="hw"
    enable_pwr = 0
    if "make" in command:
        stringlist = re.split("\s", command)
        for string in stringlist:
            # tmp_dict for each make command contains specified parameters and values
            if "=" in string:
                key, value = re.split("=", string)
                if "AIE_VARIANT" in key and value == "2" or "PART" in key and value == "xcve2802-vsvh1760-2MP-e-S":
                    device = "aie2"
                    tmp_dict[key] = value
                elif "TARGET" in key:
                    TARGET = value
                else:
                    tmp_dict[key] = value

        # convert hex numbers in dds_mixers to decimal
        tmp_dict = dds_hex_to_dec(tmp_dict)
        # sets SHIFT and DIFF_TOLERANCE for float tests
        tmp_dict = set_float_defaults(tmp_dict, default_params)
        # set library specific defaults that depend on other parameters
        tmp_dict = set_dynamic_defaults(tmp_dict, default_params, default_params_path)

        for key in default_params:
            if key in tmp_dict:
                if "." not in tmp_dict[key] and "PART" not in key:
                    tcase_name=tcase_name+str(tmp_dict[key])+"_"
                continue
            else:
                tmp_dict[key] = default_params[key]
                if "." not in tmp_dict[key] and "PART" not in key:
                    tcase_name=tcase_name+str(tmp_dict[key])+"_"

        if "DUMP_VCD" in tmp_dict and tmp_dict["DUMP_VCD"] == "1" :
            tcase_name+="VCD_"

        tcase_name+=device+"_"
        tcase_name+=TARGET+"_"
        tcase_name+=run_type
        #print(tcase_name)
        if jenkins == False:
            new_json_file[tcase_name] = tmp_dict
        elif jenkins == True:
            json_list[tcase_name] = tmp_dict
        list_case_names.append(tcase_name)

# Serializing json
if jenkins == False:
    json_object = json.dumps(new_json_file, indent=4)
else:
    json_object = json.dumps(json_list, indent=4)

# Writing to multi_params.json
with open(multi_params_path, "w") as outfile:
    outfile.write(json_object)

print(list_case_names)
