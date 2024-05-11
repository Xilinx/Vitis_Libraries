from fft_window import *
from aie_common import *
import json

#### VMC validators ####

# Note: For stream-based Window Fn, the low level requires SSR as
# number of kernels and SSR*2 is the number of input or output
# ports. But for VMC users, SSR is the number of input or output
# ports. Hence, we divide it by 2 here before passing on to low level
# IP.

ssr_min = 1
ssr_max = 32

def vmc_fn_get_ssr(args):
  ssr = args["ssr"]
  if ssr % 2 == 0:
    ssr = ssr
  else:
    ssr = -1

  return ssr

def vmc_validate_coeff_type(args):
  data_type = args["data_type"]
  coeff_type = args["coeff_type"]
  return fn_validate_coeff_type(data_type, coeff_type)

def vmc_validate_point_size(args):
  point_size = args["point_size"]
  data_type = args["data_type"]
  return fn_validate_point_size(point_size, data_type)

def vmc_validate_shift_val(args):
  data_type = args["data_type"]
  shift_val = args["shift_val"]
  return fn_validate_shift_val(data_type, shift_val)

def vmc_validate_input_window_size(args):
  point_size = args["point_size"]
  input_window_size = args["input_window_size"]
  return fn_validate_window_vsize(point_size, input_window_size)

def vmc_validate_ssr(args):
  data_type = args["data_type"]
  point_size = args["point_size"]
  interface_type = 1
  ssr = vmc_fn_get_ssr(args)
  if ssr == -1:
    return isError(f"Invalid SSR value specified. The value must be an even number.")

  return fn_validate_ssrvalue(data_type, point_size, interface_type, ssr)

def fn_validate_ssrvalue(data_type, point_size, interface_type, ssrValue):
  ssr = ssrValue//2
  res = fn_validate_minmax_value("ssr", ssr, ssr_min, ssr_max)
  if (res["is_valid"] == False):  
    return res
  if (point_size/ssr >=16 and point_size/ssr<=4096) :
    if (point_size/ssr<=1024 or interface_type==1) :
      return isValid
    else:
      return isError(f"(Point size/(SSR/2)) must be less than 1024 for windowed configurations. Got point_size={point_size} and ssr={ssrValue}.")
  else:
    return isError(f"(Point size/(SSR/2)) must be between 16 and 4096. Got point_size={point_size} and ssr={ssrValue}.")
  
def vmc_validate_is_dyn_pt_size(args):
  point_size = args["point_size"]
  ssr = vmc_fn_get_ssr(args)
  if ssr == -1:
    return isError(f"Invalid SSR value specified. The value must be an even number.")

  dyn_pt = 1 if args["is_dyn_pt_size"] else 0
  return fn_validate_dyn_pt_sizevalue(point_size, ssr, dyn_pt)

def fn_validate_dyn_pt_sizevalue(point_size, ssrValue, dyn_pt):
  ssr = ssrValue//2
  if (dyn_pt==0 or point_size/ssr >32) :
    return isValid
  else:
    return isError(f"When dynamic point FFT is selected, (Point size/(SSR/2)) must be greater than 32. Got point_size={point_size} and ssr={ssrValue}.")


def vmc_validate_coeff(args):
  dyn_pt = 1 if args["is_dyn_pt_size"] else 0
  return fn_validate_weights(args["point_size"], dyn_pt, args["coeff"])

#### VMC graph generator ####
def vmc_generate_graph(name, args):
  tmpargs = {}
  tmpargs["TT_DATA"] = args["data_type"]
  tmpargs["TT_COEFF"] = args["coeff_type"]
  tmpargs["TP_POINT_SIZE"] = args["point_size"]
  tmpargs["TP_WINDOW_VSIZE"] = args["input_window_size"]
  tmpargs["TP_SHIFT"] = args["shift_val"]
  tmpargs["TP_API"] = 1
  tmpargs["TP_SSR"] = args["ssr"]
  tmpargs["TP_DYN_PT_SIZE"] = 1 if args["is_dyn_pt_size"] else 0
  tmpargs["AIE_VARIANT"] = args["AIE_VARIANT"]
  tmpargs["weights"] = args["coeff"]
  return generate_graph(name, tmpargs)
