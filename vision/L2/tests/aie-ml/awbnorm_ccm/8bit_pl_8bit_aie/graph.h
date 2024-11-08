/*
 * Copyright 2021 Xilinx, Inc.
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

#ifndef ADF_GRAPH_H
#define ADF_GRAPH_H

#include <adf.h>
#include <array>
#include <type_traits>

#include "kernels.h"

using namespace adf;

class myGraph : public adf::graph {
   private:
    kernel k1;

   public:
    port<input> coeff;
    input_plio in1;
    output_plio out1;

    myGraph() {
        k1 = kernel::create(awbNormCCM<T, 32>);

        in1 = input_plio::create("DataIn1", adf::plio_128_bits, "data/input.txt");
        out1 = output_plio::create("DataOut1", adf::plio_128_bits, "data/output.txt");

        connect<>(in1.out[0], k1.in[0]);
        connect<parameter>(coeff, async(k1.in[1]));
        connect<>(k1.out[0], out1.in[0]);

        adf::dimensions(k1.in[0]) = {ELEM_WITH_METADATA_IN};
        adf::dimensions(k1.out[0]) = {ELEM_WITH_METADATA_OUT};

        source(k1) = "xf_awbnorm_ccm.cc";

        runtime<ratio>(k1) = 0.5;
    };
};

#endif
