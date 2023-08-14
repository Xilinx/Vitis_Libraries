from fir_sr_sym import *

#### VMC validators ####
def vmc_validate_coef_type(args):
    data_type = args["data_type"]
    coef_type = args["coef_type"]
    return fn_validate_coef_type(data_type, coef_type)

def vmc_validate_input_window_size(args):
	input_window_size = args["input_window_size"]
	data_type = args["data_type"]
	use_coeff_reload = args["use_coeff_reload"]
	coef_type = args["coef_type"]
	coeff = args["coeff"]
	api = 0
	ssr = args["ssr"]
	fir_length = args["fir_length"]
	return fn_validate_input_window_size(data_type, coef_type, fir_length, input_window_size, api, ssr)

def vmc_validate_coeff(args):
	use_coeff_reload = args["use_coeff_reload"]
	coef_type = args["coef_type"]
	coeff = args["coeff"]
	data_type = args["data_type"]
	casc_length = args["casc_length"]
	ssr = args["ssr"]
	api = 0
	fir_length = args["fir_length"]
	#TODO: Talk to DSP Lib team about separating casc length from fir_length API
	return fn_validate_fir_len(data_type, coef_type, fir_length, casc_length, ssr, api, use_coeff_reload)

def vmc_validate_shift_val(args):
	data_type = args["data_type"]
	shift_val = args["shift_val"]
	return fn_validate_shift(data_type, shift_val)

def vmc_validate_ssr(args):
    ssr = args["ssr"]
    api = 1
    return fn_validate_ssr(api, ssr);

def vmc_validate_casc_len(args):
	casc_length = args["casc_length"]
	return fn_validate_casc_len(casc_length);

def vmc_validate_out_ports(args):
	num_outputs = args["num_outputs"]
	ssr = args["ssr"]
	dual_ip = args["dual_ip"]
	return fn_validate_num_outputs(ssr, dual_ip, num_outputs)

#### VMC graph generator ####
def vmc_generate_graph(name, args):
    tmpargs = {}
    tmpargs["TT_DATA"] = args["data_type"]
    tmpargs["TT_COEF"] = args["coef_type"]
    tmpargs["TP_FIR_LEN"] = args["fir_length"]
    tmpargs["TP_SHIFT"] = args["shift_val"]
    tmpargs["TP_RND"] = args["rnd_mode"]
    tmpargs["TP_INPUT_WINDOW_VSIZE"] = args["input_window_size"]
    tmpargs["TP_CASC_LEN"] = args["casc_length"]
    tmpargs["TP_USE_COEF_RELOAD"] = 1 if args["use_coeff_reload"] else 0
    tmpargs["TP_NUM_OUTPUTS"] = 2 if args["num_outputs"] else 1
    tmpargs["TP_DUAL_IP"] = 1 if args["dual_ip"] else 0
    tmpargs["TP_API"] = 0
    tmpargs["TP_SSR"] = args["ssr"]
    tmpargs["coeff"] = args["coeff"]
   
    return generate_graph(name, tmpargs)
