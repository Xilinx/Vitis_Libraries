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
#ifndef _LS_CHOLESKY_SOLVER_REF_HPP_
#define _LS_CHOLESKY_SOLVER_REF_HPP_

/*
Least Squares Cholesky Solver reference model.

Solves Ax = y for x where A is a symmetric positive definite (SPD) square matrix
by chaining three sequential steps:
  1. Cholesky decomposition  A = L * L^H    (DIAG_INV=1: stores 1/sqrt on diagonal)
  2. Forward  substitution   L * w = y      (using DIAG_INV=1 result)
  3. Backward substitution   L^H * x = w   (using DIAG_INV=1 result)

Data conventions:
  All matrices are stored COLUMN-MAJOR:  element (row r, col c) is at index c*DIM + r.
  in_A  : TP_NUM_FRAMES contiguous DIM x DIM matrices (column-major)
  in_y  : TP_NUM_FRAMES contiguous vectors of length DIM
  out_x : TP_NUM_FRAMES contiguous solution vectors of length DIM
*/

#include <adf.h>

using namespace adf;

namespace xf {
namespace solver {
namespace aie {
namespace ls_cholesky_solver {

template <typename TT_DATA, unsigned int TP_DIM, unsigned int TP_NUM_FRAMES>
class ls_cholesky_solver_ref {
   public:
    ls_cholesky_solver_ref() = default;

    static void registerKernelClass() { REGISTER_FUNCTION(ls_cholesky_solver_ref::ls_cholesky_solver_main); }

    void ls_cholesky_solver_main(input_buffer<TT_DATA>& __restrict in_A,
                                 input_buffer<TT_DATA>& __restrict in_y,
                                 output_buffer<TT_DATA>& __restrict out_x);
};

} // namespace ls_cholesky_solver
} // namespace aie
} // namespace solver
} // namespace xf

#endif // _LS_CHOLESKY_SOLVER_REF_HPP_
