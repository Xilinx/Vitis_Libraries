import aie_common as com
from aie_common import isError,isValid

def validate_TP_NUM_INPUTS(args):
  TP_NUM_INPUTS = args["TP_NUM_INPUTS"]
  TP_IN_API = args["TP_IN_API"]
  if (TP_IN_API == 0 and TP_NUM_INPUTS > 1):
    return isError(f"Only one input is supported if using an iobuffer. Got TP_NUM_INPUTS {TP_NUM_INPUTS}")
  
  return isValid

def local_sizeof(TT_DATA):
  if TT_DATA == "int16":
    return 2
  elif TT_DATA == "int32":
    return 4
  elif TT_DATA == "float":
    return 4
  elif TT_DATA == "cint16":
    return 4
  elif TT_DATA == "cint32":
    return 8
  elif TT_DATA == "cfloat":
    return 8
  else:
    return -1



  ######### Graph Generator ############

# Used by higher layer software to figure out how to connect blocks together. 
def info_ports(args):
  TT_DATA = args["TT_DATA"]
  TP_WINDOW_VSIZE = args["TP_WINDOW_VSIZE"]
  TP_IN_API = args["TP_IN_API"]
  TP_OUT_API = args["TP_OUT_API"]
  TP_NUM_INPUTS = args["TP_NUM_INPUTS"]
  TP_NUM_OUTPUT_CLONES = args["TP_NUM_OUTPUT_CLONES"]
  TP_HEADER_BYTES = args["TP_HEADER_BYTES"]
  in_ports  = com.get_port_info("in",  "in",  TT_DATA, (TP_WINDOW_VSIZE+TP_HEADER_BYTES/local_sizeof(TT_DATA)), TP_NUM_INPUTS,        0, TP_IN_API) 
  out_ports = com.get_port_info("out", "out", TT_DATA, (TP_WINDOW_VSIZE+TP_HEADER_BYTES/local_sizeof(TT_DATA)), TP_NUM_OUTPUT_CLONES, 0, TP_OUT_API)

  return (in_ports+out_ports) # concat lists

def gen_ports_code(args): 
  TT_DATA = args["TT_DATA"]
  TP_WINDOW_VSIZE = args["TP_WINDOW_VSIZE"]
  TP_IN_API = args["TP_IN_API"]
  TP_OUT_API = args["TP_OUT_API"]
  TP_NUM_INPUTS = args["TP_NUM_INPUTS"]
  TP_NUM_OUTPUT_CLONES = args["TP_NUM_OUTPUT_CLONES"]
  TP_HEADER_BYTES = args["TP_HEADER_BYTES"]
  in_ports  = (f"  std::array<adf::port<input>, {TP_NUM_INPUTS}> in;\n") 
  out_ports = (f"  std::array<adf::port<output>, {TP_NUM_OUTPUT_CLONES}> out;\n")

  return (in_ports+out_ports) # concat strings

def gen_inports_connections(args): 
  TT_DATA = args["TT_DATA"]
  TP_WINDOW_VSIZE = args["TP_WINDOW_VSIZE"]
  TP_IN_API = args["TP_IN_API"]
  TP_OUT_API = args["TP_OUT_API"]
  TP_NUM_INPUTS = args["TP_NUM_INPUTS"]
  TP_NUM_OUTPUT_CLONES = args["TP_NUM_OUTPUT_CLONES"]
  TP_HEADER_BYTES = args["TP_HEADER_BYTES"]
  in_ports  = (f"      adf::connect<>(in[inIdx],widget_api_cast_graph.in[inIdx]);\n") 

  return (in_ports) # concat strings

def gen_outports_connections(args): 
  TT_DATA = args["TT_DATA"]
  TP_WINDOW_VSIZE = args["TP_WINDOW_VSIZE"]
  TP_IN_API = args["TP_IN_API"]
  TP_OUT_API = args["TP_OUT_API"]
  TP_NUM_INPUTS = args["TP_NUM_INPUTS"]
  TP_NUM_OUTPUT_CLONES = args["TP_NUM_OUTPUT_CLONES"]
  TP_HEADER_BYTES = args["TP_HEADER_BYTES"]
  out_ports = (f"      adf::connect<>(widget_api_cast_graph.out[outIdx], out[outIdx]);\n")

  return (out_ports) # concat strings

def generate_graph(graphname, args):

  out = {}
  out["port_info"] = info_ports(args)
  ports_code = gen_ports_code(args)
  if graphname == "":
    graphname = "default_graphname"
  TT_DATA = args["TT_DATA"]
  TP_WINDOW_VSIZE = args["TP_WINDOW_VSIZE"]
  TP_IN_API = args["TP_IN_API"]
  TP_OUT_API = args["TP_OUT_API"]
  TP_NUM_INPUTS = args["TP_NUM_INPUTS"]
  TP_NUM_OUTPUT_CLONES = args["TP_NUM_OUTPUT_CLONES"]
  TP_PATTERN = args["TP_PATTERN"]
  TP_HEADER_BYTES = args["TP_HEADER_BYTES"]
  code = (
f"""
class {graphname} : public adf::graph {{
public:
  // ports
{ports_code}
  xf::dsp::aie::widget::api_cast::widget_api_cast_graph<
    {TT_DATA},              // TT_DATA
    {TP_IN_API},            // TP_IN_API
    {TP_OUT_API},           // TP_OUT_API
    {TP_NUM_INPUTS},        // TP_NUM_INPUTS
    {TP_WINDOW_VSIZE},      // TP_WINDOW_VSIZE
    {TP_NUM_OUTPUT_CLONES}, // TP_NUM_OUTPUT_CLONES
    {TP_PATTERN},           // TP_PATTERN
    {TP_HEADER_BYTES}       // TP_HEADER_BYTES
  > widget_api_cast_graph;
  {graphname}() : widget_api_cast_graph() {{
    //kernels
    //runtime_ratio
    //connections in loop
    for (unsigned inIdx = 0; inIdx < {TP_NUM_INPUTS}; inIdx++){{
{gen_inports_connections(args)}
    }}
    for (unsigned outIdx = 0; outIdx < {TP_NUM_OUTPUT_CLONES}; outIdx++){{
{gen_outports_connections(args)}
    }}

  }}

}};
"""    
  )
  out["graph"] = code
  out["headerfile"] = "dds_mixer_graph.hpp"
  out["searchpaths"] = [
       "L2/include/aie",
       "L2/tests/aie/common/inc",
       "L1/include/aie",
       "L1/src/aie",
       "L1/tests/aie/inc",
       "L1/tests/aie/src"
  ]
  return out
