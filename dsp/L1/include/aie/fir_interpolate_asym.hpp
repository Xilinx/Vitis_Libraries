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
#ifndef FIR_INTERPOLATE_ASYM_HPP
#define FIR_INTERPOLATE_ASYM_HPP
/*
Interpolating FIR class definition

The file holds the definition of the Asymmetric Interpolation FIR kernel class.

Note on Coefficient reversal. The AIE processor intrinsics naturally sum data and coefficients in the same order,
but the conventional definition of a FIR has data and coefficient indices in opposite order. The order of
coefficients is therefore reversed during construction to yield conventional FIR behaviour.
*/

/* Coding conventions
  TT_      template type suffix
  TP_      template parameter suffix
*/

#include <adf.h>
#include <assert.h>
#include <array>
#include <cstdint>

#include "fir_utils.hpp"
#include "fir_interpolate_asym_traits.hpp"

// CEIL rounds x up to the next multiple of y, which may be x itself.
#define CEIL(x, y) (((x + y - 1) / y) * y)
//#define intDataNeeded(x) (1 + (int)(((x)-1)/TP_INTERPOLATE_FACTOR))

namespace xf {
namespace dsp {
namespace aie {
namespace fir {
namespace interpolate_asym {

template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_INTERPOLATE_FACTOR,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          bool TP_CASC_IN = CASC_IN_FALSE,
          bool TP_CASC_OUT = CASC_OUT_FALSE,
          unsigned int TP_FIR_RANGE_LEN = TP_FIR_LEN,
          unsigned int TP_KERNEL_POSITION = 0,
          unsigned int TP_CASC_LEN = 1,
          unsigned int TP_USE_COEFF_RELOAD = 0,
          unsigned int TP_DUAL_IP = 0,
          unsigned int TP_NUM_OUTPUTS = 1,
          unsigned int TP_API = 0,
          int TP_MODIFY_MARGIN_OFFSET = 0,
          unsigned int TP_COEFF_PHASE = 0,
          unsigned int TP_COEFF_PHASE_OFFSET = 0,
          unsigned int TP_COEFF_PHASES = 1,
          unsigned int TP_COEFF_PHASES_LEN = TP_FIR_LEN* TP_COEFF_PHASES,
          unsigned int TP_SAT = 1>
class fir_interpolate_asym;

template <typename fp = fir_params_defaults>
class fir_interpolate_asym_tl {
   public:
    // Get kernel's FIR range Length
    template <int pos>
    static constexpr unsigned int getKernelFirRangeLen() {
        constexpr unsigned int firRangeLen =
            pos + 1 == fp::BTP_CASC_LEN
                ? (fnFirRangeRem<fp::BTP_FIR_LEN, fp::BTP_CASC_LEN, pos, fp::BTP_INTERPOLATE_FACTOR>())
                : (fnFirRange<fp::BTP_FIR_LEN, fp::BTP_CASC_LEN, pos, fp::BTP_INTERPOLATE_FACTOR>());
        return firRangeLen;
    };

    template <int pos, int CLEN, int T_FIR_LEN, typename T_D, typename T_C, unsigned int TP_DUAL_IP, unsigned int IF>
    static constexpr unsigned int fnCheckIfFits() {
        constexpr unsigned int fir_range_len = getKernelFirRangeLen<pos>();
        constexpr unsigned int m_kColumns = sizeof(T_C) == 2 ? 2 : 1; // number of mult-adds per lane for main intrinsic
        constexpr unsigned int m_kNumOps = CEIL(fir_range_len / IF, m_kColumns) / m_kColumns;
        constexpr unsigned int m_kDataLoadVsize = (32 / sizeof(T_D)); // number of samples in a single 256-bit load
        constexpr int sizeOf1Read = (TP_DUAL_IP == 0) ? m_kDataLoadVsize / 2 : m_kDataLoadVsize;
        constexpr unsigned int m_kDataRegVsize = fnSamplesIn1024<T_D>();
        constexpr unsigned int m_kSpaces = m_kDataRegVsize - sizeOf1Read;
        if
            constexpr(m_kNumOps * m_kColumns > m_kSpaces) { return 0; }
        else {
            return 1;
        }
    }

    // Get kernel's FIR range Length
    static constexpr unsigned int getFirRangeLen() { return fp::BTP_FIR_RANGE_LEN; };

    // Get kernel's FIR Total Tap Length
    static constexpr unsigned int getTapLen() {
        int rnd = fp::BTP_SSR * fp::BTP_INTERPOLATE_FACTOR;
        return CEIL(fp::BTP_FIR_LEN, rnd) / fp::BTP_SSR;
    };
    static constexpr unsigned int getDF() { return 1; };
    static constexpr unsigned int getIF() { return fp::BTP_INTERPOLATE_FACTOR; };

    // Get kernel's FIR Total Tap Length
    static constexpr unsigned int getSSRMargin() {
        constexpr unsigned int margin =
            fnFirMargin<fp::BTP_FIR_LEN / fp::BTP_SSR / fp::BTP_INTERPOLATE_FACTOR, typename fp::BTT_DATA>();
        return margin;
    };

    // Get FIR variant
    static constexpr eFIRVariant getFirType() { return eFIRVariant::kIntAsym; };

    using parent_class = fir_interpolate_asym<typename fp::BTT_DATA,
                                              typename fp::BTT_COEFF,
                                              fp::BTP_FIR_LEN,
                                              fp::BTP_INTERPOLATE_FACTOR,
                                              fp::BTP_SHIFT,
                                              fp::BTP_RND,
                                              fp::BTP_INPUT_WINDOW_VSIZE,
                                              fp::BTP_CASC_IN,
                                              fp::BTP_CASC_OUT,
                                              fp::BTP_FIR_RANGE_LEN,
                                              fp::BTP_KERNEL_POSITION,
                                              fp::BTP_CASC_LEN,
                                              fp::BTP_USE_COEFF_RELOAD,
                                              fp::BTP_DUAL_IP,
                                              fp::BTP_NUM_OUTPUTS,
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
          unsigned int TP_INTERPOLATE_FACTOR,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          bool TP_CASC_IN = CASC_IN_FALSE,
          bool TP_CASC_OUT = CASC_OUT_FALSE,
          unsigned int TP_FIR_RANGE_LEN = TP_FIR_LEN,
          unsigned int TP_KERNEL_POSITION = 0,
          unsigned int TP_CASC_LEN = 1,
          unsigned int TP_USE_COEFF_RELOAD = 0,
          unsigned int TP_DUAL_IP = 0,
          unsigned int TP_NUM_OUTPUTS = 1,
          unsigned int TP_API = 0,
          int TP_MODIFY_MARGIN_OFFSET = 0,
          unsigned int TP_COEFF_PHASE = 0,
          unsigned int TP_COEFF_PHASE_OFFSET = 0,
          unsigned int TP_COEFF_PHASES = 1,
          unsigned int TP_COEFF_PHASES_LEN = TP_FIR_LEN* TP_COEFF_PHASES,
          unsigned int TP_SAT = 1>
class kernelFilterClass {
   private:
    // tap length __restrictions
    // Two implementations have been written for this filter. They have identical behaviour, but one is optimised for an
    // Interpolation factor
    // greater than the number of accumulator registers available.
    static constexpr unsigned int kArchIncr = 1;
    static constexpr unsigned int kArchPhaseSeries = 2;
    static constexpr unsigned int kArchPhaseParallel = 3;
    static constexpr unsigned int kArchStreamPhaseSeries = 4;
    static constexpr unsigned int kArchStreamPhaseParallel = 5;

    // Parameter value defensive and legality checks
    // static_assert(TP_FIR_LEN <= fnMaxTapssIntAsym<TT_DATA, TT_COEFF>(),"ERROR: Max supported FIR length exceeded for
    // TT_DATA/TT_COEFF combination. ");
    static_assert(TP_FIR_RANGE_LEN >= FIR_LEN_MIN,
                  "ERROR: Illegal combination of design FIR length and cascade length, resulting in kernel FIR length "
                  "below minimum required value. ");
    static_assert(TP_SHIFT >= SHIFT_MIN && TP_SHIFT <= SHIFT_MAX, "ERROR: SHIFT is out of the supported range.");
    static_assert(TP_RND >= ROUND_MIN && TP_RND <= ROUND_MAX, "ERROR: TP_RND is out of the supported range.");
    static_assert(TP_SAT >= SAT_MODE_MIN && TP_SAT <= SAT_MODE_MAX, "ERROR: TP_SAT is out of supported range");
    static_assert(TP_SAT != 2, "ERROR: TP_SAT is invalid. Valid values of TP_SAT are 0, 1, and 3");
    static_assert((TP_FIR_LEN % TP_INTERPOLATE_FACTOR) == 0,
                  "ERROR: TP_FIR_LEN must be an integer multiple of INTERPOLATE_FACTOR.");
    static_assert(fnEnumType<TT_DATA>() != enumUnknownType, "ERROR: TT_DATA is not a supported type.");
    static_assert(fnEnumType<TT_COEFF>() != enumUnknownType, "ERROR: TT_COEFF is not a supported type.");
    static_assert(fnTypeCheckDataCoeffSize<TT_DATA, TT_COEFF>() != 0,
                  "ERROR: TT_DATA type less precise than TT_COEFF is not supported.");
    static_assert(fnTypeCheckDataCoeffCmplx<TT_DATA, TT_COEFF>() != 0,
                  "ERROR: real TT_DATA with complex TT_COEFF is not supported.");
    static_assert(fnTypeCheckDataCoeffFltInt<TT_DATA, TT_COEFF>() != 0,
                  "ERROR: a mix of float and integer types of TT_DATA and TT_COEFF is not supported.");
    static_assert(TP_INTERPOLATE_FACTOR >= INTERPOLATE_FACTOR_MIN && TP_INTERPOLATE_FACTOR <= INTERPOLATE_FACTOR_MAX,
                  "ERROR: TP_INTERPOLATE_FACTOR is out of the supported range");
    static_assert(fnUnsupportedTypeCombo<TT_DATA, TT_COEFF>() != 0,
                  "ERROR: The combination of TT_DATA and TT_COEFF is not supported for this class.");
    static_assert(TP_NUM_OUTPUTS > 0 && TP_NUM_OUTPUTS <= 2, "ERROR: only single or dual outputs are supported.");
    static_assert(!(std::is_same<TT_DATA, cfloat>::value || std::is_same<TT_DATA, float>::value) || (TP_SHIFT == 0),
                  "ERROR: TP_SHIFT cannot be performed for TT_DATA=cfloat, so must be set to 0");
    // static_assert(!(std::is_same<TT_DATA,int16>::value && std::is_same<TT_COEFF,int32>::value) , "ERROR: The
    // combination of TT_DATA and TT_COEFF is currently not supported.");
    // There are additional defensive checks after architectural constants have been calculated.

    // The interpolation FIR calculates over multiple phases where such that the total number of lanes is an integer
    // multiple of the
    // interpolation factor. Hence an array of accumulators is needed for this set of lanes.
    static constexpr unsigned int m_kPermuteSupport = fnPermuteSupport();
    static constexpr unsigned int m_kNumAccRegs = fnAccRegsIntAsym<TT_DATA, TT_COEFF>();
    static constexpr unsigned int m_kWinAccessByteSize = 32;
    static constexpr unsigned int m_kColumns =
        fnNumColsIntAsym<TT_DATA, TT_COEFF, TP_API>(); // number of mult-adds per lane for main intrinsic
    static constexpr unsigned int m_kLanes =
        fnNumLanesIntAsym<TT_DATA, TT_COEFF, TP_API>(); // number of operations in parallel of this type combinations
                                                        // that the vector processor can do.
    static constexpr unsigned int m_kVOutSize =
        fnVOutSizeIntAsym<TT_DATA, TT_COEFF, TP_API>();  // This differs from kLanes for cint32/cint32
    static constexpr unsigned int m_kDataLoadsInReg = 4; // ratio of 1024-bit data buffer to 256-bit load size.
    static constexpr unsigned int m_kDataLoadVsize =
        (32 / sizeof(TT_DATA)); // number of samples in a single 256-bit load
    static constexpr unsigned int m_kSamplesInBuff = m_kDataLoadsInReg * m_kDataLoadVsize;

    static constexpr unsigned int m_kFirRangeOffset =
        fnFirRangeOffset<TP_FIR_LEN, TP_CASC_LEN, TP_KERNEL_POSITION, TP_INTERPOLATE_FACTOR>() /
        TP_INTERPOLATE_FACTOR; // FIR Cascade Offset for this kernel position
    static constexpr unsigned int m_kFirMarginLen = TP_FIR_LEN / TP_INTERPOLATE_FACTOR;
    static constexpr unsigned int m_kFirMarginOffset =
        fnFirMargin<m_kFirMarginLen, TT_DATA>() - m_kFirMarginLen + 1; // FIR Margin Offset.
    static constexpr unsigned int m_kFirInitOffset = m_kFirRangeOffset + m_kFirMarginOffset;
    static constexpr unsigned int m_kDataWindowOffset =
        TRUNC((m_kFirInitOffset), (m_kWinAccessByteSize / sizeof(TT_DATA))); // Window offset - increments by 128bit
    static constexpr unsigned int m_kDataBuffXOffset =
        m_kFirInitOffset % (m_kWinAccessByteSize / sizeof(TT_DATA)); // Remainder of m_kFirInitOffset divided by 128bit
    // In some cases, the number of accumulators needed exceeds the number available in the processor leading to
    // inefficency as the
    // accumulators are loaded and stored on the stack. An alternative implementation is used to avoid this.
    static constexpr unsigned int m_kPermuteStreamArch =
        (((m_kDataBuffXOffset + TP_FIR_RANGE_LEN + m_kLanes * m_kDataLoadVsize / m_kVOutSize) <= m_kSamplesInBuff) &&
         (TP_INPUT_WINDOW_VSIZE % (m_kLanes * m_kDataLoadsInReg) == 0))
            ? kArchStreamPhaseSeries
            :                       // execute incremental load streaming architecture
            kArchStreamPhaseSeries; // placeholder for a minicash
    static constexpr unsigned int m_kPermuteWindowArch =
        (((m_kDataBuffXOffset + TP_FIR_RANGE_LEN + m_kLanes * m_kDataLoadVsize / m_kVOutSize) <= m_kSamplesInBuff) &&
         (TP_INPUT_WINDOW_VSIZE % (m_kLanes * m_kDataLoadsInReg) == 0))
            ? kArchIncr
            :                 // execute incremental load architecture
            kArchPhaseSeries; // execute each phase in series (reloads data)
    static constexpr unsigned int m_kPermuteArch =
        TP_API == USE_WINDOW_API ? m_kPermuteWindowArch : m_kPermuteStreamArch; // Select Stream od Windowed arches
    static constexpr unsigned int m_kDecomposedArch =
        TP_API == USE_WINDOW_API ? kArchPhaseParallel : kArchStreamPhaseParallel;
    ; // execute each phase in parallel. No ArchIncr equivalent, as marginal benefit in QoR.  or Stream arch? Later
    static constexpr unsigned int m_kArch = m_kPermuteSupport == 1 ? m_kPermuteArch : m_kDecomposedArch;
    static constexpr unsigned int m_kZbuffSize = 32;
    static constexpr unsigned int m_kCoeffRegVsize = m_kZbuffSize / sizeof(TT_COEFF);
    static constexpr unsigned int m_kTotalLanes =
        fnLCMIntAsym<TT_DATA, TT_COEFF, TP_API, TP_INTERPOLATE_FACTOR>(); // Lowest common multiple of Lanes and
                                                                          // Interpolatefactor
    static constexpr unsigned int m_kLCMPhases = m_kTotalLanes / m_kLanes;
    static constexpr unsigned int m_kPhases = TP_INTERPOLATE_FACTOR;
    static constexpr unsigned int m_kNumOps = CEIL(TP_FIR_RANGE_LEN / TP_INTERPOLATE_FACTOR, m_kColumns) / m_kColumns;
    static constexpr unsigned int m_kLsize = TP_INPUT_WINDOW_VSIZE / m_kLanes; // loops required to consume input
    static constexpr unsigned int m_kInitialLoads =
        (m_kDataBuffXOffset + (m_kPhases * m_kLanes + m_kVOutSize) / TP_INTERPOLATE_FACTOR + m_kColumns - 1 +
         (m_kLanes - 1)) /
        m_kLanes; // effectively ceil[(kVOutsize+m_kColumns-1)/kLanes]
    static constexpr unsigned int m_kInitialLoadsIncr =
        CEIL(m_kDataBuffXOffset + TP_FIR_RANGE_LEN + m_kLanes * m_kDataLoadVsize / m_kVOutSize - 1, m_kDataLoadVsize) /
        m_kDataLoadVsize;
    static constexpr unsigned int m_kRepeatFactor =
        m_kDataLoadVsize / m_kVOutSize == 2 ? m_kDataLoadsInReg * 2
                                            : m_kDataLoadsInReg; // repeat x2 when dataloads to outsize ratio is 2, as
                                                                 // data is only loaded every 2nd iteration.

    template <unsigned int x>
    static constexpr int fintDataNeeded() {
        if
            constexpr(x == 0) { return 0; }
        else {
            int dataNeeded = 1 + (int)((x - 1) / TP_INTERPOLATE_FACTOR);
            return dataNeeded;
        }
    };

    // streaming architecture
    static constexpr unsigned int add1 =
        CEIL(TP_FIR_RANGE_LEN / TP_INTERPOLATE_FACTOR, m_kColumns) / m_kColumns -
        TP_FIR_RANGE_LEN / TP_INTERPOLATE_FACTOR /
            m_kColumns; // used to accomodate for the position of coefficients when ceiling with m_kColumns
    static constexpr int coefRangeStartIndex =
        TP_FIR_LEN - TP_FIR_RANGE_LEN - m_kFirRangeOffset * TP_INTERPOLATE_FACTOR;
    static constexpr int streamInitNullAccs = coefRangeStartIndex / m_kVOutSize;
    static constexpr int phaseOffset = streamInitNullAccs % m_kLCMPhases;
    static constexpr int m_kRepFactPhases = m_kRepeatFactor * m_kPhases;
    static constexpr int streamInitAccs =
        (CEIL(streamInitNullAccs, m_kRepFactPhases) - CEIL(streamInitNullAccs, m_kPhases)) / m_kPhases;

    static constexpr int initPhaseDataAccesses =
        fintDataNeeded<TP_FIR_LEN - TP_FIR_RANGE_LEN - m_kFirRangeOffset * TP_INTERPOLATE_FACTOR>();
    static constexpr int phase1Reads =
        fintDataNeeded<(CEIL(streamInitNullAccs, m_kRepFactPhases) * m_kLanes)>() - initPhaseDataAccesses;
    static constexpr int sizeOf1Read = (TP_DUAL_IP == 0) ? m_kDataLoadVsize / 2 : m_kDataLoadVsize;
    static constexpr int indexToWrite = (CEIL(phase1Reads, sizeOf1Read)) / sizeOf1Read;
    static constexpr int totalNeededDataPerStrobe = fintDataNeeded<(m_kLanes * m_kPhases)>();
    static constexpr int startIndex = 2; // arbitrarily chosen - defines the default initial index of the sbuff to write
    static constexpr int startIndexCasc =
        (startIndex + indexToWrite) %
        (m_kSamplesInBuff / sizeOf1Read); // modified startIndex, used in cascaded kernels if streamInitNullAccs!=0

