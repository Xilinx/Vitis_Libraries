from ctypes import sizeof
import aie_common as com
from aie_common import isError,isValid

#----------------------------------------
# Validate Twiddle type 
def fn_validate_out_data(TT_DATA, TT_OUT_DATA):
  validTypeCombos = [
      ("cint16", "int16"),
      ("cint32", "int32"),
      ("cfloat", "float"),
      ("int16",  "cint16"),
      ("int32",  "cint32"),
      ("float",  "cfloat")
    ]
  return (
    isValid if ((TT_DATA,TT_OUT_DATA) in validTypeCombos)
    else (
        isError(f"Invalid Data in to Data out type combination ({TT_DATA},{TT_OUT_DATA}). Supported combinations are cint16/int16, cint32/int32, cfloat/float, int16/cint16, int32/cint32, float/cfloat")
    )
  )
def validate_TT_OUT_DATA(args):
  TT_DATA = args["TT_DATA"]
  TT_OUT_DATA = args["TT_OUT_DATA"]
  return fn_validate_out_data(TT_DATA, TT_OUT_DATA)

  ######### Graph Generator ############

# Used by higher layer software to figure out how to connect blocks together. 
def info_ports(args):
  TT_DATA = args["TT_DATA"]
  TT_OUT_DATA = args["TT_OUT_DATA"]
  TP_WINDOW_VSIZE = args["TP_WINDOW_VSIZE"]
  in_port  = com.get_port_info("in",  "in",  TT_DATA, TP_WINDOW_VSIZE, 1, 0, 0)
  out_port = com.get_port_info("out", "out", TT_OUT_DATA, TP_WINDOW_VSIZE, 1, 0, 0)

  return (in_port+out_port) # concat lists

def gen_ports_code(args): 
  in_port  = (f"  std::array<adf::port<input>, 1> in;\n") 
  out_port = (f"  std::array<adf::port<output>, 1> out;\n")

  return (in_port+out_port) # concat strings

def gen_ports_connections(args): 
  in_port  = (f"      adf::connect<>(in[0],widget_real2complex_graph.in[0]);\n") 
  out_port = (f"      adf::connect<>(widget_real2complex_graph.out[0], out[0]);\n")

  return (in_port+out_port) # concat strings

def generate_graph(graphname, args):

  out = {}
  out["port_info"] = info_ports(args)
  ports_code = gen_ports_code(args)
  if graphname == "":
    graphname = "default_graphname"
  TT_DATA = args["TT_DATA"]
  TT_OUT_DATA = args["TT_OUT_DATA"]
  TP_WINDOW_VSIZE = args["TP_WINDOW_VSIZE"]
  code = (
f"""
class {graphname} : public adf::graph {{
public:
  // ports
{ports_code}
  xf::dsp::aie::widget::real2complex::widget_real2complex_graph<
    {TT_DATA}, // TT_DATA
    {TT_OUT_DATA}, // TT_OUT_DATA
    {TP_WINDOW_VSIZE} // TP_WINDOW_VSIZE
  > widget_real2complex_graph;
  {graphname}() : widget_real2complex_graph() {{
    //kernels
    //runtime_ratio
    //connections in loop
{gen_ports_connections(args)}

  }}

}};
"""    
  )
  out["graph"] = code
  out["headerfile"] = "widget_real2complex_graph.hpp"
  out["searchpaths"] = [
       "L2/include/aie",
       "L2/tests/aie/common/inc",
       "L1/include/aie",
       "L1/src/aie",
       "L1/tests/aie/inc",
       "L1/tests/aie/src"
  ]
  return out
