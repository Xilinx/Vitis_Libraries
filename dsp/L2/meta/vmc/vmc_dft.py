from vmc_dft_common import *
from dft import *
import json

#### VMC graph generator ####
def vmc_generate_graph(name, args):
    tmpargs = {}
    tmpargs["TT_DATA"] = args["data_type"]
    tmpargs["TT_TWIDDLE"] = args["twiddle_type"]
    tmpargs["TP_POINT_SIZE"] = args["point_size"]
    tmpargs["TP_SHIFT"] = args["shift_val"]
    tmpargs["TP_NUM_FRAMES"] = args["num_frames"]
    #TODO: call to partitioner to determine cascade length
    tmpargs["TP_CASC_LEN"] = args["casc_length"]
    tmpargs["TP_DYN_PT_SIZE"] = 0
    tmpargs["TP_API"] = 0
    tmpargs["TP_SSR"] = 1
    parallel_power = 0
    tmpargs["TP_PARALLEL_POWER"] = parallel_power
    tmpargs["TP_FFT_NIFFT"] = 1
    tmpargs["TP_RND"] = args["rnd_mode"]
    tmpargs["TP_SAT"] = args["sat_mode"]

    return generate_graph(name, tmpargs)
