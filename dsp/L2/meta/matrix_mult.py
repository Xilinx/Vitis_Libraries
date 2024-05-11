
import aie_common as com
from aie_common import fn_is_complex, fn_size_by_byte, isError,isValid, fn_validate_satMode
#import aie_common_fir as fir

def validate_data_type_combination(TT_DATA_A, TT_DATA_B, AIE_VARIANT):
  checks = [
    com.fn_float_coeff(TT_DATA_A, TT_DATA_B),
    com.fn_int_coeff(TT_DATA_A, TT_DATA_B)
  ]
  for check in checks:
    if check["is_valid"] == False:
      return check
  if AIE_VARIANT == 2 and "float" in TT_DATA_A:
    return isError("Matrix-multiplication does not support floating-point data types for AIE-ML devices")
  return isValid


def validate_TT_DATA_B(args):
  TT_DATA_A = args["TT_DATA_A"]
  TT_DATA_B = args["TT_DATA_B"]
  AIE_VARIANT = args["AIE_VARIANT"]
  return validate_data_type_combination(TT_DATA_A, TT_DATA_B, AIE_VARIANT)


def isMultiple(A,B):
  return (A % B == 0)


def getTilingScheme(typeA, typeB, aie_variant) :
  # needs to be compatible with c++14 -> so just use plain ifs
  # 16b x 16b
  if aie_variant == 1:
    if (typeA == "int16" and typeB == "int16") :
      return (4, 4, 4)
    # 32b x 16b
    if ((typeA == "cint16" or typeA == "int32") and typeB == "int16") :
      return (4, 4, 2)
    # 16b x 32b
    if (typeA == "int16" and (typeB == "cint16" or typeB == "int32")) :
      return (4, 2, 2)
    # 32b x 32b
    if (((typeA == "cint16" or typeA == "int32") and (typeB == "cint16" or typeB == "int32")) or
            typeA == "float" and typeB == "float"  ) :
      return (4, 4, 2)
    # 64b x 16b
    if (typeA == "cint32" and typeB == "int16") :
      return (2, 4, 2)
    # 16b x 64b
    if (typeA == "int16" and typeB == "cint32") :
      return (2, 4, 2) # 4, 4, 2 is also ok
    # 64b x 32b
    if (typeA == "cint32" and (typeB == "cint16" or typeB == "int32")) :
      return (2, 2, 2) # 2, 4, 2 is also ok
    # 32b x 64b
    if ((typeA == "cint16" or typeA == "int32") and typeB == "cint32") :
      return (2, 2, 2)
    # 64b x 64b
    if (typeA == "cint32" and typeB == "cint32") :
      return (2, 2, 2)
    # Mixed Floats
    if ((typeA == "cfloat" and typeB == "float") or
            (typeA == "float"  and typeB == "cfloat") ) :
      return (2, 4, 2) # 2, 2, 2 is also ok
    # cfloats
    if (typeA == "cfloat" and typeB == "cfloat" ) :
      return (4, 2, 2)
  if aie_variant == 2:
    if ((typeA == "int16" or typeA == "int32") and (typeB == "int16" or typeB == "int32")):
      return (4, 4, 4)
    if ((typeA == "cint16" and typeB == "int16")):
      return (4, 4, 4)
    if ((typeA == "cint16" and typeB == "cint16")):
      return (1, 4, 8)
    if ((typeA == "cint32" and typeB == "cint16")):
      return (2, 4, 8)
    if ((typeA == "cint32" and typeB == "cint32")):
      return (1, 2, 8)
  return (0, 0, 0)


def getOutputType(typeA, typeB) :
  if (fn_size_by_byte(typeA) > fn_size_by_byte(typeB)) :
    return typeA
  else :
    return typeB

