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
/*
sample_delay reference model implements a scalar circular buffer for adding delay to the input data.
*/
#include <adf.h>
#include "aie_api/aie_adf.hpp"
#include "sample_delay_ref.hpp"
#include <iostream>

using namespace adf;
namespace xf {
namespace dsp {
namespace aie {
namespace sample_delay {
//-----------------------------------------------------------------------------------------------------
template <typename TT_DATA, unsigned int TP_WINDOW_VSIZE, unsigned int TP_API, unsigned int TP_MAX_DELAY>
void sample_delay_ref<TT_DATA, TP_WINDOW_VSIZE, TP_API, TP_MAX_DELAY>::sampleDelayMain(
    input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<bufMargin> >& inBuff,
    output_circular_buffer<TT_DATA>& outBuff,
    const unsigned int sampleDelayValue) {
    auto inItr = ::aie::begin_random_circular(inBuff);
    auto outItr = ::aie::begin_random_circular(outBuff);

    // check if the sampleDelayValue is within the range (0, MAX_DELAY)
    if (sampleDelayValue > TP_MAX_DELAY) {
        sampleDelayValueLoc = TP_MAX_DELAY;
    } else {
        sampleDelayValueLoc = sampleDelayValue;
    }

    if ((startFlag) || (sampleDelayValueLoc != prevSampleDelayValue)) {
        startFlag = false;
        inItrOffset = (bufMargin - sampleDelayValueLoc); // note: buffer margin is set to TP_MAX_DELAY
        prevSampleDelayValue = sampleDelayValueLoc;
        prevInItrOffset = inItrOffset;
    } else {
        inItrOffset = prevInItrOffset;
    }

    inItr += inItrOffset;
    for (unsigned int i = 0; i < TP_WINDOW_VSIZE; i++) {
        *outItr++ = *inItr++;

        /*printf("for loop index i: %d \n", i );
        printf("sampleDelayValueTemp   %d \n", sampleDelayValueTemp);
        printf("inItrOffset   %d \n", inItrOffset);
        printf("inItrOffsetV: %d \n",  inItrOffsetV);
        vPrintf("vPtr_0 is: ", *vPtr_0);
        printf("\n");
        vPrintf("vPtr is: ", *vPtr);
        printf("\n");
        vPrintf("vPtr_1 is : ", vPtr_1);
        printf("\n"); */
        // printf("index i is %d: \n", i);
        // printf("index i is %d: \n", *inItr);
    }
};
}
}
}
}
