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
from aie_common import *
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

TP_SSR_max = 16
TP_NUM_FRAMES_max = 32

#######################################################
########## AIE_VARIANT Updater and Validator ##########
#######################################################
def update_AIE_VARIANT(args):
    return fn_update_AIE_VARIANT()

def fn_update_AIE_VARIANT():
    legal_set_AIE_VARIANT = [com.AIE, com.AIE_ML, com.AIE_MLv2]
    param_dict = {"name": "AIE_VARIANT", "enum": legal_set_AIE_VARIANT}
    return param_dict

def validate_AIE_VARIANT(args):
    AIE_VARIANT = args["AIE_VARIANT"]
    return fn_validate_aie_variant(AIE_VARIANT)

def fn_validate_aie_variant(AIE_VARIANT):
    param_dict = fn_update_AIE_VARIANT()
    return com.validate_legal_set(param_dict["enum"], "AIE_VARIANT", AIE_VARIANT)


#######################################################
########### TT_DATA_A Updater and Validator ###########
#######################################################
def update_TT_DATA_A(args):
    AIE_VARIANT = args["AIE_VARIANT"]
    return fn_update_TT_DATA_A(AIE_VARIANT)

def fn_update_TT_DATA_A(AIE_VARIANT):
    legal_set_TT_DATA_A = ["int16", "cint16", "int32", "cint32", "float", "cfloat"]
    param_dict = {"name": "TT_DATA_A", "enum": legal_set_TT_DATA_A}
    return param_dict

def validate_TT_DATA_A(args):
    AIE_VARIANT = args["AIE_VARIANT"]
    TT_DATA_A = args["TT_DATA_A"]
    return fn_validate_TT_DATA_A(AIE_VARIANT, TT_DATA_A)

def fn_validate_TT_DATA_A(AIE_VARIANT, TT_DATA_A):
    param_dict = fn_update_TT_DATA_A(AIE_VARIANT)
    return com.validate_legal_set(param_dict["enum"], "TT_DATA_A", TT_DATA_A)


#######################################################
########### TT_DATA_B Updater and Validator ###########
#######################################################
def update_TT_DATA_B(args):
    TT_DATA_A = args["TT_DATA_A"]
    return fn_update_TT_DATA_B(TT_DATA_A)

def fn_update_TT_DATA_B(TT_DATA_A):
    float_set = ["float", "cfloat"]
    int_set = ["int16", "cint16", "int32", "cint32"]
    if TT_DATA_A in int_set:
        legal_set_TT_DATA_B = int_set
    elif TT_DATA_A in float_set:
        legal_set_TT_DATA_B = float_set
    param_dict = {"name": "TT_DATA_B", "enum": legal_set_TT_DATA_B}
    return param_dict

def validate_TT_DATA_B(args):
    TT_DATA_A = args["TT_DATA_A"]
    TT_DATA_B = args["TT_DATA_B"]
    return fn_validate_TT_DATA_B(TT_DATA_A, TT_DATA_B)

def fn_validate_TT_DATA_B(TT_DATA_A, TT_DATA_B):
    param_dict = fn_update_TT_DATA_B(TT_DATA_A)
    return com.validate_legal_set(param_dict["enum"], "TT_DATA_B", TT_DATA_B)


#######################################################
######### TP_SSR Updater and Validator ###########
#######################################################
def update_TP_SSR(args):
    TP_SSR = args["TP_SSR"] if "TP_SSR" in args and args["TP_SSR"] else 0
    return fn_update_ssr(TP_SSR)

def fn_update_ssr(TP_SSR):
    param_dict = {
        "name": "TP_SSR",
        "minimum": 1,
        "maximum": TP_SSR_max
    }
    param_dict["actual"] = com.CLIP(TP_SSR, param_dict["minimum"], param_dict["maximum"])
    return param_dict

def validate_TP_SSR(args):
    TP_SSR = args["TP_SSR"]
    return fn_validate_ssr(TP_SSR)

