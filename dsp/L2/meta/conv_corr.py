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

FUNCT_CORR = 0
FUNCT_CONV = 1
COMPUTE_FULL = 0
COMPUTE_SAME = 1
COMPUTE_VALID = 2
AIE_LOAD_SIZE = 256/8   # 32 Bytes
AIE_LOAD_SIZE_IN_BITS = 256
TP_SHIFT_min = 0
TP_SHIFT_max = 60
API_WINDOW = 0
API_STREAM = 1
TP_NUM_FRAMES_MIN = 1
TP_NUM_FRAMES_MAX = 1
TP_CASC_LEN_MIN = 1
TP_CASC_LEN_MAX = 32
TP_PHASES_MIN = 1
TP_PHASES_MAX = 16
TP_G_LEN_iobuffer_max = 256

sampleSize = {
    "int8"  : 8,
    "int16" : 16,
    "int32" : 32,
    "cint16": 32,
    "cint32": 64,
    "float" : 32,
    "cfloat": 64,
    "bfloat16" : 16
}

#######################################################
########### AIE_VARIANT Updater and Validator #########
#######################################################
def update_AIE_VARIANT(args):
  return fn_update_AIE_VARIANT()

def fn_update_AIE_VARIANT():
  legal_set_AIE_VARIANT = [com.AIE, com.AIE_ML]
  
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
  return(com.validate_legal_set(legal_set_AIE_VARIANT, "AIE_VARIANT", AIE_VARIANT))

#######################################################
############# TP_API Updater and Validator ############
#######################################################
def update_TP_API(args):
  return fn_update_api()

def fn_update_api():
  param_dict={
    "name" : "TP_API",
    "enum" : [com.API_WINDOW, com.API_STREAM]
  }
  return param_dict

def validate_TP_API(args):
  TP_API = args["TP_API"]
  return fn_validate_api(TP_API)

def fn_validate_api(TP_API):
  param_dict = fn_update_api()
  return(com.validate_legal_set(param_dict["enum"], "TP_API", TP_API))

#######################################################
########## TT_DATA_F Updater and Validator ############
#######################################################
def update_TT_DATA_F(args):
  AIE_VARIANT = args["AIE_VARIANT"]
  TP_API = args["TP_API"]
  return fn_update_data_type_f(AIE_VARIANT, TP_API)

def fn_update_data_type_f(AIE_VARIANT, TP_API):
  valid_types = ["int8", "int16", "int32", "float", "bfloat16", "cfloat", "cint16", "cint32"]
  if AIE_VARIANT == com.AIE:
    if TP_API == com.API_STREAM:
       valid_types = ["cint16"]
    else:
      valid_types.remove("int8")
      valid_types.remove("bfloat16")

  param_dict={
    "name" : "TT_DATA_F",
    "enum" : valid_types
  }
  return param_dict

def validate_TT_DATA_F(args):
  TT_DATA_F = args["TT_DATA_F"]
  AIE_VARIANT = args["AIE_VARIANT"]
  TP_API = args["TP_API"]
  return fn_validate_data_type_f(TT_DATA_F, AIE_VARIANT, TP_API)

def fn_validate_data_type_f(TT_DATA_F, AIE_VARIANT, TP_API):
  param_dict = fn_update_data_type_f(AIE_VARIANT, TP_API)
  return (com.validate_legal_set(param_dict["enum"], "TT_DATA_F", TT_DATA_F))

#######################################################
########## TT_DATA_G Updater and Validator ############
#######################################################
def update_TT_DATA_G(args):
  AIE_VARIANT = args["AIE_VARIANT"]
  TT_DATA_F = args["TT_DATA_F"]
  TP_API = args["TP_API"]
  return fn_update_data_type_g(AIE_VARIANT, TT_DATA_F, TP_API)

def fn_update_data_type_g(AIE_VARIANT, TT_DATA_F, TP_API):
  valid_types = fn_get_valid_g_types_for_mul(TT_DATA_F, AIE_VARIANT, TP_API)
  param_dict={
    "name" : "TT_DATA_G",
    "enum" : valid_types
  }
  return param_dict

def validate_TT_DATA_G(args):
  TT_DATA_F = args["TT_DATA_F"]
  TT_DATA_G = args["TT_DATA_G"]
  AIE_VARIANT = args["AIE_VARIANT"]
  TP_API = args["TP_API"]
  return fn_validate_data_type_g(TT_DATA_G, TT_DATA_F, AIE_VARIANT, TP_API)

def fn_validate_data_type_g(TT_DATA_G, TT_DATA_F, AIE_VARIANT, TP_API):
  param_dict = fn_update_data_type_g(AIE_VARIANT, TT_DATA_F, TP_API)
  return (com.validate_legal_set(param_dict["enum"], "TT_DATA_G", TT_DATA_G))

