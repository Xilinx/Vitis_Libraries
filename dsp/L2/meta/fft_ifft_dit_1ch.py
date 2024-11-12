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
import json
import sys

# fft_ifft_dit_1ch.hpp:821:    static_assert(fnCheckDataType<TT_DATA>(), "ERROR: TT_IN_DATA is not a supported type");
# fft_ifft_dit_1ch.hpp:822:    static_assert(fnCheckDataType<TT_OUT_DATA>(), "ERROR: TT_OUT_DATA is not a supported type");
#   ignore this internal debug aid
# fft_ifft_dit_1ch.hpp:823:    static_assert(fnCheckDataIOType<TT_DATA, TT_OUT_DATA>(), "ERROR: TT_OUT_DATA is not a supported type");
#   ignore this internal debug aid
# fft_ifft_dit_1ch.hpp:824:    static_assert(fnCheckTwiddleType<TT_TWIDDLE>(), "ERROR: TT_TWIDDLE is not a supported type");
# fft_ifft_dit_1ch.hpp:825:    static_assert(fnCheckDataTwiddleType<TT_DATA, TT_TWIDDLE>(), "ERROR: TT_TWIDDLE is incompatible with data type");
# fft_ifft_dit_1ch.hpp:826:    static_assert(fnCheckPointSize<TP_POINT_SIZE>(),
# fft_ifft_dit_1ch.hpp:828:    static_assert(TP_FFT_NIFFT == 0 || TP_FFT_NIFFT == 1, "ERROR: TP_FFT_NIFFT must be 0 (reverse) or 1 (forward)");
# fft_ifft_dit_1ch.hpp:829:    static_assert(fnCheckShift<TP_SHIFT>(), "ERROR: TP_SHIFT is out of range (0 to 60)");
# fft_ifft_dit_1ch.hpp:830:    static_assert(fnCheckShiftFloat<TT_DATA, TP_SHIFT>(),
# fft_ifft_dit_1ch.hpp:832:    static_assert(TP_WINDOW_VSIZE % TP_POINT_SIZE == 0, "ERROR: TP_WINDOW_VSIZE must be a multiple of TP_POINT_SIZE");
# fft_ifft_dit_1ch.hpp:833:    static_assert(TP_WINDOW_VSIZE / TP_POINT_SIZE >= 1, "ERROR: TP_WINDOW_VSIZE must be a multiple of TP_POINT_SIZE")
# fft_ifft_dit_1ch.hpp:834:    static_assert((TP_DYN_PT_SIZE == 0) || (TP_POINT_SIZE != 16),
# fft_ifft_dit_1ch_graph.hpp:152:        static_assert(fnCheckCascLen<TT_DATA, TP_END_RANK, TP_CASC_LEN>(), "Error: TP_CASC_LEN is invalid");
# fft_ifft_dit_1ch_graph.hpp:153:        static_assert(fnCheckCascLen2<TT_DATA, TP_POINT_SIZE, TP_CASC_LEN>(), "Error: 16 point float FFT does not support cascade")
# fft_ifft_dit_1ch_graph.hpp:842:    static_assert(TP_API == kStreamAPI, "Error: Only Stream interface is supported for parallel FFT");
# fft_ifft_dit_1ch_graph.hpp:843:    static_assert(TP_PARALLEL_POWER >= 1 && TP_PARALLEL_POWER < 9,

TP_POINT_SIZE_min_aie1 = 8
TP_POINT_SIZE_min_aie2 = 32
TP_POINT_SIZE_max = 65536
TP_WINDOW_SIZE_max = 65536
TP_WINDOW_SIZE_max_cpp = 2**31
TP_WINDOW_VSIZE_min = 8
TP_CASC_LEN_min = 1
TP_CASC_LEN_max = 11
TP_SHIFT_min=0
TP_SHIFT_max=60
TP_RND_min = 0
TP_RND_max = 7
TP_TWIDDLE_MODE_max = 1
TP_TWIDDLE_MODE_min = 0
#TP_FFT_NIFFT_min=0
#TP_FFT_NIFFT_max=1
#TP_API_min=0
#TP_API_max=1
#AIE_VARIANT_min=2
#AIE_VARIANT_max=1

#######################################################
########### AIE_VARIANT Updater and Validator #########
#######################################################
def update_AIE_VARIANT(args):
  return fn_update_AIE_VAR()

def  fn_update_AIE_VAR():
  legal_set_AIE_VAR = [1, 2]

  param_dict ={}
  param_dict.update({"name" : "AIE_VARIANT"})
  param_dict.update({"enum" : legal_set_AIE_VAR})

  return param_dict

def validate_AIE_VARIANT(args):
  AIE_VARIANT = args["AIE_VARIANT"]
  return fn_validate_AIE_VARIANT(AIE_VARIANT)

