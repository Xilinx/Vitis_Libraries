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
########## TT_DATA_P Updater and Validator ############
#######################################################
def update_TT_DATA_P(args):
  AIE_VARIANT = args["AIE_VARIANT"]
  return fn_update_data_type_p(AIE_VARIANT)

def fn_update_data_type_p(AIE_VARIANT):
  if AIE_VARIANT in [com.AIE]:
     valid_types=["float"]
  elif AIE_VARIANT in [com.AIE_ML, com.AIE_MLv2]:
     valid_types=["float", "bfloat16"]

  param_dict={
    "name" : "TT_DATA_P",
    "enum" : valid_types
  }
  return param_dict

def validate_TT_DATA_P(args):
  TT_DATA_P = args["TT_DATA_P"]
  AIE_VARIANT = args["AIE_VARIANT"]
  return fn_validate_data_type_p(TT_DATA_P, AIE_VARIANT)

def fn_validate_data_type_p(TT_DATA_P, AIE_VARIANT):
  param_dict = fn_update_data_type_p(AIE_VARIANT)
  return (com.validate_legal_set(param_dict["enum"], "TT_DATA_P", TT_DATA_P))


#######################################################
########## TT_DATA_Q Updater and Validator ############
#######################################################
def update_TT_DATA_Q(args):
  AIE_VARIANT = args["AIE_VARIANT"]
  TT_DATA_P = args["TT_DATA_P"]
  return fn_update_data_type_q(AIE_VARIANT, TT_DATA_P)

def fn_update_data_type_q(AIE_VARIANT, TT_DATA_P):
  valid_types = fn_get_valid_in_data_types(TT_DATA_P, AIE_VARIANT)

  param_dict={
    "name" : "TT_DATA_Q",
    "enum" : valid_types
  }
  return param_dict

def validate_TT_DATA_Q(args):
  TT_DATA_P = args["TT_DATA_P"]
  TT_DATA_Q = args["TT_DATA_Q"]
  AIE_VARIANT = args["AIE_VARIANT"]
  return fn_validate_data_type_q(TT_DATA_P, TT_DATA_Q, AIE_VARIANT)

def fn_validate_data_type_q(TT_DATA_P, TT_DATA_Q, AIE_VARIANT):
  param_dict = fn_update_data_type_q(AIE_VARIANT, TT_DATA_P)
  return (com.validate_legal_set(param_dict["enum"], "TT_DATA_Q", TT_DATA_Q))

#######################################################
######### TT_DATA_OUT Updater and Validator ###########
#######################################################
def update_TT_DATA_OUT(args):
  TT_DATA_P = args["TT_DATA_P"]
  TT_DATA_Q = args["TT_DATA_Q"]
  return fn_update_data_type_out(TT_DATA_P, TT_DATA_Q)

def fn_update_data_type_out(TT_DATA_P, TT_DATA_Q):
  valid_types = fn_get_valid_out_data_types(TT_DATA_P, TT_DATA_Q)
  param_dict={
    "name" : "TT_DATA_OUT",
    "enum" : valid_types
  }
  return param_dict

def validate_TT_DATA_OUT(args):
  TT_DATA_P = args["TT_DATA_P"]
  TT_DATA_Q = args["TT_DATA_Q"]
  TT_DATA_OUT = args["TT_DATA_OUT"]
  return fn_validate_data_type_out(TT_DATA_OUT, TT_DATA_P, TT_DATA_Q)

def fn_validate_data_type_out(TT_DATA_OUT, TT_DATA_P, TT_DATA_Q):
  param_dict = fn_update_data_type_out(TT_DATA_P, TT_DATA_Q)
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
########### TP_LEN_P Updater and Validator ############
#######################################################
def update_TP_LEN_P(args):
  AIE_VARIANT = args["AIE_VARIANT"]
  TP_API = args["TP_API"]
  TT_DATA_P = args["TT_DATA_P"]
  TP_LEN_P = args["TP_LEN_P"] if args["TP_LEN_P"] else 0
  return fn_update_len_p(TP_LEN_P, TT_DATA_P, TP_API, AIE_VARIANT)

