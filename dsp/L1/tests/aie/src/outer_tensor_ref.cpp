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
/*
Outer Tensor Product reference model
*/
#include "device_defs.h"
#include "outer_tensor_ref.hpp"
#include "outer_tensor_ref_utils.hpp"
#include "aie_api/aie_adf.hpp"
#include "fir_ref_utils.hpp"

// #define _DSPLIB_OUTER_TENSOR_REF_DEBUG_

namespace xf {
namespace dsp {
namespace aie {
namespace outer_tensor {
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

// Outer Tensor Product - default/base 'specialization' for both static and dynamic point size
template <typename TT_DATA_A,
          typename TT_DATA_B,
          unsigned int TP_DIM_A,
          unsigned int TP_DIM_B,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_SHIFT,
          unsigned int TP_API,
          unsigned int TP_SSR,
          unsigned int TP_RND,
          unsigned int TP_SAT>
void outer_tensor_ref<TT_DATA_A,
                      TT_DATA_B,
                      TP_DIM_A,
                      TP_DIM_B,
                      TP_NUM_FRAMES,
                      TP_SHIFT,
                      TP_API,
                      TP_SSR,
                      TP_RND,
                      TP_SAT>::outer_tensor_main(input_buffer<TT_DATA_A>& inWindowA,
                                                 input_buffer<TT_DATA_B>& inWindowB,
                                                 output_buffer<outTypeMult_t<TT_DATA_A, TT_DATA_B> >& outWindow0) {
    using out_t = outTypeMult_t<TT_DATA_A, TT_DATA_B>;
    TT_DATA_A dA_in;
    TT_DATA_B dB_in;
    out_t d_out;

    unsigned int outDim = TP_DIM_A * TP_DIM_B;

    TT_DATA_A* inPtrA = (TT_DATA_A*)inWindowA.data();
    TT_DATA_B* inPtrB = (TT_DATA_B*)inWindowB.data();
    out_t* outPtr = (out_t*)outWindow0.data();

    // Processing of one window
    for (int frame = 0; frame < TP_NUM_FRAMES; frame++) {
        for (unsigned int i = 0; i < TP_DIM_A; i++) {
            dA_in = inPtrA[i];
            for (unsigned int j = 0; j < TP_DIM_B; j++) {
                dB_in = inPtrB[j];
                d_out = scalar_mult<TT_DATA_A, TT_DATA_B>(dA_in, dB_in, TP_SHIFT, TP_RND, TP_SAT);
                *outPtr++ = d_out;
            }
        }
        inPtrA += TP_DIM_A;
        inPtrB += TP_DIM_B;
    }
};
}
}
}
} // closing namespace xf::dsp::aie::outer_tensor
