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
#ifndef MATRIX_MULT_REF_HPP
#define MATRIX_MULT_REF_HPP

// This file holds the definition header of thematrix mult reference model graph class

#include <adf.h>
#include <vector>

#include "matrix_mult_ref.hpp"
#include "fir_ref_utils.hpp"

namespace xf {
namespace dsp {
namespace aie {
namespace blas {
namespace matrix_mult {
using namespace adf;
// The following struct returns the output type for a given input data type combination
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

template <typename TT_DATA_A,
          typename TT_DATA_B,
          unsigned int TP_DIM_A,
          unsigned int TP_DIM_AB,
          unsigned int TP_DIM_B,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_DIM_A_LEADING = ROW_MAJOR,
          unsigned int TP_DIM_B_LEADING = COL_MAJOR,
          unsigned int TP_DIM_OUT_LEADING = ROW_MAJOR,
          unsigned int TP_ADD_TILING_A = 1,     // not used - just to match UUT.
          unsigned int TP_ADD_TILING_B = 1,     // not used - just to match UUT.
          unsigned int TP_ADD_DETILING_OUT = 1, // not used - just to match UUT.
          unsigned int TP_INPUT_WINDOW_VSIZE_A = TP_DIM_A* TP_DIM_AB,
          unsigned int TP_INPUT_WINDOW_VSIZE_B = TP_DIM_B* TP_DIM_AB,
          unsigned int TP_CASC_LEN = 1, // not used - just to match UUT.
          unsigned int TP_SAT = 1,
          unsigned int TP_SSR = 1,
          typename TT_OUT_DATA = outType_t<TT_DATA_A, TT_DATA_B> >
class matrix_mult_ref_graph : public graph {
   public:
    port<input> inA[1];
    port<input> inB[1];
    port<output> out[1];

    // FIR Kernel
    kernel m_firKernel;

    // Constructor
    matrix_mult_ref_graph() {
        printf("===========================\n");
        printf("==    MATRIX MULT REF   == \n");
        printf("===========================\n");

        // Create FIR class
        m_firKernel = kernel::create_object<
            matrix_mult_ref<TT_DATA_A, TT_DATA_B, TT_OUT_DATA, TP_DIM_A, TP_DIM_AB, TP_DIM_B, TP_SHIFT, TP_RND, TP_SAT,
                            TP_DIM_A_LEADING, TP_DIM_B_LEADING, TP_DIM_OUT_LEADING, TP_INPUT_WINDOW_VSIZE_A,
                            TP_INPUT_WINDOW_VSIZE_B> >();
        printf("Created object");
        // Make connections
        // Size of window in Bytes.
        connect<>(inA[0], m_firKernel.in[0]);
        dimensions(m_firKernel.in[0]) = {TP_INPUT_WINDOW_VSIZE_A};
        connect<>(inB[0], m_firKernel.in[1]);
        dimensions(m_firKernel.in[1]) = {TP_INPUT_WINDOW_VSIZE_B};
        connect<>(m_firKernel.out[0], out[0]);
        dimensions(m_firKernel.out[0]) = {(TP_INPUT_WINDOW_VSIZE_A / TP_DIM_AB) *
                                          (TP_INPUT_WINDOW_VSIZE_B / TP_DIM_AB)};
        // Specify mapping constraints
        runtime<ratio>(m_firKernel) = 0.4;
        // Source files
        source(m_firKernel) = "matrix_mult_ref.cpp";
    };
};
}
}
}
}
}
#endif // MATRIX_MULT_REF_HPP
