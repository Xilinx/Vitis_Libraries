/*
 * Copyright (C) 2019-2022, Xilinx, Inc.
 * Copyright (C) 2022-2024, Advanced Micro Devices, Inc.
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
#ifndef _DSPLIB_MATRIX_VECTOR_MUL_HPP_
#define _DSPLIB_MATRIX_VECTOR_MUL_HPP_

/*
MATRIX_VECTOR_MUL
This file exists to capture the definition matrix_vector_mul kernel
class.
The class definition holds defensive checks on parameter range and other legality.
The constructor definition is held in this class because this class must be accessible to graph
level aie compilation.
The main runtime function is captured elsewhere as it contains aie intrinsics which are not
included in aie graph level compilation.
*/

/* Coding conventions
   TT_      template type suffix
   TP_      template parameter suffix
*/

/* Design Notes
*/

#include <adf.h>
#include <vector>

#include "matrix_vector_mul_traits.hpp"
#include "fir_utils.hpp"
#include "aie_api/aie.hpp"

namespace xf {
namespace dsp {
namespace aie {
namespace blas {
namespace matrix_vector_mul {

//-----------------------------------------------------------------------------------------------------
// Base class
// TT_DATA_A, TT_DATA_B, TP_DIM_A, TP_DIM_B, TP_SHIFT, TP_RND, TP_NUM_FRAMES, TP_CASC_LEN, TP_SAT
template <typename TT_DATA_A,
          typename TT_DATA_B,
          unsigned int TP_DIM_A,
          unsigned int TP_DIM_B,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_CASC_LEN,
          unsigned int TP_KERNEL_POSITION,
          bool TP_CASC_IN = CASC_IN_FALSE,
          bool TP_CASC_OUT = CASC_OUT_FALSE>
class kernelMatVecMulClass {
   private:
    // Parameter value defensive and legality checks
    // TODO - Add static asserts for matrix_vector_mul
    static_assert(TP_RND >= ROUND_MIN && TP_RND <= ROUND_MAX, "ERROR: TP_RND is out of the supported range.");
    static_assert(TP_SAT >= SAT_MODE_MIN && TP_SAT <= SAT_MODE_MAX, "ERROR: TP_SAT is out of supported range");
    static_assert(TP_SAT != 2, "ERROR: TT_DATA_A is not currently supported");
    static_assert(TP_SHIFT >= SHIFT_MIN && TP_SHIFT <= SHIFT_MAX, "ERROR: SHIFT is out of the supported range.");
    // static_assert(TP_DIM_A_LEADING == 1, "ERROR: Matrix-vector multiplication currently only supports matrix data
    // stored in a column major format. Row major matrices must be transposed to column major");
    static_assert((TP_DIM_A % (256 / 8 / sizeof(TT_DATA_A))) == 0,
                  "ERROR: TP_DIM_A must be a multiple of (256 / 8 / sizeof(TT_DATA_A). The input matrix data can be "
                  "zero-padded as required");
    static_assert((TP_DIM_B % (TP_CASC_LEN * 256 / 8 / sizeof(TT_DATA_B))) == 0,
                  "ERROR: TP_DIM_B must be a multiple of (256 / 8 / sizeof(TT_DATA_B) and TP_CASC_LEN. The input "
                  "vector data can be zero-padded as required ");
    static_assert(!(std::is_same<TT_DATA_A, cfloat>::value || std::is_same<TT_DATA_A, float>::value) || (TP_SHIFT == 0),
                  "ERROR: TP_SHIFT cannot be performed for TT_DATA=cfloat, so must be set to 0"); // only necessary to
                                                                                                  // check TT_DATA_A as
                                                                                                  // TT_DATA_B will also
                                                                                                  // be float or integer
                                                                                                  // to match TT_DATA_A.

    using TT_OUT = outType_t<TT_DATA_A, TT_DATA_B>;
    static constexpr int vecSampleNumA = mumLanesMatVec<TT_DATA_A, TT_DATA_B>().lanesInA;
    static constexpr int vecSampleNumB = mumLanesMatVec<TT_DATA_A, TT_DATA_B>().lanesInB;
    static constexpr int vecSampleNumOut = mumLanesMatVec<TT_DATA_A, TT_DATA_B>().lanesInOut;

