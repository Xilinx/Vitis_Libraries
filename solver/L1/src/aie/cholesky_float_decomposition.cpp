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

#include "aie/cholesky_float_decomposition.hpp"
#include "aie_api/utils.hpp"
#include "aie_api/aie_adf.hpp"
#include "aie_api/adf/stream.hpp"

void cholesky_float(input_stream<float>* __restrict matA, output_stream<float>* __restrict matL) {
    // For profiling only
    static unsigned cycle_num[2];
    aie::tile tile = aie::tile::current();
    volatile unsigned* p_cycle = cycle_num;
    cycle_num[0] = tile.cycles(); // cycle counter of the AI Engine tile

    v4float a_ik;
    aie::vector<float, 4> L_ij;
    aie::vector<float, 4> A_ik;
    aie::accum<accfloat, 4> Acc_ik;

    float val;
    float lij;
    float lkj;
    float ajj;
    float aik;
    float diag;

    float outCols[256];

    int dim, pid;
    int num, cnt;
    dim = readincr(matA);
    pid = readincr(matA);
    num = readincr(matA);
    cnt = readincr(matA);
    writeincr(matL, dim);             // dim
    writeincr(matL, pid + 1);         // pid = pid + 1
    writeincr(matL, num + dim - pid); // num = num + dim -pid
    writeincr(matL, cnt);             // cnt

    for (int i = 0; i < num; i++) chess_prepare_for_pipelining {
            val = readincr(matA);
            writeincr(matL, val);
        }
    // calculate diag element
    ajj = readincr(matA);
    diag = aie::sqrt(ajj);
    float invDiag = aie::inv(diag);
    writeincr(matL, diag);
    outCols[pid] = diag;

    // calculate other column elements
    for (int i = pid + 1; i < dim; i++) chess_prepare_for_pipelining {
            lij = readincr(matA);
            lij = lij * invDiag;
            writeincr(matL, lij);
            outCols[i] = lij;
        }
    // update the rest of matA elements
    int k = pid + 1;
    int len = (dim - k) % 4;
    for (; k < dim; k++) {
        lkj = outCols[k];
        for (int q = k; q < (len + k); q++) chess_prepare_for_pipelining {
                lij = outCols[q];
                aik = readincr(matA);
                aik = aik - lij * lkj;
                writeincr(matL, aik);
            }
        int i = k + len;
        for (; i < dim - 3; i += 4) chess_prepare_for_pipelining {
                L_ij = aie::load_v<4>(outCols + i);
                A_ik = readincr_v<4>(matA);
                Acc_ik.from_vector(A_ik);
                Acc_ik = aie::msc(Acc_ik, L_ij, lkj);
                A_ik = Acc_ik.to_vector<float>();
                writeincr(matL, A_ik);
            }
        len = (len + 3) % 4;
    }

    // For profiling only
    cycle_num[1] = tile.cycles(); // cycle counter of the AI Engine tile
#if defined(__AIESIM__) || defined(__X86SIM__)
    printf("pid=%d, start=%d,end=%d,total=%d\n", pid, cycle_num[0], cycle_num[1], cycle_num[1] - cycle_num[0]);
#endif

} // end cholesky_float
