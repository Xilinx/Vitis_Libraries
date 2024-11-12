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

TP_INPUT_WINDOW_VSIZE_min = 4
TP_INPUT_WINDOW_VSIZE_max_cpp = 2**31
TP_SSR_min = 1
TP_SSR_max = 16
TP_PARA_INTERP_POLY_min = 1
TP_CASC_LEN_min = 1
TP_CASC_LEN_max = 40
TP_FIR_LEN_min = 4
TP_FIR_LEN_max = 8192

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
############# TT_DATA Updater and Validator ###########
#######################################################
def update_TT_DATA(args):
  AIE_VARIANT=args["AIE_VARIANT"]
  return fn_update_TT_DATA(AIE_VARIANT)

def fn_update_TT_DATA(AIE_VARIANT):
  legal_set_TT_DATA = [        
        "int16",
        "cint16",
        "int32",
        "cint32",
        "float",
        "cfloat"]
  
  if AIE_VARIANT==2:
      legal_set_TT_DATA=remove_from_set(["cfloat"], legal_set_TT_DATA)
  param_dict ={"name" : "TT_DATA",
               "enum" : legal_set_TT_DATA}
  return param_dict

def validate_TT_DATA(args):
  AIE_VARIANT=args["AIE_VARIANT"]
  TT_DATA=args["TT_DATA"]
  return (fn_validate_TT_DATA(AIE_VARIANT, TT_DATA))

def fn_validate_TT_DATA(AIE_VARIANT, TT_DATA):
  param_dict = fn_update_TT_DATA(AIE_VARIANT)
  legal_set_TT_DATA = param_dict["enum"]
  return(validate_legal_set(legal_set_TT_DATA, "TT_DATA", TT_DATA))

#######################################################
############# TT_COEFF Updater and Validator ##########
#######################################################
def update_TT_COEFF(args):
  AIE_VARIANT = args["AIE_VARIANT"]
  TT_DATA = args["TT_DATA"]
  return fn_update_TT_COEFF(AIE_VARIANT, TT_DATA)

def fn_update_TT_COEFF(AIE_VARIANT, TT_DATA):
  legal_set_TT_COEFF = [        
      "int16",
      "cint16",
      "int32",
      "cint32",
      "float",
      "cfloat"]
  legal_set_TT_COEFF=fn_coeff_type_update(TT_DATA, legal_set_TT_COEFF)
  legal_set_TT_COEFF=fn_type_aieml_support_update(AIE_VARIANT, TT_DATA, legal_set_TT_COEFF)
  param_dict ={"name" : "TT_COEFF",
                "enum" : legal_set_TT_COEFF}
  
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
############ TP_API Updater and Validator #############
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
  if args["TP_FIR_LEN"]: TP_FIR_LEN = args["TP_FIR_LEN"]
  else: TP_FIR_LEN = 0
  return fn_update_TP_FIR_LEN(TT_DATA, TP_API, TP_USE_COEFF_RELOAD, TP_FIR_LEN)

def fn_update_TP_FIR_LEN(TT_DATA, TP_API, TP_USE_COEFF_RELOAD, TP_FIR_LEN):
  symFactor   = 4 # Symmetric, half-band
  TP_FIR_LEN_min_int=int(CEIL((TP_FIR_LEN_min+1),4) - 1)
  TP_FIR_LEN_max_int1=fn_max_fir_len_each_kernel_update(TT_DATA, TP_CASC_LEN_max, TP_USE_COEFF_RELOAD, TP_SSR_max, TP_API, symFactor)
  TP_FIR_LEN_max_int2=min(TP_FIR_LEN_max_int1, TP_FIR_LEN_max)
  TP_FIR_LEN_max_int=int(FLOOR((TP_FIR_LEN_max_int2+1),4) - 1)

  param_dict={
      "name" : "TP_FIR_LEN",
      "minimum" : TP_FIR_LEN_min_int,
      "maximum" : TP_FIR_LEN_max_int
  }

  if TP_FIR_LEN !=0:
    if (TP_FIR_LEN+1) % 4 != 0:
      TP_FIR_LEN_act=(round((TP_FIR_LEN+1)/4)*4) - 1

      if TP_FIR_LEN_act < param_dict["minimum"]:
        TP_FIR_LEN_act = param_dict["minimum"]

      if TP_FIR_LEN_act > param_dict["maximum"]:
        TP_FIR_LEN_act = param_dict["maximum"]
      param_dict.update({"actual" : int(TP_FIR_LEN_act)})

  return param_dict

