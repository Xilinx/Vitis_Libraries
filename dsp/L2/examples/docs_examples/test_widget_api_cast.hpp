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
#include "widget_api_cast_graph.hpp"

using namespace adf;
namespace widg1_example {
#define DATA_TYPE_WIDG1 cint16
#define IN_API 1
#define OUT_API 0
#define NUM_INPUTS 2
#define WINDOW_VSIZE_W 1024
#define NUM_OUTPUT_CLONES 1
#define PATTERN 1
class test_widg_api : public graph {
   public:
    xf::dsp::aie::port_array<input, NUM_INPUTS> in;
    xf::dsp::aie::port_array<output, NUM_OUTPUT_CLONES> out;
    xf::dsp::aie::widget::api_cast::
        widget_api_cast_graph<DATA_TYPE_WIDG1, IN_API, OUT_API, NUM_INPUTS, WINDOW_VSIZE_W, NUM_OUTPUT_CLONES, PATTERN>
            widget;
    test_widg_api() {
        for (int i = 0; i < NUM_INPUTS; i++) {
            connect<>(in[i], widget.in[i]);
        }
        for (int i = 0; i < NUM_OUTPUT_CLONES; i++) {
            connect<>(widget.out[i], out[i]);
        }
        kernel* kernels = widget.getKernels();
        runtime<ratio>(*kernels) = 0.5;
    };
};
};