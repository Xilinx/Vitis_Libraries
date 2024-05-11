import aie_common as com
from aie_common import isError,isValid, fn_validate_satMode, fn_validate_min_value, fn_validate_max_value, fn_validate_minmax_value
#dds_mixer.hpp:74:    static_assert(TP_MIXER_MODE <= 2, "ERROR: DDS Mixer Mode must be 0, 1 or 2. ");
#dds_mixer.hpp:75:    static_assert(fnEnumType<TT_DATA>() != enumUnknownType,
#dds_mixer.hpp:77:    static_assert((TP_INPUT_WINDOW_VSIZE % m_kNumLanes) == 0,
#dds_mixer.hpp:79:    static_assert(fnEnumType<TT_DATA>() != enumCint32 || TP_MIXER_MODE != MIXER_MODE_0,
#dds_mixer.hpp:129:    static_assert(TP_MIXER_MODE <= 2, "ERROR: DDS Mixer Mode must be 0, 1 or 2. ");
#dds_mixer.hpp:130:    static_assert(fnEnumType<TT_DATA>() != enumUnknownType,
#dds_mixer.hpp:132:    static_assert((TP_INPUT_WINDOW_VSIZE % m_kNumLanes) == 0,
#graph:78:    static_assert(TP_SSR > 0, "ERROR: Invalid SSR value, must be a value greater than 0.\n");

TP_SSR_min = 1
TP_INPUT_WINDOW_VSIZE_min = 4

def fn_get_dds_lanes(TT_DATA):
  type_lane_dict = {
    "cint16" : 8,
    "cint32" : 4,
    "cfloat" : 4
  }
  return type_lane_dict[TT_DATA]

def fn_validate_tt_data(TT_DATA,TP_MIXER_MODE):
    if (TP_MIXER_MODE == 0) and (TT_DATA == "cint32" ):
        return isError(f"MIXER_MODE_{TP_MIXER_MODE} does not support requested data type TT_DATA {TT_DATA} ")
    return isValid

def fn_validate_ssr(TP_SSR):
    return fn_validate_min_value("TP_SSR", TP_SSR, TP_SSR_min)

def fn_validate_aieVar(AIE_VARIANT):
    if AIE_VARIANT == 1:
      return isValid
    return isError(f"Please use the dds_mixer_lut library element for this device. This device does not support the dds_mixer element.")

def fn_validate_aieVarVMC(AIE_VARIANT):
    if AIE_VARIANT == 1:
      return isValid
    return isError(f"This library block is not supported on AIE-ML devices.")

def validate_TT_DATA(args):
    TT_DATA = args["TT_DATA"]
    TP_MIXER_MODE = args["TP_MIXER_MODE"]
    return fn_validate_tt_data(TT_DATA,TP_MIXER_MODE)

def validate_TP_WINDOW_VSIZE(args):
  TP_INPUT_WINDOW_VSIZE= args["TP_INPUT_WINDOW_VSIZE"]
  res = fn_validate_min_value("TP_INPUT_WINDOW_VSIZE", TP_INPUT_WINDOW_VSIZE, TP_INPUT_WINDOW_VSIZE_min)
  if (res["is_valid"] == False):
    return res
  TT_DATA= args["TT_DATA"]
  lanes = fn_get_dds_lanes(TT_DATA)
  if (TP_INPUT_WINDOW_VSIZE % lanes != 0):
      return isError(f"Window size ({TP_INPUT_WINDOW_VSIZE}) must be a multiple of number of lanes ({lanes})")

  return isValid

def validate_TP_SSR(args):
    TP_SSR = args["TP_SSR"]
    return fn_validate_ssr(TP_SSR)

def validate_TP_SAT(args):
  TP_SAT = args["TP_SAT"]
  return fn_validate_satMode(TP_SAT)

def validate_AIE_VARIANT(args):
  AIE_VARIANT = args["AIE_VARIANT"]
  return fn_validate_aieVar(AIE_VARIANT)

