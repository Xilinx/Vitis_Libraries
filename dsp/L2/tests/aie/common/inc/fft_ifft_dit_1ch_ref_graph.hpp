/*
 * Copyright (C) 2019-2022, Xilinx, Inc.
 * Copyright (C) 2022-2024, Advanced Micro Devices, Inc.
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
#include <adf/arch/aie_arch_properties.hpp> //for device traits like number of streams per tile
#include "fft_ifft_dit_1ch_ref.hpp"
#include "fft_r2comb_ref.hpp"
#include "widget_api_cast_ref.hpp"
#include "widget_api_cast_traits.hpp"

namespace xf {
namespace dsp {
namespace aie {
namespace fft {
namespace dit_1ch {
using namespace adf;
using namespace xf::dsp::aie::widget::api_cast;
using namespace xf::dsp::aie::fft::r2comb;

// Utility function to determine the output API of an FFT atom (atom? - single kernel FFT which may be part of a larger
// graph FFT).
// AIE1 is easy - outAPI = TP_API for single kernel  and kStream otherwise.
// For AIE2, the column of FFTs alternate between (cascade, stream) and (stream, cascade) for the 2 outputs.
// Utility function to get API for R2 input
template <unsigned int TP_API, unsigned int TP_ORIG_PAR_POWER, unsigned int TP_PARALLEL_POWER, unsigned int TP_INDEX>
constexpr unsigned int fnGetFFTSubframeOutAPI() {
    if (TP_ORIG_PAR_POWER == 0) {
        return TP_API;
    } else {
        if
            constexpr(get_input_streams_core_module() == 2) { return kStreamAPI; }
        else {
            if
                constexpr(TP_INDEX % 2 == 0) { return kCascStreamAPI; }
            else {
                return kStreamCascAPI;
            }
        }
    }
}

// Utility function to get API for R2 input
template <unsigned int TP_API, unsigned int TP_ORIG_PAR_POWER, unsigned int TP_PARALLEL_POWER, unsigned int TP_INDEX>
constexpr unsigned int fnGetR2InAPI() {
    if
        constexpr(get_input_streams_core_module() == 2) { return kStreamAPI; }
    else {
        if
            constexpr((TP_INDEX >> (TP_PARALLEL_POWER - 1)) % 2 == 0) { return kCascStreamAPI; }
        else {
            return kStreamCascAPI;
        }
    }
}

// Utility function to get API for R2 output
template <unsigned int TP_API, unsigned int TP_ORIG_PAR_POWER, unsigned int TP_PARALLEL_POWER, unsigned int TP_INDEX>
constexpr unsigned int fnGetR2OutAPI() {
    if
        constexpr(TP_ORIG_PAR_POWER == TP_PARALLEL_POWER) { return TP_API; }
    else {
        if
            constexpr(get_input_streams_core_module() == 2) { return kStreamAPI; }
        else {
            if
                constexpr((TP_INDEX >> TP_PARALLEL_POWER) % 2 == 0) { return kCascStreamAPI; }
            else {
                return kStreamCascAPI;
            }
        }
    }
}

// End of utility functions

// Start of recursive classes for R2 kernel creation
//-----------------------------------------
// Recusive classes for r2 combiner creation

// Base specialization
template <typename TT_OUT_DATA,
          typename TT_TWIDDLE,
          unsigned int TP_POINT_SIZE,
          unsigned int TP_FFT_NIFFT,
          unsigned int TP_SHIFT,
          unsigned int TP_DYN_PT_SIZE,
          unsigned int TP_WINDOW_VSIZE,
          unsigned int TP_PARALLEL_POWER,
          unsigned int TP_INDEX,
          unsigned int TP_INDEX_BASE, // the wider context. Allows calculation of TP_INDEX in overal design, not just
                                      // this level of recursion.
          unsigned int TP_ORIG_PAR_POWER,
          unsigned int TP_API,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_TWIDDLE_MODE>
class create_r2comb_kernels {
   public:
    static constexpr int kParallelFactor = 1 << TP_PARALLEL_POWER;
    static constexpr int kHeaderBytes =
        TP_DYN_PT_SIZE == 1 ? kFftDynHeadBytes : 0; // header for dynamic point size is 32 bytes
    static constexpr unsigned int kInAPI =
        fnGetR2InAPI<TP_API, TP_ORIG_PAR_POWER, TP_PARALLEL_POWER, TP_INDEX_BASE + TP_INDEX>();
    static constexpr unsigned int kOutAPI =
        fnGetR2OutAPI<TP_API, TP_ORIG_PAR_POWER, TP_PARALLEL_POWER, TP_INDEX_BASE + TP_INDEX>();
    static constexpr unsigned int kStreamsPerTile =
        get_input_streams_core_module(); // a device trait =2 for AIE1, =1 for AIE2
    static constexpr unsigned int kInputsPerLane = TP_API == kWindowAPI ? 1 : kStreamsPerTile;
    static constexpr unsigned int kOutputsPerLane = (TP_PARALLEL_POWER == TP_ORIG_PAR_POWER) ? kInputsPerLane : 2;
    static constexpr unsigned int kNumOutPorts = (TP_PARALLEL_POWER == TP_ORIG_PAR_POWER) ? kStreamsPerTile : 2;
    static constexpr unsigned int kTwIndex = TP_INDEX % kParallelFactor;

    static void create(kernel (&m_combInKernel)[kParallelFactor],
                       kernel (&m_r2Comb)[kParallelFactor],
                       kernel (&m_combOutKernel)[kParallelFactor]) {
        m_combInKernel[TP_INDEX] = kernel::create_object<
            widget_api_cast_ref<TT_OUT_DATA, kInAPI, kWindowAPI, 2, TP_WINDOW_VSIZE, 1, kSampleIntlv, kHeaderBytes> >(
            TP_INDEX);
        m_r2Comb[TP_INDEX] = kernel::create_object<
            fft_r2comb_ref<TT_OUT_DATA, TT_TWIDDLE, TP_POINT_SIZE, TP_FFT_NIFFT, TP_SHIFT, TP_DYN_PT_SIZE,
                           TP_WINDOW_VSIZE, TP_PARALLEL_POWER, TP_ORIG_PAR_POWER, TP_RND, TP_SAT, TP_TWIDDLE_MODE> >(
            kTwIndex);
        if (kOutAPI != kWindowAPI) { // if top level output is a window, not need for widget in last column
            m_combOutKernel[TP_INDEX] =
                kernel::create_object<widget_api_cast_ref<TT_OUT_DATA, kWindowAPI, kOutAPI, 1, TP_WINDOW_VSIZE,
                                                          kOutputsPerLane, kSampleIntlv, kHeaderBytes> >(TP_INDEX);
        }
        if
            constexpr(TP_INDEX > 0) { // This avoids the need for an end-of-recursion specialization
                create_r2comb_kernels<TT_OUT_DATA, TT_TWIDDLE, TP_POINT_SIZE, TP_FFT_NIFFT, TP_SHIFT, TP_DYN_PT_SIZE,
                                      TP_WINDOW_VSIZE, TP_PARALLEL_POWER, (TP_INDEX - 1), TP_INDEX_BASE,
                                      TP_ORIG_PAR_POWER, TP_API, TP_RND, TP_SAT,
                                      TP_TWIDDLE_MODE>::create(m_combInKernel, m_r2Comb, m_combOutKernel);
            }
    }
};

//-------------------------------
// Start of FFT graph definitions.

// specialization for stream I/O
template <typename TT_DATA,
          typename TT_TWIDDLE,
          unsigned int TP_POINT_SIZE,
          unsigned int TP_FFT_NIFFT,
          unsigned int TP_SHIFT,
          unsigned int TP_CASC_LEN, // necessary to match UUT, but unused by ref model
          unsigned int TP_DYN_PT_SIZE,
          unsigned int TP_WINDOW_VSIZE,
          unsigned int TP_IN_API = kWindowAPI,
          unsigned int TP_OUT_API = kWindowAPI,
          unsigned int TP_ORIG_PAR_POWER = 0,
          unsigned int TP_RND = 0,
          unsigned int TP_SAT = 1,
          unsigned int TP_TWIDDLE_MODE = 0,
          typename TT_OUT_DATA = TT_DATA>
class fft_ifft_dit_1ch_mono_ref_graph : public graph {
   public:
    static constexpr int kStreamsPerTile = get_input_streams_core_module(); // a device trait
    static constexpr int kNumInputs = get_input_streams_core_module();
    static constexpr int kNumOutputs = (TP_ORIG_PAR_POWER == 0) ? get_input_streams_core_module() : 2;

    port<input> in[kNumInputs];
    port<output> out[kNumOutputs];

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
        m_fftKernel = kernel::create_object<
            fft_ifft_dit_1ch_ref<TT_DATA, TT_TWIDDLE, TP_POINT_SIZE, TP_FFT_NIFFT, TP_SHIFT, TP_DYN_PT_SIZE,
                                 TP_WINDOW_VSIZE, TP_ORIG_PAR_POWER, TP_RND, TP_SAT, TP_TWIDDLE_MODE, TT_OUT_DATA> >();

        // Make connections
        if (TP_IN_API == kWindowAPI) {
            connect<>(in[0], m_fftKernel.in[0]);
            dimensions(m_fftKernel.in[0]) = {TP_WINDOW_VSIZE + kHeaderBytes / sizeof(TT_DATA)};
        } else {
            m_inWidget =
                kernel::create_object<widget_api_cast_ref<TT_DATA, TP_IN_API, kWindowAPI, kNumInputs, TP_WINDOW_VSIZE,
                                                          1, kSampleIntlv, kHeaderBytes> >(-1);
            for (int i = 0; i < kNumInputs; i++) {
                connect<stream>(in[i], m_inWidget.in[i]);
            }
            // Size of window in Bytes. Dynamic point size adds a 256 bit (32 byte) header. This is larger than
            // required, but keeps 256 bit alignment
            connect(m_inWidget.out[0], m_fftKernel.in[0]);
            dimensions(m_inWidget.out[0]) = {TP_WINDOW_VSIZE + kHeaderBytes / sizeof(TT_DATA)};
            dimensions(m_fftKernel.in[0]) = {TP_WINDOW_VSIZE + kHeaderBytes / sizeof(TT_DATA)};
        }

        if (TP_OUT_API == kWindowAPI) {
            connect<>(m_fftKernel.out[0], out[0]);
            dimensions(m_fftKernel.out[0]) = {TP_WINDOW_VSIZE + kHeaderBytes / sizeof(TT_OUT_DATA)};
        } else {
            m_outWidget =
                kernel::create_object<widget_api_cast_ref<TT_OUT_DATA, kWindowAPI, TP_OUT_API, 1, TP_WINDOW_VSIZE,
                                                          kNumOutputs, kSampleIntlv, kHeaderBytes> >(-1);
            connect(m_fftKernel.out[0], m_outWidget.in[0]);
            dimensions(m_fftKernel.out[0]) = {TP_WINDOW_VSIZE + kHeaderBytes / sizeof(TT_OUT_DATA)};
            dimensions(m_outWidget.in[0]) = {TP_WINDOW_VSIZE + kHeaderBytes / sizeof(TT_OUT_DATA)};
            if (kNumOutputs == 1) {
                connect<stream>(m_outWidget.out[0], out[0]);
            } else {
                if (TP_OUT_API == kStreamAPI) {
                    connect<stream>(m_outWidget.out[0], out[0]);
                    connect<stream>(m_outWidget.out[1], out[1]);
                } else if (TP_OUT_API == kCascStreamAPI) {
                    connect<cascade>(m_outWidget.out[0], out[0]);
                    connect<stream>(m_outWidget.out[1], out[1]);
                } else if (TP_OUT_API == kStreamCascAPI) {
                    connect<stream>(m_outWidget.out[0], out[0]);
                    connect<cascade>(m_outWidget.out[1], out[1]);
                }
            }
        }

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

// specialization for window in and window out (no widgets)
template <typename TT_DATA,
          typename TT_TWIDDLE,
          unsigned int TP_POINT_SIZE,
          unsigned int TP_FFT_NIFFT,
          unsigned int TP_SHIFT,
          unsigned int TP_CASC_LEN, // necessary to match UUT, but unused by ref model
          unsigned int TP_DYN_PT_SIZE,
          unsigned int TP_WINDOW_VSIZE,
          unsigned int TP_ORIG_PAR_POWER,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_TWIDDLE_MODE,
          typename TT_OUT_DATA>
class fft_ifft_dit_1ch_mono_ref_graph<TT_DATA,
                                      TT_TWIDDLE,
                                      TP_POINT_SIZE,
                                      TP_FFT_NIFFT,
                                      TP_SHIFT,
                                      TP_CASC_LEN,
                                      TP_DYN_PT_SIZE,
                                      TP_WINDOW_VSIZE,
                                      kWindowAPI,
                                      kWindowAPI,
                                      TP_ORIG_PAR_POWER,
                                      TP_RND,
                                      TP_SAT,
                                      TP_TWIDDLE_MODE,
                                      TT_OUT_DATA> : public graph {
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
        m_fftKernel = kernel::create_object<
            fft_ifft_dit_1ch_ref<TT_DATA, TT_TWIDDLE, TP_POINT_SIZE, TP_FFT_NIFFT, TP_SHIFT, TP_DYN_PT_SIZE,
                                 TP_WINDOW_VSIZE, TP_ORIG_PAR_POWER, TP_RND, TP_SAT, TP_TWIDDLE_MODE, TT_OUT_DATA> >();

        // Make connections
        // Size of window in Bytes. Dynamic point size adds a 256 bit (32 byte) header. This is larger than required,
        // but keeps 256 bit alignment
        connect(in[0], m_fftKernel.in[0]);
        dimensions(m_fftKernel.in[0]) = {TP_WINDOW_VSIZE + TP_DYN_PT_SIZE * kFftDynHeadBytes / sizeof(TT_DATA)};
        connect(m_fftKernel.out[0], out[0]);
        dimensions(m_fftKernel.out[0]) = {TP_WINDOW_VSIZE + TP_DYN_PT_SIZE * kFftDynHeadBytes / sizeof(TT_OUT_DATA)};

        // Specify mapping constraints
        runtime<ratio>(m_fftKernel) = 0.4;

        // Source files
        source(m_fftKernel) = "fft_ifft_dit_1ch_ref.cpp";
        headers(m_fftKernel) = {"fft_ifft_dit_1ch_ref.hpp"};
        printf("== Graph window specialization exit\n");
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
          unsigned int TP_USE_WIDGETS = 0, // not used by ref model
          unsigned int TP_RND = 0,
          unsigned int TP_SAT = 1,
          unsigned int TP_TWIDDLE_MODE = 0,
          typename TT_OUT_DATA = TT_DATA,
          unsigned int TP_INDEX = 0,
          unsigned int TP_ORIG_PAR_POWER = TP_PARALLEL_POWER>
class fft_ifft_dit_1ch_ref_graph : public graph {
   public:
    static_assert(!(std::is_same<TT_DATA, cfloat>::value) || (TP_SHIFT == 0),
                  "ERROR: TP_SHIFT cannot be performed for TT_DATA=cfloat, so must be set to 0");
    // static_assert(TP_API==kStreamAPI,"Error: Only Stream interface is supported for parallel FFT"); // not any more
    // as of 23.1
    static_assert(TP_PARALLEL_POWER >= 1 && TP_PARALLEL_POWER < 9,
                  "Error: TP_PARALLEL_POWER is out of supported range");
    static_assert(TP_TWIDDLE_MODE >= 0 && TP_TWIDDLE_MODE <= 1,
                  "Error: TP_TWIDDLE_MODE is out of supported range (0, 1)");

    static constexpr int kStreamsPerTile = get_input_streams_core_module(); // a device trait =2 for AIE1, =1 for AIE2
    static constexpr int kInputsPerLane = TP_API == kWindowAPI ? 1 : kStreamsPerTile;
    static constexpr int kOutputsPerLane = (TP_PARALLEL_POWER == TP_ORIG_PAR_POWER) ? kInputsPerLane : 2;
    static constexpr int kParallelFactor = 1 << TP_PARALLEL_POWER;
    static constexpr int kWindowSize = TP_WINDOW_VSIZE >> TP_PARALLEL_POWER;
    static constexpr int kR2Shift = TP_SHIFT > 0 ? 1 : 0;
    static constexpr int kFFTsubShift = TP_SHIFT > 0 ? TP_SHIFT - 1 : 0;
    static constexpr int kHeaderBytes =
        TP_DYN_PT_SIZE == 1 ? kFftDynHeadBytes : 0; // header for dynamic point size is 32 bytes
    static constexpr int kR2OutAPI = TP_API;        // TP_PARALLEL_POWER == TP_ORIG_PAR_POWER ? TP_API : -1;

    port<input> in[kInputsPerLane << TP_PARALLEL_POWER];
    port<output> out[kOutputsPerLane << TP_PARALLEL_POWER];

    kernel m_combInKernel[kParallelFactor];
    kernel m_r2Comb[kParallelFactor];
    kernel m_combOutKernel[kParallelFactor];

    fft_ifft_dit_1ch_ref_graph<TT_DATA,
                               TT_TWIDDLE,
                               (TP_POINT_SIZE >> 1),
                               TP_FFT_NIFFT,
                               kFFTsubShift,
                               TP_CASC_LEN,
                               TP_DYN_PT_SIZE,
                               (TP_WINDOW_VSIZE >> 1),
                               TP_API,
                               (TP_PARALLEL_POWER - 1),
                               TP_USE_WIDGETS,
                               TP_RND,
                               TP_SAT,
                               TP_TWIDDLE_MODE,
                               TT_OUT_DATA,
                               TP_INDEX,
                               TP_ORIG_PAR_POWER>
        FFTsubframeA; // fractal or recursive decomposition
    fft_ifft_dit_1ch_ref_graph<TT_DATA,
                               TT_TWIDDLE,
                               (TP_POINT_SIZE >> 1),
                               TP_FFT_NIFFT,
                               kFFTsubShift,
                               TP_CASC_LEN,
                               TP_DYN_PT_SIZE,
                               (TP_WINDOW_VSIZE >> 1),
                               TP_API,
                               (TP_PARALLEL_POWER - 1),
                               TP_USE_WIDGETS,
                               TP_RND,
                               TP_SAT,
                               TP_TWIDDLE_MODE,
                               TT_OUT_DATA,
                               TP_INDEX + kParallelFactor / 2,
                               TP_ORIG_PAR_POWER>
        FFTsubframeB; // fractal or recursive decomposition

    // constructor
    fft_ifft_dit_1ch_ref_graph() {
        printf("========================================\n");
        printf("Entering top level ref graph constructor\n");
        printf("========================================\n");
        printf("Point size           = %d \n", TP_POINT_SIZE);
        printf("FFT/nIFFT            = %d \n", TP_FFT_NIFFT);
        printf("Final scaling Shift  = %d \n", TP_SHIFT);
        printf("Cascade Length       = %d \n", TP_CASC_LEN);
        printf("Dynamic point size   = %d \n", TP_DYN_PT_SIZE);
        printf("Window Size          = %d \n", TP_WINDOW_VSIZE);
        printf("API_IO               = %d \n", TP_API);
        printf("PARALLEL_POWER       = %d \n", TP_PARALLEL_POWER);
        printf("Use widgets          = %d \n", TP_USE_WIDGETS);
        printf("Round mode           = %d \n", TP_RND);
        printf("Sat mode             = %d \n", TP_SAT);
        printf("Twiddle mode         = %d \n", TP_TWIDDLE_MODE);
        // create kernels
        create_r2comb_kernels<TT_OUT_DATA, TT_TWIDDLE, TP_POINT_SIZE, TP_FFT_NIFFT, kR2Shift, TP_DYN_PT_SIZE,
                              kWindowSize, TP_PARALLEL_POWER, kParallelFactor - 1, TP_INDEX, TP_ORIG_PAR_POWER,
                              kR2OutAPI, TP_RND, TP_SAT, TP_TWIDDLE_MODE>::create(m_combInKernel, m_r2Comb,
                                                                                  m_combOutKernel);

        // make input connections
        if
            constexpr(TP_API == 0) {
                for (int i = 0; i < kParallelFactor / 2; i++) {
                    connect<>(in[2 * i], FFTsubframeA.in[i]);
                    connect<>(in[2 * i + 1], FFTsubframeB.in[i]);
                }
            }
        else { // TP_API == 1 (stream(s))
            for (int i = 0; i < kStreamsPerTile * kParallelFactor / 2; i++) {
                connect<stream>(in[2 * i], FFTsubframeA.in[i]);
                connect<stream>(in[2 * i + 1], FFTsubframeB.in[i]);
            }
        }

        // internal connections
        if
            constexpr(kStreamsPerTile == 2) { // AIE1
                for (int i = 0; i < kParallelFactor; i++) {
                    connect<stream>(FFTsubframeA.out[i], m_combInKernel[i].in[0]);
                    connect<stream>(FFTsubframeB.out[i], m_combInKernel[i].in[1]);
                }
            }
        else {
            for (int i = 0; i < kParallelFactor; i++) {
                if (i < kParallelFactor / 2) { // i.e. top half
                    connect<cascade>(FFTsubframeA.out[i], m_combInKernel[i].in[0]);
                    connect<stream>(FFTsubframeB.out[i], m_combInKernel[i].in[1]);
                } else { // bottom half
                    connect<stream>(FFTsubframeA.out[i], m_combInKernel[i].in[0]);
                    connect<cascade>(FFTsubframeB.out[i], m_combInKernel[i].in[1]);
                }
            }
        }

        // widget to R2comb connections
        for (int i = 0; i < kParallelFactor; i++) {
            connect(m_combInKernel[i].out[0], m_r2Comb[i].in[0]);
            dimensions(m_combInKernel[i].out[0]) = {kWindowSize + kHeaderBytes / sizeof(TT_OUT_DATA)};
            dimensions(m_r2Comb[i].in[0]) = {kWindowSize + kHeaderBytes / sizeof(TT_OUT_DATA)};

            // for top level and window output, there is no output widget.
            if (TP_ORIG_PAR_POWER != TP_PARALLEL_POWER || TP_API == kStreamAPI) {
                connect(m_r2Comb[i].out[0], m_combOutKernel[i].in[0]);
                dimensions(m_r2Comb[i].out[0]) = {kWindowSize + kHeaderBytes / sizeof(TT_OUT_DATA)};
                dimensions(m_combOutKernel[i].in[0]) = {kWindowSize + kHeaderBytes / sizeof(TT_OUT_DATA)};
            }
        }

        // output connections
        if (TP_PARALLEL_POWER == TP_ORIG_PAR_POWER) { // i.e. top level, so outputs are really outputs
            if (TP_API == kStreamAPI) {
                for (int i = 0; i < kParallelFactor; i++) {
                    connect<stream>(m_combOutKernel[i].out[0], out[i]);
                    if (kStreamsPerTile == 2) {
                        connect<stream>(m_combOutKernel[i].out[1], out[i + kParallelFactor]);
                    }
                }
            } else {
                for (int i = 0; i < kParallelFactor; i++) {
                    connect<>(m_r2Comb[i].out[0], out[i]);
                    dimensions(m_r2Comb[i].out[0]) = {kWindowSize + kHeaderBytes / sizeof(TT_OUT_DATA)};
                }
            }
        } else { // not top level, so outputs are internal trellis connections
            if (kStreamsPerTile == 2) {
                for (int i = 0; i < kParallelFactor; i++) {
                    connect<stream>(m_combOutKernel[i].out[0], out[i]);
                    connect<stream>(m_combOutKernel[i].out[1], out[i + kParallelFactor]);
                }
            } else {
                for (int i = 0; i < kParallelFactor; i++) {
                    if ((TP_INDEX >> TP_PARALLEL_POWER) % 2 == 0) { // i.e. top half
                        connect<cascade>(m_combOutKernel[i].out[0], out[i]);
                        connect<stream>(m_combOutKernel[i].out[1], out[i + kParallelFactor]);
                    } else {
                        connect<stream>(m_combOutKernel[i].out[0], out[i]);
                        connect<cascade>(m_combOutKernel[i].out[1], out[i + kParallelFactor]);
                    }
                }
            }
        }

        // Associate kernels with Source files and set runtime ratio
        for (int i = 0; i < kParallelFactor; i++) {
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
          unsigned int TP_USE_WIDGETS,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_TWIDDLE_MODE,
          typename TT_OUT_DATA,
          unsigned int TP_INDEX,
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
                                 TP_USE_WIDGETS,
                                 TP_RND,
                                 TP_SAT,
                                 TP_TWIDDLE_MODE,
                                 TT_OUT_DATA,
                                 TP_INDEX,
                                 TP_ORIG_PAR_POWER> : public graph {
   public:
    static_assert(!(std::is_same<TT_DATA, cfloat>::value) || (TP_SHIFT == 0),
                  "ERROR: TP_SHIFT cannot be performed for TT_DATA=cfloat, so must be set to 0");
    static constexpr int kHeaderBytes =
        TP_DYN_PT_SIZE == 1 ? kFftDynHeadBytes : 0;      // header for dynamic point size is 32 bytes
    static constexpr unsigned int TP_PARALLEL_POWER = 0; // for this specialization
    static constexpr unsigned int kOutAPI =
        fnGetFFTSubframeOutAPI<kWindowAPI, TP_ORIG_PAR_POWER, TP_PARALLEL_POWER, TP_INDEX>(); // 0=win, 1=stream,
                                                                                              // 2=casc/strm,
                                                                                              // 3=strm/casc
    static constexpr unsigned int kNumOutPorts = (kOutAPI == 0) ? 1 : 2;

    port<input> in[1];
    port<output> out[kNumOutPorts];

    fft_ifft_dit_1ch_mono_ref_graph<TT_DATA,
                                    TT_TWIDDLE,
                                    TP_POINT_SIZE,
                                    TP_FFT_NIFFT,
                                    TP_SHIFT,
                                    TP_CASC_LEN,
                                    TP_DYN_PT_SIZE,
                                    TP_WINDOW_VSIZE,
                                    kWindowAPI,
                                    kOutAPI,
                                    TP_ORIG_PAR_POWER,
                                    TP_RND,
                                    TP_SAT,
                                    TP_TWIDDLE_MODE,
                                    TT_OUT_DATA>
        FFTwinproc;

    fft_ifft_dit_1ch_ref_graph() {
        connect<>(in[0], FFTwinproc.in[0]);
        // dimensions(FFTwinproc.in[0]) = {TP_WINDOW_VSIZE+kHeaderBytes/sizeof(TT_DATA)};
        if
            constexpr(kOutAPI == 0) { connect<>(FFTwinproc.out[0], out[0]); }
        else {
            connect<>(FFTwinproc.out[0], out[0]);
            connect<>(FFTwinproc.out[1], out[1]);
        }
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
          unsigned int TP_USE_WIDGETS,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_TWIDDLE_MODE,
          typename TT_OUT_DATA,
          unsigned int TP_INDEX,
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
                                 TP_USE_WIDGETS,
                                 TP_RND,
                                 TP_SAT,
                                 TP_TWIDDLE_MODE,
                                 TT_OUT_DATA,
                                 TP_INDEX,
                                 TP_ORIG_PAR_POWER> : public graph {
   public:
    static_assert(!(std::is_same<TT_DATA, cfloat>::value) || (TP_SHIFT == 0),
                  "ERROR: TP_SHIFT cannot be performed for TT_DATA=cfloat, so must be set to 0");

    static constexpr unsigned int TP_PARALLEL_POWER = 0; // for this specialization
    static constexpr unsigned int kOutAPI =
        fnGetFFTSubframeOutAPI<kStreamAPI, TP_ORIG_PAR_POWER, TP_PARALLEL_POWER, TP_INDEX>();
    static constexpr unsigned int kNumOutPorts = (kOutAPI == 0) ? 1 : 2;
    static constexpr int kStreamsPerTile = get_input_streams_core_module(); // a device trait

    port<input> in[kStreamsPerTile]; // dual streams
    port<output> out[kNumOutPorts];

    fft_ifft_dit_1ch_mono_ref_graph<TT_DATA,
                                    TT_TWIDDLE,
                                    TP_POINT_SIZE,
                                    TP_FFT_NIFFT,
                                    TP_SHIFT,
                                    TP_CASC_LEN,
                                    TP_DYN_PT_SIZE,
                                    TP_WINDOW_VSIZE,
                                    kStreamAPI,
                                    kOutAPI,
                                    TP_ORIG_PAR_POWER,
                                    TP_RND,
                                    TP_SAT,
                                    TP_TWIDDLE_MODE,
                                    TT_OUT_DATA>
        FFTstrproc;

    fft_ifft_dit_1ch_ref_graph() {
        for (int i = 0; i < kStreamsPerTile; i++) {
            connect<stream>(in[i], FFTstrproc.in[i]);
        }
        // for case that this is the top level, output ports according to kStreamsPerTile.
        if (TP_ORIG_PAR_POWER == 0) {
            for (int i = 0; i < kStreamsPerTile; i++) {
                connect<>(FFTstrproc.out[i], out[i]);
            }
        } else { // if not top level, these are trellis connections.
            for (int i = 0; i < kNumOutPorts; i++) {
                connect<>(FFTstrproc.out[i], out[i]);
            }
            /*      if (kOutAPI == kStreamAPI) {
              connect<stream>(FFTstrproc.out[0], out[0]);
              connect<stream>(FFTstrproc.out[1], out[1]);
            } else if (kOutAPI == kCascStreamAPI) {
              connect<cascade>(FFTstrproc.out[0], out[0]);
              connect<stream>(FFTstrproc.out[1], out[1]);
            } else if (kOutAPI == kStreamCascAPI) {
              connect<stream>(FFTstrproc.out[0], out[0]);
              connect<cascade>(FFTstrproc.out[1], out[1]);
              }*/
        }
    };
};
}
}
}
}
}
#endif // _DSPLIB_FFT_IFFT_DIT_1CH_REF_GRAPH_HPP_