def fn_validate_AIE_VARIANT(AIE_VARIANT):
  param_dict=fn_update_AIE_VAR()
  legal_set_AIE_VAR=param_dict["enum"]
  return (validate_legal_set(legal_set_AIE_VAR, "AIE_VARIANT", AIE_VARIANT))

#######################################################
########### TT_DATA Updater and Validator #############
#######################################################

def update_TT_DATA(args):
  return fn_update_TT_DATA()

def  fn_update_TT_DATA():
  legal_set_TT_DATA = ["cint16", "cint32", "cfloat"]

  param_dict ={}
  param_dict.update({"name" : "TT_DATA"})
  param_dict.update({"enum" : legal_set_TT_DATA})

  return param_dict

def validate_TT_DATA(args):
  TT_DATA = args["TT_DATA"]
  return fn_validate_TT_DATA(TT_DATA)

def fn_validate_TT_DATA(TT_DATA):
  legal_set_TT_DATA = ["cint16", "cint32", "cfloat"]
  return (validate_legal_set(legal_set_TT_DATA, "TT_DATA", TT_DATA))

#######################################################
########### TT_OUT_DATA Updater and Validator #########
#######################################################

def update_TT_OUT_DATA(args):
  TT_DATA = args["TT_DATA"]
  return fn_update_TT_OUT_DATA(TT_DATA)

def  fn_update_TT_OUT_DATA(TT_DATA):
  if TT_DATA in ["cint16", "cint32"]:
    legal_set_TT_OUT_DATA = ["cint16", "cint32"]
  elif TT_DATA=="cfloat": legal_set_TT_OUT_DATA = ["cfloat"]

  param_dict ={}
  param_dict.update({"name" : "TT_OUT_DATA"})
  param_dict.update({"enum" : legal_set_TT_OUT_DATA})

  return param_dict

def validate_TT_OUT_DATA(args):
  TT_DATA = args["TT_DATA"]
  TT_OUT_DATA = args["TT_OUT_DATA"]
  return fn_validate_data_out( TT_DATA, TT_OUT_DATA)

def fn_validate_data_out(TT_DATA, TT_OUT_DATA):
  param_dict=fn_update_TT_OUT_DATA(TT_DATA)
  legal_set_TT_OUT_DATA=param_dict["enum"]
  return(validate_legal_set(legal_set_TT_OUT_DATA, "TT_OUT_DATA", TT_OUT_DATA))

#######################################################
########### TT_TWIDDLE Updater and Validator ##########
#######################################################
def update_TT_TWIDDLE(args):
  AIE_VARIANT = args["AIE_VARIANT"]
  TT_DATA = args["TT_DATA"]
  return fn_update_TT_TWIDDLE(AIE_VARIANT, TT_DATA)

def  fn_update_TT_TWIDDLE(AIE_VARIANT, TT_DATA):
  if TT_DATA in ["cint16", "cint32"]:
    if AIE_VARIANT==1: legal_set_TT_TWIDDLE = ["cint16", "cint32"]
    elif AIE_VARIANT==2: legal_set_TT_TWIDDLE = ["cint16"]
  elif TT_DATA=="cfloat": legal_set_TT_TWIDDLE = ["cfloat"]

  param_dict ={}
  param_dict.update({"name" : "TT_TWIDDLE"})
  param_dict.update({"enum" : legal_set_TT_TWIDDLE})

  return param_dict

def validate_TT_TWIDDLE(args):
  AIE_VARIANT = args["AIE_VARIANT"]
  TT_DATA = args["TT_DATA"]
  TT_TWIDDLE = args["TT_TWIDDLE"]
  return fn_validate_twiddle_type(AIE_VARIANT, TT_DATA, TT_TWIDDLE)

def fn_validate_twiddle_type(AIE_VARIANT, TT_DATA, TT_TWIDDLE):
  param_dict=fn_update_TT_TWIDDLE(AIE_VARIANT, TT_DATA)
  legal_set_TT_TWIDDLE=param_dict["enum"]
  return(validate_legal_set(legal_set_TT_TWIDDLE, "TT_TWIDDLE", TT_TWIDDLE))

#######################################################
########### TP_DYN_PT_SIZE Updater and Validator ######
#######################################################
def update_TP_DYN_PT_SIZE(args):
  return fn_update_TP_DYN_PT_SIZE()

def  fn_update_TP_DYN_PT_SIZE():
  legal_set_TP_DYN_PT_SIZE = [0, 1]

  param_dict ={}
  param_dict.update({"name" : "TP_DYN_PT_SIZE"})
  param_dict.update({"enum" : legal_set_TP_DYN_PT_SIZE})

  return param_dict

