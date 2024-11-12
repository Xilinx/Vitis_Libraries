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
from fft_window import *
from aie_common import *
import json

#### VMC validators ####

# Note: For stream-based Window Fn, the low level requires SSR as
# number of kernels and SSR*2 is the number of input or output
# ports. But for VMC users, SSR is the number of input or output
# ports. Hence, we divide it by 2 here before passing on to low level
# IP.

ssr_min = 1
ssr_max = 32

def vmc_fn_get_ssr(args):
  ssr = args["ssr"]
  if ssr % 2 == 0:
    ssr = ssr
  else:
    ssr = -1

  return ssr

def vmc_validate_coeff_type(args):
  data_type = args["data_type"]
  coeff_type = args["coeff_type"]
  return fn_validate_TT_COEFF(data_type, coeff_type)

def vmc_validate_point_size(args):
  point_size = args["point_size"]
  data_type = args["data_type"]
  dyn_pt = 1 if args["is_dyn_pt_size"] else 0
  ssr = 1
  interface_type = 0
  return fn_validate_point_size(point_size, data_type, dyn_pt, interface_type, ssr)

def vmc_validate_shift_val(args):
  data_type = args["data_type"]
  shift_val = args["shift_val"]
  return fn_validate_shift_val(data_type, shift_val)

def vmc_validate_input_window_size(args):
  point_size = args["point_size"]
  input_window_size = args["input_window_size"]
  return fn_validate_window_vsize(point_size, input_window_size)

def vmc_validate_ssr(args):
  interface_type = 1
  ssr = vmc_fn_get_ssr(args)
  if ssr == -1:
    return isError(f"Invalid SSR value specified. The value must be an even number.")
  return fn_validate_ssr(interface_type, ssr)

def fn_validate_ssrvalue(data_type, point_size, interface_type, ssrValue):
  ssr = ssrValue//2
  return fn_validate_ssr(interface_type, ssr)
  
def vmc_validate_is_dyn_pt_size(args):
  dyn_pt = 1 if args["is_dyn_pt_size"] else 0
  tempargs = {}
  tempargs["TP_DYN_PT_SIZE"] = dyn_pt 
  return validate_TP_DYN_PT_SIZE(tempargs)

def vmc_validate_coeff(args):
  dyn_pt = 1 if args["is_dyn_pt_size"] else 0
  return fn_validate_weights(args["point_size"], dyn_pt, args["coeff"])

#### VMC graph generator ####
def vmc_generate_graph(name, args):
  tmpargs = {}
  tmpargs["TT_DATA"] = args["data_type"] 
  tmpargs["TT_COEFF"] = args["coeff_type"]
  tmpargs["TP_POINT_SIZE"] = args["point_size"]
  tmpargs["TP_WINDOW_VSIZE"] = args["input_window_size"]
  tmpargs["TP_SHIFT"] = args["shift_val"]
  tmpargs["TP_API"] = 1
  tmpargs["TP_SSR"] = args["ssr"]
  tmpargs["TP_DYN_PT_SIZE"] = 1 if args["is_dyn_pt_size"] else 0
  tmpargs["AIE_VARIANT"] = args["AIE_VARIANT"]
  tmpargs["weights"] = args["coeff"]
  return generate_graph(name, tmpargs)
