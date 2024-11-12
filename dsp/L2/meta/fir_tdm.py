#
# Copyright (C) 2019-2022, Xilinx, Inc.
# Copyright (C) 2022-2024, Advanced Micro Devices, Inc.
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
from aie_common import *
from aie_common_fir import *
from aie_common_fir_updaters import *


TP_INPUT_WINDOW_VSIZE_min = 4
TP_CASC_LEN_min = 1
TP_CASC_LEN_max = 40
TP_FIR_LEN_min = 4
TP_FIR_LEN_max = 128
TP_TDM_CHANNELS_min = 1
TP_TDM_CHANNELS_max = 8192
TP_SHIFT_min = 0
TP_SHIFT_max = 61
TP_SSR_min=1
TP_SSR_max=64

#######################################################
########### AIE_VARIANT Updater and Validator #########
#######################################################
def update_AIE_VARIANT(args):
  return fn_update_AIE_VARIANT()

def fn_update_AIE_VARIANT():
  legal_set_AIE_VARIANT = [1, 2]
  
  param_dict ={}
  param_dict.update({"name" : "AIE_VARIANT"})
  param_dict.update({"enum" : legal_set_AIE_VARIANT})
  return param_dict

def validate_AIE_VARIANT(args):
  AIE_VARIANT=args["AIE_VARIANT"]
  return (fn_validate_AIE_VARIANT(AIE_VARIANT))

def fn_validate_AIE_VARIANT(AIE_VARIANT):
  param_dict = fn_update_AIE_VARIANT()
  legal_set_AIE_VARIANT = param_dict["enum"]
  return(validate_legal_set(legal_set_AIE_VARIANT, "AIE_VARIANT", AIE_VARIANT))

#######################################################
########### TT_DATA Updater and Validator #############
#######################################################
def update_TT_DATA(args):
  AIE_VARIANT=args["AIE_VARIANT"]
  return fn_update_tt_data(AIE_VARIANT)
   
def validate_TT_DATA(args):
  AIE_VARIANT=args["AIE_VARIANT"]
  TT_DATA=args["TT_DATA"]
  return (fn_validate_TT_DATA(AIE_VARIANT, TT_DATA))

def fn_validate_TT_DATA(AIE_VARIANT, TT_DATA):
  param_dict = fn_update_tt_data(AIE_VARIANT)
  legal_set_TT_DATA = param_dict["enum"]
  return(validate_legal_set(legal_set_TT_DATA, "TT_DATA", TT_DATA))

#######################################################
######### TT_OUT_DATA Updater and Validator ###########
#######################################################
def update_TT_OUT_DATA(args):
    TT_DATA = args["TT_DATA"]
    return fn_update_TT_OUT_DATA(TT_DATA)


def fn_update_TT_OUT_DATA(TT_DATA):
    legal_set_TT_OUT_DATA=["int16", "cint16", "int32", "cint32", "float", "cfloat"]
    legal_set_TT_OUT_DATA=fn_greater_precision_data_in_out_update(TT_DATA, legal_set_TT_OUT_DATA.copy())
    legal_set_TT_OUT_DATA=fn_float_check_update(TT_DATA, legal_set_TT_OUT_DATA.copy())
    legal_set_TT_OUT_DATA=fn_complex_check_update(TT_DATA, legal_set_TT_OUT_DATA.copy())
    legal_set_TT_OUT_DATA=fn_int_check_update(TT_DATA, legal_set_TT_OUT_DATA.copy())
    
    param_dict={
        "name" : "TT_OUT_DATA",
        "enum" : legal_set_TT_OUT_DATA
    } 

    return param_dict

def validate_TT_OUT_DATA(args):
    TT_DATA = args["TT_DATA"]
    TT_OUT_DATA = args["TT_OUT_DATA"]

    return fn_validate_TT_OUT_DATA(TT_DATA, TT_OUT_DATA)

def fn_validate_TT_OUT_DATA(TT_DATA, TT_OUT_DATA):
    param_dict=fn_update_TT_OUT_DATA(TT_DATA)
    return validate_legal_set(param_dict["enum"], "TT_OUT_DATA", TT_OUT_DATA)

#######################################################
########### TT_COEFF Updater and Validator ############
#######################################################
def update_TT_COEFF(args):
    AIE_VARIANT = args["AIE_VARIANT"]
    TT_DATA = args["TT_DATA"]
    return fn_update_tt_coeff(AIE_VARIANT, TT_DATA)

def validate_TT_COEFF(args):
    AIE_VARIANT = args["AIE_VARIANT"]
    TT_DATA = args["TT_DATA"]
    TT_COEFF=args["TT_COEFF"]
    return (fn_validate_TT_COEFF(AIE_VARIANT, TT_DATA, TT_COEFF))

