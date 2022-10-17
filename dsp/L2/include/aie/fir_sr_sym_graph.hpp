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
#ifndef _DSPLIB_FIR_SR_SYM_GRAPH_HPP_
#define _DSPLIB_FIR_SR_SYM_GRAPH_HPP_
/*
The file captures the definition of the 'L2' graph level class for
the Single Rate Symmetrical FIR library element.
*/
/**
 * @file fir_sr_sym_graph.hpp
 *
 **/

#include <adf.h>
#include <vector>
#include "fir_graph_utils.hpp"
#include "fir_sr_sym.hpp"
#include "fir_sr_asym.hpp"
#include "widget_api_cast.hpp"
#include "fir_common_traits.hpp"
#include "fir_sr_asym_graph.hpp"
using namespace adf;
using namespace xf::dsp::aie::widget::api_cast;

namespace xf {
namespace dsp {
namespace aie {
namespace fir {
namespace sr_sym {

/**
 * @endcond
 */

//--------------------------------------------------------------------------------------------------
// fir_sr_sym_graph template
//--------------------------------------------------------------------------------------------------
/**
 * @brief fir_sr_sym is a Symmetrical Single Rate FIR filter
 *
 * @ingroup fir_graphs
 *
 * These are the templates to configure the Symmetric Single Rate FIR class.
 * @tparam TT_DATA describes the type of individual data samples input to and
 *         output from the filter function. This is a typename and must be one
 *         of the following: \n
 *         int16, cint16, int32, cint32, float, cfloat.
 * @tparam TT_COEFF describes the type of individual coefficients of the filter
 *         taps. \n It must be one of the same set of types listed for TT_DATA
 *         and must also satisfy the following rules:
 *         - Complex types are only supported when TT_DATA is also complex.
 *         - 32 bit types are only supported when TT_DATA is also a 32 bit type,
 *         - TT_COEFF must be an integer type if TT_DATA is an integer type
 *         - TT_COEFF must be a float type if TT_DATA is a float type.
 * @tparam TP_FIR_LEN is an unsigned integer which describes the number of taps
 *         in the filter.
 * @tparam TP_SHIFT describes power of 2 shift down applied to the accumulation of
 *         FIR terms before output. \n TP_SHIFT must be in the range 0 to 61.
 * @tparam TP_RND describes the selection of rounding to be applied during the
 *         shift down stage of processing. TP_RND must be in the range 0 to 7
 *         where
 *         - 0 = floor (truncate) eg. 3.8 Would become 3.
 *         - 1 = ceiling e.g. 3.2 would become 4.
 *         - 2 = round to positive infinity.
 *         - 3 = round to negative infinity.
 *         - 4 = round symmetrical to infinity.
 *         - 5 = round symmetrical to zero.
 *         - 6 = round convergent to even.
 *         - 7 = round convergent to odd. \n
 *         Modes 2 to 7 round to the nearest integer. They differ only in how
 *         they round for values of 0.5.
 * @tparam TP_INPUT_WINDOW_VSIZE describes the number of samples processed by the graph
 *         in a single iteration run.  \n
 *         When TP_API is set to 0, samples are buffered and stored in a ping-pong window buffer mapped onto Memory
 *Group banks. \n
 *         As a results, maximum number of samples processed by the graph is limited by the size of Memory Group. \n
 *         When TP_API is set to 1 and TP_SSR is set to 1, incoming samples are buffered in a similar manner.  \n
 *         When TP_API is set to 1 and TP_SSR > 1, samples are processed directly from the stream inputs and no
 *buffering takes place. \n
 *         In such case, maximum number of samples processed by the graph is limited to 32-bit value (4.294B samples per
 *iteration).  \n
 *         Note: For SSR configurations (TP_SSR>1), the input data must be split over multiple ports,
 *         where each successive sample is sent to a different input port in a round-robin fashion. \n
 *         As a results, each SSR input path will process a fraction of the frame defined by the TP_INPUT_WINDOW_VSIZE.
 *\n
 *         The number of values in the output window will be TP_INPUT_WINDOW_VSIZE
 *         also by virtue the single rate nature of this function. \n
 *         Note: Margin size should not be included in TP_INPUT_WINDOW_VSIZE.
 * @tparam TP_CASC_LEN describes the number of AIE processors to split the operation
 *         over. \n This allows resource to be traded for higher performance.
 *         TP_CASC_LEN must be in the range 1 (default) to 9.
 * @tparam TP_USE_COEFF_RELOAD allows the user to select if runtime coefficient
 *         reloading should be used. \n When defining the parameter:
 *         - 0 = static coefficients, defined in filter constructor,
 *         - 1 = reloadable coefficients, passed as argument to runtime function. \n
 *
 *         Note: when used, optional port: ``` port_conditional_array<input, (TP_USE_COEFF_RELOAD == 1), TP_SSR> coeff;
 *``` will be added to the FIR. \n
 *         Note: the size of the port array is equal to the total number of output paths  (TP_SSR).  \n
 *         Each port should contain the same taps array content, i.e. each additional port must be a duplicate of the
 *coefficient array. \n
 * @tparam TP_NUM_OUTPUTS sets the number of ports over which the output is sent. \n
 *         This can be 1 or 2. It is set to 1 by default. \n
 *         Depending on TP_API, additional output ports functionality differs.
 *         For Windows API, additional output provides flexibility in connecting
 *         FIR output with multiple destinations.
 *         Additional output ``out2`` is an exact copy of the data of the output port ``out``. \n
 *
 *         With Stream API, the additional output port increases the FIR's throughput. \n
 *         Data is sent in a 128-bit interleaved pattern, e.g. : \n
 *         - samples 0-3 is sent over stream0 for cint16 data type, \n
 *         - samples 4-7 is sent over stream1 for cint16 data type. \n
 *
 *         Note: when used, optional port: ``` port<output> out2; ``` will be added to the FIR. \n
 * @tparam TP_API specifies if the output interface should be window-based or stream-based.  \n
 *         The values supported are 0 (window API) or 1 (stream API). \n
 *         Note: due to the data buffering requirement imposed through symmetry, input interface is always set to
 *window. \n
 *         Auto-infeffed DMA stream-to-window conversion is applied when FIR is connected with an input stream. \n
 * @tparam TP_SSR specifies the number of parallel input/output paths where samples are interleaved between paths,
 *         giving an overall higher throughput.   \n
 *         A TP_SSR of 1 means just one output leg and 1 input phase, and is the backwards compatible option. \n
 *         The number of AIEs used is given by ``TP_SSR^2 * TP_CASC_LEN``. \n
 **/
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_CASC_LEN = 1,
          unsigned int TP_DUAL_IP = 0,
          unsigned int TP_USE_COEFF_RELOAD = 0, // 1 = use coeff reload, 0 = don't use coeff reload
          unsigned int TP_NUM_OUTPUTS = 1,
          unsigned int TP_API = 0,
          unsigned int TP_SSR = 1>
class fir_sr_sym_graph : public graph {
   private:
    static_assert(TP_CASC_LEN <= 40, "ERROR: Unsupported Cascade length");

