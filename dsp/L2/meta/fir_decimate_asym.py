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

fn_decimate_asym_lanes = fnNumLanes384b
TP_DECIMATE_FACTOR_min = 2
TP_DECIMATE_FACTOR_max = 7
TP_INPUT_WINDOW_VSIZE_min = 4
TP_PARA_DECI_POLY_min = 1
TP_CASC_LEN_min = 1
TP_CASC_LEN_max = 40
TP_FIR_LEN_min = 4
TP_FIR_LEN_max = 8192
TP_SSR_min = 1
TP_SSR_max = 16
TP_INPUT_WINDOW_VSIZE_max_cpp=2**31

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
########## TP_FIR_LEN Updater and Validator ###########
#######################################################
def update_TP_FIR_LEN(args):
  TT_DATA = args["TT_DATA"]
  TP_API = args["TP_API"]
  TP_USE_COEFF_RELOAD = args["TP_USE_COEFF_RELOAD"]
  AIE_VARIANT = args["AIE_VARIANT"]
  if args["TP_FIR_LEN"]: TP_FIR_LEN=args["TP_FIR_LEN"]
  else: TP_FIR_LEN=0
  return fn_update_TP_FIR_LEN(TT_DATA, TP_API, TP_USE_COEFF_RELOAD, AIE_VARIANT, TP_FIR_LEN)

 
def fn_update_TP_FIR_LEN(TT_DATA, TP_API, TP_USE_COEFF_RELOAD, AIE_VARIANT, TP_FIR_LEN):
  if AIE_VARIANT == 2:
    firLenPerPhaseDivider = TP_DECIMATE_FACTOR_max
  else:
    firLenPerPhaseDivider = 1
  TP_FIR_LEN_max_int1=fn_max_fir_len_each_kernel_update(TT_DATA, TP_CASC_LEN_max, TP_USE_COEFF_RELOAD, TP_SSR_max, TP_API, firLenPerPhaseDivider)

  if AIE_VARIANT == 2:
    TP_FIR_LEN_min_int1 = TP_DECIMATE_FACTOR_min * TP_CASC_LEN_min
  else:
    TP_FIR_LEN_min_int1 = 1
  TP_FIR_LEN_min_int2=max(TP_FIR_LEN_min, TP_FIR_LEN_min_int1)
        
  TP_FIR_LEN_max_int2=min(TP_FIR_LEN_max_int1, TP_FIR_LEN_max)
  param_dict={
      "name" : "TP_FIR_LEN",
      "minimum" : TP_FIR_LEN_min_int2,
      "maximum" : TP_FIR_LEN_max_int2
  }

  if AIE_VARIANT == 2 :
    TP_DECIMATE_FACTOR_max_int = 4
  else: TP_DECIMATE_FACTOR_max_int=TP_DECIMATE_FACTOR_max
  legal_set_TP_DECIMATE_FACTOR=list(range(TP_DECIMATE_FACTOR_min, TP_DECIMATE_FACTOR_max_int+1))

  if TP_FIR_LEN !=0:
    for deci_fact in legal_set_TP_DECIMATE_FACTOR:
      if TP_FIR_LEN % deci_fact == 0:
        return param_dict
      
      TP_FIR_LEN_act=round(TP_FIR_LEN/TP_DECIMATE_FACTOR_min) * TP_DECIMATE_FACTOR_min
      if TP_FIR_LEN_act < param_dict["minimum"]:
          TP_FIR_LEN_act = param_dict["minimum"]

      if (TP_FIR_LEN_act > param_dict["maximum"]):
          TP_FIR_LEN_act = int(FLOOR(param_dict["maximum"], TP_DECIMATE_FACTOR_min))
      param_dict.update({"actual" : int(TP_FIR_LEN_act)})
  return param_dict 


def validate_TP_FIR_LEN(args):
  TT_DATA = args["TT_DATA"]
  TP_API = args["TP_API"]
  TP_USE_COEFF_RELOAD = args["TP_USE_COEFF_RELOAD"]
  AIE_VARIANT = args["AIE_VARIANT"]
  TP_FIR_LEN = args["TP_FIR_LEN"]
  return fn_validate_TP_FIR_LEN(TT_DATA, TP_API, TP_USE_COEFF_RELOAD, AIE_VARIANT, TP_FIR_LEN)

def fn_validate_TP_FIR_LEN(TT_DATA, TP_API, TP_USE_COEFF_RELOAD, AIE_VARIANT, TP_FIR_LEN):
  if AIE_VARIANT == 2 :
    TP_DECIMATE_FACTOR_max_int = 4
  else: TP_DECIMATE_FACTOR_max_int=TP_DECIMATE_FACTOR_max
  legal_set_TP_DECIMATE_FACTOR=list(range(TP_DECIMATE_FACTOR_min, TP_DECIMATE_FACTOR_max_int+1))

  for deci_fact in legal_set_TP_DECIMATE_FACTOR:
    if TP_FIR_LEN % deci_fact == 0:
      param_dict=fn_update_TP_FIR_LEN(TT_DATA, TP_API, TP_USE_COEFF_RELOAD, AIE_VARIANT, TP_FIR_LEN)
      range_TP_FIR_LEN=[param_dict["minimum"], param_dict["maximum"]]
      return (validate_range(range_TP_FIR_LEN, "TP_FIR_LEN", TP_FIR_LEN))
  return isError(f"TP_FIR_LEN should be a multiple of one of the possible TP_DECIMATE_FACTOR legal set values : {legal_set_TP_DECIMATE_FACTOR}")
        

#######################################################
###### TP_DECIMATE_FACTOR Updater and Validator #######
#######################################################
def update_TP_DECIMATE_FACTOR(args):
  AIE_VARIANT=args["AIE_VARIANT"]
  TP_FIR_LEN=args["TP_FIR_LEN"]
  return fn_update_TP_DECIMATE_FACTOR(AIE_VARIANT, TP_FIR_LEN)

