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
FFT Window reference model
*/
#include "device_defs.h"
#include "cumsum_ref.hpp"
#include "fir_ref_utils.hpp"
#include "fft_ref_utils.hpp"
//#define _DSPLIB_CUMSUM_REF_DEBUG_

namespace xf {
namespace dsp {
namespace aie {
namespace cumsum {

// function blankSample - to return a 0 in any type
template <typename T_D>
constexpr T_D blankSample() {
    T_D retval;
    return retval;
};
template <>
constexpr int16 blankSample<int16>() {
    int16 temp;
    temp = 0;
    return temp;
};
template <>
constexpr cint16 blankSample<cint16>() {
    cint16 temp;
    temp.real = 0;
    temp.imag = 0;
    return temp;
};
template <>
constexpr int32 blankSample<int32>() {
    int32 temp;
    temp = 0;
    return temp;
};
template <>
constexpr cint32 blankSample<cint32>() {
    cint32 temp;
    temp.real = 0;
    temp.imag = 0;
    return temp;
};
template <>
constexpr float blankSample<float>() {
    float temp;
    temp = 0.0;
    return temp;
};
template <>
constexpr cfloat blankSample<cfloat>() {
    cfloat temp;
    temp.real = 0.0;
    temp.imag = 0.0;
    return temp;
};
#ifdef _SUPPORTS_BFLOAT16_
template <>
constexpr bfloat16 blankSample<bfloat16>() {
    bfloat16 temp;
    temp = 0.0;
    return temp;
};
#endif //_SUPPORTS_BFLOAT16_
#ifdef _SUPPORTS_CBFLOAT16_
template <>
constexpr cbfloat16 blankSample<cbfloat16>() {
    cbfloat16 temp;
    temp.real = 0.0;
    temp.imag = 0.0;
    return temp;
};
#endif //_SUPPORTS_CBFLOAT16_

template <typename T_D>
constexpr T_accRef<T_D> blankAcc() {
    T_accRef<T_D> retval;
    return retval;
};
template <>
constexpr T_accRef<int16> blankAcc<int16>() {
    T_accRef<int16> temp;
    temp.real = 0;
    return temp;
};
template <>
constexpr T_accRef<cint16> blankAcc<cint16>() {
    T_accRef<cint16> temp;
    temp.real = 0;
    temp.imag = 0;
    return temp;
};
template <>
constexpr T_accRef<int32> blankAcc<int32>() {
    T_accRef<int32> temp;
    temp.real = 0;
    return temp;
};
template <>
constexpr T_accRef<cint32> blankAcc<cint32>() {
    T_accRef<cint32> temp;
    temp.real = 0;
    temp.imag = 0;
    return temp;
};
template <>
constexpr T_accRef<float> blankAcc<float>() {
    T_accRef<float> temp;
    temp.real = 0.0;
    return temp;
};
template <>
constexpr T_accRef<cfloat> blankAcc<cfloat>() {
    T_accRef<cfloat> temp;
    temp.real = 0.0;
    temp.imag = 0.0;
    return temp;
};
#ifdef _SUPPORTS_BFLOAT16_
template <>
constexpr T_accRef<bfloat16> blankAcc<bfloat16>() {
    T_accRef<bfloat16> temp;
    temp.real = 0.0;
    return temp;
};
#endif //_SUPPORTS_BFLOAT16_
#ifdef _SUPPORTS_CBFLOAT16_
template <>
constexpr T_accRef<cbfloat16> blankAcc<cbfloat16>() {
    T_accRef<cbfloat16> temp;
    temp.real = 0.0;
    temp.imag = 0.0;
    return temp;
};
#endif //_SUPPORTS_CBFLOAT16_

// function add - to return a 0 in any type
template <typename T_D, typename T_DOUT>
T_accRef<T_DOUT> add(T_accRef<T_DOUT> dout, T_D din) {
    T_accRef<T_DOUT> retVal;
    retVal.real = dout.real + din;
    return retVal;
};
template <>
T_accRef<int16> add<int16, int16>(T_accRef<int16> dout, int16 din) {
    T_accRef<int16> retVal;
    retVal.real = dout.real + (int64)din;
    return retVal;
};
template <>
T_accRef<int32> add<int32, int32>(T_accRef<int32> dout, int32 din) {
    T_accRef<int32> retVal;
    retVal.real = dout.real + (int64)din;
    return retVal;
};
template <>
T_accRef<cint16> add<cint16, cint16>(T_accRef<cint16> dout, cint16 din) {
    T_accRef<cint16> retVal;
    retVal.real = dout.real + din.real;
    retVal.imag = dout.imag + din.imag;
    return retVal;
};
template <>
T_accRef<cint32> add<cint16, cint32>(T_accRef<cint32> dout, cint16 din) {
    T_accRef<cint32> retVal;
    retVal.real = dout.real + din.real;
    retVal.imag = dout.imag + din.imag;
    return retVal;
};
template <>
T_accRef<cint32> add<cint32, cint32>(T_accRef<cint32> dout, cint32 din) {
    T_accRef<cint32> retVal;
    retVal.real = dout.real + din.real;
    retVal.imag = dout.imag + din.imag;
    return retVal;
};
template <>
T_accRef<cfloat> add<cfloat, cfloat>(T_accRef<cfloat> dout, cfloat din) {
    T_accRef<cfloat> retVal;
    retVal.real = dout.real + din.real;
    retVal.imag = dout.imag + din.imag;
    return retVal;
};
#ifdef _SUPPORTS_BFLOAT16_
template <>
T_accRef<bfloat16> add<bfloat16, bfloat16>(T_accRef<bfloat16> dout, bfloat16 din) {
    T_accRef<bfloat16> retVal;
    retVal.real = dout.real + (float)din;
    return retVal;
};
#endif //_SUPPORTS_BFLOAT16_
#ifdef _SUPPORTS_CBFLOAT16_
template <>
T_accRef<cbfloat16> add<cbfloat16, cbfloat16>(T_accRef<cbfloat16> dout, cbfloat16 din) {
    T_accRef<cbfloat16> retVal;
    retVal.real = dout.real + (float)din.real;
    retVal.imag = dout.imag + (float)din.imag;
    return retVal;
};
#endif //_SUPPORTS_CBFLOAT16_

template <typename T_D>
T_accRef<T_D> upSize(T_D val) {
    T_accRef<T_D> retVal;
    retVal = (T_accRef<T_D>)val;
    return retVal;
};
template <>
T_accRef<cint16> upSize(cint16 val) {
    T_accRef<cint16> retVal;
    retVal.real = val.real;
    retVal.imag = val.imag;
    return retVal;
};
template <>
T_accRef<cint32> upSize(cint32 val) {
    T_accRef<cint32> retVal;
    retVal.real = val.real;
    retVal.imag = val.imag;
    return retVal;
};
#ifdef __SUPPORTS_CFLOAT__
template <>
T_accRef<cfloat> upSize(cfloat val) {
    T_accRef<cfloat> retVal;
    retVal.real = val.real;
    retVal.imag = val.imag;
    return retVal;
};
#endif //__SUPPORTS_CFLOAT__
#ifdef _SUPPORTS_CBFLOAT16_
template <>
T_accRef<cbfloat16> upSize(cbfloat16 val) {
    T_accRef<cbfloat16> retVal;
    retVal.real = val.real;
    retVal.imag = val.imag;
    return retVal;
};
#endif //_SUPPORTS_CBFLOAT16_

//---------------------------------------------------
// Cumsum - default/base 'specialization'
template <typename TT_DATA,
          typename TT_OUT_DATA,
          unsigned int TP_DIM_A,
          unsigned int TP_DIM_B,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_MODE,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_SAT>
void cumsum_ref<TT_DATA, TT_OUT_DATA, TP_DIM_A, TP_DIM_B, TP_NUM_FRAMES, TP_MODE, TP_SHIFT, TP_RND, TP_SAT>::
    cumsum_ref_main(input_buffer<TT_DATA>& inWindow0, output_buffer<TT_OUT_DATA>& outWindow0) {
    static constexpr int kMemWidth = __MAX_READ_WRITE__;
    static constexpr int kVectLen = kMemWidth / 8 / sizeof(TT_OUT_DATA);
    static constexpr int kinnerLoopLim = CEIL(TP_DIM_A, kVectLen);
    static constexpr int kMode2VectLen = 2 * kMemWidth / 8 / sizeof(TT_OUT_DATA);
    static constexpr int kMode2innerLoopLim = CEIL(TP_DIM_A, kMode2VectLen);

    TT_DATA d_in;
    TT_OUT_DATA d_out;
    TT_DATA* inIobBase = (TT_DATA*)inWindow0.data();
    TT_DATA* inPtr = (TT_DATA*)inWindow0.data();
    TT_OUT_DATA* outIobBase = (TT_OUT_DATA*)outWindow0.data();
    TT_OUT_DATA* outPtr = (TT_OUT_DATA*)outWindow0.data();
    TT_OUT_DATA* outPtrTrail = (TT_OUT_DATA*)outWindow0.data();
    T_accRef<TT_OUT_DATA> accum = blankAcc<TT_OUT_DATA>();
    T_accRef<TT_OUT_DATA> accum2 = blankAcc<TT_OUT_DATA>(); // needed because accum is altered during round/sat
    TT_OUT_DATA smallAccum;                                 // used to deliberately mimic overflow in uut for mode=2
    TT_OUT_DATA outData = blankSample<TT_OUT_DATA>();

    for (int frame = 0; frame < TP_NUM_FRAMES; frame++) {
        if (TP_MODE == 0) {
            for (unsigned int k = 0; k < TP_DIM_B; k++) {
                accum = blankAcc<TT_OUT_DATA>();
                for (unsigned int i = 0; i < kinnerLoopLim; i++) {
                    d_in = *inPtr++;
                    accum = add<TT_DATA, TT_OUT_DATA>(accum, d_in);
                    accum2 = accum;
                    roundAcc(TP_RND, TP_SHIFT, accum2);
                    saturateAcc(accum2, TP_SAT);
                    outData = castAcc<TT_OUT_DATA>(accum2);
                    *outPtr++ = outData;
                    //}
                }
            }
        } else if (TP_MODE == 1) { // TP_MODE=1 accumulate down second dimension
            for (unsigned int i = 0; i < kinnerLoopLim; i++) {
                accum = blankAcc<TT_OUT_DATA>();
                inPtr = &inIobBase[frame * TP_DIM_B * kinnerLoopLim + i];
                outPtr = &outIobBase[frame * TP_DIM_B * kinnerLoopLim + i];
                for (unsigned int k = 0; k < TP_DIM_B; k++) {
                    if (i >= TP_DIM_A) {
                        *outPtr = blankSample<TT_OUT_DATA>();
                    } else {
                        d_in = inPtr[kinnerLoopLim * k];
                        // printf("d_in = %d, ", d_in);
                        accum = add<TT_DATA, TT_OUT_DATA>(accum, d_in);
                        accum2 = accum;
                        // printf("raw = %d, ", accum2);
                        roundAcc(TP_RND, TP_SHIFT, accum2);
                        // printf("postround = %d, ", accum2);
                        saturateAcc(accum2, TP_SAT);
                        // printf("postsat = %d, ", accum2);
                        outData = castAcc<TT_OUT_DATA>(accum2);
                        // printf("out = %d\n ", outData);
                        outPtr[kinnerLoopLim * k] = outData;
                    }
                }
            }
        } else { // TP_MODE=2
            for (unsigned int k = 0; k < TP_DIM_B; k++) {
                accum = blankAcc<TT_OUT_DATA>();
                for (unsigned int i = 0; i < kMode2innerLoopLim; i++) {
                    d_in = *inPtr++;
                    accum = add<TT_DATA, TT_OUT_DATA>(accum, d_in);
                    // overflow occurs in the uut between the last sample of a vector and the seeding of the next
                    accum2 = accum;
                    if (i % kMode2VectLen == (kMode2VectLen - 1)) {
                        roundAcc(TP_RND, 0 /*TP_SHIFT*/, accum);
                        saturateAcc(accum, TP_SAT);
                        smallAccum = castAcc<TT_OUT_DATA>(accum); // which may overflow
                        accum = upSize<TT_OUT_DATA>(smallAccum);
                    }
                    roundAcc(TP_RND, 0 /*TP_SHIFT*/, accum2);
                    saturateAcc(accum2, TP_SAT);
                    outData = castAcc<TT_OUT_DATA>(accum2);
                    *outPtr++ = outData;
                    //}
                }
            }
        }
    }
};
}
}
}
}
