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
import aie_common as com
from aie_common import (
    validate_legal_set,
    validate_range,
    fn_is_power_of_two,
    round_power_of_2,
    isError,
    isValid,
)
from math import log2

#### naming ####
#
# Name functions with prefix
#   validate_ for validators, returning boolean result and error message as a tuple.
#   update_ for updators, returning object with default value and refined candidate constraints.
#   info_ for creating information based on parameters
#   fn_ for internal functions
#
# Name function arguments as template parameters, when possible
# so the code matches easier with API definition.


# Example of validator.
#
# The parameter itself will be passed as first argument for validator functions.
# These functions can have extra parameters as arguments, as specified as last part of in `validator`.
# These extra parameters must appear before current one in "parameters" section.
#
# A validator function returns a dictionary, with required boolean key "is_valid",
# and "err_message" if "is_valid" is False.
#

TP_SSR_max = 16  # sensible limit?


#######################################################
########### AIE_VARIANT Updater and Validator #########
#######################################################
def update_AIE_VARIANT(args):
    return fn_update_AIE_VARIANT()


def fn_update_AIE_VARIANT():
    legal_set_AIE_VARIANT = [com.AIE, com.AIE_ML, com.AIE_MLv2]

    param_dict = {}
    param_dict.update({"name": "AIE_VARIANT"})
    param_dict.update({"enum": legal_set_AIE_VARIANT})
    return param_dict


def validate_AIE_VARIANT(args):
    AIE_VARIANT = args["AIE_VARIANT"]
    return fn_validate_AIE_VARIANT(AIE_VARIANT)


def fn_validate_AIE_VARIANT(AIE_VARIANT):
    param_dict = fn_update_AIE_VARIANT()
    legal_set_AIE_VARIANT = param_dict["enum"]
    return validate_legal_set(legal_set_AIE_VARIANT, "AIE_VARIANT", AIE_VARIANT)


#######################################################
########### TT_DATA Updater and Validator #############
#######################################################
def update_TT_DATA(args):
    AIE_VARIANT = args["AIE_VARIANT"]
    return fn_update_data_type(AIE_VARIANT)


def fn_update_data_type(AIE_VARIANT):
    valid_types = ["uint16", "int16", "int32", "float"]
    if AIE_VARIANT == com.AIE:
        valid_types.remove("uint16")
    param_dict = {"name": "TT_DATA", "enum": valid_types}
    return param_dict


def validate_TT_DATA(args):
    TT_DATA = args["TT_DATA"]
    AIE_VARIANT = args["AIE_VARIANT"]
    return fn_validate_data_type(TT_DATA, AIE_VARIANT)


def fn_validate_data_type(TT_DATA, AIE_VARIANT):
    param_dict = fn_update_data_type(AIE_VARIANT)
    return validate_legal_set(param_dict["enum"], "TT_DATA", TT_DATA)


#######################################################
############ TP_DIM Updater and Validator #############
#######################################################
def update_TP_DIM(args):
    AIE_VARIANT = args["AIE_VARIANT"]
    TT_DATA = args["TT_DATA"]
    if "TP_DIM" in args and args["TP_DIM"]:
        TP_DIM = args["TP_DIM"]
    else:
        TP_DIM = 0

    return fn_update_TP_DIM(AIE_VARIANT, TT_DATA, TP_DIM)


def fn_update_TP_DIM(AIE_VARIANT, TT_DATA, TP_DIM):
    # max_read = com.k_max_read_write_bytes[AIE_VARIANT]
    # TODO - remove fixed 256b min size requirement
    max_read = 256 // 8
    data_bytes = com.fn_size_by_byte(TT_DATA)
    TP_DIM_min = int(2 * max_read / data_bytes)

    io_bytes_max = com.k_data_memory_bytes[AIE_VARIANT]
    io_samples_max = int(TP_SSR_max * io_bytes_max / data_bytes)

    param_dict = {
        "name": "TP_DIM",
        "minimum": TP_DIM_min,
        "maximum": io_samples_max,
        "maximum_pingpong_buf": int(io_samples_max / 2),
    }

    if (TP_DIM != 0) and (not fn_is_power_of_two(TP_DIM)):
        TP_DIM_act = round_power_of_2(TP_DIM)
        if TP_DIM_act > param_dict["maximum"]:
            TP_DIM_act = param_dict["maximum"]
        if TP_DIM_act < param_dict["minimum"]:
            TP_DIM_act = param_dict["minimum"]

        param_dict.update({"actual": TP_DIM_act})

    return param_dict


