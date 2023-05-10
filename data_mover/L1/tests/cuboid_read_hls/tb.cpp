/*
 * Copyright (C) 2019-2022, Xilinx, Inc.
 * Copyright (C) 2022-2023, Advanced Micro Devices, Inc.
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

#include <ap_int.h>
#include "xf_data_mover/pl_data_mover.hpp"

static constexpr int N = 3200;
static constexpr int descriptor_num = 2;
static constexpr int M = (9 * descriptor_num + 1);

void dut(hls::burst_maxi<ap_uint<64> > data,
         hls::burst_maxi<ap_uint<64> > descriptors,
         hls::stream<ap_axiu<64, 0, 0, 0> >& w_data) {
#pragma HLS interface m_axi offset = slave bundle = gmem0 port = data latency = 32 num_write_outstanding = \
    32 num_read_outstanding = 32 max_write_burst_length = 32 max_read_burst_length = 32 depth = N
#pragma HLS interface m_axi offset = slave bundle = gmem1 port = descriptors latency = 32 num_write_outstanding = \
    32 num_read_outstanding = 32 max_write_burst_length = 32 max_read_burst_length = 32 depth = M
#pragma HLS interface axis port = w_data
    xf::data_mover::read4D<64, 32, 32, 32>(descriptors, data, w_data);
}

#ifndef __SYNTHESIS__
int main() {
    ap_uint<64> cfg[M];
    cfg[0] = descriptor_num;
    cfg[1 + 0] = 10;
    cfg[1 + 1] = 1;
    cfg[1 + 2] = 10;
    cfg[1 + 3] = 10;
    cfg[1 + 4] = 10;
    cfg[1 + 5] = 100;
    cfg[1 + 6] = 10;
    cfg[1 + 7] = 1000;
    cfg[1 + 8] = 2;
    cfg[10 + 0] = 33;
    cfg[10 + 1] = 1;
    cfg[10 + 2] = 10;
    cfg[10 + 3] = 10;
    cfg[10 + 4] = 10;
    cfg[10 + 5] = 100;
    cfg[10 + 6] = 10;
    cfg[10 + 7] = 1000;
    cfg[10 + 8] = 2;
    static_assert(10 + 8 < M, "cfg overflow");

    ap_uint<64>* data = (ap_uint<64>*)malloc(N * sizeof(ap_uint<64>));
    for (int i = 0; i < N; i++) {
        data[i] = i;
    }

    hls::stream<ap_axiu<64, 0, 0, 0> > w_data;

    dut(data, cfg, w_data);

    bool check = true;
    std::cout << "start res checking" << std::endl;
    for (size_t des_idx = 0; des_idx < descriptor_num; des_idx++) {
        ap_uint<64>* tmp_cfg = cfg + (1 + des_idx * 9);

        for (size_t d4 = 0; d4 < tmp_cfg[8]; d4++) {
            for (size_t d3 = 0; d3 < tmp_cfg[6]; d3++) {
                for (size_t d2 = 0; d2 < tmp_cfg[4]; d2++) {
                    for (size_t d1 = 0; d1 < tmp_cfg[2]; d1++) {
                        size_t ptr = d4 * tmp_cfg[7] + d3 * tmp_cfg[5] + d2 * tmp_cfg[3] + d1 * tmp_cfg[1] + tmp_cfg[0];
                        ap_uint<64> f1 = data[ptr];
                        ap_axiu<64, 0, 0, 0> tmp_axiu = w_data.read();
                        ap_uint<64> tmp_data = tmp_axiu.data;

                        if (f1 != tmp_data) {
                            std::cout << "golden = " << f1 << " data = " << tmp_data << std::endl;
                            check = false;
                        }
                    }
                }
            }
        }
    }

    free(data);

    if (check) {
        return 0;
    } else {
        return 1;
    }
}
#endif