    static constexpr int actualMarginOffset =
        CEIL(TP_MODIFY_MARGIN_OFFSET * -1, TP_INTERPOLATE_FACTOR) / TP_INTERPOLATE_FACTOR;
    static constexpr int m_kXStart = startIndex * sizeOf1Read - (m_kColumns * (m_kNumOps - 1) + m_kColumns - 1) + add1 -
                                     coefRangeStartIndex / TP_INTERPOLATE_FACTOR;
    static constexpr int streamInitNullStrobes = CEIL(streamInitNullAccs, m_kPhases) / m_kPhases;

    alignas(__ALIGN_BYTE_SIZE__) int delay[(1024 / 8) / sizeof(int)] = {0};
    int doInit = (streamInitNullAccs == 0) ? 0 : 1;
    // Additional defensive checks
    struct ssr_params : public fir_params_defaults {
        using BTT_DATA = TT_DATA;
        using BTT_COEFF = TT_COEFF;
        static constexpr unsigned int BTP_FIR_LEN = TP_FIR_LEN;
        static constexpr unsigned int BTP_INTERPOLATE_FACTOR = TP_INTERPOLATE_FACTOR;
        static constexpr unsigned int BTP_CASC_LEN = TP_CASC_LEN;
    };
    static_assert(TP_INPUT_WINDOW_VSIZE % m_kLanes == 0,
                  "ERROR: TP_INPUT_WINDOW_VSIZE must be an integer multiple of the number of lanes for this data type");
    static_assert(TP_API == USE_WINDOW_API ||
                      (fir_interpolate_asym_tl<ssr_params>::template fnCheckIfFits<TP_KERNEL_POSITION,
                                                                                   TP_CASC_LEN,
                                                                                   TP_FIR_LEN,
                                                                                   TT_DATA,
                                                                                   TT_COEFF,
                                                                                   TP_DUAL_IP,
                                                                                   TP_INTERPOLATE_FACTOR>() == 1),
                  "ERROR: FIR_LENGTH is too large for the number of kernels with Stream API, increase TP_CASC_LEN");
    static_assert(TP_DUAL_IP == 0 || TP_API == USE_STREAM_API,
                  "Error: Dual input feature is only supported for stream API");

    static constexpr int tapsArraySize =
        CEIL(CEIL(TP_FIR_RANGE_LEN, m_kPhases) / m_kPhases, (256 / 8 / sizeof(TT_COEFF))); // ceil'ed to 256-bits
    // The m_internalTaps is defined in terms of samples, but loaded into a vector, so has to be memory-aligned to the
    // vector size.
    alignas(__ALIGN_BYTE_SIZE__) TT_COEFF m_internalTaps[m_kLCMPhases][m_kNumOps][m_kColumns][m_kLanes];
    alignas(__ALIGN_BYTE_SIZE__) TT_COEFF m_internalTaps2[m_kPhases][tapsArraySize];

    // Two implementations have been written for this filter. They have identical behaviour, but one is optimised for an
    // Interpolation factor
    // greater than the number of accumulator registers available.
    void filterPhaseSeries(T_inputIF<TP_CASC_IN, TT_DATA, TP_DUAL_IP> inInterface,
                           T_outputIF<TP_CASC_OUT, TT_DATA> outInterface); // Each phase is calculated in turn which
                                                                           // avoids need for multiple accumulators, but
                                                                           // requires data reloading.
    void filterPhaseParallel(
        T_inputIF<TP_CASC_IN, TT_DATA, TP_DUAL_IP> inInterface,
        T_outputIF<TP_CASC_OUT, TT_DATA> outInterface); // Parallel phase execution, requires multiple accumulators
    void filterIncr(
        T_inputIF<TP_CASC_IN, TT_DATA, TP_DUAL_IP> inInterface,
        T_outputIF<TP_CASC_OUT, TT_DATA> outInterface); // Incremental load architecture which applies for short FIR_LEN
    void filterSelectArch(T_inputIF<TP_CASC_IN, TT_DATA, TP_DUAL_IP> inInterface,
                          T_outputIF<TP_CASC_OUT, TT_DATA> outInterface);
    void filterStream(T_inputIF<TP_CASC_IN, TT_DATA, TP_DUAL_IP> inInterface,
                      T_outputIF<TP_CASC_OUT, TT_DATA> outInterface); // architecture for streaming interfaces
    void filterStreamPhaseParallel(
        T_inputIF<TP_CASC_IN, TT_DATA, TP_DUAL_IP> inInterface,
        T_outputIF<TP_CASC_OUT, TT_DATA> outInterface); // architecture for streaming interfaces
    // Constants for coeff reload
    static constexpr unsigned int m_kCoeffLoadSize = 256 / 8 / sizeof(TT_COEFF);
    alignas(__ALIGN_BYTE_SIZE__) TT_COEFF
        m_rawInTaps[CEIL(TP_COEFF_PHASES_LEN, m_kCoeffLoadSize)]; // Previous user input coefficients with zero padding
    bool isUpdateRequired;                                        // Are coefficients sets equal?

   public:
    // Access function for AIE Synthesizer
    static unsigned int get_m_kArch() { return m_kArch; };

    // Constructors
    kernelFilterClass() : m_rawInTaps{} {}

    kernelFilterClass(const TT_COEFF (&taps)[TP_FIR_LEN]) {
        // Loads taps/coefficients
        firReload(taps);
    };

    void firReload(const TT_COEFF* taps) {
        TT_COEFF* tapsPtr = (TT_COEFF*)taps;
        firReload(tapsPtr);
    }

    void firReload(TT_COEFF* taps) {
        // Since the intrinsics can have columns, any values in memory beyond the end of the taps array could
        // contaminate the calculation.
        // To avoid this hazard, the class has its own taps array which is zero-padded to the column width for the type
        // of coefficient.
        int tapIndex;
        // Coefficients are pre-arranged such that during filter execution they may simply be read from a lookup table.
        for (int phase = 0; phase < m_kLCMPhases; ++phase) {
            for (int op = 0; op < m_kNumOps; ++op) {
                for (int column = 0; column < m_kColumns; ++column) {
                    for (int lane = 0; lane < m_kLanes; ++lane) {
                        tapIndex = TP_INTERPOLATE_FACTOR - 1 -
                                   ((lane + phase * m_kLanes + TP_MODIFY_MARGIN_OFFSET + TP_INTERPOLATE_FACTOR) %
                                    TP_INTERPOLATE_FACTOR) +          // datum index of lane
                                   (column * TP_INTERPOLATE_FACTOR) + // column offset is additive
                                   ((op * m_kColumns * TP_INTERPOLATE_FACTOR));

                        if (tapIndex < TP_FIR_RANGE_LEN && tapIndex >= 0) {
                            tapIndex = TP_FIR_LEN - 1 - tapIndex -
                                       fnFirRangeOffset<TP_FIR_LEN, TP_CASC_LEN, TP_KERNEL_POSITION,
                                                        TP_INTERPOLATE_FACTOR>(); // Reverse coefficients and apply
                                                                                  // cascade range offset. See note at
                                                                                  // head of file.
                            m_internalTaps[phase][op][column][lane] = taps[tapIndex];

                        } else {
                            m_internalTaps[phase][op][column][lane] = nullElem<TT_COEFF>(); // 0 for the type.
                        }
                    }
                }
            }
        }
        for (int phase = 0; phase < m_kPhases; ++phase) {
            for (int i = 0; i < tapsArraySize; i++) { // ceiled to columns.
                int tapIndex = i * m_kPhases + (m_kPhases - 1 - phase);
                int tapsAddress =
                    TP_FIR_LEN - 1 -
                    fnFirRangeOffset<TP_FIR_LEN, TP_CASC_LEN, TP_KERNEL_POSITION, TP_INTERPOLATE_FACTOR>() - tapIndex;
                if (tapsAddress < 0 || tapIndex >= TP_FIR_RANGE_LEN) {
                    m_internalTaps2[phase][i] = nullElem<TT_COEFF>();
                } else {
                    m_internalTaps2[phase][i] = taps[tapsAddress];
                }
            }
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
        // Since the intrinsics can have columns, any values in memory beyond the end of the taps array could
        // contaminate the calculation.
        // To avoid this hazard, the class has its own taps array which is zero-padded to the column width for the type
        // of coefficient.
        int tapIndex;
        // Coefficients are pre-arranged such that during filter execution they may simply be read from a lookup table.
        for (int phase = 0; phase < m_kLCMPhases; ++phase) {
            for (int op = 0; op < m_kNumOps; ++op) {
                for (int column = 0; column < m_kColumns; ++column) {
                    for (int lane = 0; lane < m_kLanes; ++lane) {
                        tapIndex = TP_INTERPOLATE_FACTOR - 1 -
                                   ((lane + phase * m_kLanes + TP_MODIFY_MARGIN_OFFSET + TP_INTERPOLATE_FACTOR) %
                                    TP_INTERPOLATE_FACTOR) +          // datum index of lane
                                   (column * TP_INTERPOLATE_FACTOR) + // column offset is additive
                                   ((op * m_kColumns * TP_INTERPOLATE_FACTOR));

                        if (tapIndex < TP_FIR_RANGE_LEN && tapIndex >= 0) {
                            unsigned int tapsOffset =
                                tapIndex * coeffPhases + (coeffPhases - 1 - coeffPhase) + coeffPhaseOffset;
                            unsigned int tapsAddress =
                                coeffPhasesLen - 1 - tapsOffset -
                                coeffPhases * fnFirRangeOffset<TP_FIR_LEN, TP_CASC_LEN, TP_KERNEL_POSITION,
                                                               TP_INTERPOLATE_FACTOR>(); // Reverse coefficients and
                                                                                         // apply cascade range offset.
                                                                                         // See note at head of file.
                            m_internalTaps[phase][op][column][lane] = taps[tapsAddress];

                        } else {
                            m_internalTaps[phase][op][column][lane] = nullElem<TT_COEFF>(); // 0 for the type.
                        }
                    }
                }
            }
        }
        for (int phase = 0; phase < m_kPhases; ++phase) {
            for (int i = 0; i < tapsArraySize; i++) { // ceiled to columns.
                int tapIndex = i * m_kPhases + (m_kPhases - 1 - phase);
                int tapsOffset = tapIndex * coeffPhases + (coeffPhases - 1 - coeffPhase) + coeffPhaseOffset;
                int tapsAddress =
                    coeffPhasesLen - 1 -
                    coeffPhases *
                        fnFirRangeOffset<TP_FIR_LEN, TP_CASC_LEN, TP_KERNEL_POSITION, TP_INTERPOLATE_FACTOR>() -
                    tapsOffset;
                if (tapsAddress < 0 || tapIndex >= TP_FIR_RANGE_LEN) {
                    m_internalTaps2[phase][i] = nullElem<TT_COEFF>();
                } else {
                    m_internalTaps2[phase][i] = taps[tapsAddress];
                }
            }
        }
    };

    // Filter kernel for static coefficient designs
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
// Cascade layer class and specializations

//-----------------------------------------------------------------------------------------------------
// This is the main declaration of the fir_interpolate_asym class, and is also used for the Standalone kernel
// Standalone kernel specialization. Windowed. no cascade ports, no reload, single output
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_INTERPOLATE_FACTOR,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          bool TP_CASC_IN,
          bool TP_CASC_OUT,
          unsigned int TP_FIR_RANGE_LEN,
          unsigned int TP_KERNEL_POSITION,
          unsigned int TP_CASC_LEN,
          unsigned int TP_USE_COEFF_RELOAD,
          unsigned int TP_DUAL_IP,
          unsigned int TP_NUM_OUTPUTS,
          unsigned int TP_API,
          int TP_MODIFY_MARGIN_OFFSET,
          unsigned int TP_COEFF_PHASE,
          unsigned int TP_COEFF_PHASE_OFFSET,
          unsigned int TP_COEFF_PHASES,
          unsigned int TP_COEFF_PHASES_LEN,
          unsigned int TP_SAT>
class fir_interpolate_asym : public kernelFilterClass<TT_DATA,
                                                      TT_COEFF,
                                                      TP_FIR_LEN,
                                                      TP_INTERPOLATE_FACTOR,
                                                      TP_SHIFT,
                                                      TP_RND,
                                                      TP_INPUT_WINDOW_VSIZE,
                                                      TP_CASC_IN,
                                                      TP_CASC_OUT,
                                                      TP_FIR_RANGE_LEN,
                                                      0,
                                                      1,
                                                      TP_USE_COEFF_RELOAD,
                                                      TP_DUAL_IP,
                                                      TP_NUM_OUTPUTS,
                                                      TP_API,
                                                      TP_MODIFY_MARGIN_OFFSET,
                                                      TP_COEFF_PHASE,
                                                      TP_COEFF_PHASE_OFFSET,
                                                      TP_COEFF_PHASES,
                                                      TP_COEFF_PHASES_LEN,
                                                      TP_SAT> {
   private:
   public:
    // Constructor
    fir_interpolate_asym(const TT_COEFF (&taps)[TP_FIR_LEN])
        : kernelFilterClass<TT_DATA,
                            TT_COEFF,
                            TP_FIR_LEN,
                            TP_INTERPOLATE_FACTOR,
                            TP_SHIFT,
                            TP_RND,
                            TP_INPUT_WINDOW_VSIZE,
                            TP_CASC_IN,
                            TP_CASC_OUT,
                            TP_FIR_RANGE_LEN,
                            0,
                            1,
                            TP_USE_COEFF_RELOAD,
                            TP_DUAL_IP,
                            TP_NUM_OUTPUTS,
                            TP_API,
                            TP_MODIFY_MARGIN_OFFSET,
                            TP_COEFF_PHASE,
                            TP_COEFF_PHASE_OFFSET,
                            TP_COEFF_PHASES,
                            TP_COEFF_PHASES_LEN,
                            TP_SAT>(taps) {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fir_interpolate_asym::filter); }

    // FIR
    void filter(
        input_circular_buffer<TT_DATA,
                              extents<inherited_extent>,
                              margin<fnFirMargin<TP_FIR_LEN / TP_INTERPOLATE_FACTOR, TT_DATA>()> >& __restrict inWindow,
        output_circular_buffer<TT_DATA>& __restrict outWindow);
};

// Single kernel specialization. Windowed. No cascade ports, static coefficients. dual output
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_INTERPOLATE_FACTOR,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_FIR_RANGE_LEN,
          unsigned int TP_KERNEL_POSITION,
          unsigned int TP_CASC_LEN,
          int TP_MODIFY_MARGIN_OFFSET,
          unsigned int TP_COEFF_PHASE,
          unsigned int TP_COEFF_PHASE_OFFSET,
          unsigned int TP_COEFF_PHASES,
          unsigned int TP_COEFF_PHASES_LEN,
          unsigned int TP_SAT>
class fir_interpolate_asym<TT_DATA,
                           TT_COEFF,
                           TP_FIR_LEN,
                           TP_INTERPOLATE_FACTOR,
                           TP_SHIFT,
                           TP_RND,
                           TP_INPUT_WINDOW_VSIZE,
                           CASC_IN_FALSE,
                           CASC_OUT_FALSE,
                           TP_FIR_RANGE_LEN,
                           TP_KERNEL_POSITION,
                           TP_CASC_LEN,
                           USE_COEFF_RELOAD_FALSE,
                           DUAL_IP_SINGLE,
                           2,
                           USE_WINDOW_API,
                           TP_MODIFY_MARGIN_OFFSET,
                           TP_COEFF_PHASE,
                           TP_COEFF_PHASE_OFFSET,
                           TP_COEFF_PHASES,
                           TP_COEFF_PHASES_LEN,
                           TP_SAT> : public kernelFilterClass<TT_DATA,
                                                              TT_COEFF,
                                                              TP_FIR_LEN,
                                                              TP_INTERPOLATE_FACTOR,
                                                              TP_SHIFT,
                                                              TP_RND,
                                                              TP_INPUT_WINDOW_VSIZE,
                                                              CASC_IN_FALSE,
                                                              CASC_OUT_FALSE,
                                                              TP_FIR_RANGE_LEN,
                                                              TP_KERNEL_POSITION,
                                                              TP_CASC_LEN,
                                                              USE_COEFF_RELOAD_FALSE,
                                                              DUAL_IP_SINGLE,
                                                              2,
                                                              USE_WINDOW_API,
                                                              TP_MODIFY_MARGIN_OFFSET,
                                                              TP_COEFF_PHASE,
                                                              TP_COEFF_PHASE_OFFSET,
                                                              TP_COEFF_PHASES,
                                                              TP_COEFF_PHASES_LEN,
                                                              TP_SAT> {
   private:
   public:
    // Constructor
    fir_interpolate_asym(const TT_COEFF (&taps)[TP_FIR_LEN])
        : kernelFilterClass<TT_DATA,
                            TT_COEFF,
                            TP_FIR_LEN,
                            TP_INTERPOLATE_FACTOR,
                            TP_SHIFT,
                            TP_RND,
                            TP_INPUT_WINDOW_VSIZE,
                            CASC_IN_FALSE,
                            CASC_OUT_FALSE,
                            TP_FIR_LEN,
                            0,
                            1,
                            USE_COEFF_RELOAD_FALSE,
                            DUAL_IP_SINGLE,
                            2,
                            USE_WINDOW_API,
                            TP_MODIFY_MARGIN_OFFSET,
                            TP_COEFF_PHASE,
                            TP_COEFF_PHASE_OFFSET,
                            TP_COEFF_PHASES,
                            TP_COEFF_PHASES_LEN,
                            TP_SAT>(taps) {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fir_interpolate_asym::filter); }

    // FIR
    void filter(
        input_circular_buffer<TT_DATA,
                              extents<inherited_extent>,
                              margin<fnFirMargin<TP_FIR_LEN / TP_INTERPOLATE_FACTOR, TT_DATA>()> >& __restrict inWindow,
        output_circular_buffer<TT_DATA>& __restrict outWindow,
        output_circular_buffer<TT_DATA>& __restrict outWindow2);
};

//-----------------------------------------------------------------------------------------------------
// Single kernel specialization. Windowed. No cascade ports, with reload coefficients, single output
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_INTERPOLATE_FACTOR,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_FIR_RANGE_LEN,
          unsigned int TP_KERNEL_POSITION,
          unsigned int TP_CASC_LEN,
          int TP_MODIFY_MARGIN_OFFSET,
          unsigned int TP_COEFF_PHASE,
          unsigned int TP_COEFF_PHASE_OFFSET,
          unsigned int TP_COEFF_PHASES,
          unsigned int TP_COEFF_PHASES_LEN,
          unsigned int TP_SAT>
class fir_interpolate_asym<TT_DATA,
                           TT_COEFF,
                           TP_FIR_LEN,
                           TP_INTERPOLATE_FACTOR,
                           TP_SHIFT,
                           TP_RND,
                           TP_INPUT_WINDOW_VSIZE,
                           CASC_IN_FALSE,
                           CASC_OUT_FALSE,
                           TP_FIR_RANGE_LEN,
                           TP_KERNEL_POSITION,
                           TP_CASC_LEN,
                           USE_COEFF_RELOAD_TRUE,
                           DUAL_IP_SINGLE,
                           1,
                           USE_WINDOW_API,
                           TP_MODIFY_MARGIN_OFFSET,
                           TP_COEFF_PHASE,
                           TP_COEFF_PHASE_OFFSET,
                           TP_COEFF_PHASES,
                           TP_COEFF_PHASES_LEN,
                           TP_SAT> : public kernelFilterClass<TT_DATA,
                                                              TT_COEFF,
                                                              TP_FIR_LEN,
                                                              TP_INTERPOLATE_FACTOR,
                                                              TP_SHIFT,
                                                              TP_RND,
                                                              TP_INPUT_WINDOW_VSIZE,
                                                              CASC_IN_FALSE,
                                                              CASC_OUT_FALSE,
                                                              TP_FIR_RANGE_LEN,
                                                              TP_KERNEL_POSITION,
                                                              TP_CASC_LEN,
                                                              USE_COEFF_RELOAD_TRUE,
                                                              DUAL_IP_SINGLE,
                                                              1,
                                                              USE_WINDOW_API,
                                                              TP_MODIFY_MARGIN_OFFSET,
                                                              TP_COEFF_PHASE,
                                                              TP_COEFF_PHASE_OFFSET,
                                                              TP_COEFF_PHASES,
                                                              TP_COEFF_PHASES_LEN,
                                                              TP_SAT> {
   public:
    // Constructor
    fir_interpolate_asym()
        : kernelFilterClass<TT_DATA,
                            TT_COEFF,
                            TP_FIR_LEN,
                            TP_INTERPOLATE_FACTOR,
                            TP_SHIFT,
                            TP_RND,
                            TP_INPUT_WINDOW_VSIZE,
                            CASC_IN_FALSE,
                            CASC_OUT_FALSE,
                            TP_FIR_LEN,
                            0,
                            1,
                            USE_COEFF_RELOAD_TRUE,
                            DUAL_IP_SINGLE,
                            1,
                            USE_WINDOW_API,
                            TP_MODIFY_MARGIN_OFFSET,
                            TP_COEFF_PHASE,
                            TP_COEFF_PHASE_OFFSET,
                            TP_COEFF_PHASES,
                            TP_COEFF_PHASES_LEN,
                            TP_SAT>() {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fir_interpolate_asym::filter); }

