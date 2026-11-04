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
#ifndef _LS_CHOLESKY_SOLVER_REF_GRAPH_HPP_
#define _LS_CHOLESKY_SOLVER_REF_GRAPH_HPP_

/*
Reference graph for the Least Squares Cholesky Solver.

This single-kernel reference graph solves Ax = y for x where A is a symmetric
positive definite (SPD) matrix.  It is used as a golden model against which the
multi-kernel UUT graph is validated.

The reference kernel accepts:
  A_in  : full DIM x DIM matrix A  (column-major, TP_NUM_FRAMES frames)
  y_in  : vector y of length DIM   (TP_NUM_FRAMES vectors)
  x_out : solution vector x of length DIM  (TP_NUM_FRAMES solutions)
*/

#include <adf.h>
#include "ls_cholesky_solver_ref.hpp"

namespace xf {
namespace solver {
namespace aie {
namespace ls_cholesky_solver {

using namespace adf;

/**
 * @brief Reference graph for the ls_cholesky_solver IP.
 *
 * Instantiates a single reference kernel that performs:
 *   1. Cholesky decomposition (DIAG_INV = 1)
 *   2. Forward substitution
 *   3. Backward substitution
 * and writes the solution vector x.
 *
 * @tparam TT_DATA       Data type: float or cfloat.
 * @tparam TP_DIM        Dimension N of the N x N input matrix A.
 * @tparam TP_NUM_FRAMES Number of A/y pairs processed per graph invocation.
 */
template <typename TT_DATA, unsigned int TP_DIM, unsigned int TP_NUM_FRAMES = 1>
class ls_cholesky_solver_ref_graph : public adf::graph {
   public:
    /** Input port: full DIM x DIM matrix A, column-major, TP_NUM_FRAMES matrices. */
    port<input> A_in;
    /** Input port: right-hand-side vector y of length DIM, TP_NUM_FRAMES vectors. */
    port<input> y_in;
    /** Output port: solution vector x of length DIM, TP_NUM_FRAMES solutions. */
    port<output> x_out;

    kernel m_kernel;

    ls_cholesky_solver_ref_graph() {
        printf("============================\n");
        printf("== LS_CHOLESKY_SOLVER REF graph\n");
        printf("============================\n");
        printf("TP_DIM        = %d\n", TP_DIM);
        printf("TP_NUM_FRAMES = %d\n", TP_NUM_FRAMES);

        m_kernel = kernel::create_object<ls_cholesky_solver_ref<TT_DATA, TP_DIM, TP_NUM_FRAMES> >();

        runtime<ratio>(m_kernel) = 0.9;
        source(m_kernel) = "ls_cholesky_solver_ref.cpp";

        // in_A: full matrix, column-major, DIM*DIM elements per frame
        connect(A_in, m_kernel.in[0]);
        dimensions(m_kernel.in[0]) = {TP_DIM * TP_DIM * TP_NUM_FRAMES};

        // in_y: vector of length DIM per frame
        connect(y_in, m_kernel.in[1]);
        dimensions(m_kernel.in[1]) = {TP_DIM * TP_NUM_FRAMES};

        // out_x: solution vector of length DIM per frame
        connect(m_kernel.out[0], x_out);
        dimensions(m_kernel.out[0]) = {TP_DIM * TP_NUM_FRAMES};
    }
};

} // namespace ls_cholesky_solver
} // namespace aie
} // namespace solver
} // namespace xf

#endif // _LS_CHOLESKY_SOLVER_REF_GRAPH_HPP_
