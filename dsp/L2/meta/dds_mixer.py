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

# dds_mixer.hpp:74:    static_assert(TP_MIXER_MODE <= 2, "ERROR: DDS Mixer Mode must be 0, 1 or 2. ");
# dds_mixer.hpp:75:    static_assert(fnEnumType<TT_DATA>() != enumUnknownType,
# dds_mixer.hpp:77:    static_assert((TP_INPUT_WINDOW_VSIZE % m_kNumLanes) == 0,
# dds_mixer.hpp:79:    static_assert(fnEnumType<TT_DATA>() != enumCint32 || TP_MIXER_MODE != MIXER_MODE_0,
# dds_mixer.hpp:129:    static_assert(TP_MIXER_MODE <= 2, "ERROR: DDS Mixer Mode must be 0, 1 or 2. ");
# dds_mixer.hpp:130:    static_assert(fnEnumType<TT_DATA>() != enumUnknownType,
# dds_mixer.hpp:132:    static_assert((TP_INPUT_WINDOW_VSIZE % m_kNumLanes) == 0,
# graph:78:    static_assert(TP_SSR > 0, "ERROR: Invalid SSR value, must be a value greater than 0.\n");

TP_SSR_min = 1
TP_SSR_max = 32
TP_INPUT_WINDOW_VSIZE_min = 8

# PING_PONG_BUFFER_AIE1=16384
uint32_min = 0
uint32_max = 2**32 - 1


#######################################################
########### AIE_VARIANT Updater and Validator #########
#######################################################
def update_AIE_VARIANT(args):
    return fn_update_AIE_VAR()


def fn_update_AIE_VAR():
    legal_set_AIE_VAR = [1]

    param_dict = {}
    param_dict.update({"name": "AIE_VARIANT"})
    param_dict.update({"enum": legal_set_AIE_VAR})

    return param_dict


def validate_AIE_VARIANT(args):
    AIE_VARIANT = args["AIE_VARIANT"]
    return fn_validate_aieVar(AIE_VARIANT)


def fn_validate_aieVar(AIE_VARIANT):
    if AIE_VARIANT == AIE:
        return isValid
    return isError(
        f"Please use the dds_mixer_lut library element for this device. This device does not support the dds_mixer element."
    )


def fn_validate_aieVarVMC(AIE_VARIANT):
    if AIE_VARIANT == AIE:
        return isValid
    return isError(
        f"This library block is not supported on {com.k_aie_variant[AIE_VARIANT]} device. Please use the DDS LUT/DDS LUT Stream/Mixer LUT/Mixer LUT Stream block instead."
    )


#######################################################
########### TT_DATA Updater and Validator #############
#######################################################
def update_TT_DATA(args):
    return fn_update_TT_DATA()


def fn_update_TT_DATA():
    legal_set_TT_DATA = ["cint32", "cint16", "cfloat"]

    param_dict = {"name": "TT_DATA", "enum": legal_set_TT_DATA}
    return param_dict


def validate_TT_DATA(args):
    TT_DATA = args["TT_DATA"]
    return fn_validate_tt_data(TT_DATA)


def fn_validate_tt_data(TT_DATA):
    param_dict = fn_update_TT_DATA()
    legal_set_TT_DATA = param_dict["enum"]
    return validate_legal_set(legal_set_TT_DATA, "TT_DATA", TT_DATA)


#######################################################
########### TP_MIXER_MODE Updater and Validator #######
#######################################################
def update_TP_MIXER_MODE(args):
    TT_DATA = args["TT_DATA"]
    return fn_update_TP_MIXER_MODE(TT_DATA)


def fn_update_TP_MIXER_MODE(TT_DATA):
    if TT_DATA == "cint32":
        legal_set_TP_MIXER_MODE = [1, 2]
    else:
        legal_set_TP_MIXER_MODE = [0, 1, 2]

    param_dict = {"name": "TP_MIXER_MODE", "enum": legal_set_TP_MIXER_MODE}
    return param_dict


def validate_TP_MIXER_MODE(args):
    TT_DATA = args["TT_DATA"]
    TP_MIXER_MODE = args["TP_MIXER_MODE"]
    return fn_validate_TP_MIXER_MODE(TT_DATA, TP_MIXER_MODE)