def fn_update_len_p(TP_LEN_P, TT_DATA_P, TP_API, AIE_VARIANT):
  elems_per_load = com.k_max_read_write_bytes[AIE_VARIANT] // com.fn_size_by_byte(TT_DATA_P)
  TP_LEN_P_max = com.k_data_memory_bytes[AIE_VARIANT] >> 2 // (com.fn_size_by_byte(TT_DATA_P))

  param_dict={
    "name" : "TP_LEN_P",
    "minimum" : elems_per_load,
    "maximum" : TP_LEN_P_max if TP_API == com.API_BUFFER else 2**31
  }

  TP_LEN_P_act = TP_LEN_P + (elems_per_load-(TP_LEN_P % elems_per_load))
  if TP_LEN_P_act < param_dict["minimum"]: param_dict["actual"] = param_dict["minimum"]
  elif TP_LEN_P_act > param_dict["maximum"]: param_dict["actual"] = param_dict["maximum"]
  else: param_dict["actual"] = TP_LEN_P_act
  return param_dict

def validate_TP_LEN_P(args):
  AIE_VARIANT = args["AIE_VARIANT"]
  TP_API = args["TP_API"]
  TT_DATA_P = args["TT_DATA_P"]
  TP_LEN_P = args["TP_LEN_P"]
  return fn_validate_len_p(AIE_VARIANT, TT_DATA_P, TP_API, TP_LEN_P)

def fn_validate_len_p(AIE_VARIANT, TT_DATA_P, TP_API, TP_LEN_P):
  elems_per_load = com.k_max_read_write_bytes[AIE_VARIANT] // com.fn_size_by_byte(TT_DATA_P)
  param_dict = fn_update_len_p(TP_LEN_P, TT_DATA_P, TP_API, AIE_VARIANT)
  range_TP_LEN_P = [param_dict["minimum"], param_dict["maximum"]]

  if TP_LEN_P % elems_per_load != 0:
    return com.isError(f"TP_LEN_P should be divisible by {elems_per_load}.")

  return (com.validate_range(range_TP_LEN_P, "TP_LEN_P", TP_LEN_P))

#######################################################
########### TP_LEN_Q Updater and Validator ############
#######################################################
def update_TP_LEN_Q(args):
  AIE_VARIANT = args["AIE_VARIANT"]
  TP_API = args["TP_API"]
  TT_DATA_Q = args["TT_DATA_Q"]
  TP_LEN_Q = args["TP_LEN_Q"] if args["TP_LEN_Q"] else 0
  return fn_update_len_q(TP_LEN_Q, TT_DATA_Q, TP_API, AIE_VARIANT)

def fn_update_len_q(TP_LEN_Q, TT_DATA_Q, TP_API, AIE_VARIANT):
  elems_per_load = com.k_max_read_write_bytes[AIE_VARIANT] // com.fn_size_by_byte(TT_DATA_Q)
  TP_LEN_Q_max = com.k_data_memory_bytes[AIE_VARIANT] >> 2 // (com.fn_size_by_byte(TT_DATA_Q))

  param_dict={
    "name" : "TP_LEN_Q",
    "minimum" : elems_per_load,
    "maximum" : TP_LEN_Q_max if TP_API == com.API_BUFFER else 2**31
  }
  TP_LEN_Q_act = TP_LEN_Q + (elems_per_load-(TP_LEN_Q % elems_per_load))
  if TP_LEN_Q_act < param_dict["minimum"]: param_dict["actual"] = param_dict["minimum"]
  elif TP_LEN_Q_act > param_dict["maximum"]: param_dict["actual"] = param_dict["maximum"]
  else: param_dict["actual"] = TP_LEN_Q_act
  return param_dict

def validate_TP_LEN_Q(args):
  AIE_VARIANT = args["AIE_VARIANT"]
  TP_API = args["TP_API"]
  TT_DATA_Q = args["TT_DATA_Q"]
  TP_LEN_Q = args["TP_LEN_Q"]
  return fn_validate_len_q(AIE_VARIANT, TT_DATA_Q, TP_API, TP_LEN_Q)

