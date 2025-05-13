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
# from socket import TIPC_SUB_SERVICE
import aie_common as com
from aie_common import *

# import json
import math

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
# PING_PONG_BUFFER_16kB = 16384
# PING_PONG_BUFFER_32kB = 32768
TP_INPUT_NUM_FRAMES_min = 1
TP_SSR_min = 1
TP_SSR_max = 32
TP_SHIFT_max = 60
TP_SHIFT_cint16_max = 32
TP_CASC_LEN_min = 1
TP_CASC_LEN_max = 11
# TP_API_min=0
# TP_API_max=1
# TP_DYN_PT_SIZE_min=0
# TP_DYN_PT_SIZE_max=1
# AIE_VARIANT_min=2
# AIE_VARIANT_max=1

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
    return validate_legal_set(legal_set_AIE_VARIANT, "AIE_VARIANT", AIE_VARIANT)


#######################################################
########### TT_DATA_A Updater and Validator ###########
#######################################################
def update_TT_DATA_A(args):
    return fn_update_tt_data_a()


def fn_update_tt_data_a():
    legal_set_TT_DATA_A = ["int16", "int32", "cint16", "cint32", "float", "cfloat"]
    param_dict = {}

    param_dict.update({"name": "TT_DATA_A"})
    param_dict.update({"enum": legal_set_TT_DATA_A})

    return param_dict


def validate_TT_DATA_A(args):
    TT_DATA_A = args["TT_DATA_A"]
    param_dict = update_TT_DATA_A(args)
    legal_set_TT_DATA_A = param_dict["enum"]
    return validate_legal_set(legal_set_TT_DATA_A, "TT_DATA_A", TT_DATA_A)


#######################################################
########### TT_DATA_B Updater and Validator ###########
#######################################################
def update_TT_DATA_B(args):
    AIE_VARIANT = args["AIE_VARIANT"]
    TT_DATA_A = args["TT_DATA_A"]
    return fn_update_tt_data_b(AIE_VARIANT, TT_DATA_A)


def fn_update_tt_data_b(AIE_VARIANT, TT_DATA_A):
    legal_set_TT_DATA_B = ["int16", "int32", "cint16", "cint32", "float", "cfloat"]

    if TT_DATA_A in ["float", "cfloat"]:
        legal_set_TT_DATA_B = remove_from_set(
            ["int16", "int32", "cint16", "cint32"], legal_set_TT_DATA_B
        )
    elif (TT_DATA_A in "int16", "int32", "cint16", "cint32"):
        legal_set_TT_DATA_B = remove_from_set(["float", "cfloat"], legal_set_TT_DATA_B)

    if AIE_VARIANT == AIE_ML or AIE_VARIANT == AIE_MLv2:
        if TT_DATA_A == "cint16":
            legal_set_TT_DATA_B.remove("int32")
        if TT_DATA_A == "int32":
            legal_set_TT_DATA_B.remove("cint16")

    param_dict = {}
    param_dict.update({"name": "TT_DATA_B"})
    param_dict.update({"enum": legal_set_TT_DATA_B})

    return param_dict


def validate_TT_DATA_B(args):
    TT_DATA_B = args["TT_DATA_B"]
    param_dict = update_TT_DATA_B(args)
    legal_set_TT_DATA_B = param_dict["enum"]
    return validate_legal_set(legal_set_TT_DATA_B, "TT_DATA_B", TT_DATA_B)


#######################################################
########### TP_API Updater and Validator ##############
#######################################################
def update_TP_API(args):
    AIE_VARIANT = args["AIE_VARIANT"]
    return fn_update_tp_api(AIE_VARIANT)


def fn_update_tp_api(AIE_VARIANT):

    if AIE_VARIANT == AIE:
        legal_set_TP_API = [0, 1]
    elif AIE_VARIANT == AIE_ML or AIE_VARIANT == AIE_MLv2:
        legal_set_TP_API = [0]

    param_dict = {}
    param_dict.update({"name": "TP_API"})
    param_dict.update({"enum": legal_set_TP_API})

    return param_dict


def validate_TP_API(args):
    TP_API = args["TP_API"]
    param_dict = update_TP_API(args)
    legal_set_TP_API = param_dict["enum"]
    return validate_legal_set(legal_set_TP_API, "TP_API", TP_API)


#######################################################
########### TP_SSR Updater and Validator ##############
#######################################################
def update_TP_SSR(args):
    return fn_update_tp_ssr()


