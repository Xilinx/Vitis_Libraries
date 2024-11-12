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
import aie_common as com
from aie_common import *
TP_SSR_min = 1
TP_SSR_max = 16

TP_CASC_min = 1
TP_CASC_max = 16

aie1_pp_buffer=16384
aie2_pp_buffer=32768

#######################################################
########### AIE_VARIANT Updater and Validator #########
#######################################################
def update_AIE_VARIANT(args):
  return fn_update_AIE_VARIANT()

def fn_update_AIE_VARIANT():
  legal_set_AIE_VARIANT = [1,2]
  
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
########### TT_DATA_A Updater and Validator ###########
#######################################################
def update_TT_DATA_A(args):
  AIE_VARIANT = args["AIE_VARIANT"]
  return fn_update_TT_DATA_A(AIE_VARIANT)

def fn_update_TT_DATA_A(AIE_VARIANT):
  legal_set_TT_DATA_A=["int16", "cint16", "int32", "cint32", "float", "cfloat"]
  float_set=["float", "cfloat"]
  if AIE_VARIANT==2:
    legal_set_TT_DATA_A = remove_from_set(float_set, legal_set_TT_DATA_A)

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
  AIE_VARIANT = args["AIE_VARIANT"]
  return fn_update_TT_DATA_B(TT_DATA_A, AIE_VARIANT)

def fn_update_TT_DATA_B(TT_DATA_A, AIE_VARIANT):
  legal_set_TT_DATA_B=["int16", "cint16", "int32", "cint32", "float", "cfloat"]
  int_set=["int16", "cint16", "int32", "cint32"]
  float_set=["float", "cfloat"]

  if (TT_DATA_A in int_set)     :legal_set_TT_DATA_B=remove_from_set(float_set, legal_set_TT_DATA_B)
  elif (TT_DATA_A in float_set) :legal_set_TT_DATA_B=remove_from_set(int_set, legal_set_TT_DATA_B)

  # check which TT_DATA_B types have supported tiling scheme with TT_DATA_A
  for typeB in legal_set_TT_DATA_B:
    if getTilingScheme(TT_DATA_A, typeB, AIE_VARIANT) == (0,0,0):
      legal_set_TT_DATA_B = remove_from_set([typeB], legal_set_TT_DATA_B)

  param_dict={
    "name": "TT_DATA_A",
    "enum": legal_set_TT_DATA_B
  }
  return param_dict

def validate_TT_DATA_B(args):
  TT_DATA_A = args["TT_DATA_A"]
  TT_DATA_B = args["TT_DATA_B"]
  AIE_VARIANT = args["AIE_VARIANT"]
  return fn_validate_TT_DATA_B(TT_DATA_A, TT_DATA_B, AIE_VARIANT)

def fn_validate_TT_DATA_B(TT_DATA_A, TT_DATA_B, AIE_VARIANT):
  param_dict=fn_update_TT_DATA_B(TT_DATA_A, AIE_VARIANT)
  return validate_legal_set(param_dict["enum"], "TT_DATA_B", TT_DATA_B)

#######################################################
########### TP_DIM_A Updater and Validator ############
#######################################################
def update_TP_DIM_A(args):
  AIE_VARIANT = args["AIE_VARIANT"]
  TT_DATA_A = args["TT_DATA_A"]
  TT_DATA_B = args["TT_DATA_B"]

  if args["TP_DIM_A"] : TP_DIM_A=args["TP_DIM_A"]
  else:TP_DIM_A=0

  return fn_update_TP_DIM_A(AIE_VARIANT, TT_DATA_A, TT_DATA_B, TP_DIM_A)

def fn_update_TP_DIM_A(AIE_VARIANT, TT_DATA_A, TT_DATA_B, TP_DIM_A):
  (tileA, tileAB, tileB) = getTilingScheme(TT_DATA_A, TT_DATA_B, AIE_VARIANT)

  TT_OUT = getOutputType(TT_DATA_A, TT_DATA_B)
  if AIE_VARIANT==1: 
    max_buffer_sample_in=aie1_pp_buffer/fn_size_by_byte(TT_DATA_A)
    max_buffer_sample_out=aie1_pp_buffer/fn_size_by_byte(TT_OUT)
  elif AIE_VARIANT==2: 
    max_buffer_sample_in=aie2_pp_buffer/fn_size_by_byte(TT_DATA_A)
    max_buffer_sample_out=aie2_pp_buffer/fn_size_by_byte(TT_OUT)

  TP_SSR=16
  TP_CASC=16
  TP_DIM_B=tileB
  TP_DIM_AB=tileAB

  TP_DIM_A_max_buffer_in=(max_buffer_sample_in*TP_SSR*TP_CASC)/(TP_DIM_AB)
  TP_DIM_A_max_buffer_out=(max_buffer_sample_out*TP_SSR)/(TP_DIM_B)
  # print(f"TP_DIM_A_max_buffer_in={TP_DIM_A_max_buffer_in}")
  # print(f"TP_DIM_A_max_buffer_out={TP_DIM_A_max_buffer_out}")
  TP_DIM_A_max=min(TP_DIM_A_max_buffer_in, TP_DIM_A_max_buffer_out)    

  param_dict={
    "name" : "TP_DIM_A",
    "minimum" : tileA,
    "maximum" : int(FLOOR(TP_DIM_A_max,tileA))
  }

  if TP_DIM_A != 0 and (TP_DIM_A % tileA != 0): 
    TP_DIM_A_act=round(TP_DIM_A/tileA) * tileA

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
  TP_DIM_A = args["TP_DIM_A"]
  return fn_validate_TP_DIM_A(AIE_VARIANT, TT_DATA_A, TT_DATA_B, TP_DIM_A)

