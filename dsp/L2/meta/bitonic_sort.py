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
from aie_common import validate_legal_set, validate_range, fn_is_power_of_two, round_power_of_2, isError, isValid
from math import log2

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

BUFFER_BYTES = 32
# IO_BYTES_max = 16384
# TP_SSR_min = 1
# TP_SSR_max = 256

byteSize = {
  "uint16":2,
  "int16":2,
  "int32":4,
  "float":4
}

aieVariantName = {
  1:"AIE",
  2:"AIE-ML"
}

#######################################################
########### AIE_VARIANT Updater and Validator #########
#######################################################
def update_AIE_VARIANT(args):
  return fn_update_AIE_VARIANT()

def fn_update_AIE_VARIANT():
  legal_set_AIE_VARIANT = [1,2]
  
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
  AIE_VARIANT = args["AIE_VARIANT"]
  return fn_update_data_type(AIE_VARIANT)

def fn_update_data_type(AIE_VARIANT):
  valid_types = ["uint16", "int16", "int32", "float"]
  if AIE_VARIANT==com.AIE:
    valid_types.remove("uint16")
  param_dict={
    "name" : "TT_DATA",
    "enum" : valid_types
  }
  return param_dict

def validate_TT_DATA(args):
  TT_DATA = args["TT_DATA"]
  AIE_VARIANT = args["AIE_VARIANT"]
  return fn_validate_data_type(TT_DATA, AIE_VARIANT)


def fn_validate_data_type(TT_DATA, AIE_VARIANT):
  param_dict=fn_update_data_type(AIE_VARIANT)
  return (validate_legal_set(param_dict["enum"], "TT_DATA", TT_DATA))

#######################################################
############ TP_DIM Updater and Validator #############
#######################################################
def update_TP_DIM(args):
  AIE_VARIANT = args["AIE_VARIANT"]
  TT_DATA = args["TT_DATA"]
  if args["TP_DIM"]: TP_DIM = args["TP_DIM"]
  else: TP_DIM = 0

  return fn_update_TP_DIM(AIE_VARIANT, TT_DATA, TP_DIM)

def fn_update_TP_DIM(AIE_VARIANT, TT_DATA, TP_DIM):
  TP_DIM_min=int(BUFFER_BYTES/byteSize[TT_DATA])

  io_bytes_max = com.k_data_memory_bytes[AIE_VARIANT] / 2
  io_samples_max=int(io_bytes_max/byteSize[TT_DATA])

  param_dict={
    "name" : "TP_DIM",
    "minimum" : TP_DIM_min,
    "maximum" : io_samples_max
   }

  if (TP_DIM !=0) and (not fn_is_power_of_two(TP_DIM)):
    TP_DIM_act = round_power_of_2(TP_DIM)
    if (TP_DIM_act > param_dict["maximum"]):
      TP_DIM_act=param_dict["maximum"]
    if (TP_DIM_act < param_dict["minimum"]):
      TP_DIM_act=param_dict["minimum"]

    param_dict.update({"actual" : TP_DIM_act })

  return param_dict

def validate_TP_DIM(args):
  AIE_VARIANT = args["AIE_VARIANT"]
  TT_DATA = args["TT_DATA"]
  TP_DIM = args["TP_DIM"]
  return fn_validate_dim_size(AIE_VARIANT, TT_DATA, TP_DIM)

def fn_validate_dim_size(AIE_VARIANT, TT_DATA, TP_DIM):
  if (not fn_is_power_of_two(TP_DIM)):
    return isError("TP_DIM should be a power of 2!")
  else:
    param_dict=fn_update_TP_DIM(AIE_VARIANT, TT_DATA, TP_DIM)
    range_TP_DIM=[param_dict["minimum"], param_dict["maximum"]]
    return (validate_range(range_TP_DIM, "TP_DIM", TP_DIM))

#######################################################
####### TP_NUM_FRAMES Updater and Validator ###########
#######################################################
def update_TP_NUM_FRAMES(args):
  AIE_VARIANT = args["AIE_VARIANT"]
  TT_DATA = args["TT_DATA"]
  TP_DIM = args["TP_DIM"]
  return fn_update_TP_NUM_FRAMES(AIE_VARIANT, TT_DATA, TP_DIM)

def fn_update_TP_NUM_FRAMES(AIE_VARIANT, TT_DATA, TP_DIM):
  io_bytes_max = com.k_data_memory_bytes[AIE_VARIANT] / 2
  io_samples_max=io_bytes_max/byteSize[TT_DATA]
  TP_NUM_FRAMES_max=int(io_samples_max/TP_DIM)
  param_dict={
    "name" : "TP_NUM_FRAMES",
    "minimum" : 1,
    "maximum"  : TP_NUM_FRAMES_max
  }
  return param_dict

def validate_TP_NUM_FRAMES(args):
  AIE_VARIANT = args["AIE_VARIANT"]
  TT_DATA = args["TT_DATA"]
  TP_DIM = args["TP_DIM"]
  TP_NUM_FRAMES = args["TP_NUM_FRAMES"]
  return fn_validate_TP_NUM_FRAMES(AIE_VARIANT, TT_DATA, TP_DIM, TP_NUM_FRAMES)

def fn_validate_TP_NUM_FRAMES(AIE_VARIANT, TT_DATA, TP_DIM, TP_NUM_FRAMES):
  param_dict=fn_update_TP_NUM_FRAMES(AIE_VARIANT, TT_DATA, TP_DIM)
  range_TP_NUM_FRAMES=[param_dict["minimum"], param_dict["maximum"]]
  return (validate_range(range_TP_NUM_FRAMES, "TP_NUM_FRAMES", TP_NUM_FRAMES))

