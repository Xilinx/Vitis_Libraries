#
# Copyright (C) 2025, Advanced Micro Devices, Inc.
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
from aie_common import *

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

TP_DIM_A_min = 1
TP_DIM_B_min = 1
TP_NUM_FRAMES_min = 1
TP_MODE_min = 1
TP_MODE_max = 2
TP_SHIFT_min = 0


#######################################################
########### AIE_VARIANT Updater and Validator #########
#######################################################
def update_AIE_VARIANT(args):
  return fn_update_AIE_VAR()

def  fn_update_AIE_VAR():
  legal_set_AIE_VAR = [com. AIE, com.AIE_ML, com.AIE_MLv2]

  param_dict ={}
  param_dict.update({"name" : "AIE_VARIANT"})
  param_dict.update({"enum" : legal_set_AIE_VAR})

  return param_dict

def validate_AIE_VARIANT(args):
  AIE_VARIANT = args["AIE_VARIANT"]
  return fn_validate_AIE_VARIANT(AIE_VARIANT)

def fn_validate_AIE_VARIANT(AIE_VARIANT):
  param_dict=fn_update_AIE_VAR()
  legal_set_AIE_VAR=param_dict["enum"]
  return (validate_legal_set(legal_set_AIE_VAR, "AIE_VARIANT", AIE_VARIANT))

#######################################################
########### TT_DATA Updater and Validator #############
#######################################################
def update_TT_DATA(args):
  AIE_VARIANT = args["AIE_VARIANT"]
  return fn_update_TT_DATA(AIE_VARIANT)

def fn_update_TT_DATA(AIE_VARIANT):
  legal_set_TT_DATA=["int16", "cint16", "int32", "cint32", "float", "cfloat"]
  if AIE_VARIANT in [AIE_ML]:
    legal_set_TT_DATA = ["int16", "cint16", "int32", "cint32", "bfloat16", "cbfloat16"]
  elif AIE_VARIANT in [AIE_MLv2]:
    legal_set_TT_DATA = ["int16", "cint16", "int32", "cint32", "bfloat16"]
  param_dict={
       "name" : "TT_DATA",
       "enum" : legal_set_TT_DATA
    }
  return param_dict

def validate_TT_DATA(args):
  AIE_VARIANT = args["AIE_VARIANT"]
  TT_DATA=args["TT_DATA"]
  return fn_validate_TT_DATA(AIE_VARIANT,TT_DATA)

def fn_validate_TT_DATA(AIE_VARIANT,TT_DATA):
  param_dict=fn_update_TT_DATA(AIE_VARIANT)
  legal_set_TT_DATA=param_dict["enum"]
  return (validate_legal_set(legal_set_TT_DATA, "TT_DATA", TT_DATA))

#######################################################
########### TT_OUT_DATA Updater and Validator #############
#######################################################
def update_TT_OUT_DATA(args):
  TT_DATA=args["TT_DATA"]
  return fn_update_TT_OUT_DATA(TT_DATA)

def fn_update_TT_OUT_DATA(TT_DATA):
  if TT_DATA   == "int16":     legal_set_TT_OUT_DATA=["int16","int32"]
  elif TT_DATA == "cint16":    legal_set_TT_OUT_DATA=["cint16","cint32"]
  elif TT_DATA == "int32":     legal_set_TT_OUT_DATA=["int32"]
  elif TT_DATA == "cint32":    legal_set_TT_OUT_DATA=["cint32"]
  elif TT_DATA == "bfloat16":  legal_set_TT_OUT_DATA=["bfloat16"]
  elif TT_DATA == "cbfloat16": legal_set_TT_OUT_DATA=["cbfloat16"]
  elif TT_DATA == "float":     legal_set_TT_OUT_DATA=["float"]
  elif TT_DATA == "cfloat":    legal_set_TT_OUT_DATA=["cfloat"]
  param_dict={
       "name" : "TT_OUT_DATA",
       "enum" : legal_set_TT_OUT_DATA
    }
  return param_dict

def validate_TT_OUT_DATA(args):
  TT_DATA=args["TT_DATA"]
  TT_OUT_DATA=args["TT_OUT_DATA"]
  return fn_validate_TT_OUT_DATA(TT_DATA,TT_OUT_DATA)

