from ctypes import sizeof
from socket import TIPC_SUB_SERVICE
from aie_common import *
import json
import math

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
PING_PONG_BUFFER_16kB = 16384
PING_PONG_BUFFER_32kB = 32768
TP_INPUT_NUM_FRAMES_min = 1
TP_SSR_min = 1
TP_SSR_max = 32
TP_SHIFT_min = 0
TP_SHIFT_max = 60
TP_SHIFT_cint16_max = 32
TP_CASC_LEN_min = 1
TP_CASC_LEN_max = 11
#TP_API_min=0
#TP_API_max=1
#TP_DYN_PT_SIZE_min=0
#TP_DYN_PT_SIZE_max=1
#AIE_VARIANT_min=2
#AIE_VARIANT_max=1

def fn_validate_data_type(TT_DATA):
  if ((TT_DATA=="cint16") or (TT_DATA=="cint32") or (TT_DATA=="int16") or (TT_DATA=="int32") or (TT_DATA=="cfloat") or (TT_DATA=="float")):
      return isValid
  else:
    return isError("Data type must be the atomic type of data.")

def fn_validate_dim_size(AIE_VARIANT,TP_DIM_PADDED, TP_DIM, TP_API, TP_SSR, TT_DATA_A, TT_DATA_B):
  if (AIE_VARIANT==1):
    PING_PONG_BUFFER_SIZE = PING_PONG_BUFFER_16kB
  if (AIE_VARIANT==2):
    PING_PONG_BUFFER_SIZE = PING_PONG_BUFFER_32kB

  TP_DIM_BYTE = TP_DIM_PADDED * fn_det_calc_byte(TT_DATA_A, TT_DATA_B)
  VEC_IN_FRAME = calc_vecinframe(TP_API,TT_DATA_A,TT_DATA_B)
  TP_DIM_max = math.floor(PING_PONG_BUFFER_SIZE / fn_det_calc_byte(TT_DATA_A, TT_DATA_B))

  if TP_DIM_PADDED < VEC_IN_FRAME or TP_DIM_BYTE > PING_PONG_BUFFER_SIZE :
    return isError(f"Minimum and maximum value for Point Size is {VEC_IN_FRAME} and {TP_DIM_max},respectively, but got {TP_DIM}.")
  
  if (TP_DIM % TP_SSR) != 0:
    return isError(f"TP_DIM should be multiple of TP_SSR. TP_DIM and TP_SSR are {TP_DIM} and {TP_SSR},respectively.")

  if (TP_DIM/TP_SSR >=16 and TP_DIM/TP_SSR<=4096):
    if (TP_DIM/TP_SSR<=1024 or TP_API==1) :
      return isValid
    else:
      return isError(f"(Vector size/SSR) must be less than 1024 for windowed configurations. Got TP_DIM {TP_DIM} and TP_SSR {TP_SSR}")
  else:
    return isError(f"(Vector size/SSR) must be between 16 and 4096. Got TP_DIM {TP_DIM} and TP_SSR {TP_SSR}")


def fn_validate_num_frames(AIE_VARIANT, TP_INPUT_NUM_FRAME, TP_DIM_PADDED_BYTE):
  if (AIE_VARIANT==1):
    PING_PONG_BUFFER_SIZE = PING_PONG_BUFFER_16kB
  if (AIE_VARIANT==2):
    PING_PONG_BUFFER_SIZE = PING_PONG_BUFFER_32kB

  TP_INPUT_NUM_FRAMES_max = math.floor(PING_PONG_BUFFER_SIZE / TP_DIM_PADDED_BYTE)
  if TP_INPUT_NUM_FRAME < TP_INPUT_NUM_FRAMES_min or TP_INPUT_NUM_FRAME > TP_INPUT_NUM_FRAMES_max :
    return isError(f"Minimum and maximum value for Num Frame is {TP_INPUT_NUM_FRAMES_min} and {TP_INPUT_NUM_FRAMES_max},respectively, but got {TP_INPUT_NUM_FRAME}.")
  else:
    return isValid
  
