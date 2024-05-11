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
    static constexpr unsigned int BTP_INTERPOLATE_FACTOR =
        params::BTP_INTERPOLATE_FACTOR / params::BTP_PARA_INTERP_POLY;
    static constexpr unsigned int BTP_DECIMATE_FACTOR = params::BTP_DECIMATE_FACTOR / params::BTP_PARA_DECI_POLY;
    // static constexpr unsigned int BTP_PARA_INTERP_POLY = 1; // after decomposer, this is fully resolved.
    // static constexpr unsigned int BTP_PARA_DECI_POLY = 1; // after decomposer, this is fully resolved.

    static constexpr unsigned int BTP_FIR_LEN =
        params::BTP_FIR_LEN / (params::BTP_PARA_INTERP_POLY * params::BTP_PARA_DECI_POLY);
    static constexpr unsigned int BTP_FIR_RANGE_LEN = BTP_FIR_LEN;
};

struct err_type;

// Checks parameters after decomposition have fully decomposed or not.
template <typename p>
constexpr bool isSingleRate() {
    return (p::BTP_INTERPOLATE_FACTOR == 1) && (p::BTP_DECIMATE_FACTOR == 1);
}

template <typename p>
constexpr bool isInterpolator() {
    return (p::BTP_INTERPOLATE_FACTOR > 1) && (p::BTP_DECIMATE_FACTOR == 1);
}

template <typename p>
constexpr bool isDecimator() {
    return (p::BTP_INTERPOLATE_FACTOR == 1) && (p::BTP_DECIMATE_FACTOR > 1);
}

template <typename p>
constexpr bool isResampler() {
    return (p::BTP_INTERPOLATE_FACTOR > 1) && (p::BTP_DECIMATE_FACTOR > 1);
}

template <typename p>
using decomposed_kernel_tl = typename std::conditional_t<
    isSingleRate<p>(),
    sr_asym::fir_sr_asym_tl<p>,
    std::conditional_t<isInterpolator<p>(),
                       interpolate_asym::fir_interpolate_asym_tl<p>,
                       std::conditional_t<isDecimator<p>(),
                                          decimate_asym::fir_decimate_asym_tl<p>,
                                          std::conditional_t<isResampler<p>(),
                                                             resampler::fir_resampler_tl<p>,
                                                             err_type // no supported architecture
                                                             > > > >;
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
    if (isResampler<params>()) {
        return "fir_resampler.cpp";
    }
    return "unknown_kernel.cpp";
}

template <typename params>
class polyphase_decomposer {
   private:
    using p = params; // shortcut

    static constexpr unsigned int TP_NUM_OUTPUTS = p::BTP_NUM_OUTPUTS;
    static constexpr unsigned int TP_CASC_LEN = p::BTP_CASC_LEN;
    static constexpr unsigned int TP_SSR = p::BTP_SSR;
    static constexpr unsigned int TP_INTERPOLATE_FACTOR = p::BTP_INTERPOLATE_FACTOR;
    static constexpr unsigned int TP_DECIMATE_FACTOR = p::BTP_DECIMATE_FACTOR;
    static constexpr unsigned int TP_PARA_INTERP_POLY = p::BTP_PARA_INTERP_POLY;
    static constexpr unsigned int TP_PARA_DECI_POLY = p::BTP_PARA_DECI_POLY;
    static constexpr unsigned int RTP_OUT = TP_SSR * TP_PARA_INTERP_POLY;
    static constexpr unsigned int SSR_OUT = TP_SSR * TP_PARA_INTERP_POLY;
    static constexpr unsigned int SSR_IN = TP_SSR * TP_PARA_DECI_POLY;
    using coeff_vector = std::vector<typename p::BTT_COEFF>;

