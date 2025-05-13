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
#ifndef _DSPLIB_BITONIC_SORT_REF_HPP_
#define _DSPLIB_BITONIC_SORT_REF_HPP_

/*
Bitonic Sort reference model
*/

#include <adf.h>
#include <limits>
#include <array>
#include "device_defs.h"
#include "fir_ref_utils.hpp"
#include "bitonic_sort_ref_utils.hpp"

using namespace adf;

//#define _DSPLIB_BITONIC_SORT_REF_DEBUG_

namespace xf {
namespace dsp {
namespace aie {
namespace bitonic_sort {

template <typename TT_DATA,
          unsigned int TP_DIM,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_ASCENDING>
class bitonic_sort_ref_base { // base class with all features except interface which is left to inherited
   private:
   public:
    // Constructor
    bitonic_sort_ref_base() {}
};

//-----------------------------------------------------------------------------------------------------
// Bitonic Sort - default/base 'specialization'
template <typename TT_DATA,
          unsigned int TP_DIM,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_ASCENDING,
          unsigned int TP_CASC_LEN,
          unsigned int TP_SSR>
class bitonic_sort_ref : public bitonic_sort_ref_base<TT_DATA, TP_DIM, TP_NUM_FRAMES, TP_ASCENDING> {
   private:
   public:
    // Constructor
    bitonic_sort_ref() {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(bitonic_sort_ref::bitonic_sort_main); }

    void bitonic_sort_main(input_buffer<TT_DATA>& inWindow0, output_buffer<TT_DATA>& outWindow0);
};
}
}
}
} // closing namespace xf::dsp::aie::bitonic_sort

#endif // _DSPLIB_BITONIC_SORT_REF_HPP_
