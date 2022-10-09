# Common utility variables and functions used accross FIR elements.


from aie_common import *




#### internal validation functions ####


######## traits ############

#shorter accumulator
def fnNumLanes384b(TT_DATA, TT_COEF):

        if (
            (TT_DATA == "int16" and TT_COEF == "int16") or
            (TT_DATA == "int32" and TT_COEF == "int16") or
            (TT_DATA == "float" and TT_COEF == "float")
        ):
            return 8
        elif (
            (TT_DATA == "cint16" and TT_COEF == "int16") or
            (TT_DATA == "cint16" and TT_COEF == "cint16") or
            (TT_DATA == "int32" and TT_COEF == "int32") or
            (TT_DATA == "cint32" and TT_COEF == "int16") or
            (TT_DATA == "cint32" and TT_COEF == "cint16") or
            (TT_DATA == "cint32" and TT_COEF == "int32") or
            (TT_DATA == "cfloat" and TT_COEF == "float") or
            (TT_DATA == "cfloat" and TT_COEF == "cfloat")
        ):
            return 4
        elif (TT_DATA == "cint32" and TT_COEF == "cint32") :
            return 2
        else:
            return 0

#### validate input window size ####
def fnNumLanes(TT_DATA, TT_COEF, TP_API=0):
    if (TP_API == 1 and not (
        #Stream defults to short accs except for these typs (defined in traits)
        (TT_DATA == "int32" and TT_COEF == "int16") or
        (TT_DATA == "cint32" and TT_COEF == "int16") or
        (TT_DATA == "cint32" and TT_COEF == "int32") or
        (TT_DATA == "cint32" and TT_COEF == "cint16") or
        (TT_DATA == "float" and TT_COEF == "float") or
        (TT_DATA == "cfloat" and TT_COEF == "float") or
        (TT_DATA == "cfloat" and TT_COEF == "cfloat")
        )
    ):
        return fnNumLanes384b(TT_DATA, TT_COEF)
    else:
        if (TT_DATA == "int16" and TT_COEF == "int16"):
            return 16
        elif ((TT_DATA == "cint16" and TT_COEF == "int16")
                or (TT_DATA == "cint16" and TT_COEF == "cint16")
                or (TT_DATA == "int32" and TT_COEF == "int16")
                or (TT_DATA == "int32" and TT_COEF == "int32")
                or (TT_DATA == "float" and TT_COEF == "float")):
            return 8
        elif ((TT_DATA == "cint32" and TT_COEF == "int16")
                or (TT_DATA == "cint32" and TT_COEF == "cint16")
                or (TT_DATA == "cint32" and TT_COEF == "int32")
                or (TT_DATA == "cint32" and TT_COEF == "cint32")
                or (TT_DATA == "cfloat" and TT_COEF == "float")
                or (TT_DATA == "cfloat" and TT_COEF == "cfloat")):
            return 4
        else:
            return 0


# function to return the number of columns for a tall-narrow atomic intrinsic for a type combo
def fnNumCols(TT_DATA, TT_COEF, TP_API=0):
  if TP_API==0:
    return (2 if fn_size_by_byte(TT_COEF)==2 else 1)
  else :
    return fnNumCols384(TT_DATA, TT_COEF)

# function to return the number of columns for a short-wide atomic intrinsic for a type combo
def fnNumCols384(TT_DATA, TT_COEF):


  if (
    (TT_DATA == "int16" and TT_COEF == "int16") or
    (TT_DATA == "cint16" and TT_COEF == "int16")
  ):
    return 8

  if (
     (TT_DATA == "cint16" and TT_COEF == "cint16") or
     (TT_DATA == "int32" and TT_COEF == "int16") or # 80bit
     (TT_DATA == "int32" and TT_COEF == "int32")or
     (TT_DATA == "cint32" and TT_COEF == "int16")
  ) :
    return 4
  if (
     (TT_DATA == "cint32" and TT_COEF == "cint16")  or
     (TT_DATA == "cint32" and TT_COEF == "int32") or
     (TT_DATA == "cint32" and TT_COEF == "cint32") or
     (TT_DATA == "float" and TT_COEF == "float") or
     (TT_DATA == "cfloat" and TT_COEF == "float") or
     (TT_DATA == "cfloat" and TT_COEF == "cfloat")
  ) :
    return 2

  return 2*fnNumCols(TT_DATA, TT_COEF);

