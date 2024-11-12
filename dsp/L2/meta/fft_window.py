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

# TP_POINT_SIZE_min = 16
# TP_POINT_SIZE_max = 65536
TP_WINDOW_VSIZE_min = 16
TP_WINDOW_VSIZE_max = 65536
# TP_SSR_min = 1
# TP_SSR_max = 32
TP_SHIFT_min = 0
TP_SHIFT_max = 60

#######################################################
########### AIE_VARIANT Updater and Validator #########
#######################################################
def update_AIE_VARIANT(args):
  return fn_update_AIE_VAR()

def  fn_update_AIE_VAR():
  legal_set_AIE_VAR = [1,2]

  param_dict ={}
  param_dict.update({"name" : "AIE_VARIANT"})
  param_dict.update({"enum" : legal_set_AIE_VAR})

  return param_dict

def validate_AIE_VARIANT(args):
  AIE_VARIANT = args["AIE_VARIANT"]
  return fn_validate_AIE_VARIANT(AIE_VARIANT)

def fn_validate_AIE_VARIANT(AIE_VARIANT):
  return (validate_legal_set([1,2], "AIE_VARIANT", AIE_VARIANT))

#######################################################
########### TT_DATA Updater and Validator #############
#######################################################
def update_TT_DATA(args):
  return fn_update_TT_DATA()

def fn_update_TT_DATA():
  legal_set_TT_DATA=["cint16", "cint32", "cfloat"] 
  param_dict={
       "name" : "TT_DATA",
       "enum" : legal_set_TT_DATA
    }
  return param_dict

def validate_TT_DATA(args):
  TT_DATA=args["TT_DATA"]
  return fn_validate_TT_DATA(TT_DATA)

def fn_validate_TT_DATA(TT_DATA):
  param_dict=fn_update_TT_DATA()
  legal_set_TT_DATA=param_dict["enum"]
  return (validate_legal_set(legal_set_TT_DATA, "TT_DATA", TT_DATA))

#######################################################
########### TT_COEFF Updater and Validator ############
#######################################################
def update_TT_COEFF(args):
  TT_DATA=args["TT_DATA"]
  return fn_update_TT_COEFF(TT_DATA)

def fn_update_TT_COEFF(TT_DATA):
  if TT_DATA=="cint32": legal_set_TT_COEFF=["int32"]
  elif TT_DATA=="cint16": legal_set_TT_COEFF=["int16"]
  elif TT_DATA=="cfloat": legal_set_TT_COEFF=["float"]
  param_dict={
       "name" : "TT_COEFF",
       "enum" : legal_set_TT_COEFF
    }
  return param_dict

def validate_TT_COEFF(args):
  TT_DATA=args["TT_DATA"]
  TT_COEFF=args["TT_COEFF"]
  return fn_validate_TT_COEFF(TT_DATA, TT_COEFF)

def fn_validate_TT_COEFF(TT_DATA, TT_COEFF):
  param_dict=fn_update_TT_COEFF(TT_DATA)
  legal_set_TT_COEFF=param_dict["enum"]
  return (validate_legal_set(legal_set_TT_COEFF, "TT_COEFF", TT_COEFF))

#######################################################
############ TP_API Updater and Validator #############
#######################################################
def update_TP_API(args):
  return fn_update_TP_API()

def fn_update_TP_API():
  legal_set_TP_API = [0,1]
  param_dict = {
      "name"     : "TP_API",
      "enum"     : legal_set_TP_API
  }
  return param_dict

def validate_TP_API(args):
  TP_API=args["TP_API"]
  legal_set_TP_API = [0,1]
  return (validate_legal_set(legal_set_TP_API, "TP_API", TP_API))


#######################################################
############ TP_SSR Updater and Validator #############
#######################################################
def update_TP_SSR(args):
  TP_API = args["TP_API"]
  return fn_update_TP_SSR(TP_API)

def fn_update_TP_SSR(TP_API):
  if TP_API==0:
    TP_SSR_max=32
  elif TP_API==1:
    TP_SSR_max=16

  param_dict = {
      "name"     : "TP_SSR",
      "minimum"  : 1,
      "maximum"  : TP_SSR_max
  }
  return param_dict

def validate_TP_SSR(args):
  TP_API = args["TP_API"]
  TP_SSR = args["TP_SSR"]
  return fn_validate_ssr(TP_API, TP_SSR)

