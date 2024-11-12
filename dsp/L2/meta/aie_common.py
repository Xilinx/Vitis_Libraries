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
import os

AIE = 1
AIE_ML = 2
API_WINDOW = 0
API_STREAM = 1

### Data memory definitions ###
k_min_read_write_bytes = {
    AIE: 128 // 8,
    AIE_ML: 256 // 8
}
k_max_read_write_bytes = {
    AIE: 256 // 8,
    AIE_ML: 256 // 8
}

k_data_memory_bytes = {
    AIE: 32768,
    AIE_ML: 65536
}

# shortcut
isValid = {"is_valid": True}
def isError(msg) :
  return {"is_valid" : False, "err_message" : "ERROR: " + msg}

def fn_return_first_error(validList):
    if isinstance(validList, dict):
        return validList # It's not a list, just a dict.
    return next(
        (check for check in list(validList) if (check["is_valid"] == False)),
        isValid # it's valid if no errors found
    )

#### Min, Max, Factor and Power Utility functions ####
def fn_validate_minmax_value(ValueStr, Value, MinValue, MaxValue):
    if Value< MinValue or Value > MaxValue:
            return isError(f"Minimum and Maximum value for {ValueStr} is {MinValue} and {MaxValue}, respectively, but got {Value}. ")
    return isValid

def fn_validate_min_value(ValueStr, Value, MinValue):
    if Value< MinValue:
            return isError(f"Minimum value for {ValueStr} is {MinValue}, but got {Value}. ")
    return isValid

def fn_validate_max_value(ValueStr, Value, MaxValue):
    if Value > MaxValue:
            return isError(f"Maximum value for {ValueStr} is {MaxValue}, but got {Value}. ")
    return isValid

def fn_validate_multiple(ValueStr, Value, Factor):
    if Value % Factor != 0:
            return isError(f"{ValueStr} must be multiple of {Factor} but got {Value}.")
    return isValid

def fn_validate_power_of_two(ValueStr, Value):
    if (Value != 0) and ((Value & (Value-1)) == 0):
        return isValid
    return isError(f"{ValueStr} must be a power of 2, but got {Value}.")

def fn_validate_divisible(ValueStr, Value, Factor):
    if Value % Factor != 0:
            return isError(f"{ValueStr} ({Value}) must be divisible by {Factor}.")
    return isValid

