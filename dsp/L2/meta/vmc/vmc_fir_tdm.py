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
from fir_tdm import *
from aie_common import *
from aie_common_fir import *
from vmc_fir_utils import *

#### VMC validators ####
def vmc_validate_tt_data(args):
    data_type = args["data_type"]
    AIE_VARIANT = args["AIE_VARIANT"]
    return fn_validate_TT_DATA(AIE_VARIANT, data_type)

def vmc_validate_coef_type(args):
    AIE_VARIANT = args["AIE_VARIANT"]
    data_type = args["data_type"]
    coef_type = args["coef_type"]
    return fn_validate_TT_COEFF(AIE_VARIANT, data_type, coef_type)

def vmc_validate_data_out_type(args):
    data_type = args["data_type"]
    data_out_type = args["data_out_type"]
    return fn_validate_TT_OUT_DATA(data_type, data_out_type)

def vmc_validate_input_window_size(args):
    coef_type = args["coef_type"]
    data_type = args["data_type"]
    data_out_type = args["data_out_type"]
    api = 0
    ssr = args["ssr"]
    input_window_size = args["input_window_size"]
    fir_length = args["fir_length"]
    tdm_channels = args["tdm_channels"]
    AIE_VARIANT = args["AIE_VARIANT"]
    return fn_validate_TP_INPUT_WINDOW_VSIZE(data_type, data_out_type, coef_type, fir_length, input_window_size, api, ssr, tdm_channels, AIE_VARIANT)

def vmc_validate_fir_length(args):
    coef_type = args["coef_type"]
    data_type = args["data_type"]
    api = 0
    fir_length = args["fir_length"]
    return fn_validate_TP_FIR_LEN(data_type, coef_type, fir_length, api)

def vmc_validate_shift_val(args):
    AIE_VARIANT = args["AIE_VARIANT"]
    data_type = args["data_type"]
    shift_val = args["shift_val"]
    return fn_validate_shift_val(AIE_VARIANT, data_type, shift_val)

def vmc_validate_ssr(args):
    data_type = args["data_type"]
    coef_type = args["coef_type"]
    tdm_channels = args["tdm_channels"]
    ssr = args["ssr"]
    AIE_VARIANT = args["AIE_VARIANT"]
    return fn_validate_TP_SSR(AIE_VARIANT, data_type, coef_type, tdm_channels, ssr)


def validate_sat_mode(args):
    sat_mode = args["sat_mode"]
    return fn_validate_satMode(sat_mode);

def vmc_validate_rnd_mode(args):
    rnd_mode = args["rnd_mode"]
    AIE_VARIANT = args["AIE_VARIANT"]
    return fn_validate_roundMode(rnd_mode, AIE_VARIANT)

def vmc_validate_tdm_channels(args):
    coef_type = args["coef_type"]
    data_type = args["data_type"]
    tdm_channels = args["tdm_channels"]
    fir_length = args["fir_length"]
    AIE_VARIANT = args["AIE_VARIANT"]
    return fn_validate_TP_TDM_CHANNELS(data_type, coef_type, fir_length, AIE_VARIANT, tdm_channels)

def vmc_validate_casc_length(args):
    coef_type = args["coef_type"]
    data_type = args["data_type"]
    casc_length = args["casc_length"]
    fir_length = args["fir_length"]
    AIE_VARIANT = args["AIE_VARIANT"]
    return fn_validate_TP_CASC_LEN(data_type, coef_type, fir_length, AIE_VARIANT, casc_length)

#### VMC graph generator ####
def vmc_generate_graph(name, args):
    tmpargs = {}
    tmpargs["TT_DATA"] = args["data_type"]
    tmpargs["TT_OUT_DATA"] = args["data_out_type"]
    tmpargs["TT_COEFF"] = args["coef_type"]
    tmpargs["TP_FIR_LEN"] = args["fir_length"]
    tmpargs["TP_SHIFT"] = args["shift_val"]
    tmpargs["TP_RND"] = args["rnd_mode"]
    tmpargs["TP_INPUT_WINDOW_VSIZE"] = args["input_window_size"]
    tmpargs["TP_TDM_CHANNELS"] = args["tdm_channels"]
    tmpargs["TP_NUM_OUTPUTS"] = 1
    tmpargs["TP_DUAL_IP"] = 0
    tmpargs["TP_API"] = 0
    tmpargs["TP_CASC_LEN"] = args["casc_length"]
    tmpargs["TP_SSR"] = args["ssr"]
    tmpargs["coeff"] = args["coeff"]
    tmpargs["TP_SAT"] = args["sat_mode"]

    return generate_graph(name, tmpargs)
