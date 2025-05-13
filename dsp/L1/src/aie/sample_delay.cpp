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
This file captures the body of run-time code for the kernel class.
Sample delay implements a delay filter with a run time parameter for setting the delay.The unit of delay is 'number of
samples'.

Coding conventions
  TT_      template type suffix
  TP_      template parameter suffix
*/

#pragma once
#include <adf.h>
#include <vector>
#include "device_defs.h"
#include "aie_api/aie_adf.hpp"
#include "kernel_api_utils.hpp"
#include "sample_delay.hpp"
//#include "debug_utils.h"
//#include "sample_delay_traits.hpp"
//#include "sample_delay_utils.hpp"

using namespace adf;
namespace xf {
namespace dsp {
namespace aie {
namespace sample_delay {

// IO buffer interface
template <typename TT_DATA,
          unsigned int TP_WINDOW_VSIZE,
          unsigned int TP_API,
          unsigned int TP_MAX_DELAY>

NOINLINE_DECL // This function is the hook for QoR profiling, so must be identifiable after compilation.
    void
    sample_delay<TT_DATA, TP_WINDOW_VSIZE, TP_API, TP_MAX_DELAY>::sampleDelayMain(
        input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<bufMargin> >& __restrict inBuff,
        output_circular_buffer<TT_DATA>& __restrict outBuff,
        const unsigned int sampleDelayValue) {
    constexpr int vecSize = 256 / 8 / sizeof(TT_DATA);
    constexpr int stepSize = TP_WINDOW_VSIZE / vecSize;
    using t_vect = ::aie::vector<TT_DATA, vecSize>;
    auto inItr = ::aie::begin_vector_random_circular<vecSize>(inBuff);
    auto outItr = ::aie::begin_vector_random_circular<vecSize>(outBuff);

    // check if the sampleDelayValue is within the range (0, MAX_DELAY)
    if (sampleDelayValue > TP_MAX_DELAY) {
        sampleDelayValueTemp = TP_MAX_DELAY;
    } else {
        sampleDelayValueTemp = sampleDelayValue;
    }

    if ((startFlag) || (prevsampleDelayValue != sampleDelayValueTemp)) {
        startFlag = false;
        inItrOffsetV = (sampleDelayValueTemp % vecSize);
        prevInItrOffsetV = inItrOffsetV;
        prevsampleDelayValue = sampleDelayValueTemp;

        if (inItrOffsetV != 0) {
            rdItrOffset =
                (bufMargin - sampleDelayValueTemp + vecSize) / vecSize; // note: buffer_margin is equal to TP_MAX_DELAY
        } else {
            rdItrOffset = (bufMargin - sampleDelayValueTemp) / vecSize;
        }
        prevInItrOffset = rdItrOffset;
    } else {
        rdItrOffset = prevInItrOffset;
        inItrOffsetV = prevInItrOffsetV;
    }

    inItr += rdItrOffset;
    t_vect* vPtrCurrent = (t_vect*)&*inItr;
    t_vect* vPtrPast = --vPtrCurrent;
    vPtrCurrent++;

    for (unsigned int i = 0; i < stepSize; i++) chess_prepare_for_pipelining chess_loop_range(stepSize, ) {
            t_vect vPtr_1 = ::aie::shuffle_up_fill(*vPtrCurrent++, *vPtrPast++, inItrOffsetV);
            *outItr++ = vPtr_1;
        }
};

// Stream interface
template <typename TT_DATA,
          unsigned int TP_WINDOW_VSIZE,
          unsigned int TP_MAX_DELAY>
NOINLINE_DECL // This function is the hook for QoR profiling, so must be identifiable after compilation.
    void
    sample_delay<TT_DATA, TP_WINDOW_VSIZE, 1, TP_MAX_DELAY>::sampleDelayMain(
        input_stream<TT_DATA>* __restrict inStream,
        output_stream<TT_DATA>* __restrict outStream,
        const unsigned int sampleDelayValue) {
    using t_vect = ::aie::vector<TT_DATA, vecSize>;
    unsigned int sampleDelayValueTemp = 0;

    // mini cache write iterator
    auto cacheWrItr = ::aie::begin_vector_random_circular<vecSize>(miniCache, miniCacheSize);
    // mini cache read iterator
    auto cacheRdItr = ::aie::begin_vector_random_circular<vecSize>(miniCache, miniCacheSize);
    auto cacheRdItr0 =
        ::aie::begin_vector_random_circular<vecSize>(miniCache, miniCacheSize); // will point to cacheItr--

    // check if the sampleDelayValue is within the range (0, MAX_DELAY -1)
    if (sampleDelayValue > TP_MAX_DELAY) {
        sampleDelayValueTemp = TP_MAX_DELAY;
    } else {
        sampleDelayValueTemp = sampleDelayValue;
    }

    // calculate element delay offset
    rdItrOffsetV = ((sampleDelayValueTemp) % vecSize);

    // calculate vector delay offset
    if (rdItrOffsetV != 0) {
        rdItrOffset = ((cacheMrgn - sampleDelayValueTemp) / vecSize) + 1;
    } else {
        rdItrOffset = (cacheMrgn - sampleDelayValueTemp) / vecSize;
    }

    if (startFlag) {
        wrItrOffset = cacheMrgn / vecSize;
        startFlag = false;
        prevSampleDelayValue = sampleDelayValueTemp;

    } else if ((prevSampleDelayValue != sampleDelayValueTemp) && (sampleDelayValueTemp != 0)) {
        prevSampleDelayValue = sampleDelayValueTemp;
    } else {
    }

    cacheWrItr += wrItrOffset;
    cacheRdItr += rdItrOffset; // apply vector delay offset
    cacheRdItr0 += rdItrOffset;
    cacheRdItr0--; // points to cacheRdItr--

    for (unsigned int i = 0; i < frameSize; i++) chess_prepare_for_pipelining chess_loop_range(frameSize, ) {
            *cacheWrItr++ = readincr_v<vecSize, aie_stream_resource_in::a, TT_DATA>(inStream);
            t_vect dataOut =
                ::aie::shuffle_up_fill(*cacheRdItr++, *cacheRdItr0++, rdItrOffsetV); // apply element delay offset
            writeincr<aie_stream_resource_out::a, TT_DATA, vecSize>(outStream, dataOut);
        }
};
}
}
}
}