def fn_update_TP_DECIMATE_FACTOR(AIE_VARIANT, TP_FIR_LEN):

  if AIE_VARIANT == 2 :
    vector1kRegisters = 4 # AIE-ML tile contiants 4 independent 1024-bit vector registers that are used   
    TP_DECIMATE_FACTOR_max_int = vector1kRegisters
  else: TP_DECIMATE_FACTOR_max_int=TP_DECIMATE_FACTOR_max

  legal_set_deci_factor=list(range(TP_DECIMATE_FACTOR_min, TP_DECIMATE_FACTOR_max_int+1))
  remove_set=[]
  for deci_fac in legal_set_deci_factor.copy():
    if TP_FIR_LEN % deci_fac != 0:
      remove_set.append(deci_fac)
  
  legal_set_deci_factor_1=remove_from_set(remove_set, legal_set_deci_factor.copy())

  param_dict={
    "name" : "TP_DECIMATE_FACTOR",
    "enum" : legal_set_deci_factor_1
  }
  return param_dict

def validate_TP_DECIMATE_FACTOR(args):
  AIE_VARIANT=args["AIE_VARIANT"]
  TP_FIR_LEN=args["TP_FIR_LEN"]
  TP_DECIMATE_FACTOR = args["TP_DECIMATE_FACTOR"]
  return fn_validate_TP_DECIMATE_FACTOR(AIE_VARIANT, TP_FIR_LEN, TP_DECIMATE_FACTOR)

def fn_validate_TP_DECIMATE_FACTOR(AIE_VARIANT, TP_FIR_LEN, TP_DECIMATE_FACTOR):
  param_dict=fn_update_TP_DECIMATE_FACTOR(AIE_VARIANT, TP_FIR_LEN)
  return (validate_legal_set(param_dict["enum"], "TP_DECIMATE_FACTOR", TP_DECIMATE_FACTOR))

