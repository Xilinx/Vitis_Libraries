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
import sys, os
import aie_common as com
import aie_common_fir_updaters as com_upd
from aie_common import *

import math as math
from math import floor, ceil, sqrt

script_directory = os.path.dirname(os.path.abspath(__file__))
L2path=script_directory+"/.."
include_path = L2path + "/tests/aie/common/scripts"
sys.path.insert(0, include_path)

from kernel_load_utils import *


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

TP_INPUT_NUM_FRAMES_min = 1
TP_CASC_LEN_min = 1
TP_CASC_LEN_max = 40 #Needs investigation


byte_size = {"int16": 2, "int32": 4, "cint16": 4, "cint32": 8, "float": 4, "cfloat": 8}


#######################################################
########### AIE_VARIANT Updater and Validtor###########
#######################################################
def update_AIE_VARIANT(args):
    return fn_update_aie_variant()


def fn_update_aie_variant():
    legal_set_AIE_VARIANT = [com.AIE, com.AIE_ML, com.AIE_MLv2]
    param_dict = {}

    param_dict.update({"name": "AIE_VARIANT"})
    param_dict.update({"enum": legal_set_AIE_VARIANT})

    return param_dict


def validate_AIE_VARIANT(args):
    AIE_VARIANT = args["AIE_VARIANT"]
    param_dict = update_AIE_VARIANT(args)
    legal_set_AIE_VARIANT = param_dict["enum"]
    return com.validate_legal_set(legal_set_AIE_VARIANT, "AIE_VARIANT", AIE_VARIANT)


#######################################################
########### TT_DATA Updater and Validator #############
#######################################################
def update_TT_DATA(args):
    return fn_update_tt_data()


def fn_update_tt_data():
    legal_set_TT_DATA = ["float", "cfloat"]
    param_dict = {}

    param_dict.update({"name": "TT_DATA"})
    param_dict.update({"enum": legal_set_TT_DATA})

    return param_dict


def validate_TT_DATA(args):
    TT_DATA = args["TT_DATA"]
    param_dict = update_TT_DATA(args)
    legal_set_TT_DATA = param_dict["enum"]
    return com.validate_legal_set(legal_set_TT_DATA, "TT_DATA", TT_DATA)


#######################################################
########### TP_DIM_ROWS Updater and Validator #########
#######################################################
def update_TP_DIM_ROWS(args):
    AIE_VARIANT = args["AIE_VARIANT"]
    TT_DATA = args["TT_DATA"]

    if "TP_DIM_ROWS" in args and args["TP_DIM_ROWS"]:
        TP_DIM_ROWS = args["TP_DIM_ROWS"]
    else: 
        TP_DIM_ROWS = 0
    
    return fn_update_tp_dim_rows(AIE_VARIANT, TT_DATA, TP_DIM_ROWS)


def fn_update_tp_dim_rows(AIE_VARIANT, TT_DATA, TP_DIM_ROWS):
    TP_DIM_ROWS_min = com.k_max_read_write_bytes[AIE_VARIANT] / byte_size[TT_DATA]

    BUFFER_SIZE = com.k_data_memory_bytes[AIE_VARIANT]

    TP_DIM_ROWS_max = int(
        math.floor(BUFFER_SIZE / byte_size[TT_DATA])
    )

    param_dict = {}
    param_dict.update({"name": "TP_DIM_ROWS"})
    param_dict.update({"minimum": TP_DIM_ROWS_min})
    param_dict.update({"maximum": TP_DIM_ROWS_max})


    if TP_DIM_ROWS !=0:
        if (TP_DIM_ROWS % TP_DIM_ROWS_min) != 0:
            TP_DIM_ROWS_actual = int(com.CEIL(TP_DIM_ROWS, TP_DIM_ROWS_min))
            param_dict.update({"actual": TP_DIM_ROWS_actual})

    return param_dict


def validate_TP_DIM_ROWS(args):
    TP_DIM_ROWS = args["TP_DIM_ROWS"]

    param_dict = update_TP_DIM_ROWS(args)
    range_TP_DIM_ROWS = [param_dict["minimum"], param_dict["maximum"]]

    if TP_DIM_ROWS % param_dict["minimum"] != 0:
        return com.isError(f"TP_DIM_ROWS must be a multiple of {param_dict['minimum']}.")
    return com.validate_range(range_TP_DIM_ROWS, "TP_DIM_ROWS", TP_DIM_ROWS)

