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
import json
import sys

TP_POINT_SIZE_min = 8
TP_POINT_SIZE_max = 3300

TP_FFT_NIFFT_min = 0
TP_FFT_NIFFT_max = 1

TP_SHIFT_min = 0
TP_SHIFT_max = 61

TP_RND_min = 4
TP_RND_max = 7

TP_WINDOW_VSIZE_min = 8
TP_WINDOW_VSIZE_max = 4096

TP_CASC_LEN_min = 1

TP_API_min = 0
TP_API_max = 1

TP_DYN_PT_SIZE_min = 0
TP_DYN_PT_SIZE_max = 1

AIE_VARIANT_min = 1
AIE_VARIANT_max = 2

#----------------------------------------
#Utility functions
def fn_get_radix_stages(TP_POINT_SIZE, radix):
  if TP_POINT_SIZE <= 0:
    return 0
  stages = 0
  val = TP_POINT_SIZE
  while val % radix == 0:
    val = val/radix
    stages = stages +1
  return stages

def fn_get_num_stages(TP_POINT_SIZE):
  r4_stages = 0
  r2_stages = fn_get_radix_stages(TP_POINT_SIZE,2)
  r3_stages = fn_get_radix_stages(TP_POINT_SIZE,3)
  r5_stages = fn_get_radix_stages(TP_POINT_SIZE,5)
  if (r2_stages >= 4):
    r4_stages = r2_stages // 2
    r2_stages = r2_stages % 2
  return r2_stages + r3_stages + r4_stages + r5_stages

def fn_get_nearest_valid_pt_size(TP_POINT_SIZE):
  offset = 0
  while(True):
    if fn_get_num_stages(abs(TP_POINT_SIZE - offset)):
      return abs(TP_POINT_SIZE - offset)
    elif fn_get_num_stages(TP_POINT_SIZE + offset):
      return TP_POINT_SIZE + offset
    offset += 1

#######################################################
########### AIE_VARIANT Updater and Validator #########
#######################################################
def update_AIE_VARIANT(args):
  return fn_update_aie_variant()

def fn_update_aie_variant():
  legal_set_AIE_VARIANT = [1,2]
  
  param_dict ={}
  param_dict.update({"name" : "AIE_VARIANT"})
  param_dict.update({"enum" : legal_set_AIE_VARIANT})
  return param_dict

def validate_AIE_VARIANT(args):
  AIE_VARIANT=args["AIE_VARIANT"]
  return (fn_validate_aie_variant(AIE_VARIANT))

def fn_validate_aie_variant(AIE_VARIANT):
  param_dict = fn_update_aie_variant()
  legal_set_AIE_VARIANT = param_dict["enum"]
  return(com.validate_legal_set(legal_set_AIE_VARIANT, "AIE_VARIANT", AIE_VARIANT))

#######################################################
########### TT_DATA Updater and Validator #############
#######################################################
def update_TT_DATA(args):
  AIE_VARIANT = args["AIE_VARIANT"]
  return fn_update_data_type(AIE_VARIANT)

def fn_update_data_type(AIE_VARIANT):
  valid_types = ["cint16", "cint32", "cfloat"]
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
  return (com.validate_legal_set(param_dict["enum"], "TT_DATA", TT_DATA))

#######################################################
########## TT_TWIDDLE Updater and Validator ###########
#######################################################
def update_TT_TWIDDLE(args):
  TT_DATA = args["TT_DATA"]
  return fn_update_twiddle_type(TT_DATA)

def fn_update_twiddle_type(TT_DATA):
  valid_combos = {
    "cint16": ["cint16"],
    "cint32": ["cint16","cint32"],
    "cfloat": ["cfloat"]
  }
  param_dict={
    "name" : "TT_TWIDDLE",
    "enum" : valid_combos[TT_DATA]
  }
  return param_dict

def validate_TT_TWIDDLE(args):
  TT_DATA = args["TT_DATA"]
  TT_TWIDDLE = args["TT_TWIDDLE"]
  return fn_validate_twiddle_type(TT_TWIDDLE, TT_DATA)

def fn_validate_twiddle_type(TT_TWIDDLE, TT_DATA):
  param_dict=fn_update_twiddle_type(TT_DATA)
  return (com.validate_legal_set(param_dict["enum"], "TT_TWIDDLE", TT_TWIDDLE))