def fn_validate_ssr(TP_SSR):
    param_dict = fn_update_ssr(TP_SSR)
    range_TP_DIM = [param_dict["minimum"], param_dict["maximum"]]
    return com.validate_range(range_TP_DIM, "TP_SSR", TP_SSR)


#######################################################
########### TP_NUM_FRAMES Updater and Validator #######
#######################################################
def update_TP_NUM_FRAMES(args):
    TP_NUM_FRAMES = args["TP_NUM_FRAMES"] if "TP_NUM_FRAMES" in args and args["TP_NUM_FRAMES"] else 0
    return fn_update_TP_NUM_FRAMES(TP_NUM_FRAMES)

def fn_update_TP_NUM_FRAMES(TP_NUM_FRAMES):
    param_dict = {
        "name": "TP_NUM_FRAMES",
        "minimum": 1,
        "maximum": TP_NUM_FRAMES_max
    }
    TP_NUM_FRAMES = com.CLIP(TP_NUM_FRAMES, param_dict["minimum"], param_dict["maximum"])
    return param_dict

def validate_TP_NUM_FRAMES(args):
    TP_NUM_FRAMES = args["TP_NUM_FRAMES"]
    return fn_validate_TP_NUM_FRAMES(TP_NUM_FRAMES)

def fn_validate_TP_NUM_FRAMES(TP_NUM_FRAMES):
    param_dict = fn_update_TP_NUM_FRAMES(TP_NUM_FRAMES)
    range_TP_NUM_FRAMES = [param_dict["minimum"], param_dict["maximum"]]
    return com.validate_range(range_TP_NUM_FRAMES, "TP_NUM_FRAMES", TP_NUM_FRAMES)


#######################################################
########### TP_DIM_A Updater and Validator ############
#######################################################
def update_TP_DIM_A(args):
    AIE_VARIANT = args["AIE_VARIANT"]
    TT_DATA_A = args["TT_DATA_A"]
    TT_DATA_B = args["TT_DATA_B"]
    TP_SSR = args["TP_SSR"]
    TP_NUM_FRAMES = args["TP_NUM_FRAMES"]
    TP_DIM_A = args["TP_DIM_A"] if "TP_DIM_A" in args and args["TP_DIM_A"] else 0
    return fn_update_TP_DIM_A(AIE_VARIANT, TT_DATA_A, TT_DATA_B, TP_SSR, TP_NUM_FRAMES, TP_DIM_A)

