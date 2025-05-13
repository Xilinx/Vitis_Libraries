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
#ifndef _DSPLIB_FFT_IFFT_2D_REF_GRAPH_HPP_
#define _DSPLIB_FFT_IFFT_2D_REF_GRAPH_HPP_

#include <adf.h>
#include <vector>
#include <adf/arch/aie_arch_properties.hpp> //for device traits like number of streams per tile
#include "fir_ref_utils.hpp"
#include "fft_ifft_dit_1ch_ref_graph.hpp"
#include "fft_dit_2ch_real_ref_graph.hpp"
#include "transpose_ref.hpp"

using namespace adf;

namespace xf {
namespace dsp {
namespace aie {
namespace fft {
namespace two_d {

template <typename TT_DATA_D1,
          typename TT_DATA_D2,
          typename TT_TWIDDLE,
          unsigned int TP_POINT_SIZE_D1,
          unsigned int TP_POINT_SIZE_D2,
          unsigned int TP_FFT_NIFFT,
          unsigned int TP_SHIFT,
          unsigned int TP_CASC_LEN,
          unsigned int TP_WINDOW_VSIZE_D1,
          unsigned int TP_WINDOW_VSIZE_D2,
          unsigned int TP_API,
          unsigned int TP_RND = 0,
          unsigned int TP_SAT = 1,
          unsigned int TP_TWIDDLE_MODE = 0>
class fft_ifft_2d_ref_graph : public graph {
   public:
    static_assert(!(std::is_same<TT_DATA_D2, cfloat>::value) || (TP_SHIFT == 0),
                  "ERROR: TP_SHIFT cannot be performed for TT_DATA=cfloat, so must be set to 0");

    static constexpr unsigned int kNumOutPorts = 1;
    static constexpr unsigned int index = 0;
    static constexpr unsigned int origParPow = 0;
    static constexpr unsigned int dynPtSize = 0;
    static constexpr unsigned int intParPow = 0;
    static constexpr unsigned int intUseWidgets = 0;

    typedef TT_DATA_D1 TT_OUT_DATA_D1;
    typedef TT_DATA_D2 TT_OUT_DATA_D2;
    static constexpr int kStreamsPerTile = get_input_streams_core_module(); // a device trait
    port<input> in[kStreamsPerTile];                                        // dual streams
    port<output> out[kNumOutPorts];
    kernel m_tposeKernels;
    static constexpr unsigned int kIsRealDataD1 =
        (std::is_same<TT_DATA_D1, int16>::value || std::is_same<TT_DATA_D1, bfloat16>::value) ? 1 : 0;
    typedef typename std::conditional_t<
        kIsRealDataD1,
        fft_dit_2ch_real::fft_dit_2ch_real_ref_graph<TT_DATA_D2,
                                                     TT_TWIDDLE,
                                                     TP_POINT_SIZE_D1,
                                                     TP_FFT_NIFFT,
                                                     TP_SHIFT,
                                                     TP_CASC_LEN,
                                                     TP_POINT_SIZE_D1 * TP_POINT_SIZE_D2 / 2,
                                                     TP_API,
                                                     TP_RND,
                                                     TP_SAT,
                                                     TP_TWIDDLE_MODE>,
        dit_1ch::fft_ifft_dit_1ch_ref_graph<TT_DATA_D1,
                                            TT_TWIDDLE,
                                            TP_POINT_SIZE_D1,
                                            TP_FFT_NIFFT,
                                            TP_SHIFT,
                                            TP_CASC_LEN,
                                            dynPtSize,
                                            TP_POINT_SIZE_D1 * TP_POINT_SIZE_D2,
                                            TP_API,
                                            intParPow,
                                            intUseWidgets,
                                            TP_RND,
                                            TP_SAT,
                                            TP_TWIDDLE_MODE> >
        frontGraphType;

    frontGraphType frontFFT;
    static constexpr unsigned kOutPtSizeReal = TP_POINT_SIZE_D1 / 2;
    static constexpr unsigned kOutPtSizeD1 = kIsRealDataD1 ? TP_POINT_SIZE_D1 / 2 : TP_POINT_SIZE_D1;
    dit_1ch::fft_ifft_dit_1ch_ref_graph<TT_DATA_D2,
                                        TT_TWIDDLE,
                                        TP_POINT_SIZE_D2,
                                        TP_FFT_NIFFT,
                                        TP_SHIFT,
                                        TP_CASC_LEN, // necessary to match UUT, but unused by ref model
                                        dynPtSize,
                                        kOutPtSizeD1 * TP_POINT_SIZE_D2,
                                        TP_API,
                                        intParPow,
                                        intUseWidgets,
                                        TP_RND,
                                        TP_SAT,
                                        TP_TWIDDLE_MODE,
                                        TT_OUT_DATA_D2,
                                        index,
                                        origParPow>
        backFFT;

    fft_ifft_2d_ref_graph() {
        m_tposeKernels = kernel::create_object<tpose::transpose_ref<TT_DATA_D2, kOutPtSizeD1, TP_POINT_SIZE_D2> >();
        // for case that this is the top level, output ports according to kStreamsPerTile.
        connect<>(in[0], frontFFT.in[0]);
        connect<>(frontFFT.out[0], m_tposeKernels.in[0]);
        dimensions(frontFFT.out[0]) = {kOutPtSizeD1 * TP_POINT_SIZE_D2};
        dimensions(m_tposeKernels.in[0]) = {kOutPtSizeD1 * TP_POINT_SIZE_D2};
        connect<>(m_tposeKernels.out[0], backFFT.in[0]);
        dimensions(m_tposeKernels.out[0]) = {kOutPtSizeD1 * TP_POINT_SIZE_D2};
        connect<>(backFFT.out[0], out[0]);

        // Specify mapping constraints
        runtime<ratio>(m_tposeKernels) = 0.2;

        // Source files
        source(m_tposeKernels) = "transpose_ref.cpp";
        headers(m_tposeKernels) = {"transpose_ref.hpp"};
    };
};
}
}
}
}
}
#endif // _DSPLIB_FFT_IFFT_2D_REF_GRAPH_HPP_
