from ctypes import sizeof
from socket import TIPC_SUB_SERVICE
from aie_common import *
from aie_common_fir import *
import json

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
TP_CASC_LEN_min = 1
TP_CASC_LEN_max = 40
TP_FIR_LEN_min = 4
TP_FIR_LEN_max = 32
TP_TDM_CHANNELS_min = 1
TP_TDM_CHANNELS_max = 8192
TP_SHIFT_min = 0
TP_SHIFT_max = 61


def fn_validate_input_window_size(
    TT_DATA, TT_COEFF, TP_FIR_LEN, TP_INPUT_WINDOW_VSIZE, TP_API, TP_SSR=1, TP_TDM_CHANNELS=1
):
    # CAUTION: this constant overlaps many factors. The main need is a "strobe" concept that means we unroll until xbuff is back to starting conditions.
    streamRptFactor = 8
    res = fn_validate_min_value("TP_INPUT_WINDOW_VSIZE", TP_INPUT_WINDOW_VSIZE, TP_INPUT_WINDOW_VSIZE_min)
    if (res["is_valid"] == False):
        return res

    windowSizeMultiplier = (
        (fnNumLanes(TT_DATA, TT_COEFF, TP_API))
        if TP_API == 0
        # Need to take unrolloing into account
        else (fnNumLanes(TT_DATA, TT_COEFF, TP_API) * streamRptFactor)
    )

    checkMultipleLanes = fn_windowsize_multiple_lanes(
        TT_DATA, TT_COEFF, TP_INPUT_WINDOW_VSIZE, TP_API, windowSizeMultiplier, TP_SSR
    )
    checkMaxBuffer = fn_max_windowsize_for_buffer(
        TT_DATA, TP_FIR_LEN, TP_INPUT_WINDOW_VSIZE, TP_API, TP_SSR
    )
    # Input samples are round-robin split to each SSR input paths, so total frame size must be divisable by SSR factor.
    checkIfDivisableBySSR = fn_windowsize_divisible_by_param(
        TP_INPUT_WINDOW_VSIZE, TP_SSR
    )

    #
    checkIfDivisableByTDMChannels = fn_windowsize_divisible_by_param(
        TP_INPUT_WINDOW_VSIZE, TP_TDM_CHANNELS
    )

    for check in (checkMultipleLanes, checkMaxBuffer, checkIfDivisableBySSR, checkIfDivisableByTDMChannels):
        if check["is_valid"] == False:
            return check

    return isValid


#### validate ssr length ####

def fn_fir_len_divisible_ssr(
    TP_FIR_LEN, TP_SSR
):
    # check if divisible by SSR
    if TP_FIR_LEN % TP_SSR != 0:
        return isError(
            f"Filter length ({TP_FIR_LEN}) needs to be divisible by SSR ({TP_SSR})."
        )
    return isValid



def fn_validate_fir_len(
    TT_DATA, TT_COEFF, TP_FIR_LEN, TP_CASC_LEN, TP_SSR, TP_API, TP_USE_COEFF_RELOAD
):
    flenCheck = fn_validate_minmax_value("TP_FIR_LEN", TP_FIR_LEN, TP_FIR_LEN_min, TP_FIR_LEN_max)

    if TT_DATA == "cint16" and TT_COEFF == "int16" and TP_FIR_LEN % 2 == 1:
        # Unsupported FIR length. For cint16 data and int16 coeffs, FIR length must be divisible by 2.
        return isError(
            f"Unsupported TP_FIR_LEN ({TP_FIR_LEN}). Requested TT_DATA ({TT_DATA}) and TT_COEFF ({TT_COEFF}) type combination require TP_FIR_LEN ({TP_FIR_LEN}) to be divisible by 2."
        )

    divCheck = isValid

    minLenCheck = fn_min_fir_len_each_kernel(TP_FIR_LEN, TP_CASC_LEN, TP_SSR)
    symmetryFactor = 1
    maxLenCheck = fn_max_fir_len_each_kernel(TT_DATA, TP_FIR_LEN, TP_CASC_LEN, TP_USE_COEFF_RELOAD, TP_SSR, TP_API, symmetryFactor)
    for check in (flenCheck, divCheck, minLenCheck, maxLenCheck):
        if check["is_valid"] == False:
            return check

    return isValid


