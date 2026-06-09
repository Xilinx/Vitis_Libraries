/*
 * Copyright (C) 2019-2022, Xilinx, Inc.
 * Copyright (C) 2022-2026, Advanced Micro Devices, Inc.
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
#ifndef _DSPLIB_OUTER_TENSOR_HPP_
#define _DSPLIB_OUTER_TENSOR_HPP_

/*
Outer Tensor Kernel.
This file exists to capture the definition of the Outer Tensor kernel class.
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
#include "outer_tensor_traits.hpp"

using namespace adf;

namespace xf {
namespace dsp {
namespace aie {
namespace outer_tensor {

//-----------------------------------------------------------------------------------------------------
// Outer Tensor kernel base class - windows (io buffer) interface
template <typename TT_DATA_A,
          typename TT_DATA_B,
          unsigned int TP_DIM_A,
          unsigned int TP_DIM_B,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_SHIFT,
          unsigned int TP_API,
          unsigned int TP_SSR,
          unsigned int TP_RND,
          unsigned int TP_SAT>
class outer_tensor {
   private:
   public:
    using acc_t = accTypeMult_t<TT_DATA_A, TT_DATA_B>;
    using out_t = outTypeMult_t<TT_DATA_A, TT_DATA_B>;

    static constexpr unsigned int kVecSampleNumA = fnVecSampleNumMax<TT_DATA_A>();
    static constexpr unsigned int kVecSampleNumB = fnVecSampleNumMax<TT_DATA_B>();
    static constexpr unsigned int kVecSampleNumOut =
        fnVecSampleNumMax<out_t>(); // only achievable if this number of elements in vector B is >= __MIN_REGSIZE__ / 8.
    static constexpr unsigned int kVecSampleNumBSubMin =
        __MIN_REGSIZE__ / 8 / sizeof(TT_DATA_B); // we cannot extract a sub-vector smaller than __MIN_REGSIZE__.
    static constexpr unsigned int kVecSampleNumBSub = MAX(kVecSampleNumBSubMin, kVecSampleNumOut);
    static constexpr unsigned int kNumExtractions =
        kVecSampleNumB / kVecSampleNumBSub; // this is how many sub-vectors we will need to extract from vector B.
    static constexpr unsigned int kVecNumA = TP_DIM_A / kVecSampleNumA;
    static constexpr unsigned int kVecNumB = TP_DIM_B / kVecSampleNumB;

    static constexpr unsigned int pwrsOf2 =
        fnCheckIfPwr2<TP_DIM_A>() & fnCheckIfPwr2<TP_DIM_B>(); // only exploit budget if nice-round-numbers.

    // TODO: consider if this can be optimized by factorizing into 2 primes?
    static constexpr unsigned int kUnrollBudget =
        pwrsOf2 ? getUnrollBudget<out_t>()
                : 0; // This denotes how many unrolls we can delegate between the i, j, k loops.
    static constexpr unsigned int kUnrollsK = MIN(kVecNumB, fnPwr2<kUnrollBudget>());
    static constexpr unsigned int kUnrollBudgetAfterK = kUnrollBudget - fnLog2<kUnrollsK>();
    static constexpr unsigned int kUnrollsJ = MIN(kVecSampleNumA, fnPwr2<kUnrollBudgetAfterK>());
    static constexpr unsigned int kUnrollBudgetAfterJ = kUnrollBudgetAfterK - fnLog2<kUnrollsJ>();
    static constexpr unsigned int kUnrollsI = MIN(kVecNumA, fnPwr2<kUnrollBudgetAfterK>());

    static constexpr unsigned int kChessNumI = kVecNumA / kUnrollsI;
    static constexpr unsigned int kChessNumJ = kVecSampleNumA / kUnrollsJ;
    static constexpr unsigned int kChessNumK = kVecNumB / kUnrollsK;

    // Constructor
    outer_tensor(){};

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(outer_tensor::outer_tensor_main); }

    // Main function
    void outer_tensor_main(input_buffer<TT_DATA_A>& __restrict inWindowA,
                           input_buffer<TT_DATA_B>& __restrict inWindowB,
                           output_buffer<out_t>& __restrict outWindow);
};

//-----------------------------------------------------------------------------------------------------
// Outer Tensor kernel specialized class - stream interface
template <typename TT_DATA_A,
          typename TT_DATA_B,
          unsigned int TP_DIM_A,
          unsigned int TP_DIM_B,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_SHIFT,
          // unsigned int TP_API,
          unsigned int TP_SSR,
          unsigned int TP_RND,
          unsigned int TP_SAT>
class outer_tensor<TT_DATA_A, TT_DATA_B, TP_DIM_A, TP_DIM_B, TP_NUM_FRAMES, TP_SHIFT, 1, TP_SSR, TP_RND, TP_SAT> {
   private:
   public:
    using acc_t = accTypeMult_t<TT_DATA_A, TT_DATA_B>;
    using out_t = outTypeMult_t<TT_DATA_A, TT_DATA_B>;

    static constexpr unsigned int kVecSampleNumA = fnVecSampleNumMax<TT_DATA_A>();
    static constexpr unsigned int kVecSampleNumB = fnVecSampleNumMax<TT_DATA_B>();
    static constexpr unsigned int kVecSampleNumOut =
        fnVecSampleNumMax<out_t>(); // only achievable if this number of elements in vector B is >= __MIN_REGSIZE__ / 8.
    static constexpr unsigned int kVecSampleNumBSubMin =
        __MIN_REGSIZE__ / 8 / sizeof(TT_DATA_B); // we cannot extract a sub-vector smaller than __MIN_REGSIZE__.
    static constexpr unsigned int kVecSampleNumBSub = MAX(kVecSampleNumBSubMin, kVecSampleNumOut);
    static constexpr unsigned int kNumExtractions =
        kVecSampleNumB / kVecSampleNumBSub; // this is how many sub-vectors we will need to extract from vector B.
    static constexpr unsigned int kVecNumA = TP_DIM_A / kVecSampleNumA;
    static constexpr unsigned int kVecNumB = TP_DIM_B / kVecSampleNumB;

    static constexpr unsigned int pwrsOf2 =
        fnCheckIfPwr2<TP_DIM_A>() & fnCheckIfPwr2<TP_DIM_B>(); // only exploit budget if nice-round-numbers.

    // TODO: consider if this can be optimized by factorizing into primes?
    static constexpr unsigned int kUnrollBudget =
        pwrsOf2 ? getUnrollBudget<out_t>()
                : 0; // This denotes how many unrolls we can delegate between the i, j, k loops.
    static constexpr unsigned int kUnrollsK = MIN(kVecNumB, fnPwr2<kUnrollBudget>());
    static constexpr unsigned int kUnrollBudgetAfterK = kUnrollBudget - fnLog2<kUnrollsK>();
    static constexpr unsigned int kUnrollsJ = MIN(kVecSampleNumA, fnPwr2<kUnrollBudgetAfterK>());
    static constexpr unsigned int kUnrollBudgetAfterJ = kUnrollBudgetAfterK - fnLog2<kUnrollsJ>();
    static constexpr unsigned int kUnrollsI = MIN(kVecNumA, fnPwr2<kUnrollBudgetAfterK>());

    static constexpr unsigned int kChessNumI = kVecNumA / kUnrollsI;
    static constexpr unsigned int kChessNumJ = kVecSampleNumA / kUnrollsJ;
    static constexpr unsigned int kChessNumK = kVecNumB / kUnrollsK;

    // Constructor
    outer_tensor(){};

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(outer_tensor::outer_tensor_main); }

    // Main function
    void outer_tensor_main(input_buffer<TT_DATA_A>& __restrict inWindowA,
                           input_buffer<TT_DATA_B>& __restrict inWindowB,
                           output_stream<out_t>* __restrict outStream);
};
}
}
}
}

#endif // _DSPLIB_OUTER_TENSOR_HPP_
