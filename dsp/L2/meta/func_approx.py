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
# from ctypes import sizeof
import aie_common as com
from aie_common import *
import math


# ----------------------------------------
#######################################################
########### AIE_VARIANT Updater and Validator #########
#######################################################
def update_AIE_VARIANT(args):
    return fn_update_AIE_VARIANT()


def fn_update_AIE_VARIANT():
    legal_set_AIE_VARIANT = [com.AIE, com.AIE_ML, com.AIE_MLv2]

    param_dict = {}
    param_dict.update({"name": "AIE_VARIANT"})
    param_dict.update({"enum": legal_set_AIE_VARIANT})
    return param_dict


def validate_AIE_VARIANT(args):
    AIE_VARIANT = args["AIE_VARIANT"]
    return fn_validate_AIE_VARIANT(AIE_VARIANT)


def fn_validate_AIE_VARIANT(AIE_VARIANT):
    param_dict = fn_update_AIE_VARIANT()
    legal_set_AIE_VARIANT = param_dict["enum"]
    return validate_legal_set(legal_set_AIE_VARIANT, "AIE_VARIANT", AIE_VARIANT)


#######################################################
################## TT_DATA Updater ####################
#######################################################
def update_TT_DATA(args):
    AIE_VARIANT = args["AIE_VARIANT"]
    return fn_update_tt_data(AIE_VARIANT)


def fn_update_tt_data(AIE_VARIANT):
    if AIE_VARIANT == AIE:
        legal_set_TT_DATA = ["int16", "int32", "float"]
    elif AIE_VARIANT == AIE_ML:
        legal_set_TT_DATA = ["int16", "int32", "float", "bfloat16"]
    elif AIE_VARIANT == AIE_MLv2:
        legal_set_TT_DATA = ["int16", "int32", "float", "bfloat16"]

    param_dict = {}
    param_dict.update({"name": "TT_DATA"})
    param_dict.update({"enum": legal_set_TT_DATA})

    return param_dict


def validate_TT_DATA(args):
    AIE_VARIANT = args["AIE_VARIANT"]
    TT_DATA = args["TT_DATA"]
    return fn_validate_TT_DATA(AIE_VARIANT, TT_DATA)


def fn_validate_TT_DATA(AIE_VARIANT, TT_DATA):
    param_dict = fn_update_tt_data(AIE_VARIANT)
    legal_set_TT_DATA = param_dict["enum"]
    return validate_legal_set(legal_set_TT_DATA, "TT_DATA", TT_DATA)


#######################################################
########### TP_COARSE_BITS Updater and Validator ######
#######################################################
def update_TP_COARSE_BITS(args):
    AIE_VARIANT = args["AIE_VARIANT"]
    TT_DATA = args["TT_DATA"]
    return fn_update_tp_coarse_bits(AIE_VARIANT, TT_DATA)


def fn_update_tp_coarse_bits(AIE_VARIANT, TT_DATA):
    valuesPerLutSection = 2  # each lut section has a slope and offset value
    numLuts = 2  # two copies of lut exist
    lut128bitDuplication = (
        1  # linear_approx API requires each 128b of lut values to be duplicated.
    )
    dataMemoryVariant = k_data_memory_bytes[AIE_VARIANT]
    if AIE_VARIANT == AIE_ML or AIE_VARIANT == AIE_MLv2:
        # linear_approx api only supported on AIE-ML with int16 or bfloat16 datatypes
        if TT_DATA == "int16" or TT_DATA == "bfloat16":
            lut128bitDuplication = 2

    lutSectionMax=int(dataMemoryVariant/(fn_size_by_byte(TT_DATA) * numLuts * valuesPerLutSection * lut128bitDuplication))
    lutSectionMax_pp=int((dataMemoryVariant/2)/(fn_size_by_byte(TT_DATA) * numLuts * valuesPerLutSection * lut128bitDuplication))
    TP_COARSE_BITS_max = int(math.log2(lutSectionMax))
    TP_COARSE_BITS_max_pp = int(math.log2(lutSectionMax_pp))

    param_dict={}
    param_dict.update({"name" : "TP_COARSE_BITS"})
    param_dict.update({"minimum" : 1})
    param_dict.update({"maximum" : TP_COARSE_BITS_max})
    param_dict.update({"maximum_pingpong_buf" : TP_COARSE_BITS_max_pp})
    return param_dict

