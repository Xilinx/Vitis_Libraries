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
#ifndef _DSPLIB_FFT_IFFT_DIT_1CH_REF_GRAPH_HPP_
#define _DSPLIB_FFT_IFFT_DIT_1CH_REF_GRAPH_HPP_

#include <adf.h>
#include <vector>
#include <adf/arch/aie_arch_properties.hpp> // for get_input_streams_core_module
#include "widget_api_cast_ref.hpp"
#include "widget_api_cast_traits.hpp"
#include "mixed_radix_fft_ref.hpp"
#include "dynamic_dft_ref.hpp"
#include "dft_ref.hpp"
using namespace adf;
using namespace ::xf::dsp::aie::fft::dft;
using namespace xf::dsp::aie::widget::api_cast;
namespace xf {
namespace dsp {
namespace aie {
namespace fft {
namespace mixed_radix_fft {

template <typename TT_DATA,
          typename TT_TWIDDLE,
          unsigned int TP_POINT_SIZE,
          unsigned int TP_FFT_NIFFT,
          unsigned int TP_SHIFT,
          unsigned int TP_RND, // TODO - needs to be passed to dft ref model once dft ref model supports rnd and sat.
          unsigned int TP_SAT =
              1, // TODO - needs to be passed to dft ref model once dft ref model supports rnd and sat.
          unsigned int TP_WINDOW_VSIZE = TP_POINT_SIZE, // to support multiple frames in an iobuffer
          unsigned int TP_CASC_LEN = 1,                 // necessary to match UUT, but unused by ref model
          unsigned int TP_API = 0,                      // iobuffer only to begin with
          unsigned int TP_DYN_PT_SIZE = 0>
class mixed_radix_fft_ref_graph : public graph {
   public:
    static constexpr int kStreamsPerTile = get_input_streams_core_module(); // a device trait
    static constexpr int kAPIFactor = TP_API == 0 ? 1 : kStreamsPerTile;
    static constexpr int kHeaderBytes = 0; // no dynamic mode

    port<input> in[kAPIFactor];
    port<output> out[kAPIFactor];

    // FIR Kernel
    kernel m_mixed_radix_fftKernel;
    kernel m_inWidget;
    kernel m_outWidget;

