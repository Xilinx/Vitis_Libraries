# Common utility variables and functions used accross FIR elements.


from aie_common import *




#### internal validation functions ####


######## traits ############

TP_DECIMATE_FACTOR_min = 2
TP_DECIMATE_FACTOR_max = 7
TP_INTERPOLATE_FACTOR_min = 1
TP_INTERPOLATE_FACTOR_max = 16
TP_INPUT_WINDOW_VSIZE_min = 4
TP_PARA_DECI_POLY_min = 1
TP_PARA_INTERP_POLY_min = 1
TP_CASC_LEN_min = 1
TP_CASC_LEN_max = 40
TP_FIR_LEN_min = 4
TP_FIR_LEN_max = 8192
TP_SHIFT_min = 0
TP_SHIFT_max = 61
TP_SSR_min = 1


#shorter accumulator
def fnNumLanes384b(TT_DATA, TT_COEFF, AIE_VARIANT=1):
    if AIE_VARIANT == 1:
        if (
            (TT_DATA == "int16" and TT_COEFF == "int16") or
            (TT_DATA == "int16" and TT_COEFF == "int32") or
            (TT_DATA == "int32" and TT_COEFF == "int16") or
            (TT_DATA == "float" and TT_COEFF == "float")
        ):
            return 8
        elif (
            (TT_DATA == "cint16" and TT_COEFF == "int16") or
            (TT_DATA == "cint16" and TT_COEFF == "cint16") or
            (TT_DATA == "cint16" and TT_COEFF == "int32") or
            (TT_DATA == "cint16" and TT_COEFF == "cint32") or
            (TT_DATA == "int32" and TT_COEFF == "int32") or
            (TT_DATA == "cint32" and TT_COEFF == "int16") or
            (TT_DATA == "cint32" and TT_COEFF == "cint16") or
            (TT_DATA == "cint32" and TT_COEFF == "int32") or
            (TT_DATA == "cfloat" and TT_COEFF == "float") or
            (TT_DATA == "cfloat" and TT_COEFF == "cfloat")
        ):
            return 4
        elif (TT_DATA == "cint32" and TT_COEFF == "cint32") :
            return 2
        else:
            return 0
    if AIE_VARIANT == 2:
        if (
            (TT_DATA == "int16" and TT_COEFF == "int16")
        ):
            return 16
        else:
            return 8

#### validate input window size ####
def fnNumLanes(TT_DATA, TT_COEFF, TP_API=0, AIE_VARIANT=1):
    if AIE_VARIANT == 1:

      if (TP_API == 1 and not (
          #Stream defults to short accs except for these typs (defined in traits)
          (TT_DATA == "int32" and TT_COEFF == "int16") or
          (TT_DATA == "cint32" and TT_COEFF == "int16") or
          (TT_DATA == "cint32" and TT_COEFF == "int32") or
          (TT_DATA == "cint32" and TT_COEFF == "cint16") or
          (TT_DATA == "float" and TT_COEFF == "float") or
          (TT_DATA == "cfloat" and TT_COEFF == "float") or
          (TT_DATA == "cfloat" and TT_COEFF == "cfloat")
          )
      ):
          return fnNumLanes384b(TT_DATA, TT_COEFF)
      else:
          if (TT_DATA == "int16" and TT_COEFF == "int16"):
              return 16
          elif ((TT_DATA == "cint16" and TT_COEFF == "int16")
                  or (TT_DATA == "cint16" and TT_COEFF == "cint16")
                  or (TT_DATA == "int16" and TT_COEFF == "int32")
                  or (TT_DATA == "int32" and TT_COEFF == "int16")
                  or (TT_DATA == "int32" and TT_COEFF == "int32")
                  or (TT_DATA == "float" and TT_COEFF == "float")):
              return 8
          elif ((TT_DATA == "cint32" and TT_COEFF == "int16")
                  or (TT_DATA == "cint32" and TT_COEFF == "cint16")
                  or (TT_DATA == "cint16" and TT_COEFF == "int32")
                  or (TT_DATA == "cint16" and TT_COEFF == "cint32")
                  or (TT_DATA == "cint16" and TT_COEFF == "cint16")
                  or (TT_DATA == "cint32" and TT_COEFF == "int32")
                  or (TT_DATA == "cint32" and TT_COEFF == "cint32")
                  or (TT_DATA == "cfloat" and TT_COEFF == "float")
                  or (TT_DATA == "cfloat" and TT_COEFF == "cfloat")):
              return 4
          else:
              return 0
    if AIE_VARIANT == 2:
        if (
            (TT_DATA == "int16" and TT_COEFF == "int16")
        ):
            return 32
        else:
            return 16