def validate_TP_COARSE_BITS(args):
    TP_COARSE_BITS = args["TP_COARSE_BITS"]
    param_dict = update_TP_COARSE_BITS(args)
    range_TP_COARSE_BITS = [param_dict["minimum"], param_dict["maximum"]]
    return validate_range(range_TP_COARSE_BITS, "TP_COARSE_BITS", TP_COARSE_BITS)


#######################################################
########### TP_FINE_BITS Updater and Validator #######
#######################################################
def update_TP_FINE_BITS(args):
    TT_DATA = args["TT_DATA"]
    TP_COARSE_BITS = args["TP_COARSE_BITS"]
    return fn_update_tp_FINE_bits(TT_DATA, TP_COARSE_BITS)


def fn_update_tp_FINE_bits(TT_DATA, TP_COARSE_BITS):
    if TT_DATA == "int32":
        unsignedBits = 31
    else:
        # This covers int16 and floats (since floats are cast to int16)
        unsignedBits = 15
    # This is the number of bits available after sign bit and COARSE_BITS are used
    TP_FINE_BITS_max = int(unsignedBits - TP_COARSE_BITS)

    param_dict = {}
    param_dict.update({"name": "TP_FINE_BITS"})
    param_dict.update({"minimum": 1})
    param_dict.update({"maximum": TP_FINE_BITS_max})
    return param_dict


def validate_TP_FINE_BITS(args):
    TP_FINE_BITS = args["TP_FINE_BITS"]
    param_dict = update_TP_FINE_BITS(args)
    range_TP_FINE_BITS = [param_dict["minimum"], param_dict["maximum"]]
    return validate_range(range_TP_FINE_BITS, "TP_FINE_BITS", TP_FINE_BITS)


#######################################################
########## TP_DOMAIN_MODE Updater and Validator #######
#######################################################
def update_TP_DOMAIN_MODE(args):
    return fn_update_tp_DOMAIN_MODE()


def fn_update_tp_DOMAIN_MODE():
    legal_set_TP_DOMAIN_MODE = [0, 1, 2]

    param_dict = {}
    param_dict.update({"name": "TP_DOMAIN_MODE"})
    param_dict.update({"enum": legal_set_TP_DOMAIN_MODE})
    return param_dict


def validate_TP_DOMAIN_MODE(args):
    TP_DOMAIN_MODE = args["TP_DOMAIN_MODE"]
    return fn_validate_TP_DOMAIN_MODE(TP_DOMAIN_MODE)


def fn_validate_TP_DOMAIN_MODE(TP_DOMAIN_MODE):
    param_dict = fn_update_tp_DOMAIN_MODE()
    legal_set_TP_DOMAIN_MODE = param_dict["enum"]
    return validate_legal_set(
        legal_set_TP_DOMAIN_MODE, "TP_DOMAIN_MODE", TP_DOMAIN_MODE
    )


#######################################################
########### TP_WINDOW_VSIZE Updater and Validator #####
#######################################################
def update_TP_WINDOW_VSIZE(args):
    AIE_VARIANT = args["AIE_VARIANT"]
    TT_DATA = args["TT_DATA"]
    return fn_update_tp_numFrames(AIE_VARIANT, TT_DATA)


def fn_update_tp_numFrames(AIE_VARIANT, TT_DATA):
    TP_WINDOW_VSIZE_max= int(k_data_memory_bytes[AIE_VARIANT]/(fn_size_by_byte(TT_DATA)))
    TP_WINDOW_VSIZE_max_pp= int(TP_WINDOW_VSIZE_max/2)
    param_dict={}
    param_dict.update({"name" : "TP_WINDOW_VSIZE"})
    param_dict.update({"minimum" : 16})
    param_dict.update({"maximum" : TP_WINDOW_VSIZE_max})
    param_dict.update({"maximum_pingpong_buf" : TP_WINDOW_VSIZE_max_pp})
    return param_dict


def validate_TP_WINDOW_VSIZE(args):
    TP_WINDOW_VSIZE = args["TP_WINDOW_VSIZE"]
    param_dict = update_TP_WINDOW_VSIZE(args)
    range_TP_WINDOW_VSIZE = [param_dict["minimum"], param_dict["maximum"]]
    return validate_range(range_TP_WINDOW_VSIZE, "TP_WINDOW_VSIZE", TP_WINDOW_VSIZE)


#######################################################
########### TP_SHIFT Updater and Validator ############
#######################################################
def update_TP_SHIFT(args):
    TT_DATA = args["TT_DATA"]
    AIE_VARIANT = args["AIE_VARIANT"]
    return fn_update_TP_SHIFT(AIE_VARIANT, TT_DATA)


