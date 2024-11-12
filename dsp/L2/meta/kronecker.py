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
from ctypes import sizeof
from socket import TIPC_SUB_SERVICE
from aie_common import *
import json

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

TP_WINDOW_VSIZE_max = 16384 # ping pong buffers within one bank
TP_DIM_COLS_Min = 1 
TP_DIM_ROWS_Min = 1
TP_NUM_FRAMES_Min = 1
TP_SHIFT_min = 0
TP_SHIFT_max = 60
TP_SSR_min = 1
TP_SSR_max = 16

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
########### TT_DATA_A Updater and Validator ###########
#######################################################
def update_TT_DATA_A(args):
  return fn_update_TT_DATA_A()

def fn_update_TT_DATA_A():
  legal_set_TT_DATA_A=["int16", "cint16", "int32", "cint32", "float", "cfloat"]
  param_dict={
    "name" : "TT_DATA_A",
    "enum" : legal_set_TT_DATA_A
  }
  return param_dict

def validate_TT_DATA_A(args):
  TT_DATA_A = args["TT_DATA_A"]
  return fn_validate__TT_DATA_A(TT_DATA_A)


# validation function    
def fn_validate__TT_DATA_A(TT_DATA_A):
  param_dict = fn_update_TT_DATA_A()
  return validate_legal_set(param_dict["enum"], "TT_DATA_A", TT_DATA_A)

#######################################################
########### TT_DATA_B Updater and Validator ###########
#######################################################
def update_TT_DATA_B(args):
  TT_DATA_A=args["TT_DATA_A"]
  return fn_update_TT_DATA_B(TT_DATA_A)

def fn_update_TT_DATA_B(TT_DATA_A):
  legal_set_TT_DATA_B=["int16", "cint16", "int32", "cint32", "float", "cfloat"]
  float_set=["float", "cfloat"]
  integer_set=["int16", "cint16", "int32", "cint32"]
  if TT_DATA_A in float_set:
    legal_set_TT_DATA_B=remove_from_set(integer_set, legal_set_TT_DATA_B)
  elif TT_DATA_A in integer_set:
    legal_set_TT_DATA_B=remove_from_set(float_set, legal_set_TT_DATA_B)

  param_dict={
    "name" : TT_DATA_A,
    "enum" : legal_set_TT_DATA_B
  }

  return param_dict

def validate_TT_DATA_B(args):
    TT_DATA_A = args["TT_DATA_A"]
    TT_DATA_B = args["TT_DATA_B"]
    return fn_validate__TT_DATA_B(TT_DATA_A, TT_DATA_B)

# validation function    
def fn_validate__TT_DATA_B(TT_DATA_A, TT_DATA_B):
  param_dict = fn_update_TT_DATA_B(TT_DATA_A)
  return validate_legal_set(param_dict["enum"], "TT_DATA_B", TT_DATA_B)

#######################################################
############# TP_API Updater and Validator ############
#######################################################
def update_TP_API(args):
  return fn_update_TP_API()

def fn_update_TP_API():
  param_dict={
    "name" : "TP_API",
    "enum" : [0,1]
  }
  return param_dict

def validate_TP_API(args):
    TP_API = args["TP_API"]
    return fn_validate_api_val(TP_API)

def fn_validate_api_val(TP_API):
  param_dict=fn_update_TP_API()
  return validate_legal_set(param_dict["enum"], "TP_API", TP_API)

#######################################################
########### TP_SSR Updater and Validator ##############
#######################################################
def update_TP_SSR(args):
  return fn_update_ssr()

def fn_update_ssr():
  param_dict={
    "name" : "TP_SSR",
    "minimum" : TP_SSR_min,
    "maximum" : TP_SSR_max
  }
  return param_dict

def validate_TP_SSR(args):
  TP_SSR = args["TP_SSR"]
  return fn_validate_ssr(TP_SSR)