    // Constructor
    mixed_radix_fft_ref_graph() {
        printf("===================================\n");
        printf("== MIXED_RADIX_FFT 1 Channel mono REF \n");
        printf("== Graph window specialization\n");
        printf("===================================\n");

        printf("Data type            = ");
        printf(QUOTE(TT_DATA));
        printf("\n");
        printf("TWIDDLE type         = ");
        printf(QUOTE(TT_TWIDDLE));
        printf("\n");
        printf("Point size           = %d \n", TP_POINT_SIZE);
        printf("FFT/nIFFT            = %d \n", TP_FFT_NIFFT);
        printf("Final scaling Shift  = %d \n", TP_SHIFT);
        printf("Round mode           = %d \n", TP_RND);
        printf("Saturation mode      = %d \n", TP_SAT);
        printf("Dynamic point size   = %d \n", TP_DYN_PT_SIZE);

        // Create MIXED_RADIX_FFT class
        m_mixed_radix_fftKernel = kernel::create_object<
            dft_ref<TT_DATA, TT_TWIDDLE, TP_POINT_SIZE, TP_FFT_NIFFT, TP_SHIFT,
                    TP_WINDOW_VSIZE / TP_POINT_SIZE /*NUM_FRAMES*/, TP_RND, TP_SAT, 1 /*TP_SSR*/> >();

        // Make connections
        // Size of window in Bytes. Dynamic point size adds a 256 bit (32 byte) header. This is larger than required,
        // but keeps 256 bit alignment
        if (TP_API == kWindowAPI) {
            connect(in[0], m_mixed_radix_fftKernel.in[0]);
            dimensions(m_mixed_radix_fftKernel.in[0]) = {TP_WINDOW_VSIZE};
            connect(m_mixed_radix_fftKernel.out[0], out[0]);
            dimensions(m_mixed_radix_fftKernel.out[0]) = {TP_WINDOW_VSIZE};
        } else {
            m_inWidget =
                kernel::create_object<widget_api_cast_ref<TT_DATA, kStreamAPI, kWindowAPI, kAPIFactor, TP_WINDOW_VSIZE,
                                                          1, kSampleIntlv, kHeaderBytes> >(-1);
            m_outWidget = kernel::create_object<widget_api_cast_ref<TT_DATA, kWindowAPI, kStreamAPI, 1, TP_WINDOW_VSIZE,
                                                                    kAPIFactor, kSampleIntlv, kHeaderBytes> >(-1);

            for (int i = 0; i < kAPIFactor; i++) {
                connect<stream>(in[i], m_inWidget.in[i]);
                connect<stream>(m_outWidget.out[i], out[i]);
            }
            connect(m_inWidget.out[0], m_mixed_radix_fftKernel.in[0]);
            dimensions(m_inWidget.out[0]) = {TP_WINDOW_VSIZE};
            dimensions(m_mixed_radix_fftKernel.in[0]) = {TP_WINDOW_VSIZE};
            connect(m_mixed_radix_fftKernel.out[0], m_outWidget.in[0]);
            dimensions(m_outWidget.in[0]) = {TP_WINDOW_VSIZE};
            dimensions(m_mixed_radix_fftKernel.out[0]) = {TP_WINDOW_VSIZE};
        }

        // Specify mapping constraints
        runtime<ratio>(m_mixed_radix_fftKernel) = 0.7;
        runtime<ratio>(m_inWidget) = 0.1;
        runtime<ratio>(m_outWidget) = 0.1;

        // Source files
        // source(m_mixed_radix_fftKernel)  = "mixed_radix_fft_ref.cpp";
        // headers(m_mixed_radix_fftKernel) = {"mixed_radix_fft_ref.hpp"};
        source(m_mixed_radix_fftKernel) = "dft_ref.cpp";
        headers(m_mixed_radix_fftKernel) = {"dft_ref.hpp"};
        source(m_inWidget) = "widget_api_cast_ref.cpp";
        headers(m_inWidget) = {"widget_api_cast_ref.hpp"};
        source(m_outWidget) = "widget_api_cast_ref.cpp";
        headers(m_outWidget) = {"widget_api_cast_ref.hpp"};
        printf("== Graph window specialization exit\n");
    };
};

// dynamic specialisation
template <typename TT_DATA,
          typename TT_TWIDDLE,
          unsigned int TP_POINT_SIZE,
          unsigned int TP_FFT_NIFFT,
          unsigned int TP_SHIFT,
          unsigned int TP_RND, // TODO - needs to be passed to dft ref model once dft ref model supports rnd and sat.
          unsigned int TP_SAT, // TODO - needs to be passed to dft ref model once dft ref model supports rnd and sat.
          unsigned int TP_WINDOW_VSIZE, // to support multiple frames in an iobuffer
          unsigned int TP_CASC_LEN,     // necessary to match UUT, but unused by ref model
          unsigned int TP_API           // iobuffer only to begin with
          >
class mixed_radix_fft_ref_graph<TT_DATA,
                                TT_TWIDDLE,
                                TP_POINT_SIZE,
                                TP_FFT_NIFFT,
                                TP_SHIFT,
                                TP_RND,
                                TP_SAT,
                                TP_WINDOW_VSIZE,
                                TP_CASC_LEN,
                                TP_API,
                                1> : public graph {
   public:
    port<input> in[1];
    port<input> headerIn[1];
    port<output> out[1];
    port<output> headerOut[1];

    // Kernels
    kernel m_dynamic_dftKernel;

    // Constructor
    mixed_radix_fft_ref_graph() {
        printf("===================================\n");
        printf("== MIXED_RADIX_FFT 1 Channel mono REF \n");
        printf("== Dynamic specialization\n");
        printf("===================================\n");

        printf("Data type            = ");
        printf(QUOTE(TT_DATA_TYPE));
        printf("\n");
        printf("TWIDDLE type         = ");
        printf(QUOTE(TT_TWIDDLE_TYPE));
        printf("\n");
        printf("Point size           = %d \n", TP_POINT_SIZE);
        printf("FFT/nIFFT            = %d \n", TP_FFT_NIFFT);
        printf("Final scaling Shift  = %d \n", TP_SHIFT);
        printf("Round mode           = %d \n", TP_RND);
        printf("Saturation mode      = %d \n", TP_SAT);
        printf("Dynamic point size   = 1  \n");

        // Create MIXED_RADIX_FFT class
        m_dynamic_dftKernel =
            kernel::create_object<dynamic_dft_ref<TT_DATA, TT_TWIDDLE, TP_POINT_SIZE, TP_FFT_NIFFT, TP_SHIFT,
                                                  TP_WINDOW_VSIZE, TP_RND, TP_SAT, 1 /* TP_SSR */> >();

        static constexpr int kNumBytesInLoad = 32;
        // Make connections
        // Size of window in Bytes. Dynamic point size adds a 256 bit (32 byte) header. This is larger than required,
        // but keeps 256 bit alignment
        connect(in[0], m_dynamic_dftKernel.in[0]);
        dimensions(m_dynamic_dftKernel.in[0]) = {TP_WINDOW_VSIZE};
        connect(headerIn[0], m_dynamic_dftKernel.in[1]);
        dimensions(m_dynamic_dftKernel.in[1]) = {kNumBytesInLoad / sizeof(TT_DATA)};
        connect(m_dynamic_dftKernel.out[0], out[0]);
        dimensions(m_dynamic_dftKernel.out[0]) = {TP_WINDOW_VSIZE};
        connect(m_dynamic_dftKernel.out[1], headerOut[0]);
        dimensions(m_dynamic_dftKernel.out[1]) = {kNumBytesInLoad / sizeof(TT_DATA)};

        // Specify mapping constraints
        runtime<ratio>(m_dynamic_dftKernel) = 0.7;

        // Source files
        // source(m_mixed_radix_fftKernel)  = "mixed_radix_fft_ref.cpp";
        // headers(m_mixed_radix_fftKernel) = {"mixed_radix_fft_ref.hpp"};
        source(m_dynamic_dftKernel) = "dynamic_dft_ref.cpp";
        headers(m_dynamic_dftKernel) = {"dynamic_dft_ref.hpp"};
        printf("== Dynamic specialization exit\n");
    };
};
}
}
}
}
}
#endif // _DSPLIB_mixed_radix_fft_REF_GRAPH_HPP_