def validate_TP_DYN_PT_SIZE(args):
  TP_DYN_PT_SIZE = args["TP_DYN_PT_SIZE"]
  return  fn_validate_TP_DYN_PT_SIZE(TP_DYN_PT_SIZE)

def fn_validate_TP_DYN_PT_SIZE(TP_DYN_PT_SIZE):
  legal_set_TP_DYN_PT_SIZE=[0,1]
  return (validate_legal_set(legal_set_TP_DYN_PT_SIZE, "TP_DYN_PT_SIZE", TP_DYN_PT_SIZE))

#######################################################
########### TP_API Updater and Validator ##############
#######################################################
def update_TP_API(args):
  return fn_update_TP_API()

def  fn_update_TP_API():
  legal_set_TP_API = [0, 1]

  param_dict ={}
  param_dict.update({"name" : "TP_API"})
  param_dict.update({"enum" : legal_set_TP_API})

  return param_dict

def validate_TP_API(args):
  TP_API = args["TP_API"]
  return(fn_validate_TP_API(TP_API))

def fn_validate_TP_API(TP_API):
  legal_set_TP_API=[0,1]
  return (validate_legal_set(legal_set_TP_API, "TP_API", TP_API))

#######################################################
########### TP_PARALLEL_POWER Updater and Validator ###
#######################################################
def update_TP_PARALLEL_POWER(args):
  return fn_update_parallel_power()

def fn_update_parallel_power():
  range_TP_PARALLEL_POWER= [0,4]
  param_dict ={}
  param_dict.update({"name" : "TP_PARALLEL_POWER"})
  param_dict.update({"minimum" : range_TP_PARALLEL_POWER[0]})
  param_dict.update({"maximum" : range_TP_PARALLEL_POWER[1]})

  return param_dict

def validate_TP_PARALLEL_POWER(args):
  TP_PARALLEL_POWER = args["TP_PARALLEL_POWER"]
  return fn_validate_parallel_power(TP_PARALLEL_POWER)

def fn_validate_parallel_power(TP_PARALLEL_POWER):
  param_dict=fn_update_parallel_power()
  range_TP_PARALLEL_POWER=[param_dict["minimum"], param_dict["maximum"]]
  return(validate_range(range_TP_PARALLEL_POWER, "TP_PARALLEL_POWER", TP_PARALLEL_POWER))

#######################################################
########### TP_POINT_SIZE Updater and Validator #######
#######################################################
def update_TP_POINT_SIZE(args):
  AIE_VARIANT = args["AIE_VARIANT"]
  TT_DATA = args["TT_DATA"]
  TP_API = args["TP_API"]
  TP_PARALLEL_POWER = args["TP_PARALLEL_POWER"]
  TP_DYN_PT_SIZE = args["TP_DYN_PT_SIZE"]
  if args["TP_POINT_SIZE"]: TP_POINT_SIZE=args["TP_POINT_SIZE"]
  else: TP_POINT_SIZE=0
  return fn_update_TP_POINT_SIZE(AIE_VARIANT, TT_DATA, TP_API, TP_PARALLEL_POWER, TP_DYN_PT_SIZE, TP_POINT_SIZE)

def fn_update_TP_POINT_SIZE(AIE_VARIANT, TT_DATA, TP_API, TP_PARALLEL_POWER, TP_DYN_PT_SIZE, TP_POINT_SIZE):

  if TP_DYN_PT_SIZE == 0:
    if AIE_VARIANT==1: TP_POINT_SIZE_min = TP_POINT_SIZE_min_aie1
    elif AIE_VARIANT==2: TP_POINT_SIZE_min = TP_POINT_SIZE_min_aie2
  else: 
    if AIE_VARIANT==1: TP_POINT_SIZE_min = 2*TP_POINT_SIZE_min_aie1
    elif AIE_VARIANT==2: TP_POINT_SIZE_min = 2*TP_POINT_SIZE_min_aie2

  if TT_DATA == "cint16":
    if (TP_API == 0): MaxPointSizePerKernel = 4096
    else: MaxPointSizePerKernel = 4096
  elif TT_DATA == "cint32":
    if (TP_API == 0): MaxPointSizePerKernel = 2048
    else: MaxPointSizePerKernel = 4096
  else:
    if (TP_API == 0): 
      MaxPointSizePerKernel = 2048
      if (TP_DYN_PT_SIZE == 1): MaxPointSizePerKernel=int(MaxPointSizePerKernel/2)
    else: MaxPointSizePerKernel = 2048

  TP_POINT_SIZE_max_int=MaxPointSizePerKernel<<TP_PARALLEL_POWER
  TP_POINT_SIZE_min_int=TP_POINT_SIZE_min<<TP_PARALLEL_POWER

  param_dict ={}
  param_dict.update({"name" : "TP_POINT_SIZE"})
  param_dict.update({"minimum" : TP_POINT_SIZE_min_int})
  param_dict.update({"maximum" : TP_POINT_SIZE_max_int})

  if (TP_POINT_SIZE!=0) and (not fn_is_power_of_two(TP_POINT_SIZE)):
    TP_POINT_SIZE_act=round_power_of_2(TP_POINT_SIZE)
    if TP_POINT_SIZE_act<param_dict["minimum"]:
      TP_POINT_SIZE_act=param_dict["minimum"]
    if TP_POINT_SIZE_act>param_dict["maximum"]:
      TP_POINT_SIZE_act=param_dict["maximum"]
    param_dict.update({"actual" : int(TP_POINT_SIZE_act)})

  return param_dict

