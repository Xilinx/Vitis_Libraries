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
#ifndef _DSPLIB_BITONIC_SORT_HPP_
#define _DSPLIB_BITONIC_SORT_HPP_

/*
Bitonic Sort Kernel.
This file exists to capture the definition of the Bitonic Sort kernel class.
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
#include "fir_utils.hpp"
#include "bitonic_sort_traits.hpp"

using namespace adf;

//#define _DSPLIB_BITONIC_SORT_HPP_DEBUG_

namespace xf {
namespace dsp {
namespace aie {
namespace bitonic_sort {

//-----------------------------------------------------------------------------------------------------
// Bitonic Sort kernel base class - windows (io buffer) interface
template <typename TT_DATA,
          unsigned int TP_DIM,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_ASCENDING,
          unsigned int TP_CASC_LEN,
          unsigned int TP_CASC_IDX>
class bitonic_sort {
   private:
   public:
    // * kFirstStage: The stage at which this kernel is sorting from. If there is a remainer, the first kernels will...
    // * ... incorporate the remainder into an additionl stage.
    // * kCurrCascLen: The number of stages performed by this kernel.
    // * kLastStage: The last stage this kernel is sorting from.
    // * These are then used to inform the edge cases for the i and j loops.
    static constexpr unsigned int kNumStages = getNumStages<TP_DIM>();
    static constexpr unsigned int kNumCascStages = kNumStages / TP_CASC_LEN;
    static constexpr int kRemainder = kNumStages % TP_CASC_LEN;
    static constexpr unsigned int kFirstStage =
        MIN(TP_CASC_IDX, kRemainder) * (kNumCascStages + 1) + MAX((int)TP_CASC_IDX - kRemainder, 0) * kNumCascStages;
    static constexpr unsigned int kCurrCascLen = TP_CASC_IDX >= kRemainder ? kNumCascStages : kNumCascStages + 1;
    static constexpr unsigned int kLastStage = kFirstStage + kCurrCascLen - 1;

    static constexpr unsigned int kIStart = getOuterIdxForStage<kFirstStage>();
    static constexpr unsigned int kIEnd = getOuterIdxForStage<kLastStage>();
    static constexpr unsigned int kJStart = kFirstStage - getInitStageForOuterIdx<kIStart>();
    static constexpr unsigned int kJEnd = kLastStage - getInitStageForOuterIdx<kIEnd>();
    static constexpr unsigned int kVecSampleNum = 256 / 8 / sizeof(TT_DATA);
    static constexpr unsigned int kNumVecs = TP_DIM / kVecSampleNum;

    // TODO: The below could also be influenced by TP_DIM i.e. lower TP_DIMs can be unrolled more.
    // TODO: Higher the number of cascades, lower the program memory per kernel, hence more unrolls.
    static constexpr unsigned int kUnrollMax =
        sizeof(TT_DATA); // //TP_CASC_LEN > 1 ? 8 / sizeof(TT_DATA) : 16 / sizeof(TT_DATA);

    // Constructor
    bitonic_sort() {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(bitonic_sort::bitonic_sort_main); }

    // Main function
    void bitonic_sort_main(input_buffer<TT_DATA>& __restrict inWindow, output_buffer<TT_DATA>& __restrict outWindow);
};
}
}
}
}

#endif // _DSPLIB_BITONIC_SORT_HPP_