#######################################################
########### TP_DIM_COLS Updater and Validator #########
#######################################################
def update_TP_DIM_COLS(args):
    AIE_VARIANT = args["AIE_VARIANT"]
    TT_DATA = args["TT_DATA"]
    TP_DIM_ROWS = args["TP_DIM_ROWS"]

    if "TP_DIM_COLS" in args and args["TP_DIM_COLS"]:
        TP_DIM_COLS = args["TP_DIM_COLS"]
    else: 
        TP_DIM_COLS = 0
    
    return fn_update_tp_dim_cols(AIE_VARIANT, TT_DATA, TP_DIM_ROWS, TP_DIM_COLS)


def fn_update_tp_dim_cols(AIE_VARIANT, TT_DATA, TP_DIM_ROWS, TP_DIM_COLS):
    TP_DIM_COLS_min = int(com.k_max_read_write_bytes[AIE_VARIANT] / byte_size[TT_DATA])

    BUFFER_SIZE= com.k_data_memory_bytes[AIE_VARIANT]
    BUFFER_SIZE_sample = int(
        math.floor(BUFFER_SIZE / byte_size[TT_DATA])
    )

    TP_DIM_COLS_max_buffer_q = int((BUFFER_SIZE_sample/TP_DIM_ROWS) * TP_CASC_LEN_max)
    TP_DIM_COLS_max_buffer_r = int(math.sqrt(BUFFER_SIZE_sample* TP_CASC_LEN_max))
    TP_DIM_COLS_max = min(TP_DIM_COLS_max_buffer_q, TP_DIM_COLS_max_buffer_r)


    param_dict = {}
    param_dict.update({"name": "TP_DIM_ROWS"})
    param_dict.update({"minimum": TP_DIM_COLS_min})
    param_dict.update({"maximum": TP_DIM_COLS_max})

    if TP_DIM_COLS !=0:
        if (TP_DIM_COLS % TP_DIM_COLS_min) != 0:
            TP_DIM_COLS_actual = int(com.CEIL(TP_DIM_COLS, TP_DIM_COLS_min))
            param_dict.update({"actual": TP_DIM_COLS_actual})

    return param_dict


def validate_TP_DIM_COLS(args):
    TP_DIM_COLS = args["TP_DIM_COLS"]

    param_dict = update_TP_DIM_COLS(args)
    range_TP_DIM_COLS = [param_dict["minimum"], param_dict["maximum"]]

    if TP_DIM_COLS % param_dict["minimum"] != 0:
        return com.isError(f"TP_DIM_COLS must be a multiple of {param_dict['minimum']}.")
    return com.validate_range(range_TP_DIM_COLS, "TP_DIM_COLS", TP_DIM_COLS)

#######################################################
########### TP_NUM_FRAMES Updater and Validator #######
#######################################################
def update_TP_NUM_FRAMES(args):
    AIE_VARIANT = args["AIE_VARIANT"]
    TT_DATA = args["TT_DATA"]
    TP_DIM_ROWS = args["TP_DIM_ROWS"]
    TP_DIM_COLS = args["TP_DIM_COLS"]    

    return fn_update_tp_num_frames(
        AIE_VARIANT, TT_DATA,  TP_DIM_ROWS, TP_DIM_COLS
    )


def fn_update_tp_num_frames(AIE_VARIANT, TT_DATA,  TP_DIM_ROWS, TP_DIM_COLS):
    BUFFER_SIZE= com.k_data_memory_bytes[AIE_VARIANT]
    BUFFER_SIZE_sample = int(
        math.floor(BUFFER_SIZE / byte_size[TT_DATA])
    )
    cols_per_kernel_min = math.ceil(TP_DIM_COLS / TP_CASC_LEN_max)


    TP_NUM_FRAMES_max_q=int(BUFFER_SIZE_sample/(TP_DIM_ROWS*cols_per_kernel_min))
    TP_NUM_FRAMES_max_r=int(BUFFER_SIZE_sample/(cols_per_kernel_min*cols_per_kernel_min))
    TP_NUM_FRAMES_max = min(TP_NUM_FRAMES_max_q, TP_NUM_FRAMES_max_r)

    param_dict = {}

    param_dict.update({"name": "TP_NUM_FRAMES"})
    param_dict.update({"minimum": 1})
    param_dict.update({"maximum": TP_NUM_FRAMES_max})

    return param_dict


