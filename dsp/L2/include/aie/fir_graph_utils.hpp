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
#ifndef _DSPLIB_FIR_GRAPH_UTILS_HPP_
#define _DSPLIB_FIR_GRAPH_UTILS_HPP_
/*
The file captures the definition of the graph utilities commonly used across various library elements
*/

#include <adf.h>
#include <vector>

#include "fir_utils.hpp"
#include "graph_utils.hpp"

namespace xf {
namespace dsp {
namespace aie {
using namespace adf;
/**
  * @cond NOCOMMENTS
  */

// Recursive cascaded kernel creation, any FIR variant with any params
template <typename casc_params = fir_params_defaults, template <typename> typename fir_type = fir_type_default>
class casc_kernels {
   private:
    static_assert(casc_params::Bdim >= 0, "ERROR: dim must be a positive integer");

    static constexpr bool thisCasc_in = (casc_params::BTP_CASC_IN || (casc_params::Bdim == 0 ? false : true));
    static constexpr bool thisCasc_out =
        (casc_params::BTP_CASC_OUT || (casc_params::Bdim == (casc_params::BTP_CASC_LEN - 1) ? false : true));

    static constexpr unsigned int thisKernelRange =
        fir_type<casc_params>::template getKernelFirRangeLen<casc_params::Bdim>();
    static constexpr unsigned int thisNumOutputs =
        (thisCasc_out)
            ? 1
            : casc_params::BTP_NUM_OUTPUTS; // overwrite nu_outputs for internally connected kernels (no output anyway)

    struct thisKernel_params : public casc_params {
        static constexpr int Bdim = casc_params::Bdim;
        static constexpr int BTP_CASC_IN = thisCasc_in;
        static constexpr int BTP_CASC_OUT = thisCasc_out;
        static constexpr int BTP_FIR_RANGE_LEN = thisKernelRange;
        static constexpr int BTP_KERNEL_POSITION = casc_params::Bdim;
        static constexpr int BTP_NUM_OUTPUTS = thisNumOutputs;
    };

    using thisKernelClass = fir_type<thisKernel_params>;
    using thisKernelParentClass = typename fir_type<thisKernel_params>::parent_class;

    static constexpr int nextBdim = casc_params::Bdim - 1;
    static constexpr unsigned int nextKernelRange =
        fir_type<casc_params>::template getKernelFirRangeLen<casc_params::Bdim>();

    // Kernels with cascade out only have one output.
    // This assumption may not be needed.

    // need to create parameters for downstream kernels, e.g. dim.
    // inherit and overwrite non-defaults
    struct nextKernel_params : public casc_params {
        static constexpr int Bdim = nextBdim;
        static constexpr int BTP_FIR_RANGE_LEN = nextKernelRange;
        static constexpr int BTP_KERNEL_POSITION = nextBdim;
    };
    using nextKernelClass = fir_type<nextKernel_params>;

    // recursive call with updated params
    using next_casc_kernels = casc_kernels<nextKernel_params, fir_type>;