def fn_validate_ssr(TP_API, TP_SSR):
  param_dict=fn_update_TP_SSR(TP_API)
  TP_SSR_range=[param_dict["minimum"], param_dict["maximum"]]
  return validate_range(TP_SSR_range, "TP_SSR", TP_SSR)

#######################################################
############ TP_DYN_PT_SIZE Updater and Validator #####
#######################################################
def update_TP_DYN_PT_SIZE(args) :
  legal_set_TP_DYN_POINT_SIZE=[0,1]
  param_dict = {
      "name"  : "TP_DYN_PT_SIZ",
      "enum"  : legal_set_TP_DYN_POINT_SIZE
  }
  return param_dict

def validate_TP_DYN_PT_SIZE(args):
  legal_set_TP_DYN_POINT_SIZE=[0,1]
  TP_DYN_PT_SIZE = args["TP_DYN_PT_SIZE"]
  return validate_range(legal_set_TP_DYN_POINT_SIZE, "TP_DYN_PT_SIZE", TP_DYN_PT_SIZE)

#######################################################
########### TP_POINT_SIZE Updater and Validator #######
#######################################################  
def update_TP_POINT_SIZE(args):
  TT_DATA=args["TT_DATA"]
  TP_DYN_PT_SIZE=args["TP_DYN_PT_SIZE"]
  TP_API=args["TP_API"]
  TP_SSR=args["TP_SSR"]
  if args["TP_POINT_SIZE"]:
    TP_POINT_SIZE = args["TP_POINT_SIZE"]
  else: 
    TP_POINT_SIZE = 0 

  return fn_update_TP_POINT_SIZE(TT_DATA, TP_DYN_PT_SIZE, TP_SSR, TP_API, TP_POINT_SIZE)

def fn_update_TP_POINT_SIZE(TT_DATA, TP_DYN_PT_SIZE, TP_SSR, TP_API, TP_POINT_SIZE):

  if TP_API==0:
    TP_POINT_SIZE_min = 16 * TP_SSR
    TP_POINT_SIZE_max = 1024* TP_SSR
  else:
    TP_POINT_SIZE_min = 16 * TP_SSR
    TP_POINT_SIZE_max = 4096* TP_SSR

  if TP_DYN_PT_SIZE==1:
    TP_POINT_SIZE_min = 32* TP_SSR

  param_dict={
       "name" : "TP_POINT_SIZE",
       "minimum" : TP_POINT_SIZE_min,
       "maximum" : TP_POINT_SIZE_max
    }
    
  point_size_granularity= 16/fn_size_by_byte(TT_DATA)

  if TP_POINT_SIZE != 0 and (TP_POINT_SIZE % point_size_granularity != 0):
    TP_POINT_SIZE_act= int(round(TP_POINT_SIZE / point_size_granularity)*point_size_granularity)

    if TP_POINT_SIZE_act <= TP_POINT_SIZE_min:
       TP_POINT_SIZE_act= TP_POINT_SIZE_min

    if TP_POINT_SIZE_act==0:
      TP_POINT_SIZE_act=int(CEIL(TP_POINT_SIZE, point_size_granularity))

    if param_dict["maximum"]< TP_POINT_SIZE_act:
      TP_POINT_SIZE_act=int(FLOOR(param_dict["maximum"], point_size_granularity))
    
    param_dict.update({"actual":TP_POINT_SIZE_act}) 
                                                                              
  return param_dict

def validate_TP_POINT_SIZE(args):
  TT_DATA=args["TT_DATA"]
  TP_DYN_PT_SIZE=args["TP_DYN_PT_SIZE"]
  TP_API=args["TP_API"]
  TP_SSR=args["TP_SSR"]
  TP_POINT_SIZE=args["TP_POINT_SIZE"]
  return fn_validate_point_size(TP_POINT_SIZE, TT_DATA, TP_DYN_PT_SIZE, TP_API, TP_SSR)