def validate_dim(AIE_VARIANT, TT_DATA_A, TT_DATA_B, TP_DIM_A=None, TP_DIM_AB=None, TP_DIM_B=None):
  (tileA, tileAB, tileB) = getTilingScheme(TT_DATA_A, TT_DATA_B, AIE_VARIANT)
  if tileA == tileAB == tileB == 0:
    return isError(f'There are no supported multiplication modes for this data type combination (TT_DATA_A={TT_DATA_A}, TT_DATA_B={TT_DATA_B})')
  
  if TP_DIM_A:
    if (not isMultiple(TP_DIM_A, tileA)):
      return isError(f"TP_DIM_A per SSR ({TP_DIM_A}) is not a multiple of the tiling scheme ({tileA}) ")

  if TP_DIM_AB:
    if (not isMultiple(TP_DIM_AB, tileAB)):
      return isError(f"TP_DIM_AB per CASC_LEN ,({TP_DIM_AB}), is not a multiple of the tiling scheme ({tileAB}) ")

  if TP_DIM_B:
    if (not isMultiple(TP_DIM_B, tileB)):
      return isError(f"TP_DIM_B ({TP_DIM_B}) is not a multiple of the tiling scheme ({tileB}) ")

  return isValid





def validate_casc(TT_DATA_A, TT_DATA_B, TP_DIM_A, TP_DIM_AB, TP_DIM_B, TP_CASC_LEN):
  if (not isMultiple(TP_DIM_AB, TP_CASC_LEN)):
    return isError(f"TP_DIM_AB ({TP_DIM_AB}) needs to be a multiple of TP_CASC_LEN ({TP_CASC_LEN}) ")

  return isValid

def validate_casc(TT_DATA_A, TT_DATA_B, TP_DIM_A, TP_DIM_AB, TP_DIM_B, TP_SSR):
  if (not isMultiple(TP_DIM_A, TP_SSR)):
    return isError(f"TP_DIM_A ({TP_DIM_AB}) needs to be a multiple of TP_CASC_LEN ({TP_SSR}) ")

  return isValid

def validate_TP_DIM_A(args):
  AIE_VARIANT = args["AIE_VARIANT"]
  TT_DATA_A = args["TT_DATA_A"]
  TT_DATA_B = args["TT_DATA_B"]
  TP_DIM_A = args["TP_DIM_A"]
  TP_SSR =   args["TP_SSR"]
  return validate_dim(AIE_VARIANT, TT_DATA_A, TT_DATA_B, TP_DIM_A=(TP_DIM_A/TP_SSR))

def validate_TP_DIM_AB(args):
  AIE_VARIANT = args["AIE_VARIANT"]
  TT_DATA_A = args["TT_DATA_A"]
  TT_DATA_B = args["TT_DATA_B"]
  TP_DIM_AB = args["TP_DIM_AB"]
  TP_CASC_LEN = args["TP_CASC_LEN"]
  return validate_dim(AIE_VARIANT, TT_DATA_A, TT_DATA_B, TP_DIM_AB=(TP_DIM_AB/TP_CASC_LEN))

def validate_TP_DIM_B(args):
  AIE_VARIANT = args["AIE_VARIANT"]
  TT_DATA_A = args["TT_DATA_A"]
  TT_DATA_B = args["TT_DATA_B"]
  TP_DIM_B = args["TP_DIM_B"]
  return validate_dim(AIE_VARIANT, TT_DATA_A, TT_DATA_B, TP_DIM_B=TP_DIM_B)


def checkWindow(TT_DATA, WINDOW_SIZE, Rows, Cols, prefix=""):
  if (not WINDOW_SIZE == (Rows * Cols)):
    return isError(f"Batch window processing not currently available")

  # Future compatible only
  if (not isMultiple(WINDOW_SIZE, (Rows * Cols))):
    return isError(f"{prefix}WINDOW_SIZE ({WINDOW_SIZE}) is not a multiple of the matrix size ({(Rows * Cols)}) ")

  maxDataMemBytes = 32768 # bytes
  if (WINDOW_SIZE * fn_size_by_byte(TT_DATA) > maxDataMemBytes):
    return isError(f"{prefix}WINDOW_SIZE ({WINDOW_SIZE * fn_size_by_byte(TT_DATA)}B) must fit within a data memory bank of 32kB.")
  return isValid



