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


def fn_validate_coeff_type(TT_DATA, TT_COEFF):
  if ((TT_DATA=="cint16" and TT_COEFF=="int16") or (TT_DATA=="cint32" and TT_COEFF=="int32") or (TT_DATA=="cfloat" and TT_COEFF=="float")):
    return isValid
  else:
    return isError("TT_COEFF must be the atomic type of TT_DATA")

def fn_validate_point_size(TP_POINT_SIZE):
  if (TP_POINT_SIZE in [16,32,64,128,256,512,1024,2048,4096,8192,16384,32768,65536]) :
    return isValid
  else:
    return isError("TP_POINT_SIZE must be a power of 2 and must be between 16 and 65536")

def fn_validate_window_vsize(TP_POINT_SIZE,TP_WINDOW_VSIZE):
  if ((TP_WINDOW_VSIZE in [16,32,64,128,256,512,1024,2048,4096,8192,16384,32768,65536]) and (TP_WINDOW_VSIZE>=TP_POINT_SIZE) and (TP_WINDOW_VSIZE%TP_POINT_SIZE==0)) :
    return isValid
  else:
    return isError("TP_WINDOW_VSIZE must be an integer multiple of TP_POINT_SIZE")

def fn_validate_shift(TT_DATA, TP_SHIFT):
  if (TT_DATA=="cfloat"):
    if (TP_SHIFT==0) :
      return isValid
    else:
      return isError("TP_SHIFT must be 0 for TT_DATA=cfloat")
  if (TT_DATA=="cint32"):
    if (TP_SHIFT>=0 and TP_SHIFT<61) :
      return isValid
    else:
      return isError("TP_SHIFT must be in range 0 to 61 for TT_DATA=cint32")
  if (TT_DATA=="cint16"):
    if (TP_SHIFT>=0 and TP_SHIFT<32) :
      return isValid
    else:
      return isError("TP_SHIFT must be in range 0 to 31 for TT_DATA=cint16")

def fn_validate_ssr(TT_DATA, TP_POINT_SIZE, TP_API, TP_SSR):
  if (TP_POINT_SIZE/TP_SSR >=16 and TP_POINT_SIZE/TP_SSR<=4096) :
    if (TP_POINT_SIZE/TP_SSR<=1024 or TP_API==1) :
      return isValid
    else:
      return isError("TP_POINT_SIZE/TP_SSR must be less than 1024 for windowed configurations")
  else:
    return isError("TP_POINT_SIZE/TP_SSR must be between 16 and 4096")

def fn_validate_dyn_pt_size(TP_POINT_SIZE, TP_SSR, TP_DYN_PT_SIZE):
  if (TP_DYN_PT_SIZE==0 or TP_POINT_SIZE/TP_SSR >=32) :
    return isValid
  else:
    return isError("When TP_DYN_PT_SIZE is selected, TP_POINT_SIZE/TP_SSR must be at least 32")

#### validation APIs ####
def validate_TT_COEFF(args): 
    TT_DATA = args["TT_DATA"]
    TT_COEFF = args["TT_COEFF"]
    return fn_validate_coeff_type(TT_DATA, TT_COEFF)

def validate_TP_POINT_SIZE(args):
    TP_POINT_SIZE = args["TP_POINT_SIZE"]
    return fn_validate_point_size(TP_POINT_SIZE)

def validate_TP_WINDOW_VSIZE(args):
    TP_POINT_SIZE = args["TP_POINT_SIZE"]
    TP_WINDOW_VSIZE = args["TP_WINDOW_VSIZE"]
    return fn_validate_window_vsize(TP_POINT_SIZE,TP_WINDOW_VSIZE)

def validate_TP_SHIFT(args):
    TT_DATA = args["TT_DATA"]
    TP_SHIFT = args["TP_SHIFT"]
    return fn_validate_shift(TT_DATA, TP_SHIFT)

def validate_TP_SSR(args):
    TT_DATA = args["TT_DATA"]
    TP_POINT_SIZE = args["TP_POINT_SIZE"]
    TP_API = args["TP_API"]
    TP_SSR = args["TP_SSR"]
    return fn_validate_ssr(TT_DATA, TP_POINT_SIZE, TP_API, TP_SSR)

def validate_TP_DYN_PT_SIZE(args):
    TP_POINT_SIZE = args["TP_POINT_SIZE"]
    TP_SSR = args["TP_SSR"]
    TP_DYN_PT_SIZE = args["TP_DYN_PT_SIZE"]
    return fn_validate_dyn_pt_size(TP_POINT_SIZE, TP_SSR, TP_DYN_PT_SIZE)


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
def update_coeff(TT_DATA):
  valid_coeffs_default = {"cint16": int16, "cint32": int32, "cfloat": cfloat}
  return {"value": valid_coeffs_default[TT_DATA], "enum": valid_coeffs_default[TT_DATA]}

def update_TT_COEFF(args):
  TT_DATA = args["TP_DATA"]
  return update_coeff(TT_DATA)

def update_window_vsize(TT_POINT_SIZE):
  return {"value": TP_POINT_SIZE, "range": range(TP_POINT_SIZE, 65536, 1)}

def update_TP_WINDOW_VSIZE(args):
  TP_POINT_SIZE = args["TP_POINT_SIZE"]
  return update_window_vsize(TP_POINT_SIZE)

def update_shift(TT_DATA):
  valid_shift_default = {"cint16": 14, "cint32": 30, "cfloat": 0}
  valid_shift_range = {"cint16": 32, "cint32": 61, "cfloat": 0}
  return {"value": valid_shift_default[TT_DATA], "range": range(valid_shift_range[TT_DATA])}

