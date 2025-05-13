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

// This file holds the definition of the test harness for the example 2D FFT/IFFT graph.

#include <adf.h>
#include "fft_ifft_2d_graph.hpp"

#define DATA_TYPE_D1 int16
#define DATA_TYPE_D2 cint16
#define TWIDDLE_TYPE cint16
#define POINT_SIZE_D1 128
#define POINT_SIZE_D2 64
#define FFT_NIFFT 1
#define SHIFT 8
#define CASC_LEN 1
#define API_IO 0
#define WINDOW_VSIZE_D1 128
#define WINDOW_VSIZE_D2 64
#define TWIDDLE_MODE 0
#define ROUND_MODE 11
#define SAT_MODE 1

using namespace adf;

namespace fft_2d_example {

class test_fft_2d : public graph {
   public:
    xf::dsp::aie::fft::two_d::fft_ifft_2d_graph<DATA_TYPE_D1,
                                                DATA_TYPE_D2,
                                                TWIDDLE_TYPE,
                                                POINT_SIZE_D1,
                                                POINT_SIZE_D2,
                                                FFT_NIFFT,
                                                SHIFT,
                                                CASC_LEN,
                                                WINDOW_VSIZE_D1,
                                                WINDOW_VSIZE_D2,
                                                API_IO,
                                                ROUND_MODE,
                                                SAT_MODE,
                                                TWIDDLE_MODE>
        fft2dGraph;

    port<input> in;
    port<output> out;
    // Constructor
    test_fft_2d() {
        // Make connections
        connect<>(in, fft2dGraph.in[0]);
        connect<>(fft2dGraph.out[0], out);
    };
};
};
