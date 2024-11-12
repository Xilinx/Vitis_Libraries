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
from ctypes import sizeof
from aie_common import *
from aie_common_fir import *

# Utility functions

def fn_get_coeff_size(args):
    coef_type = args["coef_type"]
    coeff = args["coeff"]
    if fn_is_complex(coef_type):
        coeff_size = int(len(coeff)/2)
    else:
        coeff_size = int(len(coeff))
    return coeff_size

def fn_get_fir_length(args):
    use_coeff_reload = args["use_coeff_reload"]
    if use_coeff_reload:
        fir_length = args["fir_length"]
    else:
        fir_length = fn_get_coeff_size(args)
    return fir_length

def fn_get_fir_length_hb(args):
    use_coeff_reload = args["use_coeff_reload"]
    if use_coeff_reload:
        fir_length = args["fir_length"]
    else:
        coeff_size = fn_get_coeff_size(args)
        fir_length = (coeff_size-1)*4-1
    return fir_length

def fn_get_num_outputs(args):
    if args["num_outputs"]:
        num_outputs = 2
    else:
        num_outputs = 1
    return num_outputs
