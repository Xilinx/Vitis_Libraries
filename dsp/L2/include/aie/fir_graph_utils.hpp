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
#ifndef _DSPLIB_FIR_GRAPH_UTILS_HPP_
#define _DSPLIB_FIR_GRAPH_UTILS_HPP_
/*
The file captures the definition of the graph utilities commonly used across various library elements
*/

#include <adf.h>
#include <vector>

#include "device_defs.h"
#include "fir_utils.hpp"
#include "graph_utils.hpp"

// #define _DSPLIB_FIR_GRAPH_UTILS_DEBUG_

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
    static constexpr unsigned int nextKernelRange = fir_type<casc_params>::template getKernelFirRangeLen<nextBdim>();

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

    template <int kernelFirRangeLen, int kernelPosition, int cascLen>
    struct kernelPositionParams : public casc_params {
        static constexpr int BTP_FIR_RANGE_LEN = kernelFirRangeLen;
        static constexpr int BTP_KERNEL_POSITION = kernelPosition;
        static constexpr int BTP_CASC_LEN = cascLen;
    };

    static constexpr unsigned int fnFirRange(unsigned int TP_FL, unsigned int TP_CL, int TP_KP, int TP_Rnd = 1) {
        // TP_FL - FIR Length, TP_CL - Cascade Length, TP_KP - Kernel Position
        return ((fir::fnTrunc(TP_FL, TP_Rnd * TP_CL) / TP_CL) +
                ((TP_FL - fir::fnTrunc(TP_FL, TP_Rnd * TP_CL)) >= TP_Rnd * (TP_KP + 1) ? TP_Rnd : 0));
    }

    static constexpr unsigned int fnFirRangeRem(unsigned int TP_FL, unsigned int TP_CL, int TP_KP, int TP_Rnd = 1) {
        // TP_FL - FIR Length, TP_CL - Cascade Length, TP_KP - Kernel Position
        // this is for last in the cascade
        return ((fir::fnTrunc(TP_FL, TP_Rnd * TP_CL) / TP_CL) +
                ((TP_FL - fir::fnTrunc(TP_FL, TP_Rnd * TP_CL)) % TP_Rnd));
    }
    static constexpr unsigned int fnFirRangeOffset(
        unsigned int TP_FL, unsigned int TP_CL, int TP_KP, int TP_Rnd = 1, int TP_Sym = 1) {
        // TP_FL - FIR Length, TP_CL - Cascade Length, TP_KP - Kernel Position
        return (TP_KP * (fir::fnTrunc(TP_FL, TP_Rnd * TP_CL) / TP_CL) +
                ((TP_FL - fir::fnTrunc(TP_FL, TP_Rnd * TP_CL)) >= TP_Rnd * TP_KP
                     ? TP_Rnd * TP_KP
                     : (fir::fnTrunc(TP_FL, TP_Rnd) - fir::fnTrunc(TP_FL, TP_Rnd * TP_CL)))) /
               TP_Sym;
    }

#if __HAS_ACCUM_PERMUTES__ == 1
    // cint16/int16 combo can be overloaded with 2 column MUL/MACs.
    static constexpr unsigned int tdmColumnMultiple = (std::is_same<typename casc_params::BTT_DATA, cint16>::value &&
                                                       std::is_same<typename casc_params::BTT_COEFF, int16>::value)
                                                          ? 2
                                                          : 1;
#else
    static constexpr unsigned int tdmColumnMultiple = 1;
