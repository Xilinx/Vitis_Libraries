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
import json
import sys
import os
import re

vmc_script_directory = os.path.dirname(os.path.abspath(__file__)) 
L2_dir = vmc_script_directory+"/.." 
meta_dir= f"{L2_dir}/scripts"
sys.path.insert(0, meta_dir) 

from metadata_api import *
from vmc_dsp import ProcesStrFun
from fir_utils import *



sys.path.insert(0, meta_dir) 

def generate_graph_vmc(configuration, ip_name, graph_name):
    ip_name = ip_name
    config_args = extract_args(configuration)
    if ip_name.startswith("fir_"):
        if "_hb" in ip_name:
            config_args["TP_FIR_LEN"] = fn_get_fir_length_hb(config_args)
        elif "_sym" in ip_name:
            config_args["TP_FIR_LEN"] = fn_get_fir_length_sym(config_args)
        elif "_tdm" in ip_name:
            config_args["TP_FIR_LEN"] = config_args["TP_FIR_LEN"]
        else:
            config_args["TP_FIR_LEN"] = fn_get_fir_length(config_args)
    elif ip_name.startswith("fft_ifft"):
        config_args["TP_PARALLEL_POWER"] = fn_get_parallel_power(config_args)
    if "TP_NUM_OUTPUTS" in config_args:
        config_args["TP_NUM_OUTPUTS"] = fn_get_num_outputs(config_args)
    IP_obj_vmc = IP(ip_name, config_args)
    IP_obj_vmc.generate_graph(graph_name)
    return IP_obj_vmc.graph


def validate_all_vmc(configuration, ip_name):
    ip_name = ip_name
    config_args = extract_args(configuration)
    if ip_name.startswith("fir_"):
        if "_hb" in ip_name:
            config_args["TP_FIR_LEN"] = fn_get_fir_length_hb(config_args)
        elif "_sym" in ip_name:
            config_args["TP_FIR_LEN"] = fn_get_fir_length_sym(config_args)
        elif "_tdm" in ip_name:
            config_args["TP_FIR_LEN"] = config_args["TP_FIR_LEN"]
        else:
            config_args["TP_FIR_LEN"] = fn_get_fir_length(config_args)
    elif ip_name.startswith("fft_ifft"):
        config_args["TP_PARALLEL_POWER"] = fn_get_parallel_power(config_args)
    if "TP_NUM_OUTPUTS" in config_args:
        config_args["TP_NUM_OUTPUTS"] = fn_get_num_outputs(config_args)
    IP_obj_vmc = IP(ip_name, config_args)
    return IP_obj_vmc.validate_all()


def ProcessVMCConfig(config_file_loc, spec):
    temp_dict = validate_all_vmc(config_file_loc, spec)
    if temp_dict["is_valid"] == True:
        return temp_dict
    temp_dict["err_msg"] = ProcesStrFun(temp_dict["err_msg"], spec)
    #temp_dict["param_name"] = ProcesStrFun(temp_dict["param_name"], spec)
    error = {
        "label": "error",
        "description": temp_dict["err_msg"],
        "param_name": temp_dict["param_name"],
    }
    with open("ou.json", "w") as f:
        json.dump(error, f, indent=4)
    return temp_dict

#1-3 for validation config.json
#config_file_loc="/proj/xhdhdstaff4/mahajan/gradle_HEAD_mahajan/HEAD/src/products/fourier/src/matlab/mcode/shared/guibuilder/xmc_aie_lib/Bitonic_Sort_dc101551/config.json"
#temp_dict=validate_all_vmc(config_file_loc,"bitonic_sort")
#print(temp_dict)

#generate graph
#graph_dict=generate_graph_vmc(config_file_loc, "bitonic_sort", "Bitonic_Sort_dc101551")
#print(graph_dict["searchpaths"])
