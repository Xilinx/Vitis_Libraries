from ctypes import sizeof
from socket import TIPC_SUB_SERVICE
from aie_common import *
from aie_common_fir import *
import json
import fir_sr_asym as sr_asym
import math

# script generated file to avoid so much complicated duplication.
from getPhaseAlias import getPhaseAlias
#These can be ignred, as they were mostly development asserts
# fir_resampler.cpp:        static_assert(windowDecPhase[0] % (params.alignWindowReadBytes/params.dataSizeBytes) == 0,"ERROR: Solution doesn't meet alignment requirements. Window decrements must be aligned to 128b boundary. Increase m_kPolyphaseLaneAlias usually solves this. ");
# fir_resampler.cpp:        static_assert(windowDecPhase[m_kPolyphaseLaneAlias-1] % (params.alignWindowReadBytes/params.dataSizeBytes) == 0,"ERROR: Solution doesn't meet alignment requirements. Window decrements must be aligned to 128b boundary. Increase m_kPolyphaseLaneAlias usually solves this. ");
# fir_resampler.cpp:        static_assert(windowDecPhase[0] % (params.alignWindowReadBytes/params.dataSizeBytes) == 0,"ERROR: Solution doesn't meet alignment requirements. Window decrements must be aligned to 128b boundary. Increase m_kPolyphaseLaneAlias usually solves this. ");
# fir_resampler.cpp:        static_assert(windowDecPhase[m_kPolyphaseLaneAlias-1] % (params.alignWindowReadBytes/params.dataSizeBytes) == 0,"ERROR: Solution doesn't meet alignment requirements. Window decrements must be aligned to 128b boundary. Increase m_kPolyphaseLaneAlias usually solves this. ");



