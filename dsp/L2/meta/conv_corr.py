import aie_common as com
from aie_common import fn_is_complex, fn_size_by_byte, isError, isValid, fn_validate_satMode
#import aie_common_fir as fir

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
COMPUTE_VALID = 1
COMPUTE_SAME = 2
AIE_LOAD_SIZE = 256/8   # 32 Bytes
TP_SHIFT_min = 0
TP_SHIFT_max = 60
API_WINDOW = 0
API_STREAM = 1

byteSize = {
    "int8"  : 1,
    "int16" : 2,
    "int32" : 4,
    "int64" : 8,
    "cint16": 4,
    "cint32": 8,
    "float" : 4,
    "cfloat": 8,
    "bfloat16" : 2
}

def fn_compute_output_dimension(TP_F_LEN, TP_G_LEN, TP_COMPUTE_MODE):
    if TP_COMPUTE_MODE == COMPUTE_FULL:
        return TP_F_LEN + TP_G_LEN - 1
    elif TP_COMPUTE_MODE == COMPUTE_VALID:
        return TP_F_LEN - TP_G_LEN + 1
    elif TP_COMPUTE_MODE == COMPUTE_SAME:
        return TP_F_LEN
    else:
        return isError(f"ERROR: Invalid compute mode ({TP_COMPUTE_MODE}). Must be one of [{COMPUTE_FULL}, {COMPUTE_VALID}, {COMPUTE_SAME}].")

def fn_validate_input_data_types(TT_DATA_F, TT_DATA_G, AIE_VARIANT):
    if AIE_VARIANT == com.AIE:
        if (TT_DATA_F == "int8") and (TT_DATA_G == "int8"): return isError(f"ERROR: TT_DATA_F ({TT_DATA_F}) and TT_DATA_G ({TT_DATA_G}) are not a supported combination.")
        if (TT_DATA_F == "int16") and (TT_DATA_G == "int8"): return isError(f"ERROR: TT_DATA_F ({TT_DATA_F}) and TT_DATA_G ({TT_DATA_G}) are not a supported combination.")
        if (TT_DATA_F == "int16") and (TT_DATA_G == "int16"): return isValid
        if (TT_DATA_F == "int32") and (TT_DATA_G == "int16"): return isValid
        if (TT_DATA_F == "float") and (TT_DATA_G == "float"): return isValid
        if (TT_DATA_F == "float") and (TT_DATA_G == "cfloat"): return isError(f"ERROR: TT_DATA_F ({TT_DATA_F}) and TT_DATA_G ({TT_DATA_G}) are not a supported combination.")
        if (TT_DATA_F == "cint16") and (TT_DATA_G == "int16"): return isValid
        if (TT_DATA_F == "cint16") and (TT_DATA_G == "int32"): return isValid
        if (TT_DATA_F == "cint16") and (TT_DATA_G == "cint16"): return isValid
        if (TT_DATA_F == "cint32") and (TT_DATA_G == "int16"): return isValid
        if (TT_DATA_F == "cint32") and (TT_DATA_G == "cint16"): return isValid
        if (TT_DATA_F == "cfloat") and (TT_DATA_G == "float"): return isValid
        if (TT_DATA_F == "cfloat") and (TT_DATA_G == "cfloat"): return isValid
        return isError(f"ERROR: TT_DATA_F ({TT_DATA_F}) and TT_DATA_G ({TT_DATA_G}) are not a supported combination.")
    elif AIE_VARIANT == com.AIE_ML:
        if (TT_DATA_F == "int8") and (TT_DATA_G == "int8"): return isValid
        if (TT_DATA_F == "int16") and (TT_DATA_G == "int8"): return isError(f"ERROR: TT_DATA_F ({TT_DATA_F}) and TT_DATA_G ({TT_DATA_G}) are not a supported combination.")
        if (TT_DATA_F == "int16") and (TT_DATA_G == "int16"): return isValid
        if (TT_DATA_F == "int32") and (TT_DATA_G == "int16"): return isValid
        if (TT_DATA_F == "float") and (TT_DATA_G == "float"): return isValid
        if (TT_DATA_F == "bfloat16") and (TT_DATA_G == "bfloat16"): return isValid
        if (TT_DATA_F == "cint16") and (TT_DATA_G == "int16"): return isValid
        if (TT_DATA_F == "cint16") and (TT_DATA_G == "int32"): return isValid
        if (TT_DATA_F == "cint16") and (TT_DATA_G == "cint16"): return isValid
        if (TT_DATA_F == "cint32") and (TT_DATA_G == "int16"): return isValid
        if (TT_DATA_F == "cint32") and (TT_DATA_G == "cint16"): return isValid
        if (TT_DATA_F == "cfloat") and (TT_DATA_G == "float"): return isError(f"ERROR: TT_DATA_F ({TT_DATA_F}) and TT_DATA_G ({TT_DATA_G}) are not a supported combination.")
        if (TT_DATA_F == "cfloat") and (TT_DATA_G == "cfloat"): return isError(f"ERROR: TT_DATA_F ({TT_DATA_F}) and TT_DATA_G ({TT_DATA_G}) are not a supported combination.")
        return isError(f"ERROR: TT_DATA_F ({TT_DATA_F}) and TT_DATA_G ({TT_DATA_G}) are not a supported combination.")
    else:
        return com.fn_validate_aie_variant(AIE_VARIANT)

