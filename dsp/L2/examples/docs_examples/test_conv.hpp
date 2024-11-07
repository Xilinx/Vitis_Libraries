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
#include "conv_corr_graph.hpp"

using namespace adf;
namespace conv_example {

#define DATATYPE_F_CONV int16
#define DATATYPE_G_CONV int16
#define DATATYPE_OUT_CONV int32
#define FUNCT_TYPE_CONV 1
#define COMPUTE_MODE_CONV 0
#define F_LEN_CONV 128
#define G_LEN_CONV 32
#define SHIFT_CONV 7
#define API_CONV 0
#define CASC_LEN_CONV 1
#define PHASES_CONV 1

class test_conv : public adf::graph {
   public:
    xf::dsp::aie::port_array<input, PHASES_CONV> inF;
    xf::dsp::aie::port_array<input, 1> inG;
    xf::dsp::aie::port_array<output, PHASES_CONV> out;
    xf::dsp::aie::conv_corr::conv_corr_graph<DATATYPE_F_CONV,
                                             DATATYPE_G_CONV,
                                             DATATYPE_OUT_CONV,
                                             FUNCT_TYPE_CONV,
                                             COMPUTE_MODE_CONV,
                                             F_LEN_CONV,
                                             G_LEN_CONV,
                                             SHIFT_CONV,
                                             API_CONV,
                                             CASC_LEN_CONV,
                                             PHASES_CONV>
        conv;
    test_conv() {
        for (int i = 0; i < PHASES_CONV; i++) {
            connect<>(inF[i], conv.inF[i]);
            connect<>(conv.out[i], out[i]);
        }
        connect<>(inG[0], conv.inG);
    };
};
};