def fn_det_out_type(TT_DATA_A, TT_DATA_B):
  if (TT_DATA_A=="int16" and TT_DATA_B=="int16"):
    return "int16"
  if (TT_DATA_A=="int16" and TT_DATA_B=="int32"):
    return "int32"
  if (TT_DATA_A=="int16" and TT_DATA_B=="cint16"):
    return "cint16"
  if (TT_DATA_A=="int32" and TT_DATA_B=="int16"):
    return "int32"
  if (TT_DATA_A=="int32" and TT_DATA_B=="int32"):
    return "int32"
  if (TT_DATA_A=="int32" and TT_DATA_B=="cint16"):
    return "cint32"    
  if (TT_DATA_A=="cint16" and TT_DATA_B=="int16"):
    return "cint16"
  if (TT_DATA_A=="cint16" and TT_DATA_B=="int32"):
    return "cint32"
  if (TT_DATA_A=="cint16" and TT_DATA_B=="cint16"):
    return "cint16"
  if (TT_DATA_A=="cint32" or TT_DATA_B=="cint32"):
    return "cint32"
  if (TT_DATA_A=="float" and TT_DATA_B=="float"):
    return "float"
  if (TT_DATA_A=="cfloat" or TT_DATA_B=="cfloat"):
    return "cfloat"
  
def fn_det_calc_byte(TT_DATA_A, TT_DATA_B):
  if (TT_DATA_A=="int16" and TT_DATA_B=="int16"):
    return 2
  if (TT_DATA_A=="int16" and TT_DATA_B=="int32"):
    return 4
  if (TT_DATA_A=="int16" and TT_DATA_B=="cint16"):
    return 4
  if (TT_DATA_A=="int16" and TT_DATA_B=="cint32"):
    return 2
  if (TT_DATA_A=="int32" and TT_DATA_B=="int16"):
    return 4
  if (TT_DATA_A=="int32" and TT_DATA_B=="int32"):
    return 4
  if (TT_DATA_A=="int32" and TT_DATA_B=="cint16"):
    return 8    
  if (TT_DATA_A=="int32" and TT_DATA_B=="cint32"):
    return 8   
  if (TT_DATA_A=="cint16" and TT_DATA_B=="int16"):
    return 4
  if (TT_DATA_A=="cint16" and TT_DATA_B=="int32"):
    return 8
  if (TT_DATA_A=="cint16" and TT_DATA_B=="cint16"):
    return 4
  if (TT_DATA_A=="cint16" and TT_DATA_B=="cint32"):
    return 8
  if (TT_DATA_A=="cint32" and TT_DATA_B=="int16"):
    return 2
  if (TT_DATA_A=="cint32" and TT_DATA_B=="int32"):
    return 8
  if (TT_DATA_A=="cint32" and TT_DATA_B=="cint16"):
    return 8
  if (TT_DATA_A=="cint32" and TT_DATA_B=="cint32"):
    return 8
  if (TT_DATA_A=="float" and TT_DATA_B=="float"):
    return 4
  if (TT_DATA_A=="cfloat" or TT_DATA_B=="cfloat"):
    return 8
        
def fn_validate_shift_val(TT_DATA_A, TT_DATA_B, TP_SHIFT):
  TT_OUT=fn_det_out_type(TT_DATA_A, TT_DATA_B)
  if TP_SHIFT < TP_SHIFT_min or TP_SHIFT > TP_SHIFT_max :
    return isError(f"Minimum and maximum value for Shift is {TP_SHIFT_min} and {TP_SHIFT_max},respectively, but got {TP_SHIFT}.")
  
  if (TT_OUT=="cfloat" or TT_OUT=="float" ):
    if (TP_SHIFT==0) :
      return isValid
    else:
      return isError(f"Shift must be 0 for float/cfloat data type. Got {TP_SHIFT}.")
    
  if (TP_SHIFT>=TP_SHIFT_min and TP_SHIFT<=TP_SHIFT_max) :
    return isValid
  else:
    return isError(f"Shift must be in range {TP_SHIFT_min} to {TP_SHIFT_max}. Got {TP_SHIFT}.")
        
