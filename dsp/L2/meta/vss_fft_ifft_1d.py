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
from aie_common import *
import sys

TP_POINT_SIZE_min_aie = 16
TP_POINT_SIZE_min_aie_ml = 32
TP_POINT_SIZE_min_aie_mlv2 = 64
TP_POINT_SIZE_max = 65536
TP_WINDOW_SIZE_max = 65536
TP_WINDOW_SIZE_max_cpp = 2**31
TP_WINDOW_VSIZE_min = 8
TP_CASC_LEN_min = 1
TP_CASC_LEN_max = 11
SHIFT_min = 0
SHIFT_max = 59
TP_RND_min = 0
TP_RND_max = 7
TP_TWIDDLE_MODE_max = 1
TP_TWIDDLE_MODE_min = 0
# TP_FFT_NIFFT_min=0
# TP_FFT_NIFFT_max=1
# API_IO_min=0
# API_IO_max=1
# AIE_VARIANT_min=2
# AIE_VARIANT_max=1


#######################################################
########### PART Updater and Validator #########
#######################################################
def update_PART(args):
    return fn_update_PART()


def fn_update_PART():
    legal_set_PART = ["xcvc1902-vsva2197-2MP-e-S", "xcve2802-vsvh1760-2MP-e-S", "xc2ve3858-ssva2112-2LP-e-S"]

    param_dict = {}
    param_dict.update({"name": "PART"})
    param_dict.update({"enum": legal_set_PART})

    return param_dict


def validate_PART(args):
    PART = args["PART"]
    return fn_validate_PART(PART)


def fn_validate_PART(PART):
    param_dict = fn_update_PART()
    legal_set_PART = param_dict["enum"]
    return validate_legal_set(legal_set_PART, "PART", PART)


#######################################################
########### FREQHZ Updater and Validator #############
#######################################################
def update_FREQHZ(args):
    FREQHZ = args["FREQHZ"]
    return fn_update_FREQHZ(FREQHZ)


def fn_update_FREQHZ(FREQHZ):
    param_dict = {}
    param_dict.update({"name": "FREQHZ"})
    param_dict.update({"minimum": 1000000})
    param_dict.update({"maximum": 312500000})

    return param_dict


def validate_FREQHZ(args):
    FREQHZ = args["FREQHZ"]
    return fn_validate_FREQHZ(FREQHZ)


def fn_validate_FREQHZ(FREQHZ):
    param_dict = fn_update_FREQHZ(FREQHZ)
    range_FREQHZ = [param_dict["minimum"], param_dict["maximum"]]
    return validate_range(range_FREQHZ, "FREQHZ", int(FREQHZ))


#######################################################
########### TT_OUT_DATA Updater and Validator #########
#######################################################


def update_DATA_TYPE(args):
    DATA_TYPE = args["DATA_TYPE"]
    PART = args["PART"]
    return fn_update_DATA_TYPE(DATA_TYPE, PART)


def fn_update_DATA_TYPE(DATA_TYPE, PART):
    legal_set_DATA_TYPE = ["cfloat", "cint32"]
    if "xcvc1902-vsva2197-2MP-e-S" in PART:
        legal_set_DATA_TYPE = ["cfloat", "cint32"]
    elif "xcve2802-vsvh1760-2MP-e-S" in PART:
        legal_set_DATA_TYPE = ["cfloat", "cint32"]
    elif "xc2ve3858-ssva2112-2LP-e-S" in PART:
        legal_set_DATA_TYPE = ["cfloat", "cint32"]


    param_dict = {}
    param_dict.update({"name": "DATA_TYPE"})
    param_dict.update({"enum": legal_set_DATA_TYPE})

    return param_dict


def validate_DATA_TYPE(args):
    DATA_TYPE = args["DATA_TYPE"]
    PART = args["PART"]
    return fn_validate_DATA_TYPE(DATA_TYPE, PART)


def fn_validate_DATA_TYPE(DATA_TYPE, PART):
    param_dict = fn_update_DATA_TYPE(DATA_TYPE, PART)
    legal_set_DATA_TYPE = param_dict["enum"]
    return validate_legal_set(legal_set_DATA_TYPE, "DATA_TYPE", DATA_TYPE)