def fn_validate_len_q(AIE_VARIANT, TT_DATA_Q, TP_API, TP_LEN_Q):
  elems_per_load = com.k_max_read_write_bytes[AIE_VARIANT] // com.fn_size_by_byte(TT_DATA_Q)
  param_dict = fn_update_len_q(TP_LEN_Q, TT_DATA_Q, TP_API, AIE_VARIANT)
  range_TP_LEN_Q = [param_dict["minimum"], param_dict["maximum"]]

  if TP_LEN_Q % elems_per_load != 0:
    return com.isError(f"TP_LEN_Q should be divisible by {elems_per_load}.")

  return (com.validate_range(range_TP_LEN_Q, "TP_LEN_Q", TP_LEN_Q))

#######################################################
########### TP_DIM_P Updater and Validator ###########
#######################################################

def update_TP_DIM_P(args):
  TP_DIM_P = args["TP_DIM_P"]
  return fn_update_dim_p(TP_DIM_P)

def fn_update_dim_p(TP_DIM_P):
  param_dict ={}
  param_dict.update({"name" : "TP_DIM_P"})
  param_dict.update({"minimum" : TP_DIM_MIN})
  param_dict.update({"maximum" : TP_DIM_MAX})
  return param_dict

def validate_TP_DIM_P(args):
  TP_DIM_P = args["TP_DIM_P"]
  param_dict = fn_update_dim_p(TP_DIM_P)
  range_TP_DIM_P=[param_dict["minimum"], param_dict["maximum"]]
  return (com.validate_range(range_TP_DIM_P, "TP_DIM_P", TP_DIM_P))

#######################################################
########### TP_DIM_Q Updater and Validator ###########
#######################################################
def update_TP_DIM_Q(args):
  TP_DIM_Q = args["TP_DIM_Q"]
  TP_DIM_P = args["TP_DIM_P"]
  return fn_update_dim_q(TP_DIM_Q, TP_DIM_P)

def fn_update_dim_q(TP_DIM_Q, TP_DIM_P):
  param_dict ={}
  param_dict.update({"name" : "TP_DIM_Q"})
  param_dict.update({"minimum" : TP_DIM_P})
  param_dict.update({"maximum" : TP_DIM_P})
  return param_dict

def validate_TP_DIM_Q(args):
  TP_DIM_Q = args["TP_DIM_Q"]
  param_dict = update_TP_DIM_Q(args)
  range_TP_DIM_Q=[param_dict["minimum"], param_dict["maximum"]]
  return (com.validate_range(range_TP_DIM_Q, "TP_DIM_Q", TP_DIM_Q))

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
# Utility Functions
#### Valid input data types ####
def fn_get_valid_in_data_types(TT_DATA, AIE_VARIANT):
    if AIE_VARIANT == com.AIE:
            if (TT_DATA == "float"):  return ["float"]
    elif AIE_VARIANT == com.AIE_ML or AIE_VARIANT == com.AIE_MLv2:
        if (TT_DATA == "float"):      return ["float"]
        if (TT_DATA == "bfloat16"):   return ["bfloat16"]
    return []

#### Valid input data types ####
def fn_get_valid_out_data_types(TT_DATA_P, TT_DATA_Q):   # Don't feel too good with these functions but it's where we're at.
    if (TT_DATA_P == "float")       and (TT_DATA_Q == "float"):     return ["float"]
    if (TT_DATA_P == "bfloat16")    and (TT_DATA_Q == "bfloat16"):  return ["bfloat16"]
    return []

def getNumLanes(TT_DATA_P, TT_DATA_Q, AIE_VARIANT=1):
    if AIE_VARIANT == com.AIE:
        if (
            (TT_DATA_P == "float" and  TT_DATA_Q == "float")  
          ):
            return 8
        else:
            return 0
    if AIE_VARIANT == com.AIE_ML or AIE_VARIANT == com.AIE_MLv2:
        if (
            (TT_DATA_P == "float" and TT_DATA_Q == "float") or
            (TT_DATA_P == "bfloat16" and TT_DATA_Q == "bfloat16")
            ) :
            return 16
        else:
            return 0

