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
from getPhaseAlias import getPhaseAlias


TP_DECIMATE_FACTOR_min = 1
TP_DECIMATE_FACTOR_max = 16
TP_INTERPOLATE_FACTOR_min = 1
TP_INTERPOLATE_FACTOR_max = 16
TP_INPUT_WINDOW_VSIZE_min = 4
TP_SSR_min = 1
TP_PARA_DECI_POLY_min = 1
TP_PARA_INTERP_POLY_min = 1
TP_CASC_LEN_min = 1
TP_CASC_LEN_max = 40
TP_FIR_LEN_min = 4
TP_FIR_LEN_max = 8192
TP_SHIFT_min=0
TP_SHIFT_max=80
TP_SSR_min = 1
TP_SSR_max = 4
TP_INTERPOLATE_FACTOR_max_aie1 = 16
TP_INTERPOLATE_FACTOR_max_aie2 = 8
TP_DECIMATE_FACTOR_max_aie1 = 16
TP_DECIMATE_FACTOR_max_aie2 = 8
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
  return fn_update_TT_DATA()

def fn_update_TT_DATA():
  legal_set_TT_DATA=["int16", "int32", "cint16", "cint32", "float", "cfloat"]  
  param_dict={
     "name" : "TT_DATA",
     "enum" : legal_set_TT_DATA
  }
  return param_dict

def validate_TT_DATA(args):
  TT_DATA=args["TT_DATA"]
  return (fn_validate_TT_DATA(TT_DATA))

def fn_validate_TT_DATA(TT_DATA):
  param_dict = fn_update_TT_DATA()
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
  legal_set_TT_COEFF=["int16", "int32", "cint16", "cint32", "float", "cfloat"]  
  legal_set_TT_COEFF=fn_coeff_type_update(TT_DATA, legal_set_TT_COEFF)
  
  remove_set=[]
  if  AIE_VARIANT==1 and TT_DATA=="int16":
    remove_set.append("int16")

  legal_set_TT_COEFF=remove_from_set(remove_set, legal_set_TT_COEFF.copy())

  param_dict={
     "name" : "TT_COEFF",
     "enum" : legal_set_TT_COEFF
  }
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
########## TP_FIR_LEN Updater and Validator ###########
#######################################################
def update_TP_FIR_LEN(args):
    AIE_VARIANT = args["AIE_VARIANT"]
    TT_DATA = args["TT_DATA"]
    TT_COEFF = args["TT_COEFF"]
    TP_API = args["TP_API"]
    TP_USE_COEFF_RELOAD = args["TP_USE_COEFF_RELOAD"]
    return fn_update_TP_FIR_LEN(AIE_VARIANT, TT_DATA, TT_COEFF, TP_API, TP_USE_COEFF_RELOAD)

def fn_update_TP_FIR_LEN(AIE_VARIANT, TT_DATA, TT_COEFF, TP_API, TP_USE_COEFF_RELOAD):

    coeffSizeMult = 1 if TP_API == 0 else TP_INTERPOLATE_FACTOR_max
    TP_FIR_LEN_max_kernel=fn_max_fir_len_each_kernel_update(TT_DATA, TP_CASC_LEN_max, TP_USE_COEFF_RELOAD, TP_SSR_max, TP_API, coeffSizeMult)

    TP_FIR_LEN_max_overall = fn_max_fir_len_overall(TT_DATA, TT_COEFF)

    TP_FIR_LEN_max_int=min(TP_FIR_LEN_max_kernel, TP_FIR_LEN_max_overall, TP_FIR_LEN_max)

    if AIE_VARIANT == 2:
        interp_fact_max = TP_INTERPOLATE_FACTOR_max_aie2
        deci_fact_max = TP_DECIMATE_FACTOR_max_aie2
        TP_FIR_LEN_max_temp3= interp_fact_max * deci_fact_max * TP_CASC_LEN_max
        TP_FIR_LEN_max_int=min(TP_FIR_LEN_max_int, TP_FIR_LEN_max_temp3)

    param_dict={
        "name" : "TP_FIR_LEN",
        "minimum" : TP_FIR_LEN_min,
        "maximum" : TP_FIR_LEN_max_int
    }
    return param_dict

def validate_TP_FIR_LEN(args):
    AIE_VARIANT = args["AIE_VARIANT"]
    TT_DATA = args["TT_DATA"]
    TT_COEFF = args["TT_COEFF"]
    TP_API = args["TP_API"]
    TP_USE_COEFF_RELOAD = args["TP_USE_COEFF_RELOAD"]
    TP_FIR_LEN = args["TP_FIR_LEN"]
    return fn_validate_TP_FIR_LEN(AIE_VARIANT, TT_DATA, TT_COEFF, TP_API, TP_USE_COEFF_RELOAD, TP_FIR_LEN)

def fn_validate_TP_FIR_LEN(AIE_VARIANT, TT_DATA, TT_COEFF, TP_API, TP_USE_COEFF_RELOAD, TP_FIR_LEN):
    param_dict=fn_update_TP_FIR_LEN(AIE_VARIANT, TT_DATA, TT_COEFF, TP_API, TP_USE_COEFF_RELOAD)
    range_fir_len=[param_dict["minimum"], param_dict["maximum"]]
    return validate_range(range_fir_len, "TP_FIR_LEN", TP_FIR_LEN) 

#######################################################
##### TP_INTERPOLATE_FACTOR Updater and Validator #####
#######################################################    
def update_TP_INTERPOLATE_FACTOR(args):
    AIE_VARIANT = args["AIE_VARIANT"]
    TP_FIR_LEN = args["TP_FIR_LEN"]
    return fn_update_TP_INTERPOLATE_FACTOR(AIE_VARIANT, TP_FIR_LEN)

