

from aie_common import *
import json
import sys

# static_assert(fnCheckDataType<TT_DATA>() ,"ERROR: TT_IN_DATA is not a supported type");
# static_assert(fnCheckTwiddleType<TT_TWIDDLE>(), "ERROR: TT_TWIDDLE is not a supported type");
# static_assert(fnCheckDataTwiddleType<TT_DATA,TT_TWIDDLE>(), "ERROR: TT_TWIDDLE is incompatible with data type");
# static_assert(fnCheckPointSize<TP_POINT_SIZE>(),"ERROR: TP_POINT_SIZE is not a supported value {4-128");
# static_assert(TP_FFT_NIFFT == 0 || TP_FFT_NIFFT == 1,"ERROR: TP_FFT_NIFFT must be 0 (reverse) or 1 (forward)");
# static_assert(fnCheckShift<TP_SHIFT>(), "ERROR: TP_SHIFT is out of range (0 to 60)");
# static_assert(fnCheckShiftFloat<TT_DATA,TP_SHIFT>(), "ERROR: TP_SHIFT is ignored for data type cfloat so must be set to 0");

#Range for params
TP_POINT_SIZE_min = 8
TP_POINT_SIZE_max = 128
TP_NUM_FRAMES_min = 1
TP_NUM_FRAMES_max = 100
TP_CASC_LEN_min = 1
TP_CASC_LEN_max = 16
TP_SSR_min = 1
TP_SSR_max = 16
TP_SHIFT_min=0
TP_SHIFT_max=60

# Validate Twiddle type 
def fn_validate_twiddle_type(TT_DATA, TT_TWIDDLE):
  validTypeCombos = [
      ("cint16", "cint16"),
      ("cint32", "cint16"),
      ("cfloat", "cfloat")
    ]
  return (
    isValid if ((TT_DATA,TT_TWIDDLE) in validTypeCombos)
    else (
    isError(f"Invalid Data/Twiddle type combination ({TT_DATA},{TT_TWIDDLE}). Valid combinations are cint16/cint16, cint32/cint16 or cfloat/cfloat")
    )
  )
def validate_TT_TWIDDLE(args):
  TT_DATA = args["TT_DATA"]
  TT_TWIDDLE = args["TT_TWIDDLE"]
  return fn_validate_twiddle_type(TT_DATA, TT_TWIDDLE)

# Validate point size
def fn_validate_point_size(AIE_VARIANT, TP_POINT_SIZE, TT_DATA, TT_TWIDDLE, TP_SSR, TP_CASC_LEN):
  if AIE_VARIANT == 1:
    maxDataMemBytes = 32768; #Bytes
  if AIE_VARIANT == 2:
    maxDataMemBytes = 65536; #Bytes
  COEFF_COL_DIM = CEIL(TP_POINT_SIZE, fn_size_by_byte(TT_TWIDDLE)) / TP_SSR 
  COEFF_ROW_DIM = round(TP_POINT_SIZE / TP_CASC_LEN)
  KERNEL_COEFF_SIZE = COEFF_COL_DIM * COEFF_ROW_DIM * fn_size_by_byte(TT_TWIDDLE)
  res = fn_validate_minmax_value("TP_POINT_SIZE", TP_POINT_SIZE, TP_POINT_SIZE_min, TP_POINT_SIZE_max)

  kSamplesDataVector = 256/8/fn_size_by_byte(TT_DATA)
  paddedPointSize = CEIL(CEIL(TP_POINT_SIZE, kSamplesDataVector), kSamplesDataVector * TP_CASC_LEN)
  kSamplesTwiddleVector = 256/8/fn_size_by_byte(TT_TWIDDLE)
  twiddleColSize = CEIL(TP_POINT_SIZE, kSamplesTwiddleVector)

  if (res["is_valid"] == False):  
    return res
  else:
    if (KERNEL_COEFF_SIZE > maxDataMemBytes):
      return(
        isError(f"""Invalid POINT_SIZE ({TP_POINT_SIZE}). 
                The coefficient/twiddles require {KERNEL_COEFF_SIZE}B per kernel for this POINT_SIZE which exceeds the maximum allowed data memory per kernel ({maxDataMemBytes}B). 
                The memory required for the coeffecients per kernel is (CEIL(TP_POINT_SIZE{TP_POINT_SIZE}, sizeof(TT_TWIDDLE{TT_TWIDDLE})) / TP_SSR {TP_SSR}) * (TP_POINT_SIZE{TP_POINT_SIZE} / TP_CASC_LEN{TP_CASC_LEN}).  
                Increase the value of TP_CASC_LEN{TP_CASC_LEN} or TP_SSR{TP_SSR} to split the coefficeients across multiple kernels.""")
                )
    # if (paddedPointSize % (TP_CASC_LEN * kSamplesDataVector) != 0):
    #   return (
    #     isError(f""""Invalid TP_POINT_SIZE ({TP_POINT_SIZE}) and CASC_LEN ({TP_CASC_LEN}) combination. 
    #             TP_POINT_SIZE ({TP_POINT_SIZE}) should zero-padded to be an integer multiple of TP_CASC_LEN ({TP_CASC_LEN}) and vector size. Vector size for {TT_DATA} is {kSamplesDataVector} . 
    #             The resulting zero-padded POINT_SIZE->({paddedPointSize}) must be a multiple of TP_CASC_LEN * 256/8/sizeof(TT_DATA)->({TP_CASC_LEN * kSamplesDataVector}) """)
    #             )
    # if (twiddleColSize % (TP_SSR * kSamplesTwiddleVector) != 0):
    #   return (
    #   isError(f"""Invalid TP_POINT_SIZE ({TP_POINT_SIZE}) and TP_SSR ({TP_SSR}) combination. 
    #           The matrix of coefficients/twiddles are created with a column dimension of TP_POINT_SIZE zero-padded to a multiple 256/8/sizeof(TT_TWIDDLE) -> ({twiddleColSize}). 
    #           This must be a multiple of TP_SSR * 256/8/sizeof(TT_TWIDDLE) -> ({TP_SSR * kSamplesTwiddleVector})""")
    #   )
    return isValid
    
