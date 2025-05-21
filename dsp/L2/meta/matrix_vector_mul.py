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

TP_CASC_LEN_min = 1
TP_CASC_LEN_max = 16
TP_SSR_min = 1
TP_SSR_max = 16
TP_WINDOW_VSIZE_min = 16
# TP_WINDOW_VSIZE_max = 16384
# aie1_pp_buffer=16384
# aie2_pp_buffer=32768


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
    legal_set_TT_DATA_A = param_dict["enum"]
    return validate_legal_set(legal_set_TT_DATA_A, "TT_DATA_A", TT_DATA_A)


#######################################################
########### TT_DATA_B Updater and Validator ###########
#######################################################
def update_TT_DATA_B(args):
    TT_DATA_A = args["TT_DATA_A"]
    return fn_update_TT_DATA_B(TT_DATA_A)


def fn_update_TT_DATA_B(TT_DATA_A):
    int_set = ["int16", "int32", "cint16", "cint32"]
    float_set = ["float", "cfloat"]
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
    return validate_legal_set(param_dict["enum"], "TT_DATA_B", TT_DATA_B)


#######################################################
######## TP_USE_MATRIX_RELOAD Updater and Validator #######
#######################################################
def update_TP_USE_MATRIX_RELOAD(args):
    return fn_update_TP_USE_MATRIX_RELOAD()


def fn_update_TP_USE_MATRIX_RELOAD():
    legal_set_TP_USE_MATRIX_RELOAD = [0, 1]

    param_dict = {}
    param_dict.update({"name": "TP_USE_MATRIX_RELOAD"})
    param_dict.update({"enum": legal_set_TP_USE_MATRIX_RELOAD})
    return param_dict


def validate_TP_USE_MATRIX_RELOAD(args):
    TP_USE_MATRIX_RELOAD = args["TP_USE_MATRIX_RELOAD"]
    return fn_validate_TP_USE_MATRIX_RELOAD(TP_USE_MATRIX_RELOAD)


def fn_validate_TP_USE_MATRIX_RELOAD(TP_USE_MATRIX_RELOAD):
    param_dict = fn_update_TP_USE_MATRIX_RELOAD()
    legal_set_TP_USE_MATRIX_RELOAD = param_dict["enum"]
    return validate_legal_set(
        legal_set_TP_USE_MATRIX_RELOAD, "TP_USE_MATRIX_RELOAD", TP_USE_MATRIX_RELOAD
    )


#######################################################
########### TP_API Updater and Validator ##############
#######################################################
def update_TP_API(args):
    TP_USE_MATRIX_RELOAD = args["TP_USE_MATRIX_RELOAD"]
    return fn_update_TP_API(TP_USE_MATRIX_RELOAD)


def fn_update_TP_API(TP_USE_MATRIX_RELOAD):
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
    param_dict = fn_update_TP_API(TP_USE_MATRIX_RELOAD)
    legal_set_TP_API = param_dict["enum"]
    return validate_legal_set(legal_set_TP_API, "TP_API", TP_API)


#######################################################
############ TP_DIM_A Updater and Validator ###########
#######################################################
def update_TP_DIM_A(args):
    AIE_VARIANT = args["AIE_VARIANT"]
    TT_DATA_A = args["TT_DATA_A"]
    TT_DATA_B = args["TT_DATA_B"]
    if "TP_DIM_A" in args and args["TP_DIM_A"]:
        TP_DIM_A = args["TP_DIM_A"]
    else:
        TP_DIM_A = 0
    return fn_update_tp_dim_a(AIE_VARIANT, TT_DATA_A, TT_DATA_B, TP_DIM_A)


