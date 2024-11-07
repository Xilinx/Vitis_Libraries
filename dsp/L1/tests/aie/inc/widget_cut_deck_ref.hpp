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
#ifndef _DSPLIB_widget_cut_deck_REF_HPP_
#define _DSPLIB_widget_cut_deck_REF_HPP_

/*
Widget API Cast reference model
*/

#include <adf.h>
#include <limits>
#include "device_defs.h"
#include "fir_ref_utils.hpp"
// #include "widget_cut_deck_traits.hpp"

using namespace adf;

#ifndef _DSPLIB_WIDGET_CUT_DECK_REF_DEBUG_
//#define _DSPLIB_WIDGET_CUT_DECK_REF_DEBUG_
//#include "debug_utils.h"
#endif //_DSPLIB_WIDGET_CUT_DECK_REF_DEBUG_

namespace xf {
namespace dsp {
namespace aie {
namespace widget {
namespace deck_cut {

//-----------------------------------------------------------------------------------------------------
template <typename TT_DATA, // type of data input and output
          unsigned int TP_POINT_SIZE,
          unsigned int TP_WINDOW_VSIZE,
          unsigned int TP_ENTRY> // does not include header, so refers to payload samples
class widget_cut_deck_ref {
   public:
    widget_cut_deck_ref() {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(widget_cut_deck_ref::widget_cut_deck_ref_main); }
    void widget_cut_deck_ref_main(input_buffer<TT_DATA>& inWindow0, output_buffer<TT_DATA>& outWindow0);
};
}
}
}
}
}

#endif // _DSPLIB_widget_cut_deck_REF_HPP_