def fn_validate_ssr(TP_SSR):
  range_TP_SSR=[TP_SSR_min, TP_SSR_max] 
  return validate_range(range_TP_SSR, "TP_SSR", TP_SSR)

#######################################################
####### TP_DIM_A_ROWS Updater and Validator ###########
#######################################################
def update_TP_DIM_A_ROWS(args):
  TT_DATA_A = args["TT_DATA_A"]
  TT_DATA_B = args["TT_DATA_B"]
  TP_API = args["TP_API"]
  if args["TP_DIM_A_ROWS"]: TP_DIM_A_ROWS=args["TP_DIM_A_ROWS"]
  else: TP_DIM_A_ROWS=0
  
  return fn_update_TP_DIM_A_ROWS(TT_DATA_A, TT_DATA_B, TP_API, TP_DIM_A_ROWS)

def fn_update_TP_DIM_A_ROWS(TT_DATA_A, TT_DATA_B, TP_API, TP_DIM_A_ROWS):

  VEC_SIZE = int(256/8/fn_size_by_byte(TT_DATA_A))

  param_dict={
    "name" : "TT_DATA_A",
    "minimum" : VEC_SIZE,
  }

  TP_DIM_A_COLS_SSR = 1
  if TP_API==0:
    TP_DIM_B_ROWS = VEC_SIZE
    TP_DIM_B_COLS = 1
    TP_WINDOW_VSIZE_max_size = TP_WINDOW_VSIZE_max / fn_size_by_byte(fn_det_out_type(TT_DATA_A, TT_DATA_B))
    TP_DIM_A_ROWS_max = TP_WINDOW_VSIZE_max_size/ (TP_DIM_A_COLS_SSR * TP_DIM_B_ROWS * TP_DIM_B_COLS)

  elif TP_API==1:
    
    TP_WINDOW_VSIZE_max_size = TP_WINDOW_VSIZE_max / fn_size_by_byte(TT_DATA_A)
    TP_DIM_A_ROWS_max = TP_WINDOW_VSIZE_max_size / TP_DIM_A_COLS_SSR

  param_dict.update({"maximum" : int(FLOOR(TP_DIM_A_ROWS_max, VEC_SIZE))})


  if TP_DIM_A_ROWS != 0 and (TP_DIM_A_ROWS%VEC_SIZE != 0): 
    TP_DIM_A_ROWS_act=round(TP_DIM_A_ROWS/VEC_SIZE) * VEC_SIZE

    if TP_DIM_A_ROWS_act < param_dict["minimum"]:
      TP_DIM_A_ROWS_act = param_dict["minimum"]

    if TP_DIM_A_ROWS_act > param_dict["maximum"]:
      TP_DIM_A_ROWS_act = param_dict["maximum"]
    
    param_dict.update({"actual" : int(TP_DIM_A_ROWS_act)})
  return param_dict

def validate_TP_DIM_A_ROWS(args):
  TT_DATA_A = args["TT_DATA_A"]
  TT_DATA_B = args["TT_DATA_B"]
  TP_API = args["TP_API"]

  TP_DIM_A_ROWS = args["TP_DIM_A_ROWS"]
  return fn_validate_dim_a_rows(TT_DATA_A, TT_DATA_B, TP_API, TP_DIM_A_ROWS)

def fn_validate_dim_a_rows(TT_DATA_A, TT_DATA_B, TP_API, TP_DIM_A_ROWS):
  param_dict= fn_update_TP_DIM_A_ROWS(TT_DATA_A, TT_DATA_B, TP_API, TP_DIM_A_ROWS)
  VEC_SIZE = int(256/8/fn_size_by_byte(TT_DATA_A))

  if (TP_DIM_A_ROWS % VEC_SIZE):
    return isError(f"TP_DIM_A_ROWS should be an integer multiple of vector size. Vector size for the data type {TT_DATA_A} is {VEC_SIZE}. Got TP_DIM_A_ROWS = {TP_DIM_A_ROWS}")
  else:
    range_TP_DIM_A_ROWS=[param_dict["minimum"], param_dict["maximum"]]
    return validate_range(range_TP_DIM_A_ROWS, "TP_DIM_A_ROWS", TP_DIM_A_ROWS)

