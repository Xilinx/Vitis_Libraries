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
#ifndef _DSPLIB_FIR_SR_ASYM_GRAPH_HPP_
#define _DSPLIB_FIR_SR_ASYM_GRAPH_HPP_
/*
The file captures the definition of the 'L2' graph level class for
the Single Rate Asymmetrical FIR library element.
*/
/**
 * @file fir_sr_asym_graph.hpp
 *
 **/

#include <adf.h>
#include <vector>
#include "graph_utils.hpp"
#include <tuple>
#include <typeinfo>
#include "fir_sr_asym.hpp"
#include "fir_utils.hpp"

namespace xf {
namespace dsp {
namespace aie {
namespace fir {
namespace sr_asym {
using namespace adf;

// empty struct for conditional ports using type alias
struct empty {};

//---------------------------------------------------------------------------------------------------
// create_casc_kernel_recur
// Where the FIR function is split over multiple processors to increase throughput, recursion
// is used to generate the multiple kernels, rather than a for loop due to constraints of
// c++ template handling.
// For each such kernel, only a splice of the full array of coefficients is processed.
//---------------------------------------------------------------------------------------------------
/**
  * @cond NOCOMMENTS
  */
// Recursive kernel creation, static/reloadable coefficients
template <int dim,
          typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_CASC_LEN,
          unsigned int TP_USE_COEFF_RELOAD = 0,
          unsigned int TP_NUM_OUTPUTS = 1,
          unsigned int TP_DUAL_IP = 0,
          unsigned int TP_API = 0,
          bool TP_CASC_IN = CASC_IN_FALSE,
          bool TP_CASC_OUT = CASC_OUT_FALSE,
          int TP_MODIFY_MARGIN_OFFSET = 0>
class casc_kernels {
   private:
    static_assert(dim >= 0, "ERROR: dim must be a positive integer");
    static constexpr bool casc_in = (TP_CASC_IN || (dim == 0 ? false : true));
    static constexpr bool casc_out = (TP_CASC_OUT || (dim == (TP_CASC_LEN - 1) ? false : true));
    static constexpr unsigned int firRangeAsym =
        (dim == (TP_CASC_LEN - 1)) ? fnFirRangeRemAsym<TP_FIR_LEN, TP_CASC_LEN, dim, TT_DATA, TT_COEFF, TP_API>()
                                   : // last Kernel gets remainder taps
            fnFirRangeAsym<TP_FIR_LEN, TP_CASC_LEN, dim, TT_DATA, TT_COEFF, TP_API>();

    // Kernels with cascade out only have one output.
    static constexpr unsigned int numOutputs = (casc_out) ? 1 : TP_NUM_OUTPUTS;

   public:
    using kernelClass = fir_sr_asym<TT_DATA,                // TT_DATA
                                    TT_COEFF,               // TT_COEFF
                                    TP_FIR_LEN,             // TP_FIR_LEN
                                    TP_SHIFT,               // TP_SHIFT
                                    TP_RND,                 // TP_RND
                                    TP_INPUT_WINDOW_VSIZE,  // TP_INPUT_WINDOW_VSIZE
                                    casc_in,                // TP_CASC_IN
                                    casc_out,               // TP_CASC_OUT
                                    firRangeAsym,           // TP_FIR_RANGE_LEN
                                    dim,                    // TP_KERNEL_POSITION
                                    TP_CASC_LEN,            // TP_CASC_LEN
                                    TP_USE_COEFF_RELOAD,    // TP_USE_COEFF_RELOAD
                                    numOutputs,             // TP_NUM_OUTPUTS
                                    TP_DUAL_IP,             // TP_DUAL_IP
                                    TP_API,                 // TP_API
                                    TP_MODIFY_MARGIN_OFFSET // TP_MODIFY_MARGIN_OFFSET
                                    >;
    // convenient alias to access any class; casc_kernels_class_lookup<3>::kernelClass gives us the underlying kernel
    // class of the 4th kernel (where the 1st is at index 0).
    template <int dim_idx>
    using casc_kernels_class_lookup = casc_kernels<dim_idx,
                                                   TT_DATA,
                                                   TT_COEFF,
                                                   TP_FIR_LEN,
                                                   TP_SHIFT,
                                                   TP_RND,
                                                   TP_INPUT_WINDOW_VSIZE,
                                                   TP_CASC_LEN,
                                                   TP_USE_COEFF_RELOAD,
                                                   TP_NUM_OUTPUTS,
                                                   TP_DUAL_IP,
                                                   TP_API,
                                                   TP_CASC_IN,
                                                   TP_CASC_OUT,
                                                   TP_MODIFY_MARGIN_OFFSET>;

    using type = casc_kernels_class_lookup<dim>;                                                            // thisClass
    using recurseClass = typename std::conditional_t<(dim > 0), casc_kernels_class_lookup<dim - 1>, empty>; // nextClass

