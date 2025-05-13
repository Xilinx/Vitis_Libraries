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
#ifndef _DSPLIB_FIR_INTERPOLATE_FRACT_ASYM_REF_GRAPH_HPP_
#define _DSPLIB_FIR_INTERPOLATE_FRACT_ASYM_REF_GRAPH_HPP_

#include <adf.h>
#include <vector>
#include "fir_resampler_ref.hpp"
#include "widget_api_cast_ref.hpp"
#include "fir_ref_utils.hpp"

namespace xf {
namespace dsp {
namespace aie {
namespace fir {
namespace resampler {
using namespace adf;
using namespace xf::dsp::aie::widget::api_cast;

class empty {};

// base class description and specialization for static coeffs, single output
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_INTERPOLATE_FACTOR,
          unsigned int TP_DECIMATE_FACTOR,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_CASC_LEN = 1,
          unsigned int TP_USE_COEFF_RELOAD = 0,
          unsigned int TP_NUM_OUTPUTS = 1,
          unsigned int TP_DUAL_IP = 0,
          unsigned int TP_API = 0,
          unsigned int TP_SSR = 1,
          unsigned int TP_PARA_INTERP_POLY = 1,
          unsigned int TP_PARA_DECI_POLY = 1,
          unsigned int TP_SAT = 1>
class fir_resampler_ref_graph : public graph {
   private:
    using in2_port = typename std::conditional<(TP_DUAL_IP == 1), port<input>, empty>::type;
    using coeff_port = typename std::conditional<(TP_USE_COEFF_RELOAD == 1), port<input>, empty>::type;
    using out2_port = typename std::conditional<(TP_NUM_OUTPUTS == 2), port<output>, empty>::type;

    using widget_kernel_in = typename std::conditional<(TP_DUAL_IP == 1 && TP_API == 1), kernel, empty>::type;
    using widget_kernel_out = typename std::conditional<(TP_NUM_OUTPUTS == 2), kernel, empty>::type;

    static constexpr unsigned int kInterleavePattern = 0;   // 128-bit interleave pattern
    static constexpr unsigned int kNumFirKernelInputs = 1;  // Widgets are used to
    static constexpr unsigned int kNumFirKernelOutputs = 1; // 128-bit interleave pattern
   public:
    port<input> in[1];
    in2_port in2[1];
    coeff_port coeff[1];
    port<output> out[1];
    out2_port out2[1];

    // FIR Kernel
    kernel m_firKernel;
    widget_kernel_in m_widgetKernelIn;
    widget_kernel_out m_widgetKernelOut;

    // Constructor
    fir_resampler_ref_graph(const std::vector<TT_COEFF>& taps) {
        // Create FIR class with static coeffs - passed at construction
        m_firKernel = kernel::create_object<
            fir_resampler_ref<TT_DATA, TT_COEFF, TP_FIR_LEN, TP_INTERPOLATE_FACTOR, TP_DECIMATE_FACTOR, TP_SHIFT,
                              TP_RND, TP_INPUT_WINDOW_VSIZE, TP_USE_COEFF_RELOAD, kNumFirKernelOutputs, TP_SAT> >(taps);

        make_connections();
    };