def validate_TP_DIM(args):
    AIE_VARIANT = args["AIE_VARIANT"]
    TT_DATA = args["TT_DATA"]
    TP_DIM = args["TP_DIM"]
    return fn_validate_dim_size(AIE_VARIANT, TT_DATA, TP_DIM)


def fn_validate_dim_size(AIE_VARIANT, TT_DATA, TP_DIM):
    if not fn_is_power_of_two(TP_DIM):
        return isError("TP_DIM should be a power of 2!")
    else:
        param_dict = fn_update_TP_DIM(AIE_VARIANT, TT_DATA, TP_DIM)
        range_TP_DIM = [param_dict["minimum"], param_dict["maximum"]]
        return validate_range(range_TP_DIM, "TP_DIM", TP_DIM)


#######################################################
####### TP_NUM_FRAMES Updater and Validator ###########
#######################################################
def update_TP_NUM_FRAMES(args):
    AIE_VARIANT = args["AIE_VARIANT"]
    TT_DATA = args["TT_DATA"]
    TP_DIM = args["TP_DIM"]
    return fn_update_TP_NUM_FRAMES(AIE_VARIANT, TT_DATA, TP_DIM)


def fn_update_TP_NUM_FRAMES(AIE_VARIANT, TT_DATA, TP_DIM):
    data_bytes = com.fn_size_by_byte(TT_DATA)
    io_bytes_max = com.k_data_memory_bytes[AIE_VARIANT]
    io_samples_max = io_bytes_max / data_bytes
    # Currently TP_SSR>1 is not supported by TP_NUM_FRAMES>1
    max_ssr = 1
    TP_NUM_FRAMES_max = int(max_ssr * io_samples_max / TP_DIM)
    param_dict = {
        "name": "TP_NUM_FRAMES",
        "minimum": 1,
        "maximum": TP_NUM_FRAMES_max,
        "maximum_pingpong_buf": int(TP_NUM_FRAMES_max / 2),
    }
    return param_dict


def validate_TP_NUM_FRAMES(args):
    AIE_VARIANT = args["AIE_VARIANT"]
    TT_DATA = args["TT_DATA"]
    TP_DIM = args["TP_DIM"]
    TP_NUM_FRAMES = args["TP_NUM_FRAMES"]
    return fn_validate_TP_NUM_FRAMES(AIE_VARIANT, TT_DATA, TP_DIM, TP_NUM_FRAMES)


def fn_validate_TP_NUM_FRAMES(AIE_VARIANT, TT_DATA, TP_DIM, TP_NUM_FRAMES):
    param_dict = fn_update_TP_NUM_FRAMES(AIE_VARIANT, TT_DATA, TP_DIM)
    range_TP_NUM_FRAMES = [param_dict["minimum"], param_dict["maximum"]]
    return validate_range(range_TP_NUM_FRAMES, "TP_NUM_FRAMES", TP_NUM_FRAMES)


#######################################################
######### TP_SSR Updater and Validator ###########
#######################################################
def update_TP_SSR(args):
    AIE_VARIANT = args["AIE_VARIANT"]
    TT_DATA = args["TT_DATA"]
    TP_DIM = args["TP_DIM"]
    TP_NUM_FRAMES = args["TP_NUM_FRAMES"]
    return fn_update_ssr(AIE_VARIANT, TT_DATA, TP_DIM, TP_NUM_FRAMES)


