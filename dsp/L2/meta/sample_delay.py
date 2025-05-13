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

UINT_max_cpp = 2**31


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
###########TT_DATA Updater and Validator ##############
#######################################################
def update_TT_DATA(args):
    return fn_update_TT_DATA()


def fn_update_TT_DATA():
    legal_set_tt_data = [
        "int8",
        "int16",
        "cint16",
        "int32",
        "cint32",
        "float",
        "cfloat",
        "uint8",
    ]
    param_dict = {"name": "TT_DATA", "enum": legal_set_tt_data}
    return param_dict


def validate_TT_DATA(args):
    TT_DATA = args["TT_DATA"]
    return fn_validate_TT_DATA(TT_DATA)


def fn_validate_TT_DATA(TT_DATA):
    param_dict = fn_update_TT_DATA()
    return com.validate_legal_set(param_dict["enum"], "TT_DATA", TT_DATA)


#######################################################
########### TP_API Updater and Validator ##############
#######################################################
def update_TP_API(args):
    return fn_update_TP_API()


def fn_update_TP_API():
    legal_set_TP_API = [0, 1]
    param_dict = {"name": "TP_API", "enum": legal_set_TP_API}
    return param_dict


def validate_TP_API(args):
    TP_API = args["TP_API"]
    return fn_validate_TP_API(TP_API)


def fn_validate_TP_API(TP_API):
    param_dict = fn_update_TP_API()
    legal_set = param_dict["enum"]
    return com.validate_legal_set(legal_set, "TP_API", TP_API)


#######################################################
########## TP_MAX_DELAY Updater and Validator #########
#######################################################
def update_TP_MAX_DELAY(args):
    AIE_VARIANT = args["AIE_VARIANT"]
    TT_DATA = args["TT_DATA"]
    TP_API = args["TP_API"]
    if ("TP_MAX_DELAY" in args) and args["TP_MAX_DELAY"]:
        TP_MAX_DELAY = args["TP_MAX_DELAY"]
    else:
        TP_MAX_DELAY = 0
    return fn_update_TP_MAX_DELAY(AIE_VARIANT, TT_DATA, TP_API, TP_MAX_DELAY)


def fn_update_TP_MAX_DELAY(AIE_VARIANT, TT_DATA, TP_API, TP_MAX_DELAY):
    if TP_API:  # stream interface
        TP_MAX_DELAY_min = 128
    else:  # iobuff (window) interface
        VEC_SIZE = 256 / 8 / com.fn_size_by_byte(TT_DATA)
        TP_MAX_DELAY_min = int(VEC_SIZE)

    if not (TP_API):
        buffer_size = com.k_data_memory_bytes[AIE_VARIANT]
        buffer_sample = int(buffer_size / com.fn_size_by_byte(TT_DATA))
        TP_WINDOW_VSIZE_min = TP_MAX_DELAY_min
        TP_MAX_DELAY_max = buffer_sample - TP_WINDOW_VSIZE_min
        TP_MAX_DELAY_max_pp = (buffer_sample / 2) - TP_WINDOW_VSIZE_min
    else:
        TP_MAX_DELAY_max = UINT_max_cpp
        TP_MAX_DELAY_max_pp = UINT_max_cpp

    param_dict = {
        "name": "TP_MAX_DELAY",
        "minimum": TP_MAX_DELAY_min,
        "maximum": TP_MAX_DELAY_max,
        "maximum_pingpong_buf": TP_MAX_DELAY_max_pp,
    }

    if (TP_MAX_DELAY != 0) and (TP_MAX_DELAY % TP_MAX_DELAY_min != 0):
        TP_MAX_DELAY_act = int(
            round(TP_MAX_DELAY / TP_MAX_DELAY_min) * TP_MAX_DELAY_min
        )

        if TP_MAX_DELAY_act < param_dict["minimum"]:
            TP_MAX_DELAY_act = param_dict["minimum"]

        if TP_MAX_DELAY_act > param_dict["maximum"]:
            TP_MAX_DELAY_act = int(com.FLOOR(param_dict["maximum"], TP_MAX_DELAY_min))
        param_dict.update({"actual": TP_MAX_DELAY_act})
    return param_dict