def fn_validate_point_size(TP_POINT_SIZE, TT_DATA, TP_DYN_PT_SIZE, TP_API, TP_SSR):
  param_dict= fn_update_TP_POINT_SIZE(TT_DATA, TP_DYN_PT_SIZE, TP_SSR, TP_API, TP_POINT_SIZE)
  TP_POINT_SIZE_range=[param_dict["minimum"], param_dict["maximum"]]
  point_size_granularity= 16/fn_size_by_byte(TT_DATA)
  if (TP_POINT_SIZE % point_size_granularity != 0) :
    return isError(f"Point size must describe a frame size which is a multiple of 128 bits. TP_POINT_SIZE must be multiples of {point_size_granularity} due to the size of the choosen TT_DATA.")
  return validate_range(TP_POINT_SIZE_range, "TP_POINT_SIZE", TP_POINT_SIZE)

#######################################################
########### TP_WINDOW_VSIZE Updater and Validator #####
#######################################################  
def update_TP_WINDOW_VSIZE(args):
  TP_POINT_SIZE=args["TP_POINT_SIZE"]

  if args["TP_WINDOW_VSIZE"]: TP_WINDOW_VSIZE=args["TP_WINDOW_VSIZE"]
  else: TP_WINDOW_VSIZE=0 

  return fn_update_update_TP_WINDOW_VSIZE(TP_POINT_SIZE, TP_WINDOW_VSIZE)  

def fn_update_update_TP_WINDOW_VSIZE(TP_POINT_SIZE, TP_WINDOW_VSIZE):
  param_dict={
       "name" : "TP_WINDOW_VSIZE",
       "minimum" : TP_POINT_SIZE,
       "maximum" : TP_WINDOW_VSIZE_max
    }  

  if TP_WINDOW_VSIZE !=0 and (TP_WINDOW_VSIZE%TP_POINT_SIZE!=0):
    TP_WINDOW_VSIZE_act=int(round(TP_WINDOW_VSIZE / TP_POINT_SIZE) * TP_POINT_SIZE)
    
    if TP_WINDOW_VSIZE_act==0:
          TP_WINDOW_VSIZE_act=int(CEIL(TP_WINDOW_VSIZE, TP_POINT_SIZE))

    if param_dict["maximum"]< TP_WINDOW_VSIZE_act:
        TP_WINDOW_VSIZE_act=int(FLOOR(param_dict["maximum"], TP_POINT_SIZE))
    
    param_dict.update({"actual":TP_WINDOW_VSIZE_act})  

  return param_dict

def validate_TP_WINDOW_VSIZE(args):
  TP_POINT_SIZE=args["TP_POINT_SIZE"]
  TP_WINDOW_VSIZE=args["TP_WINDOW_VSIZE"]
  return fn_validate_window_vsize(TP_POINT_SIZE,TP_WINDOW_VSIZE)

def fn_validate_window_vsize(TP_POINT_SIZE,TP_WINDOW_VSIZE):
  param_dict=fn_update_update_TP_WINDOW_VSIZE(TP_POINT_SIZE, TP_WINDOW_VSIZE)
  TP_WINDOW_VSIZE_range=[param_dict["minimum"], param_dict["maximum"]]
  if ((TP_WINDOW_VSIZE>=TP_POINT_SIZE) and (TP_WINDOW_VSIZE%TP_POINT_SIZE==0)) :
    return validate_range(TP_WINDOW_VSIZE_range, "TP_WINDOW_VSIZE", TP_WINDOW_VSIZE)
  else:
    return isError(f"Window size must be an integer multiple of point size. Got window size of {TP_WINDOW_VSIZE} and point size of {TP_POINT_SIZE}.")

#######################################################
########## TP_SHIFT Updater and Validator #############
#######################################################
def update_TP_SHIFT(args):
  TT_DATA=args["TT_DATA"]
  return fn_update_TP_SHIFT(TT_DATA)

def fn_update_TP_SHIFT(TT_DATA):
  TP_SHIFT_min=0

  if TT_DATA=="cint16": TP_SHIFT_max=31
  elif TT_DATA=="cint32": TP_SHIFT_max=60
  elif TT_DATA=="cfloat": TP_SHIFT_max=0

  param_dict = {
      "name"     : "TP_SHIFT",
      "minimum"  : TP_SHIFT_min,
      "maximum"  : TP_SHIFT_max
  }
  return param_dict

def validate_TP_SHIFT(args):
  TT_DATA = args["TT_DATA"]
  TP_SHIFT = args["TP_SHIFT"]
  return fn_validate_shift_val(TT_DATA, TP_SHIFT)

