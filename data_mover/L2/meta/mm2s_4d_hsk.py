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

    NUM_PORTS = args["NUM_PORTS"]
    S_WIDTH = args['S_WIDTH']

    ports = []
    for i in range(0, NUM_PORTS):
        ports.append({"name": f"cfg{i}",
                      "direction": "in",
					  "type": "hls::burst_maxi<ap_uint<{S_WIDTH}> >"
                     })
        ports.append({"name": f"data{i}",
                      "direction": "in",
					  "type": "hls::burst_maxi<ap_uint<{S_WIDTH}> >"
                     })
        ports.append({"name": f"sync_i{i}",
                      "direction": "in",
                      "type": "hls::stream<ap_axiu<16,0,0,0> >&"
                     })
        ports.append({"name": f"sync_o{i}",
                      "direction": "out",
                      "type": "hls::stream<ap_axiu<16,0,0,0> >&"
                     })
        ports.append({"name": f"axis_o{i}",
                      "direction": "out",
                      "type": "hls::stream<ap_axiu<{S_WIDTH},0,0,0> >&"
                     })
    return ports

def generate_krnl(krnl_name, args):

    if not krnl_name or krnl_name == '':
        krnl_name = 'mm2s_4d_hsk'

    NUM_PORTS = args["NUM_PORTS"]
    S_WIDTH = args['S_WIDTH']
    C_DEPTH = args['C_DEPTH']
    MM_OUTSTANDING = args['MM_OUTSTANDING']
    MM_BURST_LEN = args['MM_BURST_LEN']

    latency = 32
    c_latency = 16

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

#include "xf_data_mover/pl_4d_data_mover.hpp"

extern "C" void {krnl_name} ("""

    for i in range(NUM_PORTS):
        if i != 0:
            code += ","
        code += f"""
    hls::burst_maxi<ap_uint<{S_WIDTH}> > cfg{i},
    hls::burst_maxi<ap_uint<{S_WIDTH}> > data{i},
    hls::stream<ap_axiu<16, 0, 0, 0> >& sync_i{i},
    hls::stream<ap_axiu<16, 0, 0, 0> >& sync_o{i},
    hls::stream<ap_axiu<{S_WIDTH}, 0, 0, 0> >& axis_o{i}"""

    code += ") {"
    for i in range(NUM_PORTS):
        code += f"""
#pragma HLS interface m_axi offset=slave bundle=gmem0_{i} port=cfg{i} \
max_read_burst_length={MM_BURST_LEN} num_read_outstanding={MM_OUTSTANDING} latency={latency}
#pragma HLS interface m_axi offset=slave bundle=gmem0_{i} port=data{i} \
max_read_burst_length={MM_BURST_LEN} num_read_outstanding={MM_OUTSTANDING} latency={latency}
#pragma HLS interface s_axilite bundle=control port=cfg{i}
#pragma HLS interface s_axilite bundle=control port=data{i}
#pragma HLS interface axis port=sync_i{i}
#pragma HLS interface axis port=sync_o{i}
#pragma HLS interface axis port=axis_o{i}"""

    code += """
#pragma HLS interface s_axilite bundle=control port=return
#pragma HLS dataflow
"""
    for i in range(NUM_PORTS):
        code += f"""
    xf::data_mover::ddr_to_stream<{S_WIDTH}, {C_DEPTH}, {c_latency}, {MM_BURST_LEN}, {MM_OUTSTANDING}>(cfg{i}, data{i}, sync_i{i}, sync_o{i}, axis_o{i});"""

    code += """
}
"""

    return {
        "source": code
    }