#######################################################
######## TP_POINT_SIZE Updater and Validator ##########
#######################################################
def update_TP_POINT_SIZE(args):
  AIE_VARIANT = args["AIE_VARIANT"]
  TT_DATA = args["TT_DATA"]
  # TP_API = args["TP_API"] // does this affect point size?
  TP_DYN_PT_SIZE = args["TP_DYN_PT_SIZE"]
  TP_POINT_SIZE = args["TP_POINT_SIZE"] if args["TP_POINT_SIZE"] else 0
  return fn_update_point_size(AIE_VARIANT, TT_DATA, TP_POINT_SIZE, TP_DYN_PT_SIZE)

def fn_update_point_size(AIE_VARIANT, TT_DATA, TP_POINT_SIZE, TP_DYN_PT_SIZE):
  TP_POINT_SIZE_min = 8
  TP_POINT_SIZE_max = com.k_data_memory_bytes[AIE_VARIANT] // com.fn_size_by_byte(TT_DATA)

  if TP_DYN_PT_SIZE == 1:
    TP_POINT_SIZE_max = 1920
  param_dict ={
    "name" : "TP_POINT_SIZE",
    "minimum" : TP_POINT_SIZE_min,
    "maximum" : TP_POINT_SIZE_max
  }

  TP_POINT_SIZE_act = fn_get_nearest_valid_pt_size(TP_POINT_SIZE)

  if TP_POINT_SIZE_act < param_dict["minimum"]: param_dict["actual"] = param_dict["minimum"]
  elif TP_POINT_SIZE_act > param_dict["maximum"]: param_dict["actual"] = param_dict["maximum"]
  else: param_dict["actual"] = TP_POINT_SIZE_act

  return param_dict

def validate_TP_POINT_SIZE(args):
  AIE_VARIANT = args["AIE_VARIANT"]
  TT_DATA = args["TT_DATA"]
  # TP_API = args["TP_API"]
  TP_DYN_PT_SIZE = args["TP_DYN_PT_SIZE"]
  TP_POINT_SIZE = args["TP_POINT_SIZE"]
  return fn_validate_point_size(AIE_VARIANT, TT_DATA, TP_POINT_SIZE, TP_DYN_PT_SIZE)

def fn_validate_point_size(AIE_VARIANT, TT_DATA, TP_POINT_SIZE, TP_DYN_PT_SIZE):
  param_dict=fn_update_point_size(AIE_VARIANT, TT_DATA, TP_POINT_SIZE, TP_DYN_PT_SIZE)
  range_TP_POINT_SIZE= [param_dict["minimum"], param_dict["maximum"]]

  if fn_get_num_stages(TP_POINT_SIZE) == 0:
    return com.isError(f"Point size ({TP_POINT_SIZE}) does not factorize.")

  return(com.validate_range(range_TP_POINT_SIZE, "TP_POINT_SIZE", TP_POINT_SIZE))

#######################################################
########### TP_WINDOW_VSIZE Updater and Validator #####
#######################################################
def update_TP_WINDOW_VSIZE(args):
  # TP_DYN_PT_SIZE = args["TP_DYN_PT_SIZE"]
  TP_POINT_SIZE = args["TP_POINT_SIZE"]
  AIE_VARIANT = args["AIE_VARIANT"]
  TT_DATA = args["TT_DATA"]

  if args["TP_WINDOW_VSIZE"]: TP_WINDOW_VSIZE = args["TP_WINDOW_VSIZE"]
  else: TP_WINDOW_VSIZE = 0
  return fn_update_window_vsize(AIE_VARIANT, TT_DATA, TP_POINT_SIZE, TP_WINDOW_VSIZE)

def fn_update_window_vsize(AIE_VARIANT, TT_DATA, TP_POINT_SIZE, TP_WINDOW_VSIZE):
  TP_WINDOW_VSIZE_min = TP_POINT_SIZE
  frame_bytes = com.fn_size_by_byte(TT_DATA) * TP_POINT_SIZE
  num_frames = com.k_data_memory_bytes[AIE_VARIANT] // frame_bytes
  TP_WINDOW_VSIZE_max = num_frames * TP_POINT_SIZE

  param_dict ={
    "name" : "TP_WINDOW_VSIZE",
    "minimum" : TP_WINDOW_VSIZE_min,
    "maximum" : TP_WINDOW_VSIZE_max
  }
  TP_WINDOW_VSIZE_act = round(TP_WINDOW_VSIZE/TP_POINT_SIZE) * TP_POINT_SIZE

  if TP_WINDOW_VSIZE_act < param_dict["minimum"]: param_dict["actual"] = param_dict["minimum"]
  elif TP_WINDOW_VSIZE_act > param_dict["maximum"]: param_dict["actual"] = param_dict["maximum"]
  else: param_dict["actual"] = TP_WINDOW_VSIZE_act

  return param_dict

