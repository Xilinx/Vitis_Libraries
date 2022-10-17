from fft_ifft_dit_1ch import *
import json

#### VMC validators ####
def vmc_validate_point_size(args):
    point_size = args["point_size"]
    dyn_point_size = 0;
    data_type = args["data_type"]
    parallel_power = 0;
    api = 0;
    return fn_validate_point_size(point_size, dyn_point_size, data_type, parallel_power, api)

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
	
# Get twiddle types	
k_twiddle_type = {"cfloat":"cfloat", "cint32":"cint16", "cint16":"cint16"}

def fn_get_twiddle_type(data_type):
	return k_twiddle_type[data_type]

#### VMC graph generator ####
def vmc_generate_graph(name, args):
    tmpargs = {}
    tmpargs["TT_DATA"] = args["data_type"]
    tmpargs["TT_TWIDDLE"] = fn_get_twiddle_type(args["data_type"])
    tmpargs["TP_POINT_SIZE"] = args["point_size"]
    tmpargs["TP_SHIFT"] = args["shift_val"]
    tmpargs["TP_WINDOW_VSIZE"] = args["input_window_size"]
    #TODO: call to partitioner to determine cascade length
    tmpargs["TP_CASC_LEN"] = 1
    tmpargs["TP_DYN_PT_SIZE"] = 0
    tmpargs["TP_API"] = 0
    tmpargs["TP_PARALLEL_POWER"] = 0
    tmpargs["TP_FFT_NIFFT"] = 1

    return generate_graph(name, tmpargs)
