from mixed_radix_fft import *
import json

#### VMC validators ####
def vmc_validate_point_size(args):
    point_size = args["point_size"]
    aie_variant = args["AIE_VARIANT"]
    return fn_validate_point_size(point_size, aie_variant)

def vmc_validate_shift_val(args):
    data_type = args["data_type"]
    shift_val = args["shift_val"]
    return fn_validate_shift(data_type, shift_val)

def vmc_validate_input_window_size(args):
    point_size = args["point_size"]
    input_window_size = args["input_window_size"]
    return fn_validate_window_vsize(point_size, input_window_size)

def vmc_validate_casc_length(args):
    point_size = args["point_size"]
    casc_length = args["casc_length"]
    return fn_validate_casc_len(point_size, casc_length)

def vmc_validate_TP_RND(args):
  rnd_mode = args["rnd_mode"]
  aie_variant = args["AIE_VARIANT"]
  return fn_validate_rnd(rnd_mode, aie_variant)

def vmc_validate_sat_mode(args):
    sat_mode = args["sat_mode"]
    return fn_validate_sat(sat_mode);

def vmc_validate_twiddle_type(args):
    data_type = args["data_type"]
    twiddle_type = args["twiddle_type"]
    return fn_validate_twiddle_type(data_type, twiddle_type)


#### VMC graph generator ####
def vmc_generate_graph(name, args):
    tmpargs = {}
    tmpargs["TT_DATA"] = args["data_type"]
    tmpargs["TT_TWIDDLE"] = args["twiddle_type"]
    tmpargs["TP_POINT_SIZE"] = args["point_size"]
    tmpargs["TP_SHIFT"] = args["shift_val"]
    tmpargs["TP_WINDOW_VSIZE"] = args["input_window_size"]
    tmpargs["TP_CASC_LEN"] = args["casc_length"]
    tmpargs["TP_API"] = 1
    tmpargs["TP_FFT_NIFFT"] = 0
    tmpargs["TP_RND"] = args["rnd_mode"]
    tmpargs["TP_SAT"] = args["sat_mode"]
    tmpargs["AIE_VARIANT"] = args["AIE_VARIANT"]

    return generate_graph(name, tmpargs)