    // FIR
    void filter(
        input_circular_buffer<TT_DATA,
                              extents<inherited_extent>,
                              margin<fnFirMargin<TP_FIR_LEN / TP_INTERPOLATE_FACTOR, TT_DATA>()> >& __restrict inWindow,
        output_circular_buffer<TT_DATA>& __restrict outWindow,
        const TT_COEFF (&inTaps)[TP_COEFF_PHASES_LEN]);
};

// Single kernel specialization. No cascade ports, Windowed. with reload coefficients, dual output
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_INTERPOLATE_FACTOR,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_FIR_RANGE_LEN,
          unsigned int TP_KERNEL_POSITION,
          unsigned int TP_CASC_LEN,
          int TP_MODIFY_MARGIN_OFFSET,
          unsigned int TP_COEFF_PHASE,
          unsigned int TP_COEFF_PHASE_OFFSET,
          unsigned int TP_COEFF_PHASES,
          unsigned int TP_COEFF_PHASES_LEN,
          unsigned int TP_SAT>
class fir_interpolate_asym<TT_DATA,
                           TT_COEFF,
                           TP_FIR_LEN,
                           TP_INTERPOLATE_FACTOR,
                           TP_SHIFT,
                           TP_RND,
                           TP_INPUT_WINDOW_VSIZE,
                           CASC_IN_FALSE,
                           CASC_OUT_FALSE,
                           TP_FIR_RANGE_LEN,
                           TP_KERNEL_POSITION,
                           TP_CASC_LEN,
                           USE_COEFF_RELOAD_TRUE,
                           DUAL_IP_SINGLE,
                           2,
                           USE_WINDOW_API,
                           TP_MODIFY_MARGIN_OFFSET,
                           TP_COEFF_PHASE,
                           TP_COEFF_PHASE_OFFSET,
                           TP_COEFF_PHASES,
                           TP_COEFF_PHASES_LEN,
                           TP_SAT> : public kernelFilterClass<TT_DATA,
                                                              TT_COEFF,
                                                              TP_FIR_LEN,
                                                              TP_INTERPOLATE_FACTOR,
                                                              TP_SHIFT,
                                                              TP_RND,
                                                              TP_INPUT_WINDOW_VSIZE,
                                                              CASC_IN_FALSE,
                                                              CASC_OUT_FALSE,
                                                              TP_FIR_RANGE_LEN,
                                                              TP_KERNEL_POSITION,
                                                              TP_CASC_LEN,
                                                              USE_COEFF_RELOAD_TRUE,
                                                              DUAL_IP_SINGLE,
                                                              2,
                                                              USE_WINDOW_API,
                                                              TP_MODIFY_MARGIN_OFFSET,
                                                              TP_COEFF_PHASE,
                                                              TP_COEFF_PHASE_OFFSET,
                                                              TP_COEFF_PHASES,
                                                              TP_COEFF_PHASES_LEN,
                                                              TP_SAT> {
   public:
    // Constructor
    fir_interpolate_asym()
        : kernelFilterClass<TT_DATA,
                            TT_COEFF,
                            TP_FIR_LEN,
                            TP_INTERPOLATE_FACTOR,
                            TP_SHIFT,
                            TP_RND,
                            TP_INPUT_WINDOW_VSIZE,
                            CASC_IN_FALSE,
                            CASC_OUT_FALSE,
                            TP_FIR_LEN,
                            0,
                            1,
                            USE_COEFF_RELOAD_TRUE,
                            DUAL_IP_SINGLE,
                            2,
                            USE_WINDOW_API,
                            TP_MODIFY_MARGIN_OFFSET,
                            TP_COEFF_PHASE,
                            TP_COEFF_PHASE_OFFSET,
                            TP_COEFF_PHASES,
                            TP_COEFF_PHASES_LEN,
                            TP_SAT>() {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fir_interpolate_asym::filter); }

    // FIR
    void filter(
        input_circular_buffer<TT_DATA,
                              extents<inherited_extent>,
                              margin<fnFirMargin<TP_FIR_LEN / TP_INTERPOLATE_FACTOR, TT_DATA>()> >& __restrict inWindow,
        output_circular_buffer<TT_DATA>& __restrict outWindow,
        output_circular_buffer<TT_DATA>& __restrict outWindow2,
        const TT_COEFF (&inTaps)[TP_COEFF_PHASES_LEN]);
};

//-----------------------------------------------------------------------------------------------------
// Partially specialized classes for cascaded interface (final kernel in cascade), Windowed. no reload, single output
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_INTERPOLATE_FACTOR,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_FIR_RANGE_LEN,
          unsigned int TP_KERNEL_POSITION,
          unsigned int TP_CASC_LEN,
          int TP_MODIFY_MARGIN_OFFSET,
          unsigned int TP_COEFF_PHASE,
          unsigned int TP_COEFF_PHASE_OFFSET,
          unsigned int TP_COEFF_PHASES,
          unsigned int TP_COEFF_PHASES_LEN,
          unsigned int TP_SAT>
class fir_interpolate_asym<TT_DATA,
                           TT_COEFF,
                           TP_FIR_LEN,
                           TP_INTERPOLATE_FACTOR,
                           TP_SHIFT,
                           TP_RND,
                           TP_INPUT_WINDOW_VSIZE,
                           CASC_IN_TRUE,
                           CASC_OUT_FALSE,
                           TP_FIR_RANGE_LEN,
                           TP_KERNEL_POSITION,
                           TP_CASC_LEN,
                           USE_COEFF_RELOAD_FALSE,
                           DUAL_IP_SINGLE,
                           1,
                           USE_WINDOW_API,
                           TP_MODIFY_MARGIN_OFFSET,
                           TP_COEFF_PHASE,
                           TP_COEFF_PHASE_OFFSET,
                           TP_COEFF_PHASES,
                           TP_COEFF_PHASES_LEN,
                           TP_SAT> : public kernelFilterClass<TT_DATA,
                                                              TT_COEFF,
                                                              TP_FIR_LEN,
                                                              TP_INTERPOLATE_FACTOR,
                                                              TP_SHIFT,
                                                              TP_RND,
                                                              TP_INPUT_WINDOW_VSIZE,
                                                              CASC_IN_TRUE,
                                                              CASC_OUT_FALSE,
                                                              TP_FIR_RANGE_LEN,
                                                              TP_KERNEL_POSITION,
                                                              TP_CASC_LEN,
                                                              USE_COEFF_RELOAD_FALSE,
                                                              DUAL_IP_SINGLE,
                                                              1,
                                                              USE_WINDOW_API,
                                                              TP_MODIFY_MARGIN_OFFSET,
                                                              TP_COEFF_PHASE,
                                                              TP_COEFF_PHASE_OFFSET,
                                                              TP_COEFF_PHASES,
                                                              TP_COEFF_PHASES_LEN,
                                                              TP_SAT> {
   public:
    // Constructor
    fir_interpolate_asym(const TT_COEFF (&taps)[TP_FIR_LEN])
        : kernelFilterClass<TT_DATA,
                            TT_COEFF,
                            TP_FIR_LEN,
                            TP_INTERPOLATE_FACTOR,
                            TP_SHIFT,
                            TP_RND,
                            TP_INPUT_WINDOW_VSIZE,
                            CASC_IN_TRUE,
                            CASC_OUT_FALSE,
                            TP_FIR_RANGE_LEN,
                            TP_KERNEL_POSITION,
                            TP_CASC_LEN,
                            USE_COEFF_RELOAD_FALSE,
                            DUAL_IP_SINGLE,
                            1,
                            USE_WINDOW_API,
                            TP_MODIFY_MARGIN_OFFSET,
                            TP_COEFF_PHASE,
                            TP_COEFF_PHASE_OFFSET,
                            TP_COEFF_PHASES,
                            TP_COEFF_PHASES_LEN,
                            TP_SAT>(taps) {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fir_interpolate_asym::filter); }

    // FIR
    void filter(input_async_buffer<TT_DATA, extents<inherited_extent> >& inWindow,
                input_cascade_cacc* inCascade,
                output_circular_buffer<TT_DATA>& __restrict outWindow);
};

// Partially specialized classes for cascaded interface (final kernel in cascade), Windowed. no reload, dual output
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_INTERPOLATE_FACTOR,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_FIR_RANGE_LEN,
          unsigned int TP_KERNEL_POSITION,
          unsigned int TP_CASC_LEN,
          int TP_MODIFY_MARGIN_OFFSET,
          unsigned int TP_COEFF_PHASE,
          unsigned int TP_COEFF_PHASE_OFFSET,
          unsigned int TP_COEFF_PHASES,
          unsigned int TP_COEFF_PHASES_LEN,
          unsigned int TP_SAT>
class fir_interpolate_asym<TT_DATA,
                           TT_COEFF,
                           TP_FIR_LEN,
                           TP_INTERPOLATE_FACTOR,
                           TP_SHIFT,
                           TP_RND,
                           TP_INPUT_WINDOW_VSIZE,
                           CASC_IN_TRUE,
                           CASC_OUT_FALSE,
                           TP_FIR_RANGE_LEN,
                           TP_KERNEL_POSITION,
                           TP_CASC_LEN,
                           USE_COEFF_RELOAD_FALSE,
                           DUAL_IP_SINGLE,
                           2,
                           USE_WINDOW_API,
                           TP_MODIFY_MARGIN_OFFSET,
                           TP_COEFF_PHASE,
                           TP_COEFF_PHASE_OFFSET,
                           TP_COEFF_PHASES,
                           TP_COEFF_PHASES_LEN,
                           TP_SAT> : public kernelFilterClass<TT_DATA,
                                                              TT_COEFF,
                                                              TP_FIR_LEN,
                                                              TP_INTERPOLATE_FACTOR,
                                                              TP_SHIFT,
                                                              TP_RND,
                                                              TP_INPUT_WINDOW_VSIZE,
                                                              CASC_IN_TRUE,
                                                              CASC_OUT_FALSE,
                                                              TP_FIR_RANGE_LEN,
                                                              TP_KERNEL_POSITION,
                                                              TP_CASC_LEN,
                                                              USE_COEFF_RELOAD_FALSE,
                                                              DUAL_IP_SINGLE,
                                                              2,
                                                              USE_WINDOW_API,
                                                              TP_MODIFY_MARGIN_OFFSET,
                                                              TP_COEFF_PHASE,
                                                              TP_COEFF_PHASE_OFFSET,
                                                              TP_COEFF_PHASES,
                                                              TP_COEFF_PHASES_LEN,
                                                              TP_SAT> {
   public:
    // Constructor
    fir_interpolate_asym(const TT_COEFF (&taps)[TP_FIR_LEN])
        : kernelFilterClass<TT_DATA,
                            TT_COEFF,
                            TP_FIR_LEN,
                            TP_INTERPOLATE_FACTOR,
                            TP_SHIFT,
                            TP_RND,
                            TP_INPUT_WINDOW_VSIZE,
                            CASC_IN_TRUE,
                            CASC_OUT_FALSE,
                            TP_FIR_RANGE_LEN,
                            TP_KERNEL_POSITION,
                            TP_CASC_LEN,
                            USE_COEFF_RELOAD_FALSE,
                            DUAL_IP_SINGLE,
                            2,
                            USE_WINDOW_API,
                            TP_MODIFY_MARGIN_OFFSET,
                            TP_COEFF_PHASE,
                            TP_COEFF_PHASE_OFFSET,
                            TP_COEFF_PHASES,
                            TP_COEFF_PHASES_LEN,
                            TP_SAT>(taps) {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fir_interpolate_asym::filter); }

    // FIR
    void filter(input_async_buffer<TT_DATA, extents<inherited_extent> >& inWindow,
                input_cascade_cacc* inCascade,
                output_circular_buffer<TT_DATA>& __restrict outWindow,
                output_circular_buffer<TT_DATA>& __restrict outWindow2);
};

// Partially specialized classes for cascaded interface (final kernel in cascade), Windowed. with reload, single output
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_INTERPOLATE_FACTOR,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_FIR_RANGE_LEN,
          unsigned int TP_KERNEL_POSITION,
          unsigned int TP_CASC_LEN,
          int TP_MODIFY_MARGIN_OFFSET,
          unsigned int TP_COEFF_PHASE,
          unsigned int TP_COEFF_PHASE_OFFSET,
          unsigned int TP_COEFF_PHASES,
          unsigned int TP_COEFF_PHASES_LEN,
          unsigned int TP_SAT>
class fir_interpolate_asym<TT_DATA,
                           TT_COEFF,
                           TP_FIR_LEN,
                           TP_INTERPOLATE_FACTOR,
                           TP_SHIFT,
                           TP_RND,
                           TP_INPUT_WINDOW_VSIZE,
                           CASC_IN_TRUE,
                           CASC_OUT_FALSE,
                           TP_FIR_RANGE_LEN,
                           TP_KERNEL_POSITION,
                           TP_CASC_LEN,
                           USE_COEFF_RELOAD_TRUE,
                           DUAL_IP_SINGLE,
                           1,
                           USE_WINDOW_API,
                           TP_MODIFY_MARGIN_OFFSET,
                           TP_COEFF_PHASE,
                           TP_COEFF_PHASE_OFFSET,
                           TP_COEFF_PHASES,
                           TP_COEFF_PHASES_LEN,
                           TP_SAT> : public kernelFilterClass<TT_DATA,
                                                              TT_COEFF,
                                                              TP_FIR_LEN,
                                                              TP_INTERPOLATE_FACTOR,
                                                              TP_SHIFT,
                                                              TP_RND,
                                                              TP_INPUT_WINDOW_VSIZE,
                                                              CASC_IN_TRUE,
                                                              CASC_OUT_FALSE,
                                                              TP_FIR_RANGE_LEN,
                                                              TP_KERNEL_POSITION,
                                                              TP_CASC_LEN,
                                                              USE_COEFF_RELOAD_TRUE,
                                                              DUAL_IP_SINGLE,
                                                              1,
                                                              USE_WINDOW_API,
                                                              TP_MODIFY_MARGIN_OFFSET,
                                                              TP_COEFF_PHASE,
                                                              TP_COEFF_PHASE_OFFSET,
                                                              TP_COEFF_PHASES,
                                                              TP_COEFF_PHASES_LEN,
                                                              TP_SAT> {
   public:
    // Constructor
    fir_interpolate_asym()
        : kernelFilterClass<TT_DATA,
                            TT_COEFF,
                            TP_FIR_LEN,
                            TP_INTERPOLATE_FACTOR,
                            TP_SHIFT,
                            TP_RND,
                            TP_INPUT_WINDOW_VSIZE,
                            CASC_IN_TRUE,
                            CASC_OUT_FALSE,
                            TP_FIR_RANGE_LEN,
                            TP_KERNEL_POSITION,
                            TP_CASC_LEN,
                            USE_COEFF_RELOAD_TRUE,
                            DUAL_IP_SINGLE,
                            1,
                            USE_WINDOW_API,
                            TP_MODIFY_MARGIN_OFFSET,
                            TP_COEFF_PHASE,
                            TP_COEFF_PHASE_OFFSET,
                            TP_COEFF_PHASES,
                            TP_COEFF_PHASES_LEN,
                            TP_SAT>() {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fir_interpolate_asym::filter); }

    // FIR
    void filter(input_async_buffer<TT_DATA, extents<inherited_extent> >& inWindow,
                input_cascade_cacc* inCascade,
                output_circular_buffer<TT_DATA>& __restrict outWindow);
};

// Partially specialized classes for cascaded interface (final kernel in cascade), Windowed. with reload, dual output
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_INTERPOLATE_FACTOR,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_FIR_RANGE_LEN,
          unsigned int TP_KERNEL_POSITION,
          unsigned int TP_CASC_LEN,
          int TP_MODIFY_MARGIN_OFFSET,
          unsigned int TP_COEFF_PHASE,
          unsigned int TP_COEFF_PHASE_OFFSET,
          unsigned int TP_COEFF_PHASES,
          unsigned int TP_COEFF_PHASES_LEN,
          unsigned int TP_SAT>
class fir_interpolate_asym<TT_DATA,
                           TT_COEFF,
                           TP_FIR_LEN,
                           TP_INTERPOLATE_FACTOR,
                           TP_SHIFT,
                           TP_RND,
                           TP_INPUT_WINDOW_VSIZE,
                           CASC_IN_TRUE,
                           CASC_OUT_FALSE,
                           TP_FIR_RANGE_LEN,
                           TP_KERNEL_POSITION,
                           TP_CASC_LEN,
                           USE_COEFF_RELOAD_TRUE,
                           DUAL_IP_SINGLE,
                           2,
                           USE_WINDOW_API,
                           TP_MODIFY_MARGIN_OFFSET,
                           TP_COEFF_PHASE,
                           TP_COEFF_PHASE_OFFSET,
                           TP_COEFF_PHASES,
                           TP_COEFF_PHASES_LEN,
                           TP_SAT> : public kernelFilterClass<TT_DATA,
                                                              TT_COEFF,
                                                              TP_FIR_LEN,
                                                              TP_INTERPOLATE_FACTOR,
                                                              TP_SHIFT,
                                                              TP_RND,
                                                              TP_INPUT_WINDOW_VSIZE,
                                                              CASC_IN_TRUE,
                                                              CASC_OUT_FALSE,
                                                              TP_FIR_RANGE_LEN,
                                                              TP_KERNEL_POSITION,
                                                              TP_CASC_LEN,
                                                              USE_COEFF_RELOAD_TRUE,
                                                              DUAL_IP_SINGLE,
                                                              2,
                                                              USE_WINDOW_API,
                                                              TP_MODIFY_MARGIN_OFFSET,
                                                              TP_COEFF_PHASE,
                                                              TP_COEFF_PHASE_OFFSET,
                                                              TP_COEFF_PHASES,
                                                              TP_COEFF_PHASES_LEN,
                                                              TP_SAT> {
   public:
    // Constructor
    fir_interpolate_asym()
        : kernelFilterClass<TT_DATA,
                            TT_COEFF,
                            TP_FIR_LEN,
                            TP_INTERPOLATE_FACTOR,
                            TP_SHIFT,
                            TP_RND,
                            TP_INPUT_WINDOW_VSIZE,
                            CASC_IN_TRUE,
                            CASC_OUT_FALSE,
                            TP_FIR_RANGE_LEN,
                            TP_KERNEL_POSITION,
                            TP_CASC_LEN,
                            USE_COEFF_RELOAD_TRUE,
                            DUAL_IP_SINGLE,
                            2,
                            USE_WINDOW_API,
                            TP_MODIFY_MARGIN_OFFSET,
                            TP_COEFF_PHASE,
                            TP_COEFF_PHASE_OFFSET,
                            TP_COEFF_PHASES,
                            TP_COEFF_PHASES_LEN,
                            TP_SAT>() {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fir_interpolate_asym::filter); }