# fir_resampler.hpp:        static_assert(TP_INTERPOLATE_FACTOR >= INTERPOLATE_FACTOR_MIN && TP_INTERPOLATE_FACTOR <= INTERPOLATE_FACTOR_MAX,"ERROR: TP_INTERPOLATE_FACTOR is out of the supported range.");
# fir_resampler.hpp:        static_assert(TP_DECIMATE_FACTOR > 0 && TP_DECIMATE_FACTOR <= INTERPOLATE_FACTOR_MAX ,"ERROR: TP_DECIMATE_FACTOR is out of supported range.");
# fir_resampler.hpp:        //static_assert(TP_FIR_LEN <= FIR_LEN_MAX,"ERROR: Max supported FIR length exceeded. ");
# fir_resampler.hpp:        //static_assert(TP_FIR_RANGE_LEN >= FIR_LEN_MIN,"ERROR: Illegal combination of design FIR length and cascade length, resulting in kernel FIR length below minimum required value. ");
# fir_resampler.hpp:        static_assert(TP_SHIFT >= SHIFT_MIN && TP_SHIFT <= SHIFT_MAX, "ERROR: TP_SHIFT is out of the supported range.");
# fir_resampler.hpp:        static_assert(TP_RND >= ROUND_MIN && TP_RND <= ROUND_MAX, "ERROR: TP_RND is out of the supported range.");
# fir_resampler.hpp:        static_assert(fnEnumType<TT_DATA>() != enumUnknownType,"ERROR: TT_DATA is not a supported type.");
# fir_resampler.hpp:        static_assert(fnEnumType<TT_COEFF>() != enumUnknownType,"ERROR: TT_COEFF is not a supported type.");
# fir_resampler.hpp:        //static_assert(fnFirInterpFractTypeSupport<TT_DATA, TT_COEFF>() != 0, "ERROR: This library element currently supports TT_DATA of cint16 and TT_COEFF of int16.");
# fir_resampler.hpp:        static_assert(fnTypeCheckDataCoeffSize<TT_DATA, TT_COEFF>() != 0, "ERROR: TT_DATA type less precise than TT_COEFF is not supported.");
# fir_resampler.hpp:        static_assert(fnTypeCheckDataCoeffCmplx<TT_DATA, TT_COEFF>() != 0, "ERROR: real TT_DATA with complex TT_COEFF is not supported.");
# fir_resampler.hpp:        static_assert(fnTypeCheckDataCoeffFltInt<TT_DATA, TT_COEFF>() != 0, "ERROR: a mix of float and integer types of TT_DATA and TT_COEFF is not supported.");
# fir_resampler.hpp:        // This is a constexpr that we use to trigger the static_assert and is only used in the UUT, but also gives us an error message that we need to automatically translate to python.
# fir_resampler.hpp:        static_assert(inputWindowVSizeIntegerOutputSize(windowSize, interpolateFactor, decimateFactor), "ERROR: Output Window Size must be an integer given the input window size and decimate factor");
# fir_resampler.hpp:        static_assert(( is_everything_valid<this_fir_params>()), "Configuration Error");
# fir_resampler.hpp:        static_assert((((TP_INPUT_WINDOW_VSIZE*sizeof(TT_DATA)) % (128/8)) == 0), "Number of input samples must align to 128 bits.");
# fir_resampler.hpp:        static_assert(TP_NUM_OUTPUTS >0 && TP_NUM_OUTPUTS <=2, "ERROR: only single or dual outputs are supported." );
# fir_resampler.hpp:        static_assert(!(std::is_same<TT_DATA,cfloat>::value || std::is_same<TT_DATA,float>::value) || (TP_SHIFT == 0), "ERROR: TP_SHIFT cannot be performed for TT_DATA=cfloat, so must be set to 0");
# fir_resampler.hpp:        static_assert(sizeof(TT_DATA)*m_kLanes <= m_kZbuffSize, "ERROR: Invalid assumption in archtecture. Can't fit enough data into selected (Z) buffer.");
# fir_resampler.hpp:        static_assert(m_kNumSamplesRequiredForNLanes + m_kColumns-1 <= kXYBuffSize, "ERROR: the overall decimation rate requires more samples than can be fit within the vector register.");
# fir_resampler.hpp:        static_assert(m_kNumSamplesRequiredForNLanes <= 16, "ERROR: the overall decimation rate requires more integer samples than can be indexed within the vector register (xoffsets up to 16 samples).");
# fir_resampler.hpp:        static_assert(!(m_kCFloatDataType && m_kNumSamplesRequiredForNLanes > 8), "ERROR: the overall decimation rate requires more complex floating-point data samples than can be indexed within the vector register (xoffsets up to 8 samples).");
# fir_resampler.hpp:        static_assert(m_kNumOutputs%(m_kVOutSize) == 0, "ERROR: output window size must be a multiple of number of lanes. ");
# fir_resampler.hpp:        static_assert(m_kNumOutputs%(m_kPolyphaseLaneAlias*m_kVOutSize) == 0, "ERROR: due to architectural optimisation, this window size is not currently supported. Please use a TP_INPUT_WINDOW_VSIZE that will give a number of output samples which is a multiple of Lanes and m_kPolyphaseLaneAlias.");
# fir_resampler.hpp:        static_assert(!(m_kArch == kArchStream &&  m_kInitDataNeeded > m_kSamplesInBuff), "ERROR: TP_FIR_RANGE_LEN exceeds max supported range for this data/coeff type combination. Increase TP_CASC_LEN to split the workload over more kernels.");
# fir_resampler.hpp:        static_assert(!(m_kArch == kArchStream &&  m_kLsize % m_kRepeatFactor != 0), "ERROR: For optimal design, inner loop size must schedule multiple iterations of vector operations. Please use a TP_INPUT_WINDOW_VSIZE that results in a m_kLsize being a multiple of m_kRepeatFactor.");

# fir_resampler_graph.hpp:  static_assert(TP_CASC_LEN <= 40,"ERROR: Unsupported Cascade length");
# fir_resampler_graph.hpp:  static_assert(TP_API == USE_STREAM_API || TP_DUAL_IP == DUAL_IP_SINGLE,"ERROR: Dual input ports only supported when port API is a stream. ");
# fir_resampler_graph.hpp:  static_assert(TP_FIR_LEN / TP_CASC_LEN <= kMaxTapsPerKernel,"ERROR: Requested FIR length and Cascade length exceeds supported number of taps per kernel. Please increase the cascade legnth to accomodate the FIR design.");
# fir_resampler_graph.hpp:  static_assert(TP_USE_COEFF_RELOAD == 0 || TP_FIR_LEN  <= kMaxTapsPerKernel,"ERROR: Exceeded maximum supported FIR length with reloadable coefficients. Please limit the FIR length or disable coefficient reload.");
# fir_resampler_graph.hpp:  static_assert(TP_API != 0 || inBufferSize < kMemoryModuleSize, "ERROR: Input Window size (based on requrested window size and FIR length margin) exceeds Memory Module size of 32kB");
# fir_resampler_graph.hpp:  static_assert(TP_API != 0 || outBufferSize < kMemoryModuleSize, "ERROR: Input Window size (based on requrested window size and FIR length margin) exceeds Memory Module size of 32kB");


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

