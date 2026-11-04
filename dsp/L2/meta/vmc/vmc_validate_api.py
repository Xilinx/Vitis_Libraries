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
import sys
import os
import re

vmc_script_directory = os.path.dirname(os.path.abspath(__file__))
L2_dir = vmc_script_directory + "/.."
meta_dir = f"{L2_dir}/scripts"
sys.path.insert(0, meta_dir)

from metadata_api import *
# from vmc_dsp import ProcesStrFun
from fir_utils import *

# Define AI Engine variants
AIE1 = 1
AIE_ML = 2
AIE_MLv2 = 22

# Define kernel interface
IO_BUFFER = 0
STREAM = 1

sys.path.insert(0, meta_dir) 

def generate_graph_vmc(configuration, ip_name, graph_name):
    ip_name = ip_name
    config_args = extract_args(configuration)
    if ip_name.startswith("fir_"):
        if "_hb" in ip_name:
            config_args["TP_FIR_LEN"] = fn_get_fir_length_hb(config_args)
        elif "_sym" in ip_name:
            config_args["TP_FIR_LEN"] = config_args["TP_FIR_LEN"]
        elif "_tdm" in ip_name:
            config_args["TP_FIR_LEN"] = config_args["TP_FIR_LEN"]
        else:
            config_args["TP_FIR_LEN"] = fn_get_fir_length(config_args)
    elif ip_name.startswith("matrix_mult"):
        config_args["TP_INPUT_WINDOW_VSIZE_A"] = (config_args["TP_DIM_A"] * config_args["TP_DIM_AB"])
        config_args["TP_INPUT_WINDOW_VSIZE_B"] = (config_args["TP_DIM_B"] * config_args["TP_DIM_AB"])
    elif ip_name.startswith("fft_ifft"):
        if config_args.get("TP_PARALLEL_POWER") == "auto" and config_args.get("TP_API") == 1:
            config_args["TP_PARALLEL_POWER"] = 0
        if config_args.get("TP_PARALLEL_POWER") != "auto":
            config_args["TP_PARALLEL_POWER"] = fn_get_parallel_power(config_args)
    if "TP_NUM_OUTPUTS" in config_args:
        config_args["TP_NUM_OUTPUTS"] = fn_get_num_outputs(config_args)
    if "vmc_ssr_inherit" in config_args and config_args["vmc_ssr_inherit"] == 1:
        config_args["TP_SSR"] = 1
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
            config_args["TP_FIR_LEN"] = config_args["TP_FIR_LEN"]
        elif "_tdm" in ip_name:
            config_args["TP_FIR_LEN"] = config_args["TP_FIR_LEN"]
        else:
            config_args["TP_FIR_LEN"] = fn_get_fir_length(config_args)
    elif ip_name.startswith("fft_ifft"):
        if config_args.get("TP_PARALLEL_POWER") != "auto":
            config_args["TP_PARALLEL_POWER"] = fn_get_parallel_power(config_args)
        if config_args.get("TP_PARALLEL_POWER") == "auto" and config_args.get("TP_API") == 1:
            config_args["TP_PARALLEL_POWER"] = 0
        if config_args["TP_PARALLEL_POWER"] != "auto" and (config_args["TP_PARALLEL_POWER"] <0 or config_args["TP_PARALLEL_POWER"] >4):
            # default error message for IO Buffer
            if config_args["TP_API"] == IO_BUFFER:
                err_msg = "Invalid SSR value specified. The valid SSR values should be a power of 2 (e.g., 1, 2, 4, 8, 16) within the inclusive range of 1 to 16."
            if config_args["TP_API"] == STREAM:
                if config_args["AIE_VARIANT"] == AIE1:
                    err_msg = "Invalid SSR value specified. The valid SSR values should be a power of 2 (e.g., 2, 4, 8, 16, 32) within the inclusive range of 2 to 32."
                else:
                    err_msg = "Invalid SSR value specified. The valid SSR values should be a power of 2 (e.g., 1, 2, 4, 8, 16) within the inclusive range of 1 to 16."
            error_dict = {
                "is_valid": False,
                "err_msg": err_msg,
                "param_name": "ssr",
            }
            return error_dict
    elif ip_name.startswith("matrix_mult"):
        config_args["TP_INPUT_WINDOW_VSIZE_A"] = (config_args["TP_DIM_A"]*config_args["TP_DIM_AB"])
        config_args["TP_INPUT_WINDOW_VSIZE_B"] = (config_args["TP_DIM_B"]*config_args["TP_DIM_AB"])
    elif ip_name.startswith("dds_mixer"):
        if "vmc_ssr_inherit" in config_args and config_args["vmc_ssr_inherit"] == 1:
            config_args["TP_SSR"] = 1
        if (
            config_args["TP_MIXER_MODE"] == 2
            and config_args["TP_API"] == 1
            and config_args["AIE_VARIANT"] != 1
        ):
            error_dict = {
                "is_valid": False,
                "err_msg": "mixer_mode(2) in stream interfaces is not supported for this device",
                "param_name": "AIE_VARIANT",
            }
            return error_dict
    elif ip_name.startswith("hadamard"):
        if (
            config_args.get("TP_API") == STREAM
            and config_args.get("AIE_VARIANT") in (AIE_ML, AIE_MLv2)
        ):
            error_dict = {
                "is_valid": False,
                "err_msg": "Hadamard Stream block is not supported for AIE-ML and AIE-MLv2 devices.",
                "param_name": "AIE_VARIANT",
            }
            return error_dict
    if "TP_NUM_OUTPUTS" in config_args:
        config_args["TP_NUM_OUTPUTS"] = fn_get_num_outputs(config_args)
    IP_obj_vmc = IP(ip_name, config_args)
    return IP_obj_vmc.validate_all()