def fn_validate_output_data_type(TT_DATA_F, TT_DATA_G, TT_DATA_OUT, AIE_VARIANT):
    if (AIE_VARIANT == com.AIE) or (AIE_VARIANT == com.AIE_ML):
        if (TT_DATA_F == "int8")   and (TT_DATA_G == "int8")   and (TT_DATA_OUT == "int16"):  return isValid
        if (TT_DATA_F == "int16")  and (TT_DATA_G == "int8")   and (TT_DATA_OUT == "int16"):  return isValid
        if (TT_DATA_F == "int16")  and (TT_DATA_G == "int16")  and (TT_DATA_OUT == "int32"):  return isValid
        if (TT_DATA_F == "int32")  and (TT_DATA_G == "int16")  and (TT_DATA_OUT == "int32"):  return isValid
        if (TT_DATA_F == "cint16") and (TT_DATA_G == "int16")  and (TT_DATA_OUT == "cint16"): return isValid
        if (TT_DATA_F == "cint16") and (TT_DATA_G == "int16")  and (TT_DATA_OUT == "cint32"): return isValid
        if (TT_DATA_F == "cint16") and (TT_DATA_G == "int32")  and (TT_DATA_OUT == "cint32"): return isValid
        if (TT_DATA_F == "cint16") and (TT_DATA_G == "cint16") and (TT_DATA_OUT == "cint32"): return isValid
        if (TT_DATA_F == "cint32") and (TT_DATA_G == "int16")  and (TT_DATA_OUT == "cint32"): return isValid
        if (TT_DATA_F == "cint32") and (TT_DATA_G == "cint16") and (TT_DATA_OUT == "cint32"): return isValid
        if (TT_DATA_F == "float")  and (TT_DATA_G == "float")  and (TT_DATA_OUT == "float"):  return isValid
        if (TT_DATA_F == "float")  and (TT_DATA_G == "cfloat") and (TT_DATA_OUT == "cfloat"): return isError(f"ERROR: TT_DATA_F ({TT_DATA_F}) and TT_DATA_G ({TT_DATA_G}) are not a supported combination for TT_DATA_OUT ({TT_DATA_OUT}).")
        if (TT_DATA_F == "cfloat") and (TT_DATA_G == "float")  and (TT_DATA_OUT == "cfloat"): return isError(f"ERROR: TT_DATA_F ({TT_DATA_F}) and TT_DATA_G ({TT_DATA_G}) are not a supported combination for TT_DATA_OUT ({TT_DATA_OUT}).")
        if (TT_DATA_F == "cfloat") and (TT_DATA_G == "cfloat") and (TT_DATA_OUT == "cfloat"): return isError(f"ERROR: TT_DATA_F ({TT_DATA_F}) and TT_DATA_G ({TT_DATA_G}) are not a supported combination for TT_DATA_OUT ({TT_DATA_OUT}).")
        if (TT_DATA_F == "cint16") and (TT_DATA_G == "int16")  and (TT_DATA_OUT == "int8"):   return isError(f"ERROR: TT_DATA_F ({TT_DATA_F}) and TT_DATA_G ({TT_DATA_G}) are not a supported combination for TT_DATA_OUT ({TT_DATA_OUT}).")
        if (TT_DATA_F == "cint16") and (TT_DATA_G == "int16")  and (TT_DATA_OUT == "int16"):  return isError(f"ERROR: TT_DATA_F ({TT_DATA_F}) and TT_DATA_G ({TT_DATA_G}) are not a supported combination for TT_DATA_OUT ({TT_DATA_OUT}).")
        if (TT_DATA_F == "cint16") and (TT_DATA_G == "int16")  and (TT_DATA_OUT == "int32"):  return isError(f"ERROR: TT_DATA_F ({TT_DATA_F}) and TT_DATA_G ({TT_DATA_G}) are not a supported combination for TT_DATA_OUT ({TT_DATA_OUT}).")
        if (TT_DATA_F == "cint16") and (TT_DATA_G == "int16")  and (TT_DATA_OUT == "int64"):  return isError(f"ERROR: TT_DATA_F ({TT_DATA_F}) and TT_DATA_G ({TT_DATA_G}) are not a supported combination for TT_DATA_OUT ({TT_DATA_OUT}).")
        if (TT_DATA_F == "cint16") and (TT_DATA_G == "int32")  and (TT_DATA_OUT == "int8"):   return isError(f"ERROR: TT_DATA_F ({TT_DATA_F}) and TT_DATA_G ({TT_DATA_G}) are not a supported combination for TT_DATA_OUT ({TT_DATA_OUT}).")
        if (TT_DATA_F == "cint16") and (TT_DATA_G == "int32")  and (TT_DATA_OUT == "int16"):  return isError(f"ERROR: TT_DATA_F ({TT_DATA_F}) and TT_DATA_G ({TT_DATA_G}) are not a supported combination for TT_DATA_OUT ({TT_DATA_OUT}).")
        if (TT_DATA_F == "cint16") and (TT_DATA_G == "int32")  and (TT_DATA_OUT == "int32"):  return isError(f"ERROR: TT_DATA_F ({TT_DATA_F}) and TT_DATA_G ({TT_DATA_G}) are not a supported combination for TT_DATA_OUT ({TT_DATA_OUT}).")
        if (TT_DATA_F == "cint16") and (TT_DATA_G == "int32")  and (TT_DATA_OUT == "int64"):  return isError(f"ERROR: TT_DATA_F ({TT_DATA_F}) and TT_DATA_G ({TT_DATA_G}) are not a supported combination for TT_DATA_OUT ({TT_DATA_OUT}).")
        if (TT_DATA_F == "cint16") and (TT_DATA_G == "cint16") and (TT_DATA_OUT == "int8"):   return isError(f"ERROR: TT_DATA_F ({TT_DATA_F}) and TT_DATA_G ({TT_DATA_G}) are not a supported combination for TT_DATA_OUT ({TT_DATA_OUT}).")
        if (TT_DATA_F == "cint16") and (TT_DATA_G == "cint16") and (TT_DATA_OUT == "int16"):  return isError(f"ERROR: TT_DATA_F ({TT_DATA_F}) and TT_DATA_G ({TT_DATA_G}) are not a supported combination for TT_DATA_OUT ({TT_DATA_OUT}).")
        if (TT_DATA_F == "cint16") and (TT_DATA_G == "cint16") and (TT_DATA_OUT == "int32"):  return isError(f"ERROR: TT_DATA_F ({TT_DATA_F}) and TT_DATA_G ({TT_DATA_G}) are not a supported combination for TT_DATA_OUT ({TT_DATA_OUT}).")
        if (TT_DATA_F == "cint16") and (TT_DATA_G == "cint16") and (TT_DATA_OUT == "int64"):  return isError(f"ERROR: TT_DATA_F ({TT_DATA_F}) and TT_DATA_G ({TT_DATA_G}) are not a supported combination for TT_DATA_OUT ({TT_DATA_OUT}).")
        if (TT_DATA_F == "cint32") and (TT_DATA_G == "int16")  and (TT_DATA_OUT == "int8"):   return isError(f"ERROR: TT_DATA_F ({TT_DATA_F}) and TT_DATA_G ({TT_DATA_G}) are not a supported combination for TT_DATA_OUT ({TT_DATA_OUT}).")
        if (TT_DATA_F == "cint32") and (TT_DATA_G == "int16")  and (TT_DATA_OUT == "int16"):  return isError(f"ERROR: TT_DATA_F ({TT_DATA_F}) and TT_DATA_G ({TT_DATA_G}) are not a supported combination for TT_DATA_OUT ({TT_DATA_OUT}).")
        if (TT_DATA_F == "cint32") and (TT_DATA_G == "cint16") and (TT_DATA_OUT == "int32"):  return isError(f"ERROR: TT_DATA_F ({TT_DATA_F}) and TT_DATA_G ({TT_DATA_G}) are not a supported combination for TT_DATA_OUT ({TT_DATA_OUT}).")
        if (TT_DATA_F == "cint32") and (TT_DATA_G == "cint16") and (TT_DATA_OUT == "int64"):  return isError(f"ERROR: TT_DATA_F ({TT_DATA_F}) and TT_DATA_G ({TT_DATA_G}) are not a supported combination for TT_DATA_OUT ({TT_DATA_OUT}).")
        if (TT_DATA_F == "float")  and (TT_DATA_G == "cfloat") and (TT_DATA_OUT == "float"):  return isError(f"ERROR: TT_DATA_F ({TT_DATA_F}) and TT_DATA_G ({TT_DATA_G}) are not a supported combination for TT_DATA_OUT ({TT_DATA_OUT}).")
        if (TT_DATA_F == "cfloat") and (TT_DATA_G == "float")  and (TT_DATA_OUT == "float"):  return isError(f"ERROR: TT_DATA_F ({TT_DATA_F}) and TT_DATA_G ({TT_DATA_G}) are not a supported combination for TT_DATA_OUT ({TT_DATA_OUT}).")
        if (TT_DATA_F == "cfloat") and (TT_DATA_G == "cfloat") and (TT_DATA_OUT == "float"):  return isError(f"ERROR: TT_DATA_F ({TT_DATA_F}) and TT_DATA_G ({TT_DATA_G}) are not a supported combination for TT_DATA_OUT ({TT_DATA_OUT}).")
        if AIE_VARIANT == com.AIE_ML:
            if (TT_DATA_F == "bfloat16")  and (TT_DATA_G == "bfloat16")  and (TT_DATA_OUT == "float"):  return isValid
        return isError(f"ERROR: TT_DATA_F ({TT_DATA_F}) and TT_DATA_G ({TT_DATA_G}) are not a supported combination for TT_DATA_OUT ({TT_DATA_OUT}).")
    return com.fn_validate_aie_variant(AIE_VARIANT)
    