def fn_check_samples_can_fit_streaming(TT_DATA, TT_COEF, TP_FIR_LEN, TP_INTERPOLATE_FACTOR, TP_DECIMATE_FACTOR, TP_CASC_LEN, TP_API):

  m_kWinAccessByteSize = 128//8 # fixed


  m_kDataBuffXOffset   = (m_kWinAccessByteSize/fn_size_by_byte(TT_DATA)) -1 # Let's just maximally size this to avoid needing so much duplicated code.

  m_kLanes = fnNumLanes(TT_DATA, TT_COEF, TP_API)
  m_kNumSamplesRequiredForNLanes = (m_kLanes * TP_DECIMATE_FACTOR + (TP_INTERPOLATE_FACTOR - 1)) / TP_INTERPOLATE_FACTOR

  m_kSamplesInBuff     = (1024//8)//fn_size_by_byte(TT_DATA)

  if TP_API != 0: #stream
    for kernelPos in range(TP_CASC_LEN):
      TP_FIR_RANGE_LEN =  (
        fnFirRangeRem(TP_FIR_LEN,TP_CASC_LEN,kernelPos,TP_INTERPOLATE_FACTOR)
          if (kernelPos == (TP_CASC_LEN-1))
          else
            fnFirRange(TP_FIR_LEN,TP_CASC_LEN,kernelPos,TP_INTERPOLATE_FACTOR)
      )
      m_kPolyLen =  (TP_FIR_RANGE_LEN+TP_INTERPOLATE_FACTOR-1)//TP_INTERPOLATE_FACTOR
      m_kInitDataNeeded    = m_kDataBuffXOffset + m_kPolyLen + m_kNumSamplesRequiredForNLanes - 1
      if m_kInitDataNeeded > m_kSamplesInBuff:
        return isError(f"Filter length per kernel ({TP_FIR_RANGE_LEN}) for kernel {kernelPos} exceeds max supported range for this data/coeff type combination ({TT_DATA}/{TT_COEF}). Increase cascade length to split the workload over more kernels. ")

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
    isError(f"Max supported Filter length (Filter length = {TP_FIR_LEN} > Max = {maxTaps[(TT_DATA, TT_COEF)]}) exceeded for data type and coefficient type combination {TT_DATA},{TT_COEF}.")
      if TP_FIR_LEN > maxTaps[(TT_DATA, TT_COEF)]
      else isValid
  )

def fn_validate_fir_len(TT_DATA, TT_COEF, TP_FIR_LEN, TP_INTERPOLATE_FACTOR,TP_DECIMATE_FACTOR, TP_CASC_LEN, TP_SSR, TP_API, TP_USE_COEF_RELOAD):
  minLenCheck =  fn_min_fir_len_each_kernel(TP_FIR_LEN, TP_CASC_LEN, TP_SSR, TP_Rnd=TP_INTERPOLATE_FACTOR)

  maxLenCheck = fn_max_fir_len_each_kernel(TP_FIR_LEN, TP_CASC_LEN, TP_USE_COEF_RELOAD, TP_SSR, 1)

  maxLenOverallCheck = fn_max_fir_len_overall(TT_DATA, TT_COEF, TP_FIR_LEN)

  streamingVectorRegisterCheck = fn_check_samples_can_fit_streaming(TT_DATA, TT_COEF, TP_FIR_LEN, TP_INTERPOLATE_FACTOR, TP_DECIMATE_FACTOR, TP_CASC_LEN, TP_API)

  for check in (minLenCheck,maxLenCheck,maxLenOverallCheck, streamingVectorRegisterCheck):
    if check["is_valid"] == False :
      return check

  return isValid