def validate_TP_FIR_LEN(args):
  TT_DATA = args["TT_DATA"]
  TP_API = args["TP_API"]
  TP_USE_COEFF_RELOAD = args["TP_USE_COEFF_RELOAD"]
  TP_FIR_LEN = args["TP_FIR_LEN"]
  return fn_validate_TP_FIR_LEN(TT_DATA, TP_API, TP_USE_COEFF_RELOAD, TP_FIR_LEN)

def fn_validate_TP_FIR_LEN(TT_DATA, TP_API, TP_USE_COEFF_RELOAD, TP_FIR_LEN):
  check_FIR_LEN= fn_halfband_len(TP_FIR_LEN)
  if check_FIR_LEN == isValid:
    param_dict=fn_update_TP_FIR_LEN(TT_DATA, TP_API, TP_USE_COEFF_RELOAD, TP_FIR_LEN)
    range_TP_FIR_LEN=[param_dict["minimum"], param_dict["maximum"]]
    return validate_range(range_TP_FIR_LEN, "TP_FIR_LEN", TP_FIR_LEN)
  else:
    return check_FIR_LEN

def fn_halfband_len(TP_FIR_LEN):
  return isValid if ((TP_FIR_LEN + 1) % 4 == 0) else isError(f"Filter length must be 4N-1 where N is a positive integer. Got TP_FIR_LEN {TP_FIR_LEN}")

#######################################################
######## TP_PARA_INTERP_POLY Updater and Validator ####
#######################################################
def update_TP_PARA_INTERP_POLY(args):
  return fn_update_TP_PARA_INTERP_POLY()

def fn_update_TP_PARA_INTERP_POLY():
  legal_set_TP_PARA_INTERP_POLY=[1,2]

  param_dict={
     "name" : "TP_PARA_INTERP_POLY",
     "enum" : legal_set_TP_PARA_INTERP_POLY
  }
  return param_dict

def validate_TP_PARA_INTERP_POLY(args):
  TP_PARA_INTERP_POLY   = args["TP_PARA_INTERP_POLY"]
  return fn_validate_TP_PARA_INTERP_POLY(TP_PARA_INTERP_POLY)

def fn_validate_TP_PARA_INTERP_POLY(TP_PARA_INTERP_POLY):
  param_dict=fn_update_TP_PARA_INTERP_POLY()
  return validate_legal_set(param_dict["enum"], "TP_PARA_INTERP_POLY", TP_PARA_INTERP_POLY)

#######################################################
############# TP_SSR Updater and Validator ############
#######################################################

def update_TP_SSR(args):
  AIE_VARIANT=args["AIE_VARIANT"]
  TT_DATA=args["TT_DATA"]
  TT_COEFF=args["TT_COEFF"]
  TP_API=args["TP_API"]
  TP_FIR_LEN=args["TP_FIR_LEN"]
  TP_USE_COEFF_RELOAD=args["TP_USE_COEFF_RELOAD"]
  TP_PARA_INTERP_POLY=args["TP_PARA_INTERP_POLY"]
  return fn_update_TP_SSR(AIE_VARIANT, TT_DATA, TT_COEFF, TP_API, TP_FIR_LEN, TP_USE_COEFF_RELOAD, TP_PARA_INTERP_POLY)

def fn_update_TP_SSR(AIE_VARIANT, TT_DATA, TT_COEFF, TP_API, TP_FIR_LEN, TP_USE_COEFF_RELOAD, TP_PARA_INTERP_POLY):
  param_dict={
      "name" :  "TP_SSR"}

  if (TP_PARA_INTERP_POLY == 1):
    legal_set_TP_SSR=[1]
    param_dict.update({"enum" : legal_set_TP_SSR}) 
  else:
    legal_set_TP_SSR=list(range(TP_SSR_min, TP_SSR_max+1))
    remove_list=[]
    for ssr in legal_set_TP_SSR:
      param_dict_casc_len=fn_update_TP_CASC_LEN(AIE_VARIANT, TT_DATA, TT_COEFF, TP_API, TP_FIR_LEN, TP_USE_COEFF_RELOAD, TP_PARA_INTERP_POLY, ssr)
      if "enum" in param_dict_casc_len:
        if param_dict_casc_len["enum"] == []:
          remove_list.append(ssr)
    legal_set_TP_SSR_eliminated=remove_from_set(remove_list, legal_set_TP_SSR.copy())
    if legal_set_TP_SSR_eliminated!=legal_set_TP_SSR:
      param_dict.update({"enum" : legal_set_TP_SSR_eliminated}) 
    else:
        param_dict.update({
          "minimum" : 1,
          "maximum" : TP_SSR_max
        })      
  return param_dict