#######################################################
######### TT_DATA_OUT Updater and Validator ###########
#######################################################
def update_TT_DATA_OUT(args):
  TT_DATA_F = args["TT_DATA_F"]
  TT_DATA_G = args["TT_DATA_G"]
  return fn_update_data_type_out(TT_DATA_F, TT_DATA_G)

def fn_update_data_type_out(TT_DATA_F, TT_DATA_G):
  valid_types = fn_get_valid_out_types_for_mul(TT_DATA_F, TT_DATA_G)
  param_dict={
    "name" : "TT_DATA_OUT",
    "enum" : valid_types
  }
  return param_dict

def validate_TT_DATA_OUT(args):
  TT_DATA_F = args["TT_DATA_F"]
  TT_DATA_G = args["TT_DATA_G"]
  TT_DATA_OUT = args["TT_DATA_OUT"]
  return fn_validate_data_type_out(TT_DATA_OUT, TT_DATA_F, TT_DATA_G)

def fn_validate_data_type_out(TT_DATA_OUT, TT_DATA_F, TT_DATA_G):
  param_dict = fn_update_data_type_out(TT_DATA_F, TT_DATA_G)
  return (com.validate_legal_set(param_dict["enum"], "TT_DATA_OUT", TT_DATA_OUT))

#######################################################
######### TP_FUNCT_TYPE Updater and Validator #########
#######################################################
def update_TP_FUNCT_TYPE(args):
  return fn_update_funct_type()

def fn_update_funct_type():
  param_dict={
    "name" : "TP_FUNCT_TYPE",
    "enum" : [FUNCT_CORR, FUNCT_CONV]
  }
  return param_dict

def validate_TP_FUNCT_TYPE(args):
  TP_FUNCT_TYPE = args["TP_FUNCT_TYPE"]
  return fn_validate_funct_type(TP_FUNCT_TYPE)

def fn_validate_funct_type(TP_FUNCT_TYPE):
  param_dict = fn_update_funct_type()
  return(com.validate_legal_set(param_dict["enum"], "TP_FUNCT_TYPE", TP_FUNCT_TYPE))  

#######################################################
######## TP_COMPUTE_MODE Updater and Validator ########
#######################################################
def update_TP_COMPUTE_MODE(args):
  TP_API = args["TP_API"]
  return fn_update_compute_mode(TP_API)

def fn_update_compute_mode(TP_API):
  if TP_API == com.API_STREAM:
    param_dict={
        "name" : "TP_COMPUTE_MODE",
        "enum" : [COMPUTE_VALID]
    }

  elif TP_API == com.API_WINDOW:
    param_dict={
        "name" : "TP_COMPUTE_MODE",
        "enum" : [COMPUTE_FULL, COMPUTE_SAME, COMPUTE_VALID]
    }
  return param_dict

def validate_TP_COMPUTE_MODE(args):
  TP_COMPUTE_MODE = args["TP_COMPUTE_MODE"]
  TP_API = args["TP_API"]
  return fn_validate_compute_mode(TP_COMPUTE_MODE, TP_API)

def fn_validate_compute_mode(TP_COMPUTE_MODE, TP_API):
  param_dict = fn_update_compute_mode(TP_API)
  return(com.validate_legal_set(param_dict["enum"], "TP_COMPUTE_MODE", TP_COMPUTE_MODE))  

#######################################################
########### TP_F_LEN Updater and Validator ############
#######################################################
def update_TP_F_LEN(args):
  AIE_VARIANT = args["AIE_VARIANT"]
  TP_API = args["TP_API"]
  TT_DATA_F = args["TT_DATA_F"]
  TP_F_LEN = args["TP_F_LEN"] if args["TP_F_LEN"] else 0
  return fn_update_f_len(TP_F_LEN, TT_DATA_F, TP_API, AIE_VARIANT)

def fn_update_f_len(TP_F_LEN, TT_DATA_F, TP_API, AIE_VARIANT):
  elems_per_load = com.k_max_read_write_bytes[AIE_VARIANT] // com.fn_size_by_byte(TT_DATA_F)
  # TP_F_LEN_max = com.k_data_memory_bytes[AIE_VARIANT] // (4 * com.fn_size_by_byte(TT_DATA_F)) # ! Likely limited beyond what is required.
  TP_F_LEN_max = 32768 // (4 * com.fn_size_by_byte(TT_DATA_F))  # ! Note to change this to depend on AIE variant.

  param_dict={
    "name" : "TP_F_LEN",
    "minimum" : elems_per_load,
    "maximum" : TP_F_LEN_max if TP_API == com.API_WINDOW else 2**31
  }
  TP_F_LEN_act = TP_F_LEN + (TP_F_LEN % elems_per_load)
  if TP_F_LEN_act < param_dict["minimum"]: param_dict["actual"] = param_dict["minimum"]
  elif TP_F_LEN_act > param_dict["maximum"]: param_dict["actual"] = param_dict["maximum"]
  else: param_dict["actual"] = TP_F_LEN_act
  return param_dict