def fn_validate_TT_COEFF(AIE_VARIANT, TT_DATA, TT_COEFF):
    param_dict = fn_update_tt_coeff(AIE_VARIANT, TT_DATA)
    legal_set_TT_COEFF = param_dict["enum"]
    return(validate_legal_set(legal_set_TT_COEFF, "TT_COEFF", TT_COEFF))

#######################################################
############# TP_API Updater and Validator ############
#######################################################
def update_TP_API(args):
    return fn_update_TP_API()
    
def fn_update_TP_API():
    legal_set_api=[0]
    param_dict={
        "name" : "TP_API",
        "enum" : legal_set_api}
    return param_dict

def validate_TP_API(args):
    TP_API=args["TP_API"]
    return fn_validate_TP_API(TP_API)

def fn_validate_TP_API(TP_API):
    return(validate_legal_set([0], "TP_API", TP_API))

#######################################################
############# TP_FIR_LEN Updater and Validator ########
#######################################################
def update_TP_FIR_LEN(args):
    TT_DATA = args["TT_DATA"]
    TT_COEFF = args["TT_COEFF"]
    TP_API = args["TP_API"]
    if args["TP_FIR_LEN"]: TP_FIR_LEN = args["TP_FIR_LEN"]
    else: TP_FIR_LEN = 0

    return fn_update_TP_FIR_LEN(TT_DATA, TT_COEFF, TP_API, TP_FIR_LEN)

def fn_update_TP_FIR_LEN(TT_DATA, TT_COEFF, TP_API, TP_FIR_LEN):
    TP_USE_COEFF_RELOAD =  0
    TP_CASC_LEN = 1
    symmetryFactor = 1

    TP_FIR_LEN_max_int=fn_max_fir_len_each_kernel_update(TT_DATA, TP_CASC_LEN
, TP_USE_COEFF_RELOAD, TP_SSR_max, TP_API, symmetryFactor)

    param_dict={
        "name" : "TP_FIR_LEN",
        "minimum" : TP_FIR_LEN_min,
        "maximum" : TP_FIR_LEN_max_int}

    if TP_FIR_LEN != 0:
        if TT_DATA == "cint16" and TT_COEFF == "int16" and TP_FIR_LEN % 2 == 1:
            TP_FIR_LEN_act=int(round(TP_FIR_LEN/2)*2)
            if TP_FIR_LEN_act < param_dict["minimum"]:
                TP_FIR_LEN_act=param_dict["minimum"]
            if TP_FIR_LEN_act > param_dict["maximum"]:
                TP_FIR_LEN_act=FLOOR(param_dict["maximum"],2)
            param_dict.update({"actual" : int(TP_FIR_LEN_act)})
    return param_dict    


def validate_TP_FIR_LEN(args):
    TT_DATA = args["TT_DATA"]
    TT_COEFF = args["TT_COEFF"]
    TP_API = args["TP_API"]
    TP_FIR_LEN = args["TP_FIR_LEN"]
    return fn_validate_TP_FIR_LEN(TT_DATA, TT_COEFF, TP_FIR_LEN, TP_API)

def fn_validate_TP_FIR_LEN(TT_DATA, TT_COEFF, TP_FIR_LEN, TP_API):
    param_dict=fn_update_TP_FIR_LEN(TT_DATA, TT_COEFF, TP_API, TP_FIR_LEN)
    if TT_DATA == "cint16" and TT_COEFF == "int16" and TP_FIR_LEN % 2 == 1:
        return isError("TP_FIR_LEN should be a multiple of 2!")
    range_fir_len=[param_dict["minimum"], param_dict["maximum"]]
    return validate_range(range_fir_len, "TP_FIR_LEN", TP_FIR_LEN)

#######################################################
############# TP_DUAL_IP Updater and Validator ########
#######################################################
def update_TP_DUAL_IP(args):
    return fn_update_TP_DUAL_IP()

def fn_update_TP_DUAL_IP():
    param_dict={
        "name" : "TP_DUAL_IP",
        "enum" : [0]
    }
    return param_dict

def validate_TP_DUAL_IP(args):
    TP_DUAL_IP =  args["TP_DUAL_IP"]
    return fn_validate_TP_DUAL_IP(TP_DUAL_IP)

def fn_validate_TP_DUAL_IP(TP_DUAL_IP):
    return validate_legal_set([0], "TP_DUAL_IP", TP_DUAL_IP)

#######################################################
######### TP_NUM_OUTPUTS Updater and Validator ########
#######################################################
def update_TP_NUM_OUTPUTS(args):
    return fn_update_TP_NUM_OUTPUTS()

def fn_update_TP_NUM_OUTPUTS():
    param_dict={
        "name" : "TP_NUM_OUTPUTS",
        "enum" : [1]
    }
    return param_dict

