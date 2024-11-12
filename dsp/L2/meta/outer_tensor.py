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

TP_SHIFT_max = 60
BUFFER_BYTES = 32
IO_BYTES_max_aie1 = 16384
IO_BYTES_max_aie2 = 32768
TP_SSR_min = 1
TP_SSR_max = 16

byteSize = {
  "float":4,
  "cfloat":8,
  "int16":2,
  "int32":4,
  "cint16":4,
  "cint32":8
}

aieVariantName = {
  1:"AIE",
  2:"AIE-ML"
}

#######################################################
###########AIE_VARIANT Updater and Validator ##########
#######################################################
def update_AIE_VARIANT(args):
  return fn_update_AIE_VARIANT()

def fn_update_AIE_VARIANT():
  legal_set_aie=[1, 2]
  param_dict={
    "name" : "AIE_VARIANT",
    "enum" : legal_set_aie
   }
  return param_dict

def validate_AIE_VARIANT(args):
  AIE_VARIANT = args["AIE_VARIANT"]
  return fn_validate_aie_variant(AIE_VARIANT)

def fn_validate_aie_variant(AIE_VARIANT):
  param_dict=fn_update_AIE_VARIANT()
  return(validate_legal_set(param_dict["enum"], "AIE_VARIANT", AIE_VARIANT))

#######################################################
########### TT_DATA_A Updater and Validator ###########
#######################################################
def update_TT_DATA_A(args):
  AIE_VARIANT=args["AIE_VARIANT"]
  return fn_update_TT_DATA_A(AIE_VARIANT)

def fn_update_TT_DATA_A(AIE_VARIANT):
  legal_set_TT_DATA_A=["int16", "cint16", "int32", "cint32", "float", "cfloat"]

  param_dict={
    "name" : "TT_DATA_A",
    "enum" : legal_set_TT_DATA_A
  }
  return param_dict

def validate_TT_DATA_A(args):
  AIE_VARIANT = args["AIE_VARIANT"]
  TT_DATA_A = args["TT_DATA_A"]
  return fn_validate_TT_DATA_A(AIE_VARIANT, TT_DATA_A)

def fn_validate_TT_DATA_A(AIE_VARIANT, TT_DATA_A):
  param_dict = fn_update_TT_DATA_A(AIE_VARIANT)
  return validate_legal_set(param_dict["enum"], "TT_DATA_A", TT_DATA_A)

#######################################################
########### TT_DATA_B Updater and Validator ###########
#######################################################
def update_TT_DATA_B(args):
  TT_DATA_A = args["TT_DATA_A"]
  return fn_update_TT_DATA_B(TT_DATA_A)

def fn_update_TT_DATA_B (TT_DATA_A):
  legal_set_TT_DATA_B=["int16", "cint16", "int32", "cint32", "float", "cfloat"]
  float_set=["float", "cfloat"]
  int_set=["int16", "cint16", "int32", "cint32"]
  if TT_DATA_A in int_set:
    legal_set_TT_DATA_B=remove_from_set(float_set,legal_set_TT_DATA_B)
  elif TT_DATA_A in float_set:
    legal_set_TT_DATA_B=remove_from_set(int_set,legal_set_TT_DATA_B)
  param_dict={
    "name" : "TT_DATA_B",
    "enum" : legal_set_TT_DATA_B
  }

  return param_dict

def validate_TT_DATA_B(args):
  TT_DATA_A = args["TT_DATA_A"]
  TT_DATA_B = args["TT_DATA_B"]
  return fn_validate_TT_DATA_B(TT_DATA_A, TT_DATA_B)

def fn_validate_TT_DATA_B(TT_DATA_A, TT_DATA_B):
  param_dict=fn_update_TT_DATA_B (TT_DATA_A)
  return(validate_legal_set(param_dict["enum"], "TT_DATA_B", TT_DATA_B))

#######################################################
########### TP_API Updater and Validator ##############
#######################################################
def update_TP_API(args):
  return fn_update_TP_API()

def fn_update_TP_API():
  legal_set_TP_API=[0, 1]
  param_dict={
    "name" : "TP_API",
    "enum" : legal_set_TP_API
  }
  return param_dict

def validate_TP_API(args):
    TP_API = args["TP_API"]
    return fn_validate_api_val(TP_API)

def fn_validate_api_val(TP_API):
  if TP_API != 0 and TP_API != 1:
    return isError("TP_API must be 0 (windowed) or streaming (1)")
  return isValid