def fn_validate_TP_MIXER_MODE(TT_DATA, TP_MIXER_MODE):
    param_dict = fn_update_TP_MIXER_MODE(TT_DATA)
    legal_set_TP_MIXER_MODE = param_dict["enum"]
    return validate_legal_set(legal_set_TP_MIXER_MODE, "TP_MIXER_MODE", TP_MIXER_MODE)


#######################################################
########### TP_SSR Updater and Validator ##############
#######################################################
def update_TP_SSR(args):
    return fn_update_TP_SSR()


def fn_update_TP_SSR():
    param_dict = {"name": "TP_SSR", "minimum": TP_SSR_min, "maximum": TP_SSR_max}
    return param_dict


def validate_TP_SSR(args):
    TP_SSR = args["TP_SSR"]
    return fn_validate_ssr(TP_SSR)


def fn_validate_ssr(TP_SSR):
    range_TP_SSR = [TP_SSR_min, TP_SSR_max]
    return validate_range(range_TP_SSR, "TP_SSR", TP_SSR)


#######################################################
############ TP_API Updater and Validator #############
#######################################################
def update_TP_API(args):
    return fn_update_TP_API()


def fn_update_TP_API():
    legal_set_TP_API = [0, 1]
    param_dict = {"name": "TP_API", "enum": legal_set_TP_API}
    return param_dict


def validate_TP_API(args):
    TP_API = args["TP_API"]
    legal_set_TP_API = [0, 1]
    return validate_legal_set(legal_set_TP_API, "TP_API", TP_API)


#######################################################
#### TP_INPUT_WINDOW_VSIZE Updater and Validator ######
#######################################################


def update_TP_INPUT_WINDOW_VSIZE(args):
    TT_DATA = args["TT_DATA"]
    TP_SSR = args["TP_SSR"]
    TP_API = args["TP_API"]

    if ("TP_INPUT_WINDOW_VSIZE" in args) and args["TP_INPUT_WINDOW_VSIZE"]:
        TP_INPUT_WINDOW_VSIZE = args["TP_INPUT_WINDOW_VSIZE"]
    else:
        TP_INPUT_WINDOW_VSIZE = 0
    return fn_update_TP_INPUT_WINDOW_VSIZE(
        TT_DATA, TP_SSR, TP_API, TP_INPUT_WINDOW_VSIZE
    )


def fn_update_TP_INPUT_WINDOW_VSIZE(TT_DATA, TP_SSR, TP_API, TP_INPUT_WINDOW_VSIZE):
    lanes = fn_get_dds_lanes(TT_DATA)

    if TP_API == API_BUFFER:
        TP_INPUT_WINDOW_VSIZE_max = int(
            (k_data_memory_bytes[1] * TP_SSR) / fn_size_by_byte(TT_DATA)
        )
    else:
        TP_INPUT_WINDOW_VSIZE_max = com.TP_INPUT_WINDOW_VSIZE_max_streams

    TP_INPUT_WINDOW_VSIZE_min_int = max(
        int(CEIL(TP_INPUT_WINDOW_VSIZE_min, (lanes * TP_SSR))),
        TP_INPUT_WINDOW_VSIZE_min,
    )
    param_dict = {
        "name": "TP_WINDOW_VSIZE",
        "minimum": TP_INPUT_WINDOW_VSIZE_min_int,
        "maximum": TP_INPUT_WINDOW_VSIZE_max,
        "maximum_pingpong_buf": int(TP_INPUT_WINDOW_VSIZE_max / 2),
    }

    if TP_INPUT_WINDOW_VSIZE != 0:

        if TP_INPUT_WINDOW_VSIZE % (lanes * TP_SSR) != 0:
            TP_INPUT_WINDOW_VSIZE_act = int(
                round(TP_INPUT_WINDOW_VSIZE / (lanes * TP_SSR)) * (lanes * TP_SSR)
            )

            if TP_INPUT_WINDOW_VSIZE_act == 0:
                TP_INPUT_WINDOW_VSIZE_act = int(
                    CEIL(TP_INPUT_WINDOW_VSIZE, (lanes * TP_SSR))
                )

            if param_dict["maximum"] < TP_INPUT_WINDOW_VSIZE_act:
                TP_INPUT_WINDOW_VSIZE_act = int(
                    FLOOR(param_dict["maximum"], (lanes * TP_SSR))
                )
            param_dict.update({"actual": TP_INPUT_WINDOW_VSIZE_act})

    return param_dict


