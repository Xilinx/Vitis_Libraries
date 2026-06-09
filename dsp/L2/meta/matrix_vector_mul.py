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

import aie_common as com
from aie_common import *

TP_CASC_LEN_min = 1
TP_CASC_LEN_max = 16
TP_SSR_min = 1
TP_SSR_max = 16


#######################################################
################## Helper Functions ###################
#######################################################
def getOutputType(typeA, typeB):
    if fn_size_by_byte(typeA) > fn_size_by_byte(typeB):
        return typeA
    else:
        return typeB


def get_dim_min(TT_DATA):
    """Minimum dimension granularity: 256 bits / type size in elements."""
    return 256 // 8 // fn_size_by_byte(TT_DATA)


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
    legal_set = param_dict["enum"]
    return validate_legal_set(legal_set, "AIE_VARIANT", AIE_VARIANT)


#######################################################
########### TT_DATA_A Updater and Validator ###########
#######################################################
def update_TT_DATA_A(args):
    return fn_update_TT_DATA_A()


def fn_update_TT_DATA_A():
    legal_set_TT_DATA_A = ["int16", "int32", "cint16", "cint32", "float", "cfloat"]
    param_dict = {}
    param_dict.update({"name": "TT_DATA_A"})
    param_dict.update({"enum": legal_set_TT_DATA_A})
    return param_dict


def validate_TT_DATA_A(args):
    TT_DATA_A = args["TT_DATA_A"]
    return fn_validate_TT_DATA_A(TT_DATA_A)


def fn_validate_TT_DATA_A(TT_DATA_A):
    param_dict = fn_update_TT_DATA_A()
    legal_set = param_dict["enum"]
    return validate_legal_set(legal_set, "TT_DATA_A", TT_DATA_A)


#######################################################
########### TT_DATA_B Updater and Validator ###########
#######################################################
def update_TT_DATA_B(args):
    TT_DATA_A = args["TT_DATA_A"]
    TT_DATA_B = args["TT_DATA_B"] if "TT_DATA_B" in args and args["TT_DATA_B"] else None
    return fn_update_TT_DATA_B(TT_DATA_A, TT_DATA_B)


def fn_update_TT_DATA_B(TT_DATA_A, TT_DATA_B):
    int_set = ["int16", "int32", "cint16", "cint32"]
    float_set = ["float", "cfloat"]

    if TT_DATA_A in int_set:
        legal_set_TT_DATA_B = int_set
    elif TT_DATA_A in float_set:
        legal_set_TT_DATA_B = float_set
    else:
        legal_set_TT_DATA_B = []

    param_dict = {}
    param_dict.update({"name": "TT_DATA_B"})
    param_dict.update({"enum": legal_set_TT_DATA_B})
    return param_dict


def validate_TT_DATA_B(args):
    TT_DATA_B = args["TT_DATA_B"]
    TT_DATA_A = args["TT_DATA_A"]
    return fn_validate_TT_DATA_B(TT_DATA_A, TT_DATA_B)


def fn_validate_TT_DATA_B(TT_DATA_A, TT_DATA_B):
    param_dict = fn_update_TT_DATA_B(TT_DATA_A, None)
    legal_set = param_dict["enum"]
    return validate_legal_set(legal_set, "TT_DATA_B", TT_DATA_B)


#######################################################
######## TP_USE_MATRIX_RELOAD Updater and Validator ####
#######################################################
def update_TP_USE_MATRIX_RELOAD(args):
    TP_USE_MATRIX_RELOAD = args["TP_USE_MATRIX_RELOAD"] if "TP_USE_MATRIX_RELOAD" in args and args["TP_USE_MATRIX_RELOAD"] is not None else None
    return fn_update_TP_USE_MATRIX_RELOAD(TP_USE_MATRIX_RELOAD)


def fn_update_TP_USE_MATRIX_RELOAD(TP_USE_MATRIX_RELOAD):
    legal_set_TP_USE_MATRIX_RELOAD = [0, 1]
    param_dict = {}
    param_dict.update({"name": "TP_USE_MATRIX_RELOAD"})
    param_dict.update({"enum": legal_set_TP_USE_MATRIX_RELOAD})
    return param_dict


def validate_TP_USE_MATRIX_RELOAD(args):
    TP_USE_MATRIX_RELOAD = args["TP_USE_MATRIX_RELOAD"]
    return fn_validate_TP_USE_MATRIX_RELOAD(TP_USE_MATRIX_RELOAD)


def fn_validate_TP_USE_MATRIX_RELOAD(TP_USE_MATRIX_RELOAD):
    param_dict = fn_update_TP_USE_MATRIX_RELOAD(None)
    legal_set = param_dict["enum"]
    return validate_legal_set(legal_set, "TP_USE_MATRIX_RELOAD", TP_USE_MATRIX_RELOAD)


