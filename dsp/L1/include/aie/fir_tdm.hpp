/*
 * Copyright (C) 2019-2022, Xilinx, Inc.
 * Copyright (C) 2022-2023, Advanced Micro Devices, Inc.
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
#ifndef _DSPLIB_FIR_TDM_HPP_
#define _DSPLIB_FIR_TDM_HPP_
/*
Single Rate Asymmetric FIR.
This file exists to capture the definition of the single rate asymmetric FIR
filter kernel class.
The class definition holds defensive checks on parameter range and other
legality.
The constructor definition is held in this class because this class must be
accessible to graph level aie compilation.
The main runtime filter function is captured elsewhere as it contains aie
intrinsics which are not included in aie graph level
compilation.
*/

/* Coding conventions
   TT_      template type suffix
   TP_      template parameter suffix
*/

/* Design Notes
   Note that the AIE intrinsics operate on increasing indices, but in a conventional FIR there is a convolution of data
   and coefficients.
   So as to achieve the impulse response from the filter which matches the coefficient set, the coefficient array has to
   be reversed
   to compensate for the action of the intrinsics. This reversal is performed in the constructor. To avoid common-mode
   errors
   the reference model performs this reversal at run-time.
*/

#include <adf.h>
#include "fir_utils.hpp"
#include "fir_tdm_traits.hpp"
#include <vector>

namespace xf {
namespace dsp {
namespace aie {
namespace fir {
namespace tdm {

template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_TDM_CHANNELS,
          bool TP_CASC_IN = CASC_IN_FALSE,
          bool TP_CASC_OUT = CASC_OUT_FALSE,
          unsigned int TP_FIR_RANGE_LEN = TP_FIR_LEN,
          unsigned int TP_KERNEL_POSITION = 0,
          unsigned int TP_CASC_LEN = 1,
          unsigned int TP_USE_COEFF_RELOAD = 0, // 1 = use coeff reload, 0 = don't use coeff reload
          unsigned int TP_NUM_OUTPUTS = 1,
          unsigned int TP_DUAL_IP = 0,
          unsigned int TP_API = 0,
          int TP_MODIFY_MARGIN_OFFSET = 0,
          unsigned int TP_COEFF_PHASE = 0,
          unsigned int TP_COEFF_PHASE_OFFSET = 0,
          unsigned int TP_COEFF_PHASES = 1,
          unsigned int TP_COEFF_PHASES_LEN = TP_FIR_LEN* TP_COEFF_PHASES,
          unsigned int TP_SAT = 1>
class fir_tdm;

template <typename fp = fir_params_defaults>
class fir_tdm_tl {
   public:
    using parent_class = fir_tdm<typename fp::BTT_DATA,
                                 typename fp::BTT_COEFF,
                                 fp::BTP_FIR_LEN,
                                 fp::BTP_SHIFT,
                                 fp::BTP_RND,
                                 fp::BTP_INPUT_WINDOW_VSIZE,
                                 fp::BTP_TDM_CHANNELS,
                                 fp::BTP_CASC_IN,
                                 fp::BTP_CASC_OUT,
                                 fp::BTP_FIR_RANGE_LEN,
                                 fp::BTP_KERNEL_POSITION,
                                 fp::BTP_CASC_LEN,
                                 fp::BTP_USE_COEFF_RELOAD,
                                 fp::BTP_NUM_OUTPUTS,
                                 fp::BTP_DUAL_IP,
                                 fp::BTP_API,
                                 fp::BTP_MODIFY_MARGIN_OFFSET,
                                 fp::BTP_COEFF_PHASE,
                                 fp::BTP_COEFF_PHASE_OFFSET,
                                 fp::BTP_COEFF_PHASES,
                                 fp::BTP_COEFF_PHASES_LEN,
                                 fp::BTP_SAT>;
    // Get kernel's FIR range Length
    template <int pos>
    static constexpr unsigned int getKernelFirRangeLen() {
        constexpr unsigned int firRangeLen =
            pos + 1 == fp::BTP_CASC_LEN
                ? fnFirRangeRemAsym<fp::BTP_FIR_LEN, fp::BTP_CASC_LEN, pos, typename fp::BTT_DATA,
                                    typename fp::BTT_COEFF, fp::BTP_API>()
                : fnFirRangeAsym<fp::BTP_FIR_LEN, fp::BTP_CASC_LEN, pos, typename fp::BTT_DATA, typename fp::BTT_COEFF,
                                 fp::BTP_API>();
        return firRangeLen;
    };

    template <int pos, int CLEN, int T_FIR_LEN, typename T_D, typename T_C, unsigned int SSR>
    static constexpr unsigned int fnCheckIfFits() {
        return 1;
    }

    // Get kernel's FIR range Length
    static constexpr unsigned int getFirRangeLen() { return fp::BTP_FIR_RANGE_LEN; };

