from ctypes import sizeof
from socket import TIPC_SUB_SERVICE
from aie_common import *
import json

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

TP_WINDOW_VSIZE_max = 16384 # ping pong buffers within one bank
TP_DIM_COLS_Min = 1 
TP_DIM_ROWS_Min = 1
TP_NUM_FRAMES_Min = 1
TP_SHIFT_min = 0
TP_SHIFT_max = 60
TP_SSR_min = 1
TP_SSR_max = 16

# validation helper functions
def fn_det_out_type(TT_DATA_A, TT_DATA_B):
  if (TT_DATA_A=="int16" and TT_DATA_B=="int16"):
    return "int16"
  if (TT_DATA_A=="int16" and TT_DATA_B=="int32"):
    return "int32"
  if (TT_DATA_A=="int16" and TT_DATA_B=="cint16"):
    return "cint16"
  if (TT_DATA_A=="int16" and TT_DATA_B=="cint32"):
    return "cint32"
  if (TT_DATA_A=="int32" and TT_DATA_B=="int16"):
    return "int32"
  if (TT_DATA_A=="int32" and TT_DATA_B=="int32"):
    return "int32"
  if (TT_DATA_A=="int32" and TT_DATA_B=="cint16"):
    return "cint32"   
  if (TT_DATA_A=="int32" and TT_DATA_B=="cint32"):
    return "cint32"  
  if (TT_DATA_A=="cint16" and TT_DATA_B=="int16"):
    return "cint16"
  if (TT_DATA_A=="cint16" and TT_DATA_B=="int32"):
    return "cint32"
  if (TT_DATA_A=="cint16" and TT_DATA_B=="cint16"):
    return "cint16"
  if (TT_DATA_A=="cint16" and TT_DATA_B=="cint32"):
    return "cint32"
  if (TT_DATA_A=="cint32" and TT_DATA_B=="int16"):
    return "cint32"
  if (TT_DATA_A=="cint32" and TT_DATA_B=="int32"):
    return "cint32"
  if (TT_DATA_A=="cint32" and TT_DATA_B=="cint16"):
    return "cint32"
  if (TT_DATA_A=="cint32" and TT_DATA_B=="cint32"):
    return "cint32"
  if (TT_DATA_A=="float" and TT_DATA_B=="float"):
    return "float"
  if (TT_DATA_A=="float" and TT_DATA_B=="cfloat"):
    return "cfloat"
  if (TT_DATA_A=="cfloat" and TT_DATA_B=="float"):
    return "cfloat"
  if (TT_DATA_A=="cfloat" or TT_DATA_B=="cfloat"):
    return "cfloat"


# validation function    
def fn_validate_data_type(TT_DATA):
  if ((TT_DATA=="cint16") or (TT_DATA=="cint32") or (TT_DATA=="int16") or (TT_DATA=="int32") or (TT_DATA=="cfloat") or (TT_DATA=="float")):
    return isValid
  else:
    return isError("Data type must be the atomic type of data")

def fn_validate_dim_cols(TP_DIM, TT_DATA, TP_API):
  # TP_DIM_COLS_Max = 256/8/fn_size_by_byte(TT_DATA) # only applicable when TP_API=0
  # if (TP_API==0):
  #   if TP_DIM < TP_DIM_COLS_Min or TP_DIM > TP_DIM_COLS_Max :
  #       return isError(f"Minimum and maximum value for TP_DIM_COLS is {TP_DIM_COLS_Min} and {TP_DIM_COLS_Max} respectively, but got {TP_DIM}. Consider using stream input interface.")
  return isValid

def fn_validate_dim_a_rows(TP_DIM, TT_DATA, VEC_SIZE):
  if (TP_DIM % VEC_SIZE):
    return isError(f"TP_DIM_A_ROWS should be an integer multiple of vector size. Vector size for the data type {TT_DATA} is {VEC_SIZE}. Got TP_DIM_A_ROWS = {TP_DIM}")
  return isValid

