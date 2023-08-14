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
        ports.append({"name": f"desc{i}",
                      "direction": "in",
                      "type": "ap_uint<64>*"
                     })
    return ports

def generate_krnl(krnl_name, args):

    if not krnl_name or krnl_name == '':
        krnl_name = "s2mm_4d"

    NUM_PORTS = args['NUM_PORTS']
    S_WIDTH = args['S_WIDTH']
    MM_OUTSTANDING = args['MM_OUTSTANDING']
    MM_BURST_LEN = args['MM_BURST_LEN']

    latency = 32

    code = f"""\
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

#include <cstdint>
#include "xf_data_mover/pl_data_mover.hpp"

extern "C" void {krnl_name} ("""

    for i in range(NUM_PORTS):
        if i != 0:
            code += ","
        code += f"""
    hls::stream<ap_axiu<{S_WIDTH}, 0, 0, 0> >& s{i},
    hls::burst_maxi<ap_uint<{S_WIDTH}> > mm{i},
    hls::burst_maxi<ap_uint<64> > desc{i}"""

    code += ") {"
    for i in range(NUM_PORTS):
        code += f"""
#pragma HLS interface m_axi offset=slave bundle=gmem1_{i} port=mm{i} \
max_read_burst_length={MM_BURST_LEN} num_read_outstanding={MM_OUTSTANDING} latency={latency}
#pragma HLS interface m_axi offset=slave bundle=gmem0_{i} port=desc{i} \
max_read_burst_length={MM_BURST_LEN} num_read_outstanding={MM_OUTSTANDING} latency={latency}
#pragma HLS interface s_axilite bundle=control port=mm{i}
#pragma HLS interface s_axilite bundle=control port=desc{i}
#pragma HLS interface axis port=s{i}"""

    code += """
#pragma HLS interface s_axilite bundle=control port=return
#pragma HLS dataflow
"""
    for i in range(NUM_PORTS):
        code += f"""
    xf::data_mover::write4D<{S_WIDTH}, {latency}, {MM_OUTSTANDING}, {MM_BURST_LEN}>(desc{i}, s{i}, mm{i});"""

    code += """
}
"""

    return {
        "source": code
    }