#######################################################
####### TP_DIM_A_COLS Updater and Validator ###########
#######################################################
def update_TP_DIM_A_COLS(args):
  TT_DATA_A = args["TT_DATA_A"]
  TT_DATA_B = args["TT_DATA_B"]
  TP_API = args["TP_API"]
  TP_SSR = args["TP_SSR"]
  TP_DIM_A_ROWS = args["TP_DIM_A_ROWS"]

  if args["TP_DIM_A_COLS"] : TP_DIM_A_COLS=args["TP_DIM_A_COLS"]
  else: TP_DIM_A_COLS=0

  return fn_update_TP_DIM_A_COLS(TT_DATA_A, TT_DATA_B, TP_API, TP_SSR, TP_DIM_A_ROWS, TP_DIM_A_COLS)

def fn_update_TP_DIM_A_COLS(TT_DATA_A,  TT_DATA_B, TP_API, TP_SSR, TP_DIM_A_ROWS, TP_DIM_A_COLS):
  VEC_SIZE = int(256/8/fn_size_by_byte(TT_DATA_A))

  param_dict={
  "name":"TP_DIM_A_COLS",
  "minimum" : TP_SSR
  }

  if TP_API==0:
     #minimum possible values
    TP_DIM_B_ROWS = VEC_SIZE
    TP_DIM_B_COLS = 1
    TP_WINDOW_VSIZE_max_size = TP_WINDOW_VSIZE_max / fn_size_by_byte(fn_det_out_type(TT_DATA_A, TT_DATA_B))
    TP_DIM_A_COLS_max = (TP_WINDOW_VSIZE_max_size * TP_SSR) / (TP_DIM_A_ROWS  * TP_DIM_B_ROWS * TP_DIM_B_COLS)

  elif TP_API==1:
    TP_WINDOW_VSIZE_max_size = TP_WINDOW_VSIZE_max / fn_size_by_byte(TT_DATA_A)
    TP_DIM_A_COLS_max = (TP_WINDOW_VSIZE_max_size * TP_SSR) / (TP_DIM_A_ROWS)

  param_dict.update({"maximum" :  int(FLOOR(TP_DIM_A_COLS_max, TP_SSR))})

  if TP_DIM_A_COLS != 0 and (TP_DIM_A_COLS % TP_SSR != 0):
    TP_DIM_A_COLS_act= round(TP_DIM_A_COLS / TP_SSR) * TP_SSR
    if TP_DIM_A_COLS_act < TP_SSR:
      TP_DIM_A_COLS_act = TP_SSR
    if (TP_DIM_A_COLS_act > param_dict["maximum"]):
      TP_DIM_A_COLS_act = param_dict["maximum"]
    param_dict.update({"actual" : int(TP_DIM_A_COLS_act)})

  return param_dict

def validate_TP_DIM_A_COLS(args):
  TT_DATA_A = args["TT_DATA_A"]
  TT_DATA_B = args["TT_DATA_B"]
  TP_API = args["TP_API"]
  TP_SSR = args["TP_SSR"]
  TP_DIM_A_ROWS = args["TP_DIM_A_ROWS"]
  TP_DIM_A_COLS=args["TP_DIM_A_COLS"]
  return fn_validate_dim_cols_a(TT_DATA_A, TT_DATA_B, TP_API, TP_SSR, TP_DIM_A_ROWS, TP_DIM_A_COLS)