#######################################################
########### TP_SSR Updater and Validator ##############
#######################################################

def update_TP_SSR(args):
  return fn_update_TP_SSR()

def fn_update_TP_SSR():
  legal_set_TP_SSR=[1,2,4,8,16]
  param_dict={
    "name" : "TP_SSR",
    "enum" : legal_set_TP_SSR
        }
  return param_dict

def validate_TP_SSR(args):
  TP_SSR=args["TP_SSR"]
  return fn_validate_TP_SSR(TP_SSR)

def fn_validate_TP_SSR(TP_SSR):
  param_dict=fn_update_TP_SSR()
  return(validate_legal_set(param_dict["enum"], "TP_SSR", TP_SSR))

#######################################################
########### TP_DIM_A Updater and Validator ############
#######################################################
def update_TP_DIM_A(args):
  AIE_VARIANT = args["AIE_VARIANT"]
  TT_DATA_A = args["TT_DATA_A"]
  TT_DATA_B = args["TT_DATA_B"]
  TP_API = args["TP_API"]
  TP_SSR = args["TP_SSR"]
  if args["TP_DIM_A"]: TP_DIM_A = args["TP_DIM_A"]
  else: TP_DIM_A = 0
  return fn_update_TP_DIM_A(AIE_VARIANT, TT_DATA_A, TT_DATA_B, TP_API, TP_SSR, TP_DIM_A)

def fn_update_TP_DIM_A(AIE_VARIANT, TT_DATA_A, TT_DATA_B, TP_API, TP_SSR, TP_DIM_A):

  if AIE_VARIANT == 1: IO_BYTES_max=IO_BYTES_max_aie1
  elif AIE_VARIANT == 2: IO_BYTES_max=IO_BYTES_max_aie2

  VEC_SIZE_A = BUFFER_BYTES/byteSize[TT_DATA_A]
  VEC_SIZE_B = BUFFER_BYTES/byteSize[TT_DATA_B]

  common_divisor= find_lcm(VEC_SIZE_A, TP_SSR)

  param_dict={
    "name" : "TP_DIM_A",
    "minimum" : common_divisor
  }

  TP_NUM_FRAMES=1
  if TP_API==0:
    TP_DIM_B=VEC_SIZE_B
    out_t=fn_det_out_type(TT_DATA_A, TT_DATA_B)
    TP_DIM_A_max=(IO_BYTES_max*TP_SSR)/(TP_DIM_B * byteSize[out_t] * TP_NUM_FRAMES)
    TP_DIM_A_max=int(FLOOR(TP_DIM_A_max, common_divisor))
    param_dict.update({"maximum" : TP_DIM_A_max})
  elif TP_API==1:
    TP_DIM_A_max=(IO_BYTES_max*TP_SSR)/(byteSize[TT_DATA_A] * TP_NUM_FRAMES)
    TP_DIM_A_max=int(FLOOR(TP_DIM_A_max, common_divisor))
    param_dict.update({"maximum" : TP_DIM_A_max})

  if (TP_DIM_A !=0) and (TP_DIM_A % common_divisor != 0):
    TP_DIM_A_act=round(TP_DIM_A/common_divisor) * common_divisor

    if TP_DIM_A_act < param_dict["minimum"]:
      TP_DIM_A_act = param_dict["minimum"]

    if TP_DIM_A_act > param_dict["maximum"]:
      TP_DIM_A_act = param_dict["maximum"]
    param_dict.update({"actual" : int(TP_DIM_A_act)})
  return param_dict

def validate_TP_DIM_A(args):
  AIE_VARIANT = args["AIE_VARIANT"]
  TT_DATA_A = args["TT_DATA_A"]
  TT_DATA_B = args["TT_DATA_B"]
  TP_API = args["TP_API"]
  TP_SSR = args["TP_SSR"]
  TP_DIM_A = args["TP_DIM_A"]
  return fn_validate_TP_DIM_A(AIE_VARIANT, TT_DATA_A, TT_DATA_B, TP_API, TP_SSR, TP_DIM_A)