def validate_TP_WINDOW_VSIZE(args):
  TP_WINDOW_VSIZE = args["TP_WINDOW_VSIZE"]
  TP_POINT_SIZE = args["TP_POINT_SIZE"]
  AIE_VARIANT = args["AIE_VARIANT"]
  TT_DATA = args["TT_DATA"]
  return fn_validate_window_vsize(AIE_VARIANT, TT_DATA, TP_POINT_SIZE, TP_WINDOW_VSIZE)


def fn_validate_window_vsize(AIE_VARIANT, TT_DATA, TP_POINT_SIZE, TP_WINDOW_VSIZE):
  if (TP_WINDOW_VSIZE % TP_POINT_SIZE != 0):
    return com.isError(f"Input window size ({TP_WINDOW_VSIZE}) must be a multiple of point size ({TP_POINT_SIZE}) ")

  param_dict = fn_update_window_vsize(AIE_VARIANT, TT_DATA, TP_POINT_SIZE, TP_WINDOW_VSIZE)
  range_TP_WINDOW_VSIZE= [param_dict["minimum"], param_dict["maximum"]]
  
  return(com.validate_range(range_TP_WINDOW_VSIZE, "TP_WINDOW_VSIZE", TP_WINDOW_VSIZE)) 

#######################################################
########## TP_FFT_IFFT Updater and Validator ##########
#######################################################
def update_TP_FFT_NIFFT(args):
  return fn_update_tp_fft_nifft()

def fn_update_tp_fft_nifft():
  legal_set_TP_FFT_NIFFT = [0, 1]
  
  param_dict ={}
  param_dict.update({"name" : "TP_FFT_NIFFT"})
  param_dict.update({"enum" : legal_set_TP_FFT_NIFFT})
  return param_dict

def validate_TP_FFT_NIFFT(args):
  TP_FFT_NIFFT=args["TP_FFT_NIFFT"]
  return (fn_validate_tp_fft_nifft(TP_FFT_NIFFT))

def fn_validate_tp_fft_nifft(TP_FFT_NIFFT):
  param_dict = fn_update_tp_fft_nifft()
  legal_set_TP_FFT_NIFFT = param_dict["enum"]
  return(com.validate_legal_set(legal_set_TP_FFT_NIFFT, "TP_FFT_NIFFT", TP_FFT_NIFFT))

#######################################################
######## TP_DYN_PT_SIZE Updater and Validator #########
#######################################################
def update_TP_DYN_PT_SIZE(args):
  return fn_update_dyn_pt_size()

def fn_update_dyn_pt_size():
  legal_set_TP_DYN_PT_SIZE = [0, 1]
  
  param_dict ={}
  param_dict.update({"name" : "TP_DYN_PT_SIZE"})
  param_dict.update({"enum" : legal_set_TP_DYN_PT_SIZE})
  return param_dict

def validate_TP_DYN_PT_SIZE(args):
  TP_DYN_PT_SIZE = args["TP_DYN_PT_SIZE"]
  return (fn_validate_dyn_pt_size(TP_DYN_PT_SIZE))

def fn_validate_dyn_pt_size(TP_DYN_PT_SIZE):
  param_dict = fn_update_dyn_pt_size()
  legal_set_TP_DYN_PT_SIZE = param_dict["enum"]
  return(com.validate_legal_set(legal_set_TP_DYN_PT_SIZE, "TP_DYN_PT_SIZE", TP_DYN_PT_SIZE))

#######################################################
########### TP_SHIFT Updater and Validator ############
#######################################################
def update_TP_SHIFT(args):
  TT_DATA = args["TT_DATA"]
  return fn_update_shift(TT_DATA)

def fn_update_shift(TT_DATA):
  param_dict= {
    "name" : "TP_SHIFT",
    "minimum" : 0
  }

  if TT_DATA in ["cfloat"]:
    param_dict.update({"maximum" : 0})
  else:
    param_dict.update({"maximum" : 61})

  return param_dict

