from ctypes import sizeof
from socket import TIPC_SUB_SERVICE
from aie_common import *
from aie_common_fir import *
import json
import fir_sr_asym as sr_asym
import fir_decimate_hb as deci_hb

# fir_interpolate_hb.hpp:       static_assert(TP_FIR_RANGE_LEN >= FIR_LEN_MIN,"ERROR: Illegal combination of design FIR length and cascade length, resulting in kernel FIR length below minimum required value. ");
# fir_interpolate_hb.hpp:       static_assert(TP_SHIFT >= SHIFT_MIN && TP_SHIFT <= SHIFT_MAX, "ERROR: SHIFT is out of the supported range.");
# fir_interpolate_hb.hpp:       static_assert(TP_RND >= ROUND_MIN && TP_RND <= ROUND_MAX, "ERROR: RND is out of the supported range.");
# fir_interpolate_hb.hpp:       static_assert(((TP_FIR_LEN +1)%4) == 0,"ERROR: TP_FIR_LEN must be 4N-1 where N is a positive integer.");
# fir_interpolate_hb.hpp:       static_assert(fnEnumType<TT_DATA>() != enumUnknownType,"ERROR: TT_DATA is not a supported type.");
# fir_interpolate_hb.hpp:       static_assert(fnEnumType<TT_COEFF>() != enumUnknownType,"ERROR: TT_COEFF is not a supported type.");
# fir_interpolate_hb.hpp:       static_assert(fnTypeCheckDataCoeffSize<TT_DATA,TT_COEFF>() != 0, "ERROR: TT_DATA type less precise than TT_COEFF is not supported.");
# fir_interpolate_hb.hpp:       static_assert(fnTypeCheckDataCoeffCmplx<TT_DATA,TT_COEFF>() != 0, "ERROR: real TT_DATA with complex TT_COEFF is not supported.");
# fir_interpolate_hb.hpp:       static_assert(fnTypeCheckDataCoeffFltInt<TT_DATA,TT_COEFF>() != 0, "ERROR: a mix of float and integer types of TT_DATA and TT_COEFF is not supported.");
# fir_interpolate_hb.hpp:       static_assert(TP_NUM_OUTPUTS >0 && TP_NUM_OUTPUTS <=2, "ERROR: only single or dual outputs are supported." );
# fir_interpolate_hb.hpp:       static_assert(TP_UPSHIFT_CT == 0 || fnUpshiftCTSupport<TT_DATA,TT_COEFF>() == SUPPORTED, "ERROR: Unsupported data/coeff type combination. Upshift CT is only available for 16-bit integer combinations." );
# fir_interpolate_hb.hpp:       static_assert(!(std::is_same<TT_DATA,cfloat>::value || std::is_same<TT_DATA,float>::value) || (TP_SHIFT == 0), "ERROR: TP_SHIFT cannot be performed for TT_DATA=cfloat, so must be set to 0");
# fir_interpolate_hb.hpp:       static_assert(TP_INPUT_WINDOW_VSIZE % m_kLanes == 0, "ERROR: TP_INPUT_WINDOW_VSIZE must be an integer multiple of the number of lanes for this data type");
# fir_interpolate_hb_graph.hpp: static_assert(TP_CASC_LEN <= 40,"ERROR: Unsupported Cascade length");
# fir_interpolate_hb_graph.hpp: static_assert(TP_FIR_LEN / TP_CASC_LEN <= kMaxTapsPerKernel,"ERROR: Requested FIR length and Cascade length exceeds supported number of taps per kernel. Please increase the cascade legnth to accomodate the FIR design.");
# fir_interpolate_hb_graph.hpp: static_assert(TP_USE_COEFF_RELOAD == 0 || TP_FIR_LEN <= kMaxTapsPerKernel,"ERROR: Exceeded maximum supported FIR length with reloadable coefficients. Please limit the FIR length or disable coefficient reload.");
# fir_interpolate_hb_graph.hpp: static_assert(TP_API != 0 || bufferSize < kMemoryModuleSize, "ERROR: Input Window size (based on requrested window size and FIR length margin) exceeds Memory Module size of 32kB");

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


