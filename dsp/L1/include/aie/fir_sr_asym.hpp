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
#ifndef _DSPLIB_FIR_SR_ASYM_HPP_
#define _DSPLIB_FIR_SR_ASYM_HPP_
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
#include "fir_sr_asym_traits.hpp"
#include <vector>

namespace xf {
namespace dsp {
namespace aie {
namespace fir {
namespace sr_asym {

template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          bool TP_CASC_IN = CASC_IN_FALSE,
          bool TP_CASC_OUT = CASC_OUT_FALSE,
          unsigned int TP_FIR_RANGE_LEN = TP_FIR_LEN,
          unsigned int TP_KERNEL_POSITION = 0,
          unsigned int TP_CASC_LEN = 1,
          unsigned int TP_USE_COEFF_RELOAD = 0,
          unsigned int TP_NUM_OUTPUTS = 1,
          unsigned int TP_DUAL_IP = 0,
          unsigned int TP_API = 0,
          int TP_MODIFY_MARGIN_OFFSET = 0,
          unsigned int TP_COEFF_PHASE = 0,
          unsigned int TP_COEFF_PHASE_OFFSET = 0,
          unsigned int TP_COEFF_PHASES = 1,
          unsigned int TP_COEFF_PHASES_LEN = TP_FIR_LEN* TP_COEFF_PHASES,
          unsigned int TP_SAT = 1>
class fir_sr_asym;

template <typename fp = fir_params_defaults>
class fir_sr_asym_tl {
   public:
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
        constexpr int samplesInBuff = fnSamplesIn1024<T_D>();
        constexpr unsigned int m_kFirRangeOffset =
            fnFirRangeOffsetAsym<T_FIR_LEN, CLEN, pos, T_D, T_C, 1>(); // FIR Cascade Offset for this kernel position
        constexpr unsigned int m_kFirMarginOffset = fnFirMargin<T_FIR_LEN, T_D>() - T_FIR_LEN + 1; // FIR Margin Offset.
        constexpr unsigned int m_kFirMarginRangeOffset = m_kFirMarginOffset + m_kFirRangeOffset;   // FIR Margin Offset.
        constexpr unsigned int m_kOffsetResolution = (32 / sizeof(T_D));
        constexpr unsigned int m_kDataBuffXOffset =
            m_kFirMarginRangeOffset % m_kOffsetResolution; // Remainder of m_kFirInitOffset divided by 128bit
        constexpr unsigned int fir_range_len = getKernelFirRangeLen<pos>();
        constexpr unsigned int m_kArchFirLen =
            fir_range_len + m_kDataBuffXOffset; // This will check if only the portion of the FIR (TP_FIR_RANGE_LEN +
                                                // m_kDataBuffXOffset - due to xoffset alignment) fits.
        constexpr unsigned int m_kDataLoadVsize =
            fnDataLoadVsizeSrAsym<T_D, T_C, USE_STREAM_API>(); // 16;  //ie. upd_w loads a v4 of cint16
        constexpr unsigned int m_kInitDataNeeded = ((m_kArchFirLen + m_kDataLoadVsize) - 1);
        if
            constexpr(m_kInitDataNeeded > samplesInBuff) { return 0; }
        else {
            return 1;
        }
    }

    // Get kernel's FIR range Length
    static constexpr unsigned int getFirRangeLen() { return fp::BTP_FIR_RANGE_LEN; };

    // Get kernel's FIR Total Tap Length
    static constexpr unsigned int getTapLen() { return CEIL(fp::BTP_FIR_LEN, fp::BTP_SSR) / fp::BTP_SSR; };

    // Get kernel's FIR Total Tap Length
    static constexpr unsigned int getSSRMargin() {
        constexpr unsigned int margin =
            fnFirMargin<fp::BTP_FIR_LEN / fp::BTP_SSR, typename fp::BTT_DATA, fp::BTP_MODIFY_MARGIN_OFFSET>();
        return margin;
    };

    static constexpr unsigned int getDF() { return 1; };
    static constexpr unsigned int getIF() { return 1; };

    // Get FIR variant
    static constexpr eFIRVariant getFirType() { return eFIRVariant::kSrAsym; };
    // Get FIR source file
    static const char* getFirSource() { return "fir_sr_asym.cpp"; };

    using parent_class = fir_sr_asym<typename fp::BTT_DATA,
                                     typename fp::BTT_COEFF,
                                     fp::BTP_FIR_LEN,
                                     fp::BTP_SHIFT,
                                     fp::BTP_RND,
                                     fp::BTP_INPUT_WINDOW_VSIZE,
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
};
//-----------------------------------------------------------------------------------------------------
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          bool TP_CASC_IN = CASC_IN_FALSE,
          bool TP_CASC_OUT = CASC_OUT_FALSE,
          unsigned int TP_FIR_RANGE_LEN = TP_FIR_LEN,
          unsigned int TP_KERNEL_POSITION = 0,
          unsigned int TP_CASC_LEN = 1,
          unsigned int TP_USE_COEFF_RELOAD = 0,
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

    static constexpr unsigned int m_kColumns =
        fnNumColumnsSrAsym<TT_DATA, TT_COEFF, TP_API>(); // number of mult-adds per lane for main intrinsic
    static constexpr unsigned int m_kLanes =
        fnNumLanesSrAsym<TT_DATA, TT_COEFF, TP_API>(); // number of operations in parallel of this type combinations
                                                       // that the vector processor can do.
    static constexpr unsigned int m_kVOutSize = fnVOutSizeSrAsym<TT_DATA, TT_COEFF, TP_API>();
    static constexpr unsigned int m_kFirLenCeilCols = CEIL(TP_FIR_RANGE_LEN, m_kColumns);
    static constexpr unsigned int m_kDataLoadsInReg =
        fnDataLoadsInRegSrAsym<TT_DATA, TT_COEFF, TP_API>(); // 4;  //ratio of sbuff to load size.
    static constexpr unsigned int m_kStreamReadWidth = fnStreamReadWidth<TT_DATA, TT_COEFF>(); //
    static constexpr unsigned int m_kDataLoadVsize =
        fnDataLoadVsizeSrAsym<TT_DATA, TT_COEFF, TP_API>(); // 16;  //ie. upd_w loads a v4 of cint16
    static constexpr unsigned int m_kSamplesInBuff = m_kDataLoadsInReg * m_kDataLoadVsize;
    static constexpr unsigned int m_kFirRangeOffset =
        fnFirRangeOffsetAsym<TP_FIR_LEN, TP_CASC_LEN, TP_KERNEL_POSITION, TT_DATA, TT_COEFF, TP_API>() +
        TP_MODIFY_MARGIN_OFFSET; // FIR Cascade Offset for this kernel position

