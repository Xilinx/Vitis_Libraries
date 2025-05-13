#
# Copyright (C) 2019-2022, Xilinx, Inc.
# Copyright (C) 2022-2025, Advanced Micro Devices, Inc.
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


AIE_LOAD_SIZE = 256/8   # 32 Bytes
AIE_LOAD_SIZE_IN_BITS = 256
TP_DIM_MIN = 1
TP_DIM_MAX = 4
TP_NUM_FRAMES_MIN = 1
TP_NUM_FRAMES_MAX = 1
SQRT_OUTPUT = 0
SQUARED_OUTPUT = 1


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
  legal_set_AIE_VARIANT = [com.AIE, com.AIE_ML, com.AIE_MLv2]

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
########## TT_DATA Updater and Validator ############
#######################################################
def update_TT_DATA(args):
  AIE_VARIANT = args["AIE_VARIANT"]
  return fn_update_data_type_p(AIE_VARIANT)

def fn_update_data_type_p(AIE_VARIANT):
  if AIE_VARIANT in [com.AIE]:
     valid_types=["float"]
  elif AIE_VARIANT in [com.AIE_ML, com.AIE_MLv2]:
     valid_types=["float", "bfloat16"]

  param_dict={
    "name" : "TT_DATA",
    "enum" : valid_types
  }
  return param_dict

def validate_TT_DATA(args):
  TT_DATA = args["TT_DATA"]
  AIE_VARIANT = args["AIE_VARIANT"]
  return fn_validate_data_type_p(TT_DATA, AIE_VARIANT)

def fn_validate_data_type_p(TT_DATA, AIE_VARIANT):
  param_dict = fn_update_data_type_p(AIE_VARIANT)
  return (com.validate_legal_set(param_dict["enum"], "TT_DATA", TT_DATA))


#######################################################
######### TT_DATA_OUT Updater and Validator ###########
#######################################################
def update_TT_DATA_OUT(args):
  TT_DATA = args["TT_DATA"]
  return fn_update_data_type_out(TT_DATA)

def fn_update_data_type_out(TT_DATA):
  valid_types = fn_get_valid_out_data_types(TT_DATA)
  param_dict={
    "name" : "TT_DATA_OUT",
    "enum" : valid_types
  }
  return param_dict

def validate_TT_DATA_OUT(args):
  TT_DATA = args["TT_DATA"]
  TT_DATA_OUT = args["TT_DATA_OUT"]
  return fn_validate_data_type_out(TT_DATA_OUT, TT_DATA)

def fn_validate_data_type_out(TT_DATA_OUT, TT_DATA):
  param_dict = fn_update_data_type_out(TT_DATA)
  return (com.validate_legal_set(param_dict["enum"], "TT_DATA_OUT", TT_DATA_OUT))  

#######################################################
############# TP_API Updater and Validator ############
#######################################################
def update_TP_API(args):
  return fn_update_api()

def fn_update_api():
  param_dict={
    "name" : "TP_API",
    "enum" : [com.API_BUFFER, com.API_STREAM]
  }
  return param_dict

def validate_TP_API(args):
  TP_API = args["TP_API"]
  return fn_validate_api(TP_API)

def fn_validate_api(TP_API):
  param_dict = fn_update_api()
  return(com.validate_legal_set(param_dict["enum"], "TP_API", TP_API))

#######################################################
########### TP_LEN Updater and Validator ############
#######################################################
def update_TP_LEN(args):
  AIE_VARIANT = args["AIE_VARIANT"]
  TP_API = args["TP_API"]
  TT_DATA = args["TT_DATA"]
  TP_LEN = args["TP_LEN"] if args["TP_LEN"] else 0
  return fn_update_len(TP_LEN, TT_DATA, TP_API, AIE_VARIANT)

def fn_update_len(TP_LEN, TT_DATA, TP_API, AIE_VARIANT):
  elems_per_load = com.k_max_read_write_bytes[AIE_VARIANT] // com.fn_size_by_byte(TT_DATA)
  TP_LEN_max = com.k_data_memory_bytes[AIE_VARIANT] >> 2 // (com.fn_size_by_byte(TT_DATA))

  param_dict={
    "name" : "TP_LEN",
    "minimum" : elems_per_load,
    "maximum" : TP_LEN_max if TP_API == com.API_BUFFER else 2**31
  }

  TP_LEN_act = TP_LEN + (elems_per_load-(TP_LEN % elems_per_load))
  if TP_LEN_act < param_dict["minimum"]: param_dict["actual"] = param_dict["minimum"]
  elif TP_LEN_act > param_dict["maximum"]: param_dict["actual"] = param_dict["maximum"]
  else: param_dict["actual"] = TP_LEN_act
  return param_dict

