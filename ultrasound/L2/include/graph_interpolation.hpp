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
#ifndef _BSPLINE_HPP_
#define _BSPLINE_HPP_

#include "kernels.hpp"
#include "kernel_interpolation.hpp"
//#define _USING_SHELL_ (1)
using namespace adf;

namespace us {
namespace L2 {

//----------------------------------------------------
// brief
// interpolation
//---1.top graph to switch between scaler version and shell version
//---2.top graph vecter version
//----------------------------------------------------

//---1.top graph to switch between scaler version and shell version
template <typename T,
          int NUM_LINE,
          int NUM_ELEMENT,
          int NUM_SAMPLE,
          int NUM_SEG,
          int LEN_OUT,
          int LEN_IN,
          int LEN_RF_IN,
          int VECDIM,
          int APODI_PRE_LEN32b_PARA>
class interpolation_graph_scaler_shell : public adf::graph {
   public:
    // graph port
    port<output> out;
    port<input> p_sample_in;
    port<input> p_inside_in;
    port<input> p_rfdata_in;
    port<input> para_interp_const;

    interpolation_graph_scaler_shell() {
// 1.kernel create
#ifdef _USING_SHELL_
        interp_kernel = adf::kernel::create(
            us::L1::kfun_interpolation_shell<T, LEN_OUT, LEN_IN, LEN_RF_IN, VECDIM, APODI_PRE_LEN32b_PARA>);
#else
        interp_kernel = adf::kernel::create(
            us::L1::mfun_interpolation_scaler<T, LEN_OUT, LEN_IN, LEN_RF_IN, VECDIM, APODI_PRE_LEN32b_PARA>);
#endif

        adf::source(interp_kernel) = "kernel_interpolation/kernel_interpolation.cpp";

        // 2.nets
        adf::connect<>(interp_kernel.out[0], out);
        adf::connect<>(p_sample_in, interp_kernel.in[0]);
        adf::connect<>(p_inside_in, interp_kernel.in[1]);
        adf::connect<>(p_rfdata_in, interp_kernel.in[2]);
        adf::connect<parameter>(para_interp_const, async(interp_kernel.in[3]));

        // 3.config
        adf::runtime<adf::ratio>(interp_kernel) = 0.8;
    }

   private:
    adf::kernel interp_kernel;
};

template <typename T,
          int NUM_LINE,
          int NUM_ELEMENT,
          int NUM_SAMPLE,
          int NUM_SEG,
          int LEN_OUT,
          int LEN_IN,
          int LEN_RF_IN,
          int VECDIM,
          int APODI_PRE_LEN32b_PARA>
class interpolation_graph : public adf::graph {
   public:
    // graph port
    port<output> out;
    port<input> p_sample_in;
    port<input> p_inside_in;
    port<input> p_rfdata_in;
    port<input> para_interp_const_0;
    port<input> para_interp_const_1;
    port<input> para_interp_const_2;
    port<input> para_interp_const_3;
    port<input> para_local;

    // adf::output_plio local_dump_rfbuf;
    // adf::output_plio local_dump_resamp;
    // adf::output_plio local_dump_genwin;

    interpolation_graph() {
        // 1.kernel create
        rfbuf_kernel = adf::kernel::create(
            us::L1::kfun_rfbuf_wrapper<T, LEN_OUT, LEN_IN, LEN_RF_IN, VECDIM, APODI_PRE_LEN32b_PARA>);
        resamp_kernel = adf::kernel::create(
            us::L1::kfun_resamp_wrapper2<T, LEN_OUT, LEN_IN, LEN_RF_IN, VECDIM, APODI_PRE_LEN32b_PARA>);
        genwin_kernel = adf::kernel::create(
            us::L1::kfun_genwin_wrapper2<T, LEN_OUT, LEN_IN, LEN_RF_IN, VECDIM, APODI_PRE_LEN32b_PARA>);
        interp_kernel = adf::kernel::create(
            us::L1::kfun_interpolation_wrapper<T, LEN_OUT, LEN_IN, LEN_RF_IN, VECDIM, APODI_PRE_LEN32b_PARA>);

        adf::source(rfbuf_kernel) = "kernel_interpolation/kernel_interpolation.cpp";
        adf::source(resamp_kernel) = "kernel_interpolation/kernel_interpolation.cpp";
        adf::source(genwin_kernel) = "kernel_interpolation/kernel_interpolation.cpp";
        adf::source(interp_kernel) = "kernel_interpolation/kernel_interpolation.cpp";

        // 2.nets
        // local_dump_rfbuf  = adf::output_plio::create(adf::plio_32_bits, "data/rfbuf.txt");
        // local_dump_resamp = adf::output_plio::create(adf::plio_32_bits, "data/resamp.txt");
        // local_dump_genwin = adf::output_plio::create(adf::plio_32_bits, "data/genwin.txt");
        // adf::connect<>(rfbuf_kernel.out[0], local_dump_rfbuf.in[0]);
        // adf::connect<>(resamp_kernel.out[0], local_dump_resamp.in[0]);
        // adf::connect<>(genwin_kernel.out[0], local_dump_genwin.in[0]);

        adf::connect<>(rfbuf_kernel.out[0], resamp_kernel.in[0]);
        adf::connect<>(p_rfdata_in, rfbuf_kernel.in[0]);

        adf::connect<>(p_sample_in, resamp_kernel.in[1]);
        adf::connect<>(p_inside_in, resamp_kernel.in[2]);

        adf::connect<parameter>(para_local, async(rfbuf_kernel.in[1]));
        adf::connect<parameter>(para_interp_const_0, async(rfbuf_kernel.in[2]));

        adf::connect<>(resamp_kernel.out[0], genwin_kernel.in[0]);
        adf::connect<>(resamp_kernel.out[1], genwin_kernel.in[1]);
        adf::connect<parameter>(para_interp_const_1, async(resamp_kernel.in[3]));

        adf::connect<>(genwin_kernel.out[0], interp_kernel.in[0]);
        adf::connect<>(genwin_kernel.out[1], interp_kernel.in[1]);
        adf::connect<parameter>(para_interp_const_2, async(genwin_kernel.in[2]));

        adf::connect<>(interp_kernel.out[0], out);
        adf::connect<parameter>(para_interp_const_3, async(interp_kernel.in[2]));

        // 3.config
        adf::runtime<adf::ratio>(rfbuf_kernel) = 0.8;
        adf::runtime<adf::ratio>(resamp_kernel) = 0.8;
        adf::runtime<adf::ratio>(genwin_kernel) = 0.8;
        adf::runtime<adf::ratio>(interp_kernel) = 0.8;
        adf::location<kernel>(rfbuf_kernel) = tile(25, 0);
        adf::location<kernel>(resamp_kernel) = tile(24, 0);
        adf::location<kernel>(genwin_kernel) = tile(24, 1);
        adf::location<kernel>(interp_kernel) = tile(25, 1);
    }

   private:
    adf::kernel rfbuf_kernel;
    adf::kernel resamp_kernel;
    adf::kernel genwin_kernel;
    adf::kernel interp_kernel;
};

} // namespace L2
} // namespace us

#endif