def fn_validate_shift_val(TT_DATA, TP_SHIFT):
  param_dict=fn_update_TP_SHIFT(TT_DATA)
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

#######################################################
############# weights Updater and Validator ###########
#######################################################
def update_weights(args):
  TP_POINT_SIZE = args["TP_POINT_SIZE"]
  TP_DYN_PT_SIZE = args["TP_DYN_PT_SIZE"]
  return fn_update_weights(TP_POINT_SIZE, TP_DYN_PT_SIZE)

def fn_update_weights(TP_POINT_SIZE, TP_DYN_PT_SIZE):
  param_dict={"name" : "weights"}
  if TP_DYN_PT_SIZE == 0:
    len_weights=TP_POINT_SIZE
    param_dict.update({"len" : len_weights})
  else:
    len_weights_min= TP_POINT_SIZE 
    len_weights_max= 2*TP_POINT_SIZE
    
    param_dict.update({"len" : len_weights_min})
    param_dict.update({"len_min" : len_weights_min})
    param_dict.update({"len_max" : len_weights_max})

  return param_dict

def validate_weights(args):
  TP_POINT_SIZE = args["TP_POINT_SIZE"]
  TP_DYN_PT_SIZE = args["TP_DYN_PT_SIZE"]
  weights_list = args["weights"]
  return fn_validate_weights(TP_POINT_SIZE, TP_DYN_PT_SIZE, weights_list)

def fn_validate_weights(TP_POINT_SIZE, TP_DYN_PT_SIZE, weights_list):
  param_dict=fn_update_weights(TP_POINT_SIZE, TP_DYN_PT_SIZE)
  if TP_DYN_PT_SIZE==0:
    return validate_LUT_len(weights_list, param_dict["len"])
  else:
    return validate_LUT_len_range(weights_list, param_dict["len_min"], param_dict["len_max"])

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
    "type" : "window" if TP_API==0 else "stream",
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
  TP_API = args["TP_API"]
  TP_SSR = args["TP_SSR"]
  TP_DYN_PT_SIZE = args["TP_DYN_PT_SIZE"]
  complex = fn_is_complex(TT_DATA)
  TP_WINDOW_VSIZE = args["TP_WINDOW_VSIZE"]
  AIE_VARIANT = args["AIE_VARIANT"]

  if TP_API == 0 :
    if TP_DYN_PT_SIZE == 0 :
      portsIn = get_port_info(
        portname = "in",
        dir = "in",
        dataType = TT_DATA,
        windowVsize = TP_WINDOW_VSIZE/TP_SSR,
        apiType = "window",
        vectorLength = TP_SSR
      )
      portsOut = get_port_info(
        portname = "out",
        dir = "out",
        dataType = TT_DATA,
        windowVsize = TP_WINDOW_VSIZE/TP_SSR,
        apiType = "window",
        vectorLength = TP_SSR
      )
    else :
      portsIn = get_dyn_pt_port_info("in", "in", TT_DATA, TP_WINDOW_VSIZE/TP_SSR, TP_SSR, 0, TP_API)
      portsOut = get_dyn_pt_port_info("out", "out", TT_DATA, TP_WINDOW_VSIZE/TP_SSR, TP_SSR, 0, TP_API)
  else:
    #AIE_VARIANT=1 and TP_API=1 uses 2x ports
    if AIE_VARIANT == 1:
      if TP_DYN_PT_SIZE == 0 :
        portsIn = get_port_info(
          portname = "in",
          dir = "in",
          dataType = TT_DATA,
          windowVsize = TP_WINDOW_VSIZE*2/TP_SSR,
          apiType = "stream",
          vectorLength = TP_SSR
        )
        portsOut = get_port_info(
          portname = "out",
          dir = "out",
          dataType = TT_DATA,
          windowVsize = TP_WINDOW_VSIZE*2/TP_SSR,
          apiType = "stream",
          vectorLength = TP_SSR
        )
      else:
        portsIn = get_dyn_pt_port_info("in", "in", TT_DATA, TP_WINDOW_VSIZE*2/TP_SSR, TP_SSR, 0, TP_API)
        portsOut = get_dyn_pt_port_info("out", "out", TT_DATA, TP_WINDOW_VSIZE*2/TP_SSR, TP_SSR, 0, TP_API)
    else: #AIE-ML
      if TP_DYN_PT_SIZE == 0 :
        portsIn = get_port_info(
          portname = "in",
          dir = "in",
          dataType = TT_DATA,
          windowVsize = TP_WINDOW_VSIZE/TP_SSR,
          apiType = "stream",
          vectorLength = TP_SSR
        )
        portsOut = get_port_info(
          portname = "out",
          dir = "out",
          dataType = TT_DATA,
          windowVsize = TP_WINDOW_VSIZE/TP_SSR,
          apiType = "stream",
          vectorLength = TP_SSR
        )
      else:
        portsIn = get_dyn_pt_port_info("in", "in", TT_DATA, TP_WINDOW_VSIZE/TP_SSR, TP_SSR, 0, TP_API)
        portsOut = get_dyn_pt_port_info("out", "out", TT_DATA, TP_WINDOW_VSIZE/TP_SSR, TP_SSR, 0, TP_API)

  return portsIn+portsOut


