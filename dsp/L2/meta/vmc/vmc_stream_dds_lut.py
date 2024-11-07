from dds_mixer_lut import *

#### VMC validators ####

def vmc_validate_output_window_size(args):
    input_window_size = args["output_window_size"]
    data_type = args["data_type"]
    ssr = args["ssr"]
    AIE_VARIANT = args["AIE_VARIANT"]
    API = 1
    return fn_validate_TP_INPUT_WINDOW_VSIZE(AIE_VARIANT, data_type, ssr, input_window_size, API)

def validate_USE_PHASE_RELOAD(args):
    tmpargs = {}
    tmpargs["TP_SSR"] = args["ssr"]
    tmpargs["TP_USE_PHASE_RELOAD"] = args["USE_PHASE_RELOAD"]
    return validate_TP_USE_PHASE_RELOAD(tmpargs);

#### VMC graph generator ####
def vmc_generate_graph(name, args):
    tmpargs = {}
    tmpargs["TT_DATA"] = args["data_type"]
    tmpargs["TP_INPUT_WINDOW_VSIZE"] = args["output_window_size"]
    tmpargs["TP_MIXER_MODE"] = 0
    tmpargs["TP_API"] = 1
    tmpargs["TP_SSR"] = args["ssr"]
    tmpargs["TP_RND"] = args["rnd_mode"]
    tmpargs["TP_SAT"] = args["sat_mode"]
    tmpargs["TP_SFDR"] = args["sfdr"]
    tmpargs["TP_USE_PHASE_RELOAD"] = 1 if args["USE_PHASE_RELOAD"] else 0
    tmpargs["AIE_VARIANT"] = args["AIE_VARIANT"]
    tmpargs["phaseInc"] = args["phase_increment"]
    tmpargs["initialPhaseOffset"] = args["initial_phase_offset"]
    return generate_graph(name, tmpargs)