def fn_validate_tdm_channels(
    TT_DATA, TT_COEFF, TP_TDM_CHANNELS, TP_SSR, TP_FIR_LEN, TP_CASC_LEN
):
    res = fn_validate_minmax_value("TP_TDM_CHANNELS", TP_TDM_CHANNELS, TP_TDM_CHANNELS_min, TP_TDM_CHANNELS_max)
    if (res["is_valid"] == False):
        return res

    MAX_COEFFS_PER_TILE = 16384 / (fn_size_by_byte(TT_COEFF))
    AUTO_SSR_SPLIT = 1
    if AUTO_SSR_SPLIT == 0:
        COEFFS_PER_TILE = TP_TDM_CHANNELS * TP_FIR_LEN / (TP_CASC_LEN * TP_SSR) /  (fn_size_by_byte(TT_COEFF))
    else:
        # Will be automatically split into multiple AIE tiles, therefore the condition is always met.
        COEFFS_PER_TILE = MAX_COEFFS_PER_TILE

    if MAX_COEFFS_PER_TILE < COEFFS_PER_TILE:
        return isError(
            f"Requested parameters: TP_FIR_LEN ({TP_FIR_LEN}), TDM Channel ({TP_TDM_CHANNELS}) and TP_SSR ({TP_SSR}) result in a kernel that requires {COEFFS_PER_TILE} Coeffs per Tile which exceeds maximum amount of coefficients {MAX_COEFFS_PER_TILE} that can be fit into a single AIE Tile."
            f"Please increase TP_SSR ({TP_SSR})."
        )

    # Ping-pong on AIE1:
    MAX_IO_BUFFER_SAMPLES = 16384 / (fn_size_by_byte(TT_DATA))
    IO_BUFFER_SAMPLES = TP_TDM_CHANNELS * TP_FIR_LEN / (TP_SSR)
    if MAX_IO_BUFFER_SAMPLES < IO_BUFFER_SAMPLES:
        return isError(
            f"Requested parameters: TP_FIR_LEN ({TP_FIR_LEN}), TDM Channel ({TP_TDM_CHANNELS}) and TP_SSR ({TP_SSR}) result in a kernel that requires IO Buffer size of {IO_BUFFER_SAMPLES}  which exceeds size of Memory Group ping-pong buffer of {MAX_IO_BUFFER_SAMPLES} {TT_DATA} samples."
            f"Please increase TP_SSR ({TP_SSR})."
        )

    # Check if channels can equally be spread over SSR paths.
    if ((TP_TDM_CHANNELS % TP_SSR) != 0):
      return isError(
        f"Unsupported TP_TDM_CHANNELS configuration ({TP_TDM_CHANNELS}), which must be a multiple of {TP_SSR}.\n"
      )
    lanes = fnNumLanes(TT_DATA, TT_COEFF, 0)
    # Check if channels fill inner loops completely on each SSR path.
    lanesOnSSR = TP_SSR * lanes
    if (((TP_TDM_CHANNELS / TP_SSR) % lanes) != 0):
      return isError(
        f"Unsupported TP_TDM_CHANNELS ({TP_TDM_CHANNELS}), which - for the requested TP_SSR ({TP_SSR}) - must be a multiple of {lanesOnSSR}. \n"
      )

    return isValid

def fn_validate_tdm_dual_ip(TP_NUM_OUTPUTS, TP_API, TP_DUAL_IP, AIE_VARIANT):

    # Not yet sure what the restrictions will be here.

    if TP_DUAL_IP > 0:
        return isError(
            f"Dual input ports are not supported."
        )

    return isValid
def fn_validate_tdm_num_outputs(TP_NUM_OUTPUTS, TP_API, TP_SSR, AIE_VARIANT):

    # Not yet sure what the restrictions will be here.

    if TP_NUM_OUTPUTS > 1:
        return isError(
            f"Dual output ports are not supported."
        )

    return isValid

#### validation APIs ####
def validate_TT_COEFF(args):
    TT_DATA = args["TT_DATA"]
    TT_COEFF = args["TT_COEFF"]
    return fn_validate_coeff_type(TT_DATA, TT_COEFF)


def validate_TP_INPUT_WINDOW_VSIZE(args):
    TP_INPUT_WINDOW_VSIZE = args["TP_INPUT_WINDOW_VSIZE"]
    TT_DATA = args["TT_DATA"]
    TT_COEFF = args["TT_COEFF"]
    TP_FIR_LEN = args["TP_FIR_LEN"]
    TP_API = args["TP_API"]
    TP_SSR = args["TP_SSR"]
    TP_TDM_CHANNELS = args["TP_TDM_CHANNELS"]
    return fn_validate_input_window_size(
        TT_DATA, TT_COEFF, TP_FIR_LEN, TP_INPUT_WINDOW_VSIZE, TP_API, TP_SSR, TP_TDM_CHANNELS
    )


