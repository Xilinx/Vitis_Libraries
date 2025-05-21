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
from aie_common import isError, isValid


#######################################################
###########AIE_VARIANT Updater and Validator ##########
#######################################################
def update_AIE_VARIANT(args):
    return fn_update_AIE_VARIANT()


def fn_update_AIE_VARIANT():
    legal_set_AIE_VARIANT = [com.AIE, com.AIE_ML, com.AIE_MLv2]
    param_dict = {"name": "AIE_VARIANT", "enum": legal_set_AIE_VARIANT}
    return param_dict


def validate_AIE_VARIANT(args):
    AIE_VARIANT = args["AIE_VARIANT"]
    return fn_validate_aie_variant(AIE_VARIANT)


def fn_validate_aie_variant(AIE_VARIANT):
    param_dict = fn_update_AIE_VARIANT()
    return com.validate_legal_set(param_dict["enum"], "AIE_VARIANT", AIE_VARIANT)


#######################################################
############# TT_DATA Updater and Validator ###########
#######################################################
def update_TT_DATA(args):
    return fn_update_TT_DATA()


def fn_update_TT_DATA():
    legal_set_tt_data = ["int16", "int32", "float", "cint16", "cint32", "cfloat"]
    param_dict = {"name": "TT_DATA", "enum": legal_set_tt_data}
    return param_dict


def validate_TT_DATA(args):
    TT_DATA = args["TT_DATA"]
    return fn_validate_TT_DATA(TT_DATA)


def fn_validate_TT_DATA(TT_DATA):
    legal_set_tt_data = ["int16", "int32", "float", "cint16", "cint32", "cfloat"]
    return com.validate_legal_set(legal_set_tt_data, "TT_DATA", TT_DATA)


#######################################################
############# TP_IN_API Updater and Validator #########
#######################################################
def update_TP_IN_API(args):
    return fn_update_TP_IN_API()


def fn_update_TP_IN_API():
    legal_set_TP_IN_API = [0, 1]
    param_dict = {"name": "TP_IN_API", "enum": legal_set_TP_IN_API}
    return param_dict


def validate_TP_IN_API(args):
    TP_IN_API = args["TP_IN_API"]
    return fn_validate_TP_IN_API(TP_IN_API)


def fn_validate_TP_IN_API(TP_IN_API):
    param_dict=fn_update_TP_IN_API()
    legal_set_TP_IN_API=param_dict["enum"]
    return(com.validate_legal_set(legal_set_TP_IN_API, "TP_IN_API", TP_IN_API))

#######################################################
############# TP_OUT_API Updater and Validator ########
#######################################################
def update_TP_OUT_API(args):
    return fn_update_TP_OUT_API()


def fn_update_TP_OUT_API():
    legal_set_TP_OUT_API = [0, 1]
    param_dict = {"name": "TP_OUT_API", "enum": legal_set_TP_OUT_API}
    return param_dict

def validate_TP_OUT_API(args):
    TP_OUT_API = args["TP_OUT_API"]
    return fn_validate_TP_OUT_API(TP_OUT_API)


def fn_validate_TP_OUT_API(TP_OUT_API):
    param_dict=fn_update_TP_OUT_API()
    legal_set_TP_OUT_API=param_dict["enum"]
    return(com.validate_legal_set(legal_set_TP_OUT_API, "TP_OUT_API", TP_OUT_API))

#######################################################
############# TP_NUM_INPUTS Updater and Validator #####
#######################################################
def update_TP_NUM_INPUTS(args):
    AIE_VARIANT=args["AIE_VARIANT"]
    TT_DATA=args["TT_DATA"]
    TP_IN_API=args["TP_IN_API"]
    TP_OUT_API=args["TP_OUT_API"]
    return fn_update_TP_NUM_INPUTS(AIE_VARIANT, TT_DATA, TP_IN_API, TP_OUT_API)
  
def fn_update_TP_NUM_INPUTS(AIE_VARIANT, TT_DATA, TP_IN_API, TP_OUT_API):
    legal_set_TP_NUM_INPUTS=[1,2]
    if TP_IN_API==0:
      legal_set_TP_NUM_INPUTS=[1]
    else:
      if AIE_VARIANT in [com.AIE_ML, com.AIE_MLv2]:
        legal_set_TP_NUM_INPUTS=[1]

    #int16 is not supported for multiple stream to multiple window operation
    if (TP_IN_API==1 and TP_OUT_API==0 and TT_DATA=="int16"): 
      legal_set_TP_NUM_INPUTS=[1]
  
    param_dict={
      "name" : "TP_NUM_INPUTSs",
      "enum" : legal_set_TP_NUM_INPUTS
     }
    return param_dict

