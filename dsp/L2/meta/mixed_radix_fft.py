from aie_common import *
import json
import sys

TP_POINT_SIZE_min = 16
TP_POINT_SIZE_max = 3300

TP_FFT_NIFFT_min = 0
TP_FFT_NIFFT_max = 1

TP_SHIFT_min = 0
TP_SHIFT_max = 61

TP_RND_min = 4
TP_RND_max = 7

TP_WINDOW_VSIZE_min = 16
TP_WINDOW_VSIZE_max = 4096

TP_CASC_LEN_min = 1

TP_API_min = 0
TP_API_max = 1

AIE_VARIANT_min = 1
AIE_VARIANT_max = 2

#----------------------------------------
#Utility functions
def fn_get_radix_stages(TP_POINT_SIZE, radix):
  stages = 0;
  val = TP_POINT_SIZE
  while val % radix == 0:
    val = val/radix;
    stages = stages +1
  return stages


def fn_get_num_stages(TP_POINT_SIZE):
  r4_stages = 0
  r2_stages = fn_get_radix_stages(TP_POINT_SIZE,2)
  r3_stages = fn_get_radix_stages(TP_POINT_SIZE,3)
  r5_stages = fn_get_radix_stages(TP_POINT_SIZE,5)
  if (r2_stages >= 4):
    r4_stages = r2_stages/2
    r2_stages = r2_stages % 2
  return r2_stages+r3_stages+r4_stages+r5_stages

#----------------------------------------
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
    isError(f"Invalid Data/Twiddle type combination ({TT_DATA},{TT_TWIDDLE}). Supported combinations are cint16/cint16, cint32/cint16 and cfloat/cfloat. ")
    )
  )
def validate_TT_TWIDDLE(args):
  TT_DATA = args["TT_DATA"]
  TT_TWIDDLE = args["TT_TWIDDLE"]
  return fn_validate_twiddle_type(TT_DATA, TT_TWIDDLE)

#----------------------------------------
# Validate point size
def fn_validate_point_size(TP_POINT_SIZE,AIE_VARIANT):
  if (TP_POINT_SIZE < TP_POINT_SIZE_min or TP_POINT_SIZE > TP_POINT_SIZE_max) :
      return isError(f"Invalid Point Size ({TP_POINT_SIZE}). It must be in the range {TP_POINT_SIZE_min} to {TP_POINT_SIZE_max}.")

  r2_stages = fn_get_radix_stages(TP_POINT_SIZE,2)
  r3_stages = fn_get_radix_stages(TP_POINT_SIZE,3)
  r5_stages = fn_get_radix_stages(TP_POINT_SIZE,5)

  if AIE_VARIANT == 1: #AIE-1
    vectorizationMin = 8
  elif AIE_VARIANT == 2:
    vectorizationMin = 16
  else:
    return isError(f"Unsupported AIE_VARIANT value detected ({AIE_VARIANT})")

  if (TP_POINT_SIZE % vectorizationMin != 0) :
      return isError(f"Invalid Point Size ({TP_POINT_SIZE}). It must be divisible by ({vectorizationMin}) for this AIE variant.")

  r2_stages = fn_get_radix_stages(TP_POINT_SIZE,2)
  r3_stages = fn_get_radix_stages(TP_POINT_SIZE,3)
  r5_stages = fn_get_radix_stages(TP_POINT_SIZE,5)
  if (TP_POINT_SIZE != 2**r2_stages * 3**r3_stages * 5**r5_stages):
      return isError(f"Invalid Point Size ({TP_POINT_SIZE}). It must factorize completely to 2, 3 and 5")

  return isValid


def validate_TP_POINT_SIZE(args):
  TP_POINT_SIZE = args["TP_POINT_SIZE"]
  AIE_VARIANT   = args["AIE_VARIANT"]
  return fn_validate_point_size(TP_POINT_SIZE, AIE_VARIANT)

#----------------------------------------
# Validate direction (fft_nifft)
def fn_validate_fft_nifft(TP_FFT_NIFFT):
  return (
    isValid if (TP_FFT_NIFFT == 0 or  TP_FFT_NIFFT == 1)
    else (
        isError(f"Invalid transform direction ({TP_FFT_NIFFT}). This must be 0 or 1. ")
    )
  )

def validate_TP_FFT_NIFFT(args):
  TP_FFT_NIFFT = args["TP_FFT_NIFFT"]
  return fn_validate_fft_nifft(TP_FFT_NIFFT)

#----------------------------------------
# Validate SHIFT
def fn_validate_shift(TT_DATA, TP_SHIFT):
  return (
    isValid if (TP_SHIFT == 0 or ((TT_DATA in ("cint16","cint32")) and (TP_SHIFT >= TP_SHIFT_min) and (TP_SHIFT <= TP_SHIFT_max)))
    else
    isError(f"TP_SHIFT must be 0 for cfloat and in the range {TP_SHIFT_min} to {TP_SHIFT_max} for integer data types. Got {TP_SHIFT}.")
  )