def validate_TP_SSR(args):
  AIE_VARIANT=args["AIE_VARIANT"]
  TT_DATA=args["TT_DATA"]
  TT_COEFF=args["TT_COEFF"]
  TP_API=args["TP_API"]
  TP_FIR_LEN=args["TP_FIR_LEN"]
  TP_USE_COEFF_RELOAD=args["TP_USE_COEFF_RELOAD"]
  TP_PARA_INTERP_POLY=args["TP_PARA_INTERP_POLY"]  
  TP_SSR=args["TP_SSR"]
  return fn_validate_TP_SSR(AIE_VARIANT, TT_DATA, TT_COEFF, TP_API, TP_FIR_LEN, TP_USE_COEFF_RELOAD, TP_PARA_INTERP_POLY, TP_SSR)

def fn_validate_TP_SSR(AIE_VARIANT, TT_DATA, TT_COEFF, TP_API, TP_FIR_LEN, TP_USE_COEFF_RELOAD, TP_PARA_INTERP_POLY, TP_SSR):
  param_dict=fn_update_TP_SSR(AIE_VARIANT, TT_DATA, TT_COEFF, TP_API, TP_FIR_LEN, TP_USE_COEFF_RELOAD, TP_PARA_INTERP_POLY)
  if "enum" in param_dict:
    return (validate_legal_set(param_dict["enum"], "TP_SSR", TP_SSR))
  else:
    range_TP_SSR=[param_dict["minimum"], param_dict["maximum"]]
    return (validate_range(range_TP_SSR, "TP_SSR", TP_SSR))
  
#######################################################
########### TP_CASC_LEN Updater and Validator #########
#######################################################
def update_TP_CASC_LEN(args):
  AIE_VARIANT=args["AIE_VARIANT"]
  TT_DATA=args["TT_DATA"]
  TT_COEFF=args["TT_COEFF"]
  TP_API=args["TP_API"]
  TP_FIR_LEN=args["TP_FIR_LEN"]
  TP_USE_COEFF_RELOAD=args["TP_USE_COEFF_RELOAD"]
  TP_PARA_INTERP_POLY=args["TP_PARA_INTERP_POLY"]
  TP_SSR=args["TP_SSR"]
  return fn_update_TP_CASC_LEN(AIE_VARIANT, TT_DATA, TT_COEFF, TP_API, TP_FIR_LEN, TP_USE_COEFF_RELOAD, TP_PARA_INTERP_POLY, TP_SSR)

def fn_update_TP_CASC_LEN(AIE_VARIANT, TT_DATA, TT_COEFF, TP_API, TP_FIR_LEN, TP_USE_COEFF_RELOAD, TP_PARA_INTERP_POLY, TP_SSR):
  legal_set_casc1=list(range(TP_CASC_LEN_min, TP_CASC_LEN_max+1))
  legal_set_casc2=fn_eliminate_casc_len_min_fir_len_each_kernel(legal_set_casc1.copy(), TP_FIR_LEN, TP_SSR)

  symFactor   = 4 # Symmetric, half-band
  symFactorSSR= 2 if (TP_SSR != 1 ) else symFactor # TP_SSR mode will discard the symmetry
  symApiSSR   = 0 if (TP_SSR == 1 and TP_PARA_INTERP_POLY == 1) else TP_API  # Force buffer checks when not in TP_SSR mode.

  legal_set_casc3=fn_eliminate_casc_len_max_fir_len_each_kernel(TT_DATA, TP_FIR_LEN, TP_USE_COEFF_RELOAD, TP_SSR, symApiSSR, symFactorSSR, legal_set_casc2.copy())

  if TP_PARA_INTERP_POLY > 1:
    legal_set_casc3=sr_asym.fn_eliminate_casc_len_data_needed_within_buffer_size(TT_DATA, TT_COEFF, (TP_FIR_LEN + 1)/2, TP_API, AIE_VARIANT, TP_SSR, legal_set_casc3.copy())

  if AIE_VARIANT == 2 and TP_API == 1:
    legal_set_casc3=fn_eliminate_casc_len_data_needed_within_buffer_size_ml(TT_DATA, TP_FIR_LEN, TP_SSR, legal_set_casc3.copy())

  param_dict={
      "name" :  "TP_CASC_LEN" }

  if legal_set_casc1==legal_set_casc3:
      param_dict.update({"minimum" : TP_CASC_LEN_min})
      param_dict.update({"maximum" : TP_CASC_LEN_max})
  else:
      param_dict.update({"enum" : legal_set_casc3})

  return param_dict

