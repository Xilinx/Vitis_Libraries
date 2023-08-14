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
          unsigned int TP_NUM_FRAMES>
void dft_ref<TT_DATA, TT_TWIDDLE, TP_POINT_SIZE, TP_FFT_NIFFT, TP_SHIFT, TP_NUM_FRAMES>::dftMatrixVectorMult(
    input_buffer<TT_DATA>& inWindow, output_buffer<T_outDataType>& outWindow) {
    const unsigned int shift = 15 + TP_SHIFT;
    const unsigned int round_const = (1 << (shift - 1));

    typedef typename std::conditional_t<
        std::is_same<TT_DATA, int16>::value, cint16,
        std::conditional_t<std::is_same<TT_DATA, int32>::value, cint32,
                           std::conditional_t<std::is_same<TT_DATA, float>::value, cfloat, TT_DATA> > >
        T_outDataType;
    static constexpr int kSamplesInVectOutData = 256 / 8 / sizeof(T_outDataType);
    static constexpr int kSamplesInVectTwiddle = kSamplesInVectOutData; // 256 / 8 / sizeof(TT_TWIDDLE);
    static constexpr int kSamplesInVectData = 256 / 8 / sizeof(TT_DATA);

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

    for (unsigned int frame = 0; frame < TP_NUM_FRAMES; frame++) {
        for (unsigned int i = 0; i < paddedCoeffSize; i++) {
            // window_readincr(inWindow, sampleIn);
            sampleIn = *inPtr++;
            input[i] = castInput<TT_DATA>(sampleIn);
            if (i > TP_POINT_SIZE) {
                input[i] = {0, 0};
            }
        }
        if (std::is_same<TT_TWIDDLE, cint16>::value) {
            for (int i = 0; i < TP_POINT_SIZE; ++i) {
                temp[i].real = 0;
                temp[i].imag = 0;
                for (int j = 0; j < TP_POINT_SIZE; ++j) {
                    temp[i].real += (coeff[i][j].real * input[j].real) - (coeff[i][j].imag * input[j].imag);
                    temp[i].imag +=
                        (coeff[i][j].real * input[j].imag) + (coeff[i][j].imag * input[j].real); // Rearrage the input
                }
            }
            for (int i = 0; i < paddedCoeffSize; ++i) {
                // Rounding occurs here, saturation occurs in castOutput.
                output[i].real = (temp[i].real + (int64)round_const) >> shift;
                output[i].imag = (temp[i].imag + (int64)round_const) >> shift;
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
                        (coeff[i][j].real * input[j].imag) + (coeff[i][j].imag * input[j].real); // Rearrage the input
                }
            }

            for (int i = 0; i < paddedCoeffSize; ++i) {
                // Rounding occurs here, saturation occurs in castOutput.
                output[i].real = tmp[i].real;
                output[i].imag = tmp[i].imag;
                if (i >= TP_POINT_SIZE) {
                    output[i] = {0, 0};
                }
            }
        }
        // Write samples out (natural order)
        for (unsigned int i = 0; i < paddedCoeffSize; i++) {
            // window_writeincr((output_window<T_outDataType> *)outWindow,
            // castOutput<T_outDataType>(output[i],kScaleFactor));
            *outPtr++ = castOutput<T_outDataType>(output[i], kScaleFactor);
        }
    }
};
}
}
}
}
}
