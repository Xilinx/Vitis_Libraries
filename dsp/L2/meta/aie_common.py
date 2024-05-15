AIE = 1
AIE_ML = 2

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

# returns a rounded up version of m so that it is a multiple of n
# CEIL(13,4) = 16
def CEIL(m, n):
  return (((m+n-1)//n)*n)

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


def fn_input_window_size(TP_INPUT_WINDOW_VSIZE, TT_DATA):
    return int(TP_INPUT_WINDOW_VSIZE) * fn_size_by_byte(TT_DATA)
#### internal validation functions ####

# These seem like FIR specific functions, but they are used generally
#### validate coef type ####
def fn_complex_coef(TT_DATA, TT_COEFF):
    if fn_is_complex(TT_COEFF) and not fn_is_complex(TT_DATA):
        return isError(f"Complex coefficients are only supported when Input/Output type is also complex. Got TT_DATA {TT_DATA} and TT_COEFF {TT_COEFF}.")
    return isValid

def fn_coeff_32b(TT_DATA, TT_COEFF):
    type32b = ["int32", "cint32"]
    # 2023.1 now allows 32-bit coeffs with 16-bit data
    # if TT_COEFF in type32b and not TT_DATA in type32b:
        # return isError("32-bit type coefficients are only supported when Input/Output type is also a 32-bit type.")
    return isValid

def fn_int_coeff(TT_DATA, TT_COEFF):
    if not fn_is_integral(TT_COEFF) and fn_is_integral(TT_DATA):
        return isError(f"Coefficients must be an integer type if Input/Output type is an integer type. Got {TT_DATA} and {TT_COEFF}.")
    return isValid

def fn_float_coeff(TT_DATA, TT_COEFF):
    if not fn_is_floating_point(TT_COEFF) and fn_is_floating_point(TT_DATA):
        return isError(f"Coefficients must be a float type if Input/Output type is a float type. Got {TT_DATA} and {TT_COEFF}.")
    return isValid

# Same rules apply across the board
def fn_validate_coeff_type(TT_DATA, TT_COEFF):
    for fn in (fn_complex_coef, fn_coeff_32b, fn_int_coeff, fn_float_coeff):
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