def validate_TP_CASC_LEN(args):
    AIE_VARIANT=args["AIE_VARIANT"]
    TT_DATA=args["TT_DATA"]
    TT_COEFF=args["TT_COEFF"]
    TP_API=args["TP_API"]
    TP_FIR_LEN=args["TP_FIR_LEN"]
    TP_USE_COEFF_RELOAD=args["TP_USE_COEFF_RELOAD"]
    TP_PARA_INTERP_POLY=args["TP_PARA_INTERP_POLY"]
    TP_SSR=args["TP_SSR"]
    TP_CASC_LEN=args["TP_CASC_LEN"]
    return fn_validate_TP_CASC_LEN(AIE_VARIANT, TT_DATA, TT_COEFF, TP_API, TP_FIR_LEN, TP_USE_COEFF_RELOAD, TP_PARA_INTERP_POLY, TP_SSR, TP_CASC_LEN)

def fn_validate_TP_CASC_LEN(AIE_VARIANT, TT_DATA, TT_COEFF, TP_API, TP_FIR_LEN, TP_USE_COEFF_RELOAD, TP_PARA_INTERP_POLY, TP_SSR, TP_CASC_LEN):
    param_dict=fn_update_TP_CASC_LEN(AIE_VARIANT, TT_DATA, TT_COEFF, TP_API, TP_FIR_LEN, TP_USE_COEFF_RELOAD, TP_PARA_INTERP_POLY, TP_SSR)
    if "enum" in param_dict:
        return (validate_legal_set(param_dict["enum"], "TP_CASC_LEN", TP_CASC_LEN))
    else:
        range_casc_len=[param_dict["minimum"], param_dict["maximum"]]
        return(validate_range(range_casc_len, "TP_CASC_LEN", TP_CASC_LEN))

def fn_eliminate_casc_len_data_needed_within_buffer_size_ml(TT_DATA, TP_FIR_LEN, TP_SSR, legal_set_casc_len):
  legal_set_casc_len_int=legal_set_casc_len
  firLenPerSSR = TP_FIR_LEN / TP_SSR
  kInterpolateFactor = 2
  for casc_len in legal_set_casc_len.copy():
    for TP_KP in range(casc_len):
        m_kFirRangeLen = fnFirRangeRem(firLenPerSSR, casc_len, TP_KP, 2) if (TP_KP + 1 == casc_len) else fnFirRange(firLenPerSSR, casc_len, TP_KP, 2)
        m_kNumTaps = ((m_kFirRangeLen + 1) / 2) if (TP_KP == casc_len - 1) else (m_kFirRangeLen / 2)
        m_kWinAccessByteSize = 32
        m_kFirRangeOffset = sr_asym.fnFirRangeOffset(firLenPerSSR, casc_len, TP_KP, kInterpolateFactor) / kInterpolateFactor
        m_kFirAsymTaps = (firLenPerSSR + 1) / kInterpolateFactor
        m_kFirMarginOffset = sr_asym.fnFirMargin(m_kFirAsymTaps, TT_DATA) - (m_kFirAsymTaps) + 1
        m_kFirInitOffset = m_kFirMarginOffset + m_kFirRangeOffset
        m_kDataBuffXOffset = m_kFirInitOffset % (m_kWinAccessByteSize / fn_size_by_byte(TT_DATA))
        m_kArchFirLen = m_kNumTaps + m_kDataBuffXOffset
        m_kSamplesInBuff = 1024 / 8 / fn_size_by_byte(TT_DATA)
        m_kDataLoadVsize = 256 / 8 / fn_size_by_byte(TT_DATA)
        m_kTotDataNeededPerOp = m_kArchFirLen + m_kDataLoadVsize - 1
        if m_kTotDataNeededPerOp > m_kSamplesInBuff:
          legal_set_casc_len_int=remove_from_set([casc_len], legal_set_casc_len_int.copy())
    return legal_set_casc_len_int