def validate_TP_INPUT_WINDOW_VSIZE_A(args):
  TT_DATA_A = args["TT_DATA_A"]
  TP_INPUT_WINDOW_VSIZE_A = args["TP_INPUT_WINDOW_VSIZE_A"]
  TP_DIM_A = args["TP_DIM_A"]
  TP_DIM_AB = args["TP_DIM_AB"]
  TP_CASC_LEN = args["TP_CASC_LEN"]
  TP_SSR = args["TP_SSR"]
  return checkWindow(TT_DATA_A, (TP_INPUT_WINDOW_VSIZE_A/(TP_CASC_LEN*TP_SSR)), (TP_DIM_A/TP_SSR), (TP_DIM_AB/TP_CASC_LEN), "INPUT_A_")

def validate_TP_INPUT_WINDOW_VSIZE_B(args):
  TT_DATA_A = args["TT_DATA_A"]
  TT_DATA_B = args["TT_DATA_B"]
  TP_INPUT_WINDOW_VSIZE_A = args["TP_INPUT_WINDOW_VSIZE_A"]
  TP_INPUT_WINDOW_VSIZE_B = args["TP_INPUT_WINDOW_VSIZE_B"]
  TP_DIM_A = args["TP_DIM_A"]
  TP_DIM_AB = args["TP_DIM_AB"]
  TP_DIM_B = args["TP_DIM_B"]
  TP_CASC_LEN = args["TP_CASC_LEN"]
  TP_SSR = args["TP_SSR"]
  return validate_windows(TT_DATA_A, TT_DATA_B, TP_INPUT_WINDOW_VSIZE_A, TP_INPUT_WINDOW_VSIZE_B, TP_DIM_A, TP_DIM_AB, TP_DIM_B, TP_CASC_LEN, TP_SSR)

def validate_windows(TT_DATA_A, TT_DATA_B, TP_INPUT_WINDOW_VSIZE_A, TP_INPUT_WINDOW_VSIZE_B, TP_DIM_A, TP_DIM_AB, TP_DIM_B, TP_CASC_LEN, TP_SSR):
  checkInputB = checkWindow(TT_DATA_B, (TP_INPUT_WINDOW_VSIZE_B/TP_CASC_LEN), (TP_DIM_AB/TP_CASC_LEN), TP_DIM_B, "INPUT_B_")
  TT_OUT = getOutputType(TT_DATA_A, TT_DATA_B)
  outputWinSize = (TP_DIM_A / TP_SSR) * (TP_DIM_B)
  checkOutput = checkWindow(TT_OUT, outputWinSize, (TP_DIM_A/TP_SSR), TP_DIM_B, "OUTPUT_")
  checks = [
    checkInputB,
    checkOutput
  ]
  for check in checks:
    if check["is_valid"] == False:
      return check
  return isValid




def validate_TP_SHIFT(args):
  TT_DATA_A = args["TT_DATA_A"]
  TP_SHIFT = args["TP_SHIFT"]
  return com.fn_validate_shift(TT_DATA_A, TP_SHIFT)

def validate_TP_SAT(args):
  TP_SAT = args["TP_SAT"]
  return fn_validate_satMode(TP_SAT)