def fn_validate_dim_b_rows(TP_DIM, TT_DATA, VEC_SIZE):
  if (TP_DIM % VEC_SIZE):
    return isError(f"TP_DIM_B_ROWS should be an integer multiple of vector size. Vector size for the data type {TT_DATA} is {VEC_SIZE}. Got TP_DIM__ROWS = {TP_DIM}")
  return isValid

def fn_validate_num_frames(OUT_FRAME_SIZE, TT_DATA_A, TT_DATA_B, TP_API, TP_NUM_FRAMES):
  OUT_FRAME_SIZE_BYTES = OUT_FRAME_SIZE * fn_size_by_byte(fn_det_out_type(TT_DATA_A, TT_DATA_B))
  TP_NUM_FRAMES_Max = TP_WINDOW_VSIZE_max / OUT_FRAME_SIZE_BYTES
  if (TP_API==0):
    if TP_NUM_FRAMES < TP_NUM_FRAMES_Min or TP_NUM_FRAMES > TP_NUM_FRAMES_Max :
      return isError(f"For TP_API = {TP_API}, TT_DATA_A = {TT_DATA_A} and TT_DATA_B = {TT_DATA_B}, the min and max values of TP_NUM_FRAMES are {TP_NUM_FRAMES_Min} and {TP_NUM_FRAMES_Max}, but got {TP_NUM_FRAMES}. Size of the passed single frame is {OUT_FRAME_SIZE_BYTES} which exceeds the available data memory.")
  return isValid

def fn_validate_api_val(TP_API):
  if TP_API != 0 and TP_API != 1:
    return isError("TP_API must be 0 (windowed in stream out) or 1 (window in streaming out")
  else:
    return isValid

def fn_validate_aie_variant(AIE_VARIANT):
  if ((AIE_VARIANT==1) or (AIE_VARIANT==2)):
    return isValid
  else:
    return isError("AIE_VARIANT must be set to 0 (AIE) or 1 (AIE-ML).")

def fn_validate_shift_val(TT_DATA_A, TT_DATA_B, TP_SHIFT):
  TT_OUT= fn_det_out_type(TT_DATA_A, TT_DATA_B)
  if TP_SHIFT < TP_SHIFT_min or TP_SHIFT > TP_SHIFT_max :
    return isError(f"Minimum and maximum value for Shift is {TP_SHIFT_min} and {TP_SHIFT_max},respectively, but got {TP_SHIFT}.")
  if (TT_OUT=="cfloat" or TT_OUT=="float" ):
    if (TP_SHIFT==0) :
      return isValid
    else:
      return isError("Shift must be 0 for float/cfloat data type")
    
  if (TT_OUT=="cint32"):
    if (TP_SHIFT>=0 and TP_SHIFT<61) :
      return isValid
    else:
      return isError("Shift must be in range 0 to 61 for cint32 out data type")
    
  if (TT_OUT=="cint16"):
    if (TP_SHIFT>=0 and TP_SHIFT<32) :
      return isValid
    else:
      return isError("Shift must be in range 0 to 31 for cint16 out data type")
    
  if (TT_OUT=="int16"):
    if (TP_SHIFT>=0 and TP_SHIFT<16) :
      return isValid
    else:
      return isError("Shift must be in range 0 to 15 for int16 out data type")
    
  if (TT_OUT=="int32"):
    if (TP_SHIFT>=0 and TP_SHIFT<32) :
      return isValid
    else:
      return isError("Shift must be in range 0 to 31 for int32 out data type")
    
def fn_validate_ssr(TP_DIM_A_COLS, TP_SSR):
  if TP_SSR < TP_SSR_min or TP_SSR > TP_SSR_max:
    return isError(f"Minimum and maximum value for SSR is {TP_SSR_min} and {TP_SSR_max},respectively, but got {TP_SSR}.")
  return isValid
  if (TP_DIM_A_COLS % TP_SSR == 0):
    return isValid
  else:
    return isError("ERROR: Invalid SSR value. TP_DIM_A_COLS must be divisible by TP_SSR.")