    static_assert(TP_INPUT_WINDOW_VSIZE % TP_SSR == 0,
                  "ERROR: Unsupported frame size. TP_INPUT_WINDOW_VSIZE must be divisible by TP_SSR");
    static constexpr unsigned int TP_CASC_IN = false;
    static constexpr unsigned int CASC_IN_PORT_POS = 1;

    static constexpr unsigned int kMaxFloatTaps = 256;
    static constexpr unsigned int kMaxIntTaps = 512;
    static constexpr unsigned int kMaxTapsPerKernel =
        (std::is_same<TT_DATA, cfloat>::value || std::is_same<TT_DATA, cfloat>::value) ? kMaxFloatTaps : kMaxIntTaps;

    static_assert(TP_SSR >= 1, "ERROR: TP_SSR must be 1 or higher");

    // Limit FIR length per kernel. Longer FIRs may exceed Program Memory and/or system memory combined with window
    // buffers may exceed Memory Module size.
    static_assert(TP_FIR_LEN / TP_SSR / TP_CASC_LEN <= kMaxTapsPerKernel,
                  "ERROR: Requested FIR length and Cascade length exceeds supported number of taps per kernel. Please "
                  "increase the cascade legnth to accomodate the FIR design.");
    // static_assert(!(TP_API == 0 && TP_DUAL_IP == 1),"ERROR: DUUAL_IP is only supported for streaming
    // implementations");
    // Limit FIR length for reloadable coeffs. Reloadable coeffs need a storage space that contibutes to system memory
    // exceeding Memory Module size.
    static_assert(TP_USE_COEFF_RELOAD == 0 || TP_FIR_LEN <= kMaxTapsPerKernel,
                  "ERROR: Exceeded maximum supported FIR length with reloadable coefficients. Please limit the FIR "
                  "length or disable coefficient reload.");