def fn_validate_TT_OUT_DATA(TT_DATA,TT_OUT_DATA):
  param_dict=fn_update_TT_OUT_DATA(TT_DATA)
  legal_set_TT_OUT_DATA=param_dict["enum"]
  return (validate_legal_set(legal_set_TT_OUT_DATA, "TT_OUT_DATA", TT_OUT_DATA))

#######################################################
########### TP_DIM_A Updater and Validator #############
#######################################################
#This used TT_OUT_DATA since this is always equal or larger than TT_DATA.
def update_TP_DIM_A(args):
  AIE_VARIANT = args["AIE_VARIANT"]
  TT_OUT_DATA=args["TT_OUT_DATA"]
  return fn_update_TP_DIM_A(AIE_VARIANT, TT_OUT_DATA)

def fn_update_TP_DIM_A(AIE_VARIANT, TT_OUT_DATA):
  data_bytes = com.fn_size_by_byte(TT_OUT_DATA)  
  io_bytes_max = com.k_data_memory_bytes[AIE_VARIANT]
  io_samples_max = int(io_bytes_max / data_bytes)
  
  param_dict={
    "name" : "TP_DIM_A",
    "minimum"  : TP_DIM_A_min,
    "maximum"  : io_samples_max,
    "maximum_pingpong_buf": int(io_samples_max / 2)
  }
  return param_dict

def validate_TP_DIM_A(args):
  AIE_VARIANT = args["AIE_VARIANT"]
  TP_DIM_A = args["TP_DIM_A"]
  TT_OUT_DATA = args["TT_OUT_DATA"]
  return fn_validate_TP_DIM_A(AIE_VARIANT,TT_OUT_DATA,TP_DIM_A)

def fn_validate_TP_DIM_A(AIE_VARIANT,TT_OUT_DATA,TP_DIM_A):
  param_dict=fn_update_TP_DIM_A(AIE_VARIANT,TT_OUT_DATA)
  TP_DIM_A_range=[param_dict["minimum"], param_dict["maximum"]]
  return validate_range(TP_DIM_A_range, "TP_DIM_A", TP_DIM_A)

#######################################################
########### TP_DIM_B Updater and Validator #############
#######################################################
#This used TT_OUT_DATA since this is always equal or larger than TT_DATA.
def update_TP_DIM_B(args):
  AIE_VARIANT = args["AIE_VARIANT"]
  TP_DIM_A = args["TP_DIM_A"]
  TT_OUT_DATA=args["TT_OUT_DATA"]
  return fn_update_TP_DIM_B(AIE_VARIANT, TP_DIM_A, TT_OUT_DATA)

def fn_update_TP_DIM_B(AIE_VARIANT, TP_DIM_A, TT_OUT_DATA):
  io_write_size = com.k_max_read_write_bytes[AIE_VARIANT]
  samples_in_io = (io_write_size/8)/com.fn_size_by_byte(TT_OUT_DATA)
  dim_a_ceil = CEIL(TP_DIM_A, samples_in_io)
  io_bytes_max = com.k_data_memory_bytes[AIE_VARIANT]
  io_samples_max = int(io_bytes_max / (dim_a_ceil * com.fn_size_by_byte(TT_OUT_DATA)))
  
  param_dict={
    "name" : "TP_DIM_B",
    "minimum"  : TP_DIM_B_min,
    "maximum"  : io_samples_max,
    "maximum_pingpong_buf": int(io_samples_max / 2)
  }
  return param_dict

def validate_TP_DIM_B(args):
  AIE_VARIANT = args["AIE_VARIANT"]
  TP_DIM_A = args["TP_DIM_A"]
  TP_DIM_B = args["TP_DIM_B"]
  TT_OUT_DATA = args["TT_OUT_DATA"]
  return fn_validate_TP_DIM_B(AIE_VARIANT,TP_DIM_A,TT_OUT_DATA,TP_DIM_B)

def fn_validate_TP_DIM_B(AIE_VARIANT,TP_DIM_A,TT_OUT_DATA,TP_DIM_B):
  param_dict=fn_update_TP_DIM_B(AIE_VARIANT,TP_DIM_A,TT_OUT_DATA)
  TP_DIM_B_range=[param_dict["minimum"], param_dict["maximum"]]
  return validate_range(TP_DIM_B_range, "TP_DIM_B", TP_DIM_B)