   public:
    static void create_and_recurse(kernel firKernels[casc_params::BTP_CASC_LEN],
                                   const std::vector<typename casc_params::BTT_COEFF>& taps) {
        firKernels[casc_params::Bdim] = kernel::create_object<thisKernelParentClass>(taps);
        if
            constexpr(casc_params::Bdim != 0) { next_casc_kernels::create_and_recurse(firKernels, taps); }
    }
    static void create_and_recurse(kernel firKernels[casc_params::BTP_CASC_LEN]) {
        firKernels[casc_params::Bdim] = kernel::create_object<thisKernelParentClass>();
        if
            constexpr(casc_params::Bdim != 0) { next_casc_kernels::create_and_recurse(firKernels); }
    }
};

#ifndef COEFF_DATA_ALIGNMENT
#define COEFF_DATA_ALIGNMENT 1 // data
#endif

template <typename ssr_params = fir_params_defaults, template <typename> typename fir_type = fir_type_default>
class ssr_kernels {
   public:
    using TT_DATA = typename ssr_params::BTT_DATA;
    using TT_COEFF = typename ssr_params::BTT_COEFF;
    static constexpr unsigned int dim = ssr_params::Bdim;
    static constexpr unsigned int TP_FIR_LEN = ssr_params::BTP_FIR_LEN;
    static constexpr unsigned int TP_FIR_RANGE_LEN = ssr_params::BTP_FIR_RANGE_LEN;
    static constexpr unsigned int TP_SHIFT = ssr_params::BTP_SHIFT;
    static constexpr unsigned int TP_RND = ssr_params::BTP_RND;
    static constexpr unsigned int TP_INTERPOLATE_FACTOR = ssr_params::BTP_INTERPOLATE_FACTOR;
    static constexpr unsigned int TP_DECIMATE_FACTOR = ssr_params::BTP_DECIMATE_FACTOR;
    static constexpr unsigned int TP_INPUT_WINDOW_VSIZE = ssr_params::BTP_INPUT_WINDOW_VSIZE;
    static constexpr unsigned int TP_CASC_LEN = ssr_params::BTP_CASC_LEN;
    static constexpr unsigned int TP_USE_COEFF_RELOAD = ssr_params::BTP_USE_COEFF_RELOAD;
    static constexpr unsigned int TP_NUM_OUTPUTS = ssr_params::BTP_NUM_OUTPUTS;
    static constexpr unsigned int TP_DUAL_IP = ssr_params::BTP_DUAL_IP;
    static constexpr unsigned int TP_API = ssr_params::BTP_API;
    static constexpr unsigned int TP_SSR = ssr_params::BTP_SSR;
    static constexpr unsigned int TP_COEFF_PHASE = ssr_params::BTP_COEFF_PHASE;
    static constexpr unsigned int TP_COEFF_PHASE_OFFSET = ssr_params::DTP_COEFF_PHASE_OFFSET;
    static constexpr unsigned int TP_COEFF_PHASES = ssr_params::BTP_COEFF_PHASES;
    static constexpr unsigned int TP_COEFF_PHASES_LEN = ssr_params::BTP_COEFF_PHASES_LEN;
    static constexpr unsigned int TP_PARA_INTERP_POLY =
        ssr_params::BTP_PARA_INTERP_POLY; // ssr_kernels class is already decomposed, so should not have any need for
                                          // decomposition polyphases.
    static constexpr bool TP_CASC_IN = ssr_params::BTP_CASC_IN;
    static constexpr bool TP_CASC_OUT = ssr_params::BTP_CASC_OUT;
    static constexpr int TP_MODIFY_MARGIN_OFFSET = ssr_params::BTP_MODIFY_MARGIN_OFFSET;
    static constexpr unsigned int TP_KERNEL_POSITION = ssr_params::BTP_KERNEL_POSITION;

    static constexpr unsigned int totalKernels = TP_CASC_LEN * TP_SSR * TP_SSR;

    using in2_type = port_conditional_array<input, (TP_DUAL_IP == 1), TP_SSR>;
    using out2_type = port_conditional_array<output, (TP_NUM_OUTPUTS == 2), TP_SSR>;
    using coeff_type = port_conditional_array<input, (TP_USE_COEFF_RELOAD == 1), TP_SSR>;
    using casc_in_type = port_conditional_array<input, (TP_CASC_IN == CASC_IN_TRUE), TP_SSR>;
    using net_type = typename std::array<std::array<std::array<connect<stream, stream>*, TP_CASC_LEN>, TP_SSR>, TP_SSR>;

    static int calculate_fifo_depth(int kernelPos) {
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
        const int baseFifoDepth = 32;
        unsigned int fifo_depth_multiple = 40;

        int fifoDepth = baseFifoDepth + fifo_depth_multiple * fifoStep;
        // limit size at a single memory bank - 8kB
        const int memBankSize = 2048; // 32-bit words
        int fifoDepthCap = fifoDepth < memBankSize ? fifoDepth : memBankSize;
        return fifoDepthCap;
    }