def validate_TP_SHIFT(args):
  TP_SHIFT = args["TP_SHIFT"]
  TT_DATA = args["TT_DATA"]
  return fn_validate_shift(TT_DATA, TP_SHIFT)

#----------------------------------------
# Validate rounding
def fn_validate_rnd(TP_RND, AIE_VARIANT=1):
  if AIE_VARIANT == 1:
    if TP_RND == k_rnd_mode_map_aie1["rnd_ceil"] or TP_RND == k_rnd_mode_map_aie1["rnd_floor"]:
      return isError(f"Round mode of {TP_RND} is not supported for mixed radix FFT. For the targeted AIE device, supported values are \n2: rnd_pos_inf,\n3: rnd_neg_inf,\n4: rnd_sym_zero,\n5: rnd_sym_inf,\n6: rnd_conv_even,\n7: rnd_conv_odd")
    else:
      return fn_validate_roundMode(TP_RND, AIE_VARIANT)
  elif AIE_VARIANT == 2:
    if TP_RND == k_rnd_mode_map_aie2["rnd_ceil"] or TP_RND == k_rnd_mode_map_aie2["rnd_floor"] or TP_RND == k_rnd_mode_map_aie2["rnd_sym_floor"] or TP_RND == k_rnd_mode_map_aie2["rnd_sym_ceil"]:
      return isError(f"Round mode of {TP_RND} is not supported for mixed radix FFT. For the targeted AIE device, supported values are \n 8: rnd_neg_inf,\n9: rnd_pos_inf,\n10: rnd_sym_zero,\n11: rnd_sym_inf,\n12: rnd_conv_even,\n13: rnd_conv_odd")
    else:
      fn_validate_roundMode(TP_RND, AIE_VARIANT)
  return isValid

def validate_TP_RND(args):
  TP_RND = args["TP_RND"]
  AIE_VARIANT = args["AIE_VARIANT"]
  return fn_validate_rnd(TP_RND, AIE_VARIANT)

#----------------------------------------
# Validate saturation
def fn_validate_sat(TP_SAT):
  return (
    isValid if (TP_SAT == 0 or TP_SAT == 1 or TP_SAT == 3)
    else (
        isError(f"Invalid saturation mode ({TP_SAT}, must be 0 (unsaturated), 1(asym) or 3(sym)). ")
    )
  )
def validate_TP_SAT(args):
  TP_SAT = args["TP_SAT"]
  return fn_validate_sat(TP_SAT)

#----------------------------------------
# Validate window size
def fn_validate_window_vsize(TP_POINT_SIZE, TP_WINDOW_VSIZE):
  return (
    isValid if (TP_WINDOW_VSIZE % TP_POINT_SIZE == 0 and TP_WINDOW_VSIZE>=TP_WINDOW_VSIZE_min and TP_WINDOW_VSIZE<=TP_WINDOW_VSIZE_max)
    else (
        isError(f"Invalid window_size ({TP_WINDOW_VSIZE}), must be an integer multiple of TP_POINT_SIZE and must be in the range {TP_WINDOW_VSIZE_min} to {TP_WINDOW_VSIZE_max} ")
    )
  )
def validate_TP_WINDOW_VSIZE(args):
  TP_WINDOW_VSIZE = args["TP_WINDOW_VSIZE"]
  TP_POINT_SIZE = args["TP_POINT_SIZE"]
  return fn_validate_window_vsize(TP_POINT_SIZE, TP_WINDOW_VSIZE)

#----------------------------------------
# Validate cascade length
def fn_validate_casc_len(TP_POINT_SIZE, TP_CASC_LEN):
  num_stages = fn_get_num_stages(TP_POINT_SIZE)
  return (
    isValid if (TP_CASC_LEN>=TP_CASC_LEN_min and TP_CASC_LEN<=num_stages)
    else (
        isError(f"Invalid cascade length ({TP_CASC_LEN}), must be in the range 1 to num_stages, which is {num_stages} for TP_POINT_SIZE of {TP_POINT_SIZE} ")
    )
  )
def validate_TP_CASC_LEN(args):
  TP_CASC_LEN = args["TP_CASC_LEN"]
  TP_POINT_SIZE = args["TP_POINT_SIZE"]
  return fn_validate_casc_len(TP_POINT_SIZE, TP_CASC_LEN)

#----------------------------------------
# Validate API type
def fn_validate_api(TP_API):
  return (
    isValid if (TP_API>=TP_API_min and TP_API<=TP_API_max)
    else (
        isError(f"Invalid API type. Must be in the range 0 (iobuffers) to 1 (streams). Got {TP_API} ")
    )
  )
