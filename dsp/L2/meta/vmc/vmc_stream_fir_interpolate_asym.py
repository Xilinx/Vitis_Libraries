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
from fir_interpolate_asym import *
from aie_common import *
import fir_polyphase_decomposer as poly
from vmc_fir_utils import *

def vmc_validate_data_type(args):
    AIE_VARIANT = args["AIE_VARIANT"]
    data_type = args["data_type"]
    return fn_validate_TT_DATA(AIE_VARIANT, data_type)

def vmc_validate_coeff_type(args):
    data_type = args["data_type"]
    coef_type = args["coef_type"]
    AIE_VARIANT = args["AIE_VARIANT"]
    return fn_validate_TT_COEFF(AIE_VARIANT, data_type, coef_type)

def vmc_validate_input_window_size(args):
    vargs=dict(args)
    vargs={
        "TP_INPUT_WINDOW_VSIZE" : args["input_window_size"],
        "TT_DATA"               : args["data_type"],
        "TT_COEFF"              : args["coef_type"],
        "coeff"                 : args["coeff"],
        "TP_INTERPOLATE_FACTOR" : args["interpolate_factor"],
        "TP_PARA_INTERP_POLY"   : args["interp_poly"],
        "TP_API"                : 1,
        "TP_SSR"                : args["ssr"],
        "TP_FIR_LEN"            : fn_get_fir_length(args)
    }
    return validate_TP_INPUT_WINDOW_VSIZE(vargs)

def vmc_validate_casc_length(args):
    vargs={
        "AIE_VARIANT"           : args["AIE_VARIANT"],
        "TT_DATA"               : args["data_type"],
        "TT_COEFF"              : args["coef_type"],
        "TP_USE_COEFF_RELOAD"   : args["use_coeff_reload"],
        "TP_API"                : 1,
        "TP_FIR_LEN"            : fn_get_fir_length(args),
        "TP_SSR"                : args["ssr"],
        "TP_INTERPOLATE_FACTOR" : args["interpolate_factor"],
        "TP_DUAL_IP"            : args["dual_ip"],
        "TP_CASC_LEN"           : args["casc_length"],
        "TP_PARA_INTERP_POLY"   : args["interp_poly"]
    }
    return validate_TP_CASC_LEN(vargs)

def validate_sat_mode(args):
    sat_mode = args["sat_mode"]
    return fn_validate_satMode(sat_mode)

def vmc_validate_fir_length(args):
    data_type = args["data_type"]
    coef_type = args["coef_type"]
    api = 1
    use_coeff_reload = args["use_coeff_reload"]
    AIE_VARIANT = args["AIE_VARIANT"]
    fir_length = args["fir_length"]
    return fn_validate_TP_FIR_LEN(data_type, coef_type, api, use_coeff_reload, AIE_VARIANT, fir_length)

def vmc_validate_coeff(args):
    coef_type = args["coef_type"]
    coeff = args["coeff"]
    fir_length = fn_get_fir_length(args)
    return fn_validate_coeff(coef_type, fir_length, coeff)

def vmc_validate_shift_val(args):
    data_type = args["data_type"]
    shift_val = args["shift_val"]
    return fn_validate_shift(data_type, shift_val)

def vmc_validate_ssr(args):
    vargs={
        "AIE_VARIANT"           : args["AIE_VARIANT"],
        "TT_DATA"               : args["data_type"],
        "TT_COEFF"              : args["coef_type"],
        "TP_USE_COEFF_RELOAD"   : args["use_coeff_reload"],
        "coeff"                 : args["coeff"],
        "TP_API"                : 1,
        "TP_FIR_LEN"            : fn_get_fir_length(args),
        "TP_SSR"                : args["ssr"],
        "TP_INTERPOLATE_FACTOR" : args["interpolate_factor"],
        "TP_DUAL_IP"            : args["dual_ip"],
        "TP_PARA_INTERP_POLY"   : args["interp_poly"]
    }
    return validate_TP_SSR(vargs)

def vmc_validate_interp_poly(args):
    interp_poly = args["interp_poly"]
    interpolate_factor = args["interpolate_factor"]
    return fn_validate_TP_PARA_INTERP_POLY(interpolate_factor, interp_poly)

def vmc_validate_interpolate_factor(args):
    interpolate_factor = args["interpolate_factor"]
    fir_length = fn_get_fir_length(args)
    AIE_VARIANT = args["AIE_VARIANT"]
    return fn_validate_TP_INTERPOLATE_FACTOR(fir_length, interpolate_factor, AIE_VARIANT)

def vmc_validate_input_ports(args):
    dual_ip = args["dual_ip"]
    AIE_VARIANT = args["AIE_VARIANT"]
    api = 1
    return fn_validate_TP_DUAL_IP(AIE_VARIANT, api, dual_ip)

def vmc_validate_out_ports(args):
    num_outputs = fn_get_num_outputs(args)
    AIE_VARIANT = args["AIE_VARIANT"]
    api = 1
    return fn_validate_TP_NUM_OUTPUTS(AIE_VARIANT, api, num_outputs)

def vmc_validate_rnd_mode(args):
    rnd_mode = args["rnd_mode"]
    AIE_VARIANT = args["AIE_VARIANT"]
    return fn_validate_roundMode(rnd_mode, AIE_VARIANT)

#### VMC graph generator ####
def vmc_generate_graph(name, args):
    tmpargs = {}
    tmpargs["TT_DATA"] = args["data_type"]
    use_coeff_reload = args["use_coeff_reload"]
    coef_type = args["coef_type"]
    coeff = args["coeff"]
    tmpargs["TT_COEFF"] = coef_type
    tmpargs["TP_FIR_LEN"] = fn_get_fir_length(args)
    tmpargs["TP_SHIFT"] = args["shift_val"]
    tmpargs["TP_RND"] = args["rnd_mode"]
    tmpargs["TP_INPUT_WINDOW_VSIZE"] = args["input_window_size"]
    tmpargs["TP_INTERPOLATE_FACTOR"] = args["interpolate_factor"]
    casc_length = args["casc_length"]
    tmpargs["TP_CASC_LEN"] = casc_length
    tmpargs["TP_USE_COEFF_RELOAD"] = 1 if args["use_coeff_reload"] else 0
    tmpargs["TP_NUM_OUTPUTS"] = fn_get_num_outputs(args)
    tmpargs["TP_DUAL_IP"] = 1 if args["dual_ip"] else 0
    tmpargs["TP_API"] = 1
    tmpargs["TP_SSR"] = args["ssr"]
    tmpargs["coeff"] = args["coeff"]
    tmpargs["TP_PARA_INTERP_POLY"] = args["interp_poly"]
    tmpargs["TP_SAT"] = args["sat_mode"]

    return generate_graph(name, tmpargs)