def fn_validate_ssr(TP_SSR):
  if TP_SSR < TP_SSR_min or TP_SSR > TP_SSR_max:
    return isError(f"Minimum and maximum value for SSR is {TP_SSR_min} and {TP_SSR_max},respectively, but got {TP_SSR}.")
  else:
    return isValid

def calc_vecinframe(TP_API,TT_DATA_A,TT_DATA_B):
    if(TP_API==0):
      if (TT_DATA_A=="int16" and TT_DATA_B=="cint32"):
        VEC_IN_FRAME=16/fn_det_calc_byte(TT_DATA_A, TT_DATA_B)
      elif (TT_DATA_A=="cint32" and TT_DATA_B=="int16"):
        VEC_IN_FRAME=16/fn_det_calc_byte(TT_DATA_A, TT_DATA_B)
      else: 
        VEC_IN_FRAME=32/fn_det_calc_byte(TT_DATA_A, TT_DATA_B)
    elif(TP_API==1):
      VEC_IN_FRAME=16/fn_det_calc_byte(TT_DATA_A, TT_DATA_B)
    return VEC_IN_FRAME

def calc_padded_tp_dim(VEC_IN_FRAME, TP_DIM, TP_SSR):
    TP_DIM_perSSR = TP_DIM / TP_SSR
    TP_DIM_PADDED = (math.ceil(TP_DIM_perSSR / VEC_IN_FRAME)) * VEC_IN_FRAME
    return TP_DIM_PADDED

def calc_window_size(TP_DIM_PADDED,TP_NUM_FRAMES):
    TP_WINDOW_VSIZE = TP_NUM_FRAMES * TP_DIM_PADDED
    return TP_WINDOW_VSIZE

#### validation APIs ####
def validate_TT_DATA_A(args):
    TT_DATA_A     = args["TT_DATA_A"]
    TT_DATA_B     = args["TT_DATA_B"]
    AIE_VARIANT   = args["AIE_VARIANT"]
    if (AIE_VARIANT==2) and ((TT_DATA_A=="cint16" and TT_DATA_B=="int32") or (TT_DATA_A=="int32" and TT_DATA_B=="cint16")):
      return isError(f"{TT_DATA_A} and {TT_DATA_B} data combination is not supported by AIE Variant {AIE_VARIANT}.")

    if ((TT_DATA_A=="cfloat" or TT_DATA_A=="float") and (TT_DATA_B=="int16" or TT_DATA_B=="int32"or TT_DATA_B=="cint16" or TT_DATA_B=="cint32")):
      return isError("Data types float/cfloat cannot be multiplied with integers!")
    elif ((TT_DATA_B=="cfloat" or TT_DATA_B=="float") and (TT_DATA_A=="int16" or TT_DATA_A=="int32"or TT_DATA_A=="cint16" or TT_DATA_A=="cint32")):
      return isError("Data types float/cfloat cannot be multiplied with integers!")
    else:
      return fn_validate_data_type(TT_DATA_A)

def validate_TT_DATA_B(args):
    TT_DATA     = args["TT_DATA_B"]
    return fn_validate_data_type(TT_DATA)

def validate_TP_DIM(args):
    TP_DIM = args["TP_DIM"]
    TP_API = args["TP_API"]
    TP_SSR = args["TP_SSR"]
    TT_DATA_A = args["TT_DATA_A"]
    TT_DATA_B = args["TT_DATA_B"]
    AIE_VARIANT = args["AIE_VARIANT"]
    VEC_IN_FRAME = calc_vecinframe(TP_API,TT_DATA_A,TT_DATA_B)
    TP_DIM_PADDED = calc_padded_tp_dim(VEC_IN_FRAME, TP_DIM, TP_SSR)
    return fn_validate_dim_size(AIE_VARIANT,TP_DIM_PADDED, TP_DIM, TP_API, TP_SSR, TT_DATA_A, TT_DATA_B)