#######################################################
######### TP_CASC_LEN Updater and Validator ###########
#######################################################
def update_TP_CASC_LEN(args):
  TP_DIM = args["TP_DIM"]
  return fn_update_casc_len(TP_DIM)

def fn_update_casc_len(TP_DIM):
  param_dict={
    "name" : "TP_CASC_LEN",
    "minimum" : 1,
    "maximum" : int((log2(TP_DIM)+1)*log2(TP_DIM)/2)
  }
  return param_dict

def validate_TP_CASC_LEN(args):
  TP_CASC_LEN = args["TP_CASC_LEN"]
  TP_DIM = args["TP_DIM"]
  return fn_validate_casc_len(TP_CASC_LEN, TP_DIM)

def fn_validate_casc_len(TP_CASC_LEN, TP_DIM):
  param_dict=fn_update_casc_len(TP_DIM)
  range_tp_casc_len=[param_dict["minimum"], param_dict["maximum"]]
  return (validate_range(range_tp_casc_len, "TP_CASC_LEN", TP_CASC_LEN))

#######################################################
######### TP_ASCENDING Updater and Validator ##########
#######################################################
def update_TP_ASCENDING(args):
  return fn_update_ascending()

def  fn_update_ascending():
  param_dict={
    "name" : "TP_ASCENDING",
    "enum" : [0, 1]
  }
  return param_dict

def validate_TP_ASCENDING(args):
  TP_ASCENDING = args["TP_ASCENDING"]
  return fn_validate_ascending(TP_ASCENDING)

def fn_validate_ascending(TP_ASCENDING):
  return(validate_legal_set([0,1], "TP_ASCENDING", TP_ASCENDING))  

# #### port ####
def get_port_info(portname, dir, dataType, dim, numFrames, apiType, vectorLength):
  return [{
    "name" : f"{portname}[{idx}]",
    "type" : f"{apiType}",
    "direction": f"{dir}",
    "data_type": dataType,
    "fn_is_complex": com.fn_is_complex(dataType),
    "window_size" : dim*numFrames, #com.fn_input_window_size(windowVsize, dataType),
    "margin_size" : 0
} for idx in range(vectorLength)]

def info_ports(args):
  """Standard function creating a static dictionary of information
  for upper software to correctly connect the IP.
  Some IP has a configurable number of ports according to parameter set,
  so port information has to be implemented as a function"""
  TT_DATA = args["TT_DATA"]
  TP_DIM = args["TP_DIM"]
  TP_NUM_FRAMES = args["TP_NUM_FRAMES"]
  TP_CASC_LEN = args["TP_CASC_LEN"]
  
  portsIn = get_port_info(
    portname = "in",
    dir = "in",
    dataType = TT_DATA,
    dim = TP_DIM,
    numFrames = TP_NUM_FRAMES,
    apiType = "window",
    vectorLength = TP_CASC_LEN
  )
  portsOut = get_port_info(
    portname = "out",
    dir = "out",
    dataType = TT_DATA,
    dim = TP_DIM,
    numFrames = TP_NUM_FRAMES,
    apiType = "window",
    vectorLength = TP_CASC_LEN
  )
  return portsIn + portsOut


#### graph generator ####
def generate_graph(graphname, args):

  if graphname == "":
    graphname = "default_graphname"

  TT_DATA = args["TT_DATA"]
  TP_DIM = args["TP_DIM"]
  TP_NUM_FRAMES = args["TP_NUM_FRAMES"]
  TP_ASCENDING = args["TP_ASCENDING"]
  TP_CASC_LEN = args["TP_CASC_LEN"]

  ssr = 1

  # Use formatted multi-line string to avoid a lot of \n and \t
  code  = (
f"""
class {graphname} : public adf::graph {{
public:
  static constexpr unsigned int TP_SSR = 1;
  template <typename dir>
  using ssr_port_array = std::array<adf::port<dir>, 1>;

  ssr_port_array<input> in;
  ssr_port_array<output> out;

  xf::dsp::aie::bitonic_sort::bitonic_sort_graph<
    {TT_DATA}, //TT_DATA
    {TP_DIM}, //TP_DIM
    {TP_NUM_FRAMES}, //TP_NUM_FRAMES
    {TP_ASCENDING}, //TP_ASCENDING
    {TP_CASC_LEN}, //TP_CASC_LEN
  > bitonic_sort;

  {graphname}() : bitonic_sort() {{
    adf::kernel *bitonic_sort_kernels = bitonic_sort.getKernels();
    for (int i=0; i < 1; i++) {{
      adf::runtime<ratio>(bitonic_sort_kernels[i]) = 0.9;
    }}
    for (int i=0; i < TP_SSR; i++) {{
      adf::connect<> net_in(in[i], bitonic_sort.in[i]);
      adf::connect<> net_out(bitonic_sort.out[i], out[i]);
    }}
  }}

}};
""")
  out = {}
  out["graph"] = code
  out["port_info"] = info_ports(args)
  out["headerfile"] = "bitonic_sort_graph.hpp"
  out["searchpaths"] = [
       "L2/include/aie",
       "L2/tests/aie/common/inc",
       "L1/include/aie",
       "L1/src/aie",
       "L1/tests/aie/inc",
       "L1/tests/aie/src"]

  return out
