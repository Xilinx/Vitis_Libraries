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

#ifndef _DSPLIB_FIR_DECOMPOSER_UTILS_HPP_
#define _DSPLIB_FIR_DECOMPOSER_UTILS_HPP_

// 4 different types of kernels that can be used.
#include "fir_sr_asym.hpp"
#include "fir_interpolate_asym.hpp"
#include "fir_decimate_asym.hpp"
#include "fir_resampler.hpp"

#include "fir_graph_utils.hpp"

namespace xf {
namespace dsp {
namespace aie {
namespace fir {
namespace decomposer {
template <typename params>
struct decomposed_params : params {
    // static_assert(params::BTP_INTERPOLATE_FACTOR % params::BTP_PARA_INTERP_POLY == 0 &&
    // params::BTP_INTERPOLATE_FACTOR >= params::BTP_PARA_INTERP_POLY, "Error");
    static constexpr unsigned int BTP_INTERPOLATE_FACTOR =
        params::BTP_INTERPOLATE_FACTOR / params::BTP_PARA_INTERP_POLY;
    static constexpr unsigned int BTP_PARA_INTERP_POLY = 1; // after decomposer, this is fully resolved.
    static constexpr unsigned int BTP_DECIMATE_FACTOR = params::BTP_DECIMATE_FACTOR / params::BTP_PARA_DECI_POLY;
    static constexpr unsigned int BTP_PARA_DECI_POLY = 1; // after decomposer, this is fully resolved.

    static constexpr unsigned int BTP_FIR_LEN = params::BTP_FIR_LEN / params::BTP_PARA_INTERP_POLY; // todo, decimator
};

struct err_type;
template <typename p>
using d_p = decomposed_params<p>; // short version

// Checks parameters after decomposition have fully decomposed or not.
template <typename p>
constexpr bool isSingleRate() {
    return (d_p<p>::BTP_INTERPOLATE_FACTOR == 1) && (d_p<p>::BTP_DECIMATE_FACTOR == 1);
}

template <typename p>
constexpr bool isInterpolator() {
    return (d_p<p>::BTP_INTERPOLATE_FACTOR > 1) && (d_p<p>::BTP_DECIMATE_FACTOR == 1);
}

template <typename p>
constexpr bool isDecimator() {
    return (d_p<p>::BTP_INTERPOLATE_FACTOR == 1) && (d_p<p>::BTP_DECIMATE_FACTOR > 1);
}

template <typename p>
constexpr bool isResampler() {
    return (d_p<p>::BTP_INTERPOLATE_FACTOR > 1) && (d_p<p>::BTP_DECIMATE_FACTOR > 1);
}

template <typename p>
using decomposed_kernel_tl =
    typename std::conditional_t<isSingleRate<p>(),
                                sr_asym::fir_sr_asym_tl<p>,
                                std::conditional_t<isInterpolator<p>(),
                                                   interpolate_asym::fir_interpolate_asym_tl<p>,
                                                   std::conditional_t<isDecimator<p>(),
                                                                      decimate_asym::fir_decimate_asym_tl<p>,
                                                                      // std::conditional_t<
                                                                      //  isResampler<p>(),
                                                                      //  resampler::fir_resampler_tl<res_p>,
                                                                      err_type // no supported architecture
                                                                               // >
                                                                      > > >;
// Is there a better way to do this? Maybe store it in params?
template <typename params>
constexpr const char* getSourceFile() {
    if (isSingleRate<params>()) {
        return "fir_sr_asym.cpp";
    }
    if (isInterpolator<params>()) {
        return "fir_interpolate_asym.cpp";
    }
    if (isDecimator<params>()) {
        return "fir_decimate_asym.cpp";
    }
    // if (isResampler<params>()){
    //  return "fir_resampler.cpp";
    //}
    return "unknown_kernel.cpp";
}

template <typename params>
class polyphase_decomposer {
   private:
    using p = params; // shortcut

