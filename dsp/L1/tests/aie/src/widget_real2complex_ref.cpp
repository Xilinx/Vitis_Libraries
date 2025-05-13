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
Widget API Cast reference model
*/

#include "widget_real2complex_ref.hpp"
#include "fir_ref_utils.hpp"

namespace xf {
namespace dsp {
namespace aie {
namespace widget {
namespace real2complex {

// Widget real2complex - default/base 'specialization'
// int16 to cint16
template <typename TT_DATA, typename TT_OUT_DATA, unsigned int TP_WINDOW_VSIZE>
void widget_real2complex_ref<TT_DATA, TT_OUT_DATA, TP_WINDOW_VSIZE>::convertData(
    input_buffer<TT_DATA>& inWindow0, output_buffer<TT_OUT_DATA>& outWindow0) {
    TT_DATA* inPtr = (TT_DATA*)inWindow0.data();
    TT_OUT_DATA* outPtr = (TT_OUT_DATA*)outWindow0.data();
    TT_DATA d_in;
    TT_OUT_DATA d_out;
    d_out.imag = 0;

    for (unsigned int i = 0; i < TP_WINDOW_VSIZE; i++) {
        d_in = *inPtr++; // read input data
        d_out.real = d_in;
        *outPtr++ = d_out;
    }
};

// int32 to cint32
template <unsigned int TP_WINDOW_VSIZE>
void widget_real2complex_ref<int32, cint32, TP_WINDOW_VSIZE>::convertData(input_buffer<int32>& inWindow0,
                                                                          output_buffer<cint32>& outWindow0) {
    int32* inPtr = (int32*)inWindow0.data();
    cint32* outPtr = (cint32*)outWindow0.data();
    int32 d_in;
    cint32 d_out;
    d_out.imag = 0;

    for (unsigned int i = 0; i < TP_WINDOW_VSIZE; i++) {
        d_in = *inPtr++; // read input data
        d_out.real = d_in;
        *outPtr++ = d_out;
    }
};

// float to cfloat
template <unsigned int TP_WINDOW_VSIZE>
void widget_real2complex_ref<float, cfloat, TP_WINDOW_VSIZE>::convertData(input_buffer<float>& inWindow0,
                                                                          output_buffer<cfloat>& outWindow0) {
    float* inPtr = (float*)inWindow0.data();
    cfloat* outPtr = (cfloat*)outWindow0.data();
    float d_in;
    cfloat d_out;
    d_out.imag = 0.0;

    for (unsigned int i = 0; i < TP_WINDOW_VSIZE; i++) {
        d_in = *inPtr++; // read input data
        d_out.real = d_in;
        *outPtr++ = d_out;
    }
};

// cint16 to int16
template <unsigned int TP_WINDOW_VSIZE>
void widget_real2complex_ref<cint16, int16, TP_WINDOW_VSIZE>::convertData(input_buffer<cint16>& inWindow0,
                                                                          output_buffer<int16>& outWindow0) {
    cint16* inPtr = (cint16*)inWindow0.data();
    int16* outPtr = (int16*)outWindow0.data();
    cint16 d_in;
    int16 d_out;

    for (unsigned int i = 0; i < TP_WINDOW_VSIZE; i++) {
        d_in = *inPtr++; // read input data
        d_out = d_in.real;
        *outPtr++ = d_out;
    }
};

// cint32 to int32
template <unsigned int TP_WINDOW_VSIZE>
void widget_real2complex_ref<cint32, int32, TP_WINDOW_VSIZE>::convertData(input_buffer<cint32>& inWindow0,
                                                                          output_buffer<int32>& outWindow0) {
    cint32* inPtr = (cint32*)inWindow0.data();
    int32* outPtr = (int32*)outWindow0.data();
    cint32 d_in;
    int32 d_out;

    for (unsigned int i = 0; i < TP_WINDOW_VSIZE; i++) {
        d_in = *inPtr++; // read input data
        d_out = d_in.real;
        *outPtr++ = d_out;
    }
};

// cfloat to float
template <unsigned int TP_WINDOW_VSIZE>
void widget_real2complex_ref<cfloat, float, TP_WINDOW_VSIZE>::convertData(input_buffer<cfloat>& inWindow0,
                                                                          output_buffer<float>& outWindow0) {
    cfloat* inPtr = (cfloat*)inWindow0.data();
    float* outPtr = (float*)outWindow0.data();
    cfloat d_in;
    float d_out;

    for (unsigned int i = 0; i < TP_WINDOW_VSIZE; i++) {
        d_in = *inPtr++; // read input data
        d_out = d_in.real;
        *outPtr++ = d_out;
    }
};
}
}
}
}
}
