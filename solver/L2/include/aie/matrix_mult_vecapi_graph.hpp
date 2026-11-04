/*
 * Copyright (C) 2026, Advanced Micro Devices, Inc.
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
#ifndef _SOLVERLIB_MATRIX_MULT_VECAPI_GRAPH_HPP_
#define _SOLVERLIB_MATRIX_MULT_VECAPI_GRAPH_HPP_

/*
L2 wrapper around the matrix_mult_vecapi L1 kernel (see matrix_mult_vecapi.hpp
for the algorithm). This graph exposes the same external port shape as
xf::dsp's matrix_mult_graph when used single-kernel (inA[1], inB[1], out[1]),
so mp_pseudoinverse_svd_graph can select between the two graph types with a
std::conditional keyed on architecture, without touching any of its
connect()-based wiring.

The template parameter list intentionally mirrors matrix_mult_graph's
signature (same names, order, and defaults) so that the exact same argument
list, as already written for MatMul1_t/MatMul2_t in mp_pseudoinverse_svd_graph.hpp,
works for either graph type. Parameters that this fallback graph doesn't need
(TP_SHIFT, tiling flags, TP_CASC_LEN, TP_SAT, TP_SSR) are accepted purely for
signature compatibility and asserted to their only supported value.
*/

#include <adf.h>
#include <type_traits>
#include "matrix_mult_vecapi.hpp"

using namespace adf;

#ifndef ROW_MAJOR
#define ROW_MAJOR 0
#endif // ROW_MAJOR
#ifndef COL_MAJOR
#define COL_MAJOR 1
#endif // COL_MAJOR

namespace xf {
namespace solver {
namespace aie {
namespace mp_pseudoinverse_svd {

template <typename TT_DATA_A,
          typename TT_DATA_B,
          unsigned int TP_DIM_A,
          unsigned int TP_DIM_AB,
          unsigned int TP_DIM_B,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_DIM_A_LEADING = ROW_MAJOR,
          unsigned int TP_DIM_B_LEADING = COL_MAJOR,
          unsigned int TP_DIM_OUT_LEADING = ROW_MAJOR,
          unsigned int TP_ADD_TILING_A = 1,
          unsigned int TP_ADD_TILING_B = 1,
          unsigned int TP_ADD_DETILING_OUT = 1,
          unsigned int TP_INPUT_WINDOW_VSIZE_A = TP_DIM_A* TP_DIM_AB,
          unsigned int TP_INPUT_WINDOW_VSIZE_B = TP_DIM_B* TP_DIM_AB,
          unsigned int TP_CASC_LEN = 1,
          unsigned int TP_SAT = 1,
          unsigned int TP_SSR = 1,
          typename TT_OUT_DATA = TT_DATA_A, // matrix_mult_vecapi requires TT_DATA_A==TT_DATA_B, so this is exact
          unsigned int TP_CONJ_A = 0,
          unsigned int TP_CONJ_B = 0>
class matrix_mult_vecapi_graph : public graph {
   public:
    static_assert(TP_DIM_B_LEADING == ROW_MAJOR,
                  "ERROR: matrix_mult_vecapi_graph requires B row-major (TP_DIM_B_LEADING=ROW_MAJOR); "
                  "the vector-API kernel reads B[k,:] as a contiguous vector.");
    static_assert(TP_SHIFT == 0, "ERROR: TP_SHIFT is not supported for float/cfloat.");
    static_assert(TP_CASC_LEN == 1, "ERROR: matrix_mult_vecapi_graph supports TP_CASC_LEN=1 only.");
    static_assert(TP_SSR == 1, "ERROR: matrix_mult_vecapi_graph supports TP_SSR=1 only.");
    static_assert(TP_INPUT_WINDOW_VSIZE_A == TP_DIM_A * TP_DIM_AB,
                  "ERROR: TP_INPUT_WINDOW_VSIZE_A must equal TP_DIM_A*TP_DIM_AB (no batching support).");
    static_assert(TP_INPUT_WINDOW_VSIZE_B == TP_DIM_B * TP_DIM_AB,
                  "ERROR: TP_INPUT_WINDOW_VSIZE_B must equal TP_DIM_B*TP_DIM_AB (no batching support).");

    using MatMultKernel_t = matrix_mult_vecapi<TT_DATA_A,
                                               TT_DATA_B,
                                               TT_OUT_DATA,
                                               TP_DIM_A,
                                               TP_DIM_AB,
                                               TP_DIM_B,
                                               TP_DIM_A_LEADING,
                                               TP_DIM_OUT_LEADING,
                                               TP_CONJ_A,
                                               TP_CONJ_B>;

    /** Input A (TP_DIM_A x TP_DIM_AB, leading dim per TP_DIM_A_LEADING). **/
    port<input> inA[1];
    /** Input B (TP_DIM_AB x TP_DIM_B, row-major). **/
    port<input> inB[1];
    /** Output C = A*B (TP_DIM_A x TP_DIM_B, leading dim per TP_DIM_OUT_LEADING). **/
    port<output> out[1];

    /** Single vector-API matmul kernel (no cascade/tiling subgraphs). **/
    kernel m_matMultKernel;

    matrix_mult_vecapi_graph() {
        m_matMultKernel = kernel::create_object<MatMultKernel_t>();

        connect(inA[0], m_matMultKernel.in[0]);
        dimensions(m_matMultKernel.in[0]) = {TP_INPUT_WINDOW_VSIZE_A};

        connect(inB[0], m_matMultKernel.in[1]);
        dimensions(m_matMultKernel.in[1]) = {TP_INPUT_WINDOW_VSIZE_B};

        connect(m_matMultKernel.out[0], out[0]);
        dimensions(m_matMultKernel.out[0]) = {TP_DIM_A * TP_DIM_B};

        runtime<ratio>(m_matMultKernel) = 0.9;
        source(m_matMultKernel) = "matrix_mult_vecapi.cpp";
        headers(m_matMultKernel) = {"matrix_mult_vecapi.hpp"};
    }
};

} // namespace mp_pseudoinverse_svd
} // namespace aie
} // namespace solver
} // namespace xf

#endif // _SOLVERLIB_MATRIX_MULT_VECAPI_GRAPH_HPP_
