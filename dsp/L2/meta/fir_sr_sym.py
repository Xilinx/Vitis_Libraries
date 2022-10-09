from ctypes import sizeof
from socket import TIPC_SUB_SERVICE
from aie_common import *
from aie_common_fir import *
import json
import fir_sr_asym as sr_asym



  # valid tt coef
    #static_assert(fnTypeCheckDataCoeffSize<TT_DATA,TT_COEFF>() != 0, "ERROR: TT_DATA type less precise than TT_COEFF is not supported.");
    #static_assert(fnTypeCheckDataCoeffCmplx<TT_DATA,TT_COEFF>() != 0, "ERROR: real TT_DATA with complex TT_COEFF is not supported.");
    #static_assert(fnTypeCheckDataCoeffFltInt<TT_DATA,TT_COEFF>() != 0, "ERROR: a mix of float and integer types of TT_DATA and TT_COEFF is not supported.");

  #valid t shift
    #static_assert(!(std::is_same<TT_DATA,cfloat>::value || std::is_same<TT_DATA,float>::value) || (TP_SHIFT == 0), "ERROR: TP_SHIFT cannot be performed for TT_DATA=cfloat, so must be set to 0");


    #// Limit FIR length per kernel. Longer FIRs may exceed Program Memory and/or system memory combined with window buffers may exceed Memory Module size.
    #static_assert(TP_FIR_LEN / TP_CASC_LEN <= kMaxTapsPerKernel,"ERROR: Requested FIR length and Cascade length exceeds supported number of taps per kernel. Please increase the cascade legnth to accomodate the FIR design.");

    #static_assert(TP_FIR_RANGE_LEN >= FIR_LEN_MIN,"ERROR: Illegal combination of design FIR length and cascade length, resulting in kernel FIR length below minimum required value. ");

    #// Limit FIR length for reloadable coeffs. Reloadable coeffs need a storage space that contibutes to system memory exceeding Memory Module size.
    #static_assert(TP_USE_COEFF_RELOAD == 0 || TP_FIR_LEN  <= kMaxTapsPerKernel,"ERROR: Exceeded maximum supported FIR length with reloadable coefficients. Please limit the FIR length or disable coefficient reload.");

    #// Requested Window buffer exceeds memory module size = fn_validate_input_window_size
    #static_assert(TP_API != 0 || bufferSize < kMemoryModuleSize, "ERROR: Input Window size (based on requrested window size and FIR length margin) exceeds Memory Module size of 32kB.");

    #static_assert(TP_INPUT_WINDOW_VSIZE % m_kLanes == 0, "ERROR: TP_INPUT_WINDOW_VSIZE must be an integer multiple of the number of lanes for this data type");


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


# Logic derived from sr_sym_traits
def fnNumLanesSym(TT_DATA, TT_COEF):
  return (
    fnNumLanes(TT_DATA, TT_COEF)
    if (not ((TT_DATA == "cint16" and TT_COEF == "int16") or (TT_DATA == "cint32" and TT_COEF == "cint32")))
    else (fnNumLanes384b(TT_DATA, TT_COEF)
      if (TT_DATA == "cint16" and TT_COEF == "int16")
      else 2 #cint32 cint32 only has 2 lanes
    )
  )


def fn_validate_input_window_size(TT_DATA, TT_COEF, TP_FIR_LEN, TP_INPUT_WINDOW_VSIZE, TP_API, TP_SSR=1):

    checkMultipleLanes =  fn_windowsize_multiple_lanes(TT_DATA, TT_COEF, TP_INPUT_WINDOW_VSIZE, TP_API, numLanes=fnNumLanesSym(TT_DATA, TT_COEF))
    checkMaxBuffer = fn_max_windowsize_for_buffer(TT_DATA, TP_FIR_LEN, TP_INPUT_WINDOW_VSIZE, TP_API, TP_SSR)
    # Input samples are round-robin split to each SSR input paths, so total frame size must be divisable by SSR factor.
    checkIfDivisableBySSR = fn_windowsize_divisible_by_ssr(TP_INPUT_WINDOW_VSIZE, TP_SSR)

    for check in (checkMultipleLanes,checkMaxBuffer,checkIfDivisableBySSR):
      if check["is_valid"] == False :
        return check

    return isValid