#######################################################
########### TP_API Updater and Validator ##############
#######################################################
def update_TP_API(args):
    TP_USE_MATRIX_RELOAD = args["TP_USE_MATRIX_RELOAD"]
    TP_API = args["TP_API"] if "TP_API" in args and args["TP_API"] is not None else None
    return fn_update_TP_API(TP_USE_MATRIX_RELOAD, TP_API)


def fn_update_TP_API(TP_USE_MATRIX_RELOAD, TP_API):
    legal_set_TP_API = [0, 1]
    if TP_USE_MATRIX_RELOAD == 0:
        legal_set_TP_API.remove(1)
    param_dict = {}
    param_dict.update({"name": "TP_API"})
    param_dict.update({"enum": legal_set_TP_API})
    return param_dict


def validate_TP_API(args):
    TP_API = args["TP_API"]
    TP_USE_MATRIX_RELOAD = args["TP_USE_MATRIX_RELOAD"]
    return fn_validate_TP_API(TP_API, TP_USE_MATRIX_RELOAD)


def fn_validate_TP_API(TP_API, TP_USE_MATRIX_RELOAD):
    param_dict = fn_update_TP_API(TP_USE_MATRIX_RELOAD, None)
    legal_set = param_dict["enum"]
    return validate_legal_set(legal_set, "TP_API", TP_API)


#######################################################
############ TP_DIM_A Updater and Validator ###########
#######################################################
def update_TP_DIM_A(args):
    AIE_VARIANT = args["AIE_VARIANT"]
    TT_DATA_A = args["TT_DATA_A"]
    TT_DATA_B = args["TT_DATA_B"]
    TP_DIM_A = args["TP_DIM_A"] if "TP_DIM_A" in args and args["TP_DIM_A"] else 0
    return fn_update_tp_dim_a(AIE_VARIANT, TT_DATA_A, TT_DATA_B, TP_DIM_A)


def fn_update_tp_dim_a(AIE_VARIANT, TT_DATA_A, TT_DATA_B, TP_DIM_A):
    dimA_min = get_dim_min(TT_DATA_A)
    dimB_min = get_dim_min(TT_DATA_B)
    TT_OUT = getOutputType(TT_DATA_A, TT_DATA_B)
    buffer_size = k_data_memory_bytes[AIE_VARIANT]
    max_buf_a = buffer_size // fn_size_by_byte(TT_DATA_A)
    max_buf_out = buffer_size // fn_size_by_byte(TT_OUT)

    # Optimistic: assume max SSR and max cascade for range computation
    dim_a_max_buf = (max_buf_a * TP_SSR_max * TP_CASC_LEN_max) // dimB_min
    dim_a_max_out = max_buf_out * TP_SSR_max
    TP_DIM_A_max = int(FLOOR(min(dim_a_max_buf, dim_a_max_out), dimA_min))

    param_dict = {
        "name": "TP_DIM_A",
        "minimum": dimA_min,
        "maximum": TP_DIM_A_max,
    }

    TP_DIM_A = com.CLIP(TP_DIM_A, param_dict["minimum"], param_dict["maximum"])
    TP_DIM_A = com.ROUND_TO_NEAREST_MULTIPLE(TP_DIM_A, dimA_min)
    param_dict["actual"] = TP_DIM_A

    return param_dict


def validate_TP_DIM_A(args):
    TP_DIM_A = args["TP_DIM_A"]
    AIE_VARIANT = args["AIE_VARIANT"]
    TT_DATA_A = args["TT_DATA_A"]
    TT_DATA_B = args["TT_DATA_B"]
    return fn_validate_tp_dim_a(AIE_VARIANT, TT_DATA_A, TT_DATA_B, TP_DIM_A)


def fn_validate_tp_dim_a(AIE_VARIANT, TT_DATA_A, TT_DATA_B, TP_DIM_A):
    param_dict = fn_update_tp_dim_a(AIE_VARIANT, TT_DATA_A, TT_DATA_B, TP_DIM_A)
    dimA_min = param_dict["minimum"]

    if TP_DIM_A % dimA_min != 0:
        return isError(f"TP_DIM_A ({TP_DIM_A}) must be a multiple of {dimA_min}!")

    return validate_range([param_dict["minimum"], param_dict["maximum"]], "TP_DIM_A", TP_DIM_A)


#######################################################
########### TP_DIM_B Updater and Validator ############
#######################################################
def update_TP_DIM_B(args):
    AIE_VARIANT = args["AIE_VARIANT"]
    TT_DATA_A = args["TT_DATA_A"]
    TT_DATA_B = args["TT_DATA_B"]
    TP_DIM_A = args["TP_DIM_A"]
    TP_API = args["TP_API"]
    TP_DIM_B = args["TP_DIM_B"] if "TP_DIM_B" in args and args["TP_DIM_B"] else 0
    return fn_update_tp_dim_b(AIE_VARIANT, TT_DATA_A, TT_DATA_B, TP_DIM_A, TP_API, TP_DIM_B)


