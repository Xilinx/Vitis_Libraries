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
from ctypes import sizeof
import aie_common as com
from aie_common import isError,isValid

aie1_pp_buffer=16384
aie2_pp_buffer=32768

#######################################################
###########AIE_VARIANT Updater and Validator ##########
#######################################################
def update_AIE_VARIANT(args):
  return fn_update_AIE_VARIANT()

def fn_update_AIE_VARIANT():
  legal_set_aie=[1,2]
  param_dict={
    "name" : "AIE_VARIANT",
    "enum" : legal_set_aie
   }
  return param_dict

def validate_AIE_VARIANT(args):
  AIE_VARIANT = args["AIE_VARIANT"]
  return fn_validate_aie_variant(AIE_VARIANT)

def fn_validate_aie_variant(AIE_VARIANT):
  param_dict=fn_update_AIE_VARIANT()
  return(com.validate_legal_set(param_dict["enum"], "AIE_VARIANT", AIE_VARIANT))

#######################################################
#############  TT_DATA Updater and Validator ##########
#######################################################
def update_TT_DATA(args):
  return fn_update_TT_DATA()

def fn_update_TT_DATA():
  legal_set_tt_data=[
                "int16",
                "cint16",
                "int32",
                "cint32",
                "float",
                "cfloat"]
  param_dict={
    "name" : "TT_DATA",
    "enum" : legal_set_tt_data
   }
  return param_dict

def validate_TT_DATA(args):
  TT_DATA = args["TT_DATA"]
  return fn_validate_TT_DATA(TT_DATA)

def fn_validate_TT_DATA(TT_DATA):
  param_dict=fn_update_TT_DATA()
  return(com.validate_legal_set(param_dict["enum"], "TT_DATA", TT_DATA))

#######################################################
########## TT_OUT_DATA Updater and Validator ##########
#######################################################

def update_TT_OUT_DATA(args):
  TT_DATA = args["TT_DATA"]
  return fn_update_out_data(TT_DATA)

def fn_update_out_data(TT_DATA):
  validTypeCombos = {    
    "cint16": "int16" ,
    "cint32": "int32" ,
    "cfloat": "float" ,
    "int16" : "cint16",
    "int32" : "cint32",
    "float" : "cfloat" 
    }
  legal_set_tt_out=[validTypeCombos[TT_DATA]]

  param_dict={
    "name" : "TT_OUT",
    "enum" : legal_set_tt_out
  }

  return param_dict

def validate_TT_OUT_DATA(args):
  TT_DATA = args["TT_DATA"]
  TT_OUT_DATA = args["TT_OUT_DATA"]
  return fn_validate_out_data(TT_DATA, TT_OUT_DATA)

def fn_validate_out_data(TT_DATA, TT_OUT_DATA):
  param_dict=fn_update_out_data(TT_DATA)
  return com.validate_legal_set(param_dict["enum"], "TT_OUT_DATA", TT_OUT_DATA)

#######################################################
########## TP_WINDOW_VSIZE Updater and Validator ######
#######################################################
def update_TP_WINDOW_VSIZE(args):
  AIE_VARIANT = args["AIE_VARIANT"]
  TT_DATA = args["TT_DATA"]
  TT_OUT_DATA = args["TT_OUT_DATA"]
  return fn_update_TP_WINDOW_VSIZE(AIE_VARIANT, TT_DATA, TT_OUT_DATA)

def fn_update_TP_WINDOW_VSIZE(AIE_VARIANT, TT_DATA, TT_OUT_DATA):
  if AIE_VARIANT==1: max_buffer_byte=aie1_pp_buffer
  elif AIE_VARIANT==2: max_buffer_byte=aie2_pp_buffer

  byte_size_in=com.fn_size_by_byte(TT_DATA)
  byte_size_out=com.fn_size_by_byte(TT_OUT_DATA)
  byte_limit=max(byte_size_in, byte_size_out)
  sample_limit=int(max_buffer_byte/byte_limit)

  param_dict={
    "name" : "TP_WINDOW_VSIZE",
    "minimum" : 16,
    "maximum" : sample_limit
  }
  return param_dict

def validate_TP_WINDOW_VSIZE(args):
  AIE_VARIANT = args["AIE_VARIANT"]
  TT_DATA = args["TT_DATA"]
  TT_OUT_DATA = args["TT_OUT_DATA"]
  TP_WINDOW_VSIZE = args["TP_WINDOW_VSIZE"]
  return fn_validate_TP_WINDOW_VSIZE(AIE_VARIANT, TT_DATA, TT_OUT_DATA, TP_WINDOW_VSIZE)

def fn_validate_TP_WINDOW_VSIZE(AIE_VARIANT, TT_DATA, TT_OUT_DATA, TP_WINDOW_VSIZE):
  param_dict=fn_update_TP_WINDOW_VSIZE(AIE_VARIANT, TT_DATA, TT_OUT_DATA)
  range_TP_WINDOW_VSIZE=[param_dict["minimum"], param_dict["maximum"]]
  return com.validate_range(range_TP_WINDOW_VSIZE, "TP_WINDOW_VSIZE", TP_WINDOW_VSIZE)

######### Graph Generator ############

# Used by higher layer software to figure out how to connect blocks together. 
def info_ports(args):
  TT_DATA = args["TT_DATA"]
  TT_OUT_DATA = args["TT_OUT_DATA"]
  TP_WINDOW_VSIZE = args["TP_WINDOW_VSIZE"]
  in_port  = com.get_port_info("in",  "in",  TT_DATA, TP_WINDOW_VSIZE, 1, 0, 0)
  out_port = com.get_port_info("out", "out", TT_OUT_DATA, TP_WINDOW_VSIZE, 1, 0, 0)

  return (in_port+out_port) # concat lists

def gen_ports_code(args): 
  in_port  = (f"  std::array<adf::port<input>, 1> in;\n") 
  out_port = (f"  std::array<adf::port<output>, 1> out;\n")

  return (in_port+out_port) # concat strings

def gen_ports_connections(args): 
  in_port  = (f"      adf::connect<>(in[0],widget_real2complex_graph.in[0]);\n") 
  out_port = (f"      adf::connect<>(widget_real2complex_graph.out[0], out[0]);\n")

  return (in_port+out_port) # concat strings

def generate_graph(graphname, args):

  out = {}
  out["port_info"] = info_ports(args)
  ports_code = gen_ports_code(args)
  if graphname == "":
    graphname = "default_graphname"
  TT_DATA = args["TT_DATA"]
  TT_OUT_DATA = args["TT_OUT_DATA"]
  TP_WINDOW_VSIZE = args["TP_WINDOW_VSIZE"]
  code = (
f"""
class {graphname} : public adf::graph {{
public:
  // ports
{ports_code}
  xf::dsp::aie::widget::real2complex::widget_real2complex_graph<
    {TT_DATA}, // TT_DATA
    {TT_OUT_DATA}, // TT_OUT_DATA
    {TP_WINDOW_VSIZE} // TP_WINDOW_VSIZE
  > widget_real2complex_graph;
  {graphname}() : widget_real2complex_graph() {{
    //kernels
    //runtime_ratio
    //connections in loop
{gen_ports_connections(args)}

  }}

}};
"""    
  )
  out["graph"] = code
  out["headerfile"] = "widget_real2complex_graph.hpp"
  out["searchpaths"] = [
       "L2/include/aie",
       "L2/tests/aie/common/inc",
       "L1/include/aie",
       "L1/src/aie",
       "L1/tests/aie/inc",
       "L1/tests/aie/src"
  ]
  return out