    // Get kernel's FIR Total Tap Length
    static constexpr unsigned int getTapLen() {
        return CEIL(fp::BTP_FIR_LEN * fp::BTP_TDM_CHANNELS, fp::BTP_SSR) / fp::BTP_SSR;
    };

    // Get kernel's FIR Total Tap Length
    static constexpr unsigned int getSSRMargin() {
        constexpr unsigned int margin =
            fnFirMargin<fp::BTP_FIR_LEN / fp::BTP_SSR, typename fp::BTT_DATA, fp::BTP_MODIFY_MARGIN_OFFSET>();
        return margin;
    };

    static constexpr unsigned int getDF() { return 1; };
    static constexpr unsigned int getIF() { return 1; };

    static constexpr unsigned int getLanes() { return parent_class::get_CoeffLanes(); };

    // Get FIR variant
    static constexpr eFIRVariant getFirType() { return eFIRVariant::kTDM; };
    // Get FIR source file
    static const char* getFirSource() { return "fir_tdm.cpp"; };
};
//-----------------------------------------------------------------------------------------------------
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_TDM_CHANNELS,
          bool TP_CASC_IN = CASC_IN_FALSE,
          bool TP_CASC_OUT = CASC_OUT_FALSE,
          unsigned int TP_FIR_RANGE_LEN = TP_FIR_LEN,
          unsigned int TP_KERNEL_POSITION = 0,
          unsigned int TP_CASC_LEN = 1,
          unsigned int TP_USE_COEFF_RELOAD = 0, // 1 = use coeff reload, 0 = don't use coeff reload
          unsigned int TP_NUM_OUTPUTS = 1,
          unsigned int TP_DUAL_IP = 0,
          unsigned int TP_API = 0,
          int TP_MODIFY_MARGIN_OFFSET = 0,
          unsigned int TP_COEFF_PHASE = 0,
          unsigned int TP_COEFF_PHASE_OFFSET = 0,
          unsigned int TP_COEFF_PHASES = 1,
          unsigned int TP_COEFF_PHASES_LEN = TP_FIR_LEN* TP_COEFF_PHASES,
          unsigned int TP_SAT = 1>
class kernelFilterClass {
   private:
    // Parameter value defensive and legality checks
    static_assert(TP_FIR_RANGE_LEN >= FIR_LEN_MIN,
                  "ERROR: Illegal combination of design FIR length and cascade length, resulting in kernel FIR length "
                  "below minimum required value. ");
    static_assert(TP_SHIFT >= SHIFT_MIN && TP_SHIFT <= SHIFT_MAX, "ERROR: TP_SHIFT is out of the supported range.");
    static_assert(TP_SAT >= SAT_MODE_MIN && TP_SAT <= SAT_MODE_MAX && TP_SAT != 2,
                  "ERROR: TP_SAT is out of supported range");
    static_assert(TP_SAT != 2, "ERROR: TP_SAT is invalid. Valid values of TP_SAT are 0, 1, and 3");
    static_assert(TP_RND >= ROUND_MIN && TP_RND <= ROUND_MAX, "ERROR: TP_RND is out of the supported range.");
    static_assert(fnEnumType<TT_DATA>() != enumUnknownType, "ERROR: TT_DATA is not a supported type.");
    static_assert(fnEnumType<TT_COEFF>() != enumUnknownType, "ERROR: TT_COEFF is not a supported type.");
    static_assert(fnTypeCheckDataCoeffSize<TT_DATA, TT_COEFF>() != 0,
                  "ERROR: TT_DATA type less precise than TT_COEFF is not supported.");
    static_assert(fnTypeCheckDataCoeffCmplx<TT_DATA, TT_COEFF>() != 0,
                  "ERROR: real TT_DATA with complex TT_COEFF is not supported.");
    static_assert(fnTypeCheckDataCoeffFltInt<TT_DATA, TT_COEFF>() != 0,
                  "ERROR: a mix of float and integer types of TT_DATA and TT_COEFF is not supported.");
    static_assert(fnTypeStreamSupport<TT_DATA, TT_COEFF, TP_API>() != 0,
                  "ERROR: Unsupported API interface for requested TT_DATA and TT_COEFF type combination.");
    static_assert(TP_NUM_OUTPUTS > 0 && TP_NUM_OUTPUTS <= 2, "ERROR: only single or dual outputs are supported.");
#if __SUPPORTS_CFLOAT__ == 1
    static_assert(!(std::is_same<TT_DATA, cfloat>::value || std::is_same<TT_DATA, float>::value) || (TP_SHIFT == 0),
                  "ERROR: TP_SHIFT cannot be performed for TT_DATA=cfloat, so must be set to 0");
#else
    static_assert(!(std::is_same<TT_DATA, float>::value) || (TP_SHIFT == 0),
                  "ERROR: TP_SHIFT cannot be performed for TT_DATA=cfloat, so must be set to 0");
#endif
    // There are additional defensive checks after architectural constants have been calculated.

