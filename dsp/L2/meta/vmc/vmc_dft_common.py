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
from dft import *
from aie_common import *
import json

#### VMC validators ####
def vmc_validate_TP_RND(args):
  rnd_mode = args["rnd_mode"]
  AIE_VARIANT = args["AIE_VARIANT"]
  return fn_validate_TP_RND(AIE_VARIANT, rnd_mode)
    
def vmc_validate_sat_mode(args):
    sat_mode = args["sat_mode"]
    return fn_validate_satMode(sat_mode)

def vmc_validate_point_size(args):
    point_size = args["point_size"]
    return fn_validate_TP_POINT_SIZE(point_size)

def vmc_validate_num_frames(args):
    AIE_VARIANT = args["AIE_VARIANT"]
    num_frames = args["num_frames"]
    data_type = args["data_type"]
    point_size = args["point_size"]
    return fn_validate_TP_NUM_FRAMES(AIE_VARIANT, data_type, point_size, num_frames)

def vmc_validate_ssr(args):
    AIE_VARIANT = args["AIE_VARIANT"]
    data_type = args["data_type"]
    point_size = args["point_size"]
    ssr = args["ssr"]
    return fn_validate_TP_SSR(AIE_VARIANT, data_type, point_size,ssr)

def vmc_validate_shift_val(args):
    data_type = args["data_type"]
    shift_val = args["shift_val"]
    return fn_validate_TP_SHIFT(data_type, shift_val)

def vmc_validate_casc_length(args):
    AIE_VARIANT = args["AIE_VARIANT"]
    data_type = args["data_type"]
    twiddle_type = args["twiddle_type"]
    point_size = args["point_size"]
    casc_length = args["casc_length"]
    ssr = args["ssr"]
    return fn_validate_TP_CASC_LEN(AIE_VARIANT, data_type, twiddle_type, point_size, ssr, casc_length)

def vmc_validate_twiddle_type(args):
    data_type = args["data_type"]
    twiddle_type = args["twiddle_type"]
    return fn_validate_TT_TWIDDLE(data_type, twiddle_type)

# Get twiddle types
k_twiddle_type = {"cfloat":"cfloat", "cint32":"cint16", "cint16":"cint16"}

def fn_get_twiddle_type(data_type):
	return k_twiddle_type[data_type]