#######################################################
########### TP_NUM_FRAMES Updater and Validator #############
#######################################################
#This used TT_OUT_DATA since this is always equal or larger than TT_DATA.
def update_TP_NUM_FRAMES(args):
  AIE_VARIANT = args["AIE_VARIANT"]
  TP_DIM_A = args["TP_DIM_A"]
  TP_DIM_B = args["TP_DIM_B"]
  TT_OUT_DATA=args["TT_OUT_DATA"]
  return fn_update_TP_NUM_FRAMES(AIE_VARIANT, TP_DIM_A, TP_DIM_B, TT_OUT_DATA)

def fn_update_TP_NUM_FRAMES(AIE_VARIANT, TP_DIM_A, TP_DIM_B, TT_OUT_DATA):
  #set max as num frames which can fit in a data memory
  io_write_size = com.k_max_read_write_bytes[AIE_VARIANT]
  samples_in_io = (io_write_size/8)/com.fn_size_by_byte(TT_OUT_DATA)
  dim_a_ceil = CEIL(TP_DIM_A, samples_in_io)
  frame_size = dim_a_ceil * TP_DIM_B 
  io_bytes_max = com.k_data_memory_bytes[AIE_VARIANT]
  io_samples_max = int(io_bytes_max / (frame_size * com.fn_size_by_byte(TT_OUT_DATA)))

  param_dict={
    "name" : "TP_NUM_FRAMES",
    "minimum"  : TP_NUM_FRAMES_min,
    "maximum"  : io_samples_max,
    "maximum_pingpong_buf": int(io_samples_max / 2)
  }
  return param_dict

def validate_TP_NUM_FRAMES(args):
  AIE_VARIANT = args["AIE_VARIANT"]
  TP_DIM_A = args["TP_DIM_A"]
  TP_DIM_B = args["TP_DIM_B"]
  TT_OUT_DATA = args["TT_OUT_DATA"]
  TP_NUM_FRAMES = args["TP_NUM_FRAMES"]
  return fn_validate_TP_NUM_FRAMES(AIE_VARIANT,TP_DIM_A,TP_DIM_B,TT_OUT_DATA,TP_NUM_FRAMES)

def fn_validate_TP_NUM_FRAMES(AIE_VARIANT,TP_DIM_A,TP_DIM_B,TT_OUT_DATA,TP_NUM_FRAMES):
  param_dict=fn_update_TP_NUM_FRAMES(AIE_VARIANT,TP_DIM_A,TP_DIM_B,TT_OUT_DATA)
  TP_NUM_FRAMES_range=[param_dict["minimum"], param_dict["maximum"]]
  return validate_range(TP_NUM_FRAMES_range, "TP_NUM_FRAMES", TP_NUM_FRAMES)

#######################################################
########## TP_MODE Updater and Validator #############
#######################################################
def update_TP_MODE(args):
  AIE_VARIANT = args["AIE_VARIANT"]
  TT_DATA = args["TT_DATA"]  
  TP_DIM_B = args["TP_DIM_B"]  
  return fn_update_TP_MODE(AIE_VARIANT,TT_DATA,TP_DIM_B)

def fn_update_TP_MODE(AIE_VARIANT,TT_DATA,TP_DIM_B):
  if AIE_VARIANT == 1:
    if TP_DIM_B == 1:
      legal_set_TP_MODE = [0]
    else:
      legal_set_TP_MODE = [0,1]
  else:
    if TP_DIM_B == 1:
      legal_set_TP_MODE = [0,2]
    else:
      legal_set_TP_MODE = [0,1,2]

  if TT_DATA in ["bfloat16", "cbfloat16"]:
    legal_set_TP_MODE.remove(2)
      
  param_dict={}
  param_dict.update({"name"  : "TP_MODE"})
  param_dict.update({"enum"  : legal_set_TP_MODE})

  return param_dict

def validate_TP_MODE(args):
  AIE_VARIANT = args["AIE_VARIANT"]
  TT_DATA = args["TT_DATA"]  
  TP_DIM_B = args["TP_DIM_B"]  
  TP_MODE = args["TP_MODE"]
  return fn_validate_TP_MODE(AIE_VARIANT,TT_DATA,TP_DIM_B,TP_MODE)

def fn_validate_TP_MODE(AIE_VARIANT,TT_DATA,TP_DIM_B,TP_MODE):
  param_dict=fn_update_TP_MODE(AIE_VARIANT,TT_DATA,TP_DIM_B)
  legal_set_TP_MODE = param_dict["enum"]
  return validate_legal_set(legal_set_TP_MODE, "TP_MODE", TP_MODE)