### Common constraints based on traits

def fn_windowsize_multiple_lanes(TT_DATA, TT_COEF, TP_INPUT_WINDOW_VSIZE, TP_API, numLanes=None):
    # Use the default nmber of lanes
    num_lanes = fnNumLanes(TT_DATA, TT_COEF, TP_API) if not numLanes else numLanes
    if ((TP_INPUT_WINDOW_VSIZE % num_lanes) != 0):
      return isError(
        f"Unsupported window size ({TP_INPUT_WINDOW_VSIZE}). For Input/Output and coefficient type combination :\n\t"\
          f"{TT_DATA},{TT_COEF} window size should be multiple of {num_lanes}.\n"
      )
    return isValid

def fn_windowsize_divisible_by_ssr(TP_INPUT_WINDOW_VSIZE, TP_SSR):
    # Check if window_vsize is divisible by SSR factor
    if ((TP_INPUT_WINDOW_VSIZE % TP_SSR) != 0):
      return isError(
        f"Unsupported window size ({TP_INPUT_WINDOW_VSIZE}). Input window vector size must be a multiple of {TP_SSR}.\n"
      )
    return isValid

def fn_max_windowsize_for_buffer(TT_DATA, TP_FIR_LEN, TP_INPUT_WINDOW_VSIZE, TP_API, TP_SSR=1, TP_INTERPOLATE_FACTOR=1, TP_DECIMATE_FACTOR=1):

  kMemoryModuleSize = 32768; #Bytes
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



# todo, just pad coefficients so we don't need this.
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


def fn_max_fir_len_each_kernel(TP_FIR_LEN, TP_CASC_LEN, TP_USE_COEF_RELOAD, TP_SSR=1, symFactor = 1):
  # todo - it would be good to add a comment explaining this limit.
  # IIRC, it's because we fully unroll the ops loop, and then run out of program memory
  # but it also seems like there's a different reason for coefReload.
  firLengthMax = 1024
  if TP_USE_COEF_RELOAD == 1:
    if TP_FIR_LEN <= firLengthMax: # might need to revisit for reload on SSR
      vld = True
    else:
      vld = False
  else:
    # This might not be accurate due to fir_range_len.
    firLengthMax = 256 * symFactor
    if TP_FIR_LEN / (TP_CASC_LEN * TP_SSR) <= firLengthMax:
      vld = True
    else:
      vld = False

  if not vld:
    return isError("Maximum fir length is enforced for each kernel in cascade chain. Consider increasing the number of cascade stages.")
  else :
    return isValid

def fn_stream_ssr(TP_API, TP_SSR):
  if TP_API == 0 and TP_SSR > 1:
    return isError(f"SSR > 1 is only supported for streaming ports")
  return isValid

#### validate coeff reload ####
def fn_validate_use_coeff_reload(TP_API, TP_USE_COEF_RELOAD, TP_SSR):
    if TP_API == 0 and TP_USE_COEF_RELOAD == 2:
      return isError("Stream Header based Coefficient Reload is not supported with Window ports")

    return isValid

def validate_TP_USE_COEF_RELOAD(args):
    TP_API = args["TP_API"]
    TP_USE_COEF_RELOAD = args["TP_USE_COEF_RELOAD"]
    TP_SSR = args["TP_SSR"]

    return fn_validate_use_coeff_reload(TP_API, TP_USE_COEF_RELOAD, TP_SSR)

def validate_TP_USE_COEF_RELOAD_NON_SSR(args):
    TP_API = args["TP_API"]
    TP_USE_COEF_RELOAD = args["TP_USE_COEF_RELOAD"]
    TP_SSR = 1

    return fn_validate_use_coeff_reload(TP_API, TP_USE_COEF_RELOAD, TP_SSR)