def validate_TP_LEN(args):
  AIE_VARIANT = args["AIE_VARIANT"]
  TP_API = args["TP_API"]
  TT_DATA = args["TT_DATA"]
  TP_LEN = args["TP_LEN"]
  return fn_validate_len(AIE_VARIANT, TT_DATA, TP_API, TP_LEN)

def fn_validate_len(AIE_VARIANT, TT_DATA, TP_API, TP_LEN):
  elems_per_load = com.k_max_read_write_bytes[AIE_VARIANT] // com.fn_size_by_byte(TT_DATA)
  param_dict = fn_update_len(TP_LEN, TT_DATA, TP_API, AIE_VARIANT)
  range_TP_LEN = [param_dict["minimum"], param_dict["maximum"]]

  if TP_LEN % elems_per_load != 0:
    return com.isError(f"TP_LEN should be divisible by {elems_per_load}.")

  return (com.validate_range(range_TP_LEN, "TP_LEN", TP_LEN))


#######################################################
########### TP_DIM Updater and Validator ###########
#######################################################

def update_TP_DIM(args):
  TP_DIM = args["TP_DIM"]
  return fn_update_dim(TP_DIM)

def fn_update_dim(TP_DIM):
  param_dict ={}
  param_dict.update({"name" : "TP_DIM"})
  param_dict.update({"minimum" : TP_DIM_MIN})
  param_dict.update({"maximum" : TP_DIM_MAX})
  return param_dict

def validate_TP_DIM(args):
  TP_DIM = args["TP_DIM"]
  param_dict = fn_update_dim(TP_DIM)
  range_TP_DIM=[param_dict["minimum"], param_dict["maximum"]]
  return (com.validate_range(range_TP_DIM, "TP_DIM", TP_DIM))

#######################################################
######### TP_IS_OUTPUT_SQUARED Updater and Validator #########
#######################################################
def update_TP_IS_OUTPUT_SQUARED(args):
  return fn_update_is_output_squared()

def fn_update_is_output_squared():
  param_dict={
    "name" : "TP_IS_OUTPUT_SQUARED",
    "enum" : [SQRT_OUTPUT, SQUARED_OUTPUT]
  }
  return param_dict

def validate_TP_IS_OUTPUT_SQUARED(args):
  TP_IS_OUTPUT_SQUARED = args["TP_IS_OUTPUT_SQUARED"]
  return fn_validate_is_output_squared(TP_IS_OUTPUT_SQUARED)

def fn_validate_is_output_squared(TP_IS_OUTPUT_SQUARED):
  param_dict = fn_update_is_output_squared()
  return(com.validate_legal_set(param_dict["enum"], "TP_IS_OUTPUT_SQUARED", TP_IS_OUTPUT_SQUARED))

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
# Utility Functions
#### Valid input data types ####
def fn_get_valid_in_data_types(TT_DATA, AIE_VARIANT):
    if AIE_VARIANT == com.AIE:
            if (TT_DATA == "float"):  return ["float"]
    elif AIE_VARIANT == com.AIE_ML or AIE_VARIANT == com.AIE_MLv2:
        if (TT_DATA == "float"):      return ["float"]
        if (TT_DATA == "bfloat16"):   return ["bfloat16"]
    return []

def getNumLanes(TT_DATA, AIE_VARIANT=1):
    if AIE_VARIANT == com.AIE:
        if (
            (TT_DATA == "float")  
          ):
            return 8
        else:
            return 0
    if AIE_VARIANT == com.AIE_ML or AIE_VARIANT == com.AIE_MLv2:
        if (
            (TT_DATA == "float" ) or
            (TT_DATA == "bfloat16")
            ) :
            return 16
        else:
            return 0
#### Valid output data types ####
def fn_get_valid_out_data_types(TT_DATA):   # Don't feel too good with these functions but it's where we're at.
    if (TT_DATA == "float") :     return ["float"]
    if (TT_DATA == "bfloat16"):  return ["bfloat16"]
    return []