def fn_validate_TP_DIM_A(AIE_VARIANT, TT_DATA_A, TT_DATA_B, TP_DIM_A):

  (tileA, tileAB, tileB) = getTilingScheme(TT_DATA_A, TT_DATA_B, AIE_VARIANT)

  if TP_DIM_A%tileA !=0:
    return isError(f"TP_DIM_A should be a multiple of {tileA}!")
  else:
    param_dict=fn_update_TP_DIM_A(AIE_VARIANT, TT_DATA_A, TT_DATA_B, TP_DIM_A)
    range_TP_DIM_A=[param_dict["minimum"], param_dict["maximum"]]
    return validate_range(range_TP_DIM_A, "TP_DIM_A", TP_DIM_A)

#######################################################
########### TP_DIM_B Updater and Validator ############
#######################################################
def update_TP_DIM_B(args):
  AIE_VARIANT = args["AIE_VARIANT"]
  TT_DATA_A = args["TT_DATA_A"]
  TT_DATA_B = args["TT_DATA_B"]
  TP_DIM_A = args["TP_DIM_A"]

  if args["TP_DIM_B"] : TP_DIM_B=args["TP_DIM_B"]
  else:TP_DIM_B=0

  return fn_update_TP_DIM_B(AIE_VARIANT, TT_DATA_A, TT_DATA_B, TP_DIM_A, TP_DIM_B)

def fn_update_TP_DIM_B(AIE_VARIANT, TT_DATA_A, TT_DATA_B, TP_DIM_A, TP_DIM_B):
  (tileA, tileAB, tileB) = getTilingScheme(TT_DATA_A, TT_DATA_B, AIE_VARIANT)

  TT_OUT = getOutputType(TT_DATA_A, TT_DATA_B)
  if AIE_VARIANT==1: 
    max_buffer_sample_in=aie1_pp_buffer/fn_size_by_byte(TT_DATA_B)
    max_buffer_sample_out=aie1_pp_buffer/fn_size_by_byte(TT_OUT)
  elif AIE_VARIANT==2: 
    max_buffer_sample_in=aie2_pp_buffer/fn_size_by_byte(TT_DATA_B)
    max_buffer_sample_in=aie2_pp_buffer/fn_size_by_byte(TT_DATA_B)

  TP_CASC=16
  TP_SSR=16
  TP_DIM_AB=tileAB
  TP_DIM_B_max_in=max_buffer_sample_in*TP_CASC/(TP_DIM_AB)
  TP_DIM_B_max_out=max_buffer_sample_in*TP_SSR/TP_DIM_A
  TP_DIM_B_max=min(TP_DIM_B_max_in, TP_DIM_B_max_out)

  param_dict={
    "name" : "TP_DIM_B",
    "minimum" : tileB,
    "maximum" : int(TP_DIM_B_max)
  }

  if TP_DIM_B != 0 and (TP_DIM_B % tileB != 0): 
    TP_DIM_B_act=round(TP_DIM_B/tileB) * tileB

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
  TP_DIM_A = args["TP_DIM_A"]
  TP_DIM_B = args["TP_DIM_B"]
  return fn_validate_TP_DIM_B(AIE_VARIANT, TT_DATA_A, TT_DATA_B, TP_DIM_A, TP_DIM_B)

def fn_validate_TP_DIM_B(AIE_VARIANT, TT_DATA_A, TT_DATA_B, TP_DIM_A, TP_DIM_B):
 
  (tileA, tileAB, tileB) = getTilingScheme(TT_DATA_A, TT_DATA_B, AIE_VARIANT)

  if TP_DIM_B%tileB !=0:
    return isError(f"TP_DIM_B should be a multiple of {tileB}!")
  else:
    param_dict=fn_update_TP_DIM_B(AIE_VARIANT, TT_DATA_A, TT_DATA_B, TP_DIM_A, TP_DIM_B)
    range_TP_DIM_B=[param_dict["minimum"], param_dict["maximum"]]
    return validate_range(range_TP_DIM_B, "TP_DIM_B", TP_DIM_B)

#######################################################
########### TP_DIM_AB Updater and Validator ###########
#######################################################
def update_TP_DIM_AB(args):
  AIE_VARIANT = args["AIE_VARIANT"]
  TT_DATA_A = args["TT_DATA_A"]
  TT_DATA_B = args["TT_DATA_B"]
  TP_DIM_A = args["TP_DIM_A"]
  TP_DIM_B = args["TP_DIM_B"]

  if args["TP_DIM_AB"] : TP_DIM_AB=args["TP_DIM_AB"]
  else:TP_DIM_AB=0

  return fn_update_TP_DIM_AB(AIE_VARIANT, TT_DATA_A, TT_DATA_B, TP_DIM_A, TP_DIM_B, TP_DIM_AB)