########################## Ports ###########################
def get_port_info(portname, dir, dataType, dim, numFrames, apiType, vectorLength):
    return [{
        "name" : f"{portname}[{idx}]",
        "type" : f"{apiType}",
        "direction": f"{dir}",
        "data_type": dataType,
        "window_size" : com.fn_input_window_size((dim*numFrames), dataType),
        "margin_size" : 0
    } for idx in range(vectorLength)]

def info_ports(args):
    """Standard function creating a static dictionary of information
    for upper software to correctly connect the IP.
    Some IP has a configurable number of ports according to parameter set,
    so port information has to be implemented as a function"""
    TT_DATA_P = args["TT_DATA_P"]
    TT_DATA_Q = args["TT_DATA_Q"]
    TT_DATA_OUT = args["TT_DATA_OUT"]
    TP_LEN_P = args["TP_LEN_P"]
    TP_LEN_Q = args["TP_LEN_Q"]
    TP_DIM_P = args["TP_DIM_P"]
    TP_DIM_Q = args["TP_DIM_Q"]
    TP_API = args["TP_API"]
    TP_RND = args["TP_RND"]
    TP_SAT = args["TP_SAT"]
    TP_NUM_FRAMES = args["TP_NUM_FRAMES"]
    TP_IS_OUTPUT_SQUARED = args["TP_IS_OUTPUT_SQUARED"]
    AIE_VARIANT = args["AIE_VARIANT"]

    inDataLen =  (TP_LEN_P*TP_DIM_P*TP_NUM_FRAMES)
    outDataLen = (TP_LEN_P*TP_NUM_FRAMES)

    if (TP_API == com.API_BUFFER) :
        portsInP = get_port_info(
            portname = "inP",
            dir = "in",
            dataType = TT_DATA_P,
            dim = inDataLen,
            numFrames = TP_NUM_FRAMES,
            apiType = "window",
            vectorLength = TP_LEN_P
        )
        portsInQ = get_port_info(
            portname = "inQ",
            dir = "in",
            dataType = TT_DATA_Q,
            dim = inDataLen,
            numFrames = TP_NUM_FRAMES,
            apiType = "window",
            vectorLength = TP_LEN_P
        )
        portsOut = get_port_info(
            portname = "out",
            dir = "out",
            dataType = TT_DATA_OUT,
            dim = outDataLen,
            numFrames = TP_NUM_FRAMES,
            apiType = "window",
            vectorLength = TP_LEN_P
        )
        pass

    return portsInP+portsInQ+portsOut

#### graph generator ####
def generate_graph(graphname, args):

    if graphname == "":
        graphname = "default_graphname"

    TT_DATA_P = args["TT_DATA_P"]
    TT_DATA_Q = args["TT_DATA_Q"]
    TT_DATA_OUT = args["TT_DATA_OUT"]
    TP_LEN_P = args["TP_LEN_P"]
    TP_LEN_Q = args["TP_LEN_Q"]
    TP_DIM_P = args["TP_DIM_P"]
    TP_DIM_Q = args["TP_DIM_Q"]
    TP_API = args["TP_API"]
    TP_RND = args["TP_RND"]
    TP_SAT = args["TP_SAT"]
    TP_NUM_FRAMES = args["TP_NUM_FRAMES"]
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
    {TT_DATA_P}, // TT_DATA_P
    {TT_DATA_Q}, // TT_DATA_Q
    {TT_DATA_OUT}, // TT_DATA_OUT
    {TP_LEN_P}, // TP_LEN_P
    {TP_LEN_Q}, //TP_LEN_Q
    {TP_DIM_P}, // TP_DIM_P
    {TP_DIM_Q}, //TP_DIM_Q
    {TP_API}, //TP_API
    {TP_RND}, //TP_RND
    {TP_SAT}, // TP_SAT
    {TP_NUM_FRAMES}, // TP_NUM_FRAMES
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
