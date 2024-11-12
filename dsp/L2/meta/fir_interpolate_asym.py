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

import fir_sr_asym as sr_asym
import fir_decimate_asym as deci_asym
import fir_polyphase_decomposer as poly

import importlib
from pathlib import Path
current_uut_kernel = Path(__file__).stem

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

TP_INTERPOLATE_FACTOR_min = 1
TP_INTERPOLATE_FACTOR_max_aie1 = 16
TP_INTERPOLATE_FACTOR_max_aie2 = 8

TP_INPUT_WINDOW_VSIZE_min = 4
TP_PARA_INTERP_POLY_min = 1
TP_CASC_LEN_min = 1
TP_CASC_LEN_max = 40
TP_SSR_min = 1
TP_SSR_max = 16
TP_FIR_LEN_min = 4
TP_FIR_LEN_max = 8192
offset_32bits=8

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
########### TT_COEFF Updater and Validator ############
#######################################################
def update_TT_COEFF(args):
    AIE_VARIANT = args["AIE_VARIANT"]
    TT_DATA = args["TT_DATA"]
    return fn_update_TT_COEFF(AIE_VARIANT, TT_DATA)

def fn_update_TT_COEFF(AIE_VARIANT, TT_DATA):
    param_dict=fn_update_tt_coeff(AIE_VARIANT, TT_DATA)
    legal_set_coeff=param_dict["enum"]
   
    if AIE_VARIANT==1:
      remove_set=[]
      for coeff in legal_set_coeff:
        numLanes=fnNumLanes(TT_DATA, coeff, 0, AIE_VARIANT)
        if (numLanes > offset_32bits):
          remove_set.append(coeff)
      legal_set_coeff=remove_from_set(remove_set, legal_set_coeff.copy())
    param_dict.update({"enum": legal_set_coeff})
    return param_dict

def validate_TT_COEFF(args):
    AIE_VARIANT = args["AIE_VARIANT"]
    TT_DATA = args["TT_DATA"]
    TT_COEFF=args["TT_COEFF"]
    return (fn_validate_TT_COEFF(AIE_VARIANT, TT_DATA, TT_COEFF))

def fn_validate_TT_COEFF(AIE_VARIANT, TT_DATA, TT_COEFF):
    param_dict = fn_update_TT_COEFF(AIE_VARIANT, TT_DATA)
    legal_set_TT_COEFF = param_dict["enum"]
    return(validate_legal_set(legal_set_TT_COEFF, "TT_COEFF", TT_COEFF))

#######################################################
############# TP_API Updater and Validator ############
#######################################################
def update_TP_API(args):
    return fn_update_binary("TP_API")

def validate_TP_API(args):
    TP_API=args["TP_API"]
    return fn_validate_TP_API(TP_API)

def fn_validate_TP_API(TP_API):
    return(validate_legal_set([0,1], "TP_API", TP_API))

#######################################################
###### TP_USE_COEFF_RELOAD Updater and Validator ######
#######################################################
def update_TP_USE_COEFF_RELOAD(args):
    return fn_update_binary("TP_USE_COEFF_RELOAD")

def validate_TP_USE_COEFF_RELOAD(args):
    TP_USE_COEFF_RELOAD=args["TP_USE_COEFF_RELOAD"]
    return fn_validate_TP_USE_COEFF_RELOAD(TP_USE_COEFF_RELOAD)

def fn_validate_TP_USE_COEFF_RELOAD(TP_USE_COEFF_RELOAD):
    return(validate_legal_set([0,1], "TP_USE_COEFF_RELOAD", TP_USE_COEFF_RELOAD))


#######################################################
############ TP_FIR_LEN Updater and Validator #########
#######################################################

def update_TP_FIR_LEN(args):
  TT_DATA = args["TT_DATA"]
  TT_COEFF = args["TT_COEFF"]
  TP_API = args["TP_API"]
  TP_USE_COEFF_RELOAD = args["TP_USE_COEFF_RELOAD"]
  AIE_VARIANT = args["AIE_VARIANT"]
  if args["TP_FIR_LEN"]: TP_FIR_LEN=args["TP_FIR_LEN"]
  else: TP_FIR_LEN=0
  return fn_update_TP_FIR_LEN(TT_DATA, TT_COEFF, TP_API, TP_USE_COEFF_RELOAD, AIE_VARIANT, TP_FIR_LEN)