def fn_update_TP_DIM_AB(AIE_VARIANT, TT_DATA_A, TT_DATA_B, TP_DIM_A, TP_DIM_B, TP_DIM_AB):
  (tileA, tileAB, tileB) = getTilingScheme(TT_DATA_A, TT_DATA_B, AIE_VARIANT)

  if AIE_VARIANT==1: 
    max_buffer_sample_a=aie1_pp_buffer/fn_size_by_byte(TT_DATA_A)
    max_buffer_sample_b=aie1_pp_buffer/fn_size_by_byte(TT_DATA_B)
  elif AIE_VARIANT==2: 
    max_buffer_sample_a=aie2_pp_buffer/fn_size_by_byte(TT_DATA_A)
    max_buffer_sample_b=aie2_pp_buffer/fn_size_by_byte(TT_DATA_B)

  TP_SSR=16
  TP_CASC=16
  TP_DIM_AB_max1=(max_buffer_sample_a * TP_SSR * TP_CASC)/TP_DIM_A
  TP_DIM_AB_max2=(max_buffer_sample_b * TP_CASC)/TP_DIM_B
  TP_DIM_AB_max=min(TP_DIM_AB_max1, TP_DIM_AB_max2)


  param_dict={
    "name" : "TP_DIM_AB",
    "minimum" : tileAB,
    "maximum" : int(TP_DIM_AB_max)
  }

  if TP_DIM_AB != 0 and (TP_DIM_AB % tileAB != 0): 
    TP_DIM_AB_act=round(TP_DIM_AB/tileAB) * tileAB

    if TP_DIM_AB_act < param_dict["minimum"]:
      TP_DIM_AB_act = param_dict["minimum"]

    if TP_DIM_AB_act > param_dict["maximum"]:
      TP_DIM_AB_act = param_dict["maximum"]
    
    param_dict.update({"actual" : int(TP_DIM_AB_act)})

  return param_dict

def validate_TP_DIM_AB(args):
  AIE_VARIANT = args["AIE_VARIANT"]
  TT_DATA_A = args["TT_DATA_A"]
  TT_DATA_B = args["TT_DATA_B"]
  TP_DIM_A = args["TP_DIM_A"]
  TP_DIM_B = args["TP_DIM_B"]
  TP_DIM_AB = args["TP_DIM_AB"]
  return fn_validate_TP_DIM_AB(AIE_VARIANT, TT_DATA_A, TT_DATA_B, TP_DIM_A, TP_DIM_B, TP_DIM_AB)

def fn_validate_TP_DIM_AB(AIE_VARIANT, TT_DATA_A, TT_DATA_B, TP_DIM_A, TP_DIM_B, TP_DIM_AB):
  (tileA, tileAB, tileB) = getTilingScheme(TT_DATA_A, TT_DATA_B, AIE_VARIANT)

  if TP_DIM_AB%tileAB != 0:
    return isError(f"TP_DIM_AB should be a mulriple of {tileAB}!")
  else:
    param_dict=fn_update_TP_DIM_AB(AIE_VARIANT, TT_DATA_A, TT_DATA_B, TP_DIM_A, TP_DIM_B, TP_DIM_AB)
    range_TP_DIM_AB=[param_dict["minimum"], param_dict["maximum"]]
    return validate_range(range_TP_DIM_AB, "TP_DIM_AB", TP_DIM_AB)
  
#######################################################
############ TP_SSR Updater and Validator #############
#######################################################
def update_TP_SSR(args):
  AIE_VARIANT=args["AIE_VARIANT"]
  TT_DATA_A=args["TT_DATA_A"]
  TP_DIM_A=args["TP_DIM_A"]
  TP_DIM_AB=args["TP_DIM_AB"]
  return fn_update_ssr(AIE_VARIANT, TT_DATA_A, TP_DIM_A, TP_DIM_AB)

def fn_update_ssr(AIE_VARIANT, TT_DATA_A, TP_DIM_A, TP_DIM_AB):
  if AIE_VARIANT==1: 
    max_buffer_sample_a=aie1_pp_buffer/fn_size_by_byte(TT_DATA_A)
  elif AIE_VARIANT==2: 
    max_buffer_sample_a=aie2_pp_buffer/fn_size_by_byte(TT_DATA_A)

  max_buffer_a=(max_buffer_sample_a*TP_CASC_max/TP_DIM_AB)

  legal_set_TP_SSR=find_divisors(TP_DIM_A, TP_SSR_max)

  for k in legal_set_TP_SSR.copy():
    if (TP_DIM_A/k)>max_buffer_a:
      legal_set_TP_SSR.remove(k)

  param_dict={
    "name" : "TP_SSR",
    "enum" : legal_set_TP_SSR
  }
  return param_dict

def validate_TP_SSR(args):
  AIE_VARIANT=args["AIE_VARIANT"]
  TT_DATA_A=args["TT_DATA_A"]
  TP_DIM_A=args["TP_DIM_A"]
  TP_DIM_AB=args["TP_DIM_AB"]
  TP_SSR=args["TP_SSR"]
  return fn_validate_ssr(AIE_VARIANT, TT_DATA_A, TP_DIM_A, TP_DIM_AB, TP_SSR)