def validate_TP_DUAL_IP(args):
    TP_NUM_OUTPUTS = args["TP_NUM_OUTPUTS"]
    TP_API = args["TP_API"]
    TP_DUAL_IP = args["TP_DUAL_IP"]
    AIE_VARIANT = args["AIE_VARIANT"]
    return fn_validate_tdm_dual_ip(TP_NUM_OUTPUTS, TP_API, TP_DUAL_IP, AIE_VARIANT)


def validate_TP_NUM_OUTPUTS(args):
    TP_NUM_OUTPUTS = args["TP_NUM_OUTPUTS"]
    TP_API = args["TP_API"]
    TP_SSR = args["TP_SSR"]
    AIE_VARIANT = args["AIE_VARIANT"]
    return fn_validate_tdm_num_outputs(TP_NUM_OUTPUTS, TP_API, TP_SSR, AIE_VARIANT)


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


def validate_TP_SSR(args):
    TP_SSR = args["TP_SSR"]
    return fn_validate_ssr(TP_SSR)


def validate_TP_FIR_LEN(args):
    TT_DATA = args["TT_DATA"]
    TT_COEFF = args["TT_COEFF"]
    TP_FIR_LEN = args["TP_FIR_LEN"]
    TP_SSR = args["TP_SSR"]
    TP_API = args["TP_API"]
    TP_USE_COEFF_RELOAD =  0
    TP_CASC_LEN = 1
    return fn_validate_fir_len(
        TT_DATA, TT_COEFF, TP_FIR_LEN, TP_CASC_LEN, TP_SSR, TP_API, TP_USE_COEFF_RELOAD
    )