def fn_update_tp_ssr():

    param_dict = {}
    param_dict.update({"name": "TP_SSR"})
    param_dict.update({"minimum": TP_SSR_min})
    param_dict.update({"maximum": TP_SSR_max})

    return param_dict


def validate_TP_SSR(args):
    TP_SSR = args["TP_SSR"]
    param_dict = update_TP_SSR(args)
    range_TP_SSR = [param_dict["minimum"], param_dict["maximum"]]
    return validate_range(range_TP_SSR, "TP_SSR", TP_SSR)


#######################################################
########### TP_DIM Updater and Validator ###########
#######################################################


def update_TP_DIM(args):

    AIE_VARIANT = args["AIE_VARIANT"]
    TT_DATA_A = args["TT_DATA_A"]
    TT_DATA_B = args["TT_DATA_B"]
    TP_SSR = args["TP_SSR"]

    return fn_update_tp_dim(AIE_VARIANT, TT_DATA_A, TT_DATA_B, TP_SSR)


def fn_update_tp_dim(AIE_VARIANT, TT_DATA_A, TT_DATA_B, TP_SSR):
    TP_DIM_min = 16 * TP_SSR

    BUFFER_SIZE = k_data_memory_bytes[AIE_VARIANT]
    buf_in_total = BUFFER_SIZE * TP_SSR
    TP_DIM_max = int(
        math.floor(buf_in_total / byte_size[fn_det_out_type(TT_DATA_A, TT_DATA_B)])
    )

    param_dict = {}
    param_dict.update({"name": "TP_DIM"})
    param_dict.update({"minimum": TP_DIM_min})
    param_dict.update({"maximum": TP_DIM_max})
    param_dict.update({"maximum_pingpong_buf": int(TP_DIM_max / 2)})

    return param_dict


def validate_TP_DIM(args):
    TP_DIM = args["TP_DIM"]
    param_dict = update_TP_DIM(args)
    range_TP_DIM = [param_dict["minimum"], param_dict["maximum"]]
    return validate_range(range_TP_DIM, "TP_DIM", TP_DIM)


#######################################################
########### TP_NUM_FRAMES Updater and Validator #######
#######################################################
def update_TP_NUM_FRAMES(args):
    AIE_VARIANT = args["AIE_VARIANT"]
    TP_DIM = args["TP_DIM"]
    TP_API = args["TP_API"]
    TT_DATA_A = args["TT_DATA_A"]
    TT_DATA_B = args["TT_DATA_B"]
    TP_SSR = args["TP_SSR"]

    return fn_update_tp_num_frames(
        AIE_VARIANT, TT_DATA_A, TT_DATA_B, TP_API, TP_SSR, TP_DIM
    )


def fn_update_tp_num_frames(AIE_VARIANT, TT_DATA_A, TT_DATA_B, TP_API, TP_SSR, TP_DIM):
    TP_NUM_FRAMES_max = calc_max_num_frames(
        AIE_VARIANT, TT_DATA_A, TT_DATA_B, TP_API, TP_SSR, TP_DIM
    )

    param_dict = {}

    param_dict.update({"name": "TP_NUM_FRAMES"})
    param_dict.update({"minimum": 1})
    param_dict.update({"maximum": TP_NUM_FRAMES_max})
    param_dict.update({"maximum_pingpong_buf": int(TP_NUM_FRAMES_max / 2)})

    return param_dict


def validate_TP_NUM_FRAMES(args):
    TP_NUM_FRAMES = args["TP_NUM_FRAMES"]
    param_dict = update_TP_NUM_FRAMES(args)
    range_TP_NUM_FRAMES = [param_dict["minimum"], param_dict["maximum"]]
    return validate_range(range_TP_NUM_FRAMES, "TP_NUM_FRAMES", TP_NUM_FRAMES)


def calc_max_num_frames(AIE_VARIANT, TT_DATA_A, TT_DATA_B, TP_API, TP_SSR, TP_DIM):
    BUFFER_SIZE = k_data_memory_bytes[AIE_VARIANT]
    VEC_IN_FRAME = calc_vecinframe(TP_API, TT_DATA_A, TT_DATA_B)
    TP_DIM_PADDED = calc_padded_tp_dim(VEC_IN_FRAME, TP_DIM, TP_SSR)
    OUT_BYTE = byte_size[fn_det_out_type(TT_DATA_A, TT_DATA_B)]
    TP_DIM_PADDED_BYTE = TP_DIM_PADDED * OUT_BYTE
    TP_INPUT_NUM_FRAMES_max = math.floor(BUFFER_SIZE / TP_DIM_PADDED_BYTE)
    return TP_INPUT_NUM_FRAMES_max


