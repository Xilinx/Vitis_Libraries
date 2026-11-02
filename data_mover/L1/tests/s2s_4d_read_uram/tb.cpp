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
#include "xf_data_mover/dm_4d_uram.hpp"

void dut(hls::stream<ap_uint<32> >& waddr_strm,
         hls::stream<ap_uint<64> >& wdata_strm,
         ap_uint<32>* pattern_buf,
         hls::stream<ap_uint<33> >& pattern_id,

         hls::stream<bool>& ack,
         hls::stream<ap_axiu<64, 0, 0, 0> >& w_data) {
#pragma HLS interface axis port = w_data
    xf::data_mover::uram_to_axis<64, 4 * 4096>(waddr_strm, wdata_strm, pattern_buf, pattern_id, ack, w_data);
}

#ifndef __SYNTHESIS__

static int OCP = 0xFFFF;
static int template_1D_pattern[24] = {
    OCP, 1, 1, 1, // buffer_dim[4]
    OCP, 0, 0, 0, // offset[4]
    OCP, 1, 1, 1, // tiling[4]
    0,   1, 2, 3, // dim_idx[4]
    OCP, 0, 0, 0, // stride[4]
    OCP, 1, 1, 1  // wrap[4]
};
static int template_2D_pattern[24] = {
    OCP, OCP, 1, 1, // buffer_dim[4]
    OCP, OCP, 0, 0, // offset[4]
    OCP, OCP, 1, 1, // tiling[4]
    OCP, OCP, 2, 3, // dim_idx[4]
    OCP, OCP, 0, 0, // stride[4]
    OCP, OCP, 1, 1  // wrap[4]
};
static int template_3D_pattern[24] = {
    OCP, OCP, OCP, 1, // buffer_dim[4]
    OCP, OCP, OCP, 0, // offset[4]
    OCP, OCP, OCP, 1, // tiling[4]
    OCP, OCP, OCP, 3, // dim_idx[4]
    OCP, OCP, OCP, 0, // stride[4]
    OCP, OCP, OCP, 1  // wrap[4]
};
static int template_4D_pattern[24] = {
    OCP, OCP, OCP, OCP, // buffer_dim[4]
    OCP, OCP, OCP, OCP, // offset[4]
    OCP, OCP, OCP, OCP, // tiling[4]
    OCP, OCP, OCP, OCP, // dim_idx[4]
    OCP, OCP, OCP, OCP, // stride[4]
    OCP, OCP, OCP, OCP  // wrap[4]
};

int random_gen(const int num_of_pattern, int* pattern_vec) {
    int sum_elem = 0;
    for (int p = 0; p < num_of_pattern;) {
        int dim = rand() % 4;
        int ptn_template[24];
        switch (dim) {
            case 0:
                memcpy(ptn_template, template_1D_pattern, 24 * sizeof(int));
                break;
            case 1:
                memcpy(ptn_template, template_2D_pattern, 24 * sizeof(int));
                break;
            case 2:
                memcpy(ptn_template, template_3D_pattern, 24 * sizeof(int));
                break;
            case 3:
                memcpy(ptn_template, template_4D_pattern, 24 * sizeof(int));
                break;
        }

        // for buffer dim
        for (int i = 0; i < 4; i++) {
            int* tmp = ptn_template + 0;
            if (tmp[i] == OCP) {
                tmp[i] = rand() % 3 + 8;
            }
        }
        // for offset
        for (int i = 0; i < 4; i++) {
            int* tmp = ptn_template + 4;
            if (tmp[i] == OCP) {
                tmp[i] = rand() % 3;
            }
        }
        // for stride
        for (int i = 0; i < 4; i++) {
            int* tmp = ptn_template + 16;
            if (tmp[i] == OCP) {
                tmp[i] = rand() % 3 + 2;
            }
        }
        // for wrap
        for (int i = 0; i < 4; i++) {
            int* tmp = ptn_template + 20;
            if (tmp[i] == OCP) {
                tmp[i] = rand() % 3 + 2;
            }
        }
        // offset + stride * (wrap - 1) <= buf_dim
        bool k0 = (ptn_template[0] < (ptn_template[16] * (ptn_template[20] - 1) + ptn_template[4]));
        bool k1 = (ptn_template[1] < (ptn_template[17] * (ptn_template[21] - 1) + ptn_template[5]));
        bool k2 = (ptn_template[2] < (ptn_template[18] * (ptn_template[22] - 1) + ptn_template[6]));
        bool k3 = (ptn_template[3] < (ptn_template[19] * (ptn_template[23] - 1) + ptn_template[7]));
        if (k0 || k1 || k2 || k3) continue;

        // for tiling
        for (int i = 0; i < 4; i++) {
            int* tmp = ptn_template + 8;
            if (tmp[i] == OCP) {
                tmp[i] = rand() % 3 + 2;
            }
        }
        // for dim_idx
        bool valid[4] = {true, true, true, true};
        for (int i = 0; i < 4; i++) {
            int* tmp = ptn_template + 12;
            if (tmp[i] == OCP) {
                while (1) {
                    int id = rand() % (dim + 1);
                    if (valid[id]) {
                        tmp[i] = id;
                        valid[id] = false;
                        break;
                    }
                }
            }
        }

        memcpy(pattern_vec + p * 24, ptn_template, 24 * sizeof(int));
        sum_elem += (ptn_template[8] * ptn_template[9] * ptn_template[10] * ptn_template[11] * ptn_template[20] *
                     ptn_template[21] * ptn_template[22] * ptn_template[23]);
        p++;
    }

    return sum_elem;
}