def fn_update_TP_FIR_LEN(TT_DATA, TT_COEFF, TP_API, TP_USE_COEFF_RELOAD, AIE_VARIANT, TP_FIR_LEN):
  coeffSizeMult = 1
  if AIE_VARIANT == 1:
    coeffSizeMult = 1 if TP_API == 0 else TP_INTERPOLATE_FACTOR_max_aie1
  if AIE_VARIANT == 2:
    coeffSizeMult = TP_INTERPOLATE_FACTOR_max_aie2

  TP_FIR_LEN_max_int1=fn_max_fir_len_each_kernel_update(TT_DATA, TP_CASC_LEN_max, TP_USE_COEFF_RELOAD, TP_SSR_max, TP_API, coeffSizeMult)
  TP_FIR_LEN_max_int2=fn_max_fir_len_overall(TT_DATA, TT_COEFF)
  TP_FIR_LEN_max_int3=min(TP_FIR_LEN_max_int1, TP_FIR_LEN_max_int2, TP_FIR_LEN_max)

  param_dict={
     "name"    : "TP_FIR_LEN",
     "minimum" : TP_FIR_LEN_min,
     "maximum" : TP_FIR_LEN_max_int3
  }

  if TP_FIR_LEN !=0:
    if AIE_VARIANT == 2:
      TP_INTERPOLATE_FACTOR_max=TP_INTERPOLATE_FACTOR_max_aie2
    else:
      TP_INTERPOLATE_FACTOR_max=TP_INTERPOLATE_FACTOR_max_aie1
    legal_set_TP_INTERPOLATE_FACTOR=list(range(TP_INTERPOLATE_FACTOR_min, TP_INTERPOLATE_FACTOR_max+1))

    for interp_fact in legal_set_TP_INTERPOLATE_FACTOR:
      if TP_FIR_LEN % interp_fact == 0:
        return param_dict    
      
    TP_FIR_LEN_act=round(TP_FIR_LEN/TP_INTERPOLATE_FACTOR_min) * TP_INTERPOLATE_FACTOR_min
    if TP_FIR_LEN_act < param_dict["minimum"]:
        TP_FIR_LEN_act = param_dict["minimum"]

    if (TP_FIR_LEN_act > param_dict["maximum"]):
        TP_FIR_LEN_act = int(FLOOR(param_dict["maximum"], TP_INTERPOLATE_FACTOR_min))
    param_dict.update({"actual" : int(TP_FIR_LEN_act)})
  
  return param_dict

def validate_TP_FIR_LEN(args):
  TT_DATA = args["TT_DATA"]
  TT_COEFF = args["TT_COEFF"]
  TP_API = args["TP_API"]
  TP_USE_COEFF_RELOAD = args["TP_USE_COEFF_RELOAD"]
  AIE_VARIANT = args["AIE_VARIANT"]
  TP_FIR_LEN=args["TP_FIR_LEN"]
  return fn_validate_TP_FIR_LEN(TT_DATA, TT_COEFF, TP_API, TP_USE_COEFF_RELOAD, AIE_VARIANT, TP_FIR_LEN)
  
def fn_validate_TP_FIR_LEN(TT_DATA, TT_COEFF, TP_API, TP_USE_COEFF_RELOAD, AIE_VARIANT, TP_FIR_LEN): 
  if AIE_VARIANT == 2:
    TP_INTERPOLATE_FACTOR_max=TP_INTERPOLATE_FACTOR_max_aie2
  else:
    TP_INTERPOLATE_FACTOR_max=TP_INTERPOLATE_FACTOR_max_aie1
  legal_set_TP_INTERPOLATE_FACTOR=list(range(TP_INTERPOLATE_FACTOR_min, TP_INTERPOLATE_FACTOR_max+1))

  for interp_fact in legal_set_TP_INTERPOLATE_FACTOR:
    if TP_FIR_LEN % interp_fact == 0:
      param_dict=fn_update_TP_FIR_LEN(TT_DATA, TT_COEFF, TP_API, TP_USE_COEFF_RELOAD, AIE_VARIANT, TP_FIR_LEN)
      range_TP_FIR_LEN=[param_dict["minimum"], param_dict["maximum"]]
      return (validate_range(range_TP_FIR_LEN, "TP_FIR_LEN", TP_FIR_LEN))
  return isError(f"TP_FIR_LEN should be a multiple of one of the possible TP_INTERPOLATE_FACTOR within the range : {[TP_INTERPOLATE_FACTOR_min, TP_INTERPOLATE_FACTOR_max]}")

def fn_max_fir_len_overall(TT_DATA, TT_COEFF):
  maxTaps = {
    ( "int16",  "int16") : 4096,
    ("cint16",  "int16") : 4096,
    ("cint16", "cint16") : 2048,
    ( "int32",  "int16") : 4096,
    ( "int32",  "int32") : 2048,
    ( "int16",  "int32") : 2048,
    ("cint16",  "int32") : 2048,
    ("cint16", "cint32") : 1024,
    ("cint32",  "int16") : 2048,
    ("cint32", "cint16") : 2048,
    ("cint32",  "int32") : 2048,
    ("cint32", "cint32") : 1024,
    ( "float",  "float") : 2048,
    ("cfloat",  "float") : 2048,
    ("cfloat", "cfloat") : 1024
  }

  TP_FIR_LEN_max_overall= maxTaps[(TT_DATA, TT_COEFF)]
  return TP_FIR_LEN_max_overall

#######################################################
##### TP_INTERPOLATE_FACTOR Updater and Validator #####
#######################################################
def update_TP_INTERPOLATE_FACTOR(args):
  AIE_VARIANT = args["AIE_VARIANT"]
  TP_FIR_LEN = args["TP_FIR_LEN"]
  return fn_update_TP_INTERPOLATE_FACTOR(AIE_VARIANT, TP_FIR_LEN)

def fn_update_TP_INTERPOLATE_FACTOR(AIE_VARIANT, TP_FIR_LEN):
  if AIE_VARIANT == 2:
    TP_INTERPOLATE_FACTOR_max=TP_INTERPOLATE_FACTOR_max_aie2
  else:
    TP_INTERPOLATE_FACTOR_max=TP_INTERPOLATE_FACTOR_max_aie1

  legal_set_interp_factor=list(range(TP_INTERPOLATE_FACTOR_min, TP_INTERPOLATE_FACTOR_max+1))

  remove_set=[]
  for interp_fac in legal_set_interp_factor.copy():
    if TP_FIR_LEN % interp_fac != 0:
      remove_set.append(interp_fac)
  legal_set_interp_factor_1=remove_from_set(remove_set, legal_set_interp_factor.copy())

  param_dict={
     "name"    : "TP_INTERPOLATE_FACTOR"}
  if legal_set_interp_factor ==legal_set_interp_factor_1:
    param_dict.update({"minimum" : TP_INTERPOLATE_FACTOR_min,
                       "maximum" : TP_INTERPOLATE_FACTOR_max})  
  else: 
    param_dict.update({"enum" : legal_set_interp_factor_1})  
  return param_dict

