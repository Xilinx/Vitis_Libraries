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

TP_SSR_min = 1
TP_SSR_max = 16

TP_CASC_min = 1
TP_CASC_max = 16


#######################################################
########### AIE_VARIANT Updater and Validator #########
#######################################################
def update_AIE_VARIANT(args):
    legal_set_AIE_VARIANT = [com.AIE, com.AIE_ML, com.AIE_MLv2]
    param_dict = {}
    param_dict.update({"name": "AIE_VARIANT"})
    param_dict.update({"enum": legal_set_AIE_VARIANT})
    return param_dict


def validate_AIE_VARIANT(args):
    AIE_VARIANT = args["AIE_VARIANT"]
    
    # Get legal set from updater
    legal_set = update_AIE_VARIANT(args)["enum"]
    
    return validate_legal_set(legal_set, "AIE_VARIANT", AIE_VARIANT)


#######################################################
########### TT_DATA_A Updater and Validator ###########
#######################################################
def update_TT_DATA_A(args):
    AIE_VARIANT = args["AIE_VARIANT"]
    legal_set_TT_DATA_A = ["int16", "cint16", "int32", "cint32", "float", "cfloat"]
    
    # Float types not supported on AIE-ML/MLv2
    if AIE_VARIANT == AIE_ML or AIE_VARIANT == AIE_MLv2:
        legal_set_TT_DATA_A = [t for t in legal_set_TT_DATA_A if t not in ["float", "cfloat"]]
    
    param_dict = {}
    param_dict.update({"name": "TT_DATA_A"})
    param_dict.update({"enum": legal_set_TT_DATA_A})
    return param_dict


def validate_TT_DATA_A(args):
    AIE_VARIANT = args["AIE_VARIANT"]
    TT_DATA_A = args["TT_DATA_A"]
    
    # Get legal set from updater
    legal_set = update_TT_DATA_A(args)["enum"]
    return validate_legal_set(legal_set, "TT_DATA_A", TT_DATA_A)


#######################################################
########### TT_DATA_B Updater and Validator ###########
#######################################################
def update_TT_DATA_B(args):
    TT_DATA_A = args["TT_DATA_A"]
    AIE_VARIANT = args["AIE_VARIANT"]
    legal_set_TT_DATA_B = ["int16", "cint16", "int32", "cint32", "float", "cfloat"]
    int_set = ["int16", "cint16", "int32", "cint32"]
    float_set = ["float", "cfloat"]

    # Type compatibility: int with int, float with float
    if TT_DATA_A in int_set:
        legal_set_TT_DATA_B = [t for t in legal_set_TT_DATA_B if t not in float_set]
    elif TT_DATA_A in float_set:
        legal_set_TT_DATA_B = [t for t in legal_set_TT_DATA_B if t not in int_set]

    # Check which TT_DATA_B types have supported tiling scheme with TT_DATA_A
    legal_set_TT_DATA_B = [t for t in legal_set_TT_DATA_B if getTilingScheme(TT_DATA_A, t, AIE_VARIANT) != (0, 0, 0)]

    param_dict = {}
    param_dict.update({"name": "TT_DATA_B"})
    param_dict.update({"enum": legal_set_TT_DATA_B})
    return param_dict


def validate_TT_DATA_B(args):
    TT_DATA_B = args["TT_DATA_B"]
    
    # Get legal set from updater
    legal_set = update_TT_DATA_B(args)["enum"]
    return validate_legal_set(legal_set, "TT_DATA_B", TT_DATA_B)
    

#######################################################
########### TT_OUT_DATA Updater and Validator ###########
#######################################################
def update_TT_OUT_DATA(args):
    TT_DATA_A = args["TT_DATA_A"]
    TT_DATA_B = args["TT_DATA_B"]
    legal_set_TT_OUT_DATA = ["int16", "cint16", "int32", "cint32", "float", "cfloat"]

    # If A or B complex, out must be complex, else out is real
    if fn_is_complex(TT_DATA_A) or fn_is_complex(TT_DATA_B):
        legal_set_TT_OUT_DATA = [t for t in legal_set_TT_OUT_DATA if fn_is_complex(t)]
    else:
        legal_set_TT_OUT_DATA = [t for t in legal_set_TT_OUT_DATA if not fn_is_complex(t)]
    
    # If A or B float, out must be float, else integer
    if fn_is_floating_point(TT_DATA_A) or fn_is_floating_point(TT_DATA_B):
        legal_set_TT_OUT_DATA = [t for t in legal_set_TT_OUT_DATA if fn_is_floating_point(t)]
    else:
        legal_set_TT_OUT_DATA = [t for t in legal_set_TT_OUT_DATA if not fn_is_floating_point(t)]

    # Remove out types with less precision than the larger input data
    maxBaseType = max(fn_size_by_byte(fn_base_type(TT_DATA_A)), fn_size_by_byte(fn_base_type(TT_DATA_B)))
    legal_set_TT_OUT_DATA = [t for t in legal_set_TT_OUT_DATA if fn_size_by_byte(fn_base_type(t)) >= maxBaseType]

    param_dict = {}
    param_dict.update({"name": "TT_OUT_DATA"})
    param_dict.update({"enum": legal_set_TT_OUT_DATA})
    return param_dict


def validate_TT_OUT_DATA(args):
    TT_OUT_DATA = args["TT_OUT_DATA"]
    
    # Get legal set from updater
    legal_set = update_TT_OUT_DATA(args)["enum"]
    
    return validate_legal_set(legal_set, "TT_OUT_DATA", TT_OUT_DATA)