#######################################################
########### TP_SHIFT Updater and Validator #######
#######################################################
def update_TP_SHIFT(args):
    TT_DATA = args["TT_DATA_B"]
    AIE_VARIANT = args["AIE_VARIANT"]
    return fn_update_TP_SHIFT(AIE_VARIANT, TT_DATA)


def fn_update_TP_SHIFT(AIE_VARIANT, TT_DATA):
    range_TP_SHIFT = com.fn_update_range_TP_SHIFT(AIE_VARIANT, TT_DATA)

    param_dict = {
        "name": "TP_SHIFT",
        "minimum": range_TP_SHIFT[0],
        "maximum": range_TP_SHIFT[1],
    }
    return param_dict


def validate_TP_SHIFT(args):
    TT_DATA = args["TT_DATA_B"]
    TP_SHIFT = args["TP_SHIFT"]
    AIE_VARIANT = args["AIE_VARIANT"]
    return fn_validate_TP_SHIFT(AIE_VARIANT, TT_DATA, TP_SHIFT)


def fn_validate_TP_SHIFT(AIE_VARIANT, TT_DATA, TP_SHIFT):
    param_dict = fn_update_TP_SHIFT(AIE_VARIANT, TT_DATA)
    range_TP_SHIFT = [param_dict["minimum"], param_dict["maximum"]]
    return validate_range(range_TP_SHIFT, "TP_SHIFT", TP_SHIFT)


#######################################################
########### TP_RND Updater and Validator #######
#######################################################
def update_TP_RND(args):
    AIE_VARIANT = args["AIE_VARIANT"]
    return fn_update_tp_rnd(AIE_VARIANT)


def fn_update_tp_rnd(AIE_VARIANT):
    legal_set_TP_RND = fn_get_legalSet_roundMode(AIE_VARIANT)
    param_dict = {}
    param_dict.update({"name": "TP_RND"})
    param_dict.update({"enum": legal_set_TP_RND})

    return param_dict


def validate_TP_RND(args):
    AIE_VARIANT = args["AIE_VARIANT"]
    TP_RND = args["TP_RND"]
    return fn_validate_TP_RND(AIE_VARIANT, TP_RND)


def fn_validate_TP_RND(AIE_VARIANT, TP_RND):
    legal_set_TP_RND = fn_get_legalSet_roundMode(AIE_VARIANT)
    return validate_legal_set(legal_set_TP_RND, "TP_RND", TP_RND)


#######################################################
########### TP_SAT Updater and Validator ###########
#######################################################
def update_TP_SAT(args):
    return fn_update_tp_sat()


def fn_update_tp_sat():
    legal_set = [0, 1, 3]

    param_dict = {}
    param_dict.update({"name": "TP_SAT"})
    param_dict.update({"enum": legal_set})
    return param_dict


def validate_TP_SAT(args):
    TP_SAT = args["TP_SAT"]
    param_dict = update_TP_SAT(args)
    legal_set_TP_SAT = param_dict["enum"]
    return validate_legal_set(legal_set_TP_SAT, "TP_SAT", TP_SAT)


def fn_det_out_type(TT_DATA_A, TT_DATA_B):
    if TT_DATA_A == "int16" and TT_DATA_B == "int16":
        return "int16"
    if TT_DATA_A == "int16" and TT_DATA_B == "int32":
        return "int32"
    if TT_DATA_A == "int16" and TT_DATA_B == "cint16":
        return "cint16"
    if TT_DATA_A == "int32" and TT_DATA_B == "int16":
        return "int32"
    if TT_DATA_A == "int32" and TT_DATA_B == "int32":
        return "int32"
    if TT_DATA_A == "int32" and TT_DATA_B == "cint16":
        return "cint32"
    if TT_DATA_A == "cint16" and TT_DATA_B == "int16":
        return "cint16"
    if TT_DATA_A == "cint16" and TT_DATA_B == "int32":
        return "cint32"
    if TT_DATA_A == "cint16" and TT_DATA_B == "cint16":
        return "cint16"
    if TT_DATA_A == "cint32" or TT_DATA_B == "cint32":
        return "cint32"
    if TT_DATA_A == "float" and TT_DATA_B == "float":
        return "float"
    if TT_DATA_A == "cfloat" or TT_DATA_B == "cfloat":
        return "cfloat"