def fn_validate_ssr(AIE_VARIANT, TT_DATA_A, TP_DIM_A, TP_DIM_AB, TP_SSR):
  param_dict=fn_update_ssr(AIE_VARIANT, TT_DATA_A, TP_DIM_A, TP_DIM_AB)
  legal_set_TP_SSR=param_dict["enum"]
  return validate_legal_set(legal_set_TP_SSR, "TP_SSR", TP_SSR)

#######################################################
############ TP_CASC Updater and Validator ############
#######################################################
def update_TP_CASC_LEN(args):
  AIE_VARIANT=args["AIE_VARIANT"]
  TT_DATA_A=args["TT_DATA_A"]
  TT_DATA_B=args["TT_DATA_B"]
  TP_DIM_A=args["TP_DIM_A"]
  TP_DIM_B=args["TP_DIM_B"]
  TP_DIM_AB=args["TP_DIM_AB"]
  TP_SSR=args["TP_SSR"]
  return fn_update_TP_CASC(AIE_VARIANT, TT_DATA_A, TT_DATA_B, TP_DIM_A, TP_DIM_B, TP_DIM_AB, TP_SSR)

def fn_update_TP_CASC(AIE_VARIANT, TT_DATA_A, TT_DATA_B, TP_DIM_A, TP_DIM_B, TP_DIM_AB, TP_SSR):
  if AIE_VARIANT==1: 
    max_buffer_sample_a=aie1_pp_buffer/fn_size_by_byte(TT_DATA_A)
    max_buffer_sample_b=aie1_pp_buffer/fn_size_by_byte(TT_DATA_B)
  elif AIE_VARIANT==2: 
    max_buffer_sample_a=aie2_pp_buffer/fn_size_by_byte(TT_DATA_A)
    max_buffer_sample_b=aie2_pp_buffer/fn_size_by_byte(TT_DATA_B)

  max_buffer_a=(max_buffer_sample_a*TP_SSR)/TP_DIM_A
  max_buffer_b=max_buffer_sample_b/TP_DIM_B
  max_buffer=min(max_buffer_a,max_buffer_b)
  legal_set_TP_CASC=find_divisors(TP_DIM_AB, TP_CASC_max)

  for k in legal_set_TP_CASC.copy():
    if (TP_DIM_AB/k)>max_buffer:
      legal_set_TP_CASC.remove(k)

  param_dict={
    "name" : "TP_CASC_LEN",
    "enum" : legal_set_TP_CASC
  }
  return param_dict

def validate_TP_CASC_LEN(args):
  AIE_VARIANT=args["AIE_VARIANT"]
  TT_DATA_A=args["TT_DATA_A"]
  TT_DATA_B=args["TT_DATA_B"]
  TP_DIM_A=args["TP_DIM_A"]
  TP_DIM_B=args["TP_DIM_B"]
  TP_DIM_AB=args["TP_DIM_AB"]
  TP_SSR=args["TP_SSR"]
  TP_CASC_LEN=args["TP_CASC_LEN"]
  return fn_validate_TP_CASC(AIE_VARIANT, TT_DATA_A, TT_DATA_B, TP_DIM_A, TP_DIM_B, TP_DIM_AB, TP_SSR, TP_CASC_LEN)

def fn_validate_TP_CASC(AIE_VARIANT, TT_DATA_A, TT_DATA_B, TP_DIM_A, TP_DIM_B, TP_DIM_AB, TP_SSR, TP_CASC_LEN):
  param_dict=fn_update_TP_CASC(AIE_VARIANT, TT_DATA_A, TT_DATA_B, TP_DIM_A, TP_DIM_B, TP_DIM_AB, TP_SSR)
  return validate_legal_set(param_dict["enum"], "TP_CASC_LEN", TP_CASC_LEN)

#######################################################
### TP_INPUT_WINDOW_VSIZE_A Updater and Validator #####
#######################################################
def update_TP_INPUT_WINDOW_VSIZE_A(args):
  TP_DIM_A = args["TP_DIM_A"]
  TP_DIM_AB = args["TP_DIM_AB"]
  return fn_update_TP_INPUT_WINDOW_VSIZE_A(TP_DIM_A, TP_DIM_AB)

def fn_update_TP_INPUT_WINDOW_VSIZE_A(TP_DIM_A, TP_DIM_AB):

  TP_INPUT_WINDOW_VSIZE_A_min = int(TP_DIM_A* TP_DIM_AB)
  param_dict={
    "name" : "TP_INPUT_WINDOW_VSIZE_A",
    "minimum" : TP_INPUT_WINDOW_VSIZE_A_min,
    "maximum" : TP_INPUT_WINDOW_VSIZE_A_min
  }

  return param_dict

def validate_TP_INPUT_WINDOW_VSIZE_A(args):
  TP_DIM_A = args["TP_DIM_A"]
  TP_DIM_AB = args["TP_DIM_AB"]
  TP_INPUT_WINDOW_VSIZE_A = args["TP_INPUT_WINDOW_VSIZE_A"]

  return fn_validate_TP_INPUT_WINDOW_VSIZE_A(TP_DIM_A, TP_DIM_AB, TP_INPUT_WINDOW_VSIZE_A)

