/*
 * Copyright 2019 Xilinx, Inc.
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

#include "xf_solver_L2.hpp"

#define MAXM 4 // Matrix Row size
#define MAXN 3 // Matrix Col size
#define MCU 2  // Matrix Col size
#define NCU 2  // Matrix Col size

extern "C" void kernel_gesvj_0(int rows,
                               int cols,
                               double dataA[MAXM * MAXN],
                               double sigma[MAXN],
                               double dataU[MAXM * MAXM],
                               double dataV[MAXN * MAXN]) {
#pragma HLS INTERFACE m_axi port = dataA bundle = gmem0 offset = slave num_read_outstanding = \
    16 max_read_burst_length = 32
#pragma HLS INTERFACE m_axi port = sigma bundle = gmem1 offset = slave num_write_outstanding = \
    16 max_write_burst_length = 32
#pragma HLS INTERFACE m_axi port = dataU bundle = gmem2 offset = slave num_write_outstanding = \
    16 max_write_burst_length = 32
#pragma HLS INTERFACE m_axi port = dataV bundle = gmem3 offset = slave num_write_outstanding = \
    16 max_write_burst_length = 32

#pragma HLS INTERFACE s_axilite port = rows bundle = control
#pragma HLS INTERFACE s_axilite port = cols bundle = control
#pragma HLS INTERFACE s_axilite port = dataA bundle = control
#pragma HLS INTERFACE s_axilite port = dataU bundle = control
#pragma HLS INTERFACE s_axilite port = sigma bundle = control
#pragma HLS INTERFACE s_axilite port = dataV bundle = control
#pragma HLS INTERFACE s_axilite port = return bundle = control

    //  // Calling for svd core function
    xf::solver::gesvj<double, MAXM, MAXN, MCU, NCU>(rows, cols, dataA, dataU, sigma, dataV);
}
