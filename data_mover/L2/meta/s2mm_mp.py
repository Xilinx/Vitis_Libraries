# Copyright (C) 2023, Advanced Micro Devices, Inc.
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

def info_ports(args):
    """Standard function creating a static dictionary of information
    for upper software to correctly connect the IP.
    Some IP has dynamic number of ports according to parameter set,
    so port information has to be implemented as a function"""

    NUM_PORTS = args['NUM_PORTS']
    S_WIDTH = args['S_WIDTH']

    ports = []
    for i in range(0, NUM_PORTS):
        ports.append({"name": f"s{i}",
                      "direction": "out",
                      "type": "hls::stream<ap_axiu<{S_WIDTH},0,0,0>>&"
                     })
        ports.append({"name": f"mm{i}",
                      "direction": "in",
                      "type": "ap_uint<{S_WIDTH}>*"
                     })
        ports.append({"name": f"nbytes{i}",
                      "direction": "in",
                      "type": "uint64_t"
                     })
    return ports

def generate_cu(cu_name, args):

    if not cu_name or cu_name == '':
        cu_name = 's2mm_mp'

    NUM_PORTS = args['NUM_PORTS']
    S_WIDTH = args['S_WIDTH']

    code = f"""
/*
 * Copyright (C) 2023, Advanced Micro Devices, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "xf_data_mover/s2mm.hpp"

extern "C" void {cu_name}("""

    for i in range(0, NUM_PORTS):
        if i != 0:
            code += ','
        code += f"""
    hls::stream<ap_axiu<{S_WIDTH}, 0, 0, 0> >& s{i},
    ap_uint<{S_WIDTH}>* mm{i},
    uint64_t nbytes{i}"""

    code += """) {
    using namespace xf::data_mover;
"""

    for i in range(0, NUM_PORTS):
        code += f"""
#pragma HLS interface axis port=s{i}
#pragma HLS interface m_axi offset=slave bundle=gmem{i} port=mm{i} max_read_burst_length=32 num_read_outstanding=4 latency=128
#pragma HLS interface s_axilite bundle=control port=mm{i}
#pragma HLS interface s_axilite bundle=control port=nbytes{i}"""

    code += """
#pragma HLS interface s_axilite bundle=control port=return
#pragma HLS dataflow"""

    for i in range(0, NUM_PORTS):
        code += f"""
    storeStreamToMaster(s{i}, mm{i}, nbytes{i});
"""

    code += """
}
"""
    return {
        "source": code
    }