def fn_validate_dim_cols_a(TT_DATA_A, TT_DATA_B, TP_API, TP_SSR, TP_DIM_A_ROWS, TP_DIM_A_COLS):
  param_dict=fn_update_TP_DIM_A_COLS(TT_DATA_A,  TT_DATA_B, TP_API, TP_SSR, TP_DIM_A_ROWS, TP_DIM_A_COLS)

  if (TP_DIM_A_COLS%TP_SSR != 0):
    return isError(f"TP_DIM_A_COLS should be an integer multiple of ssr. Got TP_DIM__ROWS = {TP_DIM_A_COLS}, TP_SSR = {TP_SSR}")
  else:
    range_TP_DIM_A_COLS=[param_dict["minimum"], param_dict["maximum"]]
    return validate_range(range_TP_DIM_A_COLS, "TP_DIM_A_COLS", TP_DIM_A_COLS)

  
#######################################################
####### TP_DIM_B_ROWS Updater and Validator ###########
#######################################################
def update_TP_DIM_B_ROWS(args):
  TT_DATA_A = args["TT_DATA_A"]
  TT_DATA_B = args["TT_DATA_B"]
  TP_API = args["TP_API"]
  TP_SSR = args["TP_SSR"]
  TP_DIM_A_ROWS = args["TP_DIM_A_ROWS"]
  TP_DIM_A_COLS=args["TP_DIM_A_COLS"]

  if args["TP_DIM_B_ROWS"] : TP_DIM_B_ROWS=args["TP_DIM_B_ROWS"]
  else: TP_DIM_B_ROWS=0

  return fn_update_TP_DIM_B_ROWS(TT_DATA_A, TT_DATA_B, TP_API, TP_SSR, TP_DIM_A_ROWS, TP_DIM_A_COLS, TP_DIM_B_ROWS)

def fn_update_TP_DIM_B_ROWS(TT_DATA_A, TT_DATA_B, TP_API, TP_SSR, TP_DIM_A_ROWS, TP_DIM_A_COLS, TP_DIM_B_ROWS):
  VEC_SIZE = int(256/8/fn_size_by_byte(TT_DATA_B))

  param_dict={
  "name":"TP_DIM_B_ROWS",
  "minimum" : VEC_SIZE
  }

  TP_DIM_B_COLS = 1
  if TP_API==0:
     #minimum possible values
    TP_WINDOW_VSIZE_max_size = TP_WINDOW_VSIZE_max / fn_size_by_byte(fn_det_out_type(TT_DATA_A, TT_DATA_B))
    TP_DIM_A_COLS_SSR = TP_DIM_A_COLS / TP_SSR
    TP_DIM_B_ROWS_max= TP_WINDOW_VSIZE_max_size / (TP_DIM_A_ROWS * TP_DIM_A_COLS_SSR * TP_DIM_B_COLS)
  elif TP_API==1:
    TP_WINDOW_VSIZE_max_size = TP_WINDOW_VSIZE_max / fn_size_by_byte(TT_DATA_B)
    TP_DIM_B_ROWS_max= TP_WINDOW_VSIZE_max_size / (TP_DIM_B_COLS)

  param_dict.update({"maximum" : int(FLOOR(TP_DIM_B_ROWS_max, VEC_SIZE))})

  if TP_DIM_B_ROWS != 0 and (TP_DIM_B_ROWS % VEC_SIZE != 0):
    TP_DIM_B_ROWS_act= round(TP_DIM_B_ROWS / VEC_SIZE) * VEC_SIZE
    if TP_DIM_B_ROWS_act < VEC_SIZE:
      TP_DIM_B_ROWS_act = VEC_SIZE
    if (TP_DIM_B_ROWS_act > param_dict["maximum"]):
      TP_DIM_B_ROWS_act = param_dict["maximum"]
    param_dict.update({"actual" : int(TP_DIM_B_ROWS_act)})

  return param_dict


def validate_TP_DIM_B_ROWS(args):
  TT_DATA_A = args["TT_DATA_A"]
  TT_DATA_B = args["TT_DATA_B"]
  TP_API = args["TP_API"]
  TP_SSR = args["TP_SSR"]
  TP_DIM_A_ROWS = args["TP_DIM_A_ROWS"]
  TP_DIM_A_COLS=args["TP_DIM_A_COLS"]
  TP_DIM_B_ROWS=args["TP_DIM_B_ROWS"]
  return fn_validate_dim_b_rows(TT_DATA_A, TT_DATA_B, TP_API, TP_SSR, TP_DIM_A_ROWS, TP_DIM_A_COLS, TP_DIM_B_ROWS)