def validate_TP_SHIFT(args):
  TT_DATA = args["TT_DATA"]
  TP_SHIFT = args["TP_SHIFT"]
  return fn_validate_shift(TT_DATA, TP_SHIFT)

def fn_validate_shift(TT_DATA, TP_SHIFT):
  param_dict=fn_update_shift(TT_DATA)
  range_TP_SHIFT=[param_dict["minimum"], param_dict["maximum"]]
  return com.validate_range(range_TP_SHIFT, "TP_SHIFT", TP_SHIFT)

#######################################################
########### TP_RND Updater and Validator ##############
#######################################################
def update_TP_RND(args):
  AIE_VARIANT = args["AIE_VARIANT"]
  return fn_update_rnd(AIE_VARIANT)

def fn_update_rnd(AIE_VARIANT):

  legal_set_TP_RND = com.fn_get_legalSet_roundMode(AIE_VARIANT)
  param_dict={}
  param_dict.update({"name" : "TP_RND"})
  param_dict.update({"enum" : legal_set_TP_RND})

  return param_dict

def validate_TP_RND(args):
  AIE_VARIANT = args["AIE_VARIANT"]
  TP_RND = args["TP_RND"]
  return (fn_validate_rnd(AIE_VARIANT, TP_RND))

def fn_validate_rnd(AIE_VARIANT, TP_RND):
  legal_set_TP_RND = com.fn_get_legalSet_roundMode(AIE_VARIANT)
  return(com.validate_legal_set(legal_set_TP_RND, "TP_RND", TP_RND))

#######################################################
########### TP_SAT Updater and Validator ##############
#######################################################
def update_TP_SAT(args):
  return fn_update_sat()

def fn_update_sat():
  legal_set = com.fn_legal_set_sat()

  param_dict={}
  param_dict.update({"name" : "TP_SAT"})
  param_dict.update({"enum" : legal_set})
  return param_dict

def validate_TP_SAT(args):
  TP_SAT = args["TP_SAT"]
  return (fn_validate_sat(TP_SAT))

def fn_validate_sat(TP_SAT):
  param_dict=fn_update_sat()
  legal_set_TP_SAT = param_dict["enum"]
  return (com.validate_legal_set(legal_set_TP_SAT, "TP_SAT", TP_SAT))

#######################################################
########### TP_API Updater and Validator ##############
#######################################################
def update_TP_API(args):
  TP_DYN_PT_SIZE = args["TP_DYN_PT_SIZE"]
  return fn_update_TP_API(TP_DYN_PT_SIZE)

def fn_update_TP_API(TP_DYN_PT_SIZE):
  legal_set_TP_API=[0, 1]
  if TP_DYN_PT_SIZE == 1:
    legal_set_TP_API=[0]   
  param_dict={
    "name" : "TP_API",
    "enum" : legal_set_TP_API
  }
  return param_dict

def validate_TP_API(args):
    TP_API = args["TP_API"]
    TP_DYN_PT_SIZE = args["TP_DYN_PT_SIZE"]
    return fn_validate_api_val(TP_API, TP_DYN_PT_SIZE)

def fn_validate_api_val(TP_API, TP_DYN_PT_SIZE):
  param_dict=fn_update_TP_API(TP_DYN_PT_SIZE)
  return (com.validate_legal_set(param_dict["enum"], "TP_API", TP_API))

#######################################################
######### TP_CASC_LEN Updater and Validator ###########
#######################################################
def update_TP_CASC_LEN(args):
  TP_POINT_SIZE = args["TP_POINT_SIZE"]
  TP_DYN_PT_SIZE = args["TP_DYN_PT_SIZE"]
  return fn_update_casc_len(TP_POINT_SIZE, TP_DYN_PT_SIZE)

def fn_update_casc_len(TP_POINT_SIZE, TP_DYN_PT_SIZE):
  param_dict={
    "name" : "TP_CASC_LEN",
    "minimum" : 1,
    "maximum" : 1 if TP_DYN_PT_SIZE else fn_get_num_stages(TP_POINT_SIZE)
  }
  return param_dict

def validate_TP_CASC_LEN(args):
  TP_CASC_LEN = args["TP_CASC_LEN"]
  TP_POINT_SIZE = args["TP_POINT_SIZE"]
  TP_DYN_PT_SIZE = args["TP_DYN_PT_SIZE"] # currently we do not support casc_len > 1 for dynamic mixed radix fft
  return fn_validate_casc_len(TP_CASC_LEN, TP_POINT_SIZE, TP_DYN_PT_SIZE)