def fn_type_support(TT_DATA, TT_COEF):
  return isError(f"The combination of {TT_DATA} and {TT_COEF} is not supported for this class.") if (TT_DATA == "int16" and TT_COEF == "int16") else isValid

def my_lcm(a,b):
  return abs(a*b) // math.gcd(a, b)

def fnStreamReadWidth(TT_DATA, TT_COEF):
  typesFor256b = [
    ("int32", "int16"),
    ("cint32", "int16"),
    ("cint32", "int32"),
    ("cint32", "cint16"),
    ("float", "float"),
    ("cfloat", "float"),
    ("cfloat", "cfloat")
  ]
  return 256 if ((TT_DATA, TT_COEF) in typesFor256b) else 128


def fn_check_repeatFactor(TT_DATA, TT_COEF, TP_INTERPOLATE_FACTOR, TP_DECIMATE_FACTOR, TP_INPUT_WINDOW_VSIZE, TP_API):
  m_kVOutSize = fnNumLanes(TT_DATA, TT_COEF, TP_API)
  m_kPolyphaseLaneAlias = getPhaseAlias(TT_DATA, TT_COEF, TP_INTERPOLATE_FACTOR, TP_DECIMATE_FACTOR, TP_API)
  m_kNumOutputs = ((TP_INPUT_WINDOW_VSIZE * TP_INTERPOLATE_FACTOR) / TP_DECIMATE_FACTOR)
  m_kLsize = (
        m_kNumOutputs /
        (m_kPolyphaseLaneAlias *
         m_kVOutSize)
  )

  m_kRepeatFactor = 16 # no comments or anything about this hardcoded number...

  if (TP_API == 1 and m_kLsize % m_kRepeatFactor != 0):
    isError("For optimal design, inner loop size must schedule multiple iterations of vector operations. Please use a Input window size that results in a m_kLsize being a multiple of m_kRepeatFactor.")
  return isValid