def validate_TP_API(args):
  TP_API = args["TP_API"]
  return fn_validate_api(TP_API)

#----------------------------------------
# Validate AIE variant
def fn_validate_aie_variant(AIE_VARIANT):
  return (
    isValid if (AIE_VARIANT>=AIE_VARIANT_min and AIE_VARIANT<=AIE_VARIANT_max)
    else (
        isError(f"Invalid AIE variant. Must be one of the following set:  1 (AIE1), or 2 (AIE-ML). Got {AIE_VARIANT}. ")
    )
  )
def validate_AIE_VARIANT(args):
  AIE_VARIANT = args["AIE_VARIANT"]
  return fn_validate_aie_variant(AIE_VARIANT)


  ######### Finished Validation ###########

  ######### Updators ###########

  ######### Graph Generator ############

# Used by higher layer software to figure out how to connect blocks together.
def info_ports(args):
  """Standard function creating a static dictionary of information
  for upper software to correctly connect the IP.
  Some IP has dynamic number of ports according to parameter set,
  so port information has to be implemented as a function"""
  TT_DATA = args["TT_DATA"]
  TP_POINT_SIZE = args["TP_POINT_SIZE"]
  TP_WINDOW_VSIZE = args["TP_WINDOW_VSIZE"]
  TP_API = args["TP_API"]
  AIE_VARIANT = args["AIE_VARIANT"]

  if TP_API == 0:
    numPorts = 1
  else:
    if AIE_VARIANT == 1:
      numPorts = 2
    else:
      numPorts = 1

  in_ports = get_port_info("in", "in", TT_DATA, TP_WINDOW_VSIZE//numPorts, numPorts, 0, TP_API)
  out_ports = get_port_info("out", "out", TT_DATA, TP_WINDOW_VSIZE//numPorts, numPorts, 0, TP_API)

  return in_ports + out_ports

def generate_graph(graphname, args):

  if graphname == "":
    graphname = "default_graphname"
  TT_DATA = args["TT_DATA"]
  TT_TWIDDLE = args["TT_TWIDDLE"]
  TP_POINT_SIZE = args["TP_POINT_SIZE"]
  TP_FFT_NIFFT = args["TP_FFT_NIFFT"]
  TP_SHIFT = args["TP_SHIFT"]
  TP_RND = args["TP_RND"]
  TP_SAT = args["TP_SAT"]
  TP_WINDOW_VSIZE = args["TP_WINDOW_VSIZE"]
  TP_CASC_LEN = args["TP_CASC_LEN"]
  TP_API = args["TP_API"]
  AIE_VARIANT = args["AIE_VARIANT"]


  code  = (
f"""

class {graphname} : public adf::graph {{
public:
  // ports
  //template <typename dir>

  static constexpr int kStreamsPerTile = get_input_streams_core_module(); //a device trait
  static constexpr int m_kNumPorts = {TP_API} == 0 ? 1 : kStreamsPerTile; //1 for iobuffer, 2 for streams

  std::array<adf::port<input>,m_kNumPorts> in;
  std::array<adf::port<output>,m_kNumPorts> out;


  xf::dsp::aie::fft::mixed_radix_fft::mixed_radix_fft_graph<
    {TT_DATA}, // TT_DATA
    {TT_TWIDDLE}, // TT_TWIDDLE
    {TP_POINT_SIZE}, // TP_POINT_SIZE
    {TP_FFT_NIFFT}, // TP_FFT_NIFFT
    {TP_SHIFT}, // TP_SHIFT
    {TP_RND}, // TP_RND
    {TP_SAT}, //TP_SAT
    {TP_WINDOW_VSIZE},
    {TP_CASC_LEN},
    {TP_API}
  > mixed_radix_fft_graph;

  {graphname}() : mixed_radix_fft_graph() {{
    if ({TP_API} == 0) {{
      adf::connect<> net_in(in[0], mixed_radix_fft_graph.in[0]);
      adf::connect<> net_out(mixed_radix_fft_graph.out[0], out[0]);
    }} else {{
      for (int i = 0; i<m_kNumPorts ; i++) {{
        adf::connect<> net_in(in[i], mixed_radix_fft_graph.in[i]);
        adf::connect<> net_out(mixed_radix_fft_graph.out[i], out[i]);
      }}
    }}
  }}

}};

""")

  out = {}
  out["graph"] = code
  out["port_info"] = info_ports(args)
  out["headerfile"] = "mixed_radix_fft_graph.hpp"
  out["searchpaths"] = [
       "L2/include/aie",
       "L2/tests/aie/common/inc",
       "L1/include/aie",
       "L1/src/aie",
       "L1/tests/aie/inc",
       "L1/tests/aie/src"]
  return out
