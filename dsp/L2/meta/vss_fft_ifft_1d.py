#
# Copyright (C) 2019-2022, Xilinx, Inc.
# Copyright (C) 2022-2026, Advanced Micro Devices, Inc.
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
from get_aievar_from_part import get_aie_arch_from_part_name
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
    legal_set_DATA_TYPE = ["cfloat", "cint32", "cint16"]
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
    legal_set_TWIDDLE_TYPE = ["cint16", "cint32", "cfloat"]
    if DATA_TYPE in ["cint32"]:
        if PART in ["xcvc1902-vsva2197-2MP-e-S"]:
            legal_set_TWIDDLE_TYPE = ["cint16", "cint32"]
        else:
            legal_set_TWIDDLE_TYPE = ["cint16"]
    elif DATA_TYPE in ["cint16"]:
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
    return fn_validate_twiddle_type(PART, TWIDDLE_TYPE, DATA_TYPE)


def fn_validate_twiddle_type(PART, TWIDDLE_TYPE, DATA_TYPE):
    param_dict = fn_update_TWIDDLE_TYPE(PART, TWIDDLE_TYPE, DATA_TYPE)
    legal_set_TWIDDLE_TYPE = param_dict["enum"]
    return validate_legal_set(legal_set_TWIDDLE_TYPE, "TWIDDLE_TYPE", TWIDDLE_TYPE)


#######################################################
########### TP_DYN_PT_SIZE Updater and Validator ######
#######################################################
def update_POINT_SIZE(args):
    VSS_MODE = int(args["VSS_MODE"])
    PART = args["PART"]
    DATA_TYPE = args["DATA_TYPE"]
    return fn_update_POINT_SIZE(VSS_MODE, PART, DATA_TYPE)


def fn_update_POINT_SIZE(VSS_MODE, PART, DATA_TYPE):
    legal_set_POINT_SIZE = [64]
    min_point_size_pwr_aie1 = 4
    min_point_size_pwr_aie2 = 5
    AIE_VARIANT = get_aie_arch_from_part_name(PART)
    max_point_size_power = 16
    if VSS_MODE == 1 and "cint16" in DATA_TYPE and AIE_VARIANT == 1:
        max_point_size_power = 15
    # add code that updates legalset with powers of two from 4 to 65536 in a for loop
    if VSS_MODE == 1:
        if AIE_VARIANT  == 1:
            for i in range(2*min_point_size_pwr_aie1, max_point_size_power+1): # factor of 2 appears because the overall point size of vss in mode 1 is square of point size of aie kernel
                legal_set_POINT_SIZE.append(2 ** i)
        else:
            for i in range(2*min_point_size_pwr_aie2, max_point_size_power+1):
                legal_set_POINT_SIZE.append(2 ** i)
    elif VSS_MODE == 2:
        if AIE_VARIANT  == 1:
            for i in range(min_point_size_pwr_aie1+1, max_point_size_power+1): # adding 1 to min size because overall point size of vss in mode 2 is ssr*(point size of aie kernel). Minimum ssr in this mode is 2.
                legal_set_POINT_SIZE.append(2 ** i)
        else:
            for i in range(min_point_size_pwr_aie2+1, max_point_size_power+1):
                legal_set_POINT_SIZE.append(2 ** i)
    param_dict = {}
    param_dict.update({"name": "POINT_SIZE"})
    param_dict.update({"enum": legal_set_POINT_SIZE})
    return param_dict


def validate_POINT_SIZE(args):
    POINT_SIZE = args["POINT_SIZE"]
    VSS_MODE = int(args["VSS_MODE"])
    PART = args["PART"]
    DATA_TYPE = args["DATA_TYPE"]
    return fn_validate_POINT_SIZE(POINT_SIZE, VSS_MODE, PART, DATA_TYPE)


