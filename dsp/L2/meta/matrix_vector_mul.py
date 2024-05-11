
import aie_common as com
from aie_common import fn_size_by_byte, isError,isValid, fn_validate_satMode, fn_validate_roundMode
from aie_common_fir import fn_validate_ssr

#import aie_common_fir as fir

TP_CASC_LEN_min = 1
TP_CASC_LEN_max = 16
TP_SSR_min = 1
TP_SSR_max = 16
TP_WINDOW_VSIZE_min = 16
TP_WINDOW_VSIZE_max = 16384

def isMultiple(A,B):
  return (A % B == 0)

def getOutputType(typeA, typeB) :
  if (fn_size_by_byte(typeA) > fn_size_by_byte(typeB)) :
    return typeA
  else :
    return typeB

def fn_validate_data_type_combination(TT_DATA_A, TT_DATA_B):
  checks = [
    com.fn_float_coeff(TT_DATA_A, TT_DATA_B),
    com.fn_int_coeff(TT_DATA_A, TT_DATA_B)
  ]
  for check in checks:
    if check["is_valid"] == False:
      return check
  return isValid

def fn_validate_tp_dim_a(TP_DIM_A, TT_DATA_A, TP_SSR):
  if (not isMultiple(TP_DIM_A, TP_SSR)):
    return isError(f"TP_DIM_A ({TP_DIM_A}) needs to be a multiple of TP_SSR ({TP_SSR}) ")
  if (isMultiple(TP_DIM_A ,TP_SSR*(256 / 8 / fn_size_by_byte(TT_DATA_A)))):
     return isValid
  else:
     return isError(f"Invalid DIM_A! DIM_A ({TP_DIM_A}) should be multiples of samples in calculation multiplied by TP_SSR ({TP_SSR}). Samples in calculation:{TP_SSR*(256 / 8 / fn_size_by_byte(TT_DATA_A))}.")

def fn_validate_tp_dim_b(TP_CASC_LEN, TP_DIM_B, TT_DATA_B):
  if (not isMultiple(TP_DIM_B, TP_CASC_LEN)):
    return isError(f"TP_DIM_B ({TP_DIM_B}) needs to be a multiple of TP_CASC_LEN ({TP_CASC_LEN}) ")
  if (isMultiple(TP_DIM_B, (TP_CASC_LEN * 256 / 8 / fn_size_by_byte(TT_DATA_B)))):
     return isValid
  else:
     return isError(f"Invalid DIM_B! DIM_B ({TP_DIM_B}) should be multiples of samples in calculation multiplied by TP_CASC_LEN ({TP_CASC_LEN}). Samples in calculation: {256 / 8 / fn_size_by_byte(TT_DATA_B)}, TP_CASC_LEN :{TP_CASC_LEN}")

def fn_validate_numFrames(TP_NUM_FRAMES, TP_DIM_A, TP_DIM_B, TP_CASC_LEN, TP_SSR):
  WINDOW_SIZE=TP_NUM_FRAMES*(TP_DIM_A/TP_SSR)*(TP_DIM_B/TP_CASC_LEN)
  if WINDOW_SIZE< TP_WINDOW_VSIZE_min or WINDOW_SIZE > TP_WINDOW_VSIZE_max:
      return isError(f"Minimum and Maximum value for Window Size is {TP_WINDOW_VSIZE_min} and {TP_WINDOW_VSIZE_max},respectively, but calculated Window Size is  {WINDOW_SIZE}.")
  return isValid

def fn_validate_leadDimA(TP_DIM_A_LEADING, TP_NUM_FRAMES, TT_DATA_A):
  if (not TP_DIM_A_LEADING and TP_NUM_FRAMES > 1):
      return isError(f"TP_DIM_A_LEADING ({TP_DIM_A_LEADING}) is not supported for batch processing. Row major Matrix A inputs are only supported when NUM_FRAMES = 1. However, NUM_FRAMES is set to {TP_NUM_FRAMES}")
  if (not TP_DIM_A_LEADING and TT_DATA_A == "int16"):
      return isError(f"Row major Matrix A inputs are not supported when TT_DATA_A = int16. Please provide int16 data in column major format, and set TP_DIM_A_LEADING to 1")
  if (not TP_DIM_A_LEADING and TT_DATA_A == "cint32"):
      return isError(f"Row major Matrix A inputs are not supported when TT_DATA_A = cint32. Please provide cint32 data in column major format, and set TP_DIM_A_LEADING to 1")
  if (not TP_DIM_A_LEADING and TT_DATA_A == "cfloat"):
      return isError(f"Row major Matrix A inputs are not supported when TT_DATA_A = cfloat. Please provide cfloat data in column major format, and set TP_DIM_A_LEADING to 1")
  return isValid

