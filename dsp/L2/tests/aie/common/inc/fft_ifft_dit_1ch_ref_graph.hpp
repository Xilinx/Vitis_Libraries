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
#ifndef _DSPLIB_FFT_IFFT_DIT_1CH_REF_GRAPH_HPP_
#define _DSPLIB_FFT_IFFT_DIT_1CH_REF_GRAPH_HPP_

#include <adf.h>
#include <vector>
#include "fft_ifft_dit_1ch_ref.hpp"
#include "fft_r2comb_ref.hpp"
#include "widget_api_cast_ref.hpp"

namespace xf {
namespace dsp {
namespace aie {
namespace fft {
namespace dit_1ch {
using namespace adf;
using namespace xf::dsp::aie::widget::api_cast;
using namespace xf::dsp::aie::fft::r2comb;

template <typename TT_DATA,
          typename TT_TWIDDLE,
          unsigned int TP_POINT_SIZE,
          unsigned int TP_FFT_NIFFT,
          unsigned int TP_SHIFT,
          unsigned int TP_CASC_LEN = 1, // necessary to match UUT, but unused by ref model
          unsigned int TP_DYN_PT_SIZE = 0,
          unsigned int TP_WINDOW_VSIZE = TP_POINT_SIZE,
          unsigned int TP_API = kWindowAPI,
          unsigned int TP_ORIG_PAR_POWER = 0>
class fft_ifft_dit_1ch_mono_ref_graph : public graph {
   public:
    port<input> in[1];
    port<output> out[1];

    // FIR Kernel
    kernel m_fftKernel;

