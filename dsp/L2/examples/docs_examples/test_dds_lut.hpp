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
#include "dds_mixer_lut_graph.hpp"

namespace dds_lut_example {
#define DDS_DATA_TYPE cint16
#define DDS_INPUT_WINDOW_VSIZE 1024
#define MIXER_MODE 1
#define SFDR 90
#define USE_WINDOW_API 0

using namespace adf;

class test_dds_lut : public graph {
   public:
    port<input> in;
    port<output> out;
    static constexpr unsigned int phaseInc = 0xD6555555;
    static constexpr unsigned int initialPhaseOffset = 0;
    test_dds_lut() {
        xf::dsp::aie::mixer::dds_mixer::dds_mixer_lut_graph<DDS_DATA_TYPE, MIXER_MODE, SFDR, USE_WINDOW_API,
                                                            DDS_INPUT_WINDOW_VSIZE>
            mixer(phaseInc, initialPhaseOffset);

        connect<>(in, mixer.in1[0]);
        connect<>(mixer.out[0], out);
        kernel* kernels = mixer.getKernels();
        runtime<ratio>(*kernels) = 0.5;
    };
};
};