    // FIR
    void filter(input_async_buffer<TT_DATA, extents<inherited_extent> >& inWindow,
                input_cascade_cacc* inCascade,
                output_circular_buffer<TT_DATA>& __restrict outWindow,
                output_circular_buffer<TT_DATA>& __restrict outWindow2);
};

//-----------------------------------------------------------------------------------------------------
// Partially specialized classes for cascaded interface (First kernel in cascade), Windowed. no reload
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_INTERPOLATE_FACTOR,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
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
class fir_interpolate_asym<TT_DATA,
                           TT_COEFF,
                           TP_FIR_LEN,
                           TP_INTERPOLATE_FACTOR,
                           TP_SHIFT,
                           TP_RND,
                           TP_INPUT_WINDOW_VSIZE,
                           CASC_IN_FALSE,
                           CASC_OUT_TRUE,
                           TP_FIR_RANGE_LEN,
                           TP_KERNEL_POSITION,
                           TP_CASC_LEN,
                           USE_COEFF_RELOAD_FALSE,
                           DUAL_IP_SINGLE,
                           TP_NUM_OUTPUTS,
                           USE_WINDOW_API,
                           TP_MODIFY_MARGIN_OFFSET,
                           TP_COEFF_PHASE,
                           TP_COEFF_PHASE_OFFSET,
                           TP_COEFF_PHASES,
                           TP_COEFF_PHASES_LEN,
                           TP_SAT> : public kernelFilterClass<TT_DATA,
                                                              TT_COEFF,
                                                              TP_FIR_LEN,
                                                              TP_INTERPOLATE_FACTOR,
                                                              TP_SHIFT,
                                                              TP_RND,
                                                              TP_INPUT_WINDOW_VSIZE,
                                                              CASC_IN_FALSE,
                                                              CASC_OUT_TRUE,
                                                              TP_FIR_RANGE_LEN,
                                                              TP_KERNEL_POSITION,
                                                              TP_CASC_LEN,
                                                              USE_COEFF_RELOAD_FALSE,
                                                              DUAL_IP_SINGLE,
                                                              TP_NUM_OUTPUTS,
                                                              USE_WINDOW_API,
                                                              TP_MODIFY_MARGIN_OFFSET,
                                                              TP_COEFF_PHASE,
                                                              TP_COEFF_PHASE_OFFSET,
                                                              TP_COEFF_PHASES,
                                                              TP_COEFF_PHASES_LEN,
                                                              TP_SAT> {
   public:
    // Constructor
    fir_interpolate_asym(const TT_COEFF (&taps)[TP_FIR_LEN])
        : kernelFilterClass<TT_DATA,
                            TT_COEFF,
                            TP_FIR_LEN,
                            TP_INTERPOLATE_FACTOR,
                            TP_SHIFT,
                            TP_RND,
                            TP_INPUT_WINDOW_VSIZE,
                            CASC_IN_FALSE,
                            CASC_OUT_TRUE,
                            TP_FIR_RANGE_LEN,
                            TP_KERNEL_POSITION,
                            TP_CASC_LEN,
                            USE_COEFF_RELOAD_FALSE,
                            DUAL_IP_SINGLE,
                            TP_NUM_OUTPUTS,
                            USE_WINDOW_API,
                            TP_MODIFY_MARGIN_OFFSET,
                            TP_COEFF_PHASE,
                            TP_COEFF_PHASE_OFFSET,
                            TP_COEFF_PHASES,
                            TP_COEFF_PHASES_LEN,
                            TP_SAT>(taps) {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fir_interpolate_asym::filter); }

    // FIR
    void filter(input_circular_buffer<TT_DATA,
                                      extents<inherited_extent>,
                                      margin<fnFirMargin<TP_FIR_LEN / TP_INTERPOLATE_FACTOR, TT_DATA>()> >& inWindow,
                output_cascade_cacc* outCascade,
                output_async_buffer<TT_DATA>& broadcastWindow);
};

// Partially specialized classes for cascaded interface (First kernel in cascade), Windowed. with reload
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_INTERPOLATE_FACTOR,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
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
class fir_interpolate_asym<TT_DATA,
                           TT_COEFF,
                           TP_FIR_LEN,
                           TP_INTERPOLATE_FACTOR,
                           TP_SHIFT,
                           TP_RND,
                           TP_INPUT_WINDOW_VSIZE,
                           CASC_IN_FALSE,
                           CASC_OUT_TRUE,
                           TP_FIR_RANGE_LEN,
                           TP_KERNEL_POSITION,
                           TP_CASC_LEN,
                           USE_COEFF_RELOAD_TRUE,
                           DUAL_IP_SINGLE,
                           TP_NUM_OUTPUTS,
                           USE_WINDOW_API,
                           TP_MODIFY_MARGIN_OFFSET,
                           TP_COEFF_PHASE,
                           TP_COEFF_PHASE_OFFSET,
                           TP_COEFF_PHASES,
                           TP_COEFF_PHASES_LEN,
                           TP_SAT> : public kernelFilterClass<TT_DATA,
                                                              TT_COEFF,
                                                              TP_FIR_LEN,
                                                              TP_INTERPOLATE_FACTOR,
                                                              TP_SHIFT,
                                                              TP_RND,
                                                              TP_INPUT_WINDOW_VSIZE,
                                                              CASC_IN_FALSE,
                                                              CASC_OUT_TRUE,
                                                              TP_FIR_RANGE_LEN,
                                                              TP_KERNEL_POSITION,
                                                              TP_CASC_LEN,
                                                              USE_COEFF_RELOAD_TRUE,
                                                              DUAL_IP_SINGLE,
                                                              TP_NUM_OUTPUTS,
                                                              USE_WINDOW_API,
                                                              TP_MODIFY_MARGIN_OFFSET,
                                                              TP_COEFF_PHASE,
                                                              TP_COEFF_PHASE_OFFSET,
                                                              TP_COEFF_PHASES,
                                                              TP_COEFF_PHASES_LEN,
                                                              TP_SAT> {
   public:
    // Constructor
    fir_interpolate_asym()
        : kernelFilterClass<TT_DATA,
                            TT_COEFF,
                            TP_FIR_LEN,
                            TP_INTERPOLATE_FACTOR,
                            TP_SHIFT,
                            TP_RND,
                            TP_INPUT_WINDOW_VSIZE,
                            CASC_IN_FALSE,
                            CASC_OUT_TRUE,
                            TP_FIR_RANGE_LEN,
                            TP_KERNEL_POSITION,
                            TP_CASC_LEN,
                            USE_COEFF_RELOAD_TRUE,
                            DUAL_IP_SINGLE,
                            TP_NUM_OUTPUTS,
                            USE_WINDOW_API,
                            TP_MODIFY_MARGIN_OFFSET,
                            TP_COEFF_PHASE,
                            TP_COEFF_PHASE_OFFSET,
                            TP_COEFF_PHASES,
                            TP_COEFF_PHASES_LEN,
                            TP_SAT>() {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fir_interpolate_asym::filter); }

    // FIR
    void filter(input_circular_buffer<TT_DATA,
                                      extents<inherited_extent>,
                                      margin<fnFirMargin<TP_FIR_LEN / TP_INTERPOLATE_FACTOR, TT_DATA>()> >& inWindow,
                output_cascade_cacc* outCascade,
                output_async_buffer<TT_DATA>& broadcastWindow,
                const TT_COEFF (&inTaps)[TP_COEFF_PHASES_LEN]);
};

//-----------------------------------------------------------------------------------------------------
// Partially specialized classes for cascaded interface (middle kernels in cascade), Windowed. no reload
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_INTERPOLATE_FACTOR,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
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
class fir_interpolate_asym<TT_DATA,
                           TT_COEFF,
                           TP_FIR_LEN,
                           TP_INTERPOLATE_FACTOR,
                           TP_SHIFT,
                           TP_RND,
                           TP_INPUT_WINDOW_VSIZE,
                           CASC_IN_TRUE,
                           CASC_OUT_TRUE,
                           TP_FIR_RANGE_LEN,
                           TP_KERNEL_POSITION,
                           TP_CASC_LEN,
                           USE_COEFF_RELOAD_FALSE,
                           DUAL_IP_SINGLE,
                           TP_NUM_OUTPUTS,
                           USE_WINDOW_API,
                           TP_MODIFY_MARGIN_OFFSET,
                           TP_COEFF_PHASE,
                           TP_COEFF_PHASE_OFFSET,
                           TP_COEFF_PHASES,
                           TP_COEFF_PHASES_LEN,
                           TP_SAT> : public kernelFilterClass<TT_DATA,
                                                              TT_COEFF,
                                                              TP_FIR_LEN,
                                                              TP_INTERPOLATE_FACTOR,
                                                              TP_SHIFT,
                                                              TP_RND,
                                                              TP_INPUT_WINDOW_VSIZE,
                                                              CASC_IN_TRUE,
                                                              CASC_OUT_TRUE,
                                                              TP_FIR_RANGE_LEN,
                                                              TP_KERNEL_POSITION,
                                                              TP_CASC_LEN,
                                                              USE_COEFF_RELOAD_FALSE,
                                                              DUAL_IP_SINGLE,
                                                              TP_NUM_OUTPUTS,
                                                              USE_WINDOW_API,
                                                              TP_MODIFY_MARGIN_OFFSET,
                                                              TP_COEFF_PHASE,
                                                              TP_COEFF_PHASE_OFFSET,
                                                              TP_COEFF_PHASES,
                                                              TP_COEFF_PHASES_LEN,
                                                              TP_SAT> {
   private:
   public:
    // Constructor
    fir_interpolate_asym(const TT_COEFF (&taps)[TP_FIR_LEN])
        : kernelFilterClass<TT_DATA,
                            TT_COEFF,
                            TP_FIR_LEN,
                            TP_INTERPOLATE_FACTOR,
                            TP_SHIFT,
                            TP_RND,
                            TP_INPUT_WINDOW_VSIZE,
                            CASC_IN_TRUE,
                            CASC_OUT_TRUE,
                            TP_FIR_RANGE_LEN,
                            TP_KERNEL_POSITION,
                            TP_CASC_LEN,
                            USE_COEFF_RELOAD_FALSE,
                            DUAL_IP_SINGLE,
                            TP_NUM_OUTPUTS,
                            USE_WINDOW_API,
                            TP_MODIFY_MARGIN_OFFSET,
                            TP_COEFF_PHASE,
                            TP_COEFF_PHASE_OFFSET,
                            TP_COEFF_PHASES,
                            TP_COEFF_PHASES_LEN,
                            TP_SAT>(taps) {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fir_interpolate_asym::filter); }

    // FIR
    void filter(input_async_buffer<TT_DATA>& inWindow,
                input_cascade_cacc* inCascade,
                output_cascade_cacc* outCascade,
                output_async_buffer<TT_DATA>& broadcastWindow);
};

// Partially specialized classes for cascaded interface (middle kernels in cascade), Windowed. with reload
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_INTERPOLATE_FACTOR,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
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
class fir_interpolate_asym<TT_DATA,
                           TT_COEFF,
                           TP_FIR_LEN,
                           TP_INTERPOLATE_FACTOR,
                           TP_SHIFT,
                           TP_RND,
                           TP_INPUT_WINDOW_VSIZE,
                           CASC_IN_TRUE,
                           CASC_OUT_TRUE,
                           TP_FIR_RANGE_LEN,
                           TP_KERNEL_POSITION,
                           TP_CASC_LEN,
                           USE_COEFF_RELOAD_TRUE,
                           DUAL_IP_SINGLE,
                           TP_NUM_OUTPUTS,
                           USE_WINDOW_API,
                           TP_MODIFY_MARGIN_OFFSET,
                           TP_COEFF_PHASE,
                           TP_COEFF_PHASE_OFFSET,
                           TP_COEFF_PHASES,
                           TP_COEFF_PHASES_LEN,
                           TP_SAT> : public kernelFilterClass<TT_DATA,
                                                              TT_COEFF,
                                                              TP_FIR_LEN,
                                                              TP_INTERPOLATE_FACTOR,
                                                              TP_SHIFT,
                                                              TP_RND,
                                                              TP_INPUT_WINDOW_VSIZE,
                                                              CASC_IN_TRUE,
                                                              CASC_OUT_TRUE,
                                                              TP_FIR_RANGE_LEN,
                                                              TP_KERNEL_POSITION,
                                                              TP_CASC_LEN,
                                                              USE_COEFF_RELOAD_TRUE,
                                                              DUAL_IP_SINGLE,
                                                              TP_NUM_OUTPUTS,
                                                              USE_WINDOW_API,
                                                              TP_MODIFY_MARGIN_OFFSET,
                                                              TP_COEFF_PHASE,
                                                              TP_COEFF_PHASE_OFFSET,
                                                              TP_COEFF_PHASES,
                                                              TP_COEFF_PHASES_LEN,
                                                              TP_SAT> {
   public:
    // Constructor
    fir_interpolate_asym()
        : kernelFilterClass<TT_DATA,
                            TT_COEFF,
                            TP_FIR_LEN,
                            TP_INTERPOLATE_FACTOR,
                            TP_SHIFT,
                            TP_RND,
                            TP_INPUT_WINDOW_VSIZE,
                            CASC_IN_TRUE,
                            CASC_OUT_TRUE,
                            TP_FIR_RANGE_LEN,
                            TP_KERNEL_POSITION,
                            TP_CASC_LEN,
                            USE_COEFF_RELOAD_TRUE,
                            DUAL_IP_SINGLE,
                            TP_NUM_OUTPUTS,
                            USE_WINDOW_API,
                            TP_MODIFY_MARGIN_OFFSET,
                            TP_COEFF_PHASE,
                            TP_COEFF_PHASE_OFFSET,
                            TP_COEFF_PHASES,
                            TP_COEFF_PHASES_LEN,
                            TP_SAT>() {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fir_interpolate_asym::filter); }

    // FIR
    void filter(input_async_buffer<TT_DATA>& inWindow,
                input_cascade_cacc* inCascade,
                output_cascade_cacc* outCascade,
                output_async_buffer<TT_DATA>& broadcastWindow);
};

////////////////////////////////////////////////////////////////////////////////////
/////////   STREAM                               ///////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////

// Single kernel specialization. No cascade ports. Streaming. Static coefficients, single output

template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_INTERPOLATE_FACTOR,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_FIR_RANGE_LEN,
          int TP_MODIFY_MARGIN_OFFSET,
          unsigned int TP_COEFF_PHASE,
          unsigned int TP_COEFF_PHASE_OFFSET,
          unsigned int TP_COEFF_PHASES,
          unsigned int TP_COEFF_PHASES_LEN,
          unsigned int TP_SAT>
class fir_interpolate_asym<TT_DATA,
                           TT_COEFF,
                           TP_FIR_LEN,
                           TP_INTERPOLATE_FACTOR,
                           TP_SHIFT,
                           TP_RND,
                           TP_INPUT_WINDOW_VSIZE,
                           CASC_IN_FALSE,
                           CASC_OUT_FALSE,
                           TP_FIR_RANGE_LEN,
                           0,
                           1,
                           USE_COEFF_RELOAD_FALSE,
                           DUAL_IP_SINGLE,
                           1,
                           USE_STREAM_API,
                           TP_MODIFY_MARGIN_OFFSET,
                           TP_COEFF_PHASE,
                           TP_COEFF_PHASE_OFFSET,
                           TP_COEFF_PHASES,
                           TP_COEFF_PHASES_LEN,
                           TP_SAT> : public kernelFilterClass<TT_DATA,
                                                              TT_COEFF,
                                                              TP_FIR_LEN,
                                                              TP_INTERPOLATE_FACTOR,
                                                              TP_SHIFT,
                                                              TP_RND,
                                                              TP_INPUT_WINDOW_VSIZE,
                                                              CASC_IN_FALSE,
                                                              CASC_OUT_FALSE,
                                                              TP_FIR_RANGE_LEN,
                                                              0,
                                                              1,
                                                              USE_COEFF_RELOAD_FALSE,
                                                              DUAL_IP_SINGLE,
                                                              1,
                                                              USE_STREAM_API,
                                                              TP_MODIFY_MARGIN_OFFSET,
                                                              TP_COEFF_PHASE,
                                                              TP_COEFF_PHASE_OFFSET,
                                                              TP_COEFF_PHASES,
                                                              TP_COEFF_PHASES_LEN,
                                                              TP_SAT> {
   private:
   public:
    // Constructor
    fir_interpolate_asym(const TT_COEFF (&taps)[TP_FIR_LEN])
        : kernelFilterClass<TT_DATA,
                            TT_COEFF,
                            TP_FIR_LEN,
                            TP_INTERPOLATE_FACTOR,
                            TP_SHIFT,
                            TP_RND,
                            TP_INPUT_WINDOW_VSIZE,
                            CASC_IN_FALSE,
                            CASC_OUT_FALSE,
                            TP_FIR_RANGE_LEN,
                            0,
                            1,
                            USE_COEFF_RELOAD_FALSE,
                            DUAL_IP_SINGLE,
                            1,
                            USE_STREAM_API,
                            TP_MODIFY_MARGIN_OFFSET,
                            TP_COEFF_PHASE,
                            TP_COEFF_PHASE_OFFSET,
                            TP_COEFF_PHASES,
                            TP_COEFF_PHASES_LEN,
                            TP_SAT>(taps) {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fir_interpolate_asym::filter); }

    // FIR
    void filter(input_stream<TT_DATA>* inStream, output_stream<TT_DATA>* outStream);
};

// Single kernel specialization. No cascade ports. Static coefficients, dual output

template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_INTERPOLATE_FACTOR,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_FIR_RANGE_LEN,
          int TP_MODIFY_MARGIN_OFFSET,
          unsigned int TP_COEFF_PHASE,
          unsigned int TP_COEFF_PHASE_OFFSET,
          unsigned int TP_COEFF_PHASES,
          unsigned int TP_COEFF_PHASES_LEN,
          unsigned int TP_SAT>