    static constexpr unsigned int kMemoryModuleSize = 32768;
    static constexpr unsigned int bufferSize = ((TP_FIR_LEN + TP_INPUT_WINDOW_VSIZE) * sizeof(TT_DATA));
    // Requested Window buffer exceeds memory module size
    static_assert(TP_API != 0 || bufferSize < kMemoryModuleSize,
                  "ERROR: Input Window size (based on requrested window size and FIR length margin) exceeds Memory "
                  "Module size of 32kB.");

    static_assert(!(((TP_DUAL_IP == 1 && TP_NUM_OUTPUTS == 1) || (TP_DUAL_IP == 0 && TP_NUM_OUTPUTS == 2)) &&
                    TP_SSR > 1),
                  "ERROR: if SSR is enabled, DUAL inputs is only supported with DUAL_OUTPUTS");
    static_assert(TP_API == 1 || TP_SSR == 1, "ERROR: SSR > 1 is only supported for streaming API");
    // static_assert(TP_USE_COEFF_RELOAD==0 || TP_SSR == 1, "ERROR: SSR > 1 is only supported for static coefficients");

    // 3d array for storing net information.
    // address[inPhase][outPath][cascPos]
    std::array<std::array<std::array<connect<stream, stream>*, TP_CASC_LEN>, TP_SSR>, TP_SSR> net;
    std::array<std::array<std::array<connect<stream, stream>*, TP_CASC_LEN>, TP_SSR>, TP_SSR> net2;

    void input_connections(port<input>(&in), kernel firKernels[TP_CASC_LEN], int ssrOutPathIndex, int ssrInPhaseIndex) {
        if (TP_API == 0) {
            connect<
                window<TP_INPUT_WINDOW_VSIZE * sizeof(TT_DATA), fnFirMargin<TP_FIR_LEN, TT_DATA>() * sizeof(TT_DATA)> >(
                in, m_firKernels[0].in[0]);
            for (int i = 1; i < TP_CASC_LEN; i++) {
                single_buffer(m_firKernels[i].in[0]);
                connect<window<TP_INPUT_WINDOW_VSIZE * sizeof(TT_DATA) +
                               fnFirMargin<TP_FIR_LEN, TT_DATA>() * sizeof(TT_DATA)> >(
                    async(m_firKernels[i - 1].out[1]), async(m_firKernels[i].in[0]));
            }

        } else {
            if
                constexpr(TP_DUAL_IP == DUAL_IP_SINGLE) {
                    for (int i = 0; i < TP_CASC_LEN; i++) {
                        connect<window<TP_INPUT_WINDOW_VSIZE * sizeof(TT_DATA),
                                       fnFirMargin<TP_FIR_LEN, TT_DATA>() * sizeof(TT_DATA)> >(in,
                                                                                               m_firKernels[i].in[0]);
                    }
                }
        }
    }

    // make cascade connections
    void cascade_connections(kernel firKernels[TP_CASC_LEN], unsigned int CASC_IN_PORT_POS) {
        for (int i = 1; i < TP_CASC_LEN; i++) {
            connect<cascade>(firKernels[i - 1].out[0], firKernels[i].in[CASC_IN_PORT_POS]);
        }
    }