#######################################################
########### TWIDDLE_TYPE Updater and Validator ##########
#######################################################
def update_TWIDDLE_TYPE(args):
    PART = args["PART"]
    TWIDDLE_TYPE = args["TWIDDLE_TYPE"]
    DATA_TYPE = args["DATA_TYPE"]
    return fn_update_TWIDDLE_TYPE(PART, TWIDDLE_TYPE, DATA_TYPE)


def fn_update_TWIDDLE_TYPE(PART, TWIDDLE_TYPE, DATA_TYPE):
    legal_set_TWIDDLE_TYPE = ["cint16", "cint32"]
    if DATA_TYPE in ["cint32"]:
        if PART in ["xcvc1902-vsva2197-2MP-e-S"]:
            legal_set_TWIDDLE_TYPE = ["cint16", "cint32"]
        else:
            legal_set_TWIDDLE_TYPE = ["cint16"]
    elif DATA_TYPE in ["cfloat"]:
        legal_set_TWIDDLE_TYPE = ["cfloat"]

    param_dict = {}
    param_dict.update({"name": "TWIDDLE_TYPE"})
    param_dict.update({"enum": legal_set_TWIDDLE_TYPE})

    return param_dict


def validate_TWIDDLE_TYPE(args):
    PART = args["PART"]
    DATA_TYPE = args["DATA_TYPE"]
    TWIDDLE_TYPE = args["TWIDDLE_TYPE"]
    return fn_validate_twiddle_type(PART, DATA_TYPE, TWIDDLE_TYPE)


def fn_validate_twiddle_type(PART, DATA_TYPE, TWIDDLE_TYPE):
    param_dict = fn_update_TWIDDLE_TYPE(PART, DATA_TYPE, TWIDDLE_TYPE)
    legal_set_TWIDDLE_TYPE = param_dict["enum"]
    return validate_legal_set(legal_set_TWIDDLE_TYPE, "TWIDDLE_TYPE", TWIDDLE_TYPE)


#######################################################
########### TP_DYN_PT_SIZE Updater and Validator ######
#######################################################
def update_POINT_SIZE(args):
    return fn_update_POINT_SIZE()


def fn_update_POINT_SIZE():
    param_dict = {}
    param_dict.update({"name": "POINT_SIZE"})
    param_dict.update({"minimum": 32})
    param_dict.update({"maximum": 65536})
    return param_dict


def validate_POINT_SIZE(args):
    POINT_SIZE = args["POINT_SIZE"]
    return fn_validate_POINT_SIZE(POINT_SIZE)


def fn_validate_POINT_SIZE(POINT_SIZE):
    param_dict = fn_update_POINT_SIZE()
    legal_range_POINT_SIZE = [param_dict["minimum"], param_dict["maximum"]]
    return validate_range(legal_range_POINT_SIZE, "POINT_SIZE", int(POINT_SIZE))


#######################################################
########### FFT_NIFFT Updater and Validator ########
#######################################################
def update_FFT_NIFFT(args):
    return fn_update_FFT_NIFFT()


def fn_update_FFT_NIFFT():
    legal_set_FFT_NIFFT = [0, 1]

    param_dict = {}
    param_dict.update({"name": "FFT_NIFFT"})
    param_dict.update({"enum": legal_set_FFT_NIFFT})

    return param_dict


def validate_FFT_NIFFT(args):
    FFT_NIFFT = args["FFT_NIFFT"]
    return fn_validate_FFT_NIFFT(FFT_NIFFT)


def fn_validate_FFT_NIFFT(FFT_NIFFT):
    param_dict = fn_update_FFT_NIFFT()
    legal_set_FFT_NIFFT = param_dict["enum"]
    return validate_legal_set(legal_set_FFT_NIFFT, "FFT_NIFFT", int(FFT_NIFFT))


#######################################################
########### SHIFT Updater and Validator ############
#######################################################
def update_SHIFT(args):
    DATA_TYPE = args["DATA_TYPE"]
    return fn_update_shift_val(DATA_TYPE)


def fn_update_shift_val(DATA_TYPE):
    param_dict = {}
    param_dict.update({"name": "SHIFT"})
    param_dict.update({"minimum": SHIFT_min})
    if DATA_TYPE == "cfloat":
        param_dict.update({"maximum": SHIFT_min})
    else:
        param_dict.update({"maximum": SHIFT_max})

    return param_dict