# only get a 4b offset value per lane (single hex digit), whereas some buffers are larger than this,
# so we need to catch the situation where decimate factor causes us to require more data in one op than we can index.
def fn_xoffset_range_valid(TT_DATA, TT_COEFF, TP_DECIMATE_FACTOR, TP_API):
  m_kLanes = fn_decimate_asym_lanes(TT_DATA, TT_COEFF)

  # When checking against xoffset, shouldn't we check m_kLanes*TP_DECIMATE_FACTOR??
  dataNeededBetweenOutputChunks =  (m_kLanes-1)*TP_DECIMATE_FACTOR

  m_kXoffsetRange = 8 if TT_DATA == "cfloat" else 16;
  #m_kFirInitOffset     = m_kFirRangeOffset + m_kFirMarginOffset;
  #m_kDataBuffXOffset   = m_kFirInitOffset % (m_kWinAccessByteSize/sizeof(TT_DATA));  // Remainder of m_kFirInitOffset divided by 128bit
  # CAUTION Fixed AIE1 constant for window read granularity
  m_kWinAccessByteSize = 128//8
  # Complicated to pull in lots of other code here, so we'll just go for worst case.
  m_kDataBuffXOffset = (m_kWinAccessByteSize//fn_size_by_byte(TT_DATA))-1


  buffSize = (1024//8) // fn_size_by_byte(TT_DATA)
  loadSizeBits = 128 if fn_base_type(TT_DATA) == "int32" else 256
  loadVSize = (loadSizeBits // 8) // fn_size_by_byte(TT_DATA)

  dataNeededWithAlignment = dataNeededBetweenOutputChunks + m_kDataBuffXOffset
  # CAUTION, I've tweaked this vs traits because the previous phrasing didn't make sense to me.
  # m_kDataRegVsize-m_kDataLoadVsize >= m_kDFDataRange
  if (dataNeededWithAlignment > (buffSize - loadVSize)):
    return isError(f"Decimate factor exceeded for this data type and coefficient type combination. Required input data ({dataNeededWithAlignment}) exceeds input vector's register ({(buffSize - loadVSize)}).")

  if (TP_API == 1 and dataNeededBetweenOutputChunks > m_kXoffsetRange):
    return isError(f"Decimate factor exceeded for this data type andcoefficient type combination. Required input data ({dataNeededBetweenOutputChunks}) exceeds input vector's register offset address range ({m_kXoffsetRange}).")

  return isValid


def fn_validate_decimate_factor(TT_DATA, TT_COEFF, TP_DECIMATE_FACTOR, TP_API, AIE_VARIANT):
  if AIE_VARIANT == 1 :
    # Check if permute xoffset is within range for the data type in question
    offsetRangeCheck = fn_xoffset_range_valid(TT_DATA, TT_COEFF, TP_DECIMATE_FACTOR, TP_API )
    return offsetRangeCheck
  else: return isValid

#######################################################
####### TP_PARA_DECI_POLY Updater and Validator #######
#######################################################
def update_TP_PARA_DECI_POLY(args):
  AIE_VARIANT = args["AIE_VARIANT"]
  TT_DATA = args["TT_DATA"]
  TT_COEFF = args["TT_COEFF"]
  TP_API = args["TP_API"]
  TP_DECIMATE_FACTOR = args["TP_DECIMATE_FACTOR"]
  return fn_update_TP_PARA_DECI_POLY(TT_DATA, TT_COEFF,TP_DECIMATE_FACTOR, TP_API, AIE_VARIANT)

def fn_update_TP_PARA_DECI_POLY(TT_DATA, TT_COEFF,TP_DECIMATE_FACTOR, TP_API, AIE_VARIANT):
  legal_set_TP_PARA_DECI_POLY=find_divisors(TP_DECIMATE_FACTOR, TP_DECIMATE_FACTOR)
  legal_set_TP_PARA_DECI_POLY_2=legal_set_TP_PARA_DECI_POLY.copy()

  for para_deci_poly in legal_set_TP_PARA_DECI_POLY.copy():
    kernelDecimate = TP_DECIMATE_FACTOR//para_deci_poly
    decimateValid = (
      fn_validate_decimate_factor(TT_DATA, TT_COEFF, kernelDecimate, TP_API, AIE_VARIANT) if kernelDecimate > 1
      else isValid  # no decimate factor to validate
    )
    if decimateValid != isValid:
      legal_set_TP_PARA_DECI_POLY_2=remove_from_set([para_deci_poly], legal_set_TP_PARA_DECI_POLY_2.copy())
  
  param_dict={
    "name" : "TP_PARA_DECI_POLY",
    "enum" : legal_set_TP_PARA_DECI_POLY_2
  }
  return param_dict

def validate_TP_PARA_DECI_POLY(args):
  AIE_VARIANT = args["AIE_VARIANT"]
  TT_DATA = args["TT_DATA"]
  TT_COEFF = args["TT_COEFF"]
  TP_API = args["TP_API"]
  TP_DECIMATE_FACTOR = args["TP_DECIMATE_FACTOR"]
  TP_PARA_DECI_POLY = args["TP_PARA_DECI_POLY"]
  return fn_validate_TP_PARA_DECI_POLY(TT_DATA, TT_COEFF,TP_DECIMATE_FACTOR, TP_API, AIE_VARIANT, TP_PARA_DECI_POLY)

def fn_validate_TP_PARA_DECI_POLY(TT_DATA, TT_COEFF,TP_DECIMATE_FACTOR, TP_API, AIE_VARIANT, TP_PARA_DECI_POLY):
   param_dict=fn_update_TP_PARA_DECI_POLY(TT_DATA, TT_COEFF,TP_DECIMATE_FACTOR, TP_API, AIE_VARIANT)
   return validate_legal_set(param_dict["enum"], "TP_PARA_DECI_POLY", TP_PARA_DECI_POLY)

#######################################################
############# TP_DUAL_IP Updater and Validator ########
#######################################################
def update_TP_DUAL_IP(args):
  AIE_VARIANT   = args["AIE_VARIANT"]
  TP_API   = args["TP_API"]
  return fn_update_TP_DUAL_IP(AIE_VARIANT, TP_API)

def fn_update_TP_DUAL_IP(AIE_VARIANT, TP_API):
  legal_set_TP_DUAL_IP=[0,1]
  if (AIE_VARIANT == 2 and TP_API==1) or TP_API==0:
    legal_set_TP_DUAL_IP=[0]

  param_dict={
     "name" : "TP_DUAL_IP",
     "enum" : legal_set_TP_DUAL_IP
  }
  return param_dict

def validate_TP_DUAL_IP(args):
  AIE_VARIANT= args["AIE_VARIANT"]
  TP_API     = args["TP_API"]
  TP_DUAL_IP = args["TP_DUAL_IP"]
  return fn_validate_TP_DUAL_IP(AIE_VARIANT, TP_API, TP_DUAL_IP)

def fn_validate_TP_DUAL_IP(AIE_VARIANT, TP_API, TP_DUAL_IP):
  param_dict=fn_update_TP_DUAL_IP(AIE_VARIANT, TP_API)
  return (validate_legal_set(param_dict["enum"], "TP_DUAL_IP", TP_DUAL_IP))


#######################################################
####### TP_NUM_OUTPUTS Updater and Validator ##########
#######################################################
def update_TP_NUM_OUTPUTS(args):
    AIE_VARIANT = args["AIE_VARIANT"]
    TP_API = args["TP_API"]
    return(fn_update_num_outputs(TP_API, AIE_VARIANT, "TP_NUM_OUTPUTS"))

def validate_TP_NUM_OUTPUTS(args):
    AIE_VARIANT = args["AIE_VARIANT"]
    TP_API = args["TP_API"]
    TP_NUM_OUTPUTS = args["TP_NUM_OUTPUTS"]
    return fn_validate_TP_NUM_OUTPUTS(AIE_VARIANT, TP_API, TP_NUM_OUTPUTS)

def fn_validate_TP_NUM_OUTPUTS(AIE_VARIANT, TP_API, TP_NUM_OUTPUTS):
    param_dict=fn_update_num_outputs(TP_API, AIE_VARIANT, "TP_NUM_OUTPUTS")
    return (validate_legal_set(param_dict["enum"], "TP_NUM_OUTPUTS", TP_NUM_OUTPUTS))

#######################################################
############# TP_SSR Updater and Validator ############
#######################################################
def update_TP_SSR(args):
  args, uut_kernel = poly.get_modified_args_from_polyphase_decomposer(args, current_uut_kernel)
  # if we've decomposed to another type of kernel, then import that kernel and use that update function
  if uut_kernel != current_uut_kernel:
    other_kernel = importlib.import_module(uut_kernel)
    return other_kernel.update_TP_SSR(args)
  else:
    AIE_VARIANT = args["AIE_VARIANT"]
    TT_DATA = args["TT_DATA"]
    TT_COEFF = args["TT_COEFF"]
    TP_API = args["TP_API"]
    TP_FIR_LEN = args["TP_FIR_LEN"]
    TP_USE_COEFF_RELOAD = args["TP_USE_COEFF_RELOAD"]
    TP_DECIMATE_FACTOR = args["TP_DECIMATE_FACTOR"]   
    return fn_update_TP_SSR(AIE_VARIANT, TT_DATA, TT_COEFF, TP_API, TP_FIR_LEN, TP_USE_COEFF_RELOAD, TP_DECIMATE_FACTOR)

def fn_update_TP_SSR(AIE_VARIANT, TT_DATA, TT_COEFF, TP_API, TP_FIR_LEN, TP_USE_COEFF_RELOAD, TP_DECIMATE_FACTOR):
  param_dict={"name" :  "TP_SSR"}
  if TP_API == 0:
    legal_set_TP_SSR=[1]  
    param_dict.update({"enum" : legal_set_TP_SSR})
  else:
    legal_set_TP_SSR=list(range(TP_SSR_min, TP_SSR_max+1))
    remove_list=[]
    for ssr in legal_set_TP_SSR:
      param_dict_casc_len=fn_update_TP_CASC_LEN(AIE_VARIANT, TT_DATA, TT_COEFF, TP_API, TP_FIR_LEN, TP_USE_COEFF_RELOAD, ssr, TP_DECIMATE_FACTOR)
      if "enum" in param_dict_casc_len:
        if param_dict_casc_len["enum"] == []:
          remove_list.append(ssr)
    legal_set_TP_SSR_eliminated=remove_from_set(remove_list, legal_set_TP_SSR.copy())
    if legal_set_TP_SSR_eliminated!=legal_set_TP_SSR:
      param_dict.update({"enum" : legal_set_TP_SSR_eliminated}) 
    else:
        param_dict.update({
          "minimum" : TP_SSR_min,
          "maximum" : TP_SSR_max
        })      
  return param_dict


def validate_TP_SSR(args):
  args, uut_kernel = poly.get_modified_args_from_polyphase_decomposer(args, current_uut_kernel)
  # if we've decomposed to another type of kernel, then import that kernel and use that update function
  if uut_kernel != current_uut_kernel:
    other_kernel = importlib.import_module(uut_kernel)
    return other_kernel.validate_TP_SSR(args)
  else:
    AIE_VARIANT = args["AIE_VARIANT"]
    TT_DATA = args["TT_DATA"]
    TT_COEFF = args["TT_COEFF"]
    TP_API = args["TP_API"]
    TP_FIR_LEN = args["TP_FIR_LEN"]
    TP_USE_COEFF_RELOAD = args["TP_USE_COEFF_RELOAD"]
    TP_DECIMATE_FACTOR = args["TP_DECIMATE_FACTOR"]      
    TP_SSR=args["TP_SSR"]
    return fn_validate_TP_SSR(AIE_VARIANT, TT_DATA, TT_COEFF, TP_API, TP_FIR_LEN, TP_USE_COEFF_RELOAD, TP_DECIMATE_FACTOR, TP_SSR)

def fn_validate_TP_SSR(AIE_VARIANT, TT_DATA, TT_COEFF, TP_API, TP_FIR_LEN, TP_USE_COEFF_RELOAD, TP_DECIMATE_FACTOR, TP_SSR):
  param_dict=fn_update_TP_SSR(AIE_VARIANT, TT_DATA, TT_COEFF, TP_API, TP_FIR_LEN, TP_USE_COEFF_RELOAD, TP_DECIMATE_FACTOR)
  if "enum" in param_dict:
    return (validate_legal_set(param_dict["enum"], "TP_SSR", TP_SSR))
  else:
    range_TP_SSR=[param_dict["minimum"], param_dict["maximum"]]
    return (validate_range(range_TP_SSR, "TP_SSR", TP_SSR))
  
#######################################################
########### TP_CASC_LEN Updater and Validator #########
#######################################################
def update_TP_CASC_LEN(args):
  args, uut_kernel = poly.get_modified_args_from_polyphase_decomposer(args, current_uut_kernel)
  # if we've decomposed to another type of kernel, then import that kernel and use that update function
  if uut_kernel != current_uut_kernel:
    other_kernel = importlib.import_module(uut_kernel)
    return other_kernel.update_TP_CASC_LEN(args)
  else:
    AIE_VARIANT=args["AIE_VARIANT"]
    TT_DATA=args["TT_DATA"]
    TT_COEFF=args["TT_COEFF"]
    TP_API=args["TP_API"]
    TP_FIR_LEN=args["TP_FIR_LEN"]
    TP_USE_COEFF_RELOAD=args["TP_USE_COEFF_RELOAD"]
    TP_DECIMATE_FACTOR=args["TP_DECIMATE_FACTOR"]
    TP_SSR=args["TP_SSR"]
    return fn_update_TP_CASC_LEN(AIE_VARIANT, TT_DATA, TT_COEFF, TP_API, TP_FIR_LEN, TP_USE_COEFF_RELOAD, TP_SSR, TP_DECIMATE_FACTOR)

def fn_update_TP_CASC_LEN(AIE_VARIANT, TT_DATA, TT_COEFF, TP_API, TP_FIR_LEN, TP_USE_COEFF_RELOAD, TP_SSR, TP_DECIMATE_FACTOR):
  legal_set_casc1=list(range(TP_CASC_LEN_min, TP_CASC_LEN_max+1))
  legal_set_casc2=fn_eliminate_casc_len_min_fir_len_each_kernel(legal_set_casc1.copy(), TP_FIR_LEN, TP_SSR, TP_DECIMATE_FACTOR)

  casc_len_remove_set=[]
  for casc_len in legal_set_casc2.copy():
    if AIE_VARIANT == 2:
        if (TP_FIR_LEN / casc_len) <  (TP_DECIMATE_FACTOR):
            casc_len_remove_set.append[casc_len]
  legal_set_casc3=remove_from_set(casc_len_remove_set, legal_set_casc2.copy())

  if AIE_VARIANT == 2:
      firLenPerPhaseDivider = TP_DECIMATE_FACTOR
  else:
      firLenPerPhaseDivider = 1
  legal_set_casc4=fn_eliminate_casc_len_max_fir_len_each_kernel(TT_DATA, TP_FIR_LEN, TP_USE_COEFF_RELOAD, TP_SSR, TP_API, firLenPerPhaseDivider, legal_set_casc3.copy())
  legal_set_casc5=fn_eliminate_casc_len_data_needed_within_buffer_size(TT_DATA, TT_COEFF, TP_FIR_LEN   / firLenPerPhaseDivider, TP_API, TP_SSR, TP_DECIMATE_FACTOR, legal_set_casc4.copy())
  param_dict={
      "name" :  "TP_CASC_LEN" }

  if legal_set_casc1==legal_set_casc5:
      param_dict.update({"minimum" : TP_CASC_LEN_min})
      param_dict.update({"maximum" : TP_CASC_LEN_max})
  else:
      param_dict.update({"enum" : legal_set_casc5})

  return param_dict

def validate_TP_CASC_LEN(args):   
  args, uut_kernel = poly.get_modified_args_from_polyphase_decomposer(args, current_uut_kernel)
  # if we've decomposed to another type of kernel, then import that kernel and use that update function
  if uut_kernel != current_uut_kernel:
    other_kernel = importlib.import_module(uut_kernel)
    return other_kernel.validate_TP_CASC_LEN(args)
  else:
    AIE_VARIANT=args["AIE_VARIANT"]
    TT_DATA=args["TT_DATA"]
    TT_COEFF=args["TT_COEFF"]
    TP_API=args["TP_API"]
    TP_FIR_LEN=args["TP_FIR_LEN"]
    TP_USE_COEFF_RELOAD=args["TP_USE_COEFF_RELOAD"]
    TP_DECIMATE_FACTOR=args["TP_DECIMATE_FACTOR"]
    TP_SSR=args["TP_SSR"]
    TP_CASC_LEN=args["TP_CASC_LEN"]
    return fn_validate_TP_CASC_LEN(AIE_VARIANT, TT_DATA, TT_COEFF, TP_API, TP_FIR_LEN, TP_USE_COEFF_RELOAD, TP_SSR, TP_CASC_LEN, TP_DECIMATE_FACTOR)

def fn_validate_TP_CASC_LEN(AIE_VARIANT, TT_DATA, TT_COEFF, TP_API, TP_FIR_LEN, TP_USE_COEFF_RELOAD, TP_SSR, TP_CASC_LEN, TP_DECIMATE_FACTOR):
    param_dict=fn_update_TP_CASC_LEN(AIE_VARIANT, TT_DATA, TT_COEFF, TP_API, TP_FIR_LEN, TP_USE_COEFF_RELOAD, TP_SSR, TP_DECIMATE_FACTOR)
    if "enum" in param_dict:
        return (validate_legal_set(param_dict["enum"], "TP_CASC_LEN", TP_CASC_LEN))
    else:
        range_casc_len=[param_dict["minimum"], param_dict["maximum"]]
        return(validate_range(range_casc_len, "TP_CASC_LEN", TP_CASC_LEN))
 
def fn_eliminate_casc_len_data_needed_within_buffer_size(TT_DATA, TT_COEFF, TP_FIR_LEN, TP_API, TP_SSR, TP_DECIMATE_FACTOR, legal_set_casc_len):
  legal_set_casc_len_int=legal_set_casc_len
  for casc_len in legal_set_casc_len.copy():
    check_valid=fn_data_needed_within_buffer_size(TT_DATA, TT_COEFF, TP_FIR_LEN, casc_len,TP_API, TP_SSR, TP_DECIMATE_FACTOR)
    if check_valid != isValid:
      legal_set_casc_len_int=remove_from_set([casc_len], legal_set_casc_len_int.copy())
  return legal_set_casc_len_int
  
def fn_data_needed_within_buffer_size(TT_DATA, TT_COEFF, TP_FIR_LEN, TP_CASC_LEN, TP_API, TP_SSR, TP_DECIMATE_FACTOR):
    if(TP_API == 1):
        streamReadWidthDef = sr_asym.fnStreamReadWidth(TT_DATA, TT_COEFF)
        streamReadWidth    = 256 if (TP_DECIMATE_FACTOR%2==0) else streamReadWidthDef
        m_kStreamLoadVsize = streamReadWidth / 8 / fn_size_by_byte(TT_DATA)
        m_kLanes           = fn_decimate_asym_lanes(TT_DATA, TT_COEFF)
        m_kVOutSize        = m_kLanes
        TP_MODIFY_MARGIN_OFFSET = TP_SSR - 1
        m_kSamplesInBuff     = (1024//8)//fn_size_by_byte(TT_DATA)
        fir_len_per_ssr      = CEIL(TP_FIR_LEN, (TP_SSR * TP_DECIMATE_FACTOR))//TP_SSR
        for TP_KP in range(TP_CASC_LEN):
            TP_FIR_RANGE_LEN = fnFirRangeRem(fir_len_per_ssr, TP_CASC_LEN, TP_KP, TP_DECIMATE_FACTOR) if (TP_KP == TP_CASC_LEN - 1) else fnFirRange(fir_len_per_ssr,TP_CASC_LEN, TP_KP, TP_DECIMATE_FACTOR)
            m_kFirRangeOffset = sr_asym.fnFirRangeOffset(fir_len_per_ssr, TP_CASC_LEN, TP_KP, TP_DECIMATE_FACTOR)
            emptyInitLanes    = CEIL((fir_len_per_ssr - TP_FIR_RANGE_LEN - m_kFirRangeOffset), TP_DECIMATE_FACTOR) / TP_DECIMATE_FACTOR
            streamInitNullAccs           = emptyInitLanes/ m_kVOutSize
            dataNeededLastKernel         = 1 + TP_DECIMATE_FACTOR * (m_kLanes - 1) + (streamInitNullAccs * TP_DECIMATE_FACTOR * m_kLanes)
            dataOffsetNthKernel          = (fir_len_per_ssr - TP_FIR_RANGE_LEN - m_kFirRangeOffset)
            kMinDataNeeded = (TP_MODIFY_MARGIN_OFFSET + dataNeededLastKernel - dataOffsetNthKernel)
            kMinDataLoaded      = CEIL(kMinDataNeeded, m_kStreamLoadVsize)
            kMinDataLoadCycles = kMinDataLoaded/m_kStreamLoadVsize
            m_kFirRangeOffsetLastKernel    = sr_asym.fnFirRangeOffset(fir_len_per_ssr,TP_CASC_LEN,TP_CASC_LEN-1,TP_DECIMATE_FACTOR)
            m_kInitDataNeededNoCasc    = TP_FIR_RANGE_LEN - 1 + kMinDataLoadCycles * m_kStreamLoadVsize
            m_kInitDataNeeded          = m_kInitDataNeededNoCasc + (dataOffsetNthKernel - streamInitNullAccs * TP_DECIMATE_FACTOR * m_kLanes)
            if (m_kInitDataNeeded > m_kSamplesInBuff) :
              return isError(
                    f"Requested parameters: FIR length ({TP_FIR_LEN}), cascade length ({TP_CASC_LEN}) and SSR ({TP_SSR}) result in a kernel ({TP_KP}) that requires more data samples ({m_kInitDataNeeded}) than capacity of a data buffer ({m_kSamplesInBuff}) "
                    f"Please increase the cascade length ({TP_CASC_LEN}) and/or SSR ({TP_SSR})."
              )
    return isValid

#######################################################
##### TP_INPUT_WINDOW_VSIZE Updater and Validator #####
#######################################################
def update_TP_INPUT_WINDOW_VSIZE(args):
  factor_TP_INPUT_WINDOW_VSIZE=poly.fn_factor_decomposer_TP_INPUT_WINDOW_VSIZE(args)
  args, uut_kernel = poly.get_modified_args_from_polyphase_decomposer(args, current_uut_kernel)
  # if we've decomposed to another type of kernel, then import that kernel and use that validate function
  param_dict={}
  if uut_kernel != current_uut_kernel:
    other_kernel = importlib.import_module(uut_kernel)
    param_dict.update(other_kernel.update_TP_INPUT_WINDOW_VSIZE(args))
    if args["TP_INPUT_WINDOW_VSIZE"]: TP_INPUT_WINDOW_VSIZE=args["TP_INPUT_WINDOW_VSIZE"]
    else: TP_INPUT_WINDOW_VSIZE=0
    return (fn_update_factor_TP_INPUT_VSIZE(factor_TP_INPUT_WINDOW_VSIZE, TP_INPUT_WINDOW_VSIZE, param_dict))

  else:
    AIE_VARIANT = args["AIE_VARIANT"]
    TT_DATA = args["TT_DATA"]
    TT_COEFF = args["TT_COEFF"]
    TP_FIR_LEN = args["TP_FIR_LEN"]
    TP_DECIMATE_FACTOR = args["TP_DECIMATE_FACTOR"]
    TP_API = args["TP_API"]
    TP_SSR = args["TP_SSR"]
    if args["TP_INPUT_WINDOW_VSIZE"]: TP_INPUT_WINDOW_VSIZE = args["TP_INPUT_WINDOW_VSIZE"]
    else: TP_INPUT_WINDOW_VSIZE = 0

    return(fn_update_TP_INPUT_WINDOW_VSIZE(AIE_VARIANT, TT_DATA, TT_COEFF, TP_FIR_LEN, TP_DECIMATE_FACTOR, TP_API, TP_SSR, TP_INPUT_WINDOW_VSIZE, factor_TP_INPUT_WINDOW_VSIZE))

def fn_update_TP_INPUT_WINDOW_VSIZE(AIE_VARIANT, TT_DATA, TT_COEFF, TP_FIR_LEN, TP_DECIMATE_FACTOR, TP_API, TP_SSR, TP_INPUT_WINDOW_VSIZE, factor_TP_INPUT_WINDOW_VSIZE):
  if TP_API == 0:
    TP_INPUT_WINDOW_VSIZE_max=fn_max_windowsize_for_buffer_update(TT_DATA, TP_FIR_LEN, TP_SSR, TP_INTERPOLATE_FACTOR=1, AIE_VARIANT=AIE_VARIANT)
  else: TP_INPUT_WINDOW_VSIZE_max=TP_INPUT_WINDOW_VSIZE_max_cpp

  param_dict={
    "name"    : TP_INPUT_WINDOW_VSIZE,
    "minimum" : TP_INPUT_WINDOW_VSIZE_min,
    "maximum" : TP_INPUT_WINDOW_VSIZE_max
  }

  streamRptFactor = 8
  windowSizeMultiplier = (fn_decimate_asym_lanes(TT_DATA, TT_COEFF)*TP_DECIMATE_FACTOR) if TP_API == 0 else (fn_decimate_asym_lanes(TT_DATA, TT_COEFF)*TP_DECIMATE_FACTOR*streamRptFactor)    
  numLanesMultiple = windowSizeMultiplier * TP_SSR
  factor_window_size=find_lcm_list([factor_TP_INPUT_WINDOW_VSIZE, numLanesMultiple])
  return fn_update_factor_TP_INPUT_VSIZE(factor_window_size, TP_INPUT_WINDOW_VSIZE, param_dict)
  
def fn_update_factor_TP_INPUT_VSIZE(factor_ws, TP_INPUT_WINDOW_VSIZE, param_dict):
  if TP_INPUT_WINDOW_VSIZE != 0:
    if TP_INPUT_WINDOW_VSIZE % factor_ws:
      if "actual" in param_dict:
        TP_INPUT_WINDOW_VSIZE_act=int(round(param_dict["actual"] / factor_ws) * factor_ws)
      else:
        TP_INPUT_WINDOW_VSIZE_act=int(round(TP_INPUT_WINDOW_VSIZE / factor_ws) * factor_ws)

      if TP_INPUT_WINDOW_VSIZE_act < param_dict["minimum"]:
        TP_INPUT_WINDOW_VSIZE_act = int(CEIL(param_dict["minimum"], factor_ws))

      if (TP_INPUT_WINDOW_VSIZE_act > param_dict["maximum"]):
        TP_INPUT_WINDOW_VSIZE_act = int(FLOOR(param_dict["maximum"], factor_ws))
        
      param_dict.update({"actual" : TP_INPUT_WINDOW_VSIZE_act})
  return param_dict
   
   
def validate_TP_INPUT_WINDOW_VSIZE(args):
  factor_TP_INPUT_WINDOW_VSIZE=poly.fn_factor_decomposer_TP_INPUT_WINDOW_VSIZE(args)
  args, uut_kernel = poly.get_modified_args_from_polyphase_decomposer(args, current_uut_kernel)
  if uut_kernel != current_uut_kernel:
    other_kernel = importlib.import_module(uut_kernel)
    check_uut_kernel=other_kernel.validate_TP_INPUT_WINDOW_VSIZE(args)
    if check_uut_kernel==isValid:
      TP_INPUT_WINDOW_VSIZE = args["TP_INPUT_WINDOW_VSIZE"]
      return (fn_validate_factor_TP_INPUT_VSIZE(factor_TP_INPUT_WINDOW_VSIZE, TP_INPUT_WINDOW_VSIZE))
    else:
      return check_uut_kernel

  else:
    AIE_VARIANT = args["AIE_VARIANT"]
    TT_DATA = args["TT_DATA"]
    TT_COEFF = args["TT_COEFF"]
    TP_FIR_LEN = args["TP_FIR_LEN"]
    TP_DECIMATE_FACTOR = args["TP_DECIMATE_FACTOR"]
    TP_API = args["TP_API"]
    TP_SSR = args["TP_SSR"]
    if args["TP_INPUT_WINDOW_VSIZE"]: TP_INPUT_WINDOW_VSIZE = args["TP_INPUT_WINDOW_VSIZE"]
    else: TP_INPUT_WINDOW_VSIZE = 0

    return(fn_validate_TP_INPUT_WINDOW_VSIZE(AIE_VARIANT, TT_DATA, TT_COEFF, TP_FIR_LEN, TP_DECIMATE_FACTOR, TP_API, TP_SSR, TP_INPUT_WINDOW_VSIZE, factor_TP_INPUT_WINDOW_VSIZE))

def fn_validate_TP_INPUT_WINDOW_VSIZE(AIE_VARIANT, TT_DATA, TT_COEFF, TP_FIR_LEN, TP_DECIMATE_FACTOR, TP_API, TP_SSR, TP_INPUT_WINDOW_VSIZE, factor_TP_INPUT_WINDOW_VSIZE):
  streamRptFactor = 8
  windowSizeMultiplier = (fn_decimate_asym_lanes(TT_DATA, TT_COEFF)*TP_DECIMATE_FACTOR) if TP_API == 0 else (fn_decimate_asym_lanes(TT_DATA, TT_COEFF)*TP_DECIMATE_FACTOR*streamRptFactor)    
  numLanesMultiple = windowSizeMultiplier * TP_SSR
  factor_window_size=find_lcm_list([factor_TP_INPUT_WINDOW_VSIZE, numLanesMultiple])
  check_factor=fn_validate_factor_TP_INPUT_VSIZE(factor_window_size, TP_INPUT_WINDOW_VSIZE)
  if check_factor==isValid:
    param_dict=fn_update_TP_INPUT_WINDOW_VSIZE(AIE_VARIANT, TT_DATA, TT_COEFF, TP_FIR_LEN, TP_DECIMATE_FACTOR, TP_API, TP_SSR, TP_INPUT_WINDOW_VSIZE, factor_window_size)

    range_TP_INPUT_WINDOW_VSIZE=[param_dict["minimum"], param_dict["maximum"]]
    return (validate_range(range_TP_INPUT_WINDOW_VSIZE, "TP_INPUT_WINDOW_VSIZE", TP_INPUT_WINDOW_VSIZE))
  else:
    return check_factor

def fn_validate_factor_TP_INPUT_VSIZE(factor_ws, TP_INPUT_WINDOW_VSIZE):
  if TP_INPUT_WINDOW_VSIZE % factor_ws != 0:
    return isError(f"TP_INPUT_WINDOW_VSIZE should be a multiple of {factor_ws}")
  else: 
    return isValid

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
    TP_DECIMATE_FACTOR = args["TP_DECIMATE_FACTOR"]
    TP_PARA_DECI_POLY = args["TP_PARA_DECI_POLY"]
    TP_API = args["TP_API"]
    TP_DUAL_IP = args["TP_DUAL_IP"]
    TP_NUM_OUTPUTS = args["TP_NUM_OUTPUTS"]
    TP_INTERPOLATE_FACTOR = 1

    margin_size = sr_asym.fn_margin_size(TP_FIR_LEN, TT_DATA)
    num_in_ports = TP_SSR * TP_PARA_DECI_POLY
    num_out_ports = TP_SSR
    in_win_size = get_input_window_size(TP_INPUT_WINDOW_VSIZE, num_in_ports, TP_API, TP_DUAL_IP)
    out_win_size = get_output_window_size(TP_INPUT_WINDOW_VSIZE, num_out_ports, TP_API, TP_NUM_OUTPUTS, TP_DECIMATE_FACTOR, TP_INTERPOLATE_FACTOR)

    in_ports = get_port_info("in", "in", TT_DATA, in_win_size, num_in_ports, marginSize=margin_size, TP_API=TP_API)
    in2_ports = (get_port_info("in2", "in", TT_DATA, in_win_size, num_in_ports, marginSize=margin_size, TP_API=TP_API) if (TP_DUAL_IP == 1) else [])
    coeff_ports = (get_parameter_port_info("coeff", "in", TT_COEFF, TP_SSR, TP_FIR_LEN, "async") if (args["TP_USE_COEFF_RELOAD"] == 1) else [])

    # decimate by 2 for halfband
    out_ports = get_port_info("out", "out", TT_DATA, out_win_size, TP_SSR, TP_API=args["TP_API"])
    out2_ports = (get_port_info("out2", "out", TT_DATA, out_win_size, TP_SSR, TP_API=args["TP_API"]) if (args["TP_NUM_OUTPUTS"] == 2) else [])
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
  TP_DECIMATE_FACTOR = args["TP_DECIMATE_FACTOR"]
  TP_SHIFT = args["TP_SHIFT"]
  TP_RND = args["TP_RND"]
  TP_CASC_LEN = args["TP_CASC_LEN"]
  TP_INPUT_WINDOW_VSIZE = args["TP_INPUT_WINDOW_VSIZE"]
  TP_USE_COEFF_RELOAD = args["TP_USE_COEFF_RELOAD"]
  TP_NUM_OUTPUTS = args["TP_NUM_OUTPUTS"]
  TP_DUAL_IP = args["TP_DUAL_IP"]
  TP_API = args["TP_API"]
  TP_SSR = args["TP_SSR"]
  TP_PARA_DECI_POLY = args["TP_PARA_DECI_POLY"]
  coeff_list = args["coeff"]
  TP_SAT = args["TP_SAT"]

  taps = sr_asym.fn_get_taps_vector(TT_COEFF, coeff_list)
  constr_args_str = f"taps" if TP_USE_COEFF_RELOAD == 0 else ""
  dual_ip_declare_str = f"std::array<adf::port<input>, TP_SSR*TP_PARA_DECI_POLY> in2;" if TP_DUAL_IP == 1 else "// No dual input"
  dual_ip_connect_str = f"adf::connect<> net_in2(in2[inPortIdx], filter.in2[inPortIdx]);" if TP_DUAL_IP == 1 else "// No dual input"
  coeff_ip_declare_str = f"ssr_port_array<input> coeff;" if TP_USE_COEFF_RELOAD == 1 else "//No coeff port"
  coeff_ip_connect_str = f"adf::connect<> net_coeff(coeff[outPortIdx], filter.coeff[outPortIdx]);" if TP_USE_COEFF_RELOAD == 1 else "//No coeff port"
  dual_op_declare_str = f"ssr_port_array<output> out2;" if TP_NUM_OUTPUTS == 2 else "// No dual output"
  dual_op_connect_str = f"adf::connect<> net_out2(filter.out2[outPortIdx], out2[outPortIdx]);" if TP_NUM_OUTPUTS == 2 else "// No dual output"
  # Use formatted multi-line string to avoid a lot of \n and \t
  code  = (
f"""
class {graphname} : public adf::graph {{
public:
  static constexpr unsigned int TP_SSR = {TP_SSR};
  static constexpr unsigned int TP_PARA_DECI_POLY = {TP_PARA_DECI_POLY};
  template <typename dir>
  using ssr_port_array = std::array<adf::port<dir>, TP_SSR>;

  std::array<adf::port<input>, TP_SSR*TP_PARA_DECI_POLY> in;
  {dual_ip_declare_str}
  {coeff_ip_declare_str}
  ssr_port_array<output> out;
  {dual_op_declare_str}

  std::vector<{TT_COEFF}> taps = {taps};
  xf::dsp::aie::fir::decimate_asym::fir_decimate_asym_graph<
    {TT_DATA}, //TT_DATA
    {TT_COEFF}, //TT_COEFF
    {TP_FIR_LEN}, //TP_FIR_LEN
    {TP_DECIMATE_FACTOR}, //TP_DECIMATE_FACTOR
    {TP_SHIFT}, //TP_SHIFT
    {TP_RND}, //TP_RND
    {TP_INPUT_WINDOW_VSIZE}, //TP_INPUT_WINDOW_VSIZE
    {TP_CASC_LEN}, //TP_CASC_LEN
    {TP_USE_COEFF_RELOAD}, //TP_USE_COEFF_RELOAD
    {TP_NUM_OUTPUTS}, //TP_NUM_OUTPUTS
    {TP_DUAL_IP}, //TP_DUAL_IP
    {TP_API}, //TP_API
    {TP_SSR}, //TP_SSR
    {TP_PARA_DECI_POLY}, //TP_PARA_DECI_POLY
    {TP_SAT} //TP_SAT
  > filter;

  {graphname}() : filter({constr_args_str}) {{
    adf::kernel *filter_kernels = filter.getKernels();
    for (int i=0; i < 1; i++) {{
      adf::runtime<ratio>(filter_kernels[i]) = 0.9;
    }}
    for (int paraPolyIdx=0; paraPolyIdx < TP_PARA_DECI_POLY; paraPolyIdx++) {{
      for (int ssrIdx=0; ssrIdx < TP_SSR; ssrIdx++) {{
        unsigned inPortIdx = paraPolyIdx + ssrIdx*TP_PARA_DECI_POLY;
        adf::connect<> net_in(in[inPortIdx], filter.in[inPortIdx]);
        {dual_ip_connect_str}
      }}
    }}
    for (int ssrIdx=0; ssrIdx < TP_SSR; ssrIdx++) {{
      unsigned outPortIdx = ssrIdx;
      {coeff_ip_connect_str}
      adf::connect<> net_out(filter.out[outPortIdx], out[outPortIdx]);
      {dual_op_connect_str}
    }}
  }}

}};
""")
  out = {}
  out["graph"] = code
  out["port_info"] = info_ports(args)
  out["headerfile"] = "fir_decimate_asym_graph.hpp"
  out["searchpaths"] = [
       "L2/include/aie",
       "L2/tests/aie/common/inc",
       "L1/include/aie",
       "L1/src/aie",
       "L1/tests/aie/inc",
       "L1/tests/aie/src"
  ]

  return out
