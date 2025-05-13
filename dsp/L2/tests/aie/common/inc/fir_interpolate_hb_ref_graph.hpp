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
#ifndef _DSPLIB_fir_interpolate_hb_REF_GRAPH_HPP_
#define _DSPLIB_fir_interpolate_hb_REF_GRAPH_HPP_

/*
This file holds the declaration of the reference model
graph class for the Halfband Interpolator FIR filter.
*/

#include <adf.h>
#include <vector>
#include "fir_interpolate_hb_ref.hpp"
#include "widget_api_cast_ref.hpp"
#include "fir_ref_utils.hpp"
#include <stdio.h>
namespace xf {
namespace dsp {
namespace aie {
namespace fir {
namespace interpolate_hb {
using namespace adf;
using namespace xf::dsp::aie::widget::api_cast;

struct no_port {};
class empty {};
// It is important for the reference model to have the same appearance (template parameters) as the UUT so that either
// can be used in the L2 test harness using
// the same call. However, the ref model will simply ignore those parameters which are to direct implementation
// considerations such as cascade length or dual inputs.
// default, but specialization for static coefficients and single output
template <typename TT_DATA,
          typename TT_COEFF,
          size_t TP_FIR_LEN,
          size_t TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_CASC_LEN = 1,
          unsigned int TP_DUAL_IP = 0,
          unsigned int TP_USE_COEFF_RELOAD = 0,
          unsigned int TP_NUM_OUTPUTS = 1,
          unsigned int TP_UPSHIFT_CT = 0,
          unsigned int TP_API = 0,
          unsigned int TP_SSR = 1,
          unsigned int TP_PARA_INTERP_POLY = 1,
          unsigned int TP_SAT = 1>
class fir_interpolate_hb_ref_graph : public graph {
   public:
    template <class dir>
    using ssr_port_array = std::array<port<dir>, TP_SSR>;

    using dual_ip_port = typename std::conditional_t<(TP_DUAL_IP == DUAL_IP_DUAL), ssr_port_array<input>, no_port>;
    using dual_op_port = typename std::conditional_t<(TP_NUM_OUTPUTS == 2), ssr_port_array<output>, no_port>;
    using rtp_port = typename std::conditional_t<(TP_USE_COEFF_RELOAD == 1), port<input>, no_port>;
    using widget_kernel_in = typename std::conditional<(TP_DUAL_IP == 1 && TP_API == 1), kernel, empty>::type;
    using widget_kernel_out = typename std::conditional<(TP_NUM_OUTPUTS == 2), kernel, empty>::type;

    ssr_port_array<input> in;
    ssr_port_array<output> out;
    dual_ip_port in2;
    dual_op_port out2;
    std::array<rtp_port, 1> coeff;

    // FIR Kernel
    kernel m_firKernel;
    widget_kernel_in m_widgetKernelIn;
    widget_kernel_out m_widgetKernelOut;

    // Constructor
    fir_interpolate_hb_ref_graph(const std::vector<TT_COEFF>& taps) {
        m_firKernel = kernel::create_object<
            fir_interpolate_hb_ref<TT_DATA, TT_COEFF, TP_FIR_LEN, TP_SHIFT, TP_RND, TP_INPUT_WINDOW_VSIZE,
                                   USE_COEFF_RELOAD_FALSE, TP_NUM_OUTPUTS, TP_UPSHIFT_CT, TP_API, TP_SAT> >(taps);
        create_connections();
    }

    // Constructor
    fir_interpolate_hb_ref_graph() {
        m_firKernel = kernel::create_object<
            fir_interpolate_hb_ref<TT_DATA, TT_COEFF, TP_FIR_LEN, TP_SHIFT, TP_RND, TP_INPUT_WINDOW_VSIZE,
                                   USE_COEFF_RELOAD_TRUE, TP_NUM_OUTPUTS, TP_UPSHIFT_CT, TP_API, TP_SAT> >();
        create_connections();
    }