#### validation APIs ####
def validate_TT_DATA_A(args):
    TT_DATA = args["TT_DATA_A"]
    return fn_validate_data_type(TT_DATA)

def validate_TT_DATA_B(args):
    TT_DATA = args["TT_DATA_B"]
    return fn_validate_data_type(TT_DATA)

def validate_TP_DIM_A_ROWS(args):
    TP_DIM_A_ROWS = args["TP_DIM_A_ROWS"]
    TT_DATA_A = args["TT_DATA_A"]
    VEC_SIZE = 256/8/fn_size_by_byte(TT_DATA_A)
    return fn_validate_dim_a_rows(TP_DIM_A_ROWS, TT_DATA_A, VEC_SIZE)

def validate_TP_DIM_B_ROWS(args):
    TP_DIM_B_ROWS = args["TP_DIM_B_ROWS"]
    TT_DATA_B = args["TT_DATA_B"]
    VEC_SIZE = 256/8/fn_size_by_byte(TT_DATA_B)
    return fn_validate_dim_b_rows(TP_DIM_B_ROWS, TT_DATA_B, VEC_SIZE)

def validate_TP_DIM_A_COLS(args):
    TP_DIM_A_COLS = args["TP_DIM_A_COLS"]
    TT_DATA_A = args["TT_DATA_A"]
    TP_API = args["TP_API"]
    return fn_validate_dim_cols(TP_DIM_A_COLS, TT_DATA_A, TP_API)

def validate_TP_DIM_B_COLS(args):
    TP_DIM_B_COLS = args["TP_DIM_B_COLS"]
    TT_DATA_B = args["TT_DATA_B"]
    TP_API = args["TP_API"]
    return fn_validate_dim_cols(TP_DIM_B_COLS, TT_DATA_B, TP_API)


def validate_TP_NUM_FRAMES(args):
    TP_NUM_FRAMES = args["TP_NUM_FRAMES"]
    TT_DATA_A = args["TT_DATA_A"]
    TT_DATA_B = args["TT_DATA_B"]
    TP_DIM_A_ROWS = args["TP_DIM_A_ROWS"]
    TP_DIM_A_COLS = args["TP_DIM_A_COLS"]
    TP_DIM_B_ROWS = args["TP_DIM_B_ROWS"]
    TP_DIM_B_COLS = args["TP_DIM_B_COLS"]
    TP_API = args["TP_API"]
    TP_SSR = args["TP_SSR"]
    TP_DIM_A_COLS_SSR = TP_DIM_A_COLS / TP_SSR
    OUT_FRAME_SIZE = TP_DIM_A_ROWS * TP_DIM_A_COLS_SSR * TP_DIM_B_ROWS * TP_DIM_B_COLS
    OUT_WINDOW_SIZE = OUT_FRAME_SIZE
    return fn_validate_num_frames(OUT_FRAME_SIZE, TT_DATA_A, TT_DATA_B, TP_API, TP_NUM_FRAMES)

def validate_TP_SHIFT(args):
    TT_DATA_A = args["TT_DATA_A"]
    TT_DATA_B = args["TT_DATA_B"]
    TP_SHIFT = args["TP_SHIFT"]
    return fn_validate_shift_val(TT_DATA_A, TT_DATA_B, TP_SHIFT)

def validate_TP_API(args):
    TP_API = args["TP_API"]
    return fn_validate_api_val(TP_API)

def validate_TP_SSR(args):
    TP_DIM_A_COLS = args["TP_DIM_A_COLS"]
    TP_SSR = args["TP_SSR"]
    return fn_validate_ssr(TP_DIM_A_COLS, TP_SSR)

def validate_TP_RND(args):
    TP_RND = args["TP_RND"]
    return fn_validate_roundMode(TP_RND, 1)

def validate_TP_SAT(args):
    TP_SAT = args["TP_SAT"]
    return fn_validate_satMode(TP_SAT)