def validate_TP_NUM_FRAMES(args):
    TP_NUM_FRAMES = args["TP_NUM_FRAMES"]
    param_dict = update_TP_NUM_FRAMES(args)
    range_TP_NUM_FRAMES = [param_dict["minimum"], param_dict["maximum"]]
    return com.validate_range(range_TP_NUM_FRAMES, "TP_NUM_FRAMES", TP_NUM_FRAMES)

#######################################################
########### TP_CASC_LEN Updater and Validator #########
#######################################################
def update_TP_CASC_LEN(args):
    AIE_VARIANT = args["AIE_VARIANT"]
    TT_DATA = args["TT_DATA"]
    TP_DIM_ROWS = args["TP_DIM_ROWS"]
    TP_DIM_COLS = args["TP_DIM_COLS"]    
    TP_NUM_FRAMES = args["TP_NUM_FRAMES"]    
    return fn_update_tp_casc_len(AIE_VARIANT, TT_DATA,  TP_DIM_ROWS, TP_DIM_COLS, TP_NUM_FRAMES)

def fn_update_tp_casc_len(AIE_VARIANT, TT_DATA, TP_DIM_ROWS, TP_DIM_COLS, TP_NUM_FRAMES):

    BUFFER_SIZE= com.k_data_memory_bytes[AIE_VARIANT]
    BUFFER_SIZE_sample = int(
        math.floor(BUFFER_SIZE / byte_size[TT_DATA])
    )

    total_matrix_size_q = TP_DIM_ROWS * TP_DIM_COLS * TP_NUM_FRAMES
    total_matrix_size_r = TP_DIM_COLS * TP_DIM_COLS * TP_NUM_FRAMES

    casc_len_min_q = ceil(total_matrix_size_q/BUFFER_SIZE_sample)
    casc_len_min_r = ceil(total_matrix_size_r/BUFFER_SIZE_sample)  
    casc_len_min = max(casc_len_min_q, casc_len_min_r)
    col_per_kernel_min = floor(TP_DIM_COLS/casc_len_min)
    casc_len_min = ceil(TP_DIM_COLS/col_per_kernel_min)

    casc_len_max = min(TP_DIM_COLS, TP_CASC_LEN_max)
    while True:
        col_dim_kernel_list=qrd_load_split(AIE_VARIANT, TT_DATA, TP_DIM_ROWS, TP_DIM_COLS, casc_len_max, TP_NUM_FRAMES)
        if 0 not in col_dim_kernel_list:
            break
        casc_len_max -= 1

    param_dict = {}
    param_dict.update({"name": "TP_CASC_LEN"})
    param_dict.update({"minimum": casc_len_min})
    param_dict.update({"maximum": casc_len_max})

    return param_dict


def validate_TP_CASC_LEN(args):
    TP_CASC_LEN = args["TP_CASC_LEN"]
    param_dict = update_TP_CASC_LEN(args)
    range_TP_CASC_LEN = [param_dict["minimum"], param_dict["maximum"]]
    return com.validate_range(range_TP_CASC_LEN, "TP_CASC_LEN", TP_CASC_LEN)

#######################################################
######## TP_DIM_A_LEADING Updater and Validator #######
#######################################################
def update_TP_DIM_A_LEADING(args):
    AIE_VARIANT = args["AIE_VARIANT"]
    TT_DATA = args["TT_DATA"]
    return fn_update_TP_DIM_A_LEADING(AIE_VARIANT, TT_DATA)

def fn_update_TP_DIM_A_LEADING(AIE_VARIANT, TT_DATA):
    #in 25.2 cfloat transpose will not be allowed for any AIE variants
    #aie1 is naturally not capable, aie2 and aie22 tests are not finishing, so disabling for now

    if TT_DATA in ["cfloat"]:
    # if TT_DATA in ["cfloat"] and AIE_VARIANT in [com.AIE]:
        legal_set_TP_DIM_A_LEADING = [0]
    else:
        legal_set_TP_DIM_A_LEADING = [0, 1]
    param_dict = {}
    param_dict.update({"name": "TP_DIM_A_LEADING"})
    param_dict.update({"enum": legal_set_TP_DIM_A_LEADING})
    return param_dict