def fn_update_tp_dim_b(AIE_VARIANT, TT_DATA_A, TT_DATA_B, TP_DIM_A, TP_API, TP_DIM_B):
    dimB_min = get_dim_min(TT_DATA_B)
    buffer_size = k_data_memory_bytes[AIE_VARIANT]
    max_buf_a = buffer_size // fn_size_by_byte(TT_DATA_A)
    max_buf_b = buffer_size // fn_size_by_byte(TT_DATA_B)

    # Optimistic: assume max SSR and max cascade for range computation
    dim_b_max_a = (max_buf_a * TP_SSR_max * TP_CASC_LEN_max) // TP_DIM_A

    if TP_API == 1:
        dim_b_max_b = TP_CASC_LEN_max * (1024 // 8 // fn_size_by_byte(TT_DATA_B))
    else:
        dim_b_max_b = max_buf_b * TP_CASC_LEN_max

    TP_DIM_B_max = int(FLOOR(min(dim_b_max_a, dim_b_max_b), dimB_min))

    param_dict = {
        "name": "TP_DIM_B",
        "minimum": dimB_min,
        "maximum": TP_DIM_B_max,
    }

    TP_DIM_B = com.CLIP(TP_DIM_B, param_dict["minimum"], param_dict["maximum"])
    TP_DIM_B = com.ROUND_TO_NEAREST_MULTIPLE(TP_DIM_B, dimB_min)
    param_dict["actual"] = TP_DIM_B

    return param_dict


def validate_TP_DIM_B(args):
    TP_DIM_B = args["TP_DIM_B"]
    AIE_VARIANT = args["AIE_VARIANT"]
    TT_DATA_A = args["TT_DATA_A"]
    TT_DATA_B = args["TT_DATA_B"]
    TP_DIM_A = args["TP_DIM_A"]
    TP_API = args["TP_API"]
    return fn_validate_tp_dim_b(AIE_VARIANT, TT_DATA_A, TT_DATA_B, TP_DIM_A, TP_API, TP_DIM_B)


def fn_validate_tp_dim_b(AIE_VARIANT, TT_DATA_A, TT_DATA_B, TP_DIM_A, TP_API, TP_DIM_B):
    param_dict = fn_update_tp_dim_b(AIE_VARIANT, TT_DATA_A, TT_DATA_B, TP_DIM_A, TP_API, TP_DIM_B)
    dimB_min = param_dict["minimum"]

    if TP_DIM_B % dimB_min != 0:
        return isError(f"TP_DIM_B ({TP_DIM_B}) must be a multiple of {dimB_min}!")

    return validate_range([param_dict["minimum"], param_dict["maximum"]], "TP_DIM_B", TP_DIM_B)


#######################################################
######## TP_NUM_FRAMES Updater and Validator ##########
#######################################################
def update_TP_NUM_FRAMES(args):
    AIE_VARIANT = args["AIE_VARIANT"]
    TT_DATA_A = args["TT_DATA_A"]
    TT_DATA_B = args["TT_DATA_B"]
    TP_DIM_A = args["TP_DIM_A"]
    TP_DIM_B = args["TP_DIM_B"]
    TP_API = args["TP_API"]
    TP_NUM_FRAMES = args["TP_NUM_FRAMES"] if "TP_NUM_FRAMES" in args and args["TP_NUM_FRAMES"] else 0
    return fn_update_tp_num_frames(AIE_VARIANT, TT_DATA_A, TT_DATA_B, TP_DIM_A, TP_DIM_B, TP_API, TP_NUM_FRAMES)


def fn_update_tp_num_frames(AIE_VARIANT, TT_DATA_A, TT_DATA_B, TP_DIM_A, TP_DIM_B, TP_API, TP_NUM_FRAMES):
    dimA_min = get_dim_min(TT_DATA_A)
    dimB_min = get_dim_min(TT_DATA_B)
    buffer_size = k_data_memory_bytes[AIE_VARIANT]
    max_buf_a = buffer_size // fn_size_by_byte(TT_DATA_A)
    max_buf_b = buffer_size // fn_size_by_byte(TT_DATA_B)

    # Use actual achievable SSR and cascade for this TP_DIM_A/TP_DIM_B
    maxSsr = min(TP_DIM_A // dimA_min, TP_SSR_max)
    maxCasc = min(TP_DIM_B // dimB_min, TP_CASC_LEN_max)

    num_frames_max_a = (max_buf_a * maxSsr * maxCasc) // (TP_DIM_A * TP_DIM_B)
    if TP_API:
        num_frames_max_b = (1024 // 8 // fn_size_by_byte(TT_DATA_B) * maxCasc) // TP_DIM_B
    else:
        num_frames_max_b = (max_buf_b * maxCasc) // TP_DIM_B

    TP_NUM_FRAMES_max = max(1, min(num_frames_max_a, num_frames_max_b))

    param_dict = {
        "name": "TP_NUM_FRAMES",
        "minimum": 1,
        "maximum": int(TP_NUM_FRAMES_max),
    }

    TP_NUM_FRAMES = com.CLIP(TP_NUM_FRAMES, param_dict["minimum"], param_dict["maximum"])
    param_dict["actual"] = int(TP_NUM_FRAMES)
    return param_dict


def validate_TP_NUM_FRAMES(args):
    TP_NUM_FRAMES = args["TP_NUM_FRAMES"]
    AIE_VARIANT = args["AIE_VARIANT"]
    TT_DATA_A = args["TT_DATA_A"]
    TT_DATA_B = args["TT_DATA_B"]
    TP_DIM_A = args["TP_DIM_A"]
    TP_DIM_B = args["TP_DIM_B"]
    TP_API = args["TP_API"]
    return fn_validate_numFrames(AIE_VARIANT, TT_DATA_A, TT_DATA_B, TP_DIM_A, TP_DIM_B, TP_NUM_FRAMES, TP_API)


def fn_validate_numFrames(AIE_VARIANT, TT_DATA_A, TT_DATA_B, TP_DIM_A, TP_DIM_B, TP_NUM_FRAMES, TP_API):
    param_dict = fn_update_tp_num_frames(AIE_VARIANT, TT_DATA_A, TT_DATA_B, TP_DIM_A, TP_DIM_B, TP_API, TP_NUM_FRAMES)
    return validate_range([param_dict["minimum"], param_dict["maximum"]], "TP_NUM_FRAMES", TP_NUM_FRAMES)


#######################################################
############ TP_SSR Updater and Validator #############
#######################################################
def update_TP_SSR(args):
    AIE_VARIANT = args["AIE_VARIANT"]
    TT_DATA_A = args["TT_DATA_A"]
    TT_DATA_B = args["TT_DATA_B"]
    TP_DIM_A = args["TP_DIM_A"]
    TP_DIM_B = args["TP_DIM_B"]
    TP_NUM_FRAMES = args["TP_NUM_FRAMES"]
    TP_SSR = args["TP_SSR"] if "TP_SSR" in args and args["TP_SSR"] else 0
    return fn_update_TP_SSR(AIE_VARIANT, TT_DATA_A, TT_DATA_B, TP_DIM_A, TP_DIM_B, TP_NUM_FRAMES, TP_SSR)


def fn_update_TP_SSR(AIE_VARIANT, TT_DATA_A, TT_DATA_B, TP_DIM_A, TP_DIM_B, TP_NUM_FRAMES, TP_SSR):
    dimA_min = get_dim_min(TT_DATA_A)
    dimB_min = get_dim_min(TT_DATA_B)
    buffer_size = k_data_memory_bytes[AIE_VARIANT]

    max_casc_for_dim_b = min(TP_CASC_LEN_max, TP_DIM_B // dimB_min)
    ssrMinForA = max(1, (fn_size_by_byte(TT_DATA_A) * TP_DIM_A * TP_DIM_B * TP_NUM_FRAMES + buffer_size * max_casc_for_dim_b - 1) // (buffer_size * max_casc_for_dim_b))
    maxSsr = min(TP_SSR_max, TP_DIM_A // dimA_min)

    legal_set = [k for k in find_divisors(TP_DIM_A, maxSsr)
                 if k >= ssrMinForA
                 and (TP_DIM_A // k) % dimA_min == 0]

    if not legal_set:
        return {}
    param_dict = {"name": "TP_SSR", "enum": legal_set}
    if TP_SSR and TP_SSR not in legal_set:
        param_dict["actual"] = com.GET_CLOSEST(TP_SSR, legal_set)
    return param_dict


def validate_TP_SSR(args):
    TP_SSR = args["TP_SSR"]
    AIE_VARIANT = args["AIE_VARIANT"]
    TT_DATA_A = args["TT_DATA_A"]
    TT_DATA_B = args["TT_DATA_B"]
    TP_DIM_A = args["TP_DIM_A"]
    TP_DIM_B = args["TP_DIM_B"]
    TP_NUM_FRAMES = args["TP_NUM_FRAMES"]
    return fn_validate_TP_SSR(AIE_VARIANT, TT_DATA_A, TT_DATA_B, TP_DIM_A, TP_DIM_B, TP_NUM_FRAMES, TP_SSR)


def fn_validate_TP_SSR(AIE_VARIANT, TT_DATA_A, TT_DATA_B, TP_DIM_A, TP_DIM_B, TP_NUM_FRAMES, TP_SSR):
    param_dict = fn_update_TP_SSR(AIE_VARIANT, TT_DATA_A, TT_DATA_B, TP_DIM_A, TP_DIM_B, TP_NUM_FRAMES, TP_SSR)
    legal_set = param_dict["enum"] if param_dict else []
    return validate_legal_set(legal_set, "TP_SSR", TP_SSR)


#######################################################
############ TP_CASC_LEN Updater and Validator ########
#######################################################
def update_TP_CASC_LEN(args):
    AIE_VARIANT = args["AIE_VARIANT"]
    TT_DATA_A = args["TT_DATA_A"]
    TT_DATA_B = args["TT_DATA_B"]
    TP_DIM_A = args["TP_DIM_A"]
    TP_DIM_B = args["TP_DIM_B"]
    TP_SSR = args["TP_SSR"]
    TP_NUM_FRAMES = args["TP_NUM_FRAMES"]
    TP_API = args["TP_API"]
    TP_CASC_LEN = args["TP_CASC_LEN"] if "TP_CASC_LEN" in args and args["TP_CASC_LEN"] else 0
    return fn_update_TP_CASC_LEN(AIE_VARIANT, TT_DATA_A, TT_DATA_B, TP_DIM_A, TP_DIM_B, TP_SSR, TP_NUM_FRAMES, TP_API, TP_CASC_LEN)


def fn_update_TP_CASC_LEN(AIE_VARIANT, TT_DATA_A, TT_DATA_B, TP_DIM_A, TP_DIM_B, TP_SSR, TP_NUM_FRAMES, TP_API, TP_CASC_LEN):
    dimB_min = get_dim_min(TT_DATA_B)
    buffer_size = k_data_memory_bytes[AIE_VARIANT]

    cascMinForA = max(1, (fn_size_by_byte(TT_DATA_A) * TP_DIM_A * TP_DIM_B * TP_NUM_FRAMES + buffer_size * TP_SSR - 1) // (buffer_size * TP_SSR))
    maxCasc = min(TP_CASC_LEN_max, TP_DIM_B // dimB_min)

    legal_set = [k for k in find_divisors(TP_DIM_B, maxCasc)
                 if k >= cascMinForA
                 and (TP_DIM_B // k) % dimB_min == 0]

    if TP_API:
        stream_max_samples = 1024 // 8 // fn_size_by_byte(TT_DATA_B)
        legal_set = [k for k in legal_set
                     if (TP_DIM_B * TP_NUM_FRAMES) // k <= stream_max_samples]

    if not legal_set:
        return {}
    param_dict = {"name": "TP_CASC_LEN", "enum": legal_set}
    if TP_CASC_LEN and TP_CASC_LEN not in legal_set:
        param_dict["actual"] = com.GET_CLOSEST(TP_CASC_LEN, legal_set)
    return param_dict


def validate_TP_CASC_LEN(args):
    TP_CASC_LEN = args["TP_CASC_LEN"]
    AIE_VARIANT = args["AIE_VARIANT"]
    TT_DATA_A = args["TT_DATA_A"]
    TT_DATA_B = args["TT_DATA_B"]
    TP_DIM_A = args["TP_DIM_A"]
    TP_DIM_B = args["TP_DIM_B"]
    TP_SSR = args["TP_SSR"]
    TP_NUM_FRAMES = args["TP_NUM_FRAMES"]
    TP_API = args["TP_API"]
    return fn_validate_TP_CASC_LEN(AIE_VARIANT, TT_DATA_A, TT_DATA_B, TP_DIM_A, TP_DIM_B, TP_SSR, TP_NUM_FRAMES, TP_API, TP_CASC_LEN)


def fn_validate_TP_CASC_LEN(AIE_VARIANT, TT_DATA_A, TT_DATA_B, TP_DIM_A, TP_DIM_B, TP_SSR, TP_NUM_FRAMES, TP_API, TP_CASC_LEN):
    param_dict = fn_update_TP_CASC_LEN(AIE_VARIANT, TT_DATA_A, TT_DATA_B, TP_DIM_A, TP_DIM_B, TP_SSR, TP_NUM_FRAMES, TP_API, TP_CASC_LEN)
    legal_set = param_dict["enum"] if param_dict else []
    return validate_legal_set(legal_set, "TP_CASC_LEN", TP_CASC_LEN)


#######################################################
############# TP_DUAL_IP Updater and Validator ########
#######################################################
def update_TP_DUAL_IP(args):
    AIE_VARIANT = args["AIE_VARIANT"]
    TP_API = args["TP_API"]
    legal_set_TP_DUAL_IP = [0, 1]
    if TP_API == 0 or AIE_VARIANT == AIE_ML or AIE_VARIANT == AIE_MLv2:
        legal_set_TP_DUAL_IP.remove(1)
    param_dict = {}
    param_dict.update({"name": "TP_DUAL_IP"})
    param_dict.update({"enum": legal_set_TP_DUAL_IP})
    return param_dict


def validate_TP_DUAL_IP(args):
    TP_DUAL_IP = args["TP_DUAL_IP"]
    legal_set = update_TP_DUAL_IP(args)["enum"]
    return validate_legal_set(legal_set, "TP_DUAL_IP", TP_DUAL_IP)


#######################################################
####### TP_NUM_OUTPUTS Updater and Validator ##########
#######################################################
def update_TP_NUM_OUTPUTS(args):
    AIE_VARIANT = args["AIE_VARIANT"]
    TP_API = args["TP_API"]
    legal_set_TP_NUM_OUTPUTS = [1, 2]
    if TP_API == 0 or AIE_VARIANT == AIE_ML or AIE_VARIANT == AIE_MLv2:
        legal_set_TP_NUM_OUTPUTS.remove(2)
    param_dict = {}
    param_dict.update({"name": "TP_NUM_OUTPUTS"})
    param_dict.update({"enum": legal_set_TP_NUM_OUTPUTS})
    return param_dict


def validate_TP_NUM_OUTPUTS(args):
    TP_NUM_OUTPUTS = args["TP_NUM_OUTPUTS"]
    legal_set = update_TP_NUM_OUTPUTS(args)["enum"]
    return validate_legal_set(legal_set, "TP_NUM_OUTPUTS", TP_NUM_OUTPUTS)


#######################################################
######## TP_DIM_A_LEADING Updater and Validator #######
#######################################################
def update_TP_DIM_A_LEADING(args):
    TT_DATA_A = args["TT_DATA_A"]
    TP_NUM_FRAMES = args["TP_NUM_FRAMES"]
    TP_USE_MATRIX_RELOAD = args["TP_USE_MATRIX_RELOAD"]
    legal_set_TP_DIM_A_LEADING = [1, 0]
    if (TP_NUM_FRAMES > 1) or TP_USE_MATRIX_RELOAD or TT_DATA_A in ["int16", "cint32", "cfloat"]:
        legal_set_TP_DIM_A_LEADING.remove(0)
    param_dict = {}
    param_dict.update({"name": "TP_DIM_A_LEADING"})
    param_dict.update({"enum": legal_set_TP_DIM_A_LEADING})
    return param_dict


def validate_TP_DIM_A_LEADING(args):
    TT_DATA_A = args["TT_DATA_A"]
    TP_NUM_FRAMES = args["TP_NUM_FRAMES"]
    TP_USE_MATRIX_RELOAD = args["TP_USE_MATRIX_RELOAD"]
    TP_DIM_A_LEADING = args["TP_DIM_A_LEADING"]
    return fn_validate_leadDimA(TT_DATA_A, TP_NUM_FRAMES, TP_USE_MATRIX_RELOAD, TP_DIM_A_LEADING)


def fn_validate_leadDimA(TT_DATA_A, TP_NUM_FRAMES, TP_USE_MATRIX_RELOAD, TP_DIM_A_LEADING):
    if not TP_DIM_A_LEADING and TP_NUM_FRAMES > 1:
        return isError(
            f"TP_DIM_A_LEADING ({TP_DIM_A_LEADING}) is not supported for batch processing. Row major Matrix A inputs are only supported when NUM_FRAMES = 1. However, NUM_FRAMES is set to {TP_NUM_FRAMES}"
        )
    if not TP_DIM_A_LEADING and TP_USE_MATRIX_RELOAD:
        return isError(
            f"TP_DIM_A_LEADING ({TP_DIM_A_LEADING}) is not supported when matrix A is provided via RTP port (TP_USE_MATRIX_RELOAD)"
        )
    if not TP_DIM_A_LEADING and TT_DATA_A == "int16":
        return isError(
            f"Row major Matrix A inputs are not supported when TT_DATA_A = int16. Please provide int16 data in column major format, and set TP_DIM_A_LEADING to 1"
        )
    if not TP_DIM_A_LEADING and TT_DATA_A == "cint32":
        return isError(
            f"Row major Matrix A inputs are not supported when TT_DATA_A = cint32. Please provide cint32 data in column major format, and set TP_DIM_A_LEADING to 1"
        )
    if not TP_DIM_A_LEADING and TT_DATA_A == "cfloat":
        return isError(
            f"Row major Matrix A inputs are not supported when TT_DATA_A = cfloat. Please provide cfloat data in column major format, and set TP_DIM_A_LEADING to 1"
        )
    return isValid


#######################################################
########### TP_SHIFT Updater and Validator ############
#######################################################
def update_TP_SHIFT(args):
    TT_DATA = args["TT_DATA_A"]
    AIE_VARIANT = args["AIE_VARIANT"]
    range_TP_SHIFT = com.fn_update_range_TP_SHIFT(AIE_VARIANT, TT_DATA)
    param_dict = {
        "name": "TP_SHIFT",
        "minimum": range_TP_SHIFT[0],
        "maximum": range_TP_SHIFT[1],
    }
    TP_SHIFT = args["TP_SHIFT"] if "TP_SHIFT" in args and args["TP_SHIFT"] else 0
    TP_SHIFT = com.CLIP(TP_SHIFT, param_dict["minimum"], param_dict["maximum"])
    param_dict["actual"] = int(TP_SHIFT)
    return param_dict


def validate_TP_SHIFT(args):
    TT_DATA = args["TT_DATA_A"]
    AIE_VARIANT = args["AIE_VARIANT"]
    if "TP_SHIFT" not in args:
        return isValid
    TP_SHIFT = args["TP_SHIFT"]
    return fn_validate_TP_SHIFT(AIE_VARIANT, TT_DATA, TP_SHIFT)


def fn_validate_TP_SHIFT(AIE_VARIANT, TT_DATA, TP_SHIFT):
    param_dict = update_TP_SHIFT({"AIE_VARIANT": AIE_VARIANT, "TT_DATA_A": TT_DATA, "TP_SHIFT": TP_SHIFT})
    return validate_range([param_dict["minimum"], param_dict["maximum"]], "TP_SHIFT", TP_SHIFT)


#######################################################
############## TP_RND Updater and Validator ###########
#######################################################
def update_TP_RND(args):
    AIE_VARIANT = args["AIE_VARIANT"]
    legal_set_TP_RND = fn_get_legalSet_roundMode(AIE_VARIANT)
    param_dict = {}
    param_dict.update({"name": "TP_RND"})
    param_dict.update({"enum": legal_set_TP_RND})
    return param_dict


def validate_TP_RND(args):
    AIE_VARIANT = args["AIE_VARIANT"]
    TP_RND = args["TP_RND"]
    return fn_validate_roundMode(TP_RND, AIE_VARIANT)


#######################################################
############## TP_SAT Updater and Validator ###########
#######################################################
def update_TP_SAT(args):
    legal_set_TP_SAT = fn_legal_set_sat()
    param_dict = {}
    param_dict.update({"name": "TP_SAT"})
    param_dict.update({"enum": legal_set_TP_SAT})
    return param_dict


def validate_TP_SAT(args):
    TP_SAT = args["TP_SAT"]
    return fn_validate_satMode(TP_SAT)


#######################################################
################## Port Info ##########################
#######################################################
def info_ports(args):
    TT_DATA_OUT = getOutputType(args["TT_DATA_A"], args["TT_DATA_B"])
    INPUT_WINDOW_VSIZE_A = (args["TP_NUM_FRAMES"] * args["TP_DIM_A"] * args["TP_DIM_B"] // (args["TP_SSR"] * args["TP_CASC_LEN"]))
    INPUT_WINDOW_VSIZE_B = (args["TP_NUM_FRAMES"] * args["TP_DIM_B"] // args["TP_CASC_LEN"] )
    OUTPUT_WINDOW_VSIZE = args["TP_DIM_A"] * args["TP_NUM_FRAMES"] // args["TP_SSR"]
    
    if args["TP_USE_MATRIX_RELOAD"] == 0:
        portsA = com.get_port_info(
            portname="inA",
            dir="in",
            TT_DATA=args["TT_DATA_A"],
            windowVSize=INPUT_WINDOW_VSIZE_A,
            vectorLength=(args["TP_CASC_LEN"] * args["TP_SSR"]),
        )
    else:
        portsA = get_parameter_port_info(
            portname="matrixA",
            dir="in",
            TT_DATA=args["TT_DATA_A"],
            numElements=INPUT_WINDOW_VSIZE_A,
            vectorLength=(args["TP_CASC_LEN"] * args["TP_SSR"]),
            synchronicity="async",
        )

    portsB = com.get_port_info(
        portname="inB",
        dir="in",
        TT_DATA=args["TT_DATA_B"],
        windowVSize=INPUT_WINDOW_VSIZE_B,
        vectorLength=(args["TP_CASC_LEN"] * args["TP_SSR"] * (args["TP_DUAL_IP"] + 1)),
    )

    portsOut = com.get_port_info(
        portname="out",
        dir="out",
        TT_DATA=TT_DATA_OUT,
        windowVSize=OUTPUT_WINDOW_VSIZE,
        vectorLength=(args["TP_NUM_OUTPUTS"] * args["TP_SSR"]),
    )

    return portsA + portsB + portsOut


#######################################################
################## Graph Generator ####################
#######################################################
def generate_graph(graphname, args):
    if graphname == "":
        graphname = "default_graphname"

    TT_DATA_A = args["TT_DATA_A"]
    TT_DATA_B = args["TT_DATA_B"]
    TP_DIM_A = args["TP_DIM_A"]
    TP_DIM_B = args["TP_DIM_B"]
    TP_SHIFT = args["TP_SHIFT"]
    TP_RND = args["TP_RND"]
    TP_NUM_FRAMES = args["TP_NUM_FRAMES"]
    TP_CASC_LEN = args["TP_CASC_LEN"]
    TP_SAT = args["TP_SAT"]
    TP_SSR = args["TP_SSR"]
    TP_DIM_A_LEADING = args["TP_DIM_A_LEADING"]
    TP_USE_MATRIX_RELOAD = args["TP_USE_MATRIX_RELOAD"]
    TP_API = args["TP_API"]
    TP_DUAL_IP = args["TP_DUAL_IP"]
    TP_NUM_OUTPUTS = args["TP_NUM_OUTPUTS"]

    matrix_declare_str = (
        f"rtp_port_array matrixA;"
        if TP_USE_MATRIX_RELOAD == 1
        else f"std::array<adf::port<input>,{TP_CASC_LEN} * {TP_SSR}> inA;"
    )

    code = f"""
class {graphname} : public adf::graph {{
public:

  using rtp_port_array = std::array<adf::port<input>, {TP_CASC_LEN} * {TP_SSR}>;
  {matrix_declare_str}
  std::array<adf::port<input>,{TP_CASC_LEN} * {TP_SSR} * ({TP_DUAL_IP} + 1)> inB;
  std::array<adf::port<output>, {TP_SSR} * {TP_NUM_OUTPUTS}> out;

  xf::dsp::aie::blas::matrix_vector_mul::matrix_vector_mul_graph<
    {TT_DATA_A}, // TT_DATA_A
    {TT_DATA_B}, // TT_DATA_B
    {TP_DIM_A}, // TP_DIM_A
    {TP_DIM_B}, // TP_DIM_B
    {TP_SHIFT}, // TP_SHIFT
    {TP_RND}, // TP_RND
    {TP_NUM_FRAMES}, // TP_NUM_FRAMES
    {TP_CASC_LEN}, // TP_CASC_LEN
    {TP_SAT}, // TP_SAT
    {TP_SSR}, //TP_SSR
    {TP_DIM_A_LEADING}, // TP_DIM_A_LEADING
    {TP_USE_MATRIX_RELOAD}, // TP_USE_MATRIX_RELOAD
    {TP_API}, // TP_API
    {TP_DUAL_IP}, // TP_DUAL_IP
    {TP_NUM_OUTPUTS} // TP_NUM_OUTPUTS
  > matVecMul;

  {graphname}() : matVecMul() {{
    adf::kernel *matVecMul_kernels = matVecMul.getKernels();

    for (int ssrIdx = 0; ssrIdx < {TP_SSR}; ssrIdx++) {{
        for (int cascIdx=0; cascIdx < {TP_CASC_LEN}; cascIdx++) {{
            int kernelNum = cascIdx + ssrIdx * {TP_CASC_LEN};
            
            {"// Connect Matrix A" if TP_USE_MATRIX_RELOAD == 0 else ""}
            {"adf::connect<> net_inA(inA[kernelNum], matVecMul.inA[kernelNum]);" if TP_USE_MATRIX_RELOAD == 0 else ""}
            
            // Streams B (single or dual)
            for (int dualIdx = 0; dualIdx < ({TP_DUAL_IP} + 1); dualIdx++) {{
                int bPortIdx = ({TP_DUAL_IP} + 1) * kernelNum + dualIdx;
                adf::connect<> net_inB(inB[bPortIdx], matVecMul.inB[bPortIdx]);
            }}
        }}
        {"" if TP_USE_MATRIX_RELOAD == 0 else f'''
        // Connect RTP ports for matrix reload
        for (int kernelIdx = 0; kernelIdx < {TP_CASC_LEN}; kernelIdx++) {{
            int kernelNum = kernelIdx + ssrIdx * {TP_CASC_LEN};
            adf::connect<>(matrixA[kernelNum], matVecMul.matrixA[kernelNum]);
        }}'''}
        
        for (int outIdx = 0; outIdx < {TP_NUM_OUTPUTS}; outIdx++) {{
            int outPortIdx = ({TP_NUM_OUTPUTS} * ssrIdx) + outIdx;
            adf::connect<> net_out(matVecMul.out[outPortIdx], out[outPortIdx]);
        }}
    }}
  }}
}};
"""
    out = {}
    out["graph"] = code
    out["port_info"] = info_ports(args)
    out["headerfile"] = "matrix_vector_mul_graph.hpp"
    out["searchpaths"] = ["L1/include/aie", "L2/include/aie", "L1/src/aie"]

    return out
