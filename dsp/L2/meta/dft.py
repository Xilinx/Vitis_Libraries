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
from aie_common import *
import math

# import json

# Range for params
TP_POINT_SIZE_min = 4
TP_POINT_SIZE_max = 128
TP_SSR_min_static = 1
TP_SSR_max_static = 16


# Function to number of lanes in a vector. AIE_VARIANT=2 uses 512-bits vector for cint32 and cfloat data
def fn_samples_in_dft_vector(AIE_VARIANT, TT_DATA):
    if AIE_VARIANT == AIE_ML or AIE_VARIANT == AIE_MLv2:
        kSamplesDataVector = (
            8  # optimized at 512-bits for cint32, cfloat but 256-bits for cint16
        )
    else:
        kSamplesDataVector = 256 / 8 / fn_size_by_byte(TT_DATA)
    return kSamplesDataVector


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
########### TT_DATA Updater and Validato###############
#######################################################
def update_TT_DATA(args):
    AIE_VARIANT = args["AIE_VARIANT"]
    return fn_update_tt_data(AIE_VARIANT)


def fn_update_tt_data(AIE_VARIANT):
    if AIE_VARIANT == AIE:
        legal_set_TT_DATA = ["cint16", "cint32", "cfloat"]
    elif AIE_VARIANT == AIE_ML or AIE_VARIANT == AIE_MLv2:
        legal_set_TT_DATA = ["cint16", "cint32"]

    param_dict = {}
    param_dict.update({"name": "TT_DATA"})
    param_dict.update({"enum": legal_set_TT_DATA})

    return param_dict


def validate_TT_DATA(args):
    TT_DATA = args["TT_DATA"]
    AIE_VARIANT = args["AIE_VARIANT"]
    return fn_validate_TT_DATA(AIE_VARIANT, TT_DATA)


def fn_validate_TT_DATA(AIE_VARIANT, TT_DATA):
    param_dict = fn_update_tt_data(AIE_VARIANT)
    legal_set_TT_DATA = param_dict["enum"]
    return validate_legal_set(legal_set_TT_DATA, "TT_DATA", TT_DATA)


#######################################################
########### TT_TWIDDLE Updater and Validator ###########
#######################################################
def update_TT_TWIDDLE(args):
    TT_DATA = args["TT_DATA"]
    return fn_update_tt_twiddle(TT_DATA)


def fn_update_tt_twiddle(TT_DATA):
    if TT_DATA == "cint16" or TT_DATA == "cint32":
        legal_set_TT_TWIDDLE = ["cint16"]

    elif TT_DATA == "cfloat":
        legal_set_TT_TWIDDLE = ["cfloat"]

    param_dict = {}
    param_dict.update({"name": "TT_TWIDDLE"})
    param_dict.update({"enum": legal_set_TT_TWIDDLE})
    return param_dict


def validate_TT_TWIDDLE(args):
    TT_DATA = args["TT_DATA"]
    TT_TWIDDLE = args["TT_TWIDDLE"]
    return fn_validate_TT_TWIDDLE(TT_DATA, TT_TWIDDLE)


def fn_validate_TT_TWIDDLE(TT_DATA, TT_TWIDDLE):
    param_dict = fn_update_tt_twiddle(TT_DATA)
    legal_set_TT_TWIDDLE = param_dict["enum"]
    return validate_legal_set(legal_set_TT_TWIDDLE, "TT_TWIDDLE", TT_TWIDDLE)


#######################################################
########### TP_POINT_SIZE Updater and Validator ###########
#######################################################
def update_TP_POINT_SIZE(args):

    return fn_update_tp_point_size()


def fn_update_tp_point_size():
    maxPtSize = TP_POINT_SIZE_max

    param_dict = {}
    param_dict.update({"name": "TP_POINT_SIZE"})
    param_dict.update({"minimum": TP_POINT_SIZE_min})
    param_dict.update({"maximum": maxPtSize})

    return param_dict


def validate_TP_POINT_SIZE(args):
    TP_POINT_SIZE = args["TP_POINT_SIZE"]
    return fn_validate_TP_POINT_SIZE(TP_POINT_SIZE)