def validate_TP_MAX_DELAY(args):
    AIE_VARIANT = args["AIE_VARIANT"]
    TT_DATA = args["TT_DATA"]
    TP_API = args["TP_API"]
    TP_MAX_DELAY = args["TP_MAX_DELAY"]
    return fn_validate_TP_MAX_DELAY(AIE_VARIANT, TT_DATA, TP_API, TP_MAX_DELAY)


def fn_validate_TP_MAX_DELAY(AIE_VARIANT, TT_DATA, TP_API, TP_MAX_DELAY):
    param_dict = fn_update_TP_MAX_DELAY(AIE_VARIANT, TT_DATA, TP_API, TP_MAX_DELAY)

    TP_MAX_DELAY_min = param_dict["minimum"]
    if TP_MAX_DELAY % TP_MAX_DELAY_min != 0:
        return com.isError(f"TP_MAX_DELAY should be a multiple of {TP_MAX_DELAY_min}")
    else:
        range_TP_MAX_DELAY = [param_dict["minimum"], param_dict["maximum"]]
        return com.validate_range(range_TP_MAX_DELAY, "TP_MAX_DELAY", TP_MAX_DELAY)


#######################################################
####### TP_WINDOW_VSIZE Updater and Validator #########
#######################################################
def update_TP_WINDOW_VSIZE(args):
    AIE_VARIANT = args["AIE_VARIANT"]
    TT_DATA = args["TT_DATA"]
    TP_API = args["TP_API"]
    TP_MAX_DELAY = args["TP_MAX_DELAY"]
    if ("TP_WINDOW_VSIZE" in args) and args["TP_WINDOW_VSIZE"]:
        TP_WINDOW_VSIZE = args["TP_WINDOW_VSIZE"]
    else:
        TP_WINDOW_VSIZE = 0
    return fn_update_TP_WINDOW_VSIZE(
        AIE_VARIANT, TT_DATA, TP_API, TP_MAX_DELAY, TP_WINDOW_VSIZE
    )


def fn_update_TP_WINDOW_VSIZE(
    AIE_VARIANT, TT_DATA, TP_API, TP_MAX_DELAY, TP_WINDOW_VSIZE
):
    if TP_API:
        TP_WINDOW_VSIZE_min = TP_MAX_DELAY
    else:
        VEC_SIZE = 256 / 8 / com.fn_size_by_byte(TT_DATA)
        TP_WINDOW_VSIZE_min = int(VEC_SIZE)

    if not (TP_API):
        buffer_size = com.k_data_memory_bytes[AIE_VARIANT]
        buffer_sample = int(buffer_size / com.fn_size_by_byte(TT_DATA))
        TP_WINDOW_VSIZE_max = buffer_sample - TP_MAX_DELAY
        TP_WINDOW_VSIZE_max_pp = buffer_sample / 2 - TP_MAX_DELAY

    else:
        TP_WINDOW_VSIZE_max = UINT_max_cpp
        TP_WINDOW_VSIZE_max_pp = UINT_max_cpp

    param_dict = {
        "name": "TP_WINDOW_VSIZE",
        "minimum": TP_WINDOW_VSIZE_min,
        "maximum": TP_WINDOW_VSIZE_max,
        "maximum_pingpong_buf": TP_WINDOW_VSIZE_max_pp,
    }

    if (TP_WINDOW_VSIZE != 0) and (TP_WINDOW_VSIZE % TP_WINDOW_VSIZE_min != 0):
        TP_WINDOW_VSIZE_act = int(
            round(TP_WINDOW_VSIZE / TP_WINDOW_VSIZE_min) * TP_WINDOW_VSIZE_min
        )

        if TP_WINDOW_VSIZE_act < param_dict["minimum"]:
            TP_WINDOW_VSIZE_act = param_dict["minimum"]

        if TP_WINDOW_VSIZE_act > param_dict["maximum"]:
            TP_WINDOW_VSIZE_act = int(
                com.FLOOR(param_dict["maximum"], TP_WINDOW_VSIZE_min)
            )
        param_dict.update({"actual": TP_WINDOW_VSIZE_act})
    return param_dict


