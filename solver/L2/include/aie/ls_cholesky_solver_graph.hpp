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
#ifndef _SOLVERLIB_LS_CHOLESKY_SOLVER_GRAPH_HPP_
#define _SOLVERLIB_LS_CHOLESKY_SOLVER_GRAPH_HPP_

/*
The file captures the definition of the 'L2' graph level class for the
Least Squares Cholesky Solver library element.

This IP solves the linear system Ax = y where:
  - A is a symmetric positive definite (SPD) square matrix
  - y is an input vector
  - x is the output solution vector

The algorithm used is:
  1. Cholesky decomposition: A = L * L^H  (L is lower triangular, with DIAG_INV=1
     so diagonal elements are stored as their reciprocal square roots)
  2. Forward substitution: Solve L * w = y for w
  3. Backward substitution: Solve L^H * x = w for x
*/

/**
 * @file ls_cholesky_solver_graph.hpp
 **/

#include <adf.h>
#include "cholesky_graph.hpp"
#include "substitution_graph.hpp"

namespace xf::solver::aie::ls_cholesky_solver {

using namespace adf;

/**
 * @defgroup ls_cholesky_solver Least Squares Cholesky Solver
 *
 * Least Squares Cholesky Solver: Solves Ax = y for x where A is symmetric
 * positive definite, using Cholesky decomposition followed by forward and
 * backward substitution.
 */

//--------------------------------------------------------------------------------------------------
// ls_cholesky_solver_graph template
//--------------------------------------------------------------------------------------------------
/**
 * @ingroup ls_cholesky_solver
 * @brief This class solves the linear system Ax = y for x, where A is a symmetric positive
 *        definite (SPD) square matrix. It chains three internal stages:
 *          1. Cholesky decomposition (A = L * L^H) with DIAG_INV=1
 *          2. Forward substitution (L * w = y)
 *          3. Backward substitution (L^H * x = w)
 *
 * These are the templates to configure the function.
 * @tparam TT_DATA describes the type of individual data samples.
 *         Must be one of: float, cfloat.
 * @tparam TP_DIM describes the length of one dimension of the square matrix A, in samples.
 * @tparam TP_NUM_FRAMES describes the number of matrices/vectors to process per invocation.
 * @tparam TP_GRID_DIM describes the tiling dimension used to divide the matrix across kernels.
 *         The number of A_in/y_in port tiles is TP_GRID_DIM*(TP_GRID_DIM+1)/2.
 * @tparam TP_CASC_LEN describes the cascade depth of the internal Cholesky stage.
 **/
template <typename TT_DATA,
          unsigned int TP_DIM,
          unsigned int TP_NUM_FRAMES = 1,
          unsigned int TP_GRID_DIM = 1,
          unsigned int TP_CASC_LEN = 1>
class ls_cholesky_solver_graph : public adf::graph {
   public:
    /**
    * @cond NOCOMMENTS
    */
    static constexpr unsigned int kNumPortKernels = TP_GRID_DIM * (TP_GRID_DIM + 1) / 2;
    static constexpr unsigned int kKernelDim = TP_DIM / TP_GRID_DIM;

    static_assert(std::is_same<TT_DATA, float>::value || std::is_same<TT_DATA, cfloat>::value,
                  "ERROR: TT_DATA must be either float or cfloat.");
    static_assert(TP_DIM % TP_GRID_DIM == 0, "ERROR: TP_DIM must be exactly divisible by TP_GRID_DIM.");
    static_assert(TP_NUM_FRAMES > 0, "ERROR: TP_NUM_FRAMES must be greater than 0.");
    static_assert(TP_CASC_LEN > 0, "ERROR: TP_CASC_LEN must be greater than 0.");
    static_assert(TP_CASC_LEN <= TP_DIM, "ERROR: TP_CASC_LEN must be less than or equal to TP_DIM.");
    /**
    * @endcond
    */

    /**
     * The input SPD matrix A in Ax = y.
     * Provided as kNumPortKernels tiled sub-matrix ports (column-major, lower-triangular tiling).
     **/
    port_array<adf::input, kNumPortKernels> A_in;

    /**
     * The input vector y in Ax = y.
     * Provided as kNumPortKernels ports to match the internal forward-substitution kernel grid.
     * Each port carries the y slice corresponding to the kernel column index.
     **/
    port_array<adf::input, kNumPortKernels> y_in;

    /**
     * The output solution vector x in Ax = y.
     * Provided as TP_GRID_DIM ports (one slice per diagonal tile).
     **/
    port_array<adf::output, TP_GRID_DIM> x_out;