    static void create_and_recurse(kernel (&firKernels)[totalKernels], const std::vector<TT_COEFF>& taps) {
        // only pass a subset of the taps to the casc_kernels, and have it treat that as it normally would.
        std::vector<TT_COEFF> ssrPhaseTaps(segment_taps_array_for_phase(taps, ssrCoeffPhase));
        printf("creating kernels for dataphase %d coeffPhase %d range length %d\n\n", ssrDataPhase, ssrCoeffPhase,
               TP_FIR_RANGE_LEN);
        if
            constexpr(ssr_params::Bdim == (TP_SSR * TP_SSR) - 1) {
                printf(
                    "m_firKernels[range] corresponds to ssrOutputPath,ssrInnerPhase D(ssrDataPhase) C(ssrCoeffPhase) "
                    ":\n");
            }
        printf("m_firKernels[%d:%d] = %d,%d D(%d) C(%d) \n", kernelStartingIndex, kernelStartingIndex + TP_CASC_LEN - 1,
               ssrOutputPath, ssrInnerPhase, ssrDataPhase, ssrCoeffPhase);
        // pass the kernels from a specific index so cascades can be placed as normal.

        static_assert(TP_FIR_LEN % TP_SSR == 0, "TP_FIR LEN must be divisble by TP_SSR. "); //
        // static_assert(TP_USE_COEFF_RELOAD != 2 || (TP_FIR_LEN % TP_SSR == 0), "TP_FIR LEN must be divisble by TP_SSR,
        // at least for the header based coefficient reaload."); //
        this_casc_kernels::create_and_recurse(firKernels + kernelStartingIndex, ssrPhaseTaps);

        if
            constexpr(ssr_params::Bdim > 0) { next_ssr_kernels::create_and_recurse(firKernels, taps); }
    }

    static void create_and_recurse(kernel (&firKernels)[totalKernels]) {
        // pass the kernels from a specific index so cascades can be placed as "normal" from that position.
        this_casc_kernels::create_and_recurse(firKernels + kernelStartingIndex);

        if
            constexpr(ssr_params::Bdim > 0) { next_ssr_kernels::create_and_recurse(firKernels); }
    }

