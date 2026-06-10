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
    TP_DIM_ROWS_min = int(com.k_max_read_write_bytes[AIE_VARIANT] / byte_size[TT_DATA])
    TP_COLS_ROWS_min = TP_DIM_ROWS_min
    buffer_size = com.k_data_memory_bytes[AIE_VARIANT]
    buffer_size_sample = int(
        math.floor(buffer_size / (TP_COLS_ROWS_min * byte_size[TT_DATA]))
    )   

    TP_DIM_ROWS_max = buffer_size_sample * TP_CASC_LEN_max 

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
    buffer_size = com.k_data_memory_bytes[AIE_VARIANT]
    buffer_size_sample = int(
        math.floor(buffer_size / byte_size[TT_DATA])
    )

    TP_DIM_COLS_max_buffer_q = min(TP_DIM_ROWS, int((buffer_size_sample/(TP_DIM_ROWS))))

    param_dict = {}
    param_dict.update({"name": "TP_DIM_ROWS"})
    param_dict.update({"minimum": TP_DIM_COLS_min})
    param_dict.update({"maximum": TP_DIM_COLS_max_buffer_q})

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
    buffer_size= com.k_data_memory_bytes[AIE_VARIANT]
    buffer_size_sample = int(
        math.floor(buffer_size / byte_size[TT_DATA])
    )
    rows_per_kernel = math.ceil(TP_DIM_COLS / TP_CASC_LEN_max)

    TP_NUM_FRAMES_max_q=int(buffer_size_sample/(TP_DIM_COLS*rows_per_kernel))

    param_dict = {}

    param_dict.update({"name": "TP_NUM_FRAMES"})
    param_dict.update({"minimum": 1})
    param_dict.update({"maximum": TP_NUM_FRAMES_max_q})

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

    buffer_size= com.k_data_memory_bytes[AIE_VARIANT]
    buffer_size_sample = int(
        math.floor(buffer_size / byte_size[TT_DATA])
    )

    total_matrix_size_q = TP_DIM_ROWS * TP_DIM_COLS * TP_NUM_FRAMES
    casc_len_min_q = ceil(total_matrix_size_q/buffer_size_sample)

    TP_DIM_ROWS_min = com.k_max_read_write_bytes[AIE_VARIANT] / byte_size[TT_DATA]
    casc_len_max = int(TP_DIM_ROWS / TP_DIM_ROWS_min)
    row_chunks_total = TP_DIM_ROWS / TP_DIM_ROWS_min

    TP_CASC_LEN_legal_set = []
    for casc in range(casc_len_min_q, casc_len_max+1):
        if row_chunks_total % casc == 0:
            TP_CASC_LEN_legal_set.append(casc)

    param_dict = {}
    param_dict.update({"name": "TP_CASC_LEN"})
    param_dict.update({"enum": TP_CASC_LEN_legal_set})

    return param_dict


def validate_TP_CASC_LEN(args):
    TP_CASC_LEN = args["TP_CASC_LEN"]
    param_dict = update_TP_CASC_LEN(args)
    TP_CASC_LEN_legal_set = param_dict["enum"]
    return com.validate_legal_set(TP_CASC_LEN_legal_set, "TP_CASC_LEN", TP_CASC_LEN)

#######################################################
######## TP_DIM_A_LEADING Updater and Validator #######
#######################################################
def update_TP_DIM_A_LEADING(args):
    AIE_VARIANT = args["AIE_VARIANT"]
    TT_DATA = args["TT_DATA"]
    TP_DIM_ROWS = args["TP_DIM_ROWS"]
    return fn_update_TP_DIM_A_LEADING(AIE_VARIANT, TT_DATA, TP_DIM_ROWS)

def fn_update_TP_DIM_A_LEADING(AIE_VARIANT, TT_DATA, TP_DIM_ROWS):

    if (TT_DATA in ["cfloat"] or TP_DIM_ROWS > 255) and AIE_VARIANT in [com.AIE]:
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
    TP_DIM_ROWS = args["TP_DIM_ROWS"]
    TP_DIM_A_LEADING = args["TP_DIM_A_LEADING"]
    return fn_validate_TP_DIM_A_LEADING(AIE_VARIANT, TT_DATA, TP_DIM_ROWS, TP_DIM_A_LEADING)