def validate_TP_NUM_FRAMES(args):
    TP_NUM_FRAMES = args["TP_NUM_FRAMES"]
    TP_DIM = args["TP_DIM"]
    TP_API = args["TP_API"]
    TT_DATA_A = args["TT_DATA_A"]
    TT_DATA_B = args["TT_DATA_B"]
    TP_SSR = args["TP_SSR"]
    AIE_VARIANT = args["AIE_VARIANT"]
    VEC_IN_FRAME = calc_vecinframe(TP_API,TT_DATA_A,TT_DATA_B)
    TP_DIM_PADDED = calc_padded_tp_dim(VEC_IN_FRAME, TP_DIM, TP_SSR)
    OUT_BYTE = fn_det_calc_byte(TT_DATA_A, TT_DATA_B)
    TP_DIM_PADDED_BYTE = TP_DIM_PADDED * OUT_BYTE
    return fn_validate_num_frames(AIE_VARIANT, TP_NUM_FRAMES, TP_DIM_PADDED_BYTE)

def validate_TP_SHIFT(args):
    TT_DATA_A = args["TT_DATA_A"]
    TT_DATA_B = args["TT_DATA_B"]
    TP_SHIFT = args["TP_SHIFT"]
    return fn_validate_shift_val(TT_DATA_A, TT_DATA_B, TP_SHIFT)

def validate_TP_SSR(args):
    TP_SSR = args["TP_SSR"]
    return fn_validate_ssr(TP_SSR)

def validate_TP_SAT(args):
    TP_SAT = args["TP_SAT"]
    return fn_validate_satMode(TP_SAT)

def validate_TP_API(args):
    AIE_VARIANT = args["AIE_VARIANT"]
    TP_API = args["TP_API"]

    if (AIE_VARIANT == 2) and (TP_API == 1):
      return isError(f"AIE Variant {AIE_VARIANT} does not support stream interface for Hadamard IP.")
    return isValid

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

def update_window_vsize(TP_DIM):
  return {"value": TP_DIM, "range": range(TP_DIM, 65536, 1)}

def update_TP_NUM_FRAMES(args):
  TP_DIM = args["TP_DIM"]
  return update_window_vsize(TP_DIM)

def update_shift(TT_DATA_A, TT_DATA_B):
  valid_shift_default = {"cint16": 14, "cint32": 30, "int16": 14, "int32": 30, "cfloat": 0, "float": 0}
  valid_shift_range = {"cint16": 32, "cint32": 61, "int16": 16, "int32": 32, "cfloat": 0, "float": 0}
  TT_OUT=fn_det_out_type(TT_DATA_A, TT_DATA_B)
  return {"value": valid_shift_default[TT_OUT], "range": range(valid_shift_range[TT_OUT])}

def update_TP_SHIFT(args):
  TT_DATA_A = args["TT_DATA_A"]
  TT_DATA_B = args["TT_DATA_B"]
  return update_shift(TT_DATA_A, TT_DATA_B)

def update_ssr(TP_DIM):
  lower_ssr = TP_DIM/4096
  if lower_ssr < 1 :
    lower_ssr = 1
  upper_ssr = TP_DIM/65536
  if upper_ssr > 16 :
    upper_ssr = 16
  return {"value": lower_ssr, "range": range(lower_ssr, upper_ssr, 1)}

def update_TP_SSR(args):
  TP_DIM = args["TT_POINT_SIZE"]
  return update_ssr(TP_DIM)

#### port ####
def get_port_info(portname, dir, dataType, windowVsize, apiType, vectorLength):
  windowSize = windowVsize*fn_size_by_byte(dataType)
  return [{
    "name" : f"{portname}[{idx}]",
    "type" : f"{apiType}",
    "direction": f"{dir}",
    "data_type": dataType,
    "fn_is_complex": fn_is_complex(dataType),
    "window_size" : windowSize, #com.fn_input_window_size(windowVsize, dataType),
    "margin_size" : 0
} for idx in range(vectorLength)]