def fn_validate_funct_type(TP_FUNCT_TYPE):
    if (TP_FUNCT_TYPE == FUNCT_CORR) or (TP_FUNCT_TYPE == FUNCT_CONV):
        return isValid
    return isError(f"ERROR: TP_FUNCT_TYPE must be 0-CORRELATION or 1-CONVOLUTION for AIE-1 and AIE-2 but got {TP_FUNCT_TYPE}.")

def fn_validate_compute_mode(TP_COMPUTE_MODE):
    if (TP_COMPUTE_MODE == COMPUTE_FULL) or (TP_COMPUTE_MODE == COMPUTE_VALID) or (TP_COMPUTE_MODE == COMPUTE_SAME):
        return isValid
    return isError(f"ERROR: TP_COMPUTE_MODE must be 0-FULL_MODE or 1-SAME_MODE or 2-VALID_MODE for both CONV and CORR but got {TP_COMPUTE_MODE}.")

def fn_validate_input_g_lessEqual_f(TP_F_LEN, TP_G_LEN):
    if TP_G_LEN <= TP_F_LEN:
        return isValid
    return isError("ERROR: TP_G_LEN should be always less than or equal to TP_F_LEN as per con_corr requirement.")

def fn_validate_input_dimension(TT_DATA, TP_LEN, port):
    elems_per_load = AIE_LOAD_SIZE / byteSize[TT_DATA]
    if TP_LEN % elems_per_load != 0:
        return isError(f"ERROR: {port} should be granuality of min data_load on AIE ({TT_DATA} : ({elems_per_load}*N) where N is integer > 1).")
    min_samples = elems_per_load * 2
    max_samples = 8192 / byteSize[TT_DATA]
    if min_samples <= TP_LEN <= max_samples:
        return isValid
    return isError(f"ERROR: {port} should be greater than or equal to minimum length ({TT_DATA} - min: {min_samples}, max: {max_samples}).")