    static void create_and_recurse(kernel firKernels[TP_CASC_LEN], const std::vector<TT_COEFF>& taps) {
        printf(
            "Creating: fir_sr_asym<\n\tDATA, \tCOEFF, \tTP_FIR_LEN %d, \tTP_SHIFT %d, \tTP_RND %d, "
            "\tTP_INPUT_WINDOW_VSIZE %d, \tcasc_in %d, \tcasc_out %d, \tfirRangeAsym %d, \tdim %d, \tTP_CASC_LEN %d, "
            "\tTP_USE_COEFF_RELOAD %d, \tnumOutputs %d, \tTP_DUAL_IP %d, \tTP_API %d, \tTP_MODIFY_MARGIN_OFFSET %d "
            "\n>\n",
            TP_FIR_LEN, TP_SHIFT, TP_RND, TP_INPUT_WINDOW_VSIZE, casc_in, casc_out, firRangeAsym, dim, TP_CASC_LEN,
            TP_USE_COEFF_RELOAD, numOutputs, TP_DUAL_IP, TP_API, TP_MODIFY_MARGIN_OFFSET);

        firKernels[dim] = kernel::create_object<kernelClass>(taps);
        if
            constexpr(dim != 0) { recurseClass::create_and_recurse(firKernels, taps); }
    }
    static void create_and_recurse(kernel firKernels[TP_CASC_LEN]) {
        firKernels[dim] = kernel::create_object<kernelClass>();
        if
            constexpr(dim != 0) { recurseClass::create_and_recurse(firKernels); }
    }
};

#ifndef COEFF_DATA_ALIGNMENT
#define COEFF_DATA_ALIGNMENT 1 // data
#endif

template <int dim,
          typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_CASC_LEN,
          unsigned int TP_USE_COEFF_RELOAD = 0,
          unsigned int TP_NUM_OUTPUTS = 1,
          unsigned int TP_DUAL_IP = 0,
          unsigned int TP_API = 0,
          unsigned int TP_SSR = 1,
          bool TP_CASC_IN = CASC_IN_FALSE,
          bool TP_CASC_OUT = CASC_OUT_FALSE>
class ssr_kernels {
   private:
    static_assert(dim >= 0, "ERROR: dim must be a positive integer");

   public:
    static constexpr unsigned int totalKernels = TP_CASC_LEN * TP_SSR * TP_SSR;
    static constexpr unsigned int ssrOutputPath = (dim / TP_SSR);
    static constexpr unsigned int ssrInnerPhase = (dim % TP_SSR);

    static constexpr unsigned int getSSRDataPhase(unsigned int ssrOutputPath,
                                                  unsigned int ssrInnerPhase,
                                                  unsigned int SSR) {
        if (COEFF_DATA_ALIGNMENT == 0) {
            // aligned by coefficients
            return (ssrOutputPath + (SSR - ssrInnerPhase)) % SSR;
        } else {
            // aligned by data (preferred for data deadlock/backpressure)
            return (ssrInnerPhase) % SSR;
        }
    }
    static constexpr unsigned int getSSRCoeffPhase(unsigned int ssrOutputPath,
                                                   unsigned int ssrInnerPhase,
                                                   unsigned int SSR) {
        if (COEFF_DATA_ALIGNMENT == 0) {
            // aligned by coefficients
            return (ssrInnerPhase) % SSR;
        } else {
            // aligned by data (preferred for data deadlock/backpressure)
            return (ssrOutputPath + (SSR - ssrInnerPhase)) % SSR;
        }
    }
    // aligned by coefficients
    static constexpr unsigned int ssrDataPhase = getSSRDataPhase(ssrOutputPath, ssrInnerPhase, TP_SSR);
    static constexpr unsigned int ssrCoeffPhase = getSSRCoeffPhase(ssrOutputPath, ssrInnerPhase, TP_SSR);

    static constexpr unsigned int FIR_LEN_SSR_RANGE = (TP_FIR_LEN / TP_SSR); // to be ceiled or something
    static constexpr unsigned int getKernelStartingIndex(unsigned int ssrDataPhase,
                                                         unsigned int ssrOutputPath,
                                                         unsigned int SSR,
                                                         unsigned int CASCADE_LENGTH) {
        return ssrDataPhase * CASCADE_LENGTH + ssrOutputPath * SSR * CASCADE_LENGTH;
    }
    static constexpr unsigned int kernelStartingIndex =
        getKernelStartingIndex(ssrDataPhase, ssrOutputPath, TP_SSR, TP_CASC_LEN);

