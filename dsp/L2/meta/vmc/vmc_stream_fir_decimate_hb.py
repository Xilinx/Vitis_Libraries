from fir_decimate_hb import *
from vmc_fir_utils import *

#### VMC validators ####
def vmc_validate_coef_type(args):
	data_type = args["data_type"]
	coef_type = args["coef_type"]
	standard_checks =  fn_validate_coef_type(data_type, coef_type)
	type_check = fn_type_support(data_type, coef_type)
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
	api = 1
	ssr = args["ssr"]
	deci_poly = args["deci_poly"]
	fir_length = fn_get_fir_length_hb(args)
	return fn_validate_input_window_size(data_type, coef_type, fir_length, input_window_size, api, ssr, deci_poly)

def vmc_validate_casc_length(args):
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
	ssr = args["ssr"]
	deci_poly = args["deci_poly"]
	api = 1        
	fir_length = fn_get_fir_length_hb(args)
	return fn_validate_fir_len(data_type, coef_type, fir_length, casc_length, ssr, api, use_coeff_reload, deci_poly)

def vmc_validate_shift_val(args):
	data_type = args["data_type"]
	shift_val = args["shift_val"]
	return fn_validate_shift(data_type, shift_val)

def vmc_validate_deci_poly(args):
        deci_poly = args["deci_poly"]
        ssr = args["ssr"]
        api = 1
        ret = fn_validate_para_deci_poly(api, deci_poly, ssr)
        if ret["is_valid"] == False:
          err_msg = ret["err_message"]
          err_msg = err_msg.replace("TP_PARA_DECI_POLY", "'Decimate polyphase'")
          return {"is_valid": False, "err_message": err_msg}
        else:
          return {"is_valid": True}

#### VMC graph generator ####
def vmc_generate_graph(name, args):
    tmpargs = {}
    tmpargs["TT_DATA"] = args["data_type"]
    tmpargs["TT_COEF"] = args["coef_type"]
    tmpargs["TP_FIR_LEN"] = args["fir_length"]
    tmpargs["TP_SHIFT"] = args["shift_val"]
    tmpargs["TP_RND"] = args["rnd_mode"]
    tmpargs["TP_INPUT_WINDOW_VSIZE"] = args["input_window_size"]
    casc_length = args["casc_length"]
    tmpargs["TP_CASC_LEN"] = casc_length
    tmpargs["TP_USE_COEF_RELOAD"] = 1 if args["use_coeff_reload"] else 0
    tmpargs["TP_NUM_OUTPUTS"] = 1
    tmpargs["TP_DUAL_IP"] = 0
    tmpargs["TP_API"] = 1
    tmpargs["TP_SSR"] = args["ssr"]
    tmpargs["coeff"] = args["coeff"]
   
    return generate_graph(name, tmpargs)