def validate_TP_NUM_INPUTS(args):
    AIE_VARIANT=args["AIE_VARIANT"]
    TT_DATA=args["TT_DATA"]
    TP_IN_API=args["TP_IN_API"]
    TP_OUT_API=args["TP_OUT_API"]
    TP_NUM_INPUTS=args["TP_NUM_INPUTS"]
    return fn_validate_TP_NUM_INPUTS(AIE_VARIANT, TT_DATA, TP_IN_API, TP_OUT_API, TP_NUM_INPUTS)

def fn_validate_TP_NUM_INPUTS(AIE_VARIANT, TT_DATA, TP_IN_API, TP_OUT_API, TP_NUM_INPUTS):
    param_dict=fn_update_TP_NUM_INPUTS(AIE_VARIANT, TT_DATA, TP_IN_API, TP_OUT_API)
    return(com.validate_legal_set(param_dict["enum"], "TP_NUM_INPUTS", TP_NUM_INPUTS))


#######################################################
########### TP_WINDOW_VSIZE Updater and Validator #####
#######################################################
def update_TP_WINDOW_VSIZE(args):
    return fn_update_TP_WINDOW_VSIZE()


def fn_update_TP_WINDOW_VSIZE():
    param_dict = {"name": "TP_WINDOW_VSIZE", "minimum": 4, "maximum": 4096}
    return param_dict


def validate_TP_WINDOW_VSIZE(args):
    TP_WINDOW_VSIZE = args["TP_WINDOW_VSIZE"]
    return fn_validate_TP_WINDOW_VSIZE(TP_WINDOW_VSIZE)


def fn_validate_TP_WINDOW_VSIZE(TP_WINDOW_VSIZE):
    range_TP_WINDOW_VSIZE = [4, 4096]
    return com.validate_range(range_TP_WINDOW_VSIZE, "TP_WINDOW_VSIZE", TP_WINDOW_VSIZE)


#######################################################
###### TP_NUM_OUTPUT_CLONES Updater and Validator #####
#######################################################
def update_TP_NUM_OUTPUT_CLONES(args):
  AIE_VARIANT=args["AIE_VARIANT"]
  TP_OUT_API=args["TP_OUT_API"]
  TP_IN_API=args["TP_IN_API"]
  return fn_update_TP_NUM_OUTPUT_CLONES(AIE_VARIANT, TP_IN_API, TP_OUT_API)

def fn_update_TP_NUM_OUTPUT_CLONES(AIE_VARIANT, TP_IN_API, TP_OUT_API):
  TP_NUM_OUTPUT_CLONES_max=4
  if TP_IN_API==0:
    TP_NUM_OUTPUT_CLONES_max=3

  if TP_OUT_API==1 and AIE_VARIANT in [com.AIE_ML, com.AIE_MLv2]:
    TP_NUM_OUTPUT_CLONES_max=1

  param_dict={
    "name" : "TP_NUM_OUTPUT_CLONES",
    "minimum" : 1,
    "maximum" : TP_NUM_OUTPUT_CLONES_max
   }
  return param_dict

def validate_TP_NUM_OUTPUT_CLONES(args):
  AIE_VARIANT=args["AIE_VARIANT"]
  TP_OUT_API=args["TP_OUT_API"]
  TP_IN_API=args["TP_IN_API"]  
  TP_NUM_OUTPUT_CLONES=args["TP_NUM_OUTPUT_CLONES"]
  return fn_validate_TP_NUM_OUTPUT_CLONES(AIE_VARIANT, TP_IN_API, TP_OUT_API, TP_NUM_OUTPUT_CLONES)

def fn_validate_TP_NUM_OUTPUT_CLONES(AIE_VARIANT, TP_IN_API, TP_OUT_API, TP_NUM_OUTPUT_CLONES):
  param_dict=fn_update_TP_NUM_OUTPUT_CLONES(AIE_VARIANT, TP_IN_API, TP_OUT_API)
  range_TP_NUM_OUTPUT_CLONES=[param_dict["minimum"], param_dict["maximum"]]
  return(com.validate_range(range_TP_NUM_OUTPUT_CLONES, "TP_NUM_OUTPUT_CLONES", TP_NUM_OUTPUT_CLONES))