def validate_SHIFT(args):
    SHIFT = args["SHIFT"]
    DATA_TYPE = args["DATA_TYPE"]
    return fn_validate_shift_val(DATA_TYPE, SHIFT)


def fn_validate_shift_val(DATA_TYPE, SHIFT):
    res = fn_validate_minmax_value("SHIFT", int(SHIFT), int(SHIFT_min), int(SHIFT_max))
    if res["is_valid"] == False:
        return res
    return fn_float_no_shift(DATA_TYPE, int(SHIFT))


#######################################################
########### API_IO Updater and Validator ##############
#######################################################
def update_API_IO(args):
    return fn_update_API_IO()


def fn_update_API_IO():
    legal_set_API_IO = [0]

    param_dict = {}
    param_dict.update({"name": "API_IO"})
    param_dict.update({"enum": legal_set_API_IO})

    return param_dict


def validate_API_IO(args):
    API_IO = args["API_IO"]
    return fn_validate_API_IO(API_IO)


def fn_validate_API_IO(API_IO):
    param_dict = fn_update_API_IO()
    legal_set_API_IO = param_dict["enum"]
    return validate_legal_set(legal_set_API_IO, "API_IO", int(API_IO))


######################################################
########## ROUND_MODE Updater and Validator ##############
######################################################
def update_RND(args):
    PART = args["PART"]
    return fn_update_RND(PART)


def fn_update_RND(PART):
    if PART in ["xcvc1902-vsva2197-2MP-e-S"]:
        remove_set = [k_rnd_mode_map_aie["rnd_ceil"], k_rnd_mode_map_aie["rnd_floor"]]
    else:
        remove_set = [
            k_rnd_mode_map_aie_ml["rnd_ceil"],
            k_rnd_mode_map_aie_ml["rnd_floor"],
            k_rnd_mode_map_aie_ml["rnd_sym_floor"],
            k_rnd_mode_map_aie_ml["rnd_sym_ceil"],
        ]
    legal_set_ROUND_MODE = fn_get_legalSet_roundMode(k_part2aievar_map[PART])

    legal_set_ROUND_MODE = remove_from_set(remove_set, legal_set_ROUND_MODE.copy())

    param_dict = {"name": "ROUND_MODE", "enum": legal_set_ROUND_MODE}
    return param_dict


def validate_RND(args):
    PART = args["PART"]
    ROUND_MODE = args["ROUND_MODE"]
    return fn_validate_roundMode(int(ROUND_MODE), k_part2aievar_map[PART])


######################################################
########## SAT_MODE Updater and Validator ##############
######################################################
def update_SAT(args):
    return fn_update_SAT()


def fn_update_SAT():
    legal_set = [0, 1, 3]

    param_dict = {}
    param_dict.update({"name": "SAT_MODE"})
    param_dict.update({"enum": legal_set})
    return param_dict


def validate_SAT(args):
    SAT_MODE = args["SAT_MODE"]
    param_dict = update_SAT(args)
    legal_set_SAT_MODE = param_dict["enum"]
    return validate_legal_set(legal_set_SAT_MODE, "SAT_MODE", int(SAT_MODE))


######################################################
########## TWIDDLE_MODE Updater and Validator #####
######################################################
# ---------------------------------------------------
# Twiddle mode is the amplitude of twiddles and applies to integer types only. It is ignored for float types
# Twiddle mode 0 means use max amplitude twiddles, but these saturate at 2^(N-1)-1 where N is the number of bits
# in the type, e.g. cint16 has 16 bits per component.
# Twiddle mode 1 means use 1/2 max magnitude twiddles, i.e. 2^(N-1). This avoids saturation, but loses 1 bit of
# precision and so noise overall will be higher.


def update_TWIDDLE_MODE(args):
    return fn_update_TWIDDLE_MODE()


def fn_update_TWIDDLE_MODE():
    legal_set = [0, 1]
    param_dict = {}
    param_dict.update({"name": "TWIDDLE_MODE"})
    param_dict.update({"enum": legal_set})
    return param_dict


def validate_TWIDDLE_MODE(args):
    TWIDDLE_MODE = args["TWIDDLE_MODE"]
    return fn_validate_TWIDDLE_MODE(TWIDDLE_MODE)


