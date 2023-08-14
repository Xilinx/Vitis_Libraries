
import aie_common as com
from aie_common import fn_is_complex, fn_size_by_byte, isError,isValid
#import aie_common_fir as fir

def validate_data_type_combination(TT_DATA_A, TT_DATA_B):
  checks = [
    com.fn_float_coef(TT_DATA_A, TT_DATA_B), 
    com.fn_int_coef(TT_DATA_A, TT_DATA_B)
  ]
  for check in checks:
    if check["is_valid"] == False:
      return check
  return isValid


def validate_TT_DATA_B(args):
  TT_DATA_A = args["TT_DATA_A"]
  TT_DATA_B = args["TT_DATA_B"]
  return validate_data_type_combination(TT_DATA_A, TT_DATA_B)


  
def validate_data_type_combination(TT_DATA_A, TT_DATA_B):
  checks = [
    com.fn_float_coef(TT_DATA_A, TT_DATA_B), 
    com.fn_int_coef(TT_DATA_A, TT_DATA_B)
  ]
  for check in checks:
    if check["is_valid"] == False:
      return check
  return isValid

def isMultiple(A,B):
  return (A % B == 0)
  

def getOutputType(typeA, typeB) : 
  if (fn_size_by_byte(typeA) > fn_size_by_byte(typeB)) : 
    return typeA
  else : 
    return typeB


def validate_casc(TP_DIM_B, TP_CASC_LEN):
  if (not isMultiple(TP_DIM_B, TP_CASC_LEN)):
    return isError(f"TP_DIM_B ({TP_DIM_B}) needs to be a multiple of TP_CASC_LEN ({TP_CASC_LEN}) ")

  return isValid

  
def validate_TP_SHIFT(args):
    TT_DATA_A = args["TT_DATA_A"]
    TP_SHIFT = args["TP_SHIFT"]
    return com.fn_validate_shift(TT_DATA_A, TP_SHIFT)

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
  TP_SHIFT = args["TP_SHIFT"]
  TP_RND = args["TP_RND"]
  TP_NUM_FRAMES = args["TP_NUM_FRAMES"]
  TP_CASC_LEN = args["TP_CASC_LEN"]

  # Use formatted multi-line string to avoid a lot of \n and \t
  code  = (
f"""
class {graphname} : public adf::graph {{
public:
  constexpr unsigned TP_CASC_LEN = {TP_CASC_LEN};
  std::array<adf::port<input>,TP_CASC_LEN> inA;
  std::array<adf::port<input>,TP_CASC_LEN> inB;
  adf::port<output> out;
  xf::dsp::aie::matrix_vector_mul::matrix_vector_mul_graph<
    {TT_DATA_A}, // TT_DATA_A
    {TT_DATA_B}, // TT_DATA_B
    {TP_DIM_A}, // TP_DIM_A
    {TP_DIM_B}, // TP_DIM_B
    {TP_SHIFT}, // TP_SHIFT
    {TP_RND}, // TP_RND
    {TP_NUM_FRAMES}, // TP_DIM_A_LEADING
    TP_CASC_LEN // TP_CASC_LEN
  > matVecMul;
  
  {graphname}() : matVecMul() {{
    adf::kernel *matVecMul_kernels = matVecMul.getKernels();
    for (int i=0; i < 1; i++) {{
      adf::runtime<ratio>(matVecMul_kernels[i]) = 0.9;
    }}
    for (int i=0; i < TP_CASC_LEN; i++) {{
      adf::connect<> net_inA(inA[i], matVecMul.inA[i]);
      adf::connect<> net_inB(inB[i], matVecMul.inB[i]);
    }}
    adf::connect<> net_out(matVecMul.out[0], out);
  }}

}};
""")
  out = {}
  out["graph"] = code
  out["port_info"] = info_ports(args)
  out["headerfile"] = "matrix_vector_mul_graph.hpp"
  out["searchpaths"] = ["L1/include/aie", "L2/include/aie", "L1/src/aie"]

  return out
