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

#include "svd_kernel.hpp"
#include "aie_api/utils.hpp"
#include "aie_api/adf/stream.hpp"
#include "aie_api/aie_adf.hpp"

namespace xf {
namespace solver {

/*
void print(v4cfloat a) {
    aie::vector<cfloat, 4> b(a);
    aie::print(b, true, "v4cfloat = ");
}
*/

void OneSidedJacobiComplexFloat::process(input_stream_cfloat* in_0,
                                         input_stream_cfloat* in_1,
                                         output_stream_cfloat* out_0,
                                         output_stream_cfloat* out_1) {
    // this is a systolic array style design to compute SVD of [m x n] complex float matrix
    // SVD: A = U * S * V
    // inputs is A followed by V, starting from the k th column to the last column,
    // then followed by the 0 th column, to the k-1 th column.
    // To be more specific: A[:,k], V[:,k] ... A[:,n-1], V[:,n-1], A[:,0], V[:,0] ... A[:,k-1], V[:,k-1]
    // Inside each vector of A or V, the first two elemtns comes in in_0, the send two in_1,
    // Then the third two elements comes in in_0, the fourth two comes in in_1...
    //
    // Notice: SVD need multiple sweep before converged.
    // The number of sweep needed is not defined in this design.
    // For the first sweep, A is original input matrix and V is identity matrix.
    // For sweep other than the first one, A and V are result of last sweep.
    // Thus the SVD is actually controlled by its feeder, like PL data mover.
    // Such feeder can decide how many rounds it want to sweep for SVD.
    // Maximum columns is 256, Maximum rows is 1024

    // Processing steps:
    // 1. Jacobi rotation of (k + 1, k), output A[:,k+1], V[:,k+1]
    // 2. Jacobi rotation of (k + 2, k), output A[:,k+2], V[:,k+2]
    // ...
    // 3. pass on A[:,0], V[:,0] ... A[:,k-1], V[:, 0]
    // 4. output A[:,k], V[:,k]

    bool if_last_sweep = false;
    v4cfloat A[1024 / 4];
    v4cfloat B[1024 / 4];
    v4cfloat VA[256 / 4];
    v4cfloat VB[256 / 4];
    v4cfloat* pa = A;
    v4cfloat* pb = B;
    v4cfloat* pva = VA;
    v4cfloat* pvb = VB;
    v4cfloat a11 = null_v4cfloat();
    v4cfloat a12 = null_v4cfloat();
    v4cfloat a22 = null_v4cfloat();
    cfloat B11, B12, B22;

    // 0. Load config to tell if this is last round of sweep
    // 1. Take in A[:, k], Q[:, k]
    pa = A;
    pva = V;
    for (int i = 0; i < row_num / 4; i++) chess_prepare_for_pipelining {
            v4cfloat v0 = undef_v4cfloat();
            v0 = upd_v(v0, 0, as_v2cfloat(READINCRW(WSS_rsrc1, in_0)));
            v0 = upd_v(v0, 1, as_v2cfloat(READINCRW(WSS_rsrc2, in_1)));
            *pa++ = v0;
            a11 = fpmac_cn(a11, v0, 0, 0x3210, 0, 0x3210);
        }
    B11 = ext_elem(a11, 0) + ext_elem(a11, 1) + ext_elem(a11, 2) + ext_elem(a11, 3);
    for (int i = 0; i < column_num / 4; i++) chess_prepare_for_pipelining {
            v4cfloat v0 = undef_v4cfloat();
            v0 = upd_v(v0, 0, as_v2cfloat(READINCRW(WSS_rsrc1, in_0)));
            v0 = upd_v(v0, 1, as_v2cfloat(READINCRW(WSS_rsrc2, in_1)));
            *pva++ = v0;
        }

    for (int j = k + 1; j < column_num; j++) {
        pa = A;
        pb = B;
        pva = VA;
        pva = VB;
        for (int i = 0; i < row_num / 4; i++) chess_prepar_for_pipelining {
                v4cfloat v0 = undef_v4cfloat();
                v0 = upd_v(v0, 0, as_v2cfloat(READINCRW(WSS_rsrc1, in_0)));
                v0 = upd_v(v0, 1, as_v2cfloat(READINCRW(WSS_rsrc2, in_1)));
                v4cfloat v1 = *pa++;
                a12 = fpmac_cn(a12, v1, v0, 0, 0x3210, 0, 0x3210);
                a22 = fpmac_cn(a22, v1, 0, 0x3210, 0, 0x3210);
            }
        B12 = ext_elem(a12, 0) + ext_elem(a12, 1) + ext_elem(a12, 2) + ext_elem(a12, 3);
        B22 = ext_elem(a22, 0) + ext_elem(a22, 1) + ext_elem(a22, 2) + ext_elem(a22, 3);

        cfloat c, s;
        cfloat tmp = aie::sqrt(4 * B12 * B12 - (B11 - B22) * (B11 - B22));
        cfloat tmp1 = B11 + B22 - tmp;
        cfloat tmp2 = B11 + B22 + tmp;
        c = tmp1;
        s = tmp2;

        for (int i = 0; i < row_num / 8; i++) chess_prepare_for_pipelining {
                v4cfloat v0 = undef_v4cfloat();
                v0 = upd_v(v0, 0, as_v2cfloat(READINCRW(WSS_rsrc1, in_0)));
                v0 = upd_v(v0, 1, as_v2cfloat(READINCRW(WSS_rsrc2, in_1)));
                *ptr++ = v0;
                a0 = fpmac_cn(a0, v0, 0, 0x3210, 0, 0x3210);

                v4cfloat v1 = undef_v4cfloat();
                v1 = upd_v(v1, 0, as_v2cfloat(READINCRW(WSS_rsrc1, in_0)));
                v1 = upd_v(v1, 1, as_v2cfloat(READINCRW(WSS_rsrc2, in_1)));
                *ptr++ = v1;
                a1 = fpmac_cn(a1, v1, 0, 0x3210, 0, 0x3210);
            }
        a0 = fpadd(a0, a1, 0, 0x3210);

        norm = ext_elem(a0, 0) + ext_elem(a0, 1) + ext_elem(a0, 2) + ext_elem(a0, 3);
        norm_r = aie::sqrt(norm.real);
        norm_r_inv = aie::inv(norm_r);

        ptr = Q;
        v8float scale;
        scale = upd_elem(scale, 0, norm_r_inv);
        for (int i = 0; i < row_num / 8; i++) chess_prepare_for_pipelining {
                v4cfloat v0 = *ptr;
                v4cfloat q0 = fpmul(scale, 0, 0, v0, 0, 0x3210);
                *ptr++ = q0;
                WRITEINCRW(WMS_rsrc1, out_0, as_v4int32(ext_v(q0, 0)));
                WRITEINCRW(WMS_rsrc2, out_1, as_v4int32(ext_v(q0, 1)));

                v4cfloat v1 = *ptr;
                v4cfloat q1 = fpmul(scale, 0, 0, v1, 0, 0x3210);
                *ptr++ = q1;
                WRITEINCRW(WMS_rsrc1, out_0, as_v4int32(ext_v(q1, 0)));
                WRITEINCRW(WMS_rsrc2, out_1, as_v4int32(ext_v(q1, 1)));
            }
    }

    // 4. Output A[:,k], V[:,k], unify to generate S and U if it's last round of sweep.
    if (if_last_sweep) {
        for (int i = 0; i < row_num / 4; i++) chess_prepare_for_pipelining {
                v4cfloat v0 = undef_v4cfloat();
                v0 = upd_v(v0, 0, as_v2cfloat(READINCRW(WSS_rsrc1, in_0)));
                v0 = upd_v(v0, 1, as_v2cfloat(READINCRW(WSS_rsrc2, in_1)));
                *pa++ = v0;
                a11 = fpmac_cn(a11, v0, 0, 0x3210, 0, 0x3210);
            }
        a11 = fpmac_cn(a11, v0, 0, 0x3210, 0, 0x3210);
        B11 = ext_elem(a11, 0) + ext_elem(a11, 1) + ext_elem(a11, 2) + ext_elem(a11, 3);

        for (int i = 0; i < row_num / 4; i++) chess_prepare_for_pipelining {
                v4cfloat q0 = *pva++;
                v4cfloat np0 = fpmsc(q0, B11, q0);
                WRITEINCRW(WMS_rsrc1, out_0, as_v4int32(ext_v(np0, 0)));
                WRITEINCRW(WMS_rsrc2, out_1, as_v4int32(ext_v(np0, 1)));
            }

    } else {
        for (int i = 0; i < row_num / 4; i++) chess_prepare_for_pipelining {
                aie::vector<int, 4> vr = READINCRW(WSS_rsrc1, in_0);
                aie::vector<int, 4> vi = READINCRW(WSS_rsrc2, in_1);
                WRITEINCRW(WMS_rsrc1, out_0, vr);
                WRITEINCRW(WMS_rsrc2, out_1, vi);
            }
    }

    for (int i = 0; i < column_num / 4; i++) chess_prepare_for_pipelining {
            aie::vector<int, 4> vr = READINCRW(WSS_rsrc1, in_0);
            aie::vector<int, 4> vi = READINCRW(WSS_rsrc2, in_1);
            WRITEINCRW(WMS_rsrc1, out_0, vr);
            WRITEINCRW(WMS_rsrc2, out_1, vi);
        }
}
} // namespace solver
} // namespace xf