def validate_TP_INTERPOLATE_FACTOR(args):
  TP_INTERPOLATE_FACTOR = args["TP_INTERPOLATE_FACTOR"]
  AIE_VARIANT = args["AIE_VARIANT"]
  TP_FIR_LEN = args["TP_FIR_LEN"]

  return fn_validate_TP_INTERPOLATE_FACTOR(TP_FIR_LEN, TP_INTERPOLATE_FACTOR, AIE_VARIANT)

def fn_validate_TP_INTERPOLATE_FACTOR(TP_FIR_LEN, TP_INTERPOLATE_FACTOR, AIE_VARIANT):
  param_dict=fn_update_TP_INTERPOLATE_FACTOR(AIE_VARIANT, TP_FIR_LEN)
  if "enum" in param_dict:
    return validate_legal_set(param_dict["enum"], "TP_INTERPOLATE_FACTOR", TP_INTERPOLATE_FACTOR)
  else:
    range_TP_INTERPOLATE_FACTOR=[param_dict["minimum"], param_dict["maximum"]]
    return validate_range(range_TP_INTERPOLATE_FACTOR, "TP_INTERPOLATE_FACTOR", TP_INTERPOLATE_FACTOR)

#######################################################
###### TP_PARA_INTERP_POLY Updater and Validator ######
#######################################################
def update_TP_PARA_INTERP_POLY(args):
  TP_INTERPOLATE_FACTOR = args["TP_INTERPOLATE_FACTOR"]
  return fn_update_TP_PARA_INTERP_POLY(TP_INTERPOLATE_FACTOR)

def fn_update_TP_PARA_INTERP_POLY(TP_INTERPOLATE_FACTOR):
  legal_set_TP_PARA_INTERP_POLY=find_divisors(TP_INTERPOLATE_FACTOR, TP_INTERPOLATE_FACTOR)
  param_dict={
     "name" : "TP_PARA_INTERP_POLY",
     "enum" : legal_set_TP_PARA_INTERP_POLY
    }
  return param_dict

def validate_TP_PARA_INTERP_POLY(args):
  TP_INTERPOLATE_FACTOR = args["TP_INTERPOLATE_FACTOR"]
  TP_PARA_INTERP_POLY = args["TP_PARA_INTERP_POLY"]
  return fn_validate_TP_PARA_INTERP_POLY(TP_INTERPOLATE_FACTOR, TP_PARA_INTERP_POLY)

def fn_validate_TP_PARA_INTERP_POLY(TP_INTERPOLATE_FACTOR, TP_PARA_INTERP_POLY):
  param_dict=fn_update_TP_PARA_INTERP_POLY(TP_INTERPOLATE_FACTOR)
  return(validate_legal_set(param_dict["enum"], "TP_PARA_INTERP_POLY", TP_PARA_INTERP_POLY))

#######################################################
############# TP_DUAL_IP Updater and Validator ########
#######################################################
def update_TP_DUAL_IP(args):
  AIE_VARIANT   = args["AIE_VARIANT"]
  TP_API   = args["TP_API"]
  return fn_update_interp_dual_ip(AIE_VARIANT, TP_API)

def validate_TP_DUAL_IP(args):
  AIE_VARIANT= args["AIE_VARIANT"]
  TP_API     = args["TP_API"]
  TP_DUAL_IP = args["TP_DUAL_IP"]
  return fn_validate_TP_DUAL_IP(AIE_VARIANT, TP_API, TP_DUAL_IP)

def fn_validate_TP_DUAL_IP(AIE_VARIANT, TP_API, TP_DUAL_IP):
  param_dict=fn_update_interp_dual_ip(AIE_VARIANT, TP_API)
  return (validate_legal_set(param_dict["enum"], "TP_DUAL_IP", TP_DUAL_IP))

#######################################################
####### TP_NUM_OUTPUTS Updater and Validator ##########
#######################################################
def update_TP_NUM_OUTPUTS(args):
    AIE_VARIANT = args["AIE_VARIANT"]
    TP_API = args["TP_API"]
    return(fn_update_interp_num_outs(TP_API, AIE_VARIANT))

def validate_TP_NUM_OUTPUTS(args):
    AIE_VARIANT = args["AIE_VARIANT"]
    TP_API = args["TP_API"]
    TP_NUM_OUTPUTS = args["TP_NUM_OUTPUTS"]
    return fn_validate_TP_NUM_OUTPUTS(AIE_VARIANT, TP_API, TP_NUM_OUTPUTS)

def fn_validate_TP_NUM_OUTPUTS(AIE_VARIANT, TP_API, TP_NUM_OUTPUTS):
    param_dict=fn_update_interp_num_outs(TP_API, AIE_VARIANT)
    return (validate_legal_set(param_dict["enum"], "TP_NUM_OUTPUTS", TP_NUM_OUTPUTS))

