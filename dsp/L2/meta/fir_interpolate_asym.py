

from aie_common import *
from aie_common_fir import *
import json
import fir_sr_asym as sr_asym
import fir_polyphase_decomposer as poly

import importlib
from pathlib import Path
current_uut_kernel = Path(__file__).stem

#### naming ####
#
# Name functions with prefix
#   validate_ for validators, returning boolean result and error message as a tuple.
#   update_ for updators, returning object with default value and refined candidate constraints.
#   info_ for creating information based on parameters
#   fn_ for internal functions
#
# Name function arguments as template parameters, when possible
# so the code matches easier with API definition.


# Example of validator.
#
# The parameter itself will be passed as first argument for validator functions.
# These functions can have extra parameters as arguments, as specified as last part of in `validator`.
# These extra parameters must appear before current one in "parameters" section.
#
# A validator function returns a dictionary, with required boolean key "is_valid",
# and "err_message" if "is_valid" is False.
#

TP_INPUT_WINDOW_VSIZE_min = 4
TP_PARA_INTERP_POLY_min = 1
TP_CASC_LEN_min = 1
TP_CASC_LEN_max = 40
TP_FIR_LEN_min = 4
TP_FIR_LEN_max = 8192


def fn_fir_len_multiple_interp(TP_FIR_LEN, TP_INTERPOLATE_FACTOR):
  return isError(f"Filter length ({TP_FIR_LEN}) must be an integer multiple of interpolate factor ({TP_INTERPOLATE_FACTOR}).") if TP_FIR_LEN % TP_INTERPOLATE_FACTOR != 0 else isValid