    static constexpr unsigned int TP_CASC_LEN = p::BTP_CASC_LEN;
    static constexpr unsigned int TP_SSR = p::BTP_SSR;
    static constexpr unsigned int TP_PARA_INTERP_POLY = p::BTP_PARA_INTERP_POLY;
    static constexpr unsigned int TP_PARA_DECI_POLY = p::BTP_PARA_DECI_POLY;
    static constexpr unsigned int RTP_OUT = TP_SSR * TP_PARA_INTERP_POLY;
    static constexpr unsigned int SSR_OUT = TP_SSR * TP_PARA_INTERP_POLY;
    static constexpr unsigned int SSR_IN = TP_SSR * TP_PARA_DECI_POLY;
    using coeff_vector = std::vector<typename p::BTT_COEFF>;

    // Take every para_interp_poly tap to create a smaller vector for ssr kernels; when decimator take with
    // para_deci_poly
    static const coeff_vector segment_taps_array_for_polyphase(const coeff_vector& taps, unsigned int polyphaseIndex) {
        coeff_vector ret_taps;
        for (unsigned int i = polyphaseIndex; i < taps.size(); i += TP_PARA_INTERP_POLY) {
            ret_taps.push_back(taps[i]);
        }

        return ret_taps;
    }
    static constexpr unsigned int n_kernels = TP_CASC_LEN * TP_SSR * TP_SSR * TP_PARA_INTERP_POLY * TP_PARA_DECI_POLY;
    static constexpr unsigned int n_ssr_kernels = TP_CASC_LEN * TP_SSR * TP_SSR;

    // resolved parameters, potentially with some modification from decomposer
    using res_params = decomposed_params<params>;
    // ssr kernels at top dim, ready to kick off the recurse down.
    using ssrKickOff = ssr_kernels<res_params, decomposed_kernel_tl>;

    using casc_net_array = std::array<connect<stream, stream>*, TP_CASC_LEN>;
    using ssr_net_array = std::array<std::array<casc_net_array, TP_SSR>, TP_SSR>;
    // a 2-D array for decimator and interpolator polyphases.
    using polyphase_net_array = std::array<std::array<ssr_net_array, TP_PARA_DECI_POLY>, TP_PARA_INTERP_POLY>;

    // conditional type aliases
    using in2_type = port_conditional_array<input, (p::BTP_DUAL_IP == 1), SSR_IN>;
    using out2_type = port_conditional_array<output, (p::BTP_NUM_OUTPUTS == 2), SSR_OUT>;
    using out2_vtype = typename out2_type::value_type;
    using coeff_type = port_conditional_array<input, (p::BTP_USE_COEFF_RELOAD == 1), RTP_OUT>;
    using dec_coeff_type = port_conditional_array<input, (p::BTP_USE_COEFF_RELOAD == 1), TP_SSR>;
    using cond_casc_in_type = port_conditional_array<input, (p::BTP_CASC_IN == CASC_IN_TRUE), SSR_IN>;
    template <typename vtype>
    using ssr_array = std::array<vtype, TP_SSR>;

    template <typename vtype>
    using interp_poly_array = std::array<vtype, TP_PARA_INTERP_POLY>;

    static_assert((p::BTP_INTERPOLATE_FACTOR % TP_PARA_INTERP_POLY == 0) &&
                      (p::BTP_INTERPOLATE_FACTOR >= TP_PARA_INTERP_POLY),
                  "TP_PARA_INTERP_POLY must be an integer factor (greater than 0) of TP_INTERPOLATE_FACTOR");
    static_assert((p::BTP_DECIMATE_FACTOR % TP_PARA_DECI_POLY == 0) && (p::BTP_DECIMATE_FACTOR >= TP_PARA_DECI_POLY),
                  "TP_PARA_DECI_POLY must be an integer factor (greater than 0) of TP_DECIMATE_FACTOR");

    // consider padding coefficients to avoid this.
    static_assert(p::BTP_FIR_LEN % TP_PARA_INTERP_POLY == 0,
                  "TP_FIR_LEN must be a mutliple of TP_PARA_INTERP_POLY. Pad coefficients to nearest multiple.");
    static_assert(p::BTP_FIR_LEN % TP_PARA_DECI_POLY == 0,
                  "TP_FIR_LEN must be a mutliple of TP_PARA_DECI_POLY. Pad coefficients to nearest multiple.");