def fn_validate_api_val(TP_API):
  if TP_API != API_WINDOW and TP_API != API_STREAM:
    return isError(f"TP_API must be 0 (windowed) or streaming (1) but got {TP_API}.")
  else:
    return isValid

####################### Validation APIs #######################
def validate_TT_DATA_F(args):
    TT_DATA_F = args["TT_DATA_F"]
    TT_DATA_G = args["TT_DATA_G"]
    AIE_VARIANT = args["AIE_VARIANT"]
    return fn_validate_input_data_types(TT_DATA_F, TT_DATA_G, AIE_VARIANT)

def validate_TT_DATA_G(args):
    TT_DATA_F = args["TT_DATA_F"]
    TT_DATA_G = args["TT_DATA_G"]
    AIE_VARIANT = args["AIE_VARIANT"]
    return fn_validate_input_data_types(TT_DATA_F, TT_DATA_G, AIE_VARIANT)

def validate_TT_DATA_OUT(args):
    TT_DATA_F = args["TT_DATA_F"]
    TT_DATA_G = args["TT_DATA_G"]
    TT_DATA_OUT = args["TT_DATA_OUT"]
    AIE_VARIANT = args["AIE_VARIANT"]
    return fn_validate_output_data_type(TT_DATA_F, TT_DATA_G, TT_DATA_OUT, AIE_VARIANT) 

