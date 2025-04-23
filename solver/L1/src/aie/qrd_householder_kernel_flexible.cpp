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

#include "qrd_householder_kernel_flexible.hpp"
#include "aie_api/utils.hpp"
#include "aie_api/aie_adf.hpp"

namespace xf {
namespace solver {

void printcVector(cfloat vec[], int dim, int rowId, char* vecName) {
    printf("%s[%d]: [", vecName, rowId);
    for (int i = 0; i < dim; i++) {
        printf("(%f, %f)  ", vec[i].real, vec[i].imag);
    }
    printf("] \n");
}

template <int ROW, int COL, int CoreNum, int BlkNum>
void QRDHouseholderComplexFlexible<ROW, COL, CoreNum, BlkNum>::run(
    adf::input_buffer<cfloat, adf::extents<adf::inherited_extent> >& __restrict in,
    adf::output_buffer<cfloat, adf::extents<adf::inherited_extent> >& __restrict out) {
    auto inIter = aie::begin_vector<VecSize, aie_dm_resource::a>(in);
    auto outIter = aie::begin_vector<VecSize, aie_dm_resource::b>(out);

    unsigned int pos0 = columnId / VecSize;
    unsigned int pos1 = columnId % VecSize;
    unsigned int blkPos = curColId - columnId;

    // Transfer Datas: Store R[0:ROW-1, 0:k-1]
    if (curColId < columnId) {
        for (int i = 0; i < vecNum; i++) chess_prepare_for_pipelining {
                *outIter++ = *inIter++;
            }
    }
    // calculate norm2 = ||ak|| * ||ak||
    if ((curColId >= columnId) && (curColId < columnId + BlkNum)) {
        for (int i = 0; i < pos0; i++) chess_prepare_for_pipelining {
                *outIter++ = *inIter++;
            }
        for (int i = pos0; i < vecNum; i++) chess_prepare_for_pipelining {
                A[blkPos][i] = *inIter++;
            }
        for (int k = columnId; k < curColId; k++) chess_prepare_for_pipelining {
                updA(curColId, k, columnId);
            }
        calV(curColId, columnId);
        for (int i = pos0; i < vecNum; i++) {
            aie::vector<cfloat, VecSize> ajvec = A[blkPos][i];
            *outIter++ = ajvec;
        }
    }
    if ((curColId >= columnId + BlkNum) && (curColId < COL)) {
        for (int i = 0; i < pos0; i++) {
            *outIter++ = *inIter++;
        }
        for (int i = pos0; i < vecNum; i++) {
            aie::vector<cfloat, VecSize> ajvec = *inIter++;
            A[BlkNum][i] = ajvec;
        }
        for (int k = columnId; k < columnId + BlkNum; k++) chess_prepare_for_pipelining {
                updA(curColId, k, columnId);
            }
        for (int i = pos0; i < vecNum; i++) {
            aie::vector<cfloat, VecSize> ajvec = A[BlkNum][i];
            *outIter++ = ajvec;
        }
    }
    if ((curColId >= COL) && (curColId < (COL + ROW))) {
        // update the Q[k:ROW, 0:ROW]   // k is columnId
        for (int i = 0; i < pos0; i++) chess_prepare_for_pipelining {
                *outIter++ = *inIter++;
            }
        for (int i = pos0; i < vecNum; i++) chess_prepare_for_pipelining {
                aie::vector<cfloat, VecSize> qjvec = *inIter++;
                A[BlkNum][i] = qjvec;
            }
        for (int k = columnId; k < columnId + BlkNum; k++) chess_prepare_for_pipelining {
                updA(curColId, k, columnId);
            }
        for (int i = pos0; i < vecNum; i++) chess_prepare_for_pipelining {
                aie::vector<cfloat, VecSize> qjvec = A[BlkNum][i];
                *outIter++ = qjvec;
            }
    }
    curColId++;
} // end qrd_householder

template <int ROW, int COL, int CoreNum, int BlkNum>
inline void QRDHouseholderComplexFlexible<ROW, COL, CoreNum, BlkNum>::calV(const unsigned columnId,
                                                                           const unsigned baseId) {
    const unsigned int pos0 = columnId / VecSize;
    const unsigned int pos1 = columnId % VecSize;
    const unsigned int blkPos = columnId - baseId;
    float norm = 0;
    float alph = 1;
    float dkk = 0;
    float normdkk;
    cfloat sk;
    cfloat czero = {0, 0};

    aie::vector<cfloat, VecSize> diagAvec = A[blkPos][pos0];
    cfloat ukk = diagAvec[pos1];
    aie::mask<VecSize> msk = aie::mask<VecSize>(true);
    auto msk_u = msk << pos1;
    auto msk_r = msk_u;
    aie::vector<cfloat, VecSize> diagUvec = aie::select(czero, diagAvec, msk_u);
    aie::vector<cfloat, VecSize> diagRvec = aie::select(diagAvec, czero, msk_r);
    aie::vector<float, VecSize> vr2vec = aie::abs_square(diagUvec);
    float dkk2 = vr2vec[pos1];
    norm2[blkPos] = aie::reduce_add(vr2vec);
    for (int i = 1 + pos0; i < vecNum; i++) chess_prepare_for_pipelining {
            vr2vec = aie::abs_square(A[blkPos][i]);
            norm2[blkPos] = norm2[blkPos] + aie::reduce_add(vr2vec);
            U[blkPos][i] = A[blkPos][i];
            A[blkPos][i] = aie::zeros<cfloat, VecSize>();
        }

    if (norm2[blkPos] > tol) {
        norm = aie::sqrt(norm2[blkPos]);
        sk.real = norm;
        sk.imag = 0;
        diagRvec[pos1] = sk;

        dkk = aie::sqrt(dkk2);
        alph = norm2[blkPos] + norm * dkk;
        alphInv[blkPos] = aie::inv(alph);

        // calculate reflect vector u and beta, A[k:ROW, k]
        if (dkk2 != (float)0) {
            sgn[blkPos] = ukk * aie::inv(dkk);
        }
        aie::vector<cfloat, VecSize> sgnvec = aie::broadcast<cfloat, VecSize>(sgn[blkPos]);
        normdkk = norm + dkk;
        ukk = sgn[blkPos] * normdkk;
        diagUvec[pos1] = ukk;
        U[blkPos][pos0] = diagUvec;

    } else {
        diagRvec[pos1] = {0.0, 0.0};
    }
    A[blkPos][pos0] = diagRvec;
}

template <int ROW, int COL, int CoreNum, int BlkNum>
inline void QRDHouseholderComplexFlexible<ROW, COL, CoreNum, BlkNum>::updA(const unsigned curColId,
                                                                           const unsigned columnId,
                                                                           const unsigned baseId) {
    const unsigned int pos0 = columnId / VecSize;
    const unsigned int pos1 = columnId % VecSize;
    unsigned int blkPos = columnId - baseId;
    unsigned int curPos = curColId - baseId;
    if (curPos > BlkNum) curPos = BlkNum;
    if (norm2[blkPos] > tol) {
        aie::accum<caccfloat, VecSize> acc0 = aie::zeros<caccfloat, VecSize>();
        aie::vector<cfloat, VecSize> hjvec = aie::zeros<cfloat, VecSize>();
        for (int i = pos0; i < vecNum; i++) {
            aie::vector<cfloat, VecSize> ajvec = A[curPos][i];
            aie::vector<cfloat, VecSize> ukvec = U[blkPos][i];
            acc0 = aie::mac(acc0, ajvec, aie::conj(ukvec));
        }
        hjvec = acc0.to_vector<cfloat>();
        cfloat h = aie::reduce_add(hjvec);
        h = h * alphInv[blkPos];
        hjvec = aie::broadcast<cfloat, VecSize>(h);
        for (int i = pos0; i < pos0 + 1; i++) chess_prepare_for_pipelining {
                aie::vector<cfloat, VecSize> ukvec = U[blkPos][i];
                aie::vector<cfloat, VecSize> ajvec = A[curPos][i];
                acc0.from_vector(ajvec);
                acc0 = aie::msc(acc0, hjvec, ukvec);
                ajvec = acc0.to_vector<cfloat>();
                cfloat akj = ajvec[pos1];
                cfloat bkj = -aie::conj(sgn[blkPos]) * akj;
                ajvec[pos1] = bkj;
                A[curPos][i] = ajvec;
            }
        for (int i = pos0 + 1; i < vecNum; i++) chess_prepare_for_pipelining {
                aie::vector<cfloat, VecSize> ukvec = U[blkPos][i];
                aie::vector<cfloat, VecSize> ajvec = A[curPos][i];
                acc0.from_vector(ajvec);
                acc0 = aie::msc(acc0, hjvec, ukvec);
                ajvec = acc0.to_vector<cfloat>();
                A[curPos][i] = ajvec;
            }
    }
}
} // namespace solver
} // namespace xf