def fn_validate_TWIDDLE_MODE(TWIDDLE_MODE):
    param_dict = fn_update_TWIDDLE_MODE()
    legal_set_TWIDDLE_MODE = param_dict["enum"]
    return validate_legal_set(legal_set_TWIDDLE_MODE, "TWIDDLE_MODE", int(TWIDDLE_MODE))


#######################################################
########### SSR Updater and Validator ###
#######################################################
def fn_pt_size_d1(POINT_SIZE):
    POINT_SIZE = int(POINT_SIZE)
    if POINT_SIZE == 65536:
        return 256
    elif POINT_SIZE == 32768:
        return 256
    elif POINT_SIZE == 16384:
        return 128
    elif POINT_SIZE == 8192:
        return 128
    elif POINT_SIZE == 4096:
        return 64
    elif POINT_SIZE == 2048:
        return 64
    elif POINT_SIZE == 1024:
        return 32
    elif POINT_SIZE == 512:
        return 32
    elif POINT_SIZE == 256:
        return 16
    elif POINT_SIZE == 128:
        return 16
    elif POINT_SIZE == 64:
        return 8
    elif POINT_SIZE == 32:
        return 8
    elif POINT_SIZE == 16:
        return 4
    else:
        return 0


def legal_ssr_point_size(POINT_SIZE, SSR, PART, DATA_TYPE):
    point_size_d1 = fn_pt_size_d1(POINT_SIZE)
    point_size_d2 = POINT_SIZE / point_size_d1
    ceil_d1_ssr = CEIL(point_size_d1, SSR)
    ceil_d2_ssr = CEIL(point_size_d2, SSR)
    window_size_d1 = ceil_d2_ssr / SSR * point_size_d1 * fn_size_by_byte(DATA_TYPE)
    window_size_d2 = ceil_d1_ssr / SSR * point_size_d2 * fn_size_by_byte(DATA_TYPE)
    if fn_is_power_of_two(SSR):
        return True
    elif (
        window_size_d1 > k_data_memory_bytes[k_part2aievar_map[PART]] / 2
        or window_size_d2 > k_data_memory_bytes[k_part2aievar_map[PART]] / 2
    ):
        return False
    else:
        return True


def update_SSR(args):
    POINT_SIZE = args["POINT_SIZE"]
    PART = args["PART"]
    DATA_TYPE = args["DATA_TYPE"]
    FFT_NIFFT = args["FFT_NIFFT"]
    return fn_update_ssr(POINT_SIZE, PART, DATA_TYPE, FFT_NIFFT)


def fn_update_ssr(POINT_SIZE, PART, DATA_TYPE, FFT_NIFFT):
    range_SSR = [2, 64]
    param_dict = {}
    param_dict.update({"name": "SSR"})
    legal_set_TP_SSR = list(range(range_SSR[0], range_SSR[1] + 1))
    remove_list = []
    for cur_ssr in legal_set_TP_SSR:
        if (
            legal_ssr_point_size(POINT_SIZE, cur_ssr, PART, DATA_TYPE) == False
            or fn_update_VSS_MODE(DATA_TYPE, FFT_NIFFT, cur_ssr, POINT_SIZE)["enum"]
            == []
        ):  # Second condition is to make sure that the downstream parameter does not lead to an empty configuration if the SSR value is chosen.
            remove_list.append(cur_ssr)
    legal_set_TP_SSR_eliminated = remove_from_set(remove_list, legal_set_TP_SSR.copy())
    if legal_set_TP_SSR_eliminated == legal_set_TP_SSR:
        param_dict.update({"minimum": range_SSR[0]})
        param_dict.update({"maximum": range_SSR[1]})
    else:
        param_dict.update({"enum": legal_set_TP_SSR_eliminated})
    return param_dict


def validate_SSR(args):
    SSR = args["SSR"]
    POINT_SIZE = args["POINT_SIZE"]
    PART = args["PART"]
    DATA_TYPE = args["DATA_TYPE"]
    FFT_NIFFT = args["FFT_NIFFT"]
    return fn_validate_ssr(SSR, POINT_SIZE, PART, DATA_TYPE, FFT_NIFFT)


def fn_validate_ssr(SSR, POINT_SIZE, PART, DATA_TYPE, FFT_NIFFT):
    param_dict = fn_update_ssr(POINT_SIZE, PART, DATA_TYPE, FFT_NIFFT)
    if "enum" in param_dict:
        return validate_legal_set(param_dict["enum"], "SSR", int(SSR))
    else:
        range_SSR = [param_dict["minimum"], param_dict["maximum"]]
        return validate_range(range_SSR, "SSR", SSR)