    static void create_connections(kernel* m_firKernels,
                                   port<input>* in,
                                   in2_type(&in2),
                                   port<output>* out,
                                   out2_type(&out2),
                                   coeff_type coeff,
                                   net_type& net,
                                   net_type& net2,
                                   casc_in_type(&casc_in),
                                   const char* srcFileName = "fir_sr_asym.cpp") {
        printf("ssrOutputPath, ssrInnerPhase : D(ssrDataPhase) C(ssrCoeffPhase) Starts at kernelStartingIndex: \n");
        for (unsigned int ssrOutputPath = 0; ssrOutputPath < TP_SSR; ssrOutputPath++) {
            for (unsigned int ssrInnerPhase = 0; ssrInnerPhase < TP_SSR; ssrInnerPhase++) {
                unsigned int ssrDataPhase = getSSRDataPhase(ssrOutputPath, ssrInnerPhase, TP_SSR);
                unsigned int ssrCoeffPhase = getSSRCoeffPhase(ssrOutputPath, ssrInnerPhase, TP_SSR);
                unsigned int kernelStartingIndex =
                    getKernelStartingIndex(ssrDataPhase, ssrOutputPath, TP_SSR, TP_CASC_LEN);
                kernel* firstKernelOfCascadeChain = m_firKernels + kernelStartingIndex;
                kernel* lastKernelOfCascadeChain = m_firKernels + kernelStartingIndex + TP_CASC_LEN - 1;
                unsigned int nextChainKernelStartingIndex = getKernelStartingIndex(
                    getSSRDataPhase(ssrOutputPath, ssrInnerPhase + 1, TP_SSR), ssrOutputPath, TP_SSR, TP_CASC_LEN);
                kernel* firstKernelOfNextCascadeChain = m_firKernels + nextChainKernelStartingIndex;

                printf("%d, %d : D(%d) C(%d) Starts at %d\n", ssrOutputPath, ssrInnerPhase, ssrDataPhase, ssrCoeffPhase,
                       kernelStartingIndex);
                input_connections(in[ssrDataPhase], firstKernelOfCascadeChain, ssrOutputPath, ssrInnerPhase, net);

                cascade_connections(firstKernelOfCascadeChain); // connect all cascades together

                // Connect SSR Inner phases cascade ports together.
                if (ssrInnerPhase != TP_SSR - 1) {
                    printf(
                        "cascade between ssrInnerPhase (%d) kernels. d(%d) c(%d)\nStartingIndex=%d, "
                        "nextChainKernelStartingIndex=%d, nextChainDataPhase=%d\n",
                        ssrInnerPhase, ssrDataPhase, ssrCoeffPhase, kernelStartingIndex, nextChainKernelStartingIndex,
                        getSSRDataPhase(ssrOutputPath, ssrInnerPhase + 1, TP_SSR));
                    connect<cascade>(lastKernelOfCascadeChain->out[0],
                                     firstKernelOfNextCascadeChain->in[CASC_IN_PORT_POS]);
                }

                if
                    constexpr(TP_DUAL_IP == DUAL_IP_DUAL) {
                        conditional_dual_ip_connections(in2[ssrDataPhase], firstKernelOfCascadeChain, ssrOutputPath,
                                                        ssrInnerPhase, net2);
                    }

                if
                    constexpr(TP_USE_COEFF_RELOAD == USE_COEFF_RELOAD_TRUE && TP_CASC_IN == CASC_IN_FALSE) {
                        // Currently only first kernels in chain are configured with RTP port.
                        // Subsequent kernels in cascade chain expect it's coneffs to be passed through the cascade IF.

                        if (ssrInnerPhase == 0) {
                            conditional_rtp_connections(coeff[ssrOutputPath], firstKernelOfCascadeChain);
                        }
                    }

                if (ssrInnerPhase == 0)
                    if
                        constexpr(TP_CASC_IN == CASC_IN_TRUE) {
                            conditional_casc_in_connections(casc_in[ssrOutputPath], firstKernelOfCascadeChain);
                        }

                // Final inner phase produces output.
                if (ssrInnerPhase == TP_SSR - 1) {
                    output_connections(out[ssrOutputPath], firstKernelOfCascadeChain);
                    if
                        constexpr(TP_NUM_OUTPUTS == 2) {
                            conditional_out_connections(out2[ssrOutputPath], firstKernelOfCascadeChain);
                        }
                }

                for (int i = 0; i < TP_CASC_LEN; i++) {
                    // Specify mapping constraints
                    runtime<ratio>(m_firKernels[kernelStartingIndex + i]) = 0.8;
                    // Source files
                    source(m_firKernels[kernelStartingIndex + i]) = srcFileName;
                }
            }
        }
    };

    static constexpr unsigned int getSSRDataPhase(unsigned int ssrOutputPath,
                                                  unsigned int ssrInnerPhase,
                                                  unsigned int SSR) {
        if (COEFF_DATA_ALIGNMENT == 0) {
            // aligned by coefficients
            return (ssrOutputPath * kDF + (SSR - ssrInnerPhase) - ((kIF - 1) * ssrOutputPath) + CEIL(kIF, SSR)) % SSR;
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
            int diffFromIF = (kIF - 1) * ssrInnerPhase;
            return (ssrOutputPath * kDF + (SSR - ssrInnerPhase) + (CEIL(diffFromIF, SSR) - diffFromIF)) % SSR;
            // return (ssrOutputPath * kDF + (SSR - ssrInnerPhase) - ((kIF-1)*ssrInnerPhase) + CEIL(kIF, SSR)) % SSR;
        }
    }
    static constexpr unsigned int getKernelStartingIndex(unsigned int ssrDataPhase,
                                                         unsigned int ssrOutputPath,
                                                         unsigned int SSR,
                                                         unsigned int CASCADE_LENGTH) {
        return ssrDataPhase * CASCADE_LENGTH + ssrOutputPath * SSR * CASCADE_LENGTH;
    }