def validate_TP_DIM_A_LEADING(args):
    AIE_VARIANT = args["AIE_VARIANT"]
    TT_DATA = args["TT_DATA"]
    TP_DIM_A_LEADING = args["TP_DIM_A_LEADING"]
    return fn_validate_TP_DIM_A_LEADING(AIE_VARIANT, TT_DATA, TP_DIM_A_LEADING)

def fn_validate_TP_DIM_A_LEADING(AIE_VARIANT, TT_DATA, TP_DIM_A_LEADING):    
    param_dict = fn_update_TP_DIM_A_LEADING(AIE_VARIANT, TT_DATA)
    legal_set_TP_DIM_A_LEADING = param_dict["enum"]
    return com.validate_legal_set(legal_set_TP_DIM_A_LEADING, "TP_DIM_A_LEADING", TP_DIM_A_LEADING)

#######################################################
######## TP_DIM_Q_LEADING Updater and Validator #######
#######################################################
def update_TP_DIM_Q_LEADING(args):
    AIE_VARIANT = args["AIE_VARIANT"]
    TT_DATA = args["TT_DATA"]
    return fn_update_TP_DIM_Q_LEADING(AIE_VARIANT, TT_DATA)

def fn_update_TP_DIM_Q_LEADING(AIE_VARIANT, TT_DATA):
    #in 25.2 cfloat transpose will not be allowed for any AIE variants
    #aie1 is naturally not capable, aie2 and aie22 tests are not finishing, so disabling for now

    if TT_DATA in ["cfloat"]:
    # if TT_DATA in ["cfloat"] and AIE_VARIANT in [com.AIE]:
        legal_set_TP_DIM_Q_LEADING = [0]
    else:
        legal_set_TP_DIM_Q_LEADING = [0, 1]
    param_dict = {}
    param_dict.update({"name": "TP_DIM_Q_LEADING"})
    param_dict.update({"enum": legal_set_TP_DIM_Q_LEADING})
    return param_dict

def validate_TP_DIM_Q_LEADING(args):
    AIE_VARIANT = args["AIE_VARIANT"]
    TT_DATA = args["TT_DATA"]
    TP_DIM_Q_LEADING = args["TP_DIM_Q_LEADING"]
    return fn_validate_TP_DIM_Q_LEADING(AIE_VARIANT, TT_DATA, TP_DIM_Q_LEADING)

def fn_validate_TP_DIM_Q_LEADING(AIE_VARIANT, TT_DATA, TP_DIM_Q_LEADING):    
    param_dict = fn_update_TP_DIM_Q_LEADING(AIE_VARIANT, TT_DATA)
    legal_set_TP_DIM_Q_LEADING = param_dict["enum"]
    return com.validate_legal_set(legal_set_TP_DIM_Q_LEADING, "TP_DIM_Q_LEADING", TP_DIM_Q_LEADING)

#######################################################
######## TP_DIM_R_LEADING Updater and Validator #######
#######################################################
def update_TP_DIM_R_LEADING(args):
    AIE_VARIANT = args["AIE_VARIANT"]
    TT_DATA = args["TT_DATA"]
    return fn_update_TP_DIM_R_LEADING(AIE_VARIANT, TT_DATA)

def fn_update_TP_DIM_R_LEADING(AIE_VARIANT, TT_DATA):
    #in 25.2 cfloat transpose will not be allowed for any AIE variants
    #aie1 is naturally not capable, aie2 and aie22 tests are not finishing, so disabling for now

    if TT_DATA in ["cfloat"]:
    # if TT_DATA in ["cfloat"] and AIE_VARIANT in [com.AIE]:
        legal_set_TP_DIM_R_LEADING = [0]
    else:
        legal_set_TP_DIM_R_LEADING = [0, 1]
    param_dict = {}
    param_dict.update({"name": "TP_DIM_R_LEADING"})
    param_dict.update({"enum": legal_set_TP_DIM_R_LEADING})
    return param_dict

def validate_TP_DIM_R_LEADING(args):
    AIE_VARIANT = args["AIE_VARIANT"]
    TT_DATA = args["TT_DATA"]
    TP_DIM_R_LEADING = args["TP_DIM_R_LEADING"]
    return fn_validate_TP_DIM_Q_LEADING(AIE_VARIANT, TT_DATA, TP_DIM_R_LEADING)

