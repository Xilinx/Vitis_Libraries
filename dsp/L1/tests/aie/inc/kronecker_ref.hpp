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
#ifndef _DSPLIB_KRONECKER_REF_HPP_
#define _DSPLIB_KRONECKER_REF_HPP_

/*
Kronecker Matrix Product reference model
*/

#include <adf.h>
#include <limits>
#include <array>
#include "device_defs.h"
#include "kronecker_ref_utils.hpp"

using namespace adf;
namespace xf {
namespace dsp {
namespace aie {
namespace kronecker {

// Kronecker ref class
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
class kronecker_ref {
   private:
   public:
    using out_t = outTypeMult_t<TT_DATA_A, TT_DATA_B>;
    static constexpr unsigned int sizeMatA = TP_DIM_A_ROWS * TP_DIM_A_COLS;
    static constexpr unsigned int sizeMatB = TP_DIM_B_ROWS * TP_DIM_B_COLS;
    static constexpr unsigned int rowsMatOut = TP_DIM_A_ROWS * TP_DIM_B_ROWS;
    static constexpr unsigned int colsMatOut = TP_DIM_A_COLS * TP_DIM_B_COLS;
    static constexpr unsigned int sizeMatOut = rowsMatOut * colsMatOut;

    // Constructor
    kronecker_ref() {}
    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(kronecker_ref::kronecker_main); }
    // Declaration of main function
    void kronecker_main(input_buffer<TT_DATA_A>& inWindow0,
                        input_buffer<TT_DATA_B>& inWindow1,
                        output_buffer<out_t>& outWindow0);
};

} // namespace kronecker
} // namespace aie
} // namespace xf
} // namespace dsp

#endif // _DSPLIB_KRONECKER_REF_HPP_
