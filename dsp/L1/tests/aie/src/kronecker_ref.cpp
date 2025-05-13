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
/*
Kronecker reference model
*/
#include "device_defs.h"
#include "aie_api/aie_adf.hpp"
#include "kronecker_ref.hpp"

// #define _DSPLIB_KRONECKER_REF_DEBUG_

namespace xf {
namespace dsp {
namespace aie {
namespace kronecker {

// Generic implementation of a multiplier
template <typename T_A, typename T_B>
outTypeMult_t<T_A, T_B> scalar_mult(T_A a, T_B b, const int shift, unsigned int t_rnd, unsigned int t_sat) {
    using out_t = outTypeMult_t<T_A, T_B>;
    out_t ret_val;
    T_accRef<T_A> aAcc;
    T_accRef<T_B> bAcc;
    T_accRef<out_t> outAcc;
    aAcc = val_accRef(a);
    bAcc = val_accRef(b);
    outAcc.real = ((int64)aAcc.real * (int64)bAcc.real) - ((int64)aAcc.imag * (int64)bAcc.imag);
    outAcc.imag = ((int64)aAcc.real * (int64)bAcc.imag) + ((int64)aAcc.imag * (int64)bAcc.real);
    roundAcc(t_rnd, shift, outAcc);
    saturateAcc(outAcc, t_sat);
    ret_val = castAcc(outAcc);

    return ret_val;
}

template <>
float scalar_mult(float a, float b, const int shift, unsigned int t_rnd, unsigned int t_sat) {
    float ret_val;
    T_accRef<float> aAcc;
    T_accRef<float> bAcc;
    T_accRef<float> outAcc;
    aAcc = val_accRef(a);
    bAcc = val_accRef(b);
    outAcc.real = ((float)aAcc.real * (float)bAcc.real);
    ret_val = castAcc(outAcc);

    return ret_val;
}

template <>
cfloat scalar_mult(float a, cfloat b, const int shift, unsigned int t_rnd, unsigned int t_sat) {
    cfloat ret_val;
    T_accRef<float> aAcc;
    T_accRef<cfloat> bAcc;
    T_accRef<cfloat> outAcc;
    aAcc = val_accRef(a);
    bAcc = val_accRef(b);
    outAcc.real = ((float)aAcc.real * (float)bAcc.real);
    outAcc.imag = ((float)aAcc.real * (float)bAcc.imag);
    ret_val = castAcc(outAcc);

    return ret_val;
}

template <>
cfloat scalar_mult(cfloat a, float b, const int shift, unsigned int t_rnd, unsigned int t_sat) {
    cfloat ret_val;
    T_accRef<cfloat> aAcc;
    T_accRef<float> bAcc;
    T_accRef<cfloat> outAcc;
    aAcc = val_accRef(a);
    bAcc = val_accRef(b);
    outAcc.real = ((float)aAcc.real * (float)bAcc.real);
    outAcc.imag = ((float)aAcc.imag * (float)bAcc.real);
    ret_val = castAcc(outAcc);

    return ret_val;
}

template <>
cfloat scalar_mult(cfloat a, cfloat b, const int shift, unsigned int t_rnd, unsigned int t_sat) {
    cfloat ret_val;
    T_accRef<cfloat> aAcc;
    T_accRef<cfloat> bAcc;
    T_accRef<cfloat> outAcc;
    aAcc = val_accRef(a);
    bAcc = val_accRef(b);
    outAcc.real = ((float)aAcc.real * (float)bAcc.real) - ((float)aAcc.imag * (float)bAcc.imag);
    outAcc.imag = ((float)aAcc.real * (float)bAcc.imag) + ((float)aAcc.imag * (float)bAcc.real);
    ret_val = castAcc(outAcc);

    return ret_val;
}

// Kronecker - default/base 'specialization'
template <typename TT_DATA_A,
          typename TT_DATA_B,
          unsigned int TP_DIM_A_ROWS,
          unsigned int TP_DIM_A_COLS,
          unsigned int TP_DIM_B_ROWS,
          unsigned int TP_DIM_B_COLS,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_API,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_SAT>
void kronecker_ref<TT_DATA_A,
                   TT_DATA_B,
                   TP_DIM_A_ROWS,
                   TP_DIM_A_COLS,
                   TP_DIM_B_ROWS,
                   TP_DIM_B_COLS,
                   TP_NUM_FRAMES,
                   TP_API,
                   TP_SHIFT,
                   TP_RND,
                   TP_SAT>::kronecker_main(input_buffer<TT_DATA_A>& inWindow0,
                                           input_buffer<TT_DATA_B>& inWindow1,
                                           output_buffer<out_t>& outWindow0) {
    TT_DATA_A* ptrInWindow0 = (TT_DATA_A*)inWindow0.data();
    TT_DATA_B* ptrInWindow1 = (TT_DATA_B*)inWindow1.data();
    out_t* ptrOutWindow0 = (out_t*)outWindow0.data();
    TT_DATA_A inDataA;
    TT_DATA_B inDataB;
    out_t outData;

    for (int frameIndex = 0; frameIndex < TP_NUM_FRAMES; frameIndex++) {
        for (int i = 0; i < TP_DIM_A_COLS; i++) {
            for (int j = 0; j < TP_DIM_A_ROWS; j++) {
                for (int k = 0; k < TP_DIM_B_COLS; k++) {
                    for (int l = 0; l < TP_DIM_B_ROWS; l++) {
                        int outIndex = ((i * TP_DIM_B_COLS) + k) * rowsMatOut + (j * TP_DIM_B_ROWS) + l +
                                       (frameIndex * sizeMatOut);
                        // indices of matrix A and B
                        int indexA = (i * TP_DIM_A_ROWS + j) + (frameIndex * sizeMatA);
                        int indexB = (k * TP_DIM_B_ROWS + l) + (frameIndex * sizeMatB);
                        inDataA = ptrInWindow0[indexA];
                        inDataB = ptrInWindow1[indexB];
                        outData = scalar_mult<TT_DATA_A, TT_DATA_B>(inDataA, inDataB, TP_SHIFT, TP_RND, TP_SAT);
                        ptrOutWindow0[outIndex] = outData;
                    }
                }
            }
        }
    };
}

} // namespace kronecker
} // namespace aie
} // namespace xf
} // namespace dsp