#######################################################
###### TP_PATTERN Updater and Validator ###############
#######################################################
def update_TP_PATTERN(args):
    TP_IN_API=args["TP_IN_API"]
    TP_OUT_API=args["TP_OUT_API"]
    TP_NUM_INPUTS=args["TP_NUM_INPUTS"]
    TP_NUM_OUTPUT_CLONES=args["TP_NUM_OUTPUT_CLONES"]
    return fn_update_TP_PATTERN(TP_IN_API, TP_OUT_API, TP_NUM_INPUTS, TP_NUM_OUTPUT_CLONES)

def fn_update_TP_PATTERN(TP_IN_API, TP_OUT_API, TP_NUM_INPUTS, TP_NUM_OUTPUT_CLONES):
    TP_PATTERN_max=0
    if (TP_IN_API==1 and TP_NUM_INPUTS==2) or (TP_OUT_API==1 and TP_NUM_OUTPUT_CLONES==2):
      TP_PATTERN_max=2
    param_dict={
      "name" : "TP_PATTERN",
      "minimum" : 0,
      "maximum" : TP_PATTERN_max
     }
    return param_dict

def validate_TP_PATTERN(args):
    TP_IN_API=args["TP_IN_API"]
    TP_OUT_API=args["TP_OUT_API"]
    TP_NUM_INPUTS=args["TP_NUM_INPUTS"]
    TP_NUM_OUTPUT_CLONES=args["TP_NUM_OUTPUT_CLONES"]
    TP_PATTERN=args["TP_PATTERN"]
    return fn_validate_TP_PATTERN(TP_IN_API, TP_OUT_API, TP_NUM_INPUTS, TP_NUM_OUTPUT_CLONES, TP_PATTERN)

def fn_validate_TP_PATTERN(TP_IN_API, TP_OUT_API, TP_NUM_INPUTS, TP_NUM_OUTPUT_CLONES, TP_PATTERN):
    param_dict=fn_update_TP_PATTERN(TP_IN_API, TP_OUT_API, TP_NUM_INPUTS, TP_NUM_OUTPUT_CLONES)
    range_TP_PATTERN=[param_dict["minimum"], param_dict["maximum"]]
    return(com.validate_range(range_TP_PATTERN, "TP_PATTERN", TP_PATTERN))

#######################################################
###### TP_HEADER_BYTES Updater and Validator ##########
#######################################################
def update_TP_HEADER_BYTES(args):
    return fn_update_TP_HEADER_BYTES()


def fn_update_TP_HEADER_BYTES():
    param_dict = {"name": "TP_HEADER_BYTES", "minimum": 0, "maximum": 32}
    return param_dict


def validate_TP_HEADER_BYTES(args):
    TP_HEADER_BYTES = args["TP_HEADER_BYTES"]
    return fn_validate_TP_HEADER_BYTES(TP_HEADER_BYTES)


def fn_validate_TP_HEADER_BYTES(TP_HEADER_BYTES):
    range_TP_HEADER_BYTES = [0, 32]
    return com.validate_range(range_TP_HEADER_BYTES, "TP_HEADER_BYTES", TP_HEADER_BYTES)


def local_sizeof(TT_DATA):
    if TT_DATA == "int16":
        return 2
    elif TT_DATA == "int32":
        return 4
    elif TT_DATA == "float":
        return 4
    elif TT_DATA == "cint16":
        return 4
    elif TT_DATA == "cint32":
        return 8
    elif TT_DATA == "cfloat":
        return 8
    else:
        return -1

    ######### Graph Generator ############


# Used by higher layer software to figure out how to connect blocks together.
def info_ports(args):
    TT_DATA = args["TT_DATA"]
    TP_WINDOW_VSIZE = args["TP_WINDOW_VSIZE"]
    TP_IN_API = args["TP_IN_API"]
    TP_OUT_API = args["TP_OUT_API"]
    TP_NUM_INPUTS = args["TP_NUM_INPUTS"]
    TP_NUM_OUTPUT_CLONES = args["TP_NUM_OUTPUT_CLONES"]
    TP_HEADER_BYTES = args["TP_HEADER_BYTES"]
    in_ports = com.get_port_info(
        "in",
        "in",
        TT_DATA,
        (TP_WINDOW_VSIZE + TP_HEADER_BYTES / local_sizeof(TT_DATA)),
        TP_NUM_INPUTS,
        0,
        TP_IN_API,
    )
    out_ports = com.get_port_info(
        "out",
        "out",
        TT_DATA,
        (TP_WINDOW_VSIZE + TP_HEADER_BYTES / local_sizeof(TT_DATA)),
        TP_NUM_OUTPUT_CLONES,
        0,
        TP_OUT_API,
    )

    return in_ports + out_ports  # concat lists


