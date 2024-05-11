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
# this script updates the multi_params file with new test case names that indicate device name and suite name
run_type=sys.argv[1]
multi_params_path = sys.argv[2] + "/multi_params.json"
f = open(multi_params_path, ) 
if run_type == "checkin":
    run_type = "daily"
json_list=json.load(f)
default_params=json_list["test_0_tool_canary_aie"]
new_json_file={}
default_case_params = multi_params_path + "/multi_params.json"
if "run_type" != "daily":
    for tcase in json_list:
        device = "aie1"
        tcase_name_old = tcase
        tcase_name_new = ""
        if tcase_name_old == "test_0_tool_canary_aie" or "PR" in tcase_name_old or "qor" in tcase_name_old:
            tcase_name_new = tcase_name_old
        else:
            for key in json_list[tcase]:
                if key == "AIE_VARIANT":
                    if json_list[tcase][key] == "1":
                        device = "aie1"
                    elif json_list[tcase][key] == "2":
                        device = "aie2"
                else:
                    tcase_name_new += json_list[tcase][key] + "_"
            tcase_name_new = tcase_name_new + device + "_PR"
        new_json_file[tcase_name_new] = json_list[tcase_name_old]
for command in sys.argv:
    tmp_dict={}
    tcase_name=""
    device = "aie1"
    if "make" in command:
        stringlist = re.split("\s", command)
        for string in stringlist:
            if "=" in string:
                key, value = re.split("=", string)
                if key == "AIE_VARIANT":
                    if value == "1":
                        device = "aie1"
                    elif value == "2":
                        device = "aie2"
                tmp_dict[key] = value
        for key in default_params:
            if key in tmp_dict:
                tcase_name=tcase_name+tmp_dict[key]+"_"
                continue
            else:
                tmp_dict[key] = default_params[key]
                tcase_name=tcase_name+tmp_dict[key]+"_"
        tcase_name = tcase_name + device + "_" + run_type        
        new_json_file[tcase_name] = tmp_dict

# Serializing json
json_object = json.dumps(new_json_file, indent=4)
 
# Writing to sample.json
with open(multi_params_path, "w") as outfile:
    outfile.write(json_object)