#endif

    static constexpr unsigned int getKernelFirRangeOffset(int pos) {
        unsigned int firRangeOffset =
            fnFirRangeOffset(casc_params::BTP_FIR_LEN, casc_params::BTP_CASC_LEN, pos, tdmColumnMultiple);
        return firRangeOffset;
    };

   public:
    static constexpr unsigned int getKernelFirRangeLen(int pos) {
        unsigned int firRangeLen =
            pos + 1 == casc_params::BTP_CASC_LEN
                ? fnFirRangeRem(casc_params::BTP_FIR_LEN, casc_params::BTP_CASC_LEN, pos, tdmColumnMultiple)
                : fnFirRange(casc_params::BTP_FIR_LEN, casc_params::BTP_CASC_LEN, pos, tdmColumnMultiple);
        return firRangeLen;
    };
    /**
     * @brief Separate taps into a specific cascade segment. Only used for TDM FIR (other FIRs - kernels take the full
     * array anyway)
     */
    template <unsigned int kernelPosition, unsigned int cascLen>
    static std::vector<typename casc_params::BTT_COEFF> segment_taps_array_for_cascade(
        const std::vector<typename casc_params::BTT_COEFF>& taps) {
        // internal struct of the method
        static constexpr unsigned int kernelFirRangeLen =
            fir_type<casc_params>::template getKernelFirRangeLen<kernelPosition>();
        using thisPositionParams = kernelPositionParams<kernelFirRangeLen, kernelPosition, cascLen>;

        // FIR Length divided by cascade length and at this point also by SSR
        // TDM doesn't require full array for each cascaded kernels, in addition to requiring a fraction of total array
        // for SSR purposes
        static constexpr unsigned int tdmTapOffsetPerPhase =
            fir_type<thisPositionParams>::getFirCoeffOffset(); // in the range of 0 to FL-1
        // lanes
        static constexpr unsigned int lanes = fir_type<thisPositionParams>::getLanes();
        static constexpr unsigned int firTapLanes = fir_type<thisPositionParams>::getFirRangeLen() * lanes;
        static constexpr unsigned int firTapChannels = CEIL(thisPositionParams::BTP_TDM_CHANNELS, lanes) / lanes;

        static constexpr unsigned int firTapOffsetPerPhase =
            thisPositionParams::BTP_TDM_CHANNELS == 1 ? 0 : tdmTapOffsetPerPhase * lanes;
        std::vector<typename casc_params::BTT_COEFF> cascTapsRange; //
        for (unsigned int j = 0; j < firTapChannels; j++) {
            for (unsigned int i = 0; i < firTapLanes; i++) {
                unsigned int tdmCascOffset = firTapOffsetPerPhase + j * lanes * thisPositionParams::BTP_FIR_LEN;
                unsigned int coefIndex = i + tdmCascOffset;
                // if (coefIndex < taps.size()) {
                if (coefIndex < (thisPositionParams::BTP_FIR_LEN * thisPositionParams::BTP_TDM_CHANNELS)) {
                    cascTapsRange.push_back(taps.at(coefIndex));

                } else {
                    // padding
                    cascTapsRange.push_back(fir::nullElem<typename casc_params::BTT_COEFF>());
                }
            }
        }

        // #undef _DSPLIB_FIR_GRAPH_UTILS_DEBUG_

        return cascTapsRange;
    }

    /**
     * @brief Separate taps into a specific cascade segment. Only used for TDM FIR (other FIRs - kernels take the full
     * array anyway)
     */
    static std::vector<typename casc_params::BTT_COEFF> segment_taps_array_for_cascade(
        const std::vector<typename casc_params::BTT_COEFF>& taps, unsigned int kernelNo, unsigned int cascLen) {
        // 8 tap, 4 tap per kernel. 16 channels. 128 taps in total, 64 taps each kernel
        // kernel (1) -> 0  - 31, 64 - 95
        // kernel (0) -> 32 - 63, 96 - 127

        // Kernel position in the cascade chain is reversed. Recursive calls fill the data from the last kernel
        // unsigned int kernelPosition =  cascLen - kernelNo - 1;
        unsigned int kernelPosition = kernelNo;
        unsigned int kernelFirRangeLen = getKernelFirRangeLen(kernelPosition);
        unsigned int kernelFirRangeOffset = getKernelFirRangeOffset(kernelPosition);

        // FIR Length divided by cascade length and at this point also by SSR
        // TDM doesn't require full array for each cascaded kernels, in addition to requiring a fraction of total array
        // for SSR purposes
        unsigned int tdmTapOffsetPerPhase =
            casc_params::BTP_FIR_LEN - kernelFirRangeLen - kernelFirRangeOffset; // in the range of 0 to FL-1
        // lanes
        unsigned int lanes = fir_type<casc_params>::getLanes();
        unsigned int firTapLanes = kernelFirRangeLen * lanes;
        unsigned int firTapChannels = CEIL(casc_params::BTP_TDM_CHANNELS, lanes) / lanes;

        unsigned int firTapOffsetPerPhase = casc_params::BTP_TDM_CHANNELS == 1 ? 0 : tdmTapOffsetPerPhase * lanes;
        std::vector<typename casc_params::BTT_COEFF> cascTapsRange; //
        for (unsigned int j = 0; j < firTapChannels; j++) {
            for (unsigned int i = 0; i < firTapLanes; i++) {
                unsigned int tdmCascOffset = firTapOffsetPerPhase + j * lanes * casc_params::BTP_FIR_LEN;
                unsigned int coefIndex = i + tdmCascOffset;
                // if (coefIndex < taps.size()) {
                if (coefIndex < (casc_params::BTP_FIR_LEN * casc_params::BTP_TDM_CHANNELS)) {
                    cascTapsRange.push_back(taps.at(coefIndex));

                } else {
                    // padding
                    cascTapsRange.push_back(fir::nullElem<typename casc_params::BTT_COEFF>());
                }
            }
        }

        // #undef _DSPLIB_FIR_GRAPH_UTILS_DEBUG_

        return cascTapsRange;
    }
    // #undef _DSPLIB_FIR_GRAPH_UTILS_DEBUG_

    static void create_and_recurse(kernel firKernels[casc_params::BTP_CASC_LEN],
                                   const std::vector<typename casc_params::BTT_COEFF>& taps) {
        using namespace fir;
        if
            constexpr(thisKernelClass::getFirType() == eFIRVariant::kSrAsym) {
                firKernels[casc_params::Bdim] = kernel::create_object<thisKernelParentClass>(taps);
            }
        else if
            constexpr(thisKernelClass::getFirType() == eFIRVariant::kSrSym) {
                firKernels[casc_params::Bdim] = kernel::create_object<thisKernelParentClass>(taps);
            }
        else if
            constexpr(thisKernelClass::getFirType() == eFIRVariant::kResamp) {
                firKernels[casc_params::Bdim] = kernel::create_object<thisKernelParentClass>(taps);
            }
        else if
            constexpr(thisKernelClass::getFirType() == eFIRVariant::kIntHB) {
                firKernels[casc_params::Bdim] = kernel::create_object<thisKernelParentClass>(taps);
            }
        else if
            constexpr(thisKernelClass::getFirType() == eFIRVariant::kIntAsym) {
                firKernels[casc_params::Bdim] = kernel::create_object<thisKernelParentClass>(taps);
            }
        else if
            constexpr(thisKernelClass::getFirType() == eFIRVariant::kDecHB) {
                firKernels[casc_params::Bdim] = kernel::create_object<thisKernelParentClass>(taps);
            }
        else if
            constexpr(thisKernelClass::getFirType() == eFIRVariant::kDecAsym) {
                firKernels[casc_params::Bdim] = kernel::create_object<thisKernelParentClass>(taps);
            }
        else if
            constexpr(thisKernelClass::getFirType() == eFIRVariant::kTDM) {
                std::array<typename casc_params::BTT_DATA, thisKernelClass::getInternalBufferSize()> internalBuffer{};
                std::vector<typename casc_params::BTT_COEFF> cascTaps(
                    segment_taps_array_for_cascade<casc_params::Bdim, casc_params::BTP_CASC_LEN>(taps));

                firKernels[casc_params::Bdim] = kernel::create_object<thisKernelParentClass>(cascTaps, internalBuffer);
            }
        if
            constexpr(casc_params::Bdim != 0) { next_casc_kernels::create_and_recurse(firKernels, taps); }
    }
    static void create_and_recurse(kernel firKernels[casc_params::BTP_CASC_LEN]) {
        using namespace fir;
        if
            constexpr(thisKernelClass::getFirType() == eFIRVariant::kSrAsym) {
                firKernels[casc_params::Bdim] = kernel::create_object<thisKernelParentClass>();
            }
        else if
            constexpr(thisKernelClass::getFirType() == eFIRVariant::kSrSym) {
                firKernels[casc_params::Bdim] = kernel::create_object<thisKernelParentClass>();
            }
        else if
            constexpr(thisKernelClass::getFirType() == eFIRVariant::kResamp) {
                firKernels[casc_params::Bdim] = kernel::create_object<thisKernelParentClass>();
            }
        else if
            constexpr(thisKernelClass::getFirType() == eFIRVariant::kIntHB) {
                firKernels[casc_params::Bdim] = kernel::create_object<thisKernelParentClass>();
            }
        else if
            constexpr(thisKernelClass::getFirType() == eFIRVariant::kIntAsym) {
                firKernels[casc_params::Bdim] = kernel::create_object<thisKernelParentClass>();
            }
        else if
            constexpr(thisKernelClass::getFirType() == eFIRVariant::kDecHB) {
                firKernels[casc_params::Bdim] = kernel::create_object<thisKernelParentClass>();
            }
        else if
            constexpr(thisKernelClass::getFirType() == eFIRVariant::kDecAsym) {
                firKernels[casc_params::Bdim] = kernel::create_object<thisKernelParentClass>();
            }
        else if
            constexpr(thisKernelClass::getFirType() == eFIRVariant::kTDM) {
                std::array<typename casc_params::BTT_DATA, thisKernelClass::getInternalBufferSize()> internalBuffer{};

                firKernels[casc_params::Bdim] = kernel::create_object<thisKernelParentClass>(internalBuffer);
            }
        if
            constexpr(casc_params::Bdim != 0) { next_casc_kernels::create_and_recurse(firKernels); }
    }
};