def fn_halfband_len(TP_FIR_LEN):
  return isValid if ((TP_FIR_LEN + 1) % 4 == 0) else isError("Filter length must be 4N-1 where N is a positive integer.")

def fn_validate_fir_len(TT_DATA, TT_COEF, TP_FIR_LEN, TP_CASC_LEN, TP_SSR, TP_API, TP_USE_COEF_RELOAD, TP_PARA_INTERP_POLY):
    minLenCheck =  fn_min_fir_len_each_kernel(TP_FIR_LEN, TP_CASC_LEN, TP_SSR)

    maxLenCheck = fn_max_fir_len_each_kernel(TP_FIR_LEN, TP_CASC_LEN, TP_USE_COEF_RELOAD, TP_SSR, 4)
    halfbandLenCheck = fn_halfband_len(TP_FIR_LEN)
    dataNeededCheck = isValid
    if TP_PARA_INTERP_POLY > 1:
      dataNeededCheck = sr_asym.fn_data_needed_within_buffer_size(TT_DATA, TT_COEF, (TP_FIR_LEN + 1)/2, TP_CASC_LEN,TP_API, TP_SSR )
    for check in (minLenCheck,maxLenCheck,halfbandLenCheck, dataNeededCheck):
      if check["is_valid"] == False :
        return check

    return isValid

def fn_validate_num_outputs(TP_PARA_INTERP_POLY, TP_DUAL_IP, TP_NUM_OUTPUTS):
  if TP_PARA_INTERP_POLY == 2 :
    if TP_DUAL_IP == 1  and TP_NUM_OUTPUTS == 1:
      return isError(f"When TP_PARA_INTERP_POLY is set to 2, dual inputs are only allowed with dual outputs.")
    elif TP_DUAL_IP == 0 and TP_NUM_OUTPUTS == 2:
      return isError(f"When TP_PARA_INTERP_POLY is set to 2, dual inputs are only allowed with dual outputs.")
  return isValid

def fn_parapoly_value(TP_PARA_INTERP_POLY):
    if TP_PARA_INTERP_POLY != 1 and TP_PARA_INTERP_POLY != 2:
      return isError(f"TP_PARA_INTERP_POLY can be only set to 1 or 2 for halfbands")
    return isValid

def fn_ssr_for_para_poly(TP_PARA_INTERP_POLY, TP_SSR):
  if TP_SSR > 1 and TP_PARA_INTERP_POLY != 2:
    return isError(f"SSR > 1 is only supported with TP_PARA_INTERP_POLY set to 2")
  return isValid

def fn_stream_api_poly(TP_PARA_INTERP_POLY, TP_API):
    if (TP_PARA_INTERP_POLY == 1 or TP_API == 1):
        return isValid
    return isError(f"TP_PARA_INTERP_POLY can be set to 2 only for streaming API")

def fn_validate_para_interp_poly(TP_API, TP_PARA_INTERP_POLY, TP_SSR):
    checkParaPolyVal = fn_parapoly_value(TP_PARA_INTERP_POLY)
    checkSSRPoly     = fn_ssr_for_para_poly(TP_PARA_INTERP_POLY, TP_SSR)
    checkStreamsPoly = fn_stream_api_poly(TP_PARA_INTERP_POLY, TP_API)

    for check in (checkParaPolyVal,checkSSRPoly, checkStreamsPoly):
      if check["is_valid"] == False :
        return check

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

def fn_validate_upshift_ct(TT_DATA, TP_UPSHIFT_CT):
  #implied restriction that TT_DATA restricts TT_COEF, ie, we don't support int16,int32 or int16,cint32
  return (
    isError("Upshift CT is only available for 16-bit integer combinations.")
      if ((TT_DATA not in ["cint16", "int16"] ) and (TP_UPSHIFT_CT == 1))
    else isValid
    )