#######################################################
############## TP_SSR Updater and Validator ###########
#######################################################
def update_TP_SSR(args):
  nargs, uut_kernel = poly.get_modified_args_from_polyphase_decomposer(args, current_uut_kernel)
  # if we've decomposed to another type of kernel, then import that kernel and use that validate function
  if uut_kernel != current_uut_kernel:
    other_kernel = importlib.import_module(uut_kernel)
    return other_kernel.update_TP_SSR(nargs)
  else:
    AIE_VARIANT=args["AIE_VARIANT"]
    TP_API=args["TP_API"]
    TP_FIR_LEN=args["TP_FIR_LEN"]
    TP_INTERPOLATE_FACTOR=args["TP_INTERPOLATE_FACTOR"]
    TP_PARA_INTERP_POLY=args["TP_PARA_INTERP_POLY"]
    return fn_update_TP_SSR(AIE_VARIANT, TP_API, TP_FIR_LEN, TP_INTERPOLATE_FACTOR, TP_PARA_INTERP_POLY)

def fn_update_TP_SSR(AIE_VARIANT, TP_API, TP_FIR_LEN, TP_INTERPOLATE_FACTOR, TP_PARA_INTERP_POLY):
  legal_set_TP_SSR=find_divisors(TP_FIR_LEN, TP_SSR_max)

  if (AIE_VARIANT == 2 and (TP_INTERPOLATE_FACTOR != TP_PARA_INTERP_POLY)) or (TP_API==0):
    legal_set_TP_SSR=[1]
 
  param_dict={"name" : "TP_SSR",
              "enum" : legal_set_TP_SSR}

  return param_dict

def validate_TP_SSR(args):
  nargs, uut_kernel = poly.get_modified_args_from_polyphase_decomposer(args, current_uut_kernel)
  # if we've decomposed to another type of kernel, then import that kernel and use that validate function
  if uut_kernel != current_uut_kernel:
    other_kernel = importlib.import_module(uut_kernel)
    return other_kernel.validate_TP_SSR(nargs)
  else:
    AIE_VARIANT=args["AIE_VARIANT"]
    TP_API=args["TP_API"]
    TP_FIR_LEN=args["TP_FIR_LEN"]
    TP_INTERPOLATE_FACTOR=args["TP_INTERPOLATE_FACTOR"]
    TP_PARA_INTERP_POLY=args["TP_PARA_INTERP_POLY"]
    TP_SSR=args["TP_SSR"]
    return fn_validate_TP_SSR(AIE_VARIANT, TP_API, TP_FIR_LEN, TP_SSR, TP_INTERPOLATE_FACTOR, TP_PARA_INTERP_POLY)

def fn_validate_TP_SSR(AIE_VARIANT, TP_API, TP_FIR_LEN, TP_SSR, TP_INTERPOLATE_FACTOR, TP_PARA_INTERP_POLY):
  param_dict=fn_update_TP_SSR(AIE_VARIANT, TP_API, TP_FIR_LEN, TP_INTERPOLATE_FACTOR, TP_PARA_INTERP_POLY)
  return (validate_legal_set(param_dict["enum"], "TP_SSR", TP_SSR))

#######################################################
########### TP_CASC_LEN Updater and Validator #########
#######################################################
def update_TP_CASC_LEN(args):
  nargs, uut_kernel = poly.get_modified_args_from_polyphase_decomposer(args, current_uut_kernel)
  # if we've decomposed to another type of kernel, then import that kernel and use that validate function
  if uut_kernel != current_uut_kernel:
    other_kernel = importlib.import_module(uut_kernel)
    return other_kernel.update_TP_CASC_LEN(nargs)
  else:
    AIE_VARIANT=args["AIE_VARIANT"]
    TT_DATA=args["TT_DATA"]
    TT_COEFF=args["TT_COEFF"]
    TP_API=args["TP_API"]
    TP_USE_COEFF_RELOAD=args["TP_USE_COEFF_RELOAD"]
    TP_API=args["TP_API"]
    TP_FIR_LEN=args["TP_FIR_LEN"]
    TP_SSR=args["TP_SSR"]
    TP_INTERPOLATE_FACTOR=args["TP_INTERPOLATE_FACTOR"]
    TP_DUAL_IP=args["TP_DUAL_IP"]
    return fn_update_TP_CASC_LEN(AIE_VARIANT, TT_DATA, TT_COEFF, TP_API, TP_USE_COEFF_RELOAD, TP_FIR_LEN, TP_SSR, TP_INTERPOLATE_FACTOR, TP_DUAL_IP)

def fn_update_TP_CASC_LEN(AIE_VARIANT, TT_DATA, TT_COEFF, TP_API, TP_USE_COEFF_RELOAD, TP_FIR_LEN, TP_SSR, TP_INTERPOLATE_FACTOR, TP_DUAL_IP):
  legal_set_casc1=list(range(TP_CASC_LEN_min, TP_CASC_LEN_max+1))
  legal_set_casc2=fn_eliminate_casc_len_min_fir_len_each_kernel(legal_set_casc1.copy(), TP_FIR_LEN, TP_SSR, TP_INTERPOLATE_FACTOR)
  
  coeffSizeMult = 1
  if AIE_VARIANT == 1:
    coeffSizeMult = 1 if TP_API == 0 else TP_INTERPOLATE_FACTOR
  if AIE_VARIANT == 2:
    coeffSizeMult = TP_INTERPOLATE_FACTOR

  legal_set_casc3=fn_eliminate_casc_len_max_fir_len_each_kernel(TT_DATA, TP_FIR_LEN, TP_USE_COEFF_RELOAD, TP_SSR, TP_API, coeffSizeMult, legal_set_casc2.copy())

  legal_set_casc4=fn_eliminate_casc_len_check_samples_can_fit_streaming(TT_DATA, TT_COEFF, TP_FIR_LEN, TP_INTERPOLATE_FACTOR, TP_DUAL_IP, TP_API, TP_SSR, legal_set_casc3.copy())

  param_dict={
      "name" :  "TP_CASC_LEN" }

  if legal_set_casc1==legal_set_casc4:
      param_dict.update({"minimum" : TP_CASC_LEN_min})
      param_dict.update({"maximum" : TP_CASC_LEN_max})
  else:
      param_dict.update({"enum" : legal_set_casc4})

  return param_dict
  
