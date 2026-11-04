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
#ifndef _EIGENVALUE_REF_GRAPH_HPP_
#define _EIGENVALUE_REF_GRAPH_HPP_

/*
Reference graph for the eigenvalue solver.

This single-kernel reference graph computes the eigenvalues of a square
matrix A via a plain, unshifted, undeflated QR-algorithm iteration -- see
eigenvalue_ref.hpp for why this is a deliberately different algorithm from
the multi-kernel UUT graph (hessenberg + shifted/deflated eig_qr), used as
an algorithm-independent golden model.
*/

#include <adf.h>
#include "eigenvalue_ref.hpp"

namespace xf {
namespace solver {
namespace aie {
namespace eigenvalue {

using namespace adf;

/**
 * @brief Reference graph for the eigenvalue solver IP.
 *
 * Instantiates a single reference kernel that computes the eigenvalues of A
 * via plain unshifted QR iteration.
 *
 * @tparam TT_DATA Data type: float or cfloat.
 * @tparam TP_DIM  Dimension of the square matrix A (= number of eigenvalues).
 */
template <typename TT_DATA, unsigned int TP_DIM>
class eigenvalue_ref_graph : public adf::graph {
   public:
    /** Input port: A matrix (TP_DIM x TP_DIM). */
    port<input> in;
    /** Output port: eigenvalues of A (TP_DIM elements). */
    port<output> out;

    kernel m_kernel;

    eigenvalue_ref_graph() {
        printf("============================\n");
        printf("== EIGENVALUE REF graph\n");
        printf("============================\n");
        printf("TP_DIM = %d\n", TP_DIM);

        m_kernel = kernel::create_object<eigenvalue_ref<TT_DATA, TP_DIM> >();

        runtime<ratio>(m_kernel) = 0.9;
        source(m_kernel) = "eigenvalue_ref.cpp";

        connect(in, m_kernel.in[0]);
        dimensions(m_kernel.in[0]) = {TP_DIM * TP_DIM};

        connect(m_kernel.out[0], out);
        dimensions(m_kernel.out[0]) = {TP_DIM};
    }
};

} // namespace eigenvalue
} // namespace aie
} // namespace solver
} // namespace xf

#endif // _EIGENVALUE_REF_GRAPH_HPP_
