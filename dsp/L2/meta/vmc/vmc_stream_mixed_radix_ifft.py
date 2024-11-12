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
from mixed_radix_fft import *
import json

#### VMC validators ####
def vmc_validate_point_size(args):
    aie_variant = args["AIE_VARIANT"]
    data_type = args["data_type"]
    point_size = args["point_size"]
    dyn_pt_size = args["dyn_pt_size"]
    return fn_validate_point_size(aie_variant, data_type, point_size, dyn_pt_size)

def vmc_validate_shift_val(args):
    data_type = args["data_type"]
    shift_val = args["shift_val"]
    return fn_validate_shift(data_type, shift_val)

def vmc_validate_input_window_size(args):
    aie_variant = args["AIE_VARIANT"]
    data_type = args["data_type"]
    point_size = args["point_size"]
    input_window_size = args["input_window_size"]
    return fn_validate_window_vsize(aie_variant, data_type, point_size, input_window_size)

def vmc_validate_dyn_pt_size(args):
    dyn_pt_size = args["dyn_pt_size"]
    return fn_validate_dyn_pt_size(dyn_pt_size)

def vmc_validate_casc_length(args):
    point_size = args["point_size"]
    casc_length = args["casc_length"]
    dyn_pt_size = args["dyn_pt_size"]
    return fn_validate_casc_len(casc_length, point_size, dyn_pt_size)

def vmc_validate_rnd_mode(args):
  rnd_mode = args["rnd_mode"]
  aie_variant = args["AIE_VARIANT"]
  return fn_validate_rnd(aie_variant, rnd_mode)

def vmc_validate_sat_mode(args):
    sat_mode = args["sat_mode"]
    return fn_validate_sat(sat_mode)

def vmc_validate_twiddle_type(args):
    data_type = args["data_type"]
    twiddle_type = args["twiddle_type"]
    return fn_validate_twiddle_type(twiddle_type, data_type)


#### VMC graph generator ####
def vmc_generate_graph(name, args):
    tmpargs = {}
    tmpargs["TT_DATA"] = args["data_type"]
    tmpargs["TT_TWIDDLE"] = args["twiddle_type"]
    tmpargs["TP_POINT_SIZE"] = args["point_size"]
    tmpargs["TP_SHIFT"] = args["shift_val"]
    tmpargs["TP_WINDOW_VSIZE"] = args["input_window_size"]
    tmpargs["TP_DYN_PT_SIZE"] = args["dyn_pt_size"]
    tmpargs["TP_CASC_LEN"] = args["casc_length"]
    tmpargs["TP_API"] = 1
    tmpargs["TP_FFT_NIFFT"] = 0
    tmpargs["TP_RND"] = args["rnd_mode"]
    tmpargs["TP_SAT"] = args["sat_mode"]
    tmpargs["AIE_VARIANT"] = args["AIE_VARIANT"]

    return generate_graph(name, tmpargs)