    static constexpr int TP_NUM_FRAMES = TP_INPUT_WINDOW_VSIZE / TP_TDM_CHANNELS;

    typedef typename std::conditional_t<std::is_same<TT_COEFF, int32>::value,
                                        int16,
                                        std::conditional_t<std::is_same<TT_COEFF, cint32>::value, cint16, TT_COEFF> >
        TT_COEFF_TEST;
    static constexpr unsigned int m_kZbuffSize =
        kBuffSize32Byte; // kZbuffSize (256bit) - const for all data/coeff types
    static constexpr unsigned int m_kCoeffRegVsize = m_kZbuffSize / sizeof(TT_COEFF);
    // only assume data > coeff. otherwise, we'd need to load data at 0.5 rate, i.e. every second iteration + add offset
    static constexpr unsigned int dataToCoeffSizeRatio =
        (sizeof(TT_DATA) > sizeof(TT_COEFF)) ? (sizeof(TT_DATA) / sizeof(TT_COEFF)) : 1;
    static constexpr unsigned int m_kVOutSize = fnVOutSizeTdm<TT_DATA, TT_COEFF_TEST, TP_API>();

    // static constexpr unsigned int m_kVOutSize = dataToCoeffSizeRatio * m_kCoeffRegVsize;
    static constexpr int kSamplesInVectData = dataToCoeffSizeRatio * m_kVOutSize;
    static constexpr int kSamplesInVectCoeff = dataToCoeffSizeRatio * m_kVOutSize;
    static constexpr int kSamplesInVectAcc = m_kVOutSize;

    static constexpr int paddedDataSize = CEIL(TP_FIR_LEN, kSamplesInVectData);
    static constexpr int paddedCoeffSize = CEIL(TP_FIR_LEN, kSamplesInVectCoeff);

    static constexpr int paddedFrameSize = CEIL(paddedDataSize, (kSamplesInVectData * TP_CASC_LEN));
    static constexpr int paddedWindowSize = TP_NUM_FRAMES * paddedFrameSize;

    static constexpr int cascWindowSize = paddedWindowSize / TP_CASC_LEN;
    static constexpr int cascFrameSize = paddedFrameSize / TP_CASC_LEN;

    static constexpr int stepSize = (TP_KERNEL_POSITION < (TP_FIR_LEN % TP_CASC_LEN)) + (TP_FIR_LEN / TP_CASC_LEN);

    static constexpr int kTotalCoeffSize = paddedCoeffSize * stepSize;
    static constexpr int kVecInCoeff = paddedCoeffSize / kSamplesInVectData;
    static constexpr int kPairsInCoeff = kVecInCoeff / 2;
    static constexpr int singleAccRequired = kVecInCoeff % 2;
    static constexpr int kVecInFrame = cascFrameSize / kSamplesInVectData;
    static constexpr int shift = TP_SHIFT + 15;

    static constexpr unsigned int m_kColumns =
        fnNumColumnsTdm<TT_DATA, TT_COEFF, TP_API>(); // number of mult-adds per lane for main intrinsic
    static constexpr unsigned int m_kLanes =
        fnNumLanesTdm<TT_DATA, TT_COEFF, TP_API>(); // number of operations in parallel of this type combinations
                                                    // that the vector processor can do.
    static constexpr unsigned int m_kFirLenCeilCols = CEIL(TP_FIR_RANGE_LEN, m_kColumns);
    static constexpr unsigned int m_kDataLoadsInReg =
        fnDataLoadsInRegTdm<TT_DATA, TT_COEFF, TP_API>(); // 4;  //ratio of sbuff to load size.
    static constexpr unsigned int m_kStreamReadWidth = fnStreamReadWidth<TT_DATA, TT_COEFF>(); //
    static constexpr unsigned int m_kDataLoadVsize =
        fnDataLoadVsizeTdm<TT_DATA, TT_COEFF, TP_API>(); // 16;  //ie. upd_w loads a v4 of cint16
    static constexpr unsigned int m_kSamplesInBuff = m_kDataLoadsInReg * m_kDataLoadVsize;
    static constexpr unsigned int m_kFirRangeOffset =
        fnFirRangeOffsetAsym<TP_FIR_RANGE_LEN,
                             TP_CASC_LEN,
                             TP_KERNEL_POSITION,
                             TT_DATA,
                             TT_COEFF,
                             TP_API>() +
        TP_MODIFY_MARGIN_OFFSET; // FIR Cascade Offset for this kernel position