# function to return the number of columns for a tall-narrow atomic intrinsic for a type combo
def fnNumCols(TT_DATA, TT_COEFF, TP_API=0):
  if TP_API==0:
    return (2 if fn_size_by_byte(TT_COEFF)==2 else 1)
  else :
    return fnNumCols384(TT_DATA, TT_COEFF)

# function to return the number of columns for a short-wide atomic intrinsic for a type combo
def fnNumCols384(TT_DATA, TT_COEFF):
  if (
     (TT_DATA == "int16" and TT_COEFF == "int16") or
     (TT_DATA == "cint16" and TT_COEFF == "int16")
  ) :
    return 4
  if (
     (TT_DATA == "cint16" and TT_COEFF == "int32") or
     (TT_DATA == "cint16" and TT_COEFF == "cint32") or
     (TT_DATA == "int32" and TT_COEFF == "int32")or
     (TT_DATA == "int32" and TT_COEFF == "int16") or
     (TT_DATA == "int16" and TT_COEFF == "int32") or
     (TT_DATA == "cint32" and TT_COEFF == "int16") or
     (TT_DATA == "cint32" and TT_COEFF == "cint16")  or
     (TT_DATA == "cint32" and TT_COEFF == "int32") or
     (TT_DATA == "cint32" and TT_COEFF == "cint32")
  ) :
    return 2
  if (
     (TT_DATA == "cint16" and TT_COEFF == "cint32") or
     (TT_DATA == "float" and TT_COEFF == "float") or
     (TT_DATA == "cfloat" and TT_COEFF == "float") or
     (TT_DATA == "cfloat" and TT_COEFF == "cfloat")
  ) :
    return 1

  return 2 * (2 if fn_size_by_byte(TT_COEFF)==2 else 1)

### Common constraints based on traits

def fn_windowsize_multiple_lanes(TT_DATA, TT_COEFF, TP_INPUT_WINDOW_VSIZE, TP_API, numLanes=None, TP_SSR=1, AIE_VARIANT=1):
    # Use the default nmber of lanes
    num_lanes = fnNumLanes(TT_DATA, TT_COEFF, TP_API, AIE_VARIANT) if not numLanes else numLanes
    numLanesMultiple = num_lanes * TP_SSR
    if (((TP_INPUT_WINDOW_VSIZE / TP_SSR) % num_lanes) != 0):
      return isError(
        f"Unsupported window size ({TP_INPUT_WINDOW_VSIZE}). For Input/Output and coefficient type combination :\n\t"\
          f"{TT_DATA},{TT_COEFF} and taking into account SSR operation mode ({TP_SSR}), window size should be multiple of {numLanesMultiple}.\n"
      )
    return isValid

def fn_windowsize_divisible_by_param(TP_INPUT_WINDOW_VSIZE, TP_SSR):
    # Check if window_vsize is divisible by SSR factor
    if ((TP_INPUT_WINDOW_VSIZE % TP_SSR) != 0):
      return isError(
        f"Unsupported window size ({TP_INPUT_WINDOW_VSIZE}). Input window vector size must be a multiple of: {TP_SSR}.\n"
      )
    return isValid

