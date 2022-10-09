

from aie_common import *
import json
import sys

# fft_ifft_dit_1ch.hpp:821:    static_assert(fnCheckDataType<TT_DATA>(), "ERROR: TT_IN_DATA is not a supported type");
# fft_ifft_dit_1ch.hpp:822:    static_assert(fnCheckDataType<TT_OUT_DATA>(), "ERROR: TT_OUT_DATA is not a supported type");
#   ignore this internal debug aid
# fft_ifft_dit_1ch.hpp:823:    static_assert(fnCheckDataIOType<TT_DATA, TT_OUT_DATA>(), "ERROR: TT_OUT_DATA is not a supported type");
#   ignore this internal debug aid
# fft_ifft_dit_1ch.hpp:824:    static_assert(fnCheckTwiddleType<TT_TWIDDLE>(), "ERROR: TT_TWIDDLE is not a supported type");
# fft_ifft_dit_1ch.hpp:825:    static_assert(fnCheckDataTwiddleType<TT_DATA, TT_TWIDDLE>(), "ERROR: TT_TWIDDLE is incompatible with data type");
# fft_ifft_dit_1ch.hpp:826:    static_assert(fnCheckPointSize<TP_POINT_SIZE>(),
# fft_ifft_dit_1ch.hpp:828:    static_assert(TP_FFT_NIFFT == 0 || TP_FFT_NIFFT == 1, "ERROR: TP_FFT_NIFFT must be 0 (reverse) or 1 (forward)");
# fft_ifft_dit_1ch.hpp:829:    static_assert(fnCheckShift<TP_SHIFT>(), "ERROR: TP_SHIFT is out of range (0 to 60)");
# fft_ifft_dit_1ch.hpp:830:    static_assert(fnCheckShiftFloat<TT_DATA, TP_SHIFT>(),
# fft_ifft_dit_1ch.hpp:832:    static_assert(TP_WINDOW_VSIZE % TP_POINT_SIZE == 0, "ERROR: TP_WINDOW_VSIZE must be a multiple of TP_POINT_SIZE");
# fft_ifft_dit_1ch.hpp:833:    static_assert(TP_WINDOW_VSIZE / TP_POINT_SIZE >= 1, "ERROR: TP_WINDOW_VSIZE must be a multiple of TP_POINT_SIZE")
# fft_ifft_dit_1ch.hpp:834:    static_assert((TP_DYN_PT_SIZE == 0) || (TP_POINT_SIZE != 16),
# fft_ifft_dit_1ch_graph.hpp:152:        static_assert(fnCheckCascLen<TT_DATA, TP_END_RANK, TP_CASC_LEN>(), "Error: TP_CASC_LEN is invalid");
# fft_ifft_dit_1ch_graph.hpp:153:        static_assert(fnCheckCascLen2<TT_DATA, TP_POINT_SIZE, TP_CASC_LEN>(), "Error: 16 point float FFT does not support cascade")
# fft_ifft_dit_1ch_graph.hpp:842:    static_assert(TP_API == kStreamAPI, "Error: Only Stream interface is supported for parallel FFT");
# fft_ifft_dit_1ch_graph.hpp:843:    static_assert(TP_PARALLEL_POWER >= 1 && TP_PARALLEL_POWER < 9,

def fn_validate_twiddle_type(TT_DATA, TT_TWIDDLE):
  validTypeCombos = [
      ("cint16", "cint16"),
      ("cint32", "cint16"),
      ("cfloat", "cfloat")
    ]
  return (
    isValid if ((TT_DATA,TT_TWIDDLE) in validTypeCombos)
    else (
    isError(f"Invalid Data/Twiddle type combination ({TT_DATA},{TT_TWIDDLE}). ")
    )
  )

def validate_TT_TWIDDLE(args):
  TT_DATA = args["TT_DATA"]
  TT_TWIDDLE = args["TT_TWIDDLE"]
  return fn_validate_twiddle_type(TT_DATA, TT_TWIDDLE)

#https://stackoverflow.com/a/57025941
#  every power of 2 has exactly 1 bit set to 1 (the bit in that number's log base-2 index).
# So when subtracting 1 from it, that bit flips to 0 and all preceding bits flip to 1.
# That makes these 2 numbers the inverse of each other so when AND-ing them, we will
#  get 0 as the result.
def fn_is_power_of_two(n):
  return (
    (n & (n-1) == 0) and n!=0 )