    static constexpr unsigned int m_kFirMargin = fnTDMFirMargin<TP_FIR_LEN, TT_DATA, TP_TDM_CHANNELS>();
    static constexpr unsigned int m_kFirMarginOffset =
        m_kFirMargin - TP_FIR_LEN * TP_TDM_CHANNELS + 1; // FIR Margin Offset.
    static constexpr unsigned int m_kFirMarginRangeOffset =
        m_kFirMarginOffset + m_kFirRangeOffset; // FIR Margin Offset.
    static constexpr unsigned int m_kFirInitOffset = m_kFirMarginRangeOffset;
    static constexpr unsigned int m_kOffsetResolution = (32 / sizeof(TT_DATA)); // 1024-bit input vector register
    static constexpr unsigned int m_kDataBuffXOffset =
        m_kFirInitOffset % m_kOffsetResolution; // Remainder of m_kFirInitOffset divided by 128bit
    static constexpr unsigned int m_kFirCoeffByteSize = TP_FIR_RANGE_LEN * sizeof(TT_COEFF);
// TODO - express this more elegantly
#if __HAS_SYM_PREADD__ == 1
    static constexpr unsigned int m_kPerLoopLoads = 1;
#else
    static constexpr unsigned int m_kPerLoopLoads = 2;
#endif

    static constexpr eArchType m_kArch = TP_API == 1 ? kArchStream : kArchBasic;
    static constexpr unsigned int m_kZigZagFactor = m_kArch == kArchZigZag ? 2 : 1;
    static constexpr unsigned int m_kRepeatFactor =
        m_kArch == kArchZigZag ? (m_kSamplesInBuff / m_kVOutSize) / m_kZigZagFactor : 1;
    static constexpr unsigned int m_kLsize = ((TP_INPUT_WINDOW_VSIZE) / m_kVOutSize) /
                                             (m_kZigZagFactor * m_kRepeatFactor); // m_kRepeatFactor;  // loop length,
                                                                                  // given that <m_kVOutSize> samples
                                                                                  // are output per iteration of loop

    static constexpr unsigned int m_kInitialLoads =
        CEIL(m_kDataBuffXOffset + m_kLanes + m_kColumns - 1, m_kDataLoadVsize);

    TT_COEFF (&m_internalTaps)[TP_FIR_LEN * TP_TDM_CHANNELS];

    static constexpr kernelPositionState m_kKernelPosEnum = getKernelPositionState(TP_KERNEL_POSITION, TP_CASC_LEN);

    struct ssr_params : public fir_params_defaults {
        using BTT_DATA = TT_DATA;
        using BTT_COEFF = TT_COEFF;
        static constexpr unsigned int BTP_FIR_LEN = TP_FIR_LEN;
        static constexpr unsigned int BTP_CASC_LEN = TP_CASC_LEN;
        static constexpr unsigned int BTP_TDM_CHANNELS = TP_TDM_CHANNELS;
        static constexpr unsigned int BTP_API = TP_API;
    };

    // Additional defensive checks
    static_assert(TP_INPUT_WINDOW_VSIZE % m_kLanes == 0,
                  "ERROR: TP_INPUT_WINDOW_VSIZE must be an integer multiple of the number of lanes for this data type");
    // Coefficient Load Size - number of samples in a cascade read/write
    static constexpr unsigned int m_kCoeffLoadSize = SCD_SIZE / 8 / sizeof(TT_COEFF);

    void filterSelectArch(T_inputIF<TP_CASC_IN, TT_DATA, TP_DUAL_IP> inInterface,
                          T_outputIF<TP_CASC_OUT, TT_DATA> outInterface);
    // Implementations
    void filterBasic(T_inputIF<TP_CASC_IN, TT_DATA, TP_DUAL_IP> inInterface,
                     T_outputIF<TP_CASC_OUT, TT_DATA> outInterface);

   public:
    static eArchType get_m_kArch() { return m_kArch; };

    static unsigned int get_Lanes() { return m_kVOutSize; };
    static unsigned int get_CoeffLanes() { return kSamplesInVectCoeff; };

    // Constructor
    kernelFilterClass(TT_COEFF (&taps)[TP_FIR_LEN * TP_TDM_CHANNELS]) : m_internalTaps{taps} {
        for (int i = 0; i < TP_FIR_LEN * TP_TDM_CHANNELS; i++) {
        }
    }

    // // Constructors
    // kernelFilterClass(){

    // }

    //     void firReload(const TT_COEFF* taps) {
    //         TT_COEFF* tapsPtr = (TT_COEFF*)taps;
    //         firReload(tapsPtr);
    //     }

    //     // Reload - create internalTaps array from a constructor provided array.
    //     // In SSR context, the array only contains coeffs for this coeff phase
    //     void firReload(TT_COEFF* taps) {
    //         for (int i = 0; i < TP_FIR_RANGE_LEN; i++) {
    //             unsigned int tapsAddress =
    //                 TP_FIR_LEN - 1 -
    //                 fnFirRangeOffsetAsym<TP_FIR_LEN, TP_CASC_LEN, TP_KERNEL_POSITION, TT_DATA, TT_COEFF, TP_API>() -
    //                 i;
    //             // m_internalTaps[i] = taps[tapsAddress];
    //         }
    //     };