    void conditional_dual_ip_connections(
        port<input>(&in), port<input>(&in2), kernel firKernels[TP_CASC_LEN], int ssrOutPathIndex, int ssrInPhaseIndex) {
        if
            constexpr(TP_API == 0) {
                connect<window<TP_INPUT_WINDOW_VSIZE * sizeof(TT_DATA),
                               fnFirMargin<TP_FIR_LEN, TT_DATA>() * sizeof(TT_DATA)> >(in2, m_firKernels[0].in[1]);
            }
        else {
            kernel m_inWidgetKernel;
            m_inWidgetKernel = kernel::create_object<
                widget_api_cast<TT_DATA, USE_STREAM_API, USE_WINDOW_API, 2, TP_INPUT_WINDOW_VSIZE, 1, 0> >();
            connect<stream>(in, m_inWidgetKernel.in[0]);
            connect<stream>(in2, m_inWidgetKernel.in[1]);
            connect<
                window<TP_INPUT_WINDOW_VSIZE * sizeof(TT_DATA), fnFirMargin<TP_FIR_LEN, TT_DATA>() * sizeof(TT_DATA)> >(
                m_inWidgetKernel.out[0], m_firKernels[0].in[0]);
            source(m_inWidgetKernel) = "widget_api_cast.cpp";
            headers(m_inWidgetKernel) = {"widget_api_cast.hpp"};
            runtime<ratio>(m_inWidgetKernel) = 0.8;

            for (int i = 1; i < TP_CASC_LEN; i++) {
                single_buffer(m_firKernels[i].in[0]);
                connect<window<TP_INPUT_WINDOW_VSIZE * sizeof(TT_DATA) +
                               fnFirMargin<TP_FIR_LEN, TT_DATA>() * sizeof(TT_DATA)> >(
                    async(m_firKernels[i - 1].out[1]), async(m_firKernels[i].in[0]));
            }
        }
    }

    void output_connections(port<output>(&out), kernel firKernels[TP_CASC_LEN]) {
        // make output connections
        if (TP_API == 0) {
            connect<window<TP_INPUT_WINDOW_VSIZE * sizeof(TT_DATA)> >(m_firKernels[TP_CASC_LEN - 1].out[0], out);
        } else {
            connect<stream>(m_firKernels[TP_CASC_LEN - 1].out[0], out);
        }
    }

    void conditional_out_connections(port<output>(&out2), kernel firKernels[TP_CASC_LEN]) {
        if (TP_API == 0) {
            connect<window<TP_INPUT_WINDOW_VSIZE * sizeof(TT_DATA)> >(m_firKernels[TP_CASC_LEN - 1].out[1], out2);
        } else {
            connect<stream>(m_firKernels[TP_CASC_LEN - 1].out[1], out2);
        }
    }

    void conditional_rtp_connections(port<input>(&coeff), kernel firKernels[TP_CASC_LEN]) {
        int rtpPortPos = TP_API == 0 ? TP_DUAL_IP + 1 : 1;
        connect<parameter>(coeff, async(firKernels[0].in[rtpPortPos]));
    }