def fn_validate_fir_len(TT_DATA, TT_COEF, TP_FIR_LEN, TP_CASC_LEN, TP_SSR, TP_API, TP_USE_COEF_RELOAD):
    minLenCheck =  fn_min_fir_len_each_kernel(TP_FIR_LEN, TP_CASC_LEN, TP_SSR)
    symFactor   = 1 if (TT_DATA == "cfloat" or TT_DATA == "float") else 2  # guessing this is because the code bloats up very easily for float data types and we hit program memory limitations earlier
    maxLenCheck = fn_max_fir_len_each_kernel(TP_FIR_LEN, TP_CASC_LEN, TP_USE_COEF_RELOAD, TP_SSR, symFactor)
    dataNeededCheck = isValid
    if TP_SSR > 1:
      dataNeededCheck = sr_asym.fn_data_needed_within_buffer_size(TT_DATA, TT_COEF, TP_FIR_LEN, TP_CASC_LEN,TP_API, TP_SSR )
    for check in (minLenCheck,maxLenCheck):
      if check["is_valid"] == False :
        return check

    return isValid

def fn_validate_num_outputs(TP_SSR, TP_DUAL_IP, TP_NUM_OUTPUTS):
  if TP_SSR > 1 :
    if TP_DUAL_IP == 1  and TP_NUM_OUTPUTS == 1:
      return isError(f"When SSR > 1, dual inputs are only allowed with dual outputs.")
    elif TP_DUAL_IP == 0 and TP_NUM_OUTPUTS == 2:
      return isError(f"When SSR > 1, dual inputs are only allowed with dual outputs.")
  return isValid

def fn_validate_ssr(TP_API, TP_SSR):
    ssrStreamCheck = fn_stream_ssr(TP_API, TP_SSR)
    return ssrStreamCheck

#### validation APIs ####
def validate_TT_COEF(args):
    TT_DATA = args["TT_DATA"]
    TT_COEF = args["TT_COEF"]
    return fn_validate_coef_type(TT_DATA, TT_COEF)

def validate_TP_SHIFT(args):
  TT_DATA = args["TT_DATA"]
  TP_SHIFT = args["TP_SHIFT"]
  return fn_validate_shift(TT_DATA, TP_SHIFT)

def validate_TP_INPUT_WINDOW_VSIZE(args):
    TP_INPUT_WINDOW_VSIZE = args["TP_INPUT_WINDOW_VSIZE"]
    TT_DATA = args["TT_DATA"]
    TT_COEF = args["TT_COEF"]
    TP_FIR_LEN = args["TP_FIR_LEN"]
    TP_API = args["TP_API"]
    TP_SSR = args["TP_SSR"]
    return fn_validate_input_window_size(TT_DATA, TT_COEF, TP_FIR_LEN, TP_INPUT_WINDOW_VSIZE, TP_API, TP_SSR)



def validate_TP_FIR_LEN(args):
    TT_DATA = args["TT_DATA"]
    TT_COEF = args["TT_COEF"]
    TP_FIR_LEN = args["TP_FIR_LEN"]
    TP_CASC_LEN = args["TP_CASC_LEN"]
    TP_SSR = args["TP_SSR"]
    TP_API = args["TP_API"]
    TP_USE_COEF_RELOAD = args["TP_USE_COEF_RELOAD"]

    return fn_validate_fir_len(TT_DATA, TT_COEF, TP_FIR_LEN, TP_CASC_LEN, TP_SSR, TP_API, TP_USE_COEF_RELOAD)

def validate_TP_NUM_OUTPUTS(args):
    TP_NUM_OUTPUTS    = args["TP_NUM_OUTPUTS"]
    TP_SSR = args["TP_SSR"]
    TP_DUAL_IP        = args["TP_DUAL_IP"]
    return fn_validate_num_outputs(TP_SSR, TP_DUAL_IP, TP_NUM_OUTPUTS)