def validate_TP_NUM_OUTPUTS(args):
    TP_NUM_OUTPUTS =  args["TP_NUM_OUTPUTS"]
    return fn_validate_TP_NUM_OUTPUTS(TP_NUM_OUTPUTS)

def fn_validate_TP_NUM_OUTPUTS(TP_NUM_OUTPUTS):
    return validate_legal_set([1], "TP_NUM_OUTPUTS", TP_NUM_OUTPUTS)

#######################################################
######### TP_TDM_CHANNELS Updater and Validator #######
#######################################################
def update_TP_TDM_CHANNELS(args):
    TT_DATA = args["TT_DATA"]
    TT_COEFF = args["TT_COEFF"]
    TP_FIR_LEN = args["TP_FIR_LEN"]
    AIE_VARIANT = args["AIE_VARIANT"]
    if args["TP_TDM_CHANNELS"]: TP_TDM_CHANNELS = args["TP_TDM_CHANNELS"]
    else: TP_TDM_CHANNELS = 0
    return fn_update_TP_TDM_CHANNELS(TT_DATA, TT_COEFF, TP_FIR_LEN, AIE_VARIANT, TP_TDM_CHANNELS)

def fn_update_TP_TDM_CHANNELS(TT_DATA, TT_COEFF, TP_FIR_LEN, AIE_VARIANT, TP_TDM_CHANNELS):

    memoryGroupSize = k_data_memory_bytes[AIE_VARIANT]
    MAX_COEFFS_PER_TILE = memoryGroupSize / (fn_size_by_byte(TT_COEFF))
    TP_TDM_CHANNELS_max_calc= int((MAX_COEFFS_PER_TILE * TP_CASC_LEN_max * TP_SSR_max) /TP_FIR_LEN) 
    MAX_COEFFS_PER_TILE = memoryGroupSize / (fn_size_by_byte(TT_COEFF))

    TP_TDM_CHANNELS_max_int=int(min(TP_TDM_CHANNELS_max, TP_TDM_CHANNELS_max_calc))


    param_dict={
        "name" : "TP_TDM_CHANNELS",
        "minimum" : TP_TDM_CHANNELS_min,
        "maximum" : TP_TDM_CHANNELS_max_int
    }

    if TP_TDM_CHANNELS != 0:
        lanes = fnNumLanes(TT_DATA, TT_COEFF, 0, AIE_VARIANT)
        if TP_TDM_CHANNELS  % lanes != 0:
            TP_TDM_CHANNELS_act=int(round(TP_TDM_CHANNELS/lanes)*lanes)
            if TP_TDM_CHANNELS_act < param_dict["minimum"]:
                TP_TDM_CHANNELS_act=int(lanes)
            if TP_TDM_CHANNELS_act > param_dict["maximum"]:
                TP_TDM_CHANNELS_act=int(FLOOR(TP_TDM_CHANNELS_act, lanes))
            param_dict.update({"actual" : TP_TDM_CHANNELS_act})

    return param_dict

def validate_TP_TDM_CHANNELS(args):
    TT_DATA = args["TT_DATA"]
    TT_COEFF = args["TT_COEFF"]
    TP_FIR_LEN = args["TP_FIR_LEN"]
    AIE_VARIANT = args["AIE_VARIANT"]
    TP_TDM_CHANNELS = args["TP_TDM_CHANNELS"]
    return fn_validate_TP_TDM_CHANNELS(TT_DATA, TT_COEFF, TP_FIR_LEN, AIE_VARIANT, TP_TDM_CHANNELS)

def fn_validate_TP_TDM_CHANNELS(TT_DATA, TT_COEFF, TP_FIR_LEN, AIE_VARIANT, TP_TDM_CHANNELS):
    lanes = fnNumLanes(TT_DATA, TT_COEFF, 0, AIE_VARIANT)
    if TP_TDM_CHANNELS  % lanes != 0:
        return isError(f"TP_TDM_CHANNELS must be a multiple of {lanes}!")
    else:
        param_dict=fn_update_TP_TDM_CHANNELS(TT_DATA, TT_COEFF, TP_FIR_LEN, AIE_VARIANT, TP_TDM_CHANNELS)
        range_tdm_channels=[param_dict["minimum"], param_dict["maximum"]]
        return validate_range(range_tdm_channels, "TP_TDM_CHANNELS", TP_TDM_CHANNELS)

#######################################################
################ TP_SSR Updater and Validator #########
#######################################################
def update_TP_SSR(args):
    AIE_VARIANT = args["AIE_VARIANT"]
    TT_DATA = args["TT_DATA"]
    TT_COEFF = args["TT_COEFF"]
    TP_TDM_CHANNELS = args["TP_TDM_CHANNELS"]
    return fn_update_TP_SSR(AIE_VARIANT, TT_DATA, TT_COEFF, TP_TDM_CHANNELS)

