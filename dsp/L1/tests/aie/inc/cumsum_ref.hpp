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
#ifndef _DSPLIB_CUMSUM_REF_HPP_
#define _DSPLIB_CUMSUM_REF_HPP_

/*
Cumsum reference model
*/

#include <adf.h>
#include <limits>
#include <array>
#include "device_defs.h"

using namespace adf;

//#define _DSPLIB_CUMSUM_REF_DEBUG_

namespace xf {
namespace dsp {
namespace aie {
namespace cumsum {

template <typename TT_DATA,
          typename TT_OUT_DATA,
          unsigned int TP_DIM_A,
          unsigned int TP_DIM_B,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_MODE,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_SAT>
class cumsum_ref { // base class with all features except interface which is left to inherited
   private:
   public:
    // Constructor
    cumsum_ref() {}

    // main function
    static void registerKernelClass() { REGISTER_FUNCTION(cumsum_ref::cumsum_ref_main); }
    void cumsum_ref_main(input_buffer<TT_DATA>& inWindow0, output_buffer<TT_OUT_DATA>& outWindow0);
};
}
}
}
}

#endif // _DSPLIB_CUMSUM_REF_HPP_
