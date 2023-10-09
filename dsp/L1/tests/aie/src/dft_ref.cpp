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
#include <math.h>
#include <float.h>
#include <stdio.h>
#include <stdlib.h>
#include "dft_ref.hpp"
#include "fir_ref_utils.hpp" // for saturateAcc
#include "device_defs.h"

namespace xf {
namespace dsp {
namespace aie {
namespace fft {
namespace dft {

// Example test for dft matrix vector multiply
//-----------------------------------------------------------------------------------------------------
template <typename TT_DATA,
          typename TT_TWIDDLE, // type of coefficients           (e.g. int16, cint32)
          unsigned int TP_POINT_SIZE,
          unsigned int TP_FFT_NIFFT,
          unsigned int TP_SHIFT,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_RND,
          unsigned int TP_SAT>
void dft_ref<TT_DATA, TT_TWIDDLE, TP_POINT_SIZE, TP_FFT_NIFFT, TP_SHIFT, TP_NUM_FRAMES, TP_RND, TP_SAT>::
    dftMatrixVectorMult(input_buffer<TT_DATA>& inWindow, output_buffer<T_outDataType>& outWindow) {
    const unsigned int shift = 15 + TP_SHIFT;
    typedef typename std::conditional_t<
        std::is_same<TT_DATA, int16>::value, cint16,
        std::conditional_t<std::is_same<TT_DATA, int32>::value, cint32,
                           std::conditional_t<std::is_same<TT_DATA, float>::value, cfloat, TT_DATA> > >
        T_outDataType;

#ifdef __SUPPORTS_ACC64__
    static constexpr int kSamplesInVectOutData = 8;
    static constexpr int kSamplesInVectData = 8;
#else
    static constexpr int kSamplesInVectOutData = 256 / 8 / sizeof(T_outDataType);
    static constexpr int kSamplesInVectData = 256 / 8 / sizeof(TT_DATA);
#endif //__SUPPORTS_ACC64__

    static constexpr int kSamplesInVectTwiddle = kSamplesInVectOutData; // 256 / 8 / sizeof(TT_TWIDDLE);
    static constexpr int paddedCoeffSize = CEIL(TP_POINT_SIZE, kSamplesInVectTwiddle);
    static constexpr int paddedDataSize = CEIL(TP_POINT_SIZE, kSamplesInVectData);
    static constexpr int windowSize = paddedDataSize * TP_NUM_FRAMES;

    TT_DATA sampleIn;
    T_int_data<TT_DATA> chess_storage(% chess_alignof(cint16)) input[paddedCoeffSize];

    constexpr unsigned int kScaleFactor = TP_SHIFT; // was kPtSizePwr -1; //1 is for initial rotation factor of
                                                    // 1/sqrt(2), but with TP_SHIFT this is user-config

    T_int_data<T_outDataType> chess_storage(% chess_alignof(cint16)) output[paddedDataSize];
    cint64 temp[paddedCoeffSize];
    cfloat tmp[paddedCoeffSize];
    TT_DATA* inPtr = (TT_DATA*)inWindow.data();
    T_outDataType* outPtr = (TT_DATA*)outWindow.data();
    T_accRef<TT_DATA> tempOutput;

    for (unsigned int frame = 0; frame < TP_NUM_FRAMES; frame++) {
        for (unsigned int i = 0; i < paddedCoeffSize; i++) {
            // window_readincr(inWindow, sampleIn);
            sampleIn = *inPtr++;
            input[i] = castInput<TT_DATA>(sampleIn);
            if (i > TP_POINT_SIZE) {
                input[i] = {0, 0};
            }
        }
        if (std::is_same<TT_TWIDDLE, cint16>::value) { // this caters for rounding and saturation not being applied to
                                                       // cfloat-cfloat cases.
            for (int i = 0; i < paddedCoeffSize; ++i) {
                temp[i].real = 0;
                temp[i].imag = 0;
                for (int j = 0; j < TP_POINT_SIZE; ++j) {
                    temp[i].real += (coeff[i][j].real * input[j].real) - (coeff[i][j].imag * input[j].imag);
                    temp[i].imag +=
                        (coeff[i][j].real * input[j].imag) + (coeff[i][j].imag * input[j].real); // Re-arrange the input
                }
            }
            for (int i = 0; i < paddedCoeffSize; ++i) {
                tempOutput.real = (int64)temp[i].real;
                tempOutput.imag = (int64)temp[i].imag;
                roundAcc(TP_RND, shift, tempOutput);
                saturateAcc(tempOutput, TP_SAT);
                output[i].real = (int32)tempOutput.real;
                output[i].imag = (int32)tempOutput.imag;

                if (i >= TP_POINT_SIZE) {
                    output[i] = {0, 0};
                }
            }
        } else {
            for (int i = 0; i < TP_POINT_SIZE; ++i) {
                tmp[i].real = 0;
                tmp[i].imag = 0;

                for (int j = 0; j < TP_POINT_SIZE; ++j) {
                    tmp[i].real += (coeff[i][j].real * input[j].real) - (coeff[i][j].imag * input[j].imag);
                    tmp[i].imag +=
                        (coeff[i][j].real * input[j].imag) + (coeff[i][j].imag * input[j].real); // Re-arrange the input
                }
            }

            for (int i = 0; i < paddedCoeffSize; ++i) {
                output[i].real = tmp[i].real;
                output[i].imag = tmp[i].imag;

                if (i >= TP_POINT_SIZE) {
                    output[i] = {0, 0};
                }
            }
        }
        // Write samples out (natural order)
        for (unsigned int i = 0; i < paddedCoeffSize; i++) {
            *outPtr++ = castOutput<T_outDataType>(output[i]);
        }
    }
};
}
}
}
}
}