def validate_TP_F_LEN(args):
  AIE_VARIANT = args["AIE_VARIANT"]
  TP_API = args["TP_API"]
  TT_DATA_F = args["TT_DATA_F"]
  TP_F_LEN = args["TP_F_LEN"]
  return fn_validate_f_len(AIE_VARIANT, TT_DATA_F, TP_API, TP_F_LEN)

def fn_validate_f_len(AIE_VARIANT, TT_DATA_F, TP_API, TP_F_LEN):
  elems_per_load = com.k_max_read_write_bytes[AIE_VARIANT] // com.fn_size_by_byte(TT_DATA_F)
  param_dict = fn_update_f_len(TP_F_LEN, TT_DATA_F, TP_API, AIE_VARIANT)
  range_TP_F_LEN = [param_dict["minimum"], param_dict["maximum"]]

  if TP_F_LEN % elems_per_load != 0:
    return com.isError(f"TP_F_LEN should be divisible by {elems_per_load}.")
  
  return (com.validate_range(range_TP_F_LEN, "TP_F_LEN", TP_F_LEN))
  
#######################################################
########### TP_G_LEN Updater and Validator ############
#######################################################
def update_TP_G_LEN(args):
  AIE_VARIANT = args["AIE_VARIANT"]
  TP_API = args["TP_API"]
  TT_DATA_G = args["TT_DATA_G"]
  TP_F_LEN = args["TP_F_LEN"]
  TP_G_LEN = args["TP_G_LEN"] if args["TP_G_LEN"] else 0
  return fn_update_g_len(TP_G_LEN, TT_DATA_G, TP_F_LEN, TP_API, AIE_VARIANT)

def fn_update_g_len(TP_G_LEN, TT_DATA_G, TP_F_LEN, TP_API, AIE_VARIANT):
  elems_per_load = com.k_max_read_write_bytes[AIE_VARIANT] // com.fn_size_by_byte(TT_DATA_G)

  param_dict={
    "name" : "TT_DATA_G",
    "minimum" : elems_per_load,
    "maximum" : min(TP_G_LEN_iobuffer_max, TP_F_LEN) if TP_API == API_STREAM else TP_F_LEN
  }
  TP_G_LEN_act = TP_G_LEN + (TP_G_LEN % elems_per_load)
  if TP_G_LEN_act < param_dict["minimum"]: param_dict["actual"] = param_dict["minimum"]
  elif TP_G_LEN_act > param_dict["maximum"]: param_dict["actual"] = param_dict["maximum"]
  else: param_dict["actual"] = TP_G_LEN_act
  return param_dict

def validate_TP_G_LEN(args):
  AIE_VARIANT = args["AIE_VARIANT"]
  TP_API = args["TP_API"]
  TT_DATA_G = args["TT_DATA_G"]
  TP_F_LEN = args["TP_G_LEN"]
  TP_G_LEN = args["TP_G_LEN"] if args["TP_G_LEN"] else 0
  return fn_validate_g_len(AIE_VARIANT, TP_API, TT_DATA_G, TP_F_LEN, TP_G_LEN)

def fn_validate_g_len(AIE_VARIANT, TP_API, TT_DATA_G, TP_F_LEN, TP_G_LEN):
  elems_per_load = com.k_max_read_write_bytes[AIE_VARIANT] // com.fn_size_by_byte(TT_DATA_G)
  param_dict = fn_update_g_len(TP_G_LEN, TT_DATA_G, TP_F_LEN, TP_API, AIE_VARIANT)
  range_TP_G_LEN = [param_dict["minimum"], param_dict["maximum"]]

  if TP_G_LEN % elems_per_load != 0:
    return com.isError(f"TP_G_LEN should be divisible by {elems_per_load}.")
  if TP_G_LEN > TP_F_LEN:
    return com.isError(f"TP_G_LEN cannot be greater than TP_F_LEN.")  # ! Assertion is unnecessary but descriptive.
  
  return (com.validate_range(range_TP_G_LEN, "TP_G_LEN", TP_G_LEN))

#######################################################
########## TP_NUM_FRAMES Updater and Validator ########
#######################################################
def update_TP_NUM_FRAMES(args):
  return fn_update_num_frames()

