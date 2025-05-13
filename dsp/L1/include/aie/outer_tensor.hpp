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

//#define _DSPLIB_OUTER_TENSOR_HPP_DEBUG_

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
    static constexpr unsigned int vecSampleNumA = vecSampleNum<TT_DATA_A, TT_DATA_B>().A;
    static constexpr unsigned int vecSampleNumB = vecSampleNum<TT_DATA_A, TT_DATA_B>().B;
    static constexpr unsigned int vecSampleNumAcc = vecSampleNum<TT_DATA_A, TT_DATA_B>().Acc;
    static constexpr unsigned int vecSampleNumTempOut = vecSampleNum<TT_DATA_A, TT_DATA_B>().TempOut;
    static constexpr unsigned int vecSampleNumOut = vecSampleNum<TT_DATA_A, TT_DATA_B>().Out;
    static constexpr unsigned int vecNumA = TP_DIM_A / vecSampleNumA;
    static constexpr unsigned int vecNumB = CEIL(TP_DIM_B, vecSampleNumB) / vecSampleNumB;
    static constexpr unsigned int outDim = TP_DIM_A * TP_DIM_B;
    static constexpr unsigned int vecNumOut = outDim / vecSampleNumOut;
    using out_t = outTypeMult_t<TT_DATA_A, TT_DATA_B>;
    using acc_t = accTypeMult_t<TT_DATA_A, TT_DATA_B>;

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
    static constexpr unsigned int vecSampleNumA = vecSampleNum<TT_DATA_A, TT_DATA_B>().A;
    static constexpr unsigned int vecSampleNumB = vecSampleNum<TT_DATA_A, TT_DATA_B>().B;
    static constexpr unsigned int vecSampleNumAcc = vecSampleNum<TT_DATA_A, TT_DATA_B>().Acc;
    static constexpr unsigned int vecSampleNumTempOut = vecSampleNum<TT_DATA_A, TT_DATA_B>().TempOut;
    static constexpr unsigned int vecSampleNumOut = vecSampleNum<TT_DATA_A, TT_DATA_B>().Out;
    static constexpr unsigned int vecNumA = TP_DIM_A / vecSampleNumA;
    static constexpr unsigned int vecNumB = CEIL(TP_DIM_B, vecSampleNumB) / vecSampleNumB;
    static constexpr unsigned int outDim = TP_DIM_A * TP_DIM_B;
    static constexpr unsigned int vecNumOut = outDim / vecSampleNumOut;
    using out_t = outTypeMult_t<TT_DATA_A, TT_DATA_B>;
    using acc_t = accTypeMult_t<TT_DATA_A, TT_DATA_B>;

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
