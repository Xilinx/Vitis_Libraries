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
#ifndef _GRAPH_APODIZATION_HPP_
#define _GRAPH_APODIZATION_HPP_

#include "kernels.hpp"
#include <adf.h>
#include "kernel_apodization.hpp"

using namespace adf;

namespace us {
namespace L2 {

template <typename T,
          int NUM_LINE,
          int NUM_ELEMENT,
          int NUM_SAMPLE,
          int NUM_SEG,
          int LEN_OUT,
          int LEN_IN,
          int VECDIM,
          int APODI_PRE_LEN32b_PARA>
class apodi_pre_graph : public adf::graph {
   public:
    // graph port
    port<output> out;
    port<input> img_x_in;
    port<input> img_z_in;
    port<input> para_apodi_const;

    apodi_pre_graph() {
// 1.kernel create
#ifdef _USING_SHELL_
        apodi_pre_kernel =
            adf::kernel::create(us::L1::kfun_apodization_pre_shell<T, LEN_OUT, LEN_IN, VECDIM, APODI_PRE_LEN32b_PARA>);
#else
        apodi_pre_kernel =
            adf::kernel::create(us::L1::kfun_apodization_pre<T, LEN_OUT, LEN_IN, VECDIM, APODI_PRE_LEN32b_PARA>);
#endif
        adf::source(apodi_pre_kernel) = "kernel_apodization/kernel_apodization_pre.cpp";
        // 2.nets
        adf::connect<>(apodi_pre_kernel.out[0], out);
        adf::connect<>(img_x_in, apodi_pre_kernel.in[0]);
        adf::connect<>(img_z_in, apodi_pre_kernel.in[1]);
        adf::connect<parameter>(para_apodi_const, async(apodi_pre_kernel.in[2]));

        // 3.config
        adf::runtime<adf::ratio>(apodi_pre_kernel) = 0.8;
    }

   private:
    adf::kernel apodi_pre_kernel;
};

template <typename T,
          int NUM_LINE,
          int NUM_ELEMENT,
          int NUM_SAMPLE,
          int NUM_SEG,
          int LEN_OUT,
          int LEN_IN_F,
          int LEN_IN_D,
          int VECDIM,
          int APODI_PRE_LEN32b_PARA>
class apodi_main_graph : public adf::graph {
   public:
    // graph port
    port<output> out;
    port<input> p_focal;
    port<input> p_invD;
    port<input> para_amain_const;

    apodi_main_graph() {
// 1.kernel create
#ifdef _USING_SHELL_
        apodi_main_kernel = adf::kernel::create(
            us::L1::kfun_apodization_main_shell<T, LEN_OUT, LEN_IN_F, LEN_IN_D, VECDIM, APODI_PRE_LEN32b_PARA>);
#else
        apodi_main_kernel = adf::kernel::create(
            us::L1::kfun_apodization_main<T, LEN_OUT, LEN_IN_F, LEN_IN_D, VECDIM, APODI_PRE_LEN32b_PARA>);
#endif

        adf::source(apodi_main_kernel) = "kernel_apodization/kernel_apodization_main.cpp";

        // 2.nets
        adf::connect<>(apodi_main_kernel.out[0], out);
        adf::connect<>(p_focal, apodi_main_kernel.in[0]);
        adf::connect<>(p_invD, apodi_main_kernel.in[1]);
        adf::connect<parameter>(para_amain_const, async(apodi_main_kernel.in[2]));

        // 3.config
        adf::runtime<adf::ratio>(apodi_main_kernel) = 0.8;
    }

   private:
    adf::kernel apodi_main_kernel;
};

} // namespace L2
} // namespace us

#endif