def fn_update_TP_INTERPOLATE_FACTOR(AIE_VARIANT, TP_FIR_LEN):
    param_dict_int= fn_update_interpolate_factor(AIE_VARIANT, "TP_INTERPOLATE_FACTOR")
    if AIE_VARIANT==2:
        TP_INTERPOLATE_FACTOR_max=min(TP_FIR_LEN, param_dict_int["maximum"])
        legal_set_interp_fact=find_divisors(TP_FIR_LEN, TP_INTERPOLATE_FACTOR_max)
        param_dict={"name" : "TP_INTERPOLATE_FACTOR",
                    "enum" : legal_set_interp_fact}
    else:param_dict=param_dict_int
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
#######TP_PARA_INTERP_POLY Updater and Validator ######
#######################################################  
def update_TP_PARA_INTERP_POLY(args):
    TP_FIR_LEN = args["TP_FIR_LEN"]
    TP_INTERPOLATE_FACTOR = args["TP_INTERPOLATE_FACTOR"]
    return fn_update_TP_PARA_INTERP_POLY(TP_FIR_LEN, TP_INTERPOLATE_FACTOR)
    
def fn_update_TP_PARA_INTERP_POLY(TP_FIR_LEN, TP_INTERPOLATE_FACTOR):
    if TP_FIR_LEN % TP_INTERPOLATE_FACTOR == 0:
        legal_set_para_interp_poly=[1, TP_INTERPOLATE_FACTOR]
    else:
        legal_set_para_interp_poly=[1] 

    param_dict={
        "name" : "TP_PARA_INTERP_POLY",
        "enum" : legal_set_para_interp_poly
    }
    return param_dict

def validate_TP_PARA_INTERP_POLY(args):
    TP_FIR_LEN = args["TP_FIR_LEN"]
    TP_PARA_INTERP_POLY = args["TP_PARA_INTERP_POLY"]
    TP_INTERPOLATE_FACTOR = args["TP_INTERPOLATE_FACTOR"]
    return fn_validate_TP_PARA_INTERP_POLY(TP_FIR_LEN, TP_INTERPOLATE_FACTOR, TP_PARA_INTERP_POLY)

def fn_validate_TP_PARA_INTERP_POLY(TP_FIR_LEN, TP_INTERPOLATE_FACTOR, TP_PARA_INTERP_POLY):
    param_dict=fn_update_TP_PARA_INTERP_POLY(TP_FIR_LEN, TP_INTERPOLATE_FACTOR)
    return validate_legal_set(param_dict["enum"], "TP_PARA_INTERP_POLY", TP_PARA_INTERP_POLY)

#######################################################
#######TP_DECIMATE_FACTOR Updater and Validator #######
#######################################################  
def update_TP_DECIMATE_FACTOR(args):
    AIE_VARIANT = args["AIE_VARIANT"]
    TP_FIR_LEN = args["TP_FIR_LEN"]
    TP_INTERPOLATE_FACTOR = args["TP_INTERPOLATE_FACTOR"]
    return fn_update_TP_DECIMATE_FACTOR(AIE_VARIANT, TP_FIR_LEN, TP_INTERPOLATE_FACTOR)

def fn_update_TP_DECIMATE_FACTOR(AIE_VARIANT, TP_FIR_LEN, TP_INTERPOLATE_FACTOR):
    AIE_ML_MAX_DF = 8
    if AIE_VARIANT == 2:
        TP_DECIMATE_FACTOR_max_temp=int(FLOOR(TP_FIR_LEN/TP_INTERPOLATE_FACTOR,1))
        TP_DECIMATE_FACTOR_max_int=min(AIE_ML_MAX_DF, TP_DECIMATE_FACTOR_max_temp)
    else:
        TP_DECIMATE_FACTOR_max_int=TP_DECIMATE_FACTOR_max

    param_dict={
        "name"    : "TP_DECIMATE_FACTOR",
        "minimum" : TP_DECIMATE_FACTOR_min,
        "maximum" : TP_DECIMATE_FACTOR_max_int}

    return param_dict

def validate_TP_DECIMATE_FACTOR(args):
    AIE_VARIANT = args["AIE_VARIANT"]
    TP_FIR_LEN = args["TP_FIR_LEN"]
    TP_INTERPOLATE_FACTOR = args["TP_INTERPOLATE_FACTOR"]
    TP_DECIMATE_FACTOR = args["TP_DECIMATE_FACTOR"]
    return fn_validate_TP_DECIMATE_FACTOR(AIE_VARIANT, TP_FIR_LEN, TP_INTERPOLATE_FACTOR, TP_DECIMATE_FACTOR)
 
def fn_validate_TP_DECIMATE_FACTOR(AIE_VARIANT, TP_FIR_LEN, TP_INTERPOLATE_FACTOR, TP_DECIMATE_FACTOR):
    param_dict=fn_update_TP_DECIMATE_FACTOR(AIE_VARIANT, TP_FIR_LEN, TP_INTERPOLATE_FACTOR)
    range_deci_fact=[param_dict["minimum"], param_dict["maximum"]]
    return validate_range(range_deci_fact, "TP_DECIMATE_FACTOR", TP_DECIMATE_FACTOR)

#######################################################
####### TP_PARA_DECI_POLY Updater and Validator #######
#######################################################  
def update_TP_PARA_DECI_POLY(args):
    TP_FIR_LEN = args["TP_FIR_LEN"]
    TP_DECIMATE_FACTOR = args["TP_DECIMATE_FACTOR"]
    return fn_update_TP_PARA_DECI_POLY(TP_FIR_LEN, TP_DECIMATE_FACTOR)

