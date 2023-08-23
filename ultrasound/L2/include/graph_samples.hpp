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
#ifndef __GRAPH_SAMPLES_HPP__
#define __GRAPH_SAMPLES_HPP__

// #include "kernels.hpp"
#include "kernel_sample.hpp"

using namespace adf;

namespace us {
namespace L2 {

template <class T,
          int NUM_LINE_t,
          int NUM_ELEMENT_t,
          int VECDIM_sample_t,
          int LEN_IN_sample_t,
          int LEN_OUT_sample_t,
          int LEN32b_PARA_sample_t>
class sample_graph_wrapper : public adf::graph {
   public:
    // input and output port
    adf::port<adf::direction::in> para_const;
    adf::port<adf::direction::in> para_rfdim;
    adf::port<adf::direction::in> para_elem;
    adf::port<output> sample;
    adf::port<output> inside;
    adf::port<input> img_x;
    adf::port<input> img_z;
    adf::port<input> delay;

    sample_graph_wrapper() {
// kernel definition
#ifdef _USING_SHELL_
        sample_kernel = adf::kernel::create(
            us::L1::kfun_genLineSample_wrapper_shell<T, NUM_LINE_t, NUM_ELEMENT_t, VECDIM_sample_t, LEN_IN_sample_t,
                                                     LEN_OUT_sample_t, LEN32b_PARA_sample_t>);
#else
        sample_kernel = adf::kernel::create(
            us::L1::kfun_genLineSample_wrapper<T, NUM_LINE_t, NUM_ELEMENT_t, VECDIM_sample_t, LEN_IN_sample_t,
                                               LEN_OUT_sample_t, LEN32b_PARA_sample_t>);
#endif
        adf::source(sample_kernel) = "kernel_sample/kernel_sample.cpp";

        // sample kernel
        adf::connect<>(img_x, sample_kernel.in[0]);
        adf::connect<>(img_z, sample_kernel.in[1]);
        adf::connect<>(delay, sample_kernel.in[2]);
        adf::connect<parameter>(para_const, async(sample_kernel.in[3]));
        adf::connect<parameter>(para_rfdim, async(sample_kernel.in[4]));
        adf::connect<parameter>(para_elem, async(sample_kernel.in[5]));
        adf::connect<>(sample_kernel.out[0], sample);
        adf::connect<>(sample_kernel.out[1], inside);

        // config
        adf::runtime<adf::ratio>(sample_kernel) = 0.8;
    }

   private:
    adf::kernel sample_kernel;
};

} // namespace L2
} // namespace us

#endif