def fn_det_calc_byte(TT_DATA_A, TT_DATA_B):
    if TT_DATA_A == "int16" and TT_DATA_B == "int16":
        return 2
    if TT_DATA_A == "int16" and TT_DATA_B == "int32":
        return 4
    if TT_DATA_A == "int16" and TT_DATA_B == "cint16":
        return 4
    if TT_DATA_A == "int16" and TT_DATA_B == "cint32":
        return 2
    if TT_DATA_A == "int32" and TT_DATA_B == "int16":
        return 4
    if TT_DATA_A == "int32" and TT_DATA_B == "int32":
        return 4
    if TT_DATA_A == "int32" and TT_DATA_B == "cint16":
        return 8
    if TT_DATA_A == "int32" and TT_DATA_B == "cint32":
        return 8
    if TT_DATA_A == "cint16" and TT_DATA_B == "int16":
        return 4
    if TT_DATA_A == "cint16" and TT_DATA_B == "int32":
        return 8
    if TT_DATA_A == "cint16" and TT_DATA_B == "cint16":
        return 4
    if TT_DATA_A == "cint16" and TT_DATA_B == "cint32":
        return 8
    if TT_DATA_A == "cint32" and TT_DATA_B == "int16":
        return 2
    if TT_DATA_A == "cint32" and TT_DATA_B == "int32":
        return 8
    if TT_DATA_A == "cint32" and TT_DATA_B == "cint16":
        return 8
    if TT_DATA_A == "cint32" and TT_DATA_B == "cint32":
        return 8
    if TT_DATA_A == "float" and TT_DATA_B == "float":
        return 4
    if TT_DATA_A == "cfloat" or TT_DATA_B == "cfloat":
        return 8


def calc_vecinframe(TP_API, TT_DATA_A, TT_DATA_B):
    if TP_API == API_BUFFER:
        if TT_DATA_A == "int16" and TT_DATA_B == "cint32":
            VEC_IN_FRAME = 16 / fn_det_calc_byte(TT_DATA_A, TT_DATA_B)
        elif TT_DATA_A == "cint32" and TT_DATA_B == "int16":
            VEC_IN_FRAME = 16 / fn_det_calc_byte(TT_DATA_A, TT_DATA_B)
        else:
            VEC_IN_FRAME = 32 / fn_det_calc_byte(TT_DATA_A, TT_DATA_B)
    elif TP_API == API_STREAM:
        VEC_IN_FRAME = 16 / fn_det_calc_byte(TT_DATA_A, TT_DATA_B)
    return VEC_IN_FRAME


def calc_padded_tp_dim(VEC_IN_FRAME, TP_DIM, TP_SSR):
    TP_DIM_perSSR = TP_DIM / TP_SSR
    TP_DIM_PADDED = (math.ceil(TP_DIM_perSSR / VEC_IN_FRAME)) * VEC_IN_FRAME
    return TP_DIM_PADDED


def calc_window_size(TP_DIM_PADDED, TP_NUM_FRAMES):
    TP_WINDOW_VSIZE = TP_NUM_FRAMES * TP_DIM_PADDED
    return TP_WINDOW_VSIZE


#### port ####
def get_port_info(portname, dir, dataType, windowVsize, apiType, vectorLength):
    windowSize = windowVsize * fn_size_by_byte(dataType)
    return [
        {
            "name": f"{portname}[{idx}]",
            "type": f"{apiType}",
            "direction": f"{dir}",
            "data_type": dataType,
            "fn_is_complex": fn_is_complex(dataType),
            "window_size": windowSize,  # com.fn_input_window_size(windowVsize, dataType),
            "margin_size": 0,
        }
        for idx in range(vectorLength)
    ]