    /** SSR 4 Example why we need margin offset.
     * numbers are data indexes contributing at each ssr kernel.
     * Small example of 8 taps fir_len.
     *
     * Y0
     *  0 -4
     * -1 -5  := starting sample is margin sample, so needs MARGIN_OFFSET_MODIFY
     * -2 -6  := starting sample is margin sample, so needs MARGIN_OFFSET_MODIFY
     * -3 -7  := starting sample is margin sample, so needs MARGIN_OFFSET_MODIFY
     *
     * Y1
     *  1 -3
     *  0 -4
     * -1 -5  := starting sample is margin sample, so needs MARGIN_OFFSET_MODIFY
     * -2 -6  := starting sample is margin sample, so needs MARGIN_OFFSET_MODIFY
     *
     * Y2
     *  2 -2
     *  1 -3
     *  0 -4
     * -1 -5  := starting sample is margin sample, so needs MARGIN_OFFSET_MODIFY
     *
     * Y3
     *  3 -1
     *  2 -2
     *  1 -3
     *  0 -4
     *
     *
     * We always only need a -1 margin offset, and we always have at least one spare margin sample
     * because we request FIR_LEN margin samples rather than FIR_LEN-1 margin samples.
     */
    static constexpr int MODIFY_MARGIN_OFFSET = (ssrOutputPath < ssrDataPhase) ? -1 : 0;

    /**
     * @brief Seperate taps into a specific SSR phase.
     *    Phase 0 contains taps index 0, TP_SSR, 2*TP_SSR, ...
     *    Phase 1 contains taps index 1, TP_SSR+1, 2*TP_SSR+1, ...
     *    ...
     *    Phase TP_SSR-1 contains taps index TP_SSR-1, TP_SSR+TP_SSR-1, 2*TP_SSR+TP_SSR-1, ...
     *
     * @param taps
     * @return std::array<std::vector<TT_COEFF>, TP_SSR>
     */
    static std::vector<TT_COEFF> segment_taps_array_for_phase(const std::vector<TT_COEFF>& taps,
                                                              const unsigned int coeffPhase) {
        // https://stackoverflow.com/a/421615
        typename std::vector<TT_COEFF>::const_iterator firstTap = taps.begin() + FIR_LEN_SSR_RANGE * coeffPhase;
        typename std::vector<TT_COEFF>::const_iterator lastTap = firstTap + FIR_LEN_SSR_RANGE;
        std::vector<TT_COEFF> ssrTapsRange; //
        for (unsigned int i = 0; i < FIR_LEN_SSR_RANGE; i++) {
            unsigned int coefIndex = i * TP_SSR + coeffPhase;
            if (coefIndex < TP_FIR_LEN) {
                ssrTapsRange.push_back(taps.at(coefIndex));

            } else {
                // padding
                ssrTapsRange.push_back(nullElem<TT_COEFF>());
            }
        }

        //(firstTap, lastTap);

        return ssrTapsRange;
    }

    template <int ssr_kernel_idx>
    using ssr_kernels_lookup = ssr_kernels<ssr_kernel_idx,
                                           TT_DATA,
                                           TT_COEFF,
                                           TP_FIR_LEN,
                                           TP_SHIFT,
                                           TP_RND,
                                           TP_INPUT_WINDOW_VSIZE,
                                           TP_CASC_LEN,
                                           TP_USE_COEFF_RELOAD,
                                           TP_NUM_OUTPUTS,
                                           TP_DUAL_IP,
                                           TP_API,
                                           TP_SSR,
                                           TP_CASC_IN,
                                           TP_CASC_OUT>;
    using this_ssr_kernel = ssr_kernels_lookup<dim>;

    // middle kernels connect to eachother.
    static constexpr bool casc_in = (ssrInnerPhase == 0) ? (false || TP_CASC_IN) : true; // first kernel of output phase
    static constexpr bool casc_out =
        (ssrInnerPhase == TP_SSR - 1) ? (false || TP_CASC_OUT) : true; // last kernel of output phase

    using this_casc_kernels =
        casc_kernels<TP_CASC_LEN - 1, // dim
                     TT_DATA,
                     TT_COEFF,
                     TP_FIR_LEN / TP_SSR, // each cascade kernel chain processes a specific phase of coefficients
                     TP_SHIFT,
                     TP_RND,
                     TP_INPUT_WINDOW_VSIZE,
                     TP_CASC_LEN,
                     TP_USE_COEFF_RELOAD,
                     TP_NUM_OUTPUTS,
                     TP_DUAL_IP,
                     TP_API,
                     casc_in,  // TP_CASC_IN,
                     casc_out, // TP_CASC_OUT,
                     MODIFY_MARGIN_OFFSET>;

    // avoid any reference to a ssr_kernels_lookup<-1>;
    using next_ssr_kernel = typename std::conditional_t<(dim > 0), ssr_kernels_lookup<dim - 1>, empty>;