def validate_TP_POINT_SIZE(args):
  AIE_VARIANT = args["AIE_VARIANT"]
  TT_DATA = args["TT_DATA"]
  TP_API = args["TP_API"]
  TP_PARALLEL_POWER = args["TP_PARALLEL_POWER"]
  TP_DYN_PT_SIZE = args["TP_DYN_PT_SIZE"]
  TP_POINT_SIZE = args["TP_POINT_SIZE"]
  return fn_validate_TP_POINT_SIZE(AIE_VARIANT, TT_DATA, TP_API, TP_PARALLEL_POWER, TP_DYN_PT_SIZE, TP_POINT_SIZE)

def fn_validate_TP_POINT_SIZE(AIE_VARIANT, TT_DATA, TP_API, TP_PARALLEL_POWER, TP_DYN_PT_SIZE, TP_POINT_SIZE):
  param_dict=fn_update_TP_POINT_SIZE(AIE_VARIANT, TT_DATA, TP_API, TP_PARALLEL_POWER, TP_DYN_PT_SIZE, TP_POINT_SIZE)
  range_TP_POINT_SIZE= [param_dict["minimum"], param_dict["maximum"]]

  if not fn_is_power_of_two(TP_POINT_SIZE):
    return isError(f"Point size ({TP_POINT_SIZE}) must be a power of 2")

  return(validate_range(range_TP_POINT_SIZE, "TP_POINT_SIZE", TP_POINT_SIZE))

#######################################################
########### TP_WINDOW_VSIZE Updater and Validator #####
#######################################################
def update_TP_WINDOW_VSIZE(args):
  TP_DYN_PT_SIZE = args["TP_DYN_PT_SIZE"]
  TP_POINT_SIZE = args["TP_POINT_SIZE"]

  if args["TP_WINDOW_VSIZE"]: TP_WINDOW_VSIZE=args["TP_WINDOW_VSIZE"]
  else: TP_WINDOW_VSIZE=0
  return fn_update_TP_WINDOW_VSIZE(TP_POINT_SIZE, TP_DYN_PT_SIZE, TP_WINDOW_VSIZE)

def fn_update_TP_WINDOW_VSIZE(TP_POINT_SIZE, TP_DYN_PT_SIZE, TP_WINDOW_VSIZE):
  # Disable the window_vsize check for dynamic point size, due to incorrectly created caller function's arguments.

  min_TP_WINDOW_VSIZE=TP_POINT_SIZE

  if TP_DYN_PT_SIZE==0: # no maximum check for dynamic point size
    TP_WINDOW_SIZE_max_int=TP_WINDOW_SIZE_max
  else:
    TP_WINDOW_SIZE_max_int=TP_WINDOW_SIZE_max_cpp
  
  param_dict = {"name"    : "TP_WINDOW_VSIZE",
                "minimum" : min_TP_WINDOW_VSIZE,
                "maximum" : TP_WINDOW_SIZE_max_int}

  if TP_WINDOW_VSIZE != 0 and (TP_WINDOW_VSIZE%TP_POINT_SIZE != 0): 
    TP_WINDOW_VSIZE_act=round(TP_WINDOW_VSIZE/TP_POINT_SIZE) * TP_POINT_SIZE

    if TP_WINDOW_VSIZE_act < param_dict["minimum"]:
      TP_WINDOW_VSIZE_act = param_dict["minimum"]

    if TP_WINDOW_VSIZE_act > param_dict["maximum"]:
      TP_WINDOW_VSIZE_act = int(FLOOR(param_dict["maximum"], TP_POINT_SIZE))
    
    param_dict.update({"actual" : int(TP_WINDOW_VSIZE_act)})

  return param_dict

def validate_TP_WINDOW_VSIZE(args):
  TP_POINT_SIZE = args["TP_POINT_SIZE"]
  TP_WINDOW_VSIZE = args["TP_WINDOW_VSIZE"]
  TP_DYN_PT_SIZE = args["TP_DYN_PT_SIZE"]
  return fn_validate_window_size(TP_POINT_SIZE, TP_WINDOW_VSIZE, TP_DYN_PT_SIZE)