#### graph generator ####
# def get_param_list(**kwargs):
#   [f"{value}{comma}} //{key}" for key, value in kwargs.iteritems() for comma in "," ]
def fn_get_weights_vector(TT_COEFF, weights_list):
  # todo, reformat this to use list comprehension
  weights = f"{{"
  weights += ", ".join([str(weights_list[i]) for i in range(len(weights_list))])
  weights += f"}}"
  return weights

def generate_graph(graphname, args):

  if graphname == "":
    graphname = "default_graphname"

  TT_DATA = args["TT_DATA"]
  TT_COEFF = args["TT_COEFF"]
  TP_POINT_SIZE = args["TP_POINT_SIZE"]
  TP_WINDOW_VSIZE = args["TP_WINDOW_VSIZE"]
  TP_SHIFT = args["TP_SHIFT"]
  TP_API = args["TP_API"]
  TP_SSR = args["TP_SSR"]
  TP_DYN_PT_SIZE = args["TP_DYN_PT_SIZE"]
  AIE_VARIANT = args["AIE_VARIANT"]
  coeff_list = args["weights"]
  if TP_API == 1 and AIE_VARIANT == 1: #2 streams per tile
    ssr = TP_SSR//2
  else:
    ssr = TP_SSR

  weights = fn_get_weights_vector(TT_COEFF, coeff_list)

  # Use formatted multi-line string to avoid a lot of \n and \t
  code  = (
f"""
class {graphname} : public adf::graph {{
public:
  static constexpr unsigned int TP_SSR = {TP_SSR};
  template <typename dir>
  using ssr_port_array = std::array<adf::port<dir>, TP_SSR>;

  ssr_port_array<input> in;
  ssr_port_array<output> out;

  std::array<{TT_COEFF},{TP_POINT_SIZE*(1 + TP_DYN_PT_SIZE)}> weights = {weights};
  xf::dsp::aie::fft::windowfn::fft_window_graph<
    {TT_DATA}, //TT_DATA
    {TT_COEFF}, //TT_COEFF
    {TP_POINT_SIZE}, //TP_POINT_SIZE
    {TP_WINDOW_VSIZE}, //TP_WINDOW_VSIZE
    {TP_SHIFT}, //TP_SHIFT
    {TP_API}, //TP_API
    {ssr}, //TP_SSR
    {TP_DYN_PT_SIZE} //TP_DYN_PT_SIZE
  > fft_window;

  {graphname}() : fft_window(weights) {{
    adf::kernel *fft_window_kernels = fft_window.getKernels();
    for (int i=0; i < 1; i++) {{
      adf::runtime<ratio>(fft_window_kernels[i]) = 0.9;
    }}
    for (int i=0; i < TP_SSR; i++) {{
      adf::connect<> net_in(in[i], fft_window.in[i]);
      adf::connect<> net_out(fft_window.out[i], out[i]);
    }}
  }}

}};
""")
  out = {}
  out["graph"] = code
  out["port_info"] = info_ports(args)
  out["headerfile"] = "fft_window_graph.hpp"
  out["searchpaths"] = [
       "L2/include/aie",
       "L2/tests/aie/common/inc",
       "L1/include/aie",
       "L1/src/aie",
       "L1/tests/aie/inc",
       "L1/tests/aie/src"]

  return out
