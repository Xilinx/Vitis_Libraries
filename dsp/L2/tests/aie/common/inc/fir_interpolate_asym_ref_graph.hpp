/*
 * Copyright 2022 Xilinx, Inc.
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
#ifndef FIR_INTERPOLATE_ASYM_REF_HPP
#define FIR_INTERPOLATE_ASYM_REF_HPP

// This file holds the definition header of the Asymmetric Interpolation FIR reference model graph class

#include <adf.h>
#include <vector>

#include "fir_interpolate_asym_ref.hpp"
#include "fir_ref_utils.hpp"
#include "widget_api_cast_ref.hpp"

namespace xf {
namespace dsp {
namespace aie {
namespace fir {
namespace interpolate_asym {
using namespace adf;
using namespace xf::dsp::aie::widget::api_cast;

class empty {};

template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_INTERPOLATE_FACTOR,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_CASC_LEN = 1,
          unsigned int TP_USE_COEFF_RELOAD = 0,
          unsigned int TP_DUAL_IP = 0,
          unsigned int TP_NUM_OUTPUTS = 1,
          unsigned int TP_API = 0>
class fir_interpolate_asym_ref_graph : public graph {
   public:
    using in2_port = typename std::conditional<(TP_DUAL_IP == 1), port<input>, empty>::type;
    using coeff_port = typename std::conditional<(TP_USE_COEFF_RELOAD == 1), port<input>, empty>::type;
    using out2_port = typename std::conditional<(TP_NUM_OUTPUTS == 2), port<output>, empty>::type;
    using widget_kernel_in = typename std::conditional<(TP_DUAL_IP == 1 && TP_API == 1), kernel, empty>::type;
    using widget_kernel_out = typename std::conditional<(TP_NUM_OUTPUTS == 2), kernel, empty>::type;

    port<input> in;
    in2_port in2;
    coeff_port coeff;
    port<output> out;
    out2_port out2;

    // FIR Kernel
    kernel m_firKernel;
    widget_kernel_in m_widgetKernelIn;
    widget_kernel_out m_widgetKernelOut;

    // Constructor
    fir_interpolate_asym_ref_graph(const std::vector<TT_COEFF>& taps) {
        m_firKernel = kernel::create_object<
            fir_interpolate_asym_ref<TT_DATA, TT_COEFF, TP_FIR_LEN, TP_INTERPOLATE_FACTOR, TP_SHIFT, TP_RND,
                                     TP_INPUT_WINDOW_VSIZE, USE_COEFF_RELOAD_FALSE, TP_NUM_OUTPUTS, TP_API> >(taps);
        create_connections();
    }

    fir_interpolate_asym_ref_graph() {
        m_firKernel = kernel::create_object<
            fir_interpolate_asym_ref<TT_DATA, TT_COEFF, TP_FIR_LEN, TP_INTERPOLATE_FACTOR, TP_SHIFT, TP_RND,
                                     TP_INPUT_WINDOW_VSIZE, USE_COEFF_RELOAD_TRUE, TP_NUM_OUTPUTS, TP_API> >();
        create_connections();
    }

    void create_connections() {
        printf("===========================\n");
        printf("== FIR INTERPOLATE ASYM REF\n");
        printf("===========================\n");

        // Make connections
        // Size of window in Bytes.
        if
            constexpr(TP_DUAL_IP == 1 && TP_API == 1) {
                const int kInterleavePattern = 0;
                m_widgetKernelIn = kernel::create_object<
                    widget_api_cast_ref<TT_DATA, 1, 0, 2, TP_INPUT_WINDOW_VSIZE, 1, kInterleavePattern> >();
                connect<stream>(in, m_widgetKernelIn.in[0]);
                connect<stream>(in2, m_widgetKernelIn.in[1]);
                connect<window<TP_INPUT_WINDOW_VSIZE * sizeof(TT_DATA),
                               fnFirMargin<TP_FIR_LEN / TP_INTERPOLATE_FACTOR, TT_DATA>() * sizeof(TT_DATA)> >(
                    m_widgetKernelIn.out[0], m_firKernel.in[0]);
                runtime<ratio>(m_widgetKernelIn) = 0.4;
                // Source files
                source(m_widgetKernelIn) = "widget_api_cast_ref.cpp";
            }
        else {
            connect<window<TP_INPUT_WINDOW_VSIZE * sizeof(TT_DATA),
                           fnFirMargin<TP_FIR_LEN / TP_INTERPOLATE_FACTOR, TT_DATA>() * sizeof(TT_DATA)> >(
                in, m_firKernel.in[0]);
        }
        if
            constexpr(TP_USE_COEFF_RELOAD == 1) { connect<parameter>(coeff, async(m_firKernel.in[1])); }

        if
            constexpr(TP_NUM_OUTPUTS == 2) {
                constexpr int kNumInputs = 1;               // single Fir kernel output
                constexpr int kNumOutputs = TP_NUM_OUTPUTS; //
                m_widgetKernelOut = kernel::create_object<
                    widget_api_cast_ref<TT_DATA, USE_WINDOW_API, TP_API, kNumInputs,
                                        TP_INPUT_WINDOW_VSIZE * TP_INTERPOLATE_FACTOR, kNumOutputs, 0> >();
                connect<window<TP_INPUT_WINDOW_VSIZE * sizeof(TT_DATA) * TP_INTERPOLATE_FACTOR> >(
                    m_firKernel.out[0], m_widgetKernelOut.in[0]);

                if
                    constexpr(TP_API == USE_WINDOW_API) {
                        connect<window<TP_INPUT_WINDOW_VSIZE * sizeof(TT_DATA) * TP_INTERPOLATE_FACTOR> >(
                            m_widgetKernelOut.out[0], out);
                        connect<window<TP_INPUT_WINDOW_VSIZE * sizeof(TT_DATA) * TP_INTERPOLATE_FACTOR> >(
                            m_widgetKernelOut.out[1], out2);
                    }
                else {
                    connect<stream>(m_widgetKernelOut.out[0], out);
                    connect<stream>(m_widgetKernelOut.out[1], out2);
                }

                source(m_widgetKernelOut) = "widget_api_cast_ref.cpp";
                runtime<ratio>(m_widgetKernelOut) = 0.9;
            }
        else {
            connect<window<(TP_INTERPOLATE_FACTOR * TP_INPUT_WINDOW_VSIZE * sizeof(TT_DATA))> >(m_firKernel.out[0],
                                                                                                out);
        }

        // Specify mapping constraints
        runtime<ratio>(m_firKernel) = 0.4;

        // Source files
        source(m_firKernel) = "fir_interpolate_asym_ref.cpp";
    };
};
}
}
}
}
}
#endif // FIR_INTERPOLATE_ASYM_REF_HPP
