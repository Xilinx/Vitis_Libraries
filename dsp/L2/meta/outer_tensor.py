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

TP_SHIFT_max = 60
BUFFER_BYTES = 32
IO_BYTES_max = 16384
TP_SSR_min = 1
TP_SSR_max = 16

byteSize = {
  "float":4,
  "cfloat":8,
  "int16":2,
  "int32":4,
  "cint16":4,
  "cint32":8
}

aieVariantName = {
  1:"AIE",
  2:"AIE-ML"
}

def fn_print_items(items: list):
  str = ""
  for item in items:
    str += item + ", "
  return str[:-2]


def fn_validate_data_type(TT_DATA, AIE_VARIANT):
  valid_types = ["cint16", "cint32", "int16", "int32", "cfloat", "float"]

  if AIE_VARIANT==2:
    valid_types.remove("float")
    valid_types.remove("cfloat")

  if TT_DATA not in valid_types:
    return isError(f"Input data type for {aieVariantName[AIE_VARIANT]} must be one of [{fn_print_items(valid_types)}] but got {TT_DATA}.")
  return isValid

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

def fn_validate_in_dim_size(ValueStr, TP_DIM, TT_DATA, TP_NUM_FRAMES):
  inSize = TP_DIM * byteSize[TT_DATA] * TP_NUM_FRAMES

  checks = []
  checks.append(fn_validate_power_of_two(ValueStr, inSize))
  checks.append(fn_validate_min_value(ValueStr, inSize, BUFFER_BYTES))
  checks.append(fn_validate_max_value(ValueStr, inSize, IO_BYTES_max))
  for check in checks:
    if check["is_valid"] == False:
      return check
  return isValid

def fn_validate_out_dim_size(TP_DIM_A, TP_DIM_B, TT_DATA_A, TT_DATA_B, TP_NUM_FRAMES, TP_API):
  TT_OUT=fn_det_out_type(TT_DATA_A, TT_DATA_B)

  if TP_API == 0:
    outSize = TP_DIM_A * TP_DIM_B * byteSize[TT_OUT] * TP_NUM_FRAMES
    if outSize > 16384:
      return isError(f"Output cannot exceed {IO_BYTES_max} bytes but currently requires {outSize} bytes.")
  return isValid
# Runs a legality check on the output dimension. Will need changed once interactive GUIs introduced.

def fn_validate_api_val(TP_API):
  if TP_API != 0 and TP_API != 1:
    return isError("TP_API must be 0 (windowed) or streaming (1)")
  return isValid

def fn_validate_ssr(TP_SSR):
  checks = []
  checks.append(fn_validate_minmax_value("TP_SSR", TP_SSR, TP_SSR_min, TP_SSR_max))
  checks.append(fn_validate_power_of_two("TP_SSR", TP_SSR))
  for check in checks:
    if check["is_valid"] == False:
      return check
  return isValid

#### validation APIs ####
def validate_TT_DATA_A(args):
    TT_DATA = args["TT_DATA_A"]
    AIE_VARIANT = args["AIE_VARIANT"]
    return fn_validate_data_type(TT_DATA, AIE_VARIANT)

def validate_TT_DATA_B(args):
    TT_DATA = args["TT_DATA_B"]
    AIE_VARIANT = args["AIE_VARIANT"]
    return fn_validate_data_type(TT_DATA, AIE_VARIANT)

