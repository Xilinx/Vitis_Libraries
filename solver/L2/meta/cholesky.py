#
# Copyright (C) 2025, Advanced Micro Devices, Inc.
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
from math import log2, sqrt

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

TP_GRID_DIM_max = 16


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
    valid_types = ["float", "cfloat"]
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
    TP_DIM = args["TP_DIM"] if "TP_DIM" in args else 0
    return fn_update_TP_DIM(AIE_VARIANT, TT_DATA, TP_DIM)


def fn_update_TP_DIM(AIE_VARIANT, TT_DATA, TP_DIM):
    max_read = com.k_max_read_write_bytes[AIE_VARIANT]
    data_bytes = com.fn_size_by_byte(TT_DATA)
    kVecSampleNum = max_read // data_bytes

    io_bytes_max_per_tile = com.k_data_memory_bytes[AIE_VARIANT]
    io_samples_max_per_tile = io_bytes_max_per_tile // data_bytes
    io_samples_max_ping_pong = io_samples_max_per_tile // 2

    max_dim_per_tile = com.FLOOR( sqrt(io_samples_max_per_tile), kVecSampleNum )
    max_dim_ping_pong = com.FLOOR( sqrt(io_samples_max_ping_pong), kVecSampleNum )

    io_dim_max = TP_GRID_DIM_max * max_dim_per_tile
    io_dim_max_ping_pong = TP_GRID_DIM_max * max_dim_ping_pong

    param_dict = {
        "name": "TP_DIM",
        "minimum": kVecSampleNum,
        "maximum": io_dim_max,
        "maximum_pingpong_buf": io_dim_max_ping_pong,
    }

    if TP_DIM < param_dict["minimum"]:
        param_dict.update({"actual": param_dict["minimum"]})
    elif TP_DIM > param_dict["maximum_pingpong_buf"]:
        param_dict.update({"actual": param_dict["maximum_pingpong_buf"]})
    elif TP_DIM % kVecSampleNum != 0:
        param_dict.update({"actual": com.CEIL(TP_DIM, kVecSampleNum)})
    else:
        param_dict.update({"actual": TP_DIM})

    return param_dict


def validate_TP_DIM(args):
    AIE_VARIANT = args["AIE_VARIANT"]
    TT_DATA = args["TT_DATA"]
    TP_DIM = args["TP_DIM"]
    return fn_validate_dim_size(AIE_VARIANT, TT_DATA, TP_DIM)