    // Constructor
    fir_resampler_ref_graph() {
        // Create FIR class with reloadable coeffs - passed through graph's update()
        m_firKernel = kernel::create_object<
            fir_resampler_ref<TT_DATA, TT_COEFF, TP_FIR_LEN, TP_INTERPOLATE_FACTOR, TP_DECIMATE_FACTOR, TP_SHIFT,
                              TP_RND, TP_INPUT_WINDOW_VSIZE, TP_USE_COEFF_RELOAD, kNumFirKernelOutputs, TP_SAT> >();
        make_connections();
    };
    void make_connections() {
        // Make input connections

        constexpr unsigned int headerConfigSize = 256 / 8; // 256-bits of configuration info organized in int32 fields
        constexpr unsigned int headerByteSize =
            TP_USE_COEFF_RELOAD == 2 ? CEIL(TP_FIR_LEN * sizeof(TT_COEFF), headerConfigSize) + headerConfigSize
                                     : 0; // align to 32 Byte boundary
        // Byte size of window buffer.
        constexpr unsigned int inputWindowByteSize = TP_INPUT_WINDOW_VSIZE * sizeof(TT_DATA) + headerByteSize;
        constexpr unsigned int inputWindowVectorSize = inputWindowByteSize / sizeof(TT_DATA);
        constexpr unsigned int outputWindowByteSize =
            TP_INTERPOLATE_FACTOR * TP_INPUT_WINDOW_VSIZE * sizeof(TT_DATA) / TP_DECIMATE_FACTOR;
        constexpr unsigned int outputWindowVectorSize = outputWindowByteSize / sizeof(TT_DATA);

        if
            constexpr(TP_DUAL_IP == 1 && TP_API == 1) {
                constexpr int kNumInputs = 2;                    // 2 stream inputs
                constexpr int kNumOutputs = kNumFirKernelInputs; //
                m_widgetKernelIn = kernel::create_object<
                    widget_api_cast_ref<TT_DATA, TP_API, USE_WINDOW_API, kNumInputs, inputWindowVectorSize, kNumOutputs,
                                        kInterleavePattern> >();

                connect<stream>(in[0], m_widgetKernelIn.in[0]);
                connect<stream>(in2[0], m_widgetKernelIn.in[1]);
                // connect<
                //    window<inputWindowByteSize,
                //           fnFirMargin<(TP_FIR_LEN + TP_INTERPOLATE_FACTOR - 1) / TP_INTERPOLATE_FACTOR, TT_DATA>() *
                //               sizeof(TT_DATA)> >(m_widgetKernelIn.out[0], m_firKernel.in[0]);
                connect<>(m_widgetKernelIn.out[0], m_firKernel.in[0]);
                dimensions(m_widgetKernelIn.out[0]) = {TP_INPUT_WINDOW_VSIZE};
                dimensions(m_firKernel.in[0]) = {TP_INPUT_WINDOW_VSIZE};
                runtime<ratio>(m_widgetKernelIn) = 0.9;
                source(m_widgetKernelIn) = "widget_api_cast_ref.cpp";
            }
        else {
            // connect<window<inputWindowByteSize,
            //               fnFirMargin<(TP_FIR_LEN + TP_INTERPOLATE_FACTOR - 1) / TP_INTERPOLATE_FACTOR, TT_DATA>() *
            //                   sizeof(TT_DATA)> >(in[0], m_firKernel.in[0]);
            connect<>(in[0], m_firKernel.in[0]);
            dimensions(m_firKernel.in[0]) = {TP_INPUT_WINDOW_VSIZE};
        }
        // Make output connections
        if
            constexpr(TP_NUM_OUTPUTS == 2) {
                constexpr int kNumInputs = kNumFirKernelOutputs; // single Fir kernel output
                constexpr int kNumOutputs = TP_NUM_OUTPUTS;      //

                m_widgetKernelOut = kernel::create_object<
                    widget_api_cast_ref<TT_DATA, USE_WINDOW_API, TP_API, kNumInputs, outputWindowVectorSize,
                                        kNumOutputs, kInterleavePattern> >();

                connect<>(m_firKernel.out[0], m_widgetKernelOut.in[0]);
                dimensions(m_firKernel.out[0]) = {TP_INTERPOLATE_FACTOR * TP_INPUT_WINDOW_VSIZE / TP_DECIMATE_FACTOR};
                dimensions(m_widgetKernelOut.in[0]) = {TP_INTERPOLATE_FACTOR * TP_INPUT_WINDOW_VSIZE /
                                                       TP_DECIMATE_FACTOR};

                if
                    constexpr(TP_API == USE_WINDOW_API) {
                        connect<>(m_widgetKernelOut.out[0], out[0]);
                        connect<>(m_widgetKernelOut.out[1], out2[0]);
                        dimensions(m_widgetKernelOut.out[0]) = {TP_INTERPOLATE_FACTOR * TP_INPUT_WINDOW_VSIZE /
                                                                TP_DECIMATE_FACTOR};
                        dimensions(m_widgetKernelOut.out[1]) = {TP_INTERPOLATE_FACTOR * TP_INPUT_WINDOW_VSIZE /
                                                                TP_DECIMATE_FACTOR};
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
            dimensions(m_firKernel.out[0]) = {TP_INTERPOLATE_FACTOR * TP_INPUT_WINDOW_VSIZE / TP_DECIMATE_FACTOR};
        }

        if
            constexpr(TP_USE_COEFF_RELOAD == 1) { connect<parameter>(coeff[0], async(m_firKernel.in[1])); }

        // Specify mapping constraints
        runtime<ratio>(m_firKernel) = 0.9;

        // Source files
        source(m_firKernel) = "fir_resampler_ref.cpp";
    };
};
}
}
}
}
}
#endif // _DSPLIB_FIR_INTERPOLATE_FRACT_ASYM_REF_GRAPH_HPP_
