from vmc_fft_common import *

#### VMC graph generator ####
def vmc_generate_graph(name, args):
    tmpargs = {}
    tmpargs["TT_DATA"] = args["data_type"]
    tmpargs["TT_TWIDDLE"] = fn_get_twiddle_type(args["data_type"])
    tmpargs["TP_POINT_SIZE"] = args["point_size"]
    tmpargs["TP_SHIFT"] = args["shift_val"]
    tmpargs["TP_WINDOW_VSIZE"] = args["input_window_size"]
    #TODO: call to partitioner to determine cascade length
    tmpargs["TP_CASC_LEN"] = args["casc_length"]
    tmpargs["TP_DYN_PT_SIZE"] = 0
    tmpargs["TP_API"] = 1
    ssr = args["ssr"]
    pp = fn_get_parallel_power(ssr)

    if pp == -1:
      return isError(f"Invalid SSR value specified. The value should be of the form 2^N between 2 and 512.")

    tmpargs["TP_PARALLEL_POWER"] = pp
    tmpargs["TP_FFT_NIFFT"] = 1

    return generate_graph(name, tmpargs)