def fn_validate_window_size(TP_POINT_SIZE, TP_WINDOW_VSIZE, TP_DYN_PT_SIZE):
  if (TP_WINDOW_VSIZE % TP_POINT_SIZE != 0):
    return isError(f"Input window size ({TP_WINDOW_VSIZE}) must be a multiple of point size ({TP_POINT_SIZE}) ")

  param_dict=fn_update_TP_WINDOW_VSIZE(TP_POINT_SIZE,TP_DYN_PT_SIZE, TP_WINDOW_VSIZE)
  range_TP_WINDOW_VSIZE= [param_dict["minimum"], param_dict["maximum"]]
  return(validate_range(range_TP_WINDOW_VSIZE, "TP_WINDOW_VSIZE", TP_WINDOW_VSIZE))  

#######################################################
########### TP_CASC_LEN Updater and Validator #########
#######################################################
def update_TP_CASC_LEN(args):
  TT_DATA = args["TT_DATA"]
  TP_POINT_SIZE = args["TP_POINT_SIZE"]
  TP_PARALLEL_POWER = args["TP_PARALLEL_POWER"]
  return fn_update_TP_CASC_LEN(TT_DATA, TP_POINT_SIZE, TP_PARALLEL_POWER)

def fn_update_TP_CASC_LEN(TT_DATA, TP_POINT_SIZE, TP_PARALLEL_POWER):

  # Defines how many radix-2 ranks there are in the FFT itself (subframe or main FFT).
  log2PointSize = fn_log2(TP_POINT_SIZE>>TP_PARALLEL_POWER)
  # equation for integer ffts is complicated by the fact that odd power of 2 point sizes start with a radix 2 stage
  #Further, since integer implementation uses radix4, 2 ranks per kernel after the initial possible single radix2 is forced, so
  NUM_STAGES = (CEIL(log2PointSize, 2)/2) if TT_DATA != "cfloat" else log2PointSize
  maxTP_CASC_LEN= min(TP_CASC_LEN_max, int(NUM_STAGES))

  # maxTP_CASC_LEN=TP_CASC_LEN_max
  
  param_dict ={}
  param_dict.update({"name" : "TP_CASC_LEN"})
  param_dict.update({"minimum" : TP_CASC_LEN_min})
  param_dict.update({"maximum" : maxTP_CASC_LEN})

  return param_dict  

def validate_TP_CASC_LEN(args):
  TT_DATA = args["TT_DATA"]
  TP_POINT_SIZE = args["TP_POINT_SIZE"]
  TP_PARALLEL_POWER = args["TP_PARALLEL_POWER"]
  TP_CASC_LEN = args["TP_CASC_LEN"]
  return fn_validate_casc_len(TT_DATA, TP_POINT_SIZE, TP_PARALLEL_POWER, TP_CASC_LEN)

def fn_validate_casc_len(TT_DATA, TP_POINT_SIZE, TP_PARALLEL_POWER, TP_CASC_LEN):
  param_dict=fn_update_TP_CASC_LEN(TT_DATA, TP_POINT_SIZE, TP_PARALLEL_POWER)
  range_TP_CASC_LEN=[param_dict["minimum"], param_dict["maximum"]]
  return(validate_range(range_TP_CASC_LEN, "TP_CASC_LEN", TP_CASC_LEN))

#######################################################
########### TP_FFT_NIFFT Updater and Validator ########
#######################################################
def update_TP_FFT_NIFFT(args):
  return fn_update_TP_FFT_NIFFT()

def  fn_update_TP_FFT_NIFFT():
  legal_set_TP_FFT_NIFFT= [0, 1]

  param_dict ={}
  param_dict.update({"name" : "TP_FFT_NIFFT"})
  param_dict.update({"enum" : legal_set_TP_FFT_NIFFT})

  return param_dict

def validate_TP_FFT_NIFFT(args):
  TP_FFT_NIFFT=args["TP_FFT_NIFFT"]
  return fn_validate_TP_FFT_NIFFT(TP_FFT_NIFFT)

def fn_validate_TP_FFT_NIFFT(TP_FFT_NIFFT):
  legal_set_TP_FFT_NIFFT= [0, 1]
  return(validate_legal_set(legal_set_TP_FFT_NIFFT, "TP_FFT_NIFFT", TP_FFT_NIFFT))

#######################################################
########### TP_SHIFT Updater and Validator ############
#######################################################
def update_TP_SHIFT(args):
  TT_DATA = args["TT_DATA"]
  return fn_update_shift_val(TT_DATA)

def fn_update_shift_val(TT_DATA):
  param_dict ={}
  param_dict.update({"name" : "TP_SHIFT"})
  param_dict.update({"minimum" : TP_SHIFT_min})
  if TT_DATA=="cfloat":
    param_dict.update({"maximum" : TP_SHIFT_min})
  else:
    param_dict.update({"maximum" : TP_SHIFT_max})

  return param_dict

