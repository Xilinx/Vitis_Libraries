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
#ifndef _DSPLIB_MIXED_RADIX_TWGEN_UTILS_HPP_
#define _DSPLIB_MIXED_RADIX_TWGEN_UTILS_HPP_

#include "device_defs.h"

/*
TWIDDLE GENERATION FFT (1 channel DIT) Utilities
This file contains sets of overloaded, templatized and specialized templatized functions for use
by the main twiddle generation kernel class and run-time function. These functions are separate from the traits file
because they are purely for kernel use, not graph level compilation.
*/

#include <stdio.h>
//#include <adf.h>
#include <type_traits>
#include <typeinfo>

#include "aie_api/aie_adf.hpp"
#include "aie_api/fft.hpp"

//#define _DSPLIB_MRFFT_TWGEN_HPP_DEBUG_

namespace xf {
namespace dsp {
namespace aie {
namespace fft {
namespace mixed_radix_fft {

#define __24_1__ // TODO Now 24_2 ?

template <typename TT_TWIDDLE, typename twVect_t>
auto INLINE_DECL runStage(const int& kRadixNumber,
                          int& stepsizeForStage,
                          int32* indexPtr,
                          int& indexIdx,
                          int& twiddleIdx,
                          const int& kTwVectorSize,
                          int& lengthOfLeg,
                          const int& kTwVectorSizeShift,
                          TT_TWIDDLE* coeff,
                          twVect_t* twPtr) {
    twVect_t twVect;
    static constexpr TT_TWIDDLE firstTwiddle = {32767,
                                                0}; // W(0, N) // TODO DUPLICATE definition in mixed_radix_twGen.hpp

    // Select all twiddles for this Radix kRadixNumber stage
    for (int leg = 1; leg < kRadixNumber; leg++) {
        int stepsize_for_leg = int(stepsizeForStage * leg);

        // star comes first *indexPtr-- = twiddleIdx*kTwVectorSize;
        indexPtr[indexIdx--] = twiddleIdx * kTwVectorSize;

        int numFullTwVects = lengthOfLeg >> kTwVectorSizeShift;
        int i = 0;
        for (int r = 0; r < numFullTwVects;
             r++) { // r is number of 256 bits worth of twiddles, i.e. number of full rows
            for (int t = 0; t < kTwVectorSize; t++) {
                twVect[t] = coeff[i];
                i = i + stepsize_for_leg;
            }
            twPtr[twiddleIdx++] = twVect;
        }
        for (int t = 0; t < lengthOfLeg - (numFullTwVects * kTwVectorSize); t++) {
            twVect[t] = coeff[i];
            i = i + stepsize_for_leg;
        }
        twPtr[twiddleIdx++] = twVect;
    }
};
}
}
}
}
} // namespace closures

#endif // _DSPLIB_MIXED_RADIX_TWGEN_UTILS_HPP_
