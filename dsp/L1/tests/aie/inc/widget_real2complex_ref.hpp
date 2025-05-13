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
#ifndef _DSPLIB_WIDGET_REAL2COMPLEX_REF_HPP_
#define _DSPLIB_WIDGET_REAL2COMPLEX_REF_HPP_

/*
Widget API Cast reference model
*/

#include <adf.h>
#include <limits>
#include "fir_ref_utils.hpp"

using namespace adf;

#define _DSPLIB_WIDGET_REAL2COMPLEX_REF_DEBUG_

namespace xf {
namespace dsp {
namespace aie {
namespace widget {
namespace real2complex {

//-----------------------------------------------------------------------------------------------------
// Widget real2complex - default/base 'specialization'
// int16 to cint16
template <typename TT_DATA, typename TT_OUT_DATA, unsigned int TP_WINDOW_VSIZE>
class widget_real2complex_ref {
   private:
   public:
    // Constructor
    widget_real2complex_ref() {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(widget_real2complex_ref::convertData); }
    // FIR
    void convertData(input_buffer<TT_DATA>& inWindow0, output_buffer<TT_OUT_DATA>& outWindow0);
};

template <unsigned int TP_WINDOW_VSIZE>
class widget_real2complex_ref<int32, cint32, TP_WINDOW_VSIZE> {
   private:
   public:
    // Constructor
    widget_real2complex_ref() {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(widget_real2complex_ref::convertData); }
    // FIR
    void convertData(input_buffer<int32>& inWindow0, output_buffer<cint32>& outWindow0);
};

template <unsigned int TP_WINDOW_VSIZE>
class widget_real2complex_ref<float, cfloat, TP_WINDOW_VSIZE> {
   private:
   public:
    // Constructor
    widget_real2complex_ref() {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(widget_real2complex_ref::convertData); }
    // FIR
    void convertData(input_buffer<float>& inWindow0, output_buffer<cfloat>& outWindow0);
};

template <unsigned int TP_WINDOW_VSIZE>
class widget_real2complex_ref<cint16, int16, TP_WINDOW_VSIZE> {
   private:
   public:
    // Constructor
    widget_real2complex_ref() {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(widget_real2complex_ref::convertData); }
    // FIR
    void convertData(input_buffer<cint16>& inWindow0, output_buffer<int16>& outWindow0);
};

template <unsigned int TP_WINDOW_VSIZE>
class widget_real2complex_ref<cint32, int32, TP_WINDOW_VSIZE> {
   private:
   public:
    // Constructor
    widget_real2complex_ref() {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(widget_real2complex_ref::convertData); }
    // FIR
    void convertData(input_buffer<cint32>& inWindow0, output_buffer<int32>& outWindow0);
};

template <unsigned int TP_WINDOW_VSIZE>
class widget_real2complex_ref<cfloat, float, TP_WINDOW_VSIZE> {
   private:
   public:
    // Constructor
    widget_real2complex_ref() {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(widget_real2complex_ref::convertData); }
    // FIR
    void convertData(input_buffer<cfloat>& inWindow0, output_buffer<float>& outWindow0);
};
}
}
}
}
}

#endif // _DSPLIB_WIDGET_REAL2COMPLEX_REF_HPP_