def fn_validate_TP_INPUT_WINDOW_VSIZE_A(TP_DIM_A, TP_DIM_AB, TP_INPUT_WINDOW_VSIZE_A):
  param_dict=fn_update_TP_INPUT_WINDOW_VSIZE_A(TP_DIM_A, TP_DIM_AB)
  range_TP_INPUT_WINDOW_VSIZE_A=[param_dict["minimum"], param_dict["maximum"]]
  return validate_range(range_TP_INPUT_WINDOW_VSIZE_A, "TP_INPUT_WINDOW_VSIZE_A", TP_INPUT_WINDOW_VSIZE_A)

#######################################################
### TP_INPUT_WINDOW_VSIZE_B Updater and Validator #####
#######################################################
  
def update_TP_INPUT_WINDOW_VSIZE_B(args):
  TP_DIM_B = args["TP_DIM_B"]
  TP_DIM_AB = args["TP_DIM_AB"]
  return fn_update_TP_INPUT_WINDOW_VSIZE_B(TP_DIM_B, TP_DIM_AB)

def fn_update_TP_INPUT_WINDOW_VSIZE_B(TP_DIM_B, TP_DIM_AB):

  TP_INPUT_WINDOW_VSIZE_B_min = int(TP_DIM_B* TP_DIM_AB)

  param_dict={
    "name" : "TP_INPUT_WINDOW_VSIZE_B",
    "minimum" : TP_INPUT_WINDOW_VSIZE_B_min,
    "maximum" : TP_INPUT_WINDOW_VSIZE_B_min}

  return param_dict

def validate_TP_INPUT_WINDOW_VSIZE_B(args):
  TP_INPUT_WINDOW_VSIZE_B = args["TP_INPUT_WINDOW_VSIZE_B"]
  TP_DIM_AB = args["TP_DIM_AB"]
  TP_DIM_B = args["TP_DIM_B"]
  return fn_validate_TP_INPUT_WINDOW_VSIZE_B(TP_DIM_B, TP_DIM_AB, TP_INPUT_WINDOW_VSIZE_B)

def fn_validate_TP_INPUT_WINDOW_VSIZE_B(TP_DIM_B, TP_DIM_AB, TP_INPUT_WINDOW_VSIZE_B):
  param_dict=fn_update_TP_INPUT_WINDOW_VSIZE_B(TP_DIM_B, TP_DIM_AB)
  range_TP_INPUT_WINDOW_VSIZE_B=[param_dict["minimum"], param_dict["maximum"]]
  return validate_range(range_TP_INPUT_WINDOW_VSIZE_B, "TP_INPUT_WINDOW_VSIZE_B", TP_INPUT_WINDOW_VSIZE_B)
  
#######################################################
######### TP_DIM_A_LEADING Updater and Validator ######
#######################################################
def update_TP_DIM_A_LEADING(args):
  return fn_update_TP_DIM_A_LEADING()

def fn_update_TP_DIM_A_LEADING():
  legal_set_TP_DIM_A_LEADING=[0,1]

  param_dict={
    "name" : "TP_DIM_A_LEADING",
    "enum" : legal_set_TP_DIM_A_LEADING
  }

  return param_dict

def validate_TP_DIM_A_LEADING(args):
  TP_DIM_A_LEADING = args["TP_DIM_A_LEADING"]
  return fn_validate_TP_DIM_A_LEADING(TP_DIM_A_LEADING)

def fn_validate_TP_DIM_A_LEADING(TP_DIM_A_LEADING):
  param_dict=fn_update_TP_DIM_A_LEADING()
  return validate_legal_set(param_dict["enum"], "TP_DIM_A_LEADING", TP_DIM_A_LEADING)

#######################################################
######### TP_DIM_B_LEADING Updater and Validator ######
#######################################################
def update_TP_DIM_B_LEADING(args):
  return fn_update_TP_DIM_B_LEADING()

def fn_update_TP_DIM_B_LEADING():

  legal_set=[0, 1]

  param_dict={
    "name" : "TP_DIM_B_LEADING",
    "enum" : legal_set
  }
  return param_dict

def validate_TP_DIM_B_LEADING(args):
  TP_DIM_B_LEADING = args["TP_DIM_B_LEADING"]
  return fn_validate_TP_DIM_B_LEADING(TP_DIM_B_LEADING)

def fn_validate_TP_DIM_B_LEADING(TP_DIM_B_LEADING):
  param_dict=fn_update_TP_DIM_B_LEADING()
  return validate_legal_set(param_dict["enum"], "TP_DIM_B_LEADING", TP_DIM_B_LEADING)

#######################################################
######### TP_DIM_OUT_LEADING Updater and Validator ####
#######################################################
def update_TP_DIM_OUT_LEADING(args):
  return fn_update_TP_DIM_OUT_LEADING()

def fn_update_TP_DIM_OUT_LEADING():
  legal_set=[0, 1]

  param_dict={
    "name" : "TP_DIM_OUT_LEADING",
    "enum" : legal_set
  }
  return param_dict

