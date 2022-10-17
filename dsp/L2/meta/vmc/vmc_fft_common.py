from fft_ifft_dit_1ch import *
from aie_common import *
import json
import math

def fn_get_parallel_power(ssr):
    pp = -1
    if fn_is_power_of_two(ssr):
      pp = fn_log2(ssr) - 1

    return pp

#### VMC validators ####
def vmc_validate_point_size(args):
    ssr = args["ssr"]
    pp = fn_get_parallel_power(ssr)

    if pp == -1:
      return isError(f"Invalid SSR value specified. The value should be of the form 2^N between 2 and 512.")

    point_size = args["point_size"]
    dyn_point_size = 0;
    data_type = args["data_type"]
    api = 1;
    return fn_validate_point_size(point_size, dyn_point_size, data_type, pp, api)

def vmc_validate_shift_val(args):
    data_type = args["data_type"]
    shift_val = args["shift_val"]
    return fn_validate_shift(data_type, shift_val)

def vmc_validate_input_window_size(args):
    point_size = args["point_size"]
    input_window_size = args["input_window_size"]
    dyn_point_size = 0;
    return fn_validate_window_size(point_size, input_window_size, dyn_point_size)

def vmc_validate_casc_length(args):
    use_casc_length = args["use_casc_length"]
    data_type = args["data_type"]
    point_size = args["point_size"]
    casc_length = args["casc_length"]
    if not use_casc_length:
      return {"is_valid": True}

    return fn_validate_casc_len(data_type, point_size, casc_length)

def vmc_validate_ssr(args):
    api = 1;
    ssr = args["ssr"]
    pp = fn_get_parallel_power(ssr)

    if pp == -1:
      return isError(f"Invalid SSR value specified. The value should be of the form 2^N between 2 and 512.")
	
    return fn_validate_parallel_power(api, pp)
	
# Get twiddle types	
k_twiddle_type = {"cfloat":"cfloat", "cint32":"cint16", "cint16":"cint16"}

def fn_get_twiddle_type(data_type):
	return k_twiddle_type[data_type]