def fn_update_ssr(AIE_VARIANT, TT_DATA, TP_DIM, TP_NUM_FRAMES):
    # Minimum so that list fits into SSR kernel memories
    # max_read = com.k_max_read_write_bytes[AIE_VARIANT]
    # TODO - remove fixed 256b min size requirement
    max_read = 256 // 8
    data_bytes = com.fn_size_by_byte(TT_DATA)
    TP_DIM_per_kernel_min = int(2 * max_read / data_bytes)

    legal_set_TP_SSR = list(range(1, TP_SSR_max + 1))
    legal_set_TP_SSR_pingpong = list(range(1, TP_SSR_max + 1))
    io_bytes_max = com.k_data_memory_bytes[AIE_VARIANT]
    memory_required = data_bytes * TP_DIM * TP_NUM_FRAMES
    for ssr in legal_set_TP_SSR.copy():
        if not fn_is_power_of_two(ssr):
            legal_set_TP_SSR.remove(ssr)
            legal_set_TP_SSR_pingpong.remove(ssr)
        elif ((TP_DIM * TP_NUM_FRAMES) / ssr) < TP_DIM_per_kernel_min:
            legal_set_TP_SSR.remove(ssr)
            legal_set_TP_SSR_pingpong.remove(ssr)
        elif memory_required > (ssr * io_bytes_max):
            legal_set_TP_SSR.remove(ssr)

        if ssr in legal_set_TP_SSR_pingpong and memory_required > (
            ssr * (io_bytes_max / 2)
        ):
            legal_set_TP_SSR_pingpong.remove(ssr)

    if TP_NUM_FRAMES > 1:  # Currently TP_SSR>1 is not supported by TP_NUM_FRAMES>1
        legal_set_TP_SSR = [1]
        legal_set_TP_SSR_pingpong = [1]

    param_dict = {
        "name": "TP_SSR",
        "enum": legal_set_TP_SSR,
        "enum_pingpong_buf": legal_set_TP_SSR_pingpong,
    }
    return param_dict


def validate_TP_SSR(args):
    TP_SSR = args["TP_SSR"]
    AIE_VARIANT = args["AIE_VARIANT"]
    TT_DATA = args["TT_DATA"]
    TP_DIM = args["TP_DIM"]
    TP_NUM_FRAMES = args["TP_NUM_FRAMES"]
    return fn_validate_ssr(TP_SSR, AIE_VARIANT, TT_DATA, TP_DIM, TP_NUM_FRAMES)


def fn_validate_ssr(TP_SSR, AIE_VARIANT, TT_DATA, TP_DIM, TP_NUM_FRAMES):
    param_dict = fn_update_ssr(AIE_VARIANT, TT_DATA, TP_DIM, TP_NUM_FRAMES)
    legal_set_TP_SSR = param_dict["enum"]
    return validate_legal_set(legal_set_TP_SSR, "TP_SSR", TP_SSR)


#######################################################
######### TP_CASC_LEN Updater and Validator ###########
#######################################################
def update_TP_CASC_LEN(args):
    TP_DIM = args["TP_DIM"]
    TP_SSR = args["TP_SSR"]
    return fn_update_casc_len(TP_DIM, TP_SSR)


def fn_update_casc_len(TP_DIM, TP_SSR):
    param_dict = {
        "name": "TP_CASC_LEN",
        "minimum": 1,
        "maximum": int((log2(TP_DIM / TP_SSR) + 1) * log2(TP_DIM / TP_SSR) / 2),
    }
    return param_dict


def validate_TP_CASC_LEN(args):
    TP_CASC_LEN = args["TP_CASC_LEN"]
    TP_DIM = args["TP_DIM"]
    TP_SSR = args["TP_SSR"]
    return fn_validate_casc_len(TP_CASC_LEN, TP_DIM, TP_SSR)


def fn_validate_casc_len(TP_CASC_LEN, TP_DIM, TP_SSR):
    param_dict = fn_update_casc_len(TP_DIM, TP_SSR)
    range_tp_casc_len = [param_dict["minimum"], param_dict["maximum"]]
    return validate_range(range_tp_casc_len, "TP_CASC_LEN", TP_CASC_LEN)


#######################################################
######### TP_ASCENDING Updater and Validator ##########
#######################################################
def update_TP_ASCENDING(args):
    return fn_update_ascending()


def fn_update_ascending():
    param_dict = {"name": "TP_ASCENDING", "enum": [0, 1]}
    return param_dict