def fn_validate_dim_size(AIE_VARIANT, TT_DATA, TP_DIM):
    max_read = com.k_max_read_write_bytes[AIE_VARIANT]
    data_bytes = com.fn_size_by_byte(TT_DATA)
    kVecSampleNum = max_read // data_bytes

    if TP_DIM % kVecSampleNum != 0:
        return isError("TP_DIM should be a multiple of vecSampleNum.")
    else:
        param_dict = fn_update_TP_DIM(AIE_VARIANT, TT_DATA, TP_DIM)
        range_TP_DIM = [param_dict["minimum"], param_dict["maximum_pingpong_buf"]]
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
    io_bytes_max_per_tile = com.k_data_memory_bytes[AIE_VARIANT]
    max_read = com.k_max_read_write_bytes[AIE_VARIANT]
    data_bytes = com.fn_size_by_byte(TT_DATA)
    kVecSampleNum = max_read // data_bytes

    for grid_dim in reversed(range(1, TP_GRID_DIM_max+1)):
        if TP_DIM % grid_dim == 0:
            min_dim_per_tile = TP_DIM // grid_dim
            if min_dim_per_tile % kVecSampleNum == 0:
                break

    min_frame_memory_per_tile = min_dim_per_tile * min_dim_per_tile * data_bytes
    max_frames_per_tile = io_bytes_max_per_tile // min_frame_memory_per_tile

    param_dict = {
        "name": "TP_NUM_FRAMES",
        "minimum": 1,
        "maximum": max_frames_per_tile,
        "maximum_pingpong_buf": max_frames_per_tile // 2,
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
######### TP_GRID_DIM Updater and Validator ###########
#######################################################
def update_TP_GRID_DIM(args):
    AIE_VARIANT = args["AIE_VARIANT"]
    TT_DATA = args["TT_DATA"]
    TP_DIM = args["TP_DIM"]
    TP_NUM_FRAMES = args["TP_NUM_FRAMES"]
    return fn_update_grid_dim(AIE_VARIANT, TT_DATA, TP_DIM, TP_NUM_FRAMES)


def fn_update_grid_dim(AIE_VARIANT, TT_DATA, TP_DIM, TP_NUM_FRAMES):
    io_bytes_max = com.k_data_memory_bytes[AIE_VARIANT]
    pingpong_max = io_bytes_max // 2
    max_read = com.k_max_read_write_bytes[AIE_VARIANT]
    data_bytes = com.fn_size_by_byte(TT_DATA)
    kVecSampleNum = max_read // data_bytes

    legal_set_TP_GRID_DIM = set(range(1, TP_GRID_DIM_max+1))
    legal_set_TP_GRID_DIM_pingpong = legal_set_TP_GRID_DIM.copy()

    for grid_dim in range(1, TP_GRID_DIM_max+1):
        kernel_dim = TP_DIM // grid_dim
        kernel_matrix_size = kernel_dim * kernel_dim
        memory_per_kernel = kernel_matrix_size * TP_NUM_FRAMES * data_bytes

        if TP_DIM % grid_dim != 0:
            legal_set_TP_GRID_DIM.discard(grid_dim)
            legal_set_TP_GRID_DIM_pingpong.discard(grid_dim)

        if kernel_dim % kVecSampleNum != 0:
            legal_set_TP_GRID_DIM.discard(grid_dim)
            legal_set_TP_GRID_DIM_pingpong.discard(grid_dim)

        if memory_per_kernel > pingpong_max:
            legal_set_TP_GRID_DIM_pingpong.discard(grid_dim)
        if memory_per_kernel > io_bytes_max:
            legal_set_TP_GRID_DIM.discard(grid_dim)


    param_dict = {
        "name": "TP_GRID_DIM",
        "enum": legal_set_TP_GRID_DIM,
        "enum_pingpong_buf": legal_set_TP_GRID_DIM_pingpong,
    }
    return param_dict


def validate_TP_GRID_DIM(args):
    AIE_VARIANT = args["AIE_VARIANT"]
    TT_DATA = args["TT_DATA"]
    TP_DIM = args["TP_DIM"]
    TP_NUM_FRAMES = args["TP_NUM_FRAMES"]
    TP_GRID_DIM = args["TP_GRID_DIM"]
    return fn_validate_grid_dim(AIE_VARIANT, TT_DATA, TP_DIM, TP_NUM_FRAMES, TP_GRID_DIM)


def fn_validate_grid_dim(AIE_VARIANT, TT_DATA, TP_DIM, TP_NUM_FRAMES, TP_GRID_DIM):
    param_dict = fn_update_grid_dim(AIE_VARIANT, TT_DATA, TP_DIM, TP_NUM_FRAMES)
    legal_set_TP_GRID_DIM = param_dict["enum_pingpong_buf"]
    return validate_legal_set(legal_set_TP_GRID_DIM, "TP_GRID_DIM", TP_GRID_DIM)


# #### port ####
def get_port_info(portname, dir, dataType, dim, numFrames, apiType, vectorLength):
    return [
        {
            "name": f"{portname}[{idx}]",
            "type": f"{apiType}",
            "direction": f"{dir}",
            "data_type": dataType,
            "fn_is_complex": com.fn_is_complex(dataType),
            "window_size": com.fn_input_window_size(dim * dim * numFrames, dataType),
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
    TP_GRID_DIM = args["TP_GRID_DIM"]

    portsIn = get_port_info(
        portname="in",
        dir="in",
        dataType=TT_DATA,
        dim=TP_DIM,
        numFrames=TP_NUM_FRAMES,
        apiType="window",
        vectorLength=TP_GRID_DIM * TP_GRID_DIM,
    )
    portsOut = get_port_info(
        portname="out",
        dir="out",
        dataType=TT_DATA,
        dim=TP_DIM,
        numFrames=TP_NUM_FRAMES,
        apiType="window",
        vectorLength=TP_GRID_DIM * TP_GRID_DIM,
    )
    return portsIn + portsOut


#### graph generator ####
def generate_graph(graphname, args):

    if graphname == "":
        graphname = "default_graphname"

    TT_DATA = args["TT_DATA"]
    TP_DIM = args["TP_DIM"]
    TP_NUM_FRAMES = args["TP_NUM_FRAMES"]
    TP_GRID_DIM = args["TP_GRID_DIM"]

    # Use formatted multi-line string to avoid a lot of \n and \t
    code = f"""
class {graphname} : public adf::graph {{
public:
  std::array<adf::port<input>, {TP_GRID_DIM * TP_GRID_DIM}> in;
  std::array<adf::port<output>, {TP_GRID_DIM * TP_GRID_DIM}> out;

  xf::dsp::aie::cholesky::cholesky_graph<
    {TT_DATA}, //TT_DATA
    {TP_DIM}, //TP_DIM
    {TP_NUM_FRAMES}, //TP_NUM_FRAMES
    {TP_GRID_DIM}, //TP_GRID_DIM
  > cholesky;

  {graphname}() : cholesky() {{
    adf::kernel *cholesky_kernels = cholesky.getKernels();
    for (int i=0; i < {TP_GRID_DIM * TP_GRID_DIM}; i++) {{
      adf::connect<> net_in(in[i], cholesky.in[i]);
      adf::connect<> net_out(cholesky.out[0], out[i]);
    }}
  }}
}};
"""
    out = {}
    out["graph"] = code
    out["port_info"] = info_ports(args)
    out["headerfile"] = "cholesky_graph.hpp"
    out["searchpaths"] = [
        "L2/include/aie",
        "L2/tests/aie/common/inc",
        "L1/include/aie",
        "L1/src/aie",
        "L1/tests/aie/inc",
        "L1/tests/aie/src",
    ]

    return out