    //     template <unsigned int coeffPhase,
    //               unsigned int coeffPhaseOffset,
    //               unsigned int coeffPhases,
    //               unsigned int coeffPhasesLen>
    //     void firReload(const TT_COEFF* taps) {
    //         TT_COEFF* tapsPtr = (TT_COEFF*)taps;
    //         firReload<coeffPhase, coeffPhaseOffset, coeffPhases, coeffPhasesLen>(tapsPtr);
    //     }
    //     // Reload - create internalTaps array from a full taps array.
    //     // In SSR context, only Nth coeff is required, i.e., coeffs for this coeff phase must be extracted.
    //     template <unsigned int coeffPhase,
    //               unsigned int coeffPhaseOffset,
    //               unsigned int coeffPhases,
    //               unsigned int coeffPhasesLen>
    //     void firReload(TT_COEFF* taps) {
    // // total FIR length reconstructer from kernel's TP_FIR_LEN and coeffPhases.
    // // Requires a static_assert on a higher level to disallow FIR lengths not being an integral multiple of
    // SSR/coeffPhases.
    //         for (int i = 0; i < TP_FIR_RANGE_LEN; i++) {
    //             unsigned int tapsOffset = i * coeffPhases + (coeffPhases - 1 - coeffPhase) + coeffPhaseOffset;
    //             unsigned int tapsAddress =
    //                 coeffPhasesLen - 1 -
    //                 coeffPhases *
    //                     fnFirRangeOffsetAsym<TP_FIR_LEN, TP_CASC_LEN, TP_KERNEL_POSITION, TT_DATA, TT_COEFF,
    //                     TP_API>() -
    //                 tapsOffset;

    //             //    m_internalTaps[i] = taps[tapsAddress];
    //         }
    //     };

