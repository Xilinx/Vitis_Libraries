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
#ifndef _DSPLIB_FFT_DIT_2CH_REAL_REF_GRAPH_HPP_
#define _DSPLIB_FFT_DIT_2CH_REAL_REF_GRAPH_HPP_
/*
The file captures the definition of the 'L2' graph level class for
the Widget 2-Channel Real FFT function library element.
*/
/**
 * @file fft_dit_2ch_real_graph.hpp
 *
 **/

#include <adf.h>
#include <vector>
#include <array>
#include <adf/arch/aie_arch_properties.hpp>

#include "widget_real2complex_ref_graph.hpp"
#include "widget_cut_deck_ref_graph.hpp"
#include "fft_ifft_dit_1ch_ref_graph.hpp"
#include "fir_ref_utils.hpp"

namespace xf {
namespace dsp {
namespace aie {
namespace fft_dit_2ch_real {
using namespace adf;

//--------------------------------------------------------------------------------------------------
// fft_dit_2ch_real_ref_graph template
//--------------------------------------------------------------------------------------------------
/**
 * @ingroup fft_dit_2ch_real
 * @brief This widget takes in 2 real signals (A and B) and converts them to a complex signal (A + jB)
 **/
template <typename TT_DATA,
          typename TT_TWIDDLE,
          unsigned int TP_POINT_SIZE,
          unsigned int TP_FFT_NIFFT = 1,
          unsigned int TP_SHIFT = 0,
          unsigned int TP_CASC_LEN = 1,
          unsigned int TP_WINDOW_VSIZE = TP_POINT_SIZE,
          unsigned int TP_API = 0,
          unsigned int TP_RND = 4,
          unsigned int TP_SAT = 1,
          unsigned int TP_TWIDDLE_MODE = 0,
          typename TT_OUT_DATA = TT_DATA>
class fft_dit_2ch_real_ref_graph : public graph {
   public:
    // ! Add in static asserts which enforce TP_FFT_NIFFT=1 and SSR=1.

    /**
     * The input data to the function.
     **/
    port<input> in[1];
    /**
     * The output data.
     **/
    port<output> out[1];

    widget::real2complex::widget_real2complex_ref_graph<realType_t<TT_DATA>, TT_DATA, TP_WINDOW_VSIZE * 2>
        widgetReal2Complex;
    widget::deck_cut::widget_cut_deck_ref_graph<TT_DATA, TP_POINT_SIZE, TP_WINDOW_VSIZE * 2, 1> deckCutEntryGraph;

    fft::dit_1ch::fft_ifft_dit_1ch_ref_graph<TT_DATA,
                                             TT_TWIDDLE,
                                             TP_POINT_SIZE,
                                             TP_FFT_NIFFT,
                                             TP_SHIFT,
                                             TP_CASC_LEN,
                                             0,
                                             TP_WINDOW_VSIZE * 2,
                                             TP_API,
                                             0,
                                             0,
                                             TP_RND,
                                             TP_SAT,
                                             TP_TWIDDLE_MODE,
                                             TT_OUT_DATA>
        fftGraph;

    widget::deck_cut::widget_cut_deck_ref_graph<TT_DATA, TP_POINT_SIZE, TP_WINDOW_VSIZE * 2, 0> deckCutExitGraph;

    /**
     * @brief This is the constructor function for the fft_dit_2ch_real_ref graph.
     **/
    fft_dit_2ch_real_ref_graph() {
        // make connections
        connect(in[0], widgetReal2Complex.in);
        dimensions(widgetReal2Complex.in) = {TP_WINDOW_VSIZE * 2};

        connect(widgetReal2Complex.out, deckCutEntryGraph.in);
        dimensions(widgetReal2Complex.out) = {TP_WINDOW_VSIZE * 2};
        dimensions(deckCutEntryGraph.in) = {TP_WINDOW_VSIZE * 2};

        connect(deckCutEntryGraph.out, fftGraph.in[0]);
        dimensions(deckCutEntryGraph.out) = {TP_WINDOW_VSIZE * 2};
        dimensions(fftGraph.in[0]) = {TP_WINDOW_VSIZE * 2};

        connect(fftGraph.out[0], deckCutExitGraph.in);
        dimensions(fftGraph.out[0]) = {TP_WINDOW_VSIZE * 2};
        dimensions(deckCutExitGraph.in) = {TP_WINDOW_VSIZE * 2};

        connect(deckCutExitGraph.out, out[0]);
        dimensions(deckCutExitGraph.out) = {TP_WINDOW_VSIZE};
    }; // constructor
};
}
}
}
} // namespace braces

#endif //_DSPLIB_FFT_DIT_2CH_REAL_GRAPH_HPP_
