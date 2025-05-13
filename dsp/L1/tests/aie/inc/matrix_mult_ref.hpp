/*
 * Copyright (C) 2019-2022, Xilinx, Inc.
 * Copyright (C) 2022-2025, Advanced Micro Devices, Inc.
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
#ifndef _DSPLIB_MATRIX_MULT_REF_HPP_
#define _DSPLIB_MATRIX_MULT_REF_HPP_

// This file holds the definition of the Asymmetric Interpolation FIR reference model kernel class.
// Design Notes
// Note that the AIE intrinsics operate on increasing indices, but in a conventional FIR there is a convolution of data
// and coefficients.
// So as to achieve the impulse response from the filter which matches the coefficeint set, the coefficient array has to
// be reversed
// to compensate for the action of the intrinsics. This reversal is performed in the constructor where possible, or in
// the reference
// model's run-time filter function.

#include <adf.h>
#include <limits>

using namespace adf;

namespace xf {
namespace dsp {
namespace aie {
namespace blas {
namespace matrix_mult {

//#ifndef GET_TT_OUT
//#define GET_TT_OUT(A,B) std::conditional_t<(sizeof(B) > sizeof(A)),B, A>
//#endif //GET_TT_OUT
#ifndef ROW_MAJOR
#define ROW_MAJOR 0
#endif // ROW_MAJOR
#ifndef COL_MAJOR
#define COL_MAJOR 1
#endif // COL_MAJOR

//-----------------------------------------------------------------------------------------------------
// Single Rate class
template <typename TT_DATA_A,
          typename TT_DATA_B,
          typename TT_OUT_DATA,
          size_t TP_DIM_A,
          size_t TP_DIM_AB,
          size_t TP_DIM_B,
          size_t TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_SAT = 1,
          unsigned int TP_DIM_A_LEADING = ROW_MAJOR,
          unsigned int TP_DIM_B_LEADING = COL_MAJOR,
          unsigned int TP_DIM_OUT_LEADING = ROW_MAJOR,
          unsigned int TP_INPUT_WINDOW_VSIZE_A = TP_DIM_A* TP_DIM_AB,
          unsigned int TP_INPUT_WINDOW_VSIZE_B = TP_DIM_B* TP_DIM_AB>
class matrix_mult_ref {
   public:
    // Constructor
    matrix_mult_ref() {
        // something?
    }
    // Register Kernel Class
    static void registerKernelClass() {
        REGISTER_FUNCTION(matrix_mult_ref::mmult);
        // REGISTER_PARAMETER(m_internalTapsRef);
    }
    // TT_OUT will need to be a formula.
    // If A or B is cplx, then output is cplx, else real.
    // If A or B is float then output is float, else int.
    // if A or B is *int32, then output is int32, else int16.
    //    - This can be user decision and simply changes SRS.
    void mmult(input_buffer<TT_DATA_A>& inWindowA,
               input_buffer<TT_DATA_B>& inWindowB,
               output_buffer<TT_OUT_DATA>& outWindow);

   private:
};
}
}
}
}
}

#endif // matrix_mult_ref