def fn_update_TP_SSR(AIE_VARIANT, TT_DATA, TT_COEFF, TP_TDM_CHANNELS):
    lanes = fnNumLanes(TT_DATA, TT_COEFF, 0, AIE_VARIANT)
    legal_set_ssr=find_divisors(TP_TDM_CHANNELS/lanes, TP_SSR_max)
    param_dict={
        "name" : "TP_TDM_CHANNELS",
        "enum" : legal_set_ssr
    }
    return param_dict

def validate_TP_SSR(args):
    AIE_VARIANT = args["AIE_VARIANT"]
    TT_DATA = args["TT_DATA"]
    TT_COEFF = args["TT_COEFF"]
    TP_TDM_CHANNELS = args["TP_TDM_CHANNELS"]
    TP_SSR = args["TP_SSR"]
    return fn_validate_TP_SSR(AIE_VARIANT, TT_DATA, TT_COEFF, TP_TDM_CHANNELS, TP_SSR)

def fn_validate_TP_SSR(AIE_VARIANT, TT_DATA, TT_COEFF, TP_TDM_CHANNELS, TP_SSR):
    param_dict=fn_update_TP_SSR(AIE_VARIANT, TT_DATA, TT_COEFF, TP_TDM_CHANNELS)
    return validate_legal_set(param_dict["enum"], "TP_SSR", TP_SSR)

#######################################################
########### TP_CASC_LEN Updater and Validator #########
#######################################################
def update_TP_CASC_LEN(args):
    TT_DATA = args["TT_DATA"]
    TT_COEFF = args["TT_COEFF"]
    TP_FIR_LEN = args["TP_FIR_LEN"]
    AIE_VARIANT = args["AIE_VARIANT"]
    return fn_update_TP_CASC_LEN(TT_DATA, TT_COEFF, TP_FIR_LEN, AIE_VARIANT)

def fn_update_TP_CASC_LEN(TT_DATA, TT_COEFF, TP_FIR_LEN, AIE_VARIANT):
    fir_len = int(calc_fir_len(TT_DATA, TT_COEFF, TP_FIR_LEN, AIE_VARIANT))
    TP_CASC_LEN_max_int=min(TP_CASC_LEN_max, fir_len)
    legal_set_casc_len_temp1=list(range(TP_CASC_LEN_min, TP_CASC_LEN_max_int+1))
    legal_set_casc_len_temp2=fn_eliminate_casc_len_min_fir_len_each_kernel(legal_set_casc_len_temp1.copy(), TP_FIR_LEN, TP_SSR=1)

    param_dict={"name" : "TP_CASC_LEN"}
    if legal_set_casc_len_temp1==legal_set_casc_len_temp2:
        param_dict.update({
            "minimum" : TP_CASC_LEN_min,
            "maximum" : TP_CASC_LEN_max_int
        }) 
    else:
        param_dict.update({"enum" : legal_set_casc_len_temp2})
    return param_dict

def validate_TP_CASC_LEN(args):
    TT_DATA = args["TT_DATA"]
    TT_COEFF = args["TT_COEFF"]
    TP_FIR_LEN = args["TP_FIR_LEN"]
    AIE_VARIANT = args["AIE_VARIANT"]
    TP_CASC_LEN = args["TP_CASC_LEN"]
    return fn_validate_TP_CASC_LEN(TT_DATA, TT_COEFF, TP_FIR_LEN, AIE_VARIANT, TP_CASC_LEN)

def fn_validate_TP_CASC_LEN(TT_DATA, TT_COEFF, TP_FIR_LEN, AIE_VARIANT, TP_CASC_LEN):
    param_dict=fn_update_TP_CASC_LEN(TT_DATA, TT_COEFF, TP_FIR_LEN, AIE_VARIANT)
    if "enum" in param_dict:
        return validate_legal_set(param_dict["enum"], "TP_CASC_LEN", TP_CASC_LEN)
    else:
        range_casc_len=[param_dict["minimum"], param_dict["maximum"]]
        return validate_range(range_casc_len, "TP_CASC_LEN", TP_CASC_LEN)
    
def calc_fir_len(TT_DATA, TT_COEFF, TP_FIR_LEN, AIE_VARIANT):
  # Calculate effective FIR len here, where splitting workload over requested cascade length
  # woulnd not give the expected performance benefit.
  fir_len = TP_FIR_LEN
  if (AIE_VARIANT == 1 and
      (TT_DATA=="cint16" and TT_COEFF=="int16")):
    # TDM uses 2 intrinsic columns. No benefit of splitting workload into single tap per kernel
    fir_len = TP_FIR_LEN // 2

  return fir_len

