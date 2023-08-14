import aie_common as com

def info_ports(args):
  TT_DATA = args["TT_DATA"]
  TP_WINDOW_VSIZE = args["TP_WINDOW_VSIZE"]
  TP_API = args["TP_API"]
  TP_MAX_DELAY = args["TP_MAX_DELAY"]

  in_1 = com.get_port_info("in", "in", TT_DATA, TP_WINDOW_VSIZE,TP_MAX_DELAY, TP_API)
  in_2 = com.get_parameter_port_info("numSampleDelay", "in", "uint", None,0,"async")
  out_1 = com.get_port_info("in", "in", TT_DATA, TP_WINDOW_VSIZE,TP_MAX_DELAY, TP_API)
  
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
  adf::port<input> inport; 
  adf::port<input> numSampleDelay;
  adf::port<output> outport;
  xf::dsp::aie::sample_delay::sample_delay_graph<
  {TT_DATA}, 
  {TP_WINDOW_VSIZE},
  {TP_API},
  {TP_MAX_DELAY}
  > sample_delay_graph;

  {graphname}() : sample_delay_graph(){{
  adf::connect<>(in, m_kernel.in[0]);
  adf::connect<>(m_kernel.out[0], out);
  adf::dimensions(m_kernel.in[0]) = {TP_WINDOW_VSIZE};
  adf::dimensions(m_kernel.out[0]) = {TP_WINDOW_VSIZE};
  adf::connect<parameter>(numSampleDelay, async(m_kernel.in[1])); // RTP 
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