def validate_TP_ASCENDING(args):
    TP_ASCENDING = args["TP_ASCENDING"]
    return fn_validate_ascending(TP_ASCENDING)


def fn_validate_ascending(TP_ASCENDING):
    return validate_legal_set([0, 1], "TP_ASCENDING", TP_ASCENDING)


# #### port ####
def get_port_info(portname, dir, dataType, dim, numFrames, apiType, vectorLength):
    return [
        {
            "name": f"{portname}[{idx}]",
            "type": f"{apiType}",
            "direction": f"{dir}",
            "data_type": dataType,
            "fn_is_complex": com.fn_is_complex(dataType),
            "window_size": com.fn_input_window_size(dim * numFrames, dataType),
            "margin_size": 0,
        }
        for idx in range(vectorLength)
    ]


def info_ports(args):
    """Standard function creating a static dictionary of information
    for upper software to correctly connect the IP.
    Some IP has a configurable number of ports according to parameter set,
    so port information has to be implemented as a function"""
    TT_DATA = args["TT_DATA"]
    TP_DIM = args["TP_DIM"]
    TP_NUM_FRAMES = args["TP_NUM_FRAMES"]
    TP_CASC_LEN = args["TP_CASC_LEN"]
    TP_SSR = args["TP_SSR"]

    portsIn = get_port_info(
        portname="in",
        dir="in",
        dataType=TT_DATA,
        dim=TP_DIM,
        numFrames=TP_NUM_FRAMES,
        apiType="window",
        vectorLength=TP_SSR,
    )
    # SSR > 1 requires merge sort kernel. The output to this kernel is a straem
    if TP_SSR > 1:
        portsOut = get_port_info(
            portname="out",
            dir="out",
            dataType=TT_DATA,
            dim=TP_DIM,
            numFrames=TP_NUM_FRAMES,
            apiType="stream",
            vectorLength=1,
        )
    else:
        portsOut = get_port_info(
            portname="out",
            dir="out",
            dataType=TT_DATA,
            dim=TP_DIM,
            numFrames=TP_NUM_FRAMES,
            apiType="window",
            vectorLength=1,
        )
    return portsIn + portsOut


#### graph generator ####
def generate_graph(graphname, args):

    if graphname == "":
        graphname = "default_graphname"

    TT_DATA = args["TT_DATA"]
    TP_DIM = args["TP_DIM"]
    TP_NUM_FRAMES = args["TP_NUM_FRAMES"]
    TP_ASCENDING = args["TP_ASCENDING"]
    TP_CASC_LEN = args["TP_CASC_LEN"]
    TP_SSR = args["TP_SSR"]

    # Use formatted multi-line string to avoid a lot of \n and \t
    code = f"""
class {graphname} : public adf::graph {{
public:
  std::array<adf::port<input>, {TP_SSR}> in;
  std::array<adf::port<output>, 1> out;

  xf::dsp::aie::bitonic_sort::bitonic_sort_graph<
    {TT_DATA}, //TT_DATA
    {TP_DIM}, //TP_DIM
    {TP_NUM_FRAMES}, //TP_NUM_FRAMES
    {TP_ASCENDING}, //TP_ASCENDING
    {TP_CASC_LEN}, //TP_CASC_LEN
    {TP_SSR} //TP_SSR
  > bitonic_sort;

  {graphname}() : bitonic_sort() {{
    adf::kernel *bitonic_sort_kernels = bitonic_sort.getKernels();
    for (int i=0; i < {TP_SSR}; i++) {{
      adf::connect<> net_in(in[i], bitonic_sort.in[i]);
    }}
    adf::connect<> net_out(bitonic_sort.out[0], out[0]);
  }}

}};
"""
    out = {}
    out["graph"] = code
    out["port_info"] = info_ports(args)
    out["headerfile"] = "bitonic_sort_graph.hpp"
    out["searchpaths"] = [
        "L2/include/aie",
        "L2/tests/aie/common/inc",
        "L1/include/aie",
        "L1/src/aie",
        "L1/tests/aie/inc",
        "L1/tests/aie/src",
    ]

    return out