    // Take every para_interp_poly tap to create a smaller vector for ssr kernels; when decimator take with
    // para_deci_poly
    static const coeff_vector segment_taps_array_for_polyphase(const coeff_vector& taps,
                                                               unsigned int polyInterpIdx,
                                                               unsigned int polyDeciIdx = 0) {
        // Split first in interpolate polyphase
        coeff_vector interp_taps;
        for (unsigned int i = (polyInterpIdx * TP_DECIMATE_FACTOR) % TP_INTERPOLATE_FACTOR; i < taps.size();
             i += TP_PARA_INTERP_POLY) {
            interp_taps.push_back(taps[i]);
        }

        // Then split the given interp polyphase over decimator phases / decimator moments
        coeff_vector ret_taps;
        for (unsigned int i = polyDeciIdx; i < interp_taps.size(); i += TP_PARA_DECI_POLY) {
            ret_taps.push_back(interp_taps[i]);
        }

        return ret_taps;
    }
    static constexpr unsigned int n_ssr_kernels = TP_CASC_LEN * TP_SSR * TP_SSR;
    static constexpr unsigned int n_kernels = n_ssr_kernels * TP_PARA_INTERP_POLY * TP_PARA_DECI_POLY;

    // resolved parameters
    using res_params = decomposed_params<params>;
    static constexpr const char* resSrcFile = getSourceFile<res_params>();

    // overwrite cascade parameters for the different decimator scenarios
    template <unsigned int interpPolyIdx, unsigned int deciPolyIdx>
    struct initDeciParams : public res_params {
        // Condition here if this is the one and only instance, otherwise there's a downstream casc connection
        static constexpr bool BTP_CASC_OUT = (TP_PARA_DECI_POLY == 1) ? res_params::BTP_CASC_OUT : true;
        static constexpr unsigned int BTP_PARA_INTERP_INDEX = interpPolyIdx;
        static constexpr unsigned int BTP_PARA_DECI_INDEX = deciPolyIdx;
    };
    template <unsigned int interpPolyIdx, unsigned int deciPolyIdx>
    struct midDeciParams : public res_params {
        static constexpr bool BTP_CASC_IN = true;
        static constexpr bool BTP_CASC_OUT = true;
        static constexpr int BTP_MODIFY_MARGIN_OFFSET = 0;
        static constexpr unsigned int BTP_NUM_OUTPUTS = 1; // force num output to one becuase using the cascade out
        static constexpr unsigned int BTP_PARA_INTERP_INDEX = interpPolyIdx;
        static constexpr unsigned int BTP_PARA_DECI_INDEX = deciPolyIdx;
    };

    template <unsigned int interpPolyIdx, unsigned int deciPolyIdx>
    struct lastDeciParams : public res_params {
        static constexpr bool BTP_CASC_IN = true;
        static constexpr int BTP_MODIFY_MARGIN_OFFSET = 0;
        // static constexpr int BTP_MODIFY_MARGIN_OFFSET = -1;
        static constexpr unsigned int BTP_PARA_INTERP_INDEX = interpPolyIdx;
        static constexpr unsigned int BTP_PARA_DECI_INDEX = deciPolyIdx;
    };
    // ssr kernels at top dim, ready to kick off the recurse down.
    template <unsigned int interpPolyIdx, unsigned int deciPolyIdx>
    using ssrKickOffInit = ssr_kernels<initDeciParams<interpPolyIdx, deciPolyIdx>, decomposed_kernel_tl>;
    template <unsigned int interpPolyIdx, unsigned int deciPolyIdx>
    using ssrKickOffMid = ssr_kernels<midDeciParams<interpPolyIdx, deciPolyIdx>, decomposed_kernel_tl>;
    template <unsigned int interpPolyIdx, unsigned int deciPolyIdx>
    using ssrKickOffLast = ssr_kernels<lastDeciParams<interpPolyIdx, deciPolyIdx>, decomposed_kernel_tl>;

    using casc_net_array = std::array<connect<stream, stream>*, TP_CASC_LEN>;
    using ssr_net_array = std::array<std::array<casc_net_array, TP_SSR>, TP_SSR>;
    // a 2-D array for decimator and interpolator polyphases.
    using polyphase_net_array = std::array<std::array<ssr_net_array, TP_PARA_DECI_POLY>, TP_PARA_INTERP_POLY>;