    static constexpr int loadsPerColA = TP_DIM_A / vecSampleNumA;
    static constexpr int loadsPerMatrix = loadsPerColA * (TP_DIM_B / TP_CASC_LEN);
    static constexpr int loadsPerVectorB = (TP_DIM_B / TP_CASC_LEN) / vecSampleNumB;
    static constexpr int shift = TP_SHIFT;
    static constexpr int castBtoA =
        (std::is_same<TT_DATA_A, cint32>::value && std::is_same<TT_DATA_B, cint16>::value) ||
        (std::is_same<TT_DATA_A, int32>::value && std::is_same<TT_DATA_B, int16>::value);

   public:
    // Constructor
    kernelMatVecMulClass() {}

    // MATRIX_VECTOR_MUL
    void kernelMatVecMul(T_inputIF<TP_CASC_IN, TT_DATA_A, TT_DATA_B> inInterface,
                         T_outputIF<TP_CASC_OUT, TT_DATA_A, TT_DATA_B> outInterface);
};

//-----------------------------------------------------------------------------------------------------
// This is the main declaration of the matrix_mult class, and is also used for the Standalone kernel specialization with
// no cascade ports, a single input and no reload

template <typename TT_DATA_A,
          typename TT_DATA_B,
          unsigned int TP_DIM_A,
          unsigned int TP_DIM_B,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_CASC_LEN,
          unsigned int TP_KERNEL_POSITION,
          bool TP_CASC_IN = CASC_IN_FALSE,
          bool TP_CASC_OUT = CASC_OUT_FALSE>
class matrix_vector_mul {
   private:
    kernelMatVecMulClass<TT_DATA_A,
                         TT_DATA_B,
                         TP_DIM_A,
                         TP_DIM_B,
                         TP_SHIFT,
                         TP_RND,
                         TP_SAT,
                         TP_NUM_FRAMES,
                         TP_CASC_LEN,
                         TP_KERNEL_POSITION,
                         TP_CASC_IN,
                         TP_CASC_OUT>
        m_mat_vec_mulKernel;
    using TT_OUT = outType_t<TT_DATA_A, TT_DATA_B>;