    // FIR
    void filterKernel(T_inputIF<TP_CASC_IN, TT_DATA, TP_DUAL_IP> inInterface,
                      T_outputIF<TP_CASC_OUT, TT_DATA> outInterface);
    // Filter kernel for reloadable coefficient designs
    // void filterKernel(T_inputIF<TP_CASC_IN, TT_DATA, TP_DUAL_IP> inInterface,
    //                   T_outputIF<TP_CASC_OUT, TT_DATA> outInterface,
    //                   const TT_COEFF (&inTaps)[TP_COEFF_PHASES_LEN]);
    // void filterKernelRtp(T_inputIF<TP_CASC_IN, TT_DATA, TP_DUAL_IP> inInterface,
    //                      T_outputIF<TP_CASC_OUT, TT_DATA> outInterface);
};

//-----------------------------------------------------------------------------------------------------
// Single kernel base specialization. No cascade ports. Static coefficients
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_TDM_CHANNELS,
          bool TP_CASC_IN,
          bool TP_CASC_OUT,
          unsigned int TP_FIR_RANGE_LEN,
          unsigned int TP_KERNEL_POSITION,
          unsigned int TP_CASC_LEN,
          unsigned int TP_USE_COEFF_RELOAD, // 1 = use coeff reload, 0 = don't use coeff reload
          unsigned int TP_NUM_OUTPUTS,
          unsigned int TP_DUAL_IP,
          unsigned int TP_API,
          int TP_MODIFY_MARGIN_OFFSET,
          unsigned int TP_COEFF_PHASE,
          unsigned int TP_COEFF_PHASE_OFFSET,
          unsigned int TP_COEFF_PHASES,
          unsigned int TP_COEFF_PHASES_LEN,
          unsigned int TP_SAT>
class fir_tdm : public kernelFilterClass<TT_DATA,
                                         TT_COEFF,
                                         TP_FIR_LEN,
                                         TP_SHIFT,
                                         TP_RND,
                                         TP_INPUT_WINDOW_VSIZE,
                                         TP_TDM_CHANNELS,
                                         TP_CASC_IN,
                                         TP_CASC_OUT,
                                         TP_FIR_RANGE_LEN,
                                         TP_KERNEL_POSITION,
                                         TP_CASC_LEN,
                                         TP_USE_COEFF_RELOAD,
                                         TP_NUM_OUTPUTS,
                                         TP_DUAL_IP,
                                         TP_API,
                                         TP_MODIFY_MARGIN_OFFSET,
                                         TP_COEFF_PHASE,
                                         TP_COEFF_PHASE_OFFSET,
                                         TP_COEFF_PHASES,
                                         TP_COEFF_PHASES_LEN,
                                         TP_SAT> {
   public:
    TT_COEFF (&internalTaps)[TP_FIR_LEN * TP_TDM_CHANNELS];
    // Constructor
    using thisKernelFilterClass = kernelFilterClass<TT_DATA,
                                                    TT_COEFF,
                                                    TP_FIR_LEN,
                                                    TP_SHIFT,
                                                    TP_RND,
                                                    TP_INPUT_WINDOW_VSIZE,
                                                    TP_TDM_CHANNELS,
                                                    TP_CASC_IN,
                                                    TP_CASC_OUT,
                                                    TP_FIR_RANGE_LEN,
                                                    TP_KERNEL_POSITION,
                                                    TP_CASC_LEN,
                                                    TP_USE_COEFF_RELOAD,
                                                    TP_NUM_OUTPUTS,
                                                    TP_DUAL_IP,
                                                    TP_API,
                                                    TP_MODIFY_MARGIN_OFFSET,
                                                    TP_COEFF_PHASE,
                                                    TP_COEFF_PHASE_OFFSET,
                                                    TP_COEFF_PHASES,
                                                    TP_COEFF_PHASES_LEN,
                                                    TP_SAT>;
    fir_tdm(TT_COEFF (&taps)[TP_FIR_LEN * TP_TDM_CHANNELS]) : internalTaps(taps), thisKernelFilterClass(taps) {}

    // Register Kernel Class
    static void registerKernelClass() {
        REGISTER_FUNCTION(fir_tdm::filter);
        REGISTER_PARAMETER(internalTaps);
    }

    // FIR
    void filter(
        input_circular_buffer<TT_DATA,
                              extents<inherited_extent>,
                              margin<fnTDMFirMargin<TP_FIR_LEN, TT_DATA, TP_TDM_CHANNELS>()> >& __restrict inWindow,
        output_circular_buffer<TT_DATA>& __restrict outWindow);
};

//-----------------------------------------------------------------------------------------------------
// Specialisation for window API, Static coefficients
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_TDM_CHANNELS,
          bool TP_CASC_IN,
          bool TP_CASC_OUT,
          unsigned int TP_FIR_RANGE_LEN,
          unsigned int TP_KERNEL_POSITION,
          unsigned int TP_CASC_LEN,
          unsigned int TP_NUM_OUTPUTS,
          int TP_MODIFY_MARGIN_OFFSET,
          unsigned int TP_COEFF_PHASE,
          unsigned int TP_COEFF_PHASE_OFFSET,
          unsigned int TP_COEFF_PHASES,
          unsigned int TP_COEFF_PHASES_LEN,
          unsigned int TP_SAT>

class fir_tdm<TT_DATA,
              TT_COEFF,
              TP_FIR_LEN,
              TP_SHIFT,
              TP_RND,
              TP_INPUT_WINDOW_VSIZE,
              TP_TDM_CHANNELS,
              TP_CASC_IN,
              TP_CASC_OUT,
              TP_FIR_RANGE_LEN,
              TP_KERNEL_POSITION,
              TP_CASC_LEN,
              USE_COEFF_RELOAD_FALSE,
              TP_NUM_OUTPUTS,
              DUAL_IP_SINGLE,
              USE_WINDOW_API,
              TP_MODIFY_MARGIN_OFFSET,
              TP_COEFF_PHASE,
              TP_COEFF_PHASE_OFFSET,
              TP_COEFF_PHASES,
              TP_COEFF_PHASES_LEN,
              TP_SAT> : public kernelFilterClass<TT_DATA,
                                                 TT_COEFF,
                                                 TP_FIR_LEN,
                                                 TP_SHIFT,
                                                 TP_RND,
                                                 TP_INPUT_WINDOW_VSIZE,
                                                 TP_TDM_CHANNELS,
                                                 TP_CASC_IN,
                                                 TP_CASC_OUT,
                                                 TP_FIR_RANGE_LEN,
                                                 TP_KERNEL_POSITION,
                                                 TP_CASC_LEN,
                                                 USE_COEFF_RELOAD_FALSE,
                                                 TP_NUM_OUTPUTS,
                                                 DUAL_IP_SINGLE,
                                                 USE_WINDOW_API,
                                                 TP_MODIFY_MARGIN_OFFSET,
                                                 TP_COEFF_PHASE,
                                                 TP_COEFF_PHASE_OFFSET,
                                                 TP_COEFF_PHASES,
                                                 TP_COEFF_PHASES_LEN,
                                                 TP_SAT> {
   public:
    TT_COEFF (&internalTaps)[TP_FIR_LEN * TP_TDM_CHANNELS];
    // Constructor
    using thisKernelFilterClass = kernelFilterClass<TT_DATA,
                                                    TT_COEFF,
                                                    TP_FIR_LEN,
                                                    TP_SHIFT,
                                                    TP_RND,
                                                    TP_INPUT_WINDOW_VSIZE,
                                                    TP_TDM_CHANNELS,
                                                    TP_CASC_IN,
                                                    TP_CASC_OUT,
                                                    TP_FIR_RANGE_LEN,
                                                    TP_KERNEL_POSITION,
                                                    TP_CASC_LEN,
                                                    USE_COEFF_RELOAD_FALSE,
                                                    TP_NUM_OUTPUTS,
                                                    DUAL_IP_SINGLE,
                                                    USE_WINDOW_API,
                                                    TP_MODIFY_MARGIN_OFFSET,
                                                    TP_COEFF_PHASE,
                                                    TP_COEFF_PHASE_OFFSET,
                                                    TP_COEFF_PHASES,
                                                    TP_COEFF_PHASES_LEN,
                                                    TP_SAT>;
    fir_tdm(TT_COEFF (&taps)[TP_FIR_LEN * TP_TDM_CHANNELS]) : internalTaps(taps), thisKernelFilterClass(taps) {}

    // Register Kernel Class
    static void registerKernelClass() {
        REGISTER_PARAMETER(internalTaps);
        // The second part of this condition is needed to recognise if the kernel is used for SSR on halfbands.
        // They don't have multiple coeff phases in the same way as the other rate changers since their second phase is
        // just one tap.
        if
            constexpr(TP_COEFF_PHASES == 1 && TP_COEFF_PHASES_LEN <= TP_FIR_LEN) {
                // single kernel
                if
                    constexpr(TP_CASC_IN == CASC_IN_FALSE && TP_CASC_OUT == CASC_OUT_FALSE) {
                        if
                            constexpr(TP_NUM_OUTPUTS == 1) { REGISTER_FUNCTION(fir_tdm::filterSingleKernelSingleOP); }
                        else if
                            constexpr(TP_NUM_OUTPUTS == 2) { REGISTER_FUNCTION(fir_tdm::filterSingleKernelDualOP); }
                    }
                // final kernel
                else if
                    constexpr(TP_CASC_IN == CASC_IN_TRUE && TP_CASC_OUT == CASC_OUT_FALSE) {
                        if
                            constexpr(TP_NUM_OUTPUTS == 1) { REGISTER_FUNCTION(fir_tdm::filterFinalKernelSingleOP); }
                        else if
                            constexpr(TP_NUM_OUTPUTS == 2) { REGISTER_FUNCTION(fir_tdm::filterFinalKernelDualOP); }
                    }
                // first kernel
                else if
                    constexpr(TP_CASC_IN == CASC_IN_FALSE && TP_CASC_OUT == CASC_OUT_TRUE) {
                        if
                            constexpr(TP_KERNEL_POSITION == (TP_CASC_LEN - 1)) {
                                REGISTER_FUNCTION(fir_tdm::filterFirstKernelWithoutBroadcast);
                            }
                        else {
                            REGISTER_FUNCTION(fir_tdm::filterFirstKernel);
                        }
                    }
                // middle kernel
                else if
                    constexpr((TP_CASC_IN == CASC_IN_TRUE && TP_CASC_OUT == CASC_OUT_TRUE)) {
                        if
                            constexpr(TP_KERNEL_POSITION == (TP_CASC_LEN - 1)) {
                                REGISTER_FUNCTION(fir_tdm::filterMiddleKernelWithoutBroadcast);
                            }
                        else {
                            REGISTER_FUNCTION(fir_tdm::filterMiddleKernel);
                        }
                    }
            }
        else {
            if
                constexpr(TP_CASC_IN == CASC_IN_TRUE && TP_CASC_OUT == CASC_OUT_FALSE) {
                    if
                        constexpr(TP_KERNEL_POSITION == 0) {
                            REGISTER_FUNCTION(fir_tdm::filterLastChainFirstSSRKernel);
                        }
                    else {
                        if
                            constexpr(TP_NUM_OUTPUTS == 1) { REGISTER_FUNCTION(fir_tdm::filterFinalKernelSingleOP); }
                        else if
                            constexpr(TP_NUM_OUTPUTS == 2) { REGISTER_FUNCTION(fir_tdm::filterFinalKernelDualOP); }
                    }
                }
            else if
                constexpr((TP_CASC_IN == CASC_IN_TRUE && TP_CASC_OUT == CASC_OUT_TRUE)) {
                    if
                        constexpr(TP_KERNEL_POSITION == 0 && TP_CASC_LEN > 1) {
                            REGISTER_FUNCTION(fir_tdm::filterMidChainFirstSSRKernel);
                        }
                    else if
                        constexpr(TP_KERNEL_POSITION == 0 && TP_CASC_LEN == 1) {
                            REGISTER_FUNCTION(fir_tdm::filterMidChainFirstSSRKernelNoBdcst);
                        }
                    else if
                        constexpr(TP_KERNEL_POSITION == TP_CASC_LEN - 1) {
                            REGISTER_FUNCTION(fir_tdm::filterMiddleKernelNoBdcst);
                        }
                    else {
                        REGISTER_FUNCTION(fir_tdm::filterMiddleKernel);
                    }
                }
            else if
                constexpr(TP_CASC_IN == CASC_IN_FALSE && TP_CASC_OUT == CASC_OUT_TRUE) {
                    if
                        constexpr(TP_KERNEL_POSITION == TP_CASC_LEN - 1) {
                            REGISTER_FUNCTION(fir_tdm::filterFirstKernelNoBdcst);
                        }
                    else {
                        REGISTER_FUNCTION(fir_tdm::filterFirstKernel);
                    }
                }
            else if
                constexpr(TP_CASC_IN == CASC_IN_FALSE && TP_CASC_OUT == CASC_IN_FALSE) {
                    if
                        constexpr(TP_NUM_OUTPUTS == 1) { REGISTER_FUNCTION(fir_tdm::filterSingleKernelSingleOP); }
                    else if
                        constexpr(TP_NUM_OUTPUTS == 2) { REGISTER_FUNCTION(fir_tdm::filterSingleKernelDualOP); }
                }
        }
    }

    void filterSingleKernelSingleOP(
        input_circular_buffer<TT_DATA,
                              extents<inherited_extent>,
                              margin<fnTDMFirMargin<TP_FIR_LEN, TT_DATA, TP_TDM_CHANNELS>()> >& __restrict inWindow,
        output_circular_buffer<TT_DATA>& __restrict outWindow);

    void filterSingleKernelDualOP(
        input_circular_buffer<TT_DATA,
                              extents<inherited_extent>,
                              margin<fnTDMFirMargin<TP_FIR_LEN, TT_DATA, TP_TDM_CHANNELS>()> >& __restrict inWindow,
        output_circular_buffer<TT_DATA>& __restrict outWindow,
        output_circular_buffer<TT_DATA>& __restrict outWindow2);

    void filterFinalKernelSingleOP(input_async_buffer<TT_DATA>& __restrict inWindow,
                                   input_stream_cacc48* inCascade,
                                   output_circular_buffer<TT_DATA>& __restrict outWindow);

    void filterFinalKernelDualOP(input_async_buffer<TT_DATA>& __restrict inWindow,
                                 input_stream_cacc48* inCascade,
                                 output_circular_buffer<TT_DATA>& __restrict outWindow,
                                 output_circular_buffer<TT_DATA>& __restrict outWindow2);

    void filterFirstKernel(
        input_circular_buffer<TT_DATA,
                              extents<inherited_extent>,
                              margin<fnTDMFirMargin<TP_FIR_LEN, TT_DATA, TP_TDM_CHANNELS>()> >& __restrict inWindow,
        output_stream_cacc48* outCascade,
        output_async_buffer<TT_DATA>& __restrict broadcastWindow);

    void filterFirstKernelWithoutBroadcast(
        input_circular_buffer<TT_DATA,
                              extents<inherited_extent>,
                              margin<fnTDMFirMargin<TP_FIR_LEN, TT_DATA, TP_TDM_CHANNELS>()> >& __restrict inWindow,
        output_stream_cacc48* outCascade);

    void filterMiddleKernel(input_async_buffer<TT_DATA>& __restrict inWindow,
                            input_stream_cacc48* inCascade,
                            output_stream_cacc48* outCascade,
                            output_async_buffer<TT_DATA>& __restrict broadcastWindow);

    void filterMiddleKernelNoBdcst(input_async_buffer<TT_DATA>& __restrict inWindow,
                                   input_stream_cacc48* inCascade,
                                   output_stream_cacc48* outCascade);

    void filterMiddleKernelWithoutBroadcast(input_async_buffer<TT_DATA>& __restrict inWindow,
                                            input_stream_cacc48* inCascade,
                                            output_stream_cacc48* outCascade);

    void filterLastChainFirstSSRKernel(
        input_circular_buffer<TT_DATA,
                              extents<inherited_extent>,
                              margin<fnTDMFirMargin<TP_FIR_LEN, TT_DATA, TP_TDM_CHANNELS>()> >& __restrict inWindow,
        input_stream_cacc48* inCascade,
        output_circular_buffer<TT_DATA>& __restrict outWindow);

    void filterMidChainFirstSSRKernel(
        input_circular_buffer<TT_DATA,
                              extents<inherited_extent>,
                              margin<fnTDMFirMargin<TP_FIR_LEN, TT_DATA, TP_TDM_CHANNELS>()> >& __restrict inWindow,
        input_stream_cacc48* inCascade,
        output_stream_cacc48* outCascade,
        output_async_buffer<TT_DATA>& __restrict broadcastWindow);

    void filterMidChainFirstSSRKernelNoBdcst(
        input_circular_buffer<TT_DATA,
                              extents<inherited_extent>,
                              margin<fnTDMFirMargin<TP_FIR_LEN, TT_DATA, TP_TDM_CHANNELS>()> >& __restrict inWindow,
        input_stream_cacc48* inCascade,
        output_stream_cacc48* outCascade);

    void filterFirstKernelNoBdcst(
        input_circular_buffer<TT_DATA,
                              extents<inherited_extent>,
                              margin<fnTDMFirMargin<TP_FIR_LEN, TT_DATA, TP_TDM_CHANNELS>()> >& __restrict inWindow,
        output_stream_cacc48* outCascade);
};
}
}
}
}
}

#endif // _DSPLIB_FIR_TDM_HPP_