########################## Ports ###########################
def get_port_info(portname, dir, dataType, dim, apiType, vectorLength):
    return [{
        "name" : f"{portname}[{idx}]",
        "type" : f"{apiType}",
        "direction": f"{dir}",
        "data_type": dataType,
        "window_size" : com.fn_input_window_size((dim), dataType),
        "margin_size" : 0
    } for idx in range(vectorLength)]

def info_ports(args):
    """Standard function creating a static dictionary of information
    for upper software to correctly connect the IP.
    Some IP has a configurable number of ports according to parameter set,
    so port information has to be implemented as a function"""
    TT_DATA = args["TT_DATA"]
    TP_LEN = args["TP_LEN"]
    TP_DIM = args["TP_DIM"]
    TP_API = args["TP_API"]
    TP_RND = args["TP_RND"]
    TP_SAT = args["TP_SAT"]
    TP_IS_OUTPUT_SQUARED = args["TP_IS_OUTPUT_SQUARED"]
    AIE_VARIANT = args["AIE_VARIANT"]

    inDataLen =  (TP_LEN*TP_DIM)
    outDataLen = (TP_LEN)

    if (TP_API == com.API_BUFFER) :
        portsInP = get_port_info(
            portname = "inP",
            dir = "in",
            dataType = TT_DATA,
            dim = inDataLen,
            apiType = "window",
            vectorLength = TP_LEN
        )
        portsInQ = get_port_info(
            portname = "inQ",
            dir = "in",
            dataType = TT_DATA,
            dim = inDataLen,
            apiType = "window",
            vectorLength = TP_LEN
        )
        portsOut = get_port_info(
            portname = "out",
            dir = "out",
            dataType = TT_DATA,
            dim = outDataLen,
            apiType = "window",
            vectorLength = TP_LEN
        )
        pass

    return portsInP+portsInQ+portsOut

#### graph generator ####
def generate_graph(graphname, args):

    if graphname == "":
        graphname = "default_graphname"

    TT_DATA = args["TT_DATA"]
    TP_LEN = args["TP_LEN"]
    TP_DIM = args["TP_DIM"]
    TP_API = args["TP_API"]
    TP_RND = args["TP_RND"]
    TP_SAT = args["TP_SAT"]
    TP_IS_OUTPUT_SQUARED = args["TP_IS_OUTPUT_SQUARED"]
    AIE_VARIANT = args["AIE_VARIANT"]

    # Use formatted multi-line string to avoid a lot of \n and \t
    code = (
f"""
class {graphname} : public adf::graph {{
public:
  // ports
  //template <typename dir>
  template <typename dir>
  std::array<adf::port<input>, 1> inP;
  std::array<adf::port<input>, 1> inQ;
  std::array<adf::port<output>, 1> out;

  xf::dsp::aie::euclidean_distance::euclidean_distance_graph<
    {TT_DATA}, // TT_DATA
    {TP_LEN}, // TP_LEN
    {TP_DIM}, // TP_DIM
    {TP_API}, //TP_API
    {TP_RND}, //TP_RND
    {TP_SAT}, // TP_SAT
    {TP_IS_OUTPUT_SQUARED}, //TP_IS_OUTPUT_SQUARED
    > euclidean_distance_graph;

  {graphname}() : euclidean_distance_graph() {{
    adf::kernel *euclidean_distance_kernels = euclidean_distance_graph.getKernels();
    for (int i=0; i < 1; i++) {{
      adf::runtime<ratio>(euclidean_distance_kernels[i]) = 0.9;
    }}
    adf::connect<> net_in(inP, euclidean_distance_graph.inP[0]);
    adf::connect<> net_in(inQ, euclidean_distance_graph.inQ[1]);
    adf::connect<> net_out(euclidean_distance_graph.out, out[0]);

  }}
}};
""")
    out = {}
    out["graph"] = code
    out["port_info"] = info_ports(args)
    out["headerfile"] = "euclidean_distance_graph.hpp"
    out["searchpaths"] = [
        "L2/include/aie",
        "L2/tests/aie/common/inc",
        "L1/include/aie",
        "L1/src/aie",
        "L1/tests/aie/inc",
        "L1/tests/aie/src"]

    return out