    static std::vector<TT_COEFF> convert_sym_taps_to_asym(unsigned int fLen, const std::vector<TT_COEFF>& taps) {
        std::vector<TT_COEFF> ssrTapsRange; //
        for (unsigned int i = 0; i < CEIL(fLen, TP_SSR); i++) {
            unsigned int coefIndex = i;
            if (coefIndex < fLen) {
                if (coefIndex >= fLen / 2) {
                    coefIndex = fLen - coefIndex - 1;
                }
                ssrTapsRange.push_back(taps.at(coefIndex));
            } else {
                // padding
                ssrTapsRange.push_back(fir::nullElem<TT_COEFF>());
            }
        }
        return ssrTapsRange;
    }

   private:
    static_assert(ssr_params::Bdim >= 0, "ERROR: ssr_params::Bdim must be a positive integer");
    // static_assert(TP_FIR_LEN %  TP_SSR == 0, "ERROR: TP_FIR_LEN must be an integer multiple of TP_SSR. Please ceil up
    // TP_FIR_LEN" );
    static constexpr int tmp_rnd = TP_SSR * TP_DECIMATE_FACTOR * TP_INTERPOLATE_FACTOR;

    struct tmp_flen_params : public ssr_params {
        static constexpr int BTP_FIR_LEN = CEIL(TP_FIR_LEN, tmp_rnd) / (TP_SSR);
    };
    struct tmp_clen_params : public ssr_params {
        static constexpr int BTP_CASC_LEN =
            CEIL(TP_FIR_LEN, tmp_rnd) /
            (TP_SSR * TP_DECIMATE_FACTOR * TP_INTERPOLATE_FACTOR); // arbitrarily choosing casc len to be large enough
                                                                   // to not trigger static assert. 4 is chosen as the
                                                                   // number of taps per kernel
        static constexpr int BTP_FIR_RANGE_LEN = TP_DECIMATE_FACTOR * TP_INTERPOLATE_FACTOR;
    };
    static constexpr unsigned int kDF = fir_type<tmp_flen_params>::getDF();
    static constexpr unsigned int kIF = fir_type<tmp_flen_params>::getIF();
    static constexpr unsigned int ssrOutputPath = (dim / TP_SSR);
    static constexpr unsigned int ssrInnerPhase = (dim % TP_SSR);

    // aligned by coefficients
    static constexpr unsigned int ssrDataPhase = getSSRDataPhase(ssrOutputPath, ssrInnerPhase, ssr_params::BTP_SSR);
    static constexpr unsigned int ssrCoeffPhase = getSSRCoeffPhase(ssrOutputPath, ssrInnerPhase, ssr_params::BTP_SSR);

    // static constexpr unsigned int FIR_LEN_SSR_RANGE = (CEIL(ssr_params::BTP_FIR_LEN,
    // ssr_params::BTP_SSR)/ssr_params::BTP_SSR); // to be ceiled or something
    static constexpr unsigned int firTypeTapLenPerPhase = fir_type<tmp_clen_params>::getTapLen();
    static constexpr unsigned int kernelStartingIndex =
        getKernelStartingIndex(ssrDataPhase, ssrOutputPath, ssr_params::BTP_SSR, ssr_params::BTP_CASC_LEN);