def validate_TP_INPUT_WINDOW_VSIZE(args):
    TT_DATA = args["TT_DATA"]
    TP_SSR = args["TP_SSR"]
    TP_API = args["TP_API"]
    TP_INPUT_WINDOW_VSIZE = args["TP_INPUT_WINDOW_VSIZE"]
    return fn_validate_TP_INPUT_WINDOW_VSIZE(
        TT_DATA, TP_SSR, TP_API, TP_INPUT_WINDOW_VSIZE
    )


def fn_validate_TP_INPUT_WINDOW_VSIZE(TT_DATA, TP_SSR, TP_API, TP_INPUT_WINDOW_VSIZE):
    param_dict = fn_update_TP_INPUT_WINDOW_VSIZE(
        TT_DATA, TP_SSR, TP_API, TP_INPUT_WINDOW_VSIZE
    )
    lanes = fn_get_dds_lanes(TT_DATA)

    if TP_INPUT_WINDOW_VSIZE % (lanes * TP_SSR) != 0:
        return isError(
            f"Window size ({TP_INPUT_WINDOW_VSIZE}) must be a multiple of ({lanes*TP_SSR})"
        )
    else:
        legal_range_TP_INPUT_WINDOW_VSIZE = [
            param_dict["minimum"],
            param_dict["maximum"],
        ]
        return validate_range(
            legal_range_TP_INPUT_WINDOW_VSIZE,
            "TP_INPUT_WINDOW_VSIZE",
            TP_INPUT_WINDOW_VSIZE,
        )


def fn_get_dds_lanes(TT_DATA):
    type_lane_dict = {"cint16": 8, "cint32": 4, "cfloat": 4}
    return type_lane_dict[TT_DATA]


#######################################################
###### TP_USE_PHASE_RELOAD Updater and Validator ######
#######################################################
def update_TP_USE_PHASE_RELOAD(args):
    TP_SSR = args["TP_SSR"]
    return fn_update_TP_USE_PHASE_RELOAD(TP_SSR)


def fn_update_TP_USE_PHASE_RELOAD(TP_SSR):
    if TP_SSR == 1:
        legal_set_TP_USE_PHASE_RELOAD = [0, 1]
    else:
        legal_set_TP_USE_PHASE_RELOAD = [0]
    param_dict = {"name": "TP_USE_PHASE_RELOAD", "enum": legal_set_TP_USE_PHASE_RELOAD}
    return param_dict


def validate_TP_USE_PHASE_RELOAD(args):
    TP_SSR = args["TP_SSR"]
    TP_USE_PHASE_RELOAD = args["TP_USE_PHASE_RELOAD"]
    return fn_validate_TP_USE_PHASE_RELOAD(TP_SSR, TP_USE_PHASE_RELOAD)


def fn_validate_TP_USE_PHASE_RELOAD(TP_SSR, TP_USE_PHASE_RELOAD):
    param_dict = fn_update_TP_USE_PHASE_RELOAD(TP_SSR)
    legal_set_TP_USE_PHASE_RELOAD = param_dict["enum"]
    return validate_legal_set(
        legal_set_TP_USE_PHASE_RELOAD, "TP_USE_PHASE_RELOAD", TP_USE_PHASE_RELOAD
    )


#######################################################
###### TP_PHASE_RELOAD_API Updater and Validator ######
#######################################################
def update_TP_PHASE_RELOAD_API(args):
    TP_USE_PHASE_RELOAD = args["TP_USE_PHASE_RELOAD"]
    return fn_update_TP_PHASE_RELOAD_API(TP_USE_PHASE_RELOAD)