#######################################################
############# TP_DUAL_IP Updater and Validator ########
#######################################################
def update_TP_DUAL_IP(args):
  AIE_VARIANT   = args["AIE_VARIANT"]
  return fn_update_TP_DUAL_IP(AIE_VARIANT)

def fn_update_TP_DUAL_IP(AIE_VARIANT):
  legal_set_TP_DUAL_IP=[0,1]
  if AIE_VARIANT == 2:
    legal_set_TP_DUAL_IP=[0]

  param_dict={
     "name" : "TP_DUAL_IP",
     "enum" : legal_set_TP_DUAL_IP
  }
  return param_dict

def validate_TP_DUAL_IP(args):
  AIE_VARIANT= args["AIE_VARIANT"]
  TP_DUAL_IP = args["TP_DUAL_IP"]
  return fn_validate_TP_DUAL_IP(AIE_VARIANT, TP_DUAL_IP)

def fn_validate_TP_DUAL_IP(AIE_VARIANT, TP_DUAL_IP):
  param_dict=fn_update_TP_DUAL_IP(AIE_VARIANT)
  return (validate_legal_set(param_dict["enum"], "TP_DUAL_IP", TP_DUAL_IP))

#######################################################
############# TP_NUM_OUTPUTS Updater and Validator ####
#######################################################
def update_TP_NUM_OUTPUTS(args):
  TP_PARA_INTERP_POLY = args["TP_PARA_INTERP_POLY"]
  TP_DUAL_IP = args["TP_DUAL_IP"]
  return(fn_update_hb_num_outputs(TP_PARA_INTERP_POLY, TP_DUAL_IP, "TP_NUM_OUTPUTS"))

def validate_TP_NUM_OUTPUTS(args):
  TP_PARA_INTERP_POLY = args["TP_PARA_INTERP_POLY"]
  TP_DUAL_IP = args["TP_DUAL_IP"]
  TP_NUM_OUTPUTS = args["TP_NUM_OUTPUTS"]
  return fn_validate_TP_NUM_OUTPUTS(TP_PARA_INTERP_POLY, TP_DUAL_IP, TP_NUM_OUTPUTS)

def fn_validate_TP_NUM_OUTPUTS(TP_PARA_INTERP_POLY, TP_DUAL_IP, TP_NUM_OUTPUTS):
  param_dict=fn_update_hb_num_outputs(TP_PARA_INTERP_POLY, TP_DUAL_IP, "TP_NUM_OUTPUTS")
  return (validate_legal_set(param_dict["enum"], "TP_NUM_OUTPUTS", TP_NUM_OUTPUTS))

#######################################################
##### TP_INPUT_WINDOW_VSIZE Updater and Validator #####
#######################################################
def update_TP_INPUT_WINDOW_VSIZE(args):
  TT_DATA = args["TT_DATA"]
  TT_COEFF = args["TT_COEFF"]
  TP_FIR_LEN = args["TP_FIR_LEN"]
  TP_API = args["TP_API"]
  TP_SSR = args["TP_SSR"]
  if args["TP_INPUT_WINDOW_VSIZE"] : TP_INPUT_WINDOW_VSIZE = args["TP_INPUT_WINDOW_VSIZE"]
  else: TP_INPUT_WINDOW_VSIZE = 0

  return fn_update_input_window_size(TT_DATA, TT_COEFF, TP_FIR_LEN, TP_INPUT_WINDOW_VSIZE, TP_API, TP_SSR)

