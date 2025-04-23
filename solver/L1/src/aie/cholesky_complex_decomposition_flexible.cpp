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

#include "aie/cholesky_complex_decomposition_flexible.hpp"
#include "aie_api/utils.hpp"
#include "aie_api/aie_adf.hpp"

namespace xf {
namespace solver {

template <unsigned Dim, unsigned CoreNum, unsigned BlkNum>
void CholeskyComplexFlexible<Dim, CoreNum, BlkNum>::run(
    adf::input_buffer<cfloat, adf::extents<adf::inherited_extent> >& __restrict in,
    adf::output_buffer<cfloat, adf::extents<adf::inherited_extent> >& __restrict out) {
    auto inIter = aie::begin_vector<VecSize>(in);
    auto outIter = aie::begin_vector<VecSize>(out);
    unsigned int pos0 = columnId / VecSize;
    unsigned int pos1 = columnId % VecSize;
    unsigned int blkPos = curColId - columnId;

    aie::vector<cfloat, VecSize> vdata = aie::zeros<cfloat, VecSize>();
    // Transfer Datas;
    if (curColId < columnId) {
        for (int i = 0; i < vecNum; i++) chess_prepare_for_pipelining {
                *outIter++ = *inIter++;
            }
    }
    // Calculate Target Column;
    if ((curColId >= columnId) && (curColId < columnId + BlkNum)) {
        for (int i = 0; i < pos0; i++) chess_prepare_for_pipelining {
                *outIter++ = aie::zeros<cfloat, VecSize>();
            }
        inIter += pos0;
        for (int i = pos0; i < vecNum; i++) chess_prepare_for_pipelining {
                vdata = *inIter++;
                ColA[blkPos][i] = vdata;
            }
        for (int k = columnId; k < curColId; k++) chess_prepare_for_pipelining {
                updA(curColId, k, columnId);
            }
        calV(curColId, columnId);
        for (int i = pos0; i < vecNum; i++) chess_prepare_for_pipelining {
                vdata = ColA[blkPos][i];
                *outIter++ = vdata;
            }
    }
    if ((curColId >= columnId + BlkNum) && (curColId < Dim)) {
        for (int i = 0; i < pos0; i++) chess_prepare_for_pipelining {
                *outIter++ = aie::zeros<cfloat, VecSize>();
            }
        inIter += pos0;
        for (int i = pos0; i < vecNum; i++) chess_prepare_for_pipelining {
                vdata = *inIter++;
                ColA[BlkNum][i] = vdata;
            }
        for (int k = columnId; k < columnId + BlkNum; k++) chess_prepare_for_pipelining {
                updA(curColId, k, columnId);
            }
        for (int i = pos0; i < vecNum; i++) chess_prepare_for_pipelining {
                vdata = ColA[BlkNum][i];
                *outIter++ = vdata;
            }
    }
    curColId++;
}

template <unsigned Dim, unsigned CoreNum, unsigned BlkNum>
inline void CholeskyComplexFlexible<Dim, CoreNum, BlkNum>::calV(const unsigned columnId, const unsigned baseId) {
    const unsigned int pos0 = columnId / VecSize;
    const unsigned int pos1 = columnId % VecSize;
    const unsigned int blkPos = columnId - baseId;
    aie::vector<cfloat, VecSize> vdata = ColA[blkPos][pos0];
    cfloat cdiag = vdata[pos1];
    float diag = aie::sqrt(cdiag.real);
    cdiag.real = diag;
    cdiag.imag = 0;
    float invdiag = aie::inv(diag);
    cfloat cinvdiag = {invdiag, 0.0};

    vdata = aie::mul(vdata, cinvdiag);
    for (int i = 0; i < pos1; i++) chess_prepare_for_pipelining {
            vdata[i] = {0.0, 0.0};
        }
    vdata[pos1] = cdiag;
    ColA[blkPos][pos0] = vdata;
    for (int i = pos0 + 1; i < vecNum; i++) chess_prepare_for_pipelining {
            vdata = ColA[blkPos][i];
            vdata = aie::mul(vdata, cinvdiag);
            ColA[blkPos][i] = vdata;
        }
}
template <unsigned Dim, unsigned CoreNum, unsigned BlkNum>
inline void CholeskyComplexFlexible<Dim, CoreNum, BlkNum>::updA(const unsigned curColId,
                                                                const unsigned columnId,
                                                                const unsigned baseId) {
    const unsigned int pos0 = columnId / VecSize;
    const unsigned int pos1 = columnId % VecSize;
    unsigned int blkPos = columnId - baseId;
    unsigned int curPos = curColId - baseId;
    if (curPos > BlkNum) curPos = BlkNum;
    aie::vector<cfloat, VecSize> vajk = ColA[blkPos][curColId / VecSize];
    cfloat ajk = vajk[curColId % VecSize];
    aie::accum<caccfloat, VecSize> acc0 = aie::zeros<caccfloat, VecSize>();
    for (int i = pos0; i < vecNum; i++) chess_prepare_for_pipelining {
            aie::vector<cfloat, VecSize> vdata = ColA[curPos][i];
            acc0.from_vector(vdata);
            aie::vector<cfloat, VecSize> vaik = ColA[blkPos][i];
            acc0 = aie::msc(acc0, vaik, aie::conj(ajk));
            vdata = acc0.to_vector<cfloat>();
            ColA[curPos][i] = vdata;
        }
}
} // namespace solver
} // namespace xf