def fn_validate_TP_DIM_A(AIE_VARIANT, TT_DATA_A, TT_DATA_B, TP_API, TP_SSR, TP_DIM_A):
  
  VEC_SIZE = BUFFER_BYTES/byteSize[TT_DATA_A]
  common_divisor=find_lcm(VEC_SIZE, TP_SSR)

  if (TP_DIM_A % common_divisor != 0):
    return isError(f"TP_DIM_A should be multiples of {common_divisor}!")
  else:
    param_dict=fn_update_TP_DIM_A(AIE_VARIANT, TT_DATA_A, TT_DATA_B, TP_API, TP_SSR, TP_DIM_A)
    range_TP_DIM_A=[param_dict["minimum"], param_dict["maximum"]]
    return validate_range(range_TP_DIM_A, "TP_DIM_A", TP_DIM_A)

#######################################################
########### TP_DIM_B Updater and Validator ############
#######################################################
def update_TP_DIM_B(args):
  AIE_VARIANT = args["AIE_VARIANT"]
  TT_DATA_A = args["TT_DATA_A"]
  TT_DATA_B = args["TT_DATA_B"]
  TP_API = args["TP_API"]
  TP_SSR = args["TP_SSR"]
  TP_DIM_A = args["TP_DIM_A"]
  if args["TP_DIM_B"]: TP_DIM_B = args["TP_DIM_B"]
  else: TP_DIM_B = 0
  return fn_update_TP_DIM_B(AIE_VARIANT, TT_DATA_A, TT_DATA_B, TP_API, TP_SSR, TP_DIM_A, TP_DIM_B)

def fn_update_TP_DIM_B(AIE_VARIANT, TT_DATA_A, TT_DATA_B, TP_API, TP_SSR, TP_DIM_A, TP_DIM_B):
  if AIE_VARIANT == 1: IO_BYTES_max=IO_BYTES_max_aie1
  elif AIE_VARIANT == 2: IO_BYTES_max=IO_BYTES_max_aie2

  VEC_SIZE_B = int(BUFFER_BYTES/byteSize[TT_DATA_B])

  param_dict={
    "name" : "TP_DIM_B",
    "minimum" : VEC_SIZE_B
  }

  TP_NUM_FRAMES=1
  if TP_API==0:
    out_t=fn_det_out_type(TT_DATA_A, TT_DATA_B)
    TP_DIM_B_max=(IO_BYTES_max*TP_SSR)/(TP_DIM_A * byteSize[out_t] * TP_NUM_FRAMES)
    TP_DIM_B_max=int(FLOOR(TP_DIM_B_max, VEC_SIZE_B))
  elif TP_API==1:
    TP_DIM_B_max=(IO_BYTES_max)/(byteSize[TT_DATA_B] * TP_NUM_FRAMES)
    TP_DIM_B_max=int(FLOOR(TP_DIM_B_max, VEC_SIZE_B))

  param_dict.update({"maximum" : TP_DIM_B_max})

  if TP_DIM_B !=0 and (TP_DIM_B % VEC_SIZE_B != 0):
    TP_DIM_B_act=round(TP_DIM_B/VEC_SIZE_B) * VEC_SIZE_B

    if TP_DIM_B_act < param_dict["minimum"]:
      TP_DIM_B_act = param_dict["minimum"]

    if TP_DIM_B_act > param_dict["maximum"]:
      TP_DIM_B_act = param_dict["maximum"]
    param_dict.update({"actual" : int(TP_DIM_B_act)})
  return param_dict

def validate_TP_DIM_B(args):
  AIE_VARIANT = args["AIE_VARIANT"]
  TT_DATA_A = args["TT_DATA_A"]
  TT_DATA_B = args["TT_DATA_B"]
  TP_API = args["TP_API"]
  TP_SSR = args["TP_SSR"]
  TP_DIM_A = args["TP_DIM_A"]
  TP_DIM_B = args["TP_DIM_B"]
  return fn_validate_TP_DIM_B(AIE_VARIANT, TT_DATA_A, TT_DATA_B, TP_API, TP_SSR, TP_DIM_A, TP_DIM_B)

def fn_validate_TP_DIM_B(AIE_VARIANT, TT_DATA_A, TT_DATA_B, TP_API, TP_SSR, TP_DIM_A, TP_DIM_B):
  
  VEC_SIZE_B = BUFFER_BYTES/byteSize[TT_DATA_B]

  if (TP_DIM_B % VEC_SIZE_B != 0):
    return isError(f"TP_DIM_B should be multiples of {VEC_SIZE_B}!")
  else:
    param_dict=fn_update_TP_DIM_B(AIE_VARIANT, TT_DATA_A, TT_DATA_B, TP_API, TP_SSR, TP_DIM_A, TP_DIM_B)
    range_TP_DIM_B=[param_dict["minimum"], param_dict["maximum"]]
    return validate_range(range_TP_DIM_B, "TP_DIM_B", TP_DIM_B)

