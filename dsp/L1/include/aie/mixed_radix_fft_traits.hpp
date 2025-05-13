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
#ifndef _DSPLIB_mixed_radix_fft_TRAITS_HPP_
#define _DSPLIB_mixed_radix_fft_TRAITS_HPP_

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

constexpr int kMaxRadix = 5;     // for static
constexpr int kMaxStages = 12;   // 4096 is 12 radix2 stages - for static
constexpr int kMaxStagesR2 = 12; // for dynamic - as per dsplib_internal_scripts/mrfft_dynptsize_luts_gen.py
constexpr int kMaxStagesR3 = 6;  // for dynamic - as per dsplib_internal_scripts/mrfft_dynptsize_luts_gen.py
constexpr int kMaxStagesR5 = 4;  // for dynamic - as per dsplib_internal_scripts/mrfft_dynptsize_luts_gen.py
constexpr int kNumMaxTables = kMaxRadix * kMaxStages;

// For dynamic - constants for radix stages
constexpr int kR5 = 5;
constexpr int kR3 = 3;
constexpr int kR2 = 2;
constexpr int kR4 = 4;
#define __ONLY_R2_STAGES__ \
    1 // Only use radix 2 stages. Do not perform any radix 4 stages ever. 1 or 0 when AIE1. Only 0 for AIE-ML.
constexpr int minNumR2 = 3; // point sizes must be divisible by 8 for MRFFT

constexpr int kMaxStagesR4 = kMaxStagesR2 >> 1;
constexpr int kMaxTotalStages = kMaxStagesR2;
constexpr int kMaxLegs = 2 * kMaxStagesR4 + kMaxStagesR3 * (kR3 - 1) + kMaxStagesR5 * (kR5 - 1) + 1;

// For dynamic - header type / ancillary fields in twiddles structure type
typedef int32 T_ancillaryFields;
constexpr int kNumAncillaryPerRow = 256 / (8 * sizeof(T_ancillaryFields));
static constexpr int m_knRmultiple = (kMaxStagesR2 - minNumR2) * kMaxStagesR3 * kMaxStagesR5;

// For dynamic - structure of Twiddles and Ancillary Fields (in terms of "rows" of 256 bits) // TODO this may need to be
// altered (perhaps doubled) for twiddles of larger sized data type
constexpr int headerIdxStart = 0;
constexpr int factorsStagePtSizeIdxStart = 1;
constexpr int factorsStepsizeIdxStart = factorsStagePtSizeIdxStart + 1;
constexpr int indexIdxStart =
    factorsStepsizeIdxStart + 1; // plus 1 each time because vectors of 16 int16's and 16>maxStages always
constexpr int twiddleIdxStart = indexIdxStart + 3; // maximum 3 rows needed for indices for all pointsizes

// Defensive check functions
// End of Defensive check functions

//---------------------------------------------------
// Functions

template <int T_PTSIZE, int RADIX, typename T_TW>
constexpr int fnGetNumStages() {
    constexpr int kMaxStages = 10;
    int numStages = 0;
    int ptSize = T_PTSIZE;
    // Vectorization in the final stage sets limits on factorization. e.g. radi4 stage on AIE1 outputs 16 samples, so
    // if radix4 is used, the point size must be a multiple of 16. Else set radix4 stages to 0 and hope that radix 2
    // will factorize.
    if (RADIX == 4 &&
        ((ptSize % __MRFFT_ATOM__ != 0) ||     // if factorization fails, return 0 and try radix2 stages instead.
         (std::is_same<T_TW, cfloat>::value))) // there is no api for radix4 cfloat stage.
        return 0;

    for (int i = 0; i < kMaxStages; i++) {
        if (ptSize % RADIX == 0) {
            ptSize /= RADIX;
            numStages++;
        }
    }
    return numStages;
}

template <int T_PTSIZE, int RADIX, typename T_TW>
constexpr int fnGetRadixFactor() {
    constexpr int kMaxStages = 10;
    int radixFactor = 1;
    int ptSize = T_PTSIZE;
    if (RADIX == 4 &&
        ((ptSize % __MRFFT_ATOM__ != 0) ||     // if factorization fails, return 0 and try radix2 stages instead.
         (std::is_same<T_TW, cfloat>::value))) // there is no api for radix4 cfloat stage.
        return 1;

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
