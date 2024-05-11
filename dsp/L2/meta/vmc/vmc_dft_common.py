from dft import *
from aie_common import *
import json

#### VMC validators ####
def vmc_validate_TP_RND(args):
  rnd_mode = args["rnd_mode"]
  AIE_VARIANT = args["AIE_VARIANT"]
  return fn_validate_round_val(rnd_mode, AIE_VARIANT)

def vmc_validate_sat_mode(args):
    sat_mode = args["sat_mode"]
    return fn_validate_satMode(sat_mode);

def vmc_validate_ssr_point_size(args):
    point_size = args["point_size"]
    data_type = args["data_type"]
    twiddle_type = args["twiddle_type"]
    casc_length = args["casc_length"]
    ssr = args["ssr"]
    AIE_VARIANT = args["AIE_VARIANT"]
    return fn_validate_point_size(AIE_VARIANT,point_size,data_type,twiddle_type,ssr,casc_length)

def vmc_validate_point_size(args):
    point_size = args["point_size"]
    data_type = args["data_type"]
    twiddle_type = args["twiddle_type"]
    casc_length = args["casc_length"]
    ssr = 1
    AIE_VARIANT = args["AIE_VARIANT"]
    return fn_validate_point_size(AIE_VARIANT,point_size,data_type,twiddle_type,ssr,casc_length)


def vmc_validate_num_frames(args):
    num_frames = args["num_frames"]
    return fn_validate_numFrames(num_frames)

def vmc_validate_ssr(args):
    ssr = args["ssr"]
    return fn_validate_ssr(ssr)

def vmc_validate_shift_val(args):
    data_type = args["data_type"]
    shift_val = args["shift_val"]
    return fn_validate_shift_val(data_type, shift_val)

def vmc_validate_casc_length(args):
    casc_length = args["casc_length"]
    return fn_validate_casc_len(casc_length)

def vmc_validate_twiddle_type(args):
    data_type = args["data_type"]
    twiddle_type = args["twiddle_type"]
    return fn_validate_twiddle_type(data_type, twiddle_type)

# Get twiddle types
k_twiddle_type = {"cfloat":"cfloat", "cint32":"cint16", "cint16":"cint16"}

def fn_get_twiddle_type(data_type):
	return k_twiddle_type[data_type]