    static constexpr unsigned int m_kFirMargin = TP_API == 0
                                                     ? fnFirMargin<TP_FIR_LEN, TT_DATA, TP_MODIFY_MARGIN_OFFSET>()
                                                     : fnFirMargin<TP_FIR_LEN, TT_DATA>(); // FIR Margin.
    static constexpr unsigned int m_kFirMarginOffset = m_kFirMargin - TP_FIR_LEN + 1;      // FIR Margin Offset.
    static constexpr unsigned int m_kFirMarginRangeOffset =
        m_kFirMarginOffset + m_kFirRangeOffset; // FIR Margin Offset.
    static constexpr unsigned int m_kFirInitOffset = m_kFirMarginRangeOffset;
    static constexpr unsigned int m_kOffsetResolution = (32 / sizeof(TT_DATA));
    static constexpr unsigned int m_kDataBuffXOffset =
        m_kFirInitOffset % m_kOffsetResolution; // Remainder of m_kFirInitOffset divided by 128bit
    static constexpr unsigned int m_kArchFirLen =
        TP_FIR_RANGE_LEN + m_kDataBuffXOffset; // This will check if only the portion of the FIR (TP_FIR_RANGE_LEN +
                                               // m_kDataBuffXOffset - due to xoffset alignment) fits.
    static constexpr unsigned int m_kFirCoeffByteSize = TP_FIR_RANGE_LEN * sizeof(TT_COEFF);
// TODO - express this more elegantly
#if __HAS_SYM_PREADD__ == 1
    static constexpr unsigned int m_kPerLoopLoads = 1;
#else
    static constexpr unsigned int m_kPerLoopLoads = 2;
#endif
    static constexpr unsigned int m_kInitDataNeeded = ((m_kArchFirLen + m_kDataLoadVsize * m_kPerLoopLoads) - 1);
    static constexpr eArchType m_kArchZigZagEnabled =
        kArchBasic; // zigzag architecture disabled due to compilator inefficiencies degrading performance.
    static constexpr eArchType m_kArchBufSize =
        m_kInitDataNeeded > m_kSamplesInBuff ? m_kArchZigZagEnabled : kArchIncLoads;
    static constexpr eArchType m_kArchWindow =
        (m_kFirCoeffByteSize < fnZigZagMaxCoeffByteSize<TT_COEFF>() && // Revert to Basic for "big" FIRs.
         TP_INPUT_WINDOW_VSIZE % (m_kLanes * m_kDataLoadsInReg * m_kDataLoadVsize / m_kVOutSize) == 0)
            ? m_kArchBufSize
            : kArchBasic;
    static constexpr eArchType m_kArch = TP_API == 1 ? kArchStream : m_kArchWindow;
    static constexpr unsigned int m_kZigZagFactor = m_kArch == kArchZigZag ? 2 : 1;
    static constexpr unsigned int m_kRepeatFactor =
        m_kArch == kArchZigZag ? (m_kSamplesInBuff / m_kVOutSize) / m_kZigZagFactor : 1;
    static constexpr unsigned int m_kZbuffSize =
        kBuffSize32Byte; // kZbuffSize (256bit) - const for all data/coeff types
    static constexpr unsigned int m_kCoeffRegVsize = m_kZbuffSize / sizeof(TT_COEFF);
    static constexpr unsigned int m_kLsize = ((TP_INPUT_WINDOW_VSIZE) / m_kVOutSize) /
                                             (m_kZigZagFactor * m_kRepeatFactor); // m_kRepeatFactor;  // loop length,
                                                                                  // given that <m_kVOutSize> samples
                                                                                  // are output per iteration of loop
    static constexpr unsigned int m_kInitialLoads =
        m_kArch == kArchZigZag
            ? m_kDataLoadsInReg - 1
            : m_kArch == kArchIncLoads ? CEIL(m_kInitDataNeeded, m_kDataLoadVsize) / m_kDataLoadVsize
                                       : CEIL(m_kDataBuffXOffset + m_kLanes + m_kColumns - 1, m_kDataLoadVsize) /
                                             m_kDataLoadVsize; // Mimic end of zag (m_kDataBuffXOffset +
                                                               // m_kVOutSize+m_kColumns-1+(m_kLanes-1))/m_kLanes;
                                                               // //effectively ceil[(m_kDataBuffXOffset
                                                               // m_+kVOutsize+m_kColumns-1)/m_kLanes]
    static constexpr unsigned int m_kMarginLoads = CEIL(m_kFirMargin, m_kDataLoadVsize) / m_kDataLoadVsize;
    static constexpr unsigned int m_kIncLoadsRptFactor =
        m_kDataLoadsInReg * m_kDataLoadVsize /
        m_kVOutSize; // 8 times for cint32/cint32; 4 times for other data/coeff types
    static constexpr unsigned int m_kStrInitDataNeeded = m_kFirInitOffset + TP_FIR_RANGE_LEN + m_kVOutSize - 1;

    static constexpr int streamRptFactor = m_kSamplesInBuff / m_kVOutSize;
    static constexpr int marginLoadsMappedToBuff = (m_kFirMargin % m_kSamplesInBuff) / m_kDataLoadVsize;
    static constexpr int streamDataOffsetWithinBuff = (m_kFirInitOffset) % m_kSamplesInBuff;
    static constexpr int streamInitNullAccs = ((TP_FIR_LEN - TP_FIR_RANGE_LEN - m_kFirRangeOffset) / m_kVOutSize);
    static constexpr int streamInitAccs = (CEIL(streamInitNullAccs, streamRptFactor) - streamInitNullAccs);

    // Note, both of these are non-static, so all kernels update this variable in their own class instance separately -
    // so all kernels do initial stuff.
    alignas(__ALIGN_BYTE_SIZE__) int delay[(1024 / 8) / sizeof(int)] = {
        0}; // 1024b buffer to retain margin data between calls
    int doInit = (streamInitNullAccs == 0) ? 0 : 1;

    struct ssr_params : public fir_params_defaults {
        using BTT_DATA = TT_DATA;
        using BTT_COEFF = TT_COEFF;
        static constexpr unsigned int BTP_FIR_LEN = TP_FIR_LEN;
        static constexpr unsigned int BTP_CASC_LEN = TP_CASC_LEN;
        static constexpr unsigned int BTP_API = TP_API;
    };

    // Additional defensive checks
    static_assert(TP_INPUT_WINDOW_VSIZE % m_kLanes == 0,
                  "ERROR: TP_INPUT_WINDOW_VSIZE must be an integer multiple of the number of lanes for this data type");
    static_assert(!(m_kArch == kArchStream &&
                    (fir_sr_asym_tl<ssr_params>::
                         template fnCheckIfFits<TP_KERNEL_POSITION, TP_CASC_LEN, TP_FIR_LEN, TT_DATA, TT_COEFF, 1>() ==
                     0)),
                  "ERROR: TP_FIR_RANGE_LEN exceeds max supported range for this data/coeff type combination. Increase "
                  "TP_CASC_LEN to split the workload over more kernels.");

    // Coefficient Load Size - number of samples in a cascade read/write
    static constexpr unsigned int m_kCoeffLoadSize = SCD_SIZE / 8 / sizeof(TT_COEFF);

    // The coefficients array must include zero padding up to a multiple of the number of columns
    // the MAC intrinsic used to eliminate the accidental inclusion of terms beyond the FIR length.
    // Since this zero padding cannot be applied to the class-external coefficient array
    // the supplied taps are copied to an internal array, m_internalTaps, which can be padded.
    alignas(__ALIGN_BYTE_SIZE__) TT_COEFF m_internalTaps[CEIL(TP_FIR_RANGE_LEN, m_kCoeffLoadSize)];

    static constexpr kernelPositionState m_kKernelPosEnum = getKernelPositionState(TP_KERNEL_POSITION, TP_CASC_LEN);
    static_assert(m_kKernelPosEnum != error_kernel, "Error with TP_KERNEL_POSITION and TP_CASC_LEN");
    // CASC_OUT can now be true for the last kernel, but we don't need to have the windowBroadcast port
    // (The windowBroadcast port is just there to spoof an input window connection for downstream kernels)
    static constexpr bool m_kHasCascOutputForCascadeChain =
        (((TP_CASC_OUT == true) && m_kKernelPosEnum == last_kernel_in_chain)) ? false : true;

    // CASC_IN can now be true for the first kernel, but we don't have the initial windowBroadcast section to send input
    // data down the cascade
    // So this bool is used to skip over the windowBroadcast function and null acc reads (normally used for cascade skew
    // alignment).
    static constexpr bool m_kHasCascInputFromCascadeChain =
        (((TP_CASC_IN == true) && m_kKernelPosEnum == first_kernel_in_chain)) ? false : true;

