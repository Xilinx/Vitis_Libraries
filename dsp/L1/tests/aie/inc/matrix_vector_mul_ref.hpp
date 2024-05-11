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
#ifndef _DSPLIB_MATRIX_VECTOR_MUL_REF_HPP_
#define _DSPLIB_MATRIX_VECTOR_MUL_REF_HPP_

/*
MATRIX_VECTOR_MUL single channel reference model
*/

#ifndef _DSPLIB_MATRIX_VECTOR_MUL_REF_DEBUG_
//#define _DSPLIB_MATRIX_VECTOR_MUL_REF_DEBUG_
#endif //_DSPLIB_MATRIX_VECTOR_MUL_REF_DEBUG_

#include <adf.h>
#include <limits>

using namespace adf;

namespace xf {
namespace dsp {
namespace aie {
namespace blas {
namespace matrix_vector_mul {

#ifndef ROW_MAJOR
#define ROW_MAJOR 0
#endif // ROW_MAJOR
#ifndef COL_MAJOR
#define COL_MAJOR 1
#endif // COL_MAJOR

template <typename T_A, typename T_B>
struct outType {
    using type = cint16;
};
template <>
struct outType<int16, int16> {
    using type = int16;
};
template <>
struct outType<int16, cint16> {
    using type = cint16;
};
template <>
struct outType<int16, cint32> {
    using type = cint32;
};
template <>
struct outType<int16, int32> {
    using type = int32;
};
template <>
struct outType<cint16, int16> {
    using type = cint16;
};
template <>
struct outType<cint16, cint16> {
    using type = cint16;
};
template <>
struct outType<cint16, int32> {
    using type = cint32;
};
template <>
struct outType<cint16, cint32> {
    using type = cint32;
};
template <>
struct outType<int32, int16> {
    using type = int32;
};
template <>
struct outType<int32, cint16> {
    using type = cint32;
};
template <>
struct outType<int32, int32> {
    using type = int32;
};
template <>
struct outType<int32, cint32> {
    using type = cint32;
};
template <>
struct outType<cint32, int16> {
    using type = cint32;
};
template <>
struct outType<cint32, cint16> {
    using type = cint32;
};
template <>
struct outType<cint32, int32> {
    using type = cint32;
};
template <>
struct outType<cint32, cint32> {
    using type = cint32;
};
template <>
struct outType<float, float> {
    using type = float;
};
template <>
struct outType<cfloat, float> {
    using type = cfloat;
};
template <>
struct outType<float, cfloat> {
    using type = cfloat;
};
template <>
struct outType<cfloat, cfloat> {
    using type = cfloat;
};
template <typename T_D_A, typename T_D_B>
using outType_t = typename outType<T_D_A, T_D_B>::type;

//-----------------------------------------------------------------------------------------------------
// MATRIX_VECTOR_MUL single channel reference model class
template <typename TT_DATA_A,
          typename TT_DATA_B,
          unsigned int TP_DIM_A,
          unsigned int TP_DIM_B,
          unsigned int TP_DIM_A_LEADING,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_CASC_LEN>
class matrix_vector_mul_ref {
   private:
    static constexpr int windowSizeA = TP_NUM_FRAMES * TP_DIM_A * TP_DIM_B;
    static constexpr int windowSizeB = TP_NUM_FRAMES * TP_DIM_B;
    static constexpr int windowSizeOut = TP_NUM_FRAMES * TP_DIM_A;

   public:
    // Constructor
    matrix_vector_mul_ref() {}
    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(matrix_vector_mul_ref::matrix_vector_mul); }
    // FFT
    void matrix_vector_mul(input_buffer<TT_DATA_A>& inWindowA,
                           input_buffer<TT_DATA_B>& inWindowB,
                           output_buffer<outType_t<TT_DATA_A, TT_DATA_B> >& outWindow);
};
}
}
}
}
} // namespace closing braces

#endif // _DSPLIB_MATRIX_VECTOR_MUL_REF_HPP_