def validate_TP_POINT_SIZE(args):
  AIE_VARIANT = args["AIE_VARIANT"]
  TP_POINT_SIZE = args["TP_POINT_SIZE"]
  TT_DATA = args["TT_DATA"]
  TT_TWIDDLE = args["TT_TWIDDLE"]
  TP_SSR = args["TP_SSR"]
  TP_CASC_LEN = args["TP_CASC_LEN"]
  return fn_validate_point_size(AIE_VARIANT, TP_POINT_SIZE, TT_DATA, TT_TWIDDLE, TP_SSR, TP_CASC_LEN)

# Validate SHIFT
def validate_TP_SHIFT(args):
  TP_SHIFT = args["TP_SHIFT"]
  TT_DATA = args["TT_DATA"]
  return fn_validate_shift_val(TT_DATA, TP_SHIFT)

def fn_validate_shift_val(TT_DATA, TP_SHIFT):
  res = fn_validate_minmax_value("TP_SHIFT", TP_SHIFT, TP_SHIFT_min, TP_SHIFT_max)
  if (res["is_valid"] == False):  
    return res
  return fn_float_no_shift(TT_DATA, TP_SHIFT)

# Validate CASC_LEN
def fn_validate_casc_len(TP_CASC_LEN):
  return fn_validate_minmax_value("TP_CASC_LEN", TP_CASC_LEN, TP_CASC_LEN_min, TP_CASC_LEN_max)

def validate_TP_CASC_LEN(args):
  TP_CASC_LEN = args["TP_CASC_LEN"]
  return fn_validate_casc_len(TP_CASC_LEN)

# Validate TP_SSR
def fn_validate_ssr(TP_SSR):
  return fn_validate_minmax_value("TP_SSR", TP_SSR, TP_SSR_min, TP_SSR_max)

def validate_TP_SSR(args):
  TP_SSR = args["TP_SSR"]
  return fn_validate_ssr(TP_SSR)

# TP_SAT
def validate_TP_SAT(args):
  TP_SAT = args["TP_SAT"]
  return fn_validate_satMode(TP_SAT)

# NUM_FRAMES
def fn_validate_numFrames(TP_NUM_FRAMES):
  return fn_validate_minmax_value("TP_NUM_FRAMES", TP_NUM_FRAMES, TP_NUM_FRAMES_min, TP_NUM_FRAMES_max)

def validate_TP_NUM_FRAMES(args):
  TP_NUM_FRAMES = args["TP_NUM_FRAMES"]
  return fn_validate_numFrames(TP_NUM_FRAMES)

  ######### Finished Validation ###########

  ######### Updators ###########

  ######### Graph Generator ############