def fn_update_TP_DIM_A(AIE_VARIANT, TT_DATA_A, TT_DATA_B, TP_SSR, TP_NUM_FRAMES, TP_DIM_A):
    kVecSampleNumA = com.fnVecSampleNumMax(AIE_VARIANT, TT_DATA_A)
    io_bytes_max_per_frame = com.FLOOR(com.k_data_memory_bytes[AIE_VARIANT] // TP_NUM_FRAMES, kVecSampleNumA)
    out_type = fn_det_out_type(TT_DATA_A, TT_DATA_B)
    io_samples_max_per_frame = io_bytes_max_per_frame // com.sizeof(out_type)
    io_dim_min = TP_SSR * kVecSampleNumA
    io_dim_max = TP_SSR * io_samples_max_per_frame

    param_dict = {
        "name": "TP_DIM_A",
        "minimum": io_dim_min,
        "maximum": io_dim_max
    }
    TP_DIM_A = com.CLIP(TP_DIM_A, param_dict["minimum"], param_dict["maximum"])
    TP_DIM_A = com.ROUND_TO_NEAREST_MULTIPLE(TP_DIM_A, io_dim_min)
    param_dict.update({"actual": TP_DIM_A})
    return param_dict

def validate_TP_DIM_A(args):
    AIE_VARIANT = args["AIE_VARIANT"]
    TT_DATA_A = args["TT_DATA_A"]
    TT_DATA_B = args["TT_DATA_B"]
    TP_SSR = args["TP_SSR"]
    TP_NUM_FRAMES = args["TP_NUM_FRAMES"]
    TP_DIM_A = args["TP_DIM_A"]
    return fn_validate_TP_DIM_A(AIE_VARIANT, TT_DATA_A, TT_DATA_B, TP_SSR, TP_NUM_FRAMES, TP_DIM_A)

def fn_validate_TP_DIM_A(AIE_VARIANT, TT_DATA_A, TT_DATA_B, TP_SSR, TP_NUM_FRAMES, TP_DIM_A):
    kVecSampleNumA = com.fnVecSampleNumMax(AIE_VARIANT, TT_DATA_A)
    kKernelDim = TP_DIM_A // TP_SSR

    if TP_DIM_A % TP_SSR != 0:
        return com.isError(f"TP_DIM_A must be a multiple of TP_SSR.")
    elif kKernelDim % kVecSampleNumA != 0:
        return com.isError(f"TP_DIM_A / TP_SSR must be a multiple of vecSampleNumA.")
    else:
        param_dict = fn_update_TP_DIM_A(AIE_VARIANT, TT_DATA_A, TT_DATA_B, TP_SSR, TP_NUM_FRAMES, TP_DIM_A)
        range_TP_DIM_A = [param_dict["minimum"], param_dict["maximum"]]
        return com.validate_range(range_TP_DIM_A, "TP_DIM_A", TP_DIM_A)


#######################################################
########### TP_DIM_B Updater and Validator ############
#######################################################
def update_TP_DIM_B(args):
    AIE_VARIANT = args["AIE_VARIANT"]
    TT_DATA_A = args["TT_DATA_A"]
    TT_DATA_B = args["TT_DATA_B"]
    TP_NUM_FRAMES = args["TP_NUM_FRAMES"]
    TP_DIM_B = args["TP_DIM_B"] if "TP_DIM_B" in args and args["TP_DIM_B"] else 0
    return fn_update_TP_DIM_B(AIE_VARIANT, TT_DATA_A, TT_DATA_B, TP_NUM_FRAMES, TP_DIM_B)

def fn_update_TP_DIM_B(AIE_VARIANT, TT_DATA_A, TT_DATA_B, TP_NUM_FRAMES, TP_DIM_B):
    kVecSampleNumB = com.fnVecSampleNumMax(AIE_VARIANT, TT_DATA_B)
    io_bytes_max_per_frame = com.FLOOR(com.k_data_memory_bytes[AIE_VARIANT] // TP_NUM_FRAMES, kVecSampleNumB)
    out_type = fn_det_out_type(TT_DATA_A, TT_DATA_B)
    io_samples_max_per_frame = io_bytes_max_per_frame // com.sizeof(out_type)

    param_dict = {
        "name": "TP_DIM_B",
        "minimum": kVecSampleNumB,
        "maximum": io_samples_max_per_frame
    }
    TP_DIM_B = com.CLIP(TP_DIM_B, param_dict["minimum"], param_dict["maximum"])
    TP_DIM_B = com.ROUND_TO_NEAREST_MULTIPLE(TP_DIM_B, kVecSampleNumB)
    param_dict.update({"actual": TP_DIM_B})
    return param_dict

def validate_TP_DIM_B(args):
    AIE_VARIANT = args["AIE_VARIANT"]
    TT_DATA_A = args["TT_DATA_A"]
    TT_DATA_B = args["TT_DATA_B"]
    TP_NUM_FRAMES = args["TP_NUM_FRAMES"]
    TP_DIM_B = args["TP_DIM_B"]
    return fn_validate_TP_DIM_B(AIE_VARIANT, TT_DATA_A, TT_DATA_B, TP_NUM_FRAMES, TP_DIM_B)

def fn_validate_TP_DIM_B(AIE_VARIANT, TT_DATA_A, TT_DATA_B, TP_NUM_FRAMES, TP_DIM_B):
    kVecSampleNumB = com.fnVecSampleNumMax(AIE_VARIANT, TT_DATA_B)

    if TP_DIM_B % kVecSampleNumB != 0:
        return com.isError(f"TP_DIM_B must be a multiple of vecSampleNumB.")
    else:
        param_dict = fn_update_TP_DIM_B(AIE_VARIANT, TT_DATA_A, TT_DATA_B, TP_NUM_FRAMES, TP_DIM_B)
        range_TP_DIM_B = [param_dict["minimum"], param_dict["maximum"]]
        return com.validate_range(range_TP_DIM_B, "TP_DIM_B", TP_DIM_B)


#######################################################
########### TP_API Updater and Validator ##############
#######################################################
def update_TP_API(args):
    AIE_VARIANT = args["AIE_VARIANT"]
    TT_DATA_A = args["TT_DATA_A"]
    TT_DATA_B = args["TT_DATA_B"]
    TP_DIM_A = args["TP_DIM_A"]
    TP_DIM_B = args["TP_DIM_B"]
    TP_NUM_FRAMES = args["TP_NUM_FRAMES"]
    return fn_update_TP_API(AIE_VARIANT, TT_DATA_A, TT_DATA_B, TP_DIM_A, TP_DIM_B, TP_NUM_FRAMES)

def fn_update_TP_API(AIE_VARIANT, TT_DATA_A, TT_DATA_B, TP_DIM_A, TP_DIM_B, TP_NUM_FRAMES):
    io_bytes_max_per_tile = com.k_data_memory_bytes[AIE_VARIANT]
    out_type = fn_det_out_type(TT_DATA_A, TT_DATA_B)
    out_window_bytes = TP_NUM_FRAMES * TP_DIM_A * TP_DIM_B * com.sizeof(out_type)

    legal_set_TP_API = [com.API_BUFFER, com.API_STREAM]
    if out_window_bytes > io_bytes_max_per_tile:
        legal_set_TP_API.remove(com.API_BUFFER)

    param_dict = {"name": "TP_API", "enum": legal_set_TP_API}
    return param_dict

def validate_TP_API(args):
    AIE_VARIANT = args["AIE_VARIANT"]
    TT_DATA_A = args["TT_DATA_A"]
    TT_DATA_B = args["TT_DATA_B"]
    TP_DIM_A = args["TP_DIM_A"]
    TP_DIM_B = args["TP_DIM_B"]
    TP_NUM_FRAMES = args["TP_NUM_FRAMES"]
    TP_API = args["TP_API"]
    return fn_validate_api_val(AIE_VARIANT, TT_DATA_A, TT_DATA_B, TP_DIM_A, TP_DIM_B, TP_NUM_FRAMES, TP_API)

def fn_validate_api_val(AIE_VARIANT, TT_DATA_A, TT_DATA_B, TP_DIM_A, TP_DIM_B, TP_NUM_FRAMES, TP_API):
    param_dict = fn_update_TP_API(AIE_VARIANT, TT_DATA_A, TT_DATA_B, TP_DIM_A, TP_DIM_B, TP_NUM_FRAMES)
    return com.validate_legal_set(param_dict["enum"], "TP_API", TP_API)


#######################################################
########### TP_SHIFT Updater and Validator ############
#######################################################
def update_TP_SHIFT(args):
    TT_DATA = args["TT_DATA_A"]
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
    TT_DATA = args["TT_DATA_A"]
    TP_SHIFT = args["TP_SHIFT"]
    AIE_VARIANT = args["AIE_VARIANT"]
    return fn_validate_TP_SHIFT(AIE_VARIANT, TT_DATA, TP_SHIFT)

def fn_validate_TP_SHIFT(AIE_VARIANT, TT_DATA, TP_SHIFT):
    param_dict = fn_update_TP_SHIFT(AIE_VARIANT, TT_DATA)
    range_TP_SHIFT = [param_dict["minimum"], param_dict["maximum"]]
    return com.validate_range(range_TP_SHIFT, "TP_SHIFT", TP_SHIFT)


#######################################################
########### TP_RND Updater and Validator ##############
#######################################################
def update_TP_RND(args):
    AIE_VARIANT = args["AIE_VARIANT"]
    return fn_update_tp_rnd(AIE_VARIANT)

def fn_update_tp_rnd(AIE_VARIANT):
    legal_set_TP_RND = com.fn_get_legalSet_roundMode(AIE_VARIANT)
    param_dict = {}
    param_dict.update({"name": "TP_RND"})
    param_dict.update({"enum": legal_set_TP_RND})
    return param_dict

def validate_TP_RND(args):
    AIE_VARIANT = args["AIE_VARIANT"]
    TP_RND = args["TP_RND"]
    return fn_validate_TP_RND(AIE_VARIANT, TP_RND)

def fn_validate_TP_RND(AIE_VARIANT, TP_RND):
    legal_set_TP_RND = com.fn_get_legalSet_roundMode(AIE_VARIANT)
    return com.validate_legal_set(legal_set_TP_RND, "TP_RND", TP_RND)


#######################################################
########### TP_RND Updater and Validator ##############
#######################################################
def update_TP_SAT(args):
    return fn_update_tp_sat()

def fn_update_tp_sat():
    legal_set = com.fn_legal_set_sat()
    param_dict = {}
    param_dict.update({"name": "TP_SAT"})
    param_dict.update({"enum": legal_set})
    return param_dict

def validate_TP_SAT(args):
    TP_SAT = args["TP_SAT"]
    return fn_validate_TP_SAT(TP_SAT)

def fn_validate_TP_SAT(TP_SAT):
    param_dict = fn_update_tp_sat()
    legal_set_TP_SAT = param_dict["enum"]
    return com.validate_legal_set(legal_set_TP_SAT, "TP_SAT", TP_SAT)


## utility functions
def fn_det_out_type(TT_DATA_A, TT_DATA_B):
    if TT_DATA_A == "int16" and TT_DATA_B == "int16":
        return "int16"
    if TT_DATA_A == "int16" and TT_DATA_B == "int32":
        return "int32"
    if TT_DATA_A == "int16" and TT_DATA_B == "cint16":
        return "cint16"
    if TT_DATA_A == "int16" and TT_DATA_B == "cint32":
        return "cint32"
    if TT_DATA_A == "int32" and TT_DATA_B == "int16":
        return "int32"
    if TT_DATA_A == "int32" and TT_DATA_B == "int32":
        return "int32"
    if TT_DATA_A == "int32" and TT_DATA_B == "cint16":
        return "cint32"
    if TT_DATA_A == "int32" and TT_DATA_B == "cint32":
        return "cint32"
    if TT_DATA_A == "cint16" and TT_DATA_B == "int16":
        return "cint16"
    if TT_DATA_A == "cint16" and TT_DATA_B == "int32":
        return "cint32"
    if TT_DATA_A == "cint16" and TT_DATA_B == "cint16":
        return "cint16"
    if TT_DATA_A == "cint16" and TT_DATA_B == "cint32":
        return "cint32"
    if TT_DATA_A == "cint32" and TT_DATA_B == "int16":
        return "cint32"
    if TT_DATA_A == "cint32" and TT_DATA_B == "int32":
        return "cint32"
    if TT_DATA_A == "cint32" and TT_DATA_B == "cint16":
        return "cint32"
    if TT_DATA_A == "cint32" and TT_DATA_B == "cint32":
        return "cint32"
    if TT_DATA_A == "float" and TT_DATA_B == "float":
        return "float"
    if TT_DATA_A == "float" and TT_DATA_B == "cfloat":
        return "cfloat"
    if TT_DATA_A == "cfloat" and TT_DATA_B == "float":
        return "cfloat"
    if TT_DATA_A == "cfloat" or TT_DATA_B == "cfloat":
        return "cfloat"


#### port ####
def get_port_info(portname, dir, dataType, dim, numFrames, apiType, vectorLength):
    windowSize = dim * fn_size_by_byte(dataType)
    return [
        {
            "name": f"{portname}[{idx}]",
            "type": f"{apiType}",
            "direction": f"{dir}",
            "data_type": dataType,
            "fn_is_complex": com.fn_is_complex(dataType),
            "window_size": windowSize
            * numFrames,  # com.fn_input_window_size(windowVsize, dataType),
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
    TP_DIM_A = args["TP_DIM_A"]
    TP_DIM_B = args["TP_DIM_B"]
    TP_API = args["TP_API"]
    TP_SSR = args["TP_SSR"]
    TP_NUM_FRAMES = args["TP_NUM_FRAMES"]
    if TP_API == com.API_BUFFER:
        portsInA = get_port_info(
            portname="inA",
            dir="in",
            dataType=TT_DATA_A,
            dim=TP_DIM_A / TP_SSR,
            numFrames=TP_NUM_FRAMES,
            apiType="window",
            vectorLength=TP_SSR,
        )
        portsInB = get_port_info(
            portname="inB",
            dir="in",
            dataType=TT_DATA_B,
            dim=TP_DIM_B,
            numFrames=TP_NUM_FRAMES,
            apiType="window",
            vectorLength=TP_SSR,
        )
        portsOut = get_port_info(
            portname="out",
            dir="out",
            dataType=fn_det_out_type(TT_DATA_A, TT_DATA_B),
            dim=TP_DIM_A * TP_DIM_B / TP_SSR,
            numFrames=TP_NUM_FRAMES,
            apiType="window",
            vectorLength=TP_SSR,
        )
    else:
        portsInA = get_port_info(
            portname="inA",
            dir="in",
            dataType=TT_DATA_A,
            dim=TP_DIM_A / TP_SSR,
            numFrames=TP_NUM_FRAMES,
            apiType="stream",
            vectorLength=TP_SSR,
        )
        portsInB = get_port_info(
            portname="inB",
            dir="in",
            dataType=TT_DATA_B,
            dim=TP_DIM_B,
            numFrames=TP_NUM_FRAMES,
            apiType="stream",
            vectorLength=TP_SSR,
        )
        portsOut = get_port_info(
            portname="out",
            dir="out",
            dataType=fn_det_out_type(TT_DATA_A, TT_DATA_B),
            dim=TP_DIM_A * TP_DIM_B / TP_SSR,
            numFrames=TP_NUM_FRAMES,
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
    TP_DIM_A = args["TP_DIM_A"]
    TP_DIM_B = args["TP_DIM_B"]
    TP_NUM_FRAMES = args["TP_NUM_FRAMES"]
    TP_SHIFT = args["TP_SHIFT"]
    TP_API = args["TP_API"]
    TP_SSR = args["TP_SSR"]

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

  xf::dsp::aie::outer_tensor::outer_tensor_graph<
    {TT_DATA_A}, //TT_DATA_A
    {TT_DATA_B}, //TT_DATA_B
    {TP_DIM_A}, //TP_DIM_A
    {TP_DIM_B}, //TP_DIM_B
    {TP_NUM_FRAMES}, //TP_NUM_FRAMES
    {TP_SHIFT}, //TP_SHIFT
    {TP_API}, //TP_API
    {TP_SSR} //TP_SSR
  > outer_tensor;

  {graphname}() : outer_tensor() {{
    adf::kernel *outer_tensor_kernels = outer_tensor.getKernels();

    for (int i=0; i < TP_SSR; i++) {{
      adf::connect<> net_inA(inA[i], outer_tensor.inA[i]);
      adf::connect<> net_inB(inB[i], outer_tensor.inB[i]);
      adf::connect<> net_out(outer_tensor.out[i], out[i]);
    }}
  }}

}};
"""
    out = {}
    out["graph"] = code
    out["port_info"] = info_ports(args)
    out["headerfile"] = "outer_tensor_graph.hpp"
    out["searchpaths"] = [
        "L2/include/aie",
        "L2/tests/aie/common/inc",
        "L1/include/aie",
        "L1/src/aie",
        "L1/tests/aie/inc",
        "L1/tests/aie/src",
    ]

    return out