#######################################################
########### TP_DIM_A Updater and Validator ############
#######################################################
def update_TP_DIM_A(args):
    AIE_VARIANT = args["AIE_VARIANT"]
    TT_DATA_A = args["TT_DATA_A"]
    TT_DATA_B = args["TT_DATA_B"]
    TT_OUT_DATA = args["TT_OUT_DATA"]
    TP_DIM_A_input = int(args.get("TP_DIM_A", 0)) if args.get("TP_DIM_A") else 0

    (tileA, tileAB, tileB) = getTilingScheme(TT_DATA_A, TT_DATA_B, AIE_VARIANT)
    buffer_size = k_data_memory_bytes[AIE_VARIANT]
    max_buffer_sample_in = buffer_size // fn_size_by_byte(TT_DATA_A)
    max_buffer_sample_out = buffer_size // fn_size_by_byte(TT_OUT_DATA)

    TP_SSR = 16
    TP_CASC = 16
    TP_DIM_B = tileB
    TP_DIM_AB = tileAB

    TP_DIM_A_max_buffer_in = (max_buffer_sample_in * TP_SSR * TP_CASC) // TP_DIM_AB
    TP_DIM_A_max_buffer_out = (max_buffer_sample_out * TP_SSR) // TP_DIM_B
    TP_DIM_A_max = int(FLOOR(min(TP_DIM_A_max_buffer_in, TP_DIM_A_max_buffer_out), tileA))

    param_dict = {
        "name": "TP_DIM_A",
        "minimum": tileA,
        "maximum": TP_DIM_A_max,
    }

    TP_DIM_A_act = com.CLIP(TP_DIM_A_input, param_dict["minimum"], param_dict["maximum"])
    TP_DIM_A_act = com.ROUND_TO_NEAREST_MULTIPLE(TP_DIM_A_act, tileA)
    param_dict.update({"actual": int(TP_DIM_A_act)})

    return param_dict


def validate_TP_DIM_A(args):
    AIE_VARIANT = args["AIE_VARIANT"]
    TT_DATA_A = args["TT_DATA_A"]
    TT_DATA_B = args["TT_DATA_B"]
    TT_OUT_DATA = args["TT_OUT_DATA"]
    TP_DIM_A = args["TP_DIM_A"]
    
    (tileA, tileAB, tileB) = getTilingScheme(TT_DATA_A, TT_DATA_B, AIE_VARIANT)
    
    # Check tiling alignment
    if TP_DIM_A % tileA != 0:
        return isError(f"TP_DIM_A ({TP_DIM_A}) must be a multiple of tileA ({tileA})!")
    
    # Check buffer size constraints
    buffer_size = k_data_memory_bytes[AIE_VARIANT]
    max_buffer_sample_in = buffer_size // fn_size_by_byte(TT_DATA_A)
    max_buffer_sample_out = buffer_size // fn_size_by_byte(TT_OUT_DATA)
    
    TP_SSR_max_val = 16
    TP_CASC_max_val = 16
    TP_DIM_B_min = tileB
    TP_DIM_AB_min = tileAB
    
    TP_DIM_A_max_buffer_in = (max_buffer_sample_in * TP_SSR_max_val * TP_CASC_max_val) // TP_DIM_AB_min
    TP_DIM_A_max_buffer_out = (max_buffer_sample_out * TP_SSR_max_val) // TP_DIM_B_min
    TP_DIM_A_max = min(TP_DIM_A_max_buffer_in, TP_DIM_A_max_buffer_out)
    
    if TP_DIM_A < tileA:
        return isError(f"TP_DIM_A ({TP_DIM_A}) must be at least {tileA}!")
    if TP_DIM_A > TP_DIM_A_max:
        return isError(f"TP_DIM_A ({TP_DIM_A}) exceeds maximum ({int(TP_DIM_A_max)}) based on buffer size constraints!")
    
    return isValid


