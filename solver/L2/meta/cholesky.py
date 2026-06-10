#
# Copyright (C) 2025-2026, Advanced Micro Devices, Inc.
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
from math import sqrt

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

TP_DIM_max = 1024
TP_GRID_DIM_max = 16
TP_CASC_LEN_max = 32


#######################################################
########### AIE_VARIANT Updater and Validator #########
#######################################################
def update_AIE_VARIANT(args):
    return fn_update_AIE_VARIANT()

def fn_update_AIE_VARIANT():
    legal_set_AIE_VARIANT = [com.AIE, com.AIE_ML, com.AIE_MLv2]
    param_dict = {"name": "AIE_VARIANT", "enum": legal_set_AIE_VARIANT}
    return param_dict

def validate_AIE_VARIANT(args):
    AIE_VARIANT = args["AIE_VARIANT"]
    return fn_validate_AIE_VARIANT(AIE_VARIANT)

def fn_validate_AIE_VARIANT(AIE_VARIANT):
    param_dict = fn_update_AIE_VARIANT()
    legal_set_AIE_VARIANT = param_dict["enum"]
    return com.validate_legal_set(legal_set_AIE_VARIANT, "AIE_VARIANT", AIE_VARIANT)


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
    return com.validate_legal_set(param_dict["enum"], "TT_DATA", TT_DATA)


#######################################################
######### TP_GRID_DIM Updater and Validator ###########
#######################################################
def update_TP_GRID_DIM(args):
    TP_GRID_DIM = args["TP_GRID_DIM"] if "TP_GRID_DIM" in args and args["TP_GRID_DIM"] else 0
    return fn_update_grid_dim(TP_GRID_DIM)

def fn_update_grid_dim(TP_GRID_DIM):
    param_dict = {
        "name": "TP_GRID_DIM",
        "minimum": 1,
        "maximum": TP_GRID_DIM_max
    }
    param_dict["actual"] = com.CLIP(TP_GRID_DIM, param_dict["minimum"], param_dict["maximum"])
    return param_dict

def validate_TP_GRID_DIM(args):
    TP_GRID_DIM = args["TP_GRID_DIM"]
    return fn_validate_grid_dim(TP_GRID_DIM)

def fn_validate_grid_dim(TP_GRID_DIM):
    param_dict = fn_update_grid_dim(TP_GRID_DIM)
    range_TP_DIM = [param_dict["minimum"], param_dict["maximum"]]
    return com.validate_range(range_TP_DIM, "TP_GRID_DIM", TP_GRID_DIM)


#######################################################
############ TP_DIM Updater and Validator #############
#######################################################
def update_TP_DIM(args):
    AIE_VARIANT = args["AIE_VARIANT"]
    TT_DATA = args["TT_DATA"]
    TP_GRID_DIM = args["TP_GRID_DIM"]
    TP_DIM = args["TP_DIM"] if "TP_DIM" in args and args["TP_DIM"] else 0
    return fn_update_TP_DIM(AIE_VARIANT, TT_DATA, TP_GRID_DIM, TP_DIM)

def fn_update_TP_DIM(AIE_VARIANT, TT_DATA, TP_GRID_DIM, TP_DIM):
    kVecSampleNum = com.fnVecSampleNumMax(AIE_VARIANT, TT_DATA)
    io_samples_max_per_tile = com.k_data_memory_bytes[AIE_VARIANT] // com.sizeof(TT_DATA)
    max_dim_per_tile = com.FLOOR( int(sqrt(io_samples_max_per_tile)), kVecSampleNum )
    io_dim_min = TP_GRID_DIM * kVecSampleNum
    io_dim_max = TP_GRID_DIM * max_dim_per_tile

    param_dict = {
        "name": "TP_DIM",
        "minimum": io_dim_min,
        "maximum": min(io_dim_max, TP_DIM_max)  # cascade tables do not support TP_DIM which exceeds 1024.
    }
    TP_DIM = com.CLIP(TP_DIM, param_dict["minimum"], param_dict["maximum"])
    TP_DIM = com.ROUND_TO_NEAREST_MULTIPLE(TP_DIM, io_dim_min)
    param_dict["actual"] = TP_DIM
    return param_dict

def validate_TP_DIM(args):
    AIE_VARIANT = args["AIE_VARIANT"]
    TT_DATA = args["TT_DATA"]
    TP_GRID_DIM = args["TP_GRID_DIM"]
    TP_DIM = args["TP_DIM"]
    return fn_validate_dim_size(AIE_VARIANT, TT_DATA, TP_GRID_DIM, TP_DIM)

def fn_validate_dim_size(AIE_VARIANT, TT_DATA, TP_GRID_DIM, TP_DIM):
    kVecSampleNum = com.fnVecSampleNumMax(AIE_VARIANT, TT_DATA)
    kKernelDim = TP_DIM // TP_GRID_DIM

    if kKernelDim % kVecSampleNum != 0:
        return com.isError("TP_DIM / TP_GRID_DIM should be a multiple of vecSampleNum.")
    if TP_DIM % TP_GRID_DIM != 0:
        return com.isError("TP_DIM must be a multiple of TP_GRID_DIM.")
    else:
        param_dict = fn_update_TP_DIM(AIE_VARIANT, TT_DATA, TP_GRID_DIM, TP_DIM)
        range_TP_DIM = [param_dict["minimum"], param_dict["maximum"]]
        return com.validate_range(range_TP_DIM, "TP_DIM", TP_DIM)