def fn_max_windowsize_for_buffer(TT_DATA, TP_FIR_LEN, TP_INPUT_WINDOW_VSIZE, TP_API, TP_SSR=1, TP_INTERPOLATE_FACTOR=1, TP_DECIMATE_FACTOR=1, AIE_VARIANT=1):

  if AIE_VARIANT == 1:
      kMemoryModuleSize = 32768; #Bytes
  if AIE_VARIANT == 2:
      kMemoryModuleSize = 65536; #Bytes
  TP_FIR_LEN = TP_FIR_LEN // TP_SSR # FIR Length gets reduced by SSR factor
  # Margin + requested window size in bytes
  inBufferSize = ((TP_FIR_LEN + TP_INPUT_WINDOW_VSIZE)* fn_size_by_byte(TT_DATA))
  # no margin
  outBufferSize = (((TP_INPUT_WINDOW_VSIZE * TP_INTERPOLATE_FACTOR) // TP_DECIMATE_FACTOR) * fn_size_by_byte(TT_DATA))

  if TP_API == 0 :
    if inBufferSize > kMemoryModuleSize:
      return isError(f"Input Window size ({inBufferSize}B) exceeds Memory Module size of 32kB.")
    if outBufferSize > kMemoryModuleSize:
      return isError(f"Output Window size ({outBufferSize}B) exceeds Memory Module size of 32kB.")

  return isValid



# Calculate FIR range for cascaded kernel
def fnFirRange( TP_FL, TP_CL, TP_KP, TP_Rnd=1):
  # TP_FL - FIR Length, TP_CL - Cascade Length, TP_KP - Kernel Position
  return ((fnTrunc(TP_FL,TP_Rnd * TP_CL) // TP_CL) +
          (
            TP_Rnd if (TP_FL - fnTrunc(TP_FL,TP_Rnd * TP_CL)) >= TP_Rnd*(TP_KP + 1) else
            0
          ) )

def fnFirRangeRem( TP_FL,  TP_CL, TP_KP, TP_Rnd=1):
    # TP_FL - FIR Length, TP_CL - Cascade Length, TP_KP - Kernel Position
    # this is for last in the cascade
    return ((fnTrunc(TP_FL,TP_Rnd * TP_CL) // TP_CL) +
            ((TP_FL - fnTrunc(TP_FL,TP_Rnd * TP_CL)) % TP_Rnd));


def fn_min_fir_len_each_kernel(TP_FIR_LEN, TP_CASC_LEN, TP_SSR=1, TP_Rnd=1):
  TP_FIR_LEN = TP_FIR_LEN // TP_SSR
  firLengthMin = 1
  vld=True
  # Check that the last and second last kernel has at least the minimum required taps.
  if TP_CASC_LEN > 1:
      vld = fnFirRangeRem(TP_FIR_LEN, TP_CASC_LEN, TP_CASC_LEN - 1) >= firLengthMin and \
              fnFirRange(TP_FIR_LEN, TP_CASC_LEN, TP_CASC_LEN -2) >= firLengthMin

  if not vld:
    return isError(
      f"Minimum fir length ({firLengthMin}) is enforced for each kernel in cascade chain. "\
      "Consider reducing the number of cascade stages. "
    )

  return isValid


def fn_max_fir_len_each_kernel(TT_DATA, TP_FIR_LEN, TP_CASC_LEN, TP_USE_COEFF_RELOAD, TP_SSR=1, TP_API=0, symFactor = 1):
  # Coeff array needs storage on heap and unrolled MAC operation inflate Program Memory.
  firLengthMaxCoeffArray = 256 * symFactor
  # Data samples must fit into 1024-bit (128 Byte) vector register
  firLengthMaxDataReg = 128  * symFactor / fn_size_by_byte(TT_DATA)
  # Fir length per kernel in a cascaded design that may also be decomposed into multiple SSR paths.
  firLengthPerKernel = TP_FIR_LEN / (TP_CASC_LEN * TP_SSR)

  if TP_API == 0:
    # When buffer IO, check that the coeff array fits into heap
    if TP_USE_COEFF_RELOAD == 1:
      # Coeff array gets divided up in SSR mode, where each SSR phase gets a fraction of the array.
      if TP_FIR_LEN / (TP_SSR) <= firLengthMaxCoeffArray:
        vld = True
      else:
        vld = False
    else:
      if firLengthPerKernel <= firLengthMaxCoeffArray:
        vld = True
      else:
        vld = False
  else:
    # When stream IO, check that the data fits into a 1024-bit reg. Coeff Array condition always met.
    if firLengthPerKernel <= firLengthMaxDataReg:
      vld = True
    else:
      vld = False

  if not vld:
    return isError(f"Maximum fir length ({firLengthPerKernel}) is enforced for each kernel in cascade chain. Consider increasing the number of cascade stages.")
  else :
    return isValid

def fn_stream_ssr(TP_API, TP_SSR):
  if TP_API == 0 and TP_SSR > 1:
    return isError(f"SSR > 1 is only supported for streaming ports")
  return isValid

def get_input_window_size(TP_INPUT_WINDOW_VSIZE, TP_POLY_SSR, TP_API, TP_DUAL_IP):
    if TP_API == 0:
        in_win_size = TP_INPUT_WINDOW_VSIZE / TP_POLY_SSR
    elif TP_API == 1:
        num_ports = TP_DUAL_IP + 1
        in_win_size = TP_INPUT_WINDOW_VSIZE / TP_POLY_SSR / num_ports
    return in_win_size

def get_output_window_size(TP_INPUT_WINDOW_VSIZE, TP_POLY_SSR, TP_API, TP_NUM_OUTPUTS, TP_DECIMATE_FACTOR, TP_INTERPOLATE_FACTOR):
    if TP_API == 0:
        out_win_size = ((TP_INPUT_WINDOW_VSIZE) * TP_INTERPOLATE_FACTOR) / TP_DECIMATE_FACTOR / TP_POLY_SSR
    elif TP_API == 1:
        num_ports = TP_NUM_OUTPUTS
        out_win_size = ((TP_INPUT_WINDOW_VSIZE) * TP_INTERPOLATE_FACTOR ) / TP_DECIMATE_FACTOR / num_ports / TP_POLY_SSR
    return out_win_size

#### validate coeff reload ####
def fn_validate_use_coeff_reload(TP_API, TP_USE_COEFF_RELOAD, TP_SSR):
    if TP_API == 0 and TP_USE_COEFF_RELOAD == 2:
      return isError("Stream Header based Coefficient Reload is not supported with Window ports. Got TP_USE_COEFF_RELOAD  {TP_USE_COEFF_RELOAD}")

    return isValid

def validate_TP_USE_COEFF_RELOAD(args):
    TP_API = args["TP_API"]
    TP_USE_COEFF_RELOAD = args["TP_USE_COEFF_RELOAD"]
    TP_SSR = args["TP_SSR"]

    return fn_validate_use_coeff_reload(TP_API, TP_USE_COEFF_RELOAD, TP_SSR)

def validate_TP_USE_COEFF_RELOAD_NON_SSR(args):
    TP_API = args["TP_API"]
    TP_USE_COEFF_RELOAD = args["TP_USE_COEFF_RELOAD"]
    TP_SSR = 1

    return fn_validate_use_coeff_reload(TP_API, TP_USE_COEFF_RELOAD, TP_SSR)

def fn_validate_ssr(TP_SSR):
    if TP_SSR < TP_SSR_min:
        return isError(
            f"Minimum value for SSR is {TP_SSR_min}, but got {TP_SSR}."
        )
    return isValid

def fn_stream_only_ssr(TP_API, TP_SSR):
    if TP_API == 0 and TP_SSR > 1:
        return isError(
            f"Requested SSR value {TP_SSR} is not supported with IO Buffer interface. Please use Stream interface or reduce SSR to 1."
        )
    return fn_validate_ssr(TP_SSR)

def fn_validate_hb_ssr(TP_API, TP_SSR):
    return fn_validate_ssr(TP_SSR)

def fn_validate_hw_dual_stream_ports(TP_API, TP_DUAL_IP, AIE_VARIANT):
    # AIE Variant does not
    if AIE_VARIANT == 2:
      if TP_API == 1 and TP_DUAL_IP == 1:
        return isError("Dual input stream ports not supported on this device. Got TP_DUAL_IP {TP_DUAL_IP}")

    return isValid

def fn_validate_dual_ip(TP_API, TP_DUAL_IP, AIE_VARIANT):
    # Any AIE Variant:
    # Dual Input IO buffers offer no advantage.
    if TP_API == 0 and TP_DUAL_IP == 1:
        return isError("Dual input buffer ports not supported on this device. Got TP_DUAL_IP {TP_DUAL_IP}")
    # Check if hardware supports 2 stream ports.
    return fn_validate_hw_dual_stream_ports(TP_API, TP_DUAL_IP, AIE_VARIANT)

def fn_validate_sym_dual_ip(TP_API, TP_DUAL_IP, AIE_VARIANT):
    if AIE_VARIANT == 2:
      # AIE-ML does not support symmetric operation, which are performed by assymetric calls. No advantage of using 2 intput ports
      if TP_API == 0 and TP_DUAL_IP == 1:
        return isError("Dual input buffer ports not supported on this device. Got TP_DUAL_IP {TP_DUAL_IP}")

    # Check if hardware supports 2 stream ports.
    return fn_validate_hw_dual_stream_ports(TP_API, TP_DUAL_IP, AIE_VARIANT)

def fn_validate_sr_dual_ip(TP_NUM_OUTPUTS, TP_API, TP_DUAL_IP, AIE_VARIANT=1):
    # Component produces same amount or more data on output than consumed at input.
    # No advantage of using 2 input streams and only 1 output.
    # IO Buffer (TP_API==0) unrestricted.
    if TP_API == 1 and TP_DUAL_IP == 1 and TP_NUM_OUTPUTS != 2:
        return isError(
            "Dual input streams only supported when number of output streams is also 2. Got TP_DUAL_IP {TP_DUAL_IP} and TP_NUM_OUTPUTS {TP_NUM_OUTPUTS}"
        )
    # Do generic check
    return fn_validate_dual_ip(TP_API, TP_DUAL_IP, AIE_VARIANT)

def fn_validate_num_outputs(TP_API, TP_NUM_OUTPUTS, AIE_VARIANT):
    if TP_NUM_OUTPUTS == 2 and TP_API == 1 and AIE_VARIANT == 2:
      return isError("Dual output stream ports not supported on this device.")
    return isValid

def fn_validate_hb_num_outputs(TP_PARA_POLY, TP_DUAL_IP, TP_NUM_OUTPUTS, TP_API, AIE_VARIANT):
  # Check Polyphases
  if TP_PARA_POLY == 2 :
    if (TP_DUAL_IP + 1) != TP_NUM_OUTPUTS:
      return isError(f"When Parallel polyphases enabled (set to 2), dual output ports are only allowed with dual inputs.")
  return fn_validate_num_outputs(TP_API, TP_NUM_OUTPUTS, AIE_VARIANT)

def fn_validate_interpolate_factor(TP_INTERPOLATE_FACTOR, AIE_VARIANT):
    if (
        TP_INTERPOLATE_FACTOR < TP_INTERPOLATE_FACTOR_min
        or TP_INTERPOLATE_FACTOR > TP_INTERPOLATE_FACTOR_max
    ):
        return isError(
            f"Minimum and maximum value for Interpolate factor is {TP_INTERPOLATE_FACTOR_min} and {TP_INTERPOLATE_FACTOR_max}, respectively, but got {TP_INTERPOLATE_FACTOR}."
        )
    AIE_ML_MAX_DF = 8
    if AIE_VARIANT == 2:
        if TP_INTERPOLATE_FACTOR > AIE_ML_MAX_DF:
            return isError(
                f"Maximum value for interpolation factor on this device is {AIE_ML_MAX_DF}, but got {TP_INTERPOLATE_FACTOR}." )
    return isValid


def fn_type_aieml_support(TT_DATA, TT_COEFF, AIE_VARIANT):
    if AIE_VARIANT == 2:
        # Assume all other combinations are supported.
        if ((TT_DATA == "cfloat" and TT_COEFF == "cfloat") or
           (TT_DATA == "cfloat" and TT_COEFF == "float") or
           (TT_DATA == "bfloat16" and TT_COEFF == "bfloat16") or
           (TT_DATA == "cbfloat16" and TT_COEFF == "cbfloat16")):
                return isError(
                    f"The combination of {TT_DATA} and {TT_COEFF} is not supported for this class. Got TT_DATA {TT_DATA} and TT_COEFF {TT_COEFF}"
                )
    return isValid

def fn_type_support(TT_DATA, TT_COEFF, AIE_VARIANT):
    if AIE_VARIANT == 1:
        return (
            # Combination of int16 data and int16 coeffs is not supported due to HW restrctions.
            isError(
                f"The combination of TT_DATA {TT_DATA} and TT_COEFF {TT_COEFF} is not supported for this class."
            )
            if (TT_DATA == "int16" and TT_COEFF == "int16")
            else isValid
        )
    return fn_type_aieml_support(TT_DATA, TT_COEFF, AIE_VARIANT)

def fn_type_hb_support(TT_DATA, TT_COEFF, AIE_VARIANT):
    # no restrictions on AIE1, only check AIE-ML restrictions.
    return fn_type_aieml_support(TT_DATA, TT_COEFF, AIE_VARIANT)

def fn_type_sr_support(TT_DATA, TT_COEFF, AIE_VARIANT):
    # no restrictions on AIE1, only check AIE-ML restrictions.
    return fn_type_aieml_support(TT_DATA, TT_COEFF, AIE_VARIANT)