def validate_TP_NUM_FRAMES(args):
    TP_NUM_FRAMES = args["TP_NUM_FRAMES"]
    TP_DIM_A = args["TP_DIM_A"]
    TP_DIM_B = args["TP_DIM_B"]
    TT_DATA_A = args["TT_DATA_A"]
    TT_DATA_B = args["TT_DATA_B"]
    TP_API = args["TP_API"]
    TP_SSR = args["TP_SSR"]

    checks = []
    checks.append(fn_validate_power_of_two("TP_NUM_FRAMES", TP_NUM_FRAMES))
    checks.append(fn_validate_in_dim_size("Input A size (TP_DIM_A * TT_DATA_A bytes * TP_NUM_FRAMES / TP_SSR)", TP_DIM_A // TP_SSR, TT_DATA_A, TP_NUM_FRAMES))
    checks.append(fn_validate_in_dim_size("Input B size (TP_DIM_B * TT_DATA_B bytes * TP_NUM_FRAMES)", TP_DIM_B, TT_DATA_B, TP_NUM_FRAMES))
    checks.append(fn_validate_out_dim_size(TP_DIM_A, TP_DIM_B, TT_DATA_A, TT_DATA_B, TP_NUM_FRAMES, TP_API))
    for check in checks:
      if check["is_valid"] == False:
        return check
    return isValid

def validate_TP_SHIFT(args):
    TP_SHIFT = args["TP_SHIFT"]
    TT_DATA_A = args["TT_DATA_A"]
    TT_DATA_B = args["TT_DATA_B"]
    TT_OUT=fn_det_out_type(TT_DATA_A, TT_DATA_B)
    return fn_validate_shift(TT_OUT, TP_SHIFT)

def validate_TP_API(args):
    TP_API = args["TP_API"]
    return fn_validate_api_val(TP_API)

def validate_TP_SSR(args):
    TP_SSR = args["TP_SSR"]
    return fn_validate_ssr(TP_SSR)

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
# The rest of keys are similar to parameter definition, but with candidates of enum or range values refined
# based on previously set values.
#
# An updator function will always return a dictionary,
# including key "value" for automatically filled default in GUI as dependent parameters have been set, and
# other keys for overriding the definition of parameter.
#
# For example, if a parameter has definition in JSON as
#  { "name": "foo", "type": "typename", "enum": ["int", "float", "double"] }
# And the updator returns
#  { "value": "int", "enum": ["int", "float"] }
# The GUI would show "int" as default and make "int" and "float" selectable candidates, while disabling "double".
#
# If with given combination, no valid value can be set for the parameter being updated, the updater function
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
  TP_DIM_A = args["TP_DIM_A"]
  TP_DIM_B = args["TP_DIM_B"]
  TP_API = args["TP_API"]
  TP_SSR = args["TP_SSR"]
  TP_NUM_FRAMES = args["TP_NUM_FRAMES"]
  if (TP_API==0):
    portsInA = get_port_info(
      portname = "inA",
      dir = "in",
      dataType = TT_DATA_A,
      dim = TP_DIM_A,
      numFrames = TP_NUM_FRAMES,
      apiType = "window",
      vectorLength = TP_SSR
    )
    portsInB = get_port_info(
      portname = "inB",
      dir = "in",
      dataType = TT_DATA_B,
      dim = TP_DIM_B,
      numFrames = TP_NUM_FRAMES,
      apiType = "window",
      vectorLength = TP_SSR
    )
    portsOut = get_port_info(
      portname = "out",
      dir = "out",
      dataType = fn_det_out_type(TT_DATA_A, TT_DATA_B),
      dim = TP_DIM_A*TP_DIM_B,
      numFrames = TP_NUM_FRAMES,
      apiType = "window",
      vectorLength = TP_SSR
    )
  else:
    portsInA = get_port_info(
      portname = "inA",
      dir = "in",
      dataType = TT_DATA_A,
      dim = TP_DIM_A,
      numFrames = TP_NUM_FRAMES,
      apiType = "stream",
      vectorLength = TP_SSR
    )
    portsInB = get_port_info(
      portname = "inB",
      dir = "in",
      dataType = TT_DATA_B,
      dim = TP_DIM_B,
      numFrames = TP_NUM_FRAMES,
      apiType = "stream",
      vectorLength = TP_SSR
    )
    portsOut = get_port_info(
      portname = "out",
      dir = "out",
      dataType = fn_det_out_type(TT_DATA_A, TT_DATA_B),
      dim = TP_DIM_A*TP_DIM_B,
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
  TP_DIM_A = args["TP_DIM_A"]
  TP_DIM_B = args["TP_DIM_B"]
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

  xf::dsp::aie::outer_tensor::outer_tensor_graph<
    {TT_DATA_A}, //TT_DATA_A
    {TT_DATA_B}, //TT_DATA_B
    {TP_DIM_A}, //TP_DIM_A
    {TP_DIM_B}, //TP_DIM_B
    {TP_NUM_FRAMES}, //TP_NUM_FRAMES
    {TP_SHIFT}, //TP_SHIFT
    {TP_API}, //TP_API
    {ssr}, //TP_SSR
  > outer_tensor;

  {graphname}() : outer_tensor() {{
    adf::kernel *outer_tensor_kernels = outer_tensor.getKernels();
    for (int i=0; i < 1; i++) {{
      adf::runtime<ratio>(outer_tensor_kernels[i]) = 0.9;
    }}
    for (int i=0; i < TP_SSR; i++) {{
      adf::connect<> net_in(inA[i], outer_tensor.inA[i]);
      adf::connect<> net_in(inB[i], outer_tensor.inB[i]);
      adf::connect<> net_out(outer_tensor.out[i], out[i]);
    }}
  }}

}};
""")
  out = {}
  out["graph"] = code
  out["port_info"] = info_ports(args)
  out["headerfile"] = "outer_tensor_graph.hpp"
  out["searchpaths"] = [
       "L2/include/aie",
       "L2/tests/aie/common/inc",
       "L1/include/aie",
       "L1/src/aie",
       "L1/tests/aie/inc",
       "L1/tests/aie/src"]

  return out