def update_TP_SHIFT(args):
  TT_DATA = args["TT_DATA"]
  return update_shift(TT_DATA)

def update_ssr(TP_POINT_SIZE):
  lower_ssr = TP_POINT_SIZE/4096
  if lower_ssr < 1 :
    lower_ssr = 1
  upper_ssr = TP_POINT_SIZE/65536
  if upper_ssr > 16 :
    upper_ssr = 16
  return {"value": lower_ssr, "range": range(lower_ssr, upper_ssr, 1)}

def update_TP_SSR(args):
  TP_POINT_SIZE = args["TT_POINT_SIZE"]
  return update_ssr(TP_POINT_SIZE)

def update_dyn_pt_size(TP_POINT_SIZE, TP_SSR):
  if (TP_POINT_SIZE/TP_SSR <32) :
    return {"value": 0, "range": 0}
  else :
    return {"value": 0, "range": range(1)}

def update_TP_DYN_PT_SIZE(args) :
  TP_POINT_SIZE = args["TP_POINT_SIZE"]
  TP_SSR = args["TP_SSR"]
  return update_dyn_pt_size(TP_POINT_SIZE, TP_SSR)
  
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
  TT_DATA = args["TT_DATA"]
  TP_API = args["TP_API"]
  TP_SSR = args["TP_SSR"]
  complex = fn_is_complex(TT_DATA)
  TP_WINDOW_VSIZE = args["TP_WINDOW_VSIZE"]
  if (TP_API==0):
    portsIn = get_port_info(
      portname = "in",
      dir = "in",
      dataType = TT_DATA,
      windowVsize = TP_WINDOW_VSIZE,
      apiType = "window",
      vectorLength = TP_SSR
    )
    portsOut = get_port_info(
      portname = "out",
      dir = "out",
      dataType = TT_DATA,
      windowVsize = TP_WINDOW_VSIZE,
      apiType = "window",
      vectorLength = TP_SSR
    )
  else:
    portsIn = get_port_info(
      portname = "in",
      dir = "in",
      dataType = TT_DATA,
      windowVsize = TP_WINDOW_VSIZE,
      apiType = "stream",
      vectorLength = 2* TP_SSR
    )
    portsOut = get_port_info(
      portname = "out",
      dir = "out",
      dataType = TT_DATA,
      windowVsize = TP_WINDOW_VSIZE,
      apiType = "stream",
      vectorLength = 2* TP_SSR
    )
  return portsIn+portsOut


#### graph generator ####
# def get_param_list(**kwargs):
#   [f"{value}{comma}} //{key}" for key, value in kwargs.iteritems() for comma in "," ]
def fn_get_weights_vector(TT_COEFF, weights_list):
  # todo, reformat this to use list comprehension
  weights = f"{{"
  weights += ", ".join([str(weights_list[i]) for i in range(len(weights_list))])
  weights += f"}}"
  return weights

def generate_graph(graphname, args):

  if graphname == "":
    graphname = "default_graphname"

  TT_DATA = args["TT_DATA"]
  TT_COEFF = args["TT_COEFF"]
  TP_POINT_SIZE = args["TP_POINT_SIZE"]
  TP_WINDOW_VSIZE = args["TP_WINDOW_VSIZE"]
  TP_SHIFT = args["TP_SHIFT"]
  TP_API = args["TP_API"]
  TP_SSR = args["TP_SSR"]
  TP_DYN_PT_SIZE = args["TP_DYN_PT_SIZE"]
  coeff_list = args["weights"]

  weights = fn_get_weights_vector(TT_COEFF, coeff_list)

  # Use formatted multi-line string to avoid a lot of \n and \t
  code  = (
f"""
class {graphname} : public adf::graph {{
public:
  static constexpr unsigned int TP_SSR = {TP_SSR};
  template <typename dir>
  using ssr_port_array = std::array<adf::port<dir>, TP_SSR>;

  ssr_port_array<input> in;
  ssr_port_array<output> out;

  std::vector<{TT_COEFF}> weights = {weights};
  xf::dsp::aie::fft::fft_window::fft_window_graph<
    {TT_DATA}, //TT_DATA 
    {TT_COEFF}, //TT_COEFF
    {TP_POINT_SIZE}, //TP_POINT_SIZE 
    {TP_WINDOW_VSIZE}, //TP_WINDOW_VSIZE
    {TP_SHIFT}, //TP_SHIFT
    {TP_API}, //TP_API
    {TP_SSR}, //TP_SSR 
    {TP_DYN_PT_SIZE} //TP_DYN_PT_SIZE 
  > fft_window;
  
  {graphname}() : fft_window(weights) {{
    adf::kernel *fft_window_kernels = fft_window.getKernels();
    for (int i=0; i < 1; i++) {{
      adf::runtime<ratio>(fft_window_kernels[i]) = 0.9;
    }}
    for (int i=0; i < TP_SSR; i++) {{
      adf::connect<> net_in(in[i], fft_window.in[i]);
      adf::connect<> net_out(fft_window.out[i], out[i]);
    }}
  }}

}};
""")
  out = {}
  out["graph"] = code
  out["port_info"] = info_ports(args)
  out["headerfile"] = "fft_window_graph.hpp"
  out["searchpaths"] = [
       "L2/include/aie",
       "L2/tests/aie/common/inc",
       "L1/include/aie",
       "L1/src/aie",
       "L1/tests/aie/inc",
       "L1/tests/aie/src"]

  return out