def fn_update_TP_PARA_DECI_POLY(TP_FIR_LEN, TP_DECIMATE_FACTOR):
    if TP_FIR_LEN % TP_DECIMATE_FACTOR == 0:
        legal_set_para_deci_poly=[1, TP_DECIMATE_FACTOR]
    else:
        legal_set_para_deci_poly=[1]

    param_dict={
        "name" : "TP_PARA_DECI_POLY",
        "enum" : legal_set_para_deci_poly
    }
    return param_dict

def validate_TP_PARA_DECI_POLY(args):
    TP_FIR_LEN = args["TP_FIR_LEN"]
    TP_PARA_DECI_POLY = args["TP_PARA_DECI_POLY"]
    TP_DECIMATE_FACTOR = args["TP_DECIMATE_FACTOR"]
    return fn_validate_TP_PARA_DECI_POLY(TP_FIR_LEN, TP_DECIMATE_FACTOR, TP_PARA_DECI_POLY)

def fn_validate_TP_PARA_DECI_POLY(TP_FIR_LEN, TP_DECIMATE_FACTOR, TP_PARA_DECI_POLY):
    param_dict=fn_update_TP_PARA_DECI_POLY(TP_FIR_LEN, TP_DECIMATE_FACTOR)
    return validate_legal_set(param_dict["enum"], "TP_PARA_DECI_POLY", TP_PARA_DECI_POLY)

#######################################################
############# TP_DUAL_IP Updater and Validator ########
#######################################################
def update_TP_DUAL_IP(args):
  AIE_VARIANT = args["AIE_VARIANT"]
  TP_API = args["TP_API"]
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
######### TP_NUM_OUTPUTS Updater and Validator ########
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
    TP_API = args["TP_API"]
    TP_DECIMATE_FACTOR = args["TP_DECIMATE_FACTOR"]
    TP_PARA_DECI_POLY = args["TP_PARA_DECI_POLY"]
    TP_INTERPOLATE_FACTOR = args["TP_INTERPOLATE_FACTOR"]
    TP_PARA_INTERP_POLY = args["TP_PARA_INTERP_POLY"]
    return fn_update_TP_SSR(TP_API, TP_DECIMATE_FACTOR, TP_PARA_DECI_POLY, TP_INTERPOLATE_FACTOR, TP_PARA_INTERP_POLY)

def fn_update_TP_SSR(TP_API, TP_DECIMATE_FACTOR, TP_PARA_DECI_POLY, TP_INTERPOLATE_FACTOR, TP_PARA_INTERP_POLY):
    if (TP_INTERPOLATE_FACTOR != TP_PARA_INTERP_POLY) or (TP_DECIMATE_FACTOR != TP_PARA_DECI_POLY) or (TP_API == 0):
        legal_set_ssr=[1]
       
    param_dict={
       "name" : "TP_SSR"
    }
    if "legal_set_ssr" in locals():
        param_dict.update({"enum" : legal_set_ssr})
    else:
        param_dict.update({
            "minimum" : TP_SSR_min,
            "maximum" : TP_SSR_max })        

    return param_dict

def validate_TP_SSR(args):
    TP_API = args["TP_API"]
    TP_DECIMATE_FACTOR = args["TP_DECIMATE_FACTOR"]
    TP_PARA_DECI_POLY = args["TP_PARA_DECI_POLY"]
    TP_INTERPOLATE_FACTOR = args["TP_INTERPOLATE_FACTOR"]
    TP_PARA_INTERP_POLY = args["TP_PARA_INTERP_POLY"]
    TP_SSR = args["TP_SSR"]
    return fn_validate_TP_SSR(TP_API, TP_DECIMATE_FACTOR, TP_PARA_DECI_POLY, TP_INTERPOLATE_FACTOR, TP_PARA_INTERP_POLY, TP_SSR)

def fn_validate_TP_SSR(TP_API, TP_DECIMATE_FACTOR, TP_PARA_DECI_POLY, TP_INTERPOLATE_FACTOR, TP_PARA_INTERP_POLY, TP_SSR):
   param_dict=fn_update_TP_SSR(TP_API, TP_DECIMATE_FACTOR, TP_PARA_DECI_POLY, TP_INTERPOLATE_FACTOR, TP_PARA_INTERP_POLY)
   if "enum" in param_dict:
      return validate_legal_set(param_dict["enum"], "TP_SSR", TP_SSR)
   else:
      range_ssr=[param_dict["minimum"], param_dict["maximum"]]
      return validate_range(range_ssr, "TP_SSR", TP_SSR)

#######################################################
########## TP_CASC_LEN Updater and Validator ##########
#######################################################  
def update_TP_CASC_LEN(args):
    AIE_VARIANT = args["AIE_VARIANT"]
    TT_DATA = args["TT_DATA"]
    TT_COEFF = args["TT_COEFF"]
    TP_API = args["TP_API"]
    TP_FIR_LEN = args["TP_FIR_LEN"]
    TP_SSR = args["TP_SSR"]
    TP_INTERPOLATE_FACTOR = args["TP_INTERPOLATE_FACTOR"]
    TP_DECIMATE_FACTOR = args["TP_DECIMATE_FACTOR"]
    TP_USE_COEFF_RELOAD = args["TP_USE_COEFF_RELOAD"]
    return fn_update_TP_CASC_LEN(AIE_VARIANT, TT_DATA, TT_COEFF, TP_API, TP_FIR_LEN, TP_SSR, TP_INTERPOLATE_FACTOR,TP_DECIMATE_FACTOR, TP_USE_COEFF_RELOAD)