#######################################################
########### TP_NUM_FRAMES Updater and Validator #######
#######################################################

def update_TP_NUM_FRAMES(args):
  AIE_VARIANT = args["AIE_VARIANT"]
  TT_DATA_A = args["TT_DATA_A"]
  TT_DATA_B = args["TT_DATA_B"]
  TP_API = args["TP_API"]
  TP_SSR = args["TP_SSR"]
  TP_DIM_A = args["TP_DIM_A"]
  TP_DIM_B = args["TP_DIM_B"]
  if args["TP_NUM_FRAMES"]: TP_NUM_FRAMES = args["TP_NUM_FRAMES"]
  else: TP_NUM_FRAMES = 0
  return fn_update_TP_NUM_FRAMES(AIE_VARIANT, TT_DATA_A, TT_DATA_B, TP_API, TP_SSR, TP_DIM_A, TP_DIM_B, TP_NUM_FRAMES)

def fn_update_TP_NUM_FRAMES(AIE_VARIANT, TT_DATA_A, TT_DATA_B, TP_API, TP_SSR, TP_DIM_A, TP_DIM_B, TP_NUM_FRAMES):
  
  param_dict={
    "name" : "TP_NUM_FRAMES",
    "minimum" : 1
  }
  
  if AIE_VARIANT == 1: IO_BYTES_max=IO_BYTES_max_aie1
  elif AIE_VARIANT == 2: IO_BYTES_max=IO_BYTES_max_aie2  

  if TP_API==0:
    out_t=fn_det_out_type(TT_DATA_A, TT_DATA_B)
    TP_NUM_FRAMES_max=(IO_BYTES_max*TP_SSR)/(TP_DIM_A * byteSize[out_t] * TP_DIM_B)
    min_power_2_lim = math.log2(TP_NUM_FRAMES_max)//1 
  elif TP_API==1:
    TP_NUM_FRAMES_max1 = (IO_BYTES_max*TP_SSR)/(byteSize[TT_DATA_A] * TP_DIM_A)
    TP_NUM_FRAMES_max2 = (IO_BYTES_max)/(byteSize[TT_DATA_B] * TP_DIM_B)
    TP_NUM_FRAMES_max =  min(TP_NUM_FRAMES_max1, TP_NUM_FRAMES_max2)
    min_power_2_lim = math.log2(TP_NUM_FRAMES_max)//1 

  param_dict.update({"maximum" : int(2**(min_power_2_lim))})

  
  if TP_NUM_FRAMES != 0:
    check_pwr2 = fn_validate_power_of_two("TP_NUM_FRAMES", TP_NUM_FRAMES)
    if check_pwr2 != "isValid": 
      min_power_2 = math.log2(TP_NUM_FRAMES)//1
      max_power_2 = min_power_2+1
      TP_NUM_FRAMES_act= 2**max_power_2
      if TP_NUM_FRAMES_act > param_dict["maximum"]:
        TP_NUM_FRAMES_act = param_dict["maximum"]
      
      param_dict.update({"actual" : int(TP_NUM_FRAMES_act)})

  return param_dict

def validate_TP_NUM_FRAMES(args):
  AIE_VARIANT = args["AIE_VARIANT"]
  TT_DATA_A = args["TT_DATA_A"]
  TT_DATA_B = args["TT_DATA_B"]
  TP_API = args["TP_API"]
  TP_SSR = args["TP_SSR"]
  TP_DIM_A = args["TP_DIM_A"]
  TP_DIM_B = args["TP_DIM_B"]
  TP_NUM_FRAMES = args["TP_NUM_FRAMES"]

  return fn_validate_TP_NUM_FRAMES(AIE_VARIANT, TT_DATA_A, TT_DATA_B, TP_API, TP_SSR, TP_DIM_A, TP_DIM_B, TP_NUM_FRAMES)