def fn_validate_input_window_size(TT_DATA, TT_COEF, TP_FIR_LEN, TP_INPUT_WINDOW_VSIZE, TP_API, TP_SSR=1):
    # interpolate halfband always uses 384b version of lanes. Some archs use repeat factors, like zig-zag, hence the factor of 2.
    checkMultipleLanes =  fn_windowsize_multiple_lanes(TT_DATA, TT_COEF, TP_INPUT_WINDOW_VSIZE, TP_API, numLanes=fnNumLanes384b(TT_DATA, TT_COEF)*2)
    #  also checks output size (this isn't done on static asserts for some reason right now)
    checkMaxBuffer = fn_max_windowsize_for_buffer(TT_DATA, TP_FIR_LEN, TP_INPUT_WINDOW_VSIZE, TP_API, TP_SSR, TP_INTERPOLATE_FACTOR=2, TP_DECIMATE_FACTOR=1)
    # Input samples are round-robin split to each SSR input paths, so total frame size must be divisable by SSR factor.
    checkIfDivisableBySSR = fn_windowsize_divisible_by_ssr(TP_INPUT_WINDOW_VSIZE, TP_SSR)

    for check in (checkMultipleLanes,checkMaxBuffer,checkIfDivisableBySSR):
      if check["is_valid"] == False :
        return check

    return isValid

def validate_TP_UPSHIFT_CT(args):
  TT_DATA = args["TT_DATA"]
  TP_UPSHIFT_CT = args["TP_UPSHIFT_CT"]
  return fn_validate_upshift_ct(TT_DATA, TP_UPSHIFT_CT)

def validate_TP_INPUT_WINDOW_VSIZE(args):
    TP_INPUT_WINDOW_VSIZE = args["TP_INPUT_WINDOW_VSIZE"]
    TT_DATA = args["TT_DATA"]
    TT_COEF = args["TT_COEF"]
    TP_FIR_LEN = args["TP_FIR_LEN"]
    TP_API = args["TP_API"]
    TP_SSR = args["TP_SSR"]

    #interpolate_hb traits looks like the UPSHIFT_CT types have different number of lanes, but it's actually stil the exact same as 384..
    # decimate_hb also just uses 384, so no additional rules here.
    return fn_validate_input_window_size(TT_DATA, TT_COEF, TP_FIR_LEN, TP_INPUT_WINDOW_VSIZE, TP_API, TP_SSR)



def validate_TP_FIR_LEN(args):
    TT_DATA = args["TT_DATA"]
    TT_COEF = args["TT_COEF"]
    TP_FIR_LEN = args["TP_FIR_LEN"]
    TP_CASC_LEN = args["TP_CASC_LEN"]
    TP_SSR = args["TP_SSR"]
    TP_API = args["TP_API"]
    TP_USE_COEF_RELOAD = args["TP_USE_COEF_RELOAD"]
    TP_PARA_INTERP_POLY = args["TP_PARA_INTERP_POLY"]

    return fn_validate_fir_len(TT_DATA, TT_COEF, TP_FIR_LEN, TP_CASC_LEN, TP_SSR, TP_API, TP_USE_COEF_RELOAD, TP_PARA_INTERP_POLY)

def validate_TP_NUM_OUTPUTS(args):
    TP_NUM_OUTPUTS    = args["TP_NUM_OUTPUTS"]
    TP_PARA_INTERP_POLY = args["TP_PARA_INTERP_POLY"]
    TP_DUAL_IP        = args["TP_DUAL_IP"]
    return fn_validate_num_outputs(TP_PARA_INTERP_POLY, TP_DUAL_IP, TP_NUM_OUTPUTS)