    static void create_and_recurse(kernel (&firKernels)[totalKernels], const std::vector<TT_COEFF>& taps) {
        // only pass a subset of the taps to the casc_kernels, and have it treat that as it normally would.
        std::vector<TT_COEFF> ssrPhaseTaps(segment_taps_array_for_phase(taps, ssrCoeffPhase));
        if
            constexpr(dim == (TP_SSR * TP_SSR) - 1) {
                printf(
                    "m_firKernels[range] corresponds to ssrOutputPath,ssrInnerPhase D(ssrDataPhase) C(ssrCoeffPhase) "
                    ":\n");
            }
        printf("m_firKernels[%d:%d] = %d,%d D(%d) C(%d) \n", kernelStartingIndex, kernelStartingIndex + TP_CASC_LEN - 1,
               ssrOutputPath, ssrInnerPhase, ssrDataPhase, ssrCoeffPhase);
        // pass the kernels from a specific index so cascades can be placed as normal.
        this_casc_kernels::create_and_recurse(firKernels + kernelStartingIndex, ssrPhaseTaps);

        if
            constexpr(dim > 0) next_ssr_kernel::create_and_recurse(firKernels, taps);
    }

    static void create_and_recurse(kernel (&firKernels)[totalKernels]) {
        // pass the kernels from a specific index so cascades can be placed as "normal" from that position.
        this_casc_kernels::create_and_recurse(firKernels + kernelStartingIndex);

        if
            constexpr(dim > 0) next_ssr_kernel::create_and_recurse(firKernels);
    }

    /**
     * @brief Seperate taps into an SSR number of phases. Used for debug / holistic view
     *    Phase 0 contains taps index 0, TP_SSR, 2*TP_SSR, ...
     *    Phase 1 contains taps index 1, TP_SSR+1, 2*TP_SSR+1, ...
     *    ...
     *    Phase TP_SSR-1 contains taps index TP_SSR-1, TP_SSR+TP_SSR-1, 2*TP_SSR+TP_SSR-1, ...
     *
     * @param taps
     * @return std::array<std::vector<TT_COEFF>, TP_SSR>
     */
    static std::array<std::vector<TT_COEFF>, TP_SSR> segment_taps_array(const std::vector<TT_COEFF>& taps) {
        std::array<std::vector<TT_COEFF>, TP_SSR> ssrTapsArray;
        for (unsigned int ssrCoeffPhase = 0; ssrCoeffPhase < TP_SSR; ssrCoeffPhase++) {
            std::vector<TT_COEFF> ssrTapsRange(segment_taps_array_for_phase(taps, ssrCoeffPhase));

            ssrTapsArray[ssrCoeffPhase].insert(ssrTapsArray[ssrCoeffPhase].end(), ssrTapsRange.begin(),
                                               ssrTapsRange.end()); // = ssrTapsRange;
        }
        // todo, add debug
        return ssrTapsArray;
    }
};

/**
  * @endcond
  */

//--------------------------------------------------------------------------------------------------
// fir_sr_asym_graph template
//--------------------------------------------------------------------------------------------------
/**
 * @brief fir_sr_asym is a Asymmetric Single Rate FIR filter
 *
 * These are the templates to configure the Asymmetric Single Rate FIR class.
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
 * @tparam TP_INPUT_WINDOW_VSIZE describes the number of samples to process in one
 *         simulation iteration of the filter function. \n
 *         The number of samples on the output will be TP_INPUT_WINDOW_VSIZE
 *         also by virtue the single rate nature of this function. \n
 *         When TP_DUAL_IP is used and PORT_API is streaming (1), this defines the
 *         the number of samples in total (spread over two ports) to process in
 *         one iteration.
 *         Note: Margin size should not be included in TP_INPUT_WINDOW_VSIZE.
 * @tparam TP_CASC_LEN describes the number of AIE processors to split the operation
 *         over.  \n This allows resource to be traded for higher performance.
 *         TP_CASC_LEN must be in the range 1 (default) to 9.
 * @tparam TP_USE_COEFF_RELOAD allows the user to select if runtime coefficient
 *         reloading should be used.   \n When defining the parameter:
 *         - 0 = static coefficients, defined in filter constructor,
 *         - 1 = reloadable coefficients, passed as argument to runtime function. \n
 *
 *         Note: when used, optional port: ``` port<input> coeff; ``` will be added to the FIR. \n
 * @tparam TP_NUM_OUTPUTS sets the number of ports to broadcast the output to. \n
 *         Note: when used, optional port: ``` std::array<port<output>, TP_SSR> out2; ``` will be added to the FIR. \n
 *         Note: For Windows API, additional output an exact copy of the data. \n
 *         Stream API interleaves the output data with a 128-bit pattern, e.g.: \n
 *         - samples 0-3 to be sent over stream0 for cint16 data type, \n
 *         - samples 4-7 to be sent over stream1 for cint16 data type. \n
 * @tparam TP_DUAL_IP allows 2 stream inputs to be connected to FIR, increasing available throughput. \n
 *         When set to 0, single stream will be connected as FIRs input. \n
 *         When set to 1, two stream inputs will be connected. \n
 *         In such case data should be organized in 128-bit interleaved pattern, e.g.: \n
 *         - samples 0-3 to be sent over stream0 for cint16 data type, \n
 *         - samples 4-7 to be sent over stream1 for cint16 data type. \n
 *
 *         Note: Dual input streams offer no throughput gain if only single output stream would be used.
 *         Therefore, dual input streams are only supported with 2 output streams. \n
 *         Note: Dual input ports offer no throughput gain if port api is windows.
 *         Therefore, dual input ports are only supported with streams and not windows.
 * @tparam TP_API specifies if the input/output interface should be window-based or stream-based.  \n
 *         The values supported are 0 (window API) or 1 (stream API).
 * @tparam TP_SSR specifies the number of parallel input/output paths where samples are interleaved between paths,
 giving an overall higher throughput.   \n
 *         An SSR of 1 means just one output leg, and is the backwards compatible option. The number of AIEs used is
 given by SSR^2*CASC_LEN.

 **/
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_CASC_LEN = 1,
          unsigned int TP_USE_COEFF_RELOAD = 0,
          unsigned int TP_NUM_OUTPUTS = 1,
          unsigned int TP_DUAL_IP = 0,
          unsigned int TP_API = 0,
          unsigned int TP_SSR = 1>
/**
 **/
class fir_sr_asym_graph : public graph {
   private:
    static constexpr bool TP_CASC_IN = CASC_IN_FALSE;   // should be unsigned int if exposed on graph interface
    static constexpr bool TP_CASC_OUT = CASC_OUT_FALSE; // should be unsigned int if exposed on graph interface
    /**
     * Port positions for kernel's optional ports.
     */
    static constexpr unsigned int DUAL_OUT_PORT_POS = 1;
    static constexpr unsigned int DUAL_IP_PORT_POS = 1;
    static constexpr unsigned int CASC_IN_PORT_POS = (TP_DUAL_IP == DUAL_IP_DUAL) ? 2 : 1;
    static constexpr unsigned int RTP_PORT_POS =
        ((TP_DUAL_IP == DUAL_IP_DUAL) ? ((TP_CASC_IN == CASC_IN_TRUE) ? 3 : 2) : 1);