def fn_validate_TP_NUM_FRAMES(AIE_VARIANT, TT_DATA_A, TT_DATA_B, TP_API, TP_SSR, TP_DIM_A, TP_DIM_B, TP_NUM_FRAMES):
  param_dict=fn_update_TP_NUM_FRAMES(AIE_VARIANT, TT_DATA_A, TT_DATA_B, TP_API, TP_SSR, TP_DIM_A, TP_DIM_B, TP_NUM_FRAMES)
  range_TP_NUM_FRAMES=[param_dict["minimum"], param_dict["maximum"]]
  check=validate_range(range_TP_NUM_FRAMES, "TP_NUM_FRAMES", TP_NUM_FRAMES) 

  if check == isValid:
    return fn_validate_power_of_two("TP_NUM_FRAMES", TP_NUM_FRAMES)
  else: return check

#######################################################
########### TP_SHIFT Updater and Validator ############
#######################################################
def update_TP_SHIFT(args):
  TT_DATA_A = args["TT_DATA_A"]
  return fn_update_TP_SHIFT(TT_DATA_A)

def fn_update_TP_SHIFT(TT_DATA_A):
  param_dict= {
    "name" : "TP_SHIFT",
    "minimum" : 0
  }

  if TT_DATA_A in ["float", "cfloat"]:
    param_dict.update({"maximum" : 0})
  else:
    param_dict.update({"maximum" : 61})

  return param_dict

def validate_TP_SHIFT(args):
  TT_DATA_A = args["TT_DATA_A"]
  TP_SHIFT = args["TP_SHIFT"]
  return fn_validate_TP_SHIFT(TT_DATA_A, TP_SHIFT)

def fn_validate_TP_SHIFT(TT_DATA_A, TP_SHIFT):
  param_dict=fn_update_TP_SHIFT(TT_DATA_A)
  range_TP_SHIFT=[param_dict["minimum"], param_dict["maximum"]]
  return validate_range(range_TP_SHIFT, "TP_SHIFT", TP_SHIFT)

#######################################################
########### TP_RND Updater and Validator ##############
#######################################################
def update_TP_RND(args):
  AIE_VARIANT = args["AIE_VARIANT"]
  return fn_update_tp_rnd(AIE_VARIANT)

def fn_update_tp_rnd(AIE_VARIANT):

  legal_set_TP_RND = fn_get_legalSet_roundMode(AIE_VARIANT)
  param_dict={}
  param_dict.update({"name" : "TP_RND"})
  param_dict.update({"enum" : legal_set_TP_RND})

  return param_dict

def validate_TP_RND(args):
  AIE_VARIANT = args["AIE_VARIANT"]
  TP_RND = args["TP_RND"]
  return (fn_validate_TP_RND(AIE_VARIANT, TP_RND))

def fn_validate_TP_RND(AIE_VARIANT, TP_RND):
  legal_set_TP_RND = fn_get_legalSet_roundMode(AIE_VARIANT)
  return(validate_legal_set(legal_set_TP_RND, "TP_RND", TP_RND))

#######################################################
########### TP_RND Updater and Validator ##############
#######################################################
def update_TP_SAT(args):
  return fn_update_tp_sat()

def fn_update_tp_sat():
  legal_set = fn_legal_set_sat()

  param_dict={}
  param_dict.update({"name" : "TP_SAT"})
  param_dict.update({"enum" : legal_set})
  return param_dict

def validate_TP_SAT(args):
  TP_SAT = args["TP_SAT"]
  return (fn_validate_TP_SAT(TP_SAT))

def fn_validate_TP_SAT(TP_SAT):
  param_dict=fn_update_tp_sat()
  legal_set_TP_SAT = param_dict["enum"]
  return (validate_legal_set(legal_set_TP_SAT, "TP_SAT", TP_SAT))