def validate_data_type_combination(TT_DATA_A, TT_DATA_B):
  checks = [
    com.fn_float_coeff(TT_DATA_A, TT_DATA_B),
    com.fn_int_coeff(TT_DATA_A, TT_DATA_B)
  ]
  for check in checks:
    if check["is_valid"] == False:
      return check
  return isValid

def validate_TT_DATA_B(args):
  TT_DATA_A = args["TT_DATA_A"]
  TT_DATA_B = args["TT_DATA_B"]
  return fn_validate_data_type_combination(TT_DATA_A, TT_DATA_B)

def validate_TP_CASC_LEN(args):
  TP_CASC_LEN = args["TP_CASC_LEN"]
  if TP_CASC_LEN < TP_CASC_LEN_min or TP_CASC_LEN > TP_CASC_LEN_max :
    return isError(f"Minimum and maximum value for cascade length is {TP_CASC_LEN_min} and {TP_CASC_LEN_max},respectively, but got {TP_CASC_LEN}.")
  return isValid

def validate_TP_SHIFT(args):
    TT_DATA_A = args["TT_DATA_A"]
    TP_SHIFT = args["TP_SHIFT"]
    return com.fn_validate_shift(TT_DATA_A, TP_SHIFT)

def validate_TP_SAT(args):
  TP_SAT = args["TP_SAT"]
  return fn_validate_satMode(TP_SAT)

def validate_TP_NUM_FRAMES(args):
    TP_NUM_FRAMES = args["TP_NUM_FRAMES"]
    TP_DIM_A = args["TP_DIM_A"]
    TP_DIM_B = args["TP_DIM_B"]
    TP_CASC_LEN = args["TP_CASC_LEN"]
    TP_SSR = args["TP_SSR"]
    return fn_validate_numFrames(TP_NUM_FRAMES, TP_DIM_A, TP_DIM_B, TP_CASC_LEN, TP_SSR)

def validate_TP_DIM_A_LEADING(args):
    TP_DIM_A_LEADING = args["TP_DIM_A_LEADING"]
    TP_NUM_FRAMES = args["TP_NUM_FRAMES"]
    TT_DATA_A = args["TT_DATA_A"]
    return fn_validate_leadDimA(TP_DIM_A_LEADING, TP_NUM_FRAMES, TT_DATA_A)

def validate_TP_DIM_A(args):
    TP_DIM_A = args["TP_DIM_A"]
    TT_DATA_A = args["TT_DATA_A"]
    TP_SSR = args["TP_SSR"]
    return fn_validate_tp_dim_a(TP_DIM_A, TT_DATA_A, TP_SSR)

def validate_TP_DIM_B(args):
    TT_DATA_B = args["TT_DATA_B"]
    TP_DIM_B = args["TP_DIM_B"]
    TP_CASC_LEN = args["TP_CASC_LEN"]
    return fn_validate_tp_dim_b(TP_CASC_LEN, TP_DIM_B, TT_DATA_B)

def validate_TP_SSR(args):
  TP_SSR = args["TP_SSR"]
  if TP_SSR < TP_SSR_min or TP_SSR > TP_SSR_max :
    return isError(f"Minimum and maximum value for SSR is {TP_SSR_min} and {TP_SSR_max},respectively, but got {TP_SSR}.")
  return isValid

def validate_TP_RND(args):
    TP_RND = args["TP_RND"]
    AIE_VARIANT = args["AIE_VARIANT"]
    return fn_validate_roundMode(TP_RND, AIE_VARIANT)