def fn_update_input_window_size(TT_DATA, TT_COEFF, TP_FIR_LEN, TP_INPUT_WINDOW_VSIZE, TP_API, TP_SSR):

  numLanes=fnNumLanes384b(TT_DATA, TT_COEFF)*2
  lcm_ws=TP_SSR * numLanes
  TP_INPUT_WINDOW_VSIZE_min_int=int(CEIL(TP_INPUT_WINDOW_VSIZE_min, lcm_ws))

  if TP_API==0:
    TP_INPUT_WINDOW_VSIZE_max_int=fn_max_windowsize_for_buffer_update(TT_DATA, TP_FIR_LEN, TP_SSR, TP_INTERPOLATE_FACTOR=2, TP_DECIMATE_FACTOR=1)
    TP_INPUT_WINDOW_VSIZE_max_int=int(FLOOR(TP_INPUT_WINDOW_VSIZE_max_int, lcm_ws))
  else:
    TP_INPUT_WINDOW_VSIZE_max_int=TP_INPUT_WINDOW_VSIZE_max_cpp

  param_dict={
      "name" : "TP_INPUT_WINDOW_VSIZE",
      "minimum" : TP_INPUT_WINDOW_VSIZE_min_int,
      "maximum" : TP_INPUT_WINDOW_VSIZE_max_int
  }  

  if TP_INPUT_WINDOW_VSIZE !=0:
    if TP_INPUT_WINDOW_VSIZE%lcm_ws != 0:
        TP_INPUT_WINDOW_VSIZE_act=round(TP_INPUT_WINDOW_VSIZE/lcm_ws) * lcm_ws

        if TP_INPUT_WINDOW_VSIZE_act < param_dict["minimum"]:
            TP_INPUT_WINDOW_VSIZE_act = param_dict["minimum"]

        if (TP_INPUT_WINDOW_VSIZE_act > param_dict["maximum"]):
            TP_INPUT_WINDOW_VSIZE_act = int(FLOOR(param_dict["maximum"], lcm_ws))
        param_dict.update({"actual" : int(TP_INPUT_WINDOW_VSIZE_act)})

  return param_dict

def validate_TP_INPUT_WINDOW_VSIZE(args):
  TT_DATA = args["TT_DATA"]
  TT_COEFF = args["TT_COEFF"]
  TP_FIR_LEN = args["TP_FIR_LEN"]
  TP_API = args["TP_API"]
  TP_SSR = args["TP_SSR"]
  TP_INPUT_WINDOW_VSIZE = args["TP_INPUT_WINDOW_VSIZE"]
  return fn_validate_input_window_size(TT_DATA, TT_COEFF, TP_FIR_LEN, TP_INPUT_WINDOW_VSIZE, TP_API, TP_SSR)

def fn_validate_input_window_size(TT_DATA, TT_COEFF, TP_FIR_LEN, TP_INPUT_WINDOW_VSIZE, TP_API, TP_SSR):
  numLanes=fnNumLanes384b(TT_DATA, TT_COEFF)*2
  lcm_ws=TP_SSR*numLanes
  if (TP_INPUT_WINDOW_VSIZE%lcm_ws !=0):
    return isError(f"TP_INPUT_WINDOW_VSIZE should be a multiple of {lcm_ws}!")
  else:
    param_dict=fn_update_input_window_size(TT_DATA, TT_COEFF, TP_FIR_LEN, TP_INPUT_WINDOW_VSIZE, TP_API, TP_SSR)
    range_TP_INPUT_WINDOW_VSIZE=[param_dict["minimum"], param_dict["maximum"]]
    return(validate_range(range_TP_INPUT_WINDOW_VSIZE, "TP_INPUT_WINDOW_VSIZE", TP_INPUT_WINDOW_VSIZE))  

#######################################################
######### TP_UPSHIFT_CT Updater and Validator #########
#######################################################
def update_TP_UPSHIFT_CT(args):
  TT_DATA = args["TT_DATA"]
  AIE_VARIANT = args["AIE_VARIANT"]
  return fn_update_upshift_ct(TT_DATA, AIE_VARIANT)

def fn_update_upshift_ct(TT_DATA, AIE_VARIANT):
  legal_set_TP_UPSHIFT_CT=[0,1]
  if AIE_VARIANT == 2 : legal_set_TP_UPSHIFT_CT=[0]
  if TT_DATA not in ["cint16", "int16"] : legal_set_TP_UPSHIFT_CT=[0]
  param_dict={
      "name" : "TP_UPSHIFT_CT",
      "enum" : legal_set_TP_UPSHIFT_CT
  }  
  return param_dict

def validate_TP_UPSHIFT_CT(args):
  TT_DATA = args["TT_DATA"]
  TP_UPSHIFT_CT = args["TP_UPSHIFT_CT"]
  AIE_VARIANT = args["AIE_VARIANT"]
  return fn_validate_upshift_ct(TT_DATA, TP_UPSHIFT_CT, AIE_VARIANT)


def fn_validate_upshift_ct(TT_DATA, TP_UPSHIFT_CT, AIE_VARIANT):
  param_dict=fn_update_upshift_ct(TT_DATA, AIE_VARIANT)
  return (validate_legal_set(param_dict["enum"], "TP_UPSHIFT_CT", TP_UPSHIFT_CT))