def validate_TP_SSR(args):
    TP_SSR              = args["TP_SSR"]
    TP_API              = args["TP_API"]
    return fn_validate_ssr(TP_API, TP_SSR)
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
    margin_size = sr_asym.fn_margin_size(TP_FIR_LEN//TP_SSR, TT_DATA)

    in_ports = get_port_info("in", "in", TT_DATA, TP_INPUT_WINDOW_VSIZE//TP_SSR, TP_SSR, marginSize=margin_size, TP_API=args["TP_API"])
    in2_ports = (get_port_info("in2", "in", TT_DATA, TP_INPUT_WINDOW_VSIZE//TP_SSR, TP_SSR, marginSize=margin_size, TP_API=args["TP_API"]) if (args["TP_DUAL_IP"] == 1) else [])
    coeff_ports = (get_parameter_port_info("coeff", "in", TT_COEF, TP_SSR, (TP_FIR_LEN+1)/2, "async") if (args["TP_USE_COEF_RELOAD"] == 1) else [])

    # decimate by 2 for halfband
    out_ports = get_port_info("out", "out", TT_DATA, TP_INPUT_WINDOW_VSIZE//TP_SSR, TP_SSR, TP_API=args["TP_API"])
    out2_ports = (get_port_info("out2", "out", TT_DATA, TP_INPUT_WINDOW_VSIZE//TP_SSR, TP_SSR, TP_API=args["TP_API"]) if (args["TP_NUM_OUTPUTS"] == 2) else [])
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
  TP_USE_COEF_RELOAD = args["TP_USE_COEF_RELOAD"]
  TP_NUM_OUTPUTS = args["TP_NUM_OUTPUTS"]
  TP_DUAL_IP = 0
  TP_API = args["TP_API"]
  TP_SSR = args["TP_SSR"]
  coeff_list = args["coeff"]

  taps = sr_asym.fn_get_taps_vector(TT_COEF, coeff_list)
  constr_args_str = f"taps" if TP_USE_COEF_RELOAD == 0 else ""
  dual_ip_declare_str = f"ssr_port_array<input> in2;" if TP_DUAL_IP == 1 else "// No dual input"
  dual_ip_connect_str = f"adf::connect<> net_in2(in2[i], filter.in2[i]);" if TP_DUAL_IP == 1 else "// No dual input"
  coeff_ip_declare_str = f"ssr_port_array<input> coeff;" if TP_USE_COEF_RELOAD == 1 else "//No coeff port"
  coeff_ip_connect_str = f"adf::connect<> net_coeff(coeff[i], filter.coeff[i]);" if TP_USE_COEF_RELOAD == 1 else "//No coeff port"
  dual_op_declare_str = f"ssr_port_array<output> out2;" if TP_NUM_OUTPUTS == 2 else "// No dual output"
  dual_op_connect_str = f"adf::connect<> net_out2(filter.out2[i], out2[i]);" if TP_NUM_OUTPUTS == 2 else "// No dual output"
  # Use formatted multi-line string to avoid a lot of \n and \t
  code  = (
f"""
class {graphname} : public adf::graph {{
public:
  static constexpr unsigned int TP_SSR = {TP_SSR};
  template <typename dir>
  using ssr_port_array = std::array<adf::port<dir>, TP_SSR>;

  ssr_port_array<input> in;
  {dual_ip_declare_str}
  {coeff_ip_declare_str}
  ssr_port_array<output> out;
  {dual_op_declare_str}

  std::vector<{TT_COEF}> taps = {taps};
  xf::dsp::aie::fir::sr_sym::fir_sr_sym_graph<
    {TT_DATA}, //TT_DATA
    {TT_COEF}, //TT_COEF
    {TP_FIR_LEN}, //TP_FIR_LEN
    {TP_SHIFT}, //TP_SHIFT
    {TP_RND}, //TP_RND
    {TP_INPUT_WINDOW_VSIZE}, //TP_INPUT_WINDOW_VSIZE
    {TP_CASC_LEN}, //TP_CASC_LEN
    {TP_DUAL_IP}, //TP_DUAL_IP
    {TP_USE_COEF_RELOAD}, //TP_USE_COEF_RELOAD
    {TP_NUM_OUTPUTS}, //TP_NUM_OUTPUTS
    {TP_API}, //TP_API
    {TP_SSR} //TP_SSR
  > filter;

  {graphname}() : filter({constr_args_str}) {{
    adf::kernel *filter_kernels = filter.getKernels();
    for (int i=0; i < 1; i++) {{
      adf::runtime<ratio>(filter_kernels[i]) = 0.9;
    }}
    for (int i=0; i < TP_SSR; i++) {{
      adf::connect<> net_in(in[i], filter.in[i]);
      {dual_ip_connect_str}
      {coeff_ip_connect_str}
      adf::connect<> net_out(filter.out[i], out[i]);
      {dual_op_connect_str}
    }}
  }}

}};
""")
  out = {}
  out["graph"] = code
  out["port_info"] = info_ports(args)
  out["headerfile"] = "fir_sr_sym_graph.hpp"
  out["searchpaths"] = [
       "L2/include/aie",
       "L2/tests/aie/common/inc",
       "L1/include/aie",
       "L1/src/aie",
       "L1/tests/aie/inc",
       "L1/tests/aie/src"
  ]

  return out
