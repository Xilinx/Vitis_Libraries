/*
 * Copyright (C) 2019-2022, Xilinx, Inc.
 * Copyright (C) 2022-2026, Advanced Micro Devices, Inc.
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
#ifndef _SOLVERLIB_MP_PSEUDOINVERSE_SVD_REF_GRAPH_HPP_
#define _SOLVERLIB_MP_PSEUDOINVERSE_SVD_REF_GRAPH_HPP_

/*
Reference graph for the Moore-Penrose pseudoinverse.

Provides the same external interface as mp_pseudoinverse_svd_graph (single in/out
port pair for matrix A and pseudoinverse A^+) but drives a scalar reference
kernel rather than the SVD-based production implementation.

Used when USING_UUT=0 to generate golden reference data for comparison
against the production kernel output.
*/
/**
 * @file mp_pseudoinverse_svd_ref_graph.hpp
 *
 **/

#include <adf.h>
#include "mp_pseudoinverse_svd_ref.hpp"

using namespace adf;

namespace xf {
namespace solver {
namespace aie {
namespace mp_pseudoinverse_svd {

/**
 * @brief Reference graph for the Moore-Penrose pseudoinverse.
 *
 * Computes A^+ = (A^H A)^-1 A^H using a scalar Gauss-Jordan reference kernel.
 * Suitable for full-rank overdetermined systems (TP_DIM_ROWS >= TP_DIM_COLS).
 *
 * @tparam TT_DATA    Scalar data type: float or cfloat.
 * @tparam TP_DIM_ROWS  Number of rows of A.
 * @tparam TP_DIM_COLS  Number of columns of A.
 **/
template <typename TT_DATA, unsigned int TP_DIM_ROWS, unsigned int TP_DIM_COLS>
class mp_pseudoinverse_svd_ref_graph : public graph {
   public:
    static constexpr int kInWindowVsize = TP_DIM_ROWS * TP_DIM_COLS;
    // Output: A^+ (TP_DIM_COLS × TP_DIM_ROWS, column-major, leading dim TP_DIM_COLS)
    // Matches the production kernel's output format (no kStoreCols padding).
    static constexpr int kOutWindowVsize = TP_DIM_COLS * TP_DIM_ROWS;

    /**
     * Input matrix A (TP_DIM_ROWS x TP_DIM_COLS, column-major).
     **/
    port<input> in;

    /**
     * Output pseudoinverse A^+ (TP_DIM_COLS x TP_DIM_ROWS, column-major).
     **/
    port<output> out;

    /** Reference kernel instance. **/
    kernel m_refKernel;

    /**
     * Access function.
     **/
    kernel* getKernels() { return &m_refKernel; }

    /**
     * @brief Constructor for the reference graph.
     **/
    mp_pseudoinverse_svd_ref_graph() {
        m_refKernel = kernel::create_object<mp_pseudoinverse_svd_ref<TT_DATA, TP_DIM_ROWS, TP_DIM_COLS> >();

        connect(in, m_refKernel.in[0]);
        dimensions(m_refKernel.in[0]) = {kInWindowVsize};

        connect(m_refKernel.out[0], out);
        dimensions(m_refKernel.out[0]) = {kOutWindowVsize};

        runtime<ratio>(m_refKernel) = 0.9;
        source(m_refKernel) = "mp_pseudoinverse_svd_ref.cpp";
        headers(m_refKernel) = {"mp_pseudoinverse_svd_ref.hpp"};
        stack_size(m_refKernel) = 8192; // larger stack for the Gauss-Jordan augmented matrix
    }
};

} // namespace mp_pseudoinverse_svd
} // namespace aie
} // namespace solver
} // namespace xf

#endif // _SOLVERLIB_MP_PSEUDOINVERSE_SVD_REF_GRAPH_HPP_