# will need to divide window size by SSR once this is incorporated.
def fn_validate_input_window_size(TT_DATA, TT_COEF, TP_FIR_LEN,TP_INTERPOLATE_FACTOR,TP_DECIMATE_FACTOR, TP_INPUT_WINDOW_VSIZE, TP_API, TP_SSR=1):
  # resampler doesn't actually have this constraint
  #checkMultipleLanes =  fn_windowsize_multiple_lanes(TT_DATA, TT_COEF, TP_INPUT_WINDOW_VSIZE, TP_API)
  outputWindowSize = (TP_INPUT_WINDOW_VSIZE * TP_INTERPOLATE_FACTOR) // TP_DECIMATE_FACTOR
  checkOutputMultipleLanes =  fn_windowsize_multiple_lanes(TT_DATA, TT_COEF, outputWindowSize, TP_API)
  m_kPolyphaseLaneAlias = getPhaseAlias(TT_DATA, TT_COEF, TP_INTERPOLATE_FACTOR, TP_DECIMATE_FACTOR, TP_API)
  multipleToBeChecked = m_kPolyphaseLaneAlias*fnNumLanes(TT_DATA, TT_COEF, TP_API)
  checkOutputMultipleLanesAndLaneAlias =  fn_windowsize_multiple_lanes(TT_DATA, TT_COEF, outputWindowSize, TP_API, numLanes=multipleToBeChecked)


  checkMaxBuffer = fn_max_windowsize_for_buffer(TT_DATA, TP_FIR_LEN, TP_INPUT_WINDOW_VSIZE, TP_API, TP_SSR, TP_INTERPOLATE_FACTOR, TP_DECIMATE_FACTOR)
  # Input samples are round-robin split to each SSR input paths, so total frame size must be divisable by SSR factor.
  checkIfDivisableBySSR = fn_windowsize_divisible_by_ssr(TP_INPUT_WINDOW_VSIZE, TP_SSR)

  checkIntegerOutputWindow = (
    isError("Output Window Size must be an integer given the input window size, interpolate factor and decimate factor")
    if (TP_INPUT_WINDOW_VSIZE * TP_INTERPOLATE_FACTOR) % TP_DECIMATE_FACTOR != 0 else isValid
  )

  checkAlignment128b = (
    isError("Number of input samples must align to 128 bits.")
    if ((TP_INPUT_WINDOW_VSIZE * fn_size_by_byte(TT_DATA)) % (128//8)) else isValid
  )
  checkRepeatFactor = fn_check_repeatFactor(TT_DATA, TT_COEF, TP_INTERPOLATE_FACTOR, TP_DECIMATE_FACTOR, TP_INPUT_WINDOW_VSIZE, TP_API)
  for check in (checkOutputMultipleLanes, checkOutputMultipleLanesAndLaneAlias, checkMaxBuffer, checkIfDivisableBySSR, checkIntegerOutputWindow, checkAlignment128b, checkRepeatFactor):
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
  TP_INPUT_WINDOW_VSIZE = args["TP_INPUT_WINDOW_VSIZE"]
  TT_DATA = args["TT_DATA"]
  TT_COEF = args["TT_COEF"]
  TP_FIR_LEN = args["TP_FIR_LEN"]
  TP_API = args["TP_API"]
  TP_INTERPOLATE_FACTOR = args["TP_INTERPOLATE_FACTOR"]
  TP_DECIMATE_FACTOR = args["TP_DECIMATE_FACTOR"]
  TP_SSR = 1

  #interpolate_hb traits looks like the UPSHIFT_CT types have different number of lanes, but it's actually stil the exact same as 384..
  # decimate_hb also just uses 384, so no additional rules here.
  return fn_validate_input_window_size(TT_DATA, TT_COEF, TP_FIR_LEN, TP_INTERPOLATE_FACTOR, TP_DECIMATE_FACTOR, TP_INPUT_WINDOW_VSIZE, TP_API, TP_SSR)


def validate_TP_FIR_LEN(args):
  TT_DATA = args["TT_DATA"]
  TT_COEF = args["TT_COEF"]
  TP_FIR_LEN = args["TP_FIR_LEN"]
  TP_CASC_LEN = args["TP_CASC_LEN"]
  TP_INTERPOLATE_FACTOR = args["TP_INTERPOLATE_FACTOR"]
  TP_DECIMATE_FACTOR = args["TP_DECIMATE_FACTOR"]
  TP_SSR = 1
  TP_API = args["TP_API"]
  TP_USE_COEF_RELOAD = args["TP_USE_COEF_RELOAD"]

  return fn_validate_fir_len(TT_DATA, TT_COEF, TP_FIR_LEN, TP_INTERPOLATE_FACTOR,TP_DECIMATE_FACTOR, TP_CASC_LEN, TP_SSR, TP_API, TP_USE_COEF_RELOAD)

def fn_validate_dual_ip(TP_API, TP_DUAL_IP):
  if TP_DUAL_IP == 1 and TP_API == 0:
    return isError("Dual input ports only supported when port is a stream.")
  return isValid


def validate_TP_DUAL_IP(args):
  TP_API = args["TP_API"]
  TP_DUAL_IP = args["TP_DUAL_IP"]
  return fn_validate_dual_ip(TP_API, TP_DUAL_IP)

def fn_validate_decimate_factor(TT_DATA, TT_COEF,TP_INTERPOLATE_FACTOR,TP_DECIMATE_FACTOR):

  m_kNumSamplesForNLanes = 0
  m_kColumns = 0
  kXYBuffSize = 0
  buffIndexLimit = 16 if TT_DATA != "cfloat" else 8 # 4b range, unless cfloat

  checkBuffSize = (
    isError(f"The overall decimation rate ({TP_DECIMATE_FACTOR/TP_INTERPOLATE_FACTOR}) requires more samples ({m_kNumSamplesForNLanes}) than can be fit within the vector register ({kXYBuffSize}).")
    if ((m_kNumSamplesForNLanes + (m_kColumns-1)) > kXYBuffSize) else isValid
  )

  checkBuffIndexing = (
    isError(f"The overall decimation rate ({TP_DECIMATE_FACTOR/TP_INTERPOLATE_FACTOR}) requires more samples ({m_kNumSamplesForNLanes}) than can be indexed within the vector register ({buffIndexLimit}).")
    if (m_kNumSamplesForNLanes > buffIndexLimit) else isValid
  )

  for check in (checkBuffSize,checkBuffIndexing):
    if check["is_valid"] == False :
      return check


  return isValid

def validate_TP_DECIMATE_FACTOR(args):
  TT_DATA = args["TT_DATA"]
  TT_COEF = args["TT_COEF"]
  TP_INTERPOLATE_FACTOR = args["TP_INTERPOLATE_FACTOR"]
  TP_DECIMATE_FACTOR = args["TP_DECIMATE_FACTOR"]
  return fn_validate_decimate_factor(TT_DATA, TT_COEF,TP_INTERPOLATE_FACTOR,TP_DECIMATE_FACTOR)




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
    TP_SSR = 1
    TP_INTERPOLATE_FACTOR = args["TP_INTERPOLATE_FACTOR"]
    TP_DECIMATE_FACTOR = args["TP_DECIMATE_FACTOR"]
    margin_size = sr_asym.fn_margin_size((TP_FIR_LEN+TP_INTERPOLATE_FACTOR-1)//TP_INTERPOLATE_FACTOR, TT_DATA)

    in_ports = get_port_info("in", "in", TT_DATA, TP_INPUT_WINDOW_VSIZE//TP_SSR, TP_SSR, marginSize=margin_size, TP_API=args["TP_API"])
    in2_ports = (get_port_info("in2", "in", TT_DATA, TP_INPUT_WINDOW_VSIZE//TP_SSR, TP_SSR, marginSize=margin_size, TP_API=args["TP_API"]) if (args["TP_DUAL_IP"] == 1) else [])
    coeff_ports = (get_parameter_port_info("coeff", "in", TT_COEF, TP_SSR, TP_FIR_LEN, "async") if (args["TP_USE_COEF_RELOAD"] == 1) else [])

    # interp by 2 for halfband
    out_ports = get_port_info("out", "out", TT_DATA, (TP_INPUT_WINDOW_VSIZE*TP_INTERPOLATE_FACTOR)//TP_SSR//TP_DECIMATE_FACTOR, TP_SSR, TP_API=args["TP_API"])
    out2_ports = (get_port_info("out2", "out", TT_DATA, (TP_INPUT_WINDOW_VSIZE*TP_INTERPOLATE_FACTOR)//TP_SSR//TP_DECIMATE_FACTOR, TP_SSR, TP_API=args["TP_API"]) if (args["TP_NUM_OUTPUTS"] == 2) else [])
    return in_ports + in2_ports + coeff_ports + out_ports + out2_ports


#### graph generator ####
# def get_param_list(**kwargs):
#  print("get_param_list")
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
  TP_DECIMATE_FACTOR = args["TP_DECIMATE_FACTOR"]
  TP_USE_COEF_RELOAD = args["TP_USE_COEF_RELOAD"]
  TP_NUM_OUTPUTS = args["TP_NUM_OUTPUTS"]
  TP_DUAL_IP = args["TP_DUAL_IP"]
  TP_API = args["TP_API"]
  TP_SSR = 1
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
  xf::dsp::aie::fir::resampler::fir_resampler_graph<
    {TT_DATA}, //TT_DATA
    {TT_COEF}, //TT_COEF
    {TP_FIR_LEN}, //TP_FIR_LEN
    {TP_INTERPOLATE_FACTOR}, //TP_INTERPOLATE_FACTOR
    {TP_DECIMATE_FACTOR}, //TP_DECIMATE_FACTOR
    {TP_SHIFT}, //TP_SHIFT
    {TP_RND}, //TP_RND
    {TP_INPUT_WINDOW_VSIZE}, //TP_INPUT_WINDOW_VSIZE
    {TP_CASC_LEN}, //TP_CASC_LEN
    {TP_USE_COEF_RELOAD}, //TP_USE_COEF_RELOAD
    {TP_NUM_OUTPUTS}, //TP_NUM_OUTPUTS
    {TP_DUAL_IP}, //TP_DUAL_IP
    {TP_API} //TP_API
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
  out["headerfile"] = "fir_resampler_graph.hpp"
  out["searchpaths"] = [
       "L2/include/aie",
       "L2/tests/aie/common/inc",
       "L1/include/aie",
       "L1/src/aie",
       "L1/tests/aie/inc",
       "L1/tests/aie/src"
  ]

  return out

print("finished")