#######################################################
###### TP_INPUT_WINDOW_VSIZE Updater and Validator ####
#######################################################
def update_TP_INPUT_WINDOW_VSIZE(args):
    TT_DATA = args["TT_DATA"]
    TT_OUT_DATA = args["TT_OUT_DATA"]
    TT_COEFF = args["TT_COEFF"]
    TP_FIR_LEN = args["TP_FIR_LEN"]
    TP_API = args["TP_API"]
    TP_SSR = args["TP_SSR"]
    TP_TDM_CHANNELS = args["TP_TDM_CHANNELS"]
    AIE_VARIANT = args["AIE_VARIANT"]
    if args["TP_INPUT_WINDOW_VSIZE"]: TP_INPUT_WINDOW_VSIZE = args["TP_INPUT_WINDOW_VSIZE"]
    else: TP_INPUT_WINDOW_VSIZE = 0

    return fn_update_TP_INPUT_WINDOW_VSIZE(TT_DATA, TT_OUT_DATA, TT_COEFF, TP_FIR_LEN, TP_INPUT_WINDOW_VSIZE, TP_API, TP_SSR, TP_TDM_CHANNELS, AIE_VARIANT)

def fn_update_TP_INPUT_WINDOW_VSIZE(TT_DATA, TT_OUT_DATA, TT_COEFF, TP_FIR_LEN, TP_INPUT_WINDOW_VSIZE, TP_API, TP_SSR, TP_TDM_CHANNELS, AIE_VARIANT):

    TP_INPUT_WINDOW_VSIZE_max_temp1=int(fn_max_windowsize_for_buffer_update(TT_DATA, TP_FIR_LEN, TP_SSR))

    TP_INPUT_WINDOW_VSIZE_max_temp2=fn_max_ws_in_out_buffer(AIE_VARIANT, TT_DATA, TT_OUT_DATA, TP_TDM_CHANNELS, TP_FIR_LEN, TP_SSR)

    TP_INPUT_WINDOW_VSIZE_max_int=min(TP_INPUT_WINDOW_VSIZE_max_temp1, TP_INPUT_WINDOW_VSIZE_max_temp2)
    

    param_dict={
        "name" : "TP_INPUT_WINDOW_VSIZE",
        "minimum" : TP_INPUT_WINDOW_VSIZE_min,
        "maximum" : TP_INPUT_WINDOW_VSIZE_max_int
    }

    if TP_INPUT_WINDOW_VSIZE!=0:
        streamRptFactor = 8
        windowSizeMultiplier = (
            (fnNumLanes(TT_DATA, TT_COEFF, TP_API, AIE_VARIANT))
            if TP_API == 0
            # Need to take unrolloing into account
            else (fnNumLanes(TT_DATA, TT_COEFF, TP_API, AIE_VARIANT) * streamRptFactor)
        )

        factor_ws=find_lcm(TP_SSR*windowSizeMultiplier, TP_TDM_CHANNELS)
        if TP_INPUT_WINDOW_VSIZE % factor_ws != 0:
            TP_INPUT_WINDOW_VSIZE_act=int(round(TP_INPUT_WINDOW_VSIZE/factor_ws) * factor_ws)
            if TP_INPUT_WINDOW_VSIZE_act < param_dict["minimum"]:
                TP_INPUT_WINDOW_VSIZE_act=int(factor_ws)
            if TP_INPUT_WINDOW_VSIZE_act > param_dict["maximum"]:
                TP_INPUT_WINDOW_VSIZE_act=FLOOR(param_dict["maximum"], factor_ws)
            param_dict.update({"actual" : TP_INPUT_WINDOW_VSIZE_act})

    return param_dict


def validate_TP_INPUT_WINDOW_VSIZE(args):
    TP_INPUT_WINDOW_VSIZE = args["TP_INPUT_WINDOW_VSIZE"]
    TT_DATA = args["TT_DATA"]
    TT_OUT_DATA = args["TT_OUT_DATA"]
    TT_COEFF = args["TT_COEFF"]
    TP_FIR_LEN = args["TP_FIR_LEN"]
    TP_API = args["TP_API"]
    TP_SSR = args["TP_SSR"]
    TP_TDM_CHANNELS = args["TP_TDM_CHANNELS"]
    AIE_VARIANT = args["AIE_VARIANT"]
    return fn_validate_TP_INPUT_WINDOW_VSIZE(TT_DATA, TT_OUT_DATA, TT_COEFF, TP_FIR_LEN, TP_INPUT_WINDOW_VSIZE, TP_API, TP_SSR, TP_TDM_CHANNELS, AIE_VARIANT)