class fir_interpolate_asym<TT_DATA,
                           TT_COEFF,
                           TP_FIR_LEN,
                           TP_INTERPOLATE_FACTOR,
                           TP_SHIFT,
                           TP_RND,
                           TP_INPUT_WINDOW_VSIZE,
                           CASC_IN_FALSE,
                           CASC_IN_FALSE,
                           TP_FIR_RANGE_LEN,
                           0,
                           1,
                           USE_COEFF_RELOAD_FALSE,
                           DUAL_IP_SINGLE,
                           2,
                           USE_STREAM_API,
                           TP_MODIFY_MARGIN_OFFSET,
                           TP_COEFF_PHASE,
                           TP_COEFF_PHASE_OFFSET,
                           TP_COEFF_PHASES,
                           TP_COEFF_PHASES_LEN,
                           TP_SAT> : public kernelFilterClass<TT_DATA,
                                                              TT_COEFF,
                                                              TP_FIR_LEN,
                                                              TP_INTERPOLATE_FACTOR,
                                                              TP_SHIFT,
                                                              TP_RND,
                                                              TP_INPUT_WINDOW_VSIZE,
                                                              CASC_IN_FALSE,
                                                              CASC_IN_FALSE,
                                                              TP_FIR_RANGE_LEN,
                                                              0,
                                                              1,
                                                              USE_COEFF_RELOAD_FALSE,
                                                              DUAL_IP_SINGLE,
                                                              2,
                                                              USE_STREAM_API,
                                                              TP_MODIFY_MARGIN_OFFSET,
                                                              TP_COEFF_PHASE,
                                                              TP_COEFF_PHASE_OFFSET,
                                                              TP_COEFF_PHASES,
                                                              TP_COEFF_PHASES_LEN,
                                                              TP_SAT> {
   private:
   public:
    // Constructor
    fir_interpolate_asym(const TT_COEFF (&taps)[TP_FIR_LEN])
        : kernelFilterClass<TT_DATA,
                            TT_COEFF,
                            TP_FIR_LEN,
                            TP_INTERPOLATE_FACTOR,
                            TP_SHIFT,
                            TP_RND,
                            TP_INPUT_WINDOW_VSIZE,
                            CASC_IN_FALSE,
                            CASC_IN_FALSE,
                            TP_FIR_RANGE_LEN,
                            0,
                            1,
                            USE_COEFF_RELOAD_FALSE,
                            DUAL_IP_SINGLE,
                            2,
                            USE_STREAM_API,
                            TP_MODIFY_MARGIN_OFFSET,
                            TP_COEFF_PHASE,
                            TP_COEFF_PHASE_OFFSET,
                            TP_COEFF_PHASES,
                            TP_COEFF_PHASES_LEN,
                            TP_SAT>(taps) {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fir_interpolate_asym::filter); }

    // FIR
    void filter(input_stream<TT_DATA>* inStream, output_stream<TT_DATA>* outStream, output_stream<TT_DATA>* outStream2);
};

// Single kernel specialization. No cascade ports. Reloadable coefficients, single output
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_INTERPOLATE_FACTOR,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_FIR_RANGE_LEN,
          int TP_MODIFY_MARGIN_OFFSET,
          unsigned int TP_COEFF_PHASE,
          unsigned int TP_COEFF_PHASE_OFFSET,
          unsigned int TP_COEFF_PHASES,
          unsigned int TP_COEFF_PHASES_LEN,
          unsigned int TP_SAT>
class fir_interpolate_asym<TT_DATA,
                           TT_COEFF,
                           TP_FIR_LEN,
                           TP_INTERPOLATE_FACTOR,
                           TP_SHIFT,
                           TP_RND,
                           TP_INPUT_WINDOW_VSIZE,
                           CASC_IN_FALSE,
                           CASC_IN_FALSE,
                           TP_FIR_RANGE_LEN,
                           0,
                           1,
                           USE_COEFF_RELOAD_TRUE,
                           DUAL_IP_SINGLE,
                           1,
                           USE_STREAM_API,
                           TP_MODIFY_MARGIN_OFFSET,
                           TP_COEFF_PHASE,
                           TP_COEFF_PHASE_OFFSET,
                           TP_COEFF_PHASES,
                           TP_COEFF_PHASES_LEN,
                           TP_SAT> : public kernelFilterClass<TT_DATA,
                                                              TT_COEFF,
                                                              TP_FIR_LEN,
                                                              TP_INTERPOLATE_FACTOR,
                                                              TP_SHIFT,
                                                              TP_RND,
                                                              TP_INPUT_WINDOW_VSIZE,
                                                              CASC_IN_FALSE,
                                                              CASC_IN_FALSE,
                                                              TP_FIR_RANGE_LEN,
                                                              0,
                                                              1,
                                                              USE_COEFF_RELOAD_TRUE,
                                                              DUAL_IP_SINGLE,
                                                              1,
                                                              USE_STREAM_API,
                                                              TP_MODIFY_MARGIN_OFFSET,
                                                              TP_COEFF_PHASE,
                                                              TP_COEFF_PHASE_OFFSET,
                                                              TP_COEFF_PHASES,
                                                              TP_COEFF_PHASES_LEN,
                                                              TP_SAT> {
   private:
   public:
    // Constructor
    fir_interpolate_asym()
        : kernelFilterClass<TT_DATA,
                            TT_COEFF,
                            TP_FIR_LEN,
                            TP_INTERPOLATE_FACTOR,
                            TP_SHIFT,
                            TP_RND,
                            TP_INPUT_WINDOW_VSIZE,
                            CASC_IN_FALSE,
                            CASC_IN_FALSE,
                            TP_FIR_RANGE_LEN,
                            0,
                            1,
                            USE_COEFF_RELOAD_TRUE,
                            DUAL_IP_SINGLE,
                            1,
                            USE_STREAM_API,
                            TP_MODIFY_MARGIN_OFFSET,
                            TP_COEFF_PHASE,
                            TP_COEFF_PHASE_OFFSET,
                            TP_COEFF_PHASES,
                            TP_COEFF_PHASES_LEN,
                            TP_SAT>() {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fir_interpolate_asym::filter); }

    // FIR
    void filter(input_stream<TT_DATA>* inStream,
                output_stream<TT_DATA>* outStream,
                const TT_COEFF (&inTaps)[TP_COEFF_PHASES_LEN]);
};

// Single kernel specialization. No cascade ports. Reloadable coefficients, dual output

template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_INTERPOLATE_FACTOR,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_FIR_RANGE_LEN,
          int TP_MODIFY_MARGIN_OFFSET,
          unsigned int TP_COEFF_PHASE,
          unsigned int TP_COEFF_PHASE_OFFSET,
          unsigned int TP_COEFF_PHASES,
          unsigned int TP_COEFF_PHASES_LEN,
          unsigned int TP_SAT>
class fir_interpolate_asym<TT_DATA,
                           TT_COEFF,
                           TP_FIR_LEN,
                           TP_INTERPOLATE_FACTOR,
                           TP_SHIFT,
                           TP_RND,
                           TP_INPUT_WINDOW_VSIZE,
                           CASC_IN_FALSE,
                           CASC_OUT_FALSE,
                           TP_FIR_RANGE_LEN,
                           0,
                           1,
                           USE_COEFF_RELOAD_TRUE,
                           DUAL_IP_SINGLE,
                           2,
                           USE_STREAM_API,
                           TP_MODIFY_MARGIN_OFFSET,
                           TP_COEFF_PHASE,
                           TP_COEFF_PHASE_OFFSET,
                           TP_COEFF_PHASES,
                           TP_COEFF_PHASES_LEN,
                           TP_SAT> : public kernelFilterClass<TT_DATA,
                                                              TT_COEFF,
                                                              TP_FIR_LEN,
                                                              TP_INTERPOLATE_FACTOR,
                                                              TP_SHIFT,
                                                              TP_RND,
                                                              TP_INPUT_WINDOW_VSIZE,
                                                              CASC_IN_FALSE,
                                                              CASC_OUT_FALSE,
                                                              TP_FIR_RANGE_LEN,
                                                              0,
                                                              1,
                                                              USE_COEFF_RELOAD_TRUE,
                                                              DUAL_IP_SINGLE,
                                                              2,
                                                              USE_STREAM_API,
                                                              TP_MODIFY_MARGIN_OFFSET,
                                                              TP_COEFF_PHASE,
                                                              TP_COEFF_PHASE_OFFSET,
                                                              TP_COEFF_PHASES,
                                                              TP_COEFF_PHASES_LEN,
                                                              TP_SAT> {
   private:
   public:
    // Constructor
    fir_interpolate_asym()
        : kernelFilterClass<TT_DATA,
                            TT_COEFF,
                            TP_FIR_LEN,
                            TP_INTERPOLATE_FACTOR,
                            TP_SHIFT,
                            TP_RND,
                            TP_INPUT_WINDOW_VSIZE,
                            CASC_IN_FALSE,
                            CASC_OUT_FALSE,
                            TP_FIR_RANGE_LEN,
                            0,
                            1,
                            USE_COEFF_RELOAD_TRUE,
                            DUAL_IP_SINGLE,
                            2,
                            USE_STREAM_API,
                            TP_MODIFY_MARGIN_OFFSET,
                            TP_COEFF_PHASE,
                            TP_COEFF_PHASE_OFFSET,
                            TP_COEFF_PHASES,
                            TP_COEFF_PHASES_LEN,
                            TP_SAT>() {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fir_interpolate_asym::filter); }

    // FIR
    void filter(input_stream<TT_DATA>* inStream,
                output_stream<TT_DATA>* outStream,
                output_stream<TT_DATA>* outStream2,
                const TT_COEFF (&inTaps)[TP_COEFF_PHASES_LEN]);
};

// cascaded kernels - final kernel specialisation. single output. static coefficients
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_INTERPOLATE_FACTOR,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_FIR_RANGE_LEN,
          unsigned int TP_KERNEL_POSITION,
          unsigned int TP_CASC_LEN,
          int TP_MODIFY_MARGIN_OFFSET,
          unsigned int TP_COEFF_PHASE,
          unsigned int TP_COEFF_PHASE_OFFSET,
          unsigned int TP_COEFF_PHASES,
          unsigned int TP_COEFF_PHASES_LEN,
          unsigned int TP_SAT>
class fir_interpolate_asym<TT_DATA,
                           TT_COEFF,
                           TP_FIR_LEN,
                           TP_INTERPOLATE_FACTOR,
                           TP_SHIFT,
                           TP_RND,
                           TP_INPUT_WINDOW_VSIZE,
                           CASC_IN_TRUE,
                           CASC_OUT_FALSE,
                           TP_FIR_RANGE_LEN,
                           TP_KERNEL_POSITION,
                           TP_CASC_LEN,
                           USE_COEFF_RELOAD_FALSE,
                           DUAL_IP_SINGLE,
                           1,
                           USE_STREAM_API,
                           TP_MODIFY_MARGIN_OFFSET,
                           TP_COEFF_PHASE,
                           TP_COEFF_PHASE_OFFSET,
                           TP_COEFF_PHASES,
                           TP_COEFF_PHASES_LEN,
                           TP_SAT> : public kernelFilterClass<TT_DATA,
                                                              TT_COEFF,
                                                              TP_FIR_LEN,
                                                              TP_INTERPOLATE_FACTOR,
                                                              TP_SHIFT,
                                                              TP_RND,
                                                              TP_INPUT_WINDOW_VSIZE,
                                                              CASC_IN_TRUE,
                                                              CASC_OUT_FALSE,
                                                              TP_FIR_RANGE_LEN,
                                                              TP_KERNEL_POSITION,
                                                              TP_CASC_LEN,
                                                              USE_COEFF_RELOAD_FALSE,
                                                              DUAL_IP_SINGLE,
                                                              1,
                                                              USE_STREAM_API,
                                                              TP_MODIFY_MARGIN_OFFSET,
                                                              TP_COEFF_PHASE,
                                                              TP_COEFF_PHASE_OFFSET,
                                                              TP_COEFF_PHASES,
                                                              TP_COEFF_PHASES_LEN,
                                                              TP_SAT> {
   private:
   public:
    // Constructor
    fir_interpolate_asym(const TT_COEFF (&taps)[TP_FIR_LEN])
        : kernelFilterClass<TT_DATA,
                            TT_COEFF,
                            TP_FIR_LEN,
                            TP_INTERPOLATE_FACTOR,
                            TP_SHIFT,
                            TP_RND,
                            TP_INPUT_WINDOW_VSIZE,
                            CASC_IN_TRUE,
                            CASC_OUT_FALSE,
                            TP_FIR_RANGE_LEN,
                            TP_KERNEL_POSITION,
                            TP_CASC_LEN,
                            USE_COEFF_RELOAD_FALSE,
                            DUAL_IP_SINGLE,
                            1,
                            USE_STREAM_API,
                            TP_MODIFY_MARGIN_OFFSET,
                            TP_COEFF_PHASE,
                            TP_COEFF_PHASE_OFFSET,
                            TP_COEFF_PHASES,
                            TP_COEFF_PHASES_LEN,
                            TP_SAT>(taps) {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fir_interpolate_asym::filter); }

    // FIR
    void filter(input_stream<TT_DATA>* inStream, input_cascade_cacc* inCascade, output_stream<TT_DATA>* outStream);
};

//-----------------------------------------------------------------------------------------------------
// Partially specialized classes for cascaded interface - final kernel. Static coefficients dual output
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_INTERPOLATE_FACTOR,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_FIR_RANGE_LEN,
          unsigned int TP_KERNEL_POSITION,
          unsigned int TP_CASC_LEN,
          int TP_MODIFY_MARGIN_OFFSET,
          unsigned int TP_COEFF_PHASE,
          unsigned int TP_COEFF_PHASE_OFFSET,
          unsigned int TP_COEFF_PHASES,
          unsigned int TP_COEFF_PHASES_LEN,
          unsigned int TP_SAT>
class fir_interpolate_asym<TT_DATA,
                           TT_COEFF,
                           TP_FIR_LEN,
                           TP_INTERPOLATE_FACTOR,
                           TP_SHIFT,
                           TP_RND,
                           TP_INPUT_WINDOW_VSIZE,
                           CASC_IN_TRUE,
                           CASC_OUT_FALSE,
                           TP_FIR_RANGE_LEN,
                           TP_KERNEL_POSITION,
                           TP_CASC_LEN,
                           USE_COEFF_RELOAD_FALSE,
                           DUAL_IP_SINGLE,
                           2,
                           USE_STREAM_API,
                           TP_MODIFY_MARGIN_OFFSET,
                           TP_COEFF_PHASE,
                           TP_COEFF_PHASE_OFFSET,
                           TP_COEFF_PHASES,
                           TP_COEFF_PHASES_LEN,
                           TP_SAT> : public kernelFilterClass<TT_DATA,
                                                              TT_COEFF,
                                                              TP_FIR_LEN,
                                                              TP_INTERPOLATE_FACTOR,
                                                              TP_SHIFT,
                                                              TP_RND,
                                                              TP_INPUT_WINDOW_VSIZE,
                                                              CASC_IN_TRUE,
                                                              CASC_OUT_FALSE,
                                                              TP_FIR_RANGE_LEN,
                                                              TP_KERNEL_POSITION,
                                                              TP_CASC_LEN,
                                                              USE_COEFF_RELOAD_FALSE,
                                                              DUAL_IP_SINGLE,
                                                              2,
                                                              USE_STREAM_API,
                                                              TP_MODIFY_MARGIN_OFFSET,
                                                              TP_COEFF_PHASE,
                                                              TP_COEFF_PHASE_OFFSET,
                                                              TP_COEFF_PHASES,
                                                              TP_COEFF_PHASES_LEN,
                                                              TP_SAT> {
   private:
   public:
    // Constructor
    fir_interpolate_asym(const TT_COEFF (&taps)[TP_FIR_LEN])
        : kernelFilterClass<TT_DATA,
                            TT_COEFF,
                            TP_FIR_LEN,
                            TP_INTERPOLATE_FACTOR,
                            TP_SHIFT,
                            TP_RND,
                            TP_INPUT_WINDOW_VSIZE,
                            CASC_IN_TRUE,
                            CASC_OUT_FALSE,
                            TP_FIR_RANGE_LEN,
                            TP_KERNEL_POSITION,
                            TP_CASC_LEN,
                            USE_COEFF_RELOAD_FALSE,
                            DUAL_IP_SINGLE,
                            2,
                            USE_STREAM_API,
                            TP_MODIFY_MARGIN_OFFSET,
                            TP_COEFF_PHASE,
                            TP_COEFF_PHASE_OFFSET,
                            TP_COEFF_PHASES,
                            TP_COEFF_PHASES_LEN,
                            TP_SAT>(taps) {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fir_interpolate_asym::filter); }

    // FIR
    void filter(input_stream<TT_DATA>* inStream,
                input_cascade_cacc* inCascade,
                output_stream<TT_DATA>* outStream,
                output_stream<TT_DATA>* outStream2);
};

//-----------------------------------------------------------------------------------------------------
// Partially specialized classes for cascaded interface - first kernel. Static coefficients single output
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_INTERPOLATE_FACTOR,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
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
class fir_interpolate_asym<TT_DATA,
                           TT_COEFF,
                           TP_FIR_LEN,
                           TP_INTERPOLATE_FACTOR,
                           TP_SHIFT,
                           TP_RND,
                           TP_INPUT_WINDOW_VSIZE,
                           CASC_IN_FALSE,
                           CASC_OUT_TRUE,
                           TP_FIR_RANGE_LEN,
                           TP_KERNEL_POSITION,
                           TP_CASC_LEN,
                           USE_COEFF_RELOAD_FALSE,
                           DUAL_IP_SINGLE,
                           TP_NUM_OUTPUTS,
                           USE_STREAM_API,
                           TP_MODIFY_MARGIN_OFFSET,
                           TP_COEFF_PHASE,
                           TP_COEFF_PHASE_OFFSET,
                           TP_COEFF_PHASES,
                           TP_COEFF_PHASES_LEN,
                           TP_SAT> : public kernelFilterClass<TT_DATA,
                                                              TT_COEFF,
                                                              TP_FIR_LEN,
                                                              TP_INTERPOLATE_FACTOR,
                                                              TP_SHIFT,
                                                              TP_RND,
                                                              TP_INPUT_WINDOW_VSIZE,
                                                              CASC_IN_FALSE,
                                                              CASC_OUT_TRUE,
                                                              TP_FIR_RANGE_LEN,
                                                              TP_KERNEL_POSITION,
                                                              TP_CASC_LEN,
                                                              USE_COEFF_RELOAD_FALSE,
                                                              DUAL_IP_SINGLE,
                                                              TP_NUM_OUTPUTS,
                                                              USE_STREAM_API,
                                                              TP_MODIFY_MARGIN_OFFSET,
                                                              TP_COEFF_PHASE,
                                                              TP_COEFF_PHASE_OFFSET,
                                                              TP_COEFF_PHASES,
                                                              TP_COEFF_PHASES_LEN,
                                                              TP_SAT> {
   private:
   public:
    // Constructor
    fir_interpolate_asym(const TT_COEFF (&taps)[TP_FIR_LEN])
        : kernelFilterClass<TT_DATA,
                            TT_COEFF,
                            TP_FIR_LEN,
                            TP_INTERPOLATE_FACTOR,
                            TP_SHIFT,
                            TP_RND,
                            TP_INPUT_WINDOW_VSIZE,
                            CASC_IN_FALSE,
                            CASC_OUT_TRUE,
                            TP_FIR_RANGE_LEN,
                            TP_KERNEL_POSITION,
                            TP_CASC_LEN,
                            USE_COEFF_RELOAD_FALSE,
                            DUAL_IP_SINGLE,
                            TP_NUM_OUTPUTS,
                            USE_STREAM_API,
                            TP_MODIFY_MARGIN_OFFSET,
                            TP_COEFF_PHASE,
                            TP_COEFF_PHASE_OFFSET,
                            TP_COEFF_PHASES,
                            TP_COEFF_PHASES_LEN,
                            TP_SAT>(taps) {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fir_interpolate_asym::filter); }

    // FIR
    void filter(input_stream<TT_DATA>* inStream, output_cascade_cacc* outCascade);
};