    // Constructor
    fft_ifft_dit_1ch_mono_ref_graph() {
        printf("===================================\n");
        printf("== FFT/IFFT DIT 1 Channel mono REF \n");
        printf("== Graph window specialization\n");
        printf("===================================\n");

        // Create FIR class
        m_fftKernel =
            kernel::create_object<fft_ifft_dit_1ch_ref<TT_DATA, TT_TWIDDLE, TP_POINT_SIZE, TP_FFT_NIFFT, TP_SHIFT,
                                                       TP_DYN_PT_SIZE, TP_WINDOW_VSIZE, TP_ORIG_PAR_POWER> >();

        // Make connections
        // Size of window in Bytes. Dynamic point size adds a 256 bit (32 byte) header. This is larger than required,
        // but keeps 256 bit alignment
        connect<window<TP_WINDOW_VSIZE * sizeof(TT_DATA) + TP_DYN_PT_SIZE * kFftDynHeadBytes> >(in[0],
                                                                                                m_fftKernel.in[0]);
        connect<window<TP_WINDOW_VSIZE * sizeof(TT_DATA) + TP_DYN_PT_SIZE * kFftDynHeadBytes> >(m_fftKernel.out[0],
                                                                                                out[0]);

        // Specify mapping constraints
        runtime<ratio>(m_fftKernel) = 0.4;

        // Source files
        source(m_fftKernel) = "fft_ifft_dit_1ch_ref.cpp";
        headers(m_fftKernel) = {"fft_ifft_dit_1ch_ref.hpp"};
        printf("== Graph window specialization exit\n");
    };
};

// specialization for stream I/O
template <typename TT_DATA,
          typename TT_TWIDDLE,
          unsigned int TP_POINT_SIZE,
          unsigned int TP_FFT_NIFFT,
          unsigned int TP_SHIFT,
          unsigned int TP_CASC_LEN, // necessary to match UUT, but unused by ref model
          unsigned int TP_DYN_PT_SIZE,
          unsigned int TP_WINDOW_VSIZE,
          unsigned int TP_ORIG_PAR_POWER>
class fft_ifft_dit_1ch_mono_ref_graph<TT_DATA,
                                      TT_TWIDDLE,
                                      TP_POINT_SIZE,
                                      TP_FFT_NIFFT,
                                      TP_SHIFT,
                                      TP_CASC_LEN,
                                      TP_DYN_PT_SIZE,
                                      TP_WINDOW_VSIZE,
                                      kStreamAPI,
                                      TP_ORIG_PAR_POWER> : public graph {
   public:
    port<input> in[2];
    port<output> out[2];

    // FIR Kernel
    kernel m_fftKernel;
    kernel m_inWidget;
    kernel m_outWidget;

    static constexpr int kHeaderBytes =
        TP_DYN_PT_SIZE == 1 ? kFftDynHeadBytes : 0; // header for dynamic point size is 32 bytes

    // Constructor
    fft_ifft_dit_1ch_mono_ref_graph() {
        printf("===================================\n");
        printf("== FFT/IFFT DIT 1 Channel mono REF \n");
        printf("== Graph stream specialization\n");
        printf("===================================\n");

        // Create FIR class
        m_fftKernel =
            kernel::create_object<fft_ifft_dit_1ch_ref<TT_DATA, TT_TWIDDLE, TP_POINT_SIZE, TP_FFT_NIFFT, TP_SHIFT,
                                                       TP_DYN_PT_SIZE, TP_WINDOW_VSIZE, TP_ORIG_PAR_POWER> >();
        m_inWidget = kernel::create_object<
            widget_api_cast_ref<TT_DATA, kStreamAPI, kWindowAPI, 2, TP_WINDOW_VSIZE, 1, kSampleIntlv, kHeaderBytes> >(
            -1);
        m_outWidget = kernel::create_object<
            widget_api_cast_ref<TT_DATA, kWindowAPI, kStreamAPI, 1, TP_WINDOW_VSIZE, 2, kSampleIntlv, kHeaderBytes> >(
            -1);

        // Make connections
        connect<stream>(in[0], m_inWidget.in[0]);
        connect<stream>(in[1], m_inWidget.in[1]);

        // Size of window in Bytes. Dynamic point size adds a 256 bit (32 byte) header. This is larger than required,
        // but keeps 256 bit alignment
        connect<window<TP_WINDOW_VSIZE * sizeof(TT_DATA) + kHeaderBytes> >(m_inWidget.out[0], m_fftKernel.in[0]);
        connect<window<TP_WINDOW_VSIZE * sizeof(TT_DATA) + kHeaderBytes> >(m_fftKernel.out[0], m_outWidget.in[0]);

        connect<stream>(m_outWidget.out[0], out[0]);
        connect<stream>(m_outWidget.out[1], out[1]);

        // Specify mapping constraints
        runtime<ratio>(m_fftKernel) = 0.4;
        runtime<ratio>(m_inWidget) = 0.4;
        runtime<ratio>(m_outWidget) = 0.4;

        // Source files
        source(m_fftKernel) = "fft_ifft_dit_1ch_ref.cpp";
        headers(m_fftKernel) = {"fft_ifft_dit_1ch_ref.hpp"};
        source(m_inWidget) = "widget_api_cast_ref.cpp";
        headers(m_inWidget) = {"widget_api_cast_ref.hpp"};
        source(m_outWidget) = "widget_api_cast_ref.cpp";
        headers(m_outWidget) = {"widget_api_cast_ref.hpp"};
        printf("== Graph stream specialization exit\n");
    };
};

//------------------------------
// Top level - entry point
// This form mimics the structure of the UUT and is therefore prone to common mode errors.
template <typename TT_DATA,
          typename TT_TWIDDLE,
          unsigned int TP_POINT_SIZE,
          unsigned int TP_FFT_NIFFT,
          unsigned int TP_SHIFT,
          unsigned int TP_CASC_LEN, // necessary to match UUT, but unused by ref model
          unsigned int TP_DYN_PT_SIZE,
          unsigned int TP_WINDOW_VSIZE,
          unsigned int TP_API,
          unsigned int TP_PARALLEL_POWER = 1,
          unsigned int TP_ORIG_PAR_POWER = TP_PARALLEL_POWER>
class fft_ifft_dit_1ch_ref_graph : public graph {
   public:
    static_assert(!(std::is_same<TT_DATA, cfloat>::value) || (TP_SHIFT == 0),
                  "ERROR: TP_SHIFT cannot be performed for TT_DATA=cfloat, so must be set to 0");
    static_assert(TP_API == kStreamAPI, "Error: Only Stream interface is supported for parallel FFT");
    static_assert(TP_PARALLEL_POWER >= 1 && TP_PARALLEL_POWER < 9,
                  "Error: TP_PARALLEL_POWER is out of supported range");

    port<input> in[2 << TP_PARALLEL_POWER];
    port<output> out[2 << TP_PARALLEL_POWER];

    static constexpr int kParallel_factor = 1 << TP_PARALLEL_POWER;
    static constexpr int kWindowSize = TP_WINDOW_VSIZE >> TP_PARALLEL_POWER;