def fn_update_TP_PHASE_RELOAD_API(TP_USE_PHASE_RELOAD):
    legal_set_TP_PHASE_RELOAD_API = [0, 1]
    if TP_USE_PHASE_RELOAD == 0:
        legal_set_TP_PHASE_RELOAD_API = [0]

    param_dict = {"name": "TP_PHASE_RELOAD_API", "enum": legal_set_TP_PHASE_RELOAD_API}
    return param_dict


def validate_TP_PHASE_RELOAD_API(args):
    TP_USE_PHASE_RELOAD = args["TP_USE_PHASE_RELOAD"]
    TP_PHASE_RELOAD_API = args["TP_PHASE_RELOAD_API"]
    return fn_validate_TP_PHASE_RELOAD_API(TP_USE_PHASE_RELOAD, TP_PHASE_RELOAD_API)


def fn_validate_TP_PHASE_RELOAD_API(TP_USE_PHASE_RELOAD, TP_PHASE_RELOAD_API):
    param_dict = fn_update_TP_PHASE_RELOAD_API(TP_USE_PHASE_RELOAD)
    legal_set_TP_PHASE_RELOAD_API = param_dict["enum"]
    return validate_legal_set(
        legal_set_TP_PHASE_RELOAD_API, "TP_PHASE_RELOAD_API", TP_PHASE_RELOAD_API
    )


#######################################################
############ phaseInc Updater and Validator ###########
#######################################################
def update_phaseInc(args):
    return fn_update_phaseInc()


def fn_update_phaseInc():
    param_dict = {"name": "phaseInc", "minimum": uint32_min, "maximum": uint32_max}
    return param_dict


def validate_phaseInc(args):
    phaseInc = args["phaseInc"]
    return fn_validate_phaseInc(phaseInc)


def fn_validate_phaseInc(phaseInc):
    range_phaseInc = [uint32_min, uint32_max]
    return validate_range(range_phaseInc, "phaseInc", phaseInc)


#######################################################
####### initialPhaseOffset Updater and Validator ######
#######################################################
def update_initialPhaseOffset(args):
    return fn_update_initialPhaseOffset()


def fn_update_initialPhaseOffset():
    param_dict = {
        "name": "initialPhaseOffset",
        "minimum": uint32_min,
        "maximum": uint32_max,
    }
    return param_dict


def validate_initialPhaseOffset(args):
    initialPhaseOffset = args["initialPhaseOffset"]
    return fn_validate_initialPhaseOffset(initialPhaseOffset)


def fn_validate_initialPhaseOffset(initialPhaseOffset):
    range_initialPhaseOffset = [uint32_min, uint32_max]
    return validate_range(
        range_initialPhaseOffset, "initialPhaseOffset", initialPhaseOffset
    )


#######################################################
############## TP_RND Updater and Validator ###########
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
    param_dict = fn_update_tp_rnd(AIE_VARIANT)
    legal_set_TP_RND = param_dict["enum"]
    return validate_legal_set(legal_set_TP_RND, "TP_RND", TP_RND)


#######################################################
############## TP_SAT Updater and Validator ###########
#######################################################
def update_TP_SAT(args):
    TP_MIXER_MODE = args["TP_MIXER_MODE"]
    return fn_update_tp_sat(TP_MIXER_MODE)


def fn_update_tp_sat(TP_MIXER_MODE):
    legal_set = [0, 1, 3]
    if TP_MIXER_MODE == 2:
        legal_set = [1, 3]

    param_dict = {}
    param_dict.update({"name": "TP_SAT"})
    param_dict.update({"enum": legal_set})
    return param_dict


def validate_TP_SAT(args):
    TP_SAT = args["TP_SAT"]
    TP_MIXER_MODE = args["TP_MIXER_MODE"]
    param_dict = fn_update_tp_sat(TP_MIXER_MODE)
    legal_set_TP_SAT = param_dict["enum"]
    return validate_legal_set(legal_set_TP_SAT, "TP_SAT", TP_SAT)


#######################################################
############## TP_USE_PHASE_INC_RELOAD Updater and Validator ###########
#######################################################
def update_TP_USE_PHASE_INC_RELOAD(args):
    TP_SSR = args["TP_SSR"]
    return fn_update_tp_use_phase_inc_reload(TP_SSR)


