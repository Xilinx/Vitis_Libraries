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
from dds_mixer_lut import *

#### VMC validators ####

def validate_AIE_VARIANT(args):
    tmpargs = {}
    tmpargs["AIE_VARIANT"] = args["AIE_VARIANT"]
    return validate_AIE_VARIANT(tmpargs)

def vmc_validate_input_window_size(args):
    input_window_size = args["input_window_size"]
    data_type = args["data_type"]
    ssr = args["ssr"]
    AIE_VARIANT = args["AIE_VARIANT"]
    API = 0
    return fn_validate_TP_INPUT_WINDOW_VSIZE(AIE_VARIANT, data_type, ssr, input_window_size, API)

def vmc_validate_data_type(args):
    AIE_VARIANT = args["AIE_VARIANT"]
    data_type = args["data_type"]
    return fn_validate_TT_DATA(data_type, AIE_VARIANT)

def vmc_validate_SFDR(args):
    sfdr = args["sfdr"]
    data_type = args["data_type"]
    return fn_validate_TP_SFDR(sfdr,data_type)

def validate_USE_PHASE_RELOAD(args):
    tmpargs = {}
    tmpargs["TP_SSR"] = args["ssr"]
    tmpargs["TP_USE_PHASE_RELOAD"] = args["USE_PHASE_RELOAD"]
    return validate_TP_USE_PHASE_RELOAD(tmpargs)

def vmc_validate_phase_increment(args):
    phase_increment = args["phase_increment"]
    return fn_validate_phaseInc(phase_increment)

def vmc_validate_initial_phase_offset(args):
    initial_phase_offset = args["initial_phase_offset"]
    return fn_validate_initialPhaseOffset(initial_phase_offset)

def vmc_validate_rnd_mode(args):
    tmpargs = {}
    tmpargs["TP_RND"] = args["rnd_mode"]
    tmpargs["AIE_VARIANT"] = args["AIE_VARIANT"]
    return validate_TP_RND(tmpargs)

def vmc_validate_sat_mode(args):
    tmpargs = {}
    tmpargs["TP_SAT"] = args["sat_mode"]
    return validate_TP_SAT(tmpargs)

def vmc_validate_mixer_mode(args):
    mixer_mode = args["mixer_mode"]
    AIE_VARIANT = args["AIE_VARIANT"]
    if (AIE_VARIANT == 2 and mixer_mode == 2):
      return isError(f"mixer_mode(2) in stream interfaces is not supported for AIE-ML devices")
    return fn_validate_TP_MIXER_MODE(mixer_mode)

def vmc_validate_ssr(args):
    ssr = args["ssr"]
    return fn_validate_ssr(ssr)