def fn_validate_dim_b_rows(TT_DATA_A, TT_DATA_B, TP_API, TP_SSR, TP_DIM_A_ROWS, TP_DIM_A_COLS, TP_DIM_B_ROWS):
  param_dict = fn_update_TP_DIM_B_ROWS(TT_DATA_A, TT_DATA_B, TP_API, TP_SSR, TP_DIM_A_ROWS, TP_DIM_A_COLS, TP_DIM_B_ROWS)

  VEC_SIZE = int(256/8/fn_size_by_byte(TT_DATA_B))

  if (TP_DIM_B_ROWS % VEC_SIZE):
    return isError(f"TP_DIM_B_ROWS should be an integer multiple of vector size. Vector size for the data type {TT_DATA_B} is {VEC_SIZE}. Got TP_DIM__ROWS = {TP_DIM_B_ROWS}")
  else:
    range_TP_DIM_B_ROWS=[param_dict["minimum"], param_dict["maximum"]]
    return validate_range(range_TP_DIM_B_ROWS, "TP_DIM_B_ROWS", TP_DIM_B_ROWS)
    
#######################################################
####### TP_DIM_B_COLS Updater and Validator ###########
#######################################################

def update_TP_DIM_B_COLS(args):
  TT_DATA_A = args["TT_DATA_A"]
  TT_DATA_B = args["TT_DATA_B"]
  TP_API = args["TP_API"]
  TP_SSR = args["TP_SSR"]
  TP_DIM_A_ROWS = args["TP_DIM_A_ROWS"]
  TP_DIM_A_COLS=args["TP_DIM_A_COLS"]
  TP_DIM_B_ROWS=args["TP_DIM_B_ROWS"]

  return fn_update_TP_DIM_B_COLS(TT_DATA_A, TT_DATA_B, TP_API, TP_SSR, TP_DIM_A_ROWS, TP_DIM_A_COLS, TP_DIM_B_ROWS)

def fn_update_TP_DIM_B_COLS(TT_DATA_A, TT_DATA_B, TP_API, TP_SSR, TP_DIM_A_ROWS, TP_DIM_A_COLS, TP_DIM_B_ROWS):
  param_dict={
    "name" : "TP_DIM_B_COLS",
    "minimum" : 1
  }

  if TP_API==0:
    TP_WINDOW_VSIZE_max_size = TP_WINDOW_VSIZE_max / fn_size_by_byte(fn_det_out_type(TT_DATA_A, TT_DATA_B))
    TP_DIM_A_COLS_SSR = TP_DIM_A_COLS / TP_SSR
    TP_DIM_B_COLS_max=TP_WINDOW_VSIZE_max_size/(TP_DIM_A_ROWS * TP_DIM_A_COLS_SSR * TP_DIM_B_ROWS)
  elif TP_API==1:
    TP_WINDOW_VSIZE_max_size = TP_WINDOW_VSIZE_max / fn_size_by_byte(TT_DATA_B)
    TP_DIM_B_COLS_max=TP_WINDOW_VSIZE_max_size/TP_DIM_B_ROWS

  param_dict.update({"maximum" : int(TP_DIM_B_COLS_max)})
  return param_dict

def validate_TP_DIM_B_COLS(args):
  TT_DATA_A = args["TT_DATA_A"]
  TT_DATA_B = args["TT_DATA_B"]
  TP_API = args["TP_API"]
  TP_SSR = args["TP_SSR"]
  TP_DIM_A_ROWS = args["TP_DIM_A_ROWS"]
  TP_DIM_A_COLS=args["TP_DIM_A_COLS"]
  TP_DIM_B_ROWS=args["TP_DIM_B_ROWS"]
  TP_DIM_B_COLS=args["TP_DIM_B_COLS"]
  return fn_validate_dim_cols_b(TT_DATA_A, TT_DATA_B, TP_API, TP_SSR, TP_DIM_A_ROWS, TP_DIM_A_COLS, TP_DIM_B_ROWS, TP_DIM_B_COLS)