def validate_AIE_VARIANT(args):
    AIE_VARIANT = args["AIE_VARIANT"]
    return fn_validate_aie_variant(AIE_VARIANT)


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
#  { "value": None, "err_message": "With TT_DATA as 'int' there is no valid option for TT_COEFF" }
#
# In this example, the following is the updater for TT_COEF, with TT_DATA as the dependent parameter.
# When GUI generates a wizard, TT_DATA should be required first, as it shows up in parameter list first.
# Once user has provided value for TT_DATA, this function will be called and set the value of TT_COEFF.
# Meanwhile, the candidate shown in wizard based on enum will also be updated.
#

#### updater APIs ####


def update_shift(TT_DATA_A, TT_DATA_B):
  valid_shift_default = {"cint16": 14, "cint32": 30, "int16": 14, "int32": 30, "cfloat": 0, "float": 0}
  valid_shift_range = {"cint16": 32, "cint32": 61, "int16": 16, "int32": 32, "cfloat": 0, "float": 0}
  TT_OUT=fn_det_out_type(TT_DATA_A, TT_DATA_B)
  return {"value": valid_shift_default[TT_OUT], "range": range(valid_shift_range[TT_OUT])}

def update_TP_SHIFT(args):
  TT_DATA_A = args["TT_DATA_A"]
  TT_DATA_B = args["TT_DATA_B"]
  return update_shift(TT_DATA_A, TT_DATA_B)

#### port ####
def get_port_info(portname, dir, dataType, dim, numFrames, apiType, vectorLength):
  return [{
    "name" : f"{portname}[{idx}]",
    "type" : f"{apiType}",
    "direction": f"{dir}",
    "data_type": dataType,
    "fn_is_complex": fn_is_complex(dataType),
    "window_size" : dim*numFrames, #com.fn_input_window_size(windowVsize, dataType),
    "margin_size" : 0
} for idx in range(vectorLength)]

def info_ports(args):
  """Standard function creating a static dictionary of information
  for upper software to correctly connect the IP.
  Some IP has a configurable number of ports according to parameter set,
  so port information has to be implemented as a function"""
  TT_DATA_A = args["TT_DATA_A"]
  TT_DATA_B = args["TT_DATA_B"]
  TP_DIM_A_ROWS = args["TP_DIM_A_ROWS"]
  TP_DIM_A_COLS = args["TP_DIM_A_COLS"]
  TP_DIM_B_ROWS = args["TP_DIM_B_ROWS"]
  TP_DIM_B_COLS = args["TP_DIM_B_COLS"]
  TP_NUM_FRAMES = args["TP_NUM_FRAMES"]
  TP_API = args["TP_API"]
  TP_SSR = args["TP_SSR"]
  if (TP_API==0):
    portsInA = get_port_info(
      portname = "inA",
      dir = "in",
      dataType = TT_DATA_A,
      dim = TP_DIM_A_ROWS * TP_DIM_A_COLS,
      numFrames = TP_NUM_FRAMES,
      apiType = "window",
      vectorLength = TP_SSR
    )
    portsInB = get_port_info(
      portname = "inB",
      dir = "in",
      dataType = TT_DATA_B,
      dim = TP_DIM_B_ROWS * TP_DIM_B_COLS,
      numFrames = TP_NUM_FRAMES,
      apiType = "window",
      vectorLength = TP_SSR
    )
    portsOut = get_port_info(
      portname = "out",
      dir = "out",
      dataType = fn_det_out_type(TT_DATA_A, TT_DATA_B),
      dim = TP_DIM_A_ROWS * TP_DIM_B_ROWS * TP_DIM_A_COLS * TP_DIM_B_COLS,
      numFrames = TP_NUM_FRAMES,
      apiType = "window",
      vectorLength = TP_SSR
    )
  else:
    portsInA = get_port_info(
      portname = "inA",
      dir = "in",
      dataType = TT_DATA_A,
      dim = TP_DIM_A_ROWS * TP_DIM_A_COLS,
      numFrames = TP_NUM_FRAMES,
      apiType = "stream",
      vectorLength = TP_SSR
    )
    portsInB = get_port_info(
      portname = "inB",
      dir = "in",
      dataType = TT_DATA_B,
      dim = TP_DIM_B_ROWS * TP_DIM_B_COLS,
      numFrames = TP_NUM_FRAMES,
      apiType = "stream",
      vectorLength = TP_SSR
    )
    portsOut = get_port_info(
      portname = "out",
      dir = "out",
      dataType = fn_det_out_type(TT_DATA_A, TT_DATA_B),
      dim = TP_DIM_A_ROWS * TP_DIM_B_ROWS * TP_DIM_A_COLS * TP_DIM_B_COLS,
      numFrames = TP_NUM_FRAMES,
      apiType = "stream",
      vectorLength = TP_SSR
    )
  return portsInA+portsInB+portsOut