    void filterSelectArch(T_inputIF<TP_CASC_IN, TT_DATA, TP_DUAL_IP> inInterface,
                          T_outputIF<TP_CASC_OUT, TT_DATA> outInterface);
    // Implementations
    void filterBasic(T_inputIF<TP_CASC_IN, TT_DATA, TP_DUAL_IP> inInterface,
                     T_outputIF<TP_CASC_OUT, TT_DATA> outInterface);
    void filterIncLoads(T_inputIF<TP_CASC_IN, TT_DATA, TP_DUAL_IP> inInterface,
                        T_outputIF<TP_CASC_OUT, TT_DATA> outInterface);
    void filterZigZag(T_inputIF<TP_CASC_IN, TT_DATA, TP_DUAL_IP> inInterface,
                      T_outputIF<TP_CASC_OUT, TT_DATA> outInterface);
    void filterStream(T_inputIF<TP_CASC_IN, TT_DATA, TP_DUAL_IP> inInterface,
                      T_outputIF<TP_CASC_OUT, TT_DATA> outInterface);

    alignas(__ALIGN_BYTE_SIZE__) TT_COEFF
        m_rawInTaps[CEIL(TP_COEFF_PHASES_LEN, m_kCoeffLoadSize)]; // Previous user input coefficients with zero padding
    bool isUpdateRequired = false;                                // Are coefficients sets equal?

   public:
    // Access function for AIE Synthesizer
    static eArchType get_m_kArch() { return m_kArch; };

    // Constructor
    kernelFilterClass(const TT_COEFF (&taps)[TP_FIR_LEN]) : m_rawInTaps{}, m_internalTaps{} {
        // Loads taps/coefficients
        firReload(taps);
    }

    // Constructors
    kernelFilterClass() : m_rawInTaps{}, m_internalTaps{} {}

    void firReload(const TT_COEFF* taps) {
        TT_COEFF* tapsPtr = (TT_COEFF*)taps;
        firReload(tapsPtr);
    }

    // Reload - create internalTaps array from a constructor provided array.
    // In SSR context, the array only contains coeffs for this coeff phase
    void firReload(TT_COEFF* taps) {
        for (int i = 0; i < TP_FIR_RANGE_LEN; i++) {
            unsigned int tapsAddress =
                TP_FIR_LEN - 1 -
                fnFirRangeOffsetAsym<TP_FIR_LEN, TP_CASC_LEN, TP_KERNEL_POSITION, TT_DATA, TT_COEFF, TP_API>() - i;
            m_internalTaps[i] = taps[tapsAddress];
        }
    };

    template <unsigned int coeffPhase,
              unsigned int coeffPhaseOffset,
              unsigned int coeffPhases,
              unsigned int coeffPhasesLen>
    void firReload(const TT_COEFF* taps) {
        TT_COEFF* tapsPtr = (TT_COEFF*)taps;
        firReload<coeffPhase, coeffPhaseOffset, coeffPhases, coeffPhasesLen>(tapsPtr);
    }
    // Reload - create internalTaps array from a full taps array.
    // In SSR context, only Nth coeff is required, i.e., coeffs for this coeff phase must be extracted.
    template <unsigned int coeffPhase,
              unsigned int coeffPhaseOffset,
              unsigned int coeffPhases,
              unsigned int coeffPhasesLen>
    void firReload(TT_COEFF* taps) {
        // total FIR length reconstructer from kernel's TP_FIR_LEN and coeffPhases.
        // Requires a static_assert on a higher level to disallow FIR lengths not being an integral multiple of
        // SSR/coeffPhases.
        for (int i = 0; i < TP_FIR_RANGE_LEN; i++) {
            unsigned int tapsOffset = i * coeffPhases + (coeffPhases - 1 - coeffPhase) + coeffPhaseOffset;
            unsigned int tapsAddress =
                coeffPhasesLen - 1 -
                coeffPhases *
                    fnFirRangeOffsetAsym<TP_FIR_LEN, TP_CASC_LEN, TP_KERNEL_POSITION, TT_DATA, TT_COEFF, TP_API>() -
                tapsOffset;

            m_internalTaps[i] = taps[tapsAddress];
        }
    };

    // FIR
    void filterKernel(T_inputIF<TP_CASC_IN, TT_DATA, TP_DUAL_IP> inInterface,
                      T_outputIF<TP_CASC_OUT, TT_DATA> outInterface);
    // Filter kernel for reloadable coefficient designs
    void filterKernel(T_inputIF<TP_CASC_IN, TT_DATA, TP_DUAL_IP> inInterface,
                      T_outputIF<TP_CASC_OUT, TT_DATA> outInterface,
                      const TT_COEFF (&inTaps)[TP_COEFF_PHASES_LEN]);
    void filterKernelRtp(T_inputIF<TP_CASC_IN, TT_DATA, TP_DUAL_IP> inInterface,
                         T_outputIF<TP_CASC_OUT, TT_DATA> outInterface);
};

//-----------------------------------------------------------------------------------------------------
// Single kernel base specialization. No cascade ports. Static coefficients
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          bool TP_CASC_IN,
          bool TP_CASC_OUT,
          unsigned int TP_FIR_RANGE_LEN,
          unsigned int TP_KERNEL_POSITION,
          unsigned int TP_CASC_LEN,
          unsigned int TP_USE_COEFF_RELOAD,
          unsigned int TP_NUM_OUTPUTS,
          unsigned int TP_DUAL_IP,
          unsigned int TP_API,
          int TP_MODIFY_MARGIN_OFFSET,
          unsigned int TP_COEFF_PHASE,
          unsigned int TP_COEFF_PHASE_OFFSET,
          unsigned int TP_COEFF_PHASES,
          unsigned int TP_COEFF_PHASES_LEN,
          unsigned int TP_SAT>
class fir_sr_asym : public kernelFilterClass<TT_DATA,
                                             TT_COEFF,
                                             TP_FIR_LEN,
                                             TP_SHIFT,
                                             TP_RND,
                                             TP_INPUT_WINDOW_VSIZE,
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
    // Constructor
    using thisKernelFilterClass = kernelFilterClass<TT_DATA,
                                                    TT_COEFF,
                                                    TP_FIR_LEN,
                                                    TP_SHIFT,
                                                    TP_RND,
                                                    TP_INPUT_WINDOW_VSIZE,
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
    fir_sr_asym(const TT_COEFF (&taps)[TP_FIR_LEN]) : thisKernelFilterClass(taps) {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fir_sr_asym::filter); }