int main() {
    int ptr = 0;
    const int num_of_pattern = 100;
    int* pattern_vec = (int*)malloc(num_of_pattern * 24 * sizeof(int));

    hls::stream<ap_uint<32> > waddr;
    hls::stream<ap_uint<64> > wdata;
    hls::stream<ap_uint<33> > ptn_id;
    for (int i = 0; i < 4 * 4096; i++) {
        waddr << i;
        wdata << i;
    }
    for (int i = 0; i < num_of_pattern; i++) ptn_id.write(i);
    ptn_id.write(0x100000000);

    int total_elem = random_gen(num_of_pattern, pattern_vec);
    std::cout << "Complete to generate " << num_of_pattern << " random pattern" << std::endl;

    hls::stream<bool> ack;
    hls::stream<ap_axiu<64, 0, 0, 0> > w_data;

    dut(waddr, wdata, (ap_uint<32>*)pattern_vec, ptn_id, ack, w_data);

    int nerr = 0;
    if (num_of_pattern != ack.size()) {
        std::cout << "Mismatch in number of pattern: " << ack.size() << ", expected: " << num_of_pattern << std::endl;
        nerr++;
    }
    if (total_elem != w_data.size()) {
        std::cout << "Mismatch in number of element for all patterns: " << w_data.size() << ", expected: " << total_elem
                  << std::endl;
        nerr++;
    }

    for (int p = 0; p < num_of_pattern; p++) {
        int* current_ptn = pattern_vec + 24 * p;
        int* buf_dim = current_ptn;
        int* offset = current_ptn + 4;
        int* tiling = current_ptn + 8;
        int* dim_idx = current_ptn + 12;
        int* stride = current_ptn + 16;
        int* wrap = current_ptn + 20;
        for (int w = 0; w < wrap[dim_idx[3]]; w++) {
            for (int z = 0; z < wrap[dim_idx[2]]; z++) {
                for (int y = 0; y < wrap[dim_idx[1]]; y++) {
                    for (int x = 0; x < wrap[dim_idx[0]]; x++) {
                        int bias[4];
                        bias[dim_idx[0]] = offset[dim_idx[0]] + stride[dim_idx[0]] * x;
                        bias[dim_idx[1]] = offset[dim_idx[1]] + stride[dim_idx[1]] * y;
                        bias[dim_idx[2]] = offset[dim_idx[2]] + stride[dim_idx[2]] * z;
                        bias[dim_idx[3]] = offset[dim_idx[3]] + stride[dim_idx[3]] * w;
                        for (int l = 0; l < tiling[3]; l++) {
                            for (int k = 0; k < tiling[2]; k++) {
                                for (int j = 0; j < tiling[1]; j++) {
                                    for (int i = 0; i < tiling[0]; i++) {
                                        int golden = (bias[3] + l) * (buf_dim[2] * buf_dim[1] * buf_dim[0]) +
                                                     (bias[2] + k) * (buf_dim[1] * buf_dim[0]) +
                                                     (bias[1] + j) * buf_dim[0] + (bias[0] + i);
                                        ap_axiu<64, 0, 0, 0> hw_result = w_data.read();
                                        if (golden != hw_result.data) {
                                            nerr++;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        ack.read();
    }

    free(pattern_vec);

    return nerr;
}
#endif