def validate_TP_SHIFT(args):
  TP_SHIFT = args["TP_SHIFT"]
  TT_DATA = args["TT_DATA"]
  return fn_validate_shift_val(TT_DATA, TP_SHIFT)

def fn_validate_shift_val(TT_DATA, TP_SHIFT):
  res = fn_validate_minmax_value("TP_SHIFT", TP_SHIFT, TP_SHIFT_min, TP_SHIFT_max)
  if (res["is_valid"] == False):  
    return res
  return fn_float_no_shift(TT_DATA, TP_SHIFT)

######################################################
########## TP_RND Updater and Validator ##############
######################################################
def update_TP_RND(args):
  AIE_VARIANT = args["AIE_VARIANT"]
  return fn_update_TP_RND(AIE_VARIANT)

def fn_update_TP_RND(AIE_VARIANT):
  legal_set_TP_RND=fn_get_legalSet_roundMode(AIE_VARIANT)
  if AIE_VARIANT==1:
    remove_set=[k_rnd_mode_map_aie1["rnd_ceil"], k_rnd_mode_map_aie1["rnd_floor"]]
  elif AIE_VARIANT==2:
    remove_set=[k_rnd_mode_map_aie2["rnd_ceil"], k_rnd_mode_map_aie2["rnd_floor"], k_rnd_mode_map_aie2["rnd_sym_floor"], k_rnd_mode_map_aie2["rnd_sym_ceil"]]

  legal_set_TP_RND=remove_from_set(remove_set, legal_set_TP_RND.copy())

  param_dict={
    "name" : "TP_RND",
    "enum" : legal_set_TP_RND
  }
  return param_dict

def validate_TP_RND(args):
    AIE_VARIANT = args["AIE_VARIANT"]
    TP_RND = args["TP_RND"]
    return fn_validate_roundMode(TP_RND, AIE_VARIANT)

######################################################
########## TP_SAT Updater and Validator ##############
######################################################
def update_TP_SAT(args):
  return fn_update_tp_sat()

def fn_update_tp_sat():
  legal_set = [0,1,3]

  param_dict={}
  param_dict.update({"name" : "TP_SAT"})
  param_dict.update({"enum" : legal_set})
  return param_dict


def validate_TP_SAT(args):
  TP_SAT = args["TP_SAT"]
  param_dict = update_TP_SAT(args)
  legal_set_TP_SAT = param_dict["enum"]
  return validate_legal_set(legal_set_TP_SAT, "TP_SAT", TP_SAT)

######################################################
########## TP_USE_WIDGETS Updater and Validator ######
######################################################
def update_TP_USE_WIDGETS(args):
  return fn_update_TP_USE_WIDGETS()

def fn_update_TP_USE_WIDGETS():
  legal_set = [0,1]
  param_dict={}
  param_dict.update({"name" : "TP_USE_WIDGETS"})
  param_dict.update({"enum" : legal_set})
  return param_dict

def validate_TP_USE_WIDGETS(args):
  TP_USE_WIDGETS = args["TP_USE_WIDGETS"]
  param_dict = update_TP_USE_WIDGETS(args)
  legal_set_TP_USE_WIDGETS = param_dict["enum"]
  return validate_legal_set(legal_set_TP_USE_WIDGETS, "TP_USE_WIDGETS", TP_USE_WIDGETS)

######################################################
########## TP_TWIDDLE_MODE Updater and Validator #####
######################################################

#---------------------------------------------------
#Twiddle mode is the amplitude of twiddles and applies to integer types only. It is ignored for float types
#Twiddle mode 0 means use max amplitude twiddles, but these saturate at 2^(N-1)-1 where N is the number of bits
#in the type, e.g. cint16 has 16 bits per component.
#Twiddle mode 1 means use 1/2 max magnitude twiddles, i.e. 2^(N-1). This avoids saturation, but loses 1 bit of
#precision and so noise overall will be higher.

def update_TP_TWIDDLE_MODE(args):
  return fn_update_TP_TWIDDLE_MODE()

def fn_update_TP_TWIDDLE_MODE():
  legal_set = [0,1]
  param_dict={}
  param_dict.update({"name" : "TP_TWIDDLE_MODE"})
  param_dict.update({"enum" : legal_set})
  return param_dict

def validate_TP_TWIDDLE_MODE(args):
  TP_TWIDDLE_MODE = args["TP_TWIDDLE_MODE"]
  return fn_validate_TP_TWIDDLE_MODE(TP_TWIDDLE_MODE)

