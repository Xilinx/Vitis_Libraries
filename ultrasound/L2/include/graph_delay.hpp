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
#ifndef __GRAPH_DELAY_HPP__
#define __GRAPH_DELAY_HPP__

//#include "kernels.hpp"
#include "kernel_delay.hpp"
#include <adf.h>

namespace us {
namespace L2 {

template <class T, int NUM_LINE_t, int VECDIM_delay_t, int LEN_IN_delay_t, int LEN_OUT_delay_t, int LEN32b_PARA_delay_t>
class delay_graph_wrapper : public adf::graph {
   public:
    adf::port<adf::direction::in> para_const;
    adf::port<adf::direction::in> para_t_start;
    adf::port<input> img_x;
    adf::port<input> img_z;
    adf::port<output> delay;

    delay_graph_wrapper() {
// kernel definition
#ifdef _USING_SHELL_
        delay_kernel = adf::kernel::create(
            us::L1::kfun_UpdatingDelay_line_wrapper_shell<T, NUM_LINE_t, VECDIM_delay_t, LEN_IN_delay_t,
                                                          LEN_OUT_delay_t, LEN32b_PARA_delay_t>);
#else
        delay_kernel =
            adf::kernel::create(us::L1::kfun_UpdatingDelay_line_wrapper<T, NUM_LINE_t, VECDIM_delay_t, LEN_IN_delay_t,
                                                                        LEN_OUT_delay_t, LEN32b_PARA_delay_t>);
#endif

        adf::source(delay_kernel) = "kernel_delay/kernel_delay.cpp";

        // delay kernel
        adf::connect<adf::parameter>(para_const, async(delay_kernel.in[2]));
        adf::connect<adf::parameter>(para_t_start, async(delay_kernel.in[3]));

        adf::connect<>(img_x, delay_kernel.in[0]);
        adf::connect<>(img_z, delay_kernel.in[1]);
        adf::connect<>(delay_kernel.out[0], delay);

        // config
        adf::runtime<adf::ratio>(delay_kernel) = 0.8;
    }

   private:
    adf::kernel delay_kernel;
};

} // namespace L2
} // namespace us

#endif