    // Creates connections - specific to symmetric FIR variant.
    void create_connections() {
        // Symmetric FIRs when cascaded over multiple kernels use:
        // - 1 or 2 input ports for first kernel,
        // - only 1 input port for remaining kernels in the cascade.
        //
        printf("ssrOutputPath, ssrInnerPhase : D(ssrDataPhase) C(ssrCoeffPhase) Starts at kernelStartingIndex:\n");
        for (unsigned int ssrOutputPath = 0; ssrOutputPath < TP_SSR; ssrOutputPath++) {
            for (unsigned int ssrInnerPhase = 0; ssrInnerPhase < TP_SSR; ssrInnerPhase++) {
                // Taking definition from inside ssr_kernels to avoid duplication
                unsigned int ssrDataPhase = lastSSRKernel::getSSRDataPhase(ssrOutputPath, ssrInnerPhase, TP_SSR);
                unsigned int ssrCoeffPhase = lastSSRKernel::getSSRCoeffPhase(ssrOutputPath, ssrInnerPhase, TP_SSR);
                unsigned int kernelStartingIndex =
                    lastSSRKernel::getKernelStartingIndex(ssrDataPhase, ssrOutputPath, TP_SSR, TP_CASC_LEN);
                kernel* firstKernelOfCascadeChain = m_firKernels + kernelStartingIndex;
                kernel* lastKernelOfCascadeChain = m_firKernels + kernelStartingIndex + TP_CASC_LEN - 1;
                unsigned int nextChainKernelStartingIndex = lastSSRKernel::getKernelStartingIndex(
                    lastSSRKernel::getSSRDataPhase(ssrOutputPath, ssrInnerPhase + 1, TP_SSR), ssrOutputPath, TP_SSR,
                    TP_CASC_LEN);
                kernel* firstKernelOfNextCascadeChain = m_firKernels + nextChainKernelStartingIndex;
                printf("%d, %d : D(%d) C(%d) Starts at %d\n", ssrOutputPath, ssrInnerPhase, ssrDataPhase, ssrCoeffPhase,
                       kernelStartingIndex);

                input_connections(in[ssrDataPhase], firstKernelOfCascadeChain, ssrOutputPath, ssrInnerPhase);

                cascade_connections(firstKernelOfCascadeChain, 1); // connect all cascades together

                // Connect SSR Inner phases cascade ports together.
                if (ssrInnerPhase != TP_SSR - 1) {
                    printf(
                        "cascade between ssrInnerPhase (%d) kernels. d(%d) c(%d)\nStartingIndex=%d, "
                        "nextChainKernelStartingIndex=%d, nextChainDataPhase=%d\n",
                        ssrInnerPhase, ssrDataPhase, ssrCoeffPhase, kernelStartingIndex, nextChainKernelStartingIndex,
                        lastSSRKernel::getSSRDataPhase(ssrOutputPath, ssrInnerPhase + 1, TP_SSR));
                    connect<cascade>(lastKernelOfCascadeChain->out[0],
                                     firstKernelOfNextCascadeChain->in[CASC_IN_PORT_POS]);
                }

                if
                    constexpr(TP_DUAL_IP == DUAL_IP_DUAL) conditional_dual_ip_connections(
                        in[ssrDataPhase], in2[ssrDataPhase], firstKernelOfCascadeChain, ssrOutputPath, ssrInnerPhase);

                if
                    constexpr(TP_USE_COEFF_RELOAD == USE_COEFF_RELOAD_TRUE)
                        conditional_rtp_connections(coeff[ssrCoeffPhase], firstKernelOfCascadeChain);

                if (ssrInnerPhase == TP_SSR - 1) output_connections(out[ssrOutputPath], firstKernelOfCascadeChain);

                if
                    constexpr(TP_NUM_OUTPUTS == 2) {
                        conditional_out_connections(out2[ssrOutputPath], firstKernelOfCascadeChain);
                    }

                for (int i = 0; i < TP_CASC_LEN; i++) {
                    // Specify mapping constraints
                    runtime<ratio>(m_firKernels[kernelStartingIndex + i]) = 0.8;
                    // Source files
                    source(m_firKernels[kernelStartingIndex + i]) = "fir_sr_sym.cpp";
                }
            }
        }
    }

    struct ssr_params : public fir_params_defaults {
        static constexpr int Bdim = TP_SSR * TP_SSR - 1;
        using BTT_DATA = TT_DATA;
        using BTT_COEFF = TT_COEFF;
        static constexpr int BTP_FIR_LEN = CEIL(TP_FIR_LEN, TP_SSR);
        static constexpr int BTP_SHIFT = TP_SHIFT;
        static constexpr int BTP_RND = TP_RND;
        static constexpr int BTP_INPUT_WINDOW_VSIZE = TP_INPUT_WINDOW_VSIZE / TP_SSR;
        static constexpr int BTP_CASC_LEN = TP_CASC_LEN;
        static constexpr int BTP_USE_COEFF_RELOAD = TP_USE_COEFF_RELOAD;
        static constexpr int BTP_NUM_OUTPUTS = TP_NUM_OUTPUTS;
        static constexpr int BTP_DUAL_IP = TP_DUAL_IP;
        static constexpr int BTP_API = TP_API;
        static constexpr int BTP_SSR = TP_SSR;
        static constexpr int BTP_COEFF_PHASES = TP_SSR;
        static constexpr int BTP_COEFF_PHASES_LEN = TP_FIR_LEN;
        static constexpr int BTP_CASC_IN = CASC_IN_FALSE;
        static constexpr int BTP_CASC_OUT = CASC_OUT_FALSE;
    };

    using lastSSRKernel = ssr_kernels<ssr_params, fir_sr_sym_tl>;
    using lastSSRKernelAsym = ssr_kernels<ssr_params, sr_asym::fir_sr_asym_tl>;