    kernel m_firKernels[TP_SSR * TP_SSR * TP_CASC_LEN];

    /**
     * @brief Helper Aliases
     */
    template <class dir>
    using ssr_port_array = std::array<port<dir>, TP_SSR>;

    // 3d array for storing net information.
    // address[inPhase][outPath][cascPos]
    std::array<std::array<std::array<connect<stream, stream>*, TP_CASC_LEN>, TP_SSR>, TP_SSR> net;
    std::array<std::array<std::array<connect<stream, stream>*, TP_CASC_LEN>, TP_SSR>, TP_SSR> net2;

    static constexpr unsigned int INPUT_WINDOW_BYTESIZE = TP_INPUT_WINDOW_VSIZE * sizeof(TT_DATA);
    static constexpr unsigned int OUTPUT_WINDOW_BYTESIZE = INPUT_WINDOW_BYTESIZE;
    static constexpr unsigned int MARGIN_BYTESIZE = fnFirMargin<TP_FIR_LEN / TP_SSR, TT_DATA>() * sizeof(TT_DATA);
    static constexpr int TP_MODIFY_MARGIN_OFFSET = 0;

    // This figure is mostly guesswork and will likely need changed for specfic systems.
    // If this is lower, then SSR designs (ssr2 casc2) typically suffer from stream stalls, which ruins QoR.
    static constexpr unsigned int fifo_depth_multiple = 40;

    /**
     * Base value FIFO Depth, in words (32-bits).
     * Used with streaming interface.
     * During graph construction, FIFO constraint is applied to all broadcast input nets, which are created as part of a
     *cascaded design connection.
     * Constraint is added after the input port has been broadcast and consists of:
     * - a base value of FIFO depth. This minimum value is applied to all broadcast nets.
     * - a variable part, where each net will be assigned an additional value based on position in the cascade.
     **/
    int baseFifoDepth = 32;

    int calculate_fifo_depth(int kernelPos) {
        // In FIFO mode, FIFO size has to be a multiple of 128 bits.
        // In FIFO mode, BD length has to be >= 128 bytes!
        // Conservative assumptions need to be made here.
        // For an overall decimation factor, with instreasing decimation factor,
        // the amount of input stream data required to produce output samples also increases.
        // This strains the FIFOs closer to the begining of the chain comparably more
        // than the ones closest to the output.
        //
        // Conservative assumptions need to be made here, as mapper may place multiple buffers in
        // each of the memory banks, that may introduce Memory conflicts.
        // On top of that, the placement of input source wrt brodcast kernel inputs may introduce significant routing
        // delays.
        // which may have an adverse effect on the amount of FIFO storage available for filter design purposes.
        int fifoStep = (TP_CASC_LEN - kernelPos + 1);

        int fifoDepth = baseFifoDepth + fifo_depth_multiple * fifoStep;
        // limit size at a single memory bank - 8kB
        const int memBankSize = 2048; // 32-bit words
        int fifoDepthCap = fifoDepth < memBankSize ? fifoDepth : memBankSize;
        return fifoDepthCap;
    }

