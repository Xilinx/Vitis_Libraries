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
Widget API Cast reference model
*/

#include <adf.h>
#include "aie_api/aie_adf.hpp"
#include "widget_cut_deck_ref.hpp"
#include "fir_ref_utils.hpp"

namespace xf {
namespace dsp {
namespace aie {
namespace widget {
namespace deck_cut {

// Widget API Cast - default/base 'specialization' - window to window copy (may help routing)
template <typename TT_DATA, // type of data input and output
          unsigned int TP_POINT_SIZE,
          unsigned int TP_WINDOW_VSIZE,
          unsigned int TP_ENTRY>
void widget_cut_deck_ref<TT_DATA, TP_POINT_SIZE, TP_WINDOW_VSIZE, TP_ENTRY>::widget_cut_deck_ref_main(
    input_buffer<TT_DATA>& inWindow0, output_buffer<TT_DATA>& outWindow0) {
    constexpr int kNumFrames = TP_WINDOW_VSIZE / TP_POINT_SIZE;
    TT_DATA* inPtr = inWindow0.data();
    TT_DATA* outPtr = outWindow0.data();

    if (TP_ENTRY == 1) { // separatation of samples for each FFT..
        for (int frame = 0; frame < kNumFrames / 2; frame++) {
            for (int i = 0; i < TP_POINT_SIZE * 2; i++) {
                if (i % 2 == 0) {
                    outPtr[i / 2] = inPtr[i];
                } else {
                    outPtr[i / 2 + TP_POINT_SIZE] = inPtr[i];
                }
            }
            inPtr += TP_POINT_SIZE * 2;
            outPtr += TP_POINT_SIZE * 2;
        }
    } else { // Cutting half the data..
        for (int frame = 0; frame < kNumFrames; frame++) {
            for (int i = 0; i < TP_POINT_SIZE / 2; i++) {
                *outPtr++ = *inPtr++;
            }
            inPtr += TP_POINT_SIZE / 2;
        }
    }
};
}
}
}
}
}