def fn_validate_dim_cols_b(TT_DATA_A, TT_DATA_B, TP_API, TP_SSR, TP_DIM_A_ROWS, TP_DIM_A_COLS, TP_DIM_B_ROWS, TP_DIM_B_COLS):
  param_dict = fn_update_TP_DIM_B_COLS(TT_DATA_A, TT_DATA_B, TP_API, TP_SSR, TP_DIM_A_ROWS, TP_DIM_A_COLS, TP_DIM_B_ROWS)
  range_TP_DIM_B_ROWS=[param_dict["minimum"], param_dict["maximum"]]
  return validate_range(range_TP_DIM_B_ROWS, "TP_DIM_B_COLS", TP_DIM_B_COLS)

#######################################################
####### TP_NUM_FRAMES Updater and Validator ###########
#######################################################  
def update_TP_NUM_FRAMES(args):
  TT_DATA_A = args["TT_DATA_A"]
  TT_DATA_B = args["TT_DATA_B"]
  TP_API = args["TP_API"]
  TP_SSR = args["TP_SSR"]
  TP_DIM_A_ROWS = args["TP_DIM_A_ROWS"]
  TP_DIM_A_COLS = args["TP_DIM_A_COLS"]
  TP_DIM_B_ROWS = args["TP_DIM_B_ROWS"]
  TP_DIM_B_COLS = args["TP_DIM_B_COLS"]

  return fn_update_TP_NUM_FRAMES(TT_DATA_A, TT_DATA_B, TP_API, TP_SSR, TP_DIM_A_ROWS, TP_DIM_A_COLS, TP_DIM_B_ROWS, TP_DIM_B_COLS)

def fn_update_TP_NUM_FRAMES(TT_DATA_A, TT_DATA_B, TP_API, TP_SSR, TP_DIM_A_ROWS, TP_DIM_A_COLS, TP_DIM_B_ROWS, TP_DIM_B_COLS):
  param_dict={
    "name" : "TP_NUM_FRAMES",
    "minimum" : TP_NUM_FRAMES_Min
  }
  if TP_API==0:
    TP_DIM_A_COLS_SSR = TP_DIM_A_COLS / TP_SSR
    OUT_FRAME_SIZE = TP_DIM_A_ROWS * TP_DIM_A_COLS_SSR * TP_DIM_B_ROWS * TP_DIM_B_COLS
    OUT_FRAME_SIZE_BYTES = OUT_FRAME_SIZE * fn_size_by_byte(fn_det_out_type(TT_DATA_A, TT_DATA_B))
    TP_NUM_FRAMES_max = TP_WINDOW_VSIZE_max / OUT_FRAME_SIZE_BYTES
  elif TP_API==1:
    TP_WINDOW_VSIZE_max_size_a = TP_WINDOW_VSIZE_max / fn_size_by_byte(TT_DATA_A)
    TP_NUM_FRAMES_max1 = TP_WINDOW_VSIZE_max_size_a*TP_SSR/(TP_DIM_A_COLS*TP_DIM_A_ROWS)
    TP_WINDOW_VSIZE_max_size_b = TP_WINDOW_VSIZE_max / fn_size_by_byte(TT_DATA_B)
    TP_NUM_FRAMES_max2 = TP_WINDOW_VSIZE_max_size_b/(TP_DIM_B_COLS*TP_DIM_B_ROWS)
    TP_NUM_FRAMES_max = min(TP_NUM_FRAMES_max1, TP_NUM_FRAMES_max2)

  param_dict.update({"maximum" : int(TP_NUM_FRAMES_max)})

  return param_dict

