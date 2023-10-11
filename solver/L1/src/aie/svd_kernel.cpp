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

void print(v4cfloat a) {
    aie::vector<cfloat, 4> b(a);
    aie::print(b, true, "v4cfloat = ");
}

void inline update(v4cfloat* array, cfloat& val, int idx) {
    v4cfloat tmp = array[idx / 4];
    tmp = upd_elem(tmp, idx % 4, val);
    array[idx / 4] = tmp;
}

template <int ROW, int COL, int KN>
void OneSidedJacobiComplexFloat<ROW, COL, KN>::process(input_stream_cfloat* __restrict in_0,
                                                       input_stream_cfloat* __restrict in_1,
                                                       output_stream_cfloat* __restrict out_0,
                                                       output_stream_cfloat* __restrict out_1) {
    v4cfloat Q[(1024 + 256) / 4];
    v4cfloat M[(1024 + 256) / 4];
    for (int kk = 0; kk < KN; kk++) {
        // 1. pass processed                        :load id x (r+c),   :write id x (r + c)
        if (column_id > 0) {
            v4cfloat* ptr_q = Q;
            for (int i = 0; i < (ROW + COL) / 4; i++) chess_prepare_for_pipelining {
                    v4cfloat n0;
                    n0 = upd_v(n0, 0, as_v2cfloat(READINCRW(WSS_rsrc1, in_0)));
                    n0 = upd_v(n0, 1, as_v2cfloat(READINCRW(WSS_rsrc2, in_1)));
                    *ptr_q++ = n0;
                }
        }

        for (int j = 1; j < column_id; j++) {
            v4cfloat* ptr_q = Q;
            for (int i = 0; i < (ROW + COL) / 4; i++) chess_prepare_for_pipelining {
                    v4cfloat o0, n0;
                    o0 = *ptr_q;
                    WRITEINCRW(WMS_rsrc1, out_0, as_v4int32(ext_v(o0, 0)));
                    WRITEINCRW(WMS_rsrc2, out_1, as_v4int32(ext_v(o0, 1)));
                    n0 = upd_v(n0, 0, as_v2cfloat(READINCRW(WSS_rsrc1, in_0)));
                    n0 = upd_v(n0, 1, as_v2cfloat(READINCRW(WSS_rsrc2, in_1)));
                    *ptr_q++ = n0;
                }
        }

        // 2. <a_id, a_id>, norm_a_id               :load (r+c),        :write (r+c)
        cfloat norm2, norm2_inv, norm_inv;

        if (column_id > 0) {
            v4cfloat* ptr_q = Q;
            v4cfloat ac0 = null_v4cfloat();

            for (int i = 0; i < ROW / 4; i++) chess_prepare_for_pipelining {
                    v4cfloat o0 = *ptr_q;
                    WRITEINCRW(WMS_rsrc1, out_0, as_v4int32(ext_v(o0, 0)));
                    WRITEINCRW(WMS_rsrc2, out_1, as_v4int32(ext_v(o0, 1)));

                    v4cfloat u0;
                    u0 = upd_v(u0, 0, as_v2cfloat(READINCRW(WSS_rsrc1, in_0)));
                    u0 = upd_v(u0, 1, as_v2cfloat(READINCRW(WSS_rsrc2, in_1)));
                    *ptr_q++ = u0;
                    ac0 = fpmac_cn(ac0, u0, 0, 0x3210, 0, 0x3210);
                }

            for (int i = 0; i < COL / 4; i++) chess_prepare_for_pipelining {
                    v4cfloat o0 = *ptr_q;
                    WRITEINCRW(WMS_rsrc1, out_0, as_v4int32(ext_v(o0, 0)));
                    WRITEINCRW(WMS_rsrc2, out_1, as_v4int32(ext_v(o0, 1)));

                    v4cfloat u0;
                    u0 = upd_v(u0, 0, as_v2cfloat(READINCRW(WSS_rsrc1, in_0)));
                    u0 = upd_v(u0, 1, as_v2cfloat(READINCRW(WSS_rsrc2, in_1)));
                    *ptr_q++ = u0;
                }

            norm2 = aie::reduce_add(aie::vector<cfloat, 4>(ac0));
            norm2_inv.real = aie::inv(norm2.real);
            norm2_inv.imag = 0;
            norm_inv.real = aie::sqrt(norm2_inv.real);
            norm_inv.imag = 0;
            norm2.real = aie::sqrt(norm2.real);
            update(Q, norm2, ROW + column_id);

        } else {
            v4cfloat* ptr_q = Q;
            v4cfloat ac0 = null_v4cfloat();

            for (int i = 0; i < ROW / 4; i++) chess_prepare_for_pipelining {
                    v4cfloat u0;
                    u0 = upd_v(u0, 0, as_v2cfloat(READINCRW(WSS_rsrc1, in_0)));
                    u0 = upd_v(u0, 1, as_v2cfloat(READINCRW(WSS_rsrc2, in_1)));
                    *ptr_q++ = u0;
                    ac0 = fpmac_cn(ac0, u0, 0, 0x3210, 0, 0x3210);
                }

            for (int i = 0; i < COL / 4; i++) chess_prepare_for_pipelining {
                    v4cfloat u0;
                    u0 = upd_v(u0, 0, as_v2cfloat(READINCRW(WSS_rsrc1, in_0)));
                    u0 = upd_v(u0, 1, as_v2cfloat(READINCRW(WSS_rsrc2, in_1)));
                    *ptr_q++ = u0;
                }

            norm2 = aie::reduce_add(aie::vector<cfloat, 4>(ac0));
            norm2_inv.real = aie::inv(norm2.real);
            norm2_inv.imag = 0;
            norm_inv.real = aie::sqrt(norm2_inv.real);
            norm_inv.imag = 0;
            norm2.real = aie::sqrt(norm2.real);
            update(Q, norm2, ROW + column_id);
        }

        if ((column_id + 1) >= COL) {
            v4cfloat* ptr_q = Q;
            v4cfloat scale;
            scale = upd_elem(scale, 0, norm_inv);
            scale = upd_elem(scale, 1, norm_inv);
            scale = upd_elem(scale, 2, norm_inv);
            scale = upd_elem(scale, 3, norm_inv);

            for (int i = 0; i < ROW / 4; i++) chess_prepare_for_pipelining {
                    v4cfloat u0 = *ptr_q;
                    v4cfloat q0 = fpmul_cn(scale, u0);
                    *ptr_q++ = q0;
                    WRITEINCRW(WMS_rsrc1, out_0, as_v4int32(ext_v(q0, 0)));
                    WRITEINCRW(WMS_rsrc2, out_1, as_v4int32(ext_v(q0, 1)));
                }
            for (int i = 0; i < COL / 4; i++) chess_prepare_for_pipelining {
                    v4cfloat r0 = *ptr_q++;
                    WRITEINCRW(WMS_rsrc1, out_0, as_v4int32(ext_v(r0, 0)));
                    WRITEINCRW(WMS_rsrc2, out_1, as_v4int32(ext_v(r0, 1)));
                }
        }

        // 3. <a_id + 1, a_id>                      :load (r+c),        :
        cfloat dotp;
        if ((column_id + 1) < COL) {
            v4cfloat* ptr_q = Q;
            v4cfloat* ptr_m = M;
            v4cfloat ac0 = null_v4cfloat();
            v4cfloat scale;
            scale = upd_elem(scale, 0, norm_inv);
            scale = upd_elem(scale, 1, norm_inv);
            scale = upd_elem(scale, 2, norm_inv);
            scale = upd_elem(scale, 3, norm_inv);

            for (int i = 0; i < ROW / 4; i++) chess_prepare_for_pipelining {
                    v4cfloat a0;
                    a0 = upd_v(a0, 0, as_v2cfloat(READINCRW(WSS_rsrc1, in_0)));
                    a0 = upd_v(a0, 1, as_v2cfloat(READINCRW(WSS_rsrc2, in_1)));
                    *ptr_m++ = a0;

                    v4cfloat u0 = *ptr_q++;
                    v4cfloat q0 = fpmul_cn(scale, u0);
                    ac0 = fpmac_cn(ac0, u0, a0);
                    WRITEINCRW(WMS_rsrc1, out_0, as_v4int32(ext_v(q0, 0)));
                    WRITEINCRW(WMS_rsrc2, out_1, as_v4int32(ext_v(q0, 1)));
                }

            for (int i = 0; i < COL / 4; i++) chess_prepare_for_pipelining {
                    v4cfloat r0;
                    r0 = upd_v(r0, 0, as_v2cfloat(READINCRW(WSS_rsrc1, in_0)));
                    r0 = upd_v(r0, 1, as_v2cfloat(READINCRW(WSS_rsrc2, in_1)));
                    *ptr_m++ = r0;

                    v4cfloat q0 = *ptr_q++;
                    WRITEINCRW(WMS_rsrc1, out_0, as_v4int32(ext_v(q0, 0)));
                    WRITEINCRW(WMS_rsrc2, out_1, as_v4int32(ext_v(q0, 1)));
                }

            dotp = aie::reduce_add(aie::vector<cfloat, 4>(ac0));
        }

        // 4. <a_id + 2, a_id>, a_id+1'             :load (r+c),        :write (r+c)
        // 5. ...                                   :load (r+c),        :write (r+c)
        // 6. <a_c_1, a_id>, a_c_2'                 :load (r+c),        :write (r+c)
        for (int j = column_id + 2; j < COL; j++) {
            v4cfloat* ptr_q = Q;
            v4cfloat* ptr_m = M;
            v4cfloat ac0 = null_v4cfloat();

            v4cfloat tmp_scale;
            tmp_scale = upd_elem(tmp_scale, 0, dotp);
            tmp_scale = upd_elem(tmp_scale, 1, dotp);
            v4cfloat scale;
            scale = upd_elem(scale, 0, norm2_inv);
            scale = upd_elem(scale, 1, norm_inv);
            scale = fpmul_cn(scale, tmp_scale);

            cfloat res_r = ext_elem(scale, 1);
            update(M, res_r, ROW + column_id);
            for (int i = 0; i < ROW / 4; i++) chess_prepare_for_pipelining {
                    v4cfloat u0 = *ptr_q++;
                    v4cfloat new_a0;
                    new_a0 = upd_v(new_a0, 0, as_v2cfloat(READINCRW(WSS_rsrc1, in_0)));
                    new_a0 = upd_v(new_a0, 1, as_v2cfloat(READINCRW(WSS_rsrc2, in_1)));
                    v4cfloat old_a0 = *ptr_m;
                    *ptr_m++ = new_a0;
                    ac0 = fpmac_cn(ac0, u0, new_a0);
                    v4cfloat update_old_a0 = fpmsc(old_a0, scale, 0, 0, u0, 0, 0x3210);
                    WRITEINCRW(WMS_rsrc1, out_0, as_v4int32(ext_v(update_old_a0, 0)));
                    WRITEINCRW(WMS_rsrc2, out_1, as_v4int32(ext_v(update_old_a0, 1)));
                }

            for (int i = 0; i < COL / 4; i++) chess_prepare_for_pipelining {
                    v4cfloat old_r0 = *ptr_m;
                    WRITEINCRW(WMS_rsrc1, out_0, as_v4int32(ext_v(old_r0, 0)));
                    WRITEINCRW(WMS_rsrc2, out_1, as_v4int32(ext_v(old_r0, 1)));
                    v4cfloat new_r0;
                    new_r0 = upd_v(new_r0, 0, as_v2cfloat(READINCRW(WSS_rsrc1, in_0)));
                    new_r0 = upd_v(new_r0, 1, as_v2cfloat(READINCRW(WSS_rsrc2, in_1)));
                    *ptr_m++ = new_r0;
                }

            dotp = aie::reduce_add(aie::vector<cfloat, 4>(ac0));
        }

        // 7. a_c_1'                                :                   :write (r+c)
        if (column_id + 1 < COL) {
            v4cfloat* ptr_q = Q;
            v4cfloat* ptr_m = M;

            v4cfloat tmp_scale;
            tmp_scale = upd_elem(tmp_scale, 0, dotp);
            tmp_scale = upd_elem(tmp_scale, 1, dotp);
            v4cfloat scale;
            scale = upd_elem(scale, 0, norm2_inv);
            scale = upd_elem(scale, 1, norm_inv);
            scale = fpmul_cn(scale, tmp_scale);

            cfloat res_r = ext_elem(scale, 1);
            update(M, res_r, ROW + column_id);
            for (int i = 0; i < ROW / 4; i++) chess_prepare_for_pipelining {
                    v4cfloat u0 = *ptr_q++;
                    v4cfloat old_a0 = *ptr_m++;
                    v4cfloat update_old_a0 = fpmsc(old_a0, scale, 0, 0, u0, 0, 0x3210);
                    WRITEINCRW(WMS_rsrc1, out_0, as_v4int32(ext_v(update_old_a0, 0)));
                    WRITEINCRW(WMS_rsrc2, out_1, as_v4int32(ext_v(update_old_a0, 1)));
                }

            for (int i = 0; i < COL / 4; i++) chess_prepare_for_pipelining {
                    v4cfloat old_r0 = *ptr_m++;
                    WRITEINCRW(WMS_rsrc1, out_0, as_v4int32(ext_v(old_r0, 0)));
                    WRITEINCRW(WMS_rsrc2, out_1, as_v4int32(ext_v(old_r0, 1)));
                }
        }
    }
}

} // namespace solver
} // namespace xf