    // this constraint doesn't exist for INTERP_POLY, since input window is just broadcast and not split.
    static_assert(p::BTP_INPUT_WINDOW_VSIZE % TP_PARA_DECI_POLY == 0,
                  "TP_INPUT_WINDOW_VSIZE must be a multiple of TP_PARA_DECI_POLY");

   public:
    // static coeffs
    static void create(kernel (&firKernels)[n_kernels], const coeff_vector& taps) {
        // call ssr kernels multiple times.
        for (unsigned int interpPolyphaseIndex = 0; interpPolyphaseIndex < TP_PARA_INTERP_POLY;
             interpPolyphaseIndex++) {
            unsigned int kernelIndex = TP_CASC_LEN * TP_SSR * TP_SSR * interpPolyphaseIndex; // todo decimation
            auto tapsForPolyphase = segment_taps_array_for_polyphase(taps, interpPolyphaseIndex);
            // populates kernel array with kernel objects
            ssrKickOff::create_and_recurse(reinterpret_cast<kernel(&)[n_ssr_kernels]>(firKernels[kernelIndex]),
                                           tapsForPolyphase);
        }
    }
    // reloadable coeffs
    static void create(kernel (&firKernels)[n_kernels]) {
        // call ssr kernels multiple times.
        for (unsigned int interpPolyphaseIndex = 0; interpPolyphaseIndex < TP_PARA_INTERP_POLY;
             interpPolyphaseIndex++) {
            unsigned int kernelIndex = TP_CASC_LEN * TP_SSR * TP_SSR * interpPolyphaseIndex; // todo decimation
            // populates kernel array with kernel objects
            ssrKickOff::create_and_recurse(reinterpret_cast<kernel(&)[n_ssr_kernels]>(firKernels[kernelIndex]));
        }
    }

    // make connections
    static void create_connections(kernel* m_firKernels,
                                   port<input>* in,
                                   in2_type(&in2),
                                   port<output>* out,
                                   out2_type(&out2),
                                   coeff_type coeff,
                                   polyphase_net_array& net,
                                   polyphase_net_array& net2,
                                   cond_casc_in_type(&casc_in),
                                   const char* srcFileName = "fir_sr_asym.cpp") {
        interp_poly_array<ssr_array<port<output> > > out_ssr;
        interp_poly_array<ssr_array<out2_vtype> > out2_ssr;
        for (unsigned int interpPolyIdx = 0; interpPolyIdx < TP_PARA_INTERP_POLY; interpPolyIdx++) {
            dec_coeff_type decomposed_coeff;
            unsigned int kernelIndex = TP_CASC_LEN * TP_SSR * TP_SSR * interpPolyIdx; // todo decimation
            for (unsigned int ssrIdx = 0; ssrIdx < TP_SSR; ssrIdx++) {
                unsigned int outputIndex = ssrIdx * TP_PARA_INTERP_POLY + interpPolyIdx;
                connect<>(out_ssr[interpPolyIdx][ssrIdx], out[outputIndex]);
                if
                    constexpr(params::BTP_NUM_OUTPUTS == 2) {
                        connect<>(out2_ssr[interpPolyIdx][ssrIdx], out2[outputIndex]);
                    }
                if
                    constexpr(params::BTP_USE_COEFF_RELOAD == 1) {
                        connect<>(coeff[TP_SSR * interpPolyIdx + ssrIdx], decomposed_coeff[ssrIdx]);
                    }
            }

            // interp branches simply broadcast any input ports, but do have seperate output ports
            // Need to have nets being independant because you can imagine a scenario where a parallel interp branches
            // input needs additional
            // fifo skew due to timing skew across device.
            ssrKickOff::create_connections(&m_firKernels[kernelIndex], &in[0], in2, &out_ssr[interpPolyIdx][0],
                                           out2_ssr[interpPolyIdx], decomposed_coeff, net[interpPolyIdx][0],
                                           net2[interpPolyIdx][0], casc_in, srcFileName);
        }
    }
};
}
}
}
}
} // namespace braces
#endif //_DSPLIB_FIR_DECOMPOSER_UTILS_HPP_