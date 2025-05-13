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
#ifndef _DSPLIB_KRONECKER_HPP_
#define _DSPLIB_KRONECKER_HPP_

/*
Kronecker Kernel.
This file capture the definition of the Kronecker kernel class.
*/

/* Coding conventions
   TT_      template type suffix
   TP_      template parameter suffix
*/

/* Design Notes

*/

#include <vector>
#include <array>
#include <adf.h>
#include "device_defs.h"
#include "fir_utils.hpp"
#include "kronecker_utils.hpp"

//#define _DSPLIB_KRONECKER_HPP_DEBUG_

using namespace adf;
namespace xf {
namespace dsp {
namespace aie {
namespace kronecker {

//-----------------------------------------------------------------------------------------------------

// kernel class - windows (io buffer) i/f
template <typename TT_DATA_A,
          typename TT_DATA_B,
          unsigned int TP_DIM_A_ROWS,
          unsigned int TP_DIM_A_COLS,
          unsigned int TP_DIM_B_ROWS,
          unsigned int TP_DIM_B_COLS,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_API,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_SAT>
class kronecker {
   private:
    static constexpr unsigned int kSamplesInVect = 256 / 8 / sizeof(outTypeMult_t<TT_DATA_A, TT_DATA_B>);

   public:
    using acc_t = accTypeMult_t<TT_DATA_A, TT_DATA_B>;
    using out_t = outTypeMult_t<TT_DATA_A, TT_DATA_B>;

    static constexpr unsigned int kSamplesInVectA = vecSampleNum<TT_DATA_A, TT_DATA_B>().A;
    static constexpr unsigned int kSamplesInVectB = vecSampleNum<TT_DATA_A, TT_DATA_B>().B;
    static constexpr unsigned int kVecSampleNumAcc = vecSampleNum<TT_DATA_A, TT_DATA_B>().Acc;
    static constexpr unsigned int kSamplesInVectOut = vecSampleNum<TT_DATA_A, TT_DATA_B>().Out;
    static constexpr unsigned int kSamplesInVectTempOut = vecSampleNum<TT_DATA_A, TT_DATA_B>().TempOut;
    static constexpr unsigned int sizeMatA = (TP_DIM_A_ROWS * TP_DIM_A_COLS);
    static constexpr unsigned int sizeMatB = TP_DIM_B_ROWS * TP_DIM_B_COLS;
    static constexpr unsigned int sizeMatOut = sizeMatA * sizeMatB;
    static constexpr unsigned int kMulCallsPerFrame = sizeMatA * sizeMatB;

    // Constructor
    kronecker(){};

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(kronecker::kronecker_main); }

    // Main function
    void kronecker_main(input_buffer<TT_DATA_A>& __restrict inWindowA,
                        input_buffer<TT_DATA_B>& __restrict inWindowB,
                        output_buffer<out_t>& __restrict outWindow);
};

// kernel class - stream i/o interface.
template <typename TT_DATA_A,
          typename TT_DATA_B,
          unsigned int TP_DIM_A_ROWS,
          unsigned int TP_DIM_A_COLS,
          unsigned int TP_DIM_B_ROWS,
          unsigned int TP_DIM_B_COLS,
          unsigned int TP_NUM_FRAMES,
          // unsigned int TP_API,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_SAT>
class kronecker<TT_DATA_A,
                TT_DATA_B,
                TP_DIM_A_ROWS,
                TP_DIM_A_COLS,
                TP_DIM_B_ROWS,
                TP_DIM_B_COLS,
                TP_NUM_FRAMES,
                1,
                TP_SHIFT,
                TP_RND,
                TP_SAT> {
   private:
    static constexpr unsigned int kSamplesInVect = 256 / 8 / sizeof(outTypeMult_t<TT_DATA_A, TT_DATA_B>);

   public:
    using acc_t = accTypeMult_t<TT_DATA_A, TT_DATA_B>;
    using out_t = outTypeMult_t<TT_DATA_A, TT_DATA_B>;
    static constexpr unsigned int sizeMatA = (TP_DIM_A_ROWS * TP_DIM_A_COLS);
    static constexpr unsigned int sizeMatB = TP_DIM_B_ROWS * TP_DIM_B_COLS;
    static constexpr unsigned int sizeMatOut = sizeMatA * sizeMatB;
    static constexpr unsigned int kVecInFrame = sizeMatB / kSamplesInVect;
    static constexpr unsigned int kSamplesInVectA = vecSampleNum<TT_DATA_A, TT_DATA_B>().A;
    static constexpr unsigned int kSamplesInVectB = vecSampleNum<TT_DATA_A, TT_DATA_B>().B;
    static constexpr unsigned int kVecSampleNumAcc = vecSampleNum<TT_DATA_A, TT_DATA_B>().Acc;
    static constexpr unsigned int kSamplesInVectOut = vecSampleNum<TT_DATA_A, TT_DATA_B>().Out;
    static constexpr unsigned int kSamplesInVectTempOut = vecSampleNum<TT_DATA_A, TT_DATA_B>().TempOut;

    // Constructor
    kronecker(){};

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(kronecker::kronecker_main); }
    // Main function
    void kronecker_main(input_buffer<TT_DATA_A>& __restrict inWindowA,
                        input_buffer<TT_DATA_B>& __restrict inWindowB,
                        output_stream<out_t>* __restrict outStream);
};
}
}
}
}

#endif // _DSPLIB_KRONECKER_HPP_