template <typename ssr_params = fir_params_defaults, template <typename> typename fir_type = fir_type_default>
class ssr_kernels {
   public:
    using TT_DATA = typename ssr_params::BTT_DATA;
    using TT_OUT_DATA = typename ssr_params::BTT_OUT_DATA;
    using TT_COEFF = typename ssr_params::BTT_COEFF;
    static constexpr unsigned int dim = ssr_params::Bdim;
    static constexpr unsigned int TP_FIR_LEN = ssr_params::BTP_FIR_LEN;
    static constexpr unsigned int TP_TDM_CHANNELS = ssr_params::BTP_TDM_CHANNELS;
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
    static constexpr unsigned int TP_PARA_DECI_INDEX = ssr_params::BTP_PARA_DECI_INDEX;
    static constexpr unsigned int TP_PARA_INTERP_INDEX = ssr_params::BTP_PARA_INTERP_INDEX;
    static constexpr unsigned int TP_PARA_DECI_POLY = ssr_params::BTP_PARA_DECI_POLY;
    static constexpr unsigned int TP_PARA_INTERP_POLY =
        ssr_params::BTP_PARA_INTERP_POLY; // ssr_kernels class is already decomposed, so should not have any need for
                                          // decomposition polyphases.
    static constexpr bool TP_CASC_IN = ssr_params::BTP_CASC_IN;
    static constexpr bool TP_CASC_OUT = ssr_params::BTP_CASC_OUT;
    // 0 - default: decompose to array of TP_SSR^2; 1 - decompose to a vector of TP_SSR, where kernels form independent
    // paths; otherwise set it to 1.
    static constexpr unsigned int isSSRaVector = ssr_params::BTP_SSR_MODE;
    static constexpr unsigned int SSRSize =
        ssr_params::BTP_SSR_MODE == 0 ? (TP_SSR * TP_SSR) : ssr_params::BTP_SSR_MODE == 1 ? TP_SSR : 1;
    static constexpr unsigned int totalKernels = TP_CASC_LEN * SSRSize;

