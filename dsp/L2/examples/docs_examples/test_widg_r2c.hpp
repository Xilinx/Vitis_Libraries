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
#include <adf.h>
#include "widget_real2complex_graph.hpp"

#define DATA_IN_TYPE int16
#define DATA_OUT_TYPE cint16
#define WINDOW_VSIZE_W_R2C 1024
using namespace adf;
namespace widgr2c_example {
class test_widg_r2c : public adf::graph {
   public:
    port<input> in;
    port<output> out;
    xf::dsp::aie::widget::real2complex::widget_real2complex_graph<DATA_IN_TYPE, DATA_OUT_TYPE, WINDOW_VSIZE_W_R2C>
        widget;
    test_widg_r2c() {
        connect<>(in, widget.in);
        connect<>(widget.out, out);
        kernel* kernels = widget.getKernels();
        runtime<ratio>(*kernels) = 0.5;
    };
};
};