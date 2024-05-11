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
#ifndef _DSPLIB_mixed_radix_fft_TRAITS_HPP_
#define _DSPLIB_mixed_radix_fft_TRAITS_HPP_

#ifndef INLINE_DECL
#define INLINE_DECL inline __attribute__((always_inline))
#endif
#ifndef NOINLINE_DECL
#define NOINLINE_DECL inline __attribute__((noinline))
#endif

/*
MIXED_RADIX_FFT traits.
This file contains sets of overloaded, templatized and specialized templatized functions which
encapsulate properties of the intrinsics used by the main kernal class. Specifically,
this file does not contain any vector types or intrinsics since it is required for construction
and therefore must be suitable for the aie compiler graph-level compilation.
*/

#include "device_defs.h"

namespace xf {
namespace dsp {
namespace aie {
namespace fft {
namespace mixed_radix_fft {
//-------------------------------------
// app-specific constants

constexpr int kMaxRadix = 5;
constexpr int kMaxStages = 12; // 4096 is 12 radix2 stages
constexpr int kNumMaxTables = kMaxRadix * kMaxStages;

// Defensive check functions
// End of Defensive check functions

//---------------------------------------------------
// Functions

template <int T_PTSIZE, int RADIX>
constexpr int fnGetNumStages() {
    constexpr int kMaxStages = 10;
    int numStages = 0;
    int ptSize = T_PTSIZE;
    // Vectorization in the final stage sets limits on factorization. e.g. radi4 stage on AIE1 outputs 16 samples, so
    // if radix4 is used, the point size must be a multiple of 16. Else set radix4 stages to 0 and hope that radix 2
    // will factorize.
    if (RADIX == 4 && ptSize % __MRFFT_ATOM__ != 0) return 0;

    for (int i = 0; i < kMaxStages; i++) {
        if (ptSize % RADIX == 0) {
            ptSize /= RADIX;
            numStages++;
        }
    }
    return numStages;
}

template <int T_PTSIZE, int RADIX>
constexpr int fnGetRadixFactor() {
    constexpr int kMaxStages = 10;
    int radixFactor = 1;
    int ptSize = T_PTSIZE;
    if (RADIX == 4 && ptSize % __MRFFT_ATOM__ != 0) return 1;

    for (int i = 0; i < kMaxStages; i++) {
        if (ptSize % RADIX == 0) {
            ptSize /= RADIX;
            radixFactor *= RADIX;
        }
    }
    return radixFactor;
}

template <typename TT_TWIDDLE, int TP_POINT_SIZE
          //    int kR5factor,
          // int kR3factor,
          // int kR2factor,
          // int kR4factor,
          // int kR5Stages,
          // int kR3Stages,
          // int kR2Stages,
          // int kR4Stages
          >
constexpr int fnGetTwiddleTableSize() {
    //    int masterIdx = 0; //track entry in m_twiddleTable
    // int factor = 1; //multiplies up by the radix of the stage after each stage
    // constexpr int radices[4] = {5,3,2,4};
    // constexpr int stages[4] = {kR5Stages,kR3Stages,kR2Stages,kR4Stages};
    // for (int section = 0; section < 4; section++) {
    //  for(int stage = 0; stage < kR5Stages; stage++) {
    //    int perLeg = CEIL(factor, 32/sizeof(TT_TWIDDLE));
    //    masterIdx += perLeg * (5-1);
    //    masterIdx = CEIL(masterIdx, 32/sizeof(TT_TWIDDLE)); //skip up to next aligned place for next leg
    //    printf("masterIdx = %d", masterIdx);
    //  }
    //  factor *= radices[section];
    //}
    // return masterIdx;

    // The below equation is ad hoc. The code above is an attempt to model the table size more closely, but fails
    // compilation due the use of loops in a constexpr return.
    // The overall twiddle table size is TP_POINT_SIZE, but due to alignment/padding, earlier ranks require a few more
    // entries, hence the additional factor which is a
    // roughly calculated worst case for point sizes up to ~4000
    return TP_POINT_SIZE + 128;
}
}
}
}
}
}

#endif // _DSPLIB_mixed_radix_fft_TRAITS_HPP_
