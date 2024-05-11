from fir_sr_asym import *
from aie_common import *
from vmc_fir_utils import *

#### VMC validators ####
def vmc_validate_coeff_type(args):
    data_type = args["data_type"]
    coef_type = args["coef_type"]
    AIE_VARIANT = args["AIE_VARIANT"]
    standard_checks =  fn_validate_coeff_type(data_type, coef_type)
    type_check = fn_type_sr_support(data_type, coef_type, AIE_VARIANT)
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
    ssr = args["ssr"]
    api = 1
    fir_length = fn_get_fir_length(args)
    return fn_validate_input_window_size(data_type, coef_type, fir_length, input_window_size, api, ssr)

def vmc_validate_casc_len(args):
    casc_length = args["casc_length"]
    return fn_validate_casc_len(casc_length);

def vmc_validate_dual_ip(args):
    dual_ip = args["dual_ip"]
    num_outputs = fn_get_num_outputs(args)
    api = 1
    AIE_VARIANT = args["AIE_VARIANT"]
    return fn_validate_sr_dual_ip(num_outputs, api, dual_ip, AIE_VARIANT)

def vmc_validate_out_ports(args):
    api = 1
    num_outputs = fn_get_num_outputs(args)
    AIE_VARIANT = args["AIE_VARIANT"]
    return fn_validate_num_outputs(api, num_outputs, AIE_VARIANT)

def vmc_validate_coeff(args):
    use_coeff_reload = args["use_coeff_reload"]
    coef_type = args["coef_type"]
    coeff = args["coeff"]
    data_type = args["data_type"]
    casc_length = args["casc_length"]
    ssr = args["ssr"]
    api = 1
    fir_length = fn_get_fir_length(args)
    AIE_VARIANT = args["AIE_VARIANT"]
    return fn_validate_fir_len(data_type, coef_type, fir_length, casc_length, ssr, api, use_coeff_reload, AIE_VARIANT)

def vmc_validate_shift_val(args):
    data_type = args["data_type"]
    shift_val = args["shift_val"]
    return fn_validate_shift(data_type, shift_val)

def vmc_validate_use_coeff_reload(args):
    use_coeff_reload = args["use_coeff_reload"]
    ssr = args["ssr"]
    api = 1
    return fn_validate_use_coeff_reload(api, use_coeff_reload, ssr)

def vmc_validate_ssr(args):
    ssr = args["ssr"]
    return fn_validate_ssr(ssr);

def validate_sat_mode(args):
    sat_mode = args["sat_mode"]
    return fn_validate_satMode(sat_mode);

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
    fir_length = fn_get_fir_length(args)
    tmpargs["TT_COEFF"] = coef_type
    tmpargs["TP_FIR_LEN"] = fir_length
    tmpargs["TP_SHIFT"] = args["shift_val"]
    tmpargs["TP_RND"] = args["rnd_mode"]
    tmpargs["TP_INPUT_WINDOW_VSIZE"] = args["input_window_size"]
    casc_length = args["casc_length"]
    tmpargs["TP_CASC_LEN"] = casc_length
    tmpargs["TP_USE_COEFF_RELOAD"] = 1 if args["use_coeff_reload"] else 0
    tmpargs["TP_NUM_OUTPUTS"] = fn_get_num_outputs(args)
    tmpargs["TP_DUAL_IP"] = 1 if args["dual_ip"] else 0
    tmpargs["TP_API"] = 1
    tmpargs["TP_SSR"] = args["ssr"]
    tmpargs["coeff"] = coeff
    tmpargs["TP_SAT"] = args["sat_mode"]

    return generate_graph(name, tmpargs)