    // helpers to create connections
    static constexpr unsigned int CASC_IN_PORT_POS = (ssr_params::BTP_DUAL_IP == DUAL_IP_DUAL) ? 2 : 1;
    static constexpr unsigned int RTP_PORT_POS =
        (ssr_params::BTP_CASC_IN == CASC_IN_TRUE || ssr_params::BTP_KERNEL_POSITION != 0) ? CASC_IN_PORT_POS + 1
                                                                                          : CASC_IN_PORT_POS;
    static constexpr unsigned int DUAL_OUT_PORT_POS = 1;
    static constexpr unsigned int DUAL_IP_PORT_POS = 1;
    static constexpr unsigned int INPUT_WINDOW_BYTESIZE =
        ssr_params::BTP_INPUT_WINDOW_VSIZE * sizeof(typename ssr_params::BTT_DATA);
    static constexpr unsigned int OUTPUT_WINDOW_BYTESIZE =
        TP_INTERPOLATE_FACTOR * INPUT_WINDOW_BYTESIZE / TP_DECIMATE_FACTOR;
    static constexpr unsigned int MARGIN_BYTESIZE =
        fir_type<tmp_clen_params>::getSSRMargin() * sizeof(typename ssr_params::BTT_DATA);

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

    static constexpr unsigned int firstRowDataPhase =
        (CEIL((ssrDataPhase * TP_INTERPOLATE_FACTOR), kDF) /
         kDF); // calculates first row that the first data of this dataPhase is used
    static constexpr int emptyRowsInOutputPath =
        (firstRowDataPhase / TP_SSR) + (ssrOutputPath < (firstRowDataPhase % TP_SSR));
    // static constexpr int MODIFY_MARGIN_OFFSET = (ssrOutputPath * kDF < ssrDataPhase * kIF) ? -1 * kDF *
    // (emptyRowsInOutputPath) + ((ssrOutputPath + TP_SSR) * kDF - ssrDataPhase * kIF)/TP_SSR:
    //                                            (ssrOutputPath * kDF - ssrInnerPhase * kIF)/TP_SSR;
    static constexpr int posFirstDataOfPhase = ssrOutputPath * kDF - ssrDataPhase * kIF;
    static constexpr int MODIFY_MARGIN_OFFSET = (posFirstDataOfPhase < 0)
                                                    ? -1 * (CEIL((-1 * posFirstDataOfPhase), TP_SSR) / TP_SSR)
                                                    : (posFirstDataOfPhase) / TP_SSR;
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
    static std::vector<typename ssr_params::BTT_COEFF> segment_taps_array_for_phase(
        const std::vector<typename ssr_params::BTT_COEFF>& taps, const unsigned int coeffPhase) {
        std::vector<typename ssr_params::BTT_COEFF> ssrTapsRange; //
        for (unsigned int i = 0; i < firTypeTapLenPerPhase; i++) {
            unsigned int coefIndex = i * ssr_params::BTP_SSR + coeffPhase;
            if (coefIndex < ssr_params::BTP_FIR_LEN) {
                ssrTapsRange.push_back(taps.at(coefIndex));

            } else {
                // padding
                ssrTapsRange.push_back(fir::nullElem<typename ssr_params::BTT_COEFF>());
            }
        }

        //(firstTap, lastTap);

        return ssrTapsRange;
    }

    // middle kernels connect to eachother.
    static constexpr bool casc_in = (ssrInnerPhase == 0) ? (false || TP_CASC_IN) : true; // first kernel of output phase
    static constexpr bool casc_out =
        (ssrInnerPhase == TP_SSR - 1) ? (false || TP_CASC_OUT) : true; // last kernel of output phase
    static constexpr int rnd = TP_SSR * kDF * kIF;
    struct last_casc_params : public ssr_params {
        static constexpr int Bdim = TP_CASC_LEN - 1;
        static constexpr int BTP_FIR_LEN = CEIL(TP_FIR_LEN, rnd) / TP_SSR;
        static constexpr int BTP_CASC_IN = casc_in;
        static constexpr int BTP_CASC_OUT = casc_out;
        static constexpr int BTP_COEFF_PHASE = ssrCoeffPhase;
        static constexpr int BTP_COEFF_PHASES = TP_SSR;
        static constexpr int BTP_COEFF_PHASES_LEN = TP_COEFF_PHASES_LEN;
        static constexpr int BTP_MODIFY_MARGIN_OFFSET = MODIFY_MARGIN_OFFSET;
    };

