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
#ifndef _SOLVERLIB_MP_PSEUDOINVERSE_SVD_REF_HPP_
#define _SOLVERLIB_MP_PSEUDOINVERSE_SVD_REF_HPP_

/*
Reference (golden model) kernel for the Moore-Penrose pseudoinverse.

For full-rank overdetermined systems (TP_DIM_ROWS >= TP_DIM_COLS) the
pseudoinverse is computed as:
    A^+ = (A^H A)^-1 A^H

Steps:
  1. Form the normal matrix  B = A^H * A   (TP_DIM_COLS x TP_DIM_COLS).
  2. Augment B with the identity: [B | I_n].
  3. Apply Gauss-Jordan elimination in-place to reach [I_n | B^-1].
  4. Multiply B^-1 * A^H to produce A^+.

This scalar O(n^3) reference makes no use of AIE intrinsics and is
therefore compilable for both x86sim and the AIE device.
*/

#include <adf.h>
#include "svd_traits.hpp"

using namespace adf;

namespace xf {
namespace solver {
namespace aie {
namespace mp_pseudoinverse_svd {

using namespace xf::solver::aie::svd; // for fnIsComplexDataType

/**
 * @brief Reference pseudoinverse kernel.
 *
 * Computes A^+ = (A^H*A)^-1 * A^H via Gauss-Jordan elimination.
 * Suitable for full-rank overdetermined systems (TP_DIM_ROWS >= TP_DIM_COLS).
 *
 * Output: A^+ stored column-major with TP_DIM_COLS leading dimension,
 * exactly TP_DIM_COLS × TP_DIM_ROWS samples — no kStoreCols padding.
 * A^+[i,j] is at flat index  i + j * TP_DIM_COLS.
 *
 * @tparam TT_DATA     Data type: float or cfloat.
 * @tparam TP_DIM_ROWS Number of rows of A.
 * @tparam TP_DIM_COLS Number of columns of A.
 **/
template <typename TT_DATA, unsigned int TP_DIM_ROWS, unsigned int TP_DIM_COLS>
class mp_pseudoinverse_svd_ref {
   public:
    static constexpr bool kIsComplex = fnIsComplexDataType<TT_DATA>();

    // Input: A matrix (TP_DIM_ROWS × TP_DIM_COLS, column-major)
    static constexpr unsigned int kInWindowVsize = TP_DIM_ROWS * TP_DIM_COLS;
    // Output: A^+ (TP_DIM_COLS × TP_DIM_ROWS, column-major, leading dim TP_DIM_COLS)
    // A^+[i,j] is at flat index i + j * TP_DIM_COLS.
    static constexpr unsigned int kOutWindowVsize = TP_DIM_COLS * TP_DIM_ROWS;

    static void registerKernelClass() { REGISTER_FUNCTION(mp_pseudoinverse_svd_ref::mp_pseudoinverse_svd_ref_main); }

    /**
     * @brief Compute the pseudoinverse reference.
     *
     * @param inA    Input buffer: A matrix (TP_DIM_ROWS x TP_DIM_COLS, column-major).
     * @param outAp  Output buffer: A^+ (TP_DIM_COLS x TP_DIM_ROWS, column-major).
     *               A^+[i,j] is at flat index i + j*TP_DIM_COLS.
     **/
    void mp_pseudoinverse_svd_ref_main(input_buffer<TT_DATA>& __restrict inA, output_buffer<TT_DATA>& __restrict outAp);
};

} // namespace mp_pseudoinverse_svd
} // namespace aie
} // namespace solver
} // namespace xf

#endif // _SOLVERLIB_MP_PSEUDOINVERSE_SVD_REF_HPP_
