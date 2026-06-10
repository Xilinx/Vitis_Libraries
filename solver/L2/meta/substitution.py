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

# Naming convention
#
# Name functions with prefix
#   validate_ for validators, returning boolean result and error message as a tuple.
#   update_ for updators, returning object with default value and refined candidate constraints.
#   info_ for creating information based on parameters
#
# Name function arguments as template parameters, when possible
# so the code matches easier with API definition.

TP_DIM_max = 1024
TP_GRID_DIM_max = 16

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
    MAX_FRAMES = 4 # Beyond this, buffer descriptors usage may overflow. Also the trade of throughput to latency is poor.
    #aie1 has only 2D addressing in buffer descriptors. Complex requires one dimension, as does NUM_FRAMES, so they are mutex.
    if (AIE_VARIANT == 1 and TT_DATA == "cfloat"):
        MAX_FRAMES = 1
    kKernelDim = TP_DIM // TP_GRID_DIM
    frame_memory_per_kernel = kKernelDim * kKernelDim * com.sizeof(TT_DATA)
    max_frames_per_tile = min(MAX_FRAMES, com.k_data_memory_bytes[AIE_VARIANT] // frame_memory_per_kernel)
    
    param_dict = {
        "name": "TP_NUM_FRAMES",
        "minimum": 1,
        "maximum": min(max_frames_per_tile, 4), # 4 max due to issues with buffer transposes.
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


#######################################################
######## TP_SUBST_TYPE Updater and Validator ##########
#######################################################
def update_TP_SUBST_TYPE(args):
    return fn_update_subst_type()

def fn_update_subst_type():
    param_dict = {"name": "TP_SUBST_TYPE", "enum": [0, 1]}
    return param_dict

def validate_TP_SUBST_TYPE(args):
    TP_SUBST_TYPE = args["TP_SUBST_TYPE"]
    return fn_validate_subst_type(TP_SUBST_TYPE)

def fn_validate_subst_type(TP_SUBST_TYPE):
    param_dict = fn_update_subst_type()
    legal_set_subst_type = param_dict["enum"]
    return com.validate_legal_set(legal_set_subst_type, "TP_SUBST_TYPE", TP_SUBST_TYPE)


#######################################################
######### TP_L_LEADING Updater and Validator ##########
#######################################################
def update_TP_L_LEADING(args):
    return fn_update_l_leading()

def fn_update_l_leading():
    param_dict = {"name": "TP_L_LEADING", "enum": [0, 1]}
    return param_dict

def validate_TP_L_LEADING(args):
    TP_L_LEADING = args["TP_L_LEADING"]
    return fn_validate_l_leading(TP_L_LEADING)

def fn_validate_l_leading(TP_L_LEADING):
    param_dict = fn_update_l_leading()
    legal_set_l_leading = param_dict["enum"]
    return com.validate_legal_set(legal_set_l_leading, "TP_L_LEADING", TP_L_LEADING)


def get_port_info(portname, dir, dataType, windowVsize, apiType, idx=0):
    windowSize = windowVsize * com.fn_size_by_byte(dataType)
    return [
        {
            "name": f"{portname}[{idx}]",
            "type": f"{apiType}",
            "direction": f"{dir}",
            "data_type": dataType,
            "fn_is_complex": com.fn_is_complex(dataType),
            "window_size": windowSize,  # com.fn_input_window_size(windowVsize, dataType),
            "margin_size": 0,
        }
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
    numKernels = TP_GRID_DIM*(TP_GRID_DIM+1)//2
    portsInL = []
    portsInY = []
    portsOutX = []

    for kernel in range(0, numKernels):
        portsInL += get_port_info(
            portname="inL",
            dir="in",
            dataType=TT_DATA,
            windowVsize=TP_DIM * TP_DIM * TP_NUM_FRAMES,
            apiType="window",
            idx=kernel
        )
        portsInY += get_port_info(
            portname="in",
            dir="in",
            dataType=TT_DATA,
            windowVsize=TP_DIM * TP_NUM_FRAMES,
            apiType="window",
            idx=kernel
        )
        portsOutX += get_port_info(
            portname="out",
            dir="out",
            dataType=TT_DATA,
            windowVsize=TP_DIM * TP_NUM_FRAMES,
            apiType="window",
            idx=kernel
        )
    return portsInL + portsInY + portsOutX


#### graph generator ####
def generate_graph(graphname, args):
    if graphname == "":
        graphname = "default_graphname"

    TT_DATA = args["TT_DATA"]
    TP_DIM = args["TP_DIM"]
    TP_SUBST_TYPE = args["TP_SUBST_TYPE"]
    TP_L_LEADING = args["TP_L_LEADING"]
    TP_GRID_DIM = args["TP_GRID_DIM"]

    subst_type_str = "xf::solver::aie::substitution::subst_type::FWD" if TP_SUBST_TYPE == 0 else "xf::solver::aie::substitution::subst_type::BWD"

    code = f"""
class {graphname} : public adf::graph {{
public:
    adf::port<input> in_L;
    adf::port<input> in_y;
    adf::port<output> out_x;

    xf::solver::aie::substitution::substitution_graph<
        {TT_DATA},
        {TP_DIM},
        {subst_type_str},
        {TP_L_LEADING},
        {TP_GRID_DIM}
    > substitution;

    {graphname}() : substitution() {{
        for (int i = 0; i < {TP_GRID_DIM}; i++) {{
            adf::connect<> net_in_L(in_L[i], substitution.L_in[i]);
            adf::connect<> net_in_y(in_y[i], substitution.y_in[i]);
            adf::connect<> net_out_x(substitution.x_out[i], out_x[i]);
        }}
    }}
}};
"""
    out = {}
    out["graph"] = code
    out["port_info"] = info_ports(args)
    out["headerfile"] = "substitution_graph.hpp"
    out["searchpaths"] = [
        "L2/include/aie",
        "L2/tests/aie/common/inc",
        "L1/include/aie",
        "L1/src/aie",
        "L1/tests/aie/inc",
        "L1/tests/aie/src",
    ]
    return out