# good candidate for aie_common, especially if we want to give better error messages
# finds the first instance of a given paramter name in the list of parameters.
# Retuns None if can't find it
def fn_get_parameter_json(metadata_json, param_name):
  return next((param for param in metadata_json["parameters"] if param["name"] == param_name), None)

# Resolve the dirname of this file, so this is never a relative path from cwd.
import os
path_dirname_to_this_file = os.path.dirname(os.path.abspath(__file__))
#read the metadata json file to get the min supported point size, to avoid duplication and maintence hazard
def fn_get_min_supported_point_size():

  try:
    with open(f"{path_dirname_to_this_file}/fft_ifft_dit_1ch.json") as filePointer:
      fft_metadata_json = json.load(filePointer)
  except Exception as e:
    sys.exit(f"Failed to open metadata json file. {e}")

  # grab the point size object from the json
  pointSizeJson = fn_get_parameter_json(fft_metadata_json, "TP_POINT_SIZE")
  # get the min and  if we don't find minimum, then assume no min
  minPointSize = (pointSizeJson["minimum"]) if pointSizeJson else 0
  return minPointSize



def fn_validate_point_size(TP_POINT_SIZE, TP_DYN_PT_SIZE, TT_DATA, TP_PARALLEL_POWER, TP_API):
  checkPointSizeIsPowerOf2 = isValid if fn_is_power_of_two(TP_POINT_SIZE) else (
    isError(f"Point size ({TP_POINT_SIZE}) must be a power of 2")
  )

  minPointSize = fn_get_min_supported_point_size()
  # You can't switch between multiple point sizes if the max point size specified is also the minimum supported..
  checkDynPointSizeIsMoreThanMinPointSize = (
    isError(f"Point size ({TP_POINT_SIZE}) must be higher than the minimum suppored size ({minPointSize}) when using dynamic point FFT.")
    if (TP_DYN_PT_SIZE == 1 and TP_POINT_SIZE <= minPointSize)
    else isValid
  )

  for check in (checkPointSizeIsPowerOf2,checkDynPointSizeIsMoreThanMinPointSize):
    if check["is_valid"] == False :
      return check

  if (TT_DATA=="cint16"):
    if (TP_API == 0):
      if ((TP_POINT_SIZE>>TP_PARALLEL_POWER) > 2048):
        isError(f"Point size per kernel cannot exceed 2048 for cint16 with Windowed interfaces ")
      else:
        if ((TP_POINT_SIZE>>TP_PARALLEL_POWER) > 4096):
          isError(f"Point size per kernel cannot exceed 4096 for cint16 with Streaming interfaces ")
    else:
      if (TP_API == 0):
        if ((TP_POINT_SIZE>>TP_PARALLEL_POWER) > 1024):
          isError(f"Point size per kernel cannot exceed 1024 for cint32 or cfloat with Windowed interfaces ")
        else:
          if ((TP_POINT_SIZE>>TP_PARALLEL_POWER) > 2048):
            isError(f"Point size per kernel cannot exceed 2048 for cint32 or cfloat with Streaming interfaces ")
  return isValid

def validate_TP_POINT_SIZE(args):
  TP_POINT_SIZE = args["TP_POINT_SIZE"]
  TP_DYN_PT_SIZE = args["TP_DYN_PT_SIZE"]
  TT_DATA = args["TT_DATA"]
  TP_PARALLEL_POWER = args["TP_PARALLEL_POWER"]
  TP_API = args["TP_API"]
  return fn_validate_point_size(TP_POINT_SIZE, TP_DYN_PT_SIZE, TT_DATA, TP_PARALLEL_POWER, TP_API)


def validate_TP_SHIFT(args):
  TP_SHIFT = args["TP_SHIFT"]
  TT_DATA = args["TT_DATA"]
  return fn_validate_shift(TT_DATA, TP_SHIFT)