    // FIR
    void filter(input_circular_buffer<
                    TT_DATA,
                    extents<inherited_extent>,
                    margin<fnFirMargin<TP_FIR_LEN, TT_DATA, TP_MODIFY_MARGIN_OFFSET>()> >& __restrict inWindow,
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

class fir_sr_asym<TT_DATA,
                  TT_COEFF,
                  TP_FIR_LEN,
                  TP_SHIFT,
                  TP_RND,
                  TP_INPUT_WINDOW_VSIZE,
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
    // Constructor
    using thisKernelFilterClass = kernelFilterClass<TT_DATA,
                                                    TT_COEFF,
                                                    TP_FIR_LEN,
                                                    TP_SHIFT,
                                                    TP_RND,
                                                    TP_INPUT_WINDOW_VSIZE,
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
    fir_sr_asym(const TT_COEFF (&taps)[TP_FIR_LEN]) : thisKernelFilterClass(taps) {}

    // Register Kernel Class
    static void registerKernelClass() {
        // The second part of this condition is needed to recognise if the kernel is used for SSR on halfbands.
        // They don't have multiple coeff phases in the same way as the other rate changers since their second phase is
        // just one tap.
        if
            constexpr(TP_COEFF_PHASES == 1 && TP_COEFF_PHASES_LEN <= TP_FIR_LEN) {
                // single kernel
                if
                    constexpr(TP_CASC_IN == CASC_IN_FALSE && TP_CASC_OUT == CASC_OUT_FALSE) {
                        if
                            constexpr(TP_NUM_OUTPUTS == 1) {
                                REGISTER_FUNCTION(fir_sr_asym::filterSingleKernelSingleOP);
                            }
                        else if
                            constexpr(TP_NUM_OUTPUTS == 2) { REGISTER_FUNCTION(fir_sr_asym::filterSingleKernelDualOP); }
                    }
                // final kernel
                else if
                    constexpr(TP_CASC_IN == CASC_IN_TRUE && TP_CASC_OUT == CASC_OUT_FALSE) {
                        if
                            constexpr(TP_NUM_OUTPUTS == 1) {
                                REGISTER_FUNCTION(fir_sr_asym::filterFinalKernelSingleOP);
                            }
                        else if
                            constexpr(TP_NUM_OUTPUTS == 2) { REGISTER_FUNCTION(fir_sr_asym::filterFinalKernelDualOP); }
                    }
                // first kernel
                else if
                    constexpr(TP_CASC_IN == CASC_IN_FALSE && TP_CASC_OUT == CASC_OUT_TRUE) {
                        if
                            constexpr(TP_KERNEL_POSITION == (TP_CASC_LEN - 1)) {
                                REGISTER_FUNCTION(fir_sr_asym::filterFirstKernelWithoutBroadcast);
                            }
                        else {
                            REGISTER_FUNCTION(fir_sr_asym::filterFirstKernel);
                        }
                    }
                // middle kernel
                else if
                    constexpr((TP_CASC_IN == CASC_IN_TRUE && TP_CASC_OUT == CASC_OUT_TRUE)) {
                        if
                            constexpr(TP_KERNEL_POSITION == (TP_CASC_LEN - 1)) {
                                REGISTER_FUNCTION(fir_sr_asym::filterMiddleKernelWithoutBroadcast);
                            }
                        else {
                            REGISTER_FUNCTION(fir_sr_asym::filterMiddleKernel);
                        }
                    }
            }
        else {
            if
                constexpr(TP_CASC_IN == CASC_IN_TRUE && TP_CASC_OUT == CASC_OUT_FALSE) {
                    if
                        constexpr(TP_KERNEL_POSITION == 0) {
                            REGISTER_FUNCTION(fir_sr_asym::filterLastChainFirstSSRKernel);
                        }
                    else {
                        if
                            constexpr(TP_NUM_OUTPUTS == 1) {
                                REGISTER_FUNCTION(fir_sr_asym::filterFinalKernelSingleOP);
                            }
                        else if
                            constexpr(TP_NUM_OUTPUTS == 2) { REGISTER_FUNCTION(fir_sr_asym::filterFinalKernelDualOP); }
                    }
                }
            else if
                constexpr((TP_CASC_IN == CASC_IN_TRUE && TP_CASC_OUT == CASC_OUT_TRUE)) {
                    if
                        constexpr(TP_KERNEL_POSITION == 0 && TP_CASC_LEN > 1) {
                            REGISTER_FUNCTION(fir_sr_asym::filterMidChainFirstSSRKernel);
                        }
                    else if
                        constexpr(TP_KERNEL_POSITION == 0 && TP_CASC_LEN == 1) {
                            REGISTER_FUNCTION(fir_sr_asym::filterMidChainFirstSSRKernelNoBdcst);
                        }
                    else if
                        constexpr(TP_KERNEL_POSITION == TP_CASC_LEN - 1) {
                            REGISTER_FUNCTION(fir_sr_asym::filterMiddleKernelNoBdcst);
                        }
                    else {
                        REGISTER_FUNCTION(fir_sr_asym::filterMiddleKernel);
                    }
                }
            else if
                constexpr(TP_CASC_IN == CASC_IN_FALSE && TP_CASC_OUT == CASC_OUT_TRUE) {
                    if
                        constexpr(TP_KERNEL_POSITION == TP_CASC_LEN - 1) {
                            REGISTER_FUNCTION(fir_sr_asym::filterFirstKernelNoBdcst);
                        }
                    else {
                        REGISTER_FUNCTION(fir_sr_asym::filterFirstKernel);
                    }
                }
            else if
                constexpr(TP_CASC_IN == CASC_IN_FALSE && TP_CASC_OUT == CASC_IN_FALSE) {
                    if
                        constexpr(TP_NUM_OUTPUTS == 1) { REGISTER_FUNCTION(fir_sr_asym::filterSingleKernelSingleOP); }
                    else if
                        constexpr(TP_NUM_OUTPUTS == 2) { REGISTER_FUNCTION(fir_sr_asym::filterSingleKernelDualOP); }
                }
        }
    }

    void filterSingleKernelSingleOP(
        input_circular_buffer<
            TT_DATA,
            extents<inherited_extent>,
            margin<fnFirMargin<TP_FIR_LEN, TT_DATA, TP_MODIFY_MARGIN_OFFSET>()> >& __restrict inWindow,
        output_circular_buffer<TT_DATA>& __restrict outWindow);

    void filterSingleKernelDualOP(
        input_circular_buffer<
            TT_DATA,
            extents<inherited_extent>,
            margin<fnFirMargin<TP_FIR_LEN, TT_DATA, TP_MODIFY_MARGIN_OFFSET>()> >& __restrict inWindow,
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
        input_circular_buffer<
            TT_DATA,
            extents<inherited_extent>,
            margin<fnFirMargin<TP_FIR_LEN, TT_DATA, TP_MODIFY_MARGIN_OFFSET>()> >& __restrict inWindow,
        output_stream_cacc48* outCascade,
        output_async_buffer<TT_DATA>& __restrict broadcastWindow);

    void filterFirstKernelWithoutBroadcast(
        input_circular_buffer<
            TT_DATA,
            extents<inherited_extent>,
            margin<fnFirMargin<TP_FIR_LEN, TT_DATA, TP_MODIFY_MARGIN_OFFSET>()> >& __restrict inWindow,
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
        input_circular_buffer<
            TT_DATA,
            extents<inherited_extent>,
            margin<fnFirMargin<TP_FIR_LEN, TT_DATA, TP_MODIFY_MARGIN_OFFSET>()> >& __restrict inWindow,
        input_stream_cacc48* inCascade,
        output_circular_buffer<TT_DATA>& __restrict outWindow);

    void filterMidChainFirstSSRKernel(
        input_circular_buffer<
            TT_DATA,
            extents<inherited_extent>,
            margin<fnFirMargin<TP_FIR_LEN, TT_DATA, TP_MODIFY_MARGIN_OFFSET>()> >& __restrict inWindow,
        input_stream_cacc48* inCascade,
        output_stream_cacc48* outCascade,
        output_async_buffer<TT_DATA>& __restrict broadcastWindow);

    void filterMidChainFirstSSRKernelNoBdcst(
        input_circular_buffer<
            TT_DATA,
            extents<inherited_extent>,
            margin<fnFirMargin<TP_FIR_LEN, TT_DATA, TP_MODIFY_MARGIN_OFFSET>()> >& __restrict inWindow,
        input_stream_cacc48* inCascade,
        output_stream_cacc48* outCascade);

    void filterFirstKernelNoBdcst(
        input_circular_buffer<
            TT_DATA,
            extents<inherited_extent>,
            margin<fnFirMargin<TP_FIR_LEN, TT_DATA, TP_MODIFY_MARGIN_OFFSET>()> >& __restrict inWindow,
        output_stream_cacc48* outCascade);
};

//-----------------------------------------------------------------------------------------------------
// Specialisation for window API, Reloadable coefficients
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
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
class fir_sr_asym<TT_DATA,
                  TT_COEFF,
                  TP_FIR_LEN,
                  TP_SHIFT,
                  TP_RND,
                  TP_INPUT_WINDOW_VSIZE,
                  TP_CASC_IN,
                  TP_CASC_OUT,
                  TP_FIR_RANGE_LEN,
                  TP_KERNEL_POSITION,
                  TP_CASC_LEN,
                  USE_COEFF_RELOAD_TRUE,
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
                                                     TP_CASC_IN,
                                                     TP_CASC_OUT,
                                                     TP_FIR_RANGE_LEN,
                                                     TP_KERNEL_POSITION,
                                                     TP_CASC_LEN,
                                                     USE_COEFF_RELOAD_TRUE,
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
    // Constructor
    using thisKernelFilterClass = kernelFilterClass<TT_DATA,
                                                    TT_COEFF,
                                                    TP_FIR_LEN,
                                                    TP_SHIFT,
                                                    TP_RND,
                                                    TP_INPUT_WINDOW_VSIZE,
                                                    TP_CASC_IN,
                                                    TP_CASC_OUT,
                                                    TP_FIR_RANGE_LEN,
                                                    TP_KERNEL_POSITION,
                                                    TP_CASC_LEN,
                                                    USE_COEFF_RELOAD_TRUE,
                                                    TP_NUM_OUTPUTS,
                                                    DUAL_IP_SINGLE,
                                                    USE_WINDOW_API,
                                                    TP_MODIFY_MARGIN_OFFSET,
                                                    TP_COEFF_PHASE,
                                                    TP_COEFF_PHASE_OFFSET,
                                                    TP_COEFF_PHASES,
                                                    TP_COEFF_PHASES_LEN,
                                                    TP_SAT>;
    fir_sr_asym() : thisKernelFilterClass() {}