    template <typename T_D>
    static constexpr unsigned int getMaxTapsPerKernel() {
        constexpr unsigned int kMaxTaps = (std::is_same<TT_DATA, cfloat>::value) ? 256 : 512;
        return kMaxTaps;
    }

    template <typename T_D, typename T_C, int T_PORTS>
    static constexpr unsigned int getOptTapsPerKernel() {
        unsigned int optTaps = getOptTapsPerKernelSrAsym<T_D, T_C, T_PORTS>();
        return optTaps;
    };

   public:
    /**
     * The array of kernels that will be created and mapped onto AIE tiles.
     * Number of kernels (``TP_CASC_LEN * TP_SSR``) will be connected with each other by cascade interface.
     **/
    kernel m_firKernels[TP_CASC_LEN * TP_SSR * TP_SSR];

    /**
     * The input data to the function. This input is either a window API of
     * samples of TT_DATA type or stream API (depending on TP_API).
     * Note: Margin is added internally to the graph, when connecting input port
     * with kernel port. Therefore, margin should not be added when connecting
     * graph to a higher level design unit.
     * Margin size (in Bytes) equals to TP_FIR_LEN rounded up to a nearest
     * multiple of 32 bytes.
     **/
    port_array<input, TP_SSR> in;

    /**
     * The output data from the function. This output is either a window API of
     * samples of TT_DATA type or stream API (depending on TP_API).
     * Number of output samples is determined by interpolation & decimation factors (if present).
     **/
    port_array<output, TP_SSR> out;

    /**
     * The conditional input array data to the function.
     * This input is (generated when TP_DUAL_IP == 1) either a window API of
     * samples of TT_DATA type or stream API (depending on TP_API).
     *
     **/
    port_conditional_array<input, (TP_DUAL_IP == 1), TP_SSR> in2;
    /**
     * The conditional array of input ports  used to pass run-time programmable (RTP) coeficients.
     * This port_conditional_array is (generated when TP_USE_COEFF_RELOAD == 1) an array of input ports, which size is
     *defined by TP_SSR.
     * Each port in the array holds a duplicate of the coefficient array, required to connect to each SSR input path.
     * Size of the coefficient array is dependent on the TP_SSR.
     * - When TP_SSR = 1, the taps array must be supplied in a compressed form, i.e.  \n
     *                   taps[] = {c0, c2, c4, ..., cN} where  \n
     *                   N = (TP_FIR_LEN+1)/2. \n
     *                   For example, a 7-tap halfband decimator might use coeffs
     *                   (1, 2, 3, 5, 3, 2, 1).  \n This would be input as
     *                   coeff[]= {1,2,3,5}.
     * - When TP_SSR > 1, the taps array must be uncompressed and symmetry must be removed.
     *                   For example, a 7-tap halfband decimator might use coeffs
     *                   (1, 2, 3, 5, 3, 2, 1).  \n This would be input as
     *                   coeff[]= {1,2,3,5,3,2,1}.
     **/
    port_conditional_array<input, (TP_USE_COEFF_RELOAD == 1), TP_SSR> coeff;

    /**
     * The output data from the function.
     * This output is (generated when TP_NUM_OUTPUTS == 2) either a window API of
     * samples of TT_DATA type or stream API (depending on TP_API).
     * Number of output samples is determined by interpolation & decimation factors (if present).
     **/
    port_conditional_array<output, (TP_NUM_OUTPUTS == 2), TP_SSR> out2;
    /**
     * The conditional input array data to the function.
     * This input is (generated when TP_CASC_IN == CASC_IN_TRUE) either a cascade input.
     **/
    port_conditional_array<output, (TP_CASC_IN == CASC_IN_TRUE), TP_SSR> casc_in;
    /**
     * Access function to get pointer to kernel (or first kernel in a chained configuration).
     **/
    kernel* getKernels() { return m_firKernels; };

