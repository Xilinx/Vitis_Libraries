from ctypes import sizeof
from socket import TIPC_SUB_SERVICE
from aie_common import *
from aie_common_fir import *
import json
import fir_sr_asym as sr_asym
import math
import fir_polyphase_decomposer as poly
# script generated file to avoid so much complicated duplication.
from getPhaseAlias import getPhaseAlias


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


TP_DECIMATE_FACTOR_min = 1
TP_DECIMATE_FACTOR_max = 16
TP_INTERPOLATE_FACTOR_min = 1
TP_INTERPOLATE_FACTOR_max = 16
TP_INPUT_WINDOW_VSIZE_min = 4
TP_SSR_min = 1
TP_PARA_DECI_POLY_min = 1
TP_PARA_INTERP_POLY_min = 1
TP_CASC_LEN_min = 1
TP_CASC_LEN_max = 40
TP_FIR_LEN_min = 4
TP_FIR_LEN_max = 8192
TP_SHIFT_min=0
TP_SHIFT_max=80

def fn_check_samples_can_fit_streaming(
    TT_DATA,
    TT_COEFF,
    TP_FIR_LEN,
    TP_INTERPOLATE_FACTOR,
    TP_DECIMATE_FACTOR,
    TP_CASC_LEN,
    TP_API,
):

    m_kWinAccessByteSize = 128 // 8  # fixed

    m_kDataBuffXOffset = (
        m_kWinAccessByteSize / fn_size_by_byte(TT_DATA)
    ) - 1  # Let's just maximally size this to avoid needing so much duplicated code.

    m_kLanes = fnNumLanes(TT_DATA, TT_COEFF, TP_API)
    m_kNumSamplesRequiredForNLanes = (
        m_kLanes * TP_DECIMATE_FACTOR + (TP_INTERPOLATE_FACTOR - 1)
    ) / TP_INTERPOLATE_FACTOR

    m_kSamplesInBuff = (1024 // 8) // fn_size_by_byte(TT_DATA)

    if TP_API != 0:  # stream
        for kernelPos in range(TP_CASC_LEN):
            TP_FIR_RANGE_LEN = (
                fnFirRangeRem(TP_FIR_LEN, TP_CASC_LEN, kernelPos, TP_INTERPOLATE_FACTOR)
                if (kernelPos == (TP_CASC_LEN - 1))
                else fnFirRange(
                    TP_FIR_LEN, TP_CASC_LEN, kernelPos, TP_INTERPOLATE_FACTOR
                )
            )
            m_kPolyLen = (
                TP_FIR_RANGE_LEN + TP_INTERPOLATE_FACTOR - 1
            ) // TP_INTERPOLATE_FACTOR
            m_kInitDataNeeded = (
                m_kDataBuffXOffset + m_kPolyLen + m_kNumSamplesRequiredForNLanes - 1
            )
            if m_kInitDataNeeded > m_kSamplesInBuff:
                return isError(
                    f"Filter length per kernel ({TP_FIR_RANGE_LEN}) for kernel {kernelPos} exceeds max supported range for this data/coeff type combination ({TT_DATA}/{TT_COEFF}). Increase cascade length to split the workload over more kernels. "
                )

    return isValid


# Values are derived from experimentation and are a factor of program memory limits, memory module sizes etc.
def fn_max_fir_len_overall(TT_DATA, TT_COEFF, TP_FIR_LEN):
    maxTaps = {
        ("int16", "int16"): 4096,
        ("cint16", "int16"): 4096,
        ("cint16", "cint16"): 2048,
        ("int32", "int16"): 4096,
        ("int32", "int32"): 2048,
        ("int16", "int32"): 2048,
        ("cint16", "int32"): 2048,
        ("cint16", "cint32"): 1024,
        ("cint32", "int16"): 2048,
        ("cint32", "cint16"): 2048,
        ("cint32", "int32"): 2048,
        ("cint32", "cint32"): 1024,
        ("float", "float"): 2048,
        ("cfloat", "float"): 2048,
        ("cfloat", "cfloat"): 1024,
    }
    return (
        isError(
            f"Max supported Filter length (Filter length = {TP_FIR_LEN} > Max = {maxTaps[(TT_DATA, TT_COEFF)]}) exceeded for data type and coefficient type combination {TT_DATA},{TT_COEFF}."
        )
        if TP_FIR_LEN > maxTaps[(TT_DATA, TT_COEFF)]
        else isValid
    )


def fn_validate_fir_len(
    args,
    TT_DATA,
    TT_COEFF,
    TP_FIR_LEN,
    TP_INTERPOLATE_FACTOR,
    TP_DECIMATE_FACTOR,
    TP_CASC_LEN,
    TP_SSR,
    TP_API,
    TP_USE_COEFF_RELOAD,
    AIE_VARIANT,
    TP_PARA_DECI_POLY,
    TP_PARA_INTERP_POLY
):
    check_valid_decompose = poly.fn_validate_decomposer_TP_FIR_LEN(args)
    res = fn_validate_minmax_value("TP_FIR_LEN", TP_FIR_LEN, TP_FIR_LEN_min, TP_FIR_LEN_max)
    if (res["is_valid"] == False):
        return res

    minLenCheck = fn_min_fir_len_each_kernel(
        TP_FIR_LEN, TP_CASC_LEN, TP_SSR, TP_Rnd=TP_INTERPOLATE_FACTOR
    )
    if AIE_VARIANT == 2:
        if (TP_FIR_LEN / TP_CASC_LEN) <  (TP_INTERPOLATE_FACTOR*TP_DECIMATE_FACTOR):
            return isError(
                f"FIR computation is decomposed into multiple (interpolation * decimation factors) parallel polyphases. Make sure that FIR length {TP_FIR_LEN} is greater or equal to TP_INTERPOLATE_FACTOR {TP_INTERPOLATE_FACTOR} * TP_DECIMATE_FACTOR {TP_DECIMATE_FACTOR} * TP_CASC_LEN {TP_CASC_LEN}."
            )
        if (TP_FIR_LEN / TP_CASC_LEN) % (TP_INTERPOLATE_FACTOR) != 0:
            return isError(
                f"FIR Length for each kernel must be a multiple of interpolation factor. Make sure that FIR length {TP_FIR_LEN} is a multiple of TP_INTERPOLATE_FACTOR {TP_INTERPOLATE_FACTOR} * TP_CASC_LEN {TP_CASC_LEN}."
            )


    coeffSizeMult = 1 if TP_API == 0 else TP_INTERPOLATE_FACTOR

    maxLenCheck = fn_max_fir_len_each_kernel(
        TT_DATA,
        TP_FIR_LEN,
        TP_CASC_LEN,
        TP_USE_COEFF_RELOAD,
        TP_SSR,
        TP_API,
        coeffSizeMult,
    )

    maxLenOverallCheck = fn_max_fir_len_overall(TT_DATA, TT_COEFF, TP_FIR_LEN)

    streamingVectorRegisterCheck = fn_check_samples_can_fit_streaming(
        TT_DATA,
        TT_COEFF,
        TP_FIR_LEN,
        TP_INTERPOLATE_FACTOR,
        TP_DECIMATE_FACTOR,
        TP_CASC_LEN,
        TP_API,
    )

    for check in (
        check_valid_decompose,
        minLenCheck,
        maxLenCheck,
        maxLenOverallCheck,
        streamingVectorRegisterCheck,
    ):
        if check["is_valid"] == False:
            return check

    return isValid

def my_lcm(a, b):
    return abs(a * b) // math.gcd(a, b)


def fnStreamReadWidth(TT_DATA, TT_COEFF):
    typesFor256b = [
        ("int32", "int16"),
        ("cint32", "int16"),
        ("cint32", "int32"),
        ("cint32", "cint16"),
        ("float", "float"),
        ("cfloat", "float"),
        ("cfloat", "cfloat"),
    ]
    return 256 if ((TT_DATA, TT_COEFF) in typesFor256b) else 128


def fn_check_repeatFactor(
    TT_DATA,
    TT_COEFF,
    TP_INTERPOLATE_FACTOR,
    TP_DECIMATE_FACTOR,
    TP_INPUT_WINDOW_VSIZE,
    TP_API,
):
    m_kVOutSize = fnNumLanes(TT_DATA, TT_COEFF, TP_API)
    m_kPolyphaseLaneAlias = getPhaseAlias(
        TT_DATA, TT_COEFF, TP_INTERPOLATE_FACTOR, TP_DECIMATE_FACTOR, TP_API
    )
    m_kNumOutputs = (TP_INPUT_WINDOW_VSIZE * TP_INTERPOLATE_FACTOR) / TP_DECIMATE_FACTOR
    m_kLsize = m_kNumOutputs / (m_kPolyphaseLaneAlias * m_kVOutSize)

    # 128/256/384 bits of data may be needed for loops with full
    # permute option, requiring up to 8 repeated iteratios to go through full data
    # buffer.
    m_kRepeatFactor = 8

    if TP_API == 1 and m_kLsize % m_kRepeatFactor != 0:
        isError(
            "For optimal design, inner loop size must schedule multiple iterations of vector operations. Please use a Input window size that results in a m_kLsize being a multiple of m_kRepeatFactor."
        )
    return isValid


# will need to divide window size by SSR once this is incorporated.
def fn_validate_input_window_size(
    TT_DATA,
    TT_COEFF,
    TP_FIR_LEN,
    TP_INTERPOLATE_FACTOR,
    TP_DECIMATE_FACTOR,
    TP_INPUT_WINDOW_VSIZE,
    TP_API,
    TP_SSR=1,
    TP_PARA_INTERP_POLY=1,
    TP_PARA_DECI_POLY=1,
    AIE_VARIANT=1,
):
    res = fn_validate_min_value("TP_INPUT_WINDOW_VSIZE", TP_INPUT_WINDOW_VSIZE, TP_INPUT_WINDOW_VSIZE_min)
    if (res["is_valid"] == False):
      return res
    if TP_INPUT_WINDOW_VSIZE/(TP_PARA_DECI_POLY * TP_PARA_INTERP_POLY) < TP_INPUT_WINDOW_VSIZE_min:
        return isError(
            f"Minimum value for Input size is {TP_INPUT_WINDOW_VSIZE_min}, but got {TP_INPUT_WINDOW_VSIZE/(TP_PARA_DECI_POLY * TP_PARA_INTERP_POLY)} which resulted by decomposing requested Input size {TP_INPUT_WINDOW_VSIZE} into {(TP_PARA_DECI_POLY * TP_PARA_INTERP_POLY)} parallel polyphases ."
        )
    m_kPolyphaseLaneAlias = getPhaseAlias(
        TT_DATA, TT_COEFF, TP_INTERPOLATE_FACTOR, TP_DECIMATE_FACTOR, TP_API
    )
    # Stream repeat factor is set to 8, to allow unrolling and effective pipelining.
    streamRptFactor = 8
    numLanes = fnNumLanes(TT_DATA, TT_COEFF, TP_API, AIE_VARIANT)
    if AIE_VARIANT == 1 :
        multipleToBeChecked = (
            (m_kPolyphaseLaneAlias * numLanes)
            if TP_API == 0
            else (
                m_kPolyphaseLaneAlias
                * numLanes
                * streamRptFactor
            )
        )
    if AIE_VARIANT == 2 :
        # AIE-ML decpomposes to
        multipleToBeChecked = numLanes

    inputWindowSize = (
        (TP_INPUT_WINDOW_VSIZE / TP_PARA_DECI_POLY)
    )
    if TP_PARA_DECI_POLY >  TP_PARA_DECI_POLY_min:
        print(f"INFO: Input Samples are equally split between Decimate Polyphases {TP_PARA_DECI_POLY}, resulting in Input Window Size per polyphase equal to ({inputWindowSize})." )

    checkInputDividedBySSR = fn_windowsize_multiple_lanes(
        TT_DATA, TT_COEFF, TP_INPUT_WINDOW_VSIZE, TP_API, numLanes, TP_SSR * TP_PARA_DECI_POLY, AIE_VARIANT
    )

    checkOutputMultipleLanes = fn_out_windowsize_multiple_lanes(
        TT_DATA, TT_COEFF, TP_INPUT_WINDOW_VSIZE, TP_API, multipleToBeChecked, TP_SSR, TP_INTERPOLATE_FACTOR, TP_DECIMATE_FACTOR, AIE_VARIANT
    )


    checkMaxBuffer = fn_max_windowsize_for_buffer(
        TT_DATA,
        TP_FIR_LEN,
        TP_INPUT_WINDOW_VSIZE,
        TP_API,
        TP_SSR,
        TP_INTERPOLATE_FACTOR,
        TP_DECIMATE_FACTOR,
        AIE_VARIANT
    )
    # Input samples are round-robin split to each SSR input paths, so total frame size must be divisable by SSR factor.
    checkIfDivisableBySSR = fn_windowsize_divisible_by_param(
        TP_INPUT_WINDOW_VSIZE, TP_SSR
    )

    checkIntegerOutputWindow = (
        isError(
            "Output Window Size must be an integer given the input window size, interpolate factor and decimate factor"
        )
        if (TP_INPUT_WINDOW_VSIZE * TP_INTERPOLATE_FACTOR) % TP_DECIMATE_FACTOR != 0
        else isValid
    )

    checkAlignment128b = (
        isError("Number of input samples must align to 128 bits.")
        if ((TP_INPUT_WINDOW_VSIZE * fn_size_by_byte(TT_DATA)) % (128 // 8))
        else isValid
    )
    if AIE_VARIANT == 1 :

        checkRepeatFactor = fn_check_repeatFactor(
            TT_DATA,
            TT_COEFF,
            TP_INTERPOLATE_FACTOR,
            TP_DECIMATE_FACTOR,
            TP_INPUT_WINDOW_VSIZE,
            TP_API,
        )
    if AIE_VARIANT == 2:
        #Ignore RepeatFactor check for AIE-ML device.
        checkRepeatFactor = isValid

    for check in (
        checkInputDividedBySSR,
        checkOutputMultipleLanes,
        checkMaxBuffer,
        checkIfDivisableBySSR,
        checkIntegerOutputWindow,
        checkAlignment128b,
        checkRepeatFactor,
    ):
        if check["is_valid"] == False:
            return check

    return isValid

def fn_validate_para_interp_poly(TP_INTERPOLATE_FACTOR, TP_PARA_INTERP_POLY):
    res = fn_validate_min_value("TP_PARA_INTERP_POLY", TP_PARA_INTERP_POLY, TP_PARA_INTERP_POLY_min)
    if (res["is_valid"] == False):
      return res
    if TP_PARA_INTERP_POLY == TP_INTERPOLATE_FACTOR or TP_PARA_INTERP_POLY == 1:
        return isValid
    else:
        return isError(
            f"Polyphase decomposition supports only full decomposition, where number of interpolation polyphases {TP_PARA_INTERP_POLY} must be equal to interpolation factor {TP_INTERPOLATE_FACTOR}."
        )


def fn_validate_para_deci_poly(TP_DECIMATE_FACTOR, TP_PARA_DECI_POLY):
    res = fn_validate_min_value("TP_PARA_DECI_POLY", TP_PARA_DECI_POLY, TP_PARA_DECI_POLY_min)
    if (res["is_valid"] == False):
      return res
    if TP_PARA_DECI_POLY == TP_DECIMATE_FACTOR or TP_PARA_DECI_POLY == 1:
        return isValid
    else:
        return isError(
            f"Polyphase decomposition supports only full decomposition, where number of decimation polyphases {TP_PARA_DECI_POLY} must be equal to decimation factor {TP_DECIMATE_FACTOR}."
        )


def fn_validate_casc_len(TP_CASC_LEN):
    return fn_validate_minmax_value("TP_CASC_LEN", TP_CASC_LEN, TP_CASC_LEN_min, TP_CASC_LEN_max)


#### validation APIs ####
def validate_TT_COEFF(args):
    TT_DATA = args["TT_DATA"]
    TT_COEFF = args["TT_COEFF"]
    AIE_VARIANT = args["AIE_VARIANT"]
    standard_checks = fn_validate_coeff_type(TT_DATA, TT_COEFF)
    typeCheck = fn_type_support(TT_DATA, TT_COEFF, AIE_VARIANT)
    for check in (standard_checks, typeCheck):
        if check["is_valid"] == False:
            return check
    return isValid


def validate_TP_SHIFT(args):
    TT_DATA = args["TT_DATA"]
    TP_SHIFT = args["TP_SHIFT"]
    return fn_validate_shift_val(TT_DATA, TP_SHIFT)

def fn_validate_shift_val(TT_DATA, TP_SHIFT):
  res = fn_validate_minmax_value("TP_SHIFT", TP_SHIFT, TP_SHIFT_min, TP_SHIFT_max)
  if (res["is_valid"] == False):
    return res
  return fn_float_no_shift(TT_DATA, TP_SHIFT)

def validate_TP_RND(args):
  TP_RND = args["TP_RND"]
  AIE_VARIANT = args["AIE_VARIANT"]
  return fn_validate_roundMode(TP_RND, AIE_VARIANT)

def validate_TP_SAT(args):
  TP_SAT = args["TP_SAT"]
  return fn_validate_satMode(TP_SAT)

def validate_TP_INPUT_WINDOW_VSIZE(args):
    TP_INPUT_WINDOW_VSIZE = args["TP_INPUT_WINDOW_VSIZE"]
    TT_DATA = args["TT_DATA"]
    TT_COEFF = args["TT_COEFF"]
    TP_FIR_LEN = args["TP_FIR_LEN"]
    TP_API = args["TP_API"]
    TP_INTERPOLATE_FACTOR = args["TP_INTERPOLATE_FACTOR"]
    TP_DECIMATE_FACTOR = args["TP_DECIMATE_FACTOR"]
    TP_PARA_INTERP_POLY = args["TP_PARA_INTERP_POLY"]
    TP_PARA_DECI_POLY = args["TP_PARA_DECI_POLY"]
    AIE_VARIANT = args["AIE_VARIANT"]
    TP_SSR = 1

    # interpolate_hb traits looks like the UPSHIFT_CT types have different number of lanes, but it's actually stil the exact same as 384..
    # decimate_hb also just uses 384, so no additional rules here.
    return fn_validate_input_window_size(
        TT_DATA,
        TT_COEFF,
        TP_FIR_LEN,
        TP_INTERPOLATE_FACTOR,
        TP_DECIMATE_FACTOR,
        TP_INPUT_WINDOW_VSIZE,
        TP_API,
        TP_SSR,
        TP_PARA_INTERP_POLY,
        TP_PARA_DECI_POLY,
        AIE_VARIANT
    )


def validate_TP_FIR_LEN(args):
    TT_DATA = args["TT_DATA"]
    TT_COEFF = args["TT_COEFF"]
    TP_FIR_LEN = args["TP_FIR_LEN"]
    TP_CASC_LEN = args["TP_CASC_LEN"]
    TP_INTERPOLATE_FACTOR = args["TP_INTERPOLATE_FACTOR"]
    TP_DECIMATE_FACTOR = args["TP_DECIMATE_FACTOR"]
    TP_SSR = args["TP_SSR"]
    TP_PARA_DECI_POLY = args["TP_PARA_DECI_POLY"]
    TP_PARA_INTERP_POLY = args["TP_PARA_INTERP_POLY"]
    TP_API = args["TP_API"]
    TP_USE_COEFF_RELOAD = args["TP_USE_COEFF_RELOAD"]
    AIE_VARIANT = args["AIE_VARIANT"]
    return fn_validate_fir_len(
        args,
        TT_DATA,
        TT_COEFF,
        TP_FIR_LEN,
        TP_INTERPOLATE_FACTOR,
        TP_DECIMATE_FACTOR,
        TP_CASC_LEN,
        TP_SSR,
        TP_API,
        TP_USE_COEFF_RELOAD,
        AIE_VARIANT,
        TP_PARA_DECI_POLY,
        TP_PARA_INTERP_POLY
    )

def validate_TP_DUAL_IP(args):
    TP_API = args["TP_API"]
    TP_DUAL_IP = args["TP_DUAL_IP"]
    AIE_VARIANT = args["AIE_VARIANT"]
    return fn_validate_dual_ip(TP_API, TP_DUAL_IP, AIE_VARIANT)

def validate_TP_NUM_OUTPUTS(args):
    TP_API = args["TP_API"]
    TP_NUM_OUTPUTS = args["TP_NUM_OUTPUTS"]
    AIE_VARIANT = args["AIE_VARIANT"]
    return fn_validate_num_outputs(TP_API, TP_NUM_OUTPUTS, AIE_VARIANT)


def validate_TP_PARA_DECI_POLY(args):
    TP_DECIMATE_FACTOR = args["TP_DECIMATE_FACTOR"]
    TP_PARA_DECI_POLY = args["TP_PARA_DECI_POLY"]
    return fn_validate_para_deci_poly(TP_DECIMATE_FACTOR, TP_PARA_DECI_POLY)


def validate_TP_PARA_INTERP_POLY(args):
    TP_PARA_INTERP_POLY = args["TP_PARA_INTERP_POLY"]
    TP_INTERPOLATE_FACTOR = args["TP_INTERPOLATE_FACTOR"]
    return fn_validate_para_interp_poly(TP_INTERPOLATE_FACTOR, TP_PARA_INTERP_POLY)


def validate_TP_CASC_LEN(args):
    TP_CASC_LEN = args["TP_CASC_LEN"]
    return fn_validate_casc_len(TP_CASC_LEN)

def validate_TP_INTERPOLATE_FACTOR(args):
    TP_INTERPOLATE_FACTOR = args["TP_INTERPOLATE_FACTOR"]
    AIE_VARIANT = args["AIE_VARIANT"]
    return fn_validate_interpolate_factor(TP_INTERPOLATE_FACTOR, AIE_VARIANT)

def fn_validate_decimate_factor(
    TT_DATA, TT_COEFF, TP_INTERPOLATE_FACTOR, TP_DECIMATE_FACTOR, AIE_VARIANT
):

    res = fn_validate_minmax_value("TP_DECIMATE_FACTOR", TP_DECIMATE_FACTOR, TP_DECIMATE_FACTOR_min, TP_DECIMATE_FACTOR_max)
    if (res["is_valid"] == False):
        return res
    m_kNumSamplesForNLanes = 0
    m_kColumns = 0
    kXYBuffSize = 0
    buffIndexLimit = 16 if TT_DATA != "cfloat" else 8  # 4b range, unless cfloat

    checkBuffSize = (
        isError(
            f"The overall decimation rate ({TP_DECIMATE_FACTOR/TP_INTERPOLATE_FACTOR}) requires more samples ({m_kNumSamplesForNLanes}) than can be fit within the vector register ({kXYBuffSize})."
        )
        if ((m_kNumSamplesForNLanes + (m_kColumns - 1)) > kXYBuffSize)
        else isValid
    )

    checkBuffIndexing = (
        isError(
            f"The overall decimation rate ({TP_DECIMATE_FACTOR/TP_INTERPOLATE_FACTOR}) requires more samples ({m_kNumSamplesForNLanes}) than can be indexed within the vector register ({buffIndexLimit})."
        )
        if (m_kNumSamplesForNLanes > buffIndexLimit)
        else isValid
    )
    if AIE_VARIANT == 1:
        for check in (checkBuffSize, checkBuffIndexing):
            if check["is_valid"] == False:
                return check
    # What is the max DF on AIE-ML? 4 HW 1k registers, more than that will end up on stack. Can interleave/deinterleave up to 8.
    AIE_ML_MAX_DF = 8
    if AIE_VARIANT == 2:
        if TP_DECIMATE_FACTOR > AIE_ML_MAX_DF:
            return isError(
                f"Maximum value for Decimator factor on this device is {AIE_ML_MAX_DF}, but got {TP_DECIMATE_FACTOR}." )
    return isValid

def fn_ssr_poly(AIE_VARIANT, TP_DECIMATE_FACTOR, TP_SSR, TP_PARA_DECI_POLY):

  if AIE_VARIANT == 1 :
    # AIE1 allows for SSR on a non-decomposed design, when, e.g. TP_SSR = 2 and TP_PARA_DECI_POLY = 1.
    return isValid

  if AIE_VARIANT == 2 :
    # AIE-ML only allows SSR on a fully-decomposed design, when, e.g. TP_SSR > 1 only when TP_PARA_DECI_POLY = TP_DECIMATE_FACTOR
    if (TP_DECIMATE_FACTOR > TP_PARA_DECI_POLY) and (TP_SSR > 1) :
      return isError(f" Device only allows SSR (TP_SSR > 1) on a fully decomposed design, i.e. when TP_PARA_DECI_POLY  {TP_PARA_DECI_POLY} = TP_DECIMATE_FACTOR {TP_DECIMATE_FACTOR}.")
    return isValid

def fn_validate_resampler_ssr(TP_SSR, TP_API, TP_DECIMATE_FACTOR, TP_PARA_DECI_POLY, TP_INTERPOLATE_FACTOR, TP_PARA_INTERP_POLY, AIE_VARIANT):
    # SSR only supported for streams
    # What is SSR max for Resampler?
    TP_SSR_max = 4
    if TP_SSR > TP_SSR_min and TP_API == 0:
        return isError(
            f"Requested SSR value {TP_SSR} is not supported with IO Buffer interface. Please use Stream interface or reduce SSR to 1."
        )
    if TP_SSR < TP_SSR_min:
        return isError(f"Minimum value for SSR is {TP_SSR_min}, but got {TP_SSR}.")
    if TP_SSR > TP_SSR_max:
        return isError(f"Maximum value for SSR is {TP_SSR_max}, but got {TP_SSR}.")
    # SSR when design is not decomposed is inefficient. Use Parallel polyphases first.
    if TP_SSR > 1 and TP_INTERPOLATE_FACTOR != TP_PARA_INTERP_POLY:
        return isError(f"SSR mode not available when design is not fully decomposed. Please set TP_PARA_INTERP_POLY ({TP_PARA_INTERP_POLY}) to match TP_INTERPOLATE_FACTOR {TP_INTERPOLATE_FACTOR} before increasing TP_SSR {TP_SSR}.")
    if TP_SSR > 1 and TP_DECIMATE_FACTOR != TP_PARA_DECI_POLY:
        return isError(f"SSR mode not available when design is not fully decomposed. Please set TP_PARA_DECI_POLY ({TP_PARA_DECI_POLY}) to match TP_DECIMATE_FACTOR {TP_DECIMATE_FACTOR} before increasing TP_SSR {TP_SSR}.")
    # May be over-restrictive.
    if (TP_PARA_INTERP_POLY > 1 and TP_INTERPOLATE_FACTOR != TP_PARA_INTERP_POLY):
        return isError(f"SSR decomposition is only supported when interpolation process is fully decomposed into parallel polyphases, i.e. TP_INTERPOLATE_FACTOR {TP_INTERPOLATE_FACTOR} must match TP_PARA_INTERP_POLY {TP_PARA_INTERP_POLY}.")
    if (TP_PARA_DECI_POLY > 1 and TP_DECIMATE_FACTOR != TP_PARA_DECI_POLY):
        return isError(f"SSR decomposition is only supported when decimation process is fully decomposed into parallel polyphases, i.e. TP_DECIMATE_FACTOR {TP_DECIMATE_FACTOR} must match TP_PARA_DECI_POLY {TP_PARA_DECI_POLY}.")
    return fn_stream_only_ssr(TP_API, TP_SSR)

def validate_TP_SSR(args):
    TP_API = args["TP_API"]
    TP_SSR = args["TP_SSR"]
    TP_DECIMATE_FACTOR = args["TP_DECIMATE_FACTOR"]
    TP_PARA_DECI_POLY = args["TP_PARA_DECI_POLY"]
    TP_INTERPOLATE_FACTOR = args["TP_INTERPOLATE_FACTOR"]
    TP_PARA_INTERP_POLY = args["TP_PARA_INTERP_POLY"]
    AIE_VARIANT = args["AIE_VARIANT"]
    return  fn_validate_resampler_ssr(TP_SSR, TP_API, TP_DECIMATE_FACTOR, TP_PARA_DECI_POLY, TP_INTERPOLATE_FACTOR, TP_PARA_INTERP_POLY, AIE_VARIANT)


def validate_TP_DECIMATE_FACTOR(args):
    TT_DATA = args["TT_DATA"]
    TT_COEFF = args["TT_COEFF"]
    TP_INTERPOLATE_FACTOR = args["TP_INTERPOLATE_FACTOR"]
    TP_DECIMATE_FACTOR = args["TP_DECIMATE_FACTOR"]
    AIE_VARIANT = args["AIE_VARIANT"]
    return fn_validate_decimate_factor(
        TT_DATA, TT_COEFF, TP_INTERPOLATE_FACTOR, TP_DECIMATE_FACTOR, AIE_VARIANT
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
    TP_DECIMATE_FACTOR = args["TP_DECIMATE_FACTOR"]
    TP_API = args["TP_API"]
    TP_DUAL_IP = args["TP_DUAL_IP"]
    TP_PARA_DECI_POLY = args["TP_PARA_DECI_POLY"]
    TP_PARA_INTERP_POLY = args["TP_PARA_INTERP_POLY"]
    TP_NUM_OUTPUTS = args["TP_NUM_OUTPUTS"]
    margin_size = sr_asym.fn_margin_size(
        (TP_FIR_LEN + TP_INTERPOLATE_FACTOR - 1) // TP_INTERPOLATE_FACTOR, TT_DATA
    )

    num_in_ports = TP_SSR * TP_PARA_DECI_POLY
    num_out_ports = TP_SSR * TP_PARA_INTERP_POLY

    in_win_size = get_input_window_size(TP_INPUT_WINDOW_VSIZE, num_in_ports, TP_API, TP_DUAL_IP)
    out_win_size = get_output_window_size(TP_INPUT_WINDOW_VSIZE, num_out_ports, TP_API, TP_NUM_OUTPUTS, TP_DECIMATE_FACTOR, TP_INTERPOLATE_FACTOR)

    in_ports = get_port_info("in", "in", TT_DATA, in_win_size, num_in_ports, marginSize=margin_size, TP_API=TP_API)
    in2_ports = (get_port_info( "in2", "in", TT_DATA, in_win_size, num_in_ports, marginSize=margin_size, TP_API=TP_API) if (args["TP_DUAL_IP"] == 1) else [] )
    coeff_ports = (get_parameter_port_info("coeff", "in", TT_COEFF, TP_SSR, TP_FIR_LEN, "async") if (args["TP_USE_COEFF_RELOAD"] == 1) else [])

    # interp by 2 for halfband
    out_ports = get_port_info( "out", "out", TT_DATA, out_win_size, num_out_ports, TP_API=TP_API,)
    out2_ports = (get_port_info( "out2", "out", TT_DATA, out_win_size, num_out_ports, TP_API=TP_API, ) if (args["TP_NUM_OUTPUTS"] == 2) else [])

    return in_ports + in2_ports + coeff_ports + out_ports + out2_ports


#### graph generator ####
# def get_param_list(**kwargs):
#  print("get_param_list")
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
    TP_DECIMATE_FACTOR = args["TP_DECIMATE_FACTOR"]
    TP_USE_COEFF_RELOAD = args["TP_USE_COEFF_RELOAD"]
    TP_NUM_OUTPUTS = args["TP_NUM_OUTPUTS"]
    TP_DUAL_IP = args["TP_DUAL_IP"]
    TP_API = args["TP_API"]
    TP_SSR = args["TP_SSR"]
    coeff_list = args["coeff"]
    TP_PARA_DECI_POLY = args["TP_PARA_DECI_POLY"]
    TP_PARA_INTERP_POLY = args["TP_PARA_INTERP_POLY"]
    TP_SAT = args["TP_SAT"]

    taps = sr_asym.fn_get_taps_vector(TT_COEFF, coeff_list)
    constr_args_str = f"taps" if TP_USE_COEFF_RELOAD == 0 else ""
    dual_ip_declare_str = (
        f"ssr_port_array<input, IN_SSR> in2;" if TP_DUAL_IP == 1 else "// No dual input"
    )
    dual_ip_connect_str = (
        f"connect<>(in2[i], filter.in2[i]);"
        if TP_DUAL_IP == 1
        else "// No dual input"
    )
    coeff_ip_declare_str = (
        f"ssr_port_array<input, RTP_SSR> coeff;"
        if TP_USE_COEFF_RELOAD == 1
        else "//No coeff port"
    )
    coeff_ip_connect_str = (
        f"""for (int i = 0; i < RTP_SSR; i++) {{
            connect<>(coeff[i], filter.coeff[i]);
        }}"""
        if TP_USE_COEFF_RELOAD == 1
        else "//No coeff port"
    )
    dual_op_declare_str = (
        f"ssr_port_array<output, OUT_SSR> out2;" if TP_NUM_OUTPUTS == 2 else "// No dual output"
    )
    dual_op_connect_str = (
        f"connect<>(filter.out2[i], out2[i]);"
        if TP_NUM_OUTPUTS == 2
        else "// No dual output"
    )

    IN_SSR = TP_SSR * TP_PARA_DECI_POLY
    OUT_SSR = TP_SSR * TP_PARA_INTERP_POLY
    RTP_SSR = TP_SSR * TP_PARA_INTERP_POLY
    # Use formatted multi-line string to avoid a lot of \n and \t
    code = f"""
using namespace adf;
class {graphname} : public adf::graph {{
public:
    static constexpr unsigned int IN_SSR = {IN_SSR};
    static constexpr unsigned int RTP_SSR = {RTP_SSR};
    static constexpr unsigned int OUT_SSR = {OUT_SSR};


    template <typename dir, unsigned int num_ports>
    using ssr_port_array = std::array<adf::port<dir>, num_ports>;

    ssr_port_array<input, IN_SSR> in;
    {dual_ip_declare_str}
    {coeff_ip_declare_str}
    ssr_port_array<output, OUT_SSR> out;
    {dual_op_declare_str}

    std::vector<{TT_COEFF}> taps = {taps};
    xf::dsp::aie::fir::resampler::fir_resampler_graph<
      {TT_DATA}, //TT_DATA
      {TT_COEFF}, //TT_COEFF
      {TP_FIR_LEN}, //TP_FIR_LEN
      {TP_INTERPOLATE_FACTOR}, //TP_INTERPOLATE_FACTOR
      {TP_DECIMATE_FACTOR}, //TP_DECIMATE_FACTOR
      {TP_SHIFT}, //TP_SHIFT
      {TP_RND}, //TP_RND
      {TP_INPUT_WINDOW_VSIZE}, //TP_INPUT_WINDOW_VSIZE
      {TP_CASC_LEN}, //TP_CASC_LEN
      {TP_USE_COEFF_RELOAD}, //TP_USE_COEFF_RELOAD
      {TP_NUM_OUTPUTS}, //TP_NUM_OUTPUTS
      {TP_DUAL_IP}, //TP_DUAL_IP
      {TP_API}, //TP_API
      {TP_SSR}, //TP_SSR
      {TP_PARA_INTERP_POLY}, //TP_PARA_INTERP_POLY
      {TP_PARA_DECI_POLY}, //TP_PARA_DECI_POLY
      {TP_SAT} //TP_SAT
    > filter;

    {graphname}() : filter({constr_args_str}) {{
        kernel *filter_kernels = filter.getKernels();
        for (int i=0; i < 1; i++) {{
          runtime<ratio>(filter_kernels[i]) = 0.9;
        }}

        for (unsigned int i = 0; i < IN_SSR; ++i) {{
            // Size of window in Bytes.
            connect<>(in[i], filter.in[i]);
            {dual_ip_connect_str}
        }}

        for (unsigned int i = 0; i < OUT_SSR; ++i) {{
            connect<>(filter.out[i], out[i]);
            {dual_op_connect_str}
        }}

        {coeff_ip_connect_str}
    }}

}};
"""
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
        "L1/tests/aie/src",
    ]

    return out


