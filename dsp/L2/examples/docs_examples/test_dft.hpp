
/*
 * Copyright (C) 2019-2022, Xilinx, Inc.
 * Copyright (C) 2022-2023, Advanced Micro Devices, Inc.
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
#include "dft_graph.hpp"
using namespace adf;
namespace dft_example {
#define DFT_DATA_TYPE cint16
#define DFT_TWIDDLE_TYPE cint16
#define DFT_POINT_SIZE 16
#define DFT_FFT_NIFFT 1
#define DFT_SHIFT 8
#define DFT_CASC_LEN 1
#define DFT_NUM_FRAMES 8
#define DFT_ROUND_MODE 1
#define DFT_SAT_MODE 1
class test_dft : public graph {
   public:
    xf::dsp::aie::port_array<input, CASC_LEN> in;
    xf::dsp::aie::port_array<output, 1> out;
    xf::dsp::aie::fft::dft::dft_graph<DFT_DATA_TYPE,
                                      DFT_TWIDDLE_TYPE,
                                      DFT_POINT_SIZE,
                                      DFT_FFT_NIFFT,
                                      DFT_SHIFT,
                                      DFT_CASC_LEN,
                                      DFT_NUM_FRAMES,
                                      DFT_ROUND_MODE,
                                      DFT_SAT_MODE>
        dftGraph;
    test_dft() {
        // make connections
        for (int i = 0; i < DFT_CASC_LEN; i++) {
            connect<>(in[i], dftGraph.in[i]);
        }
        connect<>(dftGraph.out[0], out[0]);
    };
}; // end of class
};