def fn_update_num_frames():
  param_dict={
    "name" : "TP_NUM_FRAMES",
    "minimum" : TP_NUM_FRAMES_MIN,
    "maximum" : TP_NUM_FRAMES_MAX
  }
  return param_dict

def validate_TP_NUM_FRAMES(args):
  TP_NUM_FRAMES = args["TP_NUM_FRAMES"]
  return fn_validate_num_frames(TP_NUM_FRAMES)

def fn_validate_num_frames(TP_NUM_FRAMES):
  param_dict = fn_update_num_frames()
  range_TP_NUM_FRAMES = [param_dict["minimum"], param_dict["maximum"]]
  return com.validate_range(range_TP_NUM_FRAMES, "TP_NUM_FRAMES", TP_NUM_FRAMES)

#######################################################
########## TP_CASC_LEN Updater and Validator ##########
#######################################################
def update_TP_CASC_LEN(args):
  TP_G_LEN = args["TP_G_LEN"]
  TP_API = args["TP_API"]
  return fn_update_casc_len(TP_G_LEN, TP_API)

def fn_update_casc_len(TP_G_LEN, TP_API):
  legal_set = []

  if TP_API == com.API_STREAM:
    legal_set = [TP_G_LEN // 32, TP_G_LEN // 16, TP_G_LEN // 8] # ? 1 not legal?
  elif TP_API == com.API_WINDOW:
    legal_set = [1]

  legal_set = [val for val in legal_set if val >= TP_CASC_LEN_MIN]
  legal_set = [val for val in legal_set if val <= TP_CASC_LEN_MAX]

  param_dict = {"name": "TP_CASC_LEN", "enum": legal_set}
  return param_dict

def validate_TP_CASC_LEN(args):
  TP_CASC_LEN = args["TP_CASC_LEN"]
  TP_G_LEN = args["TP_G_LEN"]
  TP_API = args["TP_API"]
  return fn_validate_casc_len(TP_CASC_LEN, TP_G_LEN, TP_API)

def fn_validate_casc_len(TP_CASC_LEN, TP_G_LEN, TP_API):
  param_dict = fn_update_casc_len(TP_G_LEN, TP_API)

  legal_set_casc_len = param_dict["enum"]
  return(com.validate_legal_set(legal_set_casc_len, "TP_CASC_LEN", TP_CASC_LEN))

#######################################################
########## TP_PHASES Updater and Validator ############
#######################################################
def update_TP_PHASES(args):
  TP_CASC_LEN = args["TP_CASC_LEN"]
  TP_G_LEN = args["TP_G_LEN"]
  TP_API = args["TP_API"]
  return fn_update_phases(TP_G_LEN, TP_API, TP_CASC_LEN)

def fn_update_phases(TP_G_LEN, TP_API, TP_CASC_LEN):
  legal_set = [i for i in range(TP_PHASES_MIN, TP_PHASES_MAX+1)]
  legal_set = [i for i in legal_set if com.fn_is_power_of_two(i)]

  if (TP_API == com.API_STREAM) and (TP_CASC_LEN != TP_G_LEN // 8):
    legal_set = [1]
  elif TP_API == com.API_WINDOW:
    legal_set = [1]

  param_dict = {"name": "TP_PHASES", "enum": legal_set}
  return param_dict

def validate_TP_PHASES(args):
  TP_PHASES = args["TP_PHASES"]
  TP_G_LEN = args["TP_G_LEN"]
  TP_API = args["TP_API"]
  TP_CASC_LEN = args["TP_CASC_LEN"]
  return fn_validate_phases(TP_PHASES, TP_G_LEN, TP_API, TP_CASC_LEN)

def fn_validate_phases(TP_PHASES, TP_G_LEN, TP_API, TP_CASC_LEN):
  param_dict = fn_update_phases(TP_G_LEN, TP_API, TP_CASC_LEN)

  legal_set_phases = param_dict["enum"]
  return(com.validate_legal_set(legal_set_phases, "TP_PHASES", TP_PHASES))

#######################################################
############### TP_SHIFT Updater and Validator ########
#######################################################
def update_TP_SHIFT(args):
  AIE_VARIANT = args["AIE_VARIANT"]
  TT_DATA_OUT = args["TT_DATA_OUT"]
  return fn_update_TP_SHIFT(AIE_VARIANT, TT_DATA_OUT)

def fn_update_TP_SHIFT(AIE_VARIANT, TT_DATA):
    range_TP_SHIFT = com.fn_update_range_TP_SHIFT(AIE_VARIANT, TT_DATA)

    param_dict={
        "name" : "TP_SHIFT",
        "minimum" : range_TP_SHIFT[0],
        "maximum" : range_TP_SHIFT[1]
    }
    return param_dict

def validate_TP_SHIFT(args):
    AIE_VARIANT = args["AIE_VARIANT"]
    TT_DATA_OUT = args["TT_DATA_OUT"]
    TP_SHIFT = args["TP_SHIFT"]
    return fn_validate_shift_val(AIE_VARIANT, TT_DATA_OUT, TP_SHIFT)

def fn_validate_shift_val(AIE_VARIANT, TT_DATA_OUT, TP_SHIFT):
  param_dict = fn_update_TP_SHIFT(AIE_VARIANT, TT_DATA_OUT)
  range_TP_SHIFT = [param_dict["minimum"], param_dict["maximum"]]
  return com.validate_range(range_TP_SHIFT, "TP_SHIFT", TP_SHIFT)

#######################################################
############# TP_RND Updater and Validator ############
#######################################################
def update_TP_RND(args):
  AIE_VARIANT = args["AIE_VARIANT"]
  return fn_update_TP_RND(AIE_VARIANT)

def fn_update_TP_RND(AIE_VARIANT):
  legal_set_TP_RND = com.fn_get_legalSet_roundMode(AIE_VARIANT)
  param_dict={
    "name" : "TP_RND",
    "enum" : legal_set_TP_RND
  }
  return param_dict

def validate_TP_RND(args):
    AIE_VARIANT = args["AIE_VARIANT"]
    TP_RND = args["TP_RND"]
    return com.fn_validate_roundMode(TP_RND, AIE_VARIANT)

#######################################################
############ TP_SAT Updater and Validator #############
#######################################################  
def update_TP_SAT(args):
  legal_set_sat = com.fn_legal_set_sat()
  param_dict={
    "name" : "TP_SAT",
    "enum" : legal_set_sat
  }
  return param_dict
                               
def validate_TP_SAT(args):
  TP_SAT = args["TP_SAT"]
  return com.fn_validate_satMode(TP_SAT)


#######################################################

# Example of updater.
#
# Updater are functions to help GUI to hint user on parameter setting with already given parameters.
# The return object will provide "value" which will be set in the wizard as the dependent parameter is being set.
# The rest of keys are similar to parameter definition, but with candidates of enum or range values refined
# based on previously set values.
#
# An updator function will always return a dictionary,
# including key "value" for automatically filled default in GUI as dependent parameters have been set, and
# other keys for overriding the definition of parameter.
#
# For example, if a parameter has definition in JSON as
#  { "name": "foo", "type": "typename", "enum": ["int", "float", "double"] }
# And the updator returns
#  { "value": "int", "enum": ["int", "float"] }
# The GUI would show "int" as default and make "int" and "float" selectable candidates, while disabling "double".
#
# If with given combination, no valid value can be set for the parameter being updated, the updater function
# should set "value" to None, to indicate an error and provide error message via "err_message".
# For example
#  { "value": None, "err_message": "With TT_DATA as 'int' there is no valid option for TT_COEFF" }
#
# In this example, the following is the updater for TT_COEF, with TT_DATA as the dependent parameter.
# When GUI generates a wizard, TT_DATA should be required first, as it shows up in parameter list first.
# Once user has provided value for TT_DATA, this function will be called and set the value of TT_COEFF.
# Meanwhile, the candidate shown in wizard based on enum will also be updated.
#

# Utility Functions
#### Valid Sliding Mul Combos ####
def fn_get_valid_g_types_for_mul(TT_DATA_F, AIE_VARIANT, TP_API):
    if AIE_VARIANT == com.AIE:
        if (TP_API == API_STREAM):
            if TT_DATA_F == "cint16": return ["cint16"]

        elif (TP_API == API_WINDOW):    
            if (TT_DATA_F == "int16"):  return ["int16"]
            if (TT_DATA_F == "int32"):  return ["int16"]
            if (TT_DATA_F == "float"):  return ["float"]
            if (TT_DATA_F == "cint16"): return ["int16", "int32", "cint16"]
            if (TT_DATA_F == "cint32"): return ["int16", "cint16"]
            if (TT_DATA_F == "cfloat"): return ["float", "cfloat"]

    elif AIE_VARIANT == com.AIE_ML:
        if (TT_DATA_F == "int8"):       return ["int8"]
        if (TT_DATA_F == "int16"):      return ["int16"]
        if (TT_DATA_F == "int32"):      return ["int16"]
        if (TT_DATA_F == "float"):      return ["float"]
        if (TT_DATA_F == "bfloat16"):   return ["bfloat16"]
        if (TT_DATA_F == "cint16"):     return ["int16", "int32", "cint16"]
        if (TT_DATA_F == "cint32"):     return ["int16", "cint16"]
    return []

def fn_get_valid_out_types_for_mul(TT_DATA_F, TT_DATA_G):   # Don't feel too good with these functions but it's where we're at.
    if (TT_DATA_F == "int8")        and (TT_DATA_G == "int8"):      return ["int16"]
    if (TT_DATA_F == "int16")       and (TT_DATA_G == "int8"):      return ["int16"]
    if (TT_DATA_F == "int16")       and (TT_DATA_G == "int16"):     return ["int32"]
    if (TT_DATA_F == "int32")       and (TT_DATA_G == "int16"):     return ["int32"]
    if (TT_DATA_F == "cint16")      and (TT_DATA_G == "int16"):     return ["cint16", "cint32"]
    if (TT_DATA_F == "cint16")      and (TT_DATA_G == "int32"):     return ["cint32"]
    if (TT_DATA_F == "cint16")      and (TT_DATA_G == "cint16"):    return ["cint16", "cint32"]
    if (TT_DATA_F == "cint32")      and (TT_DATA_G == "int16"):     return ["cint32"]
    if (TT_DATA_F == "cint32")      and (TT_DATA_G == "cint16"):    return ["cint32"]
    if (TT_DATA_F == "float")       and (TT_DATA_G == "float"):     return ["float"]
    if (TT_DATA_F == "bfloat16")    and (TT_DATA_G == "bfloat16"):  return ["float"]
    return []

def getNumLanes(TT_DATA_F, TT_DATA_G, AIE_VARIANT=1):
    if AIE_VARIANT == 1:
        if (
            (TT_DATA_F == "int8" and TT_DATA_G == "int8") or
            (TT_DATA_F == "int16" and TT_DATA_G == "int8") or
            (TT_DATA_F == "int16" and TT_DATA_G == "int16") 
        ):
            return 16
        elif (
            (TT_DATA_F == "int32" and  TT_DATA_G == "int16") or
            (TT_DATA_F == "float" and  TT_DATA_G == "float") or
            (TT_DATA_F == "cint16" and TT_DATA_G == "int16") or
            (TT_DATA_F == "cint16" and TT_DATA_G == "int32") or
            (TT_DATA_F == "cint16" and TT_DATA_G == "cint16")
        ):
            return 8
        elif (
            (TT_DATA_F == "float" and  TT_DATA_G == "cfloat") or
            (TT_DATA_F == "cint32" and  TT_DATA_G == "int16") or
            (TT_DATA_F == "cint32" and TT_DATA_G == "cint16") or
            (TT_DATA_F == "cfloat" and TT_DATA_G == "float") or
            (TT_DATA_F == "cfloat" and TT_DATA_G == "cfloat")
            ):
            return 4
        else:
            return 0
    if AIE_VARIANT == 2:
        if (
            (TT_DATA_F == "int8" and TT_DATA_G == "int8")
           ):
            return 8
        elif (
            (TT_DATA_F == "int16" and TT_DATA_G == "int8") or
            (TT_DATA_F == "int16" and TT_DATA_G == "int16") or
            (TT_DATA_F == "int32" and TT_DATA_G == "int16") or
            (TT_DATA_F == "float" and TT_DATA_G == "float") or
            (TT_DATA_F == "bfloat16" and TT_DATA_G == "bfloat16") or
            (TT_DATA_F == "cint16" and TT_DATA_G == "int16") or
            (TT_DATA_F == "cint16" and TT_DATA_G == "int32") 
            ) :
            return 16
        elif (
            (TT_DATA_F == "cint16" and TT_DATA_G == "cint16") or
            (TT_DATA_F == "cint32" and TT_DATA_G == "int16") or
            (TT_DATA_F == "cint32" and TT_DATA_G == "cint16")
        ):
            return 8
        else:
            return 0

def fn_compute_output_dimension(TP_F_LEN, TP_G_LEN, TP_COMPUTE_MODE):
    if TP_COMPUTE_MODE == COMPUTE_FULL:
        return TP_F_LEN + TP_G_LEN - 1
    elif TP_COMPUTE_MODE == COMPUTE_VALID:
        return TP_F_LEN - TP_G_LEN + 1
    elif TP_COMPUTE_MODE == COMPUTE_SAME:
        return TP_F_LEN
    else:
        return com.isError(f"ERROR: Invalid compute mode ({TP_COMPUTE_MODE}). Must be one of [{COMPUTE_FULL}, {COMPUTE_VALID}, {COMPUTE_SAME}].")

def fn_get_input_paddedLength(TT_DATA_F, TT_DATA_G, TP_F_LEN, TP_G_LEN, TP_COMPUTE_MODE):
    PaddedLength = 0
    lanes = getNumLanes(TT_DATA_F, TT_DATA_G)
    dataSamples = sampleSize[TT_DATA_F]
    dataLoad = (AIE_LOAD_SIZE_IN_BITS / dataSamples)
    
    if TP_COMPUTE_MODE == COMPUTE_FULL:
        PaddedLength = ((TP_G_LEN - 1) + TP_F_LEN + (TP_G_LEN - 1))
        PaddedLength = com.CEIL(PaddedLength, dataLoad)
        return PaddedLength
    elif TP_COMPUTE_MODE == COMPUTE_VALID:
        if lanes > dataLoad :
            PaddedLength = (TP_F_LEN + (dataLoad << 1))
            PaddedLength = com.CEIL(PaddedLength, dataLoad)
            return PaddedLength
        else :
            PaddedLength = (TP_F_LEN + dataLoad)
            PaddedLength = com.CEIL(PaddedLength, dataLoad)
            return PaddedLength
    elif TP_COMPUTE_MODE == COMPUTE_SAME:
        PaddedLength = (((TP_G_LEN/2) - 1) + TP_F_LEN + ((TP_G_LEN/2) - 1))
        PaddedLength = com.CEIL(PaddedLength, dataLoad)
        return PaddedLength
    else:
        return com.isError(f"ERROR: Invalid compute mode ({TP_COMPUTE_MODE}) to compute the padded length. Must be one of [{COMPUTE_FULL}, {COMPUTE_VALID}, {COMPUTE_SAME}].")

########################## Ports ###########################
def get_port_info(portname, dir, dataType, dim, numFrames, numPhases, apiType, vectorLength):
    return [{
        "name" : f"{portname}[{idx}]",
        "type" : f"{apiType}",
        "direction": f"{dir}",
        "data_type": dataType,
        "fn_is_complex": com.fn_is_complex(dataType),
        "window_size" : dim*numFrames, #com.fn_input_window_size(windowVsize, dataType),
        "margin_size" : 0,
        "phases" : numPhases
    } for idx in range(vectorLength)]

def info_ports(args):
    """Standard function creating a static dictionary of information
    for upper software to correctly connect the IP.
    Some IP has a configurable number of ports according to parameter set,
    so port information has to be implemented as a function"""
    TT_DATA_F = args["TT_DATA_F"]
    TT_DATA_G = args["TT_DATA_G"]
    TT_DATA_OUT = args["TT_DATA_OUT"]
    TP_FUNCT_TYPE = args["TP_FUNCT_TYPE"]
    TP_COMPUTE_MODE = args["TP_COMPUTE_MODE"]
    TP_F_LEN = args["TP_F_LEN"]
    TP_G_LEN = args["TP_G_LEN"]
    TP_SHIFT = args["TP_SHIFT"]
    TP_API = args["TP_API"]
    TP_RND = args["TP_RND"]
    TP_SAT = args["TP_SAT"]
    TP_NUM_FRAMES = args["TP_NUM_FRAMES"]
    TP_CASC_LEN = args["TP_CASC_LEN"]
    TP_PHASES = args["TP_PHASES"]
    AIE_VARIANT = args["AIE_VARIANT"]
    
    lanes = getNumLanes(TT_DATA_F, TT_DATA_G, AIE_VARIANT)
    loopcount = fn_compute_output_dimension(TP_F_LEN, TP_G_LEN, TP_COMPUTE_MODE)
    inDataLen = fn_get_input_paddedLength(TT_DATA_F, TT_DATA_G, TP_F_LEN, TP_G_LEN, TP_COMPUTE_MODE)
    outDataLen = ((((com.CEIL(loopcount,lanes))/lanes))*lanes)
    
    if (TP_API==0) :
        portsInF = get_port_info(
            portname = "inF",
            dir = "in",
            dataType = TT_DATA_F,
            dim = inDataLen,
            numFrames = TP_NUM_FRAMES,
            numPhases = TP_PHASES,
            apiType = "window",
            vectorLength = TP_PHASES
        )
        portsInG = get_port_info(
            portname = "inG",
            dir = "in",
            dataType = TT_DATA_G,
            dim = TP_G_LEN,
            numFrames = TP_NUM_FRAMES,
            numPhases = TP_PHASES,
            apiType = "window",
            vectorLength = TP_PHASES
        )
        portsOut = get_port_info(
            portname = "out",
            dir = "out",
            dataType = TT_DATA_OUT,
            dim = outDataLen,
            numFrames = TP_NUM_FRAMES,
            numPhases = TP_PHASES,
            apiType = "window",
            vectorLength = TP_PHASES
        )
    elif (TP_API == 1):
        portsInF = get_port_info(
            portname = "inF",
            dir = "in",
            dataType = TT_DATA_F,
            dim = TP_F_LEN,
            numFrames = TP_NUM_FRAMES,
            numPhases = TP_PHASES,
            apiType = "stream",
            vectorLength = TP_PHASES
        )
        portsInG = get_port_info(
            portname = "inG",
            dir = "in",
            dataType = TT_DATA_G,
            dim = TP_G_LEN,
            numFrames = TP_NUM_FRAMES,
            numPhases = TP_PHASES,
            apiType = "stream",
            vectorLength = TP_PHASES
        )
        portsOut = get_port_info(
            portname = "out",
            dir = "out",
            dataType = TT_DATA_OUT,
            dim = fn_compute_output_dimension(TP_F_LEN, TP_G_LEN, TP_COMPUTE_MODE),
            numFrames = TP_NUM_FRAMES,
            numPhases = TP_PHASES,
            apiType = "stream",
            vectorLength = TP_PHASES
        )
        pass

    return portsInF+portsInG+portsOut

#### graph generator ####
def generate_graph(graphname, args):

    if graphname == "":
        graphname = "default_graphname"

    TT_DATA_F = args["TT_DATA_F"]
    TT_DATA_G = args["TT_DATA_G"]
    TT_DATA_OUT = args["TT_DATA_OUT"]
    TP_FUNCT_TYPE = args["TP_FUNCT_TYPE"]
    TP_COMPUTE_MODE = args["TP_COMPUTE_MODE"]
    TP_F_LEN = args["TP_F_LEN"]
    TP_G_LEN = args["TP_G_LEN"]
    TP_SHIFT = args["TP_SHIFT"]
    TP_API = args["TP_API"]
    TP_RND = args["TP_RND"]
    TP_SAT = args["TP_SAT"]
    TP_NUM_FRAMES = args["TP_NUM_FRAMES"]
    TP_CASC_LEN = args["TP_CASC_LEN"]
    TP_PHASES = args["TP_PHASES"]
    AIE_VARIANT = args["AIE_VARIANT"]
    
    # Use formatted multi-line string to avoid a lot of \n and \t
    code = (
f"""
class {graphname} : public adf::graph {{
public:
  // ports
  //template <typename dir>
  static constexpr unsigned int TP_PHASES = {TP_PHASES};
  template <typename dir>
  using ssr_port_array = std::array<adf::port<dir>, TP_PHASES>;
  
  std::array<adf::port<input>, {TP_PHASES}> inF;
  std::array<adf::port<input>, 1> inG;
  std::array<adf::port<output>, {TP_PHASES}> out;

  xf::dsp::aie::conv_corr::conv_corr_graph<
    {TT_DATA_F}, // TT_DATA_F
    {TT_DATA_G}, // TT_DATA_G
    {TT_DATA_OUT}, // TT_DATA_OUT
    {TP_FUNCT_TYPE}, // TP_FUNCT_TYPE
    {TP_COMPUTE_MODE}, // TP_COMPUTE_MODE
    {TP_F_LEN}, // TP_F_LEN
    {TP_G_LEN}, //TP_G_LEN
    {TP_SHIFT}, //TP_SHIFT
    {TP_API}, //TP_API
    {TP_RND}, //TP_RND
    {TP_SAT}, // TP_SAT
    {TP_NUM_FRAMES}, // TP_NUM_FRAMES
    {TP_CASC_LEN}, //TP_CASC_LEN
    {TP_PHASES} // TP_PHASES
    > conv_corr_graph;

  {graphname}() : conv_corr_graph() {{
    adf::kernel *conv_corr_kernels = conv_corr_graph.getKernels();
    for (int i=0; i < 1; i++) {{
      adf::runtime<ratio>(conv_corr_kernels[i]) = 0.9;
    }}
    for (int phIndx = 0; phIndx < TP_PHASES; phIndx++) {{
        adf::connect<> net_in(inF[phIndx], conv_corr_graph.inF[phIndx]);
        adf::connect<> net_out(conv_corr_graph.out[phIndx], out[phIndx]);
    }}
    adf::connect<> net_in(inG, conv_corr_graph.inG[1]);
  }}
}};
""")
    out = {}
    out["graph"] = code
    out["port_info"] = info_ports(args)
    out["headerfile"] = "conv_corr_graph.hpp"
    out["searchpaths"] = [
        "L2/include/aie",
        "L2/tests/aie/common/inc",
        "L1/include/aie",
        "L1/src/aie",
        "L1/tests/aie/inc",
        "L1/tests/aie/src"]

    return out
