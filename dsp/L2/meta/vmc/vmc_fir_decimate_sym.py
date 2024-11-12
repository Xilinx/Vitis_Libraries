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
from fir_decimate_sym import *
from aie_common import *
from vmc_fir_utils import *

#### VMC validators ####
def vmc_validate_coeff_type(args):
    data_type = args["data_type"]
    coef_type = args["coef_type"]
    AIE_VARIANT = args["AIE_VARIANT"]
    return fn_validate_TT_COEFF(AIE_VARIANT, data_type, coef_type)

def vmc_validate_input_window_size(args):
    input_window_size = args["input_window_size"]
    data_type = args["data_type"]
    coef_type = args["coef_type"]
    decimate_factor = args["decimate_factor"]
    api = 0
    ssr = 1
    fir_length = args["fir_length"]
    AIE_VARIANT = args["AIE_VARIANT"]
    return fn_validate_TP_INPUT_WINDOW_VSIZE(data_type, coef_type, fir_length, decimate_factor, input_window_size, api, ssr, AIE_VARIANT)

def vmc_validate_casc_length(args):
    AIE_VARIANT = args["AIE_VARIANT"]
    data_type = args["data_type"]
    coef_type = args["coef_type"]
    use_coeff_reload = args["use_coeff_reload"]
    decimate_factor = args["decimate_factor"]
    api = 0
    ssr = 1
    fir_length = args["fir_length"]
    casc_length = args["casc_length"]
    return fn_validate_TP_CASC_LEN(AIE_VARIANT, data_type, coef_type, api, fir_length, use_coeff_reload, ssr, decimate_factor, casc_length)

def validate_sat_mode(args):
    sat_mode = args["sat_mode"]
    return fn_validate_satMode(sat_mode)

def vmc_validate_coeff(args):
    use_coeff_reload = args["use_coeff_reload"]
    coef_type = args["coef_type"]
    data_type = args["data_type"]
    api = 0
    fir_length = args["fir_length"]
    return fn_validate_TP_FIR_LEN(data_type, coef_type, fir_length, api, use_coeff_reload)

def vmc_validate_shift_val(args):
    AIE_VARIANT = args["AIE_VARIANT"]
    data_type = args["data_type"]
    shift_val = args["shift_val"]
    return fn_validate_shift_val(AIE_VARIANT, data_type, shift_val)

def vmc_validate_decimate_factor(args):
    data_type = args["data_type"]
    coef_type = args["coef_type"]
    decimate_factor = args["decimate_factor"]
    fir_length = args["fir_length"]
    return fn_validate_TP_DECIMATE_FACTOR(data_type, coef_type, fir_length, decimate_factor)

def vmc_validate_ssr(args):
    ssr = 1
    api = 0
    fir_length = args["fir_length"]
    return fn_validate_TP_SSR(api, fir_length, ssr)

def vmc_validate_dual_ip(args):
    dual_ip = 1 if args["dual_ip"] else 0
    AIE_VARIANT = args["AIE_VARIANT"]
    return fn_validate_TP_DUAL_IP(dual_ip, AIE_VARIANT)

def vmc_validate_rnd_mode(args):
    rnd_mode = args["rnd_mode"]
    AIE_VARIANT = args["AIE_VARIANT"]
    return fn_validate_roundMode(rnd_mode, AIE_VARIANT)

#### VMC graph generator ####
def vmc_generate_graph(name, args):
    tmpargs = {}
    tmpargs["TT_DATA"] = args["data_type"]
    tmpargs["TT_COEFF"] = args["coef_type"]
    tmpargs["TP_FIR_LEN"] = args["fir_length"]
    tmpargs["TP_DECIMATE_FACTOR"] = args["decimate_factor"]
    tmpargs["TP_SHIFT"] = args["shift_val"]
    tmpargs["TP_RND"] = args["rnd_mode"]
    tmpargs["TP_INPUT_WINDOW_VSIZE"] = args["input_window_size"]
    tmpargs["TP_CASC_LEN"] = args["casc_length"]
    tmpargs["TP_USE_COEFF_RELOAD"] = 1 if args["use_coeff_reload"] else 0
    tmpargs["TP_NUM_OUTPUTS"] = fn_get_num_outputs(args)
    tmpargs["TP_DUAL_IP"] = 1 if args["dual_ip"] else 0
    tmpargs["TP_API"] = 0
    tmpargs["TP_SSR"] = 1
    tmpargs["coeff"] = args["coeff"]
    tmpargs["AIE_VARIANT"] = args["AIE_VARIANT"]
    tmpargs["TP_SAT"] = args["sat_mode"]

    return generate_graph(name, tmpargs)