def fn_validate_TP_TWIDDLE_MODE(TP_TWIDDLE_MODE):
  param_dict = fn_update_TP_TWIDDLE_MODE()
  legal_set_TP_TWIDDLE_MODE = param_dict["enum"]
  return validate_legal_set(legal_set_TP_TWIDDLE_MODE, "TP_TWIDDLE_MODE", TP_TWIDDLE_MODE)

#---------------------------------------------------
#Utility functions
#https://stackoverflow.com/a/57025941
#  every power of 2 has exactly 1 bit set to 1 (the bit in that number's log base-2 index).
# So when subtracting 1 from it, that bit flips to 0 and all preceding bits flip to 1.
# That makes these 2 numbers the inverse of each other so when AND-ing them, we will
#  get 0 as the result.
def fn_is_power_of_two(n):
  return (
    (n & (n-1) == 0) and n!=0 )

# good candidate for aie_common, especially if we want to give better error messages
# finds the first instance of a given parameter name in the list of parameters.
# Returns None if can't find it

#assumes n is a power of 2
def fn_log2(n):
  original_n = n
  if not fn_is_power_of_two(n):
    sys.exit("invalid assumption that n is a power of two")
  if n != 0:
    power_cnt = 0
    while n % 2 == 0:
      # keep right shifting until the power of two bit is at the LSB.
      n = n >> 1
      power_cnt+=1
      #print(f"n={n} and iter={power_cnt}")
      if n == 0 :
        sys.exit(f"Something went wrong when log2 {original_n}")
    return power_cnt
  else:
    sys.exit("Can't log2 0")
    #return Inf


  ######### Graph Generator ############

# Used by higher layer software to figure out how to connect blocks together.

def get_dyn_pt_port_info(portname, dir, TT_DATA, windowVSize, vectorLength=None, marginSize=0, TP_API=0):
  return [{
    "name" : f"{portname}[{idx}]" if vectorLength else f"{portname}", # portname no index
    "type" : "window" if TP_API==0 else "stream",
    "direction" : f"{dir}",
    "data_type" : TT_DATA,
    "fn_is_complex" : fn_is_complex(TT_DATA),
    "window_size" : fn_input_window_size(windowVSize, TT_DATA) + 32, #32bytes is the size of the header
    "margin_size": marginSize
  } for idx in range((vectorLength if vectorLength else 1))] # do just one port if vectorLength=None

def info_ports(args):
  """Standard function creating a static dictionary of information
  for upper software to correctly connect the IP.
  Some IP has dynamic number of ports according to parameter set,
  so port information has to be implemented as a function"""
  TT_DATA = args["TT_DATA"]
  TT_OUT_DATA = args["TT_OUT_DATA"]
  TP_WINDOW_VSIZE = args["TP_WINDOW_VSIZE"]
  TP_PARALLEL_POWER=args["TP_PARALLEL_POWER"]
  TP_API = args["TP_API"]
  TP_DYN_PT_SIZE = args["TP_DYN_PT_SIZE"]
  AIE_VARIANT = args["AIE_VARIANT"]
  marginSize = 0

  if TP_API == 0 :
    if TP_DYN_PT_SIZE == 0 :
      in_ports = get_port_info("in", "in", TT_DATA, (TP_WINDOW_VSIZE/2**(TP_PARALLEL_POWER)), 2**TP_PARALLEL_POWER, 0, TP_API)
      out_ports = get_port_info("out", "out", TT_OUT_DATA, (TP_WINDOW_VSIZE/2**(TP_PARALLEL_POWER)), 2**TP_PARALLEL_POWER, 0, TP_API)
    else :
      in_ports = get_dyn_pt_port_info("in", "in", TT_DATA, (TP_WINDOW_VSIZE/2**(TP_PARALLEL_POWER)), 2**TP_PARALLEL_POWER, 0, TP_API)
      out_ports = get_dyn_pt_port_info("out", "out", TT_OUT_DATA, (TP_WINDOW_VSIZE/2**(TP_PARALLEL_POWER)), 2**TP_PARALLEL_POWER, 0, TP_API)
  else :
    if AIE_VARIANT == 1 : #2 ports per kernel
      if TP_DYN_PT_SIZE == 0 :
        in_ports = get_port_info("in", "in", TT_DATA, (TP_WINDOW_VSIZE/2**(TP_PARALLEL_POWER+1)), 2**(TP_PARALLEL_POWER+1), 0, 1)
        out_ports = get_port_info("out", "out", TT_OUT_DATA, (TP_WINDOW_VSIZE/2**(TP_PARALLEL_POWER+1)), 2**(TP_PARALLEL_POWER+1), 0, 1)
      else:
        in_ports = get_dyn_pt_port_info("in", "in", TT_DATA, (TP_WINDOW_VSIZE/2**(TP_PARALLEL_POWER+1)), 2**(TP_PARALLEL_POWER+1), 0, TP_API)
        out_ports = get_dyn_pt_port_info("out", "out", TT_OUT_DATA, (TP_WINDOW_VSIZE/2**(TP_PARALLEL_POWER+1)), 2**(TP_PARALLEL_POWER+1), 0, TP_API)
    else : #1 port per kernel
      if TP_DYN_PT_SIZE == 0 :
        in_ports = get_port_info("in", "in", TT_DATA, (TP_WINDOW_VSIZE/2**(TP_PARALLEL_POWER)), 2**(TP_PARALLEL_POWER), 0, 1)
        out_ports = get_port_info("out", "out", TT_OUT_DATA, (TP_WINDOW_VSIZE/2**(TP_PARALLEL_POWER)), 2**(TP_PARALLEL_POWER), 0, 1)
      else:
        in_ports = get_dyn_pt_port_info("in", "in", TT_DATA, (TP_WINDOW_VSIZE/2**(TP_PARALLEL_POWER)), 2**(TP_PARALLEL_POWER), 0, TP_API)
        out_ports = get_dyn_pt_port_info("out", "out", TT_OUT_DATA, (TP_WINDOW_VSIZE/2**(TP_PARALLEL_POWER)), 2**(TP_PARALLEL_POWER), 0, TP_API)

  return in_ports + out_ports


