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
#include <cmath>
#include <float.h>
#include <stdio.h>
#include <stdlib.h>
#include "dynamic_dft_ref.hpp"
#include "fir_ref_utils.hpp" // for saturateAcc
#include "device_defs.h"

namespace xf {
namespace dsp {
namespace aie {
namespace fft {
namespace mixed_radix_fft {

// Example test for dft matrix vector multiply
//-----------------------------------------------------------------------------------------------------
template <typename TT_DATA,
          typename TT_TWIDDLE,        // type of coefficients           (e.g. int16, cint32)
          unsigned int TP_POINT_SIZE, // maximum pointsize
          unsigned int TP_FFT_NIFFT,
          unsigned int TP_SHIFT,
          unsigned int TP_WINDOW_VSIZE,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_SSR>
void dynamic_dft_ref<TT_DATA,
                     TT_TWIDDLE,
                     TP_POINT_SIZE,
                     TP_FFT_NIFFT,
                     TP_SHIFT,
                     TP_WINDOW_VSIZE,
                     TP_RND,
                     TP_SAT,
                     TP_SSR>::dynDftMatrixVectorMult(input_buffer<TT_DATA>& inWindow,
                                                     input_buffer<TT_DATA>& headerInWindow,
                                                     output_buffer<TT_DATA>& outWindow,
                                                     output_buffer<TT_DATA>& headerOutWindow) {
    const unsigned int shift = 15 + TP_SHIFT;

#ifdef __SUPPORTS_ACC64__
    static constexpr int kSamplesInVectData = 8;
#else
    static constexpr int kSamplesInVectData = 256 / 8 / sizeof(TT_DATA);
#endif //__SUPPORTS_ACC64__

    static constexpr int kSamplesInVectTwiddle = kSamplesInVectData; // 256 / 8 / sizeof(TT_TWIDDLE);
    static constexpr int paddedCoeffSize = TP_POINT_SIZE;
    static constexpr int frameSizeWithPad = CEIL(TP_POINT_SIZE, (TP_SSR * kSamplesInVectTwiddle));

    TT_DATA sampleIn;
    TT_TWIDDLE twidAct;
    T_int_data<TT_DATA> input[TP_POINT_SIZE];

    constexpr unsigned int kScaleFactor = TP_SHIFT; // was kPtSizePwr -1; //1 is for initial rotation factor of
                                                    // 1/sqrt(2), but with TP_SHIFT this is user-config

    T_int_data<TT_DATA> output[paddedCoeffSize];
    cfloat outRaw[paddedCoeffSize];
    cfloat tmpTwidFloat;
    TT_DATA* inPtr = (TT_DATA*)inWindow.data();
    TT_DATA* headerInPtr = (TT_DATA*)headerInWindow.data();
    TT_DATA* outPtr = (TT_DATA*)outWindow.data();
    TT_DATA* headerOutPtr = (TT_DATA*)headerOutWindow.data();
    T_accRef<TT_DATA> outRawInt;

    // Load header input data (same for all frames in this iteration)
    TT_DATA readVal = *headerInPtr++;
    int invInt = readVal.real;
    *headerOutPtr++ = readVal;

    readVal = *headerInPtr++;
    int nR2 = readVal.real;
    int nR3 = readVal.imag;
    *headerOutPtr++ = readVal;

    readVal = *headerInPtr++;
    int nR5 = readVal.real;
    *headerOutPtr++ = readVal;
    printf("R2s: %d , R3s %d , R5s %d\n", nR2, nR3, nR5);
    int headerPadSize = (sizeof(TT_DATA) == 4) ? 8 : 4; // size
    for (unsigned int k = 3; k < headerPadSize - 1;
         k++) { // headerPadSize-1 since errorFlag needs to be added in final entry
        *headerOutPtr++ = {0, 0};
    }

    // Calculate pointsize (same for all frames in this iteration)
    int iter_pointsize = int(pow(2.0, double(nR2)) * pow(3.0, double(nR3)) * pow(5.0, double(nR5)));
    printf("iter_pointsize %d \n", iter_pointsize);

    // add errorFlag
    int in_range = (iter_pointsize <= TP_POINT_SIZE) ? 1 : 0;
    int multiple_eight = (nR2 >= 3) ? 1 : 0;
    int errorFlag = (in_range && multiple_eight) ? 0 : 1;
    *headerOutPtr++ = {0, static_cast<short>(errorFlag)};

    // Form twiddle table for this point size;
    double inv = (invInt == 1) ? -1.0 : 1.0;
    int block_size = 32 / sizeof(TT_DATA);                                   // 32 bytes / bits of datatype
    int num_frames = int(TP_WINDOW_VSIZE / CEIL(TP_POINT_SIZE, block_size)); // floor value
    // Perform DFT using twiddles/coefficients
    for (unsigned int frame = 0; frame < num_frames; frame++) {
        // Load input data
        for (int point = 0; point < TP_POINT_SIZE; point++) {
            sampleIn = *inPtr++;
            input[point] = castInput<TT_DATA>(sampleIn);
        }

        for (int i = 0; i < TP_POINT_SIZE; ++i) {
            outRaw[i].real = 0;
            outRaw[i].imag = 0;
            for (int j = 0; j < TP_POINT_SIZE; ++j) {
                if (i < iter_pointsize && j < iter_pointsize) {
                    tmpTwidFloat.real = (cos(M_PI * 2.0 * (double)i * (double)j / (double)iter_pointsize));
                    tmpTwidFloat.imag = (inv * sin(M_PI * 2.0 * (double)i * (double)j / (double)iter_pointsize));

                    if (std::is_same<TT_TWIDDLE, cfloat>::value) {
                        twidAct.real = tmpTwidFloat.real;
                        twidAct.imag = tmpTwidFloat.imag;
                    } else {
                        tmpTwidFloat.real = (tmpTwidFloat.real * 32768.0);
                        tmpTwidFloat.imag = (tmpTwidFloat.imag * 32768.0);
                        if (tmpTwidFloat.real >= 32767.0) {
                            tmpTwidFloat.real = 32767.0;
                        }
                        if (tmpTwidFloat.imag >= 32767.0) {
                            tmpTwidFloat.imag = 32767.0;
                        }
                        twidAct.real = (int16)tmpTwidFloat.real;
                        twidAct.imag = (int16)tmpTwidFloat.imag;
                    }
                } else {
                    twidAct.real = 0;
                    twidAct.imag = 0;
                }
                outRaw[i].real += (twidAct.real * input[j].real) - (twidAct.imag * input[j].imag);
                outRaw[i].imag +=
                    (twidAct.real * input[j].imag) + (twidAct.imag * input[j].real); // Re-arrange the input
            }
            if (std::is_same<TT_TWIDDLE, cfloat>::value) {
                output[i].real = outRaw[i].real;
                output[i].imag = outRaw[i].imag;
            } else {
                outRawInt.real = (int64)outRaw[i].real;
                outRawInt.imag = (int64)outRaw[i].imag;
                roundAcc(TP_RND, shift, outRawInt);
                saturateAcc(outRawInt, TP_SAT);
                output[i].real = (int32)outRawInt.real;
                output[i].imag = (int32)outRawInt.imag;
            }
        }
        // Write samples out (natural order) and pad output frame if necessary
        for (unsigned int k = 0; k < frameSizeWithPad; k++) {
            if (k >= TP_POINT_SIZE) {
                output[k].real = 0;
                output[k].imag = 0;
            }
            *outPtr++ = castOutput<TT_DATA>(output[k]);
        }
    }
};
}
}
}
}
}