def fn_validate_POINT_SIZE(POINT_SIZE, VSS_MODE, PART, DATA_TYPE):
    param_dict = fn_update_POINT_SIZE(VSS_MODE, PART, DATA_TYPE)
    legal_set_POINT_SIZE = param_dict["enum"]
    return validate_legal_set(legal_set_POINT_SIZE, "POINT_SIZE", int(POINT_SIZE))


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
    VSS_MODE = args["VSS_MODE"]
    SSR = int(args["SSR"])
    return fn_update_shift_val(DATA_TYPE, VSS_MODE, SSR)


def fn_update_shift_val(DATA_TYPE, VSS_MODE, SSR):
    param_dict = {}
    param_dict.update({"name": "SHIFT"})
    param_dict.update({"minimum": SHIFT_min})
    if DATA_TYPE == "cfloat":
        param_dict.update({"maximum": SHIFT_min})
    else:
        if VSS_MODE == 2:
            min_shift_vss2  = fn_log2(int(SSR))
            param_dict.update({"minimum": min_shift_vss2})
        param_dict.update({"maximum": SHIFT_max})
    return param_dict


def validate_SHIFT(args):
    SHIFT = args["SHIFT"]
    DATA_TYPE = args["DATA_TYPE"]
    VSS_MODE = args["VSS_MODE"]
    SSR = int(args["SSR"])
    return fn_validate_shift_val(DATA_TYPE, SHIFT, VSS_MODE, SSR)


def fn_validate_shift_val(DATA_TYPE, SHIFT, VSS_MODE, SSR):
    param_dict = fn_update_shift_val(DATA_TYPE, VSS_MODE, SSR)
    legal_range_SHIFT = [param_dict["minimum"], param_dict["maximum"]]
    return validate_range(legal_range_SHIFT, "SHIFT", int(SHIFT))


#######################################################
########### API_IO Updater and Validator ##############
#######################################################
def update_API_IO(args):
    VSS_MODE = args["VSS_MODE"]
    PART = args["PART"]
    return fn_update_API_IO(VSS_MODE, PART)

def fn_update_API_IO(VSS_MODE, PART):
    AIE_VARIANT = get_aie_arch_from_part_name(PART)
    if AIE_VARIANT == 1 and str(VSS_MODE) == "2":
        legal_set_API_IO = [0, 1]
    else:
        legal_set_API_IO = [0]
    param_dict = {}
    param_dict.update({"name": "API_IO"})
    param_dict.update({"enum": legal_set_API_IO})
    return param_dict


def validate_API_IO(args):
    API_IO = args["API_IO"]
    VSS_MODE = args["VSS_MODE"]
    PART = args["PART"]
    return fn_validate_API_IO(API_IO, VSS_MODE, PART)


def fn_validate_API_IO(API_IO, VSS_MODE, PART):
    param_dict = fn_update_API_IO(VSS_MODE, PART)
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


def legal_ssr_point_size(POINT_SIZE, SSR, PART, DATA_TYPE, POINT_SIZE_D1):
    point_size_d1_act = fn_pt_size_d1(POINT_SIZE) if POINT_SIZE_D1 == 1 else POINT_SIZE_D1
    point_size_d2 = POINT_SIZE / point_size_d1_act
    ceil_d1_ssr = CEIL(point_size_d1_act, SSR)
    ceil_d2_ssr = CEIL(point_size_d2, SSR)
    window_size_d1 = ceil_d2_ssr / SSR * point_size_d1_act * fn_size_by_byte(DATA_TYPE)
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
    POINT_SIZE = int(args["POINT_SIZE"])
    PART = args["PART"]
    DATA_TYPE = args["DATA_TYPE"]
    VSS_MODE = args["VSS_MODE"]
    POINT_SIZE_D1 = int(args["POINT_SIZE_D1"])
    return fn_update_ssr(POINT_SIZE, PART, DATA_TYPE, VSS_MODE, POINT_SIZE_D1)

