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

#include "aie/cholesky_complex_decomposition.hpp"
#include "aie_api/utils.hpp"
#include "aie_api/aie_adf.hpp"
#include "aie_api/adf/stream.hpp"

union DType {
    float f;
    int i;
};
void cholesky_complex(input_stream<float>* __restrict matA_real,
                      input_stream<float>* __restrict matA_imag,
                      output_stream<float>* __restrict matL_real,
                      output_stream<float>* __restrict matL_imag) {
    // For profiling only
    static unsigned cycle_num[2];
    aie::tile tile = aie::tile::current();
    volatile unsigned* p_cycle = cycle_num;
    cycle_num[0] = tile.cycles(); // cycle counter of the AI Engine tile

    v4float a_ik_real;
    v4float a_ik_imag;
    aie::vector<float, 4> L_ij_real;
    aie::vector<float, 4> L_ij_imag;
    aie::vector<float, 4> A_ik_real;
    aie::vector<float, 4> A_ik_imag;
    aie::accum<accfloat, 4> Acc_ik_real;
    aie::accum<accfloat, 4> Acc_ik_imag;

    DType info0_real, info0_imag;
    DType info1_real, info1_imag;
    DType val_real, val_imag;
    DType lij_real, lij_imag;
    DType lkj_real, lkj_imag;
    DType ajj_real, ajj_imag;
    DType aik_real, aik_imag;
    DType diag;

    float outCols_real[256];
    float outCols_imag[256];

    int dim, pid;
    int num, cnt;
    info0_real.i = READINCR(SS_rsrc1, matA_real);
    info0_imag.i = READINCR(SS_rsrc2, matA_imag);
    dim = info0_real.f;
    num = info0_imag.f;
    info1_real.i = READINCR(SS_rsrc1, matA_real);
    info1_imag.i = READINCR(SS_rsrc2, matA_imag);
    pid = info1_real.f;
    cnt = info1_imag.f;
    info1_real.f = pid + 1;
    info0_imag.f = num + dim - pid;
    WRITEINCR(MS_rsrc1, matL_real, info0_real.i); // dim
    WRITEINCR(MS_rsrc2, matL_imag, info0_imag.i); // num = num + dim -pid
    WRITEINCR(MS_rsrc1, matL_real, info1_real.i); // pid = pid + 1
    WRITEINCR(MS_rsrc2, matL_imag, info1_imag.i); // cnt

    for (int i = 0; i < num; i++) chess_prepare_for_pipelining {
            val_real.i = READINCR(SS_rsrc1, matA_real);
            val_imag.i = READINCR(SS_rsrc2, matA_imag);
            WRITEINCR(MS_rsrc1, matL_real, val_real.i);
            WRITEINCR(MS_rsrc2, matL_imag, val_imag.i);
        }
    // calculate diag element
    ajj_real.i = READINCR(SS_rsrc1, matA_real);
    ajj_imag.i = READINCR(SS_rsrc2, matA_imag);
    diag.f = aie::sqrt(ajj_real.f);
    float invDiag = aie::inv(diag.f);
    ajj_real.f = diag.f;
    ajj_imag.f = 0;
    WRITEINCR(MS_rsrc1, matL_real, diag.i);
    WRITEINCR(MS_rsrc2, matL_imag, 0);
    outCols_real[pid] = diag.f;
    outCols_imag[pid] = 0;

    // calculate other column elements
    for (int i = pid + 1; i < dim; i++) chess_prepare_for_pipelining {
            lij_real.i = READINCR(SS_rsrc1, matA_real);
            lij_imag.i = READINCR(SS_rsrc2, matA_imag);
            lij_real.f = lij_real.f * invDiag;
            lij_imag.f = lij_imag.f * invDiag;
            WRITEINCR(MS_rsrc1, matL_real, lij_real.i);
            WRITEINCR(MS_rsrc2, matL_imag, lij_imag.i);
            outCols_real[i] = lij_real.f;
            outCols_imag[i] = lij_imag.f;
        }
    // update the rest of matA elements
    int k = pid + 1;
    for (; k < dim; k++) {
        lkj_real.f = outCols_real[k];
        lkj_imag.f = -(outCols_imag[k]);
        int len = (dim - k) % 4;
        for (int q = k; q < (len + k); q++) chess_prepare_for_pipelining {
                lij_real.f = outCols_real[q];
                lij_imag.f = outCols_imag[q];
                aik_real.i = READINCR(SS_rsrc1, matA_real);
                aik_imag.i = READINCR(SS_rsrc2, matA_imag);
                aik_real.f = aik_real.f - lij_real.f * lkj_real.f + lij_imag.f * lkj_imag.f;
                aik_imag.f = aik_imag.f - lij_real.f * lkj_imag.f - lij_imag.f * lkj_real.f;
                WRITEINCR(MS_rsrc1, matL_real, aik_real.i);
                WRITEINCR(MS_rsrc2, matL_imag, aik_imag.i);
            }
        for (int i = k + len; i < dim - 3; i += 4) chess_prepare_for_pipelining {
                L_ij_real = aie::load_v<4>(outCols_real + i);
                L_ij_imag = aie::load_v<4>(outCols_imag + i);
                a_ik_real = as_v4float(READINCRW(WSS_rsrc1, matA_real));
                a_ik_imag = as_v4float(READINCRW(WSS_rsrc2, matA_imag));
                A_ik_real = a_ik_real;
                A_ik_imag = a_ik_imag;
                Acc_ik_real.from_vector(A_ik_real);
                Acc_ik_imag.from_vector(A_ik_imag);
                Acc_ik_real = aie::msc(Acc_ik_real, L_ij_real, lkj_real.f);
                Acc_ik_real = aie::mac(Acc_ik_real, L_ij_imag, lkj_imag.f);
                Acc_ik_imag = aie::msc(Acc_ik_imag, L_ij_real, lkj_imag.f);
                Acc_ik_imag = aie::msc(Acc_ik_imag, L_ij_imag, lkj_real.f);
                A_ik_real = Acc_ik_real.to_vector<float>();
                a_ik_real = A_ik_real;
                A_ik_imag = Acc_ik_imag.to_vector<float>();
                a_ik_imag = A_ik_imag;
                WRITEINCRW(WMS_rsrc1, matL_real, as_v4int32(a_ik_real));
                WRITEINCRW(WMS_rsrc2, matL_imag, as_v4int32(a_ik_imag));
            }
    }

    // For profiling only
    cycle_num[1] = tile.cycles(); // cycle counter of the AI Engine tile
#if defined(__AIESIM__) || defined(__X86SIM__)
    printf("pid=%d, start=%d,end=%d,total=%d\n", pid, cycle_num[0], cycle_num[1], cycle_num[1] - cycle_num[0]);
#endif

} // end cholesky_complex
