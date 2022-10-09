
#### VMC graph generator ####
def vmc_generate_graph(name, args):
    tmpargs = {}
    tmpargs["TT_DATA"] = args["data_type"]
    tmpargs["TT_TWIDDLE"] = fn_get_twiddle_type(args["data_type"])
    tmpargs["TP_POINT_SIZE"] = args["point_size"]
    tmpargs["TP_SHIFT"] = args["shift_val"]
    tmpargs["TP_WINDOW_VSIZE"] = args["input_window_size"]
    #TODO: call to partitioner to determine cascade length
    tmpargs["TP_CASC_LEN"] = 1
    tmpargs["TP_DYN_PT_SIZE"] = 0
    tmpargs["TP_API"] = 1
    ssr = args["ssr"]
    parallel_power = int(math.log2(ssr))
    tmpargs["TP_PARALLEL_POWER"] = parallel_power
    tmpargs["TP_FFT_NIFFT"] = 0

    return generate_graph(name, tmpargs)
