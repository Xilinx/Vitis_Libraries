

from aie_common import *
from aie_common_fir import *
import json
import fir_sr_asym as sr_asym
import fir_polyphase_decomposer as poly

import importlib
from pathlib import Path
current_uut_kernel = Path(__file__).stem
# fir_interpolate_asym.hpp:         static_assert(TP_FIR_LEN <= fnMaxTapssIntAsym<TT_DATA,TT_COEFF>(),"ERROR: Max supported FIR length exceeded for TT_DATA/TT_COEFF combination. ");
# fir_interpolate_asym.hpp:         static_assert(TP_FIR_RANGE_LEN >= FIR_LEN_MIN,"ERROR: Illegal combination of design FIR length and cascade length, resulting in kernel FIR length below minimum required value. ");
# fir_interpolate_asym.hpp:         static_assert(TP_SHIFT >= SHIFT_MIN && TP_SHIFT <= SHIFT_MAX, "ERROR: SHIFT is out of the supported range.");
# fir_interpolate_asym.hpp:         static_assert(TP_RND >= ROUND_MIN && TP_RND <= ROUND_MAX, "ERROR: RND is out of the supported range.");
# fir_interpolate_asym.hpp:         static_assert((TP_FIR_LEN % TP_INTERPOLATE_FACTOR) == 0,"ERROR: TP_FIR_LEN must be an integer multiple of INTERPOLATE_FACTOR.");
# fir_interpolate_asym.hpp:         static_assert(fnEnumType<TT_DATA>() != enumUnknownType,"ERROR: TT_DATA is not a supported type.");
# fir_interpolate_asym.hpp:         static_assert(fnEnumType<TT_COEFF>() != enumUnknownType,"ERROR: TT_COEFF is not a supported type.");
# fir_interpolate_asym.hpp:         static_assert(fnTypeCheckDataCoeffSize<TT_DATA,TT_COEFF>() != 0, "ERROR: TT_DATA type less precise than TT_COEFF is not supported.");
# fir_interpolate_asym.hpp:         static_assert(fnTypeCheckDataCoeffCmplx<TT_DATA,TT_COEFF>() != 0, "ERROR: real TT_DATA with complex TT_COEFF is not supported.");
# fir_interpolate_asym.hpp:         static_assert(fnTypeCheckDataCoeffFltInt<TT_DATA,TT_COEFF>() != 0, "ERROR: a mix of float and integer types of TT_DATA and TT_COEFF is not supported.");
# fir_interpolate_asym.hpp:         static_assert(TP_INTERPOLATE_FACTOR >= INTERPOLATE_FACTOR_MIN && TP_INTERPOLATE_FACTOR <= INTERPOLATE_FACTOR_MAX,"ERROR: TP_INTERPOLATE_FACTOR is out of the supported range");
# fir_interpolate_asym.hpp:         static_assert(fnUnsupportedTypeCombo<TT_DATA,TT_COEFF>() != 0,"ERROR: The combination of TT_DATA and TT_COEFF is not supported for this class.");
# fir_interpolate_asym.hpp:         static_assert(TP_NUM_OUTPUTS >0 && TP_NUM_OUTPUTS <=2, "ERROR: only single or dual outputs are supported." );
# fir_interpolate_asym.hpp:         static_assert(!(std::is_same<TT_DATA,cfloat>::value || std::is_same<TT_DATA,float>::value) || (TP_SHIFT == 0), "ERROR: TP_SHIFT cannot be performed for TT_DATA=cfloat, so must be set to 0");
# fir_interpolate_asym.hpp:         static_assert(TP_INPUT_WINDOW_VSIZE % m_kLanes == 0, "ERROR: TP_INPUT_WINDOW_VSIZE must be an integer multiple of the number of lanes for this data type");
# fir_interpolate_asym.hpp:         static_assert(TP_API==USE_WINDOW_API || m_kNumOps * m_kColumns <= m_kSpaces, "ERROR: FIR_LENGTH is too large for the number of kernels with Stream API, increase TP_CASC_LEN");
# fir_interpolate_asym.hpp:         static_assert(TP_DUAL_IP==0 || TP_API==USE_STREAM_API, "Error: Dual input feature is only supported for stream API");
# fir_interpolate_asym_graph.hpp:   static_assert(TP_CASC_LEN <= 40,"ERROR: Unsupported Cascade length");
# fir_interpolate_asym_graph.hpp:   static_assert(TP_API == USE_STREAM_API || TP_DUAL_IP == DUAL_IP_SINGLE,"ERROR: Dual input ports only supported when port API is a stream. ");
# fir_interpolate_asym_graph.hpp:   static_assert(TP_FIR_LEN / TP_CASC_LEN <= kMaxTapsPerKernel,"ERROR: Requested FIR length and Cascade length exceeds supported number of taps per kernel. Please increase the cascade legnth to accomodate the FIR design.");
# fir_interpolate_asym_graph.hpp:   static_assert(TP_USE_COEFF_RELOAD == 0 || TP_FIR_LEN <= kMaxTapsPerKernel,"ERROR: Exceeded maximum supported FIR length with reloadable coefficients. Please limit the FIR length or disable coefficient reload.");
# fir_interpolate_asym_graph.hpp:   static_assert(TP_API != 0 || inBufferSize < kMemoryModuleSize, "ERROR: Input Window size (based on requrested window size and FIR length margin) exceeds Memory Module size of 32kB");
# fir_interpolate_asym_graph.hpp:   static_assert(TP_API != 0 || outBufferSize < kMemoryModuleSize, "ERROR: Input Window size (based on requrested window size and FIR length margin) exceeds Memory Module size of 32kB");

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

