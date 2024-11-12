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
from vmc_dft_common import *
from dft import *
import json

#### VMC graph generator ####
def vmc_generate_graph(name, args):
    tmpargs = {}
    tmpargs["AIE_VARIANT"] = args["AIE_VARIANT"]
    tmpargs["TT_DATA"] = args["data_type"]
    tmpargs["TT_TWIDDLE"] = args["twiddle_type"]
    tmpargs["TP_POINT_SIZE"] = args["point_size"]
    tmpargs["TP_SHIFT"] = args["shift_val"]
    tmpargs["TP_NUM_FRAMES"] = args["num_frames"]
    #TODO: call to partitioner to determine cascade length
    tmpargs["TP_CASC_LEN"] = args["casc_length"]
    tmpargs["TP_DYN_PT_SIZE"] = 0
    tmpargs["TP_API"] = 0
    tmpargs["TP_SSR"] = args["ssr"]
    parallel_power = 0
    tmpargs["TP_PARALLEL_POWER"] = parallel_power
    tmpargs["TP_FFT_NIFFT"] = 1
    tmpargs["TP_RND"] = args["rnd_mode"]
    tmpargs["TP_SAT"] = args["sat_mode"]

    return generate_graph(name, tmpargs)