    // Register Kernel Class
    static void registerKernelClass() {
        // single kernel
        // The second part of this condition is needed to recognise if the kernel is used for SSR on halfbands.
        // They don't have multiple coeff phases in the same way as the other rate changers since their second phase is
        // just one tap.
        if
            constexpr(TP_COEFF_PHASES == 1 && TP_COEFF_PHASES_LEN <= TP_FIR_LEN) {
                if
                    constexpr(TP_CASC_IN == CASC_IN_FALSE && TP_CASC_OUT == CASC_OUT_FALSE) {
                        if
                            constexpr(TP_NUM_OUTPUTS == 1) {
                                REGISTER_FUNCTION(fir_sr_asym::filterSingleKernelSingleOP);
                            }
                        else if
                            constexpr(TP_NUM_OUTPUTS == 2) { REGISTER_FUNCTION(fir_sr_asym::filterSingleKernelDualOP); }
                    }
                // final kernel
                else if
                    constexpr(TP_CASC_IN == CASC_IN_TRUE && TP_CASC_OUT == CASC_OUT_FALSE) {
                        if
                            constexpr(TP_NUM_OUTPUTS == 1) {
                                REGISTER_FUNCTION(fir_sr_asym::filterFinalKernelSingleOP);
                            }
                        else if
                            constexpr(TP_NUM_OUTPUTS == 2) { REGISTER_FUNCTION(fir_sr_asym::filterFinalKernelDualOP); }
                    }
                // first kernel
                else if
                    constexpr(TP_CASC_IN == CASC_IN_FALSE && TP_CASC_OUT == CASC_OUT_TRUE) {
                        if
                            constexpr(TP_KERNEL_POSITION == (TP_CASC_LEN - 1)) {
                                REGISTER_FUNCTION(fir_sr_asym::filterFirstKernelWithoutBroadcast);
                            }
                        else {
                            REGISTER_FUNCTION(fir_sr_asym::filterFirstKernel);
                        }
                    }
                // middle kernel
                else if
                    constexpr(TP_CASC_IN == CASC_IN_TRUE && TP_CASC_OUT == CASC_OUT_TRUE) {
                        if
                            constexpr(TP_KERNEL_POSITION == (TP_CASC_LEN - 1)) {
                                REGISTER_FUNCTION(fir_sr_asym::filterMiddleKernelWithoutBroadcast);
                            }
                        else {
                            REGISTER_FUNCTION(fir_sr_asym::filterMiddleKernel);
                        }
                    }
            }
        else {
            if
                constexpr(TP_CASC_IN == CASC_IN_TRUE && TP_CASC_OUT == CASC_OUT_FALSE) {
                    if
                        constexpr(TP_KERNEL_POSITION == 0) {
                            REGISTER_FUNCTION(fir_sr_asym::filterLastChainFirstSSRKernel);
                        }
                    else {
                        if
                            constexpr(TP_NUM_OUTPUTS == 1) {
                                REGISTER_FUNCTION(fir_sr_asym::filterFinalKernelSingleOP);
                            }
                        else if
                            constexpr(TP_NUM_OUTPUTS == 2) { REGISTER_FUNCTION(fir_sr_asym::filterFinalKernelDualOP); }
                    }
                }
            else if
                constexpr((TP_CASC_IN == CASC_IN_TRUE && TP_CASC_OUT == CASC_OUT_TRUE)) {
                    if
                        constexpr(TP_KERNEL_POSITION == 0 && TP_CASC_LEN > 1) {
                            REGISTER_FUNCTION(fir_sr_asym::filterMidChainFirstSSRKernel);
                        }
                    else if
                        constexpr(TP_KERNEL_POSITION == 0 && TP_CASC_LEN == 1) {
                            REGISTER_FUNCTION(fir_sr_asym::filterMidChainFirstSSRKernelNoBdcst);
                        }
                    else if
                        constexpr(TP_KERNEL_POSITION == TP_CASC_LEN - 1) {
                            REGISTER_FUNCTION(fir_sr_asym::filterMiddleKernelNoBdcst);
                        }
                    else {
                        REGISTER_FUNCTION(fir_sr_asym::filterMiddleKernel);
                    }
                }
            else if
                constexpr(TP_CASC_IN == CASC_IN_FALSE && TP_CASC_OUT == CASC_OUT_TRUE) {
                    if
                        constexpr(TP_KERNEL_POSITION == TP_CASC_LEN - 1) {
                            REGISTER_FUNCTION(fir_sr_asym::filterFirstKernelNoBdcst);
                        }
                    else {
                        REGISTER_FUNCTION(fir_sr_asym::filterFirstKernel);
                    }
                }
            else if
                constexpr(TP_CASC_IN == CASC_IN_FALSE && TP_CASC_OUT == CASC_IN_FALSE) {
                    if
                        constexpr(TP_NUM_OUTPUTS == 1) { REGISTER_FUNCTION(fir_sr_asym::filterSingleKernelSingleOP); }
                    else if
                        constexpr(TP_NUM_OUTPUTS == 2) { REGISTER_FUNCTION(fir_sr_asym::filterSingleKernelDualOP); }
                }
        }
    }

    // FIR
    void filterSingleKernelSingleOP(
        input_circular_buffer<
            TT_DATA,
            extents<inherited_extent>,
            margin<fnFirMargin<TP_FIR_LEN, TT_DATA, TP_MODIFY_MARGIN_OFFSET>()> >& __restrict inWindow,
        output_circular_buffer<TT_DATA>& __restrict outWindow,
        const TT_COEFF (&inTaps)[TP_COEFF_PHASES_LEN]);

    void filterSingleKernelDualOP(
        input_circular_buffer<
            TT_DATA,
            extents<inherited_extent>,
            margin<fnFirMargin<TP_FIR_LEN, TT_DATA, TP_MODIFY_MARGIN_OFFSET>()> >& __restrict inWindow,
        output_circular_buffer<TT_DATA>& __restrict outWindow,
        output_circular_buffer<TT_DATA>& __restrict outWindow2,
        const TT_COEFF (&inTaps)[TP_COEFF_PHASES_LEN]);