def validate_TP_USE_PHASE_RELOAD(args):
  TP_USE_PHASE_RELOAD = args["TP_USE_PHASE_RELOAD"]
  TP_SSR = args["TP_SSR"]
  if (TP_USE_PHASE_RELOAD == 1 and TP_SSR !=1):
    return isError("Phase Offset Update cannot be used for TP_SSR > 1!")
  return isValid

  ######### Graph Generator ############

# Used by higher layer software to figure out how to connect blocks together.
def info_ports(args):
  TT_DATA = args["TT_DATA"]
  TP_INPUT_WINDOW_VSIZE = args["TP_INPUT_WINDOW_VSIZE"]
  TP_SSR = args["TP_SSR"]
  TP_API = args["TP_API"]
  TP_MIXER_MODE = args["TP_MIXER_MODE"]
  TP_USE_PHASE_RELOAD = args["TP_USE_PHASE_RELOAD"]
  in1_ports = (com.get_port_info("in1", "in", TT_DATA, (TP_INPUT_WINDOW_VSIZE/TP_SSR), TP_SSR, 0, TP_API) if (TP_MIXER_MODE in [1,2]) else [])
  in2_ports = (com.get_port_info("in2", "in", TT_DATA, (TP_INPUT_WINDOW_VSIZE/TP_SSR), TP_SSR, 0, TP_API) if (TP_MIXER_MODE == 2) else [])
  in3_ports = (com.get_parameter_port_info("PhaseRTP", "in", "int32", None, TP_SSR, "async") if (TP_USE_PHASE_RELOAD == 1) else [])
  out_ports = com.get_port_info("out", "out", TT_DATA, (TP_INPUT_WINDOW_VSIZE/TP_SSR), TP_SSR, 0, TP_API)

  return (in1_ports+in2_ports+in3_ports+out_ports) # concat strings

def gen_ports_code(args):
  TP_SSR = args["TP_SSR"]
  TP_MIXER_MODE = args["TP_MIXER_MODE"]
  TP_USE_PHASE_RELOAD = args["TP_USE_PHASE_RELOAD"]
  in1_ports = ((f"  std::array<adf::port<input>, {TP_SSR}> in1;\n") if (TP_MIXER_MODE in [1,2]) else "")
  in2_ports = ((f"  std::array<adf::port<input>, {TP_SSR}> in2;\n") if (TP_MIXER_MODE == 2) else "")
  in3_ports = ((f"  std::array<adf::port<input>, {TP_SSR}> PhaseRTP;\n") if (TP_USE_PHASE_RELOAD == 1) else "")
  out_ports = (f"  std::array<adf::port<output>, {TP_SSR}> out;\n")

  return (in1_ports+in2_ports+in3_ports+out_ports) # concat strings

def gen_ports_connections(args):
  TP_SSR = args["TP_SSR"]
  TP_USE_PHASE_RELOAD = args["TP_USE_PHASE_RELOAD"]
  TP_MIXER_MODE = args["TP_MIXER_MODE"]
  in1_ports = ((f"      adf::connect<>(in1[ssrIdx],mixer_graph.in1[ssrIdx]);\n") if (TP_MIXER_MODE in [1,2]) else "")
  in2_ports = ((f"      adf::connect<>(in2[ssrIdx],mixer_graph.in2[ssrIdx]);\n") if (TP_MIXER_MODE == 2) else "")
  in3_ports = ((f"      adf::connect<>(PhaseRTP[ssrIdx],mixer_graph.PhaseRTP[ssrIdx]);\n") if (TP_USE_PHASE_RELOAD == 1) else "")
  out_ports = (f"      adf::connect<>(mixer_graph.out[ssrIdx], out[ssrIdx]);\n")

  return (in1_ports+in2_ports+in3_ports+out_ports) # concat strings

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
  TP_RND = args["TP_RND"]
  TP_SAT = args["TP_SAT"]
  TP_USE_PHASE_RELOAD = args["TP_USE_PHASE_RELOAD"]
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
    {TP_SSR}, // TP_SSR
    {TP_RND}, //TP_RND
    {TP_SAT}, //TP_SAT
    {TP_USE_PHASE_RELOAD} // TP_USE_PHASE_RELOAD

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