def fn_update_tp_use_phase_inc_reload(TP_SSR):
    if TP_SSR == 1:
        legal_set = [0, 1]
    else:
        legal_set = [0]
    param_dict = {}
    param_dict.update({"name": "TP_USE_PHASE_INC_RELOAD"})
    param_dict.update({"enum": legal_set})
    return param_dict


def validate_TP_USE_PHASE_INC_RELOAD(args):
    TP_SSR = args["TP_SSR"]
    TP_USE_PHASE_INC_RELOAD = args["TP_USE_PHASE_INC_RELOAD"]
    param_dict = fn_update_tp_use_phase_inc_reload(TP_SSR)
    legal_set_TP_USE_PHASE_INC_RELOAD = param_dict["enum"]
    return validate_legal_set(
        legal_set_TP_USE_PHASE_INC_RELOAD,
        "TP_USE_PHASE_INC_RELOAD",
        TP_USE_PHASE_INC_RELOAD,
    )

    ######### Graph Generator ############


# Used by higher layer software to figure out how to connect blocks together.
def info_ports(args):
    TT_DATA = args["TT_DATA"]
    TP_INPUT_WINDOW_VSIZE = args["TP_INPUT_WINDOW_VSIZE"]
    TP_SSR = args["TP_SSR"]
    TP_API = args["TP_API"]
    TP_MIXER_MODE = args["TP_MIXER_MODE"]
    TP_USE_PHASE_RELOAD = args["TP_USE_PHASE_RELOAD"]
    TP_PHASE_RELOAD_API = args["TP_PHASE_RELOAD_API"]
    TP_USE_PHASE_INC_RELOAD = args["TP_USE_PHASE_INC_RELOAD"]

    in1_ports = (
        com.get_port_info(
            "in1", "in", TT_DATA, (TP_INPUT_WINDOW_VSIZE / TP_SSR), TP_SSR, 0, TP_API
        )
        if (TP_MIXER_MODE in [1, 2])
        else []
    )
    in2_ports = (
        com.get_port_info(
            "in2", "in", TT_DATA, (TP_INPUT_WINDOW_VSIZE / TP_SSR), TP_SSR, 0, TP_API
        )
        if (TP_MIXER_MODE == 2)
        else []
    )
    if TP_USE_PHASE_RELOAD == 1:
        if TP_PHASE_RELOAD_API == 0:
            in3_ports = com.get_parameter_port_info(
                "PhaseRTP", "in", "uint32", TP_SSR, 1, "async"
            )
        else:
            in3_ports = com.get_parameter_port_info(
                "PhaseRTP", "in", "uint32", TP_SSR, 1
            )
    else:
        in3_ports = []
    if TP_USE_PHASE_INC_RELOAD == 1:
        in4_ports = com.get_parameter_port_info(
            "PhaseIncRTP", "in", "uint32", TP_SSR, 1, "async"
        )
    else:
        in4_ports = []
    out_ports = com.get_port_info(
        "out", "out", TT_DATA, (TP_INPUT_WINDOW_VSIZE / TP_SSR), TP_SSR, 0, TP_API
    )

    return in1_ports + in2_ports + in3_ports + in4_ports + out_ports  # concat strings


def gen_ports_code(args):
    TP_SSR = args["TP_SSR"]
    TP_MIXER_MODE = args["TP_MIXER_MODE"]
    TP_USE_PHASE_RELOAD = args["TP_USE_PHASE_RELOAD"]
    TP_USE_PHASE_INC_RELOAD = args["TP_USE_PHASE_INC_RELOAD"]
    in1_ports = (
        (f"  std::array<adf::port<input>, {TP_SSR}> in1;\n")
        if (TP_MIXER_MODE in [1, 2])
        else ""
    )
    in2_ports = (
        (f"  std::array<adf::port<input>, {TP_SSR}> in2;\n")
        if (TP_MIXER_MODE == 2)
        else ""
    )
    in3_ports = (
        (f"  std::array<adf::port<input>, {TP_SSR}> PhaseRTP;\n")
        if (TP_USE_PHASE_RELOAD == 1)
        else ""
    )
    in4_ports = (
        (f"  std::array<adf::port<input>, {TP_SSR}> PhaseIncRTP;\n")
        if (TP_USE_PHASE_INC_RELOAD == 1)
        else ""
    )
    out_ports = f"  std::array<adf::port<output>, {TP_SSR}> out;\n"

    return in1_ports + in2_ports + in3_ports + in4_ports + out_ports  # concat strings