def fn_update_ssr(POINT_SIZE, PART, DATA_TYPE, VSS_MODE, POINT_SIZE_D1):
    range_SSR = [2, 64]
    param_dict = {}
    param_dict.update({"name": "SSR"})
    if VSS_MODE == 2:
        # create a list with powers of two from 2 to 64
        legal_set_TP_SSR = [2**i for i in range(1, 7)]
        param_dict.update({"enum": legal_set_TP_SSR})
    else:
        legal_set_TP_SSR = list(range(range_SSR[0], range_SSR[1] + 1))
        remove_list = []
        for cur_ssr in legal_set_TP_SSR:
            if (
                legal_ssr_point_size(POINT_SIZE, cur_ssr, PART, DATA_TYPE, POINT_SIZE_D1) == False
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
    SSR = int(args["SSR"])
    POINT_SIZE = int(args["POINT_SIZE"])
    PART = args["PART"]
    DATA_TYPE = args["DATA_TYPE"]
    VSS_MODE = int(args["VSS_MODE"])
    POINT_SIZE_D1 = int(args["POINT_SIZE_D1"])
    return fn_validate_ssr(SSR, POINT_SIZE, PART, DATA_TYPE, VSS_MODE, POINT_SIZE_D1)


def fn_validate_ssr(SSR, POINT_SIZE, PART, DATA_TYPE, VSS_MODE, POINT_SIZE_D1):
    param_dict = fn_update_ssr(POINT_SIZE, PART, DATA_TYPE, VSS_MODE, POINT_SIZE_D1)
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
    return fn_update_VSS_MODE()


def fn_update_VSS_MODE():
    legal_set = [1, 2]
    param_dict = {}
    param_dict.update({"name": "VSS_MODE"})
    param_dict.update({"enum": legal_set})
    return param_dict


def validate_VSS_MODE(args):
    VSS_MODE = args["VSS_MODE"]
    return fn_validate_VSS_MODE(VSS_MODE)


def fn_validate_VSS_MODE(VSS_MODE):
    param_dict = fn_update_VSS_MODE()
    legal_set_VSS_MODE = param_dict["enum"]
    return validate_legal_set(legal_set_VSS_MODE, "VSS_MODE", int(VSS_MODE))

#######################################################
########### TP_CASC_LEN Updater and Validator #########
#######################################################
def update_TP_CASC_LEN(args):
    PART = args["PART"]
    DATA_TYPE = args["DATA_TYPE"]
    POINT_SIZE = int(args["POINT_SIZE"])
    POINT_SIZE_D1 = int(args.get("POINT_SIZE_D1", 1))
    VSS_MODE = int(args["VSS_MODE"])
    return fn_update_TP_CASC_LEN(PART, DATA_TYPE, POINT_SIZE, POINT_SIZE_D1, VSS_MODE)


def fn_update_TP_CASC_LEN(PART, DATA_TYPE, POINT_SIZE, POINT_SIZE_D1, VSS_MODE):
    point_size_d1_act = fn_pt_size_d1(POINT_SIZE) if POINT_SIZE_D1 == 1 else POINT_SIZE_D1
    point_size_d2 = POINT_SIZE / point_size_d1_act
    aie_point_size = max(point_size_d1_act, point_size_d2) if VSS_MODE == 1 else point_size_d1_act
    log2PointSize = fn_log2(int(aie_point_size))
    AIE_VARIANT = get_aie_arch_from_part_name(PART)
    # equation for integer ffts is complicated by the fact that odd power of 2 point sizes start with a radix 2 stage
    # Further, since integer implementation uses radix4, 2 ranks per kernel after the initial possible single radix2 is forced, so
    NUM_STAGES = (CEIL(log2PointSize, 2) / 2) if DATA_TYPE in ["cint16", "cint32"] else log2PointSize
    maxTP_CASC_LEN = min(TP_CASC_LEN_max, int(NUM_STAGES))

    #In one particular scenario, cascade increases the memory requirement per kernel because the cascade data type is cint32 which increases memory use past the limit  
    if (AIE_VARIANT==AIE) and (aie_point_size == 4096) and (DATA_TYPE in ["cint16", "cint32"]):
        maxTP_CASC_LEN = 1

    param_dict = {}
    param_dict.update({"name": "TP_CASC_LEN"})
    param_dict.update({"minimum": TP_CASC_LEN_min})
    param_dict.update({"maximum": maxTP_CASC_LEN})

    return param_dict


def validate_TP_CASC_LEN(args):
    PART = args["PART"]
    DATA_TYPE = args["DATA_TYPE"]
    POINT_SIZE = int(args["POINT_SIZE"])
    TP_CASC_LEN = int(args["CASC_LEN"])
    POINT_SIZE_D1 = int(args["POINT_SIZE_D1"])
    VSS_MODE = int(args["VSS_MODE"])
    return fn_validate_casc_len(PART, DATA_TYPE, POINT_SIZE, TP_CASC_LEN, POINT_SIZE_D1, VSS_MODE)


def fn_validate_casc_len(PART, DATA_TYPE, POINT_SIZE, TP_CASC_LEN, POINT_SIZE_D1, VSS_MODE):
    param_dict = fn_update_TP_CASC_LEN(PART, DATA_TYPE, POINT_SIZE, POINT_SIZE_D1, VSS_MODE)
    range_TP_CASC_LEN = [param_dict["minimum"], param_dict["maximum"]]
    return validate_range(range_TP_CASC_LEN, "TP_CASC_LEN", TP_CASC_LEN)


#######################################################
########### USE_WIDGETS Updater and Validator ##############
#######################################################
def update_USE_WIDGETS(args):
    VSS_MODE = int(args["VSS_MODE"])
    PART = args["PART"]
    API_IO = int(args["API_IO"])
    return fn_update_USE_WIDGETS(VSS_MODE, PART, API_IO)

def fn_update_USE_WIDGETS(VSS_MODE, PART, API_IO):
    AIE_VARIANT = get_aie_arch_from_part_name(PART)
    if AIE_VARIANT == 1 and API_IO == 1 and VSS_MODE == 2:
        legal_set_USE_WIDGETS = [0, 1]
    else:
        legal_set_USE_WIDGETS = [0]
    
    param_dict = {}
    param_dict.update({"name": "USE_WIDGETS"})
    param_dict.update({"enum": legal_set_USE_WIDGETS})

    return param_dict


def validate_USE_WIDGETS(args):
    USE_WIDGETS = args["USE_WIDGETS"]
    VSS_MODE = args["VSS_MODE"]
    PART = args["PART"]
    API_IO = args["API_IO"]
    return fn_validate_USE_WIDGETS(USE_WIDGETS, VSS_MODE, PART, API_IO)


def fn_validate_USE_WIDGETS(USE_WIDGETS, VSS_MODE, PART, API_IO):
    param_dict = fn_update_USE_WIDGETS(VSS_MODE, PART, API_IO)
    legal_set_USE_WIDGETS = param_dict["enum"]
    return validate_legal_set(legal_set_USE_WIDGETS, "USE_WIDGETS", int(USE_WIDGETS))


#######################################################
########### POINT_SIZE_D1 Updater and Validator #######
#######################################################
def update_POINT_SIZE_D1(args):
    POINT_SIZE = int(args["POINT_SIZE"])
    VSS_MODE = int(args["VSS_MODE"])
    DATA_TYPE = args["DATA_TYPE"]
    PART = args["PART"]
    return fn_update_POINT_SIZE_D1(POINT_SIZE, VSS_MODE, DATA_TYPE, PART)


def fn_update_POINT_SIZE_D1(POINT_SIZE, VSS_MODE, DATA_TYPE, PART):
    param_dict = {}
    param_dict.update({"name": "POINT_SIZE_D1"})
    param_dict.update({"minimum": 1})
    AIE_VARIANT = get_aie_arch_from_part_name(PART)
    if VSS_MODE == 1 and AIE_VARIANT == 1 and "cint16" in DATA_TYPE:
        param_dict.update({"maximum": 255})
    elif VSS_MODE == 2 and AIE_VARIANT == 1 and "cint16" in DATA_TYPE and POINT_SIZE_D1 != 1: # point_size_d1 = 1 means default simplified configuration, means that AIE BDs don't need to be used so only check for other cases
        param_dict.update({"maximum": 255})
    else:
        param_dict.update({"maximum": int(POINT_SIZE) // 2})
    return param_dict


def validate_POINT_SIZE_D1(args):
    POINT_SIZE_D1 = int(args["POINT_SIZE_D1"])
    POINT_SIZE = int(args["POINT_SIZE"])
    VSS_MODE = int(args["VSS_MODE"])
    DATA_TYPE = args["DATA_TYPE"]
    PART = args["PART"]
    return fn_validate_POINT_SIZE_D1(POINT_SIZE_D1, POINT_SIZE, VSS_MODE, DATA_TYPE, PART)


def fn_validate_POINT_SIZE_D1(POINT_SIZE_D1, POINT_SIZE, VSS_MODE, DATA_TYPE, PART):
    param_dict = fn_update_POINT_SIZE_D1(POINT_SIZE, VSS_MODE, DATA_TYPE, PART)
    range_POINT_SIZE_D1 = [param_dict["minimum"], param_dict["maximum"]]
    
    # Check range first
    range_check = validate_range(range_POINT_SIZE_D1, "POINT_SIZE_D1", POINT_SIZE_D1)
    if range_check["is_valid"] == False:
        return range_check
    
    # Check divisibility: POINT_SIZE % POINT_SIZE_D1 == 0
    if POINT_SIZE % POINT_SIZE_D1 != 0:
        return isError(f"POINT_SIZE_D1 ({POINT_SIZE_D1}) must evenly divide POINT_SIZE ({POINT_SIZE})")
    
    return range_check


#######################################################
########### ADD_FRONT_TRANSPOSE Updater and Validator #
#######################################################
def update_ADD_FRONT_TRANSPOSE(args):
    return fn_update_ADD_FRONT_TRANSPOSE()


def fn_update_ADD_FRONT_TRANSPOSE():
    legal_set = [1]
    param_dict = {}
    param_dict.update({"name": "ADD_FRONT_TRANSPOSE"})
    param_dict.update({"enum": legal_set})
    return param_dict


def validate_ADD_FRONT_TRANSPOSE(args):
    ADD_FRONT_TRANSPOSE = args["ADD_FRONT_TRANSPOSE"]
    return fn_validate_ADD_FRONT_TRANSPOSE(ADD_FRONT_TRANSPOSE)


def fn_validate_ADD_FRONT_TRANSPOSE(ADD_FRONT_TRANSPOSE):
    param_dict = fn_update_ADD_FRONT_TRANSPOSE()
    legal_set = param_dict["enum"]
    return validate_legal_set(legal_set, "ADD_FRONT_TRANSPOSE", int(ADD_FRONT_TRANSPOSE))


#######################################################
########### ADD_BACK_TRANSPOSE Updater and Validator ##
#######################################################
def update_ADD_BACK_TRANSPOSE(args):
    return fn_update_ADD_BACK_TRANSPOSE()


def fn_update_ADD_BACK_TRANSPOSE():
    legal_set = [1]
    param_dict = {}
    param_dict.update({"name": "ADD_BACK_TRANSPOSE"})
    param_dict.update({"enum": legal_set})
    return param_dict


def validate_ADD_BACK_TRANSPOSE(args):
    ADD_BACK_TRANSPOSE = args["ADD_BACK_TRANSPOSE"]
    return fn_validate_ADD_BACK_TRANSPOSE(ADD_BACK_TRANSPOSE)


def fn_validate_ADD_BACK_TRANSPOSE(ADD_BACK_TRANSPOSE):
    param_dict = fn_update_ADD_BACK_TRANSPOSE()
    legal_set = param_dict["enum"]
    return validate_legal_set(legal_set, "ADD_BACK_TRANSPOSE", int(ADD_BACK_TRANSPOSE))


def info_ports():
    return 0


def generate_cfg(aie_graph_name, args):
    out = {}
    PART = args["PART"]
    FREQ = args["FREQHZ"]
    macro_body_str_init = f"""part={PART}
freqhz={FREQ}

[aie]
enable-partition=5:26:fft_aie
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

def generate_graph(aie_graph_name, args):
    return {}