#assumes n is a power of 2
def fn_log2(n):
  original_n = n
  if not fn_is_power_of_two(n):
    sys.exit("invalid assumption that n is a power of two")
  if n != 0:
    power_cnt = 0
    while n % 2 == 0:
      # keep right shifting until the power of two bit is at the LSB.
      n = n >> 1
      power_cnt+=1
      #print(f"n={n} and iter={power_cnt}")
      if n == 0 :
        sys.exit(f"Something went wrong when log2 {original_n}")
    return power_cnt
  else:
    sys.exit("Can't log2 0")
    #return Inf

def fn_validate_casc_len(TT_DATA, TP_POINT_SIZE, TP_CASC_LEN):
  # Defines how many radix-2 ranks there are.
  log2PointSize = fn_log2(TP_POINT_SIZE)
  # equation for integer ffts is complicated by the fact that odd power of 2 point sizes start with a radix 2 stage
  TP_END_RANK = CEIL(log2PointSize, 2) if TT_DATA != "cfloat" else log2PointSize

  checkCascLenIsNotGreaterThanRanks = (
    isValid if (TP_CASC_LEN <= TP_END_RANK) else
    isError(f"Cascade length is greater than ({TP_END_RANK})")
  )

  return checkCascLenIsNotGreaterThanRanks

def validate_TP_CASC_LEN(args):
  TT_DATA = args["TT_DATA"]
  TP_POINT_SIZE = args["TP_POINT_SIZE"]
  TP_CASC_LEN = args["TP_CASC_LEN"]
  return fn_validate_casc_len(TT_DATA, TP_POINT_SIZE, TP_CASC_LEN)



def fn_validate_window_size(TP_POINT_SIZE, TP_WINDOW_VSIZE, TP_DYN_PT_SIZE):
  # Disable the window_vsize check for dynamic point size, due to incorrectly created caller function's arguments.
  if (TP_DYN_PT_SIZE == 1) :
   return isValid

  if (TP_WINDOW_VSIZE % TP_POINT_SIZE != 0):
    return isError(f"Input window size ({TP_WINDOW_VSIZE}) must be a multiple of point size ({TP_POINT_SIZE}) ")
  return isValid


def validate_TP_WINDOW_VSIZE(args):
  TP_POINT_SIZE = args["TP_POINT_SIZE"]
  TP_WINDOW_VSIZE = args["TP_WINDOW_VSIZE"]
  TP_DYN_PT_SIZE = args["TP_DYN_PT_SIZE"]
  return fn_validate_window_size(TP_POINT_SIZE, TP_WINDOW_VSIZE, TP_DYN_PT_SIZE)



def fn_validate_parallel_power(TP_API, TP_PARALLEL_POWER):
  if (TP_API == 0 and TP_PARALLEL_POWER >=1 ):
    return isError("Only stream interface is supported for parallel FFT.")
  return isValid

def validate_TP_PARALLEL_POWER(args):
  TP_API = args["TP_API"]
  TP_PARALLEL_POWER = args["TP_PARALLEL_POWER"]
  return fn_validate_parallel_power(TP_API, TP_PARALLEL_POWER)



  ######### Finished Validation ###########

  ######### Updators ###########

  ######### Graph Generator ############

# Used by higher layer software to figure out how to connect blocks together.

def get_dyn_pt_port_info(portname, dir, TT_DATA, windowVSize, vectorLength=None, marginSize=0, TP_API=0):
  return [{
    "name" : f"{portname}[{idx}]" if vectorLength else f"{portname}", # portname no index
    "type" : "window" if TP_API==0 else "stream",
    "direction" : f"{dir}",
    "data_type" : TT_DATA,
    "fn_is_complex" : fn_is_complex(TT_DATA),
    "window_size" : fn_input_window_size(windowVSize, TT_DATA) + 32,
    "margin_size": marginSize
  } for idx in range((vectorLength if vectorLength else 1))] # do just one port if vectorLength=None