def fn_fir_len_multiple_interp(TP_FIR_LEN, TP_INTERPOLATE_FACTOR):
  return isError(f"Filter length ({TP_FIR_LEN}) must be an integer multiple of interpolate factor ({TP_INTERPOLATE_FACTOR}).") if TP_FIR_LEN % TP_INTERPOLATE_FACTOR != 0 else isValid

def fn_check_samples_can_fit_streaming(TT_DATA, TT_COEF, TP_FIR_LEN, TP_INTERPOLATE_FACTOR, TP_CASC_LEN, TP_DUAL_IP, TP_API, TP_SSR):
  m_kNumColumns = 2 if fn_size_by_byte(TT_COEF) == 2 else 1

  m_kSamplesInBuff     = (1024//8)//fn_size_by_byte(TT_DATA)
  sizeOfA256Read = (256//8)//fn_size_by_byte(TT_DATA)

  sizeOfARead = sizeOfA256Read//2 if TP_DUAL_IP == 0 else sizeOfA256Read;
  m_kSpaces = m_kSamplesInBuff - sizeOfARead; # duplicating conservative spaces.
  firLenPerSsr = CEIL(TP_FIR_LEN, (TP_INTERPOLATE_FACTOR*TP_SSR))/TP_SSR
  if TP_API != 0:
    for kernelPos in range(TP_CASC_LEN):
      TP_FIR_RANGE_LEN =  (
        fnFirRangeRem(firLenPerSsr,TP_CASC_LEN,kernelPos,TP_INTERPOLATE_FACTOR)
          if (kernelPos == (TP_CASC_LEN-1))
          else
            fnFirRange(firLenPerSsr,TP_CASC_LEN,kernelPos,TP_INTERPOLATE_FACTOR)
      )
      numSamples = CEIL(TP_FIR_RANGE_LEN//TP_INTERPOLATE_FACTOR, m_kNumColumns)
      if numSamples > m_kSpaces :
        return isError(f"kernel[{kernelPos}] requires too much data ({numSamples} samples) to fit in a single buffer ({m_kSpaces} samples), due to the filter length per kernel- influenced by filter length ({TP_FIR_LEN}), interpolate factor ({TP_INTERPOLATE_FACTOR}) and cascade length ({TP_CASC_LEN})")

  return isValid

# Values are derived from experimentation and are a factor of program memory limits, memory module sizes etc.
def fn_max_fir_len_overall(TT_DATA, TT_COEF, TP_FIR_LEN):
  maxTaps = {
    ("cint16",  "int16") : 4096,
    ("cint16", "cint16") : 2048,
    ( "int32",  "int16") : 4096,
    ( "int32",  "int32") : 2048,
    ("cint32",  "int16") : 2048,
    ("cint32", "cint16") : 2048,
    ("cint32",  "int32") : 2048,
    ("cint32", "cint32") : 1024,
    ( "float",  "float") : 2048,
    ("cfloat",  "float") : 2048,
    ("cfloat", "cfloat") : 1024
  }
  return (
    isError(f"Max supported filter length (filter length = {TP_FIR_LEN} > Max = {maxTaps[(TT_DATA, TT_COEF)]}) exceeded for data type and coefficient type combination {TT_DATA},{TT_COEF}.")
      if TP_FIR_LEN > maxTaps[(TT_DATA, TT_COEF)]
      else isValid
  )

def fn_validate_fir_len(TT_DATA, TT_COEF, TP_FIR_LEN, TP_INTERPOLATE_FACTOR, TP_CASC_LEN, TP_SSR, TP_API, TP_USE_COEF_RELOAD, TP_DUAL_IP):
    minLenCheck =  fn_min_fir_len_each_kernel(TP_FIR_LEN, TP_CASC_LEN, TP_SSR, TP_Rnd=TP_INTERPOLATE_FACTOR)

    maxLenCheck = fn_max_fir_len_each_kernel(TP_FIR_LEN, TP_CASC_LEN, TP_USE_COEF_RELOAD, TP_SSR, 1)

    maxLenOverallCheck = fn_max_fir_len_overall(TT_DATA, TT_COEF, TP_FIR_LEN)

    multipleInterpolationRateCheck = fn_fir_len_multiple_interp(TP_FIR_LEN, TP_INTERPOLATE_FACTOR)

    streamingVectorRegisterCheck = fn_check_samples_can_fit_streaming(TT_DATA, TT_COEF, TP_FIR_LEN, TP_INTERPOLATE_FACTOR, TP_CASC_LEN, TP_DUAL_IP, TP_API, TP_SSR)

    for check in (minLenCheck,maxLenCheck,maxLenOverallCheck, multipleInterpolationRateCheck, streamingVectorRegisterCheck):
      if check["is_valid"] == False :
        return check

    return isValid


def fn_type_support(TT_DATA, TT_COEF):
  return isError(f"The combination of {TT_DATA} and {TT_COEF} is not supported for this class.") if (TT_DATA == "int16" and TT_COEF == "int16") else isValid


def fn_validate_input_window_size(TT_DATA, TT_COEF, TP_FIR_LEN,TP_INTERPOLATE_FACTOR, TP_INPUT_WINDOW_VSIZE, TP_API, TP_SSR=1):
    # interpolate asym uses common lanes, but doesn't use shorter acc for streaming arch.. why?
    checkMultipleLanes =  fn_windowsize_multiple_lanes(TT_DATA, TT_COEF, TP_INPUT_WINDOW_VSIZE, 0)
    checkMaxBuffer = fn_max_windowsize_for_buffer(TT_DATA, TP_FIR_LEN, TP_INPUT_WINDOW_VSIZE, TP_API, TP_SSR, TP_INTERPOLATE_FACTOR)
    # Input samples are round-robin split to each SSR input paths, so total frame size must be divisable by SSR factor.
    checkIfDivisableBySSR = fn_windowsize_divisible_by_ssr(TP_INPUT_WINDOW_VSIZE, TP_SSR)

    for check in (checkMultipleLanes,checkMaxBuffer,checkIfDivisableBySSR):
      if check["is_valid"] == False :
        return check

    return isValid

#### validation APIs ####
def validate_TT_COEF(args):
    TT_DATA = args["TT_DATA"]
    TT_COEF = args["TT_COEF"]
    standard_checks = fn_validate_coef_type(TT_DATA, TT_COEF)
    typeCheck = fn_type_support(TT_DATA, TT_COEF)
    for check in (standard_checks,typeCheck):
      if check["is_valid"] == False :
        return check
    return isValid

def validate_TP_SHIFT(args):
  TT_DATA = args["TT_DATA"]
  TP_SHIFT = args["TP_SHIFT"]
  return fn_validate_shift(TT_DATA, TP_SHIFT)

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
    TT_COEF = args["TT_COEF"]
    TP_FIR_LEN = args["TP_FIR_LEN"]
    TP_API = args["TP_API"]
    TP_INTERPOLATE_FACTOR = args["TP_INTERPOLATE_FACTOR"]
    TP_SSR = args["TP_SSR"]

    #interpolate_hb traits looks like the UPSHIFT_CT types have different number of lanes, but it's actually stil the exact same as 384..
    # decimate_hb also just uses 384, so no additional rules here.
    return fn_validate_input_window_size(TT_DATA, TT_COEF, TP_FIR_LEN, TP_INTERPOLATE_FACTOR, TP_INPUT_WINDOW_VSIZE, TP_API, TP_SSR)



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
    TT_COEF = args["TT_COEF"]
    TP_FIR_LEN = args["TP_FIR_LEN"]
    TP_CASC_LEN = args["TP_CASC_LEN"]
    TP_INTERPOLATE_FACTOR = args["TP_INTERPOLATE_FACTOR"]
    TP_SSR = args["TP_SSR"]
    TP_API = args["TP_API"]
    TP_USE_COEF_RELOAD = args["TP_USE_COEF_RELOAD"]
    TP_DUAL_IP = args["TP_DUAL_IP"]

    return fn_validate_fir_len(TT_DATA, TT_COEF, TP_FIR_LEN,TP_INTERPOLATE_FACTOR, TP_CASC_LEN, TP_SSR, TP_API, TP_USE_COEF_RELOAD, TP_DUAL_IP)

def validate_TP_DUAL_IP(args):
  return sr_asym.validate_TP_DUAL_IP(args)

def fn_interp_ssr(TP_INTERPOLATE_FACTOR, TP_SSR):
  if TP_INTERPOLATE_FACTOR == TP_SSR:
    return isError(f"Currently, SSR equal to interpolate factor is not supported. Please set SSR to next higher value to get required throughput")
  return isValid

def fn_validate_ssr(TP_SSR, TP_INTERPOLATE_FACTOR, TP_API):
    ssrStreamCheck = fn_stream_ssr(TP_API, TP_SSR)
    ssrIPFactorCheck = fn_interp_ssr(TP_INTERPOLATE_FACTOR, TP_SSR)
    for check in (ssrStreamCheck, ssrIPFactorCheck):
      if check["is_valid"] == False :
        return check

    return isValid

def validate_TP_SSR(args):
    TP_INTERPOLATE_FACTOR = args["TP_INTERPOLATE_FACTOR"]
    TP_PARA_INTERP_POLY = args["TP_PARA_INTERP_POLY"]
    TP_API = args["TP_API"]
    TP_SSR = args["TP_SSR"]
    kernelInterpolate = TP_INTERPOLATE_FACTOR//TP_PARA_INTERP_POLY
    return (
      fn_validate_ssr(TP_SSR, TP_INTERPOLATE_FACTOR//TP_PARA_INTERP_POLY, TP_API) if kernelInterpolate > 1
      else isValid # assume single rate SSR is valid, since it doesn't have a validation function
    )

# Example of updater.
#
# Updater are functions to help GUI to hint user on parameter setting with already given parameters.
# The return object will provide "value" which will be set in the wizard as the dependent parameter is being set.
# The rest of keys are similar to paramster definition, but with candidates of enum or range values refined
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
#  { "value": None, "err_message": "With TT_DATA as 'int' there is no valid option for TT_COEF" }
#
# In this example, the following is the updater for TT_COEF, with TT_DATA as the dependent paramster.
# When GUI generates a wizard, TT_DATA should be required first, as it shows up in parameter list first.
# Once user has provided value for TT_DATA, this function will be called and set the value of TT_COEF.
# Meanwhile, the candidate shown in wizard based on enum will also be updated.
#
def update_TT_COEF(TT_DATA):
    return {"value": TT_DATA,
            "enum": [TT_DATA]}

#### port ####


def info_ports(args):
    """Standard function creating a static dictionary of information
    for upper software to correctly connect the IP.
    Some IP has dynamic number of ports according to parameter set,
    so port information has to be implemented as a function"""
    TT_DATA = args["TT_DATA"]
    TT_COEF = args["TT_COEF"]
    TP_INPUT_WINDOW_VSIZE = args["TP_INPUT_WINDOW_VSIZE"]
    TP_FIR_LEN = args["TP_FIR_LEN"]
    TP_SSR = args["TP_SSR"]
    TP_INTERPOLATE_FACTOR = args["TP_INTERPOLATE_FACTOR"]
    TP_PARA_INTERP_POLY = (args["TP_PARA_INTERP_POLY"] if ("TP_PARA_INTERP_POLY" in args)  else 1)
    margin_size = sr_asym.fn_margin_size(TP_FIR_LEN//TP_INTERPOLATE_FACTOR, TT_DATA)
    num_in_ports = TP_SSR # *TP_PARA_DECI_POLY (not in the internpolator)
    in_win_size = TP_INPUT_WINDOW_VSIZE//num_in_ports
    num_out_ports = TP_SSR*TP_PARA_INTERP_POLY
    out_win_size = (TP_INPUT_WINDOW_VSIZE*TP_INTERPOLATE_FACTOR)//num_out_ports

    in_ports = get_port_info("in", "in", TT_DATA, in_win_size, num_in_ports, marginSize=margin_size, TP_API=args["TP_API"])
    in2_ports = (get_port_info("in2", "in", TT_DATA, in_win_size
    , num_in_ports, marginSize=margin_size, TP_API=args["TP_API"]) if (args["TP_DUAL_IP"] == 1) else [])
    coeff_ports = (get_parameter_port_info("coeff", "in", TT_COEF, TP_SSR, TP_FIR_LEN, "async") if (args["TP_USE_COEF_RELOAD"] == 1) else [])

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

  TT_COEF = args["TT_COEF"]
  TT_DATA = args["TT_DATA"]
  TP_FIR_LEN = args["TP_FIR_LEN"]
  TP_SHIFT = args["TP_SHIFT"]
  TP_RND = args["TP_RND"]
  TP_CASC_LEN = args["TP_CASC_LEN"]
  TP_INPUT_WINDOW_VSIZE = args["TP_INPUT_WINDOW_VSIZE"]
  TP_INTERPOLATE_FACTOR = args["TP_INTERPOLATE_FACTOR"]
  TP_USE_COEF_RELOAD = args["TP_USE_COEF_RELOAD"]
  TP_NUM_OUTPUTS = args["TP_NUM_OUTPUTS"]
  TP_DUAL_IP = args["TP_DUAL_IP"]
  TP_API = args["TP_API"]
  TP_PARA_INTERP_POLY = (args["TP_PARA_INTERP_POLY"] if ("TP_PARA_INTERP_POLY" in args)  else 1)
  TP_SSR = args["TP_SSR"]
  coeff_list = args["coeff"]

  taps = sr_asym.fn_get_taps_vector(TT_COEF, coeff_list)
  constr_args_str = f"taps" if TP_USE_COEF_RELOAD == 0 else ""
  dual_ip_declare_str = f" std::array<adf::port<input>, TP_SSR> in2;" if TP_DUAL_IP == 1 else "// No dual input"
  dual_ip_connect_str = f"adf::connect<> net_in2(in2[ssrIdx], filter.in2[ssrIdx]);" if TP_DUAL_IP == 1 else "// No dual input"
  coeff_ip_declare_str = f"std::array<adf::port<input>, TP_SSR> coeff;" if TP_USE_COEF_RELOAD == 1 else "//No coeff port"
  coeff_ip_connect_str = f"adf::connect<> net_coeff(coeff[ssrIdx], filter.coeff[ssrIdx]);" if TP_USE_COEF_RELOAD == 1 else "//No coeff port"

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

  std::vector<{TT_COEF}> taps = {taps};
  xf::dsp::aie::fir::interpolate_asym::fir_interpolate_asym_graph<
    {TT_DATA}, //TT_DATA
    {TT_COEF}, //TT_COEF
    {TP_FIR_LEN}, //TP_FIR_LEN
    {TP_INTERPOLATE_FACTOR}, //TP_INTERPOLATE_FACTOR
    {TP_SHIFT}, //TP_SHIFT
    {TP_RND}, //TP_RND
    {TP_INPUT_WINDOW_VSIZE}, //TP_INPUT_WINDOW_VSIZE
    {TP_CASC_LEN}, //TP_CASC_LEN
    {TP_USE_COEF_RELOAD}, //TP_USE_COEF_RELOAD
    {TP_DUAL_IP}, //TP_DUAL_IP
    {TP_NUM_OUTPUTS}, //TP_NUM_OUTPUTS
    {TP_API}, //TP_API
    {TP_SSR}, //TP_SSR
    {TP_PARA_INTERP_POLY} //TP_PARA_INTERP_POLY
  > filter;

  {graphname}() : filter({constr_args_str}) {{
    adf::kernel *filter_kernels = filter.getKernels();
    for (int i=0; i < 1; i++) {{
      adf::runtime<ratio>(filter_kernels[i]) = 0.9;
    }}
    for (int ssrIdx=0; ssrIdx < TP_SSR; ssrIdx++) {{
      adf::connect<> net_in(in[ssrIdx], filter.in[ssrIdx]);
      {dual_ip_connect_str}
      {coeff_ip_connect_str}
    }}

    for (int paraPolyIdx=0; paraPolyIdx < TP_PARA_INTERP_POLY; paraPolyIdx++) {{
      for (int ssrIdx=0; ssrIdx < TP_SSR; ssrIdx++) {{
        unsigned outPortIdx = paraPolyIdx+ssrIdx*TP_PARA_INTERP_POLY;
        adf::connect<> net_out(filter.out[outPortIdx], out[outPortIdx]);
      {dual_op_connect_str}
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