    /**
    * @brief Access function to get kernel's architecture (or first kernel's architecture in a chained configuration).
    **/
    unsigned int getKernelArchs() {
        constexpr unsigned int firRange = (TP_CASC_LEN == 1) ? TP_FIR_LEN : fnFirRangeSym<TP_FIR_LEN, TP_CASC_LEN, 0>();
        // return the architecture for first kernel in the design (only one for single kernel designs).
        // First kernel will always be the slowest of the kernels and so it will reflect on the designs performance
        // best.
        return fir_sr_sym<TT_DATA, TT_COEFF, TP_FIR_LEN, TP_SHIFT, TP_RND, (TP_INPUT_WINDOW_VSIZE / TP_SSR), false,
                          true, firRange, 0, TP_CASC_LEN, TP_DUAL_IP, TP_USE_COEFF_RELOAD, TP_NUM_OUTPUTS,
                          TP_API>::get_m_kArch();
    };

    /**
     * @brief This is the constructor function for the FIR graph with static coefficients.
     * @param[in] taps   a reference to the std::vector array of taps values of type TT_COEFF.
     **/
    fir_sr_sym_graph(const std::vector<TT_COEFF>& taps) {
        if
            constexpr(TP_SSR == 1) {
                lastSSRKernel::create_and_recurse(m_firKernels, taps);
                create_connections();
            }
        else {
            std::vector<TT_COEFF> srAsymTaps = lastSSRKernelAsym::convert_sym_taps_to_asym(TP_FIR_LEN, taps);
            lastSSRKernelAsym::create_and_recurse(m_firKernels, srAsymTaps);
            lastSSRKernelAsym::create_connections(m_firKernels, &in[0], in2, &out[0], out2, coeff, net, net2, casc_in,
                                                  "fir_sr_asym.cpp");
        }
    };

    /**
     * @brief This is the constructor function for the FIR graph with reloadable coefficients.
     **/
    fir_sr_sym_graph() {
        if
            constexpr(TP_SSR == 1) {
                lastSSRKernel::create_and_recurse(m_firKernels);
                create_connections();
            }
        else {
            lastSSRKernelAsym::create_and_recurse(m_firKernels);
            lastSSRKernelAsym::create_connections(m_firKernels, &in[0], in2, &out[0], out2, coeff, net, net2, casc_in,
                                                  "fir_sr_asym.cpp");
        }
    };

    /**
    * @brief Access function to get Graph's minimum cascade length for a given configuration.
    * @tparam T_FIR_LEN length of the fir filter
    * @tparam T_API interface type : 0 - window, 1 - stream
    * @tparam T_D data type
    * @tparam T_C coeff type
    * @tparam SSR parallelism factor set for super sample rate operation
    **/
    template <int T_FIR_LEN, int T_API, typename T_D, typename T_C, unsigned int SSR>
    static constexpr unsigned int getMinCascLen() {
        if
            constexpr(SSR == 1) {
                constexpr int kMaxTaps = getMaxTapsPerKernel<T_D>();
                return xf::dsp::aie::getMinCascLen<T_FIR_LEN, kMaxTaps>();
            }
        else {
            using asym_graph =
                sr_asym::fir_sr_asym_graph<cint16, int16, 8, 0, 0, 128>; // making an arbitrary template list to call
                                                                         // sr_asym, can be avoided if we have default
                                                                         // template parameters for all arguments
            return asym_graph::getMinCascLen<T_FIR_LEN, T_API, T_D, T_C, SSR>();
        }
    };

    /**
    * @brief Access function to get graph's cascade length to obtain maximum performance. (This is relevant only when
    *SSR > 1.)
    * @tparam T_FIR_LEN length of the fir filter
    * @tparam T_D data type
    * @tparam T_C coeff type
    * @tparam T_API ports interface type : 0 - window, 1 - stream
    * @tparam T_PORTS single/dual input and output ports. 1 : single, 2 : dual
    * @tparam SSR parallelism factor set for super sample rate operation
    **/
    template <int T_FIR_LEN, typename T_D, typename T_C, int T_API, int T_PORTS, unsigned int SSR>
    static constexpr unsigned int getOptCascLen() {
        constexpr int kMaxTaps = getMaxTapsPerKernel<T_D>();
        constexpr int kRawOptTaps = getOptTapsPerKernel<T_D, T_C, T_PORTS>();
        return xf::dsp::aie::getOptCascLen<kMaxTaps, kRawOptTaps, T_FIR_LEN, SSR>();
    };
};
}
}
}
}
}

#endif // _DSPLIB_FIR_SR_SYM_GRAPH_HPP_