def fn_validate_TP_INPUT_WINDOW_VSIZE(TT_DATA, TT_OUT_DATA, TT_COEFF, TP_FIR_LEN, TP_INPUT_WINDOW_VSIZE, TP_API, TP_SSR, TP_TDM_CHANNELS, AIE_VARIANT):
    streamRptFactor = 8
    windowSizeMultiplier = (
        (fnNumLanes(TT_DATA, TT_COEFF, TP_API, AIE_VARIANT))
        if TP_API == 0
        # Need to take unrolloing into account
        else (fnNumLanes(TT_DATA, TT_COEFF, TP_API, AIE_VARIANT) * streamRptFactor))

    factor_ws=find_lcm(TP_SSR*windowSizeMultiplier, TP_TDM_CHANNELS)
    if TP_INPUT_WINDOW_VSIZE % factor_ws != 0:
        return isError(f"TP_INPUT_WINDOW_VSIZE should be a multiple of {factor_ws}")
    else:
        param_dict=fn_update_TP_INPUT_WINDOW_VSIZE(TT_DATA, TT_OUT_DATA, TT_COEFF, TP_FIR_LEN, TP_INPUT_WINDOW_VSIZE, TP_API, TP_SSR, TP_TDM_CHANNELS, AIE_VARIANT)
        range_ws=[param_dict["minimum"], param_dict["maximum"]]
        return validate_range(range_ws, "TP_TDM_CHANNELS", TP_TDM_CHANNELS)


def fn_max_ws_in_out_buffer(AIE_VARIANT, TT_DATA, TT_OUT_DATA, TP_TDM_CHANNELS, TP_FIR_LEN, TP_SSR):
    # Need to check Input and output size doesn't exceed max Ping pong size of the device.

    BufferSize = k_data_memory_bytes[AIE_VARIANT]
    maxInBufferSampleSize = BufferSize / (fn_size_by_byte(TT_DATA))

    # Margin is put on an internal buffer for AIE1.
    if AIE_VARIANT == 1:
        marginSamples = 0
    else:
        marginSamples = (TP_TDM_CHANNELS * (TP_FIR_LEN - 1)) / (TP_SSR)
    # Data samples per SSR path

    TP_INPUT_WINDOW_VSIZE_max_in=(maxInBufferSampleSize - marginSamples) * TP_SSR 

    # Ping-pong size:
    maxOutBufferSampleSize = BufferSize / (fn_size_by_byte(TT_OUT_DATA))

    TP_INPUT_WINDOW_VSIZE_max_out=maxOutBufferSampleSize*TP_SSR

    TP_INPUT_WINDOW_VSIZE_max=int(min(TP_INPUT_WINDOW_VSIZE_max_in, TP_INPUT_WINDOW_VSIZE_max_out))
    return TP_INPUT_WINDOW_VSIZE_max

#######################################################
############### TP_SHIFT Updater and Validator ########
#######################################################
def update_TP_SHIFT(args):
  AIE_VARIANT = args["AIE_VARIANT"]
  TT_DATA = args["TT_DATA"]
  return fn_update_TP_SHIFT(AIE_VARIANT, TT_DATA)

def fn_update_TP_SHIFT(AIE_VARIANT, TT_DATA):
    range_TP_SHIFT=fn_update_range_TP_SHIFT(AIE_VARIANT, TT_DATA)

    param_dict={
        "name" : "TP_SHIFT",
        "minimum" : range_TP_SHIFT[0],
        "maximum" : range_TP_SHIFT[1]
    }
    return param_dict


def validate_TP_SHIFT(args):
    AIE_VARIANT = args["AIE_VARIANT"]
    TT_DATA = args["TT_DATA"]
    TP_SHIFT = args["TP_SHIFT"]
    return fn_validate_shift_val(AIE_VARIANT, TT_DATA, TP_SHIFT)

def fn_validate_shift_val(AIE_VARIANT, TT_DATA, TP_SHIFT):
  param_dict=fn_update_TP_SHIFT(AIE_VARIANT, TT_DATA)
  range_TP_SHIFT=[param_dict["minimum"], param_dict["maximum"]]
  return validate_range(range_TP_SHIFT, "TP_SHIFT", TP_SHIFT)

#######################################################
##############TP_RND Updater and Validator ############
#######################################################
def update_TP_RND(args):
  AIE_VARIANT = args["AIE_VARIANT"]
  return fn_update_TP_RND(AIE_VARIANT)

def fn_update_TP_RND(AIE_VARIANT):
  legal_set_TP_RND=fn_get_legalSet_roundMode(AIE_VARIANT)
  param_dict={
    "name" : "TP_RND",
    "enum" : legal_set_TP_RND
  }
  return param_dict

def validate_TP_RND(args):
    AIE_VARIANT = args["AIE_VARIANT"]
    TP_RND = args["TP_RND"]
    return fn_validate_roundMode(TP_RND, AIE_VARIANT)