    using in2_type = port_conditional_array<input, (TP_DUAL_IP == 1), TP_SSR>;
    using out2_type = port_conditional_array<output, (TP_NUM_OUTPUTS == 2), TP_SSR>;
    using coeff_type = port_conditional_array<input, (TP_USE_COEFF_RELOAD == 1), TP_SSR>;
    using tdm_coeff_type = port_conditional_array<input, (TP_USE_COEFF_RELOAD == 1), TP_SSR * TP_CASC_LEN>;
    using casc_in_type = port_conditional_array<input, (TP_CASC_IN == CASC_IN_TRUE), TP_SSR>;
    using net_type = typename std::array<std::array<std::array<connect<stream, stream>*, TP_CASC_LEN>, TP_SSR>, TP_SSR>;

    static int calculate_fifo_depth(int kernelPos) {
        // In FIFO mode, FIFO size has to be a multiple of 128 bits.
        // In FIFO mode, BD length has to be >= 128 bytes!
        // Conservative assumptions need to be made here.
        // For an overall decimation factor, with increasing decimation factor,
        // the amount of input stream data required to produce output samples also increases.
        // This strains the FIFOs closer to the beginning of the chain comparably more
        // than the ones closest to the output.
        //
        // Conservative assumptions need to be made here, as mapper may place multiple buffers in
        // each of the memory banks, that may introduce Memory conflicts.
        // On top of that, the placement of input source wrt broadcast kernel inputs may introduce significant routing
        // delays.
        // which may have an adverse effect on the amount of FIFO storage available for filter design purposes.
        int fifoStep =
            (TP_INTERPOLATE_FACTOR <= TP_DECIMATE_FACTOR
                 ? (TP_CASC_LEN - kernelPos + 1) * CEIL(TP_INTERPOLATE_FACTOR, TP_DECIMATE_FACTOR) / TP_DECIMATE_FACTOR
                 : (kernelPos + 1) * CEIL(TP_DECIMATE_FACTOR, TP_INTERPOLATE_FACTOR) / TP_INTERPOLATE_FACTOR);

        const int baseFifoDepth = 32;
        // Using DMA as FIFO allows Memory Modules to be used to extend stream FIFOs, allowing greater buffering
        // capabilities on stream connections.
        constexpr unsigned int extendedDMAFifos = __SUPPORTS_DMA_FIFO__;
        unsigned int fifo_depth_multiple = extendedDMAFifos == 1 ? 32 : 16;

        int fifoDepth = baseFifoDepth + fifo_depth_multiple * fifoStep;
        // limit size at a single memory bank - 8kB
        const int memBankSize = 2048; // 32-bit words
        int fifoDepthCap = fifoDepth < memBankSize ? fifoDepth : memBankSize;
        return fifoDepthCap;
    }

    static void create_and_recurse(kernel (&firKernels)[totalKernels], const std::vector<TT_COEFF>& taps) {
        // only pass a subset of the taps to the casc_kernels, and have it treat that as it normally would.
        std::vector<TT_COEFF> ssrPhaseTaps(segment_taps_array_for_phase(taps, ssrCoeffPhase));
        unsigned int kernelStartingIndexInt = isSSRaVector == 1 ? ssrDataPhase * TP_CASC_LEN : kernelStartingIndex;

        static_assert((TP_FIR_LEN * TP_TDM_CHANNELS) % TP_SSR == 0, "TP_FIR LEN must be divisible by TP_SSR. "); //
        this_casc_kernels::create_and_recurse(firKernels + kernelStartingIndexInt, ssrPhaseTaps);

        if
            constexpr(ssr_params::Bdim > 0) { next_ssr_kernels::create_and_recurse(firKernels, taps); }
    }