def fn_update_TP_CASC_LEN(AIE_VARIANT, TT_DATA, TT_COEFF, TP_API, TP_FIR_LEN, TP_SSR, TP_INTERPOLATE_FACTOR,TP_DECIMATE_FACTOR, TP_USE_COEFF_RELOAD):
    legal_set_casc_len=list(range(TP_CASC_LEN_min, TP_CASC_LEN_max))
    if AIE_VARIANT==AIE_ML:
        legal_set_casc_len=find_divisors((TP_FIR_LEN/TP_INTERPOLATE_FACTOR), TP_CASC_LEN_max)
    legal_set_casc_len_temp1=fn_eliminate_casc_len_min_fir_len_each_kernel(legal_set_casc_len.copy(), TP_FIR_LEN, TP_SSR, TP_INTERPOLATE_FACTOR)

    remove_set=[]
    for casc_len in legal_set_casc_len_temp1.copy():
        streamingVectorRegisterCheck = fn_check_samples_can_fit_streaming(
            TT_DATA,
            TT_COEFF,
            TP_FIR_LEN,
            TP_INTERPOLATE_FACTOR,
            TP_DECIMATE_FACTOR,
            casc_len,
            TP_API)
        if streamingVectorRegisterCheck != isValid:
           remove_set.append(casc_len)

        if AIE_VARIANT == 2:
            if (TP_FIR_LEN / casc_len) <  (TP_INTERPOLATE_FACTOR*TP_DECIMATE_FACTOR):
                remove_set.append(casc_len)
    
    legal_set_casc_len_temp2=remove_from_set(remove_set, legal_set_casc_len_temp1.copy())

    coeffSizeMult = 1 if TP_API == 0 else TP_INTERPOLATE_FACTOR
    legal_set_casc_len_temp3=fn_eliminate_casc_len_max_fir_len_each_kernel(TT_DATA, TP_FIR_LEN, TP_USE_COEFF_RELOAD, TP_SSR, TP_API, coeffSizeMult, legal_set_casc_len_temp2.copy())

    param_dict={
       "name" : "TP_CASC_LEN",
       "enum" : legal_set_casc_len_temp3}
    return param_dict

def validate_TP_CASC_LEN(args):
    AIE_VARIANT = args["AIE_VARIANT"]
    TT_DATA = args["TT_DATA"]
    TT_COEFF = args["TT_COEFF"]
    TP_API = args["TP_API"]
    TP_FIR_LEN = args["TP_FIR_LEN"]
    TP_SSR = args["TP_SSR"]
    TP_INTERPOLATE_FACTOR = args["TP_INTERPOLATE_FACTOR"]
    TP_DECIMATE_FACTOR = args["TP_DECIMATE_FACTOR"]
    TP_USE_COEFF_RELOAD = args["TP_USE_COEFF_RELOAD"]
    TP_CASC_LEN = args["TP_CASC_LEN"]
    return fn_validate_TP_CASC_LEN(AIE_VARIANT, TT_DATA, TT_COEFF, TP_API, TP_FIR_LEN, TP_SSR, TP_INTERPOLATE_FACTOR,TP_DECIMATE_FACTOR, TP_USE_COEFF_RELOAD, TP_CASC_LEN)

def fn_validate_TP_CASC_LEN(AIE_VARIANT, TT_DATA, TT_COEFF, TP_API, TP_FIR_LEN, TP_SSR, TP_INTERPOLATE_FACTOR,TP_DECIMATE_FACTOR, TP_USE_COEFF_RELOAD, TP_CASC_LEN):
    param_dict=fn_update_TP_CASC_LEN(AIE_VARIANT, TT_DATA, TT_COEFF, TP_API, TP_FIR_LEN, TP_SSR, TP_INTERPOLATE_FACTOR,TP_DECIMATE_FACTOR, TP_USE_COEFF_RELOAD)
    return validate_legal_set(param_dict["enum"], "TP_CASC_LEN", TP_CASC_LEN)