//-----------------------------------------------------------------------------------------------------
// Partially specialized classes for cascaded interface - middle kernel. Static coefficients
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_INTERPOLATE_FACTOR,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
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
class fir_interpolate_asym<TT_DATA,
                           TT_COEFF,
                           TP_FIR_LEN,
                           TP_INTERPOLATE_FACTOR,
                           TP_SHIFT,
                           TP_RND,
                           TP_INPUT_WINDOW_VSIZE,
                           CASC_IN_TRUE,
                           CASC_OUT_TRUE,
                           TP_FIR_RANGE_LEN,
                           TP_KERNEL_POSITION,
                           TP_CASC_LEN,
                           USE_COEFF_RELOAD_FALSE,
                           DUAL_IP_SINGLE,
                           TP_NUM_OUTPUTS,
                           USE_STREAM_API,
                           TP_MODIFY_MARGIN_OFFSET,
                           TP_COEFF_PHASE,
                           TP_COEFF_PHASE_OFFSET,
                           TP_COEFF_PHASES,
                           TP_COEFF_PHASES_LEN,
                           TP_SAT> : public kernelFilterClass<TT_DATA,
                                                              TT_COEFF,
                                                              TP_FIR_LEN,
                                                              TP_INTERPOLATE_FACTOR,
                                                              TP_SHIFT,
                                                              TP_RND,
                                                              TP_INPUT_WINDOW_VSIZE,
                                                              CASC_IN_TRUE,
                                                              CASC_OUT_TRUE,
                                                              TP_FIR_RANGE_LEN,
                                                              TP_KERNEL_POSITION,
                                                              TP_CASC_LEN,
                                                              USE_COEFF_RELOAD_FALSE,
                                                              DUAL_IP_SINGLE,
                                                              TP_NUM_OUTPUTS,
                                                              USE_STREAM_API,
                                                              TP_MODIFY_MARGIN_OFFSET,
                                                              TP_COEFF_PHASE,
                                                              TP_COEFF_PHASE_OFFSET,
                                                              TP_COEFF_PHASES,
                                                              TP_COEFF_PHASES_LEN,
                                                              TP_SAT> {
   private:
   public:
    // Constructor
    fir_interpolate_asym(const TT_COEFF (&taps)[TP_FIR_LEN])
        : kernelFilterClass<TT_DATA,
                            TT_COEFF,
                            TP_FIR_LEN,
                            TP_INTERPOLATE_FACTOR,
                            TP_SHIFT,
                            TP_RND,
                            TP_INPUT_WINDOW_VSIZE,
                            CASC_IN_TRUE,
                            CASC_OUT_TRUE,
                            TP_FIR_RANGE_LEN,
                            TP_KERNEL_POSITION,
                            TP_CASC_LEN,
                            USE_COEFF_RELOAD_FALSE,
                            DUAL_IP_SINGLE,
                            TP_NUM_OUTPUTS,
                            USE_STREAM_API,
                            TP_MODIFY_MARGIN_OFFSET,
                            TP_COEFF_PHASE,
                            TP_COEFF_PHASE_OFFSET,
                            TP_COEFF_PHASES,
                            TP_COEFF_PHASES_LEN,
                            TP_SAT>(taps) {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fir_interpolate_asym::filter); }

    // FIR
    void filter(input_stream<TT_DATA>* inStream, input_cascade_cacc* inCascade, output_cascade_cacc* outCascade);
};

//-----------------------------------------------------------------------------------------------------
// Partially specialized classes for cascaded interface - final kernel. Reloadable coefficients single output
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_INTERPOLATE_FACTOR,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_FIR_RANGE_LEN,
          unsigned int TP_KERNEL_POSITION,
          unsigned int TP_CASC_LEN,
          int TP_MODIFY_MARGIN_OFFSET,
          unsigned int TP_COEFF_PHASE,
          unsigned int TP_COEFF_PHASE_OFFSET,
          unsigned int TP_COEFF_PHASES,
          unsigned int TP_COEFF_PHASES_LEN,
          unsigned int TP_SAT>
class fir_interpolate_asym<TT_DATA,
                           TT_COEFF,
                           TP_FIR_LEN,
                           TP_INTERPOLATE_FACTOR,
                           TP_SHIFT,
                           TP_RND,
                           TP_INPUT_WINDOW_VSIZE,
                           CASC_IN_TRUE,
                           CASC_OUT_FALSE,
                           TP_FIR_RANGE_LEN,
                           TP_KERNEL_POSITION,
                           TP_CASC_LEN,
                           USE_COEFF_RELOAD_TRUE,
                           DUAL_IP_SINGLE,
                           1,
                           USE_STREAM_API,
                           TP_MODIFY_MARGIN_OFFSET,
                           TP_COEFF_PHASE,
                           TP_COEFF_PHASE_OFFSET,
                           TP_COEFF_PHASES,
                           TP_COEFF_PHASES_LEN,
                           TP_SAT> : public kernelFilterClass<TT_DATA,
                                                              TT_COEFF,
                                                              TP_FIR_LEN,
                                                              TP_INTERPOLATE_FACTOR,
                                                              TP_SHIFT,
                                                              TP_RND,
                                                              TP_INPUT_WINDOW_VSIZE,
                                                              CASC_IN_TRUE,
                                                              CASC_OUT_FALSE,
                                                              TP_FIR_RANGE_LEN,
                                                              TP_KERNEL_POSITION,
                                                              TP_CASC_LEN,
                                                              USE_COEFF_RELOAD_TRUE,
                                                              DUAL_IP_SINGLE,
                                                              1,
                                                              USE_STREAM_API,
                                                              TP_MODIFY_MARGIN_OFFSET,
                                                              TP_COEFF_PHASE,
                                                              TP_COEFF_PHASE_OFFSET,
                                                              TP_COEFF_PHASES,
                                                              TP_COEFF_PHASES_LEN,
                                                              TP_SAT> {
   private:
   public:
    // Constructor
    fir_interpolate_asym()
        : kernelFilterClass<TT_DATA,
                            TT_COEFF,
                            TP_FIR_LEN,
                            TP_INTERPOLATE_FACTOR,
                            TP_SHIFT,
                            TP_RND,
                            TP_INPUT_WINDOW_VSIZE,
                            CASC_IN_TRUE,
                            CASC_OUT_FALSE,
                            TP_FIR_RANGE_LEN,
                            TP_KERNEL_POSITION,
                            TP_CASC_LEN,
                            USE_COEFF_RELOAD_TRUE,
                            DUAL_IP_SINGLE,
                            1,
                            USE_STREAM_API,
                            TP_MODIFY_MARGIN_OFFSET,
                            TP_COEFF_PHASE,
                            TP_COEFF_PHASE_OFFSET,
                            TP_COEFF_PHASES,
                            TP_COEFF_PHASES_LEN,
                            TP_SAT>() {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fir_interpolate_asym::filter); }

    // FIR
    void filter(input_stream<TT_DATA>* inStream, input_cascade_cacc* inCascade, output_stream<TT_DATA>* outStream);
};

//-----------------------------------------------------------------------------------------------------
// Partially specialized classes for cascaded interface - final kernel. Reloadable coefficients. dual output
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_INTERPOLATE_FACTOR,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_FIR_RANGE_LEN,
          unsigned int TP_KERNEL_POSITION,
          unsigned int TP_CASC_LEN,
          int TP_MODIFY_MARGIN_OFFSET,
          unsigned int TP_COEFF_PHASE,
          unsigned int TP_COEFF_PHASE_OFFSET,
          unsigned int TP_COEFF_PHASES,
          unsigned int TP_COEFF_PHASES_LEN,
          unsigned int TP_SAT>
class fir_interpolate_asym<TT_DATA,
                           TT_COEFF,
                           TP_FIR_LEN,
                           TP_INTERPOLATE_FACTOR,
                           TP_SHIFT,
                           TP_RND,
                           TP_INPUT_WINDOW_VSIZE,
                           CASC_IN_TRUE,
                           CASC_OUT_FALSE,
                           TP_FIR_RANGE_LEN,
                           TP_KERNEL_POSITION,
                           TP_CASC_LEN,
                           USE_COEFF_RELOAD_TRUE,
                           DUAL_IP_SINGLE,
                           2,
                           USE_STREAM_API,
                           TP_MODIFY_MARGIN_OFFSET,
                           TP_COEFF_PHASE,
                           TP_COEFF_PHASE_OFFSET,
                           TP_COEFF_PHASES,
                           TP_COEFF_PHASES_LEN,
                           TP_SAT> : public kernelFilterClass<TT_DATA,
                                                              TT_COEFF,
                                                              TP_FIR_LEN,
                                                              TP_INTERPOLATE_FACTOR,
                                                              TP_SHIFT,
                                                              TP_RND,
                                                              TP_INPUT_WINDOW_VSIZE,
                                                              CASC_IN_TRUE,
                                                              CASC_OUT_FALSE,
                                                              TP_FIR_RANGE_LEN,
                                                              TP_KERNEL_POSITION,
                                                              TP_CASC_LEN,
                                                              USE_COEFF_RELOAD_TRUE,
                                                              DUAL_IP_SINGLE,
                                                              2,
                                                              USE_STREAM_API,
                                                              TP_MODIFY_MARGIN_OFFSET,
                                                              TP_COEFF_PHASE,
                                                              TP_COEFF_PHASE_OFFSET,
                                                              TP_COEFF_PHASES,
                                                              TP_COEFF_PHASES_LEN,
                                                              TP_SAT> {
   private:
   public:
    // Constructor
    fir_interpolate_asym()
        : kernelFilterClass<TT_DATA,
                            TT_COEFF,
                            TP_FIR_LEN,
                            TP_INTERPOLATE_FACTOR,
                            TP_SHIFT,
                            TP_RND,
                            TP_INPUT_WINDOW_VSIZE,
                            CASC_IN_TRUE,
                            CASC_OUT_FALSE,
                            TP_FIR_RANGE_LEN,
                            TP_KERNEL_POSITION,
                            TP_CASC_LEN,
                            USE_COEFF_RELOAD_TRUE,
                            DUAL_IP_SINGLE,
                            2,
                            USE_STREAM_API,
                            TP_MODIFY_MARGIN_OFFSET,
                            TP_COEFF_PHASE,
                            TP_COEFF_PHASE_OFFSET,
                            TP_COEFF_PHASES,
                            TP_COEFF_PHASES_LEN,
                            TP_SAT>() {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fir_interpolate_asym::filter); }

    // FIR
    void filter(input_stream<TT_DATA>* inStream,
                input_cascade_cacc* inCascade,
                output_stream<TT_DATA>* outStream,
                output_stream<TT_DATA>* outStream2);
};

//-----------------------------------------------------------------------------------------------------
// Partially specialized classes for cascaded interface - first kernel. Reloadable coefficients. single output
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_INTERPOLATE_FACTOR,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
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
class fir_interpolate_asym<TT_DATA,
                           TT_COEFF,
                           TP_FIR_LEN,
                           TP_INTERPOLATE_FACTOR,
                           TP_SHIFT,
                           TP_RND,
                           TP_INPUT_WINDOW_VSIZE,
                           CASC_IN_FALSE,
                           CASC_OUT_TRUE,
                           TP_FIR_RANGE_LEN,
                           TP_KERNEL_POSITION,
                           TP_CASC_LEN,
                           USE_COEFF_RELOAD_TRUE,
                           DUAL_IP_SINGLE,
                           TP_NUM_OUTPUTS,
                           USE_STREAM_API,
                           TP_MODIFY_MARGIN_OFFSET,
                           TP_COEFF_PHASE,
                           TP_COEFF_PHASE_OFFSET,
                           TP_COEFF_PHASES,
                           TP_COEFF_PHASES_LEN,
                           TP_SAT> : public kernelFilterClass<TT_DATA,
                                                              TT_COEFF,
                                                              TP_FIR_LEN,
                                                              TP_INTERPOLATE_FACTOR,
                                                              TP_SHIFT,
                                                              TP_RND,
                                                              TP_INPUT_WINDOW_VSIZE,
                                                              CASC_IN_FALSE,
                                                              CASC_OUT_TRUE,
                                                              TP_FIR_RANGE_LEN,
                                                              TP_KERNEL_POSITION,
                                                              TP_CASC_LEN,
                                                              USE_COEFF_RELOAD_TRUE,
                                                              DUAL_IP_SINGLE,
                                                              TP_NUM_OUTPUTS,
                                                              USE_STREAM_API,
                                                              TP_MODIFY_MARGIN_OFFSET,
                                                              TP_COEFF_PHASE,
                                                              TP_COEFF_PHASE_OFFSET,
                                                              TP_COEFF_PHASES,
                                                              TP_COEFF_PHASES_LEN,
                                                              TP_SAT> {
   private:
   public:
    // Constructor
    fir_interpolate_asym()
        : kernelFilterClass<TT_DATA,
                            TT_COEFF,
                            TP_FIR_LEN,
                            TP_INTERPOLATE_FACTOR,
                            TP_SHIFT,
                            TP_RND,
                            TP_INPUT_WINDOW_VSIZE,
                            CASC_IN_FALSE,
                            CASC_OUT_TRUE,
                            TP_FIR_RANGE_LEN,
                            TP_KERNEL_POSITION,
                            TP_CASC_LEN,
                            USE_COEFF_RELOAD_TRUE,
                            DUAL_IP_SINGLE,
                            TP_NUM_OUTPUTS,
                            USE_STREAM_API,
                            TP_MODIFY_MARGIN_OFFSET,
                            TP_COEFF_PHASE,
                            TP_COEFF_PHASE_OFFSET,
                            TP_COEFF_PHASES,
                            TP_COEFF_PHASES_LEN,
                            TP_SAT>() {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fir_interpolate_asym::filter); }

    // FIR
    void filter(input_stream<TT_DATA>* inStream,
                output_cascade_cacc* inCascade,
                const TT_COEFF (&inTaps)[TP_COEFF_PHASES_LEN]);
};

//-----------------------------------------------------------------------------------------------------
// Partially specialized classes for cascaded interface - middle kernel. Reloadable coefficients
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_INTERPOLATE_FACTOR,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
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
class fir_interpolate_asym<TT_DATA,
                           TT_COEFF,
                           TP_FIR_LEN,
                           TP_INTERPOLATE_FACTOR,
                           TP_SHIFT,
                           TP_RND,
                           TP_INPUT_WINDOW_VSIZE,
                           CASC_IN_TRUE,
                           CASC_OUT_TRUE,
                           TP_FIR_RANGE_LEN,
                           TP_KERNEL_POSITION,
                           TP_CASC_LEN,
                           USE_COEFF_RELOAD_TRUE,
                           DUAL_IP_SINGLE,
                           TP_NUM_OUTPUTS,
                           USE_STREAM_API,
                           TP_MODIFY_MARGIN_OFFSET,
                           TP_COEFF_PHASE,
                           TP_COEFF_PHASE_OFFSET,
                           TP_COEFF_PHASES,
                           TP_COEFF_PHASES_LEN,
                           TP_SAT> : public kernelFilterClass<TT_DATA,
                                                              TT_COEFF,
                                                              TP_FIR_LEN,
                                                              TP_INTERPOLATE_FACTOR,
                                                              TP_SHIFT,
                                                              TP_RND,
                                                              TP_INPUT_WINDOW_VSIZE,
                                                              CASC_IN_TRUE,
                                                              CASC_OUT_TRUE,
                                                              TP_FIR_RANGE_LEN,
                                                              TP_KERNEL_POSITION,
                                                              TP_CASC_LEN,
                                                              USE_COEFF_RELOAD_TRUE,
                                                              DUAL_IP_SINGLE,
                                                              TP_NUM_OUTPUTS,
                                                              USE_STREAM_API,
                                                              TP_MODIFY_MARGIN_OFFSET,
                                                              TP_COEFF_PHASE,
                                                              TP_COEFF_PHASE_OFFSET,
                                                              TP_COEFF_PHASES,
                                                              TP_COEFF_PHASES_LEN,
                                                              TP_SAT> {
   private:
   public:
    // Constructor
    fir_interpolate_asym()
        : kernelFilterClass<TT_DATA,
                            TT_COEFF,
                            TP_FIR_LEN,
                            TP_INTERPOLATE_FACTOR,
                            TP_SHIFT,
                            TP_RND,
                            TP_INPUT_WINDOW_VSIZE,
                            CASC_IN_TRUE,
                            CASC_OUT_TRUE,
                            TP_FIR_RANGE_LEN,
                            TP_KERNEL_POSITION,
                            TP_CASC_LEN,
                            USE_COEFF_RELOAD_TRUE,
                            DUAL_IP_SINGLE,
                            TP_NUM_OUTPUTS,
                            USE_STREAM_API,
                            TP_MODIFY_MARGIN_OFFSET,
                            TP_COEFF_PHASE,
                            TP_COEFF_PHASE_OFFSET,
                            TP_COEFF_PHASES,
                            TP_COEFF_PHASES_LEN,
                            TP_SAT>() {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fir_interpolate_asym::filter); }

    // FIR
    void filter(input_stream<TT_DATA>* inStream, input_cascade_cacc* inCascade, output_cascade_cacc* outCascade);
};

///////////////////////////////////////////////////////////////////////////////////////////
/////////////               Dual Input Stream

// Single kernel specialization. No cascade ports. Static coefficients, single output

template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_INTERPOLATE_FACTOR,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_FIR_RANGE_LEN,
          int TP_MODIFY_MARGIN_OFFSET,
          unsigned int TP_COEFF_PHASE,
          unsigned int TP_COEFF_PHASE_OFFSET,
          unsigned int TP_COEFF_PHASES,
          unsigned int TP_COEFF_PHASES_LEN,
          unsigned int TP_SAT>
class fir_interpolate_asym<TT_DATA,
                           TT_COEFF,
                           TP_FIR_LEN,
                           TP_INTERPOLATE_FACTOR,
                           TP_SHIFT,
                           TP_RND,
                           TP_INPUT_WINDOW_VSIZE,
                           CASC_IN_FALSE,
                           CASC_OUT_FALSE,
                           TP_FIR_RANGE_LEN,
                           0,
                           1,
                           USE_COEFF_RELOAD_FALSE,
                           DUAL_IP_DUAL,
                           1,
                           USE_STREAM_API,
                           TP_MODIFY_MARGIN_OFFSET,
                           TP_COEFF_PHASE,
                           TP_COEFF_PHASE_OFFSET,
                           TP_COEFF_PHASES,
                           TP_COEFF_PHASES_LEN,
                           TP_SAT> : public kernelFilterClass<TT_DATA,
                                                              TT_COEFF,
                                                              TP_FIR_LEN,
                                                              TP_INTERPOLATE_FACTOR,
                                                              TP_SHIFT,
                                                              TP_RND,
                                                              TP_INPUT_WINDOW_VSIZE,
                                                              CASC_IN_FALSE,
                                                              CASC_OUT_FALSE,
                                                              TP_FIR_RANGE_LEN,
                                                              0,
                                                              1,
                                                              USE_COEFF_RELOAD_FALSE,
                                                              DUAL_IP_DUAL,
                                                              1,
                                                              USE_STREAM_API,
                                                              TP_MODIFY_MARGIN_OFFSET,
                                                              TP_COEFF_PHASE,
                                                              TP_COEFF_PHASE_OFFSET,
                                                              TP_COEFF_PHASES,
                                                              TP_COEFF_PHASES_LEN,
                                                              TP_SAT> {
   private:
   public:
    // Constructor
    fir_interpolate_asym(const TT_COEFF (&taps)[TP_FIR_LEN])
        : kernelFilterClass<TT_DATA,
                            TT_COEFF,
                            TP_FIR_LEN,
                            TP_INTERPOLATE_FACTOR,
                            TP_SHIFT,
                            TP_RND,
                            TP_INPUT_WINDOW_VSIZE,
                            CASC_IN_FALSE,
                            CASC_OUT_FALSE,
                            TP_FIR_RANGE_LEN,
                            0,
                            1,
                            USE_COEFF_RELOAD_FALSE,
                            DUAL_IP_DUAL,
                            1,
                            USE_STREAM_API,
                            TP_MODIFY_MARGIN_OFFSET,
                            TP_COEFF_PHASE,
                            TP_COEFF_PHASE_OFFSET,
                            TP_COEFF_PHASES,
                            TP_COEFF_PHASES_LEN,
                            TP_SAT>(taps) {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fir_interpolate_asym::filter); }

    // FIR
    void filter(input_stream<TT_DATA>* inStream, input_stream<TT_DATA>* inStream2, output_stream<TT_DATA>* outStream);
};