   public:
    matrix_vector_mul() {}
    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(matrix_vector_mul::matVecMulMain); }
    // DFT
    void matVecMulMain(input_buffer<TT_DATA_A>& __restrict inWindowA,
                       input_buffer<TT_DATA_B>& __restrict inWindowB,
                       output_buffer<TT_OUT>& __restrict outWindow);
};
//-----------------------------------------------------------------------------------------------------
// Partially specialized classes for cascaded interafce - FIRST kernel in cascade

template <typename TT_DATA_A,
          typename TT_DATA_B,
          unsigned int TP_DIM_A,
          unsigned int TP_DIM_B,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_CASC_LEN,
          unsigned int TP_KERNEL_POSITION>
class matrix_vector_mul<TT_DATA_A,
                        TT_DATA_B,
                        TP_DIM_A,
                        TP_DIM_B,
                        TP_SHIFT,
                        TP_RND,
                        TP_SAT,
                        TP_NUM_FRAMES,
                        TP_CASC_LEN,
                        TP_KERNEL_POSITION,
                        CASC_IN_FALSE,
                        CASC_OUT_TRUE> {
   private:
    kernelMatVecMulClass<TT_DATA_A,
                         TT_DATA_B,
                         TP_DIM_A,
                         TP_DIM_B,
                         TP_SHIFT,
                         TP_RND,
                         TP_SAT,
                         TP_NUM_FRAMES,
                         TP_CASC_LEN,
                         TP_KERNEL_POSITION,
                         CASC_IN_FALSE,
                         CASC_OUT_TRUE>
        m_mat_vec_mulKernel;
    using TT_OUT = outType_t<TT_DATA_A, TT_DATA_B>;

   public:
    matrix_vector_mul() {}
    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(matrix_vector_mul::matVecMulMain); }
    void matVecMulMain(input_buffer<TT_DATA_A>& __restrict inWindowA,
                       input_buffer<TT_DATA_B>& __restrict inWindowB,
                       output_stream<accType_t<TT_DATA_A, TT_DATA_B> >* outCascade);
};
//-----------------------------------------------------------------------------------------------------
// Partially specialized classes for cascaded interafce - MIDDLE kernel in cascade

template <typename TT_DATA_A,
          typename TT_DATA_B,
          unsigned int TP_DIM_A,
          unsigned int TP_DIM_B,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_CASC_LEN,
          unsigned int TP_KERNEL_POSITION>
class matrix_vector_mul<TT_DATA_A,
                        TT_DATA_B,
                        TP_DIM_A,
                        TP_DIM_B,
                        TP_SHIFT,
                        TP_RND,
                        TP_SAT,
                        TP_NUM_FRAMES,
                        TP_CASC_LEN,
                        TP_KERNEL_POSITION,
                        CASC_IN_TRUE,
                        CASC_OUT_TRUE> {
   private:
    kernelMatVecMulClass<TT_DATA_A,
                         TT_DATA_B,
                         TP_DIM_A,
                         TP_DIM_B,
                         TP_SHIFT,
                         TP_RND,
                         TP_SAT,
                         TP_NUM_FRAMES,
                         TP_CASC_LEN,
                         TP_KERNEL_POSITION,
                         CASC_IN_TRUE,
                         CASC_OUT_TRUE>
        m_mat_vec_mulKernel;
    using TT_OUT = outType_t<TT_DATA_A, TT_DATA_B>;

   public:
    matrix_vector_mul() {}
    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(matrix_vector_mul::matVecMulMain); }
    void matVecMulMain(input_buffer<TT_DATA_A>& __restrict inWindowA,
                       input_buffer<TT_DATA_B>& __restrict inWindowB,
                       input_stream<accType_t<TT_DATA_A, TT_DATA_B> >* inCascade,
                       output_stream<accType_t<TT_DATA_A, TT_DATA_B> >* outCascade);
};
//-----------------------------------------------------------------------------------------------------
// Partially specialized classes for cascaded interafce - FINAL kernel in cascade

template <typename TT_DATA_A,
          typename TT_DATA_B,
          unsigned int TP_DIM_A,
          unsigned int TP_DIM_B,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_CASC_LEN,
          unsigned int TP_KERNEL_POSITION>
class matrix_vector_mul<TT_DATA_A,
                        TT_DATA_B,
                        TP_DIM_A,
                        TP_DIM_B,
                        TP_SHIFT,
                        TP_RND,
                        TP_SAT,
                        TP_NUM_FRAMES,
                        TP_CASC_LEN,
                        TP_KERNEL_POSITION,
                        CASC_IN_TRUE,
                        CASC_OUT_FALSE> {
   private:
    kernelMatVecMulClass<TT_DATA_A,
                         TT_DATA_B,
                         TP_DIM_A,
                         TP_DIM_B,
                         TP_SHIFT,
                         TP_RND,
                         TP_SAT,
                         TP_NUM_FRAMES,
                         TP_CASC_LEN,
                         TP_KERNEL_POSITION,
                         CASC_IN_TRUE,
                         CASC_OUT_FALSE>
        m_mat_vec_mulKernel;
    using TT_OUT = outType_t<TT_DATA_A, TT_DATA_B>;

   public:
    matrix_vector_mul() {}
    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(matrix_vector_mul::matVecMulMain); }
    void matVecMulMain(input_buffer<TT_DATA_A>& __restrict inWindowA,
                       input_buffer<TT_DATA_B>& __restrict inWindowB,
                       input_stream<accType_t<TT_DATA_A, TT_DATA_B> >* inCascade,
                       output_buffer<TT_OUT>& __restrict outWindow);
};
}
}
}
}
}
#endif // _DSPLIB_MATRIX_VECTOR_MUL_HPP_