def validate_TP_CASC_LEN(args):
  nargs, uut_kernel = poly.get_modified_args_from_polyphase_decomposer(args, current_uut_kernel)
  # if we've decomposed to another type of kernel, then import that kernel and use that validate function
  if uut_kernel != current_uut_kernel:
    other_kernel = importlib.import_module(uut_kernel)
    return other_kernel.validate_TP_CASC_LEN(nargs)
  else:
    AIE_VARIANT=args["AIE_VARIANT"]
    TT_DATA=args["TT_DATA"]
    TT_COEFF=args["TT_COEFF"]
    TP_API=args["TP_API"]
    TP_USE_COEFF_RELOAD=args["TP_USE_COEFF_RELOAD"]
    TP_API=args["TP_API"]
    TP_FIR_LEN=args["TP_FIR_LEN"]
    TP_SSR=args["TP_SSR"]
    TP_INTERPOLATE_FACTOR=args["TP_INTERPOLATE_FACTOR"]
    TP_DUAL_IP=args["TP_DUAL_IP"]
    TP_CASC_LEN=args["TP_CASC_LEN"]
    return fn_validate_TP_CASC_LEN(AIE_VARIANT, TT_DATA, TT_COEFF, TP_API, TP_USE_COEFF_RELOAD, TP_FIR_LEN, TP_SSR, TP_INTERPOLATE_FACTOR, TP_DUAL_IP, TP_CASC_LEN)

def fn_validate_TP_CASC_LEN(AIE_VARIANT, TT_DATA, TT_COEFF, TP_API, TP_USE_COEFF_RELOAD, TP_FIR_LEN, TP_SSR, TP_INTERPOLATE_FACTOR, TP_DUAL_IP, TP_CASC_LEN):
  param_dict=fn_update_TP_CASC_LEN(AIE_VARIANT, TT_DATA, TT_COEFF, TP_API, TP_USE_COEFF_RELOAD, TP_FIR_LEN, TP_SSR, TP_INTERPOLATE_FACTOR, TP_DUAL_IP)
  if "enum" in param_dict:
      return (validate_legal_set(param_dict["enum"], "TP_CASC_LEN", TP_CASC_LEN))
  else:
      range_casc_len=[param_dict["minimum"], param_dict["maximum"]]
      return(validate_range(range_casc_len, "TP_CASC_LEN", TP_CASC_LEN))

def fn_eliminate_casc_len_check_samples_can_fit_streaming(TT_DATA, TT_COEFF, TP_FIR_LEN, TP_INTERPOLATE_FACTOR, TP_DUAL_IP, TP_API, TP_SSR, legal_set_casc_len):
  legal_set_casc_len_int=legal_set_casc_len.copy()

  remove_list=[]
  for casc_len in legal_set_casc_len_int.copy():
    streamingVectorRegisterCheck = fn_check_samples_can_fit_streaming(TT_DATA, TT_COEFF, TP_FIR_LEN, TP_INTERPOLATE_FACTOR, casc_len, TP_DUAL_IP, TP_API, TP_SSR)
    if streamingVectorRegisterCheck != isValid:
      remove_list.append(casc_len)

  legal_set_casc_len_int=remove_from_set(remove_list, legal_set_casc_len_int.copy())
  return legal_set_casc_len_int