def info_ports(args):
    portsA = com.get_port_info(
        portname = "inA",
        dir = "in",
        TT_DATA = args["TT_DATA_A"],
        windowVSize = (args["TP_NUM_FRAMES"] * args["TP_DIM_A"] * args["TP_DIM_B"]),
        vectorLength = args["TP_CASC_LEN"]
    )
    portsB = com.get_port_info(
        portname = "inB",
        dir = "in",
        TT_DATA = args["TT_DATA_B"],
        windowVSize = (args["TP_NUM_FRAMES"] * args["TP_DIM_B"] / args["TP_CASC_LEN"]),
        vectorLength = args["TP_CASC_LEN"]
    )
    TT_DATA_OUT = getOutputType(args["TT_DATA_A"], args["TT_DATA_B"])
    TP_OUTPUT_WINDOW_VSIZE = (args["TP_DIM_B"] * args["TP_NUM_FRAMES"])
    portsOut = com.get_port_info(
        portname = "out",
        dir = "out",
        TT_DATA = TT_DATA_OUT,
        windowVSize = (TP_OUTPUT_WINDOW_VSIZE),
        vectorLength = None
    )
  # join lists of ports together and return
    return portsA + portsB + portsOut


def generate_graph(graphname, args):
  if graphname == "":
    graphname = "default_graphname"

  TT_DATA_A = args["TT_DATA_A"]
  TT_DATA_B = args["TT_DATA_B"]
  TP_DIM_A = args["TP_DIM_A"]
  TP_DIM_B = args["TP_DIM_B"]
  TP_DIM_A_LEADING = args["TP_DIM_A_LEADING"]
  TP_SHIFT = args["TP_SHIFT"]
  TP_RND = args["TP_RND"]
  TP_NUM_FRAMES = args["TP_NUM_FRAMES"]
  TP_CASC_LEN = args["TP_CASC_LEN"]
  TP_SSR = args["TP_SSR"]
  TP_SAT = args["TP_SAT"]

  # Use formatted multi-line string to avoid a lot of \n and \t
  code  = (
f"""
class {graphname} : public adf::graph {{
public:
  constexpr unsigned TP_CASC_LEN = {TP_CASC_LEN};
  constexpr unsigned TP_SSR = {TP_SSR};
  std::array<adf::port<input>,TP_CASC_LEN * TP_SSR> inA;
  std::array<adf::port<input>,TP_CASC_LEN * TP_SSR> inB;
  std::array<adf::port<output>, TP_SSR> out;
  xf::dsp::aie::matrix_vector_mul::matrix_vector_mul_graph<
    {TT_DATA_A}, // TT_DATA_A
    {TT_DATA_B}, // TT_DATA_B
    {TP_DIM_A}, // TP_DIM_A
    {TP_DIM_B}, // TP_DIM_B
    {TP_SHIFT}, // TP_SHIFT
    {TP_RND}, // TP_RND
    {TP_NUM_FRAMES}, // TP_NUM_FRAMES
    {TP_CASC_LEN}, // TP_CASC_LEN
    {TP_SAT} // TP_SAT
    {TP_SSR} //TP_SSR
    {TP_DIM_A_LEADING} // TP_DIM_A_LEADING

  > matVecMul;
  {graphname}() : matVecMul() {{
    adf::kernel *matVecMul_kernels = matVecMul.getKernels();
    for (int i=0; i < (TP_CASC_LEN * TP_SSR); i++) {{
      adf::runtime<ratio>(matVecMul_kernels[i]) = 0.9;
    }}
    for (int ssrIdx = 0; ssrRank < TP_SSR; ssrIdx++) {{
      for (int cascIdx=0; cascIdx < TP_CASC_LEN; cascIdx++) {{
        adf::connect<> net_inA(inA[cascIdx + ssrIdx * TP_CASC_LEN], matVecMul.inA[cascIdx + ssrIdx * TP_CASC_LEN]);
        adf::connect<> net_inB(inB[cascIdx + ssrIdx * TP_CASC_LEN], matVecMul.inB[cascIdx + ssrIdx * TP_CASC_LEN]);
      }}
        adf::connect<> net_out(matVecMul.out[ssrIdx], out[ssrIdx]);
    }}
  }}

}};
""")
  out = {}
  out["graph"] = code
  out["port_info"] = info_ports(args)
  out["headerfile"] = "matrix_vector_mul_graph.hpp"
  out["searchpaths"] = ["L1/include/aie", "L2/include/aie", "L1/src/aie"]

  return out