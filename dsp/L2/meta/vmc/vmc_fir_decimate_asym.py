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
from fir_decimate_asym import *
from aie_common import *
from vmc_fir_utils import *

#### VMC validators ####
def vmc_validate_coeff_type(args):
    data_type = args["data_type"]
    coef_type = args["coef_type"]
    standard_checks =  fn_validate_coeff_type(data_type, coef_type)
    AIE_VARIANT = args["AIE_VARIANT"]
    type_check = fn_type_support(data_type, coef_type,AIE_VARIANT)
    for check in (standard_checks,type_check) :
        if check["is_valid"] == False :
            return check
    return {"is_valid": True}

def vmc_validate_input_window_size(args):
    vmc_args = dict(args)
    vmc_args={
        "AIE_VARIANT"           : args["AIE_VARIANT"],
        "TP_INPUT_WINDOW_VSIZE" : args["input_window_size"],
        "TT_DATA"               : args["data_type"],
        "TT_COEFF"              : args["coef_type"],
        "TP_USE_COEFF_RELOAD"   : args["use_coeff_reload"],
        "coeff"                 : args["coeff"],
        "TP_DECIMATE_FACTOR"    : args["decimate_factor"],
        "TP_API"                : 0,
        "TP_SSR"                : 1,
        "TP_FIR_LEN"            : fn_get_fir_length(args) }
    return validate_TP_INPUT_WINDOW_VSIZE(vmc_args)

def vmc_validate_casc_length(args):
    AIE_VARIANT = args["AIE_VARIANT"]
    data_type = args["data_type"]
    coef_type = args["coef_type"]
    api=0
    fir_len = fn_get_fir_length(args)
    use_coeff_reload = args["use_coeff_reload"]
    ssr=1
    casc_length = args["casc_length"]
    decimate_factor = args["decimate_factor"]
    return fn_validate_TP_CASC_LEN(AIE_VARIANT, data_type, coef_type, api, fir_len, use_coeff_reload, ssr, casc_length, decimate_factor)


def validate_sat_mode(args):
    sat_mode = args["sat_mode"]
    return fn_validate_satMode(sat_mode);

def vmc_validate_coeff(args):
    fir_len = fn_get_fir_length(args)
    coef_type = args["coef_type"]
    coeff = args["coeff"]
    return fn_validate_coeff(coef_type, fir_len, coeff)

def vmc_validate_shift_val(args):
    data_type = args["data_type"]
    shift_val = args["shift_val"]
    return fn_validate_shift(data_type, shift_val)

def vmc_validate_decimate_factor(args):
    decimate_factor = args["decimate_factor"]
    AIE_VARIANT = args["AIE_VARIANT"]
    fir_len = fn_get_fir_length(args)
    return fn_validate_TP_DECIMATE_FACTOR(AIE_VARIANT, fir_len, decimate_factor)

def vmc_validate_deci_poly(args):
    data_type = args["data_type"]
    coef_type = args["coef_type"]
    decimate_factor = args["decimate_factor"]
    api = 0
    deci_poly = args["deci_poly"]
    AIE_VARIANT = args["AIE_VARIANT"]
    return fn_validate_TP_PARA_DECI_POLY(data_type, coef_type, decimate_factor, api, AIE_VARIANT, deci_poly)

def vmc_validate_out_ports(args):
    num_outputs = fn_get_num_outputs(args)
    AIE_VARIANT = args["AIE_VARIANT"]
    api = 0
    return fn_validate_num_outputs(api, num_outputs, AIE_VARIANT)

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
    tmpargs["TP_DECIMATE_FACTOR"] = args["decimate_factor"]
    tmpargs["TP_SHIFT"] = args["shift_val"]
    tmpargs["TP_RND"] = args["rnd_mode"]
    tmpargs["TP_INPUT_WINDOW_VSIZE"] = args["input_window_size"]
    tmpargs["TP_CASC_LEN"] = args["casc_length"]
    tmpargs["TP_USE_COEFF_RELOAD"] = 1 if args["use_coeff_reload"] else 0
    tmpargs["TP_NUM_OUTPUTS"] = fn_get_num_outputs(args)
    tmpargs["TP_DUAL_IP"] = 0
    tmpargs["TP_API"] = 0
    tmpargs["TP_SSR"] = 1
    tmpargs["TP_PARA_DECI_POLY"] = args["deci_poly"]
    tmpargs["coeff"] = args["coeff"]
    tmpargs["TP_SAT"] = args["sat_mode"]

    return generate_graph(name, tmpargs)