    template <int ssr_dim>
    using ssrKernelLookup = ssr_kernels<ssr_dim,
                                        TT_DATA,
                                        TT_COEFF,
                                        TP_FIR_LEN,
                                        TP_SHIFT,
                                        TP_RND,
                                        TP_INPUT_WINDOW_VSIZE,
                                        TP_CASC_LEN,
                                        TP_USE_COEFF_RELOAD,
                                        TP_NUM_OUTPUTS,
                                        TP_DUAL_IP,
                                        TP_API,
                                        TP_SSR,
                                        TP_CASC_IN,
                                        TP_CASC_OUT>;
    using lastSSRKernel = ssrKernelLookup<(TP_SSR * TP_SSR) - 1>;
    template <int casc_dim, int ssr_dim = 0>
    using cascKernelLookup =
        typename ssrKernelLookup<ssr_dim>::this_casc_kernels::template casc_kernels_class_lookup<casc_dim>;
    template <int ssr_dim = 0>
    using lastCascKernel = cascKernelLookup<ssr_dim, TP_CASC_LEN - 1>;

    using uut_kernel = typename cascKernelLookup<0, 0>::kernelClass; // very first kernel

    // make connections for a
    void input_connections(port<input>(&in), kernel firKernels[TP_CASC_LEN], int ssrOutPathIndex, int ssrInPhaseIndex) {
        // make in connections
        if (TP_API == USE_WINDOW_API) {
            connect<window<INPUT_WINDOW_BYTESIZE, MARGIN_BYTESIZE> >(in, firKernels[0].in[0]);
            for (int i = 1; i < TP_CASC_LEN; i++) {
                single_buffer(firKernels[i].in[0]);
                connect<window<INPUT_WINDOW_BYTESIZE + MARGIN_BYTESIZE> >(async(firKernels[i - 1].out[1]),
                                                                          async(firKernels[i].in[0]));
            }
        } else if (TP_API == USE_STREAM_API) {
            for (int i = 0; i < TP_CASC_LEN; i++) {
                net[ssrInPhaseIndex][ssrOutPathIndex][i] = new connect<stream, stream>(in, firKernels[i].in[0]);
                fifo_depth(*net[ssrInPhaseIndex][ssrOutPathIndex][i]) = calculate_fifo_depth(i);
            }
        } else {
            for (int i = 0; i < TP_CASC_LEN; i++) {
                connect<window<INPUT_WINDOW_BYTESIZE, MARGIN_BYTESIZE> >(in, firKernels[i].in[0]);
            }
        }
    }

    // make cascade connections
    void cascade_connections(kernel firKernels[TP_CASC_LEN]) {
        for (int i = 1; i < TP_CASC_LEN; i++) {
            connect<cascade>(firKernels[i - 1].out[0], firKernels[i].in[CASC_IN_PORT_POS]);
        }
    }

    // make output connections
    void output_connections(port<output>(&out), kernel firKernels[TP_CASC_LEN]) {
        if (TP_CASC_OUT == CASC_OUT_TRUE) {
            connect<cascade>(firKernels[TP_CASC_LEN - 1].out[0], out);
        } else {
            if (TP_API == USE_WINDOW_API) {
                connect<window<INPUT_WINDOW_BYTESIZE> >(firKernels[TP_CASC_LEN - 1].out[0], out);
            } else {
                connect<stream>(firKernels[TP_CASC_LEN - 1].out[0], out);
            }
        }
    }

    // make dual input connections
    void conditional_dual_ip_connections(port<input>(&in2),
                                         kernel firKernels[TP_CASC_LEN],
                                         int ssrOutPathIndex,
                                         int ssrInPhaseIndex) {
        if
            constexpr(TP_DUAL_IP == DUAL_IP_DUAL) {
                if (TP_API == USE_STREAM_API) {
                    for (int i = 0; i < TP_CASC_LEN; i++) {
                        net2[ssrOutPathIndex][ssrInPhaseIndex][i] =
                            new connect<stream, stream>(in2, firKernels[i].in[DUAL_IP_PORT_POS]);
                        fifo_depth(*net2[ssrOutPathIndex][ssrInPhaseIndex][i]) = calculate_fifo_depth(i);
                    }
                }
            }
    }

    // make cascade input connection
    void conditional_casc_in_connections(port<input>(&casc_in), kernel firKernels[TP_CASC_LEN]) {
        if
            constexpr(TP_CASC_IN == CASC_IN_TRUE) { connect<cascade>(casc_in, firKernels[0].in[CASC_IN_PORT_POS]); }
    }

    // make output connections
    void conditional_out_connections(port<output>(&out2), kernel firKernels[TP_CASC_LEN]) {
        if
            constexpr(TP_NUM_OUTPUTS == 2) {
                if (TP_API == USE_WINDOW_API) {
                    connect<window<OUTPUT_WINDOW_BYTESIZE> >(firKernels[TP_CASC_LEN - 1].out[DUAL_OUT_PORT_POS], out2);
                } else {
                    connect<stream>(firKernels[TP_CASC_LEN - 1].out[DUAL_OUT_PORT_POS], out2);
                }
            }
    }