    static constexpr int kR2Shift = TP_SHIFT > 0 ? 1 : 0;
    static constexpr int kFFTsubShift = TP_SHIFT > 0 ? TP_SHIFT - 1 : 0;

    static constexpr int kHeaderBytes =
        TP_DYN_PT_SIZE == 1 ? kFftDynHeadBytes : 0; // header for dynamic point size is 32 bytes

    kernel m_combInKernel[kParallel_factor];
    kernel m_r2Comb[kParallel_factor];
    kernel m_combOutKernel[kParallel_factor];

    fft_ifft_dit_1ch_ref_graph<TT_DATA,
                               TT_TWIDDLE,
                               (TP_POINT_SIZE >> 1),
                               TP_FFT_NIFFT,
                               kFFTsubShift,
                               TP_CASC_LEN,
                               TP_DYN_PT_SIZE,
                               (TP_WINDOW_VSIZE >> 1),
                               kStreamAPI,
                               (TP_PARALLEL_POWER - 1),
                               TP_ORIG_PAR_POWER>
        FFTsubframe[2]; // fractal or recursive decomposition

    // constructor
    fft_ifft_dit_1ch_ref_graph() {
        printf("========================================\n");
        printf("Entering top level ref graph constructor\n");
        printf("========================================\n");
        for (int i = 0; i < kParallel_factor; i++) {
            m_combInKernel[i] = kernel::create_object<
                widget_api_cast_ref<TT_DATA, kStreamAPI, kWindowAPI, 2, kWindowSize, 1, kSampleIntlv, kHeaderBytes> >(
                i);
            m_r2Comb[i] = kernel::create_object<
                fft_r2comb_ref<TT_DATA, TT_TWIDDLE, TP_POINT_SIZE, TP_FFT_NIFFT, kR2Shift, TP_DYN_PT_SIZE, kWindowSize,
                               TP_PARALLEL_POWER, TP_ORIG_PAR_POWER> >(i);
            m_combOutKernel[i] = kernel::create_object<
                widget_api_cast_ref<TT_DATA, kWindowAPI, kStreamAPI, 1, kWindowSize, 2, kSampleIntlv, kHeaderBytes> >(
                i);
        }
        // make connections
        for (int i = 0; i < kParallel_factor; i++) {
            connect<stream>(in[2 * i], FFTsubframe[0].in[i]);     // stream connection
            connect<stream>(in[2 * i + 1], FFTsubframe[1].in[i]); // stream connection
            connect<stream>(FFTsubframe[0].out[i], m_combInKernel[i].in[0]);
            connect<stream>(FFTsubframe[1].out[i], m_combInKernel[i].in[1]);
            connect<window<kWindowSize * sizeof(TT_DATA) + kHeaderBytes> >(m_combInKernel[i].out[0], m_r2Comb[i].in[0]);
            connect<window<kWindowSize * sizeof(TT_DATA) + kHeaderBytes> >(m_r2Comb[i].out[0],
                                                                           m_combOutKernel[i].in[0]);
            connect<stream>(m_combOutKernel[i].out[0], out[i]);
            connect<stream>(m_combOutKernel[i].out[1], out[i + kParallel_factor]);
        }

        // Associate kernels with Source files and set runtime ratio
        for (int i = 0; i < kParallel_factor; i++) {
            source(m_combInKernel[i]) = "widget_api_cast_ref.cpp";
            source(m_r2Comb[i]) = "fft_r2comb_ref.cpp";
            source(m_combOutKernel[i]) = "widget_api_cast_ref.cpp";
            headers(m_combInKernel[i]) = {"widget_api_cast_ref.hpp"};
            headers(m_r2Comb[i]) = {"fft_r2comb_ref.hpp"};
            headers(m_combOutKernel[i]) = {"widget_api_cast_ref.hpp"};
            runtime<ratio>(m_combInKernel[i]) = 0.94;
            runtime<ratio>(m_r2Comb[i]) = 0.94;
            runtime<ratio>(m_combOutKernel[i]) = 0.94;
        }
        printf("Exiting top level ref graph constructor\n");
    }; // end of constructor
};     // end of class

// specialization for trivial mapping, i.e. single (monolithic) FFT, window API
template <typename TT_DATA,
          typename TT_TWIDDLE,
          unsigned int TP_POINT_SIZE,
          unsigned int TP_FFT_NIFFT,
          unsigned int TP_SHIFT,
          unsigned int TP_CASC_LEN,
          unsigned int TP_DYN_PT_SIZE,
          unsigned int TP_WINDOW_VSIZE,
          unsigned int TP_ORIG_PAR_POWER>
class fft_ifft_dit_1ch_ref_graph<TT_DATA,
                                 TT_TWIDDLE,
                                 TP_POINT_SIZE,
                                 TP_FFT_NIFFT,
                                 TP_SHIFT,
                                 TP_CASC_LEN,
                                 TP_DYN_PT_SIZE,
                                 TP_WINDOW_VSIZE,
                                 kWindowAPI,
                                 0,
                                 TP_ORIG_PAR_POWER> : public graph {
   public:
    static_assert(!(std::is_same<TT_DATA, cfloat>::value) || (TP_SHIFT == 0),
                  "ERROR: TP_SHIFT cannot be performed for TT_DATA=cfloat, so must be set to 0");
    static constexpr int kHeaderBytes =
        TP_DYN_PT_SIZE == 1 ? kFftDynHeadBytes : 0; // header for dynamic point size is 32 bytes

    port<input> in[1];
    port<output> out[1];

    fft_ifft_dit_1ch_mono_ref_graph<TT_DATA,
                                    TT_TWIDDLE,
                                    TP_POINT_SIZE,
                                    TP_FFT_NIFFT,
                                    TP_SHIFT,
                                    TP_CASC_LEN,
                                    TP_DYN_PT_SIZE,
                                    TP_WINDOW_VSIZE,
                                    kWindowAPI,
                                    TP_ORIG_PAR_POWER>
        FFTwinproc;

    fft_ifft_dit_1ch_ref_graph() {
        connect<window<TP_WINDOW_VSIZE * sizeof(TT_DATA) + kHeaderBytes> >(in[0], FFTwinproc.in[0]);
        connect<window<TP_WINDOW_VSIZE * sizeof(TT_DATA) + kHeaderBytes> >(FFTwinproc.out[0], out[0]);
    };
};
// specialization for end of recursion, i.e. single FFT, stream API
template <typename TT_DATA,
          typename TT_TWIDDLE,
          unsigned int TP_POINT_SIZE,
          unsigned int TP_FFT_NIFFT,
          unsigned int TP_SHIFT,
          unsigned int TP_CASC_LEN,
          unsigned int TP_DYN_PT_SIZE,
          unsigned int TP_WINDOW_VSIZE,
          unsigned int TP_ORIG_PAR_POWER>
class fft_ifft_dit_1ch_ref_graph<TT_DATA,
                                 TT_TWIDDLE,
                                 TP_POINT_SIZE,
                                 TP_FFT_NIFFT,
                                 TP_SHIFT,
                                 TP_CASC_LEN,
                                 TP_DYN_PT_SIZE,
                                 TP_WINDOW_VSIZE,
                                 kStreamAPI,
                                 0,
                                 TP_ORIG_PAR_POWER> : public graph {
   public:
    static_assert(!(std::is_same<TT_DATA, cfloat>::value) || (TP_SHIFT == 0),
                  "ERROR: TP_SHIFT cannot be performed for TT_DATA=cfloat, so must be set to 0");

    port<input> in[2]; // dual streams
    port<output> out[2];

    fft_ifft_dit_1ch_mono_ref_graph<TT_DATA,
                                    TT_TWIDDLE,
                                    TP_POINT_SIZE,
                                    TP_FFT_NIFFT,
                                    TP_SHIFT,
                                    TP_CASC_LEN,
                                    TP_DYN_PT_SIZE,
                                    TP_WINDOW_VSIZE,
                                    kStreamAPI,
                                    TP_ORIG_PAR_POWER>
        FFTstrproc;

    fft_ifft_dit_1ch_ref_graph() {
        printf("end of recursion, stream graph\n");
        connect<stream>(in[0], FFTstrproc.in[0]);
        connect<stream>(in[1], FFTstrproc.in[1]);
        connect<stream>(FFTstrproc.out[0], out[0]);
        connect<stream>(FFTstrproc.out[1], out[1]);
    };
};
}
}
}
}
}
#endif // _DSPLIB_FFT_IFFT_DIT_1CH_REF_GRAPH_HPP_