######################################################
########## AIE_PLIO_WIDTH Updater and Validator #####
######################################################


def update_AIE_PLIO_WIDTH(args):
    return fn_update_AIE_PLIO_WIDTH()


def fn_update_AIE_PLIO_WIDTH():
    legal_set = [64, 128]
    param_dict = {}
    param_dict.update({"name": "AIE_PLIO_WIDTH"})
    param_dict.update({"enum": legal_set})
    return param_dict


def validate_AIE_PLIO_WIDTH(args):
    AIE_PLIO_WIDTH = args["AIE_PLIO_WIDTH"]
    return fn_validate_AIE_PLIO_WIDTH(AIE_PLIO_WIDTH)


def fn_validate_AIE_PLIO_WIDTH(AIE_PLIO_WIDTH):
    param_dict = fn_update_AIE_PLIO_WIDTH()
    legal_set_AIE_PLIO_WIDTH = param_dict["enum"]
    return validate_legal_set(
        legal_set_AIE_PLIO_WIDTH, "AIE_PLIO_WIDTH", int(AIE_PLIO_WIDTH)
    )


######################################################
########## VSS_MODE Updater and Validator #####
######################################################


def update_VSS_MODE(args):
    DATA_TYPE = args["DATA_TYPE"]
    FFT_NIFFT = args["FFT_NIFFT"]
    SSR = args["SSR"]
    POINT_SIZE = args["POINT_SIZE"]
    return fn_update_VSS_MODE(DATA_TYPE, FFT_NIFFT, SSR, POINT_SIZE)


def fn_update_VSS_MODE(DATA_TYPE, FFT_NIFFT, SSR, POINT_SIZE):
    legal_set = [1, 2]
    if (
        FFT_NIFFT == 0
        or DATA_TYPE in ["cint32"]
        or fn_is_power_of_two(SSR) != 0
        or (POINT_SIZE % SSR != 0)
    ):
        legal_set = [1]
        if SSR < 4:
            legal_set = []
    elif SSR == 2:
        legal_set = [2]
    param_dict = {}
    param_dict.update({"name": "VSS_MODE"})
    param_dict.update({"enum": legal_set})
    return param_dict


def validate_VSS_MODE(args):
    VSS_MODE = args["VSS_MODE"]
    DATA_TYPE = args["DATA_TYPE"]
    FFT_NIFFT = args["FFT_NIFFT"]
    SSR = args["SSR"]
    POINT_SIZE = args["POINT_SIZE"]
    return fn_validate_VSS_MODE(VSS_MODE, DATA_TYPE, FFT_NIFFT, SSR, POINT_SIZE)


def fn_validate_VSS_MODE(VSS_MODE, DATA_TYPE, FFT_NIFFT, SSR, POINT_SIZE):
    param_dict = fn_update_VSS_MODE(DATA_TYPE, FFT_NIFFT, SSR, POINT_SIZE)
    legal_set_VSS_MODE = param_dict["enum"]
    return validate_legal_set(legal_set_VSS_MODE, "VSS_MODE", int(VSS_MODE))


def info_ports():
    return 0


def generate_cfg(aie_graph_name, args):
    out = {}
    PART = args["PART"]
    FREQ = args["FREQHZ"]
    macro_body_str_init = f"""part={PART}
freqhz={FREQ}

[aie]
enable-partition=0:35:fft_aie
"""

    macro_body = []

    for key, value in args.items():
        if key != "PART":
            if key in [
                "DATA_TYPE",
                "TWIDDLE_TYPE",
                "POINT_SIZE",
                "FFT_NIFFT",
                "SHIFT",
                "API_IO",
                "ROUND_MODE",
                "SAT_MODE",
                "TWIDDLE_MODE",
                "SSR",
                "AIE_PLIO_WIDTH",
                "VSS_MODE",
            ]:
                macro_body.append(
                    f"""
{key}={value}"""
                )
    macro_body_str = "".join(macro_body)
    # Use formatted multi-line string to avoid a lot of \n and \t
    macro_body_str = f"""

[APP_PARAMS]
{macro_body_str}
"""
    out["cfg"] = macro_body_str_init + macro_body_str
    return out
