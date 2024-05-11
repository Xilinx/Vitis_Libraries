from fir_tdm import *
from aie_common import *
from aie_common_fir import *
from vmc_fir_utils import *

#### VMC validators ####
def vmc_validate_coef_type(args):
    data_type = args["data_type"]
    coef_type = args["coef_type"]
    return fn_validate_coeff_type(data_type, coef_type)

def vmc_validate_input_window_size(args):
    coef_type = args["coef_type"]
    data_type = args["data_type"]
    api = 1
    ssr = args["ssr"]
    input_window_size = args["input_window_size"]
    fir_length = args["fir_length"]
    tdm_channels = args["tdm_channels"]
    return fn_validate_input_window_size(data_type, coef_type, fir_length, input_window_size, api, ssr, tdm_channels)

def vmc_validate_fir_length(args):
    CASC_LEN  = 1
    USE_COEFF_RELOAD  = 0
    coef_type = args["coef_type"]
    data_type = args["data_type"]
    ssr = args["ssr"]
    api = 1
    fir_length = args["fir_length"]
    return fn_validate_fir_len(data_type, coef_type, fir_length, CASC_LEN, ssr, api, USE_COEFF_RELOAD)

def vmc_validate_shift_val(args):
    data_type = args["data_type"]
    shift_val = args["shift_val"]
    return fn_validate_shift(data_type, shift_val)

def vmc_validate_ssr(args):
    ssr = args["ssr"]
    return fn_validate_ssr(ssr);

def vmc_validate_dual_ip(args):
    num_outputs = fn_get_num_outputs(args)
    dual_ip = args["dual_ip"]
    api = 1
    AIE_VARIANT = args["AIE_VARIANT"]
    return fn_validate_tdm_dual_ip(num_outputs, api, dual_ip, AIE_VARIANT);

def vmc_validate_out_ports(args):
    num_outputs = fn_get_num_outputs(args)
    ssr = args["ssr"]
    api = 1
    AIE_VARIANT = args["AIE_VARIANT"]
    return fn_validate_tdm_num_outputs(num_outputs, api, ssr, AIE_VARIANT)

def validate_sat_mode(args):
    sat_mode = args["sat_mode"]
    return fn_validate_satMode(sat_mode);

def vmc_validate_rnd_mode(args):
    rnd_mode = args["rnd_mode"]
    AIE_VARIANT = args["AIE_VARIANT"]
    return fn_validate_roundMode(rnd_mode, AIE_VARIANT)

def vmc_validate_tdm_channels(args):
    CASC_LEN  = 1
    coef_type = args["coef_type"]
    data_type = args["data_type"]
    ssr = args["ssr"]
    tdm_channels = args["tdm_channels"]
    fir_length = args["fir_length"]
    return fn_validate_tdm_channels(data_type, coef_type, tdm_channels, ssr, fir_length, CASC_LEN)

#### VMC graph generator ####
def vmc_generate_graph(name, args):
    tmpargs = {}
    tmpargs["TT_DATA"] = args["data_type"]
    tmpargs["TT_COEFF"] = args["coef_type"]
    tmpargs["TP_FIR_LEN"] = args["fir_length"]
    tmpargs["TP_SHIFT"] = args["shift_val"]
    tmpargs["TP_RND"] = args["rnd_mode"]
    tmpargs["TP_INPUT_WINDOW_VSIZE"] = args["input_window_size"]
    tmpargs["TP_TDM_CHANNELS"] = args["tdm_channels"]
    tmpargs["TP_NUM_OUTPUTS"] = fn_get_num_outputs(args)
    tmpargs["TP_DUAL_IP"] = 1 if args["dual_ip"] else 0
    tmpargs["TP_API"] = 1
    tmpargs["TP_SSR"] = args["ssr"]
    tmpargs["coeff"] = args["coeff"]
    tmpargs["TP_SAT"] = args["sat_mode"]

    return generate_graph(name, tmpargs)