    // conditional type aliases
    using in2_type = port_conditional_array<input, (p::BTP_DUAL_IP == 1), SSR_IN>;
    using in2_vtype = typename in2_type::value_type;
    using out2_type = port_conditional_array<output, (p::BTP_NUM_OUTPUTS == 2), SSR_OUT>;
    using out2_vtype = typename out2_type::value_type;
    using coeff_type = port_conditional_array<input, (p::BTP_USE_COEFF_RELOAD == 1), RTP_OUT>;
    using dec_coeff_type = port_conditional_array<input, (p::BTP_USE_COEFF_RELOAD == 1), TP_SSR>; // TBC and tested
    using coeff_vtype = typename coeff_type::value_type;
    using cond_casc_in_type = port_conditional_array<input, (p::BTP_CASC_IN == CASC_IN_TRUE), TP_SSR>;
    template <typename vtype>
    using ssr_array = std::array<vtype, TP_SSR>;

    template <typename vtype>
    using interp_poly_array = std::array<vtype, TP_PARA_INTERP_POLY>;
    template <typename vtype>
    using deci_poly_array = std::array<vtype, TP_PARA_DECI_POLY>;

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
    static_assert(p::BTP_FIR_LEN % (TP_PARA_DECI_POLY * TP_PARA_INTERP_POLY) == 0,
                  "TP_FIR_LEN must be a mutliple of TP_PARA_DECI_POLY. Pad coefficients to nearest multiple.");

    // Flattened order with deciPoly being the inner dim - arbitrary choice,
    // hopefully consitent with other ports and net definitions
    static unsigned int getKernelIdx(unsigned int interpPolyIdx, unsigned int deciPolyIdx = 0) {
        return (n_ssr_kernels * TP_PARA_DECI_POLY * interpPolyIdx) + (n_ssr_kernels * deciPolyIdx);
    }

    // static coeffs
    template <unsigned int interpPolyIdx, unsigned int deciPolyIdx>
    static void create_and_recurse(kernel (&firKernels)[n_kernels], const coeff_vector& taps) {
        unsigned int kernelIndex = getKernelIdx(interpPolyIdx, deciPolyIdx);
        auto tapsForPolyphase = segment_taps_array_for_polyphase(taps, interpPolyIdx, deciPolyIdx);

        if (deciPolyIdx == 0) {
            // populates kernel array with kernel objects
            ssrKickOffInit<interpPolyIdx, deciPolyIdx>::create_and_recurse(
                reinterpret_cast<kernel(&)[n_ssr_kernels]>(firKernels[kernelIndex]), tapsForPolyphase);
        } else if (deciPolyIdx >= 1 && deciPolyIdx < (TP_PARA_DECI_POLY - 1)) {
            if
                constexpr(TP_PARA_DECI_POLY > 2) {
                    // populates kernel array with kernel objects

                    // printParams<midDeciParams<interpPolyIdx, deciPolyIdx>>();
                    ssrKickOffMid<interpPolyIdx, deciPolyIdx>::create_and_recurse(
                        reinterpret_cast<kernel(&)[n_ssr_kernels]>(firKernels[kernelIndex]), tapsForPolyphase);
                }
        } else {
            if
                constexpr(TP_PARA_DECI_POLY > 1) {
                    // printParams<lastDeciParams<interpPolyIdx, deciPolyIdx>>();
                    // populates kernel array with kernel objects
                    ssrKickOffLast<interpPolyIdx, deciPolyIdx>::create_and_recurse(
                        reinterpret_cast<kernel(&)[n_ssr_kernels]>(firKernels[kernelIndex]), tapsForPolyphase);
                }
        }
        if
            constexpr(interpPolyIdx == 0 && deciPolyIdx == 0) {
                // break recursive call
            }
        else {
            constexpr unsigned int nextInterpPolyIdx = deciPolyIdx == 0 ? interpPolyIdx - 1 : interpPolyIdx;
            constexpr unsigned int nextDeciPolyIdx = deciPolyIdx == 0 ? TP_PARA_DECI_POLY - 1 : deciPolyIdx - 1;
            create_and_recurse<nextInterpPolyIdx, nextDeciPolyIdx>(firKernels, taps);
        }
    }