def validate_TP_WINDOW_VSIZE(args):
    AIE_VARIANT = args["AIE_VARIANT"]
    TT_DATA = args["TT_DATA"]
    TP_API = args["TP_API"]
    TP_MAX_DELAY = args["TP_MAX_DELAY"]
    TP_WINDOW_VSIZE = args["TP_WINDOW_VSIZE"]
    return fn_validate_TP_WINDOW_VSIZE(
        AIE_VARIANT, TT_DATA, TP_API, TP_MAX_DELAY, TP_WINDOW_VSIZE
    )


def fn_validate_TP_WINDOW_VSIZE(
    AIE_VARIANT, TT_DATA, TP_API, TP_MAX_DELAY, TP_WINDOW_VSIZE
):
    param_dict = fn_update_TP_WINDOW_VSIZE(
        AIE_VARIANT, TT_DATA, TP_API, TP_MAX_DELAY, TP_WINDOW_VSIZE
    )
    TP_WINDOW_VSIZE_min = param_dict["minimum"]
    if TP_WINDOW_VSIZE % TP_WINDOW_VSIZE_min != 0:
        return com.isError(
            f"TP_WINDOW_VSIZE should be a multiple of {TP_WINDOW_VSIZE_min}"
        )
    else:
        range_TP_WINDOW_VSIZE = [param_dict["minimum"], param_dict["maximum"]]
        return com.validate_range(
            range_TP_WINDOW_VSIZE, "TP_WINDOW_VSIZE", TP_WINDOW_VSIZE
        )


def info_ports(args):
    TT_DATA = args["TT_DATA"]
    TP_WINDOW_VSIZE = args["TP_WINDOW_VSIZE"]
    TP_API = args["TP_API"]
    TP_MAX_DELAY = args["TP_MAX_DELAY"]

    in_1 = com.get_port_info("in", "in", TT_DATA, TP_WINDOW_VSIZE, None, 0, TP_API)
    in_2 = com.get_parameter_port_info(
        "numSampleDelay", "in", "uint32", None, 1, "async"
    )
    out_1 = com.get_port_info("out", "out", TT_DATA, TP_WINDOW_VSIZE, None, 0, TP_API)
    return in_1 + in_2 + out_1


def generate_graph(graphname, args):

    if graphname == "":
        graphname = "default_graphname"
    TT_DATA = args["TT_DATA"]
    TP_WINDOW_VSIZE = args["TP_WINDOW_VSIZE"]
    TP_API = args["TP_API"]
    TP_MAX_DELAY = args["TP_MAX_DELAY"]
    code = f"""
class {graphname} : public adf::graph {{
public:
  adf::port<input> in;
  adf::port<input> numSampleDelay;
  adf::port<output> out;
  xf::dsp::aie::sample_delay::sample_delay_graph<
  {TT_DATA},
  {TP_WINDOW_VSIZE},
  {TP_API},
  {TP_MAX_DELAY}
  > sample_delay_graph;

  {graphname}() : sample_delay_graph(){{
  adf::connect<>(in, sample_delay_graph.in);
  adf::connect<>(sample_delay_graph.out, out);
  adf::connect<>(numSampleDelay, sample_delay_graph.numSampleDelay); // RTP
  }}

}};
"""
    out = {}
    out["graph"] = code
    out["port_info"] = info_ports(args)
    out["headerfile"] = "sample_delay_graph.hpp"
    out["searchpaths"] = [
        "L2/include/aie",
        "L2/tests/aie/common/inc",
        "L1/include/aie",
        "L1/src/aie",
        "L1/tests/aie/inc",
        "L1/tests/aie/src",
    ]

    return out