    // make RTP connection
    void conditional_rtp_connections(port<input>(&coeff), kernel firKernels[TP_CASC_LEN]) {
        if
            constexpr(TP_USE_COEFF_RELOAD == USE_COEFF_RELOAD_TRUE) {
                connect<parameter>(coeff, async(firKernels[0].in[RTP_PORT_POS]));
            }
    }

    // make connections
    void create_connections() {
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

                cascade_connections(firstKernelOfCascadeChain); // connect all cascades together

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
                        in2[ssrDataPhase], firstKernelOfCascadeChain, ssrOutputPath, ssrInnerPhase);
                if
                    constexpr(TP_USE_COEFF_RELOAD == USE_COEFF_RELOAD_TRUE)
                        conditional_rtp_connections(coeff[ssrCoeffPhase], firstKernelOfCascadeChain);

                if (ssrInnerPhase == 0)
                    if
                        constexpr(TP_CASC_IN == CASC_IN_TRUE)
                            conditional_casc_in_connections(casc_in[ssrOutputPath], firstKernelOfCascadeChain);

                // Final inner phase produces output.
                if (ssrInnerPhase == TP_SSR - 1) {
                    output_connections(out[ssrOutputPath], firstKernelOfCascadeChain);
                    if
                        constexpr(TP_NUM_OUTPUTS == 2)
                            conditional_out_connections(out2[ssrOutputPath], firstKernelOfCascadeChain);
                }

                for (int i = 0; i < TP_CASC_LEN; i++) {
                    // Specify mapping constraints
                    runtime<ratio>(m_firKernels[kernelStartingIndex + i]) = 0.8;
                    // Source files
                    source(m_firKernels[kernelStartingIndex + i]) = "fir_sr_asym.cpp";
                }
            }
        }
    };

    static_assert(TP_SSR >= 1, "ERROR: TP_SSR must be 1 or higher");
    static_assert(TP_FIR_LEN % TP_SSR == 0,
                  "FIR LEN must be divisble by TP_SSR"); // Consider removing later with padding.
    static_assert(TP_CASC_LEN <= 40, "ERROR: Unsupported Cascade length");
    // Dual input streams offer no throughput gain if only single output stream would be used.
    // Therefore, dual input streams are only supported with 2 output streams.
    static_assert(TP_NUM_OUTPUTS > TP_DUAL_IP,
                  "ERROR: Dual input streams only supported when number of output streams is also 2. ");
    // Dual input ports offer no throughput gain if port api is windows.
    // Therefore, dual input ports are only supported with streams and not windows.
    static_assert(TP_API == USE_STREAM_API || TP_DUAL_IP == DUAL_IP_SINGLE,
                  "ERROR: Dual input ports only supported when port API is a stream. ");
    // Dual input ports offer no throughput gain if port api is windows.
    // Therefore, dual input ports are only supported with streams and not windows.
    static_assert(not(TP_API == USE_WINDOW_API && TP_SSR > 1 && TP_NUM_OUTPUTS == 2),
                  "ERROR: Dual output ports is not supported when port API is a window and SSR > 1. ");

    static constexpr unsigned int kMaxTapsPerKernel = 256;
    // Limit FIR length per kernel. Longer FIRs may exceed Program Memory and/or system memory combined with window
    // buffers may exceed Memory Module size
    static_assert((TP_FIR_LEN / TP_SSR) / TP_CASC_LEN <= kMaxTapsPerKernel,
                  "ERROR: Requested FIR length and Cascade length exceeds supported number of taps per kernel. Please "
                  "increase the cascade legnth to accomodate the FIR design.");

    // Limit FIR length for reloadable coeffs. Reloadable coeffs need a storage space that contibutes to system memory
    // exceeding Memory Module size.
    static_assert(TP_USE_COEFF_RELOAD == 0 || (TP_FIR_LEN / TP_SSR) <= kMaxTapsPerKernel,
                  "ERROR: Exceeded maximum supported FIR length with reloadable coefficients. Please limit the FIR "
                  "length or disable coefficient reload.");
    static_assert(TP_USE_COEFF_RELOAD == 0 || (TP_USE_COEFF_RELOAD == 1 && TP_SSR <= 1),
                  "ERROR: Coefficient reload is not supported for SSR configurations.");

    static constexpr unsigned int kMemoryModuleSize = 32768;
    static constexpr unsigned int bufferSize = (((TP_FIR_LEN / TP_SSR) + TP_INPUT_WINDOW_VSIZE) * sizeof(TT_DATA));
    // Requested Window buffer exceeds memory module size
    static_assert(TP_API != 0 || bufferSize < kMemoryModuleSize,
                  "ERROR: Input Window size (based on requrested window size and FIR length margin) exceeds Memory "
                  "Module size of 32kB");