    // static coeffs
    template <unsigned int interpPolyIdx, unsigned int deciPolyIdx>
    static void create_and_recurse(kernel (&firKernels)[n_kernels]) {
        unsigned int kernelIndex = getKernelIdx(interpPolyIdx, deciPolyIdx);

        if (deciPolyIdx == 0) {
            // populates kernel array with kernel objects
            ssrKickOffInit<interpPolyIdx, deciPolyIdx>::create_and_recurse(
                reinterpret_cast<kernel(&)[n_ssr_kernels]>(firKernels[kernelIndex]));
        } else if (deciPolyIdx >= 1 && deciPolyIdx < (TP_PARA_DECI_POLY - 1)) {
            if
                constexpr(TP_PARA_DECI_POLY > 2) {
                    // populates kernel array with kernel objects
                    ssrKickOffMid<interpPolyIdx, deciPolyIdx>::create_and_recurse(
                        reinterpret_cast<kernel(&)[n_ssr_kernels]>(firKernels[kernelIndex]));
                }
        } else {
            if
                constexpr(TP_PARA_DECI_POLY > 1) {
                    // populates kernel array with kernel objects
                    ssrKickOffLast<interpPolyIdx, deciPolyIdx>::create_and_recurse(
                        reinterpret_cast<kernel(&)[n_ssr_kernels]>(firKernels[kernelIndex]));
                }
        }

        if
            constexpr(interpPolyIdx == 0 && deciPolyIdx == 0) {
                // break recursive call
            }
        else {
            constexpr unsigned int nextInterpPolyIdx = deciPolyIdx == 0 ? interpPolyIdx - 1 : interpPolyIdx;
            constexpr unsigned int nextDeciPolyIdx = deciPolyIdx == 0 ? TP_PARA_DECI_POLY - 1 : deciPolyIdx - 1;
            create_and_recurse<nextInterpPolyIdx, nextDeciPolyIdx>(firKernels);
        }
    }

    // static coeffs
    template <unsigned int interpPolyIdx, unsigned int deciPolyIdx>
    static void create_connections_and_recurse(
        kernel* firKernels,
        interp_poly_array<ssr_array<coeff_vtype> > decomposed_coeff,
        interp_poly_array<ssr_array<port<output> > >& out_ssr,
        interp_poly_array<deci_poly_array<ssr_array<out2_vtype> > >& out2_ssr,
        interp_poly_array<deci_poly_array<ssr_array<port<input> > > > in_ssr,
        interp_poly_array<deci_poly_array<ssr_array<in2_vtype> > > in2_ssr,
        interp_poly_array<deci_poly_array<ssr_array<port<output> > > > out_deci_intermediate,
        interp_poly_array<deci_poly_array<ssr_array<port<input> > > > casc_in_deci_intermediate,
        polyphase_net_array& net,
        polyphase_net_array& net2,
        cond_casc_in_type(&casc_in)) {
        unsigned int kernelIndex = getKernelIdx(interpPolyIdx, deciPolyIdx);

        // Need to call three different variants of ssr_kernels to handle intermediate connections on polyphase
        // decimator
        if (deciPolyIdx == 0) {
            // if we have dual output and non DECI_POLY usecase, then out2 connection will be made - otherwise it's
            // no_port because only cascade is used.
            port_conditional_array<output, (TP_NUM_OUTPUTS == 2), TP_SSR> out2_deci_initial;
            if
                constexpr(TP_PARA_DECI_POLY == 1 && params::BTP_NUM_OUTPUTS == 2) {
                    for (unsigned int ssrIdx = 0; ssrIdx < TP_SSR; ssrIdx++) {
                        connect<>(out2_deci_initial[ssrIdx], out2_ssr[interpPolyIdx][deciPolyIdx][ssrIdx]);
                    }
                }

            // initialDecimatorPolyphase, which might also be the last if TP_PARA_DECI_POLY==1
            ssrKickOffInit<interpPolyIdx, deciPolyIdx>::create_connections(
                &firKernels[kernelIndex], &in_ssr[interpPolyIdx][deciPolyIdx][0], in2_ssr[interpPolyIdx][deciPolyIdx],
                &out_deci_intermediate[interpPolyIdx][deciPolyIdx][0], out2_deci_initial,
                decomposed_coeff[interpPolyIdx], net[interpPolyIdx][deciPolyIdx], net2[interpPolyIdx][deciPolyIdx],
                casc_in, resSrcFile);

        } else if (deciPolyIdx >= 1 && deciPolyIdx < (TP_PARA_DECI_POLY - 1)) {
            // middle decimator polyphases
            if
                constexpr(TP_PARA_DECI_POLY > 2) {
                    ssr_array<no_port> out2_no_port; // never has a out2 output, always cascaded
                    ssrKickOffMid<interpPolyIdx, deciPolyIdx>::create_connections(
                        &firKernels[kernelIndex], &in_ssr[interpPolyIdx][deciPolyIdx][0],
                        in2_ssr[interpPolyIdx][deciPolyIdx], &out_deci_intermediate[interpPolyIdx][deciPolyIdx][0],
                        out2_no_port, decomposed_coeff[interpPolyIdx], net[interpPolyIdx][deciPolyIdx],
                        net2[interpPolyIdx][deciPolyIdx], casc_in_deci_intermediate[interpPolyIdx][deciPolyIdx - 1],
                        resSrcFile);
                }

        } else {
            // last decimator polyphase only if paraDeciPoly > 1
            if
                constexpr(TP_PARA_DECI_POLY > 1) {
                    ssrKickOffLast<interpPolyIdx, deciPolyIdx>::create_connections(
                        &firKernels[kernelIndex], &in_ssr[interpPolyIdx][deciPolyIdx][0],
                        in2_ssr[interpPolyIdx][deciPolyIdx], &out_deci_intermediate[interpPolyIdx][deciPolyIdx][0],
                        out2_ssr[interpPolyIdx][deciPolyIdx], decomposed_coeff[interpPolyIdx],
                        net[interpPolyIdx][deciPolyIdx], net2[interpPolyIdx][deciPolyIdx],
                        casc_in_deci_intermediate[interpPolyIdx][deciPolyIdx - 1], resSrcFile);
                }
        }
        if
            constexpr(interpPolyIdx == 0 && deciPolyIdx == 0) {
                // break recursive call
            }
        else {
            constexpr unsigned int nextInterpPolyIdx = deciPolyIdx == 0 ? interpPolyIdx - 1 : interpPolyIdx;
            constexpr unsigned int nextDeciPolyIdx = deciPolyIdx == 0 ? TP_PARA_DECI_POLY - 1 : deciPolyIdx - 1;
            create_connections_and_recurse<nextInterpPolyIdx, nextDeciPolyIdx>(
                firKernels, decomposed_coeff, out_ssr, out2_ssr, in_ssr, in2_ssr, out_deci_intermediate,
                casc_in_deci_intermediate, net, net2, casc_in);
        }
    }