    static void create_and_recurse(kernel (&firKernels)[totalKernels]) {
        // pass the kernels from a specific index so cascades can be placed as "normal" from that position.
        unsigned int kernelStartingIndexInt = isSSRaVector == 1 ? ssrDataPhase * TP_CASC_LEN : kernelStartingIndex;
        this_casc_kernels::create_and_recurse(firKernels + kernelStartingIndexInt);

        if
            constexpr(ssr_params::Bdim > 0) { next_ssr_kernels::create_and_recurse(firKernels); }
    }

    static void create_connections(kernel* m_firKernels,
                                   port<input>* in,
                                   in2_type(&in2),
                                   port<output>* out,
                                   out2_type out2,
                                   coeff_type coeff,
                                   net_type& net,
                                   net_type& net2,
                                   casc_in_type(&casc_in),
                                   const char* srcFileName = "fir_sr_asym.cpp") {
        for (unsigned int ssrOutputPath = 0; ssrOutputPath < TP_SSR; ssrOutputPath++) {
            for (unsigned int ssrInnerPhase = 0; ssrInnerPhase < TP_SSR; ssrInnerPhase++) {
                unsigned int ssrDataPhase = getSSRDataPhase(ssrOutputPath, ssrInnerPhase, TP_SSR);
                unsigned int kernelStartingIndex =
                    getKernelStartingIndex(ssrDataPhase, ssrOutputPath, TP_SSR, TP_CASC_LEN);
                kernel* firstKernelOfCascadeChain = m_firKernels + kernelStartingIndex;
                kernel* lastKernelOfCascadeChain = m_firKernels + kernelStartingIndex + TP_CASC_LEN - 1;
                unsigned int nextChainKernelStartingIndex = getKernelStartingIndex(
                    getSSRDataPhase(ssrOutputPath, ssrInnerPhase + 1, TP_SSR), ssrOutputPath, TP_SSR, TP_CASC_LEN);
                kernel* firstKernelOfNextCascadeChain = m_firKernels + nextChainKernelStartingIndex;

                input_connections(in[ssrDataPhase], firstKernelOfCascadeChain, ssrOutputPath, ssrInnerPhase, net);
                cascade_connections(firstKernelOfCascadeChain); // connect all cascades together

                // Connect SSR Inner phases cascade ports together.
                if (ssrInnerPhase != TP_SSR - 1) {
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
                        // Subsequent kernels in cascade chain expect its coeffs to be passed through the cascade IF.

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
    static void create_tdm_connections(kernel* m_firKernels,
                                       port<input>* in,
                                       in2_type(&in2),
                                       port<output>* out,
                                       out2_type out2,
                                       tdm_coeff_type coeff,
                                       net_type& net,
                                       net_type& net2,
                                       casc_in_type(&casc_in),
                                       const char* srcFileName = "fir_sr_asym.cpp") {
        for (unsigned int ssrInnerPhase = 0; ssrInnerPhase < TP_SSR; ssrInnerPhase++) {
            unsigned int ssrOutputPath = ssrInnerPhase;
            unsigned int ssrDataPhase = ssrInnerPhase;
            unsigned int kernelStartingIndex = ssrDataPhase * TP_CASC_LEN;
            kernel* firstKernelOfCascadeChain = m_firKernels + kernelStartingIndex;
            // printParams<ssr_params>();

            input_tdm_connections(in[ssrDataPhase], firstKernelOfCascadeChain, ssrOutputPath, ssrInnerPhase, net);

            // connect all cascades together
            cascade_tdm_connections(firstKernelOfCascadeChain);

            if
                constexpr(TP_DUAL_IP == DUAL_IP_DUAL) {
                    conditional_dual_ip_connections(in2[ssrDataPhase], firstKernelOfCascadeChain, ssrOutputPath,
                                                    ssrInnerPhase, net2);
                }

            if
                constexpr(TP_USE_COEFF_RELOAD == USE_COEFF_RELOAD_TRUE) {
                    for (int i = 0; i < TP_CASC_LEN; i++) {
                        unsigned int kernelIndex = ssrDataPhase * TP_CASC_LEN + i;
                        unsigned int rtpPosition = i == 0 ? 1 : 2;

                        kernel* kernelInCascadeChain = m_firKernels + kernelIndex;
                        printf("kernelIndex: %d \n", kernelIndex);

                        conditional_rtp_connections(coeff[kernelIndex], kernelInCascadeChain, rtpPosition);
                    }
                }

            // if (ssrInnerPhase == 0)
            //     if
            //         constexpr(TP_CASC_IN == CASC_IN_TRUE) {
            //             conditional_casc_in_connections(casc_in[ssrOutputPath], firstKernelOfCascadeChain);
            //         }

            // Each SSR path has an output.
            output_connections(out[ssrOutputPath], firstKernelOfCascadeChain);
            if
                constexpr(TP_NUM_OUTPUTS == 2) {
                    conditional_out_connections(out2[ssrOutputPath], firstKernelOfCascadeChain);
                }

            for (int i = 0; i < TP_CASC_LEN; i++) {
                // Specify mapping constraints
                runtime<ratio>(m_firKernels[kernelStartingIndex + i]) = 0.8;
                // Source files
                source(m_firKernels[kernelStartingIndex + i]) = srcFileName;
            }
        }
    };

#ifndef COEFF_DATA_ALIGNMENT
#define COEFF_DATA_ALIGNMENT 1 // data
#endif

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

    static std::vector<TT_COEFF> revert_taps(const std::vector<TT_COEFF>& taps) {
        std::vector<TT_COEFF> revertedTaps; //
        for (unsigned int i = 0; i < taps.size(); i++) {
            unsigned int coefIndex = taps.size() - i - 1;
            revertedTaps.push_back(taps.at(coefIndex));
        }
        return revertedTaps;
    }

   private:
    static_assert(ssr_params::Bdim >= 0, "ERROR: ssr_params::Bdim must be a positive integer");
    // static_assert(TP_FIR_LEN %  TP_SSR == 0, "ERROR: TP_FIR_LEN must be an integer multiple of TP_SSR. Please ceil up
    // TP_FIR_LEN" );

    static constexpr unsigned int kDF = fir_type<ssr_params>::getDF();
    static constexpr unsigned int kIF = fir_type<ssr_params>::getIF();
    static constexpr unsigned int ssrInnerPhase = (dim % TP_SSR);
    static constexpr unsigned int ssrOutputPath = isSSRaVector == 1 ? ssrInnerPhase : (dim / TP_SSR);

    // aligned by coefficients
    static constexpr unsigned int ssrDataPhase = getSSRDataPhase(ssrOutputPath, ssrInnerPhase, ssr_params::BTP_SSR);
    static constexpr unsigned int ssrCoeffPhase =
        isSSRaVector == 1 ? ssrInnerPhase
                          : ((getSSRCoeffPhase(ssrOutputPath, ssrInnerPhase, ssr_params::BTP_SSR)) +
                             ((TP_PARA_DECI_INDEX == 0) ? 0 : (TP_SSR - 1))) %
                                TP_SSR; //

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

    // Decomposed Inner Phase - takes position in the decomposed structure into account
    static constexpr unsigned int decompInnerPhases = TP_SSR * TP_PARA_DECI_POLY; // kDF * ???
    static constexpr unsigned int decompInnerSSRPhase = dim * TP_PARA_DECI_POLY;
    static constexpr unsigned int decompInnerDeciPhase = TP_PARA_DECI_INDEX;
    static constexpr unsigned int decompInnerInterpPhase = 0; // uused
    // ((TP_PARA_INTERP_INDEX * kDF * TP_PARA_DECI_POLY) / (kIF * TP_PARA_INTERP_POLY));
    static constexpr unsigned int decompInnerPhase = 0; //  unused
    // (decompInnerSSRPhase + decompInnerDeciPhase + decompInnerInterpPhase) % decompInnerPhases;
    static constexpr unsigned int decompInnerPhase2 =
        (dim * TP_PARA_DECI_POLY + (TP_PARA_DECI_POLY - TP_PARA_DECI_INDEX) % TP_PARA_DECI_POLY +
         (TP_PARA_INTERP_INDEX * kDF * TP_PARA_DECI_POLY) / (kIF * TP_PARA_INTERP_POLY)) %
        decompInnerPhases;
    // unsigned int inputDataIndex = (ssrIdx * TP_PARA_DECI_POLY +
    //                               (TP_PARA_DECI_POLY - deciPolyIdx) %
    //                                   (TP_PARA_DECI_POLY)
    //                                   + (interpPolyIdx * TP_DECIMATE_FACTOR) / TP_INTERPOLATE_FACTOR )%
    //                                   (TP_PARA_DECI_POLY * TP_SSR);
    // unsigned int inputDataIndex = ssrIdx * TP_PARA_DECI_POLY + (TP_PARA_DECI_POLY - deciPolyIdx + ((interpPolyIdx *
    // TP_DECIMATE_FACTOR) / TP_INTERPOLATE_FACTOR)) % TP_PARA_DECI_POLY;

    // Position of the first non-zero data sample in each SSR phase, relative to first SSR phase
    static constexpr int decompOutputPath =
        (ssrOutputPath)*kDF * TP_PARA_DECI_POLY +
        (TP_PARA_INTERP_INDEX * kDF * TP_PARA_DECI_POLY) / (kIF * TP_PARA_INTERP_POLY) % decompInnerPhases;
    static constexpr int posFirstDataOfPhase = decompOutputPath - decompInnerPhase2 * kIF;
    // Position of the first non-zero data sample in each Parallel Polyphase phase, relative to first polyphase
    // (TP_PARA_INTERP_INDEX, TP_PARA_DECI_INDEX = 0)
    static constexpr int dataIndexPoly =
        (ssrOutputPath + TP_PARA_INTERP_INDEX * TP_SSR) * kDF * TP_PARA_DECI_POLY / (kIF * TP_PARA_INTERP_POLY) -
        (ssrInnerPhase + TP_PARA_DECI_INDEX * TP_SSR);
    static constexpr int dataIndex = TP_SSR == 1 ? dataIndexPoly : posFirstDataOfPhase;
    static constexpr int MODIFY_MARGIN_OFFSET2 =
        ((posFirstDataOfPhase < 0) ? -1 * (CEIL((-1 * posFirstDataOfPhase), decompInnerPhases) / decompInnerPhases)
                                   : (posFirstDataOfPhase) / decompInnerPhases) +
        ssr_params::BTP_MODIFY_MARGIN_OFFSET; // add whatever margin offset that is sent to ssr kernels
    static constexpr int MODIFY_MARGIN_OFFSET =
        ((dataIndex < 0) ? -1 * (CEIL((-1 * dataIndex), decompInnerPhases) / decompInnerPhases)
                         : (dataIndex) / decompInnerPhases) +
        ssr_params::BTP_MODIFY_MARGIN_OFFSET; // add whatever margin offset that is sent to ssr kernels

   public:
    /**
     * @brief Separate taps into a specific SSR phase.
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
        static constexpr unsigned int firTypeTapLenPerPhase = fir_type<ssr_params>::getTapLen();
        for (unsigned int i = 0; i < firTypeTapLenPerPhase; i++) {
            unsigned int coefIndex = i * ssr_params::BTP_SSR + coeffPhase;
            if (coefIndex < (ssr_params::BTP_FIR_LEN * ssr_params::BTP_TDM_CHANNELS)) {
                ssrTapsRange.push_back(taps.at(coefIndex));

            } else {
                // padding
                ssrTapsRange.push_back(fir::nullElem<typename ssr_params::BTT_COEFF>());
            }
        }

        return ssrTapsRange;
    }

   private:
    // middle kernels connect to each other.
    static constexpr bool casc_in =
        isSSRaVector == 1 ? false : (ssrInnerPhase == 0) ? (false || TP_CASC_IN) : true; // first kernel of output phase
    static constexpr bool casc_out =
        isSSRaVector == 1
            ? false
            : (ssrInnerPhase == TP_SSR - 1) ? (false || TP_CASC_OUT) : true; // last kernel of output phase
    // static constexpr int rnd = TP_SSR * kDF * kIF; // causes resampler to create excessive FIR lenths
    static constexpr int rnd = TP_SSR;
    struct last_casc_params : public ssr_params {
        static constexpr int Bdim = TP_CASC_LEN - 1;
        static constexpr int BTP_FIR_LEN = isSSRaVector == 1 ? TP_FIR_LEN : (CEIL(TP_FIR_LEN, rnd) / TP_SSR);
        static constexpr int BTP_TDM_CHANNELS = ssr_params::BTP_TDM_CHANNELS / TP_SSR;
        static constexpr int BTP_CASC_IN = casc_in;
        static constexpr int BTP_CASC_OUT = casc_out;
        static constexpr int BTP_COEFF_PHASE = ssrCoeffPhase * TP_PARA_INTERP_POLY * TP_PARA_DECI_POLY +
                                               (TP_PARA_INTERP_INDEX * TP_PARA_DECI_POLY) % TP_PARA_INTERP_POLY +
                                               TP_PARA_DECI_INDEX * TP_PARA_INTERP_POLY;
        static constexpr int BTP_SSR = 1; // this struct calls decomposed kernels
        static constexpr int BTP_COEFF_PHASES = TP_SSR * TP_PARA_INTERP_POLY * TP_PARA_DECI_POLY;
        static constexpr int BTP_COEFF_PHASES_LEN = TP_COEFF_PHASES_LEN;
        static constexpr int BTP_MODIFY_MARGIN_OFFSET = MODIFY_MARGIN_OFFSET;
    };

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
    static constexpr unsigned int OUTPUT_WINDOW_BYTESIZE = TP_INTERPOLATE_FACTOR * ssr_params::BTP_INPUT_WINDOW_VSIZE *
                                                           sizeof(typename ssr_params::BTT_OUT_DATA) /
                                                           TP_DECIMATE_FACTOR;
    static constexpr unsigned int MARGIN_BYTESIZE =
        fir_type<last_casc_params>::getSSRMargin() * sizeof(typename ssr_params::BTT_DATA);

   public:
    using this_casc_kernels = casc_kernels<last_casc_params, fir_type>;

    /**
     * @brief Separate taps into a specific cascade segment.
     */
    static std::vector<typename ssr_params::BTT_COEFF> segment_taps_array_for_cascade(
        const std::vector<typename ssr_params::BTT_COEFF>& taps, unsigned int kernelPosition, unsigned int cascLen) {
        // call casc_kernels class method:
        std::vector<TT_COEFF> kernelTaps =
            this_casc_kernels::segment_taps_array_for_cascade(taps, kernelPosition, cascLen);
        return kernelTaps;
    }
    /**
     * @brief Separate taps into a specific cascade segment.
     */
    static int getKernelFirRangeLen(int pos) {
        // call casc_kernels class method:
        return this_casc_kernels::getKernelFirRangeLen(pos);
    }

   private:
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
        // make in connections
        if (TP_API == USE_WINDOW_API) {
            connect<>(in, firKernels[0].in[0]);
            dimensions(firKernels[0].in[0]) = {INPUT_WINDOW_BYTESIZE / sizeof(TT_DATA)};
            for (int i = 1; i < TP_CASC_LEN; i++) {
                single_buffer(firKernels[i].in[0]);
                connect<>(firKernels[i - 1].out[1], firKernels[i].in[0]);
                dimensions(firKernels[i - 1].out[1]) = {(INPUT_WINDOW_BYTESIZE + MARGIN_BYTESIZE) / sizeof(TT_DATA)};
                dimensions(firKernels[i].in[0]) = {(INPUT_WINDOW_BYTESIZE + MARGIN_BYTESIZE) / sizeof(TT_DATA)};
            }
        } else if (TP_API == USE_STREAM_API) {
            for (int i = 0; i < TP_CASC_LEN; i++) {
                net[ssrOutPathIndex][ssrInPhaseIndex][i] = new connect<stream, stream>(in, firKernels[i].in[0]);
                fifo_depth(*net[ssrOutPathIndex][ssrInPhaseIndex][i]) = calculate_fifo_depth(i);
            }
        } else {
            for (int i = 0; i < TP_CASC_LEN; i++) {
                connect<>(in, firKernels[i].in[0]);
                dimensions(firKernels[i].in[0]) = {INPUT_WINDOW_BYTESIZE / sizeof(TT_DATA)};
            }
        }
    }

    // make connections for a
    static void input_tdm_connections(
        port<input>(&in), kernel firKernels[TP_CASC_LEN], int ssrOutPathIndex, int ssrInPhaseIndex, net_type& net) {
        // make in connections
        if (TP_API == USE_WINDOW_API) {
            for (int i = 0; i < TP_CASC_LEN; i++) {
                connect<>(in, firKernels[i].in[0]);
                dimensions(firKernels[i].in[0]) = {INPUT_WINDOW_BYTESIZE / sizeof(TT_DATA)};
            }
        } else if (TP_API == USE_STREAM_API) {
            for (int i = 0; i < TP_CASC_LEN; i++) {
                net[ssrOutPathIndex][ssrInPhaseIndex][i] = new connect<stream, stream>(in, firKernels[i].in[0]);
                fifo_depth(*net[ssrOutPathIndex][ssrInPhaseIndex][i]) = calculate_fifo_depth(i);
            }
        } else {
            for (int i = 0; i < TP_CASC_LEN; i++) {
                connect<>(in, firKernels[i].in[0]);
                dimensions(firKernels[i].in[0]) = {INPUT_WINDOW_BYTESIZE / sizeof(TT_DATA)};
            }
        }
    }

    // make cascade connections
    static void cascade_tdm_connections(kernel firKernels[TP_CASC_LEN]) {
        for (int i = 1; i < TP_CASC_LEN; i++) {
            // TDM always has input buffer, hence index is always 1
            connect<cascade>(firKernels[i - 1].out[0], firKernels[i].in[1]);
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
                connect<>(firKernels[TP_CASC_LEN - 1].out[0], out);
                dimensions(firKernels[TP_CASC_LEN - 1].out[0]) = {OUTPUT_WINDOW_BYTESIZE / sizeof(TT_OUT_DATA)};
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
            constexpr(TP_NUM_OUTPUTS == 2 && casc_out == false) {
                if (TP_API == USE_WINDOW_API) {
                    connect<>(firKernels[TP_CASC_LEN - 1].out[DUAL_OUT_PORT_POS], out2);
                    dimensions(firKernels[TP_CASC_LEN - 1].out[DUAL_OUT_PORT_POS]) = {OUTPUT_WINDOW_BYTESIZE /
                                                                                      sizeof(TT_OUT_DATA)};
                } else {
                    connect<stream>(firKernels[TP_CASC_LEN - 1].out[DUAL_OUT_PORT_POS], out2);
                }
            }
    }

    // make RTP connection
    static void conditional_rtp_connections(port<input>(&coeff),
                                            kernel firKernels[TP_CASC_LEN],
                                            int rtpPosition = RTP_PORT_POS) {
        if
            constexpr(TP_USE_COEFF_RELOAD == USE_COEFF_RELOAD_TRUE) {
                printf("rtpPosition: %d \n", rtpPosition);
                connect<parameter>(coeff, async(firKernels[0].in[rtpPosition]));
            }
    }
};

template <int T_FIR_LEN, int kMaxTaps>
static constexpr unsigned int getMinCascLen() {
    return T_FIR_LEN < kMaxTaps ? 1 : T_FIR_LEN % kMaxTaps == 0 ? T_FIR_LEN / kMaxTaps : T_FIR_LEN / kMaxTaps + 1;
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
 * The Graphs utilities contain helper functions and classes that ease usage of library elements.
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