def info_ports(args):
  """Standard function creating a static dictionary of information
  for upper software to correctly connect the IP.
  Some IP has a configurable number of ports according to parameter set,
  so port information has to be implemented as a function"""
  TT_DATA_A = args["TT_DATA_A"]
  TT_DATA_B = args["TT_DATA_B"]
  TP_API = args["TP_API"]
  TP_SSR = args["TP_SSR"]
  TP_NUM_FRAMES = args["TP_NUM_FRAMES"]
  TP_DIM = args["TP_DIM"]

  VEC_IN_FRAME = calc_vecinframe(TP_API,TT_DATA_A,TT_DATA_B)
  TP_DIM_PADDED = calc_padded_tp_dim(VEC_IN_FRAME, TP_DIM, TP_SSR)
  TP_WINDOW_VSIZE = calc_window_size(TP_DIM_PADDED, TP_NUM_FRAMES)
  
  if (TP_API==0):
    portsInA = get_port_info(
      portname = "inA",
      dir = "in",
      dataType = TT_DATA_A,
      windowVsize = TP_WINDOW_VSIZE,
      apiType = "window",
      vectorLength = TP_SSR
    )
    portsInB = get_port_info(
      portname = "inB",
      dir = "in",
      dataType = TT_DATA_B,
      windowVsize = TP_WINDOW_VSIZE,
      apiType = "window",
      vectorLength = TP_SSR
    )
    portsOut = get_port_info(
      portname = "out",
      dir = "out",
      dataType = fn_det_out_type(TT_DATA_A, TT_DATA_B),
      windowVsize = TP_WINDOW_VSIZE,
      apiType = "window",
      vectorLength = TP_SSR
    )
  else:
    portsInA = get_port_info(
      portname = "inA",
      dir = "in",
      dataType = TT_DATA_A,
      windowVsize = TP_WINDOW_VSIZE,
      apiType = "stream",
      vectorLength = TP_SSR
    )
    portsInB = get_port_info(
      portname = "inB",
      dir = "in",
      dataType = TT_DATA_B,
      windowVsize = TP_WINDOW_VSIZE,
      apiType = "stream",
      vectorLength = TP_SSR
    )
    portsOut = get_port_info(
      portname = "out",
      dir = "out",
      dataType = fn_det_out_type(TT_DATA_A, TT_DATA_B),
      windowVsize = TP_WINDOW_VSIZE,
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
  TP_DIM = args["TP_DIM"]
  TP_NUM_FRAMES = args["TP_NUM_FRAMES"]
  TP_SHIFT = args["TP_SHIFT"]
  TP_API = args["TP_API"]
  TP_SSR = args["TP_SSR"]

  if TP_API == 1:
    ssr = TP_SSR//2
  else:
    ssr = TP_SSR

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

  xf::dsp::aie::hadamard::hadamard_graph<
    {TT_DATA_A}, //TT_DATA_A
    {TT_DATA_B}, //TT_DATA_B
    {TP_DIM}, //TP_DIM
    {TP_NUM_FRAMES}, //TP_NUM_FRAMES
    {TP_SHIFT}, //TP_SHIFT
    {TP_API}, //TP_API
    {ssr}, //TP_SSR
  > hadamard;

  {graphname}() : hadamard() {{
    adf::kernel *hadamard_kernels = hadamard.getKernels();
    for (int i=0; i < 1; i++) {{
      adf::runtime<ratio>(hadamard_kernels[i]) = 0.9;
    }}
    for (int i=0; i < TP_SSR; i++) {{
      adf::connect<> net_in(inA[i], hadamard.inA[i]);
      adf::connect<> net_in(inB[i], hadamard.inB[i]);
      adf::connect<> net_out(hadamard.out[i], out[i]);
    }}
  }}

}};
""")
  out = {}
  out["graph"] = code
  out["port_info"] = info_ports(args)
  out["headerfile"] = "hadamard_graph.hpp"
  out["searchpaths"] = [
       "L2/include/aie",
       "L2/tests/aie/common/inc",
       "L1/include/aie",
       "L1/src/aie",
       "L1/tests/aie/inc",
       "L1/tests/aie/src"]

  return out