def info_ports(args):
    """Standard function creating a static dictionary of information
    for upper software to correctly connect the IP.
    Some IP has a configurable number of ports according to parameter set,
    so port information has to be implemented as a function"""
    TT_DATA_A = args["TT_DATA_A"]
    TT_DATA_B = args["TT_DATA_B"]
    TP_API = args["TP_API"]
    TP_SSR = args["TP_SSR"]
    TP_NUM_FRAMES = args["TP_NUM_FRAMES"]
    TP_DIM = args["TP_DIM"]

    VEC_IN_FRAME = calc_vecinframe(TP_API, TT_DATA_A, TT_DATA_B)
    TP_DIM_PADDED = calc_padded_tp_dim(VEC_IN_FRAME, TP_DIM, TP_SSR)
    TP_WINDOW_VSIZE = calc_window_size(TP_DIM_PADDED, TP_NUM_FRAMES)

    if TP_API == API_BUFFER:
        portsInA = get_port_info(
            portname="inA",
            dir="in",
            dataType=TT_DATA_A,
            windowVsize=TP_WINDOW_VSIZE,
            apiType="window",
            vectorLength=TP_SSR,
        )
        portsInB = get_port_info(
            portname="inB",
            dir="in",
            dataType=TT_DATA_B,
            windowVsize=TP_WINDOW_VSIZE,
            apiType="window",
            vectorLength=TP_SSR,
        )
        portsOut = get_port_info(
            portname="out",
            dir="out",
            dataType=fn_det_out_type(TT_DATA_A, TT_DATA_B),
            windowVsize=TP_WINDOW_VSIZE,
            apiType="window",
            vectorLength=TP_SSR,
        )
    else:
        portsInA = get_port_info(
            portname="inA",
            dir="in",
            dataType=TT_DATA_A,
            windowVsize=TP_WINDOW_VSIZE,
            apiType="stream",
            vectorLength=TP_SSR,
        )
        portsInB = get_port_info(
            portname="inB",
            dir="in",
            dataType=TT_DATA_B,
            windowVsize=TP_WINDOW_VSIZE,
            apiType="stream",
            vectorLength=TP_SSR,
        )
        portsOut = get_port_info(
            portname="out",
            dir="out",
            dataType=fn_det_out_type(TT_DATA_A, TT_DATA_B),
            windowVsize=TP_WINDOW_VSIZE,
            apiType="stream",
            vectorLength=TP_SSR,
        )
    return portsInA + portsInB + portsOut


#### graph generator ####
def generate_graph(graphname, args):

    if graphname == "":
        graphname = "default_graphname"

    TT_DATA_A = args["TT_DATA_A"]
    TT_DATA_B = args["TT_DATA_B"]
    TP_DIM = args["TP_DIM"]
    TP_NUM_FRAMES = args["TP_NUM_FRAMES"]
    TP_SHIFT = args["TP_SHIFT"]
    TP_API = args["TP_API"]
    TP_SSR = args["TP_SSR"]

    if TP_API == API_STREAM:
        ssr = TP_SSR // 2
    else:
        ssr = TP_SSR

    # Use formatted multi-line string to avoid a lot of \n and \t
    code = f"""
class {graphname} : public adf::graph {{
public:
  static constexpr unsigned int TP_SSR = {TP_SSR};
  template <typename dir>
  using ssr_port_array = std::array<adf::port<dir>, TP_SSR>;

  ssr_port_array<input> inA;
  ssr_port_array<input> inB;
  ssr_port_array<output> out;

  xf::dsp::aie::hadamard::hadamard_graph<
    {TT_DATA_A}, //TT_DATA_A
    {TT_DATA_B}, //TT_DATA_B
    {TP_DIM}, //TP_DIM
    {TP_NUM_FRAMES}, //TP_NUM_FRAMES
    {TP_SHIFT}, //TP_SHIFT
    {TP_API}, //TP_API
    {ssr}, //TP_SSR
  > hadamard;

  {graphname}() : hadamard() {{
    adf::kernel *hadamard_kernels = hadamard.getKernels();

    for (int i=0; i < TP_SSR; i++) {{
      adf::connect<> net_in(inA[i], hadamard.inA[i]);
      adf::connect<> net_in(inB[i], hadamard.inB[i]);
      adf::connect<> net_out(hadamard.out[i], out[i]);
    }}
  }}

}};
"""
    out = {}
    out["graph"] = code
    out["port_info"] = info_ports(args)
    out["headerfile"] = "hadamard_graph.hpp"
    out["searchpaths"] = [
        "L2/include/aie",
        "L2/tests/aie/common/inc",
        "L1/include/aie",
        "L1/src/aie",
        "L1/tests/aie/inc",
        "L1/tests/aie/src",
    ]

    return out