#######################################################
############# TP_SHIFT Updater and Validator ##########
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
  
  if fn_is_complex(TT_COEFF) : len_coeff=int((TP_FIR_LEN + 1) / 2)
  else: len_coeff=int((TP_FIR_LEN + 1) / 4)

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
    TP_API = args["TP_API"]
    TP_NUM_OUTPUTS = args["TP_NUM_OUTPUTS"]
    TP_PARA_INTERP_POLY = args["TP_PARA_INTERP_POLY"]
    TP_DUAL_IP = args["TP_DUAL_IP"]
    TP_USE_COEFF_RELOAD = args["TP_USE_COEFF_RELOAD"]
    AIE_VARIANT = args["AIE_VARIANT"]
    TP_DECIMATE_FACTOR = 1
    TP_INTERPOLATE_FACTOR = 2
    margin_size = sr_asym.fn_margin_size(TP_FIR_LEN//2, TT_DATA)
    num_in_ports = TP_SSR
    num_tot_out_ports = TP_SSR*TP_PARA_INTERP_POLY
    num_out_ports_per_poly = TP_SSR
    num_rtp_taps = ((TP_FIR_LEN+1)/4+1)
    if (TP_PARA_INTERP_POLY > 1):
      num_rtp_taps = ((TP_FIR_LEN+1)/2) 
    elif (AIE_VARIANT == 2) :
      num_rtp_taps = ((TP_FIR_LEN+1)/2) + 1
    else :
      num_rtp_taps = ((TP_FIR_LEN+1)/4+1)

    in_win_size = get_input_window_size(TP_INPUT_WINDOW_VSIZE, num_in_ports, TP_API, TP_DUAL_IP)
    out_win_size = get_output_window_size(TP_INPUT_WINDOW_VSIZE, num_tot_out_ports, TP_API, TP_NUM_OUTPUTS, TP_DECIMATE_FACTOR, TP_INTERPOLATE_FACTOR)

    in_ports = get_port_info("in", "in", TT_DATA, in_win_size, num_in_ports, margin_size, TP_API=TP_API)
    in2_ports = (get_port_info("in2", "in", TT_DATA, in_win_size, num_in_ports, margin_size, TP_API=TP_API) if (TP_DUAL_IP == 1) else [])
    coeff_ports = (get_parameter_port_info("coeff", "in", TT_COEFF, TP_SSR, num_rtp_taps, "async") if (TP_USE_COEFF_RELOAD == 1) else [])
    coeffCT_ports = (get_parameter_port_info("coeffCT", "in", TT_COEFF, TP_SSR, 1, "async") if (TP_USE_COEFF_RELOAD == 1 and TP_PARA_INTERP_POLY > 1) else [])

    # interp by 2 for halfband
    out_ports = get_port_info("out", "out", TT_DATA, out_win_size, num_out_ports_per_poly, TP_API=TP_API)
    out2_ports = (get_port_info("out2", "out", TT_DATA, out_win_size, num_out_ports_per_poly, TP_API=TP_API) if (TP_NUM_OUTPUTS == 2) else [])
    out3_ports = (get_port_info("out3", "out", TT_DATA, out_win_size, num_out_ports_per_poly, TP_API=TP_API) if (TP_PARA_INTERP_POLY > 1) else [])
    out4_ports = (get_port_info("out4", "out", TT_DATA, out_win_size, num_out_ports_per_poly, TP_API=TP_API) if (TP_PARA_INTERP_POLY > 1 and TP_NUM_OUTPUTS == 2) else [])
    return in_ports + in2_ports + coeff_ports + coeffCT_ports + out_ports + out2_ports + out3_ports + out4_ports


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
  TP_USE_COEFF_RELOAD = args["TP_USE_COEFF_RELOAD"]
  TP_NUM_OUTPUTS = args["TP_NUM_OUTPUTS"]
  TP_DUAL_IP = args["TP_DUAL_IP"]
  TP_UPSHIFT_CT = args["TP_UPSHIFT_CT"]
  TP_API = args["TP_API"]
  TP_SSR = args["TP_SSR"]
  TP_PARA_INTERP_POLY = args["TP_PARA_INTERP_POLY"]
  coeff_list = args["coeff"]
  TP_SAT = args["TP_SAT"]

  taps = sr_asym.fn_get_taps_vector(TT_COEFF, coeff_list)
  constr_args_str = f"taps" if TP_USE_COEFF_RELOAD == 0 else ""
  dual_ip_declare_str = f"ssr_port_array<input> in2;" if TP_DUAL_IP == 1 else "// No dual input"
  dual_ip_connect_str = f"adf::connect<> net_in2(in2[i], filter.in2[i]);" if TP_DUAL_IP == 1 else "// No dual input"
  coeff_ip_declare_str = f"ssr_port_array<input> coeff;" if TP_USE_COEFF_RELOAD == 1 else "// No coeff port"
  coeff_ip_connect_str = f"adf::connect<> net_coeff(coeff[i], filter.coeff[i]);" if TP_USE_COEFF_RELOAD == 1 else "// No coeff port"
  coeffCT_ip_declare_str = f"ssr_port_array<input> coeffCT;" if (TP_USE_COEFF_RELOAD == 1 and TP_PARA_INTERP_POLY > 1) else "// No coeffCT port"
  coeffCT_ip_connect_str = f"adf::connect<> net_coeffCT(coeffCT[i], filter.coeffCT[i]);" if (TP_USE_COEFF_RELOAD == 1 and TP_PARA_INTERP_POLY > 1) == 1 else "// No coeffCT port"
  dual_op_declare_str = f"ssr_port_array<output> out2;" if TP_NUM_OUTPUTS == 2 else "// No dual output"
  dual_op_connect_str = f"adf::connect<> net_out2(filter.out2[i], out2[i]);" if TP_NUM_OUTPUTS == 2 else "// No dual output"
  op3_declare_str = f"ssr_port_array<output> out3;" if TP_PARA_INTERP_POLY > 1 else ""
  op3_connect_str = f"adf::connect<> net_out3(filter.out3[i], out3[i]);" if TP_PARA_INTERP_POLY > 1 else ""
  op4_declare_str = f"ssr_port_array<output> out4;" if (TP_PARA_INTERP_POLY > 1 and TP_NUM_OUTPUTS == 2) else ""
  op4_connect_str = f"adf::connect<> net_out4(filter.out4[i], out4[i]);" if (TP_PARA_INTERP_POLY > 1 and TP_NUM_OUTPUTS == 2) else ""

  # Use formatted multi-line string to avoid a lot of \n and \t
  code  = (
f"""
class {graphname} : public adf::graph {{
public:
  static constexpr unsigned int TP_SSR = {TP_SSR};
  static constexpr unsigned int TP_PARA_INTERP_POLY = {TP_PARA_INTERP_POLY};
  template <typename dir>
  using ssr_port_array = std::array<adf::port<dir>, TP_SSR>;

  ssr_port_array<input> in;
  {dual_ip_declare_str}
  {coeff_ip_declare_str}
  {coeffCT_ip_declare_str}
  ssr_port_array<output> out;
  {dual_op_declare_str}
  {op3_declare_str}
  {op4_declare_str}

  std::vector<{TT_COEFF}> taps = {taps};
  xf::dsp::aie::fir::interpolate_hb::fir_interpolate_hb_graph<
    {TT_DATA}, //TT_DATA
    {TT_COEFF}, //TT_COEFF
    {TP_FIR_LEN}, //TP_FIR_LEN
    {TP_SHIFT}, //TP_SHIFT
    {TP_RND}, //TP_RND
    {TP_INPUT_WINDOW_VSIZE}, //TP_INPUT_WINDOW_VSIZE
    {TP_CASC_LEN}, //TP_CASC_LEN
    {TP_DUAL_IP}, //TP_DUAL_IP
    {TP_USE_COEFF_RELOAD}, //TP_USE_COEFF_RELOAD
    {TP_NUM_OUTPUTS}, //TP_NUM_OUTPUTS
    {TP_UPSHIFT_CT}, //TP_UPSHIFT_CT
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
    for (int i=0; i < TP_SSR; i++) {{
      adf::connect<> net_in(in[i], filter.in[i]);
      {dual_ip_connect_str}
      {coeff_ip_connect_str}
      {coeffCT_ip_connect_str}
      adf::connect<> net_out(filter.out[i], out[i]);
      {dual_op_connect_str}
      {op3_connect_str}
      {op4_connect_str}
    }}
  }}

}};
""")
  out = {}
  out["graph"] = code
  out["port_info"] = info_ports(args)
  out["headerfile"] = "fir_interpolate_hb_graph.hpp"
  out["searchpaths"] = [
       "L2/include/aie",
       "L2/tests/aie/common/inc",
       "L1/include/aie",
       "L1/src/aie",
       "L1/tests/aie/inc",
       "L1/tests/aie/src"
  ]

  return out

