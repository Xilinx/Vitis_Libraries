from sample_delay import *
from aie_common import *

#### VMC validators ####

def vmc_validate_AIE_VARIANT(args):
  tempargs = {}
  tempargs["AIE_VARIANT"] = args["AIE_VARIANT"]

  return validate_AIE_VARIANT(tempargs)

def vmc_validate_TT_DATA(args):
  tempargs = {}
  tempargs["TT_DATA"] = args["data_type"]

  return validate_TT_DATA(tempargs)

def vmc_validate_TP_MAX_DELAY(args):
  tempargs = {}
  tempargs["AIE_VARIANT"] = args["AIE_VARIANT"]
  tempargs["TP_MAX_DELAY"] = args["max_sample_delay"]
  tempargs["TT_DATA"] = args["data_type"]
  tempargs["TP_API"] = 1

  return validate_TP_MAX_DELAY(tempargs)

def vmc_validate_TP_WINDOW_VSIZE(args):
  tempargs = {}
  tempargs["AIE_VARIANT"] = args["AIE_VARIANT"]
  tempargs["TP_WINDOW_VSIZE"] = args["input_window_size"]
  tempargs["TT_DATA"] = args["data_type"]
  tempargs["TP_MAX_DELAY"] = args["max_sample_delay"]
  tempargs["TP_API"] = 1

  return validate_TP_WINDOW_VSIZE(tempargs)


#### VMC graph generator ####

def vmc_generate_graph(name, args):
    tmpargs = {}
    tmpargs["TT_DATA"] = args["data_type"]
    tmpargs["TP_WINDOW_VSIZE"] = args["input_window_size"]
    tmpargs["TP_API"] = 1
    tmpargs["TP_MAX_DELAY"] = args["max_sample_delay"]

    return generate_graph(name, tmpargs)