    void create_connections() {
        // Create FIR class
        // The reference model can ignore cascades.
        printf("===========================\n");
        printf("== FIR INTERPOLATE HB REF  \n");
        printf("===========================\n");
        // Specify mapping constraints
        runtime<ratio>(m_firKernel) = 0.4;

        // Source files
        source(m_firKernel) = "fir_interpolate_hb_ref.cpp";
        const unsigned short kInterpolateFactor = 2;
        const unsigned short kOutputWindowReductionFactor = (TP_API == 1 && TP_NUM_OUTPUTS == 2) ? 2 : 1;

        if
            constexpr(TP_DUAL_IP == 1 && TP_API == 1) {
                const int kInterleavePattern = 0;
                m_widgetKernelIn = kernel::create_object<
                    widget_api_cast_ref<TT_DATA, 1, 0, 2, TP_INPUT_WINDOW_VSIZE, 1, kInterleavePattern> >();
                connect<stream>(in[0], m_widgetKernelIn.in[0]);
                connect<stream>(in2[0], m_widgetKernelIn.in[1]);
                connect<>(m_widgetKernelIn.out[0], m_firKernel.in[0]);
                dimensions(m_widgetKernelIn.out[0]) = {TP_INPUT_WINDOW_VSIZE};
                dimensions(m_firKernel.in[0]) = {TP_INPUT_WINDOW_VSIZE};
                runtime<ratio>(m_widgetKernelIn) = 0.4;
                // Source files
                source(m_widgetKernelIn) = "widget_api_cast_ref.cpp";
            }
        else {
            connect<>(in[0], m_firKernel.in[0]);
            dimensions(m_firKernel.in[0]) = {TP_INPUT_WINDOW_VSIZE};
        }

        if
            constexpr(TP_NUM_OUTPUTS == 2) {
                constexpr int kNumInputs = 1;               // single Fir kernel output
                constexpr int kNumOutputs = TP_NUM_OUTPUTS; //
                m_widgetKernelOut = kernel::create_object<
                    widget_api_cast_ref<TT_DATA, USE_WINDOW_API, TP_API, kNumInputs,
                                        TP_INPUT_WINDOW_VSIZE * kInterpolateFactor, kNumOutputs, 0> >();
                connect<>(m_firKernel.out[0], m_widgetKernelOut.in[0]);
                dimensions(m_firKernel.out[0]) = {TP_INPUT_WINDOW_VSIZE * kInterpolateFactor};
                dimensions(m_widgetKernelOut.in[0]) = {TP_INPUT_WINDOW_VSIZE * kInterpolateFactor};

                if
                    constexpr(TP_API == USE_WINDOW_API) {
                        connect<>(m_widgetKernelOut.out[0], out[0]);
                        connect<>(m_widgetKernelOut.out[1], out2[0]);
                        dimensions(m_widgetKernelOut.out[0]) = {TP_INPUT_WINDOW_VSIZE * kInterpolateFactor};
                        dimensions(m_widgetKernelOut.out[1]) = {TP_INPUT_WINDOW_VSIZE * kInterpolateFactor};
                    }
                else {
                    connect<stream>(m_widgetKernelOut.out[0], out[0]);
                    connect<stream>(m_widgetKernelOut.out[1], out2[0]);
                }

                source(m_widgetKernelOut) = "widget_api_cast_ref.cpp";
                runtime<ratio>(m_widgetKernelOut) = 0.9;
            }
        else {
            connect<>(m_firKernel.out[0], out[0]);
            dimensions(m_firKernel.out[0]) = {TP_INPUT_WINDOW_VSIZE * kInterpolateFactor};
        }

        if
            constexpr(TP_USE_COEFF_RELOAD == 1) { connect<parameter>(coeff[0], async(m_firKernel.in[1])); }
    }
};
}
}
}
}
}
#endif // _DSPLIB_fir_interpolate_hb_REF_GRAPH_HPP_
