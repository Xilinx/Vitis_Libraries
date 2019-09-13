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

#define NRC 4
#define NCU 1

void top_gtsv(int n, double* matDiagLow, double* matDiag, double* matDiagUp, double* rhs) {
#pragma HLS INTERFACE m_axi offset = slave bundle = gmem0 port = matDiagLow latency = 64 num_read_outstanding = \
    16 num_write_outstanding = 16 max_read_burst_length = 64 max_write_burst_length = 64 depth = 16

#pragma HLS INTERFACE m_axi offset = slave bundle = gmem0 port = matDiag latency = 64 num_read_outstanding = \
    16 num_write_outstanding = 16 max_read_burst_length = 64 max_write_burst_length = 64 depth = 16

#pragma HLS INTERFACE m_axi offset = slave bundle = gmem0 port = matDiagUp latency = 64 num_read_outstanding = \
    16 num_write_outstanding = 16 max_read_burst_length = 64 max_write_burst_length = 64 depth = 16

#pragma HLS INTERFACE m_axi offset = slave bundle = gmem0 port = rhs latency = 64 num_read_outstanding = \
    16 num_write_outstanding = 16 max_read_burst_length = 64 max_write_burst_length = 64 depth = 16

#pragma HLS INTERFACE s_axilite port = matDiagLow bundle = control
#pragma HLS INTERFACE s_axilite port = matDiag bundle = control
#pragma HLS INTERFACE s_axilite port = matDiagUp bundle = control
#pragma HLS INTERFACE s_axilite port = rhs bundle = control

#pragma HLS INTERFACE s_axilite port = return bundle = control

    xf::solver::gtsv<double, NRC, NCU>(n, matDiagLow, matDiag, matDiagUp, rhs);
};

int main() {
    double matDiagLow1[NRC];
    double matDiag1[NRC];
    double matDiagUp1[NRC];
    double rhs1[NRC];

    double matDiagLow2[NRC];
    double matDiag2[NRC];
    double matDiagUp2[NRC];
    double rhs2[NRC];

    for (int i = 0; i < NRC; i++) {
        matDiagLow1[i] = -1.0;
        matDiag1[i] = 2.0;
        matDiagUp1[i] = -1.0;
        rhs1[i] = 0.0;
        matDiagLow2[i] = -1.0;
        matDiag2[i] = 2.0;
        matDiagUp2[i] = -1.0;
        rhs2[i] = 0.0;
    };
    rhs1[0] = 1.0;
    rhs1[NRC - 1] = 1.0;
    rhs2[0] = 1.0;
    rhs2[NRC - 1] = 1.0;

    top_gtsv(NRC, matDiagLow1, matDiag1, matDiagUp1, rhs1);

    for (int i = 0; i < NRC; i++) {
        std::cout << rhs1[i] << std::endl;
    }

    int info = 0;
    for (int i = 0; i < NRC; i++) {
        if (i == 0) {
            double r = matDiag1[i] * rhs1[i] + matDiagUp1[i] * rhs1[i + 1];
            if (r != rhs2[i]) {
                info = 1;
                break;
            }
        } else if (i == (NRC - 1)) {
            double r = matDiagLow1[i - 1] * rhs1[i - 1] + matDiag1[i] * rhs1[i];
            if (r != rhs2[i]) {
                info = 1;
                break;
            }
        } else {
            double r = matDiagLow1[i - 1] * rhs1[i - 1] + matDiag1[i] * rhs1[i] + matDiagUp1[i] * rhs1[i + 1];
            if (r != rhs2[i]) {
                info = 1;
                break;
            }
        }
    }

    return info;
};