// Single kernel specialization. No cascade ports. Static coefficients, dual output

template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_INTERPOLATE_FACTOR,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_FIR_RANGE_LEN,
          int TP_MODIFY_MARGIN_OFFSET,
          unsigned int TP_COEFF_PHASE,
          unsigned int TP_COEFF_PHASE_OFFSET,
          unsigned int TP_COEFF_PHASES,
          unsigned int TP_COEFF_PHASES_LEN,
          unsigned int TP_SAT>
class fir_interpolate_asym<TT_DATA,
                           TT_COEFF,
                           TP_FIR_LEN,
                           TP_INTERPOLATE_FACTOR,
                           TP_SHIFT,
                           TP_RND,
                           TP_INPUT_WINDOW_VSIZE,
                           CASC_IN_FALSE,
                           CASC_IN_FALSE,
                           TP_FIR_RANGE_LEN,
                           0,
                           1,
                           USE_COEFF_RELOAD_FALSE,
                           DUAL_IP_DUAL,
                           2,
                           USE_STREAM_API,
                           TP_MODIFY_MARGIN_OFFSET,
                           TP_COEFF_PHASE,
                           TP_COEFF_PHASE_OFFSET,
                           TP_COEFF_PHASES,
                           TP_COEFF_PHASES_LEN,
                           TP_SAT> : public kernelFilterClass<TT_DATA,
                                                              TT_COEFF,
                                                              TP_FIR_LEN,
                                                              TP_INTERPOLATE_FACTOR,
                                                              TP_SHIFT,
                                                              TP_RND,
                                                              TP_INPUT_WINDOW_VSIZE,
                                                              CASC_IN_FALSE,
                                                              CASC_IN_FALSE,
                                                              TP_FIR_RANGE_LEN,
                                                              0,
                                                              1,
                                                              USE_COEFF_RELOAD_FALSE,
                                                              DUAL_IP_DUAL,
                                                              2,
                                                              USE_STREAM_API,
                                                              TP_MODIFY_MARGIN_OFFSET,
                                                              TP_COEFF_PHASE,
                                                              TP_COEFF_PHASE_OFFSET,
                                                              TP_COEFF_PHASES,
                                                              TP_COEFF_PHASES_LEN,
                                                              TP_SAT> {
   private:
   public:
    // Constructor
    fir_interpolate_asym(const TT_COEFF (&taps)[TP_FIR_LEN])
        : kernelFilterClass<TT_DATA,
                            TT_COEFF,
                            TP_FIR_LEN,
                            TP_INTERPOLATE_FACTOR,
                            TP_SHIFT,
                            TP_RND,
                            TP_INPUT_WINDOW_VSIZE,
                            CASC_IN_FALSE,
                            CASC_IN_FALSE,
                            TP_FIR_RANGE_LEN,
                            0,
                            1,
                            USE_COEFF_RELOAD_FALSE,
                            DUAL_IP_DUAL,
                            2,
                            USE_STREAM_API,
                            TP_MODIFY_MARGIN_OFFSET,
                            TP_COEFF_PHASE,
                            TP_COEFF_PHASE_OFFSET,
                            TP_COEFF_PHASES,
                            TP_COEFF_PHASES_LEN,
                            TP_SAT>(taps) {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fir_interpolate_asym::filter); }

    // FIR
    void filter(input_stream<TT_DATA>* inStream,
                input_stream<TT_DATA>* inStream2,
                output_stream<TT_DATA>* outStream,
                output_stream<TT_DATA>* outStream2);
};

// Single kernel specialization. No cascade ports. Reloadable coefficients, single output
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_INTERPOLATE_FACTOR,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_FIR_RANGE_LEN,
          int TP_MODIFY_MARGIN_OFFSET,
          unsigned int TP_COEFF_PHASE,
          unsigned int TP_COEFF_PHASE_OFFSET,
          unsigned int TP_COEFF_PHASES,
          unsigned int TP_COEFF_PHASES_LEN,
          unsigned int TP_SAT>
class fir_interpolate_asym<TT_DATA,
                           TT_COEFF,
                           TP_FIR_LEN,
                           TP_INTERPOLATE_FACTOR,
                           TP_SHIFT,
                           TP_RND,
                           TP_INPUT_WINDOW_VSIZE,
                           CASC_IN_FALSE,
                           CASC_IN_FALSE,
                           TP_FIR_RANGE_LEN,
                           0,
                           1,
                           USE_COEFF_RELOAD_TRUE,
                           DUAL_IP_DUAL,
                           1,
                           USE_STREAM_API,
                           TP_MODIFY_MARGIN_OFFSET,
                           TP_COEFF_PHASE,
                           TP_COEFF_PHASE_OFFSET,
                           TP_COEFF_PHASES,
                           TP_COEFF_PHASES_LEN,
                           TP_SAT> : public kernelFilterClass<TT_DATA,
                                                              TT_COEFF,
                                                              TP_FIR_LEN,
                                                              TP_INTERPOLATE_FACTOR,
                                                              TP_SHIFT,
                                                              TP_RND,
                                                              TP_INPUT_WINDOW_VSIZE,
                                                              CASC_IN_FALSE,
                                                              CASC_IN_FALSE,
                                                              TP_FIR_RANGE_LEN,
                                                              0,
                                                              1,
                                                              USE_COEFF_RELOAD_TRUE,
                                                              DUAL_IP_DUAL,
                                                              1,
                                                              USE_STREAM_API,
                                                              TP_MODIFY_MARGIN_OFFSET,
                                                              TP_COEFF_PHASE,
                                                              TP_COEFF_PHASE_OFFSET,
                                                              TP_COEFF_PHASES,
                                                              TP_COEFF_PHASES_LEN,
                                                              TP_SAT> {
   private:
   public:
    // Constructor
    fir_interpolate_asym()
        : kernelFilterClass<TT_DATA,
                            TT_COEFF,
                            TP_FIR_LEN,
                            TP_INTERPOLATE_FACTOR,
                            TP_SHIFT,
                            TP_RND,
                            TP_INPUT_WINDOW_VSIZE,
                            CASC_IN_FALSE,
                            CASC_IN_FALSE,
                            TP_FIR_RANGE_LEN,
                            0,
                            1,
                            USE_COEFF_RELOAD_TRUE,
                            DUAL_IP_DUAL,
                            1,
                            USE_STREAM_API,
                            TP_MODIFY_MARGIN_OFFSET,
                            TP_COEFF_PHASE,
                            TP_COEFF_PHASE_OFFSET,
                            TP_COEFF_PHASES,
                            TP_COEFF_PHASES_LEN,
                            TP_SAT>() {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fir_interpolate_asym::filter); }

    // FIR
    void filter(input_stream<TT_DATA>* inStream,
                input_stream<TT_DATA>* inStream2,
                output_stream<TT_DATA>* outStream,
                const TT_COEFF (&inTaps)[TP_COEFF_PHASES_LEN]);
};

// Single kernel specialization. No cascade ports. Reloadable coefficients, dual output

template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_INTERPOLATE_FACTOR,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_FIR_RANGE_LEN,
          int TP_MODIFY_MARGIN_OFFSET,
          unsigned int TP_COEFF_PHASE,
          unsigned int TP_COEFF_PHASE_OFFSET,
          unsigned int TP_COEFF_PHASES,
          unsigned int TP_COEFF_PHASES_LEN,
          unsigned int TP_SAT>
class fir_interpolate_asym<TT_DATA,
                           TT_COEFF,
                           TP_FIR_LEN,
                           TP_INTERPOLATE_FACTOR,
                           TP_SHIFT,
                           TP_RND,
                           TP_INPUT_WINDOW_VSIZE,
                           CASC_IN_FALSE,
                           CASC_OUT_FALSE,
                           TP_FIR_RANGE_LEN,
                           0,
                           1,
                           USE_COEFF_RELOAD_TRUE,
                           DUAL_IP_DUAL,
                           2,
                           USE_STREAM_API,
                           TP_MODIFY_MARGIN_OFFSET,
                           TP_COEFF_PHASE,
                           TP_COEFF_PHASE_OFFSET,
                           TP_COEFF_PHASES,
                           TP_COEFF_PHASES_LEN,
                           TP_SAT> : public kernelFilterClass<TT_DATA,
                                                              TT_COEFF,
                                                              TP_FIR_LEN,
                                                              TP_INTERPOLATE_FACTOR,
                                                              TP_SHIFT,
                                                              TP_RND,
                                                              TP_INPUT_WINDOW_VSIZE,
                                                              CASC_IN_FALSE,
                                                              CASC_OUT_FALSE,
                                                              TP_FIR_RANGE_LEN,
                                                              0,
                                                              1,
                                                              USE_COEFF_RELOAD_TRUE,
                                                              DUAL_IP_DUAL,
                                                              2,
                                                              USE_STREAM_API,
                                                              TP_MODIFY_MARGIN_OFFSET,
                                                              TP_COEFF_PHASE,
                                                              TP_COEFF_PHASE_OFFSET,
                                                              TP_COEFF_PHASES,
                                                              TP_COEFF_PHASES_LEN,
                                                              TP_SAT> {
   private:
   public:
    // Constructor
    fir_interpolate_asym()
        : kernelFilterClass<TT_DATA,
                            TT_COEFF,
                            TP_FIR_LEN,
                            TP_INTERPOLATE_FACTOR,
                            TP_SHIFT,
                            TP_RND,
                            TP_INPUT_WINDOW_VSIZE,
                            CASC_IN_FALSE,
                            CASC_OUT_FALSE,
                            TP_FIR_RANGE_LEN,
                            0,
                            1,
                            USE_COEFF_RELOAD_TRUE,
                            DUAL_IP_DUAL,
                            2,
                            USE_STREAM_API,
                            TP_MODIFY_MARGIN_OFFSET,
                            TP_COEFF_PHASE,
                            TP_COEFF_PHASE_OFFSET,
                            TP_COEFF_PHASES,
                            TP_COEFF_PHASES_LEN,
                            TP_SAT>() {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fir_interpolate_asym::filter); }

    // FIR
    void filter(input_stream<TT_DATA>* inStream,
                input_stream<TT_DATA>* inStream2,
                output_stream<TT_DATA>* outStream,
                output_stream<TT_DATA>* outStream2,
                const TT_COEFF (&inTaps)[TP_COEFF_PHASES_LEN]);
};

// cascaded kernels - final kernel specialisation. single output. static coefficients
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_INTERPOLATE_FACTOR,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_FIR_RANGE_LEN,
          unsigned int TP_KERNEL_POSITION,
          unsigned int TP_CASC_LEN,
          int TP_MODIFY_MARGIN_OFFSET,
          unsigned int TP_COEFF_PHASE,
          unsigned int TP_COEFF_PHASE_OFFSET,
          unsigned int TP_COEFF_PHASES,
          unsigned int TP_COEFF_PHASES_LEN,
          unsigned int TP_SAT>
class fir_interpolate_asym<TT_DATA,
                           TT_COEFF,
                           TP_FIR_LEN,
                           TP_INTERPOLATE_FACTOR,
                           TP_SHIFT,
                           TP_RND,
                           TP_INPUT_WINDOW_VSIZE,
                           CASC_IN_TRUE,
                           CASC_OUT_FALSE,
                           TP_FIR_RANGE_LEN,
                           TP_KERNEL_POSITION,
                           TP_CASC_LEN,
                           USE_COEFF_RELOAD_FALSE,
                           DUAL_IP_DUAL,
                           1,
                           USE_STREAM_API,
                           TP_MODIFY_MARGIN_OFFSET,
                           TP_COEFF_PHASE,
                           TP_COEFF_PHASE_OFFSET,
                           TP_COEFF_PHASES,
                           TP_COEFF_PHASES_LEN,
                           TP_SAT> : public kernelFilterClass<TT_DATA,
                                                              TT_COEFF,
                                                              TP_FIR_LEN,
                                                              TP_INTERPOLATE_FACTOR,
                                                              TP_SHIFT,
                                                              TP_RND,
                                                              TP_INPUT_WINDOW_VSIZE,
                                                              CASC_IN_TRUE,
                                                              CASC_OUT_FALSE,
                                                              TP_FIR_RANGE_LEN,
                                                              TP_KERNEL_POSITION,
                                                              TP_CASC_LEN,
                                                              USE_COEFF_RELOAD_FALSE,
                                                              DUAL_IP_DUAL,
                                                              1,
                                                              USE_STREAM_API,
                                                              TP_MODIFY_MARGIN_OFFSET,
                                                              TP_COEFF_PHASE,
                                                              TP_COEFF_PHASE_OFFSET,
                                                              TP_COEFF_PHASES,
                                                              TP_COEFF_PHASES_LEN,
                                                              TP_SAT> {
   private:
   public:
    // Constructor
    fir_interpolate_asym(const TT_COEFF (&taps)[TP_FIR_LEN])
        : kernelFilterClass<TT_DATA,
                            TT_COEFF,
                            TP_FIR_LEN,
                            TP_INTERPOLATE_FACTOR,
                            TP_SHIFT,
                            TP_RND,
                            TP_INPUT_WINDOW_VSIZE,
                            CASC_IN_TRUE,
                            CASC_OUT_FALSE,
                            TP_FIR_RANGE_LEN,
                            TP_KERNEL_POSITION,
                            TP_CASC_LEN,
                            USE_COEFF_RELOAD_FALSE,
                            DUAL_IP_DUAL,
                            1,
                            USE_STREAM_API,
                            TP_MODIFY_MARGIN_OFFSET,
                            TP_COEFF_PHASE,
                            TP_COEFF_PHASE_OFFSET,
                            TP_COEFF_PHASES,
                            TP_COEFF_PHASES_LEN,
                            TP_SAT>(taps) {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fir_interpolate_asym::filter); }

    // FIR
    void filter(input_stream<TT_DATA>* inStream,
                input_stream<TT_DATA>* inStream2,
                input_cascade_cacc* inCascade,
                output_stream<TT_DATA>* outStream);
};

//-----------------------------------------------------------------------------------------------------
// Partially specialized classes for cascaded interface - final kernel. Static coefficients dual output
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_INTERPOLATE_FACTOR,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_FIR_RANGE_LEN,
          unsigned int TP_KERNEL_POSITION,
          unsigned int TP_CASC_LEN,
          int TP_MODIFY_MARGIN_OFFSET,
          unsigned int TP_COEFF_PHASE,
          unsigned int TP_COEFF_PHASE_OFFSET,
          unsigned int TP_COEFF_PHASES,
          unsigned int TP_COEFF_PHASES_LEN,
          unsigned int TP_SAT>
class fir_interpolate_asym<TT_DATA,
                           TT_COEFF,
                           TP_FIR_LEN,
                           TP_INTERPOLATE_FACTOR,
                           TP_SHIFT,
                           TP_RND,
                           TP_INPUT_WINDOW_VSIZE,
                           CASC_IN_TRUE,
                           CASC_OUT_FALSE,
                           TP_FIR_RANGE_LEN,
                           TP_KERNEL_POSITION,
                           TP_CASC_LEN,
                           USE_COEFF_RELOAD_FALSE,
                           DUAL_IP_DUAL,
                           2,
                           USE_STREAM_API,
                           TP_MODIFY_MARGIN_OFFSET,
                           TP_COEFF_PHASE,
                           TP_COEFF_PHASE_OFFSET,
                           TP_COEFF_PHASES,
                           TP_COEFF_PHASES_LEN,
                           TP_SAT> : public kernelFilterClass<TT_DATA,
                                                              TT_COEFF,
                                                              TP_FIR_LEN,
                                                              TP_INTERPOLATE_FACTOR,
                                                              TP_SHIFT,
                                                              TP_RND,
                                                              TP_INPUT_WINDOW_VSIZE,
                                                              CASC_IN_TRUE,
                                                              CASC_OUT_FALSE,
                                                              TP_FIR_RANGE_LEN,
                                                              TP_KERNEL_POSITION,
                                                              TP_CASC_LEN,
                                                              USE_COEFF_RELOAD_FALSE,
                                                              DUAL_IP_DUAL,
                                                              2,
                                                              USE_STREAM_API,
                                                              TP_MODIFY_MARGIN_OFFSET,
                                                              TP_COEFF_PHASE,
                                                              TP_COEFF_PHASE_OFFSET,
                                                              TP_COEFF_PHASES,
                                                              TP_COEFF_PHASES_LEN,
                                                              TP_SAT> {
   private:
   public:
    // Constructor
    fir_interpolate_asym(const TT_COEFF (&taps)[TP_FIR_LEN])
        : kernelFilterClass<TT_DATA,
                            TT_COEFF,
                            TP_FIR_LEN,
                            TP_INTERPOLATE_FACTOR,
                            TP_SHIFT,
                            TP_RND,
                            TP_INPUT_WINDOW_VSIZE,
                            CASC_IN_TRUE,
                            CASC_OUT_FALSE,
                            TP_FIR_RANGE_LEN,
                            TP_KERNEL_POSITION,
                            TP_CASC_LEN,
                            USE_COEFF_RELOAD_FALSE,
                            DUAL_IP_DUAL,
                            2,
                            USE_STREAM_API,
                            TP_MODIFY_MARGIN_OFFSET,
                            TP_COEFF_PHASE,
                            TP_COEFF_PHASE_OFFSET,
                            TP_COEFF_PHASES,
                            TP_COEFF_PHASES_LEN,
                            TP_SAT>(taps) {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fir_interpolate_asym::filter); }

    // FIR
    void filter(input_stream<TT_DATA>* inStream,
                input_stream<TT_DATA>* inStream2,
                input_cascade_cacc* inCascade,
                output_stream<TT_DATA>* outStream,
                output_stream<TT_DATA>* outStream2);
};