#######################################################
############ TP_SAT Updater and Validator #############
#######################################################  
def update_TP_SAT(args):
  legal_set_sat=fn_legal_set_sat()
  param_dict={
    "name" : "TP_SAT",
    "enum" : legal_set_sat
  }
  return param_dict
                               
def validate_TP_SAT(args):
  TP_SAT = args["TP_SAT"]
  return fn_validate_satMode(TP_SAT)

#######################################################
############## coeff Updater and Validator ############
#######################################################
def update_coeff(args):
  TT_COEFF = args["TT_COEFF"]
  TP_FIR_LEN = args["TP_FIR_LEN"]
  TP_TDM_CHANNELS = args["TP_TDM_CHANNELS"]
  return fn_update_coeff(TT_COEFF, TP_FIR_LEN, TP_TDM_CHANNELS)

def fn_update_coeff(TT_COEFF, TP_FIR_LEN, TP_TDM_CHANNELS):
  
  if fn_is_complex(TT_COEFF) : len_coeff=2*TP_FIR_LEN*TP_TDM_CHANNELS
  else: len_coeff=TP_FIR_LEN*TP_TDM_CHANNELS

  param_dict={"name" : "coeff",
              "len"  : len_coeff}

  return param_dict

def validate_coeff(args):
  TT_COEFF = args["TT_COEFF"]
  TP_FIR_LEN = args["TP_FIR_LEN"]
  TP_TDM_CHANNELS = args["TP_TDM_CHANNELS"]
  coeff_list = args["coeff"]
  return fn_validate_coeff(TT_COEFF, TP_FIR_LEN, TP_TDM_CHANNELS, coeff_list)

def fn_validate_coeff(TT_COEFF, TP_FIR_LEN, TP_TDM_CHANNELS, coeff_list):
  param_dict=fn_update_coeff(TT_COEFF, TP_FIR_LEN, TP_TDM_CHANNELS)
  return validate_LUT_len(coeff_list, param_dict["len"])



def fn_ceil(m, n):
    return int(((m + n - 1) / n)) * n

def fn_margin_size(TP_FIR_LEN, TT_DATA, TP_TDM_CHANNELS):
    tmpmargin = (int(TP_FIR_LEN) - 1) * fn_size_by_byte(TT_DATA) * TP_TDM_CHANNELS
    return fn_ceil(tmpmargin, 32)