def fn_validate_TP_DIM_A_LEADING(AIE_VARIANT, TT_DATA, TP_DIM_ROWS, TP_DIM_A_LEADING):    
    param_dict = fn_update_TP_DIM_A_LEADING(AIE_VARIANT, TT_DATA, TP_DIM_ROWS)
    legal_set_TP_DIM_A_LEADING = param_dict["enum"]
    return com.validate_legal_set(legal_set_TP_DIM_A_LEADING, "TP_DIM_A_LEADING", TP_DIM_A_LEADING)

#######################################################
######## TP_DIM_Q_LEADING Updater and Validator #######
#######################################################
def update_TP_DIM_Q_LEADING(args):
    AIE_VARIANT = args["AIE_VARIANT"]
    TT_DATA = args["TT_DATA"]
    TP_DIM_ROWS = args["TP_DIM_ROWS"]

    return fn_update_TP_DIM_Q_LEADING(AIE_VARIANT, TT_DATA, TP_DIM_ROWS)

def fn_update_TP_DIM_Q_LEADING(AIE_VARIANT, TT_DATA, TP_DIM_ROWS):

    if (TT_DATA in ["cfloat"] or TP_DIM_ROWS > 255) and AIE_VARIANT in [com.AIE]:
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
    TP_DIM_ROWS = args["TP_DIM_ROWS"]
    TP_DIM_Q_LEADING = args["TP_DIM_Q_LEADING"]
    return fn_validate_TP_DIM_Q_LEADING(AIE_VARIANT, TT_DATA, TP_DIM_ROWS, TP_DIM_Q_LEADING)

def fn_validate_TP_DIM_Q_LEADING(AIE_VARIANT, TT_DATA, TP_DIM_ROWS, TP_DIM_Q_LEADING):    
    param_dict = fn_update_TP_DIM_Q_LEADING(AIE_VARIANT, TT_DATA, TP_DIM_ROWS)
    legal_set_TP_DIM_Q_LEADING = param_dict["enum"]
    return com.validate_legal_set(legal_set_TP_DIM_Q_LEADING, "TP_DIM_Q_LEADING", TP_DIM_Q_LEADING)

#######################################################
######## TP_DIM_R_LEADING Updater and Validator #######
#######################################################
def update_TP_DIM_R_LEADING(args):
    AIE_VARIANT = args["AIE_VARIANT"]
    TT_DATA = args["TT_DATA"]
    TP_DIM_COLS = args["TP_DIM_COLS"]
    return fn_update_TP_DIM_R_LEADING(AIE_VARIANT, TT_DATA, TP_DIM_COLS)

def fn_update_TP_DIM_R_LEADING(AIE_VARIANT, TT_DATA, TP_DIM_COLS):

    if (TT_DATA in ["cfloat"] or TP_DIM_COLS > 255) and AIE_VARIANT in [com.AIE]:
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
    TP_DIM_COLS = args["TP_DIM_COLS"]
    TP_DIM_R_LEADING = args["TP_DIM_R_LEADING"]
    return fn_validate_TP_DIM_Q_LEADING(AIE_VARIANT, TT_DATA, TP_DIM_COLS, TP_DIM_R_LEADING)

def fn_validate_TP_DIM_R_LEADING(AIE_VARIANT, TT_DATA, TP_DIM_COLS, TP_DIM_R_LEADING):    
    param_dict = fn_update_TP_DIM_R_LEADING(AIE_VARIANT, TT_DATA, TP_DIM_COLS)
    legal_set_TP_DIM_Q_LEADING = param_dict["enum"]
    return com.validate_legal_set(legal_set_TP_DIM_Q_LEADING, "TP_DIM_R_LEADING", TP_DIM_R_LEADING)