def fn_validate_TP_POINT_SIZE(TP_POINT_SIZE):
    param_dict = fn_update_tp_point_size()
    range_TP_POINT_SIZE = [param_dict["minimum"], param_dict["maximum"]]
    return validate_range(range_TP_POINT_SIZE, "TP_POINT_SIZE", TP_POINT_SIZE)


#######################################################
########### TP_SSR Updater and Validator ##############
#######################################################
def update_TP_SSR(args):
    AIE_VARIANT = args["AIE_VARIANT"]
    TT_DATA = args["TT_DATA"]
    TP_POINT_SIZE = args["TP_POINT_SIZE"]
    return fn_update_tp_ssr(AIE_VARIANT, TT_DATA, TP_POINT_SIZE)


def fn_update_tp_ssr(AIE_VARIANT, TT_DATA, TP_POINT_SIZE):
    kSamplesDataVector = fn_samples_in_dft_vector(AIE_VARIANT, TT_DATA)
    TP_SSR_min = TP_SSR_min_static
    # Data can be padded to a maximum of 2 * TP_POINT_SIZE to achieve maximum ssr
    TP_SSR_max = min(TP_SSR_max_static, int(2 * TP_POINT_SIZE / kSamplesDataVector))
    param_dict = {}
    param_dict.update({"name": "TP_SSR"})
    param_dict.update({"minimum": TP_SSR_min})
    param_dict.update({"maximum": TP_SSR_max})

    return param_dict


def validate_TP_SSR(args):
    AIE_VARIANT = args["AIE_VARIANT"]
    TT_DATA = args["TT_DATA"]
    TP_POINT_SIZE = args["TP_POINT_SIZE"]
    TP_SSR = args["TP_SSR"]
    return fn_validate_TP_SSR(AIE_VARIANT, TT_DATA, TP_POINT_SIZE, TP_SSR)


def fn_validate_TP_SSR(AIE_VARIANT, TT_DATA, TP_POINT_SIZE, TP_SSR):
    param_dict = fn_update_tp_ssr(AIE_VARIANT, TT_DATA, TP_POINT_SIZE)
    range_TP_SSR = [param_dict["minimum"], param_dict["maximum"]]
    return validate_range(range_TP_SSR, "TP_SSR", TP_SSR)


#######################################################
########### TP_CASC_LEN Updater and Validator ###########
#######################################################
def update_TP_CASC_LEN(args):
    AIE_VARIANT = args["AIE_VARIANT"]
    TT_DATA = args["TT_DATA"]
    TT_TWIDDLE = args["TT_TWIDDLE"]
    TP_POINT_SIZE = args["TP_POINT_SIZE"]
    TP_SSR = args["TP_SSR"]
    return fn_update_tp_casc_len(
        AIE_VARIANT, TT_DATA, TT_TWIDDLE, TP_POINT_SIZE, TP_SSR
    )


def fn_update_tp_casc_len(AIE_VARIANT, TT_DATA, TT_TWIDDLE, TP_POINT_SIZE, TP_SSR):
    [TP_CASC_LEN_min, TP_CASC_LEN_max, CASC_LEN_min_pingpong_buf] = (
        fn_calc_casc_len_range(AIE_VARIANT, TT_DATA, TT_TWIDDLE, TP_POINT_SIZE, TP_SSR)
    )
    param_dict = {}
    param_dict.update({"name": "TP_CASC_LEN"})
    param_dict.update({"minimum": TP_CASC_LEN_min})
    param_dict.update({"maximum": TP_CASC_LEN_max})
    param_dict.update({"minimum_pingpong_buf": CASC_LEN_min_pingpong_buf})

    return param_dict


