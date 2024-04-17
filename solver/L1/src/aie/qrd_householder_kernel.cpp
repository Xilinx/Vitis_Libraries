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

#include "qrd_householder_kernel.hpp"
#include "aie_api/utils.hpp"
#include "aie_api/aie_adf.hpp"
#include "aie_api/adf/stream.hpp"

const float tol = 5.0e-4;
const float eta = 1.0e-3;

namespace xf {
namespace solver {

union DType {
    float f;
    int i;
};

void printcVector(cfloat vec[], int dim, int rowId, char* vecName) {
    printf("%s[%d]: [", vecName, rowId);
    for (int i = 0; i < dim; i++) {
        printf("(%f, %f)  ", vec[i].real, vec[i].imag);
    }
    printf("] \n");
}

template <int M, int N>
void qrd_householder(input_stream<cfloat>* __restrict matAU_0,
                     input_stream<cfloat>* __restrict matAU_1,
                     output_stream<cfloat>* __restrict matRQ_0,
                     output_stream<cfloat>* __restrict matRQ_1,
                     const int column_id) {
    DType tmp;
    int k = column_id;

    const int L = M / 4;
    const int len4 = k / 4;
    const int idk = k % 4;
    float norm2 = 0;
    float norm = 0;
    float dkk2 = 0;
    float dkk = 0;
    float alph = 1;
    float alphInv = 1;
    cfloat sgn = {1, 0};
    cfloat ukk = {0, 0};

    v4cfloat U[L];
    v4cfloat A[L];
    v4cfloat Q[L];
    v4cfloat __aie_dm_resource_a* __restrict p_vecU = (v4cfloat __aie_dm_resource_a*)U;
    v4cfloat __aie_dm_resource_b* __restrict p_vecA = (v4cfloat __aie_dm_resource_b*)A;
    v4cfloat __aie_dm_resource_c* __restrict p_vecQ = (v4cfloat __aie_dm_resource_c*)Q;

    aie::vector<float, 4> vr2vec = aie::zeros<float, 4>();
    aie::vector<cfloat, 4> diagAvec = aie::zeros<cfloat, 4>();
    aie::vector<cfloat, 4> diagRvec = aie::zeros<cfloat, 4>();
    aie::vector<cfloat, 4> diagUvec = aie::zeros<cfloat, 4>();
    v4cfloat diagAv = null_v4cfloat();
    v4cfloat diagRv = null_v4cfloat();
    v4cfloat diagUv = null_v4cfloat();

    // Store R[0:M-1, 0:k-1]
    for (int j = 0; j < k; j++) {
        for (int i = 0; i < M; i += 4) chess_prepare_for_pipelining {
                v2cfloat tmpv_0 = as_v2cfloat(READINCRW(WSS_rsrc1, matAU_0));
                v2cfloat tmpv_1 = as_v2cfloat(READINCRW(WSS_rsrc2, matAU_1));
                WRITEINCRW(WMS_rsrc1, matRQ_0, as_v4int32(tmpv_0));
                WRITEINCRW(WMS_rsrc2, matRQ_1, as_v4int32(tmpv_1));
            }
    }

    // calculate norm2 = ||ak|| * ||ak||
    for (int i = 0; i < len4; i++) chess_prepare_for_pipelining {
            v2cfloat tmpv_0 = as_v2cfloat(READINCRW(WSS_rsrc1, matAU_0));
            v2cfloat tmpv_1 = as_v2cfloat(READINCRW(WSS_rsrc2, matAU_1));
            WRITEINCRW(WMS_rsrc1, matRQ_0, as_v4int32(tmpv_0));
            WRITEINCRW(WMS_rsrc2, matRQ_1, as_v4int32(tmpv_1));
        }
    for (int i = len4; i < len4 + 1; i++) {
        v2cfloat tmpv_0 = as_v2cfloat(READINCRW(WSS_rsrc1, matAU_0));
        v2cfloat tmpv_1 = as_v2cfloat(READINCRW(WSS_rsrc2, matAU_1));
        diagAv = upd_v(diagAv, 0, tmpv_0);
        diagAv = upd_v(diagAv, 1, tmpv_1);
        diagAvec = diagAv;
        ukk = diagAvec[idk];
        aie::mask<4> msk = aie::mask<4>(true);
        auto msk_u = msk << idk;
        auto msk_r = msk_u;
        cfloat czero = {0, 0};
        diagUvec = aie::select(czero, diagAvec, msk_u);
        diagRvec = aie::select(diagAvec, czero, msk_r);
        diagUv = diagUvec;
        vr2vec = aie::abs_square(diagUvec);
        dkk2 = vr2vec[idk];
        norm2 = aie::reduce_add(vr2vec);
    }
    for (int i = 1 + len4; i < L; i++) chess_prepare_for_pipelining {
            v2cfloat tmpv_0 = as_v2cfloat(READINCRW(WSS_rsrc1, matAU_0));
            v2cfloat tmpv_1 = as_v2cfloat(READINCRW(WSS_rsrc2, matAU_1));
            v4cfloat ukv;
            ukv = upd_v(ukv, 0, tmpv_0);
            ukv = upd_v(ukv, 1, tmpv_1);
            p_vecU[i] = ukv;
            vr2vec = aie::abs_square(aie::vector<cfloat, 4>(ukv));
            norm2 = norm2 + aie::reduce_add(vr2vec);
        }
    if (norm2 > tol) {
        norm = aie::sqrt(norm2);
        cfloat sk;
        sk.real = norm;
        sk.imag = 0;
        diagRvec[idk] = sk;
        diagRv = diagRvec;

        for (int i = len4; i < (len4 + 1); i++) chess_prepare_for_pipelining {
                v2cfloat tmpv_0 = ext_v(diagRv, 0);
                v2cfloat tmpv_1 = ext_v(diagRv, 1);
                WRITEINCRW(WMS_rsrc1, matRQ_0, as_v4int32(tmpv_0));
                WRITEINCRW(WMS_rsrc2, matRQ_1, as_v4int32(tmpv_1));
            }
        for (int i = len4 + 1; i < L; i++) chess_prepare_for_pipelining {
                v2cfloat tmpv_0 = null_v2cfloat();
                v2cfloat tmpv_1 = null_v2cfloat();
                WRITEINCRW(WMS_rsrc1, matRQ_0, as_v4int32(tmpv_0));
                WRITEINCRW(WMS_rsrc2, matRQ_1, as_v4int32(tmpv_1));
            }

        dkk = aie::sqrt(dkk2);
        alph = norm2 + norm * dkk;
        alphInv = aie::inv(alph);

        // calculate reflect vector u and beta, A[k:M, k]
        if (dkk2 != (float)0) {
            sgn = ukk * aie::inv(dkk);
        }
        aie::vector<cfloat, 4> sgnvec = aie::broadcast<cfloat, 4>(sgn);
        float normdkk = norm + dkk;
        ukk = sgn * normdkk;
        diagUv = upd_elem(diagUv, idk, ukk);
        p_vecU[len4] = diagUv;

        // update the R[k:M, k+1:N]   // k is columnId
        cfloat h;
        v4cfloat hjv = null_v4cfloat();
        aie::vector<cfloat, 4> hjvec = hjv;
        for (int i = 0; i < len4; i++) {
            v4cfloat ajv = null_v4cfloat();
            v2cfloat tmpv_0 = as_v2cfloat(READINCRW(WSS_rsrc1, matAU_0));
            v2cfloat tmpv_1 = as_v2cfloat(READINCRW(WSS_rsrc2, matAU_1));
            ajv = upd_v(ajv, 0, tmpv_0);
            ajv = upd_v(ajv, 1, tmpv_1);
            p_vecA[i] = ajv;
        }
        for (int i = len4; i < L; i++) {
            v4cfloat ajv = null_v4cfloat();
            v2cfloat tmpv_0 = as_v2cfloat(READINCRW(WSS_rsrc1, matAU_0));
            v2cfloat tmpv_1 = as_v2cfloat(READINCRW(WSS_rsrc2, matAU_1));
            ajv = upd_v(ajv, 0, tmpv_0);
            ajv = upd_v(ajv, 1, tmpv_1);
            p_vecA[i] = ajv;
            v4cfloat ukv = p_vecU[i];
            hjv = fpmac_cn(hjv, ukv, ajv);
        }
        h = aie::reduce_add(aie::vector<cfloat, 4>(hjv));
        h = h * alphInv;
        for (int j = k + 1; j < N - 1; j++) {
            hjvec = aie::broadcast<cfloat, 4>(h);
            v4cfloat acc1 = null_v4cfloat();
            for (int i = 0; i < len4; i++) chess_prepare_for_pipelining {
                    v4cfloat ajv = p_vecA[i];
                    v2cfloat tmpv_0 = ext_v(ajv, 0);
                    v2cfloat tmpv_1 = ext_v(ajv, 1);
                    WRITEINCRW(WMS_rsrc1, matRQ_0, as_v4int32(tmpv_0));
                    WRITEINCRW(WMS_rsrc2, matRQ_1, as_v4int32(tmpv_1));

                    // calculate beta for next column;
                    v4cfloat a1jv = null_v4cfloat();
                    v2cfloat tmp1v_0 = as_v2cfloat(READINCRW(WSS_rsrc1, matAU_0));
                    v2cfloat tmp1v_1 = as_v2cfloat(READINCRW(WSS_rsrc2, matAU_1));
                    a1jv = upd_v(a1jv, 0, tmp1v_0);
                    a1jv = upd_v(a1jv, 1, tmp1v_1);
                    p_vecA[i] = a1jv;
                }
            for (int i = len4; i < len4 + 1; i++) chess_prepare_for_pipelining {
                    v4cfloat ukv = p_vecU[i];
                    v4cfloat ajv = p_vecA[i];
                    hjv = hjvec;
                    v4cfloat acc0 = fpmsc(ajv, hjv, ukv);
                    cfloat akj = ext_elem(acc0, idk);
                    cfloat bkj = -aie::conj(sgn) * akj;
                    acc0 = upd_elem(acc0, idk, bkj);
                    ajv = acc0;
                    v2cfloat tmpv_0 = ext_v(ajv, 0);
                    v2cfloat tmpv_1 = ext_v(ajv, 1);
                    WRITEINCRW(WMS_rsrc1, matRQ_0, as_v4int32(tmpv_0));
                    WRITEINCRW(WMS_rsrc2, matRQ_1, as_v4int32(tmpv_1));

                    // calculate beta for next column;
                    v4cfloat a1jv = null_v4cfloat();
                    v2cfloat tmp1v_0 = as_v2cfloat(READINCRW(WSS_rsrc1, matAU_0));
                    v2cfloat tmp1v_1 = as_v2cfloat(READINCRW(WSS_rsrc2, matAU_1));
                    a1jv = upd_v(a1jv, 0, tmp1v_0);
                    a1jv = upd_v(a1jv, 1, tmp1v_1);
                    p_vecA[i] = a1jv;
                    acc1 = fpmac_cn(acc1, ukv, a1jv);
                }
            for (int i = len4 + 1; i < L; i++) chess_prepare_for_pipelining {
                    v4cfloat ukv = p_vecU[i];
                    v4cfloat ajv = p_vecA[i];
                    hjv = hjvec;
                    v4cfloat acc0 = fpmsc(ajv, hjv, ukv);
                    ajv = acc0;
                    v2cfloat tmpv_0 = ext_v(ajv, 0);
                    v2cfloat tmpv_1 = ext_v(ajv, 1);
                    WRITEINCRW(WMS_rsrc1, matRQ_0, as_v4int32(tmpv_0));
                    WRITEINCRW(WMS_rsrc2, matRQ_1, as_v4int32(tmpv_1));

                    // calculate beta for next column;
                    v4cfloat a1jv = null_v4cfloat();
                    v2cfloat tmp1v_0 = as_v2cfloat(READINCRW(WSS_rsrc1, matAU_0));
                    v2cfloat tmp1v_1 = as_v2cfloat(READINCRW(WSS_rsrc2, matAU_1));
                    a1jv = upd_v(a1jv, 0, tmp1v_0);
                    a1jv = upd_v(a1jv, 1, tmp1v_1);
                    p_vecA[i] = a1jv;
                    acc1 = fpmac_cn(acc1, ukv, a1jv);
                }
            cfloat h1 = aie::reduce_add(aie::vector<cfloat, 4>(acc1));
            h = h1 * alphInv;
        }
        hjvec = aie::broadcast<cfloat, 4>(h);
        for (int i = 0; i < len4; i++) chess_prepare_for_pipelining {
                v4cfloat ajv = p_vecA[i];
                v2cfloat tmpv_0 = ext_v(ajv, 0);
                v2cfloat tmpv_1 = ext_v(ajv, 1);
                WRITEINCRW(WMS_rsrc1, matRQ_0, as_v4int32(tmpv_0));
                WRITEINCRW(WMS_rsrc2, matRQ_1, as_v4int32(tmpv_1));
            }
        for (int i = len4; i < len4 + 1; i++) chess_prepare_for_pipelining {
                v4cfloat ukv = p_vecU[i];
                v4cfloat ajv = p_vecA[i];
                hjv = hjvec;
                v4cfloat acc0 = fpmsc(ajv, hjv, ukv);
                cfloat akj = ext_elem(acc0, idk);
                cfloat bkj = -aie::conj(sgn) * akj;
                acc0 = upd_elem(acc0, idk, bkj);
                ajv = acc0;
                v2cfloat tmpv_0 = ext_v(ajv, 0);
                v2cfloat tmpv_1 = ext_v(ajv, 1);
                WRITEINCRW(WMS_rsrc1, matRQ_0, as_v4int32(tmpv_0));
                WRITEINCRW(WMS_rsrc2, matRQ_1, as_v4int32(tmpv_1));
            }
        for (int i = len4 + 1; i < L; i++) chess_prepare_for_pipelining {
                v4cfloat ukv = p_vecU[i];
                v4cfloat ajv = p_vecA[i];
                hjv = hjvec;
                v4cfloat acc0 = fpmsc(ajv, hjv, ukv);
                ajv = acc0;
                v2cfloat tmpv_0 = ext_v(ajv, 0);
                v2cfloat tmpv_1 = ext_v(ajv, 1);
                WRITEINCRW(WMS_rsrc1, matRQ_0, as_v4int32(tmpv_0));
                WRITEINCRW(WMS_rsrc2, matRQ_1, as_v4int32(tmpv_1));
            }

        // update the Q[k:M, 0:M]   // k is columnId
        cfloat s;
        v4cfloat sjv = null_v4cfloat();
        aie::vector<cfloat, 4> sjvec = sjv;
        for (int i = 0; i < len4; i++) chess_prepare_for_pipelining {
                v4cfloat qjv = null_v4cfloat();
                v2cfloat tmpv_0 = as_v2cfloat(READINCRW(WSS_rsrc1, matAU_0));
                v2cfloat tmpv_1 = as_v2cfloat(READINCRW(WSS_rsrc2, matAU_1));
                qjv = upd_v(qjv, 0, tmpv_0);
                qjv = upd_v(qjv, 1, tmpv_1);
                p_vecQ[i] = qjv;
            }
        for (int i = len4; i < L; i++) chess_prepare_for_pipelining {
                v4cfloat qjv = null_v4cfloat();
                v2cfloat tmpv_0 = as_v2cfloat(READINCRW(WSS_rsrc1, matAU_0));
                v2cfloat tmpv_1 = as_v2cfloat(READINCRW(WSS_rsrc2, matAU_1));
                qjv = upd_v(qjv, 0, tmpv_0);
                qjv = upd_v(qjv, 1, tmpv_1);
                p_vecQ[i] = qjv;
                v4cfloat ukv = p_vecU[i];
                sjv = fpmac_cn(sjv, ukv, qjv);
            }
        s = aie::reduce_add(aie::vector<cfloat, 4>(sjv));
        s = s * alphInv;
        for (int j = 0; j < M - 1; j++) {
            sjvec = aie::broadcast<cfloat, 4>(s);
            v4cfloat qacc1 = null_v4cfloat();
            for (int i = 0; i < len4; i++) chess_prepare_for_pipelining {
                    v4cfloat qjv = p_vecQ[i];
                    v2cfloat tmpv_0 = ext_v(qjv, 0);
                    v2cfloat tmpv_1 = ext_v(qjv, 1);
                    WRITEINCRW(WMS_rsrc1, matRQ_0, as_v4int32(tmpv_0));
                    WRITEINCRW(WMS_rsrc2, matRQ_1, as_v4int32(tmpv_1));

                    // calculate beta for next column
                    v4cfloat q1jv = null_v4cfloat();
                    v2cfloat tmp1v_0 = as_v2cfloat(READINCRW(WSS_rsrc1, matAU_0));
                    v2cfloat tmp1v_1 = as_v2cfloat(READINCRW(WSS_rsrc2, matAU_1));
                    q1jv = upd_v(q1jv, 0, tmp1v_0);
                    q1jv = upd_v(q1jv, 1, tmp1v_1);
                    p_vecQ[i] = q1jv;
                }
            for (int i = len4; i < len4 + 1; i++) chess_prepare_for_pipelining {
                    v4cfloat ukv = p_vecU[i];
                    v4cfloat qjv = p_vecQ[i];
                    sjv = sjvec;
                    v4cfloat qacc0 = fpmsc(qjv, sjv, ukv);
                    cfloat qkj = ext_elem(qacc0, idk);
                    cfloat tkj = -aie::conj(sgn) * qkj;
                    qacc0 = upd_elem(qacc0, idk, tkj);
                    qjv = qacc0;
                    v2cfloat tmpv_0 = ext_v(qjv, 0);
                    v2cfloat tmpv_1 = ext_v(qjv, 1);
                    WRITEINCRW(WMS_rsrc1, matRQ_0, as_v4int32(tmpv_0));
                    WRITEINCRW(WMS_rsrc2, matRQ_1, as_v4int32(tmpv_1));

                    // calculate beta for next column
                    v4cfloat q1jv = null_v4cfloat();
                    v2cfloat tmp1v_0 = as_v2cfloat(READINCRW(WSS_rsrc1, matAU_0));
                    v2cfloat tmp1v_1 = as_v2cfloat(READINCRW(WSS_rsrc2, matAU_1));
                    q1jv = upd_v(q1jv, 0, tmp1v_0);
                    q1jv = upd_v(q1jv, 1, tmp1v_1);
                    p_vecQ[i] = q1jv;
                    qacc1 = fpmac_cn(qacc1, ukv, q1jv);
                }
            for (int i = len4 + 1; i < L; i++) chess_prepare_for_pipelining {
                    v4cfloat ukv = p_vecU[i];
                    v4cfloat qjv = p_vecQ[i];
                    sjv = sjvec;
                    v4cfloat qacc0 = fpmsc(qjv, sjv, ukv);
                    qjv = qacc0;
                    v2cfloat tmpv_0 = ext_v(qjv, 0);
                    v2cfloat tmpv_1 = ext_v(qjv, 1);
                    WRITEINCRW(WMS_rsrc1, matRQ_0, as_v4int32(tmpv_0));
                    WRITEINCRW(WMS_rsrc2, matRQ_1, as_v4int32(tmpv_1));

                    // calculate beta for next column
                    v4cfloat q1jv = null_v4cfloat();
                    v2cfloat tmp1v_0 = as_v2cfloat(READINCRW(WSS_rsrc1, matAU_0));
                    v2cfloat tmp1v_1 = as_v2cfloat(READINCRW(WSS_rsrc2, matAU_1));
                    q1jv = upd_v(q1jv, 0, tmp1v_0);
                    q1jv = upd_v(q1jv, 1, tmp1v_1);
                    p_vecQ[i] = q1jv;
                    qacc1 = fpmac_cn(qacc1, ukv, q1jv);
                }
            cfloat s1 = aie::reduce_add(aie::vector<cfloat, 4>(qacc1));
            s = s1 * alphInv;
        }
        sjvec = aie::broadcast<cfloat, 4>(s);
        for (int i = 0; i < len4; i++) chess_prepare_for_pipelining {
                v4cfloat qjv = p_vecQ[i];
                v2cfloat tmpv_0 = ext_v(qjv, 0);
                v2cfloat tmpv_1 = ext_v(qjv, 1);
                WRITEINCRW(WMS_rsrc1, matRQ_0, as_v4int32(tmpv_0));
                WRITEINCRW(WMS_rsrc2, matRQ_1, as_v4int32(tmpv_1));
            }
        for (int i = len4; i < len4 + 1; i++) chess_prepare_for_pipelining {
                v4cfloat ukv = p_vecU[i];
                v4cfloat qjv = p_vecQ[i];
                sjv = sjvec;
                v4cfloat qacc0 = fpmsc(qjv, sjv, ukv);
                cfloat qkj = ext_elem(qacc0, idk);
                cfloat tkj = -aie::conj(sgn) * qkj;
                qacc0 = upd_elem(qacc0, idk, tkj);
                qjv = qacc0;
                v2cfloat tmpv_0 = ext_v(qjv, 0);
                v2cfloat tmpv_1 = ext_v(qjv, 1);
                WRITEINCRW(WMS_rsrc1, matRQ_0, as_v4int32(tmpv_0));
                WRITEINCRW(WMS_rsrc2, matRQ_1, as_v4int32(tmpv_1));
            }
        for (int i = len4 + 1; i < L; i++) chess_prepare_for_pipelining {
                v4cfloat ukv = p_vecU[i];
                v4cfloat qjv = p_vecQ[i];
                sjv = sjvec;
                v4cfloat qacc0 = fpmsc(qjv, sjv, ukv);
                qjv = qacc0;
                v2cfloat tmpv_0 = ext_v(qjv, 0);
                v2cfloat tmpv_1 = ext_v(qjv, 1);
                WRITEINCRW(WMS_rsrc1, matRQ_0, as_v4int32(tmpv_0));
                WRITEINCRW(WMS_rsrc2, matRQ_1, as_v4int32(tmpv_1));
            }
    } else {
        // store R[k:M-1, k]
        for (int i = k; i < M; i++) {
            WRITEINCR(MS_rsrc1, matRQ_0, 0);
            WRITEINCR(MS_rsrc2, matRQ_1, 0);
        }
        for (int j = k + 1; j < N; j++) {
            for (int i = 0; i < M; i += 4) chess_prepare_for_pipelining {
                    v2cfloat tmpv_0 = as_v2cfloat(READINCRW(WSS_rsrc1, matAU_0));
                    v2cfloat tmpv_1 = as_v2cfloat(READINCRW(WSS_rsrc2, matAU_1));
                    WRITEINCRW(WMS_rsrc1, matRQ_0, as_v4int32(tmpv_0));
                    WRITEINCRW(WMS_rsrc2, matRQ_1, as_v4int32(tmpv_1));
                }
        }
        for (int j = 0; j < M; j++) {
            for (int i = 0; i < M; i += 4) chess_prepare_for_pipelining {
                    v2cfloat tmpv_0 = as_v2cfloat(READINCRW(WSS_rsrc1, matAU_0));
                    v2cfloat tmpv_1 = as_v2cfloat(READINCRW(WSS_rsrc2, matAU_1));
                    WRITEINCRW(WMS_rsrc1, matRQ_0, as_v4int32(tmpv_0));
                    WRITEINCRW(WMS_rsrc2, matRQ_1, as_v4int32(tmpv_1));
                }
        }
    }
} // end qrd_householder

template <int M, int N>
void qrd_householder_last(input_stream<cfloat>* __restrict matAU_0,
                          input_stream<cfloat>* __restrict matAU_1,
                          output_stream<cfloat>* __restrict matRQ_0,
                          output_stream<cfloat>* __restrict matRQ_1,
                          const int column_id) {
    DType tmp;
    int k = column_id;

    const int L = M / 4;
    const int len4 = k / 4;
    const int idk = k % 4;
    float norm2 = 0;
    float norm = 0;
    float dkk2 = 0;
    float dkk = 0;
    float alph = 1;
    float alphInv = 1;
    cfloat sgn = {1, 0};
    cfloat ukk = {0, 0};

    v4cfloat U[L];
    v4cfloat Q[L];
    v4cfloat* __restrict p_vecU = chess_copy((v4cfloat*)U);
    v4cfloat* __restrict p_vecQ = chess_copy((v4cfloat*)Q);

    aie::vector<float, 4> vr2vec = aie::zeros<float, 4>();
    aie::vector<cfloat, 4> diagAvec = aie::zeros<cfloat, 4>();
    aie::vector<cfloat, 4> diagRvec = aie::zeros<cfloat, 4>();
    aie::vector<cfloat, 4> diagUvec = aie::zeros<cfloat, 4>();
    v4cfloat diagAv = null_v4cfloat();
    v4cfloat diagRv = null_v4cfloat();
    v4cfloat diagUv = null_v4cfloat();

    // Store R[0:M-1, 0:k-1]
    for (int j = 0; j < k; j++) {
        for (int i = 0; i < M; i += 4) chess_prepare_for_pipelining {
                v2cfloat tmpv_0 = as_v2cfloat(READINCRW(WSS_rsrc1, matAU_0));
                v2cfloat tmpv_1 = as_v2cfloat(READINCRW(WSS_rsrc2, matAU_1));
                WRITEINCRW(WMS_rsrc1, matRQ_0, as_v4int32(tmpv_0));
                WRITEINCRW(WMS_rsrc2, matRQ_1, as_v4int32(tmpv_1));
            }
    }

    // calculate norm2 = ||ak|| * ||ak||
    for (int i = 0; i < len4; i++) chess_prepare_for_pipelining {
            v2cfloat tmpv_0 = as_v2cfloat(READINCRW(WSS_rsrc1, matAU_0));
            v2cfloat tmpv_1 = as_v2cfloat(READINCRW(WSS_rsrc2, matAU_1));
            WRITEINCRW(WMS_rsrc1, matRQ_0, as_v4int32(tmpv_0));
            WRITEINCRW(WMS_rsrc2, matRQ_1, as_v4int32(tmpv_1));
        }
    for (int i = len4; i < len4 + 1; i++) {
        v2cfloat tmpv_0 = as_v2cfloat(READINCRW(WSS_rsrc1, matAU_0));
        v2cfloat tmpv_1 = as_v2cfloat(READINCRW(WSS_rsrc2, matAU_1));
        diagAv = upd_v(diagAv, 0, tmpv_0);
        diagAv = upd_v(diagAv, 1, tmpv_1);
        diagAvec = diagAv;
        ukk = diagAvec[idk];
        aie::mask<4> msk = aie::mask<4>(true);
        auto msk_u = msk << idk;
        cfloat czero = {0, 0};
        diagUvec = aie::select(czero, diagAvec, msk_u);
        diagRvec = aie::select(diagAvec, czero, msk_u);
        diagUv = diagUvec;
        vr2vec = aie::abs_square(diagUvec);
        dkk2 = vr2vec[idk];
        norm2 = aie::reduce_add(vr2vec);
    }
    for (int i = 1 + len4; i < L; i++) chess_prepare_for_pipelining {
            v2cfloat tmpv_0 = as_v2cfloat(READINCRW(WSS_rsrc1, matAU_0));
            v2cfloat tmpv_1 = as_v2cfloat(READINCRW(WSS_rsrc2, matAU_1));
            v4cfloat ukv;
            ukv = upd_v(ukv, 0, tmpv_0);
            ukv = upd_v(ukv, 1, tmpv_1);
            p_vecU[i] = ukv;
            vr2vec = aie::abs_square(aie::vector<cfloat, 4>(ukv));
            norm2 = norm2 + aie::reduce_add(vr2vec);
        }
    if (norm2 > tol) {
        norm = aie::sqrt(norm2);
        cfloat sk;
        sk.real = norm;
        sk.imag = 0;
        diagRvec[idk] = sk;
        diagRv = diagRvec;

        for (int i = len4; i < (len4 + 1); i++) chess_prepare_for_pipelining {
                v2cfloat tmpv_0 = ext_v(diagRv, 0);
                v2cfloat tmpv_1 = ext_v(diagRv, 1);
                WRITEINCRW(WMS_rsrc1, matRQ_0, as_v4int32(tmpv_0));
                WRITEINCRW(WMS_rsrc2, matRQ_1, as_v4int32(tmpv_1));
            }
        for (int i = len4 + 1; i < L; i++) chess_prepare_for_pipelining {
                v2cfloat tmpv_0 = null_v2cfloat();
                v2cfloat tmpv_1 = null_v2cfloat();
                WRITEINCRW(WMS_rsrc1, matRQ_0, as_v4int32(tmpv_0));
                WRITEINCRW(WMS_rsrc2, matRQ_1, as_v4int32(tmpv_1));
            }

        dkk = aie::sqrt(dkk2);
        alph = norm2 + norm * dkk;
        alphInv = aie::inv(alph);

        // calculate reflect vector u and beta, A[k:M, k]
        if (dkk2 != (float)0) {
            sgn = ukk * aie::inv(dkk);
        }
        aie::vector<cfloat, 4> sgnvec = aie::broadcast<cfloat, 4>(sgn);
        float normdkk = norm + dkk;
        ukk = sgn * normdkk;
        diagUv = upd_elem(diagUv, idk, ukk);
        p_vecU[len4] = diagUv;

        // update the Q[k:M, 0:M]   // k is columnId
        cfloat s;
        v4cfloat sjv = null_v4cfloat();
        aie::vector<cfloat, 4> sjvec = sjv;
        for (int i = 0; i < len4; i++) chess_prepare_for_pipelining {
                v4cfloat qjv = null_v4cfloat();
                v2cfloat tmpv_0 = as_v2cfloat(READINCRW(WSS_rsrc1, matAU_0));
                v2cfloat tmpv_1 = as_v2cfloat(READINCRW(WSS_rsrc2, matAU_1));
                qjv = upd_v(qjv, 0, tmpv_0);
                qjv = upd_v(qjv, 1, tmpv_1);
                p_vecQ[i] = qjv;
            }
        for (int i = len4; i < L; i++) chess_prepare_for_pipelining {
                v4cfloat qjv = null_v4cfloat();
                v2cfloat tmpv_0 = as_v2cfloat(READINCRW(WSS_rsrc1, matAU_0));
                v2cfloat tmpv_1 = as_v2cfloat(READINCRW(WSS_rsrc2, matAU_1));
                qjv = upd_v(qjv, 0, tmpv_0);
                qjv = upd_v(qjv, 1, tmpv_1);
                p_vecQ[i] = qjv;
                v4cfloat ukv = p_vecU[i];
                sjv = fpmac_cn(sjv, ukv, qjv);
            }
        s = aie::reduce_add(aie::vector<cfloat, 4>(sjv));
        s = s * alphInv;
        for (int j = 0; j < M - 1; j++) {
            sjvec = aie::broadcast<cfloat, 4>(s);
            v4cfloat qacc1 = null_v4cfloat();
            for (int i = 0; i < len4; i++) chess_prepare_for_pipelining {
                    v4cfloat qjv = p_vecQ[i];
                    v2cfloat tmpv_0 = ext_v(qjv, 0);
                    v2cfloat tmpv_1 = ext_v(qjv, 1);
                    WRITEINCRW(WMS_rsrc1, matRQ_0, as_v4int32(tmpv_0));
                    WRITEINCRW(WMS_rsrc2, matRQ_1, as_v4int32(tmpv_1));

                    // calculate beta for next column
                    v4cfloat q1jv = null_v4cfloat();
                    v2cfloat tmp1v_0 = as_v2cfloat(READINCRW(WSS_rsrc1, matAU_0));
                    v2cfloat tmp1v_1 = as_v2cfloat(READINCRW(WSS_rsrc2, matAU_1));
                    q1jv = upd_v(q1jv, 0, tmp1v_0);
                    q1jv = upd_v(q1jv, 1, tmp1v_1);
                    p_vecQ[i] = q1jv;
                }
            for (int i = len4; i < len4 + 1; i++) chess_prepare_for_pipelining {
                    v4cfloat ukv = p_vecU[i];
                    v4cfloat qjv = p_vecQ[i];
                    sjv = sjvec;
                    v4cfloat qacc0 = fpmsc(qjv, sjv, ukv);
                    cfloat qkj = ext_elem(qacc0, idk);
                    cfloat tkj = -aie::conj(sgn) * qkj;
                    qacc0 = upd_elem(qacc0, idk, tkj);
                    qjv = qacc0;
                    v2cfloat tmpv_0 = ext_v(qjv, 0);
                    v2cfloat tmpv_1 = ext_v(qjv, 1);
                    WRITEINCRW(WMS_rsrc1, matRQ_0, as_v4int32(tmpv_0));
                    WRITEINCRW(WMS_rsrc2, matRQ_1, as_v4int32(tmpv_1));

                    // calculate beta for next column
                    v4cfloat q1jv = null_v4cfloat();
                    v2cfloat tmp1v_0 = as_v2cfloat(READINCRW(WSS_rsrc1, matAU_0));
                    v2cfloat tmp1v_1 = as_v2cfloat(READINCRW(WSS_rsrc2, matAU_1));
                    q1jv = upd_v(q1jv, 0, tmp1v_0);
                    q1jv = upd_v(q1jv, 1, tmp1v_1);
                    p_vecQ[i] = q1jv;
                    qacc1 = fpmac_cn(qacc1, ukv, q1jv);
                }
            for (int i = len4 + 1; i < L; i++) chess_prepare_for_pipelining {
                    v4cfloat ukv = p_vecU[i];
                    v4cfloat qjv = p_vecQ[i];
                    sjv = sjvec;
                    v4cfloat qacc0 = fpmsc(qjv, sjv, ukv);
                    qjv = qacc0;
                    v2cfloat tmpv_0 = ext_v(qjv, 0);
                    v2cfloat tmpv_1 = ext_v(qjv, 1);
                    WRITEINCRW(WMS_rsrc1, matRQ_0, as_v4int32(tmpv_0));
                    WRITEINCRW(WMS_rsrc2, matRQ_1, as_v4int32(tmpv_1));

                    // calculate beta for next column
                    v4cfloat q1jv = null_v4cfloat();
                    v2cfloat tmp1v_0 = as_v2cfloat(READINCRW(WSS_rsrc1, matAU_0));
                    v2cfloat tmp1v_1 = as_v2cfloat(READINCRW(WSS_rsrc2, matAU_1));
                    q1jv = upd_v(q1jv, 0, tmp1v_0);
                    q1jv = upd_v(q1jv, 1, tmp1v_1);
                    p_vecQ[i] = q1jv;
                    qacc1 = fpmac_cn(qacc1, ukv, q1jv);
                }
            cfloat s1 = aie::reduce_add(aie::vector<cfloat, 4>(qacc1));
            s = s1 * alphInv;
        }
        sjvec = aie::broadcast<cfloat, 4>(s);
        for (int i = 0; i < len4; i++) chess_prepare_for_pipelining {
                v4cfloat qjv = p_vecQ[i];
                v2cfloat tmpv_0 = ext_v(qjv, 0);
                v2cfloat tmpv_1 = ext_v(qjv, 1);
                WRITEINCRW(WMS_rsrc1, matRQ_0, as_v4int32(tmpv_0));
                WRITEINCRW(WMS_rsrc2, matRQ_1, as_v4int32(tmpv_1));
            }
        for (int i = len4; i < len4 + 1; i++) chess_prepare_for_pipelining {
                v4cfloat ukv = p_vecU[i];
                v4cfloat qjv = p_vecQ[i];
                sjv = sjvec;
                v4cfloat qacc0 = fpmsc(qjv, sjv, ukv);
                cfloat qkj = ext_elem(qacc0, idk);
                cfloat tkj = -aie::conj(sgn) * qkj;
                qacc0 = upd_elem(qacc0, idk, tkj);
                qjv = qacc0;
                v2cfloat tmpv_0 = ext_v(qjv, 0);
                v2cfloat tmpv_1 = ext_v(qjv, 1);
                WRITEINCRW(WMS_rsrc1, matRQ_0, as_v4int32(tmpv_0));
                WRITEINCRW(WMS_rsrc2, matRQ_1, as_v4int32(tmpv_1));
            }
        for (int i = len4 + 1; i < L; i++) chess_prepare_for_pipelining {
                v4cfloat ukv = p_vecU[i];
                v4cfloat qjv = p_vecQ[i];
                sjv = sjvec;
                v4cfloat qacc0 = fpmsc(qjv, sjv, ukv);
                qjv = qacc0;
                v2cfloat tmpv_0 = ext_v(qjv, 0);
                v2cfloat tmpv_1 = ext_v(qjv, 1);
                WRITEINCRW(WMS_rsrc1, matRQ_0, as_v4int32(tmpv_0));
                WRITEINCRW(WMS_rsrc2, matRQ_1, as_v4int32(tmpv_1));
            }
    } else {
        // store R[k:M-1, k]
        for (int i = k; i < M; i++) {
            WRITEINCR(MS_rsrc1, matRQ_0, 0);
            WRITEINCR(MS_rsrc2, matRQ_1, 0);
        }
        for (int j = k + 1; j < N; j++) {
            for (int i = 0; i < M; i += 4) chess_prepare_for_pipelining {
                    v2cfloat tmpv_0 = as_v2cfloat(READINCRW(WSS_rsrc1, matAU_0));
                    v2cfloat tmpv_1 = as_v2cfloat(READINCRW(WSS_rsrc2, matAU_1));
                    WRITEINCRW(WMS_rsrc1, matRQ_0, as_v4int32(tmpv_0));
                    WRITEINCRW(WMS_rsrc2, matRQ_1, as_v4int32(tmpv_1));
                }
        }
        for (int j = 0; j < M; j++) {
            for (int i = 0; i < M; i += 4) chess_prepare_for_pipelining {
                    v2cfloat tmpv_0 = as_v2cfloat(READINCRW(WSS_rsrc1, matAU_0));
                    v2cfloat tmpv_1 = as_v2cfloat(READINCRW(WSS_rsrc2, matAU_1));
                    WRITEINCRW(WMS_rsrc1, matRQ_0, as_v4int32(tmpv_0));
                    WRITEINCRW(WMS_rsrc2, matRQ_1, as_v4int32(tmpv_1));
                }
        }
    }
} // end qrd_householder_end

} // namespace solver
} // namespace xf