def validate_TP_TDM_CHANNELS(args):
    TT_DATA = args["TT_DATA"]
    TT_COEFF = args["TT_COEFF"]
    TP_FIR_LEN = args["TP_FIR_LEN"]
    TP_TDM_CHANNELS = args["TP_TDM_CHANNELS"]
    TP_SSR = args["TP_SSR"]
    TP_CASC_LEN = 1

    return fn_validate_tdm_channels(
        TT_DATA, TT_COEFF, TP_TDM_CHANNELS, TP_SSR, TP_FIR_LEN, TP_CASC_LEN
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
    return {"value": TT_DATA, "enum": [TT_DATA]}


#### port ####


def fn_ceil(m, n):
    return int(((m + n - 1) / n)) * n


def fn_margin_size(TP_FIR_LEN, TT_DATA, TP_TDM_CHANNELS):
    tmpmargin = (int(TP_FIR_LEN) - 1) * fn_size_by_byte(TT_DATA) * TP_TDM_CHANNELS
    return fn_ceil(tmpmargin, 32)


def info_ports(args):
    """Standard function creating a static dictionary of information
    for upper software to correctly connect the IP.
    Some IP has dynamic number of ports according to parameter set,
    so port information has to be implemented as a function"""
    TT_DATA = args["TT_DATA"]
    TP_INPUT_WINDOW_VSIZE = args["TP_INPUT_WINDOW_VSIZE"]
    TP_TDM_CHANNELS = args["TP_TDM_CHANNELS"]
    TP_FIR_LEN = args["TP_FIR_LEN"]
    TP_SSR = args["TP_SSR"]
    TP_API = args["TP_API"]
    TP_DUAL_IP = args["TP_DUAL_IP"]
    TP_NUM_OUTPUTS = args["TP_NUM_OUTPUTS"]
    TP_DECIMATE_FACTOR = 1
    TP_INTERPOLATE_FACTOR = 1

    margin_size = fn_margin_size(TP_FIR_LEN // TP_SSR, TT_DATA, TP_TDM_CHANNELS)
    num_in_ports = TP_SSR
    num_out_ports = TP_SSR

    in_win_size = get_input_window_size(TP_INPUT_WINDOW_VSIZE, num_in_ports, TP_API, TP_DUAL_IP)
    out_win_size = get_output_window_size(TP_INPUT_WINDOW_VSIZE, num_out_ports, TP_API, TP_NUM_OUTPUTS, TP_DECIMATE_FACTOR, TP_INTERPOLATE_FACTOR)

    in_ports = get_port_info( "in", "in", TT_DATA, in_win_size, num_in_ports, marginSize=margin_size, TP_API=TP_API)
    in2_ports = (get_port_info( "in2", "in", TT_DATA, in_win_size, num_in_ports, marginSize=margin_size, TP_API=TP_API, ) if (args["TP_DUAL_IP"] == 1) else [])

    # decimate by 2 for halfband
    out_ports = get_port_info( "out", "out", TT_DATA, out_win_size, num_out_ports, TP_API=TP_API,)
    out2_ports = (get_port_info( "out2", "out", TT_DATA, out_win_size, num_out_ports, TP_API=TP_API, ) if (args["TP_NUM_OUTPUTS"] == 2) else [])
    return in_ports + in2_ports + out_ports + out2_ports


#### graph generator ####
# def get_param_list(**kwargs):
#   [f"{value}{comma}} //{key}" for key, value in kwargs.iteritems() for comma in "," ]

# Returns formatted string with taps
def fn_get_taps_vector(TT_COEFF, coeff_list):

    cplx = fn_is_complex(TT_COEFF)

    # todo, reformat this to use list comprehension
    taps = f"{{"
    # complex pair
    if cplx:
        taps += ", ".join(
            [
                f"{{{coeff_list[2*i]} , {coeff_list[2*i+1]}}}"
                for i in range(int(len(coeff_list) / 2))
            ]
        )
    else:
        taps += ", ".join([str(coeff_list[i]) for i in range(len(coeff_list))])
    taps += f"}}"

    return taps


def generate_graph(graphname, args):

    if graphname == "":
        graphname = "default_graphname"

    TT_COEFF = args["TT_COEFF"]
    TT_DATA = args["TT_DATA"]
    TP_FIR_LEN = args["TP_FIR_LEN"]
    TP_SHIFT = args["TP_SHIFT"]
    TP_RND = args["TP_RND"]
    TP_TDM_CHANNELS = args["TP_TDM_CHANNELS"]
    TP_INPUT_WINDOW_VSIZE = args["TP_INPUT_WINDOW_VSIZE"]
    TP_NUM_OUTPUTS = args["TP_NUM_OUTPUTS"]
    TP_DUAL_IP = args["TP_DUAL_IP"]
    TP_API = args["TP_API"]
    TP_SSR = args["TP_SSR"]
    coeff_list = args["coeff"]
    TP_SAT = args["TP_SAT"]

    taps = fn_get_taps_vector(TT_COEFF, coeff_list)
    dual_ip_declare_str = (
        f"ssr_port_array<input> in2;" if TP_DUAL_IP == 1 else "// No dual input"
    )
    dual_ip_connect_str = (
        f"adf::connect<> net_in2(in2[i], filter.in2[i]);"
        if TP_DUAL_IP == 1
        else "// No dual input"
    )
    dual_op_declare_str = (
        f"ssr_port_array<output> out2;" if TP_NUM_OUTPUTS == 2 else "// No dual output"
    )
    dual_op_connect_str = (
        f"adf::connect<> net_out2(filter.out2[i], out2[i]);"
        if TP_NUM_OUTPUTS == 2
        else "// No dual output"
    )

    # Use formatted multi-line string to avoid a lot of \n and \t
    code = f"""
class {graphname} : public adf::graph {{
public:
  static constexpr unsigned int TP_SSR = {TP_SSR};
  template <typename dir>
  using ssr_port_array = std::array<adf::port<dir>, TP_SSR>;

  ssr_port_array<input> in;
  {dual_ip_declare_str}
  ssr_port_array<output> out;
  {dual_op_declare_str}

  std::vector<{TT_COEFF}> taps = {taps};
  xf::dsp::aie::fir::tdm::fir_tdm_graph<
    {TT_DATA}, //TT_DATA
    {TT_COEFF}, //TT_COEFF
    {TP_FIR_LEN}, //TP_FIR_LEN
    {TP_SHIFT}, //TP_SHIFT
    {TP_RND}, //TP_RND
    {TP_INPUT_WINDOW_VSIZE}, //TP_INPUT_WINDOW_VSIZE
    {TP_TDM_CHANNELS}, //TP_TDM_CHANNELS
    {TP_NUM_OUTPUTS}, //TP_NUM_OUTPUTS
    {TP_DUAL_IP}, //TP_DUAL_IP
    // {TP_API}, //TP_API
    {TP_SSR}, //TP_SSR
    {TP_SAT} //TP_SAT
  > filter;

  {graphname}() : filter({taps}) {{
    adf::kernel *filter_kernels = filter.getKernels();
    for (int i=0; i < 1; i++) {{
      adf::runtime<ratio>(filter_kernels[i]) = 0.9;
    }}
    for (int i=0; i < TP_SSR; i++) {{
      adf::connect<> net_in(in[i], filter.in[i]);
      {dual_ip_connect_str}
      adf::connect<> net_out(filter.out[i], out[i]);
      {dual_op_connect_str}
    }}
  }}

}};
"""
    out = {}
    out["graph"] = code
    out["port_info"] = info_ports(args)
    out["headerfile"] = "fir_tdm_graph.hpp"
    out["searchpaths"] = [
        "L2/include/aie",
        "L2/tests/aie/common/inc",
        "L1/include/aie",
        "L1/src/aie",
        "L1/tests/aie/inc",
        "L1/tests/aie/src",
    ]
    return out