def fn_calc_casc_len_range(AIE_VARIANT, TT_DATA, TT_TWIDDLE, TP_POINT_SIZE, TP_SSR):

    maxDataMemBytes = k_data_memory_bytes[AIE_VARIANT]
    # The column dimension of the coefficients are padded
    COEFF_COL_DIM = CEIL(TP_POINT_SIZE, fn_size_by_byte(TT_TWIDDLE)) / TP_SSR
    # Total size of coefficient array for each SSR rank.
    # This is used to find the minimum CASC_LEN (min number of kernel memories required to store coefficients)
    COEFF_SIZE_PER_SSR = TP_POINT_SIZE * COEFF_COL_DIM
    CASC_LEN_min = math.ceil(
        fn_size_by_byte(TT_TWIDDLE) * COEFF_SIZE_PER_SSR / maxDataMemBytes
    )
    CASC_LEN_min_pingpong_buf = math.ceil(
        2 * (fn_size_by_byte(TT_TWIDDLE) * COEFF_SIZE_PER_SSR / maxDataMemBytes)
    )

    kSamplesDataVector = fn_samples_in_dft_vector(AIE_VARIANT, TT_DATA)
    TP_CASC_LEN_min = int(CASC_LEN_min)
    # Data can be padded to a maximum of 2 * TP_POINT_SIZE to achieve maximum casc_len
    TP_CASC_LEN_max = int(2 * TP_POINT_SIZE / (kSamplesDataVector))

    return [TP_CASC_LEN_min, TP_CASC_LEN_max, CASC_LEN_min_pingpong_buf]


def validate_TP_CASC_LEN(args):
    AIE_VARIANT = args["AIE_VARIANT"]
    TT_DATA = args["TT_DATA"]
    TT_TWIDDLE = args["TT_TWIDDLE"]
    TP_POINT_SIZE = args["TP_POINT_SIZE"]
    TP_SSR = args["TP_SSR"]
    TP_CASC_LEN = args["TP_CASC_LEN"]
    return fn_validate_TP_CASC_LEN(
        AIE_VARIANT, TT_DATA, TT_TWIDDLE, TP_POINT_SIZE, TP_SSR, TP_CASC_LEN
    )


def fn_validate_TP_CASC_LEN(
    AIE_VARIANT, TT_DATA, TT_TWIDDLE, TP_POINT_SIZE, TP_SSR, TP_CASC_LEN
):
    param_dict = fn_update_tp_casc_len(
        AIE_VARIANT, TT_DATA, TT_TWIDDLE, TP_POINT_SIZE, TP_SSR
    )
    range_TP_CASC_LEN = [param_dict["minimum"], param_dict["maximum"]]
    return validate_range(range_TP_CASC_LEN, "TP_CASC_LEN", TP_CASC_LEN)


#######################################################
########### TP_NUM_FRAMES Updater and Validator ###########
#######################################################


def update_TP_NUM_FRAMES(args):
    AIE_VARIANT = args["AIE_VARIANT"]
    TT_DATA = args["TT_DATA"]
    TP_POINT_SIZE = args["TP_POINT_SIZE"]
    return fn_update_tp_numFrames(AIE_VARIANT, TT_DATA, TP_POINT_SIZE)


def fn_update_tp_numFrames(AIE_VARIANT, TT_DATA, TP_POINT_SIZE):

    TP_NUM_FRAMES_max = int(
        k_data_memory_bytes[AIE_VARIANT] / (fn_size_by_byte(TT_DATA) * TP_POINT_SIZE)
    )
    TP_NUM_FRAMES_max_pingpong_buf = int(TP_NUM_FRAMES_max / 2)

    param_dict = {}
    param_dict.update({"name": "TP_NUM_FRAMES"})
    param_dict.update({"minimum": 1})
    param_dict.update({"maximum": TP_NUM_FRAMES_max})
    param_dict.update({"maximum_pingpong_buf": TP_NUM_FRAMES_max_pingpong_buf})

    return param_dict


def validate_TP_NUM_FRAMES(args):
    AIE_VARIANT = args["AIE_VARIANT"]
    TT_DATA = args["TT_DATA"]
    TP_POINT_SIZE = args["TP_POINT_SIZE"]
    TP_NUM_FRAMES = args["TP_NUM_FRAMES"]
    return fn_validate_TP_NUM_FRAMES(AIE_VARIANT, TT_DATA, TP_POINT_SIZE, TP_NUM_FRAMES)