def validate_tiling_for_leading_dim(TT_DATA_A, TT_DATA_B, TP_DIM_A_LEADING=None, TP_DIM_B_LEADING=None, TP_DIM_OUT_LEADING=None, TP_ADD_TILING_A=None, TP_ADD_TILING_B=None, TP_ADD_DETILING_OUT=None):
  TT_DATA_OUT = getOutputType(TT_DATA_A, TT_DATA_B)
  # Check parameters have been passed.
  # Int16 needs weird xsquare and we've limited support since it can't easily tile/detile.
  AHasError =  (TT_DATA_A == "int16" and TP_ADD_TILING_A == 1 and TP_DIM_A_LEADING == 1) if TP_ADD_TILING_A and TP_DIM_A_LEADING else False
  BHasError =  (TT_DATA_B == "int16" and TP_ADD_TILING_B == 1 and TP_DIM_B_LEADING == 1) if TP_ADD_TILING_B and TP_DIM_B_LEADING else False
  OutHasError =  (TT_DATA_OUT == "int16" and TP_ADD_DETILING_OUT == 1 and TP_DIM_OUT_LEADING == 1) if TP_ADD_DETILING_OUT and TP_DIM_OUT_LEADING else False
  def giveTypeTilingError(typephrase, type):
    return isError(f"Unable to provide tiling solution for {typephrase}={type} with column major matrix.")
  if (AHasError):
    return giveTypeTilingError("TT_DATA_A", TT_DATA_A)
  if (BHasError):
    return giveTypeTilingError("TT_DATA_B", TT_DATA_B)
  if (OutHasError):
    return giveTypeTilingError("TT_DATA_OUT", TT_DATA_OUT)
  return isValid

