from fir_decimate_sym import *
from aie_common import *
from vmc_fir_utils import *

#### VMC validators ####
def vmc_validate_coeff_type(args):
    data_type = args["data_type"]
    coef_type = args["coef_type"]
    AIE_VARIANT = args["AIE_VARIANT"]
    standard_checks =  fn_validate_coeff_type(data_type, coef_type)
    type_check = fn_type_sym_dec_support(data_type, coef_type, AIE_VARIANT)
    for check in (standard_checks,type_check) :
        if check["is_valid"] == False :
            return check
    return {"is_valid": True}

def vmc_validate_input_window_size(args):
    input_window_size = args["input_window_size"]
    data_type = args["data_type"]
    use_coeff_reload = args["use_coeff_reload"]
    coef_type = args["coef_type"]
    coeff = args["coeff"]
    decimate_factor = args["decimate_factor"]
    api = 1
    ssr = args["ssr"]
    fir_length = args["fir_length"]
    AIE_VARIANT = args["AIE_VARIANT"]
    return fn_validate_input_window_size(data_type, coef_type, fir_length, decimate_factor, input_window_size, api, ssr, AIE_VARIANT)

def vmc_validate_casc_length(args):
    casc_length = args["casc_length"]
    return fn_validate_casc_len(casc_length);

def validate_sat_mode(args):
    sat_mode = args["sat_mode"]
    return fn_validate_satMode(sat_mode);

def vmc_validate_coeff(args):
    use_coeff_reload = args["use_coeff_reload"]
    coef_type = args["coef_type"]
    coeff = args["coeff"]
    data_type = args["data_type"]
    casc_length = args["casc_length"]
    decimate_factor = args["decimate_factor"]
    ssr = args["ssr"]
    api = 1
    fir_length = args["fir_length"]
    AIE_VARIANT = args["AIE_VARIANT"]
    return fn_validate_fir_len(data_type, coef_type, fir_length, decimate_factor, casc_length, ssr, api, use_coeff_reload, AIE_VARIANT )

def vmc_validate_shift_val(args):
    data_type = args["data_type"]
    shift_val = args["shift_val"]
    return fn_validate_shift(data_type, shift_val)

def vmc_validate_ssr(args):
    decimate_factor = args["decimate_factor"]
    ssr = args["ssr"]
    api = 1
    return fn_validate_deci_ssr(ssr, decimate_factor, api)

def vmc_validate_decimate_factor(args):
    data_type = args["data_type"]
    coef_type = args["coef_type"]
    decimate_factor = args["decimate_factor"]
    api = 1
    return fn_validate_decimate_factor(data_type, coef_type, decimate_factor, api)

def vmc_validate_dual_ip(args):
    dual_ip = 1 if args["dual_ip"] else 0
    api = 1
    AIE_VARIANT = args["AIE_VARIANT"]
    return fn_validate_sym_dual_ip(api, dual_ip, AIE_VARIANT)

def vmc_validate_num_outputs(args):
    num_outputs = fn_get_num_outputs(args)
    api = 1
    AIE_VARIANT = args["AIE_VARIANT"]
    return fn_validate_num_outputs(api, num_outputs, AIE_VARIANT)

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
    tmpargs["TP_API"] = 1
    tmpargs["TP_SSR"] = args["ssr"]
    tmpargs["coeff"] = args["coeff"]
    tmpargs["TP_SAT"] = args["sat_mode"]
    tmpargs["AIE_VARIANT"] = args["AIE_VARIANT"]

    return generate_graph(name, tmpargs)