def _intercept_api_legal_set_message(param_name, err_msg):
    """If the validation layer emitted exactly the message
    'TP_API (0) is not in the legal set of [1]' (with the optional
    'ERROR: ' prefix that `isError` adds), replace it with a clearer hint
    that the buffer block does not support this configuration and the
    stream block may. The reverse direction and all other variants are
    intentionally left untouched.
    NOTE: The caller (`ProcessVMCConfig`) is responsible for restricting
    this interception to the outer tensor IP only.
    """
    if param_name != "TP_API" or not isinstance(err_msg, str):
        return err_msg
    if "TP_API (0) is not in the legal set of [1]" in err_msg:
        return (
            "This configuration is not supported by the buffer block. "
            "It may be supported by the stream block."
        )
    return err_msg


def _intercept_add_tiling_legal_set_message(param_name, err_msg):
    """If the validation layer emitted exactly the message
    'TP_ADD_TILING_A (1) is not in the legal set of [0]' (or _B) -- with
    the optional 'ERROR: ' prefix that `isError` adds -- replace it with
    a concise, user-facing hint asking the user to uncheck the
    corresponding 'Add tiling to input A/B' checkbox on the GEMM block.
    All other variants are intentionally left untouched.
    NOTE: The caller (`ProcessVMCConfig`) is responsible for restricting
    this interception to the GEMM (matrix_mult) IP only.
    """
    if not isinstance(err_msg, str):
        return err_msg
    if (
        param_name == "TP_ADD_TILING_A"
        and "TP_ADD_TILING_A (1) is not in the legal set of [0]" in err_msg
    ):
        return (
            "'Add tiling to input A' is not supported for this "
            "configuration."
        )
    if (
        param_name == "TP_ADD_TILING_B"
        and "TP_ADD_TILING_B (1) is not in the legal set of [0]" in err_msg
    ):
        return (
            "'Add tiling to input B' is not supported for this "
            "configuration."
        )
    return err_msg


def ProcesStrFun(input_string, key_label_map):
    if not isinstance(input_string, str) or not input_string:
        return input_string  # Return as is if not a string or empty

    # Remove "ERROR: " from the input string if it exists
    if "ERROR: " in input_string:
        input_string = input_string.replace("ERROR: ", "")

    # Create a cleaned version of the key_label_map with trailing colons removed from labels
    cleaned_key_label_map = {}
    for old_word, new_word in key_label_map.items():
        # Remove trailing colon from the label (new_word) if it exists
        cleaned_new_word = new_word.rstrip(':') if isinstance(new_word, str) else new_word
        cleaned_key_label_map[old_word] = cleaned_new_word
    # Perform replacements using the cleaned map.
    # Strip trailing punctuation (e.g., '.', ',', ':', ';') from each word before
    # looking it up in the map so that tokens like "TT_DATA." at the end of a
    # sentence are still matched and replaced with their label. The stripped
    # punctuation is re-appended after substitution to preserve the original
    # sentence structure.
    trailing_punct = ".,:;!?"
    words = input_string.split()
    for i, word in enumerate(words):
        stripped = word.rstrip(trailing_punct)
        suffix = word[len(stripped):]
        if stripped in cleaned_key_label_map:
            words[i] = cleaned_key_label_map[stripped] + suffix
        elif word in cleaned_key_label_map:
            words[i] = cleaned_key_label_map[word]
    input_string = ' '.join(words)
    return input_string

def ProcessVMCConfig(config_file_loc, spec, key_label_map=None):
    if key_label_map is None:
        key_label_map = {}
    temp_dict = validate_all_vmc(config_file_loc, spec)
    if temp_dict["is_valid"] == True:
        return temp_dict
    # For outer tensor only, replace the generic
    # "TP_API (...) is not in the legal set ..." message with a
    # buffer-vs-stream hint before label substitution runs.
    if isinstance(spec, str) and spec.startswith("outer_tensor"):
        temp_dict["err_msg"] = _intercept_api_legal_set_message(
            temp_dict.get("param_name", ""), temp_dict["err_msg"]
        )
    # For GEMM (matrix_mult) only, replace the generic
    # "TP_ADD_TILING_A/B (1) is not in the legal set of [0]" message
    # with an actionable "Uncheck Add tiling to input A/B" hint before
    # label substitution runs.
    if isinstance(spec, str) and spec.startswith("matrix_mult"):
        temp_dict["err_msg"] = _intercept_add_tiling_legal_set_message(
            temp_dict.get("param_name", ""), temp_dict["err_msg"]
        )
    temp_dict["err_msg"] = ProcesStrFun(temp_dict["err_msg"], key_label_map)
    return temp_dict

#1-3 for validation config.json
#config_file_loc="<path>/xmc_aie_lib/DDS_6e89f154/config.json"
#temp_dict=ProcessVMCConfig(config_file_loc,"dds_mixer")
#print(temp_dict)

#generate graph
#graph_dict=generate_graph_vmc(config_file_loc, "dds_mixer", "DFT_c6844DDS_6e89f154f4f")
#print(graph_dict["port_info"])
