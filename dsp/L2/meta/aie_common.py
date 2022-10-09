
# shortcut
isValid = {"is_valid": True}
def isError(msg) :
  return {"is_valid" : False, "err_message" : msg}

def fn_return_first_error(validList): 
    return next(
        (check for check in list(validList) if (check["is_valid"] == False)),
        isValid # it's valid if no errors found
    )

  

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
    "cint16": "int16",
    "cint32": "int32",
    "cfloat": "float"
}

k_base_type_map = {
    "int16": "int16",
    "int32": "int32",
    "float": "float",
    "cint16": "int16",
    "cint32": "int32",
    "cfloat": "float"
}

k_base_type_size_map = {
    "int8": 1,
    "int16": 2,
    "int32": 4,
    "float": 4
}

k_integral_types = ["int16", "int32", "cint16", "cint32"]
k_floating_point_types = ["float", "cfloat"]

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
def fn_complex_coef(TT_DATA, TT_COEF):
    if fn_is_complex(TT_COEF) and not fn_is_complex(TT_DATA):
        return isError("Complex coefficients are only supported when Input/Output type is also complex.")
    return isValid

def fn_coef_32b(TT_DATA, TT_COEF):
    type32b = ["int32", "cint32"]
    if TT_COEF in type32b and not TT_DATA in type32b:
        return isError("32-bit type coefficients are only supported when Input/Output type is also a 32-bit type.")
    return isValid

def fn_int_coef(TT_DATA, TT_COEF):
    if not fn_is_integral(TT_COEF) and fn_is_integral(TT_DATA):
        return isError("Coefficients must be an integer type if Input/Output type is an integer type.")
    return isValid

def fn_float_coef(TT_DATA, TT_COEF):
    if not fn_is_floating_point(TT_COEF) and fn_is_floating_point(TT_DATA):
        return isError("Coefficients must be a float type if Input/Output type is a float type.")
    return isValid

# Same rules apply across the board
def fn_validate_coef_type(TT_DATA, TT_COEF):
    for fn in (fn_complex_coef, fn_coef_32b, fn_int_coef, fn_float_coef):
        check = fn(TT_DATA, TT_COEF)
        if check["is_valid"] == False:
            return check
    return isValid

### Validate Shift ###
def fn_float_no_shift(TT_DATA, TP_SHIFT):
    if fn_is_floating_point(TT_DATA) and (TP_SHIFT > 0):
      return isError("Shift cannot be performed for 'cfloat' data type, so must be set to 0")
    return isValid

# most library element only need to check this to validate shift
def fn_validate_shift(TT_DATA, TP_SHIFT):
  return fn_float_no_shift(TT_DATA, TP_SHIFT)



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