def generate_graph(graphname, args):

  if graphname == "":
    graphname = "default_graphname"
  TT_DATA = args["TT_DATA"]
  TT_OUT_DATA = args["TT_OUT_DATA"]
  TT_TWIDDLE = args["TT_TWIDDLE"]
  TP_POINT_SIZE = args["TP_POINT_SIZE"]
  TP_FFT_NIFFT = args["TP_FFT_NIFFT"]
  TP_SHIFT = args["TP_SHIFT"]
  TP_CASC_LEN = args["TP_CASC_LEN"]
  TP_DYN_PT_SIZE = args["TP_DYN_PT_SIZE"]
  TP_WINDOW_VSIZE = args["TP_WINDOW_VSIZE"]
  TP_API = args["TP_API"]
  TP_PARALLEL_POWER = args["TP_PARALLEL_POWER"]
  TP_USE_WIDGETS = args["TP_USE_WIDGETS"]
  TP_RND = args["TP_RND"]
  TP_SAT = args["TP_SAT"]
  TP_TWIDDLE_MODE = args["TP_TWIDDLE_MODE"]
  AIE_VARIANT = args["AIE_VARIANT"]

  if TP_API == 0:
    ssr = 2**(TP_PARALLEL_POWER)
  else :
    if AIE_VARIANT == 1 :
      ssr = 2**(TP_PARALLEL_POWER+1)
    else :
      ssr = 2**(TP_PARALLEL_POWER)



  code = (
f"""
class {graphname} : public adf::graph {{
public:
  // ports
  template <typename dir>
  using ssr_port_array = std::array<adf::port<dir>, {ssr}>;

  ssr_port_array<input> in;
  ssr_port_array<output> out;


  xf::dsp::aie::fft::dit_1ch::fft_ifft_dit_1ch_graph<
    {TT_DATA},           // TT_DATA
    {TT_TWIDDLE},        // TT_TWIDDLE
    {TP_POINT_SIZE},     // TP_POINT_SIZE
    {TP_FFT_NIFFT},      // TP_FFT_NIFFT
    {TP_SHIFT},          // TP_SHIFT
    {TP_CASC_LEN},       // TP_CASC_LEN
    {TP_DYN_PT_SIZE},    // TP_DYN_PT_SIZE
    {TP_WINDOW_VSIZE},   // TP_WINDOW_VSIZE
    {TP_API},            // TP_API
    {TP_PARALLEL_POWER}, // TP_PARALLEL_POWER
    {TP_USE_WIDGETS},    // TP_USE_WIDGETS
    {TP_RND},            // TP_RND
    {TP_SAT},            // TP_SAT
    {TP_TWIDDLE_MODE},   // TP_TWIDDLE_MODE
    {TT_OUT_DATA}        // TT_OUT_DATA
  > fft_graph;

  {graphname}() : fft_graph() {{
    for (int i=0; i < {ssr}; i++) {{
      adf::connect<> net_in(in[i], fft_graph.in[i]);
      adf::connect<> net_out(fft_graph.out[i], out[i]);
    }}
  }}
}};
"""
  )
  out = {}
  out["graph"] = code
  out["port_info"] = info_ports(args)
  out["headerfile"] = "fft_ifft_dit_1ch_graph.hpp"
  out["searchpaths"] = [
       "L2/include/aie",
       "L2/tests/aie/common/inc",
       "L1/include/aie",
       "L1/src/aie",
       "L1/tests/aie/inc",
       "L1/tests/aie/src"
  ]
  return out