    void filterFinalKernelSingleOP(input_async_buffer<TT_DATA>& __restrict inWindow,
                                   input_stream_cacc48* inCascade,
                                   output_circular_buffer<TT_DATA>& __restrict outWindow);

    void filterFinalKernelDualOP(input_async_buffer<TT_DATA>& __restrict inWindow,
                                 input_stream_cacc48* inCascade,
                                 output_circular_buffer<TT_DATA>& __restrict outWindow,
                                 output_circular_buffer<TT_DATA>& __restrict outWindow2);

    void filterFirstKernel(
        input_circular_buffer<
            TT_DATA,
            extents<inherited_extent>,
            margin<fnFirMargin<TP_FIR_LEN, TT_DATA, TP_MODIFY_MARGIN_OFFSET>()> >& __restrict inWindow,
        output_stream_cacc48* outCascade,
        output_async_buffer<TT_DATA>& __restrict broadcastWindow,
        const TT_COEFF (&inTaps)[TP_COEFF_PHASES_LEN]);

    void filterFirstKernelWithoutBroadcast(
        input_circular_buffer<
            TT_DATA,
            extents<inherited_extent>,
            margin<fnFirMargin<TP_FIR_LEN, TT_DATA, TP_MODIFY_MARGIN_OFFSET>()> >& __restrict inWindow,
        output_stream_cacc48* outCascade,
        const TT_COEFF (&inTaps)[TP_COEFF_PHASES_LEN]);

    void filterMiddleKernel(input_async_buffer<TT_DATA>& __restrict inWindow,
                            input_stream_cacc48* inCascade,
                            output_stream_cacc48* outCascade,
                            output_async_buffer<TT_DATA>& __restrict broadcastWindow);

    void filterMiddleKernelWithoutBroadcast(input_async_buffer<TT_DATA>& __restrict inWindow,
                                            input_stream_cacc48* inCascade,
                                            output_stream_cacc48* outCascade);

    void filterLastChainFirstSSRKernel(
        input_circular_buffer<
            TT_DATA,
            extents<inherited_extent>,
            margin<fnFirMargin<TP_FIR_LEN, TT_DATA, TP_MODIFY_MARGIN_OFFSET>()> >& __restrict inWindow,
        input_stream_cacc48* inCascade,
        output_circular_buffer<TT_DATA>& __restrict outWindow);

    void filterMidChainFirstSSRKernel(
        input_circular_buffer<
            TT_DATA,
            extents<inherited_extent>,
            margin<fnFirMargin<TP_FIR_LEN, TT_DATA, TP_MODIFY_MARGIN_OFFSET>()> >& __restrict inWindow,
        input_stream_cacc48* inCascade,
        output_stream_cacc48* outCascade,
        output_async_buffer<TT_DATA>& __restrict broadcastWindow);

    void filterMidChainFirstSSRKernelNoBdcst(
        input_circular_buffer<
            TT_DATA,
            extents<inherited_extent>,
            margin<fnFirMargin<TP_FIR_LEN, TT_DATA, TP_MODIFY_MARGIN_OFFSET>()> >& __restrict inWindow,
        input_stream_cacc48* inCascade,
        output_stream_cacc48* outCascade);
    void filterMiddleKernelNoBdcst(input_async_buffer<TT_DATA>& __restrict inWindow,
                                   input_stream_cacc48* inCascade,
                                   output_stream_cacc48* outCascade);

