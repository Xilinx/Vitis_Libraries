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
from dds_mixer import *

#### VMC validators ####


def vmc_validate_output_window_size(args):
    tempargs = {}
    tempargs["TP_INPUT_WINDOW_VSIZE"] = args["output_window_size"]
    tempargs["TT_DATA"] = args["data_type"]
    tempargs["TP_SSR"] =  1
    tempargs["TP_API"] = 0

    return validate_TP_INPUT_WINDOW_VSIZE(tempargs)

def vmc_validate_data_type(args):
    data_type = args["data_type"]
    if data_type == "cint32":
      return isError(f"cint32 data_type is not supported for DDS block")
    return fn_validate_tt_data(data_type)

def vmc_validate_sat_mode(args):
    tmpargs = {}
    tmpargs["TP_SAT"] = args["sat_mode"]
    tmpargs["TP_MIXER_MODE"] = 0
    return validate_TP_SAT(tmpargs);

def vmc_validate_ssr(args):
    ssr = args["ssr"]
    return fn_validate_ssr(ssr)

def validate_AIE_VARIANT(args):
    AIE_VARIANT = args["AIE_VARIANT"]
    return fn_validate_aieVarVMC(AIE_VARIANT)

def validate_USE_PHASE_RELOAD(args):
    tmpargs = {}
    tmpargs["TP_SSR"] = args["ssr"]
    tmpargs["TP_USE_PHASE_RELOAD"] = args["USE_PHASE_RELOAD"]
    return validate_TP_USE_PHASE_RELOAD(tmpargs)

#### VMC graph generator ####
def vmc_generate_graph(name, args):
    tmpargs = {}
    tmpargs["TT_DATA"] = args["data_type"]
    tmpargs["TP_MIXER_MODE"] = 0
    tmpargs["TP_INPUT_WINDOW_VSIZE"] = args["output_window_size"]
    tmpargs["TP_NUM_OUTPUTS"] = 1
    tmpargs["TP_SSR"] = args["ssr"]
    tmpargs["TP_API"] = 0
    tmpargs["phaseInc"] = args["phase_increment"]
    tmpargs["initialPhaseOffset"] = 0
    tmpargs["TP_USE_PHASE_RELOAD"] = 1 if args["USE_PHASE_RELOAD"] else 0
    tmpargs["TP_RND"] = args["rnd_mode"]
    tmpargs["TP_SAT"] = args["sat_mode"]

    return generate_graph(name, tmpargs)
