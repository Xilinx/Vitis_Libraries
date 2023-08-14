from fir_decimate_asym import *

#### VMC validators ####
def vmc_validate_coef_type(args):
	data_type = args["data_type"]
	coef_type = args["coef_type"]
	standard_checks =  fn_validate_coef_type(data_type, coef_type)
	AIE_VARIANT = 1
	type_check = fn_type_support(data_type, coef_type,AIE_VARIANT)
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
	deci_poly = args["deci_poly"]
	if use_coeff_reload:
		fir_length = args["fir_length"]
	else:
		if fn_is_complex(coef_type):
			fir_length = int(len(coeff)/2)
		else:
			fir_length = int(len(coeff))
	return fn_validate_input_window_size(data_type, coef_type, fir_length, decimate_factor, input_window_size, api, ssr)

def vmc_validate_casc_length(args):
    casc_length = args["casc_length"]
    #if not use_casc_length:
	# TODO : Talk to DSP lib team/sumanta about how 
	# cascade validation works - confirm its just fir length related
	#return fn_validate_casc_length(fir_length, casc_length, use_coeff_reload)
    return fn_validate_casc_len(casc_length);
    

def vmc_validate_coeff(args):
	use_coeff_reload = args["use_coeff_reload"]
	coef_type = args["coef_type"]
	coeff = args["coeff"]
	data_type = args["data_type"]
	casc_length = args["casc_length"]
	decimate_factor = args["decimate_factor"]
	ssr = args["ssr"]
	deci_poly = args["deci_poly"]
	api = 1
	if use_coeff_reload:
		fir_length = args["fir_length"]
	else:
		if fn_is_complex(coef_type):
			fir_length = int(len(coeff)/2)
		else:
			fir_length = int(len(coeff))
	#TODO: Talk to DSP Lib team about separating casc length from fir_length API
	return fn_validate_fir_len(data_type, coef_type, fir_length, decimate_factor, casc_length, ssr, api, use_coeff_reload )

def vmc_validate_shift_val(args):
	data_type = args["data_type"]
	shift_val = args["shift_val"]
	return fn_validate_shift(data_type, shift_val)
	
def vmc_validate_decimate_factor(args):
	data_type = args["data_type"]
	coef_type = args["coef_type"]
	decimate_factor = args["decimate_factor"]
	api = 1
	AIE_VARIANT = 1
	return fn_validate_decimate_factor(data_type, coef_type, decimate_factor, api, AIE_VARIANT)

def vmc_validate_decimate_poly(args):
	TP_PARA_DECI_POLY = args["deci_poly"]
	return fn_validate_deci_poly(TP_PARA_DECI_POLY);

def vmc_validate_ssr(args):
    ssr = args["ssr"]
    api = 1
    deci_poly = args["deci_poly"]
    AIE_VARIANT = 1
    decimate_factor = args["decimate_factor"]
    return fn_validate_ssr(ssr, api, decimate_factor, deci_poly, AIE_VARIANT)

def vmc_validate_input_ports(args):
	dual_ip = args["dual_ip"]
	AIE_VARIANT = 1
	api = 1
	return fn_validate_dual_ip(api, dual_ip, AIE_VARIANT)

def vmc_validate_out_ports(args):
	num_outputs = args["num_outputs"]
	AIE_VARIANT = 1
	api = 1
	return fn_validate_num_outputs(api, num_outputs, AIE_VARIANT)

#### VMC graph generator ####
def vmc_generate_graph(name, args):
	tmpargs = {}
	tmpargs["TT_DATA"] = args["data_type"]
	use_coeff_reload = args["use_coeff_reload"]
	coef_type = args["coef_type"]
	coeff = args["coeff"]
	if use_coeff_reload:
		fir_length = args["fir_length"]
	else:
		if fn_is_complex(coef_type):
			fir_length = int(len(coeff)/2)
		else:
			fir_length = int(len(coeff))
	tmpargs["TT_COEF"] = coef_type
	tmpargs["TP_FIR_LEN"] = fir_length
	tmpargs["TP_DECIMATE_FACTOR"] = args["decimate_factor"]
	tmpargs["TP_SHIFT"] = args["shift_val"]
	tmpargs["TP_RND"] = args["rnd_mode"]
	tmpargs["TP_INPUT_WINDOW_VSIZE"] = args["input_window_size"]
	casc_length = args["casc_length"]
	tmpargs["TP_CASC_LEN"] = casc_length
	tmpargs["TP_USE_COEF_RELOAD"] = 1 if args["use_coeff_reload"] else 0
	tmpargs["TP_NUM_OUTPUTS"] = 2 if args["num_outputs"] else 1
	tmpargs["TP_DUAL_IP"] = 1 if args["dual_ip"] else 0
	tmpargs["TP_API"] = 1
	tmpargs["TP_SSR"] = args["ssr"]
	tmpargs["coeff"] = args["coeff"]
	tmpargs["TP_PARA_DECI_POLY"] = args["deci_poly"]
	   
	return generate_graph(name, tmpargs)