   public:
    // static coeffs
    static void create(kernel (&firKernels)[n_kernels], const coeff_vector& taps) {
        create_and_recurse<TP_PARA_INTERP_POLY - 1, TP_PARA_DECI_POLY - 1>(firKernels, taps);
    }
    // reloadable coeffs
    static void create(kernel (&firKernels)[n_kernels]) {
        create_and_recurse<TP_PARA_INTERP_POLY - 1, TP_PARA_DECI_POLY - 1>(firKernels);
    }

    // make connections
    static void create_connections(kernel* firKernels,
                                   port<input>* in,
                                   in2_type(&in2),
                                   port<output>* out,
                                   out2_type(&out2),
                                   coeff_type coeff,
                                   polyphase_net_array& net,
                                   polyphase_net_array& net2,
                                   cond_casc_in_type(&casc_in),
                                   const char* srcFileName = "fir_sr_asym.cpp") {
        interp_poly_array<ssr_array<coeff_vtype> > decomposed_coeff;
        interp_poly_array<ssr_array<port<output> > > out_ssr;
        interp_poly_array<deci_poly_array<ssr_array<out2_vtype> > > out2_ssr;
        interp_poly_array<deci_poly_array<ssr_array<port<input> > > > in_ssr;
        interp_poly_array<deci_poly_array<ssr_array<in2_vtype> > > in2_ssr;
        interp_poly_array<deci_poly_array<ssr_array<port<output> > > > out_deci_intermediate;
        interp_poly_array<deci_poly_array<ssr_array<port<input> > > > casc_in_deci_intermediate;
        create_connections_and_recurse<TP_PARA_INTERP_POLY - 1, TP_PARA_DECI_POLY - 1>(
            firKernels, decomposed_coeff, out_ssr, out2_ssr, in_ssr, in2_ssr, out_deci_intermediate,
            casc_in_deci_intermediate, net, net2, casc_in);

        for (unsigned int interpPolyIdx = 0; interpPolyIdx < TP_PARA_INTERP_POLY; interpPolyIdx++) {
            for (unsigned int deciPolyIdx = 0; deciPolyIdx < TP_PARA_DECI_POLY; deciPolyIdx++) {
                // unsigned int kernelIndex = getKernelIdx(interpPolyIdx, deciPolyIdx);
                // Connect different polyphase inputs to each decimator polyphase branch
                for (unsigned int ssrIdx = 0; ssrIdx < TP_SSR; ssrIdx++) {
                    // Data goes in reverse direction vs taps polyphases. for para_deci_poly=3:
                    // data0 -> poly0, data2 -> poly3, data1 -> poly2
                    unsigned int inputDataIndex =
                        (ssrIdx * TP_PARA_DECI_POLY +
                         (TP_PARA_DECI_POLY - deciPolyIdx + ((0 * TP_DECIMATE_FACTOR) / TP_INTERPOLATE_FACTOR)) %
                             (TP_PARA_DECI_POLY) +
                         (interpPolyIdx * TP_DECIMATE_FACTOR) / TP_INTERPOLATE_FACTOR) %
                        (TP_PARA_DECI_POLY * TP_SSR);
                    unsigned int inputDataIndexOld = ssrIdx * TP_PARA_DECI_POLY +
                                                     (TP_PARA_DECI_POLY - deciPolyIdx +
                                                      ((interpPolyIdx * TP_DECIMATE_FACTOR) / TP_INTERPOLATE_FACTOR)) %
                                                         (TP_PARA_DECI_POLY);
                    connect<>(in[inputDataIndex], in_ssr[interpPolyIdx][deciPolyIdx][ssrIdx]);
                    if
                        constexpr(params::BTP_DUAL_IP == 1) {
                            connect<>(in2[inputDataIndex], in2_ssr[interpPolyIdx][deciPolyIdx][ssrIdx]);
                        }
                    if
                        constexpr((params::BTP_USE_COEFF_RELOAD == 1)) {
                            if (deciPolyIdx == 0) {
                                connect<>(coeff[TP_SSR * interpPolyIdx + ssrIdx],
                                          decomposed_coeff[interpPolyIdx][ssrIdx]);
                            }
                        }
                }

                // Intermediate (between decimator polyphases) cascade output connections to next polyphase casc_in
                // connection (which only exists if para_deci_poly>1)
                if
                    constexpr(TP_PARA_DECI_POLY > 1) {
                        if (deciPolyIdx != (TP_PARA_DECI_POLY - 1)) {
                            for (unsigned int ssrIdx = 0; ssrIdx < TP_SSR; ssrIdx++) {
                                connect<>(out_deci_intermediate[interpPolyIdx][deciPolyIdx][ssrIdx],
                                          casc_in_deci_intermediate[interpPolyIdx][deciPolyIdx][ssrIdx]);
                            }
                        }
                    }
            }

            for (unsigned int ssrIdx = 0; ssrIdx < TP_SSR; ssrIdx++) {
                // Final decimator polyphase output connection to actual output interp polyphase connection
                connect<>(out_deci_intermediate[interpPolyIdx][(TP_PARA_DECI_POLY - 1)][ssrIdx],
                          out_ssr[interpPolyIdx][ssrIdx]);

                // connect interp polyphase outputs to flattened graph output.
                unsigned int outputIndex = ssrIdx * TP_PARA_INTERP_POLY + interpPolyIdx;
                connect<>(out_ssr[interpPolyIdx][ssrIdx], out[outputIndex]);
                if
                    constexpr(params::BTP_NUM_OUTPUTS == 2) {
                        connect<>(out2_ssr[interpPolyIdx][TP_PARA_DECI_POLY - 1][ssrIdx], out2[outputIndex]);
                    }
            }
        }
    }
};
}
}
}
}
} // namespace braces
#endif //_DSPLIB_FIR_DECOMPOSER_UTILS_HPP_