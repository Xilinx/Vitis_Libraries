import aie_common as com
import aie_common_fir as fir
from aie_common import isError, isValid

def fn_mod_args_with_defaults(args):
  ret_args = dict(args)
  for key in ["TP_PARA_INTERP_POLY", "TP_PARA_DECI_POLY", "TP_DECIMATE_FACTOR", "TP_INTERPOLATE_FACTOR", "TP_INPUT_WINDOW_VSIZE", "TP_FIR_LEN"]: 
    if key not in ret_args: 
      ret_args[key] = 1
  return ret_args

def fn_determine_decomposed_uut_kernel(args, uut_kernel): 
  # avoid having to constantly check for parameters if exist
  if ( args["TP_PARA_INTERP_POLY"] == 1 and args["TP_PARA_DECI_POLY"] == 1): 
    # no decomposing
    return uut_kernel
  
  interp_is_fully_decomposed = (args["TP_INTERPOLATE_FACTOR"] // args["TP_PARA_INTERP_POLY"] == 1)
  deci_is_fully_decomposed = (args["TP_DECIMATE_FACTOR"] // args["TP_PARA_DECI_POLY"] == 1)
  if interp_is_fully_decomposed and deci_is_fully_decomposed: 
    return "fir_sr_asym"
  elif interp_is_fully_decomposed and not deci_is_fully_decomposed: 
    return "fir_decimate_asym"
  elif not interp_is_fully_decomposed and deci_is_fully_decomposed: 
    return "fir_interpolate_asym"
  else:
    return "fir_resampler"

def get_modified_args_from_polyphase_decomposer(args, uut_kernel):
  mod_args = dict(args) # explicit copy, not reference
  if (
    "TP_PARA_INTERP_POLY" not in args and 
    "TP_PARA_DECI_POLY" not in args 
  ):
    return args, uut_kernel
  else: 
    
    # remove para poly from modified dict (avoid infinite recursion)
    if "TP_PARA_INTERP_POLY" in mod_args :
      del mod_args["TP_PARA_INTERP_POLY"] 
    if "TP_PARA_DECI_POLY" in mod_args:
      del mod_args["TP_PARA_DECI_POLY"]

    # We're in decomposer territory
    args_with_default = fn_mod_args_with_defaults(args)
    if args_with_default["TP_PARA_INTERP_POLY"] == 1 and args_with_default["TP_PARA_DECI_POLY"] == 1: 
      # no decomposing to be done, no further modification
      return mod_args, uut_kernel


    mod_args["TP_FIR_LEN"] = args_with_default["TP_FIR_LEN"] // (args_with_default["TP_PARA_INTERP_POLY"] * args_with_default["TP_PARA_DECI_POLY"])
    mod_args["TP_INPUT_WINDOW_VSIZE"] = args_with_default["TP_INPUT_WINDOW_VSIZE"] // (args_with_default["TP_PARA_DECI_POLY"])

    mod_args["TP_DECIMATE_FACTOR"] = args_with_default["TP_DECIMATE_FACTOR"] // args_with_default["TP_PARA_DECI_POLY"]
    mod_args["TP_INTERPOLATE_FACTOR"] = args_with_default["TP_INTERPOLATE_FACTOR"] // args_with_default["TP_PARA_INTERP_POLY"]
    
    # If it's fully decomposed, then the new uut won't be expecting this parameter in it's args dictionary
    if mod_args["TP_DECIMATE_FACTOR"] == 1: 
      del mod_args["TP_DECIMATE_FACTOR"]
    if mod_args["TP_INTERPOLATE_FACTOR"] == 1: 
      del mod_args["TP_INTERPOLATE_FACTOR"]
    

    ret_uut_kernel = fn_determine_decomposed_uut_kernel(args_with_default, uut_kernel)
    return mod_args, ret_uut_kernel

def fn_validate_decomposer_TP_FIR_LEN(args): 
  #default_args = dargs
  dargs = fn_mod_args_with_defaults(args)
  TP_FIR_LEN = dargs["TP_FIR_LEN"]
  TP_PARA_INTERP_POLY = dargs["TP_PARA_INTERP_POLY"]
  TP_PARA_DECI_POLY = dargs["TP_PARA_DECI_POLY"]
  if TP_FIR_LEN % TP_PARA_INTERP_POLY != 0: 
    return isError(
      f"Filter length ({ TP_FIR_LEN }) must be a mutliple of TP_PARA_INTERP_POLY ({ TP_PARA_INTERP_POLY })"
    )
  if TP_FIR_LEN % TP_PARA_DECI_POLY != 0: 
    return isError(
      f"Filter length ({ TP_FIR_LEN }) must be a mutliple of TP_PARA_DECI_POLY ({ TP_PARA_DECI_POLY })"
    )
  
  return isValid


def fn_validate_decomposer_TP_INPUT_WINDOW_VSIZE(args): 
  #default_args = dargs
  dargs = fn_mod_args_with_defaults(args)
  TP_INPUT_WINDOW_VSIZE = dargs["TP_INPUT_WINDOW_VSIZE"]
  TP_PARA_DECI_POLY = dargs["TP_PARA_DECI_POLY"]
  if TP_INPUT_WINDOW_VSIZE % TP_PARA_DECI_POLY != 0: 
    return isError(f"Input window size ({TP_INPUT_WINDOW_VSIZE}) must be a mutliple of TP_PARA_DECI_POLY ({TP_PARA_DECI_POLY})")
  
  return isValid

def fn_validate_interp_poly(TP_PARA_INTERP_POLY, TP_INTERPOLATE_FACTOR):
  return (
    isError(f"TP_PARA_INTERP_POLY ({TP_PARA_INTERP_POLY}) must be an integer factor (greater than 0) of TP_INTERPOLATE_FACTOR ({TP_INTERPOLATE_FACTOR}) ")
      if ((TP_INTERPOLATE_FACTOR % TP_PARA_INTERP_POLY != 0) and (TP_INTERPOLATE_FACTOR//TP_PARA_INTERP_POLY >= 1)) 
    else isValid
  )

def validate_TP_PARA_INTERP_POLY(args): 
  TP_PARA_INTERP_POLY = args["TP_PARA_INTERP_POLY"]
  TP_INTERPOLATE_FACTOR = args["TP_INTERPOLATE_FACTOR"]
  return fn_validate_interp_poly(TP_PARA_INTERP_POLY, TP_INTERPOLATE_FACTOR)

def validate_TP_PARA_DECI_POLY(args): 
  TP_PARA_DECI_POLY = args["TP_PARA_DECI_POLY"]
  TP_DECIMATE_FACTOR = args["TP_DECIMATE_FACTOR"]
  return (
    isError(f"TP_PARA_DECI_POLY ({TP_PARA_DECI_POLY}) must be an integer factor (greater than 0) of TP_DECIMATE_FACTOR ({TP_DECIMATE_FACTOR}) ")
      if ((TP_DECIMATE_FACTOR % TP_PARA_DECI_POLY != 0) and (TP_DECIMATE_FACTOR//TP_PARA_DECI_POLY >= 1)) 
    else isValid
  )