   public:
    /**
     * The input data array to the function. This input array is either a window API of
     * samples of TT_DATA type or stream API (depending on TP_API).
     * Note: Margin is added internally to the graph, when connecting input port
     * with kernel port. Therefore, margin should not be added when connecting
     * graph to a higher level design unit.
     * Margin size (in Bytes) equals to TP_FIR_LEN rounded up to a nearest
     * multiple of 32 bytes.
     **/
    port_array<input, TP_SSR> in;

    /**
     * The output data array from the function. This output is either a window API of
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
     * The conditional coefficient array data to the function.
     * This port is (generated when TP_USE_COEFF_RELOAD == 1) an array of coefficients of TT_COEFF type.
     *
     **/
    port_conditional_array<input, (TP_USE_COEFF_RELOAD == 1), TP_SSR> coeff;

    /**
     * The output data array from the function.
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
     * Access function to get pointer to kernel (or first kernel in a chained and/or SSR configurations).
     * No arguments required.
     **/
    kernel* getKernels() { return m_firKernels; };

    /**
     * Access function to get pointer to an indexed kernel.
     * @param[in] ssrOutPathIndex      an index to the output data Path.
     * @param[in] ssrInPhaseIndex     an index to the input data Phase
     * @param[in] cascadePosition   an index to the kernel's position in the cascade.
     **/
    kernel* getKernels(int ssrOutPathIndex, int ssrInPhaseIndex, int cascadePosition) {
        return m_firKernels + ssrOutPathIndex * TP_SSR * TP_CASC_LEN + ssrInPhaseIndex * TP_CASC_LEN + cascadePosition;
    };

    /**
     * Access function to get pointer to net of the ``` in ``` port.
     * Nets only get assigned when streaming interface is being broadcast, i.e.
     * nets only get used when TP_API == 1 and TP_CASC_LEN > 1 or TP_SSR > 1
     * @param[in] ssrOutPathIndex      an index to the output data Path.
     * @param[in] ssrInPhaseIndex     an index to the input data Phase
     * @param[in] cascadePosition   an index to the kernel's position in the cascade.
     **/
    connect<stream, stream>* getInNet(int ssrOutPathIndex, int ssrInPhaseIndex, int cascadePosition) {
        return net[ssrOutPathIndex][ssrInPhaseIndex][cascadePosition];
    };

    /**
     * Access function to get pointer to net of the ``` in2 ``` port,
     * when port is being generated, i.e. when TP_DUAL_IP == 1.
     * Nets only get assigned when streaming interface is being broadcast, i.e.
     * nets only get used when TP_API == 1 and TP_CASC_LEN > 1 or TP_SSR > 1
     * @param[in] ssrOutPathIndex      an index to the output data Path.
     * @param[in] ssrInPhaseIndex     an index to the input data Phase
     * @param[in] cascadePosition   an index to the kernel's position in the cascade.
     **/
    connect<stream, stream>* getIn2Net(int ssrOutPathIndex, int ssrInPhaseIndex, int cascadePosition) {
        return net2[ssrOutPathIndex][ssrInPhaseIndex][cascadePosition];
    };

    /**
     * @cond NOCOMMENTS
     */
    /**
    * @brief Access function to get Graphs minimum cascade length for a given configuration.
    **/
    static unsigned int getMinCascadeLength() {
        unsigned int minCascLen = CEIL(TP_FIR_LEN, kMaxTapsPerKernel) / kMaxTapsPerKernel;

        return minCascLen;
    };
    /**
     * @endcond
     */

    /**
    * @brief Access function to get kernel's architecture (or first kernel's architecture in a chained configuration).
    **/
    unsigned int getKernelArchs() {
        // return the architecture for first kernel in the design (only one for single kernel designs).
        // First kernel will always be the slowest of the kernels and so it will reflect on the designs performance
        // best.
        return uut_kernel::get_m_kArch();
    };

    /**
     * @brief This is the constructor function for the FIR graph with reloadable coefficients. \n
     * Constructor has no args. To be used with TP_USE_COEFF_RELOAD=1, taps needs to be passed through RTP
     **/
    fir_sr_asym_graph() {
        printf("== class fir_sr_asym_base_graph (reloadable): \n");
        lastSSRKernel::create_and_recurse(m_firKernels);
        create_connections();
    };

    /**
     * @brief This is the constructor function for the FIR graph with static coefficients.
     * @param[in] taps   a reference to the std::vector array of taps values of type TT_COEFF.
     **/
    fir_sr_asym_graph(const std::vector<TT_COEFF>& taps) {
        printf("== class fir_sr_asym_base_graph (static): \n");
        lastSSRKernel::create_and_recurse(m_firKernels, taps);
        create_connections();
    };
};
}
}
}
}
} // namespace braces
#endif //_DSPLIB_FIR_SR_ASYM_GRAPH_HPP_