def fn_check_samples_can_fit_streaming(TT_DATA, TT_COEFF, TP_FIR_LEN, TP_INTERPOLATE_FACTOR, TP_CASC_LEN, TP_DUAL_IP, TP_API, TP_SSR):
  m_kNumColumns = 2 if fn_size_by_byte(TT_COEFF) == 2 else 1

  m_kSamplesInBuff     = (1024//8)//fn_size_by_byte(TT_DATA)
  sizeOfA256Read = (256//8)//fn_size_by_byte(TT_DATA)

  sizeOfARead = sizeOfA256Read//2 if TP_DUAL_IP == 0 else sizeOfA256Read;
  firLenPerSsr = CEIL(TP_FIR_LEN, (TP_INTERPOLATE_FACTOR*TP_SSR))/TP_SSR
  if TP_API != 0:
    for kernelPos in range(TP_CASC_LEN):
      TP_FIR_RANGE_LEN =  (
        fnFirRangeRem(firLenPerSsr,TP_CASC_LEN,kernelPos,TP_INTERPOLATE_FACTOR)
          if (kernelPos == (TP_CASC_LEN-1))
          else
            fnFirRange(firLenPerSsr,TP_CASC_LEN,kernelPos,TP_INTERPOLATE_FACTOR)
      )
      numSamples = CEIL(TP_FIR_RANGE_LEN//TP_INTERPOLATE_FACTOR, m_kNumColumns) + sizeOfARead
      if numSamples > m_kSamplesInBuff :
        return isError(
            f"Requested parameters: FIR length ({TP_FIR_LEN}), interpolate factor ({TP_INTERPOLATE_FACTOR}), cascade length ({TP_CASC_LEN}) and SSR ({TP_SSR}) result in a kernel ({kernelPos}) that requires more data samples ({numSamples}) than capacity of a data buffer ({m_kSamplesInBuff}) "
            f"Please increase the cascade length ({TP_CASC_LEN}) and/or SSR ({TP_SSR})."
        )

  return isValid

#######################################################
##### TP_INPUT_WINDOW_VSIZE Updater and Validator #####
#######################################################
def update_TP_INPUT_WINDOW_VSIZE(args):

  factor_TP_INPUT_WINDOW_VSIZE=poly.fn_factor_decomposer_TP_INPUT_WINDOW_VSIZE(args)
  args, uut_kernel = poly.get_modified_args_from_polyphase_decomposer(args, current_uut_kernel)

  param_dict={}
  if uut_kernel != current_uut_kernel:
    other_kernel = importlib.import_module(uut_kernel)
    param_dict.update(other_kernel.update_TP_INPUT_WINDOW_VSIZE(args))
    if args["TP_INPUT_WINDOW_VSIZE"]: TP_INPUT_WINDOW_VSIZE=args["TP_INPUT_WINDOW_VSIZE"]
    else: TP_INPUT_WINDOW_VSIZE=0
    return (deci_asym.fn_update_factor_TP_INPUT_VSIZE(factor_TP_INPUT_WINDOW_VSIZE, TP_INPUT_WINDOW_VSIZE, param_dict))
  else:
    if args["TP_INPUT_WINDOW_VSIZE"]: TP_INPUT_WINDOW_VSIZE=args["TP_INPUT_WINDOW_VSIZE"]
    else: TP_INPUT_WINDOW_VSIZE=0
    TT_DATA = args["TT_DATA"]
    TT_COEFF = args["TT_COEFF"]
    TP_FIR_LEN = args["TP_FIR_LEN"]
    TP_API = args["TP_API"]
    TP_INTERPOLATE_FACTOR = args["TP_INTERPOLATE_FACTOR"]
    TP_SSR = args["TP_SSR"]
    return fn_update_TP_INPUT_WINDOW_VSIZE(TT_DATA, TT_COEFF, TP_FIR_LEN, TP_INTERPOLATE_FACTOR, TP_INPUT_WINDOW_VSIZE, TP_API, TP_SSR, factor_TP_INPUT_WINDOW_VSIZE)

def fn_update_TP_INPUT_WINDOW_VSIZE(TT_DATA, TT_COEFF, TP_FIR_LEN, TP_INTERPOLATE_FACTOR, TP_INPUT_WINDOW_VSIZE, TP_API, TP_SSR, factor_TP_INPUT_WINDOW_VSIZE):

  if TP_API == 0:
    TP_INPUT_WINDOW_VSIZE_max=fn_max_windowsize_for_buffer_update(TT_DATA, TP_FIR_LEN, TP_SSR, TP_INTERPOLATE_FACTOR, AIE_VARIANT=1)
  else: TP_INPUT_WINDOW_VSIZE_max=TP_INPUT_WINDOW_VSIZE_max_cpp

  param_dict={
    "name"    : TP_INPUT_WINDOW_VSIZE,
    "minimum" : TP_INPUT_WINDOW_VSIZE_min,
    "maximum" : TP_INPUT_WINDOW_VSIZE_max
  }

  streamRptFactor = 4
  windowSizeMultiplier = (fnNumLanes(TT_DATA, TT_COEFF, TP_API)) if TP_API == 0 else (fnNumLanes(TT_DATA, TT_COEFF, TP_API)*streamRptFactor)

  factor_window_size= windowSizeMultiplier * TP_SSR
  lcm_ws=find_lcm_list([factor_window_size, factor_TP_INPUT_WINDOW_VSIZE])
  return deci_asym.fn_update_factor_TP_INPUT_VSIZE(lcm_ws, TP_INPUT_WINDOW_VSIZE, param_dict)


def validate_TP_INPUT_WINDOW_VSIZE(args):
  factor_TP_INPUT_WINDOW_VSIZE=poly.fn_factor_decomposer_TP_INPUT_WINDOW_VSIZE(args)
  args, uut_kernel = poly.get_modified_args_from_polyphase_decomposer(args, current_uut_kernel)
  if uut_kernel != current_uut_kernel:
    other_kernel = importlib.import_module(uut_kernel)
    check_uut_kernel=other_kernel.validate_TP_INPUT_WINDOW_VSIZE(args)
    if check_uut_kernel==isValid:
      TP_INPUT_WINDOW_VSIZE = args["TP_INPUT_WINDOW_VSIZE"]
      return (deci_asym.fn_validate_factor_TP_INPUT_VSIZE(factor_TP_INPUT_WINDOW_VSIZE, TP_INPUT_WINDOW_VSIZE))
    else:
      return check_uut_kernel
  else:
    TP_INPUT_WINDOW_VSIZE = args["TP_INPUT_WINDOW_VSIZE"]
    TT_DATA = args["TT_DATA"]
    TT_COEFF = args["TT_COEFF"]
    TP_FIR_LEN = args["TP_FIR_LEN"]
    TP_API = args["TP_API"]
    TP_INTERPOLATE_FACTOR = args["TP_INTERPOLATE_FACTOR"]
    TP_SSR = args["TP_SSR"]
    return fn_validate_TP_INPUT_WINDOW_VSIZE(TT_DATA, TT_COEFF, TP_FIR_LEN, TP_INTERPOLATE_FACTOR, TP_INPUT_WINDOW_VSIZE, TP_API, TP_SSR, factor_TP_INPUT_WINDOW_VSIZE)
  
def fn_validate_TP_INPUT_WINDOW_VSIZE(TT_DATA, TT_COEFF, TP_FIR_LEN, TP_INTERPOLATE_FACTOR, TP_INPUT_WINDOW_VSIZE, TP_API, TP_SSR, factor_TP_INPUT_WINDOW_VSIZE):
  streamRptFactor = 4
  windowSizeMultiplier = (fnNumLanes(TT_DATA, TT_COEFF, TP_API)) if TP_API == 0 else (fnNumLanes(TT_DATA, TT_COEFF, TP_API)*streamRptFactor)

  factor_window_size= windowSizeMultiplier * TP_SSR
  lcm_ws=find_lcm_list([factor_window_size, factor_TP_INPUT_WINDOW_VSIZE])

  check_factor=deci_asym.fn_validate_factor_TP_INPUT_VSIZE(lcm_ws, TP_INPUT_WINDOW_VSIZE)
  if check_factor==isValid:
    param_dict=fn_update_TP_INPUT_WINDOW_VSIZE(TT_DATA, TT_COEFF, TP_FIR_LEN, TP_INTERPOLATE_FACTOR, TP_INPUT_WINDOW_VSIZE, TP_API, TP_SSR, factor_TP_INPUT_WINDOW_VSIZE)

    range_TP_INPUT_WINDOW_VSIZE=[param_dict["minimum"], param_dict["maximum"]]
    return (validate_range(range_TP_INPUT_WINDOW_VSIZE, "TP_INPUT_WINDOW_VSIZE", TP_INPUT_WINDOW_VSIZE))
  else:
    return check_factor

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
  return fn_update_coeff(TT_COEFF, TP_FIR_LEN)

def fn_update_coeff(TT_COEFF, TP_FIR_LEN):
  
  if fn_is_complex(TT_COEFF) : len_coeff=2*TP_FIR_LEN
  else: len_coeff=TP_FIR_LEN

  param_dict={"name" : "coeff",
              "len"  : len_coeff}

  return param_dict

def validate_coeff(args):
  TT_COEFF = args["TT_COEFF"]
  TP_FIR_LEN = args["TP_FIR_LEN"]
  coeff_list = args["coeff"]
  return fn_validate_coeff(TT_COEFF, TP_FIR_LEN, coeff_list)

def fn_validate_coeff(TT_COEFF, TP_FIR_LEN, coeff_list):
  param_dict=fn_update_coeff(TT_COEFF, TP_FIR_LEN)
  return validate_LUT_len(coeff_list, param_dict["len"])


#### port ####
def info_ports(args):
    """Standard function creating a static dictionary of information
    for upper software to correctly connect the IP.
    Some IP has dynamic number of ports according to parameter set,
    so port information has to be implemented as a function"""
    TT_DATA = args["TT_DATA"]
    TT_COEFF = args["TT_COEFF"]
    TP_INPUT_WINDOW_VSIZE = args["TP_INPUT_WINDOW_VSIZE"]
    TP_FIR_LEN = args["TP_FIR_LEN"]
    TP_SSR = args["TP_SSR"]
    TP_INTERPOLATE_FACTOR = args["TP_INTERPOLATE_FACTOR"]
    TP_API = args["TP_API"]
    TP_DUAL_IP = args["TP_DUAL_IP"]
    TP_NUM_OUTPUTS = args["TP_NUM_OUTPUTS"]
    TP_PARA_INTERP_POLY = (args["TP_PARA_INTERP_POLY"] if ("TP_PARA_INTERP_POLY" in args)  else 1)
    TP_DECIMATE_FACTOR = 1
    margin_size = sr_asym.fn_margin_size(TP_FIR_LEN//TP_INTERPOLATE_FACTOR, TT_DATA)

    num_in_ports = TP_SSR # *TP_PARA_DECI_POLY (not in the internpolator)
    num_out_ports = TP_SSR*TP_PARA_INTERP_POLY

    in_win_size = get_input_window_size(TP_INPUT_WINDOW_VSIZE, num_in_ports, TP_API, TP_DUAL_IP)
    out_win_size = get_output_window_size(TP_INPUT_WINDOW_VSIZE, num_out_ports, TP_API, TP_NUM_OUTPUTS, TP_DECIMATE_FACTOR, TP_INTERPOLATE_FACTOR)

    in_ports = get_port_info("in", "in", TT_DATA, in_win_size, num_in_ports, marginSize=margin_size, TP_API=TP_API)
    in2_ports = (get_port_info("in2", "in", TT_DATA, in_win_size, num_in_ports, marginSize=margin_size, TP_API=TP_API) if (TP_DUAL_IP == 1) else [])
    coeff_ports = (get_parameter_port_info("coeff", "in", TT_COEFF, TP_SSR, TP_FIR_LEN, "async") if (args["TP_USE_COEFF_RELOAD"] == 1) else [])

    # decimate by 2 for halfband
    out_ports = get_port_info("out", "out", TT_DATA, out_win_size, num_out_ports, TP_API=args["TP_API"])
    out2_ports = (get_port_info("out2", "out", TT_DATA, out_win_size, num_out_ports, TP_API=args["TP_API"]) if (args["TP_NUM_OUTPUTS"] == 2) else [])
    return in_ports + in2_ports + coeff_ports + out_ports + out2_ports


#### graph generator ####
# def get_param_list(**kwargs):
#   [f"{value}{comma}} //{key}" for key, value in kwargs.iteritems() for comma in "," ]
def generate_graph(graphname, args):

  if graphname == "":
    graphname = "default_graphname"

  TT_COEFF = args["TT_COEFF"]
  TT_DATA = args["TT_DATA"]
  TP_FIR_LEN = args["TP_FIR_LEN"]
  TP_SHIFT = args["TP_SHIFT"]
  TP_RND = args["TP_RND"]
  TP_CASC_LEN = args["TP_CASC_LEN"]
  TP_INPUT_WINDOW_VSIZE = args["TP_INPUT_WINDOW_VSIZE"]
  TP_INTERPOLATE_FACTOR = args["TP_INTERPOLATE_FACTOR"]
  TP_USE_COEFF_RELOAD = args["TP_USE_COEFF_RELOAD"]
  TP_NUM_OUTPUTS = args["TP_NUM_OUTPUTS"]
  TP_DUAL_IP = args["TP_DUAL_IP"]
  TP_API = args["TP_API"]
  TP_PARA_INTERP_POLY = (args["TP_PARA_INTERP_POLY"] if ("TP_PARA_INTERP_POLY" in args)  else 1)
  TP_SSR = args["TP_SSR"]
  coeff_list = args["coeff"]
  TP_SAT = args["TP_SAT"]

  taps = sr_asym.fn_get_taps_vector(TT_COEFF, coeff_list)
  constr_args_str = f"taps" if TP_USE_COEFF_RELOAD == 0 else ""
  dual_ip_declare_str = f" std::array<adf::port<input>, TP_SSR> in2;" if TP_DUAL_IP == 1 else "// No dual input"
  dual_ip_connect_str = f"adf::connect<> net_in2(in2[ssrIdx], filter.in2[ssrIdx]);" if TP_DUAL_IP == 1 else "// No dual input"
  coeff_ip_declare_str = f"std::array<adf::port<input>, TP_SSR*TP_PARA_INTERP_POLY> coeff;" if TP_USE_COEFF_RELOAD == 1 else "//No coeff port"
  coeff_ip_connect_str = f"adf::connect<> net_coeff(coeff[outPortIdx], filter.coeff[outPortIdx]);" if TP_USE_COEFF_RELOAD == 1 else "//No coeff port"

  dual_op_declare_str = f"std::array<adf::port<output>, TP_SSR*TP_PARA_INTERP_POLY> out2;" if TP_NUM_OUTPUTS == 2 else "// No dual output"
  dual_op_connect_str = f"adf::connect<> net_out2(filter.out2[outPortIdx], out2[outPortIdx]);" if TP_NUM_OUTPUTS == 2 else "// No dual output"
  # Use formatted multi-line string to avoid a lot of \n and \t
  code  = (
f"""
class {graphname} : public adf::graph {{
public:
  static constexpr unsigned int TP_SSR = {TP_SSR};
  static constexpr unsigned int TP_PARA_INTERP_POLY = {TP_PARA_INTERP_POLY};

  std::array<adf::port<input>, TP_SSR> in;
  {dual_ip_declare_str}
  {coeff_ip_declare_str}
  std::array<adf::port<output>, TP_SSR*TP_PARA_INTERP_POLY> out;
  {dual_op_declare_str}

  std::vector<{TT_COEFF}> taps = {taps};
  xf::dsp::aie::fir::interpolate_asym::fir_interpolate_asym_graph<
    {TT_DATA}, //TT_DATA
    {TT_COEFF}, //TT_COEFF
    {TP_FIR_LEN}, //TP_FIR_LEN
    {TP_INTERPOLATE_FACTOR}, //TP_INTERPOLATE_FACTOR
    {TP_SHIFT}, //TP_SHIFT
    {TP_RND}, //TP_RND
    {TP_INPUT_WINDOW_VSIZE}, //TP_INPUT_WINDOW_VSIZE
    {TP_CASC_LEN}, //TP_CASC_LEN
    {TP_USE_COEFF_RELOAD}, //TP_USE_COEFF_RELOAD
    {TP_DUAL_IP}, //TP_DUAL_IP
    {TP_NUM_OUTPUTS}, //TP_NUM_OUTPUTS
    {TP_API}, //TP_API
    {TP_SSR}, //TP_SSR
    {TP_PARA_INTERP_POLY}, //TP_PARA_INTERP_POLY
    {TP_SAT} //TP_SAT
  > filter;

  {graphname}() : filter({constr_args_str}) {{
    adf::kernel *filter_kernels = filter.getKernels();
    for (int i=0; i < 1; i++) {{
      adf::runtime<ratio>(filter_kernels[i]) = 0.9;
    }}
    for (int ssrIdx=0; ssrIdx < TP_SSR; ssrIdx++) {{
      adf::connect<> net_in(in[ssrIdx], filter.in[ssrIdx]);
      {dual_ip_connect_str}
    }}

    for (int paraPolyIdx=0; paraPolyIdx < TP_PARA_INTERP_POLY; paraPolyIdx++) {{
      for (int ssrIdx=0; ssrIdx < TP_SSR; ssrIdx++) {{
        unsigned outPortIdx = paraPolyIdx+ssrIdx*TP_PARA_INTERP_POLY;
        adf::connect<> net_out(filter.out[outPortIdx], out[outPortIdx]);
      {dual_op_connect_str}
      {coeff_ip_connect_str}
      }}
    }}
  }}

}};
""")
  out = {}
  out["graph"] = code
  out["port_info"] = info_ports(args)
  out["headerfile"] = "fir_interpolate_asym_graph.hpp"
  out["searchpaths"] = [
       "L2/include/aie",
       "L2/tests/aie/common/inc",
       "L1/include/aie",
       "L1/src/aie",
       "L1/tests/aie/inc",
       "L1/tests/aie/src"
  ]

  return out