    void filterFirstKernelNoBdcst(
        input_circular_buffer<
            TT_DATA,
            extents<inherited_extent>,
            margin<fnFirMargin<TP_FIR_LEN, TT_DATA, TP_MODIFY_MARGIN_OFFSET>()> >& __restrict inWindow,
        output_stream_cacc48* outCascade,
        const TT_COEFF (&inTaps)[TP_COEFF_PHASES_LEN]);
};

// ----------------------------------------------------------------------------
// ---------------------------------- STREAM ----------------------------------
// ----------------------------------------------------------------------------

// Single kernel specialization. No cascade ports. Static coefficients
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          bool TP_CASC_IN,
          bool TP_CASC_OUT,
          unsigned int TP_FIR_RANGE_LEN,
          unsigned int TP_KERNEL_POSITION,
          unsigned int TP_CASC_LEN,
          unsigned int TP_USE_COEFF_RELOAD,
          unsigned int TP_NUM_OUTPUTS,
          unsigned int TP_DUAL_IP,
          int TP_MODIFY_MARGIN_OFFSET,
          unsigned int TP_COEFF_PHASE,
          unsigned int TP_COEFF_PHASE_OFFSET,
          unsigned int TP_COEFF_PHASES,
          unsigned int TP_COEFF_PHASES_LEN,
          unsigned int TP_SAT>

class fir_sr_asym<TT_DATA,
                  TT_COEFF,
                  TP_FIR_LEN,
                  TP_SHIFT,
                  TP_RND,
                  TP_INPUT_WINDOW_VSIZE,
                  TP_CASC_IN,
                  TP_CASC_OUT,
                  TP_FIR_RANGE_LEN,
                  TP_KERNEL_POSITION,
                  TP_CASC_LEN,
                  TP_USE_COEFF_RELOAD,
                  TP_NUM_OUTPUTS,
                  TP_DUAL_IP,
                  USE_STREAM_API,
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
                                                     TP_CASC_IN,
                                                     TP_CASC_OUT,
                                                     TP_FIR_RANGE_LEN,
                                                     TP_KERNEL_POSITION,
                                                     TP_CASC_LEN,
                                                     TP_USE_COEFF_RELOAD,
                                                     TP_NUM_OUTPUTS,
                                                     TP_DUAL_IP,
                                                     USE_STREAM_API,
                                                     TP_MODIFY_MARGIN_OFFSET,
                                                     TP_COEFF_PHASE,
                                                     TP_COEFF_PHASE_OFFSET,
                                                     TP_COEFF_PHASES,
                                                     TP_COEFF_PHASES_LEN,
                                                     TP_SAT> {
   public:
    // Constructor
    using thisKernelFilterClass = kernelFilterClass<TT_DATA,
                                                    TT_COEFF,
                                                    TP_FIR_LEN,
                                                    TP_SHIFT,
                                                    TP_RND,
                                                    TP_INPUT_WINDOW_VSIZE,
                                                    TP_CASC_IN,
                                                    TP_CASC_OUT,
                                                    TP_FIR_RANGE_LEN,
                                                    TP_KERNEL_POSITION,
                                                    TP_CASC_LEN,
                                                    TP_USE_COEFF_RELOAD,
                                                    TP_NUM_OUTPUTS,
                                                    TP_DUAL_IP,
                                                    USE_STREAM_API,
                                                    TP_MODIFY_MARGIN_OFFSET,
                                                    TP_COEFF_PHASE,
                                                    TP_COEFF_PHASE_OFFSET,
                                                    TP_COEFF_PHASES,
                                                    TP_COEFF_PHASES_LEN,
                                                    TP_SAT>;
    fir_sr_asym(const TT_COEFF (&taps)[TP_FIR_LEN]) : thisKernelFilterClass(taps) {}

    // Constructor - Header based Coeff Reload allows to skip coeffs at construction.
    fir_sr_asym() : thisKernelFilterClass() {}

    // Register Kernel Class
    static void registerKernelClass() {
        if
            constexpr(TP_CASC_IN == CASC_IN_FALSE && TP_CASC_OUT == CASC_OUT_FALSE) {
                if
                    constexpr(TP_DUAL_IP == DUAL_IP_SINGLE && TP_NUM_OUTPUTS == 1) {
                        REGISTER_FUNCTION(fir_sr_asym::filterSingleKernelSingleIPSingleOP);
                    }
                else if
                    constexpr(TP_DUAL_IP == DUAL_IP_DUAL && TP_NUM_OUTPUTS == 1) {
                        REGISTER_FUNCTION(fir_sr_asym::filterSingleKernelDualIPSingleOP);
                    }
                else if
                    constexpr(TP_DUAL_IP == DUAL_IP_SINGLE && TP_NUM_OUTPUTS == 2) {
                        REGISTER_FUNCTION(fir_sr_asym::filterSingleKernelSingleIPDualOP);
                    }
                else if
                    constexpr(TP_DUAL_IP == DUAL_IP_DUAL && TP_NUM_OUTPUTS == 2) {
                        REGISTER_FUNCTION(fir_sr_asym::filterSingleKernelDualIPDualOP);
                    }
            }
        else if
            constexpr(TP_CASC_IN == CASC_IN_TRUE && TP_CASC_OUT == CASC_OUT_FALSE) {
                if
                    constexpr(TP_DUAL_IP == DUAL_IP_SINGLE && TP_NUM_OUTPUTS == 1) {
                        REGISTER_FUNCTION(fir_sr_asym::filterFinalKernelSingleIPSingleOP);
                    }
                else if
                    constexpr(TP_DUAL_IP == DUAL_IP_DUAL && TP_NUM_OUTPUTS == 1) {
                        REGISTER_FUNCTION(fir_sr_asym::filterFinalKernelDualIPSingleOP);
                    }
                else if
                    constexpr(TP_DUAL_IP == DUAL_IP_SINGLE && TP_NUM_OUTPUTS == 2) {
                        REGISTER_FUNCTION(fir_sr_asym::filterFinalKernelSingleIPDualOP);
                    }
                else if
                    constexpr(TP_DUAL_IP == DUAL_IP_DUAL && TP_NUM_OUTPUTS == 2) {
                        REGISTER_FUNCTION(fir_sr_asym::filterFinalKernelDualIPDualOP);
                    }
            }
        else if
            constexpr(TP_CASC_IN == CASC_IN_FALSE && TP_CASC_OUT == CASC_OUT_TRUE) {
                if
                    constexpr(TP_DUAL_IP == DUAL_IP_SINGLE) {
                        REGISTER_FUNCTION(fir_sr_asym::filterFirstKernelSingleIP);
                    }
                else if
                    constexpr(TP_DUAL_IP == DUAL_IP_DUAL) { REGISTER_FUNCTION(fir_sr_asym::filterFirstKernelDualIP); }
            }
        else if
            constexpr(TP_CASC_IN == CASC_IN_TRUE && TP_CASC_OUT == CASC_OUT_TRUE) {
                if
                    constexpr(TP_DUAL_IP == DUAL_IP_SINGLE) {
                        REGISTER_FUNCTION(fir_sr_asym::filterMiddleKernelSingleIP);
                    }
                else if
                    constexpr(TP_DUAL_IP == DUAL_IP_DUAL) { REGISTER_FUNCTION(fir_sr_asym::filterMiddleKernelDualIP); }
            }
    }

    // FIR
    void filterSingleKernelSingleIPSingleOP(input_stream<TT_DATA>* inStream, output_stream<TT_DATA>* outStream);

    void filterSingleKernelDualIPSingleOP(input_stream<TT_DATA>* inStream,
                                          input_stream<TT_DATA>* inStream2,
                                          output_stream<TT_DATA>* outStream);

    void filterSingleKernelSingleIPDualOP(input_stream<TT_DATA>* inStream,
                                          output_stream<TT_DATA>* outStream,
                                          output_stream<TT_DATA>* outStream2);

    void filterSingleKernelDualIPDualOP(input_stream<TT_DATA>* inStream,
                                        input_stream<TT_DATA>* inStream2,
                                        output_stream<TT_DATA>* outStream,
                                        output_stream<TT_DATA>* outStream2);

    void filterFinalKernelSingleIPSingleOP(input_stream<TT_DATA>* inStream,
                                           input_stream_cacc48* inCascade,
                                           output_stream<TT_DATA>* outStream);

    void filterFinalKernelSingleIPDualOP(input_stream<TT_DATA>* inStream,
                                         input_stream_cacc48* inCascade,
                                         output_stream<TT_DATA>* outStream,
                                         output_stream<TT_DATA>* outStream2);

    void filterFinalKernelDualIPSingleOP(input_stream<TT_DATA>* inStream,
                                         input_stream<TT_DATA>* inStream2,
                                         input_stream_cacc48* inCascade,
                                         output_stream<TT_DATA>* outStream);

    void filterFinalKernelDualIPDualOP(input_stream<TT_DATA>* inStream,
                                       input_stream<TT_DATA>* inStream2,
                                       input_stream_cacc48* inCascade,
                                       output_stream<TT_DATA>* outStream,
                                       output_stream<TT_DATA>* outStream2);

    void filterFirstKernelSingleIP(input_stream<TT_DATA>* inStream, output_stream_cacc48* outCascade);

    void filterFirstKernelDualIP(input_stream<TT_DATA>* inStream,
                                 input_stream<TT_DATA>* inStream2,
                                 output_stream_cacc48* outCascade);

    void filterMiddleKernelSingleIP(input_stream<TT_DATA>* inStream,
                                    input_stream_cacc48* inCascade,
                                    output_stream_cacc48* outCascade);

    void filterMiddleKernelDualIP(input_stream<TT_DATA>* inStream,
                                  input_stream<TT_DATA>* inStream2,
                                  input_stream_cacc48* inCascade,
                                  output_stream_cacc48* outCascade);
};

// Single kernel specialization. No cascade ports. Reloadable coefficients
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          bool TP_CASC_IN,
          bool TP_CASC_OUT,
          unsigned int TP_FIR_RANGE_LEN,
          unsigned int TP_KERNEL_POSITION,
          unsigned int TP_CASC_LEN,
          unsigned int TP_NUM_OUTPUTS,
          unsigned int TP_DUAL_IP,
          int TP_MODIFY_MARGIN_OFFSET,
          unsigned int TP_COEFF_PHASE,
          unsigned int TP_COEFF_PHASE_OFFSET,
          unsigned int TP_COEFF_PHASES,
          unsigned int TP_COEFF_PHASES_LEN,
          unsigned int TP_SAT>

class fir_sr_asym<TT_DATA,
                  TT_COEFF,
                  TP_FIR_LEN,
                  TP_SHIFT,
                  TP_RND,
                  TP_INPUT_WINDOW_VSIZE,
                  TP_CASC_IN,
                  TP_CASC_OUT,
                  TP_FIR_RANGE_LEN,
                  TP_KERNEL_POSITION,
                  TP_CASC_LEN,
                  USE_COEFF_RELOAD_TRUE,
                  TP_NUM_OUTPUTS,
                  TP_DUAL_IP,
                  USE_STREAM_API,
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
                                                     TP_CASC_IN,
                                                     TP_CASC_OUT,
                                                     TP_FIR_RANGE_LEN,
                                                     TP_KERNEL_POSITION,
                                                     TP_CASC_LEN,
                                                     USE_COEFF_RELOAD_TRUE,
                                                     TP_NUM_OUTPUTS,
                                                     TP_DUAL_IP,
                                                     USE_STREAM_API,
                                                     TP_MODIFY_MARGIN_OFFSET,
                                                     TP_COEFF_PHASE,
                                                     TP_COEFF_PHASE_OFFSET,
                                                     TP_COEFF_PHASES,
                                                     TP_COEFF_PHASES_LEN,
                                                     TP_SAT> {
   public:
    // Constructor
    using thisKernelFilterClass = kernelFilterClass<TT_DATA,
                                                    TT_COEFF,
                                                    TP_FIR_LEN,
                                                    TP_SHIFT,
                                                    TP_RND,
                                                    TP_INPUT_WINDOW_VSIZE,
                                                    TP_CASC_IN,
                                                    TP_CASC_OUT,
                                                    TP_FIR_RANGE_LEN,
                                                    TP_KERNEL_POSITION,
                                                    TP_CASC_LEN,
                                                    USE_COEFF_RELOAD_TRUE,
                                                    TP_NUM_OUTPUTS,
                                                    TP_DUAL_IP,
                                                    USE_STREAM_API,
                                                    TP_MODIFY_MARGIN_OFFSET,
                                                    TP_COEFF_PHASE,
                                                    TP_COEFF_PHASE_OFFSET,
                                                    TP_COEFF_PHASES,
                                                    TP_COEFF_PHASES_LEN,
                                                    TP_SAT>;
    fir_sr_asym() : thisKernelFilterClass() {}

    // Register Kernel Class
    static void registerKernelClass() {
        if
            constexpr(TP_CASC_IN == CASC_IN_FALSE && TP_CASC_OUT == CASC_OUT_FALSE) {
                if
                    constexpr(TP_DUAL_IP == DUAL_IP_SINGLE && TP_NUM_OUTPUTS == 1) {
                        REGISTER_FUNCTION(fir_sr_asym::filterSingleKernelSingleIPSingleOP);
                    }
                else if
                    constexpr(TP_DUAL_IP == DUAL_IP_DUAL && TP_NUM_OUTPUTS == 1) {
                        REGISTER_FUNCTION(fir_sr_asym::filterSingleKernelDualIPSingleOP);
                    }
                else if
                    constexpr(TP_DUAL_IP == DUAL_IP_SINGLE && TP_NUM_OUTPUTS == 2) {
                        REGISTER_FUNCTION(fir_sr_asym::filterSingleKernelSingleIPDualOP);
                    }
                else if
                    constexpr(TP_DUAL_IP == DUAL_IP_DUAL && TP_NUM_OUTPUTS == 2) {
                        REGISTER_FUNCTION(fir_sr_asym::filterSingleKernelDualIPDualOP);
                    }
            }
        else if
            constexpr(TP_CASC_IN == CASC_IN_TRUE && TP_CASC_OUT == CASC_OUT_FALSE) {
                if
                    constexpr(TP_DUAL_IP == DUAL_IP_SINGLE && TP_NUM_OUTPUTS == 1) {
                        REGISTER_FUNCTION(fir_sr_asym::filterFinalKernelSingleIPSingleOP);
                    }
                else if
                    constexpr(TP_DUAL_IP == DUAL_IP_DUAL && TP_NUM_OUTPUTS == 1) {
                        REGISTER_FUNCTION(fir_sr_asym::filterFinalKernelDualIPSingleOP);
                    }
                else if
                    constexpr(TP_DUAL_IP == DUAL_IP_SINGLE && TP_NUM_OUTPUTS == 2) {
                        REGISTER_FUNCTION(fir_sr_asym::filterFinalKernelSingleIPDualOP);
                    }
                else if
                    constexpr(TP_DUAL_IP == DUAL_IP_DUAL && TP_NUM_OUTPUTS == 2) {
                        REGISTER_FUNCTION(fir_sr_asym::filterFinalKernelDualIPDualOP);
                    }
            }
        else if
            constexpr(TP_CASC_IN == CASC_IN_FALSE && TP_CASC_OUT == CASC_OUT_TRUE) {
                if
                    constexpr(TP_DUAL_IP == DUAL_IP_SINGLE) {
                        REGISTER_FUNCTION(fir_sr_asym::filterFirstKernelSingleIP);
                    }
                else if
                    constexpr(TP_DUAL_IP == DUAL_IP_DUAL) { REGISTER_FUNCTION(fir_sr_asym::filterFirstKernelDualIP); }
            }
        else if
            constexpr(TP_CASC_IN == CASC_IN_TRUE && TP_CASC_OUT == CASC_OUT_TRUE) {
                if
                    constexpr(TP_DUAL_IP == DUAL_IP_SINGLE) {
                        REGISTER_FUNCTION(fir_sr_asym::filterMiddleKernelSingleIP);
                    }
                else if
                    constexpr(TP_DUAL_IP == DUAL_IP_DUAL) { REGISTER_FUNCTION(fir_sr_asym::filterMiddleKernelDualIP); }
            }
    }

    // FIR
    void filterSingleKernelSingleIPSingleOP(input_stream<TT_DATA>* inStream,
                                            output_stream<TT_DATA>* outStream,
                                            const TT_COEFF (&inTaps)[TP_COEFF_PHASES_LEN]);

    void filterSingleKernelDualIPSingleOP(input_stream<TT_DATA>* inStream,
                                          input_stream<TT_DATA>* inStream2,
                                          output_stream<TT_DATA>* outStream,
                                          const TT_COEFF (&inTaps)[TP_COEFF_PHASES_LEN]);

    void filterSingleKernelSingleIPDualOP(input_stream<TT_DATA>* inStream,
                                          output_stream<TT_DATA>* outStream,
                                          output_stream<TT_DATA>* outStream2,
                                          const TT_COEFF (&inTaps)[TP_COEFF_PHASES_LEN]);

    void filterSingleKernelDualIPDualOP(input_stream<TT_DATA>* inStream,
                                        input_stream<TT_DATA>* inStream2,
                                        output_stream<TT_DATA>* outStream,
                                        output_stream<TT_DATA>* outStream2,
                                        const TT_COEFF (&inTaps)[TP_COEFF_PHASES_LEN]);

    void filterFinalKernelSingleIPSingleOP(input_stream<TT_DATA>* inStream,
                                           input_stream_cacc48* inCascade,
                                           output_stream<TT_DATA>* outStream);

    void filterFinalKernelSingleIPDualOP(input_stream<TT_DATA>* inStream,
                                         input_stream_cacc48* inCascade,
                                         output_stream<TT_DATA>* outStream,
                                         output_stream<TT_DATA>* outStream2);

    void filterFinalKernelDualIPSingleOP(input_stream<TT_DATA>* inStream,
                                         input_stream<TT_DATA>* inStream2,
                                         input_stream_cacc48* inCascade,
                                         output_stream<TT_DATA>* outStream);

    void filterFinalKernelDualIPDualOP(input_stream<TT_DATA>* inStream,
                                       input_stream<TT_DATA>* inStream2,
                                       input_stream_cacc48* inCascade,
                                       output_stream<TT_DATA>* outStream,
                                       output_stream<TT_DATA>* outStream2);

    void filterFirstKernelSingleIP(input_stream<TT_DATA>* inStream,
                                   output_stream_cacc48* outCascade,
                                   const TT_COEFF (&inTaps)[TP_COEFF_PHASES_LEN]);

    void filterFirstKernelDualIP(input_stream<TT_DATA>* inStream,
                                 input_stream<TT_DATA>* inStream2,
                                 output_stream_cacc48* outCascade,
                                 const TT_COEFF (&inTaps)[TP_COEFF_PHASES_LEN]);

    void filterMiddleKernelSingleIP(input_stream<TT_DATA>* inStream,
                                    input_stream_cacc48* inCascade,
                                    output_stream_cacc48* outCascade);

    void filterMiddleKernelDualIP(input_stream<TT_DATA>* inStream,
                                  input_stream<TT_DATA>* inStream2,
                                  input_stream_cacc48* inCascade,
                                  output_stream_cacc48* outCascade);
};
}
}
}
}
}

#endif // _DSPLIB_FIR_SR_ASYM_HPP_