def validate_TP_NUM_FRAMES(args):
  TT_DATA_A = args["TT_DATA_A"]
  TT_DATA_B = args["TT_DATA_B"]
  TP_API = args["TP_API"]
  TP_SSR = args["TP_SSR"]
  TP_DIM_A_ROWS = args["TP_DIM_A_ROWS"]
  TP_DIM_A_COLS = args["TP_DIM_A_COLS"]
  TP_DIM_B_ROWS = args["TP_DIM_B_ROWS"]
  TP_DIM_B_COLS = args["TP_DIM_B_COLS"]
  TP_NUM_FRAMES = args["TP_NUM_FRAMES"]
  return fn_validate_num_frames(TT_DATA_A, TT_DATA_B, TP_API, TP_SSR, TP_DIM_A_ROWS, TP_DIM_A_COLS, TP_DIM_B_ROWS, TP_DIM_B_COLS, TP_NUM_FRAMES)

def fn_validate_num_frames(TT_DATA_A, TT_DATA_B, TP_API, TP_SSR, TP_DIM_A_ROWS, TP_DIM_A_COLS, TP_DIM_B_ROWS, TP_DIM_B_COLS, TP_NUM_FRAMES):
  param_dict=fn_update_TP_NUM_FRAMES(TT_DATA_A, TT_DATA_B, TP_API, TP_SSR, TP_DIM_A_ROWS, TP_DIM_A_COLS, TP_DIM_B_ROWS, TP_DIM_B_COLS)
  return validate_range([param_dict["minimum"], param_dict["maximum"]], "TP_NUM_FRAMES", TP_NUM_FRAMES)

#######################################################
########### TP_SHIFT Updater and Validator ############
#######################################################  

def update_TP_SHIFT(args):
  TT_DATA_A = args["TT_DATA_A"]
  TT_DATA_B = args["TT_DATA_B"]
  return fn_update_TP_SHIFT(TT_DATA_A, TT_DATA_B)

def fn_update_TP_SHIFT(TT_DATA_A, TT_DATA_B):
  out_t=fn_det_out_type(TT_DATA_A, TT_DATA_B)
  range_TP_SHIFT=fn_range_shift(out_t)

  param_dict={
    "name" : "TP_SHIFT",
    "minimum" : range_TP_SHIFT[0],
    "maximum" : range_TP_SHIFT[1]
  }
  return param_dict


def validate_TP_SHIFT(args):
    TT_DATA_A = args["TT_DATA_A"]
    TT_DATA_B = args["TT_DATA_B"]
    TP_SHIFT = args["TP_SHIFT"]
    return fn_validate_shift_val(TT_DATA_A, TT_DATA_B, TP_SHIFT)

def fn_validate_shift_val(TT_DATA_A, TT_DATA_B, TP_SHIFT):
  param_dict=fn_update_TP_SHIFT(TT_DATA_A, TT_DATA_B)
  range_TP_SHIFT=[param_dict["minimum"], param_dict["maximum"]]

  return validate_range(range_TP_SHIFT, "TP_SHIFT", TP_SHIFT)
    

#######################################################
############ TP_RND Updater and Validator #############
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



