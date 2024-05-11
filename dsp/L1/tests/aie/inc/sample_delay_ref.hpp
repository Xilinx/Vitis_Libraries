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
#ifndef _DSPLIB_SAMPLE_DELAY_REF_HPP_
#define _DSPLIB_SAMPLE_DELAY_REF_HPP_

/*
sample delay reference model
*/

#include <adf.h>

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

class sample_delay_ref {
   private:
    bool startFlag = true;
    unsigned int inItrOffset = 0;
    unsigned int prevInItrOffset = 0;
    unsigned int sampleDelayValueLoc = 0;
    unsigned int prevSampleDelayValue = 0;
    const static unsigned int bufMargin = TP_MAX_DELAY;

   public:
    // default constructor
    sample_delay_ref() {}
    // Constructor arg
    sample_delay_ref(unsigned int sampleDelayValue) {
        inItrOffset = bufMargin - sampleDelayValue;
        prevInItrOffset = sampleDelayValue;
    }
    // register kernel class
    static void registerKernelClass() { REGISTER_FUNCTION(sample_delay_ref::sampleDelayMain); }

    // function definitions
    void sampleDelayMain(input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<bufMargin> >& inWindow0,
                         output_circular_buffer<TT_DATA>& outWindow0,
                         const unsigned int sampleDelayValue);
};
}
}
}
} // namespace brackets

#endif // _DSPLIB_SAMPLE_DELAY_REF_HPP_