#### port ####

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

    portsInA = []
    portsOutQ = []
    portsOutR = []

    row_per_kernel = math.ceil(TP_DIM_ROWS / TP_CASC_LEN)
    casc_num_r = math.ceil(TP_DIM_COLS / row_per_kernel) ##cascade number up to which R will be outputted
    R_col_dim_per_kernel=[]
    for kernel in range(casc_num_r):
        cols_distributred_to_kernel = kernel * row_per_kernel
        if row_per_kernel < TP_DIM_COLS - cols_distributred_to_kernel:
            R_col_dim_per_kernel.append(row_per_kernel)
        else:   
            R_col_dim_per_kernel.append(TP_DIM_COLS - cols_distributred_to_kernel) 

    for casc_num in range(0, TP_CASC_LEN):
        if casc_num < casc_num_r:
            TP_WINDOW_VSIZE_R = R_col_dim_per_kernel[casc_num]*TP_DIM_COLS*TP_NUM_FRAMES
        TP_WINDOW_VSIZE_Q = row_per_kernel*TP_DIM_COLS*TP_NUM_FRAMES
        
        portsInA += get_port_info(
            portname="inA",
            dir="in",
            dataType=TT_DATA,
            windowVsize=TP_WINDOW_VSIZE_Q,
            apiType="window",
            idx=casc_num
        )
        portsOutQ += get_port_info(
            portname="outQ",
            dir="out",
            dataType=TT_DATA,
            windowVsize=TP_WINDOW_VSIZE_Q,
            apiType="window",
            idx=casc_num
        )

        if casc_num < casc_num_r:
            portsOutR += get_port_info(
                portname="outR",
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
    TP_DIM_A_LEADING = args["TP_DIM_A_LEADING"]
    TP_DIM_Q_LEADING = args["TP_DIM_Q_LEADING"]
    TP_DIM_R_LEADING = args["TP_DIM_R_LEADING"]


    # Use formatted multi-line string to avoid a lot of \n and \t
    code = f"""
class {graphname} : public adf::graph {{
public:
  static constexpr unsigned int TP_CASC_LEN = {TP_CASC_LEN};
  static constexpr unsigned int row_per_kernel = ceil({TP_DIM_ROWS}/TP_CASC_LEN);
  static constexpr unsigned int casc_num_r = ceil({TP_DIM_COLS}/row_per_kernel);

  template <typename dir>
  using casc_port_array = std::array<adf::port<dir>, TP_CASC_LEN>;
  using casc_port_array_r = std::array<adf::port<dir>, casc_num_r>;

  casc_port_array<input> inA;
  casc_port_array<output> outQ;
  casc_port_array<output> outR;

  xf::dsp::aie::qrd_hh::qrd_hh_graph<
    {TT_DATA}, //TT_DATA
    {TP_DIM_ROWS}, //TP_DIM_ROWS
    {TP_DIM_COLS}, //TP_DIM_COLS
    {TP_NUM_FRAMES}, //TP_NUM_FRAMES
    {TP_CASC_LEN}, //TP_CASC_LEN
    {TP_DIM_A_LEADING}, //TP_DIM_A_LEADING
    {TP_DIM_Q_LEADING}, //TP_DIM_Q_LEADING
    {TP_DIM_R_LEADING} //TP_DIM_R_LEADING
  > qrd_hh;

  {graphname}() : qrd_hh() {{
    adf::kernel *qrd_hh_kernels = qrd_hh.getKernels();

    for (int i=0; i < TP_CASC_LEN; i++) {{
      unsigned int kRrowsDistributed = i*({int(TP_DIM_ROWS/TP_CASC_LEN)});
      bool routEn = (kRrowsDistributed < {TP_DIM_COLS});

      adf::connect<> net_in(inA[i], qrd_hh.inA[i]);
      adf::connect<> net_out(qrd_hh.outQ[i], outQ[i]);
      if (routEn) {{adf::connect<> net_out(qrd_hh.outR[i], outR[i]);}}
    }}
  }}

}};
"""
    out = {}
    out["graph"] = code
    out["port_info"] = info_ports(args)
    out["headerfile"] = "qrd_hh_graph.hpp"
    out["searchpaths"] = [
        "L2/include/aie",
        "L2/tests/aie/common/inc",
        "L1/include/aie",
        "L1/src/aie",
        "L1/tests/aie/inc",
        "L1/tests/aie/src",
    ]

    return out
