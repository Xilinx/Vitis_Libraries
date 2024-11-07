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
/*
Outer Tensor Product reference model
*/
#include "device_defs.h"
#include "bitonic_sort_ref.hpp"
#include "bitonic_sort_ref_utils.hpp"
#include "aie_api/aie_adf.hpp"
#include "fir_ref_utils.hpp"

// #define _DSPLIB_BITONIC_SORT_REF_DEBUG_

namespace xf {
namespace dsp {
namespace aie {
namespace bitonic_sort {

// Bitonic Sort - default/base 'specialization' for both static and dynamic point size
template <typename TT_DATA,
          unsigned int TP_DIM,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_ASCENDING,
          unsigned int TP_CASC_LEN>
void bitonic_sort_ref<TT_DATA, TP_DIM, TP_NUM_FRAMES, TP_ASCENDING, TP_CASC_LEN>::bitonic_sort_main(
    input_buffer<TT_DATA>& inWindow0, output_buffer<TT_DATA>& outWindow0) {
    TT_DATA d_in, d_out;
    TT_DATA* inPtr = (TT_DATA*)inWindow0.data();
    TT_DATA* outPtr = (TT_DATA*)outWindow0.data();

    // Processing of one window
    for (int frame = 0; frame < TP_NUM_FRAMES; frame++) {
        bitonicSortRecursive(inPtr, 0, TP_DIM, TP_ASCENDING);

        for (int i = 0; i < TP_DIM; i++) {
            outPtr[i] = inPtr[i];
        }

        outPtr += TP_DIM;
        inPtr += TP_DIM;
    }
}
};
}
}
} // closing namespace xf::dsp::aie::bitonic_sort