    using this_casc_kernels = casc_kernels<last_casc_params, fir_type>;

    static constexpr int nextSSRBdim = ssr_params::Bdim - 1;

    // need to create parameters for downstream kernels, e.g. dim.
    // inherit and overwrite non-defaults
    struct nextSSRKernels_params : public ssr_params {
        static constexpr int Bdim = nextSSRBdim;
    };

    // recursive call with updated params
    using next_ssr_kernels = ssr_kernels<nextSSRKernels_params, fir_type>;

    // make connections for a
    static void input_connections(
        port<input>(&in), kernel firKernels[TP_CASC_LEN], int ssrOutPathIndex, int ssrInPhaseIndex, net_type& net) {
        // printParams<ssr_params>();
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
    static void cascade_connections(kernel firKernels[TP_CASC_LEN]) {
        for (int i = 1; i < TP_CASC_LEN; i++) {
            connect<cascade>(firKernels[i - 1].out[0], firKernels[i].in[CASC_IN_PORT_POS]);
        }
    }

    // make output connections
    static void output_connections(port<output>(&out), kernel firKernels[TP_CASC_LEN]) {
        if (TP_CASC_OUT == CASC_OUT_TRUE) {
            connect<cascade>(firKernels[TP_CASC_LEN - 1].out[0], out);
        } else {
            if (TP_API == USE_WINDOW_API) {
                connect<window<OUTPUT_WINDOW_BYTESIZE> >(firKernels[TP_CASC_LEN - 1].out[0], out);
            } else {
                connect<stream>(firKernels[TP_CASC_LEN - 1].out[0], out);
            }
        }
    }

    // make dual input connections
    static void conditional_dual_ip_connections(
        port<input>(&in2), kernel firKernels[TP_CASC_LEN], int ssrOutPathIndex, int ssrInPhaseIndex, net_type& net2) {
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
    static void conditional_casc_in_connections(port<input>(&casc_in), kernel firKernels[TP_CASC_LEN]) {
        if
            constexpr(TP_CASC_IN == CASC_IN_TRUE) { connect<cascade>(casc_in, firKernels[0].in[CASC_IN_PORT_POS]); }
    }

    // make output connections
    static void conditional_out_connections(port<output>(&out2), kernel firKernels[TP_CASC_LEN]) {
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
    static void conditional_rtp_connections(port<input>(&coeff), kernel firKernels[TP_CASC_LEN]) {
        if
            constexpr(TP_USE_COEFF_RELOAD == USE_COEFF_RELOAD_TRUE) {
                connect<parameter>(coeff, async(firKernels[0].in[RTP_PORT_POS]));
            }
    }
};

template <int T_FIR_LEN, int kMaxTaps>
static constexpr unsigned int getMinCascLen() {
    return T_FIR_LEN % kMaxTaps == 0 ? T_FIR_LEN / kMaxTaps : T_FIR_LEN / kMaxTaps + 1;
};

template <int kMaxTaps, int kRawOptTaps, int T_FIR_LEN, int SSR = 1>
constexpr unsigned int getOptCascLen() {
    constexpr int kOptTaps = kRawOptTaps < kMaxTaps ? kRawOptTaps : kMaxTaps;
    constexpr int firLenPerSSR = T_FIR_LEN / SSR;
    return (firLenPerSSR) % kOptTaps == 0 ? (firLenPerSSR) / kOptTaps : (firLenPerSSR) / kOptTaps + 1;
};

/**
 * @endcond
 */

/**
 * @defgroup graph_utils Graph utils
 *
 * The Graphs utilities contain helper funcitons and classes that ease usage of library elements.
 *
 */

//--------------------------------------------------------------------------------------------------
// convert_sym_taps_to_asym
//--------------------------------------------------------------------------------------------------
/**
 * @ingroup graph_utils
 *
 * @brief convert_sym_taps_to_asym is an helper function to convert users input coefficient array. \n
 * Function creates an asymmetric array in the area provided from a symmetric one. \n
 * Function can be used when run-time programmable coefficients are being passed to the FIR graph, \n
 * using the graph's class ``update()`` method.
 * @tparam TT_COEFF describes the type of individual coefficients of the filter taps.
 * @param[out] tapsOut  a pointer to the output taps array of uncompressed (flen) size.
 * @param[in] fLen  input argument defining the size of the uncompressed array.
 * @param[in] tapsIn pointer to the input taps array of compressed (fLen+1)/2 size.
 */
template <typename TT_COEFF>
void convert_sym_taps_to_asym(TT_COEFF* tapsOut, unsigned int fLen, TT_COEFF* tapsIn) {
    for (unsigned int i = 0; i < fLen; i++) {
        unsigned int coefIndex = i;
        if (coefIndex >= fLen / 2) {
            coefIndex = fLen - coefIndex - 1;
        }
        tapsOut[i] = tapsIn[coefIndex];
    }
};

//--------------------------------------------------------------------------------------------------
// convert_hb_taps_to_asym
//--------------------------------------------------------------------------------------------------
/**
 * @ingroup graph_utils
 *
 * @brief convert_hb_taps_to_asym is an helper function to convert users input coefficient array. \n
 * Function can be used when run-time programmable coefficients are being passed to the FIR graph, \n
 * using the graph's class ``update()`` method. \n
 * \n
 * HB taps arrays are compressed arrays of taps with the center tap at the end,  \n
 * with a length of: ``hbFirLen = (FIR Length + 1) / 4 + 1``.
 * When converting to Asym, we want to convert the symmetric taps to asymmetric,
 * but it's useful to have the center tap at the end,
 * (since it is processed by separate polyphase lane and is offloaded to a separate, dedicated kernel). \n
 * However for SSR cases, the array needs to be padded with zeros to a multiple of SSR factor. \n
 * For example, for a FIR Length of 7, where coeffs are: ``1, 0, 2, 5, 2, 0, 1`` \n
 * ``tapsIn: 1, 2, 5`` \n
 * ``hbFirLen: 3`` \n
 * For SSR: 1, \n
 * ``tapsOut: 1, 2, 2, 1, 5`` \n
 * For SSR: 3 \n
 * ``tapsOut: 1, 2, 2, 1, 0, 0, 5`` \n
 *
 * @tparam TT_COEFF describes the type of individual coefficients of the filter taps.
 * @param[out] tapsOut  a pointer to the output taps array of uncompressed (flen) size.
 * @param[in] hbFirLen  input argument defining the size of the uncompressed array.
 * @param[in] tapsIn pointer to the input taps array of compressed (fLen+1)/2 size.
 * @param[in] ssr ssr parameter
 */
template <typename TT_COEFF>
void convert_hb_taps_to_asym(TT_COEFF* tapsOut, unsigned int hbFirLen, TT_COEFF* tapsIn, unsigned int ssr) {
    int symFLen = (hbFirLen - 1) * 2; // (FIR_LEN+1)/2

    for (unsigned int i = 0; i < CEIL(symFLen, ssr); i++) {
        unsigned int coefIndex = i;
        if (coefIndex < symFLen) {
            if (coefIndex >= symFLen / 2) {
                coefIndex = symFLen - coefIndex - 1;
            }
            tapsOut[i] = tapsIn[coefIndex];
        } else {
            tapsOut[i] = fir::nullElem<TT_COEFF>();
        }
    }
    tapsOut[CEIL(symFLen, ssr)] = tapsIn[(symFLen + 1) / 2];
};
}
}
} // namespace braces
#endif //_DSPLIB_FIR_GRAPH_UTILS_HPP_
