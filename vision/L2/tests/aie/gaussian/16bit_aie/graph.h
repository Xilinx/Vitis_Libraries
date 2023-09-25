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

#ifndef __GRAPH_H__
#define __GRAPH_H__

#include <adf.h>
#include <common/xf_aie_const.hpp>
#include "kernels.h"

static constexpr int TILE_ELEMENTS = 2048;
static constexpr int TILE_WINDOW_SIZE = TILE_ELEMENTS * sizeof(int16_t) + xf::cv::aie::METADATA_SIZE;
static constexpr int ELEM_WITH_METADATA = TILE_ELEMENTS + (xf::cv::aie::METADATA_SIZE / sizeof(int16_t));

using namespace adf;

class gaussianGraph : public graph {
   private:
    kernel gauss1;

   public:
    input_plio in;
    output_plio out;
    port<input> kernelCoefficients;

    gaussianGraph() {
        gauss1 = kernel::create(gaussian);

        in = input_plio::create("DataIn1", adf::plio_64_bits, "data/input.txt");
        out = output_plio::create("DataOut1", adf::plio_64_bits, "data/output.txt");

        adf::connect<>(in.out[0], gauss1.in[0]);
        adf::connect<>(gauss1.out[0], out.in[0]);

        adf::dimensions(gauss1.in[0]) = {ELEM_WITH_METADATA};
        adf::dimensions(gauss1.out[0]) = {ELEM_WITH_METADATA};

        connect<parameter>(kernelCoefficients, async(gauss1.in[1]));

        source(gauss1) = "xf_gaussian.cc";

        runtime<ratio>(gauss1) = 0.5;
    };
};

#endif
