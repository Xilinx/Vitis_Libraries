/*
 * Copyright (C) 2019-2022, Xilinx, Inc.
 * Copyright (C) 2022-2023, Advanced Micro Devices, Inc.
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
#ifndef __GRAPH_MULT_HPP__
#define __GRAPH_MULT_HPP__

#include "kernel_mult.hpp"
#include <adf.h>

using namespace adf;

namespace us {
namespace L2 {

template <class T,
          int NUM_LINE_t,
          int NUM_ELEMENT_t,
          int NUM_SAMPLE_t,
          int NUM_SEG_t,
          int NUM_DEP_SEG_t,
          int VECDIM_mult_t,
          int LEN_IN_mult_t,
          int LEN_OUT_mult_t,
          int LEN32b_PARA_mult_t,
          int MULT_ID_t>
class mult_graph_wrapper : public graph {
   public:
    port<direction::in> para_const_pre;
    port<direction::in> para_const_0;
    port<direction::in> para_const_1;
    port<direction::in> para_const_2;
    port<direction::in> para_const_3;

    port<direction::in> para_local_0_0;
    port<direction::in> para_local_0_1;
    port<direction::in> para_local_0_2;
    port<direction::in> para_local_0_3;
    port<direction::in> para_local_1_0;
    port<direction::in> para_local_1_1;
    port<direction::in> para_local_1_2;
    port<direction::in> para_local_1_3;
    port<input> interp;
    port<input> apod;
    port<output> mult_0;
    port<output> mult_1;
    port<output> mult_2;
    port<output> mult_3;

    mult_graph_wrapper() {
        // kernel definition
        mult_kernel_pre =
            kernel::create(us::L1::kfun_mult_pre<T, NUM_LINE_t, NUM_ELEMENT_t, NUM_SAMPLE_t, NUM_SEG_t, NUM_DEP_SEG_t,
                                                 VECDIM_mult_t, LEN_IN_mult_t, LEN_OUT_mult_t, LEN32b_PARA_mult_t, 0>);
        mult_kernel_0 = kernel::create(
            us::L1::kfun_mult_cascade<T, NUM_LINE_t, NUM_ELEMENT_t, NUM_SAMPLE_t, NUM_SEG_t, NUM_DEP_SEG_t,
                                      VECDIM_mult_t, LEN_IN_mult_t, LEN_OUT_mult_t, LEN32b_PARA_mult_t, 0>);
        mult_kernel_1 = kernel::create(
            us::L1::kfun_mult_cascade<T, NUM_LINE_t, NUM_ELEMENT_t, NUM_SAMPLE_t, NUM_SEG_t, NUM_DEP_SEG_t,
                                      VECDIM_mult_t, LEN_IN_mult_t, LEN_OUT_mult_t, LEN32b_PARA_mult_t, 1>);
        mult_kernel_2 = kernel::create(
            us::L1::kfun_mult_cascade<T, NUM_LINE_t, NUM_ELEMENT_t, NUM_SAMPLE_t, NUM_SEG_t, NUM_DEP_SEG_t,
                                      VECDIM_mult_t, LEN_IN_mult_t, LEN_OUT_mult_t, LEN32b_PARA_mult_t, 2>);
        mult_kernel_3 = kernel::create(
            us::L1::kfun_mult_cascade<T, NUM_LINE_t, NUM_ELEMENT_t, NUM_SAMPLE_t, NUM_SEG_t, NUM_DEP_SEG_t,
                                      VECDIM_mult_t, LEN_IN_mult_t, LEN_OUT_mult_t, LEN32b_PARA_mult_t, 3, 3>);

        // mult kernel pre
        source(mult_kernel_pre) = "kernel_mult/kernel_mult.cpp";
        connect<parameter>(para_const_pre, async(mult_kernel_pre.in[0]));
        connect<>(interp, mult_kernel_pre.in[1]);
        connect<>(apod, mult_kernel_pre.in[2]);
        connect<cascade>(mult_kernel_pre.out[0], mult_kernel_0.in[3]);

        runtime<ratio>(mult_kernel_pre) = 0.8;

        adf::source(mult_kernel_0) = "kernel_mult/kernel_mult.cpp";
        adf::source(mult_kernel_1) = "kernel_mult/kernel_mult.cpp";
        adf::source(mult_kernel_2) = "kernel_mult/kernel_mult.cpp";
        adf::source(mult_kernel_3) = "kernel_mult/kernel_mult.cpp";

        // mult kernel 0
        connect<parameter>(para_const_0, async(mult_kernel_0.in[0]));
        connect<parameter>(para_local_0_0, async(mult_kernel_0.in[1]));
        connect<parameter>(para_local_1_0, async(mult_kernel_0.in[2]));

        connect<cascade>(mult_kernel_0.out[0], mult_kernel_1.in[3]);
        connect<>(mult_kernel_0.out[1], mult_0);

        // mult kernel 1
        connect<parameter>(para_const_1, async(mult_kernel_1.in[0]));
        connect<parameter>(para_local_0_1, async(mult_kernel_1.in[1]));
        connect<parameter>(para_local_1_1, async(mult_kernel_1.in[2]));

        connect<cascade>(mult_kernel_1.out[0], mult_kernel_2.in[3]);
        connect<>(mult_kernel_1.out[1], mult_1);

        // mult kernel 2
        connect<parameter>(para_const_2, async(mult_kernel_2.in[0]));
        connect<parameter>(para_local_0_2, async(mult_kernel_2.in[1]));
        connect<parameter>(para_local_1_2, async(mult_kernel_2.in[2]));

        connect<cascade>(mult_kernel_2.out[0], mult_kernel_3.in[3]);
        connect<>(mult_kernel_2.out[1], mult_2);

        // mult kernel 3
        connect<parameter>(para_const_3, async(mult_kernel_3.in[0]));
        connect<parameter>(para_local_0_3, async(mult_kernel_3.in[1]));
        connect<parameter>(para_local_1_3, async(mult_kernel_3.in[2]));

        connect<>(mult_kernel_3.out[0], mult_3);

        // config
        runtime<ratio>(mult_kernel_0) = 0.8;
        runtime<ratio>(mult_kernel_1) = 0.8;
        runtime<ratio>(mult_kernel_2) = 0.8;
        runtime<ratio>(mult_kernel_3) = 0.8;
    }

   private:
    kernel mult_kernel_pre;
    kernel mult_kernel_0;
    kernel mult_kernel_1;
    kernel mult_kernel_2;
    kernel mult_kernel_3;
};

} // namespace L2
} // namespace us

#endif
