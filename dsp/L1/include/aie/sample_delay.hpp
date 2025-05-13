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
#ifndef _DSPLIB_SAMPLE_DELAY_HPP_
#define _DSPLIB_SAMPLE_DELAY_HPP_
#ifndef _DSPLIB_SAMPLE_DELAY_HPP_DEBUG_
#define _DSPLIB_SAMPLE_DELAY_HPP_DEBUG_

/*
*/

/* Coding conventions
   TT_      template type suffix
   TP_      template parameter suffix
*/

/* Design Notes

*/

#include <adf.h>
#include "device_defs.h"
#include "aie_api/aie_adf.hpp"
#include "aie_api/aie.hpp"
//#include "kernel_api_utils.hpp"
//#include "sample_delay_traits.hpp"
//#include <vector>

using namespace adf;
namespace xf {
namespace dsp {
namespace aie {
namespace sample_delay {

//-----------------------------------------------------------------------------------------------------
// default specialization
template <typename TT_DATA, // io data type
          unsigned int TP_WINDOW_VSIZE,
          unsigned int TP_API,
          unsigned int TP_MAX_DELAY>
class kernelClass {
   private:
   public:
    // Constructor
    kernelClass() {}

    // void kernelClassMain (const TT_DATA* __restrict inBuff,
    //                            TT_DATA* __restrict outBuff);
};

//-----------------------------------------------------------------------------------------------------
// Single kernel specialization for io bufer
template <typename TT_DATA, unsigned int TP_WINDOW_VSIZE, unsigned int TP_API, unsigned int TP_MAX_DELAY>
class sample_delay : public kernelClass<TT_DATA, TP_WINDOW_VSIZE, TP_API, TP_MAX_DELAY> {
   private:
    bool startFlag = true;
    unsigned int rdItrOffset = 0;
    unsigned int prevInItrOffset = 0;
    unsigned int inItrOffsetV = 0;
    unsigned int prevInItrOffsetV = 0;
    unsigned int sampleDelayValueTemp = 0;
    unsigned int prevsampleDelayValue = 0;

   public:
    static constexpr unsigned int bufMargin = TP_MAX_DELAY;
    // Constructor
    sample_delay() {
        rdItrOffset = bufMargin;
        prevInItrOffset = 0;
    }

    // Constructor arg
    sample_delay(unsigned int sampleDelayValue) {
        rdItrOffset = bufMargin - sampleDelayValue;
        prevInItrOffset = sampleDelayValue;
    }
    // register kernel class
    static void registerKernelClass() { REGISTER_FUNCTION(sample_delay::sampleDelayMain); }

    // function definitions
    void sampleDelayMain(input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<bufMargin> >& inBuff,
                         output_circular_buffer<TT_DATA>& outBuff,
                         const unsigned int sampleDelayValue);
};

// Single kernel specialisation for streaming interface
template <typename TT_DATA, unsigned int TP_WINDOW_VSIZE, unsigned int TP_MAX_DELAY>
class sample_delay<TT_DATA, TP_WINDOW_VSIZE, 1, TP_MAX_DELAY>
    : public kernelClass<TT_DATA, TP_WINDOW_VSIZE, 1, TP_MAX_DELAY> {
   private:
    bool startFlag = true;
    unsigned int rdItrOffset = 0;
    unsigned int wrItrOffset = 0;
    unsigned int rdItrOffsetV = 0;
    // unsigned int sampleDelayValueTemp = 0;
    unsigned int prevSampleDelayValue = 0;

    static constexpr int vecSize = 256 / 8 / sizeof(TT_DATA);
    static constexpr int frameSize = TP_WINDOW_VSIZE / vecSize;
    static constexpr int perfBuffSize = 0;
    static constexpr int miniCacheSize = (TP_MAX_DELAY + perfBuffSize);
    ptrdiff_t cacheMrgn = TP_MAX_DELAY;
    alignas(__ALIGN_BYTE_SIZE__) TT_DATA miniCache[miniCacheSize] = {};

   public:
    // Constructor
    sample_delay() {}

    // Constructor arg
    sample_delay(unsigned int sampleDelayValue) {
        rdItrOffset = (cacheMrgn - sampleDelayValue) / vecSize;
        // prevRdItrOffset = sampleDelayValue;
    }
    // register kernel class
    static void registerKernelClass() { REGISTER_FUNCTION(sample_delay::sampleDelayMain); }

    // function definitions
    void sampleDelayMain(input_stream<TT_DATA>* __restrict inStream,
                         output_stream<TT_DATA>* __restrict outStream,
                         const unsigned int sampleDelayValue);
};
}
}
}
} // namespace brackets

#endif // _DSPLIB_SAMPLE_DELAY_HPP_
#endif //_DSPLIB_SAMPLE_DELAY_HPP_DEBUG_