    // Internal sub-graphs
    // Stage 1: Cholesky decomposition  A = L * L^H  (DIAG_INV=1)
    xf::solver::aie::cholesky::
        cholesky_graph<TT_DATA, TP_DIM, TP_NUM_FRAMES, TP_GRID_DIM, TP_CASC_LEN, 1 /*TP_DIAG_INV*/>
            cholesky_g;

    // Stage 2: Forward substitution  L * w = y  (column-major L, DIAG_INV=1)
    xf::solver::aie::substitution::substitution_graph<TT_DATA,
                                                      TP_DIM,
                                                      0, /*TP_SUBST_TYPE=0: forward*/
                                                      1, /*TP_L_LEADING=1: column-major*/
                                                      TP_GRID_DIM,
                                                      TP_NUM_FRAMES,
                                                      1 /*TP_DIAG_INV*/>
        fwd_subst_g;

    // Stage 3: Backward substitution  L^H * x = w  (column-major L, DIAG_INV=1)
    // With TP_L_LEADING=1 and TP_SUBST_TYPE=1, the buffer descriptor inside
    // substitution_graph transposes L before the kernel sees it, yielding L^H.
    xf::solver::aie::substitution::substitution_graph<TT_DATA,
                                                      TP_DIM,
                                                      1, /*TP_SUBST_TYPE=1: backward*/
                                                      1, /*TP_L_LEADING=1: column-major*/
                                                      TP_GRID_DIM,
                                                      TP_NUM_FRAMES,
                                                      1 /*TP_DIAG_INV*/>
        bwd_subst_g;

    /**
     * @brief Constructor: wires the three internal sub-graphs together.
     **/
    ls_cholesky_solver_graph() {
        // -------------------------------------------------------------------
        // 1. A_in  →  Cholesky
        // -------------------------------------------------------------------
        for (unsigned int i = 0; i < kNumPortKernels; i++) {
            connect(A_in[i], cholesky_g.in[i]);
        }

        // -------------------------------------------------------------------
        // 2. Cholesky out  →  Forward substitution L_in  (direct, 1:1)
        //    Cholesky out  →  Backward substitution L_in  (direct, 1:1)
        //    substitution_graph with TP_SUBST_TYPE=1 applies an internal
        //    L_port_map that re-routes each external L_in[i] to the correct
        //    backward kernel.  No pre-remapping is needed here: applying the
        //    same (involutive) permutation a second time in this graph would
        //    cancel the internal one and produce an incorrect result.
        //    Mark cholesky outputs as single_buffer so both substitution
        //    stages can read them without the buffer being overwritten.
        // -------------------------------------------------------------------
        for (unsigned int i = 0; i < kNumPortKernels; i++) {
            connect(cholesky_g.out[i], fwd_subst_g.L_in[i]);
            connect(cholesky_g.out[i], bwd_subst_g.L_in[i]);
            single_buffer(cholesky_g.out[i]);
        }

        // -------------------------------------------------------------------
        // 3. y_in  →  Forward substitution y_in  (direct)
        // -------------------------------------------------------------------
        for (unsigned int i = 0; i < kNumPortKernels; i++) {
            connect(y_in[i], fwd_subst_g.y_in[i]);
        }

        // -------------------------------------------------------------------
        // 4. Forward substitution x_out (w slices)  →  Backward substitution y_in
        //
        //    fwd_subst_g.x_out[gx] = w slice gx  (gx = 0..GRID_DIM-1)
        //    For flat index fwd_idx at grid position (gy, gx), connect
        //    w[gx] to bwd_subst_g.y_in[fwd_idx].
        //    substitution_graph's internal L_port_map then routes
        //    y_in[fwd_idx] to the correct backward kernel.
        //    Multiple fwd_idx values that share the same gx result in a
        //    one-to-many connection, which ADF handles automatically.
        // -------------------------------------------------------------------
        {
            int fwd_idx = 0;
            for (int gy = 0; gy < (int)TP_GRID_DIM; gy++) {
                for (int gx = 0; gx <= gy; gx++) {
                    connect(fwd_subst_g.x_out[gx], bwd_subst_g.y_in[fwd_idx]);
                    fwd_idx++;
                }
            }
        }

        // -------------------------------------------------------------------
        // 5. Backward substitution x_out  →  external x_out
        // -------------------------------------------------------------------
        for (unsigned int i = 0; i < TP_GRID_DIM; i++) {
            connect(bwd_subst_g.x_out[i], x_out[i]);
        }
    }
};

} // namespace xf::solver::aie::ls_cholesky_solver

#endif // _SOLVERLIB_LS_CHOLESKY_SOLVER_GRAPH_HPP_
