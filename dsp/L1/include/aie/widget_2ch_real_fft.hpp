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
#ifndef _DSPLIB_WIDGET_2CH_REAL_FFT_HPP_
#define _DSPLIB_WIDGET_2CH_REAL_FFT_HPP_

/*
Widget 2-Channel Real FFT Kernel.
This file exists to capture the definition of the Widget 2-Channel Real FFT kernel class.
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

// #include <vector>
#include <array>
#include <adf.h>
#include "device_defs.h"
#include "fir_utils.hpp"
// #include "widget_2ch_real_fft_traits.hpp"

using namespace adf;

//#define _DSPLIB_WIDGET_2CH_REAL_FFT_HPP_DEBUG_

namespace xf {
namespace dsp {
namespace aie {
namespace widget_2ch_real_fft {

//-----------------------------------------------------------------------------------------------------
// Widget 2-Channel Real FFT kernel base class - windows (io buffer) interface
template <typename TT_DATA, unsigned int TP_POINT_SIZE, unsigned int TP_WINDOW_VSIZE>
class widget_2ch_real_fft {
   private:
   public:
    static constexpr unsigned int kVecSampleNum = 256 / 8 / sizeof(TT_DATA);
    static constexpr unsigned int kNumVecs = TP_POINT_SIZE / kVecSampleNum;
    static constexpr unsigned int kNumFrames = TP_WINDOW_VSIZE / TP_POINT_SIZE;

    // Constructor
    widget_2ch_real_fft() {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(widget_2ch_real_fft::widget_2ch_real_fft_main); }

    // Main function
    void widget_2ch_real_fft_main(input_buffer<TT_DATA>& __restrict inWindow,
                                  output_buffer<TT_DATA>& __restrict outWindow);
};
}
}
}
}

#endif // _DSPLIB_WIDGET_2CH_REAL_FFT_HPP_