def fn_check_samples_can_fit_streaming(TT_DATA, TT_COEFF, TP_FIR_LEN, TP_INTERPOLATE_FACTOR, TP_CASC_LEN, TP_DUAL_IP, TP_API, TP_SSR):
  m_kNumColumns = 2 if fn_size_by_byte(TT_COEFF) == 2 else 1

  m_kSamplesInBuff     = (1024//8)//fn_size_by_byte(TT_DATA)
  sizeOfA256Read = (256//8)//fn_size_by_byte(TT_DATA)

  sizeOfARead = sizeOfA256Read//2 if TP_DUAL_IP == 0 else sizeOfA256Read;
  firLenPerSsr = CEIL(TP_FIR_LEN, (TP_INTERPOLATE_FACTOR*TP_SSR))/TP_SSR
  if TP_API != 0:
    for kernelPos in range(TP_CASC_LEN):
      TP_FIR_RANGE_LEN =  (
        fnFirRangeRem(firLenPerSsr,TP_CASC_LEN,kernelPos,TP_INTERPOLATE_FACTOR)
          if (kernelPos == (TP_CASC_LEN-1))
          else
            fnFirRange(firLenPerSsr,TP_CASC_LEN,kernelPos,TP_INTERPOLATE_FACTOR)
      )
      numSamples = CEIL(TP_FIR_RANGE_LEN//TP_INTERPOLATE_FACTOR, m_kNumColumns) + sizeOfARead
      if numSamples > m_kSamplesInBuff :
        return isError(
            f"Requested parameters: FIR length ({TP_FIR_LEN}), interpolate factor ({TP_INTERPOLATE_FACTOR}), cascade length ({TP_CASC_LEN}) and SSR ({TP_SSR}) result in a kernel ({kernelPos}) that requires more data samples ({numSamples}) than capacity of a data buffer ({m_kSamplesInBuff}) "
            f"Please increase the cascade length ({TP_CASC_LEN}) and/or SSR ({TP_SSR})."
        )

  return isValid

# Values are derived from experimentation and are a factor of program memory limits, memory module sizes etc.
def fn_max_fir_len_overall(TT_DATA, TT_COEFF, TP_FIR_LEN):
  maxTaps = {
    ( "int16",  "int16") : 4096,
    ("cint16",  "int16") : 4096,
    ("cint16", "cint16") : 2048,
    ( "int32",  "int16") : 4096,
    ( "int32",  "int32") : 2048,
    ( "int16",  "int32") : 2048,
    ("cint16",  "int32") : 2048,
    ("cint16", "cint32") : 1024,
    ("cint32",  "int16") : 2048,
    ("cint32", "cint16") : 2048,
    ("cint32",  "int32") : 2048,
    ("cint32", "cint32") : 1024,
    ( "float",  "float") : 2048,
    ("cfloat",  "float") : 2048,
    ("cfloat", "cfloat") : 1024
  }
  return (
    isError(f"Max supported filter length (filter length = {TP_FIR_LEN} > Max = {maxTaps[(TT_DATA, TT_COEFF)]}) exceeded for data type and coefficient type combination {TT_DATA},{TT_COEFF}.")
      if TP_FIR_LEN > maxTaps[(TT_DATA, TT_COEFF)]
      else isValid
  )

def fn_validate_fir_len(TT_DATA, TT_COEFF, TP_FIR_LEN, TP_INTERPOLATE_FACTOR, TP_CASC_LEN, TP_SSR, TP_API, TP_USE_COEFF_RELOAD, TP_DUAL_IP, AIE_VARIANT):
    res = fn_validate_minmax_value("TP_FIR_LEN", TP_FIR_LEN, TP_FIR_LEN_min, TP_FIR_LEN_max)
    if (res["is_valid"] == False):
        return res
    minLenCheck =  fn_min_fir_len_each_kernel(TP_FIR_LEN, TP_CASC_LEN, TP_SSR, TP_Rnd=TP_INTERPOLATE_FACTOR)

    coeffSizeMult = 1
    if AIE_VARIANT == 1:
      coeffSizeMult = 1 if TP_API == 0 else TP_INTERPOLATE_FACTOR
    if AIE_VARIANT == 2:
      coeffSizeMult = TP_INTERPOLATE_FACTOR

    maxLenCheck = fn_max_fir_len_each_kernel(TT_DATA, TP_FIR_LEN, TP_CASC_LEN, TP_USE_COEFF_RELOAD, TP_SSR, TP_API, coeffSizeMult)

    maxLenOverallCheck = fn_max_fir_len_overall(TT_DATA, TT_COEFF, TP_FIR_LEN)

    multipleInterpolationRateCheck = fn_fir_len_multiple_interp(TP_FIR_LEN, TP_INTERPOLATE_FACTOR)

    streamingVectorRegisterCheck = fn_check_samples_can_fit_streaming(TT_DATA, TT_COEFF, TP_FIR_LEN, TP_INTERPOLATE_FACTOR, TP_CASC_LEN, TP_DUAL_IP, TP_API, TP_SSR)

    for check in (minLenCheck,maxLenCheck,maxLenOverallCheck, multipleInterpolationRateCheck, streamingVectorRegisterCheck):
      if check["is_valid"] == False :
        return check

    return isValid

def fn_validate_input_window_size(TT_DATA, TT_COEFF, TP_FIR_LEN,TP_INTERPOLATE_FACTOR, TP_INPUT_WINDOW_VSIZE, TP_API, TP_SSR=1):
    # CAUTION: this constant overlaps many factors. The main need is a "strobe" concept that means we unroll until xbuff is back to starting conditions.
    streamRptFactor = 4
    res = fn_validate_min_value("TP_INPUT_WINDOW_VSIZE", TP_INPUT_WINDOW_VSIZE, TP_INPUT_WINDOW_VSIZE_min)
    if (res["is_valid"] == False):
      return res
    # Need to take unrolloing into account
    windowSizeMultiplier = (fnNumLanes(TT_DATA, TT_COEFF, TP_API)) if TP_API == 0 else (fnNumLanes(TT_DATA, TT_COEFF, TP_API)*streamRptFactor)
    # interpolate asym uses common lanes, but doesn't use shorter acc for streaming arch.. why?
    checkMultipleLanes =  fn_windowsize_multiple_lanes(TT_DATA, TT_COEFF, TP_INPUT_WINDOW_VSIZE, TP_API, windowSizeMultiplier, TP_SSR)
    checkMaxBuffer = fn_max_windowsize_for_buffer(TT_DATA, TP_FIR_LEN, TP_INPUT_WINDOW_VSIZE, TP_API, TP_SSR, TP_INTERPOLATE_FACTOR)
    # Input samples are round-robin split to each SSR input paths, so total frame size must be divisable by SSR factor.
    checkIfDivisableBySSR = fn_windowsize_divisible_by_param(TP_INPUT_WINDOW_VSIZE, TP_SSR)

    for check in (checkMultipleLanes,checkMaxBuffer,checkIfDivisableBySSR):
      if check["is_valid"] == False :
        return check

    return isValid

def fn_validate_interp_dual_ip(TP_NUM_OUTPUTS, TP_API, TP_DUAL_IP, AIE_VARIANT=1):
    # Equivalent to:
    return fn_validate_sr_dual_ip(TP_NUM_OUTPUTS, TP_API, TP_DUAL_IP, AIE_VARIANT)

def validate_TP_INTERPOLATE_FACTOR(args):
    TP_INTERPOLATE_FACTOR = args["TP_INTERPOLATE_FACTOR"]
    AIE_VARIANT = args["AIE_VARIANT"]
    return fn_validate_interpolate_factor(TP_INTERPOLATE_FACTOR, AIE_VARIANT)

def fn_validate_para_interp_poly(TP_INTERPOLATE_FACTOR, TP_PARA_INTERP_POLY):
    res = fn_validate_min_value("TP_PARA_INTERP_POLY", TP_PARA_INTERP_POLY, TP_PARA_INTERP_POLY_min)
    if (res["is_valid"] == False):
      return res
    if TP_INTERPOLATE_FACTOR % TP_PARA_INTERP_POLY != 0:
        return isError(
            f"TP_PARA_INTERP_POLY must be a number that is exactly divisible by TP_INTERPOLATE_FACTOR. Got TP_PARA_INTERP_POLY {TP_PARA_INTERP_POLY} and TP_INTERPOLATE_FACTOR {TP_INTERPOLATE_FACTOR}."
        )
    return isValid

#### validation APIs ####
def validate_TT_COEFF(args):
    TT_DATA = args["TT_DATA"]
    TT_COEFF = args["TT_COEFF"]
    AIE_VARIANT = args["AIE_VARIANT"]
    standard_checks = fn_validate_coeff_type(TT_DATA, TT_COEFF)
    typeCheck = fn_type_support(TT_DATA, TT_COEFF, AIE_VARIANT)
    for check in (standard_checks,typeCheck):
      if check["is_valid"] == False :
        return check
    return isValid

def validate_TP_SHIFT(args):
  TT_DATA = args["TT_DATA"]
  TP_SHIFT = args["TP_SHIFT"]
  return fn_validate_shift(TT_DATA, TP_SHIFT)

def validate_TP_RND(args):
  TP_RND = args["TP_RND"]
  AIE_VARIANT = args["AIE_VARIANT"]
  return fn_validate_roundMode(TP_RND, AIE_VARIANT)

def validate_TP_SAT(args):
  TP_SAT = args["TP_SAT"]
  return fn_validate_satMode(TP_SAT)

def validate_TP_INPUT_WINDOW_VSIZE(args):

    check_valid_decompose = poly.fn_validate_decomposer_TP_INPUT_WINDOW_VSIZE(args)
    if (check_valid_decompose["is_valid"] == False):
      # error out before continuing to validate
      return check_valid_decompose

    # valid decompose
    #overwrite args with the decomposed version
    args, uut_kernel = poly.get_modified_args_from_polyphase_decomposer(args, current_uut_kernel)
    # if we've decomposed to another type of kernel, then import that kernel and use that validate function
    if uut_kernel != current_uut_kernel:
      other_kernel = importlib.import_module(uut_kernel)
      return other_kernel.validate_TP_INPUT_WINDOW_VSIZE(args)


    # continue using current library element's validator (with potentially modified parameters)
    TP_INPUT_WINDOW_VSIZE = args["TP_INPUT_WINDOW_VSIZE"]
    TT_DATA = args["TT_DATA"]
    TT_COEFF = args["TT_COEFF"]
    TP_FIR_LEN = args["TP_FIR_LEN"]
    TP_API = args["TP_API"]
    TP_INTERPOLATE_FACTOR = args["TP_INTERPOLATE_FACTOR"]
    TP_SSR = args["TP_SSR"]

    #interpolate_hb traits looks like the UPSHIFT_CT types have different number of lanes, but it's actually stil the exact same as 384..
    # decimate_hb also just uses 384, so no additional rules here.
    return fn_validate_input_window_size(TT_DATA, TT_COEFF, TP_FIR_LEN, TP_INTERPOLATE_FACTOR, TP_INPUT_WINDOW_VSIZE, TP_API, TP_SSR)



def validate_TP_FIR_LEN(args):

    check_valid_decompose = poly.fn_validate_decomposer_TP_FIR_LEN(args)
    if (check_valid_decompose["is_valid"] == False):
      # error out before continuing to validate
      return check_valid_decompose
    # valid decompose
    #overwrite args with the decomposed version
    args, uut_kernel = poly.get_modified_args_from_polyphase_decomposer(args, current_uut_kernel)
    # if we've decomposed to another type of kernel, then import that kernel and use that validate function
    if uut_kernel != current_uut_kernel:
      other_kernel = importlib.import_module(uut_kernel)
      return other_kernel.validate_TP_FIR_LEN(args)

    # continue using current library element's validator (with potentially modified parameters)
    TT_DATA = args["TT_DATA"]
    TT_COEFF = args["TT_COEFF"]
    TP_FIR_LEN = args["TP_FIR_LEN"]
    TP_CASC_LEN = args["TP_CASC_LEN"]
    TP_INTERPOLATE_FACTOR = args["TP_INTERPOLATE_FACTOR"]
    TP_SSR = args["TP_SSR"]
    TP_API = args["TP_API"]
    TP_USE_COEFF_RELOAD = args["TP_USE_COEFF_RELOAD"]
    TP_DUAL_IP = args["TP_DUAL_IP"]
    AIE_VARIANT = args["AIE_VARIANT"]

    return fn_validate_fir_len(TT_DATA, TT_COEFF, TP_FIR_LEN,TP_INTERPOLATE_FACTOR, TP_CASC_LEN, TP_SSR, TP_API, TP_USE_COEFF_RELOAD, TP_DUAL_IP, AIE_VARIANT)

def validate_TP_DUAL_IP(args):
    TP_NUM_OUTPUTS = args["TP_NUM_OUTPUTS"]
    TP_API = args["TP_API"]
    TP_DUAL_IP = args["TP_DUAL_IP"]
    AIE_VARIANT = args["AIE_VARIANT"]
    return fn_validate_interp_dual_ip(TP_NUM_OUTPUTS, TP_API, TP_DUAL_IP, AIE_VARIANT)

def validate_TP_NUM_OUTPUTS(args):
    TP_API = args["TP_API"]
    TP_NUM_OUTPUTS = args["TP_NUM_OUTPUTS"]
    AIE_VARIANT = args["AIE_VARIANT"]
    return fn_validate_num_outputs(TP_API, TP_NUM_OUTPUTS, AIE_VARIANT)

def fn_validate_interp_ssr(TP_SSR, TP_INTERPOLATE_FACTOR, TP_PARA_INTERP_POLY, TP_API, AIE_VARIANT):
  if AIE_VARIANT == 2:
    # AIE-ML doesn't allow SSR on a non-decomposed designs, as it is highly inefficient.
    if TP_SSR > 1 and TP_INTERPOLATE_FACTOR != TP_PARA_INTERP_POLY:
        return isError(f"SSR mode not available when design is not fully decomposed. Please set TP_PARA_INTERP_POLY ({TP_PARA_INTERP_POLY}) to match TP_INTERPOLATE_FACTOR {TP_INTERPOLATE_FACTOR} before increasing TP_SSR {TP_SSR}.")
  return fn_stream_only_ssr(TP_API, TP_SSR)

def fn_validate_casc_len(TP_CASC_LEN):
    return fn_validate_minmax_value("TP_CASC_LEN", TP_CASC_LEN, TP_CASC_LEN_min, TP_CASC_LEN_max)

def validate_TP_CASC_LEN(args):
    TP_CASC_LEN = args["TP_CASC_LEN"]
    return fn_validate_casc_len(TP_CASC_LEN)

def validate_TP_PARA_INTERP_POLY(args):
    TP_PARA_INTERP_POLY = args["TP_PARA_INTERP_POLY"]
    TP_INTERPOLATE_FACTOR = args["TP_INTERPOLATE_FACTOR"]
    return fn_validate_para_interp_poly(TP_INTERPOLATE_FACTOR, TP_PARA_INTERP_POLY)

def validate_TP_SSR(args):
    TP_INTERPOLATE_FACTOR = args["TP_INTERPOLATE_FACTOR"]
    TP_PARA_INTERP_POLY = args["TP_PARA_INTERP_POLY"]
    TP_API = args["TP_API"]
    TP_SSR = args["TP_SSR"]
    AIE_VARIANT = args["AIE_VARIANT"]
    kernelInterpolate = TP_INTERPOLATE_FACTOR//TP_PARA_INTERP_POLY
    return (
      fn_validate_interp_ssr(TP_SSR, TP_INTERPOLATE_FACTOR, TP_PARA_INTERP_POLY, TP_API, AIE_VARIANT) if kernelInterpolate > 1
      else isValid # assume single rate SSR is valid, since it doesn't have a validation function
    )

# Example of updater.
#
# Updater are functions to help GUI to hint user on parameter setting with already given parameters.
# The return object will provide "value" which will be set in the wizard as the dependent parameter is being set.
# The rest of keys are similar to parameter definition, but with candidates of enum or range values refined
# based on previously set values.
#
# An updator function always return a dictionary,
# including key "value" for automatically filled default in GUI as dependent parameters have been set, and
# other keys for overriding the definition of parameter.
#
# For example, if a parameter has definition in JSON as
#  { "name": "foo", "type": "typename", "enum": ["int", "float", "double"] }
# And the updator returns
#  { "value": "int", "enum": ["int", "float"] }
# The GUI would show "int" as default and make "int" and "float" selectable candidates, while disabling "double".
#
# If with given combination, no valid value can be set for the parameter being updated, the upater function
# should set "value" to None, to indicate an error and provide error message via "err_message".
# For example
#  { "value": None, "err_message": "With TT_DATA as 'int' there is no valid option for TT_COEFF" }
#
# In this example, the following is the updater for TT_COEFF, with TT_DATA as the dependent parameter.
# When GUI generates a wizard, TT_DATA should be required first, as it shows up in parameter list first.
# Once user has provided value for TT_DATA, this function will be called and set the value of TT_COEFF.
# Meanwhile, the candidate shown in wizard based on enum will also be updated.
#
def update_TT_COEFF(TT_DATA):
    return {"value": TT_DATA,
            "enum": [TT_DATA]}

#### port ####


def info_ports(args):
    """Standard function creating a static dictionary of information
    for upper software to correctly connect the IP.
    Some IP has dynamic number of ports according to parameter set,
    so port information has to be implemented as a function"""
    TT_DATA = args["TT_DATA"]
    TT_COEFF = args["TT_COEFF"]
    TP_INPUT_WINDOW_VSIZE = args["TP_INPUT_WINDOW_VSIZE"]
    TP_FIR_LEN = args["TP_FIR_LEN"]
    TP_SSR = args["TP_SSR"]
    TP_INTERPOLATE_FACTOR = args["TP_INTERPOLATE_FACTOR"]
    TP_API = args["TP_API"]
    TP_DUAL_IP = args["TP_DUAL_IP"]
    TP_NUM_OUTPUTS = args["TP_NUM_OUTPUTS"]
    TP_PARA_INTERP_POLY = (args["TP_PARA_INTERP_POLY"] if ("TP_PARA_INTERP_POLY" in args)  else 1)
    TP_DECIMATE_FACTOR = 1
    margin_size = sr_asym.fn_margin_size(TP_FIR_LEN//TP_INTERPOLATE_FACTOR, TT_DATA)

    num_in_ports = TP_SSR # *TP_PARA_DECI_POLY (not in the internpolator)
    num_out_ports = TP_SSR*TP_PARA_INTERP_POLY

    in_win_size = get_input_window_size(TP_INPUT_WINDOW_VSIZE, num_in_ports, TP_API, TP_DUAL_IP)
    out_win_size = get_output_window_size(TP_INPUT_WINDOW_VSIZE, num_out_ports, TP_API, TP_NUM_OUTPUTS, TP_DECIMATE_FACTOR, TP_INTERPOLATE_FACTOR)

    in_ports = get_port_info("in", "in", TT_DATA, in_win_size, num_in_ports, marginSize=margin_size, TP_API=TP_API)
    in2_ports = (get_port_info("in2", "in", TT_DATA, in_win_size, num_in_ports, marginSize=margin_size, TP_API=TP_API) if (TP_DUAL_IP == 1) else [])
    coeff_ports = (get_parameter_port_info("coeff", "in", TT_COEFF, TP_SSR, TP_FIR_LEN, "async") if (args["TP_USE_COEFF_RELOAD"] == 1) else [])

    # decimate by 2 for halfband
    out_ports = get_port_info("out", "out", TT_DATA, out_win_size, num_out_ports, TP_API=args["TP_API"])
    out2_ports = (get_port_info("out2", "out", TT_DATA, out_win_size, num_out_ports, TP_API=args["TP_API"]) if (args["TP_NUM_OUTPUTS"] == 2) else [])
    return in_ports + in2_ports + coeff_ports + out_ports + out2_ports


#### graph generator ####
# def get_param_list(**kwargs):
#   [f"{value}{comma}} //{key}" for key, value in kwargs.iteritems() for comma in "," ]
def generate_graph(graphname, args):

  if graphname == "":
    graphname = "default_graphname"

  TT_COEFF = args["TT_COEFF"]
  TT_DATA = args["TT_DATA"]
  TP_FIR_LEN = args["TP_FIR_LEN"]
  TP_SHIFT = args["TP_SHIFT"]
  TP_RND = args["TP_RND"]
  TP_CASC_LEN = args["TP_CASC_LEN"]
  TP_INPUT_WINDOW_VSIZE = args["TP_INPUT_WINDOW_VSIZE"]
  TP_INTERPOLATE_FACTOR = args["TP_INTERPOLATE_FACTOR"]
  TP_USE_COEFF_RELOAD = args["TP_USE_COEFF_RELOAD"]
  TP_NUM_OUTPUTS = args["TP_NUM_OUTPUTS"]
  TP_DUAL_IP = args["TP_DUAL_IP"]
  TP_API = args["TP_API"]
  TP_PARA_INTERP_POLY = (args["TP_PARA_INTERP_POLY"] if ("TP_PARA_INTERP_POLY" in args)  else 1)
  TP_SSR = args["TP_SSR"]
  coeff_list = args["coeff"]
  TP_SAT = args["TP_SAT"]

  taps = sr_asym.fn_get_taps_vector(TT_COEFF, coeff_list)
  constr_args_str = f"taps" if TP_USE_COEFF_RELOAD == 0 else ""
  dual_ip_declare_str = f" std::array<adf::port<input>, TP_SSR> in2;" if TP_DUAL_IP == 1 else "// No dual input"
  dual_ip_connect_str = f"adf::connect<> net_in2(in2[ssrIdx], filter.in2[ssrIdx]);" if TP_DUAL_IP == 1 else "// No dual input"
  coeff_ip_declare_str = f"std::array<adf::port<input>, TP_SSR*TP_PARA_INTERP_POLY> coeff;" if TP_USE_COEFF_RELOAD == 1 else "//No coeff port"
  coeff_ip_connect_str = f"adf::connect<> net_coeff(coeff[outPortIdx], filter.coeff[outPortIdx]);" if TP_USE_COEFF_RELOAD == 1 else "//No coeff port"

  dual_op_declare_str = f"std::array<adf::port<output>, TP_SSR*TP_PARA_INTERP_POLY> out2;" if TP_NUM_OUTPUTS == 2 else "// No dual output"
  dual_op_connect_str = f"adf::connect<> net_out2(filter.out2[outPortIdx], out2[outPortIdx]);" if TP_NUM_OUTPUTS == 2 else "// No dual output"
  # Use formatted multi-line string to avoid a lot of \n and \t
  code  = (
f"""
class {graphname} : public adf::graph {{
public:
  static constexpr unsigned int TP_SSR = {TP_SSR};
  static constexpr unsigned int TP_PARA_INTERP_POLY = {TP_PARA_INTERP_POLY};

  std::array<adf::port<input>, TP_SSR> in;
  {dual_ip_declare_str}
  {coeff_ip_declare_str}
  std::array<adf::port<output>, TP_SSR*TP_PARA_INTERP_POLY> out;
  {dual_op_declare_str}

  std::vector<{TT_COEFF}> taps = {taps};
  xf::dsp::aie::fir::interpolate_asym::fir_interpolate_asym_graph<
    {TT_DATA}, //TT_DATA
    {TT_COEFF}, //TT_COEFF
    {TP_FIR_LEN}, //TP_FIR_LEN
    {TP_INTERPOLATE_FACTOR}, //TP_INTERPOLATE_FACTOR
    {TP_SHIFT}, //TP_SHIFT
    {TP_RND}, //TP_RND
    {TP_INPUT_WINDOW_VSIZE}, //TP_INPUT_WINDOW_VSIZE
    {TP_CASC_LEN}, //TP_CASC_LEN
    {TP_USE_COEFF_RELOAD}, //TP_USE_COEFF_RELOAD
    {TP_DUAL_IP}, //TP_DUAL_IP
    {TP_NUM_OUTPUTS}, //TP_NUM_OUTPUTS
    {TP_API}, //TP_API
    {TP_SSR}, //TP_SSR
    {TP_PARA_INTERP_POLY}, //TP_PARA_INTERP_POLY
    {TP_SAT} //TP_SAT
  > filter;

  {graphname}() : filter({constr_args_str}) {{
    adf::kernel *filter_kernels = filter.getKernels();
    for (int i=0; i < 1; i++) {{
      adf::runtime<ratio>(filter_kernels[i]) = 0.9;
    }}
    for (int ssrIdx=0; ssrIdx < TP_SSR; ssrIdx++) {{
      adf::connect<> net_in(in[ssrIdx], filter.in[ssrIdx]);
      {dual_ip_connect_str}
    }}

    for (int paraPolyIdx=0; paraPolyIdx < TP_PARA_INTERP_POLY; paraPolyIdx++) {{
      for (int ssrIdx=0; ssrIdx < TP_SSR; ssrIdx++) {{
        unsigned outPortIdx = paraPolyIdx+ssrIdx*TP_PARA_INTERP_POLY;
        adf::connect<> net_out(filter.out[outPortIdx], out[outPortIdx]);
      {dual_op_connect_str}
      {coeff_ip_connect_str}
      }}
    }}
  }}

}};
""")
  out = {}
  out["graph"] = code
  out["port_info"] = info_ports(args)
  out["headerfile"] = "fir_interpolate_asym_graph.hpp"
  out["searchpaths"] = [
       "L2/include/aie",
       "L2/tests/aie/common/inc",
       "L1/include/aie",
       "L1/src/aie",
       "L1/tests/aie/inc",
       "L1/tests/aie/src"
  ]

  return out


