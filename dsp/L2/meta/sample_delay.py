import aie_common as com

def validate_TP_MAX_DELAY(args):
  TT_DATA = args["TT_DATA"]
  TP_MAX_DELAY = args["TP_MAX_DELAY"]
  TP_API = args["TP_API"]
  VEC_SIZE = 256/8/com.fn_size_by_byte(TT_DATA)

  if (TP_API): # stream interface
    if (TP_MAX_DELAY % 128):
      return com.isError(f"Min value of TP_MAX_DELAY for TP_API = {TP_API} is 128. The legal values of TP_MAX_DELAY are 2^n where n = 7, 8, 9 ... Got {TP_MAX_DELAY}")
    return com.isValid
  else: # iobuff (window) interface
    if (TP_MAX_DELAY % VEC_SIZE) :
      return com.isError(f"TP_MAX_DELAY for TP_API = {TP_API} should be an integer multiple of vector size. Vector size for the data type {TT_DATA} is {VEC_SIZE}. Got {TP_MAX_DELAY}")
  return com.isValid

def validate_TP_WINDOW_VSIZE(args):
  TT_DATA = args["TT_DATA"]
  TP_MAX_DELAY = args["TP_MAX_DELAY"]
  TP_WINDOW_VSIZE = args["TP_WINDOW_VSIZE"]
  TP_API = args["TP_API"]
  VEC_SIZE = 256/8/com.fn_size_by_byte(TT_DATA)

  if (TP_API): # stream interface
    if (TP_WINDOW_VSIZE % TP_MAX_DELAY):
      return com.isError(f"TP_WINDOW_VSIZE for TP_API = {TP_API} should be an integer multiple of TP_MAX_DELAY. Got {TP_WINDOW_VSIZE}")
    return com.isValid
  else: # iobuff (window) interface
    if (TP_WINDOW_VSIZE % VEC_SIZE):
      return com.isError(f"TP_WINDOW_VSIZE for TP_API = {TP_API} should be an integer multiple of vector size. Vector size for the data type {TT_DATA} is {VEC_SIZE}. Got {TP_WINDOW_VSIZE}")
  return com.isValid

def info_ports(args):
  TT_DATA = args["TT_DATA"]
  TP_WINDOW_VSIZE = args["TP_WINDOW_VSIZE"]
  TP_API = args["TP_API"]
  TP_MAX_DELAY = args["TP_MAX_DELAY"]

  in_1 = com.get_port_info("in", "in", TT_DATA, TP_WINDOW_VSIZE, None, 0, TP_API)
  in_2 = com.get_parameter_port_info("numSampleDelay", "in", "uint32", None, 1, "async")
  out_1 = com.get_port_info("out", "out", TT_DATA, TP_WINDOW_VSIZE, None, 0, TP_API)  
  return (in_1+in_2+out_1)

def generate_graph(graphname, args):
  
  if graphname =="":
    graphname = "default_graphname"
  TT_DATA = args["TT_DATA"]
  TP_WINDOW_VSIZE = args["TP_WINDOW_VSIZE"]
  TP_API = args["TP_API"]
  TP_MAX_DELAY = args["TP_MAX_DELAY"]
  code  = (
f"""
class {graphname} : public adf::graph {{
public:
  adf::port<input> in; 
  adf::port<input> numSampleDelay;
  adf::port<output> out;
  xf::dsp::aie::sample_delay::sample_delay_graph<
  {TT_DATA}, 
  {TP_WINDOW_VSIZE},
  {TP_API},
  {TP_MAX_DELAY}
  > sample_delay_graph;

  {graphname}() : sample_delay_graph(){{
  adf::connect<>(in, sample_delay_graph.in);
  adf::connect<>(sample_delay_graph.out, out);
  adf::connect<>(numSampleDelay, sample_delay_graph.numSampleDelay); // RTP 
  }}

}};
"""
)
  out = {}
  out["graph"] = code
  out["port_info"] = info_ports(args)
  out["headerfile"] = "sample_delay_graph.hpp"
  out["searchpaths"] = [
       "L2/include/aie",
       "L2/tests/aie/common/inc",
       "L1/include/aie",
       "L1/src/aie",
       "L1/tests/aie/inc",
       "L1/tests/aie/src"
  ]

  return out

print("finished")
