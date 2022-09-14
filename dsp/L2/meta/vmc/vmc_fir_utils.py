from ctypes import sizeof
from aie_common import *
from aie_common_fir import *

# Utility functions

def fn_get_fir_length_hb(args):
	use_coeff_reload = args["use_coeff_reload"]
	coef_type = args["coef_type"]
	coeff = args["coeff"]
	if use_coeff_reload:
		fir_length = args["fir_length"]
	else:
		if fn_is_complex(coef_type):
			coeff_size = int(len(coeff)/2)
		else:
			coeff_size = int(len(coeff))
		fir_length = (coeff_size-1)*4-1
	return fir_length
