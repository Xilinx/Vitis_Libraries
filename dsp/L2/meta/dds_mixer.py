import aie_common as com
from aie_common import isError,isValid
#dds_mixer.hpp:74:    static_assert(TP_MIXER_MODE <= 2, "ERROR: DDS Mixer Mode must be 0, 1 or 2. ");
#dds_mixer.hpp:75:    static_assert(fnEnumType<TT_DATA>() != enumUnknownType,
#dds_mixer.hpp:77:    static_assert((TP_INPUT_WINDOW_VSIZE % m_kNumLanes) == 0,
#dds_mixer.hpp:79:    static_assert(fnEnumType<TT_DATA>() != enumCint32 || TP_MIXER_MODE != MIXER_MODE_0,
#dds_mixer.hpp:129:    static_assert(TP_MIXER_MODE <= 2, "ERROR: DDS Mixer Mode must be 0, 1 or 2. ");
#dds_mixer.hpp:130:    static_assert(fnEnumType<TT_DATA>() != enumUnknownType,
#dds_mixer.hpp:132:    static_assert((TP_INPUT_WINDOW_VSIZE % m_kNumLanes) == 0,
#graph:78:    static_assert(TP_SSR > 0, "ERROR: Invalid SSR value, must be a value greater than 0.\n");

def fn_get_dds_lanes(TT_DATA):
  type_lane_dict = {
    "cint16" : 8,
    "cint32" : 4,
    "cfloat" : 4
  }
  return type_lane_dict[TT_DATA]

def validate_TP_WINDOW_VSIZE(args):
  TP_INPUT_WINDOW_VSIZE= args["TP_INPUT_WINDOW_VSIZE"]
  TT_DATA= args["TT_DATA"]
  lanes = fn_get_dds_lanes(TT_DATA)
  if (TP_INPUT_WINDOW_VSIZE % lanes != 0):
    return isError(f"Window size ({TP_INPUT_WINDOW_VSIZE}) must be a multiple of number of lanes ({lanes})")
  
  return isValid


  ######### Graph Generator ############

# Used by higher layer software to figure out how to connect blocks together. 
def info_ports(args):
  TT_DATA = args["TT_DATA"]
  TP_INPUT_WINDOW_VSIZE = args["TP_INPUT_WINDOW_VSIZE"]
  TP_SSR = args["TP_SSR"]
  TP_API = args["TP_API"]
  TP_MIXER_MODE = args["TP_MIXER_MODE"]
  in1_ports = (com.get_port_info("in1", "in", TT_DATA, (TP_INPUT_WINDOW_VSIZE/TP_SSR), TP_SSR, 0, TP_API) if (TP_MIXER_MODE in [1,2]) else [])
  in2_ports = (com.get_port_info("in2", "in", TT_DATA, (TP_INPUT_WINDOW_VSIZE/TP_SSR), TP_SSR, 0, TP_API) if (TP_MIXER_MODE == 2) else [])
  out_ports = com.get_port_info("out", "out", TT_DATA, (TP_INPUT_WINDOW_VSIZE/TP_SSR), TP_SSR, 0, TP_API)

  return (in1_ports+in2_ports+out_ports) # concat lists

def gen_ports_code(args): 
  TP_SSR = args["TP_SSR"]
  TP_MIXER_MODE = args["TP_MIXER_MODE"]
  in1_ports = ((f"  std::array<adf::port<input>, {TP_SSR}> in1;\n") if (TP_MIXER_MODE in [1,2]) else "")
  in2_ports = ((f"  std::array<adf::port<input>, {TP_SSR}> in2;\n") if (TP_MIXER_MODE == 2) else "")
  out_ports = (f"  std::array<adf::port<output>, {TP_SSR}> out;\n")

  return (in1_ports+in2_ports+out_ports) # concat strings

def gen_ports_connections(args): 
  TP_SSR = args["TP_SSR"]
  TP_MIXER_MODE = args["TP_MIXER_MODE"]
  in1_ports = ((f"      adf::connect<>(in1[ssrIdx],mixer_graph.in1[ssrIdx]);\n") if (TP_MIXER_MODE in [1,2]) else "")
  in2_ports = ((f"      adf::connect<>(in2[ssrIdx],mixer_graph.in2[ssrIdx]);\n") if (TP_MIXER_MODE == 2) else "")
  out_ports = (f"      adf::connect<>(mixer_graph.out[ssrIdx], out[ssrIdx]);\n")

  return (in1_ports+in2_ports+out_ports) # concat strings

def generate_graph(graphname, args):

  out = {}
  out["port_info"] = info_ports(args)
  ports_code = gen_ports_code(args)
  if graphname == "":
    graphname = "default_graphname"
  TT_DATA = args["TT_DATA"]
  TP_INPUT_WINDOW_VSIZE = args["TP_INPUT_WINDOW_VSIZE"]
  TP_MIXER_MODE = args["TP_MIXER_MODE"]
  TP_API = args["TP_API"]
  TP_SSR = args["TP_SSR"]
  code = (
f"""
class {graphname} : public adf::graph {{
public:
  // ports
{ports_code}
  xf::dsp::aie::mixer::dds_mixer::dds_mixer_graph<
    {TT_DATA}, // TT_DATA
    {TP_INPUT_WINDOW_VSIZE}, // TP_INPUT_WINDOW_VSIZE
    {TP_MIXER_MODE}, // TP_MIXER_MODE
    {TP_API}, // TP_API
    {TP_SSR} // TP_SSR
  > mixer_graph;
  {graphname}() : mixer_graph({args["phaseInc"]}, {args["initialPhaseOffset"]}) {{
    //kernels
    //runtime_ratio
    //connections in loop
    for (unsigned ssrIdx = 0; ssrIdx < {TP_SSR}; ssrIdx++){{
{gen_ports_connections(args)}
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
