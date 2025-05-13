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
          unsigned int TP_SSR,
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
                                TP_SSR,
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
    using coeffVect_t = ::aie::vector<TT_TWIDDLE, kSamplesInVectData>;

    using accVect_t = ::aie::accum<typename accType<T_outDataType, TT_TWIDDLE>::type, kSamplesInVectData>;

    dataVect_t dataVect;
    dataVect_t* __restrict inPointer;
    outDataVect_t blankVect = ::aie::zeros<T_outDataType, kSamplesInVectData>(); // to initialise acc
    outDataVect_t outVect, outVect2;

    coeffVect_t* __restrict coeffVectPtr;
    coeffVectPtr = (coeffVect_t*)coeffPtr;

    coeffVect_t coeffVect, coeffVect2;
    accVect_t acc, acc2;
    dataVect_t* frameStart = (dataVect_t*)inInterface.inWindow;
    outDataVect_t* __restrict outPtr = (dataVect_t*)outInterface.outWindow;

    TT_TWIDDLE* coeffCopy = coeffPtr;
    // Loop through each frame
    for (int frame = 0; frame < TP_NUM_FRAMES; frame++) chess_prepare_for_pipelining chess_loop_range(TP_NUM_FRAMES, ) {
            // pointer to a start of coefficients
            coeffCopy = chess_copy(coeffCopy);
            coeffVectPtr = ((coeffVect_t*)coeffCopy);
            ////////////////////////// Parallel/interleaved MAC stage ////////////////////////////
            // Loop thorugh parallel acc and MACs if present

            for (unsigned int vectCoeff = 0; vectCoeff < kPairsInCoeff; vectCoeff++) {
                // get acc data, or initialize if first or only kernel in cascade
                if
                    constexpr(TP_CASC_IN == CASC_IN_TRUE) {
                        acc = (accVect_t)readincr_v<kSamplesInVectData>(inInterface.inCascade);
                        acc2 = (accVect_t)readincr_v<kSamplesInVectData>(inInterface.inCascade);
                    }
                else {
                    acc = (accVect_t)blankVect; // Initialize if no cascade in
                    acc2 = (accVect_t)blankVect;
                }
                // pointer to start of a frame
                inPointer = frameStart;
                // Loop through all elements in the unpadded vectors of a frame
                if
                    constexpr(kVecNoPad > 0) {
                        // #pragma unroll (kVecNoPad)
                        for (int vec = 0; vec < kVecNoPad; vec++) {
                            dataVect = *inPointer++;
#pragma unroll(kSamplesInVectData)
                            for (int idx = 0; idx < kSamplesInVectData; idx++) {
                                coeffVect = *coeffVectPtr++;
                                acc = ::aie::mac(acc, dataVect[idx], coeffVect);

                                coeffVect2 = *coeffVectPtr++;
                                acc2 = ::aie::mac(acc2, dataVect[idx], coeffVect2);
                            }
                        }
                    }
                // Loop through only the non-padded elems in final padded vector of a frame
                if
                    constexpr(elemsInPaddedVec > 0) {
                        dataVect = *inPointer++;
#pragma unroll(elemsInPaddedVec)
                        for (int jdx = 0; jdx < elemsInPaddedVec; jdx++) {
                            coeffVect = *coeffVectPtr++;
                            acc = ::aie::mac(acc, dataVect[jdx], coeffVect);

                            coeffVect2 = *coeffVectPtr++;
                            acc2 = ::aie::mac(acc2, dataVect[jdx], coeffVect2);
                        }
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

            // ////////////////////////////// Singular MAC stage ///////////////////////////////////////
            // // if number of vectors in a column isn't a multiple of 2, the final mac/acc is computed alone
            if
                constexpr(singleAccRequired) {
                    // get acc data, or initialize if first or only kernel in cascade
                    if
                        constexpr(TP_CASC_IN == CASC_IN_TRUE) {
                            acc = (accVect_t)readincr_v<kSamplesInVectData>(inInterface.inCascade);
                        }
                    else {
                        acc = (accVect_t)blankVect;
                    }

                    inPointer = frameStart;
                    // Loop through all elements in unpadded vectors of frame
                    for (int vec = 0; vec < kVecNoPad; vec++) {
                        dataVect = *inPointer++;
#pragma unroll(kSamplesInVectData)
                        for (int idx = 0; idx < kSamplesInVectData; idx++) {
                            acc = ::aie::mac(acc, dataVect[idx], *coeffVectPtr++);
                        }
                    }
                    // Loop through only the non-padded elems in final padded vector of frame
                    if
                        constexpr(elemsInPaddedVec > 0) {
                            dataVect = *inPointer++;
#pragma unroll(elemsInPaddedVec)
                            for (int jdx = 0; jdx < elemsInPaddedVec; jdx++) {
                                coeffVect = *coeffVectPtr++;
                                acc = ::aie::mac(acc, dataVect[jdx], coeffVect);
                            }
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
            // End of frame - just to next frame of input data
            frameStart += kVecTotal;
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
          unsigned int TP_SSR,
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
                       TP_SSR,
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
          unsigned int TP_SSR,
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
                       TP_SSR,
                       TP_NUM_FRAMES,
                       TP_KERNEL_POSITION,
                       CASC_IN_FALSE,
                       CASC_OUT_TRUE,
                       TP_RND,
                       TP_SAT>::dftMain(input_buffer<TT_DATA>& __restrict inWindow,
                                        output_cascade<accType_t<T_outDataType, TT_TWIDDLE> >* outCascade) {
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
          unsigned int TP_SSR,
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
                       TP_SSR,
                       TP_NUM_FRAMES,
                       TP_KERNEL_POSITION,
                       CASC_IN_TRUE,
                       CASC_OUT_TRUE,
                       TP_RND,
                       TP_SAT>::dftMain(input_buffer<TT_DATA>& __restrict inWindow,
                                        input_cascade<accType_t<T_outDataType, TT_TWIDDLE> >* inCascade,
                                        output_cascade<accType_t<T_outDataType, TT_TWIDDLE> >* outCascade) {
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
          unsigned int TP_SSR,
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
                       TP_SSR,
                       TP_NUM_FRAMES,
                       TP_KERNEL_POSITION,
                       CASC_IN_TRUE,
                       CASC_OUT_FALSE,
                       TP_RND,
                       TP_SAT>::dftMain(input_buffer<TT_DATA>& __restrict inWindow,
                                        input_cascade<accType_t<T_outDataType, TT_TWIDDLE> >* inCascade,
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