def fn_validate_TP_NUM_FRAMES(AIE_VARIANT, TT_DATA, TP_POINT_SIZE, TP_NUM_FRAMES):
    param_dict = fn_update_tp_numFrames(AIE_VARIANT, TT_DATA, TP_POINT_SIZE)
    range_TP_NUM_FRAMES = [param_dict["minimum"], param_dict["maximum"]]
    return validate_range(range_TP_NUM_FRAMES, "TP_NUM_FRAMES", TP_NUM_FRAMES)


#######################################################
########### TP_SHIFT Updater and Validator ###########
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
########### TP_RND Updater and Validator ###########
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
########### TP_SAT Updater and Validator ###########
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
########### TP_API Updater and Validator ###########
#######################################################
def update_TP_API(args):
    return fn_update_tp_api()


def fn_update_tp_api():
    legal_set_TP_API = [0]

    param_dict = {}
    param_dict.update({"name": "TP_API"})
    param_dict.update({"enum": legal_set_TP_API})
    return param_dict


def validate_TP_API(args):
    TP_API = args["TP_API"]
    return fn_validate_TP_API(TP_API)


def fn_validate_TP_API(TP_API):
    param_dict = fn_update_tp_api()
    legal_set_TP_API = param_dict["enum"]
    return validate_legal_set(legal_set_TP_API, "TP_API", TP_API)


#######################################################
########### TP_API Updater and Validator ###########
#######################################################
def update_TP_FFT_NIFFT(args):
    return fn_update_tp_fft_nifft()


def fn_update_tp_fft_nifft():
    legal_set_TP_FFT_NIFFT = [0, 1]

    param_dict = {}
    param_dict.update({"name": "TP_FFT_NIFFT"})
    param_dict.update({"enum": legal_set_TP_FFT_NIFFT})
    return param_dict


def validate_TP_FFT_NIFFT(args):
    TP_FFT_NIFFT = args["TP_FFT_NIFFT"]
    return fn_validate_TP_FFT_NIFFT(TP_FFT_NIFFT)


def fn_validate_TP_FFT_NIFFT(TP_FFT_NIFFT):
    param_dict = fn_update_tp_fft_nifft()
    legal_set_TP_FFT_NIFFT = param_dict["enum"]
    return validate_legal_set(legal_set_TP_FFT_NIFFT, "TP_FFT_NIFFT", TP_FFT_NIFFT)


########## Functions ##########


# Used by higher layer software to figure out how to connect blocks together.
def get_window_sizes(
    AIE_VARIANT, TT_DATA, TP_POINT_SIZE, TP_NUM_FRAMES, TP_CASC_LEN, TP_SSR
):
    kSamplesDataVector = fn_samples_in_dft_vector(AIE_VARIANT, TT_DATA)
    OUT_WINDOW_VSIZE = (
        CEIL(TP_POINT_SIZE, (kSamplesDataVector * TP_SSR)) / TP_SSR
    ) * TP_NUM_FRAMES
    IN_WINDOW_VSIZE = (
        CEIL(TP_POINT_SIZE, (kSamplesDataVector * TP_CASC_LEN)) / TP_CASC_LEN
    ) * TP_NUM_FRAMES
    return IN_WINDOW_VSIZE, OUT_WINDOW_VSIZE

    ######### Graph Generator ############


def info_ports(args):
    """Standard function creating a static dictionary of information
    for upper software to correctly connect the IP.
    Some IP has dynamic number of ports according to parameter set,
    so port information has to be implemented as a function"""
    AIE_VARIANT = args["AIE_VARIANT"]
    TT_DATA = args["TT_DATA"]
    TP_POINT_SIZE = args["TP_POINT_SIZE"]
    TP_CASC_LEN = args["TP_CASC_LEN"]
    TP_SSR = args["TP_SSR"]
    TP_NUM_FRAMES = args["TP_NUM_FRAMES"]

    IN_WINDOW_VSIZE, OUT_WINDOW_VSIZE = get_window_sizes(
        AIE_VARIANT, TT_DATA, TP_POINT_SIZE, TP_NUM_FRAMES, TP_CASC_LEN, TP_SSR
    )

    in_ports = get_port_info(
        "in", "in", TT_DATA, IN_WINDOW_VSIZE, TP_CASC_LEN * TP_SSR, 0
    )
    out_ports = get_port_info("out", "out", TT_DATA, OUT_WINDOW_VSIZE, TP_SSR, 0)
    return in_ports + out_ports

    ######### Parameter Range Generator ############