def fn_update_tp_dim_a(AIE_VARIANT, TT_DATA_A, TT_DATA_B, TP_DIM_A):

    TP_DIM_A_min = 256 / 8 / fn_size_by_byte(TT_DATA_A)
    TP_DIM_B_min = 256 / 8 / fn_size_by_byte(TT_DATA_B)

    TT_OUT = getOutputType(TT_DATA_A, TT_DATA_B)
    buffer_size = k_data_memory_bytes[AIE_VARIANT]
    max_buffer_samples_a = buffer_size / fn_size_by_byte(TT_DATA_A)
    max_buffer_samples_out = buffer_size / fn_size_by_byte(TT_OUT)

    dim_a_max_a = (max_buffer_samples_a * TP_SSR_max) / (TP_DIM_B_min)
    dim_a_max_out = max_buffer_samples_out * TP_SSR_max

    TP_DIM_A_max = min(dim_a_max_a, dim_a_max_out)
    param_dict = {
        "name": "TP_DIM_A",
        "minimum": int(TP_DIM_A_min),
        "maximum": int(FLOOR(TP_DIM_A_max, TP_DIM_A_min)),
        "maximum_pingpong_buf": int(FLOOR(TP_DIM_A_max / 2, TP_DIM_A_min)),
    }
    if TP_DIM_A != 0 and (TP_DIM_A % TP_DIM_A_min != 0):
        TP_DIM_A_act = CEIL(TP_DIM_A, TP_DIM_A_min)

        if TP_DIM_A_act < param_dict["minimum"]:
            TP_DIM_A_act = param_dict["minimum"]

        if TP_DIM_A_act > param_dict["maximum"]:
            TP_DIM_A_act = param_dict["maximum"]
        param_dict.update({"actual": TP_DIM_A_act})

    return param_dict


def validate_TP_DIM_A(args):
    AIE_VARIANT = args["AIE_VARIANT"]
    TT_DATA_A = args["TT_DATA_A"]
    TP_DIM_A = args["TP_DIM_A"]
    TT_DATA_B = args["TT_DATA_B"]
    return fn_validate_tp_dim_a(AIE_VARIANT, TT_DATA_A, TT_DATA_B, TP_DIM_A)


def fn_validate_tp_dim_a(AIE_VARIANT, TT_DATA_A, TT_DATA_B, TP_DIM_A):
    param_dict = fn_update_tp_dim_a(AIE_VARIANT, TT_DATA_A, TT_DATA_B, TP_DIM_A)
    TP_DIM_A_min = param_dict["minimum"]
    if TP_DIM_A % TP_DIM_A_min != 0:
        return isError(f"TP_DIM_A should be a multiple of {TP_DIM_A_min}!")
    else:
        range_TP_DIM_A = [param_dict["minimum"], param_dict["maximum"]]
        return validate_range(range_TP_DIM_A, "TP_DIM_A", TP_DIM_A)


#######################################################
########### TP_DIM_B Updater and Validator ############
#######################################################
def update_TP_DIM_B(args):
    AIE_VARIANT = args["AIE_VARIANT"]
    TT_DATA_A = args["TT_DATA_A"]
    TT_DATA_B = args["TT_DATA_B"]
    TP_DIM_A = args["TP_DIM_A"]
    TP_API = args["TP_API"]
    if ("TP_DIM_B" in args) and args["TP_DIM_B"]:
        TP_DIM_B = args["TP_DIM_B"]
    else:
        TP_DIM_B = 0
    return fn_update_tp_dim_b(
        AIE_VARIANT, TT_DATA_A, TT_DATA_B, TP_DIM_A, TP_DIM_B, TP_API
    )


