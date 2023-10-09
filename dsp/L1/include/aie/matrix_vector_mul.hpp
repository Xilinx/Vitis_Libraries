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
#ifndef _DSPLIB_MATRIX_VECTOR_MUL_HPP_
#define _DSPLIB_MATRIX_VECTOR_MUL_HPP_

/*
MATRIX_VECTOR_MUL
This file exists to capture the definition of the single channel MATRIX_VECTOR_MUL/iMATRIX_VECTOR_MUL filter kernel
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
#ifndef _DSPLIB_MATRIX_VECTOR_MUL_HPP_DEBUG_
// #define _DSPLIB_MATRIX_VECTOR_MUL_HPP_DEBUG_
#endif //_DSPLIB_MATRIX_VECTOR_MUL_HPP_DEBUG_

namespace xf {
namespace dsp {
namespace aie {
namespace blas {
namespace matrix_vector_mul {

struct no_port {};
template <typename T_A, typename T_B>
struct accType {
    using type = cacc48;
};
template <typename T_A, typename T_B>
struct outType {
    using type = cint16;
};
template <>
struct accType<int16, int16> {
    using type = acc48;
};
template <>
struct outType<int16, int16> {
    using type = int16;
};
template <>
struct accType<int16, cint16> {
    using type = cacc48;
};
template <>
struct outType<int16, cint16> {
    using type = cint16;
};
template <>
struct accType<int16, cint32> {
    using type = cacc80;
};
template <>
struct outType<int16, cint32> {
    using type = cint32;
};
template <>
struct accType<int16, int32> {
    using type = acc80;
};
template <>
struct outType<int16, int32> {
    using type = int32;
};

template <>
struct accType<cint16, int16> {
    using type = cacc48;
};
template <>
struct outType<cint16, int16> {
    using type = cint16;
};
template <>
struct accType<cint16, cint16> {
    using type = cacc48;
};
template <>
struct outType<cint16, cint16> {
    using type = cint16;
};
template <>
struct accType<cint16, int32> {
    using type = cacc80;
};
template <>
struct outType<cint16, int32> {
    using type = cint32;
};
template <>
struct accType<cint16, cint32> {
    using type = cacc80;
};
template <>
struct outType<cint16, cint32> {
    using type = cint32;
};

template <>
struct accType<int32, int16> {
    using type = acc80;
};
template <>
struct outType<int32, int16> {
    using type = int32;
};
template <>
struct accType<int32, cint16> {
    using type = cacc80;
};
template <>
struct outType<int32, cint16> {
    using type = cint32;
};
template <>
struct accType<int32, int32> {
    using type = acc80;
};
template <>
struct outType<int32, int32> {
    using type = int32;
};
template <>
struct accType<int32, cint32> {
    using type = cacc80;
};
template <>
struct outType<int32, cint32> {
    using type = cint32;
};

template <>
struct accType<cint32, int16> {
    using type = cacc80;
};
template <>
struct outType<cint32, int16> {
    using type = cint32;
};
template <>
struct accType<cint32, cint16> {
    using type = cacc80;
};
template <>
struct outType<cint32, cint16> {
    using type = cint32;
};
template <>
struct accType<cint32, int32> {
    using type = cacc80;
};
template <>
struct outType<cint32, int32> {
    using type = cint32;
};
template <>
struct accType<cint32, cint32> {
    using type = cacc80;
};
template <>
struct outType<cint32, cint32> {
    using type = cint32;
};

template <>
struct accType<float, float> {
    using type = accfloat;
};
template <>
struct outType<float, float> {
    using type = float;
};
template <>
struct accType<cfloat, float> {
    using type = caccfloat;
};
template <>
struct outType<cfloat, float> {
    using type = cfloat;
};
template <>
struct accType<float, cfloat> {
    using type = caccfloat;
};
template <>
struct outType<float, cfloat> {
    using type = cfloat;
};
template <>
struct accType<cfloat, cfloat> {
    using type = caccfloat;
};
template <>
struct outType<cfloat, cfloat> {
    using type = cfloat;
};

template <typename T_D_A, typename T_D_B>
using accType_t = typename accType<T_D_A, T_D_B>::type;
template <typename T_D_A, typename T_D_B>
using outType_t = typename outType<T_D_A, T_D_B>::type;

// IF input type-------------------------------------
template <bool T_CASC_IN, typename T_A, typename T_B>
struct T_inputIF {};
// CASC_IN_FALSE
template <typename T_A, typename T_B>
struct T_inputIF<CASC_IN_FALSE, T_A, T_B> {
    T_A* __restrict inWindowA;
    T_B* __restrict inWindowB;
    no_port* inCascade;
};
// CASC_IN_TRUE
template <typename T_A, typename T_B>
struct T_inputIF<CASC_IN_TRUE, T_A, T_B> {
    T_A* __restrict inWindowA;
    T_B* __restrict inWindowB;
    input_stream<accType_t<T_A, T_B> >* inCascade;
};
// IF output type ------------------------------------
template <bool T_CASC_OUT, typename T_A, typename T_B>
struct T_outputIF {};
// CASC_OUT_FALSE
template <typename T_A, typename T_B>
struct T_outputIF<CASC_OUT_FALSE, T_A, T_B> {
    outType_t<T_A, T_B>* __restrict outWindow;
    no_port* outCascade;
};
// CASC_OUT_TRUE
template <typename T_A, typename T_B>
struct T_outputIF<CASC_OUT_TRUE, T_A, T_B> {
    no_port* outWindow;
    output_stream<accType_t<T_A, T_B> >* outCascade;
};

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
    // Currently unsupported data combinations
    static_assert(!(std::is_same<TT_DATA_A, cint32>::value && std::is_same<TT_DATA_B, int16>::value),
                  "Matrix-vector mutliplcation of a cint32 matrix (TT_DATA_A=cint32) and an int16 vector "
                  "(TT_DATA_B=int16) is currently unsupported");
    static_assert(!(std::is_same<TT_DATA_A, int16>::value && std::is_same<TT_DATA_B, cint32>::value),
                  "Matrix-vector mutliplcation of an int16 matrix (TT_DATA_A=int16) and a cint32 vector "
                  "(TT_DATA_B=cint32) is currently unsupported");
    static_assert(!(std::is_same<TT_DATA_A, int32>::value && std::is_same<TT_DATA_B, cint16>::value),
                  "Matrix-vector mutliplcation of an int32 matrix (TT_DATA_A=int32) and a cint16 vector "
                  "(TT_DATA_B=cint16) is currently unsupported");

    using TT_OUT = outType_t<TT_DATA_A, TT_DATA_B>;
    static constexpr int kSamplesVecA = 256 / 8 / sizeof(TT_DATA_A);
    static constexpr int kSamplesVecB = 256 / 8 / sizeof(TT_DATA_B);
    static constexpr int kSamplesVecOut = 256 / 8 / sizeof(TT_OUT);

    static constexpr int loadsPerCol = TP_DIM_A / kSamplesVecA;
    static constexpr int loadsPerMatrix = loadsPerCol * (TP_DIM_B / TP_CASC_LEN);
    static constexpr int loadsPerVectorB = (TP_DIM_B / TP_CASC_LEN) / kSamplesVecB;

    static constexpr int doubleAccPerCol = (kSamplesVecA / kSamplesVecOut == 2);
    static constexpr int shift = TP_SHIFT;

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