#######################################################
########### TP_DIM_B Updater and Validator ############
#######################################################
def update_TP_DIM_B(args):
    AIE_VARIANT = args["AIE_VARIANT"]
    TT_DATA_A = args["TT_DATA_A"]
    TT_DATA_B = args["TT_DATA_B"]
    TP_DIM_A = int(args["TP_DIM_A"])
    TT_OUT_DATA = args["TT_OUT_DATA"]
    TP_DIM_B_input = int(args.get("TP_DIM_B", 0)) if args.get("TP_DIM_B") else 0

    (tileA, tileAB, tileB) = getTilingScheme(TT_DATA_A, TT_DATA_B, AIE_VARIANT)
    buffer_size = k_data_memory_bytes[AIE_VARIANT]
    max_buffer_sample_in = buffer_size // fn_size_by_byte(TT_DATA_B)
    max_buffer_sample_out = buffer_size // fn_size_by_byte(TT_OUT_DATA)

    TP_CASC = 16
    TP_SSR = min(TP_SSR_max, TP_DIM_A // tileA)
    TP_DIM_AB = tileAB
    TP_DIM_B_max_in = (max_buffer_sample_in * TP_CASC) // TP_DIM_AB
    TP_DIM_B_max_out = (max_buffer_sample_out * TP_SSR) // TP_DIM_A
    TP_DIM_B_max = int(FLOOR(min(TP_DIM_B_max_in, TP_DIM_B_max_out), tileB))

    param_dict = {
        "name": "TP_DIM_B",
        "minimum": tileB,
        "maximum": TP_DIM_B_max,
    }

    TP_DIM_B_act = com.CLIP(TP_DIM_B_input, param_dict["minimum"], param_dict["maximum"])
    TP_DIM_B_act = com.ROUND_TO_NEAREST_MULTIPLE(TP_DIM_B_act, tileB)
    param_dict.update({"actual": int(TP_DIM_B_act)})

    return param_dict


def validate_TP_DIM_B(args):
    AIE_VARIANT = args["AIE_VARIANT"]
    TT_DATA_A = args["TT_DATA_A"]
    TT_DATA_B = args["TT_DATA_B"]
    TT_OUT_DATA = args["TT_OUT_DATA"]
    TP_DIM_A = args["TP_DIM_A"]
    TP_DIM_B = args["TP_DIM_B"]
    
    (tileA, tileAB, tileB) = getTilingScheme(TT_DATA_A, TT_DATA_B, AIE_VARIANT)
    
    # Check tiling alignment
    if TP_DIM_B % tileB != 0:
        return isError(f"TP_DIM_B ({TP_DIM_B}) must be a multiple of tileB ({tileB})!")
    
    # Check buffer size constraints
    buffer_size = k_data_memory_bytes[AIE_VARIANT]
    max_buffer_sample_in = buffer_size // fn_size_by_byte(TT_DATA_B)
    max_buffer_sample_out = buffer_size // fn_size_by_byte(TT_OUT_DATA)
    
    TP_CASC_max_val = 16
    TP_SSR_max_val = min(TP_SSR_max, TP_DIM_A // tileA)
    TP_DIM_AB_min = tileAB

    TP_DIM_B_max_in = (max_buffer_sample_in * TP_CASC_max_val) // TP_DIM_AB_min
    TP_DIM_B_max_out = (max_buffer_sample_out * TP_SSR_max_val) // TP_DIM_A
    TP_DIM_B_max = FLOOR(min(TP_DIM_B_max_in, TP_DIM_B_max_out), tileB)
    
    if TP_DIM_B < tileB:
        return isError(f"TP_DIM_B ({TP_DIM_B}) must be at least {tileB}!")
    if TP_DIM_B > TP_DIM_B_max:
        return isError(f"TP_DIM_B ({TP_DIM_B}) exceeds maximum ({int(TP_DIM_B_max)}) based on buffer size constraints!")
    
    return isValid


#######################################################
########### TP_DIM_AB Updater and Validator ###########
#######################################################
def update_TP_DIM_AB(args):
    AIE_VARIANT = args["AIE_VARIANT"]
    TT_DATA_A = args["TT_DATA_A"]
    TT_DATA_B = args["TT_DATA_B"]
    TP_DIM_A = int(args["TP_DIM_A"])
    TP_DIM_B = int(args["TP_DIM_B"])
    TP_DIM_AB_input = int(args.get("TP_DIM_AB", 0)) if args.get("TP_DIM_AB") else 0

    (tileA, tileAB, tileB) = getTilingScheme(TT_DATA_A, TT_DATA_B, AIE_VARIANT)
    buffer_size = k_data_memory_bytes[AIE_VARIANT]
    max_buffer_sample_a = buffer_size // fn_size_by_byte(TT_DATA_A)
    max_buffer_sample_b = buffer_size // fn_size_by_byte(TT_DATA_B)

    TP_SSR = 16
    TP_CASC = 16
    TP_DIM_AB_max1 = (max_buffer_sample_a * TP_SSR * TP_CASC) // TP_DIM_A
    TP_DIM_AB_max2 = (max_buffer_sample_b * TP_CASC) // TP_DIM_B
    TP_DIM_AB_max = int(FLOOR(min(TP_DIM_AB_max1, TP_DIM_AB_max2), tileAB))

    param_dict = {
        "name": "TP_DIM_AB",
        "minimum": tileAB,
        "maximum": TP_DIM_AB_max,
    }

    TP_DIM_AB_act = com.CLIP(TP_DIM_AB_input, param_dict["minimum"], param_dict["maximum"])
    TP_DIM_AB_act = com.ROUND_TO_NEAREST_MULTIPLE(TP_DIM_AB_act, tileAB)
    param_dict.update({"actual": int(TP_DIM_AB_act)})

    return param_dict


def validate_TP_DIM_AB(args):
    AIE_VARIANT = args["AIE_VARIANT"]
    TT_DATA_A = args["TT_DATA_A"]
    TT_DATA_B = args["TT_DATA_B"]
    TP_DIM_A = args["TP_DIM_A"]
    TP_DIM_B = args["TP_DIM_B"]
    TP_DIM_AB = args["TP_DIM_AB"]
    
    (tileA, tileAB, tileB) = getTilingScheme(TT_DATA_A, TT_DATA_B, AIE_VARIANT)
    
    # Check tiling alignment
    if TP_DIM_AB % tileAB != 0:
        return isError(f"TP_DIM_AB ({TP_DIM_AB}) must be a multiple of tileAB ({tileAB})!")
    
    # Check buffer size constraints
    buffer_size = k_data_memory_bytes[AIE_VARIANT]
    max_buffer_sample_a = buffer_size // fn_size_by_byte(TT_DATA_A)
    max_buffer_sample_b = buffer_size // fn_size_by_byte(TT_DATA_B)
    
    TP_SSR_max_val = 16
    TP_CASC_max_val = 16
    
    TP_DIM_AB_max1 = (max_buffer_sample_a * TP_SSR_max_val * TP_CASC_max_val) // TP_DIM_A
    TP_DIM_AB_max2 = (max_buffer_sample_b * TP_CASC_max_val) // TP_DIM_B
    TP_DIM_AB_max = FLOOR(min(TP_DIM_AB_max1, TP_DIM_AB_max2), tileAB)
    
    if TP_DIM_AB < tileAB:
        return isError(f"TP_DIM_AB ({TP_DIM_AB}) must be at least {tileAB}!")
    if TP_DIM_AB > TP_DIM_AB_max:
        return isError(f"TP_DIM_AB ({TP_DIM_AB}) exceeds maximum ({int(TP_DIM_AB_max)}) based on buffer size constraints!")
    
    return isValid


#######################################################
############ TP_SSR Updater and Validator #############
#######################################################
def update_TP_SSR(args):
    AIE_VARIANT = args["AIE_VARIANT"]
    TT_DATA_A = args["TT_DATA_A"]
    TT_DATA_B = args["TT_DATA_B"]
    TT_OUT_DATA = args["TT_OUT_DATA"]
    TP_DIM_A = args["TP_DIM_A"]
    TP_DIM_AB = args["TP_DIM_AB"]
    TP_DIM_B = args["TP_DIM_B"]
    TP_SSR = args["TP_SSR"] if "TP_SSR" in args and args["TP_SSR"] else 0

    (tileA, tileAB, tileB) = getTilingScheme(TT_DATA_A, TT_DATA_B, AIE_VARIANT)
    buffer_size = k_data_memory_bytes[AIE_VARIANT]
    
    # Calculate max possible cascade length based on actual TP_DIM_AB
    max_casc_for_dim_ab = min(TP_CASC_max, TP_DIM_AB // tileAB)
    
    # Calculate buffer constraints - use max_casc_for_dim_ab since more splits allow larger dimensions
    ssrMinForA = max(1, (fn_size_by_byte(TT_DATA_A) * TP_DIM_A * TP_DIM_AB + (max_casc_for_dim_ab * buffer_size) - 1) // (max_casc_for_dim_ab * buffer_size))
    ssrMinForOut = max(1, (fn_size_by_byte(TT_OUT_DATA) * TP_DIM_A * TP_DIM_B + buffer_size - 1) // buffer_size)
    minSsr = max(ssrMinForA, ssrMinForOut)
    maxSsr = min(TP_SSR_max, TP_DIM_A // tileA)

    # Build legal set using find_divisors
    legal_set_TP_SSR = find_divisors(TP_DIM_A, maxSsr)
    
    # Filter based on constraints
    legal_set_TP_SSR = [k for k in legal_set_TP_SSR 
                        if k >= minSsr and k <= maxSsr 
                        and (TP_DIM_A // k) % tileA == 0]
    
    if not legal_set_TP_SSR:
        return False    # failure state

    param_dict = {"name": "TP_SSR", "enum": legal_set_TP_SSR, "actual": com.GET_CLOSEST(TP_SSR, legal_set_TP_SSR)}
    return param_dict


def validate_TP_SSR(args):
    TP_SSR = args["TP_SSR"]
    result = update_TP_SSR(args)
    legal_set = result.get("enum", []) if result else []
    return validate_legal_set(legal_set, "TP_SSR", TP_SSR)


#######################################################
############ TP_CASC_LEN Updater and Validator ########
#######################################################
def update_TP_CASC_LEN(args):
    AIE_VARIANT = args["AIE_VARIANT"]
    TT_DATA_A = args["TT_DATA_A"]
    TT_DATA_B = args["TT_DATA_B"]
    TP_DIM_A = args["TP_DIM_A"]
    TP_DIM_AB = args["TP_DIM_AB"]
    TP_DIM_B = args["TP_DIM_B"]
    TP_SSR = args["TP_SSR"]
    TP_CASC_LEN = args["TP_CASC_LEN"] if "TP_CASC_LEN" in args and args["TP_CASC_LEN"] else 0

    (tileA, tileAB, tileB) = getTilingScheme(TT_DATA_A, TT_DATA_B, AIE_VARIANT)
    buffer_size = k_data_memory_bytes[AIE_VARIANT]
    
    # Calculate buffer constraints
    cascMinForA = max(1, (fn_size_by_byte(TT_DATA_A) * TP_DIM_A * TP_DIM_AB + (TP_SSR * buffer_size) - 1) // (TP_SSR * buffer_size))
    cascMinForB = max(1, (fn_size_by_byte(TT_DATA_B) * TP_DIM_B * TP_DIM_AB + buffer_size - 1) // buffer_size)
    minCasc = max(cascMinForA, cascMinForB)
    
    # maxCasc is limited by TP_DIM_AB / tileAB (can't split more than this)
    maxCasc = min(TP_CASC_max, TP_DIM_AB // tileAB)

    # Build legal set using find_divisors - use maxCasc based on actual TP_DIM_AB
    legal_set_TP_CASC = find_divisors(TP_DIM_AB, maxCasc)
    
    # Filter based on constraints
    legal_set_TP_CASC = [k for k in legal_set_TP_CASC 
                         if k >= minCasc and k <= maxCasc 
                         and (TP_DIM_AB // k) % tileAB == 0]
    
    if not legal_set_TP_CASC:
        return False    # failure state
    
    param_dict = {"name": "TP_CASC_LEN", "enum": legal_set_TP_CASC, "actual": com.GET_CLOSEST(TP_CASC_LEN, legal_set_TP_CASC)}
    return param_dict


def validate_TP_CASC_LEN(args):
    TP_CASC_LEN = args["TP_CASC_LEN"]
    result = update_TP_CASC_LEN(args)
    legal_set = result.get("enum", []) if result else []
    return validate_legal_set(legal_set, "TP_CASC_LEN", TP_CASC_LEN)


#######################################################
### TP_INPUT_WINDOW_VSIZE_A Updater and Validator #####
#######################################################
def update_TP_INPUT_WINDOW_VSIZE_A(args):
    TP_DIM_A = args["TP_DIM_A"]
    TP_DIM_AB = args["TP_DIM_AB"]
    TP_INPUT_WINDOW_VSIZE_A_min = int(TP_DIM_A * TP_DIM_AB)
    param_dict = {
        "name": "TP_INPUT_WINDOW_VSIZE_A",
        "minimum": TP_INPUT_WINDOW_VSIZE_A_min,
        "maximum": TP_INPUT_WINDOW_VSIZE_A_min,
    }
    return param_dict


def validate_TP_INPUT_WINDOW_VSIZE_A(args):
    TP_DIM_A = args["TP_DIM_A"]
    TP_DIM_AB = args["TP_DIM_AB"]
    TP_INPUT_WINDOW_VSIZE_A = args["TP_INPUT_WINDOW_VSIZE_A"]

    # Check that window size is an integer multiple of matrix size
    matrix_size = TP_DIM_A * TP_DIM_AB
    if TP_INPUT_WINDOW_VSIZE_A % matrix_size != 0:
        return isError(f"TP_INPUT_WINDOW_VSIZE_A ({TP_INPUT_WINDOW_VSIZE_A}) must be an integer multiple of TP_DIM_A*TP_DIM_AB ({matrix_size})!")
    
    # For single matrix per window, must be exactly equal
    if TP_INPUT_WINDOW_VSIZE_A != matrix_size:
        return isError(f"TP_INPUT_WINDOW_VSIZE_A ({TP_INPUT_WINDOW_VSIZE_A}) must equal TP_DIM_A*TP_DIM_AB ({matrix_size}) for single matrix per window!")

    return isValid


#######################################################
### TP_INPUT_WINDOW_VSIZE_B Updater and Validator #####
#######################################################
def update_TP_INPUT_WINDOW_VSIZE_B(args):
    TP_DIM_B = args["TP_DIM_B"]
    TP_DIM_AB = args["TP_DIM_AB"]
    TP_INPUT_WINDOW_VSIZE_B_min = int(TP_DIM_B * TP_DIM_AB)
    param_dict = {
        "name": "TP_INPUT_WINDOW_VSIZE_B",
        "minimum": TP_INPUT_WINDOW_VSIZE_B_min,
        "maximum": TP_INPUT_WINDOW_VSIZE_B_min,
    }
    return param_dict


def validate_TP_INPUT_WINDOW_VSIZE_B(args):
    TP_INPUT_WINDOW_VSIZE_B = args["TP_INPUT_WINDOW_VSIZE_B"]
    TP_DIM_AB = args["TP_DIM_AB"]
    TP_DIM_B = args["TP_DIM_B"]

    # Check that window size is an integer multiple of matrix size
    matrix_size = TP_DIM_B * TP_DIM_AB
    if TP_INPUT_WINDOW_VSIZE_B % matrix_size != 0:
        return isError(f"TP_INPUT_WINDOW_VSIZE_B ({TP_INPUT_WINDOW_VSIZE_B}) must be an integer multiple of TP_DIM_B*TP_DIM_AB ({matrix_size})!")
    
    # For single matrix per window, must be exactly equal
    if TP_INPUT_WINDOW_VSIZE_B != matrix_size:
        return isError(f"TP_INPUT_WINDOW_VSIZE_B ({TP_INPUT_WINDOW_VSIZE_B}) must equal TP_DIM_B*TP_DIM_AB ({matrix_size}) for single matrix per window!")

    return isValid


#######################################################
######### TP_DIM_A_LEADING Updater and Validator ######
#######################################################
def update_TP_DIM_A_LEADING(args):
    legal_set_TP_DIM_A_LEADING = [0, 1]
    param_dict = {}
    param_dict.update({"name": "TP_DIM_A_LEADING"})
    param_dict.update({"enum": legal_set_TP_DIM_A_LEADING})
    return param_dict


def validate_TP_DIM_A_LEADING(args):
    TP_DIM_A_LEADING = args["TP_DIM_A_LEADING"]
    
    # Get legal set from updater
    legal_set = update_TP_DIM_A_LEADING(args)["enum"]
    return validate_legal_set(legal_set, "TP_DIM_A_LEADING", TP_DIM_A_LEADING)


#######################################################
######### TP_DIM_B_LEADING Updater and Validator ######
#######################################################
def update_TP_DIM_B_LEADING(args):
    legal_set_TP_DIM_B_LEADING = [0, 1]
    param_dict = {}
    param_dict.update({"name": "TP_DIM_B_LEADING"})
    param_dict.update({"enum": legal_set_TP_DIM_B_LEADING})
    return param_dict


def validate_TP_DIM_B_LEADING(args):
    TP_DIM_B_LEADING = args["TP_DIM_B_LEADING"]
    # Get legal set from updater
    legal_set = update_TP_DIM_B_LEADING(args)["enum"]
    return validate_legal_set(legal_set, "TP_DIM_B_LEADING", TP_DIM_B_LEADING)



#######################################################
######### TP_DIM_OUT_LEADING Updater and Validator ####
#######################################################
def update_TP_DIM_OUT_LEADING(args):
    legal_set_TP_DIM_OUT_LEADING = [0, 1]
    param_dict = {}
    param_dict.update({"name": "TP_DIM_OUT_LEADING"})
    param_dict.update({"enum": legal_set_TP_DIM_OUT_LEADING})
    return param_dict


def validate_TP_DIM_OUT_LEADING(args):
    TP_DIM_OUT_LEADING = args["TP_DIM_OUT_LEADING"]
    
    # Get legal set from updater
    legal_set = update_TP_DIM_OUT_LEADING(args)["enum"]
    
    return validate_legal_set(legal_set, "TP_DIM_OUT_LEADING", TP_DIM_OUT_LEADING)


#######################################################
######## TP_ADD_TILING_A Updater and Validator ########
#######################################################
def update_TP_ADD_TILING_A(args):
    AIE_VARIANT = args["AIE_VARIANT"]
    TT_DATA_A = args["TT_DATA_A"]
    TP_DIM_A = args["TP_DIM_A"]
    TP_DIM_AB = args["TP_DIM_AB"]
    TP_SSR = args["TP_SSR"]
    TP_CASC_LEN = args["TP_CASC_LEN"]
    TP_DIM_A_LEADING = args["TP_DIM_A_LEADING"]
    legal_set_TP_ADD_TILING_A = [0, 1]
    
    # Tiling not supported on AIE-MLv2
    if AIE_VARIANT == com.AIE_MLv2:
        legal_set_TP_ADD_TILING_A = [0]
    else:
        # Check matrix size constraints
        majorDim = TP_DIM_A // TP_SSR
        commonDim = TP_DIM_AB // TP_CASC_LEN
        matrixSize = fn_size_by_byte(TT_DATA_A) * majorDim * commonDim
        
        # Tiling not needed for small matrices or specific configurations
        if ((TP_DIM_A_LEADING == 1) and (TT_DATA_A == "int16")) or (matrixSize < 512 // 8):
            legal_set_TP_ADD_TILING_A = [0]

    param_dict = {}
    param_dict.update({"name": "TP_ADD_TILING_A"})
    param_dict.update({"enum": legal_set_TP_ADD_TILING_A})
    return param_dict


def validate_TP_ADD_TILING_A(args):
    TP_ADD_TILING_A = args["TP_ADD_TILING_A"]
    
    # Get legal set from updater
    legal_set = update_TP_ADD_TILING_A(args)["enum"]
    return validate_legal_set(legal_set, "TP_ADD_TILING_A", TP_ADD_TILING_A)



#######################################################
######## TP_ADD_TILING_B Updater and Validator ########
#######################################################
def update_TP_ADD_TILING_B(args):
    AIE_VARIANT = args["AIE_VARIANT"]
    TT_DATA_B = args["TT_DATA_B"]
    TP_DIM_B = args["TP_DIM_B"]
    TP_DIM_AB = args["TP_DIM_AB"]
    TP_CASC_LEN = args["TP_CASC_LEN"]
    TP_DIM_B_LEADING = args["TP_DIM_B_LEADING"]
    legal_set_TP_ADD_TILING_B = [0, 1]
    
    # Tiling not supported on AIE-MLv2
    if AIE_VARIANT == com.AIE_MLv2:
        legal_set_TP_ADD_TILING_B = [0]
    else:
        # Check matrix size constraints
        majorDim = TP_DIM_B
        commonDim = TP_DIM_AB // TP_CASC_LEN
        matrixSize = fn_size_by_byte(TT_DATA_B) * majorDim * commonDim
        
        # Tiling not needed for small matrices or specific configurations
        if ((TP_DIM_B_LEADING == 1) and (TT_DATA_B == "int16")) or (matrixSize < 512 // 8):
            legal_set_TP_ADD_TILING_B = [0]

    param_dict = {}
    param_dict.update({"name": "TP_ADD_TILING_B"})
    param_dict.update({"enum": legal_set_TP_ADD_TILING_B})
    return param_dict


def validate_TP_ADD_TILING_B(args):
    TP_ADD_TILING_B = args["TP_ADD_TILING_B"]
    
    # Get legal set from updater
    legal_set = update_TP_ADD_TILING_B(args)["enum"]    
    return validate_legal_set(legal_set, "TP_ADD_TILING_B", TP_ADD_TILING_B)


#######################################################
####### TP_ADD_DETILING_OUT Updater and Validator #####
#######################################################
def update_TP_ADD_DETILING_OUT(args):
    AIE_VARIANT = args["AIE_VARIANT"]
    TT_DATA_A = args["TT_DATA_A"]
    TT_DATA_B = args["TT_DATA_B"]
    TT_OUT_DATA = args["TT_OUT_DATA"]
    TP_DIM_A = args["TP_DIM_A"]
    TP_DIM_B = args["TP_DIM_B"]
    TP_SSR = args["TP_SSR"]
    TP_DIM_OUT_LEADING = args["TP_DIM_OUT_LEADING"]
    legal_set_TP_ADD_DETILING_OUT = [0, 1]
    
    # Detiling not supported on AIE-MLv2
    if AIE_VARIANT == com.AIE_MLv2:
        legal_set_TP_ADD_DETILING_OUT = [0]
    else:
        # Check matrix size constraints
        majorDim = TP_DIM_A // TP_SSR
        commonDim = TP_DIM_B
        matrixSize = fn_size_by_byte(TT_OUT_DATA) * majorDim * commonDim
        
        # Detiling not needed for small matrices or specific configurations
        if ((TP_DIM_OUT_LEADING == 1) and (TT_OUT_DATA == "int16")) or (matrixSize < 512 // 8):
            legal_set_TP_ADD_DETILING_OUT = [0]

    param_dict = {}
    param_dict.update({"name": "TP_ADD_DETILING_OUT"})
    param_dict.update({"enum": legal_set_TP_ADD_DETILING_OUT})
    return param_dict


def validate_TP_ADD_DETILING_OUT(args):
    TP_ADD_DETILING_OUT = args["TP_ADD_DETILING_OUT"]
    
    # Get legal set from updater
    legal_set = update_TP_ADD_DETILING_OUT(args)["enum"]  
    return validate_legal_set(legal_set, "TP_ADD_DETILING_OUT", TP_ADD_DETILING_OUT)

#######################################################
############## TP_SHIFT Updater and Validator #########
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
    return param_dict


def validate_TP_SHIFT(args):
    TT_DATA = args["TT_DATA_A"]
    AIE_VARIANT = args["AIE_VARIANT"]
    if "TP_SHIFT" not in args:
        return isValid
    TP_SHIFT = args["TP_SHIFT"]
    
    # Check that TP_SHIFT is 0 for float types
    if fn_is_floating_point(TT_DATA) and TP_SHIFT != 0:
        return isError(f"TP_SHIFT must be 0 for floating point types (TT_DATA_A={TT_DATA}), but got TP_SHIFT={TP_SHIFT}!")
    
    # Get range based on AIE variant and data type
    range_TP_SHIFT = com.fn_update_range_TP_SHIFT(AIE_VARIANT, TT_DATA)
    return validate_range(range_TP_SHIFT, "TP_SHIFT", TP_SHIFT)



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
############ TP_SAT Updater and Validator #############
#######################################################
def update_TP_SAT(args):
    legal_set_sat = fn_legal_set_sat()
    param_dict = {}
    param_dict.update({"name": "TP_SAT"})
    param_dict.update({"enum": legal_set_sat})
    return param_dict


def validate_TP_SAT(args):
    TP_SAT = args["TP_SAT"]
    return fn_validate_satMode(TP_SAT)


#######################################################
################## Helper Functions ###################
#######################################################
def getTilingScheme(typeA, typeB, AIE_VARIANT):
    # needs to be compatible with c++14 -> so just use plain ifs
    # 16b x 16b
    if AIE_VARIANT == AIE:
        if typeA == "int16" and typeB == "int16":
            return (4, 4, 4)
        # 32b x 16b
        if (typeA == "cint16" or typeA == "int32") and typeB == "int16":
            return (4, 4, 2)
        # 16b x 32b
        if typeA == "int16" and (typeB == "cint16" or typeB == "int32"):
            return (4, 2, 2)
        # 32b x 32b
        if (
            (
                (typeA == "cint16" or typeA == "int32")
                and (typeB == "cint16" or typeB == "int32")
            )
            or typeA == "float"
            and typeB == "float"
        ):
            return (4, 4, 2)
        # 64b x 16b
        if typeA == "cint32" and typeB == "int16":
            return (2, 4, 2)
        # 16b x 64b
        if typeA == "int16" and typeB == "cint32":
            return (2, 4, 2)
        # 64b x 32b
        if typeA == "cint32" and (typeB == "cint16" or typeB == "int32"):
            return (2, 2, 2)
        # 32b x 64b
        if (typeA == "cint16" or typeA == "int32") and typeB == "cint32":
            return (2, 2, 2)
        # 64b x 64b
        if typeA == "cint32" and typeB == "cint32":
            return (2, 2, 2)
        # Mixed Floats
        if (typeA == "cfloat" and typeB == "float") or (typeA == "float" and typeB == "cfloat"):
            return (2, 4, 2)
        # cfloats
        if typeA == "cfloat" and typeB == "cfloat":
            return (4, 2, 2)
    if AIE_VARIANT == AIE_ML:
        if (typeA == "int16" or typeA == "int32") and (typeB == "int16" or typeB == "int32"):
            return (4, 4, 4)
        if typeA == "cint16" and typeB == "int16":
            return (4, 4, 4)
        if typeA == "cint16" and typeB == "cint16":
            return (1, 4, 8)
        if typeA == "cint32" and typeB == "cint16":
            return (2, 4, 8)
        if typeA == "cint32" and typeB == "cint32":
            return (1, 2, 8)
    if AIE_VARIANT == AIE_MLv2:
        if typeA == "int8" and typeB == "int8":
            return (4, 8, 8)
        if typeA == "int16" and typeB == "int8":
            return (4, 4, 8)
        if typeA == "int16" and typeB == "int16":
            return (4, 4, 8)
        if typeA == "int32" and typeB == "int16":
            return (4, 2, 8)
        if typeA == "int16" and typeB == "int32":
            return (4, 4, 8)
        if typeA == "int32" and typeB == "int32":
            return (4, 2, 8)
        if typeA == "bfloat16" and typeB == "bfloat16":
            return (4, 8, 8)
        if typeA == "float" and typeB == "float":
            return (4, 8, 4)
        if typeA == "cint16" and typeB == "int16":
            return (4, 4, 8)
        if typeA == "cint16" and typeB == "cint16":
            return (1, 4, 8)
        if typeA == "cint32" and typeB == "cint16":
            return (1, 2, 8)
        if typeA == "cint32" and typeB == "cint32":
            return (1, 2, 8)
    return (0, 0, 0)


#######################################################
################## Port Info ##########################
#######################################################
def info_ports(args):
    portsA = com.get_port_info(
        portname="inA",
        dir="in",
        TT_DATA=args["TT_DATA_A"],
        windowVSize=(args["TP_INPUT_WINDOW_VSIZE_A"] // (args["TP_SSR"] * args["TP_CASC_LEN"])),
        vectorLength=(args["TP_CASC_LEN"] * args["TP_SSR"]),
    )

    portsB = com.get_port_info(
        portname="inB",
        dir="in",
        TT_DATA=args["TT_DATA_B"],
        windowVSize=(args["TP_INPUT_WINDOW_VSIZE_B"] // args["TP_CASC_LEN"]),
        vectorLength=(args["TP_CASC_LEN"] * args["TP_SSR"]),
    )
    
    TP_OUTPUT_WINDOW_VSIZE = (args["TP_DIM_A"] // args["TP_SSR"]) * args["TP_DIM_B"]
    portsOut = com.get_port_info(
        portname="out",
        dir="out",
        TT_DATA=args["TT_OUT_DATA"],
        windowVSize=TP_OUTPUT_WINDOW_VSIZE,
        vectorLength=args["TP_SSR"],
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
    TP_DIM_AB = args["TP_DIM_AB"]
    TP_DIM_B = args["TP_DIM_B"]
    TP_SHIFT = args["TP_SHIFT"]
    TP_RND = args["TP_RND"]
    TP_SAT = args["TP_SAT"]
    TP_DIM_A_LEADING = args["TP_DIM_A_LEADING"]
    TP_DIM_B_LEADING = args["TP_DIM_B_LEADING"]
    TP_DIM_OUT_LEADING = args["TP_DIM_OUT_LEADING"]
    TP_ADD_TILING_A = args["TP_ADD_TILING_A"]
    TP_ADD_TILING_B = args["TP_ADD_TILING_B"]
    TP_ADD_DETILING_OUT = args["TP_ADD_DETILING_OUT"]
    TP_INPUT_WINDOW_VSIZE_A = args["TP_INPUT_WINDOW_VSIZE_A"]
    TP_INPUT_WINDOW_VSIZE_B = args["TP_INPUT_WINDOW_VSIZE_B"]
    TP_CASC_LEN = args["TP_CASC_LEN"]
    TP_SSR = args["TP_SSR"]
    TT_OUT_DATA = args["TT_OUT_DATA"]

    code = f"""
class {graphname} : public adf::graph {{
public:
  std::array<adf::port<input>,{TP_CASC_LEN} * {TP_SSR}> inA;
  std::array<adf::port<input>,{TP_CASC_LEN} * {TP_SSR}> inB;
  std::array<adf::port<output>, {TP_SSR}> out;
  xf::dsp::aie::blas::matrix_mult::matrix_mult_graph<
    {TT_DATA_A}, // TT_DATA_A
    {TT_DATA_B}, // TT_DATA_B
    {TP_DIM_A}, // TP_DIM_A
    {TP_DIM_AB}, // TP_DIM_AB
    {TP_DIM_B}, // TP_DIM_B
    {TP_SHIFT}, // TP_SHIFT
    {TP_RND}, // TP_RND
    {TP_DIM_A_LEADING}, // TP_DIM_A_LEADING
    {TP_DIM_B_LEADING}, // TP_DIM_B_LEADING
    {TP_DIM_OUT_LEADING}, // TP_DIM_OUT_LEADING
    {TP_ADD_TILING_A}, // TP_ADD_TILING_A
    {TP_ADD_TILING_B}, // TP_ADD_TILING_B
    {TP_ADD_DETILING_OUT}, // TP_ADD_DETILING_OUT
    {TP_INPUT_WINDOW_VSIZE_A}, // TP_INPUT_WINDOW_VSIZE_A
    {TP_INPUT_WINDOW_VSIZE_B}, // TP_INPUT_WINDOW_VSIZE_B
    {TP_CASC_LEN}, // TP_CASC_LEN
    {TP_SAT}, // TP_SAT
    {TP_SSR}, // TP_SSR
    {TT_OUT_DATA} // TT_OUT_DATA
  > mmult;

  {graphname}() : mmult() {{
    adf::kernel *mmult_kernels = mmult.getKernels();
    for (int ssrIdx = 0; ssrIdx < {TP_SSR}; ssrIdx++) {{ 
        for (int cascIdx =0; cascIdx < {TP_CASC_LEN}; cascIdx++) {{
            adf::connect<> net_inA(inA[cascIdx + ssrIdx * {TP_CASC_LEN}], mmult.inA[cascIdx + ssrIdx * {TP_CASC_LEN}]);
            adf::connect<> net_inB(inB[cascIdx + ssrIdx * {TP_CASC_LEN}], mmult.inB[cascIdx + ssrIdx * {TP_CASC_LEN}]);
        }}
        adf::connect<> net_out(mmult.out[ssrIdx], out[ssrIdx]);
    }}
  }}

}};
"""
    out = {}
    out["graph"] = code
    out["port_info"] = info_ports(args)
    out["headerfile"] = "matrix_mult_graph.hpp"
    out["searchpaths"] = ["L1/include/aie", "L2/include/aie", "L1/src/aie"]
    return out