#### graph generator ####
def generate_graph(graphname, args):

  if graphname == "":
    graphname = "default_graphname"

  TT_DATA_A = args["TT_DATA_A"]
  TT_DATA_B = args["TT_DATA_B"]
  TP_DIM_A_ROWS = args["TP_DIM_A_ROWS"]
  TP_DIM_A_COLS = args["TP_DIM_A_COLS"]
  TP_DIM_B_ROWS = args["TP_DIM_B_ROWS"]
  TP_DIM_B_ROWS = args["TP_DIM_B_COLS"]
  TP_NUM_FRAMES = args["TP_NUM_FRAMES"]
  TP_SHIFT = args["TP_SHIFT"]
  TP_API = args["TP_API"]
  TP_SSR = args["TP_SSR"]

  # if TP_API == 1:
  #   ssr = TP_SSR//2
  # else:
  #   ssr = TP_SSR

  # Use formatted multi-line string to avoid a lot of \n and \t
  code  = (
f"""
class {graphname} : public adf::graph {{
public:
  static constexpr unsigned int TP_SSR = {TP_SSR};
  template <typename dir>
  using ssr_port_array = std::array<adf::port<dir>, TP_SSR>;

  ssr_port_array<input> inA;
  ssr_port_array<input> inB;
  ssr_port_array<output> out;

  xf::dsp::aie::kronecker::kronecker_graph<
    {TT_DATA_A}, //TT_DATA_A
    {TT_DATA_B}, //TT_DATA_B
    {TP_DIM_A_ROWS}, //TP_DIM_A_ROWS
    {TP_DIM_A_COLS}, //TP_DIM_A_COLS
    {TP_DIM_B_ROWS}, //TP_DIM_B_ROWS
    {TP_DIM_A_COLS}, //TP_DIM_B_COLS
    {TP_NUM_FRAMES}, //TP_NUM_FRAMES
    {TP_API}, //TP_API
    {TP_SHIFT}, //TP_SHIFT
    {TP_SSR}, //TP_SSR
  > kronecker;

  {graphname}() : kronecker() {{
    adf::kernel *kronecker_kernels = kronecker.getKernels();
    for (int i=0; i < 1; i++) {{
      adf::runtime<ratio>(kronecker_kernels[i]) = 0.9;
    }}
    for (int i=0; i < TP_SSR; i++) {{
      adf::connect<> net_in(inA[i], kronecker.inA[i]);
      adf::connect<> net_in(inB[i], kronecker.inB[i]);
      adf::connect<> net_out(kronecker.out[i], out[i]);
    }}
  }}

}};
""")
  out = {}
  out["graph"] = code
  out["port_info"] = info_ports(args)
  out["headerfile"] = "kronecker_graph.hpp"
  out["searchpaths"] = [
       "L2/include/aie",
       "L2/tests/aie/common/inc",
       "L1/include/aie",
       "L1/src/aie",
       "L1/tests/aie/inc",
       "L1/tests/aie/src"]

  return out
