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

#include <adf.h>
#include "mixed_radix_fft_graph.hpp"

using namespace adf;

namespace fft_example {

#define LOC_XBASE 1
#define LOC_YBASE 1
#define DATA_TYPE_FFT cint16
#define TWIDDLE_TYPE cint16
#define POINT_SIZE 432
#define FFT_NIFFT 1
#define SHIFT 8
#define ROUND_MODE 0
#define SAT_MODE 1
#define WINDOW_VSIZE 432
//#define CASC_LEN 1        //currently not supported
//#define DYN_PT_SIZE 0     //currently not supported
//#define API_IO 1          //currently not supported
//#define PARALLEL_POWER 1  //currently not supported

class test_fft : public graph {
   public:
    xf::dsp::aie::port_array<input, 1> in;
    xf::dsp::aie::port_array<output, 1> out;
    xf::dsp::aie::fft::mixed_radix_fft::mixed_radix_fft_graph<DATA_TYPE_FFT,
                                                              TWIDDLE_TYPE,
                                                              POINT_SIZE,
                                                              FFT_NIFFT,
                                                              SHIFT,
                                                              ROUND_MODE,
                                                              SAT_MODE,
                                                              WINDOW_VSIZE>
        fftGraph; // instance name
    test_fft() {
        // make connections
        connect<>(in[0], fftGraph.in[0]);
        connect<>(fftGraph.out[0], out[0]);
        location<kernel>(fftGraph.m_mixed_radix_fftKernels[0]) = tile(LOC_XBASE, LOC_YBASE);
    };
}; // end of class
};