# Used by higher layer software to figure out how to connect blocks together.
def get_window_sizes(TT_DATA,TP_POINT_SIZE,TP_NUM_FRAMES,TP_CASC_LEN,TP_SSR):
  kSamplesInVect = 256/8/fn_size_by_byte(TT_DATA)
  OUT_WINDOW_VSIZE = (CEIL(TP_POINT_SIZE, (kSamplesInVect * TP_SSR)) / TP_SSR) * TP_NUM_FRAMES
  IN_WINDOW_VSIZE = (CEIL(TP_POINT_SIZE, (kSamplesInVect * TP_CASC_LEN)) / TP_CASC_LEN) * TP_NUM_FRAMES


  return IN_WINDOW_VSIZE, OUT_WINDOW_VSIZE

def info_ports(args):
  """Standard function creating a static dictionary of information
  for upper software to correctly connect the IP.
  Some IP has dynamic number of ports according to parameter set,
  so port information has to be implemented as a function"""
  TT_DATA = args["TT_DATA"]
  TP_POINT_SIZE = args["TP_POINT_SIZE"]
  TP_CASC_LEN = args["TP_CASC_LEN"]
  TP_SSR = args["TP_SSR"]
  TP_NUM_FRAMES = args["TP_NUM_FRAMES"]  
  
  IN_WINDOW_VSIZE, OUT_WINDOW_VSIZE = get_window_sizes(TT_DATA,TP_POINT_SIZE,TP_NUM_FRAMES,TP_CASC_LEN,TP_SSR)

  in_ports = get_port_info("in", "in", TT_DATA, IN_WINDOW_VSIZE, TP_CASC_LEN * TP_SSR, 0)
  out_ports = get_port_info("out", "out", TT_DATA, OUT_WINDOW_VSIZE, TP_SSR, 0)
  return in_ports + out_ports

def generate_graph(graphname, args):

  if graphname == "":
    graphname = "default_graphname"
  TT_DATA = args["TT_DATA"]
  TT_TWIDDLE = args["TT_TWIDDLE"]
  TP_POINT_SIZE = args["TP_POINT_SIZE"]
  TP_FFT_NIFFT = args["TP_FFT_NIFFT"]
  TP_SHIFT = args["TP_SHIFT"]
  TP_CASC_LEN = args["TP_CASC_LEN"]
  TP_NUM_FRAMES = args["TP_NUM_FRAMES"]
  TP_RND = args["TP_RND"]
  TP_SAT = args["TP_SAT"]
  TP_SSR= args["TP_SSR"]


  code = (
f"""
class {graphname} : public adf::graph {{
public:
  // ports
  //template <typename dir>
  
  std::array<adf::port<input>, {TP_SSR} * {TP_CASC_LEN}> in;
  std::array<adf::port<output>, {TP_SSR}> out;

  xf::dsp::aie::fft::dft::dft_graph<
    {TT_DATA}, // TT_DATA
    {TT_TWIDDLE}, // TT_TWIDDLE
    {TP_POINT_SIZE}, // TP_POINT_SIZE
    {TP_FFT_NIFFT}, // TP_FFT_NIFFT
    {TP_SHIFT}, // TP_SHIFT
    {TP_CASC_LEN}, // TP_CASC_LEN
    {TP_NUM_FRAMES}, //TP_NUM_FRAMES
    {TP_RND}, //TP_RND
    {TP_SAT}, //TP_SAT
    {TP_SSR} //TP_SSR


  > dft_graph;

  {graphname}() : dft_graph() {{
    for (int ssrIdx = 0; ssrIdx < {TP_SSR}; ssrIdx++) {{
      for (int cascIdx = 0; cascIdx < {TP_CASC_LEN}; cascIdx++) {{
        adf::connect<> net_in(in[cascIdx + ssrIdx * {TP_CASC_LEN}], dft_graph.in[cascIdx + ssrIdx * {TP_CASC_LEN}]);
      }}
      adf::connect<> net_out(dft_graph.out[ssrIdx], out[ssrIdx]);
    }}
  }}
}};
"""
  )
  out = {}
  out["graph"] = code
  out["port_info"] = info_ports(args)
  out["headerfile"] = "dft_graph.hpp"
  out["searchpaths"] = [
       "L2/include/aie",
       "L2/tests/aie/common/inc",
       "L1/include/aie",
       "L1/src/aie",
       "L1/tests/aie/inc",
       "L1/tests/aie/src"
  ]
  return out