# helper functions

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
  TP_DIM_A_ROWS = args["TP_DIM_A_ROWS"]
  TP_DIM_A_COLS = args["TP_DIM_A_COLS"]
  TP_DIM_B_ROWS = args["TP_DIM_B_ROWS"]
  TP_DIM_B_COLS = args["TP_DIM_B_COLS"]
  TP_NUM_FRAMES = args["TP_NUM_FRAMES"]
  TP_API = args["TP_API"]
  TP_SSR = args["TP_SSR"]
  if (TP_API==0):
    portsInA = get_port_info(
      portname = "inA",
      dir = "in",
      dataType = TT_DATA_A,
      dim = TP_DIM_A_ROWS * TP_DIM_A_COLS,
      numFrames = TP_NUM_FRAMES,
      apiType = "window",
      vectorLength = TP_SSR
    )
    portsInB = get_port_info(
      portname = "inB",
      dir = "in",
      dataType = TT_DATA_B,
      dim = TP_DIM_B_ROWS * TP_DIM_B_COLS,
      numFrames = TP_NUM_FRAMES,
      apiType = "window",
      vectorLength = TP_SSR
    )
    portsOut = get_port_info(
      portname = "out",
      dir = "out",
      dataType = fn_det_out_type(TT_DATA_A, TT_DATA_B),
      dim = TP_DIM_A_ROWS * TP_DIM_B_ROWS * TP_DIM_A_COLS * TP_DIM_B_COLS,
      numFrames = TP_NUM_FRAMES,
      apiType = "window",
      vectorLength = TP_SSR
    )
  else:
    portsInA = get_port_info(
      portname = "inA",
      dir = "in",
      dataType = TT_DATA_A,
      dim = TP_DIM_A_ROWS * TP_DIM_A_COLS,
      numFrames = TP_NUM_FRAMES,
      apiType = "stream",
      vectorLength = TP_SSR
    )
    portsInB = get_port_info(
      portname = "inB",
      dir = "in",
      dataType = TT_DATA_B,
      dim = TP_DIM_B_ROWS * TP_DIM_B_COLS,
      numFrames = TP_NUM_FRAMES,
      apiType = "stream",
      vectorLength = TP_SSR
    )
    portsOut = get_port_info(
      portname = "out",
      dir = "out",
      dataType = fn_det_out_type(TT_DATA_A, TT_DATA_B),
      dim = TP_DIM_A_ROWS * TP_DIM_B_ROWS * TP_DIM_A_COLS * TP_DIM_B_COLS,
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
  TP_DIM_A_ROWS = args["TP_DIM_A_ROWS"]
  TP_DIM_A_COLS = args["TP_DIM_A_COLS"]
  TP_DIM_B_ROWS = args["TP_DIM_B_ROWS"]
  TP_DIM_B_ROWS = args["TP_DIM_B_COLS"]
  TP_NUM_FRAMES = args["TP_NUM_FRAMES"]
  TP_SHIFT = args["TP_SHIFT"]
  TP_API = args["TP_API"]
  TP_SSR = args["TP_SSR"]

  # if TP_API == 1:
  #   ssr = TP_SSR//2
  # else:
  #   ssr = TP_SSR

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

  xf::dsp::aie::kronecker::kronecker_graph<
    {TT_DATA_A}, //TT_DATA_A
    {TT_DATA_B}, //TT_DATA_B
    {TP_DIM_A_ROWS}, //TP_DIM_A_ROWS
    {TP_DIM_A_COLS}, //TP_DIM_A_COLS
    {TP_DIM_B_ROWS}, //TP_DIM_B_ROWS
    {TP_DIM_A_COLS}, //TP_DIM_B_COLS
    {TP_NUM_FRAMES}, //TP_NUM_FRAMES
    {TP_API}, //TP_API
    {TP_SHIFT}, //TP_SHIFT
    {TP_SSR}, //TP_SSR
  > kronecker;

  {graphname}() : kronecker() {{
    adf::kernel *kronecker_kernels = kronecker.getKernels();
    for (int i=0; i < 1; i++) {{
      adf::runtime<ratio>(kronecker_kernels[i]) = 0.9;
    }}
    for (int i=0; i < TP_SSR; i++) {{
      adf::connect<> net_in(inA[i], kronecker.inA[i]);
      adf::connect<> net_in(inB[i], kronecker.inB[i]);
      adf::connect<> net_out(kronecker.out[i], out[i]);
    }}
  }}

}};
""")
  out = {}
  out["graph"] = code
  out["port_info"] = info_ports(args)
  out["headerfile"] = "kronecker_graph.hpp"
  out["searchpaths"] = [
       "L2/include/aie",
       "L2/tests/aie/common/inc",
       "L1/include/aie",
       "L1/src/aie",
       "L1/tests/aie/inc",
       "L1/tests/aie/src"]

  return out