def validate_TP_PARA_INTERP_POLY(args):
    TP_PARA_INTERP_POLY   = args["TP_PARA_INTERP_POLY"]
    TP_API              = args["TP_API"]
    TP_SSR              = args["TP_SSR"]
    return fn_validate_para_interp_poly(TP_API, TP_PARA_INTERP_POLY, TP_SSR)

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
    TP_API = args["TP_API"]
    TP_NUM_OUTPUTS = args["TP_NUM_OUTPUTS"]
    TP_PARA_INTERP_POLY = args["TP_PARA_INTERP_POLY"]
    TP_DUAL_IP = args["TP_DUAL_IP"]
    TP_USE_COEF_RELOAD = args["TP_USE_COEF_RELOAD"]
    margin_size = sr_asym.fn_margin_size(TP_FIR_LEN//2, TT_DATA)
    num_in_ports = TP_SSR
    in_win_size = TP_INPUT_WINDOW_VSIZE//num_in_ports
    num_out_ports = TP_SSR*TP_PARA_INTERP_POLY
    out_win_size = (TP_INPUT_WINDOW_VSIZE*2)//num_out_ports

    in_ports = get_port_info("in", "in", TT_DATA, in_win_size, TP_SSR, margin_size, TP_API)
    in2_ports = (get_port_info("in2", "in", TT_DATA, in_win_size, TP_SSR, margin_size, TP_API) if (TP_DUAL_IP == 1) else [])
    coeff_ports = (get_parameter_port_info("coeff", "in", TT_COEF, TP_SSR, ((TP_FIR_LEN+1)/4+1), "async") if (TP_USE_COEF_RELOAD == 1) else [])
    coeffCT_ports = (get_parameter_port_info("coeffCT", "in", TT_COEF, TP_SSR, ((TP_FIR_LEN+1)/4+1), "async") if (TP_USE_COEF_RELOAD == 1 and TP_PARA_INTERP_POLY > 1) else [])

    # interp by 2 for halfband
    out_ports = get_port_info("out", "out", TT_DATA, out_win_size, TP_SSR, TP_API=args["TP_API"])
    out2_ports = (get_port_info("out2", "out", TT_DATA, out_win_size, TP_SSR, TP_API=args["TP_API"]) if (TP_NUM_OUTPUTS == 2) else [])
    out3_ports = (get_port_info("out3", "out", TT_DATA, out_win_size, TP_SSR, TP_API=args["TP_API"]) if (TP_PARA_INTERP_POLY > 1) else [])
    out4_ports = (get_port_info("out4", "out", TT_DATA, out_win_size, TP_SSR, TP_API=args["TP_API"]) if (TP_PARA_INTERP_POLY > 1 and TP_NUM_OUTPUTS == 2) else [])
    return in_ports + in2_ports + coeff_ports + coeffCT_ports + out_ports + out2_ports + out3_ports + out4_ports


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
  TP_DUAL_IP = args["TP_DUAL_IP"]
  TP_UPSHIFT_CT = args["TP_UPSHIFT_CT"]
  TP_API = args["TP_API"]
  TP_SSR = args["TP_SSR"]
  TP_PARA_INTERP_POLY = args["TP_PARA_INTERP_POLY"]
  coeff_list = args["coeff"]

  taps = sr_asym.fn_get_taps_vector(TT_COEF, coeff_list)
  constr_args_str = f"taps" if TP_USE_COEF_RELOAD == 0 else ""
  dual_ip_declare_str = f"ssr_port_array<input> in2;" if TP_DUAL_IP == 1 else "// No dual input"
  dual_ip_connect_str = f"adf::connect<> net_in2(in2[i], filter.in2[i]);" if TP_DUAL_IP == 1 else "// No dual input"
  coeff_ip_declare_str = f"ssr_port_array<input> coeff;" if TP_USE_COEF_RELOAD == 1 else "// No coeff port"
  coeff_ip_connect_str = f"adf::connect<> net_coeff(coeff[i], filter.coeff[i]);" if TP_USE_COEF_RELOAD == 1 else "// No coeff port"
  coeffCT_ip_declare_str = f"ssr_port_array<input> coeffCT;" if (TP_USE_COEF_RELOAD == 1 and TP_PARA_INTERP_POLY > 1) else "// No coeffCT port"
  coeffCT_ip_connect_str = f"adf::connect<> net_coeffCT(coeffCT[i], filter.coeffCT[i]);" if (TP_USE_COEF_RELOAD == 1 and TP_PARA_INTERP_POLY > 1) == 1 else "// No coeffCT port"
  dual_op_declare_str = f"ssr_port_array<output> out2;" if TP_NUM_OUTPUTS == 2 else "// No dual output"
  dual_op_connect_str = f"adf::connect<> net_out2(filter.out2[i], out2[i]);" if TP_NUM_OUTPUTS == 2 else "// No dual output"
  op3_declare_str = f"ssr_port_array<output> out3;" if TP_PARA_INTERP_POLY > 1 else ""
  op3_connect_str = f"adf::connect<> net_out3(filter.out3[i], out3[i]);" if TP_PARA_INTERP_POLY > 1 else ""
  op4_declare_str = f"ssr_port_array<output> out4;" if (TP_PARA_INTERP_POLY > 1 and TP_NUM_OUTPUTS == 2) else ""
  op4_connect_str = f"adf::connect<> net_out4(filter.out4[i], out4[i]);" if (TP_PARA_INTERP_POLY > 1 and TP_NUM_OUTPUTS == 2) else ""

  # Use formatted multi-line string to avoid a lot of \n and \t
  code  = (
f"""
class {graphname} : public adf::graph {{
public:
  static constexpr unsigned int TP_SSR = {TP_SSR};
  static constexpr unsigned int TP_PARA_INTERP_POLY = {TP_PARA_INTERP_POLY};
  template <typename dir>
  using ssr_port_array = std::array<adf::port<dir>, TP_SSR>;

  ssr_port_array<input> in;
  {dual_ip_declare_str}
  {coeff_ip_declare_str}
  {coeffCT_ip_declare_str}
  ssr_port_array<output> out;
  {dual_op_declare_str}
  {op3_declare_str}
  {op4_declare_str}

  std::vector<{TT_COEF}> taps = {taps};
  xf::dsp::aie::fir::interpolate_hb::fir_interpolate_hb_graph<
    {TT_DATA}, //TT_DATA
    {TT_COEF}, //TT_COEFF
    {TP_FIR_LEN}, //TP_FIR_LEN
    {TP_SHIFT}, //TP_SHIFT
    {TP_RND}, //TP_RND
    {TP_INPUT_WINDOW_VSIZE}, //TP_INPUT_WINDOW_VSIZE
    {TP_CASC_LEN}, //TP_CASC_LEN
    {TP_DUAL_IP}, //TP_DUAL_IP
    {TP_USE_COEF_RELOAD}, //TP_USE_COEF_RELOAD
    {TP_NUM_OUTPUTS}, //TP_NUM_OUTPUTS
    {TP_UPSHIFT_CT}, //TP_UPSHIFT_CT
    {TP_API}, //TP_API
    {TP_SSR}, //TP_SSR
    {TP_PARA_INTERP_POLY} //TP_PARA_INTERP_POLY
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
      {coeffCT_ip_connect_str}
      adf::connect<> net_out(filter.out[i], out[i]);
      {dual_op_connect_str}
      {op3_connect_str}
      {op4_connect_str}
    }}
  }}

}};
""")
  out = {}
  out["graph"] = code
  out["port_info"] = info_ports(args)
  out["headerfile"] = "fir_interpolate_hb_graph.hpp"
  out["searchpaths"] = [
       "L2/include/aie",
       "L2/tests/aie/common/inc",
       "L1/include/aie",
       "L1/src/aie",
       "L1/tests/aie/inc",
       "L1/tests/aie/src"
  ]

  return out

