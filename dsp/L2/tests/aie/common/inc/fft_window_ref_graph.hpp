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
#ifndef _DSPLIB_FFT_WINDOW_REF_GRAPH_HPP_
#define _DSPLIB_FFT_WINDOW_REF_GRAPH_HPP_

/*
This file holds the definition of the Widget Real2complex Reference model graph.
*/

#include <adf.h>
#include <vector>
#include <array>
#include "fft_window_ref.hpp"
#include "fft_ref_utils.hpp"

namespace xf {
namespace dsp {
namespace aie {
namespace fft {
namespace windowfn {
using namespace adf;

template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_POINT_SIZE,
          unsigned int TP_WINDOW_VSIZE,
          unsigned int TP_SHIFT,
          unsigned int TP_API,
          unsigned int TP_SSR,
          unsigned int TP_DYN_PT_SIZE>
class fft_window_ref_graph : public graph {
   public:
    static constexpr int kPtSize = TP_POINT_SIZE / TP_SSR;
    static constexpr int kWindowVsize = TP_WINDOW_VSIZE / TP_SSR;
    static constexpr int kAPIFactor = TP_API == 0 ? 1 : 2;
    static constexpr int kHeaderBytes = TP_DYN_PT_SIZE > 0 ? 32 : 0;

    port<input> in[kAPIFactor * TP_SSR];
    port<output> out[kAPIFactor * TP_SSR];

    kernel m_kernels[TP_SSR];

    // Constructor
    fft_window_ref_graph(const std::array<TT_COEFF, TP_POINT_SIZE*(1 + TP_DYN_PT_SIZE)>& weights) {
        printf("============================\n");
        printf("== FFT_WINDOW REF Graph\n");
        printf("============================\n");
        printf("TP_POINT_SIZE        = %d\n", TP_POINT_SIZE);
        printf("TP_WINDOW_VSIZE      = %d\n", TP_WINDOW_VSIZE);
        printf("TP_SHIFT             = %d\n", TP_SHIFT);
        printf("TP_API               = %d\n", TP_API);
        printf("TP_SSR               = %d\n", TP_SSR);
        printf("TP_DYN_PT_SIZE       = %d\n", TP_DYN_PT_SIZE);

        std::array<std::array<TT_COEFF, kPtSize*(1 + TP_DYN_PT_SIZE)>, TP_SSR> kernel_weights;
        for (int k = 0; k < TP_SSR; k++) {
            for (int i = 0; i < kPtSize * (1 + TP_DYN_PT_SIZE); i++) {
                kernel_weights[k][i] = weights[i * TP_SSR + k];
            }
        }

        for (int i = 0; i < TP_SSR; i++) {
            m_kernels[i] = kernel::create_object<
                fft_window_ref<TT_DATA, TT_COEFF, kPtSize, kWindowVsize, TP_SHIFT, TP_API, TP_SSR, TP_DYN_PT_SIZE> >(
                kernel_weights[i]);
            // Specify mapping constraints
            runtime<ratio>(m_kernels[i]) = 0.1; // Nominal figure. The real figure requires knowledge of the sample
                                                // rate.
            // Source files
            source(m_kernels[i]) = "fft_window_ref.cpp";

            // make connections
            if (TP_API == 0) {
                connect<window<kWindowVsize * sizeof(TT_DATA) + kHeaderBytes> >(in[i], m_kernels[i].in[0]);
                connect<window<kWindowVsize * sizeof(TT_DATA) + kHeaderBytes> >(m_kernels[i].out[0], out[i]);
            } else {
                connect<stream>(in[i * 2], m_kernels[i].in[0]);
                connect<stream>(in[i * 2 + 1], m_kernels[i].in[1]);
                connect<stream>(m_kernels[i].out[0], out[i * 2]);
                connect<stream>(m_kernels[i].out[1], out[i * 2 + 1]);
            }
        }
    };
};
}
}
}
}
}
#endif // _DSPLIB_FFT_WINDOW_REF_GRAPH_HPP_