//-----------------------------------------------------------------------------------------------------
// Partially specialized classes for cascaded interface - first kernel. Static coefficients single output
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_INTERPOLATE_FACTOR,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
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
class fir_interpolate_asym<TT_DATA,
                           TT_COEFF,
                           TP_FIR_LEN,
                           TP_INTERPOLATE_FACTOR,
                           TP_SHIFT,
                           TP_RND,
                           TP_INPUT_WINDOW_VSIZE,
                           CASC_IN_FALSE,
                           CASC_OUT_TRUE,
                           TP_FIR_RANGE_LEN,
                           TP_KERNEL_POSITION,
                           TP_CASC_LEN,
                           USE_COEFF_RELOAD_FALSE,
                           DUAL_IP_DUAL,
                           TP_NUM_OUTPUTS,
                           USE_STREAM_API,
                           TP_MODIFY_MARGIN_OFFSET,
                           TP_COEFF_PHASE,
                           TP_COEFF_PHASE_OFFSET,
                           TP_COEFF_PHASES,
                           TP_COEFF_PHASES_LEN,
                           TP_SAT> : public kernelFilterClass<TT_DATA,
                                                              TT_COEFF,
                                                              TP_FIR_LEN,
                                                              TP_INTERPOLATE_FACTOR,
                                                              TP_SHIFT,
                                                              TP_RND,
                                                              TP_INPUT_WINDOW_VSIZE,
                                                              CASC_IN_FALSE,
                                                              CASC_OUT_TRUE,
                                                              TP_FIR_RANGE_LEN,
                                                              TP_KERNEL_POSITION,
                                                              TP_CASC_LEN,
                                                              USE_COEFF_RELOAD_FALSE,
                                                              DUAL_IP_DUAL,
                                                              TP_NUM_OUTPUTS,
                                                              USE_STREAM_API,
                                                              TP_MODIFY_MARGIN_OFFSET,
                                                              TP_COEFF_PHASE,
                                                              TP_COEFF_PHASE_OFFSET,
                                                              TP_COEFF_PHASES,
                                                              TP_COEFF_PHASES_LEN,
                                                              TP_SAT> {
   private:
   public:
    // Constructor
    fir_interpolate_asym(const TT_COEFF (&taps)[TP_FIR_LEN])
        : kernelFilterClass<TT_DATA,
                            TT_COEFF,
                            TP_FIR_LEN,
                            TP_INTERPOLATE_FACTOR,
                            TP_SHIFT,
                            TP_RND,
                            TP_INPUT_WINDOW_VSIZE,
                            CASC_IN_FALSE,
                            CASC_OUT_TRUE,
                            TP_FIR_RANGE_LEN,
                            TP_KERNEL_POSITION,
                            TP_CASC_LEN,
                            USE_COEFF_RELOAD_FALSE,
                            DUAL_IP_DUAL,
                            TP_NUM_OUTPUTS,
                            USE_STREAM_API,
                            TP_MODIFY_MARGIN_OFFSET,
                            TP_COEFF_PHASE,
                            TP_COEFF_PHASE_OFFSET,
                            TP_COEFF_PHASES,
                            TP_COEFF_PHASES_LEN,
                            TP_SAT>(taps) {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fir_interpolate_asym::filter); }

    // FIR
    void filter(input_stream<TT_DATA>* inStream, input_stream<TT_DATA>* inStream2, output_cascade_cacc* outCascade);
};

//-----------------------------------------------------------------------------------------------------
// Partially specialized classes for cascaded interface - middle kernel. Static coefficients
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_INTERPOLATE_FACTOR,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
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
class fir_interpolate_asym<TT_DATA,
                           TT_COEFF,
                           TP_FIR_LEN,
                           TP_INTERPOLATE_FACTOR,
                           TP_SHIFT,
                           TP_RND,
                           TP_INPUT_WINDOW_VSIZE,
                           CASC_IN_TRUE,
                           CASC_OUT_TRUE,
                           TP_FIR_RANGE_LEN,
                           TP_KERNEL_POSITION,
                           TP_CASC_LEN,
                           USE_COEFF_RELOAD_FALSE,
                           DUAL_IP_DUAL,
                           TP_NUM_OUTPUTS,
                           USE_STREAM_API,
                           TP_MODIFY_MARGIN_OFFSET,
                           TP_COEFF_PHASE,
                           TP_COEFF_PHASE_OFFSET,
                           TP_COEFF_PHASES,
                           TP_COEFF_PHASES_LEN,
                           TP_SAT> : public kernelFilterClass<TT_DATA,
                                                              TT_COEFF,
                                                              TP_FIR_LEN,
                                                              TP_INTERPOLATE_FACTOR,
                                                              TP_SHIFT,
                                                              TP_RND,
                                                              TP_INPUT_WINDOW_VSIZE,
                                                              CASC_IN_TRUE,
                                                              CASC_OUT_TRUE,
                                                              TP_FIR_RANGE_LEN,
                                                              TP_KERNEL_POSITION,
                                                              TP_CASC_LEN,
                                                              USE_COEFF_RELOAD_FALSE,
                                                              DUAL_IP_DUAL,
                                                              TP_NUM_OUTPUTS,
                                                              USE_STREAM_API,
                                                              TP_MODIFY_MARGIN_OFFSET,
                                                              TP_COEFF_PHASE,
                                                              TP_COEFF_PHASE_OFFSET,
                                                              TP_COEFF_PHASES,
                                                              TP_COEFF_PHASES_LEN,
                                                              TP_SAT> {
   private:
   public:
    // Constructor
    fir_interpolate_asym(const TT_COEFF (&taps)[TP_FIR_LEN])
        : kernelFilterClass<TT_DATA,
                            TT_COEFF,
                            TP_FIR_LEN,
                            TP_INTERPOLATE_FACTOR,
                            TP_SHIFT,
                            TP_RND,
                            TP_INPUT_WINDOW_VSIZE,
                            CASC_IN_TRUE,
                            CASC_OUT_TRUE,
                            TP_FIR_RANGE_LEN,
                            TP_KERNEL_POSITION,
                            TP_CASC_LEN,
                            USE_COEFF_RELOAD_FALSE,
                            DUAL_IP_DUAL,
                            TP_NUM_OUTPUTS,
                            USE_STREAM_API,
                            TP_MODIFY_MARGIN_OFFSET,
                            TP_COEFF_PHASE,
                            TP_COEFF_PHASE_OFFSET,
                            TP_COEFF_PHASES,
                            TP_COEFF_PHASES_LEN,
                            TP_SAT>(taps) {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fir_interpolate_asym::filter); }

    // FIR
    void filter(input_stream<TT_DATA>* inStream,
                input_stream<TT_DATA>* inStream2,
                input_cascade_cacc* inCascade,
                output_cascade_cacc* outCascade);
};

//-----------------------------------------------------------------------------------------------------
// Partially specialized classes for cascaded interface - final kernel. Reloadable coefficients single output
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_INTERPOLATE_FACTOR,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_FIR_RANGE_LEN,
          unsigned int TP_KERNEL_POSITION,
          unsigned int TP_CASC_LEN,
          int TP_MODIFY_MARGIN_OFFSET,
          unsigned int TP_COEFF_PHASE,
          unsigned int TP_COEFF_PHASE_OFFSET,
          unsigned int TP_COEFF_PHASES,
          unsigned int TP_COEFF_PHASES_LEN,
          unsigned int TP_SAT>
class fir_interpolate_asym<TT_DATA,
                           TT_COEFF,
                           TP_FIR_LEN,
                           TP_INTERPOLATE_FACTOR,
                           TP_SHIFT,
                           TP_RND,
                           TP_INPUT_WINDOW_VSIZE,
                           CASC_IN_TRUE,
                           CASC_OUT_FALSE,
                           TP_FIR_RANGE_LEN,
                           TP_KERNEL_POSITION,
                           TP_CASC_LEN,
                           USE_COEFF_RELOAD_TRUE,
                           DUAL_IP_DUAL,
                           1,
                           USE_STREAM_API,
                           TP_MODIFY_MARGIN_OFFSET,
                           TP_COEFF_PHASE,
                           TP_COEFF_PHASE_OFFSET,
                           TP_COEFF_PHASES,
                           TP_COEFF_PHASES_LEN,
                           TP_SAT> : public kernelFilterClass<TT_DATA,
                                                              TT_COEFF,
                                                              TP_FIR_LEN,
                                                              TP_INTERPOLATE_FACTOR,
                                                              TP_SHIFT,
                                                              TP_RND,
                                                              TP_INPUT_WINDOW_VSIZE,
                                                              CASC_IN_TRUE,
                                                              CASC_OUT_FALSE,
                                                              TP_FIR_RANGE_LEN,
                                                              TP_KERNEL_POSITION,
                                                              TP_CASC_LEN,
                                                              USE_COEFF_RELOAD_TRUE,
                                                              DUAL_IP_DUAL,
                                                              1,
                                                              USE_STREAM_API,
                                                              TP_MODIFY_MARGIN_OFFSET,
                                                              TP_COEFF_PHASE,
                                                              TP_COEFF_PHASE_OFFSET,
                                                              TP_COEFF_PHASES,
                                                              TP_COEFF_PHASES_LEN,
                                                              TP_SAT> {
   private:
   public:
    // Constructor
    fir_interpolate_asym()
        : kernelFilterClass<TT_DATA,
                            TT_COEFF,
                            TP_FIR_LEN,
                            TP_INTERPOLATE_FACTOR,
                            TP_SHIFT,
                            TP_RND,
                            TP_INPUT_WINDOW_VSIZE,
                            CASC_IN_TRUE,
                            CASC_OUT_FALSE,
                            TP_FIR_RANGE_LEN,
                            TP_KERNEL_POSITION,
                            TP_CASC_LEN,
                            USE_COEFF_RELOAD_TRUE,
                            DUAL_IP_DUAL,
                            1,
                            USE_STREAM_API,
                            TP_MODIFY_MARGIN_OFFSET,
                            TP_COEFF_PHASE,
                            TP_COEFF_PHASE_OFFSET,
                            TP_COEFF_PHASES,
                            TP_COEFF_PHASES_LEN,
                            TP_SAT>() {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fir_interpolate_asym::filter); }

    // FIR
    void filter(input_stream<TT_DATA>* inStream,
                input_stream<TT_DATA>* inStream2,
                input_cascade_cacc* inCascade,
                output_stream<TT_DATA>* outStream);
};

//-----------------------------------------------------------------------------------------------------
// Partially specialized classes for cascaded interface - final kernel. Reloadable coefficients. dual output
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_INTERPOLATE_FACTOR,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_FIR_RANGE_LEN,
          unsigned int TP_KERNEL_POSITION,
          unsigned int TP_CASC_LEN,
          int TP_MODIFY_MARGIN_OFFSET,
          unsigned int TP_COEFF_PHASE,
          unsigned int TP_COEFF_PHASE_OFFSET,
          unsigned int TP_COEFF_PHASES,
          unsigned int TP_COEFF_PHASES_LEN,
          unsigned int TP_SAT>
class fir_interpolate_asym<TT_DATA,
                           TT_COEFF,
                           TP_FIR_LEN,
                           TP_INTERPOLATE_FACTOR,
                           TP_SHIFT,
                           TP_RND,
                           TP_INPUT_WINDOW_VSIZE,
                           CASC_IN_TRUE,
                           CASC_OUT_FALSE,
                           TP_FIR_RANGE_LEN,
                           TP_KERNEL_POSITION,
                           TP_CASC_LEN,
                           USE_COEFF_RELOAD_TRUE,
                           DUAL_IP_DUAL,
                           2,
                           USE_STREAM_API,
                           TP_MODIFY_MARGIN_OFFSET,
                           TP_COEFF_PHASE,
                           TP_COEFF_PHASE_OFFSET,
                           TP_COEFF_PHASES,
                           TP_COEFF_PHASES_LEN,
                           TP_SAT> : public kernelFilterClass<TT_DATA,
                                                              TT_COEFF,
                                                              TP_FIR_LEN,
                                                              TP_INTERPOLATE_FACTOR,
                                                              TP_SHIFT,
                                                              TP_RND,
                                                              TP_INPUT_WINDOW_VSIZE,
                                                              CASC_IN_TRUE,
                                                              CASC_OUT_FALSE,
                                                              TP_FIR_RANGE_LEN,
                                                              TP_KERNEL_POSITION,
                                                              TP_CASC_LEN,
                                                              USE_COEFF_RELOAD_TRUE,
                                                              DUAL_IP_DUAL,
                                                              2,
                                                              USE_STREAM_API,
                                                              TP_MODIFY_MARGIN_OFFSET,
                                                              TP_COEFF_PHASE,
                                                              TP_COEFF_PHASE_OFFSET,
                                                              TP_COEFF_PHASES,
                                                              TP_COEFF_PHASES_LEN,
                                                              TP_SAT> {
   private:
   public:
    // Constructor
    fir_interpolate_asym()
        : kernelFilterClass<TT_DATA,
                            TT_COEFF,
                            TP_FIR_LEN,
                            TP_INTERPOLATE_FACTOR,
                            TP_SHIFT,
                            TP_RND,
                            TP_INPUT_WINDOW_VSIZE,
                            CASC_IN_TRUE,
                            CASC_OUT_FALSE,
                            TP_FIR_RANGE_LEN,
                            TP_KERNEL_POSITION,
                            TP_CASC_LEN,
                            USE_COEFF_RELOAD_TRUE,
                            DUAL_IP_DUAL,
                            2,
                            USE_STREAM_API,
                            TP_MODIFY_MARGIN_OFFSET,
                            TP_COEFF_PHASE,
                            TP_COEFF_PHASE_OFFSET,
                            TP_COEFF_PHASES,
                            TP_COEFF_PHASES_LEN,
                            TP_SAT>() {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fir_interpolate_asym::filter); }

    // FIR
    void filter(input_stream<TT_DATA>* inStream,
                input_stream<TT_DATA>* inStream2,
                input_cascade_cacc* inCascade,
                output_stream<TT_DATA>* outStream,
                output_stream<TT_DATA>* outStream2);
};

//-----------------------------------------------------------------------------------------------------
// Partially specialized classes for cascaded interface - first kernel. Reloadable coefficients. single output
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_INTERPOLATE_FACTOR,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
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
class fir_interpolate_asym<TT_DATA,
                           TT_COEFF,
                           TP_FIR_LEN,
                           TP_INTERPOLATE_FACTOR,
                           TP_SHIFT,
                           TP_RND,
                           TP_INPUT_WINDOW_VSIZE,
                           CASC_IN_FALSE,
                           CASC_OUT_TRUE,
                           TP_FIR_RANGE_LEN,
                           TP_KERNEL_POSITION,
                           TP_CASC_LEN,
                           USE_COEFF_RELOAD_TRUE,
                           DUAL_IP_DUAL,
                           TP_NUM_OUTPUTS,
                           USE_STREAM_API,
                           TP_MODIFY_MARGIN_OFFSET,
                           TP_COEFF_PHASE,
                           TP_COEFF_PHASE_OFFSET,
                           TP_COEFF_PHASES,
                           TP_COEFF_PHASES_LEN,
                           TP_SAT> : public kernelFilterClass<TT_DATA,
                                                              TT_COEFF,
                                                              TP_FIR_LEN,
                                                              TP_INTERPOLATE_FACTOR,
                                                              TP_SHIFT,
                                                              TP_RND,
                                                              TP_INPUT_WINDOW_VSIZE,
                                                              CASC_IN_FALSE,
                                                              CASC_OUT_TRUE,
                                                              TP_FIR_RANGE_LEN,
                                                              TP_KERNEL_POSITION,
                                                              TP_CASC_LEN,
                                                              USE_COEFF_RELOAD_TRUE,
                                                              DUAL_IP_DUAL,
                                                              TP_NUM_OUTPUTS,
                                                              USE_STREAM_API,
                                                              TP_MODIFY_MARGIN_OFFSET,
                                                              TP_COEFF_PHASE,
                                                              TP_COEFF_PHASE_OFFSET,
                                                              TP_COEFF_PHASES,
                                                              TP_COEFF_PHASES_LEN,
                                                              TP_SAT> {
   private:
   public:
    // Constructor
    fir_interpolate_asym()
        : kernelFilterClass<TT_DATA,
                            TT_COEFF,
                            TP_FIR_LEN,
                            TP_INTERPOLATE_FACTOR,
                            TP_SHIFT,
                            TP_RND,
                            TP_INPUT_WINDOW_VSIZE,
                            CASC_IN_FALSE,
                            CASC_OUT_TRUE,
                            TP_FIR_RANGE_LEN,
                            TP_KERNEL_POSITION,
                            TP_CASC_LEN,
                            USE_COEFF_RELOAD_TRUE,
                            DUAL_IP_DUAL,
                            TP_NUM_OUTPUTS,
                            USE_STREAM_API,
                            TP_MODIFY_MARGIN_OFFSET,
                            TP_COEFF_PHASE,
                            TP_COEFF_PHASE_OFFSET,
                            TP_COEFF_PHASES,
                            TP_COEFF_PHASES_LEN,
                            TP_SAT>() {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fir_interpolate_asym::filter); }

    // FIR
    void filter(input_stream<TT_DATA>* inStream,
                input_stream<TT_DATA>* inStream2,
                output_cascade_cacc* inCascade,
                const TT_COEFF (&inTaps)[TP_COEFF_PHASES_LEN]);
};

//-----------------------------------------------------------------------------------------------------
// Partially specialized classes for cascaded interface - middle kernel. Reloadable coefficients
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_INTERPOLATE_FACTOR,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
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
class fir_interpolate_asym<TT_DATA,
                           TT_COEFF,
                           TP_FIR_LEN,
                           TP_INTERPOLATE_FACTOR,
                           TP_SHIFT,
                           TP_RND,
                           TP_INPUT_WINDOW_VSIZE,
                           CASC_IN_TRUE,
                           CASC_OUT_TRUE,
                           TP_FIR_RANGE_LEN,
                           TP_KERNEL_POSITION,
                           TP_CASC_LEN,
                           USE_COEFF_RELOAD_TRUE,
                           DUAL_IP_DUAL,
                           TP_NUM_OUTPUTS,
                           USE_STREAM_API,
                           TP_MODIFY_MARGIN_OFFSET,
                           TP_COEFF_PHASE,
                           TP_COEFF_PHASE_OFFSET,
                           TP_COEFF_PHASES,
                           TP_COEFF_PHASES_LEN,
                           TP_SAT> : public kernelFilterClass<TT_DATA,
                                                              TT_COEFF,
                                                              TP_FIR_LEN,
                                                              TP_INTERPOLATE_FACTOR,
                                                              TP_SHIFT,
                                                              TP_RND,
                                                              TP_INPUT_WINDOW_VSIZE,
                                                              CASC_IN_TRUE,
                                                              CASC_OUT_TRUE,
                                                              TP_FIR_RANGE_LEN,
                                                              TP_KERNEL_POSITION,
                                                              TP_CASC_LEN,
                                                              USE_COEFF_RELOAD_TRUE,
                                                              DUAL_IP_DUAL,
                                                              TP_NUM_OUTPUTS,
                                                              USE_STREAM_API,
                                                              TP_MODIFY_MARGIN_OFFSET,
                                                              TP_COEFF_PHASE,
                                                              TP_COEFF_PHASE_OFFSET,
                                                              TP_COEFF_PHASES,
                                                              TP_COEFF_PHASES_LEN,
                                                              TP_SAT> {
   private:
   public:
    // Constructor
    fir_interpolate_asym()
        : kernelFilterClass<TT_DATA,
                            TT_COEFF,
                            TP_FIR_LEN,
                            TP_INTERPOLATE_FACTOR,
                            TP_SHIFT,
                            TP_RND,
                            TP_INPUT_WINDOW_VSIZE,
                            CASC_IN_TRUE,
                            CASC_OUT_TRUE,
                            TP_FIR_RANGE_LEN,
                            TP_KERNEL_POSITION,
                            TP_CASC_LEN,
                            USE_COEFF_RELOAD_TRUE,
                            DUAL_IP_DUAL,
                            TP_NUM_OUTPUTS,
                            USE_STREAM_API,
                            TP_MODIFY_MARGIN_OFFSET,
                            TP_COEFF_PHASE,
                            TP_COEFF_PHASE_OFFSET,
                            TP_COEFF_PHASES,
                            TP_COEFF_PHASES_LEN,
                            TP_SAT>() {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fir_interpolate_asym::filter); }

    // FIR
    void filter(input_stream<TT_DATA>* inStream,
                input_stream<TT_DATA>* inStream2,
                input_cascade_cacc* inCascade,
                output_cascade_cacc* outCascade);
};
}
}
}
}
} // namespaces
#endif // fir_interpolate_asym_HPP
