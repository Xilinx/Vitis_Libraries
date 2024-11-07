from dds_mixer_lut import *

def vmc_validate_mixer_mode(args):
    mixer_mode = args["mixer_mode"]
    return fn_validate_TP_MIXER_MODE(mixer_mode)

#### VMC graph generator ####
def vmc_generate_graph(name, args):
    tmpargs = {}
    tmpargs["TT_DATA"] = args["data_type"]
    tmpargs["TP_INPUT_WINDOW_VSIZE"] = args["input_window_size"]
    tmpargs["TP_MIXER_MODE"] = args["mixer_mode"]
    tmpargs["TP_API"] = 0
    tmpargs["TP_SSR"] = args["ssr"]
    tmpargs["TP_RND"] = args["rnd_mode"]
    tmpargs["TP_SAT"] = args["sat_mode"]
    tmpargs["TP_SFDR"] = args["sfdr"]
    tmpargs["TP_USE_PHASE_RELOAD"] = 1 if args["USE_PHASE_RELOAD"] else 0
    tmpargs["AIE_VARIANT"] = args["AIE_VARIANT"]
    tmpargs["phaseInc"] = args["phase_increment"]
    tmpargs["initialPhaseOffset"] = args["initial_phase_offset"]
    

    return generate_graph(name, tmpargs)