#######################################################
####### TP_NUM_FRAMES Updater and Validator ###########
#######################################################
def update_TP_NUM_FRAMES(args):
    AIE_VARIANT = args["AIE_VARIANT"]
    TT_DATA = args["TT_DATA"]
    TP_GRID_DIM = args["TP_GRID_DIM"]
    TP_DIM = args["TP_DIM"]
    TP_NUM_FRAMES = args["TP_NUM_FRAMES"] if "TP_NUM_FRAMES" in args and args["TP_NUM_FRAMES"] else 0
    return fn_update_TP_NUM_FRAMES(AIE_VARIANT, TT_DATA, TP_GRID_DIM, TP_DIM, TP_NUM_FRAMES)

def fn_update_TP_NUM_FRAMES(AIE_VARIANT, TT_DATA, TP_GRID_DIM, TP_DIM, TP_NUM_FRAMES):
    kKernelDim = TP_DIM // TP_GRID_DIM
    frame_memory_per_kernel = kKernelDim * kKernelDim * com.sizeof(TT_DATA)
    max_frames_per_tile = com.k_data_memory_bytes[AIE_VARIANT] // frame_memory_per_kernel
    
    param_dict = {
        "name": "TP_NUM_FRAMES",
        "minimum": 1,
        "maximum": max_frames_per_tile,
    }
    TP_NUM_FRAMES = com.CLIP(TP_NUM_FRAMES, param_dict["minimum"], param_dict["maximum"])
    param_dict["actual"] = TP_NUM_FRAMES
    return param_dict

def validate_TP_NUM_FRAMES(args):
    AIE_VARIANT = args["AIE_VARIANT"]
    TT_DATA = args["TT_DATA"]
    TP_GRID_DIM = args["TP_GRID_DIM"]
    TP_DIM = args["TP_DIM"]
    TP_NUM_FRAMES = args["TP_NUM_FRAMES"]
    return fn_validate_TP_NUM_FRAMES(AIE_VARIANT, TT_DATA, TP_GRID_DIM, TP_DIM, TP_NUM_FRAMES)

def fn_validate_TP_NUM_FRAMES(AIE_VARIANT, TT_DATA, TP_GRID_DIM, TP_DIM, TP_NUM_FRAMES):
    param_dict = fn_update_TP_NUM_FRAMES(AIE_VARIANT, TT_DATA, TP_GRID_DIM, TP_DIM, TP_NUM_FRAMES)
    range_TP_NUM_FRAMES = [param_dict["minimum"], param_dict["maximum"]]
    return com.validate_range(range_TP_NUM_FRAMES, "TP_NUM_FRAMES", TP_NUM_FRAMES)


#######################################################
######### TP_CASC_LEN Updater and Validator ###########
#######################################################
def update_TP_CASC_LEN(args):
    TP_DIM = args["TP_DIM"]
    TP_CASC_LEN = args["TP_CASC_LEN"] if "TP_CASC_LEN" in args and args["TP_CASC_LEN"] else 0
    return fn_update_casc_len(TP_DIM, TP_CASC_LEN)

def fn_update_casc_len(TP_DIM, TP_CASC_LEN):
    param_dict = {
        "name": "TP_CASC_LEN",
        "minimum": 1,
        "maximum": min(TP_CASC_LEN_max, TP_DIM)
    }
    TP_CASC_LEN = com.CLIP(TP_CASC_LEN, param_dict["minimum"], param_dict["maximum"])
    param_dict["actual"] = TP_CASC_LEN
    return param_dict

def validate_TP_CASC_LEN(args):
    TP_CASC_LEN = args["TP_CASC_LEN"]
    TP_DIM = args["TP_DIM"]
    return fn_validate_casc_len(TP_CASC_LEN, TP_DIM)

def fn_validate_casc_len(TP_CASC_LEN, TP_DIM):
    param_dict = fn_update_casc_len(TP_DIM, TP_CASC_LEN)
    range_CASC_LEN = [param_dict["minimum"], param_dict["maximum"]]
    return com.validate_range(range_CASC_LEN, "TP_CASC_LEN", TP_CASC_LEN)


#######################################################
########## TP_DIAG_INV Updater and Validator ##########
#######################################################
def update_TP_DIAG_INV(args):
    return fn_update_diag_inv()

def fn_update_diag_inv():
    param_dict = {"name": "TP_DIAG_INV", "enum": [0, 1]}
    return param_dict

def validate_TP_DIAG_INV(args):
    TP_DIAG_INV = args["TP_DIAG_INV"]
    return fn_validate_diag_inv(TP_DIAG_INV)

def fn_validate_diag_inv(TP_DIAG_INV):
    param_dict = fn_update_diag_inv()
    legal_set_diag_inv = param_dict["enum"]
    return com.validate_legal_set(legal_set_diag_inv, "TP_DIAG_INV", TP_DIAG_INV)


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
    TP_CASC_LEN = args["TP_CASC_LEN"]

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
    {TP_CASC_LEN}, //TP_CASC_LEN
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