def fn_update_tp_dim_b(AIE_VARIANT, TT_DATA_A, TT_DATA_B, TP_DIM_A, TP_DIM_B, TP_API):

    # Martix A * B must fit in buffer
    TP_DIM_B_min = 256 / 8 / fn_size_by_byte(TT_DATA_B)
    TT_OUT = getOutputType(TT_DATA_A, TT_DATA_B)
    buffer_size = k_data_memory_bytes[AIE_VARIANT]
    max_buffer_samples_a = buffer_size / fn_size_by_byte(TT_DATA_A)
    max_buffer_samples_b = buffer_size / fn_size_by_byte(TT_DATA_A)
    max_buffer_samples_out = buffer_size / fn_size_by_byte(TT_OUT)

    # Max dim B on matrix A
    dim_b_max_a = (max_buffer_samples_a * TP_SSR_max * TP_CASC_LEN_max) / TP_DIM_A

    # If streams, dimB must fit in 1024-b reg
    if TP_API == 1:
        dim_b_max_b = TP_CASC_LEN_max * (1024 // 8 // fn_size_by_byte(TT_DATA_B))
    else:
        dim_b_max_b = max_buffer_samples_b * TP_CASC_LEN_max
    TP_DIM_B_max = min(dim_b_max_a, dim_b_max_b)

    param_dict = {
        "name": "TP_DIM_B",
        "minimum": int(TP_DIM_B_min),
        "maximum": int(FLOOR(TP_DIM_B_max, TP_DIM_B_min)),
        "maximum_pingpong_buf": int(FLOOR(TP_DIM_B_max / 2, TP_DIM_B_min)),
    }

    if TP_DIM_B != 0 and (TP_DIM_B % TP_DIM_B_min != 0):
        TP_DIM_B_act = CEIL(TP_DIM_B, TP_DIM_B_min)

        if TP_DIM_B_act < param_dict["minimum"]:
            TP_DIM_B_act = param_dict["minimum"]

        if TP_DIM_B_act > param_dict["maximum"]:
            TP_DIM_B_act = param_dict["maximum"]
        param_dict.update({"actual": TP_DIM_B_act})

    return param_dict


def validate_TP_DIM_B(args):
    AIE_VARIANT = args["AIE_VARIANT"]
    TT_DATA_A = args["TT_DATA_A"]
    TT_DATA_B = args["TT_DATA_B"]
    TP_DIM_A = args["TP_DIM_A"]
    TP_DIM_B = args["TP_DIM_B"]
    TP_API = args["TP_API"]

    return fn_validate_tp_dim_b(
        AIE_VARIANT, TT_DATA_A, TT_DATA_B, TP_DIM_A, TP_DIM_B, TP_API
    )


def fn_validate_tp_dim_b(AIE_VARIANT, TT_DATA_A, TT_DATA_B, TP_DIM_A, TP_DIM_B, TP_API):
    param_dict = fn_update_tp_dim_b(
        AIE_VARIANT, TT_DATA_A, TT_DATA_B, TP_DIM_A, TP_DIM_B, TP_API
    )
    TP_DIM_B_min = param_dict["minimum"]
    if TP_DIM_B % TP_DIM_B_min != 0:
        return isError(f"TP_DIM_B should be a multiple of {TP_DIM_B_min}!")
    else:
        range_TP_DIM_B = [param_dict["minimum"], param_dict["maximum"]]
        return validate_range(range_TP_DIM_B, "TP_DIM_B", TP_DIM_B)


#######################################################
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

    return fn_update_tp_num_frames(
        AIE_VARIANT, TT_DATA_A, TT_DATA_B, TP_DIM_A, TP_DIM_B, TP_API
    )


def fn_update_tp_num_frames(
    AIE_VARIANT, TT_DATA_A, TT_DATA_B, TP_DIM_A, TP_DIM_B, TP_API
):
    TP_DIM_A_min = 256 / 8 / fn_size_by_byte(TT_DATA_A)
    TP_DIM_B_min = 256 / 8 / fn_size_by_byte(TT_DATA_B)
    buffer_size = k_data_memory_bytes[AIE_VARIANT]
    max_buffer_samples_a = buffer_size / fn_size_by_byte(TT_DATA_A)
    max_buffer_samples_b = buffer_size / fn_size_by_byte(TT_DATA_B)

    maxSsr = min(TP_DIM_A / TP_DIM_A_min, TP_SSR_max)
    maxCasc = min(TP_DIM_B / TP_DIM_B_min, TP_CASC_LEN_max)
    num_frames_max_a = (max_buffer_samples_a * maxSsr * maxCasc) / (TP_DIM_A * TP_DIM_B)
    if TP_API:
        num_frames_max_b = (1024 // 8 // fn_size_by_byte(TT_DATA_B) * maxCasc) / (
            TP_DIM_B
        )
    else:
        num_frames_max_b = (max_buffer_samples_b * maxCasc) / (TP_DIM_B)
    TP_NUM_FRAMES_max = min(num_frames_max_a, num_frames_max_b)

    param_dict = {
        "name": "TP_NUM_FRAMES",
        "minimum": 1,
        "maximum": int(TP_NUM_FRAMES_max),
        "maximum_pingpong_buf": int(TP_NUM_FRAMES_max / 2),
    }

    return param_dict


def validate_TP_NUM_FRAMES(args):
    AIE_VARIANT = args["AIE_VARIANT"]
    TT_DATA_A = args["TT_DATA_A"]
    TT_DATA_B = args["TT_DATA_B"]
    TP_DIM_A = args["TP_DIM_A"]
    TP_DIM_B = args["TP_DIM_B"]
    TP_NUM_FRAMES = args["TP_NUM_FRAMES"]
    TP_API = args["TP_API"]

    return fn_validate_numFrames(
        AIE_VARIANT, TT_DATA_A, TT_DATA_B, TP_DIM_A, TP_DIM_B, TP_NUM_FRAMES, TP_API
    )


def fn_validate_numFrames(
    AIE_VARIANT, TT_DATA_A, TT_DATA_B, TP_DIM_A, TP_DIM_B, TP_NUM_FRAMES, TP_API
):
    param_dict = fn_update_tp_num_frames(
        AIE_VARIANT, TT_DATA_A, TT_DATA_B, TP_DIM_A, TP_DIM_B, TP_API
    )
    range_num_frames = [param_dict["minimum"], param_dict["maximum"]]
    return validate_range(range_num_frames, "TP_NUM_FRAMES", TP_NUM_FRAMES)


#######################################################
############ TP_SSR Updater and Validator #############
#######################################################
def update_TP_SSR(args):
    AIE_VARIANT = args["AIE_VARIANT"]
    TT_DATA_A = args["TT_DATA_A"]
    TP_DIM_A = args["TP_DIM_A"]
    TP_DIM_B = args["TP_DIM_B"]
    TP_NUM_FRAMES = args["TP_NUM_FRAMES"]
    return fn_update_TP_SSR(AIE_VARIANT, TT_DATA_A, TP_DIM_A, TP_DIM_B, TP_NUM_FRAMES)


def fn_update_TP_SSR(AIE_VARIANT, TT_DATA_A, TP_DIM_A, TP_DIM_B, TP_NUM_FRAMES):
    buffer_size = k_data_memory_bytes[AIE_VARIANT]
    max_buffer_sample_in = buffer_size / fn_size_by_byte(TT_DATA_A)
    legal_set_tp_ssr = find_divisors(TP_DIM_A, TP_SSR_max)

    legal_set_tp_ssr_pingpong = legal_set_tp_ssr.copy()
    for k in legal_set_tp_ssr.copy():
        if (
            ((TP_DIM_A * TP_DIM_B * TP_NUM_FRAMES) / (k * TP_CASC_LEN_max))
            > max_buffer_sample_in
        ) or ((TP_DIM_A / k) % (256 / 8 / fn_size_by_byte(TT_DATA_A)) != 0):
            legal_set_tp_ssr.remove(k)
        if (
            ((TP_DIM_A * TP_DIM_B * TP_NUM_FRAMES) / (k * TP_CASC_LEN_max))
            > max_buffer_sample_in / 2
        ) or ((TP_DIM_A / k) % (256 / 8 / fn_size_by_byte(TT_DATA_A)) != 0):
            legal_set_tp_ssr_pingpong.remove(k)

    param_dict = {
        "name": "TP_SSR",
        "enum": legal_set_tp_ssr,
        "enum_pingpong_buf": legal_set_tp_ssr_pingpong,
    }
    return param_dict


def validate_TP_SSR(args):
    AIE_VARIANT = args["AIE_VARIANT"]
    TT_DATA_A = args["TT_DATA_A"]
    TP_DIM_A = args["TP_DIM_A"]
    TP_DIM_B = args["TP_DIM_B"]
    TP_NUM_FRAMES = args["TP_NUM_FRAMES"]
    TP_SSR = args["TP_SSR"]
    return fn_validate_TP_SSR(
        AIE_VARIANT, TT_DATA_A, TP_DIM_A, TP_DIM_B, TP_NUM_FRAMES, TP_SSR
    )


def fn_validate_TP_SSR(
    AIE_VARIANT, TT_DATA_A, TP_DIM_A, TP_DIM_B, TP_NUM_FRAMES, TP_SSR
):
    param_dict = fn_update_TP_SSR(
        AIE_VARIANT, TT_DATA_A, TP_DIM_A, TP_DIM_B, TP_NUM_FRAMES
    )
    return validate_legal_set(param_dict["enum"], "TP_SSR", TP_SSR)


#######################################################
############ TP_CASC_LEN Updater and Validator ########
##################################################2#####
def update_TP_CASC_LEN(args):
    AIE_VARIANT = args["AIE_VARIANT"]
    TT_DATA_A = args["TT_DATA_A"]
    TT_DATA_B = args["TT_DATA_B"]
    TP_DIM_A = args["TP_DIM_A"]
    TP_DIM_B = args["TP_DIM_B"]
    TP_SSR = args["TP_SSR"]
    TP_NUM_FRAMES = args["TP_NUM_FRAMES"]
    TP_API = args["TP_API"]
    return fn_update_TP_CASC_LEN(
        AIE_VARIANT,
        TT_DATA_A,
        TT_DATA_B,
        TP_DIM_A,
        TP_DIM_B,
        TP_SSR,
        TP_NUM_FRAMES,
        TP_API,
    )


def fn_update_TP_CASC_LEN(
    AIE_VARIANT, TT_DATA_A, TT_DATA_B, TP_DIM_A, TP_DIM_B, TP_SSR, TP_NUM_FRAMES, TP_API
):

    buffer_size = k_data_memory_bytes[AIE_VARIANT]
    max_buffer_sample_in = buffer_size / fn_size_by_byte(TT_DATA_A)
    legal_set_tp_casc = find_divisors(TP_DIM_B, TP_CASC_LEN_max)
    legal_set_tp_casc_pingpong = legal_set_tp_casc.copy()
    for k in legal_set_tp_casc.copy():
        if (
            ((TP_DIM_A * TP_DIM_B * TP_NUM_FRAMES) / (k * TP_SSR))
            > max_buffer_sample_in
        ) or ((TP_DIM_B / k) % (256 / 8 / fn_size_by_byte(TT_DATA_B)) != 0):
            legal_set_tp_casc.remove(k)
        if (
            ((TP_DIM_A * TP_DIM_B * TP_NUM_FRAMES) / (k * TP_SSR))
            > max_buffer_sample_in / 2
        ) or ((TP_DIM_B / k) % (256 / 8 / fn_size_by_byte(TT_DATA_B)) != 0):
            legal_set_tp_casc_pingpong.remove(k)
        if (TP_API and (TP_DIM_B * TP_NUM_FRAMES) / (k)) > (
            1024 // 8 // fn_size_by_byte(TT_DATA_B)
        ):
            if k in legal_set_tp_casc: legal_set_tp_casc.remove(k)
            if k in legal_set_tp_casc_pingpong: legal_set_tp_casc_pingpong.remove(k)

    param_dict = {
        "name": "TP_CASC_LEN",
        "enum": legal_set_tp_casc,
        "enum_pingpong_buf": legal_set_tp_casc_pingpong,
    }
    return param_dict


def validate_TP_CASC_LEN(args):
    AIE_VARIANT = args["AIE_VARIANT"]
    TT_DATA_A = args["TT_DATA_A"]
    TT_DATA_B = args["TT_DATA_B"]
    TP_DIM_A = args["TP_DIM_A"]
    TP_DIM_B = args["TP_DIM_B"]
    TP_NUM_FRAMES = args["TP_NUM_FRAMES"]
    TP_SSR = args["TP_SSR"]
    TP_CASC_LEN = args["TP_CASC_LEN"]
    TP_API = args["TP_API"]
    return fn_validate_TP_CASC_LEN(
        AIE_VARIANT,
        TT_DATA_A,
        TT_DATA_B,
        TP_DIM_A,
        TP_DIM_B,
        TP_SSR,
        TP_NUM_FRAMES,
        TP_CASC_LEN,
        TP_API,
    )


def fn_validate_TP_CASC_LEN(
    AIE_VARIANT,
    TT_DATA_A,
    TT_DATA_B,
    TP_DIM_A,
    TP_DIM_B,
    TP_SSR,
    TP_NUM_FRAMES,
    TP_CASC_LEN,
    TP_API,
):
    param_dict = fn_update_TP_CASC_LEN(
        AIE_VARIANT,
        TT_DATA_A,
        TT_DATA_B,
        TP_DIM_A,
        TP_DIM_B,
        TP_SSR,
        TP_NUM_FRAMES,
        TP_API,
    )
    return validate_legal_set(param_dict["enum"], "TP_CASC_LEN", TP_CASC_LEN)


#######################################################
############# TP_DUAL_IP Updater and Validator ########
#######################################################
def update_TP_DUAL_IP(args):
    AIE_VARIANT = args["AIE_VARIANT"]
    TP_API = args["TP_API"]
    return fn_update_TP_DUAL_IP(AIE_VARIANT, TP_API)


def fn_update_TP_DUAL_IP(AIE_VARIANT, TP_API):
    legal_set_TP_DUAL_IP = [0, 1]
    if TP_API == 0 or AIE_VARIANT == AIE_ML or AIE_VARIANT == AIE_MLv2:
        legal_set_TP_DUAL_IP = [0]

    param_dict = {"name": "TP_DUAL_IP", "enum": legal_set_TP_DUAL_IP}
    return param_dict


def validate_TP_DUAL_IP(args):
    AIE_VARIANT = args["AIE_VARIANT"]
    TP_API = args["TP_API"]
    TP_DUAL_IP = args["TP_DUAL_IP"]
    return fn_validate_TP_DUAL_IP(AIE_VARIANT, TP_API, TP_DUAL_IP)


def fn_validate_TP_DUAL_IP(AIE_VARIANT, TP_API, TP_DUAL_IP):
    param_dict = fn_update_TP_DUAL_IP(AIE_VARIANT, TP_API)
    return validate_legal_set(param_dict["enum"], "TP_DUAL_IP", TP_DUAL_IP)


#######################################################
####### TP_NUM_OUTPUTS Updater and Validator ##########
#######################################################
def update_TP_NUM_OUTPUTS(args):
    AIE_VARIANT = args["AIE_VARIANT"]
    TP_API = args["TP_API"]
    return fn_update_TP_NUM_OUTPUTS(AIE_VARIANT, TP_API)


def fn_update_TP_NUM_OUTPUTS(AIE_VARIANT, TP_API):
    legal_set_TP_NUM_OUTPUTS = [1, 2]
    if TP_API == 0 or AIE_VARIANT == AIE_ML or AIE_VARIANT == AIE_MLv2:
        legal_set_TP_NUM_OUTPUTS = [1]

    param_dict = {"name": "TP_NUM_OUTPUTS", "enum": legal_set_TP_NUM_OUTPUTS}
    return param_dict


def validate_TP_NUM_OUTPUTS(args):
    AIE_VARIANT = args["AIE_VARIANT"]
    TP_API = args["TP_API"]
    TP_NUM_OUTPUTS = args["TP_NUM_OUTPUTS"]
    return fn_validate_TP_NUM_OUTPUTS(AIE_VARIANT, TP_API, TP_NUM_OUTPUTS)


def fn_validate_TP_NUM_OUTPUTS(AIE_VARIANT, TP_API, TP_NUM_OUTPUTS):
    param_dict = fn_update_TP_NUM_OUTPUTS(AIE_VARIANT, TP_API)
    return validate_legal_set(param_dict["enum"], "TP_NUM_OUTPUTS", TP_NUM_OUTPUTS)


#######################################################
######## TP_DIM_A_LEADING Updater and Validator #######
#######################################################
def update_TP_DIM_A_LEADING(args):
    TT_DATA_A = args["TT_DATA_A"]
    TP_NUM_FRAMES = args["TP_NUM_FRAMES"]
    TP_USE_MATRIX_RELOAD = args["TP_USE_MATRIX_RELOAD"]
    return fn_update_TP_DIM_A_LEADING(TT_DATA_A, TP_NUM_FRAMES, TP_USE_MATRIX_RELOAD)


def fn_update_TP_DIM_A_LEADING(TT_DATA_A, TP_NUM_FRAMES, TP_USE_MATRIX_RELOAD):
    legal_set = [1, 0]
    if (TP_NUM_FRAMES > 1) or (
        TT_DATA_A in ["int16", "cint32", "cfloat"] or TP_USE_MATRIX_RELOAD
    ):
        legal_set = [1]

    param_dict = {"name": "TP_DIM_A_LEADING", "enum": legal_set}

    return param_dict


def validate_TP_DIM_A_LEADING(args):
    TT_DATA_A = args["TT_DATA_A"]
    TP_NUM_FRAMES = args["TP_NUM_FRAMES"]
    TP_USE_MATRIX_RELOAD = args["TP_USE_MATRIX_RELOAD"]
    TP_DIM_A_LEADING = args["TP_DIM_A_LEADING"]
    return fn_validate_leadDimA(
        TT_DATA_A, TP_NUM_FRAMES, TP_USE_MATRIX_RELOAD, TP_DIM_A_LEADING
    )


def fn_validate_leadDimA(
    TT_DATA_A, TP_NUM_FRAMES, TP_USE_MATRIX_RELOAD, TP_DIM_A_LEADING
):
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
    return validate_range(range_TP_SHIFT, "TP_SHIFT", TP_SHIFT)


#######################################################
############## TP_RND Updater and Validator ###########
#######################################################
def update_TP_RND(args):
    AIE_VARIANT = args["AIE_VARIANT"]
    return fn_update_TP_RND(AIE_VARIANT)


def fn_update_TP_RND(AIE_VARIANT):
    legal_set_TP_RND = fn_get_legalSet_roundMode(AIE_VARIANT)
    param_dict = {"name": "TP_RND", "enum": legal_set_TP_RND}
    return param_dict


def validate_TP_RND(args):
    AIE_VARIANT = args["AIE_VARIANT"]
    TP_RND = args["TP_RND"]
    return fn_validate_roundMode(TP_RND, AIE_VARIANT)


#######################################################
############## TP_SAT Updater and Validator ###########
#######################################################
def update_TP_SAT(args):
    legal_set_sat = fn_legal_set_sat()
    param_dict = {"name": "TP_SAT", "enum": legal_set_sat}
    return param_dict


def validate_TP_SAT(args):
    TP_SAT = args["TP_SAT"]
    return fn_validate_satMode(TP_SAT)


def isMultiple(A, B):
    return A % B == 0


def getOutputType(typeA, typeB):
    if fn_size_by_byte(typeA) > fn_size_by_byte(typeB):
        return typeA
    else:
        return typeB


def info_ports(args):
    portsA = com.get_port_info(
        portname="inA",
        dir="in",
        TT_DATA=args["TT_DATA_A"],
        windowVSize=(args["TP_NUM_FRAMES"] * args["TP_DIM_A"] * args["TP_DIM_B"]),
        vectorLength=args["TP_CASC_LEN"],
    )
    portsB = com.get_port_info(
        portname="inB",
        dir="in",
        TT_DATA=args["TT_DATA_B"],
        windowVSize=(args["TP_NUM_FRAMES"] * args["TP_DIM_B"] / args["TP_CASC_LEN"]),
        vectorLength=args["TP_CASC_LEN"],
    )
    TT_DATA_OUT = getOutputType(args["TT_DATA_A"], args["TT_DATA_B"])
    TP_OUTPUT_WINDOW_VSIZE = args["TP_DIM_B"] * args["TP_NUM_FRAMES"]
    portsOut = com.get_port_info(
        portname="out",
        dir="out",
        TT_DATA=TT_DATA_OUT,
        windowVSize=(TP_OUTPUT_WINDOW_VSIZE),
        vectorLength=None,
    )
    # join lists of ports together and return
    return portsA + portsB + portsOut


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

    # Use formatted multi-line string to avoid a lot of \n and \t
    code = f"""
class {graphname} : public adf::graph {{
public:
  std::array<adf::port<input>,{TP_CASC_LEN} * {TP_SSR}> inA;
  std::array<adf::port<input>,{TP_CASC_LEN} * {TP_SSR}> inB;
  std::array<adf::port<output>, {TP_SSR}> out;
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
        adf::connect<> net_inA(inA[cascIdx + ssrIdx * {TP_CASC_LEN}], matVecMul.inA[cascIdx + ssrIdx * {TP_CASC_LEN}]);
        adf::connect<> net_inB(inB[cascIdx + ssrIdx * {TP_CASC_LEN}], matVecMul.inB[cascIdx + ssrIdx * {TP_CASC_LEN}]);
      }}
        adf::connect<> net_out(matVecMul.out[ssrIdx], out[ssrIdx]);
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
