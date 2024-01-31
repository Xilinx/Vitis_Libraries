from fir_resampler import *
from aie_common import *
from vmc_fir_utils import *

#### VMC validators ####
def vmc_validate_coeff_type(args):
    data_type = args["data_type"]
    coef_type = args["coef_type"]
    AIE_VARIANT = args["AIE_VARIANT"]
    standard_checks = fn_validate_coeff_type(data_type, coef_type)
    type_check = fn_type_support(data_type, coef_type, AIE_VARIANT)
    for check in (standard_checks, type_check):
        if check["is_valid"] == False:
            return check
    return {"is_valid": True}


def vmc_validate_input_window_size(args):
    input_window_size = args["input_window_size"]
    data_type = args["data_type"]
    use_coeff_reload = args["use_coeff_reload"]
    coef_type = args["coef_type"]
    coeff = args["coeff"]
    decimate_factor = args["decimate_factor"]
    interpolate_factor = args["interpolate_factor"]
    deci_poly = args["deci_poly"]
    interp_poly = args["interp_poly"]
    AIE_VARIANT = args["AIE_VARIANT"]
    api = 0
    ssr = 1
    fir_length = fn_get_fir_length(args)
    return fn_validate_input_window_size(
        data_type,
        coef_type,
        fir_length,
        interpolate_factor,
        decimate_factor,
        input_window_size,
        api,
        ssr,
        interp_poly,
        deci_poly,
        AIE_VARIANT
    )


def vmc_validate_casc_length(args):
    casc_length = args["casc_length"]
    return fn_validate_casc_len(casc_length)


def vmc_validate_coeff(args):
    use_coeff_reload = args["use_coeff_reload"]
    coef_type = args["coef_type"]
    coeff = args["coeff"]
    data_type = args["data_type"]
    deci_poly = args["deci_poly"]
    interp_poly = args["interp_poly"]
    casc_length = args["casc_length"]
    decimate_factor = args["decimate_factor"]
    interpolate_factor = args["interpolate_factor"]
    ssr = 1
    AIE_VARIANT = args["AIE_VARIANT"]
    api = 0
    fir_length = fn_get_fir_length(args)
    tmpargs = {}
    tmpargs["TT_COEFF"] = coef_type
    tmpargs["TP_FIR_LEN"] = fir_length
    tmpargs["TP_DECIMATE_FACTOR"] = decimate_factor
    tmpargs["TP_INTERPOLATE_FACTOR"] = interpolate_factor
    tmpargs["TT_DATA"] = data_type
    tmpargs["TP_CASC_LEN"] = casc_length
    tmpargs["TP_USE_COEFF_RELOAD"] = 1 if use_coeff_reload else 0
    tmpargs["AIE_VARIANT"] = AIE_VARIANT
    tmpargs["TP_DUAL_IP"] = 0
    tmpargs["TP_API"] = api
    tmpargs["TP_SSR"] = ssr
    tmpargs["TP_PARA_INTERP_POLY"] = interp_poly
    tmpargs["TP_PARA_DECI_POLY"] = deci_poly
    return fn_validate_fir_len(
        tmpargs,
        data_type,
        coef_type,
        fir_length,
        interpolate_factor,
        decimate_factor,
        casc_length,
        ssr,
        api,
        use_coeff_reload,
        AIE_VARIANT,
        deci_poly,
        interp_poly
    )


def vmc_validate_shift_val(args):
    data_type = args["data_type"]
    shift_val = args["shift_val"]
    return fn_validate_shift_val(data_type, shift_val)

def vmc_validate_decimate_factor(args):
    data_type = args["data_type"]
    coef_type = args["coef_type"]
    decimate_factor = args["decimate_factor"]
    interpolate_factor = args["interpolate_factor"]
    AIE_VARIANT = args["AIE_VARIANT"]
    return fn_validate_decimate_factor(data_type, coef_type, interpolate_factor, decimate_factor, AIE_VARIANT)

def vmc_validate_interpolate_factor(args):
    interpolate_factor = args["interpolate_factor"]
    AIE_VARIANT = args["AIE_VARIANT"]
    return fn_validate_interpolate_factor(interpolate_factor,AIE_VARIANT)

def vmc_validate_deci_poly(args):
    deci_poly = args["deci_poly"]
    decimate_factor = args["decimate_factor"]
    return fn_validate_para_deci_poly(decimate_factor,deci_poly)

def vmc_validate_interp_poly(args):
    interp_poly = args["interp_poly"]
    interpolate_factor = args["interpolate_factor"]
    return fn_validate_para_interp_poly(interpolate_factor, interp_poly)

def validate_sat_mode(args):
    sat_mode = args["sat_mode"]
    return fn_validate_satMode(sat_mode);

def validate_num_outputs(args):
    api = 0
    num_outputs = args["num_outputs"]
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
    use_coeff_reload = args["use_coeff_reload"]
    coef_type = args["coef_type"]
    coeff = args["coeff"]
    tmpargs["TT_COEFF"] = coef_type
    tmpargs["TP_FIR_LEN"] = fn_get_fir_length(args)
    tmpargs["TP_DECIMATE_FACTOR"] = args["decimate_factor"]
    tmpargs["TP_INTERPOLATE_FACTOR"] = args["interpolate_factor"]
    tmpargs["TP_SHIFT"] = args["shift_val"]
    tmpargs["TP_RND"] = args["rnd_mode"]
    tmpargs["TP_INPUT_WINDOW_VSIZE"] = args["input_window_size"]
    tmpargs["TP_CASC_LEN"] = args["casc_length"]
    tmpargs["TP_USE_COEFF_RELOAD"] = 1 if args["use_coeff_reload"] else 0
    tmpargs["TP_NUM_OUTPUTS"] = fn_get_num_outputs(args)
    tmpargs["TP_DUAL_IP"] = 0
    tmpargs["TP_API"] = 0
    tmpargs["TP_SSR"] = 1
    tmpargs["coeff"] = args["coeff"]
    tmpargs["TP_PARA_INTERP_POLY"] = args["interp_poly"]
    tmpargs["TP_PARA_DECI_POLY"] = args["deci_poly"]
    tmpargs["TP_SAT"] = args["sat_mode"]


    return generate_graph(name, tmpargs)