def info_ports(args):
  """Standard function creating a static dictionary of information
  for upper software to correctly connect the IP.
  Some IP has dynamic number of ports according to parameter set,
  so port information has to be implemented as a function"""
  TT_DATA = args["TT_DATA"]
  TP_WINDOW_VSIZE = args["TP_WINDOW_VSIZE"]
  TP_PARALLEL_POWER=args["TP_PARALLEL_POWER"]
  TP_API = args["TP_API"]
  TP_DYN_PT_SIZE = args["TP_DYN_PT_SIZE"]
  marginSize = 0

  if TP_API == 0 and TP_DYN_PT_SIZE == 0:
    in_ports = get_port_info("in", "in", TT_DATA, TP_WINDOW_VSIZE, 1, 0, TP_API)
    out_ports = get_port_info("out", "out", TT_DATA, TP_WINDOW_VSIZE, 1, 0, TP_API)
  elif TP_API == 0 and TP_DYN_PT_SIZE == 1:
    in_ports = get_dyn_pt_port_info("in", "in", TT_DATA, (TP_WINDOW_VSIZE/2**(TP_PARALLEL_POWER)), 2**TP_PARALLEL_POWER, 0, TP_API)
    out_ports = get_dyn_pt_port_info("out", "out", TT_DATA, (TP_WINDOW_VSIZE/2**(TP_PARALLEL_POWER)), 2**TP_PARALLEL_POWER, 0, TP_API)
  elif TP_API == 1 and TP_DYN_PT_SIZE == 1:
    in_ports = get_dyn_pt_port_info("in", "in", TT_DATA, (TP_WINDOW_VSIZE/2**(TP_PARALLEL_POWER + 1)), 2**(TP_PARALLEL_POWER + 1), 0, TP_API)
    out_ports = get_dyn_pt_port_info("out", "out", TT_DATA, (TP_WINDOW_VSIZE/2**(TP_PARALLEL_POWER + 1)), 2**(TP_PARALLEL_POWER + 1), 0, TP_API)
  else:
    in_ports = get_port_info("in", "in", TT_DATA, (TP_WINDOW_VSIZE/2**(TP_PARALLEL_POWER+1)), 2**(TP_PARALLEL_POWER+1), 0, 1)
    out_ports = get_port_info("out", "out", TT_DATA, (TP_WINDOW_VSIZE/2**(TP_PARALLEL_POWER+1)), 2**(TP_PARALLEL_POWER+1), 0, 1)
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
  TP_DYN_PT_SIZE = args["TP_DYN_PT_SIZE"]
  TP_WINDOW_VSIZE = args["TP_WINDOW_VSIZE"]
  TP_API = args["TP_API"]
  TP_PARALLEL_POWER = args["TP_PARALLEL_POWER"]

  if TP_API == 0 and TP_DYN_PT_SIZE == 0:
    ssr = 1
  elif TP_API == 0 and TP_DYN_PT_SIZE == 1:
    ssr = 2**(TP_PARALLEL_POWER)
  else:
    ssr = 2**(TP_PARALLEL_POWER+1)

  code = (
f"""
class {graphname} : public adf::graph {{
public:
  // ports
  template <typename dir>
  using ssr_port_array = std::array<adf::port<dir>, {ssr}>;

  ssr_port_array<input> in;
  ssr_port_array<output> out;


  xf::dsp::aie::fft::dit_1ch::fft_ifft_dit_1ch_graph<
    {TT_DATA}, // TT_DATA
    {TT_TWIDDLE}, // TT_TWIDDLE
    {TP_POINT_SIZE}, // TP_POINT_SIZE
    {TP_FFT_NIFFT}, // TP_FFT_NIFFT
    {TP_SHIFT}, // TP_SHIFT
    {TP_CASC_LEN}, // TP_CASC_LEN
    {TP_DYN_PT_SIZE}, // TP_DYN_PT_SIZE
    {TP_WINDOW_VSIZE}, // TP_WINDOW_VSIZE
    {TP_API}, // TP_API
    {TP_PARALLEL_POWER} // TP_PARALLEL_POWER
  > fft_graph;

  {graphname}() : fft_graph() {{
    for (int i=0; i < {ssr}; i++) {{
      adf::connect<> net_in(in[i], fft_graph.in[i]);
      adf::connect<> net_out(fft_graph.out[i], out[i]);
    }}
  }}
}};
"""
  )
  out = {}
  out["graph"] = code
  out["port_info"] = info_ports(args)
  out["headerfile"] = "fft_ifft_dit_1ch_graph.hpp"
  out["searchpaths"] = [
       "L2/include/aie",
       "L2/tests/aie/common/inc",
       "L1/include/aie",
       "L1/src/aie",
       "L1/tests/aie/inc",
       "L1/tests/aie/src"
  ]
  return out