def gen_ports_code(args):
    TT_DATA = args["TT_DATA"]
    TP_WINDOW_VSIZE = args["TP_WINDOW_VSIZE"]
    TP_IN_API = args["TP_IN_API"]
    TP_OUT_API = args["TP_OUT_API"]
    TP_NUM_INPUTS = args["TP_NUM_INPUTS"]
    TP_NUM_OUTPUT_CLONES = args["TP_NUM_OUTPUT_CLONES"]
    TP_HEADER_BYTES = args["TP_HEADER_BYTES"]
    in_ports = f"  std::array<adf::port<input>, {TP_NUM_INPUTS}> in;\n"
    out_ports = f"  std::array<adf::port<output>, {TP_NUM_OUTPUT_CLONES}> out;\n"

    return in_ports + out_ports  # concat strings


def gen_inports_connections(args):
    TT_DATA = args["TT_DATA"]
    TP_WINDOW_VSIZE = args["TP_WINDOW_VSIZE"]
    TP_IN_API = args["TP_IN_API"]
    TP_OUT_API = args["TP_OUT_API"]
    TP_NUM_INPUTS = args["TP_NUM_INPUTS"]
    TP_NUM_OUTPUT_CLONES = args["TP_NUM_OUTPUT_CLONES"]
    TP_HEADER_BYTES = args["TP_HEADER_BYTES"]
    in_ports = f"      adf::connect<>(in[inIdx],widget_api_cast_graph.in[inIdx]);\n"

    return in_ports  # concat strings


def gen_outports_connections(args):
    TT_DATA = args["TT_DATA"]
    TP_WINDOW_VSIZE = args["TP_WINDOW_VSIZE"]
    TP_IN_API = args["TP_IN_API"]
    TP_OUT_API = args["TP_OUT_API"]
    TP_NUM_INPUTS = args["TP_NUM_INPUTS"]
    TP_NUM_OUTPUT_CLONES = args["TP_NUM_OUTPUT_CLONES"]
    TP_HEADER_BYTES = args["TP_HEADER_BYTES"]
    out_ports = (
        f"      adf::connect<>(widget_api_cast_graph.out[outIdx], out[outIdx]);\n"
    )

    return out_ports  # concat strings


def generate_graph(graphname, args):

    out = {}
    out["port_info"] = info_ports(args)
    ports_code = gen_ports_code(args)
    if graphname == "":
        graphname = "default_graphname"
    TT_DATA = args["TT_DATA"]
    TP_WINDOW_VSIZE = args["TP_WINDOW_VSIZE"]
    TP_IN_API = args["TP_IN_API"]
    TP_OUT_API = args["TP_OUT_API"]
    TP_NUM_INPUTS = args["TP_NUM_INPUTS"]
    TP_NUM_OUTPUT_CLONES = args["TP_NUM_OUTPUT_CLONES"]
    TP_PATTERN = args["TP_PATTERN"]
    TP_HEADER_BYTES = args["TP_HEADER_BYTES"]
    code = f"""
class {graphname} : public adf::graph {{
public:
  // ports
{ports_code}
  xf::dsp::aie::widget::api_cast::widget_api_cast_graph<
    {TT_DATA},              // TT_DATA
    {TP_IN_API},            // TP_IN_API
    {TP_OUT_API},           // TP_OUT_API
    {TP_NUM_INPUTS},        // TP_NUM_INPUTS
    {TP_WINDOW_VSIZE},      // TP_WINDOW_VSIZE
    {TP_NUM_OUTPUT_CLONES}, // TP_NUM_OUTPUT_CLONES
    {TP_PATTERN},           // TP_PATTERN
    {TP_HEADER_BYTES}       // TP_HEADER_BYTES
  > widget_api_cast_graph;
  {graphname}() : widget_api_cast_graph() {{
    //kernels
    //runtime_ratio
    //connections in loop
    for (unsigned inIdx = 0; inIdx < {TP_NUM_INPUTS}; inIdx++){{
{gen_inports_connections(args)}
    }}
    for (unsigned outIdx = 0; outIdx < {TP_NUM_OUTPUT_CLONES}; outIdx++){{
{gen_outports_connections(args)}
    }}

  }}

}};
"""
    out["graph"] = code
    out["headerfile"] = "dds_mixer_graph.hpp"
    out["searchpaths"] = [
        "L2/include/aie",
        "L2/tests/aie/common/inc",
        "L1/include/aie",
        "L1/src/aie",
        "L1/tests/aie/inc",
        "L1/tests/aie/src",
    ]
    return out
