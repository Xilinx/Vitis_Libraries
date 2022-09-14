from fir_sr_asym import *

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
	ssr = 1
	if use_coeff_reload:
		fir_length = args["fir_length"]
	else:
		if fn_is_complex(coef_type):
			fir_length = int(len(coeff)/2)
		else:
			fir_length = int(len(coeff))
	return fn_validate_input_window_size(data_type, coef_type, fir_length, input_window_size, api, ssr)

def vmc_validate_casc_length(args):
    use_casc_length = args["use_casc_length"]
    use_coeff_reload = args["use_coeff_reload"]
    fir_length = args["fir_length"]
    casc_length = args["casc_length"]
    #if not use_casc_length:
	# TODO : Talk to DSP lib team/sumanta about how 
	# cascade validation works - confirm its just fir length related
	#return fn_validate_casc_length(fir_length, casc_length, use_coeff_reload)
    return {"is_valid": True}
    

def vmc_validate_coeff(args):
	use_coeff_reload = args["use_coeff_reload"]
	coef_type = args["coef_type"]
	coeff = args["coeff"]
	data_type = args["data_type"]
	casc_length = args["casc_length"]
	use_casc_length = args["use_casc_length"]
	ssr = 1
	api = 0
	if use_coeff_reload:
		fir_length = args["fir_length"]
	else:
		if fn_is_complex(coef_type):
			fir_length = int(len(coeff)/2)
		else:
			fir_length = int(len(coeff))
	#TODO: Talk to DSP Lib team about separating casc length from fir_length API
	if use_casc_length:
		return fn_validate_fir_len(data_type, coef_type, fir_length, casc_length, ssr, api, use_coeff_reload)
	return {"is_valid": True}

def vmc_validate_shift_val(args):
	data_type = args["data_type"]
	shift_val = args["shift_val"]
	return fn_validate_shift(data_type, shift_val)

#### VMC graph generator ####
def vmc_generate_graph(name, args):
    tmpargs = {}
    tmpargs["TT_DATA"] = args["data_type"]
    tmpargs["TT_COEF"] = args["coef_type"]
    tmpargs["TP_FIR_LEN"] = args["fir_length"]
    tmpargs["TP_SHIFT"] = args["shift_val"]
    tmpargs["TP_RND"] = args["rnd_mode"]
    tmpargs["TP_INPUT_WINDOW_VSIZE"] = args["input_window_size"]
    use_casc_length = args["use_casc_length"]
    if use_casc_length:
      casc_length = args["casc_length"]
    else:
      #TODO: call to partitioner to determine cascade length
      casc_length = 1

    tmpargs["TP_CASC_LEN"] = casc_length
    tmpargs["TP_USE_COEF_RELOAD"] = 1 if args["use_coeff_reload"] else 0
    tmpargs["TP_NUM_OUTPUTS"] = 1
    tmpargs["TP_DUAL_IP"] = 0
    tmpargs["TP_API"] = 0
    tmpargs["TP_SSR"] = 1
    tmpargs["coeff"] = args["coeff"]
   
    return generate_graph(name, tmpargs)