def fn_update_TP_SHIFT(AIE_VARIANT, TT_DATA):
    range_TP_SHIFT = com.fn_update_range_TP_SHIFT(AIE_VARIANT, TT_DATA)

    param_dict = {
        "name": "TP_SHIFT",
        "minimum": range_TP_SHIFT[0],
        "maximum": range_TP_SHIFT[1],
    }
    return param_dict


def validate_TP_SHIFT(args):
    TT_DATA = args["TT_DATA"]
    TP_SHIFT = args["TP_SHIFT"]
    AIE_VARIANT = args["AIE_VARIANT"]
    return fn_validate_TP_SHIFT(AIE_VARIANT, TT_DATA, TP_SHIFT)


def fn_validate_TP_SHIFT(AIE_VARIANT, TT_DATA, TP_SHIFT):
    param_dict = fn_update_TP_SHIFT(AIE_VARIANT, TT_DATA)
    range_TP_SHIFT = [param_dict["minimum"], param_dict["maximum"]]
    return validate_range(range_TP_SHIFT, "TP_SHIFT", TP_SHIFT)


#######################################################
########### TP_RND Updater and Validator #############
#######################################################
def update_TP_RND(args):
    AIE_VARIANT = args["AIE_VARIANT"]
    return fn_update_tp_rnd(AIE_VARIANT)


def fn_update_tp_rnd(AIE_VARIANT):

    legal_set_TP_RND = fn_get_legalSet_roundMode(AIE_VARIANT)
    param_dict = {}
    param_dict.update({"name": "TP_RND"})
    param_dict.update({"enum": legal_set_TP_RND})

    return param_dict


def validate_TP_RND(args):
    AIE_VARIANT = args["AIE_VARIANT"]
    TP_RND = args["TP_RND"]
    return fn_validate_TP_RND(AIE_VARIANT, TP_RND)


def fn_validate_TP_RND(AIE_VARIANT, TP_RND):
    legal_set_TP_RND = fn_get_legalSet_roundMode(AIE_VARIANT)
    return validate_legal_set(legal_set_TP_RND, "TP_RND", TP_RND)


#######################################################
########### ##TP_SAT Updater and Validator ############
#######################################################
def update_TP_SAT(args):
    return fn_update_tp_sat()


def fn_update_tp_sat():
    legal_set = [0, 1, 3]

    param_dict = {}
    param_dict.update({"name": "TP_SAT"})
    param_dict.update({"enum": legal_set})
    return param_dict


def validate_TP_SAT(args):
    TP_SAT = args["TP_SAT"]
    return fn_validate_TP_SAT(TP_SAT)


def fn_validate_TP_SAT(TP_SAT):
    param_dict = fn_update_tp_sat()
    legal_set_TP_SAT = param_dict["enum"]
    return validate_legal_set(legal_set_TP_SAT, "TP_SAT", TP_SAT)


#######################################################
######lookup values  Updater and Validator ############
#######################################################
def update_lookup_values(args):
    TP_COARSE_BITS = args["TP_COARSE_BITS"]
    return fn_update_lut_values(TP_COARSE_BITS)


def fn_update_lut_values(TP_COARSE_BITS):
    valuesInLut = 2 << TP_COARSE_BITS
    param_dict = {}
    param_dict.update({"name": "lookup_values"})
    param_dict.update({"len": valuesInLut})
    return param_dict


def validate_lookup_values(args):
    lut = args["lookup_values"]
    TP_COARSE_BITS = args["TP_COARSE_BITS"]
    return fn_validate_lut_values(TP_COARSE_BITS, lut)


def fn_validate_lut_values(TP_COARSE_BITS, lut_list):
    param_dict = fn_update_lut_values(TP_COARSE_BITS)
    len_lut = param_dict["len"]
    return validate_LUT_len(lut_list, len_lut)

    ######### Graph Generator ############


# Used by higher layer software to figure out how to connect blocks together.
def info_ports(args):
    """Standard function creating a static dictionary of information
    for upper software to correctly connect the IP.
    Some IP has dynamic number of ports according to parameter set,
    so port information has to be implemented as a function"""
    TT_DATA = args["TT_DATA"]
    TP_COARSE_BITS = args["TP_COARSE_BITS"]
    TP_FINE_BITS = args["TP_FINE_BITS"]
    TP_WINDOW_VSIZE = args["TP_WINDOW_VSIZE"]
    TP_SHIFT = args["TP_SHIFT"]

    in_ports = get_port_info("in", "in", TT_DATA, TP_WINDOW_VSIZE, 1, 0)
    out_ports = get_port_info("out", "out", TT_DATA, TP_WINDOW_VSIZE, 1, 0)
    return in_ports + out_ports

    ######### Parameter Range Generator ############


