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
#ifndef _DSPLIB_CUMSUM_HPP_
#define _DSPLIB_CUMSUM_HPP_

/*
Cumsum Kernel.
This file exists to capture the definition of the Cumsum kernel class.
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

//#include "cumsum_traits.hpp"
#include <vector>
#include <array>
#include <adf.h>
#include "device_defs.h"

using namespace adf;

//#define _DSPLIB_CUMSUM_HPP_DEBUG_
#ifndef CEIL
#define CEIL(x, y) (((x + y - 1) / y) * y)
#endif // CEIL

#include "cumsum_traits.hpp"

namespace xf {
namespace dsp {
namespace aie {
namespace cumsum {

//-----------------------------------------------------------------------------------------------------
// Cumsum kernel entry class
template <typename TT_DATA,
          typename TT_OUT_DATA,
          unsigned int TP_DIM_A,
          unsigned int TP_DIM_B,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_MODE,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_SAT>
class cumsumBase {
   private:
    static constexpr unsigned int kSamplesInVect = __ALIGN_BYTE_SIZE__ / sizeof(TT_DATA);

   public:
    // Constructor
    cumsumBase(){};

    void cumsumBase_mode0(input_buffer<TT_DATA>& __restrict inWindow, output_buffer<TT_OUT_DATA>& __restrict outWindow);
    void cumsumBase_mode0_accacc(input_buffer<TT_DATA>& __restrict inWindow,
                                 output_buffer<TT_OUT_DATA>& __restrict outWindow);
    void cumsumBase_mode1(input_buffer<TT_DATA>& __restrict inWindow, output_buffer<TT_OUT_DATA>& __restrict outWindow);
    void cumsumBase_mode2(input_buffer<TT_DATA>& __restrict inWindow, output_buffer<TT_OUT_DATA>& __restrict outWindow);

    // Main function
    void cumsumBase_main(input_buffer<TT_DATA>& __restrict inWindow, output_buffer<TT_OUT_DATA>& __restrict outWindow);
};

//-----------------------------------------------------------------------------------------------------
// Cumsum kernel entry class
template <typename TT_DATA,
          typename TT_OUT_DATA,
          unsigned int TP_DIM_A,
          unsigned int TP_DIM_B,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_MODE,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_SAT>
class cumsum
    : public cumsumBase<TT_DATA, TT_OUT_DATA, TP_DIM_A, TP_DIM_B, TP_NUM_FRAMES, TP_MODE, TP_SHIFT, TP_RND, TP_SAT> {
   private:
   public:
    // Constructor
    cumsum(){};

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(cumsum::cumsum_main); }

    // Main function
    void cumsum_main(input_buffer<TT_DATA>& __restrict inWindow, output_buffer<TT_OUT_DATA>& __restrict outWindow);
};
}
}
}
}

#endif // _DSPLIB_CUMSUM_HPP_
