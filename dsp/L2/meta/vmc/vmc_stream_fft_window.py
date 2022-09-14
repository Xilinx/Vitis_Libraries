from fft_window import *
import json

#### VMC validators ####
def vmc_validate_coeff_type(args):
  data_type = args["data_type"]
  coeff_type = args["coeff_type"]
  return fn_validate_coeff_type(data_type, coeff_type)

def vmc_validate_point_size(args):
  point_size = args["point_size"]
  return fn_validate_point_size(point_size)

def vmc_validate_shift_val(args):
  data_type = args["data_type"]
  shift_val = args["shift_val"]
  return fn_validate_shift(data_type, shift_val)

def vmc_validate_input_window_size(args):
  point_size = args["point_size"]
  input_window_size = args["input_window_size"]
  return fn_validate_window_vsize(point_size, input_window_size)

def vmc_validate_ssr(args):
  data_type = args["data_type"]
  point_size = args["point_size"]
  interface_type = 1
  ssr = args["ssr"]
  return fn_validate_ssr(data_type, point_size, interface_type, ssr)


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
  tmpargs["TP_DYN_PT_SIZE"] = 0
  tmpargs["weights"] = args["coeff"]
  return generate_graph(name, tmpargs)