#######################################################
########## TP_SHIFT Updater and Validator #############
#######################################################
def update_TP_SHIFT(args):
  TT_OUT_DATA=args["TT_OUT_DATA"]
  TP_MODE=args["TP_MODE"]
  return fn_update_TP_SHIFT(TT_OUT_DATA,TP_MODE)

def fn_update_TP_SHIFT(TT_OUT_DATA,TP_MODE):
  TP_SHIFT_min=0

  if TT_OUT_DATA=="int16": TP_SHIFT_max=31
  elif TT_OUT_DATA=="cint16": TP_SHIFT_max=31
  elif TT_OUT_DATA=="int32": TP_SHIFT_max=59
  elif TT_OUT_DATA=="cint32": TP_SHIFT_max=59
  else: TP_SHIFT_max=0
  if TP_MODE==3: TP_SHIFT_max=0

  param_dict = {
      "name"     : "TP_SHIFT",
      "minimum"  : TP_SHIFT_min,
      "maximum"  : TP_SHIFT_max
  }
  return param_dict

def validate_TP_SHIFT(args):
  TT_OUT_DATA = args["TT_OUT_DATA"]
  TP_MODE = args["TP_MODE"]
  TP_SHIFT= args["TP_SHIFT"]
  return fn_validate_shift_val(TT_OUT_DATA, TP_MODE, TP_SHIFT)

def fn_validate_shift_val(TT_OUT_DATA, TP_MODE, TP_SHIFT):
  param_dict=fn_update_TP_SHIFT(TT_OUT_DATA,TP_MODE)
  TP_SHIFT_range=[param_dict["minimum"], param_dict["maximum"]]
  return validate_range(TP_SHIFT_range, "TP_SHIFT", TP_SHIFT)

#######################################################
############## TP_RND Updater and Validator ###########
#######################################################
def update_TP_RND(args):
  AIE_VARIANT=args["AIE_VARIANT"]
  return fn_update_tp_rnd(AIE_VARIANT)

def fn_update_tp_rnd(AIE_VARIANT):
  legal_set_TP_RND= fn_get_legalSet_roundMode(AIE_VARIANT)

  param_dict={}
  param_dict.update({"name" : "TP_RND"})
  param_dict.update({"enum" : legal_set_TP_RND})

  return param_dict

def validate_TP_RND(args):
  AIE_VARIANT = args["AIE_VARIANT"]
  TP_RND = args["TP_RND"]
  param_dict = fn_update_tp_rnd(AIE_VARIANT)
  legal_set_TP_RND = param_dict["enum"]
  return(validate_legal_set(legal_set_TP_RND, "TP_RND", TP_RND))

#######################################################
############## TP_SAT Updater and Validator ###########
#######################################################
def update_TP_SAT(args):
  return fn_update_tp_sat()

def fn_update_tp_sat():
  legal_set = [0,1,3]

  param_dict={}
  param_dict.update({"name" : "TP_SAT"})
  param_dict.update({"enum" : legal_set})
  return param_dict

def validate_TP_SAT(args):
  TP_SAT = args["TP_SAT"]
  return fn_validate_satMode(TP_SAT)


def validate_TP_SAT(args):
  TP_SAT = args["TP_SAT"]
  return fn_validate_satMode(TP_SAT)

#### port ####

def get_port_info(portname, dir, dataType, windowVsize, apiType, vectorLength):
  windowSize = windowVsize*fn_size_by_byte(dataType)
  return [{
    "name" : f"{portname}[{idx}]",
    "type" : f"{apiType}",
    "direction": f"{dir}",
    "data_type": dataType,
    "fn_is_complex": fn_is_complex(dataType),
    "window_size" : windowSize, #com.fn_input_window_size(windowVsize, dataType),
    "margin_size" : 0
} for idx in range(vectorLength)]

def get_dyn_pt_port_info(portname, dir, TT_DATA, windowVSize, vectorLength=None, marginSize=0, TP_API=0):
  return [{
    "name" : f"{portname}[{idx}]" if vectorLength else f"{portname}", # portname no index
    "type" : "window" if TP_API == API_BUFFER else "stream",
    "direction" : f"{dir}",
    "data_type" : TT_DATA,
    "fn_is_complex" : fn_is_complex(TT_DATA),
    "window_size" : fn_input_window_size(windowVSize, TT_DATA) + 32,
    "margin_size": marginSize
  } for idx in range((vectorLength if vectorLength else 1))] # do just one port if vectorLength=None