def info_ports(args):
    """Standard function creating a static dictionary of information
    for upper software to correctly connect the IP.
    Some IP has dynamic number of ports according to parameter set,
    so port information has to be implemented as a function"""
    TT_DATA = args["TT_DATA"]
    TT_OUT_DATA = args["TT_OUT_DATA"]
    TP_INPUT_WINDOW_VSIZE = args["TP_INPUT_WINDOW_VSIZE"]
    TP_TDM_CHANNELS = args["TP_TDM_CHANNELS"]
    TP_FIR_LEN = args["TP_FIR_LEN"]
    TP_SSR = args["TP_SSR"]
    TP_API = args["TP_API"]
    TP_DUAL_IP = args["TP_DUAL_IP"]
    TP_NUM_OUTPUTS = args["TP_NUM_OUTPUTS"]
    TP_DECIMATE_FACTOR = 1
    TP_INTERPOLATE_FACTOR = 1

    margin_size = fn_margin_size(TP_FIR_LEN // TP_SSR, TT_DATA, TP_TDM_CHANNELS)
    num_in_ports = TP_SSR
    num_out_ports = TP_SSR

    in_win_size = get_input_window_size(TP_INPUT_WINDOW_VSIZE, num_in_ports, TP_API, TP_DUAL_IP)
    out_win_size = get_output_window_size(TP_INPUT_WINDOW_VSIZE, num_out_ports, TP_API, TP_NUM_OUTPUTS, TP_DECIMATE_FACTOR, TP_INTERPOLATE_FACTOR)

    in_ports = get_port_info( "in", "in", TT_DATA, in_win_size, num_in_ports, marginSize=margin_size, TP_API=TP_API)
    in2_ports = (get_port_info( "in2", "in", TT_DATA, in_win_size, num_in_ports, marginSize=margin_size, TP_API=TP_API, ) if (args["TP_DUAL_IP"] == 1) else [])

    # decimate by 2 for halfband
    out_ports = get_port_info( "out", "out", TT_OUT_DATA, out_win_size, num_out_ports, TP_API=TP_API,)
    out2_ports = (get_port_info( "out2", "out", TT_OUT_DATA, out_win_size, num_out_ports, TP_API=TP_API, ) if (args["TP_NUM_OUTPUTS"] == 2) else [])
    return in_ports + in2_ports + out_ports + out2_ports


#### graph generator ####
# def get_param_list(**kwargs):
#   [f"{value}{comma}} //{key}" for key, value in kwargs.iteritems() for comma in "," ]

# Returns formatted string with taps
def fn_get_taps_vector(TT_COEFF, coeff_list):

    cplx = fn_is_complex(TT_COEFF)

    # todo, reformat this to use list comprehension
    taps = f"{{"
    # complex pair
    if cplx:
        taps += ", ".join(
            [
                f"{{{coeff_list[2*i]} , {coeff_list[2*i+1]}}}"
                for i in range(int(len(coeff_list) / 2))
            ]
        )
    else:
        taps += ", ".join([str(coeff_list[i]) for i in range(len(coeff_list))])
    taps += f"}}"

    return taps


def generate_graph(graphname, args):

    if graphname == "":
        graphname = "default_graphname"

    TT_COEFF = args["TT_COEFF"]
    TT_DATA = args["TT_DATA"]
    TT_OUT_DATA = args["TT_OUT_DATA"]
    TP_FIR_LEN = args["TP_FIR_LEN"]
    TP_SHIFT = args["TP_SHIFT"]
    TP_RND = args["TP_RND"]
    TP_TDM_CHANNELS = args["TP_TDM_CHANNELS"]
    TP_INPUT_WINDOW_VSIZE = args["TP_INPUT_WINDOW_VSIZE"]
    TP_NUM_OUTPUTS = args["TP_NUM_OUTPUTS"]
    TP_DUAL_IP = args["TP_DUAL_IP"]
    TP_API = args["TP_API"]
    TP_SSR = args["TP_SSR"]
    coeff_list = args["coeff"]
    TP_SAT = args["TP_SAT"]
    TP_CASC_LEN = args["TP_CASC_LEN"]

    taps = fn_get_taps_vector(TT_COEFF, coeff_list)
    dual_ip_declare_str = (
        f"ssr_port_array<input> in2;" if TP_DUAL_IP == 1 else "// No dual input"
    )
    dual_ip_connect_str = (
        f"adf::connect<> net_in2(in2[i], filter.in2[i]);"
        if TP_DUAL_IP == 1
        else "// No dual input"
    )
    dual_op_declare_str = (
        f"ssr_port_array<output> out2;" if TP_NUM_OUTPUTS == 2 else "// No dual output"
    )
    dual_op_connect_str = (
        f"adf::connect<> net_out2(filter.out2[i], out2[i]);"
        if TP_NUM_OUTPUTS == 2
        else "// No dual output"
    )

    # Use formatted multi-line string to avoid a lot of \n and \t
    code = f"""
class {graphname} : public adf::graph {{
public:
  static constexpr unsigned int TP_SSR = {TP_SSR};
  template <typename dir>
  using ssr_port_array = std::array<adf::port<dir>, TP_SSR>;

  ssr_port_array<input> in;
  {dual_ip_declare_str}
  ssr_port_array<output> out;
  {dual_op_declare_str}

  std::vector<{TT_COEFF}> taps = {taps};
  xf::dsp::aie::fir::tdm::fir_tdm_graph<
    {TT_DATA}, //TT_DATA
    {TT_COEFF}, //TT_COEFF
    {TP_FIR_LEN}, //TP_FIR_LEN
    {TP_SHIFT}, //TP_SHIFT
    {TP_RND}, //TP_RND
    {TP_INPUT_WINDOW_VSIZE}, //TP_INPUT_WINDOW_VSIZE
    {TP_TDM_CHANNELS}, //TP_TDM_CHANNELS
    {TP_NUM_OUTPUTS}, //TP_NUM_OUTPUTS
    {TP_DUAL_IP}, //TP_DUAL_IP
    // {TP_API}, //TP_API
    {TP_SSR}, //TP_SSR
    {TP_SAT}, //TP_SAT
    {TP_CASC_LEN}, //TP_CASC_LEN
    {TT_OUT_DATA} //TT_OUT_DATA
  > filter;

  {graphname}() : filter({taps}) {{
    adf::kernel *filter_kernels = filter.getKernels();
    for (int i=0; i < 1; i++) {{
      adf::runtime<ratio>(filter_kernels[i]) = 0.9;
    }}
    for (int i=0; i < TP_SSR; i++) {{
      adf::connect<> net_in(in[i], filter.in[i]);
      {dual_ip_connect_str}
      adf::connect<> net_out(filter.out[i], out[i]);
      {dual_op_connect_str}
    }}
  }}

}};
"""
    out = {}
    out["graph"] = code
    out["port_info"] = info_ports(args)
    out["headerfile"] = "fir_tdm_graph.hpp"
    out["searchpaths"] = [
        "L2/include/aie",
        "L2/tests/aie/common/inc",
        "L1/include/aie",
        "L1/src/aie",
        "L1/tests/aie/inc",
        "L1/tests/aie/src",
    ]
    return out