def info_params(args):
    """Standard function creating a static dictionary of information
    for upper software to correctly update the IP GUI.
    """
    info_TT_DATA = [update_TT_DATA(args)]
    info_TT_TWIDDLE = [update_TT_TWIDDLE(args)]
    info_TP_CASC_LEN = [update_TP_CASC_LEN(args)]
    info_TP_SSR = [update_TP_SSR(args)]
    info_TP_POINT_SIZE = [update_TP_POINT_SIZE(args)]
    info_TP_NUM_FRAMES = [update_TP_NUM_FRAMES(args)]
    info_TP_SHIFT = [update_TP_SHIFT(args)]
    info_TP_RND = [update_TP_RND(args)]
    info_TP_SAT = [update_TP_SAT(args)]
    info_TP_API = [update_TP_API(args)]
    info_TP_FFT_NIFFT = [update_TP_FFT_NIFFT(args)]

    return (
        info_TT_DATA
        + info_TT_TWIDDLE
        + info_TP_CASC_LEN
        + info_TP_SSR
        + info_TP_POINT_SIZE
        + info_TP_NUM_FRAMES
        + info_TP_SHIFT
        + info_TP_RND
        + info_TP_SAT
        + info_TP_API
        + info_TP_FFT_NIFFT
    )


def generate_graph(graphname, args):

    if graphname == "":
        graphname = "default_graphname"
    TT_DATA = args["TT_DATA"]
    TT_TWIDDLE = args["TT_TWIDDLE"]
    TP_POINT_SIZE = args["TP_POINT_SIZE"]
    TP_FFT_NIFFT = args["TP_FFT_NIFFT"]
    TP_SHIFT = args["TP_SHIFT"]
    TP_CASC_LEN = args["TP_CASC_LEN"]
    TP_NUM_FRAMES = args["TP_NUM_FRAMES"]
    TP_RND = args["TP_RND"]
    TP_SAT = args["TP_SAT"]
    TP_SSR = args["TP_SSR"]

    code = f"""
class {graphname} : public adf::graph {{
public:
  // ports
  //template <typename dir>

  std::array<adf::port<input>, {TP_SSR} * {TP_CASC_LEN}> in;
  std::array<adf::port<output>, {TP_SSR}> out;

  xf::dsp::aie::fft::dft::dft_graph<
    {TT_DATA}, // TT_DATA
    {TT_TWIDDLE}, // TT_TWIDDLE
    {TP_POINT_SIZE}, // TP_POINT_SIZE
    {TP_FFT_NIFFT}, // TP_FFT_NIFFT
    {TP_SHIFT}, // TP_SHIFT
    {TP_CASC_LEN}, // TP_CASC_LEN
    {TP_NUM_FRAMES}, //TP_NUM_FRAMES
    {TP_RND}, //TP_RND
    {TP_SAT}, //TP_SAT
    {TP_SSR} //TP_SSR


  > dft_graph;

  {graphname}() : dft_graph() {{
    for (int ssrIdx = 0; ssrIdx < {TP_SSR}; ssrIdx++) {{
      for (int cascIdx = 0; cascIdx < {TP_CASC_LEN}; cascIdx++) {{
        adf::connect<> net_in(in[cascIdx + ssrIdx * {TP_CASC_LEN}], dft_graph.in[cascIdx + ssrIdx * {TP_CASC_LEN}]);
      }}
      adf::connect<> net_out(dft_graph.out[ssrIdx], out[ssrIdx]);
    }}
  }}
}};
"""
    out = {}
    out["graph"] = code
    out["port_info"] = info_ports(args)
    out["headerfile"] = "dft_graph.hpp"
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
    out["headerfile"] = "dft_graph.hpp"
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