def info_ports(args):
  """Standard function creating a static dictionary of information
  for upper software to correctly connect the IP.
  Some IP has a configurable number of ports according to parameter set,
  so port information has to be implemented as a function"""
  TT_DATA = args["TT_DATA"]
  TT_OUT_DATA = args["TT_OUT_DATA"]
  TP_DIM_A = args["TP_DIM_A"]
  TP_DIM_B = args["TP_DIM_B"]
  TP_API = API_BUFFER
  TP_NUM_FRAMES = args["TP_NUM_FRAMES"]
  complex = fn_is_complex(TT_DATA)
  AIE_VARIANT = args["AIE_VARIANT"]

  if AIE_VARIANT in [AIE, AIE_ML]:
    io_write_size = 256 #i.e. __MAX_READ_WRITE__ from device_defs.h 
  elif AIE_VARIANT in [AIE_MLv2]:
    io_write_size = 512
  samples_in_io = (io_write_size/8)/com.fn_size_by_byte(TT_OUT_DATA)
  dim_a_ceil = CEIL(TP_DIM_A, samples_in_io)
  frame_size = dim_a_ceil * TP_DIM_B
  
  TP_WINDOW_VSIZE = frame_size * TP_NUM_FRAMES

  if TP_API == API_BUFFER :
      portsIn = get_port_info(
        portname = "in",
        dir = "in",
        dataType = TT_DATA,
        windowVsize = TP_WINDOW_VSIZE,
        apiType = "window",
        vectorLength = 1
      )
      portsOut = get_port_info(
        portname = "out",
        dir = "out",
        dataType = TT_OUT_DATA,
        windowVsize = TP_WINDOW_VSIZE,
        apiType = "window",
        vectorLength = 1
      )

  return portsIn+portsOut


#### graph generator ####

def generate_graph(graphname, args):

  if graphname == "":
    graphname = "default_graphname"

  AIE_VARIANT = args["AIE_VARIANT"]
  TT_DATA = args["TT_DATA"]
  TT_OUT_DATA = args["TT_OUT_DATA"]
  TP_DIM_A = args["TP_DIM_A"]
  TP_DIM_B = args["TP_DIM_B"]
  TP_NUM_FRAMES = args["TP_NUM_FRAMES"]
  TP_MODE = args["TP_MODE"]
  TP_SHIFT = args["TP_SHIFT"]
  TP_RND = args["TP_RND"]
  TP_SAT = args["TP_SAT"]


  # Use formatted multi-line string to avoid a lot of \n and \t
  code  = (
f"""
class {graphname} : public adf::graph {{
public:
  static constexpr unsigned int TP_SSR = 1;
  template <typename dir>
  using ssr_port_array = std::array<adf::port<dir>, TP_SSR>;

  ssr_port_array<input> in;
  ssr_port_array<output> out;

  xf::dsp::aie::cumsum::cumsum_graph<
    {TT_DATA}, //TT_DATA
    {TT_OUT_DATA}, //TT_OUT_DATA
    {TP_DIM_A}, //TP_DIM_A
    {TP_DIM_B}, //TP_DIM_B
    {TP_NUM_FRAMES}, //TP_NUM_FRAMES
    {TP_MODE}, //TP_MODE
    {TP_SHIFT}, //TP_SHIFT
    {TP_RND}, //TP_RND
    {TP_SAT} //TP_SAT
  > cumsum;

  {graphname}() : cumsum() {{
    adf::kernel *cumsum_kernels = cumsum.getKernels();

    for (int i=0; i < TP_SSR; i++) {{
      adf::connect<> net_in(in[i], cumsum.in[i]);
      adf::connect<> net_out(cumsum.out[i], out[i]);
    }}
  }}

}};
""")
  out = {}
  out["graph"] = code
  out["port_info"] = info_ports(args)
  out["headerfile"] = "cumsum_graph.hpp"
  out["searchpaths"] = [
       "L2/include/aie",
       "L2/tests/aie/common/inc",
       "L1/include/aie",
       "L1/src/aie",
       "L1/tests/aie/inc",
       "L1/tests/aie/src"]

  return out
