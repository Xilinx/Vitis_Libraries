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
MIXED_RADIX_FFT kernal code.
This file captures the body of run-time code for the kernal class.

Coding conventions
  TT_      template type suffix
  TP_      template parameter suffix
*/

#include <adf.h>
#include <stdio.h>

using namespace std;

#include "device_defs.h"
#define FFT_SHIFT15 15

// if we use 1kb registers -> aie api uses 2x512b registers for 1024b so we need this for QoR
#define __AIE_API_USE_NATIVE_1024B_VECTOR__
#include "aie_api/aie_adf.hpp"

//#define _DSPLIB_MRFFT_TWGEN_HPP_DEBUG_

#include "mixed_radix_twGen.hpp"
#include "mixed_radix_twGen_utils.hpp"
#include "mixed_radix_fft_utils.hpp"
#include "kernel_api_utils.hpp"

#define ALGORITHM 2

namespace xf {
namespace dsp {
namespace aie {
namespace fft {
namespace mixed_radix_fft {

// MIXED_RADIX_FFT Twiddle Generation base of specialization .
//-----------------------------------------------------------------------------------------------------

template <typename TT_DATA, // data type of header input
          typename TT_TWIDDLE,
          unsigned int TP_POINT_SIZE, // + 128 for the twiddle_out size
          unsigned int TP_RND,
          unsigned int TP_SAT>
INLINE_DECL void
kernel_MRFFTTwiddleGenerationClass<TT_DATA, TT_TWIDDLE, TP_POINT_SIZE, TP_RND, TP_SAT>::kernelMixedRadixFFTtwiddleGen(
    input_buffer<TT_DATA>* headerInWindow,
    output_buffer<TT_DATA>* headerOutWindow,
    output_buffer<TT_TWIDDLE>* outWindow) {
    set_rnd_mode<TP_RND>();
    set_sat_mode<TP_SAT>();

    using infoVect_t = ::aie::vector<T_ancillaryFields, kVectorSize>;
    using twVect_t = ::aie::vector<TT_TWIDDLE, kTwVectorSize>;
    infoVect_t* headerInPtr = (infoVect_t*)headerInWindow->data();   // for header input
    infoVect_t* headerOutPtr = (infoVect_t*)headerOutWindow->data(); // for header output
    twVect_t* twPtr = (twVect_t*)outWindow->data();                  // for twiddle fields of twiddle output
    infoVect_t* infoOutPtr = (infoVect_t*)twPtr;                     // for ancillary fields of twiddle output
    infoVect_t headerRead = *headerInPtr;
    // Initialise indices
    int twiddleIdx = twiddleIdxStart; // only need row index since populating in vectors of kTwVectorSize
    // Calculate stagePtSize factors and pointsize
    infoVect_t stagePtSizes;
    int invInt = headerRead.get(0);
    int nR2raw = headerRead.get(2);
    int nR3 = headerRead.get(3);
    int nR5 = headerRead.get(4);
    int nR2;
    int nR4;

#if __FFT_R4_IMPL__ == 0 // AIE1
    // TODO OPTIMISATION Do operation instead of conditionals
    if (nR2raw >= 4 && !(__ONLY_R2_STAGES__)) { // if pointsize is divisible by 16
        // Re-calculate number of R2s, and calculate number of R4s
        nR2 = (nR2raw & 1);
        nR4 = nR2raw >> 1;
    } else {
        nR2 = nR2raw;
        nR4 = 0;
    }
    int totalLegsInAllStages = nR3 * 2 + nR5 * 4 + nR4 * 2 + nR2;
#elif __FFT_R4_IMPL__ == 1 // AIE-ML
    nR2 = (nR2raw & 1);
    nR4 = nR2raw >> 1;
    int totalLegsInAllStages = nR3 * 2 + nR5 * 4 + nR4 * 3 + nR2;
#endif // __FFT_R4_IMPL__
    int totalStages = nR2 + nR4 + nR3 + nR5;
    int count = 0;
    int prev = 1;
    stagePtSizes[count++] = prev;
    for (int r5 = 0; r5 < nR5; r5++) {
        prev *= kR5;
        stagePtSizes[count++] = prev;
    }
    for (int r3 = 0; r3 < nR3; r3++) {
        prev *= kR3;
        stagePtSizes[count++] = prev;
    }
    for (int r2 = 0; r2 < nR2 - 1; r2++) { // -1 because final *kR2 is done outside of loop
        prev *= kR2;
        // printf("\ncount = %d, prev = %d", count, prev);
        stagePtSizes[count++] = prev;
    }
    prev *= kR2;

    headerRead[7] = ((prev <= TP_POINT_SIZE) && (nR2raw >= 3)) ? 0 : 1; // errorflag
    *headerOutPtr = headerRead;
    infoOutPtr[headerIdxStart] = headerRead;
    // if error then terminate

    infoVect_t ptsizeOnlyVect;
    ptsizeOnlyVect[0] = prev;
    infoOutPtr[factorsStagePtSizeIdxStart] = ptsizeOnlyVect;

    // More Initialise indices
    int32* indexPtr = (int32*)&infoOutPtr[indexIdxStart]; // int32 due to tw_ptrs in mixed_radix_fft.cpp requiring int32
                                                          // for API function inside opt_dyn
    int indexIdx = totalLegsInAllStages - 1;

    // Initialisations
    int stagePtSizeFactorIdx = count - 1; // TODO or -1
    int stepsizeFactorIdx = totalStages - 1;
    int32 stagePtSize;
    int32 stepsize_for_stage = 1;
    int32 stepsize_for_leg = 1;
    infoVect_t stepsizes;

    // Find index of fan and step arrays
    int positionInIndicesLUT = (nR2raw - minNumR2) * kMaxStagesR3 * kMaxStagesR5 + nR3 * kMaxStagesR5 + nR5;
    int index_for_pointsize = m_kIndicesLUT[positionInIndicesLUT];

    // Select fan and step array for this runtime pointsize
    using fanVect_t = ::aie::vector<TT_TWIDDLE, kFanVectorSize>;
    fanVect_t* fanPtr = (fanVect_t*)m_kFanLUT;
    fanVect_t fan = *(fanPtr + index_for_pointsize);
    // using stepVect_t = std::array<TT_TWIDDLE, kStepVectorSize>;
    // stepVect_t* stepPtr = (stepVect_t*)m_kStepLUT;
    // stepVect_t step = *(stepPtr + index_for_pointsize);
    TT_TWIDDLE* step = m_kStepLUT + index_for_pointsize * kStepVectorSize;

    fanVect_t* coeffPtr = (fanVect_t*)coeff;

    fan[0] = firstTwiddle;
    *coeffPtr++ = fan;

    for (int s = 0; s < kStepVectorSize; s++) {
        *coeffPtr++ = ::aie::mul(step[s], fan)
                          .template to_vector<TT_TWIDDLE>(kTwShift); // TODO 15 is twiddle shift for my case - cint16
    } // Cherry tree up to kFanVectorSize*kStepVectorSize + kFanVectorSize has been built

#if __FFT_R4_IMPL__ == 0 // AIE1 "radix 4" is spoofed 2 radix 2s
    // RADIX 2 STAGE
    // LAST RADIX 2 STAGE
    twVect_t* twiddleArrayPtr = (twVect_t*)&coeff[0];
    stepsizes[stagePtSizeFactorIdx] = stepsize_for_stage;
    stagePtSize = stagePtSizes[stagePtSizeFactorIdx--];
    // Select all twiddles for this Radix 2 stage

    // star comes first *indexPtr-- = twiddleIdx*kTwVectorSize;
    indexPtr[indexIdx--] = twiddleIdx * kTwVectorSize;
    int numFullTwVects = stagePtSize >> kTwVectorSizeShift;
    for (int r = 0; r < numFullTwVects; r++) { // r is number of 256 bits worth of twiddles, i.e. number of full rows
        twPtr[twiddleIdx++] = *twiddleArrayPtr++;
    }
    if (stagePtSize - (numFullTwVects * kTwVectorSize) > 0) {
        twPtr[twiddleIdx++] = *twiddleArrayPtr++;
    }
    // for next stage
    stepsize_for_stage = stepsize_for_stage * kR2;

    // OTHER RADIX 2 STAGES
    // int nR2raw_polarity = nR2raw & 1;
    for (int stage = nR2 - 2; stage >= 0;
         stage--) { // nR2-1 -1 with the second -1 due to one r2 stage being done before the loop
        stepsizes[stagePtSizeFactorIdx] = stepsize_for_stage;
        stagePtSize = stagePtSizes[stagePtSizeFactorIdx--];
        runStage<TT_TWIDDLE, twVect_t>(kR2, stepsize_for_stage, indexPtr, indexIdx, twiddleIdx, kTwVectorSize,
                                       stagePtSize, kTwVectorSizeShift, &coeff[0], twPtr);
        stepsize_for_stage = stepsize_for_stage * kR2;
    }  // end RADIX 2 stages
#else  // AIE-ML / AIE2 __FFT_R4_IMPL__ == 0
    // RADIX 4 STAGES
    for (int stage = nR4; stage >= 0; stage--) {
        stepsizes[stagePtSizeFactorIdx] = stepsize_for_stage;
        stagePtSize = stagePtSizes[stagePtSizeFactorIdx--];
        row = 0;
        for (int leg = 1; leg < kR4; leg++) {
            // TODO instead of if series, write out code inside loop three times? - one for each leg value in the order
            // 2, 1, 3
            if (leg == 1) {
                stepsize_for_leg = int(stepsize_for_stage * 2);
            } else if (leg == 2) {
                stepsize_for_leg = int(stepsize_for_stage * 1);
            } else {
                stepsize_for_leg = int(stepsize_for_stage * leg);
            }
            stepsize_for_leg = int(stepsize_for_stage * leg);
            twVect[0] = firstTwiddle; // W(0, pointsize)

            //*indexPtr-- = twiddleIdx*kTwVectorSize;
            indexPtr[indexIdx--] = twiddleIdx * kTwVectorSize;
            x = 1; // to prevent division by 8 when outputing in vectors of eight since selecting cherries individually,
                   // initially 1 since 1st tw in leg already added
            int upperLimit = stagePtSize * stepsize_for_leg;
            for (int i4 = stepsize_for_leg; i4 < CEIL(stagePtSize, kTwVectorSize) * stepsize_for_leg;
                 i4 += stepsize_for_leg) {
                if (i4 < upperLimit) {
                    twVect[x] = coeff[i4];
                    // twVect.insert(x, coeff.get(i4*stepsize_for_leg)); // TODO i4 += stepsize_for_leg in for loop
                    // might be better OPTIMISATION
                } else {
                    twVect[x] = blankTwiddle; // pad leg up to 256 bits
                    // twVect.insert(x, blank);  // TODO is this needed? Probably not OPTIMISATION
                }

                x++;
                if (x == kTwVectorSize) {
                    twPtr[twiddleIdx++] = twVect;
                    row++;
                    x = 0; // reset
                }
            }
            row++;
        }
        stepsize_for_stage = stepsize_for_stage * kR4;
    }               // end RADIX 4 stages
    if (nR2 == 0) { // TODO Check condition is correct 1 or 0
        stepsizes[stagePtSizeFactorIdx] = stepsize_for_stage;
        stagePtSize = stagePtSizes[stagePtSizeFactorIdx--];
        runStage<TT_TWIDDLE, twVect_t>(kR2, stepsize_for_stage, indexPtr, indexIdx, twiddleIdx, kTwVectorSize,
                                       stagePtSize, kTwVectorSizeShift, &coeff[0], twPtr);
        stepsize_for_stage = stepsize_for_stage * kR2;
    } // end RADIX 2 stages
#endif // __FFT_R4_IMPL__
    // RADIX 3 STAGES
    for (int stage = nR3 - 1; stage >= 0; stage--) {
        stepsizes[stagePtSizeFactorIdx] = stepsize_for_stage;
        stagePtSize = stagePtSizes[stagePtSizeFactorIdx--];
        runStage<TT_TWIDDLE, twVect_t>(kR3, stepsize_for_stage, indexPtr, indexIdx, twiddleIdx, kTwVectorSize,
                                       stagePtSize, kTwVectorSizeShift, &coeff[0], twPtr);
        stepsize_for_stage = stepsize_for_stage * kR3;
    } // end RADIX 3 stages

    // RADIX 5 STAGES
    for (int stage = nR5 - 1; stage >= 0; stage--) {
        stepsizes[stagePtSizeFactorIdx] = stepsize_for_stage;
        stagePtSize = stagePtSizes[stagePtSizeFactorIdx--];
        runStage<TT_TWIDDLE, twVect_t>(kR5, stepsize_for_stage, indexPtr, indexIdx, twiddleIdx, kTwVectorSize,
                                       stagePtSize, kTwVectorSizeShift, &coeff[0], twPtr);
        stepsize_for_stage = stepsize_for_stage * kR5;
    } // end RADIX 5 stages
    infoOutPtr[factorsStepsizeIdxStart] = stepsizes;
};

//-----------------------------------------------------------------------------------------------------
// For a single kernel - iobuffer in and out, no cascades
template <typename TT_DATA, typename TT_TWIDDLE, unsigned int TP_POINT_SIZE, unsigned int TP_RND, unsigned int TP_SAT>
NOINLINE_DECL void mixed_radix_twGen<TT_DATA, TT_TWIDDLE, TP_POINT_SIZE, TP_RND, TP_SAT>::mixed_radix_twGenMain(
    input_buffer<TT_DATA>& __restrict headerInWindow,
    output_buffer<TT_DATA>& __restrict headerOutWindow,
    output_buffer<TT_TWIDDLE>& __restrict outWindow) {
    this->kernelMixedRadixFFTtwiddleGen((input_buffer<TT_DATA>*)&headerInWindow,
                                        (output_buffer<TT_DATA>*)&headerOutWindow,
                                        (output_buffer<TT_TWIDDLE>*)&outWindow);
};
}
}
}
}
}