def fn_check_min_matrix_size(TT_DATA, majorDim, commonDim, addTiling):
  matrixSize  = (fn_size_by_byte(TT_DATA) * majorDim * (commonDim))
  #must fill at least a 512b buffer if we're using the tilers
  matrixTooSmall = ( matrixSize < 512//8 and addTiling == 1 )
  return (
    isError("When Tiling the matrix, please ensure that the matrix is at least 512b in size.")
    if matrixTooSmall else
    isValid
  )


def fn_check_min_matrix_sizes(TT_DATA_A, TT_DATA_B, TP_DIM_A=None, TP_DIM_AB=None, TP_DIM_B=None, TP_CASC_LEN=None, TP_SSR=None, TP_ADD_TILING_A=None, TP_ADD_TILING_B=None, TP_ADD_DETILING_OUT=None ):

  TT_DATA_OUT = getOutputType(TT_DATA_A, TT_DATA_B)
  matrixTooSmallA = fn_check_min_matrix_size(TT_DATA_A, TP_DIM_A//TP_SSR, TP_DIM_AB//TP_CASC_LEN, TP_ADD_TILING_A) if TP_DIM_A and TP_ADD_TILING_A else isValid
  matrixTooSmallB = fn_check_min_matrix_size(TT_DATA_B, TP_DIM_B, TP_DIM_AB//TP_CASC_LEN, TP_ADD_TILING_B) if TP_DIM_B and TP_ADD_TILING_B else isValid
  matrixTooSmallOut = fn_check_min_matrix_size(TT_DATA_OUT, TP_DIM_A//TP_SSR, TP_DIM_B, TP_ADD_DETILING_OUT) if TP_DIM_A and TP_DIM_B and TP_ADD_DETILING_OUT else isValid
  return com.fn_return_first_error([matrixTooSmallA, matrixTooSmallB, matrixTooSmallOut])

def validate_tiling(TT_DATA_A, TT_DATA_B, TP_DIM_A=None, TP_DIM_AB=None, TP_DIM_B=None, TP_CASC_LEN=None, TP_SSR=None, TP_DIM_A_LEADING=None, TP_DIM_B_LEADING=None, TP_DIM_OUT_LEADING=None, TP_ADD_TILING_A=None, TP_ADD_TILING_B=None, TP_ADD_DETILING_OUT=None):

  checkInit16ColMajorTiling = validate_tiling_for_leading_dim(TT_DATA_A, TT_DATA_B, TP_DIM_A_LEADING, TP_DIM_B_LEADING, TP_DIM_OUT_LEADING, TP_ADD_TILING_A, TP_ADD_TILING_B, TP_ADD_DETILING_OUT)
  checkMinMatrixSizeTiling = fn_check_min_matrix_sizes(TT_DATA_A, TT_DATA_B, TP_DIM_A, TP_DIM_AB, TP_DIM_B, TP_CASC_LEN, TP_SSR, TP_ADD_TILING_A, TP_ADD_TILING_B, TP_ADD_DETILING_OUT )

  return com.fn_return_first_error([checkInit16ColMajorTiling, checkMinMatrixSizeTiling])


def validate_TP_ADD_TILING_A(args):
  TT_DATA_A = args["TT_DATA_A"]
  TT_DATA_B = args["TT_DATA_B"]
  TP_DIM_A = args["TP_DIM_A"]
  TP_DIM_AB = args["TP_DIM_AB"]
  TP_CASC_LEN = args["TP_CASC_LEN"]
  TP_SSR = args["TP_SSR"]
  TP_DIM_A_LEADING = args["TP_DIM_A_LEADING"]
  TP_ADD_TILING_A = args["TP_ADD_TILING_A"]
  return validate_tiling(TT_DATA_A, TT_DATA_B, TP_DIM_A=TP_DIM_A, TP_DIM_AB=TP_DIM_AB, TP_CASC_LEN=TP_CASC_LEN, TP_SSR=TP_SSR, TP_DIM_A_LEADING=TP_DIM_A_LEADING, TP_ADD_TILING_A=TP_ADD_TILING_A)

def validate_TP_ADD_TILING_B(args):
  TT_DATA_A = args["TT_DATA_A"]
  TT_DATA_B = args["TT_DATA_B"]
  TP_DIM_B = args["TP_DIM_B"]
  TP_DIM_AB = args["TP_DIM_AB"]
  TP_CASC_LEN = args["TP_CASC_LEN"]
  TP_DIM_B_LEADING = args["TP_DIM_B_LEADING"]
  TP_ADD_TILING_B = args["TP_ADD_TILING_B"]
  return validate_tiling(TT_DATA_A, TT_DATA_B, TP_DIM_B=TP_DIM_B, TP_DIM_AB=TP_DIM_AB, TP_CASC_LEN=TP_CASC_LEN, TP_DIM_B_LEADING=TP_DIM_B_LEADING, TP_ADD_TILING_B=TP_ADD_TILING_B)

def validate_TP_ADD_DETILING_OUT(args):
  TT_DATA_A = args["TT_DATA_A"]
  TT_DATA_B = args["TT_DATA_B"]
  TP_DIM_A = args["TP_DIM_A"]
  TP_DIM_B = args["TP_DIM_B"]
  TP_SSR = args["TP_SSR"]
  TP_DIM_OUT_LEADING = args["TP_DIM_OUT_LEADING"]
  TP_ADD_DETILING_OUT = args["TP_ADD_DETILING_OUT"]
  return validate_tiling(TT_DATA_A, TT_DATA_B, TP_DIM_A=TP_DIM_A, TP_DIM_B=TP_DIM_B, TP_SSR=TP_SSR, TP_DIM_OUT_LEADING=TP_DIM_OUT_LEADING, TP_ADD_DETILING_OUT=TP_ADD_DETILING_OUT)




def info_ports(args):

  portsA = com.get_port_info(
    portname = "inA",
    dir = "in",
    TT_DATA = args["TT_DATA_A"],
    windowVSize = (args["TP_INPUT_WINDOW_VSIZE_A"] // (args["TP_SSR"] * args["TP_CASC_LEN"])),
    vectorLength = (args["TP_CASC_LEN"] * args["TP_SSR"])
  )

  portsB = com.get_port_info(
    portname = "inB",
    dir = "in",
    TT_DATA = args["TT_DATA_B"],
    windowVSize = (args["TP_INPUT_WINDOW_VSIZE_B"] // args["TP_CASC_LEN"]),
    vectorLength = args["TP_CASC_LEN"]
  )
  TT_DATA_OUT = getOutputType(args["TT_DATA_A"], args["TT_DATA_B"])
  TP_OUTPUT_WINDOW_VSIZE = ((args["TP_DIM_A"] // args["TP_SSR"]) * args["TP_DIM_B"])
  portsOut = com.get_port_info(
    portname = "out",
    dir = "out",
    TT_DATA = TT_DATA_OUT,
    windowVSize = (TP_OUTPUT_WINDOW_VSIZE),
    vectorLength = args["TP_SSR"]
  )
  # join lists of ports together and return
  return portsA + portsB + portsOut


def generate_graph(graphname, args):
  if graphname == "":
    graphname = "default_graphname"

  TT_DATA_A = args["TT_DATA_A"]
  TT_DATA_B = args["TT_DATA_B"]
  TP_DIM_A = args["TP_DIM_A"]
  TP_DIM_AB = args["TP_DIM_AB"]
  TP_DIM_B = args["TP_DIM_B"]
  TP_SHIFT = args["TP_SHIFT"]
  TP_RND = args["TP_RND"]
  TP_SAT = args["TP_SAT"]
  TP_DIM_A_LEADING = args["TP_DIM_A_LEADING"]
  TP_DIM_B_LEADING = args["TP_DIM_B_LEADING"]
  TP_DIM_OUT_LEADING = args["TP_DIM_OUT_LEADING"]
  TP_ADD_TILING_A = args["TP_ADD_TILING_A"]
  TP_ADD_TILING_B = args["TP_ADD_TILING_B"]
  TP_ADD_DETILING_OUT = args["TP_ADD_DETILING_OUT"]
  TP_INPUT_WINDOW_VSIZE_A = args["TP_INPUT_WINDOW_VSIZE_A"]
  TP_INPUT_WINDOW_VSIZE_B = args["TP_INPUT_WINDOW_VSIZE_B"]
  TP_CASC_LEN = args["TP_CASC_LEN"]
  TP_SSR = args["TP_SSR"]

  # Use formatted multi-line string to avoid a lot of \n and \t
  code  = (
f"""
class {graphname} : public adf::graph {{
public:
  constexpr unsigned TP_CASC_LEN = {TP_CASC_LEN};
  std::array<adf::port<input>,TP_CASC_LEN> inA;
  std::array<adf::port<input>,TP_CASC_LEN> inB;
  adf::port<output> out;
  xf::dsp::aie::blas::matrix_mult::matrix_mult_graph<
    {TT_DATA_A}, // TT_DATA_A
    {TT_DATA_B}, // TT_DATA_B
    {TP_DIM_A}, // TP_DIM_A
    {TP_DIM_AB}, // TP_DIM_AB
    {TP_DIM_B}, // TP_DIM_B
    {TP_SHIFT}, // TP_SHIFT
    {TP_RND}, // TP_RND
    {TP_DIM_A_LEADING}, // TP_DIM_A_LEADING
    {TP_DIM_B_LEADING}, // TP_DIM_B_LEADING
    {TP_DIM_OUT_LEADING}, // TP_DIM_OUT_LEADING
    {TP_ADD_TILING_A}, // TP_ADD_TILING_A
    {TP_ADD_TILING_B}, // TP_ADD_TILING_B
    {TP_ADD_DETILING_OUT}, // TP_ADD_DETILING_OUT
    {TP_INPUT_WINDOW_VSIZE_A}, // TP_INPUT_WINDOW_VSIZE_A
    {TP_INPUT_WINDOW_VSIZE_B} // TP_INPUT_WINDOW_VSIZE_B
    {TP_CASC_LEN} // TP_CASC_LEN
    {TP_SAT} // TP_SAT
    {TP_SSR} // TP_SSR
  > mmult;

  {graphname}() : mmult() {{
    adf::kernel *mmult_kernels = mmult.getKernels();
    for (int i=0; i < 1; i++) {{
      adf::runtime<ratio>(mmult_kernels[i]) = 0.9;
    }}
    for (int i=0; i < TP_CASC_LEN; i++) {{
      adf::connect<> net_inA(inA[i], mmult.inA[i]);
      adf::connect<> net_inB(inB[i], mmult.inB[i]);
    }}
    adf::connect<> net_out(mmult.out[0], out);
  }}

}};
""")
  out = {}
  out["graph"] = code
  out["port_info"] = info_ports(args)
  out["headerfile"] = "matrix_mult_graph.hpp"
  out["searchpaths"] = ["L1/include/aie", "L2/include/aie", "L1/src/aie"]

  return out
