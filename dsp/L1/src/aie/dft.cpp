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

/*
DFT kernal code.
This file captures the body of run-time code for the kernal class.
Coding conventions
  TT_      template type suffix
  TP_      template parameter suffix
*/
//#include <adf.h>
#include <stdio.h>
using namespace std;
#include "device_defs.h"
// if we use 1kb registers -> aie api uses 2x512b registers for 1024b so we need this for QoR
#define __AIE_API_USE_NATIVE_1024B_VECTOR__
#include "aie_api/aie_adf.hpp"
#include "dft.hpp"
#include "kernel_api_utils.hpp"

// #define _DSPLIB_DFT_HPP_DEBUG_

namespace xf {
namespace dsp {
namespace aie {
namespace fft {
namespace dft {

// DFT single channel function - base of specialization .
//-----------------------------------------------------------------------------------------------------
template <typename TT_DATA,
          typename TT_TWIDDLE,
          unsigned int TP_POINT_SIZE,
          unsigned int TP_FFT_NIFFT,
          unsigned int TP_SHIFT,
          unsigned int TP_CASC_LEN,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_KERNEL_POSITION,
          bool TP_CASC_IN,
          bool TP_CASC_OUT,
          unsigned int TP_RND,
          unsigned int TP_SAT>
INLINE_DECL void kernelDFTClass<TT_DATA,
                                TT_TWIDDLE,
                                TP_POINT_SIZE,
                                TP_FFT_NIFFT,
                                TP_SHIFT,
                                TP_CASC_LEN,
                                TP_NUM_FRAMES,
                                TP_KERNEL_POSITION,
                                TP_CASC_IN,
                                TP_CASC_OUT,
                                TP_RND,
                                TP_SAT>::kernelDFT(T_inputIF<TP_CASC_IN, TT_DATA, TT_TWIDDLE> inInterface,
                                                   TT_TWIDDLE* __restrict coeffPtr,
                                                   T_outputIF<TP_CASC_OUT, T_outDataType, TT_TWIDDLE> outInterface) {
    set_rnd_mode<TP_RND>();
    set_sat_mode<TP_SAT>();

    using dataVect_t = ::aie::vector<TT_DATA, kSamplesInVectData>;
    using outDataVect_t = ::aie::vector<T_outDataType, kSamplesInVectData>;
    using coeffVect_t = ::aie::vector<TT_TWIDDLE, kSamplesInVectTwiddle>;

    using accVect_t = ::aie::accum<typename tAccBaseType<T_outDataType, TT_TWIDDLE>::type, kSamplesInVectData>;

    dataVect_t dataVect;
    dataVect_t* inPointer;
    outDataVect_t blankVect = ::aie::zeros<T_outDataType, kSamplesInVectData>(); // to initialise acc
    outDataVect_t outVect, outVect2;
    coeffVect_t* coeffVectPtr;

    coeffVect_t coeffVect, coeffVect2;
    accVect_t acc, acc2;
    dataVect_t* frameStart = (dataVect_t*)inInterface.inWindow;
    outDataVect_t* outPtr = (dataVect_t*)outInterface.outWindow;

    // Loop through each frame
    for (int frame = 0; frame < TP_NUM_FRAMES; frame++) chess_prepare_for_pipelining chess_loop_count(TP_NUM_FRAMES) {
            // pointer to a vector of coefficients
            coeffVectPtr = (coeffVect_t*)(coeffPtr);
            // #pragma unroll kPairsInCoeff
            for (unsigned int vectCoeff = 0; vectCoeff < kPairsInCoeff; vectCoeff++)
                chess_prepare_for_pipelining chess_loop_count(kPairsInCoeff) {
                    // get acc data, or initialize if first or only kernel in cascade
                    if
                        constexpr(TP_CASC_IN == CASC_IN_TRUE) {
                            acc = (accVect_t)readincr_v<kSamplesInVectData>(inInterface.inCascade);
                            acc2 = (accVect_t)readincr_v<kSamplesInVectData>(inInterface.inCascade);
                        }
                    else {
                        acc = (accVect_t)blankVect;  // Initialize if one kernel used or the first used
                        acc2 = (accVect_t)blankVect; // Initialize if one kernel used or the first used
                    }

                    inPointer = frameStart;
#pragma unroll(kSamplesInVectData)
                    for (int point = 0; point < stepSize; point++) {
                        if (point % (kSamplesInVectData) == 0) {
                            dataVect = *inPointer++;
                        }
                        coeffVect = *coeffVectPtr++;
                        acc = ::aie::mac(acc, dataVect[point % kSamplesInVectData], coeffVect);
                        coeffVect2 = *coeffVectPtr++;
                        acc2 = ::aie::mac(acc2, dataVect[point % kSamplesInVectData], coeffVect2);
                    }

                    // output to outVect or to outCascade
                    if
                        constexpr(TP_CASC_OUT == CASC_OUT_FALSE) {
                            if
                                constexpr(std::is_same<TT_TWIDDLE, cfloat>()) {
                                    outVect = acc.template to_vector<T_outDataType>();
                                    outVect2 = acc2.template to_vector<T_outDataType>();
                                }
                            else {
                                outVect = acc.template to_vector<T_outDataType>(shift);
                                outVect2 = acc2.template to_vector<T_outDataType>(shift);
                            }
                            *outPtr++ = outVect;
                            *outPtr++ = outVect2;
                        }
                    else {
                        writeincr(outInterface.outCascade, acc);
                        writeincr(outInterface.outCascade, acc2);
                    }
                }
            // if number of vector columns isn't a multiple of 2, the final mac/acc is computed alone
            if
                constexpr(singleAccRequired) {
                    // get acc data, or initialize if first or only kernel in cascade
                    if
                        constexpr(TP_CASC_IN == CASC_IN_TRUE) {
                            acc = (accVect_t)readincr_v<kSamplesInVectData>(inInterface.inCascade);
                        }
                    else {
                        acc = (accVect_t)blankVect; // Initialize if one kernel used or the first used
                    }

                    inPointer = frameStart;
#pragma unroll(stepSize)
                    for (int point = 0; point < stepSize; point++) {
                        if (point % kSamplesInVectData == 0) {
                            dataVect = *inPointer++;
                        }
                        coeffVect = *coeffVectPtr++;
                        acc = ::aie::mac(acc, dataVect[point % kSamplesInVectData], coeffVect);
                    }
                    // output to outVect or to outCascade
                    if
                        constexpr(TP_CASC_OUT == CASC_OUT_FALSE) {
                            if
                                constexpr(std::is_same<TT_TWIDDLE, cfloat>()) {
                                    outVect = acc.template to_vector<T_outDataType>();
                                }
                            else {
                                outVect = acc.template to_vector<T_outDataType>(shift);
                            }
                            *outPtr++ = outVect;
                        }
                    else {
                        writeincr(outInterface.outCascade, acc);
                    }
                }

            // Jump to data of next frame
            frameStart += kVecInFrame;
        }
}
//-----------------------------------------------------------------------------------------------------
// For a single kernel - iobuffer in and out, no cascades
template <typename TT_DATA,
          typename TT_TWIDDLE,
          unsigned int TP_POINT_SIZE,
          unsigned int TP_FFT_NIFFT,
          unsigned int TP_SHIFT,
          unsigned int TP_CASC_LEN,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_KERNEL_POSITION,
          bool TP_CASC_IN,
          bool TP_CASC_OUT,
          unsigned int TP_RND,
          unsigned int TP_SAT>
NOINLINE_DECL void dft<TT_DATA,
                       TT_TWIDDLE,
                       TP_POINT_SIZE,
                       TP_FFT_NIFFT,
                       TP_SHIFT,
                       TP_CASC_LEN,
                       TP_NUM_FRAMES,
                       TP_KERNEL_POSITION,
                       TP_CASC_IN,
                       TP_CASC_OUT,
                       TP_RND,
                       TP_SAT>::dftMain(input_buffer<TT_DATA>& __restrict inWindow,
                                        output_buffer<T_outDataType>& __restrict outWindow) {
    T_inputIF<CASC_IN_FALSE, TT_DATA, TT_TWIDDLE> inInterface;
    TT_TWIDDLE* __restrict coeffPtr = &inCoeff[0];
    T_outputIF<CASC_OUT_FALSE, T_outDataType, TT_TWIDDLE> outInterface;
    inInterface.inWindow = inWindow.data();
    outInterface.outWindow = outWindow.data();
    m_dftKernel.kernelDFT(inInterface, coeffPtr, outInterface);
    // m_dftKernel.kernelDFT(inWindow, outWindow);
};
//--------------------------------------------------------------------------------------------------------
// For multiple kernels, first kernel - iobuffer in, cascade out
template <typename TT_DATA,
          typename TT_TWIDDLE,
          unsigned int TP_POINT_SIZE,
          unsigned int TP_FFT_NIFFT,
          unsigned int TP_SHIFT,
          unsigned int TP_CASC_LEN,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_KERNEL_POSITION,
          unsigned int TP_RND,
          unsigned int TP_SAT>
NOINLINE_DECL void dft<TT_DATA,
                       TT_TWIDDLE,
                       TP_POINT_SIZE,
                       TP_FFT_NIFFT,
                       TP_SHIFT,
                       TP_CASC_LEN,
                       TP_NUM_FRAMES,
                       TP_KERNEL_POSITION,
                       CASC_IN_FALSE,
                       CASC_OUT_TRUE,
                       TP_RND,
                       TP_SAT>::dftMain(input_buffer<TT_DATA>& __restrict inWindow,
                                        output_stream<accType_t<T_outDataType, TT_TWIDDLE> >* outCascade) {
    T_inputIF<CASC_IN_FALSE, TT_DATA, TT_TWIDDLE> inInterface;
    TT_TWIDDLE* __restrict coeffPtr = &inCoeff[0];
    T_outputIF<CASC_OUT_TRUE, T_outDataType, TT_TWIDDLE> outInterface;
    inInterface.inWindow = inWindow.data();
    outInterface.outCascade = outCascade;
    m_dftKernel.kernelDFT(inInterface, coeffPtr, outInterface);
};
//------------------------------------------------------------------------------------------------------------
// For multiple kernels, middle kernels - iobuffer in, cascade in AND out
template <typename TT_DATA,
          typename TT_TWIDDLE,
          unsigned int TP_POINT_SIZE,
          unsigned int TP_FFT_NIFFT,
          unsigned int TP_SHIFT,
          unsigned int TP_CASC_LEN,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_KERNEL_POSITION,
          unsigned int TP_RND,
          unsigned int TP_SAT>
NOINLINE_DECL void dft<TT_DATA,
                       TT_TWIDDLE,
                       TP_POINT_SIZE,
                       TP_FFT_NIFFT,
                       TP_SHIFT,
                       TP_CASC_LEN,
                       TP_NUM_FRAMES,
                       TP_KERNEL_POSITION,
                       CASC_IN_TRUE,
                       CASC_OUT_TRUE,
                       TP_RND,
                       TP_SAT>::dftMain(input_buffer<TT_DATA>& __restrict inWindow,
                                        input_stream<accType_t<T_outDataType, TT_TWIDDLE> >* inCascade,
                                        output_stream<accType_t<T_outDataType, TT_TWIDDLE> >* outCascade) {
    T_inputIF<CASC_IN_TRUE, TT_DATA, TT_TWIDDLE> inInterface;
    TT_TWIDDLE* __restrict coeffPtr = &inCoeff[0];
    T_outputIF<CASC_OUT_TRUE, T_outDataType, TT_TWIDDLE> outInterface;
    inInterface.inWindow = inWindow.data();
    inInterface.inCascade = inCascade;
    outInterface.outCascade = outCascade;
    m_dftKernel.kernelDFT(inInterface, coeffPtr, outInterface);
};
//-------------------------------------------------------------------------------------------------------------
// For multiple kernels, last kernel - iobuffer in, cascade in, iobuffer rout
template <typename TT_DATA,
          typename TT_TWIDDLE,
          unsigned int TP_POINT_SIZE,
          unsigned int TP_FFT_NIFFT,
          unsigned int TP_SHIFT,
          unsigned int TP_CASC_LEN,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_KERNEL_POSITION,
          unsigned int TP_RND,
          unsigned int TP_SAT>
NOINLINE_DECL void dft<TT_DATA,
                       TT_TWIDDLE,
                       TP_POINT_SIZE,
                       TP_FFT_NIFFT,
                       TP_SHIFT,
                       TP_CASC_LEN,
                       TP_NUM_FRAMES,
                       TP_KERNEL_POSITION,
                       CASC_IN_TRUE,
                       CASC_OUT_FALSE,
                       TP_RND,
                       TP_SAT>::dftMain(input_buffer<TT_DATA>& __restrict inWindow,
                                        input_stream<accType_t<T_outDataType, TT_TWIDDLE> >* inCascade,
                                        output_buffer<T_outDataType>& __restrict outWindow) {
    T_inputIF<CASC_IN_TRUE, TT_DATA, TT_TWIDDLE> inInterface;
    TT_TWIDDLE* __restrict coeffPtr = &inCoeff[0];
    T_outputIF<CASC_OUT_FALSE, T_outDataType, TT_TWIDDLE> outInterface;
    inInterface.inWindow = inWindow.data();
    inInterface.inCascade = inCascade;
    outInterface.outWindow = outWindow.data();
    m_dftKernel.kernelDFT(inInterface, coeffPtr, outInterface);
};
}
}
}
}
}
