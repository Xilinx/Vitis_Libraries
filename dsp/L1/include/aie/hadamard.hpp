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
#ifndef _DSPLIB_HADAMARD_HPP_
#define _DSPLIB_HADAMARD_HPP_

/*
Hadamard Kernel.
This file exists to capture the definition of the Hadamard kernel class.
The class definition holds defensive checks on parameter range and other
legality.
The constructor definition is held in this class because this class must be
accessible to graph level aie compilation.
The main runtime function is captured elsewhere (cpp) as it contains aie
intrinsics (albeit aie api) which are not included in aie graph level
compilation.
*/

/* Coding conventions
   TT_      template type suffix
   TP_      template parameter suffix
*/

/* Design Notes

*/

// #include <vector>
#include <array>
#include <adf.h>
#include "device_defs.h"
#include "hadamard_traits.hpp"

using namespace adf;

//#define _DSPLIB_HADAMARD_HPP_DEBUG_

// #include "hadamard_traits.hpp" //for fnPointSizePwr

namespace xf {
namespace dsp {
namespace aie {
namespace hadamard {

//-----------------------------------------------------------------------------------------------------
// Hadamard kernel class
template <typename TT_DATA_A,
          typename TT_DATA_B,
          unsigned int TP_DIM,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_SHIFT,
          unsigned int TP_API,
          unsigned int TP_SSR,
          unsigned int TP_RND,
          unsigned int TP_SAT>
class hadamard {
   private:
    static_assert(TP_RND >= ROUND_MIN && TP_RND <= ROUND_MAX, "ERROR: TP_RND is out of the supported range.");

    static constexpr unsigned int kSamplesInVect = 256 / 8 / (vectByte<TT_DATA_A, TT_DATA_B>().val_byteBuffWin);
    static constexpr unsigned int kVecInFrame = CEIL(TP_DIM, kSamplesInVect) / kSamplesInVect;
    static constexpr unsigned int kUNROLL = MIN(kVecInFrame, kUnrollMax);

    static constexpr int kKernelWindowVsize = (TP_NUM_FRAMES * TP_DIM);
    static_assert(kKernelWindowVsize * sizeof(outTypeMult_t<TT_DATA_A, TT_DATA_B>) <= __DATA_MEM_BYTES__,
                  "ERROR: TP_NUM_FRAMES*(TP_DIM/TP_SSR) must be at no more than data memory size.");

   public:
    using out_t = outTypeMult_t<TT_DATA_A, TT_DATA_B>;
    using acc_t = accTypeMult_t<TT_DATA_A, TT_DATA_B>;

    // Constructor
    hadamard();

    // Register Kernel Class

    static void registerKernelClass() { REGISTER_FUNCTION(hadamard::hadamard_main); }

    // Main function
    void hadamard_main(input_buffer<TT_DATA_A>& __restrict inWindowA,
                       input_buffer<TT_DATA_B>& __restrict inWindowB,
                       output_buffer<out_t>& __restrict outWindow);
};

// Hadamard Product kernel class - stream specialization.
template <typename TT_DATA_A,
          typename TT_DATA_B,
          unsigned int TP_DIM,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_SHIFT,
          unsigned int TP_SSR,
          unsigned int TP_RND,
          unsigned int TP_SAT>
class hadamard<TT_DATA_A, TT_DATA_B, TP_DIM, TP_NUM_FRAMES, TP_SHIFT, 1, TP_SSR, TP_RND, TP_SAT> {
   private:
    static_assert(TP_RND >= ROUND_MIN && TP_RND <= ROUND_MAX, "ERROR: TP_RND is out of the supported range.");
    static constexpr unsigned int kSamplesInVect = MAX(128 / 8 / sizeof(TT_DATA_A), 128 / 8 / sizeof(TT_DATA_B));
    static constexpr unsigned int kVecInFrame = CEIL(TP_DIM, kSamplesInVect) / kSamplesInVect;
    static constexpr unsigned int kUNROLL = MIN(kVecInFrame, kUnrollMax);

   public:
    using out_t = outTypeMult_t<TT_DATA_A, TT_DATA_B>;
    using acc_t = accTypeMult_t<TT_DATA_A, TT_DATA_B>;
    // Constructor
    hadamard();

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(hadamard::hadamard_main); }

    // Main function
    // These could be claused out according to __STREAMS_PER_TILE__, but the overload is unambiguous.
    void hadamard_main(input_stream<TT_DATA_A>* __restrict inStreamA,
                       input_stream<TT_DATA_B>* __restrict inStreamB,
                       output_stream<out_t>* __restrict outStream);
};
}
}
}
}

#endif // _DSPLIB_HADAMARD_HPP_