## utility functions
def fn_det_out_type(TT_DATA_A, TT_DATA_B):
  if (TT_DATA_A=="int16" and TT_DATA_B=="int16"):
    return "int16"
  if (TT_DATA_A=="int16" and TT_DATA_B=="int32"):
    return "int32"
  if (TT_DATA_A=="int16" and TT_DATA_B=="cint16"):
    return "cint16"
  if (TT_DATA_A=="int16" and TT_DATA_B=="cint32"):
    return "cint32"
  if (TT_DATA_A=="int32" and TT_DATA_B=="int16"):
    return "int32"
  if (TT_DATA_A=="int32" and TT_DATA_B=="int32"):
    return "int32"
  if (TT_DATA_A=="int32" and TT_DATA_B=="cint16"):
    return "cint32"   
  if (TT_DATA_A=="int32" and TT_DATA_B=="cint32"):
    return "cint32"  
  if (TT_DATA_A=="cint16" and TT_DATA_B=="int16"):
    return "cint16"
  if (TT_DATA_A=="cint16" and TT_DATA_B=="int32"):
    return "cint32"
  if (TT_DATA_A=="cint16" and TT_DATA_B=="cint16"):
    return "cint16"
  if (TT_DATA_A=="cint16" and TT_DATA_B=="cint32"):
    return "cint32"
  if (TT_DATA_A=="cint32" and TT_DATA_B=="int16"):
    return "cint32"
  if (TT_DATA_A=="cint32" and TT_DATA_B=="int32"):
    return "cint32"
  if (TT_DATA_A=="cint32" and TT_DATA_B=="cint16"):
    return "cint32"
  if (TT_DATA_A=="cint32" and TT_DATA_B=="cint32"):
    return "cint32"
  if (TT_DATA_A=="float" and TT_DATA_B=="float"):
    return "float"
  if (TT_DATA_A=="float" and TT_DATA_B=="cfloat"):
    return "cfloat"
  if (TT_DATA_A=="cfloat" and TT_DATA_B=="float"):
    return "cfloat"
  if (TT_DATA_A=="cfloat" or TT_DATA_B=="cfloat"):
    return "cfloat"

#### port ####
def get_port_info(portname, dir, dataType, dim, numFrames, apiType, vectorLength):
  return [{
    "name" : f"{portname}[{idx}]",
    "type" : f"{apiType}",
    "direction": f"{dir}",
    "data_type": dataType,
    "fn_is_complex": fn_is_complex(dataType),
    "window_size" : dim*numFrames, #com.fn_input_window_size(windowVsize, dataType),
    "margin_size" : 0
} for idx in range(vectorLength)]

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
  if (TP_API==0):
    portsInA = get_port_info(
      portname = "inA",
      dir = "in",
      dataType = TT_DATA_A,
      dim = TP_DIM_A,
      numFrames = TP_NUM_FRAMES,
      apiType = "window",
      vectorLength = TP_SSR
    )
    portsInB = get_port_info(
      portname = "inB",
      dir = "in",
      dataType = TT_DATA_B,
      dim = TP_DIM_B,
      numFrames = TP_NUM_FRAMES,
      apiType = "window",
      vectorLength = TP_SSR
    )
    portsOut = get_port_info(
      portname = "out",
      dir = "out",
      dataType = fn_det_out_type(TT_DATA_A, TT_DATA_B),
      dim = TP_DIM_A*TP_DIM_B,
      numFrames = TP_NUM_FRAMES,
      apiType = "window",
      vectorLength = TP_SSR
    )
  else:
    portsInA = get_port_info(
      portname = "inA",
      dir = "in",
      dataType = TT_DATA_A,
      dim = TP_DIM_A,
      numFrames = TP_NUM_FRAMES,
      apiType = "stream",
      vectorLength = TP_SSR
    )
    portsInB = get_port_info(
      portname = "inB",
      dir = "in",
      dataType = TT_DATA_B,
      dim = TP_DIM_B,
      numFrames = TP_NUM_FRAMES,
      apiType = "stream",
      vectorLength = TP_SSR
    )
    portsOut = get_port_info(
      portname = "out",
      dir = "out",
      dataType = fn_det_out_type(TT_DATA_A, TT_DATA_B),
      dim = TP_DIM_A*TP_DIM_B,
      numFrames = TP_NUM_FRAMES,
      apiType = "stream",
      vectorLength = TP_SSR
    )
  return portsInA+portsInB+portsOut


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

  if TP_API == 1:
    ssr = TP_SSR//2
  else:
    ssr = TP_SSR

  # Use formatted multi-line string to avoid a lot of \n and \t
  code  = (
f"""
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
    {ssr}, //TP_SSR
  > outer_tensor;

  {graphname}() : outer_tensor() {{
    adf::kernel *outer_tensor_kernels = outer_tensor.getKernels();
    for (int i=0; i < 1; i++) {{
      adf::runtime<ratio>(outer_tensor_kernels[i]) = 0.9;
    }}
    for (int i=0; i < TP_SSR; i++) {{
      adf::connect<> net_in(inA[i], outer_tensor.inA[i]);
      adf::connect<> net_in(inB[i], outer_tensor.inB[i]);
      adf::connect<> net_out(outer_tensor.out[i], out[i]);
    }}
  }}

}};
""")
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
       "L1/tests/aie/src"]

  return out
