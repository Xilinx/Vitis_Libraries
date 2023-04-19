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

#include "qrd_kernel.hpp"
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

void GramSchmidtKernelComplexFloat::process(input_stream_cfloat* in_0,
                                            input_stream_cfloat* in_1,
                                            output_stream_cfloat* out_0,
                                            output_stream_cfloat* out_1) {
// For profiling only
#if defined(__AIESIM__) || defined(__X86SIM__)
    static unsigned cycle_num[2];
    aie::tile tile = aie::tile::current();
    volatile unsigned* p_cycle = cycle_num;
    cycle_num[0] = tile.cycles();
#endif
    // this is a systolic array style design
    // if this is the k th stage, its input is
    // Qc[0], Qc[1]... Qc[k-1], Ac'[k], Ac'[k+1] ... Ac'[col_num - 1], Rr[k - 1] .. Rr[1], Rr[0]
    // Qc[i] is Q's i th column, Ac'[i] is modified A's i th column, Rr[i] is R's i th row (upper traingle half)

    // Processing steps
    // 1. pass Qc[0] ... Qc[k-1] on to next
    // 2. take Ac'[k], generate Qc[k] and Rr[k][k] and pass Qc[k] on to next
    // 3. Modify Ac'[k+1] . Ac'[column - 1] and rest of Rr[k], pass on Ac'[k+1] ... Ac'[column-1]to next
    // 4. pass Rr[k] Rr[k - 1] .. Rr[1], Rr[0] on to next

    v4cfloat Q[1024 / 4];
    v4cfloat M[1024 / 4];
    cfloat R[256];
    for (int i = 0; i < column_id; i++) chess_prepare_for_pipelining {
            R[i].real = 0;
            R[i].imag = 0;
        }
    // 1. pass Qc[0] ... Qc[k-1] on to next. XXX Opt done.
    for (int j = 0; j < column_id; j++) {
        for (int i = 0; i < row_num / 4; i++) chess_prepare_for_pipelining {
                aie::vector<int, 4> vr = READINCRW(WSS_rsrc1, in_0);
                aie::vector<int, 4> vi = READINCRW(WSS_rsrc2, in_1);
                WRITEINCRW(WMS_rsrc1, out_0, vr);
                WRITEINCRW(WMS_rsrc2, out_1, vi);
            }
    }
    // 2. take Ac'[k], generate Qc[k] and Rr[k],  pass Qc[k] on to next
    {
        cfloat norm;
        float norm_r;
        float norm_r_inv;

        v4cfloat* ptr = Q;
        v4cfloat a0 = null_v4cfloat();
        v4cfloat a1 = null_v4cfloat();

        // take Ac'[k]
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

        // Rr[k]
        R[column_id].real = norm_r;
        R[column_id].imag = 0;

        // generate Qc[k] and pass on to next
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
    // 3. Modify Ac'[k+1] ... Ac'[column - 1] and pass on to next
    cfloat dot_product;
    if ((column_id + 1) < column_num) {
        v4cfloat* p_q = Q;
        v4cfloat* p_m = M;
        v4cfloat a0 = null_v4cfloat();
        v4cfloat a1 = null_v4cfloat();

        for (int i = 0; i < row_num / 8; i++) chess_prepare_for_pipelining {
                v4cfloat v0 = undef_v4cfloat();
                v0 = upd_v(v0, 0, as_v2cfloat(READINCRW(WSS_rsrc1, in_0)));
                v0 = upd_v(v0, 1, as_v2cfloat(READINCRW(WSS_rsrc2, in_1)));
                v4cfloat q0 = *p_q++;
                a0 = fpmac_cn(a0, q0, v0);
                *p_m++ = v0;

                v4cfloat v1 = undef_v4cfloat();
                v1 = upd_v(v1, 0, as_v2cfloat(READINCRW(WSS_rsrc1, in_0)));
                v1 = upd_v(v1, 1, as_v2cfloat(READINCRW(WSS_rsrc2, in_1)));
                v4cfloat q1 = *p_q++;
                a1 = fpmac_cn(a1, q1, v1);
                *p_m++ = v1;
            }
        a0 = fpadd(a0, a1, 0, 0x3210);
        dot_product = ext_elem(a0, 0) + ext_elem(a0, 1) + ext_elem(a0, 2) + ext_elem(a0, 3);
        R[column_id + 1] = dot_product;
    }

    for (int j = column_id + 2; j < column_num; j++) {
        v4cfloat* p_q = Q;
        v4cfloat* p_m = M;
        v4cfloat a0 = null_v4cfloat();
        v4cfloat a1 = null_v4cfloat();

        v4cfloat scale;
        scale = upd_elem(scale, 0, dot_product);
        scale = upd_elem(scale, 1, dot_product);
        scale = upd_elem(scale, 2, dot_product);
        scale = upd_elem(scale, 3, dot_product);
        for (int i = 0; i < row_num / 8; i++) chess_prepare_for_pipelining {
                v4cfloat q0 = *p_q++;
                v4cfloat v0 = undef_v4cfloat();
                v0 = upd_v(v0, 0, as_v2cfloat(READINCRW(WSS_rsrc1, in_0)));
                v0 = upd_v(v0, 1, as_v2cfloat(READINCRW(WSS_rsrc2, in_1)));
                v4cfloat op0 = *p_m;
                *p_m++ = v0;
                a0 = fpmac_cn(a0, q0, v0);
                v4cfloat np0 = fpmsc(op0, scale, q0);
                WRITEINCRW(WMS_rsrc1, out_0, as_v4int32(ext_v(np0, 0)));
                WRITEINCRW(WMS_rsrc2, out_1, as_v4int32(ext_v(np0, 1)));

                v4cfloat q1 = *p_q++;
                v4cfloat v1 = undef_v4cfloat();
                v1 = upd_v(v1, 0, as_v2cfloat(READINCRW(WSS_rsrc1, in_0)));
                v1 = upd_v(v1, 1, as_v2cfloat(READINCRW(WSS_rsrc2, in_1)));
                v4cfloat op1 = *p_m;
                *p_m++ = v1;
                a1 = fpmac_cn(a1, q1, v1);
                v4cfloat np1 = fpmsc(op1, scale, q1);
                WRITEINCRW(WMS_rsrc1, out_0, as_v4int32(ext_v(np1, 0)));
                WRITEINCRW(WMS_rsrc2, out_1, as_v4int32(ext_v(np1, 1)));
            }
        a0 = fpadd(a0, a1, 0, 0x3210);
        dot_product = ext_elem(a0, 0) + ext_elem(a0, 1) + ext_elem(a0, 2) + ext_elem(a0, 3);
        R[j] = dot_product;
    }

    if ((column_id + 1) < column_num) {
        v4cfloat* p_q = Q;
        v4cfloat* p_m = M;
        v4cfloat scale;
        scale = upd_elem(scale, 0, dot_product);
        scale = upd_elem(scale, 1, dot_product);
        scale = upd_elem(scale, 2, dot_product);
        scale = upd_elem(scale, 3, dot_product);
        for (int i = 0; i < row_num / 8; i++) chess_prepare_for_pipelining {
                v4cfloat q0 = *p_q++;
                v4cfloat op0 = *p_m++;
                v4cfloat np0 = fpmsc(op0, scale, q0);
                WRITEINCRW(WMS_rsrc1, out_0, as_v4int32(ext_v(np0, 0)));
                WRITEINCRW(WMS_rsrc2, out_1, as_v4int32(ext_v(np0, 1)));

                v4cfloat q1 = *p_q++;
                v4cfloat op1 = *p_m++;
                v4cfloat np1 = fpmsc(op1, scale, q1);
                WRITEINCRW(WMS_rsrc1, out_0, as_v4int32(ext_v(np1, 0)));
                WRITEINCRW(WMS_rsrc2, out_1, as_v4int32(ext_v(np1, 1)));
            }
    }
    // 4. pass Rr[0] .. Rr[k] on to next
    for (int i = 0; i < column_id; i++) {
        for (int j = 0; j < column_num / 4; j++) chess_prepare_for_pipelining {
                aie::vector<int, 4> vr = READINCRW(WSS_rsrc1, in_0);
                aie::vector<int, 4> vi = READINCRW(WSS_rsrc2, in_1);
                WRITEINCRW(WMS_rsrc1, out_0, vr);
                WRITEINCRW(WMS_rsrc2, out_1, vi);
            }
    }
    for (int j = 0; j < column_num; j += 4) chess_prepare_for_pipelining {
            writeincr(out_0, R[j]);
            writeincr(out_0, R[j + 1]);
            writeincr(out_1, R[j + 2]);
            writeincr(out_1, R[j + 3]);
        }
// for profiling only
#if defined(__AIESIM__) || defined(__X86SIM__)
    cycle_num[1] = tile.cycles();
    printf("column_id=%d, start=%d,end=%d,total=%d\n", column_id, cycle_num[0], cycle_num[1],
           cycle_num[1] - cycle_num[0]);
#endif
}
} // namespace solver
} // namespace xf