def fn_validate_TP_DIM_R_LEADING(AIE_VARIANT, TT_DATA, TP_DIM_R_LEADING):    
    param_dict = fn_update_TP_DIM_R_LEADING(AIE_VARIANT, TT_DATA)
    legal_set_TP_DIM_Q_LEADING = param_dict["enum"]
    return com.validate_legal_set(legal_set_TP_DIM_Q_LEADING, "TP_DIM_R_LEADING", TP_DIM_R_LEADING)

#### port ####
def calc_port_window_size(TT_DATA, TP_DIM_ROWS, TP_DIM_COLS, TP_NUM_FRAMES, TP_CAS_LEN):
    return TP_DIM_ROWS * TP_DIM_COLS * TP_NUM_FRAMES * com.fn_size_by_byte(TT_DATA)



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
    AIE_VARIANT = args["AIE_VARIANT"]
    TT_DATA = args["TT_DATA"]
    TP_DIM_ROWS = args["TP_DIM_ROWS"]
    TP_DIM_COLS = args["TP_DIM_COLS"]
    TP_NUM_FRAMES = args["TP_NUM_FRAMES"]
    TP_CASC_LEN = args["TP_CASC_LEN"]

    col_dim_kernel_list=qrd_load_split(AIE_VARIANT, TT_DATA, TP_DIM_ROWS, TP_DIM_COLS, TP_CASC_LEN, TP_NUM_FRAMES)
    portsInA = []
    portsOutQ = []
    portsOutR = []
    for casc_num in range(0, TP_CASC_LEN):
        colSize = col_dim_kernel_list[casc_num]
        TP_WINDOW_VSIZE_R = TP_DIM_ROWS*colSize*TP_NUM_FRAMES
        TP_WINDOW_VSIZE_Q = colSize*colSize*TP_NUM_FRAMES
        
        portsInA += get_port_info(
            portname="inA",
            dir="in",
            dataType=TT_DATA,
            windowVsize=TP_WINDOW_VSIZE_Q,
            apiType="window",
            idx=casc_num
        )
        portsOutQ += get_port_info(
            portname="out",
            dir="out",
            dataType=TT_DATA,
            windowVsize=TP_WINDOW_VSIZE_Q,
            apiType="window",
            idx=casc_num
        )
        portsOutR += get_port_info(
            portname="out",
            dir="out",
            dataType=TT_DATA,
            windowVsize=TP_WINDOW_VSIZE_R,
            apiType="window",
            idx=casc_num
        )   
    return portsInA + portsOutQ + portsOutR


#### graph generator ####
def generate_graph(graphname, args):

    if graphname == "":
        graphname = "default_graphname"

    TT_DATA = args["TT_DATA"]
    TP_DIM_ROWS = args["TP_DIM_ROWS"]
    TP_DIM_COLS = args["TP_DIM_COLS"]
    TP_NUM_FRAMES = args["TP_NUM_FRAMES"]
    TP_CASC_LEN = args["TP_CASC_LEN"]


    # Use formatted multi-line string to avoid a lot of \n and \t
    code = f"""
class {graphname} : public adf::graph {{
public:
  static constexpr unsigned int TP_CASC_LEN = {TP_CASC_LEN};
  template <typename dir>
  using casc_port_array = std::array<adf::port<dir>, TP_CASC_LEN>;

  casc_port_array<input> inA;
  casc_port_array<output> outQ;
  casc_port_array<output> outR;

  xf::dsp::aie::qrd::qrd_graph<
    {TT_DATA}, //TT_DATA
    {TP_DIM_ROWS}, //TP_DIM_ROWS
    {TP_DIM_COLS}, //TP_DIM_COLS
    {TP_NUM_FRAMES}, //TP_NUM_FRAMES
    {TP_CASC_LEN}, //TP_CASC_LEN
  > qrd;

  {graphname}() : qrd() {{
    adf::kernel *qrd_kernels = qrd.getKernels();

    for (int i=0; i < TP_CASC_LEN; i++) {{
      adf::connect<> net_in(inA[i], qrd.inA[i]);
      adf::connect<> net_in(inB[i], qrd.inB[i]);
      adf::connect<> net_out(qrd.out[i], out[i]);
    }}
  }}

}};
"""
    out = {}
    out["graph"] = code
    out["port_info"] = info_ports(args)
    out["headerfile"] = "qrd_graph.hpp"
    out["searchpaths"] = [
        "L2/include/aie",
        "L2/tests/aie/common/inc",
        "L1/include/aie",
        "L1/src/aie",
        "L1/tests/aie/inc",
        "L1/tests/aie/src",
    ]

    return out