def gen_ports_connections(args):
    TP_SSR = args["TP_SSR"]
    TP_USE_PHASE_RELOAD = args["TP_USE_PHASE_RELOAD"]
    TP_MIXER_MODE = args["TP_MIXER_MODE"]
    TP_USE_PHASE_INC_RELOAD = args["TP_USE_PHASE_INC_RELOAD"]
    in1_ports = (
        (f"      adf::connect<>(in1[ssrIdx],mixer_graph.in1[ssrIdx]);\n")
        if (TP_MIXER_MODE in [1, 2])
        else ""
    )
    in2_ports = (
        (f"      adf::connect<>(in2[ssrIdx],mixer_graph.in2[ssrIdx]);\n")
        if (TP_MIXER_MODE == 2)
        else ""
    )
    in3_ports = (
        (f"      adf::connect<>(PhaseRTP[ssrIdx],mixer_graph.PhaseRTP[ssrIdx]);\n")
        if (TP_USE_PHASE_RELOAD == 1)
        else ""
    )
    in4_ports = (
        (
            f"      adf::connect<>(PhaseIncRTP[ssrIdx],mixer_graph.PhaseIncRTP[ssrIdx]);\n"
        )
        if (TP_USE_PHASE_INC_RELOAD == 1)
        else ""
    )
    out_ports = f"      adf::connect<>(mixer_graph.out[ssrIdx], out[ssrIdx]);\n"

    return in1_ports + in2_ports + in3_ports + in4_ports + out_ports  # concat strings


def generate_graph(graphname, args):

    out = {}
    out["port_info"] = info_ports(args)
    ports_code = gen_ports_code(args)
    if graphname == "":
        graphname = "default_graphname"
    TT_DATA = args["TT_DATA"]
    TP_INPUT_WINDOW_VSIZE = args["TP_INPUT_WINDOW_VSIZE"]
    TP_MIXER_MODE = args["TP_MIXER_MODE"]
    TP_API = args["TP_API"]
    TP_SSR = args["TP_SSR"]
    TP_RND = args["TP_RND"]
    TP_SAT = args["TP_SAT"]
    TP_USE_PHASE_RELOAD = args["TP_USE_PHASE_RELOAD"]
    TP_PHASE_RELOAD_API = args["TP_PHASE_RELOAD_API"]
    TP_USE_PHASE_INC_RELOAD = args["TP_USE_PHASE_INC_RELOAD"]
    code = f"""
class {graphname} : public adf::graph {{
public:
  // ports
{ports_code}
  xf::dsp::aie::mixer::dds_mixer::dds_mixer_graph<
    {TT_DATA}, // TT_DATA
    {TP_INPUT_WINDOW_VSIZE}, // TP_INPUT_WINDOW_VSIZE
    {TP_MIXER_MODE}, // TP_MIXER_MODE
    {TP_API}, // TP_API
    {TP_SSR}, // TP_SSR
    {TP_RND}, //TP_RND
    {TP_SAT}, //TP_SAT
    {TP_USE_PHASE_RELOAD},    // TP_USE_PHASE_RELOAD
    {TP_PHASE_RELOAD_API},    // TP_PHASE_RELOAD_API
    {TP_USE_PHASE_INC_RELOAD} // TP_USE_PHASE_INC_RELOAD
  > mixer_graph;
  {graphname}() : mixer_graph({args["phaseInc"]}, {args["initialPhaseOffset"]}) {{
    //kernels
    //runtime_ratio
    //connections in loop
    for (unsigned ssrIdx = 0; ssrIdx < {TP_SSR}; ssrIdx++){{
{gen_ports_connections(args)}
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