# returns a rounded up version of m so that it is a multiple of n
# CEIL(13,4) = 16
def CEIL(m, n):
  return (((m+n-1)//n)*n)

def FLOOR(m, n):
  return ((m//n) * n) 

def fnTrunc(x, y):
    a = int(x)
    b = int(y)
    return (a // b) * b

#### Data Type Info ####
# TODO full list referring to AIETypes.cxx
k_complex_base_type_map = {
    "cint8": "int8",
    "cint16": "int16",
    "cint32": "int32",
    "cfloat": "float"
}

k_base_type_map = {
    "uint8": "int8",
    "int8": "int8",
    "uint16": "uint16",
    "int16": "int16",
    "int32": "int32",
    "float": "float",
    "cint16": "int16",
    "cint32": "int32",
    "cfloat": "float",
    "bfloat16": "bfloat16"
}

k_base_type_size_map = {
    "int8": 1,
    "int16": 2,
    "int32": 4,
    "float": 4,
    "bfloat16": 2
}

k_rnd_mode_map_aie2 = {
    "rnd_floor": 0,
    "rnd_ceil": 1,
    "rnd_sym_floor": 2,
    "rnd_sym_ceil": 3,
    "rnd_neg_inf": 8,
    "rnd_pos_inf": 9,
    "rnd_sym_zero": 10,
    "rnd_sym_inf": 11,
    "rnd_conv_even": 12,
    "rnd_conv_odd": 13
}

k_rnd_mode_map_aie1 = {
    "rnd_floor": 0,
    "rnd_ceil": 1,
    "rnd_pos_inf": 2,
    "rnd_neg_inf": 3,
    "rnd_sym_inf": 4,
    "rnd_sym_zero": 5,
    "rnd_conv_even": 6,
    "rnd_conv_odd": 7
}

k_integral_types = ["uint8", "int8", "int16", "int32", "cint8", "cint16", "cint32"]
k_floating_point_types = ["float", "cfloat", "bfloat16"]
k_complex_types = ["cint8", "cint16", "cint32", "cfloat"]
k_non_complex_types = ["int8", "int16", "int32", "float"]
k_integer_types = ["int8", "int16", "int32"]
#### Common Functions ####

def fn_is_integral(type):
    return type in k_integral_types

def fn_is_floating_point(type):
    return type in k_floating_point_types

def fn_is_complex(type):
    return (type in k_complex_base_type_map)

def fn_base_type(type):
    return k_complex_base_type_map.get(type, type)

def fn_size_by_byte(type):
    return (
        k_base_type_size_map[k_base_type_map[type]]
            * (2 if fn_is_complex(type) else 1)
    )

def fn_is_32b_type(type):
    # It is a 32 type when base type is 4 Bytes.
    return (k_base_type_size_map[k_base_type_map[type]] == 4)



def fn_input_window_size(TP_INPUT_WINDOW_VSIZE, TT_DATA):
    return int(TP_INPUT_WINDOW_VSIZE) * fn_size_by_byte(TT_DATA)
#### internal validation functions ####

# These seem like FIR specific functions, but they are used generally
#### validate coef type ####
def fn_complex_check(TT_DATA, TT_COEFF, TT_DATA_STR="TT_DATA", TT_COEFF_STR = "TT_COEFF"):
    if fn_is_complex(TT_COEFF) and not fn_is_complex(TT_DATA):
        return isError(f"Parameter {TT_COEFF_STR} can only be a complex type when {TT_DATA_STR} is also complex. Got {TT_DATA_STR}: {TT_DATA} and {TT_COEFF_STR}: {TT_COEFF}.")
    return isValid

def fn_coeff_32b(TT_DATA, TT_COEFF):
    type32b = ["int32", "cint32"]
    # 2023.1 now allows 32-bit coeffs with 16-bit data
    # if TT_COEFF in type32b and not TT_DATA in type32b:
        # return isError("32-bit type coefficients are only supported when Input/Output type is also a 32-bit type.")
    return isValid

def fn_int_check(TT_DATA, TT_COEFF, TT_DATA_STR="TT_DATA", TT_COEFF_STR = "TT_COEFF"):
    if not fn_is_integral(TT_COEFF) and fn_is_integral(TT_DATA):
        return isError(f"Parameter {TT_COEFF_STR} can only be an integer type when {TT_DATA_STR} is also integer. Got {TT_DATA_STR}: {TT_DATA} and {TT_COEFF_STR}: {TT_COEFF}.")
    return isValid

def fn_float_check(TT_DATA, TT_COEFF, TT_DATA_STR="TT_DATA", TT_COEFF_STR = "TT_COEFF"):
    if not fn_is_floating_point(TT_COEFF) and fn_is_floating_point(TT_DATA):
        return isError(f"Parameter {TT_COEFF_STR} can only be a float type when {TT_DATA_STR} is also float. Got {TT_DATA_STR}: {TT_DATA} and {TT_COEFF_STR}: {TT_COEFF}.")
    return isValid

def fn_32b_check(TT_DATA, TT_COEFF, TT_DATA_STR="TT_DATA", TT_COEFF_STR = "TT_COEFF"):
    if not fn_is_32b_type(TT_DATA) and fn_is_32b_type(TT_COEFF):
        return isError(f"Parameter {TT_COEFF_STR} can only have a 32-bit base type when {TT_DATA_STR} also has a 32-bit base type. Got {TT_DATA_STR}: {TT_DATA} and {TT_COEFF_STR}: {TT_COEFF}.")
    return isValid

def fn_greater_precision_check(TT_DATA, TT_COEFF, TT_DATA_STR="TT_DATA", TT_COEFF_STR = "TT_COEFF"):
    # Checks if TT_COEFF is of same or greater precision than TT_DATA.
    data_precision = k_base_type_size_map[k_base_type_map[TT_DATA]] * 8 # Byte sized map
    coeff_precision = k_base_type_size_map[k_base_type_map[TT_COEFF]] * 8 # Byte sized map
    if data_precision >  coeff_precision :
        return isError(f"Parameter's {TT_COEFF_STR} base type bit width precision ({coeff_precision}) can only be of same or greater bit as {TT_DATA_STR} base type bit width precision ({data_precision}). Got {TT_DATA_STR}: {TT_DATA} and {TT_COEFF_STR}: {TT_COEFF}.")
    return isValid

# Same rules apply across the board
def fn_validate_coeff_type(TT_DATA, TT_COEFF):
    for fn in (fn_complex_check, fn_coeff_32b, fn_int_check, fn_float_check):
        check = fn(TT_DATA, TT_COEFF)
        if check["is_valid"] == False:
            return check
    return isValid

### Validate Shift ###
def fn_float_no_shift(TT_DATA, TP_SHIFT):
    if fn_is_floating_point(TT_DATA) and (TP_SHIFT > 0):
      return isError(f"Shift cannot be performed for '{TT_DATA}' data type, so must be set to 0. Got {TP_SHIFT}.")
    return isValid

# most library element only need to check this to validate shift
def fn_validate_shift(TT_DATA, TP_SHIFT):
    if TP_SHIFT< 0 or TP_SHIFT > 61:
            return isError(f"Minimum and Maximum value for parameter Shift is 0 and 61, but got {TP_SHIFT}. ")
    return fn_float_no_shift(TT_DATA, TP_SHIFT)

# most library element only need to check this to validate saturation modes
def fn_validate_satMode(TP_SAT) :
    if (not 0 <= TP_SAT <= 3) or (TP_SAT == 2):
        return isError(f"Invalid saturation mode. Valid values for TP_SAT are 0, 1, and 3. Got {TP_SAT}")
    return isValid

def fn_get_legalSet_roundMode(AIE_VARIANT):
  if AIE_VARIANT == 1:
    legal_set_rnd= list(k_rnd_mode_map_aie1.values())
  elif AIE_VARIANT == 2:
    legal_set_rnd= list(k_rnd_mode_map_aie2.values())
  return legal_set_rnd

def fn_validate_roundMode(TP_RND, AIE_VARIANT):
  if AIE_VARIANT == 1:
    if (TP_RND not in k_rnd_mode_map_aie1.values()):
        return isError(f"Invalid rounding mode. Valid values of AIE-1 TP_RND are {0, 1, 2, 3, 4, 5, 6, 7}. Got {TP_RND}.")
  elif AIE_VARIANT == 2:
    if (TP_RND not in k_rnd_mode_map_aie2.values()):
        return isError(f"Invalid rounding mode. Valid values of of AIE-ML TP_RND are {0, 1, 2, 3, 8, 9, 10, 11, 12, 13}. Got {TP_RND}.")
  return isValid

def fn_validate_aie_variant(AIE_VARIANT):
    if AIE_VARIANT == AIE or AIE_VARIANT == AIE_ML:
      return isValid
    return isError("AIE_VARIANT invalid.")

# returns a list of port objects, vectorLength=None for no index on the portname
def get_port_info(portname, dir, TT_DATA, windowVSize, vectorLength=None, marginSize=0, TP_API=0):
  return [{
    "name" : f"{portname}[{idx}]" if vectorLength else f"{portname}", # portname no index
    "type" : "window" if TP_API==0 else "stream",
    "direction" : f"{dir}",
    "data_type" : TT_DATA,
    "fn_is_complex" : fn_is_complex(TT_DATA),
    "window_size" : fn_input_window_size(windowVSize, TT_DATA),
    "margin_size": marginSize
  } for idx in range((vectorLength if vectorLength else 1))] # do just one port if vectorLength=None

def get_parameter_port_info(portname, dir, TT_DATA, vectorLength=None, numElements=0, synchronicity="async"):
  return [{
        "name" : f"{portname}[{idx}]" if vectorLength else f"{portname}",
        "type" : "parameter",
        "direction": f"{dir}",
        "data_type": TT_DATA,
        "fn_is_complex" : fn_is_complex(TT_DATA),
        "num_elements": numElements,
        "synchronicity": synchronicity
      } for idx in range((vectorLength if vectorLength else 1))] # do just one port if vectorLength=None

def get_parameter_range_info(portname, default, minimum, maximum, vectorLength=None):
  return [{
        "name" : f"{portname}[{idx}]" if vectorLength else f"{portname}",
        "type" : "parameter",
        "default" : default,
        "minimum": minimum,
        "maximum": maximum
      } for idx in range((vectorLength if vectorLength else 1))] # do just one port if vectorLength=None

def get_parameter_enum_info(portname, default, enum_list, vectorLength=None):
  return [{
        "name" : f"{portname}[{idx}]" if vectorLength else f"{portname}",
        "type" : "parameter",
        "default" : default,
        "enum": enum_list
      } for idx in range((vectorLength if vectorLength else 1))] # do just one port if vectorLength=None

def validate_legal_set(legal_set, param_name, param_value):
    if param_value in legal_set:
        return isValid
    else:
        return isError(f"{param_name} ({param_value}) is not in the legal set of {legal_set}")

def validate_range(range, param_name, param_value):
    if (len(range) > 2):
        raise ValueError(f"Length of range list is {len(range)}. Expecting minimum and maximum values only!")  
    
    if param_value >= range[0] and param_value <= range[1]:
        return isValid
    else:
        return isError(f"{param_name} ({param_value}) is not in the range of {range}")

def remove_from_set(remove_list, original_list):
    for k in remove_list:
        if k in original_list:
            original_list.remove(k)
    return original_list

def add_to_set(add_list, original_list):
    for k in add_list:
        original_list.append(k)
    return original_list

def isComplex(param):
  if param == "float" or param == "cfloat":
    return True
  else:
    return False
  
def validate_LUT_len(lut_list, len_lut):
  if (len(lut_list) != len_lut):
    return isError(f"The lookup list array must contain {len_lut} values. However, the provided list contains {len(lut_list)}.")
  return isValid

def validate_LUT_len_range(lut_list, len_lut_min, len_lut_max):
  if (len(lut_list) >= len_lut_min and len(lut_list) <= len_lut_max):
    return isValid
  else: 
    return isError(f"The lookup list array must contain values between {len_lut_min} and {len_lut_max}. However, the provided list contains {len(lut_list)} values.")
  
def validate_min(min_val, param_name, param_value):
    if param_value >= min_val:
        return isValid
    else:
        return isError(f"{param_name} is smaller than {min_val}. It should be equal to or greater than {min_val}.")
    
def fn_range_shift(tt_data):
    valid_shift_range = {"cint16": 31, "cint32": 61, "int16": 15, "int32": 31, "cfloat": 0, "float": 0}
    range_shift=[0, valid_shift_range[tt_data]]

    return range_shift

def fn_legal_set_sat():
    return [0, 1, 3]

def round_power_of_2(input_val):
    power = 1  
    while power < input_val:  
        power *= 2  
    # Now power is the first power of two greater than or equal to n.  
    # Check if the previous power of two is closer.  
    if power - input_val > input_val - power / 2:  
        return power / 2  
    else:  
        return power  

def find_lcm(num1, num2):  
   if(num1>num2):  
       num = num1  
       den = num2  
   else:  
       num = num2  
       den = num1  
   remainder = num % den  
   while(remainder != 0):  
       num = den  
       den = remainder  
       remainder = num % den  
   gcd = den  
   lcm = int(int(num1 * num2)/int(gcd))  
   return lcm 

def find_lcm_list(nums_list):  
    prev_result=1
    for number in nums_list:
        prev_result=find_lcm(number, prev_result)
    return prev_result

def find_divisors(number, max_divisor):
    divisor_list=[]
    
    for i in range(1,max_divisor+1):
        if number%i==0:
           divisor_list.append(i) 
    
    return divisor_list

def isMultiple(A,B):
  return (A % B == 0)

def round_power_of_2(input_val):
    power = 1  
    while power < input_val:  
        power *= 2  
    # Now power is the first power of two greater than or equal to n.  
    # Check if the previous power of two is closer.  
    if power - input_val > input_val - power / 2:  
        return int(power / 2 ) 
    else:  
        return int(power) 
      
def fn_is_power_of_two(n):
  return (
    (n & (n-1) == 0) and n!=0 )


def fn_coeff_type_update(TT_DATA, legal_set):
    legal_set_int=legal_set
    legal_set_int=fn_complex_check_update(TT_DATA, legal_set_int)
    legal_set_int=fn_int_check_update(TT_DATA, legal_set_int)
    legal_set_int=fn_float_check_update(TT_DATA, legal_set_int)
    return legal_set_int

def fn_complex_check_update(TT_DATA, legal_set):
    legal_set_int=legal_set
    if not(fn_is_complex(TT_DATA)):
        legal_set_int=remove_from_set(k_complex_types, legal_set_int)
    return legal_set_int

def fn_int_check_update(TT_DATA, legal_set):
    legal_set_int=legal_set
    if not(fn_is_integral(TT_DATA)):
        legal_set_int=remove_from_set(k_integral_types, legal_set_int)
    return legal_set_int

def fn_float_check_update(TT_DATA, legal_set):
    legal_set_int=legal_set
    if not(fn_is_floating_point(TT_DATA)):
        legal_set_int=remove_from_set(k_floating_point_types, legal_set_int)
    return legal_set_int

def fn_update_range_TP_SHIFT(AIE_VARIANT, TT_DATA):
  if TT_DATA in ["float", "cfloat"]:
    range_TP_SHIFT=[0,0]
  else:
    if AIE_VARIANT ==1: range_TP_SHIFT=[0,61]
    elif AIE_VARIANT==2: range_TP_SHIFT=[0,59]
  return range_TP_SHIFT