def validate_TP_DIM_OUT_LEADING(args):
  TP_DIM_OUT_LEADING = args["TP_DIM_OUT_LEADING"]
  return fn_validate_TP_DIM_OUT_LEADING(TP_DIM_OUT_LEADING)

def fn_validate_TP_DIM_OUT_LEADING(TP_DIM_OUT_LEADING):
  param_dict=fn_update_TP_DIM_OUT_LEADING()
  return validate_legal_set(param_dict["enum"], "TP_DIM_OUT_LEADING", TP_DIM_OUT_LEADING)

#######################################################
######## TP_ADD_TILING_A Updater and Validator ########
#######################################################
def update_TP_ADD_TILING_A(args):
  TT_DATA_A=args["TT_DATA_A"]
  TP_DIM_A=args["TP_DIM_A"]
  TP_DIM_AB=args["TP_DIM_AB"]
  TP_SSR=args["TP_SSR"]
  TP_CASC_LEN=args["TP_CASC_LEN"]
  TP_DIM_A_LEADING=args["TP_ADD_TILING_A"]
  return fn_update_TP_ADD_TILING_A(TT_DATA_A, TP_DIM_A, TP_DIM_AB, TP_SSR, TP_CASC_LEN, TP_DIM_A_LEADING)

def fn_update_TP_ADD_TILING_A(TT_DATA_A, TP_DIM_A, TP_DIM_AB, TP_SSR, TP_CASC_LEN, TP_DIM_A_LEADING):

  majorDim=TP_DIM_A//TP_SSR
  commonDim=TP_DIM_AB//TP_CASC_LEN
  matrixSize  = (fn_size_by_byte(TT_DATA_A) * majorDim * (commonDim))

  legal_set_TP_ADD_TILING_A = [0,1]
  if( (TP_DIM_A_LEADING==1) and (TT_DATA_A == "int16")) or (matrixSize < 512//8):
      legal_set_TP_ADD_TILING_A = [0]

  param_dict={
    "name" : "TP_ADD_TILING_A",
    "enum" : legal_set_TP_ADD_TILING_A
  }

  return param_dict

def validate_TP_ADD_TILING_A(args):
  TT_DATA_A=args["TT_DATA_A"]
  TP_DIM_A=args["TP_DIM_A"]
  TP_DIM_AB=args["TP_DIM_AB"]
  TP_SSR=args["TP_SSR"]
  TP_CASC_LEN=args["TP_CASC_LEN"]
  TP_DIM_A_LEADING=args["TP_DIM_A_LEADING"]
  TP_ADD_TILING_A=args["TP_ADD_TILING_A"]
  return fn_validate_TP_ADD_TILING_A(TT_DATA_A, TP_DIM_A, TP_DIM_AB, TP_SSR, TP_CASC_LEN,TP_DIM_A_LEADING, TP_ADD_TILING_A)

def fn_validate_TP_ADD_TILING_A(TT_DATA_A, TP_DIM_A, TP_DIM_AB, TP_SSR, TP_CASC_LEN,TP_DIM_A_LEADING, TP_ADD_TILING_A):
  param_dict=fn_update_TP_ADD_TILING_A(TT_DATA_A, TP_DIM_A, TP_DIM_AB, TP_SSR, TP_CASC_LEN, TP_DIM_A_LEADING)
  return validate_legal_set(param_dict["enum"], "TP_ADD_TILING_A", TP_ADD_TILING_A)

#######################################################
######## TP_ADD_TILING_B Updater and Validator ########
#######################################################
def update_TP_ADD_TILING_B(args):
  TT_DATA_B = args["TT_DATA_B"]
  TP_DIM_B = args["TP_DIM_B"]
  TP_DIM_AB = args["TP_DIM_AB"]
  TP_CASC_LEN = args["TP_CASC_LEN"]
  TP_DIM_B_LEADING = args["TP_DIM_B_LEADING"]
  return fn_update_TP_ADD_TILING_B(TT_DATA_B, TP_DIM_B, TP_DIM_AB, TP_CASC_LEN, TP_DIM_B_LEADING)

def fn_update_TP_ADD_TILING_B(TT_DATA_B, TP_DIM_B, TP_DIM_AB, TP_CASC_LEN, TP_DIM_B_LEADING):
  majorDim=TP_DIM_B
  commonDim=TP_DIM_AB//TP_CASC_LEN
  matrixSize  = (fn_size_by_byte(TT_DATA_B) * majorDim * (commonDim))

  legal_set_TP_ADD_TILING_B = [0,1]
  if( (TP_DIM_B_LEADING==1) and (TT_DATA_B == "int16")) or (matrixSize < 512//8):
      legal_set_TP_ADD_TILING_B = [0]

  param_dict={
    "name" : "TP_ADD_TILING_A",
    "enum" : legal_set_TP_ADD_TILING_B
  }

  return param_dict

def validate_TP_ADD_TILING_B(args):
  TT_DATA_B = args["TT_DATA_B"]
  TP_DIM_B = args["TP_DIM_B"]
  TP_DIM_AB = args["TP_DIM_AB"]
  TP_CASC_LEN = args["TP_CASC_LEN"]
  TP_DIM_B_LEADING = args["TP_DIM_B_LEADING"]
  TP_ADD_TILING_B = args["TP_ADD_TILING_B"]
  return fn_validate_TP_ADD_TILING_B(TT_DATA_B, TP_DIM_B, TP_DIM_AB, TP_CASC_LEN, TP_DIM_B_LEADING, TP_ADD_TILING_B)

def fn_validate_TP_ADD_TILING_B(TT_DATA_B, TP_DIM_B, TP_DIM_AB, TP_CASC_LEN, TP_DIM_B_LEADING, TP_ADD_TILING_B):
  param_dict=fn_update_TP_ADD_TILING_B(TT_DATA_B, TP_DIM_B, TP_DIM_AB, TP_CASC_LEN, TP_DIM_B_LEADING)
  return validate_legal_set(param_dict["enum"], "TP_ADD_TILING_B", TP_ADD_TILING_B)

#######################################################
####### TP_ADD_DETILING_OUT Updater and Validator #####
#######################################################
def update_TP_ADD_DETILING_OUT(args):
  TT_DATA_A = args["TT_DATA_A"]
  TT_DATA_B = args["TT_DATA_B"]
  TP_DIM_A = args["TP_DIM_A"]
  TP_DIM_B = args["TP_DIM_B"]
  TP_SSR = args["TP_SSR"]
  TP_DIM_OUT_LEADING = args["TP_DIM_OUT_LEADING"]
  return fn_update_TP_ADD_DETILING_OUT(TT_DATA_A, TT_DATA_B, TP_DIM_A, TP_DIM_B, TP_SSR, TP_DIM_OUT_LEADING)

def fn_update_TP_ADD_DETILING_OUT(TT_DATA_A, TT_DATA_B, TP_DIM_A, TP_DIM_B, TP_SSR, TP_DIM_OUT_LEADING):
  majorDim=TP_DIM_A//TP_SSR
  commonDim=TP_DIM_B
  TT_OUT=getOutputType(TT_DATA_A, TT_DATA_B)
  matrixSize  = (fn_size_by_byte(TT_OUT) * majorDim * (commonDim))

  legal_set_TP_ADD_DETILING_OUT = [0,1]
  if( (TP_DIM_OUT_LEADING==1) and (TT_OUT == "int16")) or (matrixSize < 512//8):
      legal_set_TP_ADD_DETILING_OUT = [0]

  param_dict={
    "name" : "TP_ADD_DETILING_OUT",
    "enum" : legal_set_TP_ADD_DETILING_OUT
  }

  return param_dict

def validate_TP_ADD_DETILING_OUT(args):
  TT_DATA_A = args["TT_DATA_A"]
  TT_DATA_B = args["TT_DATA_B"]
  TP_DIM_A = args["TP_DIM_A"]
  TP_DIM_B = args["TP_DIM_B"]
  TP_SSR = args["TP_SSR"]
  TP_DIM_OUT_LEADING = args["TP_DIM_OUT_LEADING"]
  TP_ADD_DETILING_OUT = args["TP_ADD_DETILING_OUT"]
  return fn_validate_TP_ADD_DETILING_OUT(TT_DATA_A, TT_DATA_B, TP_DIM_A, TP_DIM_B, TP_SSR, TP_DIM_OUT_LEADING, TP_ADD_DETILING_OUT)


def fn_validate_TP_ADD_DETILING_OUT(TT_DATA_A, TT_DATA_B, TP_DIM_A, TP_DIM_B, TP_SSR, TP_DIM_OUT_LEADING, TP_ADD_DETILING_OUT):
  param_dict= fn_update_TP_ADD_DETILING_OUT(TT_DATA_A, TT_DATA_B, TP_DIM_A, TP_DIM_B, TP_SSR, TP_DIM_OUT_LEADING)
  return validate_legal_set(param_dict["enum"], "TP_ADD_DETILING_OUT", TP_ADD_DETILING_OUT)

#######################################################
############## TP_SHIFT Updater and Validator #########
#######################################################
def update_TP_SHIFT(args):
  TT_DATA_A = args["TT_DATA_A"]
  return fn_update_TP_SHIFT(TT_DATA_A)

def fn_update_TP_SHIFT(TT_DATA_A):
  if TT_DATA_A in ["float", "cfloat"]:
    range_TP_SHIFT=[0,0]
  else:
    range_TP_SHIFT=[0,61]

  param_dict={
    "name" : "TP_SHIFT",
    "minimum" : range_TP_SHIFT[0],
    "maximum" : range_TP_SHIFT[1]
  }
  return param_dict


def validate_TP_SHIFT(args):
    TT_DATA_A = args["TT_DATA_A"]
    TP_SHIFT = args["TP_SHIFT"]
    return fn_validate_shift_val(TT_DATA_A, TP_SHIFT)

def fn_validate_shift_val(TT_DATA_A, TP_SHIFT):
  param_dict=fn_update_TP_SHIFT(TT_DATA_A)
  range_TP_SHIFT=[param_dict["minimum"], param_dict["maximum"]]
  return validate_range(range_TP_SHIFT, "TP_SHIFT", TP_SHIFT)

#######################################################
############## TP_RND Updater and Validator ###########
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


def getTilingScheme(typeA, typeB, aie_variant) :
  # needs to be compatible with c++14 -> so just use plain ifs
  # 16b x 16b
  if aie_variant == 1:
    if (typeA == "int16" and typeB == "int16") :
      return (4, 4, 4)
    # 32b x 16b
    if ((typeA == "cint16" or typeA == "int32") and typeB == "int16") :
      return (4, 4, 2)
    # 16b x 32b
    if (typeA == "int16" and (typeB == "cint16" or typeB == "int32")) :
      return (4, 2, 2)
    # 32b x 32b
    if (((typeA == "cint16" or typeA == "int32") and (typeB == "cint16" or typeB == "int32")) or
            typeA == "float" and typeB == "float"  ) :
      return (4, 4, 2)
    # 64b x 16b
    if (typeA == "cint32" and typeB == "int16") :
      return (2, 4, 2)
    # 16b x 64b
    if (typeA == "int16" and typeB == "cint32") :
      return (2, 4, 2) # 4, 4, 2 is also ok
    # 64b x 32b
    if (typeA == "cint32" and (typeB == "cint16" or typeB == "int32")) :
      return (2, 2, 2) # 2, 4, 2 is also ok
    # 32b x 64b
    if ((typeA == "cint16" or typeA == "int32") and typeB == "cint32") :
      return (2, 2, 2)
    # 64b x 64b
    if (typeA == "cint32" and typeB == "cint32") :
      return (2, 2, 2)
    # Mixed Floats
    if ((typeA == "cfloat" and typeB == "float") or
            (typeA == "float"  and typeB == "cfloat") ) :
      return (2, 4, 2) # 2, 2, 2 is also ok
    # cfloats
    if (typeA == "cfloat" and typeB == "cfloat" ) :
      return (4, 2, 2)
  if aie_variant == 2:
    if ((typeA == "int16" or typeA == "int32") and (typeB == "int16" or typeB == "int32")):
      return (4, 4, 4)
    if ((typeA == "cint16" and typeB == "int16")):
      return (4, 4, 4)
    if ((typeA == "cint16" and typeB == "cint16")):
      return (1, 4, 8)
    if ((typeA == "cint32" and typeB == "cint16")):
      return (2, 4, 8)
    if ((typeA == "cint32" and typeB == "cint32")):
      return (1, 2, 8)
  return (0, 0, 0)


def getOutputType(typeA, typeB) :
  if (fn_size_by_byte(typeA) > fn_size_by_byte(typeB)) :
    return typeA
  else :
    return typeB


def info_ports(args):

  portsA = com.get_port_info(
    portname = "inA",
    dir = "in",
    TT_DATA = args["TT_DATA_A"],
    windowVSize = (args["TP_INPUT_WINDOW_VSIZE_A"] // (args["TP_SSR"] * args["TP_CASC_LEN"])),
    vectorLength = (args["TP_CASC_LEN"] * args["TP_SSR"])
  )

  portsB = com.get_port_info(
    portname = "inB",
    dir = "in",
    TT_DATA = args["TT_DATA_B"],
    windowVSize = (args["TP_INPUT_WINDOW_VSIZE_B"] // args["TP_CASC_LEN"]),
    vectorLength = args["TP_CASC_LEN"]
  )
  TT_DATA_OUT = getOutputType(args["TT_DATA_A"], args["TT_DATA_B"])
  TP_OUTPUT_WINDOW_VSIZE = ((args["TP_DIM_A"] // args["TP_SSR"]) * args["TP_DIM_B"])
  portsOut = com.get_port_info(
    portname = "out",
    dir = "out",
    TT_DATA = TT_DATA_OUT,
    windowVSize = (TP_OUTPUT_WINDOW_VSIZE),
    vectorLength = args["TP_SSR"]
  )
  # join lists of ports together and return
  return portsA + portsB + portsOut


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

  # Use formatted multi-line string to avoid a lot of \n and \t
  code  = (
f"""
class {graphname} : public adf::graph {{
public:
  constexpr unsigned TP_CASC_LEN = {TP_CASC_LEN};
  std::array<adf::port<input>,TP_CASC_LEN> inA;
  std::array<adf::port<input>,TP_CASC_LEN> inB;
  adf::port<output> out;
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
    {TP_INPUT_WINDOW_VSIZE_B} // TP_INPUT_WINDOW_VSIZE_B
    {TP_CASC_LEN} // TP_CASC_LEN
    {TP_SAT} // TP_SAT
    {TP_SSR} // TP_SSR
  > mmult;

  {graphname}() : mmult() {{
    adf::kernel *mmult_kernels = mmult.getKernels();
    for (int i=0; i < 1; i++) {{
      adf::runtime<ratio>(mmult_kernels[i]) = 0.9;
    }}
    for (int i=0; i < TP_CASC_LEN; i++) {{
      adf::connect<> net_inA(inA[i], mmult.inA[i]);
      adf::connect<> net_inB(inB[i], mmult.inB[i]);
    }}
    adf::connect<> net_out(mmult.out[0], out);
  }}

}};
""")
  out = {}
  out["graph"] = code
  out["port_info"] = info_ports(args)
  out["headerfile"] = "matrix_mult_graph.hpp"
  out["searchpaths"] = ["L1/include/aie", "L2/include/aie", "L1/src/aie"]

  return out