def fn_validate_casc_len(TP_CASC_LEN, TP_POINT_SIZE, TP_DYN_PT_SIZE):
  param_dict = fn_update_casc_len(TP_POINT_SIZE, TP_DYN_PT_SIZE)
  range_tp_casc_len = [param_dict["minimum"], param_dict["maximum"]]
  return (com.validate_range(range_tp_casc_len, "TP_CASC_LEN", TP_CASC_LEN))

  ######### Graph Generator ############

# Used by higher layer software to figure out how to connect blocks together.
def info_ports(args):
  """Standard function creating a static dictionary of information
  for upper software to correctly connect the IP.
  Some IP has dynamic number of ports according to parameter set,
  so port information has to be implemented as a function"""
  TT_DATA = args["TT_DATA"]
  TP_POINT_SIZE = args["TP_POINT_SIZE"]
  TP_WINDOW_VSIZE = args["TP_WINDOW_VSIZE"]
  TP_API = args["TP_API"]
  AIE_VARIANT = args["AIE_VARIANT"]

  if TP_API == 0:
    numPorts = 1
  else:
    if AIE_VARIANT == 1:
      numPorts = 2
    else:
      numPorts = 1

  in_ports = com.get_port_info("in", "in", TT_DATA, TP_WINDOW_VSIZE//numPorts, numPorts, 0, TP_API)
  out_ports = com.get_port_info("out", "out", TT_DATA, TP_WINDOW_VSIZE//numPorts, numPorts, 0, TP_API)

  return in_ports + out_ports

def generate_graph(graphname, args):

  if graphname == "":
    graphname = "default_graphname"
  TT_DATA = args["TT_DATA"]
  TT_TWIDDLE = args["TT_TWIDDLE"]
  TP_POINT_SIZE = args["TP_POINT_SIZE"]
  TP_FFT_NIFFT = args["TP_FFT_NIFFT"]
  TP_SHIFT = args["TP_SHIFT"]
  TP_RND = args["TP_RND"]
  TP_SAT = args["TP_SAT"]
  TP_WINDOW_VSIZE = args["TP_WINDOW_VSIZE"]
  TP_CASC_LEN = args["TP_CASC_LEN"]
  TP_API = args["TP_API"]
  AIE_VARIANT = args["AIE_VARIANT"]


  code  = (
f"""

class {graphname} : public adf::graph {{
public:
  // ports
  //template <typename dir>

  static constexpr int kStreamsPerTile = get_input_streams_core_module(); //a device trait
  static constexpr int m_kNumPorts = {TP_API} == 0 ? 1 : kStreamsPerTile; //1 for iobuffer, 2 for streams

  std::array<adf::port<input>,m_kNumPorts> in;
  std::array<adf::port<output>,m_kNumPorts> out;


  xf::dsp::aie::fft::mixed_radix_fft::mixed_radix_fft_graph<
    {TT_DATA}, // TT_DATA
    {TT_TWIDDLE}, // TT_TWIDDLE
    {TP_POINT_SIZE}, // TP_POINT_SIZE
    {TP_FFT_NIFFT}, // TP_FFT_NIFFT
    {TP_SHIFT}, // TP_SHIFT
    {TP_RND}, // TP_RND
    {TP_SAT}, //TP_SAT
    {TP_WINDOW_VSIZE},
    {TP_CASC_LEN},
    {TP_API}
  > mixed_radix_fft_graph;

  {graphname}() : mixed_radix_fft_graph() {{
    if ({TP_API} == 0) {{
      adf::connect<> net_in(in[0], mixed_radix_fft_graph.in[0]);
      adf::connect<> net_out(mixed_radix_fft_graph.out[0], out[0]);
    }} else {{
      for (int i = 0; i<m_kNumPorts ; i++) {{
        adf::connect<> net_in(in[i], mixed_radix_fft_graph.in[i]);
        adf::connect<> net_out(mixed_radix_fft_graph.out[i], out[i]);
      }}
    }}
  }}

}};

""")

  out = {}
  out["graph"] = code
  out["port_info"] = info_ports(args)
  out["headerfile"] = "mixed_radix_fft_graph.hpp"
  out["searchpaths"] = [
       "L2/include/aie",
       "L2/tests/aie/common/inc",
       "L1/include/aie",
       "L1/src/aie",
       "L1/tests/aie/inc",
       "L1/tests/aie/src"]
  return out
