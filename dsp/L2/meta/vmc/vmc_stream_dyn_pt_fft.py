#
# Copyright (C) 2019-2022, Xilinx, Inc.
# Copyright (C) 2022-2024, Advanced Micro Devices, Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
from fft_ifft_dit_1ch import *
from vmc_fft_common import *
import json

#### VMC validators ####
def vmc_validate_point_size(args):
    ssr = args["ssr"]
    api = 1 #stream.
    variant = args["AIE_VARIANT"]
    pp = fn_get_parallel_power(ssr, api, variant)

    if pp == -1:
      return isError(f"Invalid SSR value specified. The value should be of the form 2^N between 2 and 512.")

    point_size = args["point_size"]
    dyn_point_size = 1
    data_type = args["data_type"]
    api = 1
    AIE_VARIANT = args["AIE_VARIANT"]
    return fn_validate_TP_POINT_SIZE(AIE_VARIANT, data_type, api, pp, dyn_point_size, point_size)

def vmc_validate_shift_val(args):
    data_type = args["data_type"]
    shift_val = args["shift_val"]
    return fn_validate_shift_val(data_type, shift_val)

def vmc_validate_input_window_size(args):
    point_size = args["point_size"]
    input_window_size = args["input_window_size"]
    dyn_point_size = 1;
    return fn_validate_window_size(point_size, input_window_size, dyn_point_size)

def vmc_validate_casc_length(args):
    data_type = args["data_type"]
    point_size = args["point_size"]
    casc_length = args["casc_length"]
    ssr = args["ssr"]
    api = 1
    variant = args["AIE_VARIANT"]
    pp = fn_get_parallel_power(ssr, api, variant)

    if pp == -1:
      return isError(f"Invalid SSR value specified. The value should be of the form 2^N between 2 and 512.")

    return fn_validate_casc_len(data_type, point_size, pp, casc_length)

# Get twiddle types
k_twiddle_type = {"cfloat":"cfloat", "cint32":"cint16", "cint16":"cint16"}

def fn_get_twiddle_type(data_type):
    return k_twiddle_type[data_type]

#### VMC graph generator ####
def vmc_generate_graph(name, args):
    tmpargs = {}
    tmpargs["AIE_VARIANT"] = args["AIE_VARIANT"]
    tmpargs["TT_DATA"] = args["data_type"]
    tmpargs["TT_OUT_DATA"] = args["data_out_type"]
    tmpargs["TT_TWIDDLE"] = args["twiddle_type"]
    tmpargs["TP_POINT_SIZE"] = args["point_size"]
    tmpargs["TP_SHIFT"] = args["shift_val"]
    tmpargs["TP_WINDOW_VSIZE"] = args["input_window_size"]
    #TODO: call to partitioner to determine cascade length
    tmpargs["TP_CASC_LEN"] = args["casc_length"]
    tmpargs["TP_DYN_PT_SIZE"] = 1
    tmpargs["TP_API"] = 1
    ssr = args["ssr"]
    api = 1
    variant = args["AIE_VARIANT"]
    pp = fn_get_parallel_power(ssr, api, variant)

    if pp == -1:
      return isError(f"Invalid SSR value specified. The value should be of the form 2^N between 2 and 512.")

    tmpargs["TP_PARALLEL_POWER"] = pp
    tmpargs["TP_FFT_NIFFT"] = 1
    tmpargs["TP_RND"] = args["rnd_mode"]
    tmpargs["TP_SAT"] = args["sat_mode"]
    tmpargs["AIE_VARIANT"] = args["AIE_VARIANT"]
    tmpargs["TP_USE_WIDGETS"] = 1 if args["use_ssr_widget_kernels"] else 0
    # tmpargs["TP_TWIDDLE_MODE"] = args["TP_TWIDDLE_MODE"]
    tmpargs["TP_TWIDDLE_MODE"] = 0
    return generate_graph(name, tmpargs)