def info_params(args):
    """Standard function creating a static dictionary of information
    for upper software to correctly update the IP GUI.
    """
    info_TT_DATA = [update_TT_DATA(args)]
    info_TP_COARSE_BITS = [update_TP_COARSE_BITS(args)]
    info_TP_FINE_BITS = [update_TP_FINE_BITS(args)]
    info_TP_DOMAIN_MODE = [update_TP_DOMAIN_MODE(args)]
    info_TP_WINDOW_VSIZE = [update_TP_WINDOW_VSIZE(args)]
    info_TP_SHIFT = [update_TP_SHIFT(args)]
    info_TP_RND = [update_TP_RND(args)]
    info_TP_SAT = [update_TP_SAT(args)]

    return (
        info_TT_DATA
        + info_TP_COARSE_BITS
        + info_TP_FINE_BITS
        + info_TP_DOMAIN_MODE
        + info_TP_WINDOW_VSIZE
        + info_TP_SHIFT
        + info_TP_SHIFT
        + info_TP_RND
        + info_TP_SAT
    )


def fn_get_lookup_vector(TT_DATA, lut_list):
    # todo, reformat this to use list comprehension
    lut = f"{{"
    lut += ", ".join([str(lut_list[i]) for i in range(len(lut_list))])
    lut += f"}}"
    return lut


def generate_graph(graphname, args):

    if graphname == "":
        graphname = "default_graphname"
    TT_DATA = args["TT_DATA"]
    TP_COARSE_BITS = args["TP_COARSE_BITS"]
    TP_FINE_BITS = args["TP_FINE_BITS"]
    TP_DOMAIN_MODE = args["TP_DOMAIN_MODE"]
    TP_WINDOW_VSIZE = args["TP_WINDOW_VSIZE"]
    TP_SHIFT = args["TP_SHIFT"]
    TP_RND = args["TP_RND"]
    TP_SAT = args["TP_SAT"]
    lookup_list = args["lookup_values"]

    lookup_vector = fn_get_lookup_vector(TT_DATA, lookup_list)

    code = f"""

class {graphname} : public adf::graph {{
public:
  // ports
  std::array<adf::port<input>, 1> in;
  std::array<adf::port<output>, 1> out;

  static constexpr int sectionTotal = 1 << COARSE_BITS;
  static constexpr int sectionWidth = 1 << FINE_BITS;
  static constexpr int lutSize = sectionTotal * 2;
  std::vector<DATA_TYPE> m_luts_ab, m_luts_cd;

  xf::dsp::aie::func_approx<
    {TT_DATA}, // TT_DATA
    {TP_COARSE_BITS}, // TP_COARSE_BITS
    {TP_FINE_BITS}, // TP_FINE_BITS
    {TP_DOMAIN_MODE}, // TP_DOMAIN_MODE
    {TP_WINDOW_VSIZE}, // TP_WINDOW_VSIZE
    {TP_SHIFT}, // TP_SHIFT
    {TP_RND}, //TP_RND
    {TP_SAT} //TP_SAT
    > func_approx_graph;

    m_luts_ab = {lookup_vector};

    {graphname}() : func_approx_graph(m_luts_ab) {{
      adf::connect<> net_in(in[0], func_approx_graph.in[0]);
      adf::connect<> net_out(func_approx_graph.out[0], out[0]);
    }}

}};
"""
    out = {}
    out["graph"] = code
    out["port_info"] = info_ports(args)
    out["headerfile"] = ["func_approx_graph.hpp"]
    out["searchpaths"] = [
        "L2/include/aie",
        "L2/tests/aie/common/inc",
        "L1/include/aie",
        "L1/src/aie",
        "L1/tests/aie/inc",
        "L1/tests/aie/src",
    ]
    return out


# GUI config generator output
# _aie refers to AIE IP (L2 graph top level)
def update_params(args):
    out = {}
    out["headerfile"] = ["func_approx_graph.hpp"]
    out["searchpaths"] = [
        "L2/include/aie",
        "L2/tests/aie/common/inc",
        "L1/include/aie",
        "L1/src/aie",
        "L1/tests/aie/inc",
        "L1/tests/aie/src",
    ]
    out["parameters"] = info_params(args)
    return out