def validate_TP_FUNCT_TYPE(args):
    TP_FUNCT_TYPE = args["TP_FUNCT_TYPE"]
    return fn_validate_funct_type(TP_FUNCT_TYPE)    

def validate_TP_COMPUTE_MODE(args):
    TP_COMPUTE_MODE = args["TP_COMPUTE_MODE"]
    return fn_validate_compute_mode(TP_COMPUTE_MODE)  

def validate_TP_F_LEN(args):
    TT_DATA_F = args["TT_DATA_F"]
    TP_F_LEN = args["TP_F_LEN"]
    return fn_validate_input_dimension(TT_DATA_F, TP_F_LEN, "TP_F_LEN")

def validate_TP_G_LEN(args):
    TT_DATA_G = args["TT_DATA_G"]
    TP_G_LEN = args["TP_G_LEN"]
    TP_F_LEN = args["TP_F_LEN"]

    checks = []
    checks.append(fn_validate_input_dimension(TT_DATA_G, TP_G_LEN, "TP_G_LEN"))
    checks.append(fn_validate_input_g_lessEqual_f(TP_F_LEN, TP_G_LEN))
    for check in checks:
      if check["is_valid"] == False:
        return check
    return isValid

def validate_TP_SHIFT(args):
    TP_SHIFT = args["TP_SHIFT"]
    TT_DATA_OUT = args["TT_DATA_OUT"]
    return com.fn_validate_shift(TT_DATA_OUT, TP_SHIFT)

def validate_TP_API(args):
    TP_API = args["TP_API"]
    return fn_validate_api_val(TP_API)

def validate_TP_RND(args):
    TP_RND = args["TP_RND"]
    return com.fn_validate_roundMode(TP_RND, 1) # TODO: Consider AIE_VARIANT=2

def validate_TP_SAT(args):
    TP_SAT = args["TP_SAT"]
    return com.fn_validate_satMode(TP_SAT)

def validate_AIE_VARIANT(args):
    AIE_VARIANT = args["AIE_VARIANT"]
    return com.fn_validate_aie_variant(AIE_VARIANT)

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


####################### Updater APIs #######################
# TODO


########################## Ports ###########################
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
    TT_DATA_F = args["TT_DATA_F"]
    TT_DATA_G = args["TT_DATA_G"]
    TT_DATA_OUT = args["TT_DATA_OUT"]
    TP_COMPUTE_MODE = args["TP_COMPUTE_MODE"]
    TP_F_LEN = args["TP_F_LEN"]
    TP_G_LEN = args["TP_G_LEN"]
    TP_API = args["TP_API"]
    if (TP_API==0):
        portsInA = get_port_info(
            portname = "inWindowF",
            dir = "in",
            dataType = TT_DATA_F,
            dim = TP_F_LEN,
            numFrames = 1,  # TODO: Populate metadata for multiple frames.
            apiType = "window",
            vectorLength = 1
        )
        portsInB = get_port_info(
            portname = "inWindowG",
            dir = "in",
            dataType = TT_DATA_G,
            dim = TP_G_LEN,
            numFrames = 1,
            apiType = "window",
            vectorLength = 1
        )
        portsOut = get_port_info(
            portname = "outWindow",
            dir = "out",
            dataType = TT_DATA_OUT,
            dim = fn_compute_output_dimension(TP_F_LEN, TP_G_LEN, TP_COMPUTE_MODE),
            numFrames = 1,
            apiType = "window",
            vectorLength = 1
        )
    else:
        # TODO: Populate for TP_API == 1
        pass

    return portsInA+portsInB+portsOut



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

    # Use formatted multi-line string to avoid a lot of \n and \t
    code  = ("")  # TODO 
    out = {}
    out["graph"] = code
    out["port_info"] = []   # TODO
    out["headerfile"] = "conv_corr_graph.hpp"
    out["searchpaths"] = [
        "L2/include/aie",
        "L2/tests/aie/common/inc",
        "L1/include/aie",
        "L1/src/aie",
        "L1/tests/aie/inc",
        "L1/tests/aie/src"]

    return out
