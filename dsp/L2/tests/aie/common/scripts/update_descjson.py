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
import json
import sys
import os

branch_name = sys.argv[1]
if len(sys.argv) == 3:
    xf_dsp_path = sys.argv[2]
    test_dir = xf_dsp_path + "/L2/tests/aie/"
    libelemlist = os.listdir(path=test_dir)
else:
    xf_dsp_path = "./"
    libelemlist = [""]
    test_dir = "./"

print(branch_name)


for libelem in libelemlist:
    try:
        multi_params_path = test_dir + "/" + libelem + "/multi_params.json"
        f = open(multi_params_path)
        json_list = json.load(f)

        description_json_path = test_dir + "/" + libelem + "/description.json"
        description_json_file_object = open(description_json_path)
        description_json = json.load(description_json_file_object)

        existing_params_set = {}
        aie1_pr_test_list_dev = []
        aie2_pr_test_list_dev = []

        # pr test list for next is same as daily test list for dev
        aie1_pr_test_list_next = []
        aie2_pr_test_list_next = []

        aie1_qor_test_list_next = []
        aie2_qor_test_list_next = []

        aie1_test_list_all = []
        aie2_test_list_all = []
        description_json["testinfo"]["param_set"] = {}
        description_json["testinfo"]["param_set"]["pull_req"] = {}
        description_json["testinfo"]["param_set"]["daily"] = {}
        for tcase in json_list:
            print(tcase)
            if "PR" in str(tcase) and "aie1" in str(tcase):
                aie1_pr_test_list_dev.append(str(tcase))
            if "PR" in str(tcase) and "aie2" in str(tcase):
                aie2_pr_test_list_dev.append(str(tcase))
            if "daily" in str(tcase) and "aie1" in str(tcase):
                aie1_pr_test_list_next.append(str(tcase))
            if "daily" in str(tcase) and "aie2" in str(tcase):
                aie2_pr_test_list_next.append(str(tcase))
            if "qor" in str(tcase) and "aie1" in str(tcase):
                aie1_qor_test_list_next.append(str(tcase))
            if "qor" in str(tcase) and "aie2" in str(tcase):
                aie2_qor_test_list_next.append(str(tcase))
        if branch_name == "dev":
            aie1_test_list_all = aie1_pr_test_list_dev + aie1_pr_test_list_next
            aie2_test_list_all = aie2_pr_test_list_dev + aie2_pr_test_list_next
            pr_test_list_all = aie1_pr_test_list_dev + aie2_pr_test_list_dev
            daily_test_list_all = aie1_pr_test_list_next + aie2_pr_test_list_next
            description_json["platform_properties"]["vck190"][
                "param_set"
            ] = aie1_test_list_all
            try:
                description_json["platform_properties"]["vek280"][
                    "param_set"
                ] = aie2_test_list_all
            except:
                pass

            for item in pr_test_list_all:
                description_json["testinfo"]["param_set"]["pull_req"][item] = [
                    "vitis_aie_x86sim",
                    "vitis_aie_sim",
                ]

            for item in daily_test_list_all:
                description_json["testinfo"]["param_set"]["daily"][item] = [
                    "vitis_aie_x86sim",
                    "vitis_aie_sim",
                ]

        if branch_name == "next":
            aie1_test_list_all = aie1_pr_test_list_next + aie1_qor_test_list_next
            aie2_test_list_all = aie2_pr_test_list_next + aie2_qor_test_list_next
            pr_test_list_all = aie1_pr_test_list_next + aie2_pr_test_list_next
            daily_test_list_all = aie1_qor_test_list_next + aie2_qor_test_list_next

            test_list_all = aie1_test_list_all + aie2_test_list_all
            description_json["platform_properties"]["vck190"][
                "param_set"
            ] = aie1_test_list_all
            try:
                description_json["platform_properties"]["vek280"][
                    "param_set"
                ] = aie2_test_list_all
            except:
                pass

            for item in pr_test_list_all:
                description_json["testinfo"]["param_set"]["pull_req"][item] = [
                    "vitis_aie_x86sim",
                    "vitis_aie_sim",
                ]

            for item in daily_test_list_all:
                description_json["testinfo"]["param_set"]["daily"][item] = [
                    "vitis_aie_x86sim",
                    "vitis_aie_sim",
                ]

        json_object = json.dumps(description_json, indent=4)
        with open(description_json_path, "w") as outfile:
            outfile.write(json_object)
    except:
        pass