def fn_check_samples_can_fit_streaming(
    TT_DATA,
    TT_COEFF,
    TP_FIR_LEN,
    TP_INTERPOLATE_FACTOR,
    TP_DECIMATE_FACTOR,
    TP_CASC_LEN,
    TP_API,
):

    m_kWinAccessByteSize = 128 // 8  # fixed

    m_kDataBuffXOffset = (
        m_kWinAccessByteSize / fn_size_by_byte(TT_DATA)
    ) - 1  # Let's just maximally size this to avoid needing so much duplicated code.

    m_kLanes = fnNumLanes(TT_DATA, TT_COEFF, TP_API)
    m_kNumSamplesRequiredForNLanes = (
        m_kLanes * TP_DECIMATE_FACTOR + (TP_INTERPOLATE_FACTOR - 1)
    ) / TP_INTERPOLATE_FACTOR

    m_kSamplesInBuff = (1024 // 8) // fn_size_by_byte(TT_DATA)

    if TP_API != 0:  # stream
        for kernelPos in range(TP_CASC_LEN):
            TP_FIR_RANGE_LEN = (
                fnFirRangeRem(TP_FIR_LEN, TP_CASC_LEN, kernelPos, TP_INTERPOLATE_FACTOR)
                if (kernelPos == (TP_CASC_LEN - 1))
                else fnFirRange(
                    TP_FIR_LEN, TP_CASC_LEN, kernelPos, TP_INTERPOLATE_FACTOR
                )
            )
            m_kPolyLen = (
                TP_FIR_RANGE_LEN + TP_INTERPOLATE_FACTOR - 1
            ) // TP_INTERPOLATE_FACTOR
            m_kInitDataNeeded = (
                m_kDataBuffXOffset + m_kPolyLen + m_kNumSamplesRequiredForNLanes - 1
            )
            if m_kInitDataNeeded > m_kSamplesInBuff:
                return isError(
                    f"Filter length per kernel ({TP_FIR_RANGE_LEN}) for kernel {kernelPos} exceeds max supported range for this data/coeff type combination ({TT_DATA}/{TT_COEFF}). Increase cascade length to split the workload over more kernels. "
                )

    return isValid
#######################################################
##### TP_INPUT_WINDOW_VSIZE Updater and Validator #####
#######################################################  

def update_TP_INPUT_WINDOW_VSIZE(args):
    TT_DATA = args["TT_DATA"]
    TT_COEFF = args["TT_COEFF"]
    TP_FIR_LEN = args["TP_FIR_LEN"]
    TP_API = args["TP_API"]
    TP_INTERPOLATE_FACTOR = args["TP_INTERPOLATE_FACTOR"]
    TP_DECIMATE_FACTOR = args["TP_DECIMATE_FACTOR"]
    TP_PARA_INTERP_POLY = args["TP_PARA_INTERP_POLY"]
    TP_PARA_DECI_POLY = args["TP_PARA_DECI_POLY"]
    AIE_VARIANT = args["AIE_VARIANT"]
    TP_SSR = 1
    if args["TP_INPUT_WINDOW_VSIZE"]: TP_INPUT_WINDOW_VSIZE = args["TP_INPUT_WINDOW_VSIZE"]
    else: TP_INPUT_WINDOW_VSIZE = 0

    return fn_update_TP_INPUT_WINDOW_VSIZE(
        TT_DATA,
        TT_COEFF,
        TP_FIR_LEN,
        TP_INTERPOLATE_FACTOR,
        TP_DECIMATE_FACTOR,
        TP_INPUT_WINDOW_VSIZE,
        TP_API,
        TP_SSR,
        TP_PARA_INTERP_POLY,
        TP_PARA_DECI_POLY,
        AIE_VARIANT
    )

def fn_update_TP_INPUT_WINDOW_VSIZE(
        TT_DATA,
        TT_COEFF,
        TP_FIR_LEN,
        TP_INTERPOLATE_FACTOR,
        TP_DECIMATE_FACTOR,
        TP_INPUT_WINDOW_VSIZE,
        TP_API,
        TP_SSR,
        TP_PARA_INTERP_POLY,
        TP_PARA_DECI_POLY,
        AIE_VARIANT
    ):

    streamRptFactor = 8
    m_kPolyphaseLaneAlias = getPhaseAlias(TT_DATA, TT_COEFF, TP_INTERPOLATE_FACTOR, TP_DECIMATE_FACTOR, TP_API)
    numLanes = fnNumLanes(TT_DATA, TT_COEFF, TP_API, AIE_VARIANT)
    if AIE_VARIANT == 1 :
        multipleToBeChecked = (
            (m_kPolyphaseLaneAlias * numLanes)
            if TP_API == 0
            else (
                m_kPolyphaseLaneAlias
                * numLanes
                * streamRptFactor
            )
        )
    if AIE_VARIANT == 2 :
        # AIE-ML decpomposes to
        multipleToBeChecked = numLanes

    factor_ws=TP_SSR * TP_PARA_DECI_POLY * numLanes

    Alignment256b = (256 // 8) / fn_size_by_byte(TT_DATA)
    lcm_ws=find_lcm_list([Alignment256b , factor_ws])

    TP_INPUT_WINDOW_VSIZE_min_temp=TP_INPUT_WINDOW_VSIZE_min*(TP_PARA_DECI_POLY * TP_PARA_INTERP_POLY)   
    TP_INPUT_WINDOW_VSIZE_min_int=int(CEIL(TP_INPUT_WINDOW_VSIZE_min_temp, lcm_ws))

    if TP_API==0:
        TP_INPUT_WINDOW_VSIZE_max_temp=fn_max_windowsize_for_buffer_update(TT_DATA, TP_FIR_LEN, TP_SSR, TP_INTERPOLATE_FACTOR, TP_DECIMATE_FACTOR, AIE_VARIANT)
        TP_INPUT_WINDOW_VSIZE_max_int=int(FLOOR(TP_INPUT_WINDOW_VSIZE_max_temp, lcm_ws))
    else:
        TP_INPUT_WINDOW_VSIZE_max_int=TP_INPUT_WINDOW_VSIZE_max_cpp

    param_dict={
       "name"    : TP_INPUT_WINDOW_VSIZE,
       "minimum" : TP_INPUT_WINDOW_VSIZE_min_int,
       "maximum" : TP_INPUT_WINDOW_VSIZE_max_int
    }
    if TP_INPUT_WINDOW_VSIZE != 0:
        checkOutputMultipleLanes = fn_out_windowsize_multiple_lanes(
        TT_DATA, TT_COEFF, TP_INPUT_WINDOW_VSIZE, TP_API, multipleToBeChecked, TP_SSR, TP_INTERPOLATE_FACTOR, TP_DECIMATE_FACTOR, AIE_VARIANT
        )   
        if checkOutputMultipleLanes == isValid: #Only advice a close number if this check passes
            if TP_INPUT_WINDOW_VSIZE % lcm_ws != 0:
                TP_INPUT_WINDOW_VSIZE_act=int(round(TP_INPUT_WINDOW_VSIZE/lcm_ws)*lcm_ws)
                if TP_INPUT_WINDOW_VSIZE_act < TP_INPUT_WINDOW_VSIZE_min_int:
                    TP_INPUT_WINDOW_VSIZE_act=int(CEIL(TP_INPUT_WINDOW_VSIZE_act, TP_INPUT_WINDOW_VSIZE_min_int))  
                if TP_INPUT_WINDOW_VSIZE_act > TP_INPUT_WINDOW_VSIZE_max_int:
                    TP_INPUT_WINDOW_VSIZE_act=int(CEIL(TP_INPUT_WINDOW_VSIZE_act, TP_INPUT_WINDOW_VSIZE_max_int))  

                if AIE_VARIANT == 1 :   
                    TP_INPUT_WINDOW_VSIZE_act = fn_ws_update_check_repeatFactor(
                        TT_DATA,
                        TT_COEFF,
                        TP_INTERPOLATE_FACTOR,
                        TP_DECIMATE_FACTOR,
                        TP_INPUT_WINDOW_VSIZE_act/TP_SSR,
                        TP_API,
                        TP_INPUT_WINDOW_VSIZE_min_int,
                        TP_INPUT_WINDOW_VSIZE_max_int
                    )
                param_dict.update({"actual" : TP_INPUT_WINDOW_VSIZE_act})
    return param_dict


def validate_TP_INPUT_WINDOW_VSIZE(args):
    TP_INPUT_WINDOW_VSIZE = args["TP_INPUT_WINDOW_VSIZE"]
    TT_DATA = args["TT_DATA"]
    TT_COEFF = args["TT_COEFF"]
    TP_FIR_LEN = args["TP_FIR_LEN"]
    TP_API = args["TP_API"]
    TP_INTERPOLATE_FACTOR = args["TP_INTERPOLATE_FACTOR"]
    TP_DECIMATE_FACTOR = args["TP_DECIMATE_FACTOR"]
    TP_PARA_INTERP_POLY = args["TP_PARA_INTERP_POLY"]
    TP_PARA_DECI_POLY = args["TP_PARA_DECI_POLY"]
    AIE_VARIANT = args["AIE_VARIANT"]
    TP_SSR = 1

    # interpolate_hb traits looks like the UPSHIFT_CT types have different number of lanes, but it's actually stil the exact same as 384..
    # decimate_hb also just uses 384, so no additional rules here.
    return fn_validate_TP_INPUT_WINDOW_VSIZE(
        TT_DATA,
        TT_COEFF,
        TP_FIR_LEN,
        TP_INTERPOLATE_FACTOR,
        TP_DECIMATE_FACTOR,
        TP_INPUT_WINDOW_VSIZE,
        TP_API,
        TP_SSR,
        TP_PARA_INTERP_POLY,
        TP_PARA_DECI_POLY,
        AIE_VARIANT)

def fn_validate_TP_INPUT_WINDOW_VSIZE(
        TT_DATA,
        TT_COEFF,
        TP_FIR_LEN,
        TP_INTERPOLATE_FACTOR,
        TP_DECIMATE_FACTOR,
        TP_INPUT_WINDOW_VSIZE,
        TP_API,
        TP_SSR,
        TP_PARA_INTERP_POLY,
        TP_PARA_DECI_POLY,
        AIE_VARIANT):

    streamRptFactor = 8
    m_kPolyphaseLaneAlias = getPhaseAlias(TT_DATA, TT_COEFF, TP_INTERPOLATE_FACTOR, TP_DECIMATE_FACTOR, TP_API)
    numLanes = fnNumLanes(TT_DATA, TT_COEFF, TP_API, AIE_VARIANT)
    if AIE_VARIANT == 1 :
        multipleToBeChecked = (
            (m_kPolyphaseLaneAlias * numLanes)
            if TP_API == 0
            else (
                m_kPolyphaseLaneAlias
                * numLanes
                * streamRptFactor
            )
        )
    if AIE_VARIANT == 2 :
        # AIE-ML decpomposes to
        multipleToBeChecked = numLanes

    checkOutputMultipleLanes = fn_out_windowsize_multiple_lanes(
        TT_DATA, TT_COEFF, TP_INPUT_WINDOW_VSIZE, TP_API, multipleToBeChecked, TP_SSR, TP_INTERPOLATE_FACTOR, TP_DECIMATE_FACTOR, AIE_VARIANT
    )

    if checkOutputMultipleLanes != isValid: #This Check should pass first for the sake of config_helper
        return checkOutputMultipleLanes
        
    factor_ws=TP_SSR * TP_PARA_DECI_POLY * numLanes

    Alignment256b = (256 // 8) / fn_size_by_byte(TT_DATA)
    lcm_ws=find_lcm_list([Alignment256b , factor_ws])

    if AIE_VARIANT == 1 :
        checkRepeatFactor = fn_check_repeatFactor(
            TT_DATA,
            TT_COEFF,
            TP_INTERPOLATE_FACTOR,
            TP_DECIMATE_FACTOR,
            TP_INPUT_WINDOW_VSIZE,
            TP_API,
        )
    else:
       checkRepeatFactor = isValid

    if (TP_INPUT_WINDOW_VSIZE % lcm_ws == 0): check_lcm=isValid 
    else: check_lcm=isError(f"TP_INPUT_WINDOW_VSIZE should be a multiple of {lcm_ws}")

    for check in (checkRepeatFactor, check_lcm):
        if check["is_valid"] == False:
            return check
    
    param_dict= fn_update_TP_INPUT_WINDOW_VSIZE(
        TT_DATA,
        TT_COEFF,
        TP_FIR_LEN,
        TP_INTERPOLATE_FACTOR,
        TP_DECIMATE_FACTOR,
        TP_INPUT_WINDOW_VSIZE,
        TP_API,
        TP_SSR,
        TP_PARA_INTERP_POLY,
        TP_PARA_DECI_POLY,
        AIE_VARIANT)
    range_ws=[param_dict["minimum"], param_dict["maximum"]]
    return validate_range(range_ws, "TP_INPUT_WINDOW_VSIZE", TP_INPUT_WINDOW_VSIZE)

# Values are derived from experimentation and are a factor of program memory limits, memory module sizes etc.
def fn_max_fir_len_overall(TT_DATA, TT_COEFF):
    maxTaps = {
        ("int16", "int16"): 4096,
        ("cint16", "int16"): 4096,
        ("cint16", "cint16"): 2048,
        ("int32", "int16"): 4096,
        ("int32", "int32"): 2048,
        ("int16", "int32"): 2048,
        ("cint16", "int32"): 2048,
        ("cint16", "cint32"): 1024,
        ("cint32", "int16"): 2048,
        ("cint32", "cint16"): 2048,
        ("cint32", "int32"): 2048,
        ("cint32", "cint32"): 1024,
        ("float", "float"): 2048,
        ("cfloat", "float"): 2048,
        ("cfloat", "cfloat"): 1024,
    }
    return maxTaps[(TT_DATA, TT_COEFF)]

def fn_ws_update_check_repeatFactor(
    TT_DATA,
    TT_COEFF,
    TP_INTERPOLATE_FACTOR,
    TP_DECIMATE_FACTOR,
    TP_INPUT_WINDOW_VSIZE,
    TP_API,
    ws_min_val,
    ws_max_val
):
    m_kVOutSize = fnNumLanes(TT_DATA, TT_COEFF, TP_API)
    m_kPolyphaseLaneAlias = getPhaseAlias(
        TT_DATA, TT_COEFF, TP_INTERPOLATE_FACTOR, TP_DECIMATE_FACTOR, TP_API
    )
    m_kNumOutputs = (TP_INPUT_WINDOW_VSIZE * TP_INTERPOLATE_FACTOR) / TP_DECIMATE_FACTOR
    m_kLsize = m_kNumOutputs / (m_kPolyphaseLaneAlias * m_kVOutSize)

    m_kNumOutputs_min_val=(ws_min_val * TP_INTERPOLATE_FACTOR) / TP_DECIMATE_FACTOR
    m_kNumOutputs_max_val=(ws_max_val * TP_INTERPOLATE_FACTOR) / TP_DECIMATE_FACTOR
    m_kLsize_min_val = m_kNumOutputs_min_val / (m_kPolyphaseLaneAlias * m_kVOutSize)
    m_kLsize_max_val = m_kNumOutputs_max_val / (m_kPolyphaseLaneAlias * m_kVOutSize)

    # 128/256/384 bits of data may be needed for loops with full
    # permute option, requiring up to 8 repeated iteratios to go through full data
    # buffer.
    m_kRepeatFactor = 8

    if TP_API == 1 and m_kLsize % m_kRepeatFactor != 0:
        m_kLsize_act = int(round(m_kLsize/m_kRepeatFactor) * m_kRepeatFactor)
        if m_kLsize_act < m_kLsize_min_val:
           m_kLsize_act = int(CEIL(m_kLsize_act, m_kRepeatFactor))
        if m_kLsize_act < m_kLsize_max_val:
           m_kLsize_act = int(FLOOR(m_kLsize_act, m_kRepeatFactor))

        m_kNumOutputs_act = m_kLsize_act*(m_kPolyphaseLaneAlias * m_kVOutSize)
        TP_INPUT_WINDOW_VSIZE_act = int((m_kNumOutputs_act*TP_DECIMATE_FACTOR)/TP_INTERPOLATE_FACTOR)
    else: TP_INPUT_WINDOW_VSIZE_act = TP_INPUT_WINDOW_VSIZE
    return TP_INPUT_WINDOW_VSIZE_act


def fn_check_repeatFactor(
    TT_DATA,
    TT_COEFF,
    TP_INTERPOLATE_FACTOR,
    TP_DECIMATE_FACTOR,
    TP_INPUT_WINDOW_VSIZE,
    TP_API,
):
    m_kVOutSize = fnNumLanes(TT_DATA, TT_COEFF, TP_API)
    m_kPolyphaseLaneAlias = getPhaseAlias(
        TT_DATA, TT_COEFF, TP_INTERPOLATE_FACTOR, TP_DECIMATE_FACTOR, TP_API
    )
    m_kNumOutputs = (TP_INPUT_WINDOW_VSIZE * TP_INTERPOLATE_FACTOR) / TP_DECIMATE_FACTOR
    m_kLsize = m_kNumOutputs / (m_kPolyphaseLaneAlias * m_kVOutSize)

    # 128/256/384 bits of data may be needed for loops with full
    # permute option, requiring up to 8 repeated iteratios to go through full data
    # buffer.
    m_kRepeatFactor = 8

    if TP_API == 1 and m_kLsize % m_kRepeatFactor != 0:
        isError(
            "For optimal design, inner loop size must schedule multiple iterations of vector operations. Please use a Input window size that results in a m_kLsize being a multiple of m_kRepeatFactor."
        )
    return isValid

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
    TP_INTERPOLATE_FACTOR = args["TP_INTERPOLATE_FACTOR"]
    TP_DECIMATE_FACTOR = args["TP_DECIMATE_FACTOR"]
    TP_API = args["TP_API"]
    TP_DUAL_IP = args["TP_DUAL_IP"]
    TP_PARA_DECI_POLY = args["TP_PARA_DECI_POLY"]
    TP_PARA_INTERP_POLY = args["TP_PARA_INTERP_POLY"]
    TP_NUM_OUTPUTS = args["TP_NUM_OUTPUTS"]
    margin_size = sr_asym.fn_margin_size(
        (TP_FIR_LEN + TP_INTERPOLATE_FACTOR - 1) // TP_INTERPOLATE_FACTOR, TT_DATA
    )

    num_in_ports = TP_SSR * TP_PARA_DECI_POLY
    num_out_ports = TP_SSR * TP_PARA_INTERP_POLY

    in_win_size = get_input_window_size(TP_INPUT_WINDOW_VSIZE, num_in_ports, TP_API, TP_DUAL_IP)
    out_win_size = get_output_window_size(TP_INPUT_WINDOW_VSIZE, num_out_ports, TP_API, TP_NUM_OUTPUTS, TP_DECIMATE_FACTOR, TP_INTERPOLATE_FACTOR)

    in_ports = get_port_info("in", "in", TT_DATA, in_win_size, num_in_ports, marginSize=margin_size, TP_API=TP_API)
    in2_ports = (get_port_info( "in2", "in", TT_DATA, in_win_size, num_in_ports, marginSize=margin_size, TP_API=TP_API) if (args["TP_DUAL_IP"] == 1) else [] )
    coeff_ports = (get_parameter_port_info("coeff", "in", TT_COEFF, TP_SSR, TP_FIR_LEN, "async") if (args["TP_USE_COEFF_RELOAD"] == 1) else [])

    # interp by 2 for halfband
    out_ports = get_port_info( "out", "out", TT_DATA, out_win_size, num_out_ports, TP_API=TP_API,)
    out2_ports = (get_port_info( "out2", "out", TT_DATA, out_win_size, num_out_ports, TP_API=TP_API, ) if (args["TP_NUM_OUTPUTS"] == 2) else [])

    return in_ports + in2_ports + coeff_ports + out_ports + out2_ports


#### graph generator ####
# def get_param_list(**kwargs):
#  print("get_param_list")
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
    TP_DECIMATE_FACTOR = args["TP_DECIMATE_FACTOR"]
    TP_USE_COEFF_RELOAD = args["TP_USE_COEFF_RELOAD"]
    TP_NUM_OUTPUTS = args["TP_NUM_OUTPUTS"]
    TP_DUAL_IP = args["TP_DUAL_IP"]
    TP_API = args["TP_API"]
    TP_SSR = args["TP_SSR"]
    coeff_list = args["coeff"]
    TP_PARA_DECI_POLY = args["TP_PARA_DECI_POLY"]
    TP_PARA_INTERP_POLY = args["TP_PARA_INTERP_POLY"]
    TP_SAT = args["TP_SAT"]

    taps = sr_asym.fn_get_taps_vector(TT_COEFF, coeff_list)
    constr_args_str = f"taps" if TP_USE_COEFF_RELOAD == 0 else ""
    dual_ip_declare_str = (
        f"ssr_port_array<input, IN_SSR> in2;" if TP_DUAL_IP == 1 else "// No dual input"
    )
    dual_ip_connect_str = (
        f"connect<>(in2[i], filter.in2[i]);"
        if TP_DUAL_IP == 1
        else "// No dual input"
    )
    coeff_ip_declare_str = (
        f"ssr_port_array<input, RTP_SSR> coeff;"
        if TP_USE_COEFF_RELOAD == 1
        else "//No coeff port"
    )
    coeff_ip_connect_str = (
        f"""for (int i = 0; i < RTP_SSR; i++) {{
            connect<>(coeff[i], filter.coeff[i]);
        }}"""
        if TP_USE_COEFF_RELOAD == 1
        else "//No coeff port"
    )
    dual_op_declare_str = (
        f"ssr_port_array<output, OUT_SSR> out2;" if TP_NUM_OUTPUTS == 2 else "// No dual output"
    )
    dual_op_connect_str = (
        f"connect<>(filter.out2[i], out2[i]);"
        if TP_NUM_OUTPUTS == 2
        else "// No dual output"
    )

    IN_SSR = TP_SSR * TP_PARA_DECI_POLY
    OUT_SSR = TP_SSR * TP_PARA_INTERP_POLY
    RTP_SSR = TP_SSR * TP_PARA_INTERP_POLY
    # Use formatted multi-line string to avoid a lot of \n and \t
    code = f"""
using namespace adf;
class {graphname} : public adf::graph {{
public:
    static constexpr unsigned int IN_SSR = {IN_SSR};
    static constexpr unsigned int RTP_SSR = {RTP_SSR};
    static constexpr unsigned int OUT_SSR = {OUT_SSR};


    template <typename dir, unsigned int num_ports>
    using ssr_port_array = std::array<adf::port<dir>, num_ports>;

    ssr_port_array<input, IN_SSR> in;
    {dual_ip_declare_str}
    {coeff_ip_declare_str}
    ssr_port_array<output, OUT_SSR> out;
    {dual_op_declare_str}

    std::vector<{TT_COEFF}> taps = {taps};
    xf::dsp::aie::fir::resampler::fir_resampler_graph<
      {TT_DATA}, //TT_DATA
      {TT_COEFF}, //TT_COEFF
      {TP_FIR_LEN}, //TP_FIR_LEN
      {TP_INTERPOLATE_FACTOR}, //TP_INTERPOLATE_FACTOR
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
      {TP_PARA_INTERP_POLY}, //TP_PARA_INTERP_POLY
      {TP_PARA_DECI_POLY}, //TP_PARA_DECI_POLY
      {TP_SAT} //TP_SAT
    > filter;

    {graphname}() : filter({constr_args_str}) {{
        kernel *filter_kernels = filter.getKernels();
        for (int i=0; i < 1; i++) {{
          runtime<ratio>(filter_kernels[i]) = 0.9;
        }}

        for (unsigned int i = 0; i < IN_SSR; ++i) {{
            // Size of window in Bytes.
            connect<>(in[i], filter.in[i]);
            {dual_ip_connect_str}
        }}

        for (unsigned int i = 0; i < OUT_SSR; ++i) {{
            connect<>(filter.out[i], out[i]);
            {dual_op_connect_str}
        }}

        {coeff_ip_connect_str}
    }}

}};
"""
    out = {}
    out["graph"] = code
    out["port_info"] = info_ports(args)
    out["headerfile"] = "fir_resampler_graph.hpp"
    out["searchpaths"] = [
        "L2/include/aie",
        "L2/tests/aie/common/inc",
        "L1/include/aie",
        "L1/src/aie",
        "L1/tests/aie/inc",
        "L1/tests/aie/src",
    ]

    return out


