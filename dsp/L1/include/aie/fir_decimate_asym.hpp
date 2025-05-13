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
#ifndef _DSPLIB_FIR_DECIMATE_ASYM_HPP_
#define _DSPLIB_FIR_DECIMATE_ASYM_HPP_

/*
Decimator Asymmetric FIR

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
   the reference model performs this reversal at run-time. This decimator implementation solves all polyphases for an
   output in a single lane.
   For large decimation factors, or large number of lanes (as required by data and coefficient type), it is not always
   possible to accommodate the
   input data step between lanes required because the maximum offset between lanes in a single operation is limited to
   15.
   Hence, the implementation may operate on fewer lanes per operation than the hardware supports.
*/

#include <array>

#include <adf.h>
#include "fir_utils.hpp"
#include "fir_decimate_asym_traits.hpp"

#include <vector>

namespace xf {
namespace dsp {
namespace aie {
namespace fir {
namespace decimate_asym {

template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_DECIMATE_FACTOR,
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
class fir_decimate_asym;

template <typename fp = fir_params_defaults>
class fir_decimate_asym_tl {
   public:
    // Get kernel's FIR range Length
    template <int pos>
    static constexpr unsigned int getKernelFirRangeLen() {
        constexpr unsigned int firRangeLen =
            pos + 1 == fp::BTP_CASC_LEN
                ? (fnFirRangeRem<fp::BTP_FIR_LEN, fp::BTP_CASC_LEN, pos, fp::BTP_DECIMATE_FACTOR>())
                : (fnFirRange<fp::BTP_FIR_LEN, fp::BTP_CASC_LEN, pos, fp::BTP_DECIMATE_FACTOR>());
        return firRangeLen;
    };

    template <int pos, int CLEN, unsigned int DF, int T_FIR_LEN, typename T_D, typename T_C, unsigned int SSR>
    static constexpr unsigned int fnCheckIfFits() {
        constexpr unsigned int decimateFactor = fnPermuteSupport() == 1 ? DF : 1;
        constexpr int samplesInBuff = fnSamplesIn1024<T_D>();
        constexpr unsigned int loadSize = getKernelStreamLoadVsize<T_D, T_C, decimateFactor>();
        constexpr unsigned int lanes = fnNumLanesDecAsym<T_D, T_C>();
        constexpr int rangeOffsetLastKernel = fnFirRangeOffset<T_FIR_LEN, CLEN, CLEN - 1, decimateFactor>();
        constexpr unsigned int fir_range_len = getKernelFirRangeLen<pos>();
        constexpr unsigned int firRangeOffset = fnFirRangeOffset<T_FIR_LEN, CLEN, pos, decimateFactor>();
        constexpr unsigned int dataOffsetNthKernel = getDataOffset<T_FIR_LEN, fir_range_len, firRangeOffset>();
        constexpr int streamInitNullAccs = getInitNullAccs<dataOffsetNthKernel, decimateFactor, loadSize>();
        constexpr int m_kInitDataNeeded =
            getInitDataNeeded<decimateFactor, lanes, streamInitNullAccs, SSR - 1, dataOffsetNthKernel, loadSize,
                              fir_range_len, firRangeOffset, rangeOffsetLastKernel>();
        if
            constexpr(m_kInitDataNeeded > samplesInBuff) { return 0; }
        else {
            return 1;
        }
    }

    // Get kernel's FIR range Length
    static constexpr unsigned int getFirRangeLen() { return fp::BTP_FIR_RANGE_LEN; };

    // Get kernel's FIR Total Tap Length
    static constexpr unsigned int getTapLen() {
        constexpr int rnd = fp::BTP_SSR * fp::BTP_DECIMATE_FACTOR;
        return CEIL(fp::BTP_FIR_LEN, rnd) / fp::BTP_SSR;
    };
    static constexpr unsigned int getDF() { return fp::BTP_DECIMATE_FACTOR; };
    static constexpr unsigned int getIF() { return 1; };

    // Get kernel's FIR Total Tap Length
    static constexpr unsigned int getSSRMargin() {
        constexpr unsigned int margin = fnFirMargin<fp::BTP_FIR_LEN / fp::BTP_SSR, typename fp::BTT_DATA>();
        return margin;
    };

    // Get FIR variant
    static constexpr eFIRVariant getFirType() { return eFIRVariant::kDecAsym; };

    using parent_class = fir_decimate_asym<typename fp::BTT_DATA,
                                           typename fp::BTT_COEFF,
                                           fp::BTP_FIR_LEN,
                                           fp::BTP_DECIMATE_FACTOR,
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
          unsigned int TP_DECIMATE_FACTOR,
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
class kernelFilterClass {
   private:
    // Parameter value defensive and legality checks
    // static_assert(TP_FIR_RANGE_LEN >= FIR_LEN_MIN,"ERROR: Illegal combination of design FIR length and cascade
    // length, resulting in kernel FIR length below minimum required value. ");
    static_assert(TP_FIR_LEN % TP_DECIMATE_FACTOR == 0, "ERROR: TP_FIR_LEN must be a multiple of TP_DECIMATE_FACTOR");
    static_assert(TP_FIR_RANGE_LEN % TP_DECIMATE_FACTOR == 0,
                  "ERROR: Illegal combination of design FIR length and cascade length. TP_FIR_RANGE_LEN must be a "
                  "multiple of TP_DECIMATE_FACTOR");
    static_assert(TP_DECIMATE_FACTOR >= DECIMATE_FACTOR_MIN && TP_DECIMATE_FACTOR <= DECIMATE_FACTOR_MAX,
                  "ERROR:TP_DECIMATE_FACTOR is outside the supported range.");
    static_assert(TP_SHIFT >= SHIFT_MIN && TP_SHIFT <= SHIFT_MAX, "ERROR: TP_SHIFT is out of the supported range.");
    static_assert(TP_RND >= ROUND_MIN && TP_RND <= ROUND_MAX, "ERROR: TP_RND is out of the supported range.");
    static_assert(TP_SAT >= SAT_MODE_MIN && TP_SAT <= SAT_MODE_MAX, "ERROR: TP_SAT is out of supported range");
    static_assert(TP_SAT != 2, "ERROR: TP_SAT is invalid. Valid values of TP_SAT are 0, 1, and 3");
    static_assert(fnEnumType<TT_DATA>() != enumUnknownType, "ERROR: TT_DATA is not a supported type.");
    static_assert(fnEnumType<TT_COEFF>() != enumUnknownType, "ERROR: TT_COEFF is not a supported type.");
    static_assert(fnFirDecAsymTypeSupport<TT_DATA, TT_COEFF>() != 0,
                  "ERROR: The combination of TT_DATA and TT_COEFF is not supported for this class.");
    static_assert(fnTypeCheckDataCoeffSize<TT_DATA, TT_COEFF>() != 0,
                  "ERROR: TT_DATA type less precise than TT_COEFF is not supported.");
    static_assert(fnTypeCheckDataCoeffCmplx<TT_DATA, TT_COEFF>() != 0,
                  "ERROR: real TT_DATA with complex TT_COEFF is not supported.");
    static_assert(fnTypeCheckDataCoeffFltInt<TT_DATA, TT_COEFF>() != 0,
                  "ERROR: a mix of float and integer types of TT_DATA and TT_COEFF is not supported.");
    static_assert(TP_NUM_OUTPUTS > 0 && TP_NUM_OUTPUTS <= 2, "ERROR: only single or dual outputs are supported.");
    static_assert(!(std::is_same<TT_DATA, cfloat>::value || std::is_same<TT_DATA, float>::value) || (TP_SHIFT == 0),
                  "ERROR: TP_SHIFT cannot be performed for TT_DATA=cfloat, so must be set to 0");

    static constexpr unsigned int m_kPermuteSupport = fnPermuteSupport();
    // constants derived from configuration parameters
    static constexpr unsigned int m_kDataRegVsize = kBuffSize128Byte / (sizeof(TT_DATA)); // buff size in Bytes
    static constexpr unsigned int m_kColumns =
        fnNumColumnsDecAsym<TT_DATA, TT_COEFF>(); // number of mult-adds per lane for main intrinsic
    static constexpr unsigned int m_kLanes = fnNumLanesDecAsym<TT_DATA, TT_COEFF>(); // number of operations in parallel
                                                                                     // of this type combinations that
                                                                                     // the vector processor can do.
    static constexpr unsigned int m_kInitLoadsInReg =
        fnDataLoadsInRegDecAsym<TT_DATA>(); // 4;  //ratio of sbuff to init load size.
    static constexpr unsigned int m_kInitLoadVsize =
        fnDataLoadVsizeDecAsym<TT_DATA>(); // number of samples in 256-bit init upd_w loads
    static constexpr unsigned int m_kDataLoadSize =
        m_kPermuteSupport == 1 ? fnLoadSizeDecAsym<TT_DATA, TT_COEFF>() : 256; // 256-bit or 128-bit loads
    static constexpr unsigned int m_kDataLoadVsize =
        m_kDataLoadSize /
        (8 * sizeof(TT_DATA)); // 8 samples when 256-bit loads are in use, 4 samples when 128-bit loads are used.
    static constexpr unsigned int m_kDataLoadsInReg =
        m_kDataLoadSize == 256 ? 4 : 8; // kBuffSize128Byte / m_kDataLoadVsize
    static constexpr unsigned int m_kSamplesInBuff = m_kDataLoadsInReg * m_kDataLoadVsize;
    static constexpr unsigned int m_kWinAccessByteSize =
        m_kPermuteSupport == 1
            ? 16
            : 32; // Restrict window accesses to 256-bits when full set of permutes are not available.
    static constexpr unsigned int m_kFirRangeOffset =
        fnFirRangeOffset<TP_FIR_LEN, TP_CASC_LEN, TP_KERNEL_POSITION, TP_DECIMATE_FACTOR>(); // FIR Cascade Offset for
                                                                                             // this kernel position
    static constexpr unsigned int m_kFirMargin = fnFirMargin<TP_FIR_LEN, TT_DATA>();         // FIR Margin.
    static constexpr unsigned int m_kFirMarginOffset =
        fnFirMargin<TP_FIR_LEN, TT_DATA>() - TP_FIR_LEN + 1; // FIR Margin Offset.
    static constexpr unsigned int m_kFirInitOffset = m_kFirRangeOffset + m_kFirMarginOffset + TP_MODIFY_MARGIN_OFFSET;
    static constexpr unsigned int m_kFirInitWinOffset =
        TRUNC((m_kFirInitOffset), (m_kWinAccessByteSize / sizeof(TT_DATA)));
    static constexpr unsigned int m_kDataBuffXOffset =
        m_kFirInitOffset % (m_kWinAccessByteSize / sizeof(TT_DATA)); // Remainder of m_kFirInitOffset divided by 128bit
    static constexpr unsigned int m_kFirRangeOffsetLastKernel =
        fnFirRangeOffset<TP_FIR_LEN, TP_CASC_LEN, TP_CASC_LEN - 1, TP_DECIMATE_FACTOR>(); // FIR Cascade Offset for this
                                                                                          // kernel position

    static constexpr unsigned int m_kStreamReadWidth = getStreamReadWidth<TT_DATA, TT_COEFF, TP_DECIMATE_FACTOR>();
    static constexpr unsigned int m_kStreamLoadVsize =
        getKernelStreamLoadVsize<TT_DATA, TT_COEFF, TP_DECIMATE_FACTOR>(); //

    static constexpr unsigned int m_kVOutSize = fnVOutSizeDecAsym<TT_DATA, TT_COEFF>();
    static constexpr unsigned int m_kLsize =
        (TP_INPUT_WINDOW_VSIZE / TP_DECIMATE_FACTOR) /
        m_kVOutSize; // loop length, given that <m_kVOutSize> samples are output per iteration of loop
    static constexpr unsigned int m_kRepeatFactor =
        TP_DECIMATE_FACTOR % 2 == 0 ? m_kInitLoadsInReg
                                    : m_kSamplesInBuff / m_kVOutSize; // only FACTORS of 2 or 3 supported

    static constexpr int dataOffsetNthKernel = getDataOffset<TP_FIR_LEN, TP_FIR_RANGE_LEN, m_kFirRangeOffset>();
    static constexpr int streamInitNullAccs =
        getInitNullAccs<dataOffsetNthKernel, TP_DECIMATE_FACTOR, m_kVOutSize>(); // Number of Null Mac Vectors sent as
                                                                                 // partial prouducts over cascade.
    static constexpr int dataNeededLastKernel =
        1 + TP_DECIMATE_FACTOR * (m_kLanes - 1) + (streamInitNullAccs * TP_DECIMATE_FACTOR * m_kLanes);
    static constexpr unsigned int kMinDataNeeded =
        (TP_MODIFY_MARGIN_OFFSET + dataNeededLastKernel - dataOffsetNthKernel);
    static constexpr unsigned int kMinDataLoaded = CEIL(kMinDataNeeded, m_kStreamLoadVsize);
    static constexpr unsigned int kMinDataLoadCycles = kMinDataLoaded / m_kStreamLoadVsize;

    static constexpr unsigned int m_kInitDataNeeded = getInitDataNeeded<TP_DECIMATE_FACTOR,
                                                                        m_kLanes,
                                                                        streamInitNullAccs,
                                                                        TP_MODIFY_MARGIN_OFFSET,
                                                                        dataOffsetNthKernel,
                                                                        m_kStreamLoadVsize,
                                                                        TP_FIR_RANGE_LEN,
                                                                        m_kFirRangeOffset,
                                                                        m_kFirRangeOffsetLastKernel>();

    static constexpr unsigned int m_kXoffsetRange = fnMaxXoffsetRange<TT_DATA>();
    static constexpr eArchType m_kArchIncrStrobeEn =
        (fnFirDecIncStrSupported<TT_DATA, TT_COEFF>() == SUPPORTED) ? kArchIncrStrobe : kArchBasic;
    static constexpr eArchType m_kArchWindow =
        ((TP_INPUT_WINDOW_VSIZE / TP_DECIMATE_FACTOR) % (m_kLanes * m_kRepeatFactor) == 0) &&
                (m_kInitDataNeeded <= m_kInitLoadVsize * (m_kInitLoadsInReg - 1) + 1)
            ? m_kArchIncrStrobeEn
            : kArchBasic;
    static constexpr eArchType m_kPermuteArch = TP_API == 1 ? kArchStream : m_kArchWindow;
    static constexpr eArchType m_kDecomposedArch =
        TP_API == 1 ? kArchStreamPhaseParallel : kArchPhaseParallel; // execute each phase in parallel. No ArchIncr
                                                                     // equivalent, as marginal benefit in QoR.  or
                                                                     // Stream arch? Later
    static constexpr eArchType m_kArch = m_kPermuteSupport == 1 ? m_kPermuteArch : m_kDecomposedArch;
    static constexpr unsigned int m_kDFDataRange = m_kDataBuffXOffset + (m_kLanes - 1) * TP_DECIMATE_FACTOR;
    static constexpr unsigned int m_kDFX = (m_kLanes - 1) * TP_DECIMATE_FACTOR < m_kXoffsetRange ? kLowDF : kHighDF;
    static constexpr unsigned int m_kFirLenCeilCols = CEIL(TP_FIR_RANGE_LEN, m_kColumns);
    static constexpr unsigned int m_kZbuffSize = 32; // kZbuffSize (256bit) - const for all data/coeff types
    static constexpr unsigned int m_kCoeffRegVsize = m_kZbuffSize / sizeof(TT_COEFF);
    unsigned int m_kDecimateOffsets;

    static constexpr int streamRptFactor =
        m_kPermuteSupport == 1 ? 8 : 4; // devices with permute support may read 128 or 256-bits per iteration. Devices
                                        // without permute support always read 256-bits of data, i.e. will wrap around a
                                        // 1024-bit register after 4 iterations, instead of 8.
    static constexpr int marginLoadsMappedToBuff = (m_kFirMargin % m_kSamplesInBuff) / m_kStreamLoadVsize;
    static constexpr int streamDataOffsetWithinBuff = (m_kFirInitOffset) % m_kSamplesInBuff;

    static constexpr int streamInitAccs =
        streamInitNullAccs == 0 ? streamRptFactor : (CEIL(streamInitNullAccs, streamRptFactor) - streamInitNullAccs);
    // static constexpr int streamInitAccs =  (CEIL(streamInitNullAccs, streamRptFactor) - streamInitNullAccs);
    // Single vector register gets stored on heap beetween iterations when permute is supported. Multiple registers are
    // used when permute is not supported.
    static constexpr int delaySize = m_kPermuteSupport == 1 ? 32 : 32 * TP_DECIMATE_FACTOR;
    alignas(__ALIGN_BYTE_SIZE__) int delay[delaySize] = {0};
    // int doInit = (TP_CASC_LEN == 1 || streamInitNullAccs==0)?0: 1;
    // int doPhaseParallelInit = (TP_CASC_LEN == 1 || streamInitNullAccs==0)?0: 1;
    int doInit = 1;
    // Coefficient Load Size - number of samples in 256-bits
    static constexpr unsigned int m_kCoeffLoadSize = 256 / 8 / sizeof(TT_COEFF);
    alignas(__ALIGN_BYTE_SIZE__) TT_COEFF
        m_rawInTaps[CEIL(TP_COEFF_PHASES_LEN, m_kCoeffLoadSize)]; // Previous user input coefficients with zero padding
    bool isUpdateRequired;                                        // Are coefficients sets equal?

    template <int cLen>
    struct ssr_params : public fir_params_defaults {
        using BTT_DATA = TT_DATA;
        using BTT_COEFF = TT_COEFF;
        static constexpr unsigned int BTP_FIR_LEN = TP_FIR_LEN;
        static constexpr unsigned int BTP_DECIMATE_FACTOR = TP_DECIMATE_FACTOR;
        static constexpr unsigned int BTP_CASC_LEN = cLen;
        static constexpr unsigned int BTP_FIR_RANGE_LEN = TP_DECIMATE_FACTOR;
        static constexpr unsigned int BTP_INPUT_WINDOW_VSIZE = TP_INPUT_WINDOW_VSIZE;
    };
    static constexpr int vectorRegistersNo = 4; // AIE-ML device provides 4 1024-bit vector registers

    static_assert(TP_INPUT_WINDOW_VSIZE % (TP_DECIMATE_FACTOR * m_kLanes) == 0,
                  "ERROR: TP_INPUT_WINDOW_VSIZE must be a multiple of TP_DECIMATE_FACTOR  and of the number of lanes "
                  "for the MUL/MAC intrinsic");
    static_assert(m_kPermuteSupport == 0 || (m_kDataRegVsize - m_kDataLoadVsize >= m_kDFDataRange),
                  "ERROR: TP_DECIMATION_FACTOR exceeded for this data/coeff type combination. Required input data "
                  "exceeds input vector's register offset address range.");
    static_assert(!(m_kArch == kArchStream && m_kDFX == kHighDF),
                  "ERROR: TP_DECIMATION_FACTOR exceeded for this data/coeff type combination. Required input data "
                  "exceeds input vector's register offset address range.");
    static_assert(!(m_kArch == kArchStream &&
                    (fir_decimate_asym_tl<ssr_params<TP_CASC_LEN> >::template fnCheckIfFits<TP_KERNEL_POSITION,
                                                                                            TP_CASC_LEN,
                                                                                            TP_DECIMATE_FACTOR,
                                                                                            TP_FIR_LEN,
                                                                                            TT_DATA,
                                                                                            TT_COEFF,
                                                                                            1>() == 0)),
                  "ERROR: TP_FIR_RANGE_LEN exceeds max supported range for this data/coeff type combination. Increase "
                  "TP_CASC_LEN to split the workload over more kernels.");
    static_assert(!(m_kArch == kArchStream &&
                    TP_INPUT_WINDOW_VSIZE % (TP_DECIMATE_FACTOR * m_kLanes * streamRptFactor) != 0),
                  "ERROR: TP_INPUT_WINDOW_VSIZE must be a multiple of (TP_DECIMATE_FACTOR * 8)  and of the number of "
                  "lanes for the streaming MUL/MAC intrinsic");
    static_assert(!(m_kArch == kArchStreamPhaseParallel &&
                    TP_INPUT_WINDOW_VSIZE % (TP_DECIMATE_FACTOR * m_kLanes * streamRptFactor) != 0),
                  "ERROR: TP_INPUT_WINDOW_VSIZE must be a multiple of (TP_DECIMATE_FACTOR * 4)  and of the number of "
                  "lanes for the streaming MUL/MAC intrinsic");
    static_assert(!(m_kArch == kArchStreamPhaseParallel && TP_DECIMATE_FACTOR > vectorRegistersNo),
                  "ERROR: Max Decimation factor exceeds available HW tile resource. Please decompose the desing into "
                  "multiple kernels.");

    // Int16 data and int32 coeffs data/coeff type combo is supported. Check high DF only when device supports permutes,
    // as otherwise, a Parallel architectre will be used.
    static_assert(!(m_kPermuteSupport == 1 && std::is_same<TT_DATA, int16>::value && m_kDFX == kHighDF),
                  "ERROR: TP_DECIMATION_FACTOR exceeded for this data type. Pre-select addressing is not available, "
                  "int16 data is only supported without pre-select.");

    // The coefficients array must include zero padding up to a multiple of the number of columns
    // the MAC intrinsic used to eliminate the accidental inclusion of terms beyond the FIR length.
    // Since this zero padding cannot be applied to the class-external coefficient array
    // the supplied taps are copied to an internal array, m_internalTaps, which can be padded.
    alignas(__ALIGN_BYTE_SIZE__) TT_COEFF
        m_internalTaps[CEIL(TP_FIR_RANGE_LEN, m_kCoeffLoadSize)]; // Filter taps/coefficients
    static constexpr int tapsArraySize = CEIL(CEIL(TP_FIR_RANGE_LEN, TP_DECIMATE_FACTOR) / TP_DECIMATE_FACTOR,
                                              (256 / 8 / sizeof(TT_COEFF))); // ceil'ed to 256-bits
    alignas(__ALIGN_BYTE_SIZE__) TT_COEFF m_internalTaps2[TP_DECIMATE_FACTOR][tapsArraySize];

    // Filter implementation functions
    void filterSelectArch(T_inputIF<TP_CASC_IN, TT_DATA, TP_DUAL_IP> inInterface,
                          T_outputIF<TP_CASC_OUT, TT_DATA> outInterface);
    void filterBasic(T_inputIF<TP_CASC_IN, TT_DATA, TP_DUAL_IP> inInterface,
                     T_outputIF<TP_CASC_OUT, TT_DATA> outInterface);
    void filterIncrStrobe(T_inputIF<TP_CASC_IN, TT_DATA, TP_DUAL_IP> inInterface,
                          T_outputIF<TP_CASC_OUT, TT_DATA> outInterface);
    void filterStream(T_inputIF<TP_CASC_IN, TT_DATA, TP_DUAL_IP> inInterface,
                      T_outputIF<TP_CASC_OUT, TT_DATA> outInterface);
    void filterPhaseParallel(T_inputIF<TP_CASC_IN, TT_DATA, TP_DUAL_IP> inInterface,
                             T_outputIF<TP_CASC_OUT, TT_DATA> outInterface);
    void filterStreamPhaseParallel(T_inputIF<TP_CASC_IN, TT_DATA, TP_DUAL_IP> inInterface,
                                   T_outputIF<TP_CASC_OUT, TT_DATA> outInterface);

   public:
    // Access function for AIE Synthesizer
    static unsigned int get_m_kArch() { return m_kArch; };

    // Constructor for reloadable coefficient designs
    // Calculates offsets required for coefficient reloads and m_kDecimateOffsets
    kernelFilterClass() : m_rawInTaps{}, m_internalTaps{} { setDecimateOffsets(); }

    // Constructor for static coefficient designs
    // Calculates m_kDecimateOffsets and writes coefficients to m_internalTaps
    kernelFilterClass(const TT_COEFF (&taps)[TP_FIR_LEN]) : m_internalTaps{} {
        setDecimateOffsets();
        // Loads taps/coefficients
        firReload(taps);
    }

    // setDecimateOffsets
    void setDecimateOffsets() {
        switch (TP_DECIMATE_FACTOR) {
            case 2:
                m_kDecimateOffsets = 0xECA86420;
                break; // No point in hi because range is exceeded.
            case 3:
                m_kDecimateOffsets = m_kDFX == kLowDF ? 0x9630 : m_kColumns == 1 ? 0x3030 : 0xA9764310;
                break; // 4x1 -cfloat exceeds 3-bit range; 8x2 int32xint16 exceed 4-bit range; 4-lane combos use kLowDF
            case 4:
                m_kDecimateOffsets =
                    m_kDFX == kLowDF ? 0xC840 : m_kColumns == 1 ? 0x4040 : m_kColumns == 2 ? 0xDC985410 : 0x76543210;
                break; //
            case 5:
                m_kDecimateOffsets =
                    m_kDFX == kLowDF ? 0xFA50 : m_kColumns == 1 ? 0x5050 : m_kColumns == 2 ? 0x65106510 : 0x87653210;
                break; //
            case 6:
                m_kDecimateOffsets =
                    m_kDFX == kLowDF ? 0x60 : m_kColumns == 1 ? 0x6060 : m_kColumns == 2 ? 0x76107610 : 0x98763210;
                break; //
            case 7:
                m_kDecimateOffsets =
                    m_kDFX == kLowDF ? 0x70 : m_kColumns == 1 ? 0x7070 : m_kColumns == 2 ? 0x87108710 : 0xA9873210;
                break; //
            default:
                break;
        }
    }; // setDecimateOffsets

    // Copys taps into m_internalTaps

    void firReload(const TT_COEFF* taps) {
        TT_COEFF* tapsPtr = (TT_COEFF*)taps;
        firReload(tapsPtr);
    }

    void firReload(TT_COEFF* taps) {
        if
            constexpr(m_kPermuteSupport == 1) {
                // Loads taps/coefficients
                for (int i = 0; i < TP_FIR_RANGE_LEN; i++) {
                    unsigned int tapsAddress =
                        TP_FIR_LEN - 1 -
                        fnFirRangeOffset<TP_FIR_LEN, TP_CASC_LEN, TP_KERNEL_POSITION, TP_DECIMATE_FACTOR>() - i;
                    m_internalTaps[i] = taps[tapsAddress];
                }
            }
        else {
            //  Scalar loads and stores are 32-bit granularity, using int16 directly causes read-modify-write which is
            //  inefficient.
            // TODO: Stick 2 int16 into an int32 to avoid it, for an example, look at fir_resampler kernel's firReload
            for (int phase = 0; phase < TP_DECIMATE_FACTOR; ++phase) {
                for (int i = 0; i < tapsArraySize; i++) { // ceiled to columns.
                    int tapIndex = i * TP_DECIMATE_FACTOR + TP_DECIMATE_FACTOR - 1 - phase;
                    int tapsAddress =
                        TP_FIR_LEN - 1 -
                        fnFirRangeOffset<TP_FIR_LEN, TP_CASC_LEN, TP_KERNEL_POSITION, TP_DECIMATE_FACTOR>() - tapIndex;
                    if (tapsAddress < 0 || tapIndex >= TP_FIR_RANGE_LEN) {
                        m_internalTaps2[phase][i] = nullElem<TT_COEFF>();
                    } else {
                        m_internalTaps2[phase][i] = taps[tapsAddress];
                    }
                }
            }
        }
    }

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
                coeffPhases * fnFirRangeOffset<TP_FIR_LEN, TP_CASC_LEN, TP_KERNEL_POSITION, TP_DECIMATE_FACTOR>() -
                tapsOffset;

            m_internalTaps[i] = taps[tapsAddress];
        }

        for (int phase = 0; phase < TP_DECIMATE_FACTOR; ++phase) {
            for (int i = 0; i < tapsArraySize; i++) { // ceiled to columns.
                int tapIndex = i * TP_DECIMATE_FACTOR + TP_DECIMATE_FACTOR - 1 - phase;
                int tapsOffset = tapIndex * coeffPhases + (coeffPhases - 1 - coeffPhase) + coeffPhaseOffset;
                int tapsAddress =
                    coeffPhasesLen - 1 -
                    coeffPhases * fnFirRangeOffset<TP_FIR_LEN, TP_CASC_LEN, TP_KERNEL_POSITION, TP_DECIMATE_FACTOR>() -
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
// Single kernel base specialization. Windowed. No cascade ports. Static coefficients
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_DECIMATE_FACTOR,
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
class fir_decimate_asym : public kernelFilterClass<TT_DATA,
                                                   TT_COEFF,
                                                   TP_FIR_LEN,
                                                   TP_DECIMATE_FACTOR,
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
    fir_decimate_asym(const TT_COEFF (&taps)[TP_FIR_LEN])
        : kernelFilterClass<TT_DATA,
                            TT_COEFF,
                            TP_FIR_LEN,
                            TP_DECIMATE_FACTOR,
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
                            TP_SAT>(taps) {}

    static void registerKernelClass() { REGISTER_FUNCTION(fir_decimate_asym::filter); }

    void filter(input_circular_buffer<TT_DATA,
                                      extents<inherited_extent>,
                                      margin<fnFirMargin<TP_FIR_LEN, TT_DATA>()> >& __restrict inWindow,
                output_circular_buffer<TT_DATA>& __restrict outWindow);
};

// Single kernel specialization. Windowed. No cascade ports. Static coefficients, dual output
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_DECIMATE_FACTOR,
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

class fir_decimate_asym<TT_DATA,
                        TT_COEFF,
                        TP_FIR_LEN,
                        TP_DECIMATE_FACTOR,
                        TP_SHIFT,
                        TP_RND,
                        TP_INPUT_WINDOW_VSIZE,
                        false,
                        false,
                        TP_FIR_RANGE_LEN,
                        0,
                        1,
                        0,
                        2,
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
                                                           TP_DECIMATE_FACTOR,
                                                           TP_SHIFT,
                                                           TP_RND,
                                                           TP_INPUT_WINDOW_VSIZE,
                                                           false,
                                                           false,
                                                           TP_FIR_RANGE_LEN,
                                                           0,
                                                           1,
                                                           0,
                                                           2,
                                                           DUAL_IP_SINGLE,
                                                           USE_WINDOW_API,
                                                           TP_MODIFY_MARGIN_OFFSET,
                                                           TP_COEFF_PHASE,
                                                           TP_COEFF_PHASE_OFFSET,
                                                           TP_COEFF_PHASES,
                                                           TP_COEFF_PHASES_LEN,
                                                           TP_SAT> {
   public:
    fir_decimate_asym(const TT_COEFF (&taps)[TP_FIR_LEN])
        : kernelFilterClass<TT_DATA,
                            TT_COEFF,
                            TP_FIR_LEN,
                            TP_DECIMATE_FACTOR,
                            TP_SHIFT,
                            TP_RND,
                            TP_INPUT_WINDOW_VSIZE,
                            false,
                            false,
                            TP_FIR_RANGE_LEN,
                            0,
                            1,
                            0,
                            2,
                            DUAL_IP_SINGLE,
                            USE_WINDOW_API,
                            TP_MODIFY_MARGIN_OFFSET,
                            TP_COEFF_PHASE,
                            TP_COEFF_PHASE_OFFSET,
                            TP_COEFF_PHASES,
                            TP_COEFF_PHASES_LEN,
                            TP_SAT>(taps) {}

    static void registerKernelClass() { REGISTER_FUNCTION(fir_decimate_asym::filter); }

    void filter(input_circular_buffer<TT_DATA,
                                      extents<inherited_extent>,
                                      margin<fnFirMargin<TP_FIR_LEN, TT_DATA>()> >& __restrict inWindow,
                output_circular_buffer<TT_DATA>& __restrict outWindow,
                output_circular_buffer<TT_DATA>& __restrict outWindow2);
};

//-----------------------------------------------------------------------------------------------------
// Single kernel specialization. Windowed. No cascade ports, with reload coefficients, single output
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_DECIMATE_FACTOR,
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

class fir_decimate_asym<TT_DATA,
                        TT_COEFF,
                        TP_FIR_LEN,
                        TP_DECIMATE_FACTOR,
                        TP_SHIFT,
                        TP_RND,
                        TP_INPUT_WINDOW_VSIZE,
                        CASC_IN_FALSE,
                        CASC_OUT_FALSE,
                        TP_FIR_RANGE_LEN,
                        TP_KERNEL_POSITION,
                        TP_CASC_LEN,
                        USE_COEFF_RELOAD_TRUE,
                        1,
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
                                                           TP_DECIMATE_FACTOR,
                                                           TP_SHIFT,
                                                           TP_RND,
                                                           TP_INPUT_WINDOW_VSIZE,
                                                           CASC_IN_FALSE,
                                                           CASC_OUT_FALSE,
                                                           TP_FIR_RANGE_LEN,
                                                           TP_KERNEL_POSITION,
                                                           TP_CASC_LEN,
                                                           USE_COEFF_RELOAD_TRUE,
                                                           1,
                                                           DUAL_IP_SINGLE,
                                                           USE_WINDOW_API,
                                                           TP_MODIFY_MARGIN_OFFSET,
                                                           TP_COEFF_PHASE,
                                                           TP_COEFF_PHASE_OFFSET,
                                                           TP_COEFF_PHASES,
                                                           TP_COEFF_PHASES_LEN,
                                                           TP_SAT> {
   public:
    fir_decimate_asym()
        : kernelFilterClass<TT_DATA,
                            TT_COEFF,
                            TP_FIR_LEN,
                            TP_DECIMATE_FACTOR,
                            TP_SHIFT,
                            TP_RND,
                            TP_INPUT_WINDOW_VSIZE,
                            CASC_IN_FALSE,
                            CASC_OUT_FALSE,
                            TP_FIR_RANGE_LEN,
                            TP_KERNEL_POSITION,
                            TP_CASC_LEN,
                            USE_COEFF_RELOAD_TRUE,
                            1,
                            DUAL_IP_SINGLE,
                            USE_WINDOW_API,
                            TP_MODIFY_MARGIN_OFFSET,
                            TP_COEFF_PHASE,
                            TP_COEFF_PHASE_OFFSET,
                            TP_COEFF_PHASES,
                            TP_COEFF_PHASES_LEN,
                            TP_SAT>() {}
    fir_decimate_asym(const TT_COEFF (&taps)[TP_FIR_LEN])
        : kernelFilterClass<TT_DATA,
                            TT_COEFF,
                            TP_FIR_LEN,
                            TP_DECIMATE_FACTOR,
                            TP_SHIFT,
                            TP_RND,
                            TP_INPUT_WINDOW_VSIZE,
                            CASC_IN_FALSE,
                            CASC_OUT_FALSE,
                            TP_FIR_RANGE_LEN,
                            TP_KERNEL_POSITION,
                            TP_CASC_LEN,
                            USE_COEFF_RELOAD_TRUE,
                            1,
                            DUAL_IP_SINGLE,
                            USE_WINDOW_API,
                            TP_MODIFY_MARGIN_OFFSET,
                            TP_COEFF_PHASE,
                            TP_COEFF_PHASE_OFFSET,
                            TP_COEFF_PHASES,
                            TP_COEFF_PHASES_LEN,
                            TP_SAT>(taps) {}

    static void registerKernelClass() { REGISTER_FUNCTION(fir_decimate_asym::filter); }

    void filter(input_circular_buffer<TT_DATA,
                                      extents<inherited_extent>,
                                      margin<fnFirMargin<TP_FIR_LEN, TT_DATA>()> >& __restrict inWindow,
                output_circular_buffer<TT_DATA>& __restrict outWindow,
                const TT_COEFF (&inTaps)[TP_COEFF_PHASES_LEN]);
};

// Single kernel specialization. No cascade ports, Windowed. with reload coefficients, dual output
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_DECIMATE_FACTOR,
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

class fir_decimate_asym<TT_DATA,
                        TT_COEFF,
                        TP_FIR_LEN,
                        TP_DECIMATE_FACTOR,
                        TP_SHIFT,
                        TP_RND,
                        TP_INPUT_WINDOW_VSIZE,
                        CASC_IN_FALSE,
                        CASC_OUT_FALSE,
                        TP_FIR_RANGE_LEN,
                        TP_KERNEL_POSITION,
                        TP_CASC_LEN,
                        USE_COEFF_RELOAD_TRUE,
                        2,
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
                                                           TP_DECIMATE_FACTOR,
                                                           TP_SHIFT,
                                                           TP_RND,
                                                           TP_INPUT_WINDOW_VSIZE,
                                                           CASC_IN_FALSE,
                                                           CASC_OUT_FALSE,
                                                           TP_FIR_RANGE_LEN,
                                                           TP_KERNEL_POSITION,
                                                           TP_CASC_LEN,
                                                           USE_COEFF_RELOAD_TRUE,
                                                           2,
                                                           DUAL_IP_SINGLE,
                                                           USE_WINDOW_API,
                                                           TP_MODIFY_MARGIN_OFFSET,
                                                           TP_COEFF_PHASE,
                                                           TP_COEFF_PHASE_OFFSET,
                                                           TP_COEFF_PHASES,
                                                           TP_COEFF_PHASES_LEN,
                                                           TP_SAT> {
   public:
    fir_decimate_asym()
        : kernelFilterClass<TT_DATA,
                            TT_COEFF,
                            TP_FIR_LEN,
                            TP_DECIMATE_FACTOR,
                            TP_SHIFT,
                            TP_RND,
                            TP_INPUT_WINDOW_VSIZE,
                            CASC_IN_FALSE,
                            CASC_OUT_FALSE,
                            TP_FIR_RANGE_LEN,
                            TP_KERNEL_POSITION,
                            TP_CASC_LEN,
                            USE_COEFF_RELOAD_TRUE,
                            2,
                            DUAL_IP_SINGLE,
                            USE_WINDOW_API,
                            TP_MODIFY_MARGIN_OFFSET,
                            TP_COEFF_PHASE,
                            TP_COEFF_PHASE_OFFSET,
                            TP_COEFF_PHASES,
                            TP_COEFF_PHASES_LEN,
                            TP_SAT>() {}
    fir_decimate_asym(const TT_COEFF (&taps)[TP_FIR_LEN])
        : kernelFilterClass<TT_DATA,
                            TT_COEFF,
                            TP_FIR_LEN,
                            TP_DECIMATE_FACTOR,
                            TP_SHIFT,
                            TP_RND,
                            TP_INPUT_WINDOW_VSIZE,
                            CASC_IN_FALSE,
                            CASC_OUT_FALSE,
                            TP_FIR_RANGE_LEN,
                            TP_KERNEL_POSITION,
                            TP_CASC_LEN,
                            USE_COEFF_RELOAD_TRUE,
                            2,
                            DUAL_IP_SINGLE,
                            USE_WINDOW_API,
                            TP_MODIFY_MARGIN_OFFSET,
                            TP_COEFF_PHASE,
                            TP_COEFF_PHASE_OFFSET,
                            TP_COEFF_PHASES,
                            TP_COEFF_PHASES_LEN,
                            TP_SAT>(taps) {}

    static void registerKernelClass() { REGISTER_FUNCTION(fir_decimate_asym::filter); }

    void filter(input_circular_buffer<TT_DATA,
                                      extents<inherited_extent>,
                                      margin<fnFirMargin<TP_FIR_LEN, TT_DATA>()> >& __restrict inWindow,
                output_circular_buffer<TT_DATA>& __restrict outWindow,
                output_circular_buffer<TT_DATA>& __restrict outWindow2,
                const TT_COEFF (&inTaps)[TP_COEFF_PHASES_LEN]);
};

//-----------------------------------------------------------------------------------------------------
// Partially specialized classes for cascaded interface (final kernel in cascade), Windowed. no reload, single output
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_DECIMATE_FACTOR,
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

class fir_decimate_asym<TT_DATA,
                        TT_COEFF,
                        TP_FIR_LEN,
                        TP_DECIMATE_FACTOR,
                        TP_SHIFT,
                        TP_RND,
                        TP_INPUT_WINDOW_VSIZE,
                        CASC_IN_TRUE,
                        CASC_OUT_FALSE,
                        TP_FIR_RANGE_LEN,
                        TP_KERNEL_POSITION,
                        TP_CASC_LEN,
                        USE_COEFF_RELOAD_FALSE,
                        1,
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
                                                           TP_DECIMATE_FACTOR,
                                                           TP_SHIFT,
                                                           TP_RND,
                                                           TP_INPUT_WINDOW_VSIZE,
                                                           CASC_IN_TRUE,
                                                           CASC_OUT_FALSE,
                                                           TP_FIR_RANGE_LEN,
                                                           TP_KERNEL_POSITION,
                                                           TP_CASC_LEN,
                                                           USE_COEFF_RELOAD_FALSE,
                                                           1,
                                                           DUAL_IP_SINGLE,
                                                           USE_WINDOW_API,
                                                           TP_MODIFY_MARGIN_OFFSET,
                                                           TP_COEFF_PHASE,
                                                           TP_COEFF_PHASE_OFFSET,
                                                           TP_COEFF_PHASES,
                                                           TP_COEFF_PHASES_LEN,
                                                           TP_SAT> {
   public:
    fir_decimate_asym(const TT_COEFF (&taps)[TP_FIR_LEN])
        : kernelFilterClass<TT_DATA,
                            TT_COEFF,
                            TP_FIR_LEN,
                            TP_DECIMATE_FACTOR,
                            TP_SHIFT,
                            TP_RND,
                            TP_INPUT_WINDOW_VSIZE,
                            CASC_IN_TRUE,
                            CASC_OUT_FALSE,
                            TP_FIR_RANGE_LEN,
                            TP_KERNEL_POSITION,
                            TP_CASC_LEN,
                            USE_COEFF_RELOAD_FALSE,
                            1,
                            DUAL_IP_SINGLE,
                            USE_WINDOW_API,
                            TP_MODIFY_MARGIN_OFFSET,
                            TP_COEFF_PHASE,
                            TP_COEFF_PHASE_OFFSET,
                            TP_COEFF_PHASES,
                            TP_COEFF_PHASES_LEN,
                            TP_SAT>(taps) {}

    static void registerKernelClass() { REGISTER_FUNCTION(fir_decimate_asym::filter); }

    void filter(input_async_buffer<TT_DATA, extents<inherited_extent> >& inWindow,
                input_cascade_cacc* inCascade,
                output_circular_buffer<TT_DATA>& __restrict outWindow);
};

// Partially specialized classes for cascaded interface (final kernel in cascade), no reload, dual output
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_DECIMATE_FACTOR,
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

class fir_decimate_asym<TT_DATA,
                        TT_COEFF,
                        TP_FIR_LEN,
                        TP_DECIMATE_FACTOR,
                        TP_SHIFT,
                        TP_RND,
                        TP_INPUT_WINDOW_VSIZE,
                        CASC_IN_TRUE,
                        CASC_OUT_FALSE,
                        TP_FIR_RANGE_LEN,
                        TP_KERNEL_POSITION,
                        TP_CASC_LEN,
                        USE_COEFF_RELOAD_FALSE,
                        2,
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
                                                           TP_DECIMATE_FACTOR,
                                                           TP_SHIFT,
                                                           TP_RND,
                                                           TP_INPUT_WINDOW_VSIZE,
                                                           CASC_IN_TRUE,
                                                           CASC_OUT_FALSE,
                                                           TP_FIR_RANGE_LEN,
                                                           TP_KERNEL_POSITION,
                                                           TP_CASC_LEN,
                                                           USE_COEFF_RELOAD_FALSE,
                                                           2,
                                                           DUAL_IP_SINGLE,
                                                           USE_WINDOW_API,
                                                           TP_MODIFY_MARGIN_OFFSET,
                                                           TP_COEFF_PHASE,
                                                           TP_COEFF_PHASE_OFFSET,
                                                           TP_COEFF_PHASES,
                                                           TP_COEFF_PHASES_LEN,
                                                           TP_SAT> {
   public:
    fir_decimate_asym(const TT_COEFF (&taps)[TP_FIR_LEN])
        : kernelFilterClass<TT_DATA,
                            TT_COEFF,
                            TP_FIR_LEN,
                            TP_DECIMATE_FACTOR,
                            TP_SHIFT,
                            TP_RND,
                            TP_INPUT_WINDOW_VSIZE,
                            CASC_IN_TRUE,
                            CASC_OUT_FALSE,
                            TP_FIR_RANGE_LEN,
                            TP_KERNEL_POSITION,
                            TP_CASC_LEN,
                            USE_COEFF_RELOAD_FALSE,
                            2,
                            DUAL_IP_SINGLE,
                            USE_WINDOW_API,
                            TP_MODIFY_MARGIN_OFFSET,
                            TP_COEFF_PHASE,
                            TP_COEFF_PHASE_OFFSET,
                            TP_COEFF_PHASES,
                            TP_COEFF_PHASES_LEN,
                            TP_SAT>(taps) {}

    static void registerKernelClass() { REGISTER_FUNCTION(fir_decimate_asym::filter); }

    void filter(input_async_buffer<TT_DATA, extents<inherited_extent> >& inWindow,
                input_cascade_cacc* inCascade,
                output_circular_buffer<TT_DATA>& __restrict outWindow,
                output_circular_buffer<TT_DATA>& __restrict outWindow2);
};

//-----------------------------------------------------------------------------------------------------
// Partially specialized classes for cascaded interface (final kernel in cascade), Windowed. with reload, single output
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_DECIMATE_FACTOR,
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

class fir_decimate_asym<TT_DATA,
                        TT_COEFF,
                        TP_FIR_LEN,
                        TP_DECIMATE_FACTOR,
                        TP_SHIFT,
                        TP_RND,
                        TP_INPUT_WINDOW_VSIZE,
                        CASC_IN_TRUE,
                        CASC_OUT_FALSE,
                        TP_FIR_RANGE_LEN,
                        TP_KERNEL_POSITION,
                        TP_CASC_LEN,
                        USE_COEFF_RELOAD_TRUE,
                        1,
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
                                                           TP_DECIMATE_FACTOR,
                                                           TP_SHIFT,
                                                           TP_RND,
                                                           TP_INPUT_WINDOW_VSIZE,
                                                           CASC_IN_TRUE,
                                                           CASC_OUT_FALSE,
                                                           TP_FIR_RANGE_LEN,
                                                           TP_KERNEL_POSITION,
                                                           TP_CASC_LEN,
                                                           USE_COEFF_RELOAD_TRUE,
                                                           1,
                                                           DUAL_IP_SINGLE,
                                                           USE_WINDOW_API,
                                                           TP_MODIFY_MARGIN_OFFSET,
                                                           TP_COEFF_PHASE,
                                                           TP_COEFF_PHASE_OFFSET,
                                                           TP_COEFF_PHASES,
                                                           TP_COEFF_PHASES_LEN,
                                                           TP_SAT> {
   public:
    fir_decimate_asym()
        : kernelFilterClass<TT_DATA,
                            TT_COEFF,
                            TP_FIR_LEN,
                            TP_DECIMATE_FACTOR,
                            TP_SHIFT,
                            TP_RND,
                            TP_INPUT_WINDOW_VSIZE,
                            CASC_IN_TRUE,
                            CASC_OUT_FALSE,
                            TP_FIR_RANGE_LEN,
                            TP_KERNEL_POSITION,
                            TP_CASC_LEN,
                            USE_COEFF_RELOAD_TRUE,
                            1,
                            DUAL_IP_SINGLE,
                            USE_WINDOW_API,
                            TP_MODIFY_MARGIN_OFFSET,
                            TP_COEFF_PHASE,
                            TP_COEFF_PHASE_OFFSET,
                            TP_COEFF_PHASES,
                            TP_COEFF_PHASES_LEN,
                            TP_SAT>() {}
    fir_decimate_asym(const TT_COEFF (&taps)[TP_FIR_LEN])
        : kernelFilterClass<TT_DATA,
                            TT_COEFF,
                            TP_FIR_LEN,
                            TP_DECIMATE_FACTOR,
                            TP_SHIFT,
                            TP_RND,
                            TP_INPUT_WINDOW_VSIZE,
                            CASC_IN_TRUE,
                            CASC_OUT_FALSE,
                            TP_FIR_RANGE_LEN,
                            TP_KERNEL_POSITION,
                            TP_CASC_LEN,
                            USE_COEFF_RELOAD_TRUE,
                            1,
                            DUAL_IP_SINGLE,
                            USE_WINDOW_API,
                            TP_MODIFY_MARGIN_OFFSET,
                            TP_COEFF_PHASE,
                            TP_COEFF_PHASE_OFFSET,
                            TP_COEFF_PHASES,
                            TP_COEFF_PHASES_LEN,
                            TP_SAT>(taps) {}

    static void registerKernelClass() { REGISTER_FUNCTION(fir_decimate_asym::filter); }

    void filter(input_async_buffer<TT_DATA, extents<inherited_extent> >& inWindow,
                input_cascade_cacc* inCascade,
                output_circular_buffer<TT_DATA>& __restrict outWindow);
};

// Partially specialized classes for cascaded interface (final kernel in cascade), Windowed. with reload, dual output
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_DECIMATE_FACTOR,
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

class fir_decimate_asym<TT_DATA,
                        TT_COEFF,
                        TP_FIR_LEN,
                        TP_DECIMATE_FACTOR,
                        TP_SHIFT,
                        TP_RND,
                        TP_INPUT_WINDOW_VSIZE,
                        CASC_IN_TRUE,
                        CASC_OUT_FALSE,
                        TP_FIR_RANGE_LEN,
                        TP_KERNEL_POSITION,
                        TP_CASC_LEN,
                        USE_COEFF_RELOAD_TRUE,
                        2,
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
                                                           TP_DECIMATE_FACTOR,
                                                           TP_SHIFT,
                                                           TP_RND,
                                                           TP_INPUT_WINDOW_VSIZE,
                                                           CASC_IN_TRUE,
                                                           CASC_OUT_FALSE,
                                                           TP_FIR_RANGE_LEN,
                                                           TP_KERNEL_POSITION,
                                                           TP_CASC_LEN,
                                                           USE_COEFF_RELOAD_TRUE,
                                                           2,
                                                           DUAL_IP_SINGLE,
                                                           USE_WINDOW_API,
                                                           TP_MODIFY_MARGIN_OFFSET,
                                                           TP_COEFF_PHASE,
                                                           TP_COEFF_PHASE_OFFSET,
                                                           TP_COEFF_PHASES,
                                                           TP_COEFF_PHASES_LEN,
                                                           TP_SAT> {
   public:
    fir_decimate_asym()
        : kernelFilterClass<TT_DATA,
                            TT_COEFF,
                            TP_FIR_LEN,
                            TP_DECIMATE_FACTOR,
                            TP_SHIFT,
                            TP_RND,
                            TP_INPUT_WINDOW_VSIZE,
                            CASC_IN_TRUE,
                            CASC_OUT_FALSE,
                            TP_FIR_RANGE_LEN,
                            TP_KERNEL_POSITION,
                            TP_CASC_LEN,
                            USE_COEFF_RELOAD_TRUE,
                            2,
                            DUAL_IP_SINGLE,
                            USE_WINDOW_API,
                            TP_MODIFY_MARGIN_OFFSET,
                            TP_COEFF_PHASE,
                            TP_COEFF_PHASE_OFFSET,
                            TP_COEFF_PHASES,
                            TP_COEFF_PHASES_LEN,
                            TP_SAT>() {}
    fir_decimate_asym(const TT_COEFF (&taps)[TP_FIR_LEN])
        : kernelFilterClass<TT_DATA,
                            TT_COEFF,
                            TP_FIR_LEN,
                            TP_DECIMATE_FACTOR,
                            TP_SHIFT,
                            TP_RND,
                            TP_INPUT_WINDOW_VSIZE,
                            CASC_IN_TRUE,
                            CASC_OUT_FALSE,
                            TP_FIR_RANGE_LEN,
                            TP_KERNEL_POSITION,
                            TP_CASC_LEN,
                            USE_COEFF_RELOAD_TRUE,
                            2,
                            DUAL_IP_SINGLE,
                            USE_WINDOW_API,
                            TP_MODIFY_MARGIN_OFFSET,
                            TP_COEFF_PHASE,
                            TP_COEFF_PHASE_OFFSET,
                            TP_COEFF_PHASES,
                            TP_COEFF_PHASES_LEN,
                            TP_SAT>(taps) {}

    static void registerKernelClass() { REGISTER_FUNCTION(fir_decimate_asym::filter); }

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
          unsigned int TP_DECIMATE_FACTOR,
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

class fir_decimate_asym<TT_DATA,
                        TT_COEFF,
                        TP_FIR_LEN,
                        TP_DECIMATE_FACTOR,
                        TP_SHIFT,
                        TP_RND,
                        TP_INPUT_WINDOW_VSIZE,
                        CASC_IN_FALSE,
                        CASC_OUT_TRUE,
                        TP_FIR_RANGE_LEN,
                        TP_KERNEL_POSITION,
                        TP_CASC_LEN,
                        USE_COEFF_RELOAD_FALSE,
                        1,
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
                                                           TP_DECIMATE_FACTOR,
                                                           TP_SHIFT,
                                                           TP_RND,
                                                           TP_INPUT_WINDOW_VSIZE,
                                                           CASC_IN_FALSE,
                                                           CASC_OUT_TRUE,
                                                           TP_FIR_RANGE_LEN,
                                                           TP_KERNEL_POSITION,
                                                           TP_CASC_LEN,
                                                           USE_COEFF_RELOAD_FALSE,
                                                           1,
                                                           DUAL_IP_SINGLE,
                                                           USE_WINDOW_API,
                                                           TP_MODIFY_MARGIN_OFFSET,
                                                           TP_COEFF_PHASE,
                                                           TP_COEFF_PHASE_OFFSET,
                                                           TP_COEFF_PHASES,
                                                           TP_COEFF_PHASES_LEN,
                                                           TP_SAT> {
   public:
    fir_decimate_asym(const TT_COEFF (&taps)[TP_FIR_LEN])
        : kernelFilterClass<TT_DATA,
                            TT_COEFF,
                            TP_FIR_LEN,
                            TP_DECIMATE_FACTOR,
                            TP_SHIFT,
                            TP_RND,
                            TP_INPUT_WINDOW_VSIZE,
                            CASC_IN_FALSE,
                            CASC_OUT_TRUE,
                            TP_FIR_RANGE_LEN,
                            TP_KERNEL_POSITION,
                            TP_CASC_LEN,
                            USE_COEFF_RELOAD_FALSE,
                            1,
                            DUAL_IP_SINGLE,
                            USE_WINDOW_API,
                            TP_MODIFY_MARGIN_OFFSET,
                            TP_COEFF_PHASE,
                            TP_COEFF_PHASE_OFFSET,
                            TP_COEFF_PHASES,
                            TP_COEFF_PHASES_LEN,
                            TP_SAT>(taps) {}

    static void registerKernelClass() { REGISTER_FUNCTION(fir_decimate_asym::filter); }

    void filter(input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<fnFirMargin<TP_FIR_LEN, TT_DATA>()> >&
                    inWindow,
                output_cascade_cacc* outCascade,
                output_async_buffer<TT_DATA>& broadcastWindow);
};

//-----------------------------------------------------------------------------------------------------
// Partially specialized classes for cascaded interface (First kernel in cascade), Windowed. with reload
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_DECIMATE_FACTOR,
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

class fir_decimate_asym<TT_DATA,
                        TT_COEFF,
                        TP_FIR_LEN,
                        TP_DECIMATE_FACTOR,
                        TP_SHIFT,
                        TP_RND,
                        TP_INPUT_WINDOW_VSIZE,
                        CASC_IN_FALSE,
                        CASC_OUT_TRUE,
                        TP_FIR_RANGE_LEN,
                        TP_KERNEL_POSITION,
                        TP_CASC_LEN,
                        USE_COEFF_RELOAD_TRUE,
                        1,
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
                                                           TP_DECIMATE_FACTOR,
                                                           TP_SHIFT,
                                                           TP_RND,
                                                           TP_INPUT_WINDOW_VSIZE,
                                                           CASC_IN_FALSE,
                                                           CASC_OUT_TRUE,
                                                           TP_FIR_RANGE_LEN,
                                                           TP_KERNEL_POSITION,
                                                           TP_CASC_LEN,
                                                           USE_COEFF_RELOAD_TRUE,
                                                           1,
                                                           DUAL_IP_SINGLE,
                                                           USE_WINDOW_API,
                                                           TP_MODIFY_MARGIN_OFFSET,
                                                           TP_COEFF_PHASE,
                                                           TP_COEFF_PHASE_OFFSET,
                                                           TP_COEFF_PHASES,
                                                           TP_COEFF_PHASES_LEN,
                                                           TP_SAT> {
   public:
    fir_decimate_asym()
        : kernelFilterClass<TT_DATA,
                            TT_COEFF,
                            TP_FIR_LEN,
                            TP_DECIMATE_FACTOR,
                            TP_SHIFT,
                            TP_RND,
                            TP_INPUT_WINDOW_VSIZE,
                            CASC_IN_FALSE,
                            CASC_OUT_TRUE,
                            TP_FIR_RANGE_LEN,
                            TP_KERNEL_POSITION,
                            TP_CASC_LEN,
                            USE_COEFF_RELOAD_TRUE,
                            1,
                            DUAL_IP_SINGLE,
                            USE_WINDOW_API,
                            TP_MODIFY_MARGIN_OFFSET,
                            TP_COEFF_PHASE,
                            TP_COEFF_PHASE_OFFSET,
                            TP_COEFF_PHASES,
                            TP_COEFF_PHASES_LEN,
                            TP_SAT>() {}
    fir_decimate_asym(const TT_COEFF (&taps)[TP_FIR_LEN])
        : kernelFilterClass<TT_DATA,
                            TT_COEFF,
                            TP_FIR_LEN,
                            TP_DECIMATE_FACTOR,
                            TP_SHIFT,
                            TP_RND,
                            TP_INPUT_WINDOW_VSIZE,
                            CASC_IN_FALSE,
                            CASC_OUT_TRUE,
                            TP_FIR_RANGE_LEN,
                            TP_KERNEL_POSITION,
                            TP_CASC_LEN,
                            USE_COEFF_RELOAD_TRUE,
                            1,
                            DUAL_IP_SINGLE,
                            USE_WINDOW_API,
                            TP_MODIFY_MARGIN_OFFSET,
                            TP_COEFF_PHASE,
                            TP_COEFF_PHASE_OFFSET,
                            TP_COEFF_PHASES,
                            TP_COEFF_PHASES_LEN,
                            TP_SAT>(taps) {}

    static void registerKernelClass() { REGISTER_FUNCTION(fir_decimate_asym::filter); }

    void filter(input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<fnFirMargin<TP_FIR_LEN, TT_DATA>()> >&
                    inWindow,
                output_cascade_cacc* outCascade,
                output_async_buffer<TT_DATA>& broadcastWindow,
                const TT_COEFF (&inTaps)[TP_COEFF_PHASES_LEN]);
};

//-----------------------------------------------------------------------------------------------------
// Partially specialized classes for cascaded interface (middle kernels in cascade), Windowed. no reload
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_DECIMATE_FACTOR,
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

class fir_decimate_asym<TT_DATA,
                        TT_COEFF,
                        TP_FIR_LEN,
                        TP_DECIMATE_FACTOR,
                        TP_SHIFT,
                        TP_RND,
                        TP_INPUT_WINDOW_VSIZE,
                        CASC_IN_TRUE,
                        CASC_OUT_TRUE,
                        TP_FIR_RANGE_LEN,
                        TP_KERNEL_POSITION,
                        TP_CASC_LEN,
                        USE_COEFF_RELOAD_FALSE,
                        1,
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
                                                           TP_DECIMATE_FACTOR,
                                                           TP_SHIFT,
                                                           TP_RND,
                                                           TP_INPUT_WINDOW_VSIZE,
                                                           CASC_IN_TRUE,
                                                           CASC_OUT_TRUE,
                                                           TP_FIR_RANGE_LEN,
                                                           TP_KERNEL_POSITION,
                                                           TP_CASC_LEN,
                                                           USE_COEFF_RELOAD_FALSE,
                                                           1,
                                                           DUAL_IP_SINGLE,
                                                           USE_WINDOW_API,
                                                           TP_MODIFY_MARGIN_OFFSET,
                                                           TP_COEFF_PHASE,
                                                           TP_COEFF_PHASE_OFFSET,
                                                           TP_COEFF_PHASES,
                                                           TP_COEFF_PHASES_LEN,
                                                           TP_SAT> {
   public:
    fir_decimate_asym(const TT_COEFF (&taps)[TP_FIR_LEN])
        : kernelFilterClass<TT_DATA,
                            TT_COEFF,
                            TP_FIR_LEN,
                            TP_DECIMATE_FACTOR,
                            TP_SHIFT,
                            TP_RND,
                            TP_INPUT_WINDOW_VSIZE,
                            CASC_IN_TRUE,
                            CASC_OUT_TRUE,
                            TP_FIR_RANGE_LEN,
                            TP_KERNEL_POSITION,
                            TP_CASC_LEN,
                            USE_COEFF_RELOAD_FALSE,
                            1,
                            DUAL_IP_SINGLE,
                            USE_WINDOW_API,
                            TP_MODIFY_MARGIN_OFFSET,
                            TP_COEFF_PHASE,
                            TP_COEFF_PHASE_OFFSET,
                            TP_COEFF_PHASES,
                            TP_COEFF_PHASES_LEN,
                            TP_SAT>(taps) {}

    static void registerKernelClass() { REGISTER_FUNCTION(fir_decimate_asym::filter); }

    void filter(input_async_buffer<TT_DATA>& inWindow,
                input_cascade_cacc* inCascade,
                output_cascade_cacc* outCascade,
                output_async_buffer<TT_DATA>& broadcastWindow);
};

//-----------------------------------------------------------------------------------------------------
// Partially specialized classes for cascaded interface (middle kernels in cascade), with reload
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_DECIMATE_FACTOR,
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

class fir_decimate_asym<TT_DATA,
                        TT_COEFF,
                        TP_FIR_LEN,
                        TP_DECIMATE_FACTOR,
                        TP_SHIFT,
                        TP_RND,
                        TP_INPUT_WINDOW_VSIZE,
                        CASC_IN_TRUE,
                        CASC_OUT_TRUE,
                        TP_FIR_RANGE_LEN,
                        TP_KERNEL_POSITION,
                        TP_CASC_LEN,
                        USE_COEFF_RELOAD_TRUE,
                        1,
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
                                                           TP_DECIMATE_FACTOR,
                                                           TP_SHIFT,
                                                           TP_RND,
                                                           TP_INPUT_WINDOW_VSIZE,
                                                           CASC_IN_TRUE,
                                                           CASC_OUT_TRUE,
                                                           TP_FIR_RANGE_LEN,
                                                           TP_KERNEL_POSITION,
                                                           TP_CASC_LEN,
                                                           USE_COEFF_RELOAD_TRUE,
                                                           1,
                                                           DUAL_IP_SINGLE,
                                                           USE_WINDOW_API,
                                                           TP_MODIFY_MARGIN_OFFSET,
                                                           TP_COEFF_PHASE,
                                                           TP_COEFF_PHASE_OFFSET,
                                                           TP_COEFF_PHASES,
                                                           TP_COEFF_PHASES_LEN,
                                                           TP_SAT> {
   public:
    fir_decimate_asym()
        : kernelFilterClass<TT_DATA,
                            TT_COEFF,
                            TP_FIR_LEN,
                            TP_DECIMATE_FACTOR,
                            TP_SHIFT,
                            TP_RND,
                            TP_INPUT_WINDOW_VSIZE,
                            CASC_IN_TRUE,
                            CASC_OUT_TRUE,
                            TP_FIR_RANGE_LEN,
                            TP_KERNEL_POSITION,
                            TP_CASC_LEN,
                            USE_COEFF_RELOAD_TRUE,
                            1,
                            DUAL_IP_SINGLE,
                            USE_WINDOW_API,
                            TP_MODIFY_MARGIN_OFFSET,
                            TP_COEFF_PHASE,
                            TP_COEFF_PHASE_OFFSET,
                            TP_COEFF_PHASES,
                            TP_COEFF_PHASES_LEN,
                            TP_SAT>() {}
    fir_decimate_asym(const TT_COEFF (&taps)[TP_FIR_LEN])
        : kernelFilterClass<TT_DATA,
                            TT_COEFF,
                            TP_FIR_LEN,
                            TP_DECIMATE_FACTOR,
                            TP_SHIFT,
                            TP_RND,
                            TP_INPUT_WINDOW_VSIZE,
                            CASC_IN_TRUE,
                            CASC_OUT_TRUE,
                            TP_FIR_RANGE_LEN,
                            TP_KERNEL_POSITION,
                            TP_CASC_LEN,
                            USE_COEFF_RELOAD_TRUE,
                            1,
                            DUAL_IP_SINGLE,
                            USE_WINDOW_API,
                            TP_MODIFY_MARGIN_OFFSET,
                            TP_COEFF_PHASE,
                            TP_COEFF_PHASE_OFFSET,
                            TP_COEFF_PHASES,
                            TP_COEFF_PHASES_LEN,
                            TP_SAT>(taps) {}

    static void registerKernelClass() { REGISTER_FUNCTION(fir_decimate_asym::filter); }

    void filter(input_async_buffer<TT_DATA>& inWindow,
                input_cascade_cacc* inCascade,
                output_cascade_cacc* outCascade,
                output_async_buffer<TT_DATA>& broadcastWindow);
};

// ----------------------------------------------------------------------------
// ---------------------------------- STREAM ----------------------------------
// ----------------------------------------------------------------------------

//-----------------------------------------------------------------------------------------------------
// Single kernel base specialization. No cascade ports. Streaming. Static coefficients

// Single kernel specialization. No cascade ports. Static coefficients, dual output
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_DECIMATE_FACTOR,
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

class fir_decimate_asym<TT_DATA,
                        TT_COEFF,
                        TP_FIR_LEN,
                        TP_DECIMATE_FACTOR,
                        TP_SHIFT,
                        TP_RND,
                        TP_INPUT_WINDOW_VSIZE,
                        false,
                        false,
                        TP_FIR_RANGE_LEN,
                        0,
                        1,
                        0,
                        1,
                        DUAL_IP_SINGLE,
                        USE_STREAM_API,
                        TP_MODIFY_MARGIN_OFFSET,
                        TP_COEFF_PHASE,
                        TP_COEFF_PHASE_OFFSET,
                        TP_COEFF_PHASES,
                        TP_COEFF_PHASES_LEN,
                        TP_SAT> : public kernelFilterClass<TT_DATA,
                                                           TT_COEFF,
                                                           TP_FIR_LEN,
                                                           TP_DECIMATE_FACTOR,
                                                           TP_SHIFT,
                                                           TP_RND,
                                                           TP_INPUT_WINDOW_VSIZE,
                                                           false,
                                                           false,
                                                           TP_FIR_RANGE_LEN,
                                                           0,
                                                           1,
                                                           0,
                                                           1,
                                                           DUAL_IP_SINGLE,
                                                           USE_STREAM_API,
                                                           TP_MODIFY_MARGIN_OFFSET,
                                                           TP_COEFF_PHASE,
                                                           TP_COEFF_PHASE_OFFSET,
                                                           TP_COEFF_PHASES,
                                                           TP_COEFF_PHASES_LEN,
                                                           TP_SAT> {
   public:
    fir_decimate_asym(const TT_COEFF (&taps)[TP_FIR_LEN])
        : kernelFilterClass<TT_DATA,
                            TT_COEFF,
                            TP_FIR_LEN,
                            TP_DECIMATE_FACTOR,
                            TP_SHIFT,
                            TP_RND,
                            TP_INPUT_WINDOW_VSIZE,
                            false,
                            false,
                            TP_FIR_RANGE_LEN,
                            0,
                            1,
                            0,
                            1,
                            DUAL_IP_SINGLE,
                            USE_STREAM_API,
                            TP_MODIFY_MARGIN_OFFSET,
                            TP_COEFF_PHASE,
                            TP_COEFF_PHASE_OFFSET,
                            TP_COEFF_PHASES,
                            TP_COEFF_PHASES_LEN,
                            TP_SAT>(taps) {}

    static void registerKernelClass() { REGISTER_FUNCTION(fir_decimate_asym::filter); }

    void filter(input_stream<TT_DATA>* inStream, output_stream<TT_DATA>* outStream);
};
// Single kernel specialization. No cascade ports. Static coefficients, dual output
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_DECIMATE_FACTOR,
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

class fir_decimate_asym<TT_DATA,
                        TT_COEFF,
                        TP_FIR_LEN,
                        TP_DECIMATE_FACTOR,
                        TP_SHIFT,
                        TP_RND,
                        TP_INPUT_WINDOW_VSIZE,
                        false,
                        false,
                        TP_FIR_RANGE_LEN,
                        0,
                        1,
                        0,
                        2,
                        DUAL_IP_SINGLE,
                        USE_STREAM_API,
                        TP_MODIFY_MARGIN_OFFSET,
                        TP_COEFF_PHASE,
                        TP_COEFF_PHASE_OFFSET,
                        TP_COEFF_PHASES,
                        TP_COEFF_PHASES_LEN,
                        TP_SAT> : public kernelFilterClass<TT_DATA,
                                                           TT_COEFF,
                                                           TP_FIR_LEN,
                                                           TP_DECIMATE_FACTOR,
                                                           TP_SHIFT,
                                                           TP_RND,
                                                           TP_INPUT_WINDOW_VSIZE,
                                                           false,
                                                           false,
                                                           TP_FIR_RANGE_LEN,
                                                           0,
                                                           1,
                                                           0,
                                                           2,
                                                           DUAL_IP_SINGLE,
                                                           USE_STREAM_API,
                                                           TP_MODIFY_MARGIN_OFFSET,
                                                           TP_COEFF_PHASE,
                                                           TP_COEFF_PHASE_OFFSET,
                                                           TP_COEFF_PHASES,
                                                           TP_COEFF_PHASES_LEN,
                                                           TP_SAT> {
   public:
    fir_decimate_asym(const TT_COEFF (&taps)[TP_FIR_LEN])
        : kernelFilterClass<TT_DATA,
                            TT_COEFF,
                            TP_FIR_LEN,
                            TP_DECIMATE_FACTOR,
                            TP_SHIFT,
                            TP_RND,
                            TP_INPUT_WINDOW_VSIZE,
                            false,
                            false,
                            TP_FIR_RANGE_LEN,
                            0,
                            1,
                            0,
                            2,
                            DUAL_IP_SINGLE,
                            USE_STREAM_API,
                            TP_MODIFY_MARGIN_OFFSET,
                            TP_COEFF_PHASE,
                            TP_COEFF_PHASE_OFFSET,
                            TP_COEFF_PHASES,
                            TP_COEFF_PHASES_LEN,
                            TP_SAT>(taps) {}

    static void registerKernelClass() { REGISTER_FUNCTION(fir_decimate_asym::filter); }

    void filter(input_stream<TT_DATA>* inStream, output_stream<TT_DATA>* outStream, output_stream<TT_DATA>* outStream2);
};

//-----------------------------------------------------------------------------------------------------
// Single kernel specialization. No cascade ports. Using coefficient reload, single output
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_DECIMATE_FACTOR,
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

class fir_decimate_asym<TT_DATA,
                        TT_COEFF,
                        TP_FIR_LEN,
                        TP_DECIMATE_FACTOR,
                        TP_SHIFT,
                        TP_RND,
                        TP_INPUT_WINDOW_VSIZE,
                        CASC_IN_FALSE,
                        CASC_OUT_FALSE,
                        TP_FIR_RANGE_LEN,
                        TP_KERNEL_POSITION,
                        TP_CASC_LEN,
                        USE_COEFF_RELOAD_TRUE,
                        1,
                        DUAL_IP_SINGLE,
                        USE_STREAM_API,
                        TP_MODIFY_MARGIN_OFFSET,
                        TP_COEFF_PHASE,
                        TP_COEFF_PHASE_OFFSET,
                        TP_COEFF_PHASES,
                        TP_COEFF_PHASES_LEN,
                        TP_SAT> : public kernelFilterClass<TT_DATA,
                                                           TT_COEFF,
                                                           TP_FIR_LEN,
                                                           TP_DECIMATE_FACTOR,
                                                           TP_SHIFT,
                                                           TP_RND,
                                                           TP_INPUT_WINDOW_VSIZE,
                                                           CASC_IN_FALSE,
                                                           CASC_OUT_FALSE,
                                                           TP_FIR_RANGE_LEN,
                                                           TP_KERNEL_POSITION,
                                                           TP_CASC_LEN,
                                                           USE_COEFF_RELOAD_TRUE,
                                                           1,
                                                           DUAL_IP_SINGLE,
                                                           USE_STREAM_API,
                                                           TP_MODIFY_MARGIN_OFFSET,
                                                           TP_COEFF_PHASE,
                                                           TP_COEFF_PHASE_OFFSET,
                                                           TP_COEFF_PHASES,
                                                           TP_COEFF_PHASES_LEN,
                                                           TP_SAT> {
   public:
    fir_decimate_asym()
        : kernelFilterClass<TT_DATA,
                            TT_COEFF,
                            TP_FIR_LEN,
                            TP_DECIMATE_FACTOR,
                            TP_SHIFT,
                            TP_RND,
                            TP_INPUT_WINDOW_VSIZE,
                            CASC_IN_FALSE,
                            CASC_OUT_FALSE,
                            TP_FIR_RANGE_LEN,
                            TP_KERNEL_POSITION,
                            TP_CASC_LEN,
                            USE_COEFF_RELOAD_TRUE,
                            1,
                            DUAL_IP_SINGLE,
                            USE_STREAM_API,
                            TP_MODIFY_MARGIN_OFFSET,
                            TP_COEFF_PHASE,
                            TP_COEFF_PHASE_OFFSET,
                            TP_COEFF_PHASES,
                            TP_COEFF_PHASES_LEN,
                            TP_SAT>() {}
    fir_decimate_asym(const TT_COEFF (&taps)[TP_FIR_LEN])
        : kernelFilterClass<TT_DATA,
                            TT_COEFF,
                            TP_FIR_LEN,
                            TP_DECIMATE_FACTOR,
                            TP_SHIFT,
                            TP_RND,
                            TP_INPUT_WINDOW_VSIZE,
                            CASC_IN_FALSE,
                            CASC_OUT_FALSE,
                            TP_FIR_RANGE_LEN,
                            TP_KERNEL_POSITION,
                            TP_CASC_LEN,
                            USE_COEFF_RELOAD_TRUE,
                            1,
                            DUAL_IP_SINGLE,
                            USE_STREAM_API,
                            TP_MODIFY_MARGIN_OFFSET,
                            TP_COEFF_PHASE,
                            TP_COEFF_PHASE_OFFSET,
                            TP_COEFF_PHASES,
                            TP_COEFF_PHASES_LEN,
                            TP_SAT>(taps) {}

    static void registerKernelClass() { REGISTER_FUNCTION(fir_decimate_asym::filter); }

    void filter(input_stream<TT_DATA>* inStream,
                output_stream<TT_DATA>* outStream,
                const TT_COEFF (&inTaps)[TP_COEFF_PHASES_LEN]);
};

// Single kernel specialization. No cascade ports. Using coefficient reload, dual output
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_DECIMATE_FACTOR,
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

class fir_decimate_asym<TT_DATA,
                        TT_COEFF,
                        TP_FIR_LEN,
                        TP_DECIMATE_FACTOR,
                        TP_SHIFT,
                        TP_RND,
                        TP_INPUT_WINDOW_VSIZE,
                        CASC_IN_FALSE,
                        CASC_OUT_FALSE,
                        TP_FIR_RANGE_LEN,
                        TP_KERNEL_POSITION,
                        TP_CASC_LEN,
                        USE_COEFF_RELOAD_TRUE,
                        2,
                        DUAL_IP_SINGLE,
                        USE_STREAM_API,
                        TP_MODIFY_MARGIN_OFFSET,
                        TP_COEFF_PHASE,
                        TP_COEFF_PHASE_OFFSET,
                        TP_COEFF_PHASES,
                        TP_COEFF_PHASES_LEN,
                        TP_SAT> : public kernelFilterClass<TT_DATA,
                                                           TT_COEFF,
                                                           TP_FIR_LEN,
                                                           TP_DECIMATE_FACTOR,
                                                           TP_SHIFT,
                                                           TP_RND,
                                                           TP_INPUT_WINDOW_VSIZE,
                                                           CASC_IN_FALSE,
                                                           CASC_OUT_FALSE,
                                                           TP_FIR_RANGE_LEN,
                                                           TP_KERNEL_POSITION,
                                                           TP_CASC_LEN,
                                                           USE_COEFF_RELOAD_TRUE,
                                                           2,
                                                           DUAL_IP_SINGLE,
                                                           USE_STREAM_API,
                                                           TP_MODIFY_MARGIN_OFFSET,
                                                           TP_COEFF_PHASE,
                                                           TP_COEFF_PHASE_OFFSET,
                                                           TP_COEFF_PHASES,
                                                           TP_COEFF_PHASES_LEN,
                                                           TP_SAT> {
   public:
    fir_decimate_asym()
        : kernelFilterClass<TT_DATA,
                            TT_COEFF,
                            TP_FIR_LEN,
                            TP_DECIMATE_FACTOR,
                            TP_SHIFT,
                            TP_RND,
                            TP_INPUT_WINDOW_VSIZE,
                            CASC_IN_FALSE,
                            CASC_OUT_FALSE,
                            TP_FIR_RANGE_LEN,
                            TP_KERNEL_POSITION,
                            TP_CASC_LEN,
                            USE_COEFF_RELOAD_TRUE,
                            2,
                            DUAL_IP_SINGLE,
                            USE_STREAM_API,
                            TP_MODIFY_MARGIN_OFFSET,
                            TP_COEFF_PHASE,
                            TP_COEFF_PHASE_OFFSET,
                            TP_COEFF_PHASES,
                            TP_COEFF_PHASES_LEN,
                            TP_SAT>() {}
    fir_decimate_asym(const TT_COEFF (&taps)[TP_FIR_LEN])
        : kernelFilterClass<TT_DATA,
                            TT_COEFF,
                            TP_FIR_LEN,
                            TP_DECIMATE_FACTOR,
                            TP_SHIFT,
                            TP_RND,
                            TP_INPUT_WINDOW_VSIZE,
                            CASC_IN_FALSE,
                            CASC_OUT_FALSE,
                            TP_FIR_RANGE_LEN,
                            TP_KERNEL_POSITION,
                            TP_CASC_LEN,
                            USE_COEFF_RELOAD_TRUE,
                            2,
                            DUAL_IP_SINGLE,
                            USE_STREAM_API,
                            TP_MODIFY_MARGIN_OFFSET,
                            TP_COEFF_PHASE,
                            TP_COEFF_PHASE_OFFSET,
                            TP_COEFF_PHASES,
                            TP_COEFF_PHASES_LEN,
                            TP_SAT>(taps) {}

    static void registerKernelClass() { REGISTER_FUNCTION(fir_decimate_asym::filter); }

    void filter(input_stream<TT_DATA>* inStream,
                output_stream<TT_DATA>* outStream,
                output_stream<TT_DATA>* outStream2,
                const TT_COEFF (&inTaps)[TP_COEFF_PHASES_LEN]);
};

//-----------------------------------------------------------------------------------------------------
// Partially specialized classes for cascaded interface - final kernel. Static coefficients single output
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_DECIMATE_FACTOR,
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

class fir_decimate_asym<TT_DATA,
                        TT_COEFF,
                        TP_FIR_LEN,
                        TP_DECIMATE_FACTOR,
                        TP_SHIFT,
                        TP_RND,
                        TP_INPUT_WINDOW_VSIZE,
                        CASC_IN_TRUE,
                        CASC_OUT_FALSE,
                        TP_FIR_RANGE_LEN,
                        TP_KERNEL_POSITION,
                        TP_CASC_LEN,
                        USE_COEFF_RELOAD_FALSE,
                        1,
                        DUAL_IP_SINGLE,
                        USE_STREAM_API,
                        TP_MODIFY_MARGIN_OFFSET,
                        TP_COEFF_PHASE,
                        TP_COEFF_PHASE_OFFSET,
                        TP_COEFF_PHASES,
                        TP_COEFF_PHASES_LEN,
                        TP_SAT> : public kernelFilterClass<TT_DATA,
                                                           TT_COEFF,
                                                           TP_FIR_LEN,
                                                           TP_DECIMATE_FACTOR,
                                                           TP_SHIFT,
                                                           TP_RND,
                                                           TP_INPUT_WINDOW_VSIZE,
                                                           CASC_IN_TRUE,
                                                           CASC_OUT_FALSE,
                                                           TP_FIR_RANGE_LEN,
                                                           TP_KERNEL_POSITION,
                                                           TP_CASC_LEN,
                                                           USE_COEFF_RELOAD_FALSE,
                                                           1,
                                                           DUAL_IP_SINGLE,
                                                           USE_STREAM_API,
                                                           TP_MODIFY_MARGIN_OFFSET,
                                                           TP_COEFF_PHASE,
                                                           TP_COEFF_PHASE_OFFSET,
                                                           TP_COEFF_PHASES,
                                                           TP_COEFF_PHASES_LEN,
                                                           TP_SAT> {
   public:
    fir_decimate_asym(const TT_COEFF (&taps)[TP_FIR_LEN])
        : kernelFilterClass<TT_DATA,
                            TT_COEFF,
                            TP_FIR_LEN,
                            TP_DECIMATE_FACTOR,
                            TP_SHIFT,
                            TP_RND,
                            TP_INPUT_WINDOW_VSIZE,
                            CASC_IN_TRUE,
                            CASC_OUT_FALSE,
                            TP_FIR_RANGE_LEN,
                            TP_KERNEL_POSITION,
                            TP_CASC_LEN,
                            USE_COEFF_RELOAD_FALSE,
                            1,
                            DUAL_IP_SINGLE,
                            USE_STREAM_API,
                            TP_MODIFY_MARGIN_OFFSET,
                            TP_COEFF_PHASE,
                            TP_COEFF_PHASE_OFFSET,
                            TP_COEFF_PHASES,
                            TP_COEFF_PHASES_LEN,
                            TP_SAT>(taps) {}

    static void registerKernelClass() { REGISTER_FUNCTION(fir_decimate_asym::filter); }

    void filter(input_stream<TT_DATA>* inStream, input_cascade_cacc* inCascade, output_stream<TT_DATA>* outStream);
};

// Partially specialized classes for cascaded interface - final kernel. Static coefficients, dual output
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_DECIMATE_FACTOR,
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

class fir_decimate_asym<TT_DATA,
                        TT_COEFF,
                        TP_FIR_LEN,
                        TP_DECIMATE_FACTOR,
                        TP_SHIFT,
                        TP_RND,
                        TP_INPUT_WINDOW_VSIZE,
                        CASC_IN_TRUE,
                        CASC_OUT_FALSE,
                        TP_FIR_RANGE_LEN,
                        TP_KERNEL_POSITION,
                        TP_CASC_LEN,
                        USE_COEFF_RELOAD_FALSE,
                        2,
                        DUAL_IP_SINGLE,
                        USE_STREAM_API,
                        TP_MODIFY_MARGIN_OFFSET,
                        TP_COEFF_PHASE,
                        TP_COEFF_PHASE_OFFSET,
                        TP_COEFF_PHASES,
                        TP_COEFF_PHASES_LEN,
                        TP_SAT> : public kernelFilterClass<TT_DATA,
                                                           TT_COEFF,
                                                           TP_FIR_LEN,
                                                           TP_DECIMATE_FACTOR,
                                                           TP_SHIFT,
                                                           TP_RND,
                                                           TP_INPUT_WINDOW_VSIZE,
                                                           CASC_IN_TRUE,
                                                           CASC_OUT_FALSE,
                                                           TP_FIR_RANGE_LEN,
                                                           TP_KERNEL_POSITION,
                                                           TP_CASC_LEN,
                                                           USE_COEFF_RELOAD_FALSE,
                                                           2,
                                                           DUAL_IP_SINGLE,
                                                           USE_STREAM_API,
                                                           TP_MODIFY_MARGIN_OFFSET,
                                                           TP_COEFF_PHASE,
                                                           TP_COEFF_PHASE_OFFSET,
                                                           TP_COEFF_PHASES,
                                                           TP_COEFF_PHASES_LEN,
                                                           TP_SAT> {
   public:
    fir_decimate_asym(const TT_COEFF (&taps)[TP_FIR_LEN])
        : kernelFilterClass<TT_DATA,
                            TT_COEFF,
                            TP_FIR_LEN,
                            TP_DECIMATE_FACTOR,
                            TP_SHIFT,
                            TP_RND,
                            TP_INPUT_WINDOW_VSIZE,
                            CASC_IN_TRUE,
                            CASC_OUT_FALSE,
                            TP_FIR_RANGE_LEN,
                            TP_KERNEL_POSITION,
                            TP_CASC_LEN,
                            USE_COEFF_RELOAD_FALSE,
                            2,
                            DUAL_IP_SINGLE,
                            USE_STREAM_API,
                            TP_MODIFY_MARGIN_OFFSET,
                            TP_COEFF_PHASE,
                            TP_COEFF_PHASE_OFFSET,
                            TP_COEFF_PHASES,
                            TP_COEFF_PHASES_LEN,
                            TP_SAT>(taps) {}

    static void registerKernelClass() { REGISTER_FUNCTION(fir_decimate_asym::filter); }

    void filter(input_stream<TT_DATA>* inStream,
                input_cascade_cacc* inCascade,
                output_stream<TT_DATA>* outStream,
                output_stream<TT_DATA>* outStream2);
};

//-----------------------------------------------------------------------------------------------------
// Partially specialized classes for cascaded interface - first kernel. Static coefficients
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_DECIMATE_FACTOR,
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

class fir_decimate_asym<TT_DATA,
                        TT_COEFF,
                        TP_FIR_LEN,
                        TP_DECIMATE_FACTOR,
                        TP_SHIFT,
                        TP_RND,
                        TP_INPUT_WINDOW_VSIZE,
                        CASC_IN_FALSE,
                        CASC_OUT_TRUE,
                        TP_FIR_RANGE_LEN,
                        TP_KERNEL_POSITION,
                        TP_CASC_LEN,
                        USE_COEFF_RELOAD_FALSE,
                        1,
                        DUAL_IP_SINGLE,
                        USE_STREAM_API,
                        TP_MODIFY_MARGIN_OFFSET,
                        TP_COEFF_PHASE,
                        TP_COEFF_PHASE_OFFSET,
                        TP_COEFF_PHASES,
                        TP_COEFF_PHASES_LEN,
                        TP_SAT> : public kernelFilterClass<TT_DATA,
                                                           TT_COEFF,
                                                           TP_FIR_LEN,
                                                           TP_DECIMATE_FACTOR,
                                                           TP_SHIFT,
                                                           TP_RND,
                                                           TP_INPUT_WINDOW_VSIZE,
                                                           CASC_IN_FALSE,
                                                           CASC_OUT_TRUE,
                                                           TP_FIR_RANGE_LEN,
                                                           TP_KERNEL_POSITION,
                                                           TP_CASC_LEN,
                                                           USE_COEFF_RELOAD_FALSE,
                                                           1,
                                                           DUAL_IP_SINGLE,
                                                           USE_STREAM_API,
                                                           TP_MODIFY_MARGIN_OFFSET,
                                                           TP_COEFF_PHASE,
                                                           TP_COEFF_PHASE_OFFSET,
                                                           TP_COEFF_PHASES,
                                                           TP_COEFF_PHASES_LEN,
                                                           TP_SAT> {
   public:
    fir_decimate_asym(const TT_COEFF (&taps)[TP_FIR_LEN])
        : kernelFilterClass<TT_DATA,
                            TT_COEFF,
                            TP_FIR_LEN,
                            TP_DECIMATE_FACTOR,
                            TP_SHIFT,
                            TP_RND,
                            TP_INPUT_WINDOW_VSIZE,
                            CASC_IN_FALSE,
                            CASC_OUT_TRUE,
                            TP_FIR_RANGE_LEN,
                            TP_KERNEL_POSITION,
                            TP_CASC_LEN,
                            USE_COEFF_RELOAD_FALSE,
                            1,
                            DUAL_IP_SINGLE,
                            USE_STREAM_API,
                            TP_MODIFY_MARGIN_OFFSET,
                            TP_COEFF_PHASE,
                            TP_COEFF_PHASE_OFFSET,
                            TP_COEFF_PHASES,
                            TP_COEFF_PHASES_LEN,
                            TP_SAT>(taps) {}

    static void registerKernelClass() { REGISTER_FUNCTION(fir_decimate_asym::filter); }

    void filter(input_stream<TT_DATA>* inStream, output_cascade_cacc* outCascade);
};

//-----------------------------------------------------------------------------------------------------
// Partially specialized classes for cascaded interface - middle kernel. Static coefficients
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_DECIMATE_FACTOR,
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

class fir_decimate_asym<TT_DATA,
                        TT_COEFF,
                        TP_FIR_LEN,
                        TP_DECIMATE_FACTOR,
                        TP_SHIFT,
                        TP_RND,
                        TP_INPUT_WINDOW_VSIZE,
                        CASC_IN_TRUE,
                        CASC_OUT_TRUE,
                        TP_FIR_RANGE_LEN,
                        TP_KERNEL_POSITION,
                        TP_CASC_LEN,
                        USE_COEFF_RELOAD_FALSE,
                        1,
                        DUAL_IP_SINGLE,
                        USE_STREAM_API,
                        TP_MODIFY_MARGIN_OFFSET,
                        TP_COEFF_PHASE,
                        TP_COEFF_PHASE_OFFSET,
                        TP_COEFF_PHASES,
                        TP_COEFF_PHASES_LEN,
                        TP_SAT> : public kernelFilterClass<TT_DATA,
                                                           TT_COEFF,
                                                           TP_FIR_LEN,
                                                           TP_DECIMATE_FACTOR,
                                                           TP_SHIFT,
                                                           TP_RND,
                                                           TP_INPUT_WINDOW_VSIZE,
                                                           CASC_IN_TRUE,
                                                           CASC_OUT_TRUE,
                                                           TP_FIR_RANGE_LEN,
                                                           TP_KERNEL_POSITION,
                                                           TP_CASC_LEN,
                                                           USE_COEFF_RELOAD_FALSE,
                                                           1,
                                                           DUAL_IP_SINGLE,
                                                           USE_STREAM_API,
                                                           TP_MODIFY_MARGIN_OFFSET,
                                                           TP_COEFF_PHASE,
                                                           TP_COEFF_PHASE_OFFSET,
                                                           TP_COEFF_PHASES,
                                                           TP_COEFF_PHASES_LEN,
                                                           TP_SAT> {
   public:
    fir_decimate_asym(const TT_COEFF (&taps)[TP_FIR_LEN])
        : kernelFilterClass<TT_DATA,
                            TT_COEFF,
                            TP_FIR_LEN,
                            TP_DECIMATE_FACTOR,
                            TP_SHIFT,
                            TP_RND,
                            TP_INPUT_WINDOW_VSIZE,
                            CASC_IN_TRUE,
                            CASC_OUT_TRUE,
                            TP_FIR_RANGE_LEN,
                            TP_KERNEL_POSITION,
                            TP_CASC_LEN,
                            USE_COEFF_RELOAD_FALSE,
                            1,
                            DUAL_IP_SINGLE,
                            USE_STREAM_API,
                            TP_MODIFY_MARGIN_OFFSET,
                            TP_COEFF_PHASE,
                            TP_COEFF_PHASE_OFFSET,
                            TP_COEFF_PHASES,
                            TP_COEFF_PHASES_LEN,
                            TP_SAT>(taps) {}

    static void registerKernelClass() { REGISTER_FUNCTION(fir_decimate_asym::filter); }

    void filter(input_stream<TT_DATA>* inStream, input_cascade_cacc* inCascade, output_cascade_cacc* outCascade);
};

//-----------------------------------------------------------------------------------------------------
// Partially specialized classes for cascaded interface - final kernel. Reloadable coefficients
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_DECIMATE_FACTOR,
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

class fir_decimate_asym<TT_DATA,
                        TT_COEFF,
                        TP_FIR_LEN,
                        TP_DECIMATE_FACTOR,
                        TP_SHIFT,
                        TP_RND,
                        TP_INPUT_WINDOW_VSIZE,
                        CASC_IN_TRUE,
                        CASC_OUT_FALSE,
                        TP_FIR_RANGE_LEN,
                        TP_KERNEL_POSITION,
                        TP_CASC_LEN,
                        USE_COEFF_RELOAD_TRUE,
                        1,
                        DUAL_IP_SINGLE,
                        USE_STREAM_API,
                        TP_MODIFY_MARGIN_OFFSET,
                        TP_COEFF_PHASE,
                        TP_COEFF_PHASE_OFFSET,
                        TP_COEFF_PHASES,
                        TP_COEFF_PHASES_LEN,
                        TP_SAT> : public kernelFilterClass<TT_DATA,
                                                           TT_COEFF,
                                                           TP_FIR_LEN,
                                                           TP_DECIMATE_FACTOR,
                                                           TP_SHIFT,
                                                           TP_RND,
                                                           TP_INPUT_WINDOW_VSIZE,
                                                           CASC_IN_TRUE,
                                                           CASC_OUT_FALSE,
                                                           TP_FIR_RANGE_LEN,
                                                           TP_KERNEL_POSITION,
                                                           TP_CASC_LEN,
                                                           USE_COEFF_RELOAD_TRUE,
                                                           1,
                                                           DUAL_IP_SINGLE,
                                                           USE_STREAM_API,
                                                           TP_MODIFY_MARGIN_OFFSET,
                                                           TP_COEFF_PHASE,
                                                           TP_COEFF_PHASE_OFFSET,
                                                           TP_COEFF_PHASES,
                                                           TP_COEFF_PHASES_LEN,
                                                           TP_SAT> {
   public:
    fir_decimate_asym()
        : kernelFilterClass<TT_DATA,
                            TT_COEFF,
                            TP_FIR_LEN,
                            TP_DECIMATE_FACTOR,
                            TP_SHIFT,
                            TP_RND,
                            TP_INPUT_WINDOW_VSIZE,
                            CASC_IN_TRUE,
                            CASC_OUT_FALSE,
                            TP_FIR_RANGE_LEN,
                            TP_KERNEL_POSITION,
                            TP_CASC_LEN,
                            USE_COEFF_RELOAD_TRUE,
                            1,
                            DUAL_IP_SINGLE,
                            USE_STREAM_API,
                            TP_MODIFY_MARGIN_OFFSET,
                            TP_COEFF_PHASE,
                            TP_COEFF_PHASE_OFFSET,
                            TP_COEFF_PHASES,
                            TP_COEFF_PHASES_LEN,
                            TP_SAT>() {}
    fir_decimate_asym(const TT_COEFF (&taps)[TP_FIR_LEN])
        : kernelFilterClass<TT_DATA,
                            TT_COEFF,
                            TP_FIR_LEN,
                            TP_DECIMATE_FACTOR,
                            TP_SHIFT,
                            TP_RND,
                            TP_INPUT_WINDOW_VSIZE,
                            CASC_IN_TRUE,
                            CASC_OUT_FALSE,
                            TP_FIR_RANGE_LEN,
                            TP_KERNEL_POSITION,
                            TP_CASC_LEN,
                            USE_COEFF_RELOAD_TRUE,
                            1,
                            DUAL_IP_SINGLE,
                            USE_STREAM_API,
                            TP_MODIFY_MARGIN_OFFSET,
                            TP_COEFF_PHASE,
                            TP_COEFF_PHASE_OFFSET,
                            TP_COEFF_PHASES,
                            TP_COEFF_PHASES_LEN,
                            TP_SAT>(taps) {}

    static void registerKernelClass() { REGISTER_FUNCTION(fir_decimate_asym::filter); }

    void filter(input_stream<TT_DATA>* inStream, input_cascade_cacc* inCascade, output_stream<TT_DATA>* outStream);
};

// Partially specialized classes for cascaded interface - final kernel. Re-loadable coefficients, dual output
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_DECIMATE_FACTOR,
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

class fir_decimate_asym<TT_DATA,
                        TT_COEFF,
                        TP_FIR_LEN,
                        TP_DECIMATE_FACTOR,
                        TP_SHIFT,
                        TP_RND,
                        TP_INPUT_WINDOW_VSIZE,
                        CASC_IN_TRUE,
                        CASC_OUT_FALSE,
                        TP_FIR_RANGE_LEN,
                        TP_KERNEL_POSITION,
                        TP_CASC_LEN,
                        USE_COEFF_RELOAD_TRUE,
                        2,
                        DUAL_IP_SINGLE,
                        USE_STREAM_API,
                        TP_MODIFY_MARGIN_OFFSET,
                        TP_COEFF_PHASE,
                        TP_COEFF_PHASE_OFFSET,
                        TP_COEFF_PHASES,
                        TP_COEFF_PHASES_LEN,
                        TP_SAT> : public kernelFilterClass<TT_DATA,
                                                           TT_COEFF,
                                                           TP_FIR_LEN,
                                                           TP_DECIMATE_FACTOR,
                                                           TP_SHIFT,
                                                           TP_RND,
                                                           TP_INPUT_WINDOW_VSIZE,
                                                           CASC_IN_TRUE,
                                                           CASC_OUT_FALSE,
                                                           TP_FIR_RANGE_LEN,
                                                           TP_KERNEL_POSITION,
                                                           TP_CASC_LEN,
                                                           USE_COEFF_RELOAD_TRUE,
                                                           2,
                                                           DUAL_IP_SINGLE,
                                                           USE_STREAM_API,
                                                           TP_MODIFY_MARGIN_OFFSET,
                                                           TP_COEFF_PHASE,
                                                           TP_COEFF_PHASE_OFFSET,
                                                           TP_COEFF_PHASES,
                                                           TP_COEFF_PHASES_LEN,
                                                           TP_SAT> {
   public:
    fir_decimate_asym()
        : kernelFilterClass<TT_DATA,
                            TT_COEFF,
                            TP_FIR_LEN,
                            TP_DECIMATE_FACTOR,
                            TP_SHIFT,
                            TP_RND,
                            TP_INPUT_WINDOW_VSIZE,
                            CASC_IN_TRUE,
                            CASC_OUT_FALSE,
                            TP_FIR_RANGE_LEN,
                            TP_KERNEL_POSITION,
                            TP_CASC_LEN,
                            USE_COEFF_RELOAD_TRUE,
                            2,
                            DUAL_IP_SINGLE,
                            USE_STREAM_API,
                            TP_MODIFY_MARGIN_OFFSET,
                            TP_COEFF_PHASE,
                            TP_COEFF_PHASE_OFFSET,
                            TP_COEFF_PHASES,
                            TP_COEFF_PHASES_LEN,
                            TP_SAT>() {}
    fir_decimate_asym(const TT_COEFF (&taps)[TP_FIR_LEN])
        : kernelFilterClass<TT_DATA,
                            TT_COEFF,
                            TP_FIR_LEN,
                            TP_DECIMATE_FACTOR,
                            TP_SHIFT,
                            TP_RND,
                            TP_INPUT_WINDOW_VSIZE,
                            CASC_IN_TRUE,
                            CASC_OUT_FALSE,
                            TP_FIR_RANGE_LEN,
                            TP_KERNEL_POSITION,
                            TP_CASC_LEN,
                            USE_COEFF_RELOAD_TRUE,
                            2,
                            DUAL_IP_SINGLE,
                            USE_STREAM_API,
                            TP_MODIFY_MARGIN_OFFSET,
                            TP_COEFF_PHASE,
                            TP_COEFF_PHASE_OFFSET,
                            TP_COEFF_PHASES,
                            TP_COEFF_PHASES_LEN,
                            TP_SAT>(taps) {}

    static void registerKernelClass() { REGISTER_FUNCTION(fir_decimate_asym::filter); }

    void filter(input_stream<TT_DATA>* inStream,
                input_cascade_cacc* inCascade,
                output_stream<TT_DATA>* outStream,
                output_stream<TT_DATA>* outStream2);
};

//-----------------------------------------------------------------------------------------------------
// Partially specialized classes for cascaded interface - first kernel. Re-loadable coefficients
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_DECIMATE_FACTOR,
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

class fir_decimate_asym<TT_DATA,
                        TT_COEFF,
                        TP_FIR_LEN,
                        TP_DECIMATE_FACTOR,
                        TP_SHIFT,
                        TP_RND,
                        TP_INPUT_WINDOW_VSIZE,
                        CASC_IN_FALSE,
                        CASC_OUT_TRUE,
                        TP_FIR_RANGE_LEN,
                        TP_KERNEL_POSITION,
                        TP_CASC_LEN,
                        USE_COEFF_RELOAD_TRUE,
                        1,
                        DUAL_IP_SINGLE,
                        USE_STREAM_API,
                        TP_MODIFY_MARGIN_OFFSET,
                        TP_COEFF_PHASE,
                        TP_COEFF_PHASE_OFFSET,
                        TP_COEFF_PHASES,
                        TP_COEFF_PHASES_LEN,
                        TP_SAT> : public kernelFilterClass<TT_DATA,
                                                           TT_COEFF,
                                                           TP_FIR_LEN,
                                                           TP_DECIMATE_FACTOR,
                                                           TP_SHIFT,
                                                           TP_RND,
                                                           TP_INPUT_WINDOW_VSIZE,
                                                           CASC_IN_FALSE,
                                                           CASC_OUT_TRUE,
                                                           TP_FIR_RANGE_LEN,
                                                           TP_KERNEL_POSITION,
                                                           TP_CASC_LEN,
                                                           USE_COEFF_RELOAD_TRUE,
                                                           1,
                                                           DUAL_IP_SINGLE,
                                                           USE_STREAM_API,
                                                           TP_MODIFY_MARGIN_OFFSET,
                                                           TP_COEFF_PHASE,
                                                           TP_COEFF_PHASE_OFFSET,
                                                           TP_COEFF_PHASES,
                                                           TP_COEFF_PHASES_LEN,
                                                           TP_SAT> {
   public:
    fir_decimate_asym()
        : kernelFilterClass<TT_DATA,
                            TT_COEFF,
                            TP_FIR_LEN,
                            TP_DECIMATE_FACTOR,
                            TP_SHIFT,
                            TP_RND,
                            TP_INPUT_WINDOW_VSIZE,
                            CASC_IN_FALSE,
                            CASC_OUT_TRUE,
                            TP_FIR_RANGE_LEN,
                            TP_KERNEL_POSITION,
                            TP_CASC_LEN,
                            USE_COEFF_RELOAD_TRUE,
                            1,
                            DUAL_IP_SINGLE,
                            USE_STREAM_API,
                            TP_MODIFY_MARGIN_OFFSET,
                            TP_COEFF_PHASE,
                            TP_COEFF_PHASE_OFFSET,
                            TP_COEFF_PHASES,
                            TP_COEFF_PHASES_LEN,
                            TP_SAT>() {}
    fir_decimate_asym(const TT_COEFF (&taps)[TP_FIR_LEN])
        : kernelFilterClass<TT_DATA,
                            TT_COEFF,
                            TP_FIR_LEN,
                            TP_DECIMATE_FACTOR,
                            TP_SHIFT,
                            TP_RND,
                            TP_INPUT_WINDOW_VSIZE,
                            CASC_IN_FALSE,
                            CASC_OUT_TRUE,
                            TP_FIR_RANGE_LEN,
                            TP_KERNEL_POSITION,
                            TP_CASC_LEN,
                            USE_COEFF_RELOAD_TRUE,
                            1,
                            DUAL_IP_SINGLE,
                            USE_STREAM_API,
                            TP_MODIFY_MARGIN_OFFSET,
                            TP_COEFF_PHASE,
                            TP_COEFF_PHASE_OFFSET,
                            TP_COEFF_PHASES,
                            TP_COEFF_PHASES_LEN,
                            TP_SAT>(taps) {}

    static void registerKernelClass() { REGISTER_FUNCTION(fir_decimate_asym::filter); }

    void filter(input_stream<TT_DATA>* inStream,
                output_cascade_cacc* outCascade,
                const TT_COEFF (&inTaps)[TP_COEFF_PHASES_LEN]);
};

//-----------------------------------------------------------------------------------------------------
// Partially specialized classes for cascaded interface - middle kernel. Re-loadable coefficients
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_DECIMATE_FACTOR,
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

class fir_decimate_asym<TT_DATA,
                        TT_COEFF,
                        TP_FIR_LEN,
                        TP_DECIMATE_FACTOR,
                        TP_SHIFT,
                        TP_RND,
                        TP_INPUT_WINDOW_VSIZE,
                        CASC_IN_TRUE,
                        CASC_OUT_TRUE,
                        TP_FIR_RANGE_LEN,
                        TP_KERNEL_POSITION,
                        TP_CASC_LEN,
                        USE_COEFF_RELOAD_TRUE,
                        1,
                        DUAL_IP_SINGLE,
                        USE_STREAM_API,
                        TP_MODIFY_MARGIN_OFFSET,
                        TP_COEFF_PHASE,
                        TP_COEFF_PHASE_OFFSET,
                        TP_COEFF_PHASES,
                        TP_COEFF_PHASES_LEN,
                        TP_SAT> : public kernelFilterClass<TT_DATA,
                                                           TT_COEFF,
                                                           TP_FIR_LEN,
                                                           TP_DECIMATE_FACTOR,
                                                           TP_SHIFT,
                                                           TP_RND,
                                                           TP_INPUT_WINDOW_VSIZE,
                                                           CASC_IN_TRUE,
                                                           CASC_OUT_TRUE,
                                                           TP_FIR_RANGE_LEN,
                                                           TP_KERNEL_POSITION,
                                                           TP_CASC_LEN,
                                                           USE_COEFF_RELOAD_TRUE,
                                                           1,
                                                           DUAL_IP_SINGLE,
                                                           USE_STREAM_API,
                                                           TP_MODIFY_MARGIN_OFFSET,
                                                           TP_COEFF_PHASE,
                                                           TP_COEFF_PHASE_OFFSET,
                                                           TP_COEFF_PHASES,
                                                           TP_COEFF_PHASES_LEN,
                                                           TP_SAT> {
   public:
    fir_decimate_asym()
        : kernelFilterClass<TT_DATA,
                            TT_COEFF,
                            TP_FIR_LEN,
                            TP_DECIMATE_FACTOR,
                            TP_SHIFT,
                            TP_RND,
                            TP_INPUT_WINDOW_VSIZE,
                            CASC_IN_TRUE,
                            CASC_OUT_TRUE,
                            TP_FIR_RANGE_LEN,
                            TP_KERNEL_POSITION,
                            TP_CASC_LEN,
                            USE_COEFF_RELOAD_TRUE,
                            1,
                            DUAL_IP_SINGLE,
                            USE_STREAM_API,
                            TP_MODIFY_MARGIN_OFFSET,
                            TP_COEFF_PHASE,
                            TP_COEFF_PHASE_OFFSET,
                            TP_COEFF_PHASES,
                            TP_COEFF_PHASES_LEN,
                            TP_SAT>() {}
    fir_decimate_asym(const TT_COEFF (&taps)[TP_FIR_LEN])
        : kernelFilterClass<TT_DATA,
                            TT_COEFF,
                            TP_FIR_LEN,
                            TP_DECIMATE_FACTOR,
                            TP_SHIFT,
                            TP_RND,
                            TP_INPUT_WINDOW_VSIZE,
                            CASC_IN_TRUE,
                            CASC_OUT_TRUE,
                            TP_FIR_RANGE_LEN,
                            TP_KERNEL_POSITION,
                            TP_CASC_LEN,
                            USE_COEFF_RELOAD_TRUE,
                            1,
                            DUAL_IP_SINGLE,
                            USE_STREAM_API,
                            TP_MODIFY_MARGIN_OFFSET,
                            TP_COEFF_PHASE,
                            TP_COEFF_PHASE_OFFSET,
                            TP_COEFF_PHASES,
                            TP_COEFF_PHASES_LEN,
                            TP_SAT>(taps) {}

    static void registerKernelClass() { REGISTER_FUNCTION(fir_decimate_asym::filter); }

    void filter(input_stream<TT_DATA>* inStream, input_cascade_cacc* inCascade, output_cascade_cacc* outCascade);
};

// ----------------------------------------------------------------------------
// ----------------------------- DUAL STREAM ----------------------------------
// ----------------------------------------------------------------------------

//-----------------------------------------------------------------------------------------------------
// Single kernel base specialization. No cascade ports. Static coefficients

// Single kernel specialization. No cascade ports. Static coefficients, dual output
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_DECIMATE_FACTOR,
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

class fir_decimate_asym<TT_DATA,
                        TT_COEFF,
                        TP_FIR_LEN,
                        TP_DECIMATE_FACTOR,
                        TP_SHIFT,
                        TP_RND,
                        TP_INPUT_WINDOW_VSIZE,
                        false,
                        false,
                        TP_FIR_RANGE_LEN,
                        0,
                        1,
                        0,
                        1,
                        DUAL_IP_DUAL,
                        USE_STREAM_API,
                        TP_MODIFY_MARGIN_OFFSET,
                        TP_COEFF_PHASE,
                        TP_COEFF_PHASE_OFFSET,
                        TP_COEFF_PHASES,
                        TP_COEFF_PHASES_LEN,
                        TP_SAT> : public kernelFilterClass<TT_DATA,
                                                           TT_COEFF,
                                                           TP_FIR_LEN,
                                                           TP_DECIMATE_FACTOR,
                                                           TP_SHIFT,
                                                           TP_RND,
                                                           TP_INPUT_WINDOW_VSIZE,
                                                           false,
                                                           false,
                                                           TP_FIR_RANGE_LEN,
                                                           0,
                                                           1,
                                                           0,
                                                           1,
                                                           DUAL_IP_DUAL,
                                                           USE_STREAM_API,
                                                           TP_MODIFY_MARGIN_OFFSET,
                                                           TP_COEFF_PHASE,
                                                           TP_COEFF_PHASE_OFFSET,
                                                           TP_COEFF_PHASES,
                                                           TP_COEFF_PHASES_LEN,
                                                           TP_SAT> {
   public:
    fir_decimate_asym(const TT_COEFF (&taps)[TP_FIR_LEN])
        : kernelFilterClass<TT_DATA,
                            TT_COEFF,
                            TP_FIR_LEN,
                            TP_DECIMATE_FACTOR,
                            TP_SHIFT,
                            TP_RND,
                            TP_INPUT_WINDOW_VSIZE,
                            false,
                            false,
                            TP_FIR_RANGE_LEN,
                            0,
                            1,
                            0,
                            1,
                            DUAL_IP_DUAL,
                            USE_STREAM_API,
                            TP_MODIFY_MARGIN_OFFSET,
                            TP_COEFF_PHASE,
                            TP_COEFF_PHASE_OFFSET,
                            TP_COEFF_PHASES,
                            TP_COEFF_PHASES_LEN,
                            TP_SAT>(taps) {}

    static void registerKernelClass() { REGISTER_FUNCTION(fir_decimate_asym::filter); }

    void filter(input_stream<TT_DATA>* inStream, input_stream<TT_DATA>* inStream2, output_stream<TT_DATA>* outStream);
};
// Single kernel specialization. No cascade ports. Static coefficients, dual output
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_DECIMATE_FACTOR,
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

class fir_decimate_asym<TT_DATA,
                        TT_COEFF,
                        TP_FIR_LEN,
                        TP_DECIMATE_FACTOR,
                        TP_SHIFT,
                        TP_RND,
                        TP_INPUT_WINDOW_VSIZE,
                        false,
                        false,
                        TP_FIR_RANGE_LEN,
                        0,
                        1,
                        0,
                        2,
                        DUAL_IP_DUAL,
                        USE_STREAM_API,
                        TP_MODIFY_MARGIN_OFFSET,
                        TP_COEFF_PHASE,
                        TP_COEFF_PHASE_OFFSET,
                        TP_COEFF_PHASES,
                        TP_COEFF_PHASES_LEN,
                        TP_SAT> : public kernelFilterClass<TT_DATA,
                                                           TT_COEFF,
                                                           TP_FIR_LEN,
                                                           TP_DECIMATE_FACTOR,
                                                           TP_SHIFT,
                                                           TP_RND,
                                                           TP_INPUT_WINDOW_VSIZE,
                                                           false,
                                                           false,
                                                           TP_FIR_RANGE_LEN,
                                                           0,
                                                           1,
                                                           0,
                                                           2,
                                                           DUAL_IP_DUAL,
                                                           USE_STREAM_API,
                                                           TP_MODIFY_MARGIN_OFFSET,
                                                           TP_COEFF_PHASE,
                                                           TP_COEFF_PHASE_OFFSET,
                                                           TP_COEFF_PHASES,
                                                           TP_COEFF_PHASES_LEN,
                                                           TP_SAT> {
   public:
    fir_decimate_asym(const TT_COEFF (&taps)[TP_FIR_LEN])
        : kernelFilterClass<TT_DATA,
                            TT_COEFF,
                            TP_FIR_LEN,
                            TP_DECIMATE_FACTOR,
                            TP_SHIFT,
                            TP_RND,
                            TP_INPUT_WINDOW_VSIZE,
                            false,
                            false,
                            TP_FIR_RANGE_LEN,
                            0,
                            1,
                            0,
                            2,
                            DUAL_IP_DUAL,
                            USE_STREAM_API,
                            TP_MODIFY_MARGIN_OFFSET,
                            TP_COEFF_PHASE,
                            TP_COEFF_PHASE_OFFSET,
                            TP_COEFF_PHASES,
                            TP_COEFF_PHASES_LEN,
                            TP_SAT>(taps) {}

    static void registerKernelClass() { REGISTER_FUNCTION(fir_decimate_asym::filter); }

    void filter(input_stream<TT_DATA>* inStream,
                input_stream<TT_DATA>* inStream2,
                output_stream<TT_DATA>* outStream,
                output_stream<TT_DATA>* outStream2);
};

//-----------------------------------------------------------------------------------------------------
// Single kernel specialization. No cascade ports. Using coefficient reload, single output
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_DECIMATE_FACTOR,
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

class fir_decimate_asym<TT_DATA,
                        TT_COEFF,
                        TP_FIR_LEN,
                        TP_DECIMATE_FACTOR,
                        TP_SHIFT,
                        TP_RND,
                        TP_INPUT_WINDOW_VSIZE,
                        CASC_IN_FALSE,
                        CASC_OUT_FALSE,
                        TP_FIR_RANGE_LEN,
                        TP_KERNEL_POSITION,
                        TP_CASC_LEN,
                        USE_COEFF_RELOAD_TRUE,
                        1,
                        DUAL_IP_DUAL,
                        USE_STREAM_API,
                        TP_MODIFY_MARGIN_OFFSET,
                        TP_COEFF_PHASE,
                        TP_COEFF_PHASE_OFFSET,
                        TP_COEFF_PHASES,
                        TP_COEFF_PHASES_LEN,
                        TP_SAT> : public kernelFilterClass<TT_DATA,
                                                           TT_COEFF,
                                                           TP_FIR_LEN,
                                                           TP_DECIMATE_FACTOR,
                                                           TP_SHIFT,
                                                           TP_RND,
                                                           TP_INPUT_WINDOW_VSIZE,
                                                           CASC_IN_FALSE,
                                                           CASC_OUT_FALSE,
                                                           TP_FIR_RANGE_LEN,
                                                           TP_KERNEL_POSITION,
                                                           TP_CASC_LEN,
                                                           USE_COEFF_RELOAD_TRUE,
                                                           1,
                                                           DUAL_IP_DUAL,
                                                           USE_STREAM_API,
                                                           TP_MODIFY_MARGIN_OFFSET,
                                                           TP_COEFF_PHASE,
                                                           TP_COEFF_PHASE_OFFSET,
                                                           TP_COEFF_PHASES,
                                                           TP_COEFF_PHASES_LEN,
                                                           TP_SAT> {
   public:
    fir_decimate_asym()
        : kernelFilterClass<TT_DATA,
                            TT_COEFF,
                            TP_FIR_LEN,
                            TP_DECIMATE_FACTOR,
                            TP_SHIFT,
                            TP_RND,
                            TP_INPUT_WINDOW_VSIZE,
                            CASC_IN_FALSE,
                            CASC_OUT_FALSE,
                            TP_FIR_RANGE_LEN,
                            TP_KERNEL_POSITION,
                            TP_CASC_LEN,
                            USE_COEFF_RELOAD_TRUE,
                            1,
                            DUAL_IP_DUAL,
                            USE_STREAM_API,
                            TP_MODIFY_MARGIN_OFFSET,
                            TP_COEFF_PHASE,
                            TP_COEFF_PHASE_OFFSET,
                            TP_COEFF_PHASES,
                            TP_COEFF_PHASES_LEN,
                            TP_SAT>() {}
    fir_decimate_asym(const TT_COEFF (&taps)[TP_FIR_LEN])
        : kernelFilterClass<TT_DATA,
                            TT_COEFF,
                            TP_FIR_LEN,
                            TP_DECIMATE_FACTOR,
                            TP_SHIFT,
                            TP_RND,
                            TP_INPUT_WINDOW_VSIZE,
                            CASC_IN_FALSE,
                            CASC_OUT_FALSE,
                            TP_FIR_RANGE_LEN,
                            TP_KERNEL_POSITION,
                            TP_CASC_LEN,
                            USE_COEFF_RELOAD_TRUE,
                            1,
                            DUAL_IP_DUAL,
                            USE_STREAM_API,
                            TP_MODIFY_MARGIN_OFFSET,
                            TP_COEFF_PHASE,
                            TP_COEFF_PHASE_OFFSET,
                            TP_COEFF_PHASES,
                            TP_COEFF_PHASES_LEN,
                            TP_SAT>(taps) {}

    static void registerKernelClass() { REGISTER_FUNCTION(fir_decimate_asym::filter); }

    void filter(input_stream<TT_DATA>* inStream,
                input_stream<TT_DATA>* inStream2,
                output_stream<TT_DATA>* outStream,
                const TT_COEFF (&inTaps)[TP_COEFF_PHASES_LEN]);
};

// Single kernel specialization. No cascade ports. Using coefficient reload, dual output
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_DECIMATE_FACTOR,
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

class fir_decimate_asym<TT_DATA,
                        TT_COEFF,
                        TP_FIR_LEN,
                        TP_DECIMATE_FACTOR,
                        TP_SHIFT,
                        TP_RND,
                        TP_INPUT_WINDOW_VSIZE,
                        CASC_IN_FALSE,
                        CASC_OUT_FALSE,
                        TP_FIR_RANGE_LEN,
                        TP_KERNEL_POSITION,
                        TP_CASC_LEN,
                        USE_COEFF_RELOAD_TRUE,
                        2,
                        DUAL_IP_DUAL,
                        USE_STREAM_API,
                        TP_MODIFY_MARGIN_OFFSET,
                        TP_COEFF_PHASE,
                        TP_COEFF_PHASE_OFFSET,
                        TP_COEFF_PHASES,
                        TP_COEFF_PHASES_LEN,
                        TP_SAT> : public kernelFilterClass<TT_DATA,
                                                           TT_COEFF,
                                                           TP_FIR_LEN,
                                                           TP_DECIMATE_FACTOR,
                                                           TP_SHIFT,
                                                           TP_RND,
                                                           TP_INPUT_WINDOW_VSIZE,
                                                           CASC_IN_FALSE,
                                                           CASC_OUT_FALSE,
                                                           TP_FIR_RANGE_LEN,
                                                           TP_KERNEL_POSITION,
                                                           TP_CASC_LEN,
                                                           USE_COEFF_RELOAD_TRUE,
                                                           2,
                                                           DUAL_IP_DUAL,
                                                           USE_STREAM_API,
                                                           TP_MODIFY_MARGIN_OFFSET,
                                                           TP_COEFF_PHASE,
                                                           TP_COEFF_PHASE_OFFSET,
                                                           TP_COEFF_PHASES,
                                                           TP_COEFF_PHASES_LEN,
                                                           TP_SAT> {
   public:
    fir_decimate_asym()
        : kernelFilterClass<TT_DATA,
                            TT_COEFF,
                            TP_FIR_LEN,
                            TP_DECIMATE_FACTOR,
                            TP_SHIFT,
                            TP_RND,
                            TP_INPUT_WINDOW_VSIZE,
                            CASC_IN_FALSE,
                            CASC_OUT_FALSE,
                            TP_FIR_RANGE_LEN,
                            TP_KERNEL_POSITION,
                            TP_CASC_LEN,
                            USE_COEFF_RELOAD_TRUE,
                            2,
                            DUAL_IP_DUAL,
                            USE_STREAM_API,
                            TP_MODIFY_MARGIN_OFFSET,
                            TP_COEFF_PHASE,
                            TP_COEFF_PHASE_OFFSET,
                            TP_COEFF_PHASES,
                            TP_COEFF_PHASES_LEN,
                            TP_SAT>() {}
    fir_decimate_asym(const TT_COEFF (&taps)[TP_FIR_LEN])
        : kernelFilterClass<TT_DATA,
                            TT_COEFF,
                            TP_FIR_LEN,
                            TP_DECIMATE_FACTOR,
                            TP_SHIFT,
                            TP_RND,
                            TP_INPUT_WINDOW_VSIZE,
                            CASC_IN_FALSE,
                            CASC_OUT_FALSE,
                            TP_FIR_RANGE_LEN,
                            TP_KERNEL_POSITION,
                            TP_CASC_LEN,
                            USE_COEFF_RELOAD_TRUE,
                            2,
                            DUAL_IP_DUAL,
                            USE_STREAM_API,
                            TP_MODIFY_MARGIN_OFFSET,
                            TP_COEFF_PHASE,
                            TP_COEFF_PHASE_OFFSET,
                            TP_COEFF_PHASES,
                            TP_COEFF_PHASES_LEN,
                            TP_SAT>(taps) {}

    static void registerKernelClass() { REGISTER_FUNCTION(fir_decimate_asym::filter); }

    void filter(input_stream<TT_DATA>* inStream,
                input_stream<TT_DATA>* inStream2,
                output_stream<TT_DATA>* outStream,
                output_stream<TT_DATA>* outStream2,
                const TT_COEFF (&inTaps)[TP_COEFF_PHASES_LEN]);
};

//-----------------------------------------------------------------------------------------------------
// Partially specialized classes for cascaded interface - final kernel. Static coefficients single output
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_DECIMATE_FACTOR,
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

class fir_decimate_asym<TT_DATA,
                        TT_COEFF,
                        TP_FIR_LEN,
                        TP_DECIMATE_FACTOR,
                        TP_SHIFT,
                        TP_RND,
                        TP_INPUT_WINDOW_VSIZE,
                        CASC_IN_TRUE,
                        CASC_OUT_FALSE,
                        TP_FIR_RANGE_LEN,
                        TP_KERNEL_POSITION,
                        TP_CASC_LEN,
                        USE_COEFF_RELOAD_FALSE,
                        1,
                        DUAL_IP_DUAL,
                        USE_STREAM_API,
                        TP_MODIFY_MARGIN_OFFSET,
                        TP_COEFF_PHASE,
                        TP_COEFF_PHASE_OFFSET,
                        TP_COEFF_PHASES,
                        TP_COEFF_PHASES_LEN,
                        TP_SAT> : public kernelFilterClass<TT_DATA,
                                                           TT_COEFF,
                                                           TP_FIR_LEN,
                                                           TP_DECIMATE_FACTOR,
                                                           TP_SHIFT,
                                                           TP_RND,
                                                           TP_INPUT_WINDOW_VSIZE,
                                                           CASC_IN_TRUE,
                                                           CASC_OUT_FALSE,
                                                           TP_FIR_RANGE_LEN,
                                                           TP_KERNEL_POSITION,
                                                           TP_CASC_LEN,
                                                           USE_COEFF_RELOAD_FALSE,
                                                           1,
                                                           DUAL_IP_DUAL,
                                                           USE_STREAM_API,
                                                           TP_MODIFY_MARGIN_OFFSET,
                                                           TP_COEFF_PHASE,
                                                           TP_COEFF_PHASE_OFFSET,
                                                           TP_COEFF_PHASES,
                                                           TP_COEFF_PHASES_LEN,
                                                           TP_SAT> {
   public:
    fir_decimate_asym(const TT_COEFF (&taps)[TP_FIR_LEN])
        : kernelFilterClass<TT_DATA,
                            TT_COEFF,
                            TP_FIR_LEN,
                            TP_DECIMATE_FACTOR,
                            TP_SHIFT,
                            TP_RND,
                            TP_INPUT_WINDOW_VSIZE,
                            CASC_IN_TRUE,
                            CASC_OUT_FALSE,
                            TP_FIR_RANGE_LEN,
                            TP_KERNEL_POSITION,
                            TP_CASC_LEN,
                            USE_COEFF_RELOAD_FALSE,
                            1,
                            DUAL_IP_DUAL,
                            USE_STREAM_API,
                            TP_MODIFY_MARGIN_OFFSET,
                            TP_COEFF_PHASE,
                            TP_COEFF_PHASE_OFFSET,
                            TP_COEFF_PHASES,
                            TP_COEFF_PHASES_LEN,
                            TP_SAT>(taps) {}

    static void registerKernelClass() { REGISTER_FUNCTION(fir_decimate_asym::filter); }

    void filter(input_stream<TT_DATA>* inStream,
                input_stream<TT_DATA>* inStream2,
                input_cascade_cacc* inCascade,
                output_stream<TT_DATA>* outStream);
};

// Partially specialized classes for cascaded interface - final kernel. Static coefficients, dual output
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_DECIMATE_FACTOR,
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

class fir_decimate_asym<TT_DATA,
                        TT_COEFF,
                        TP_FIR_LEN,
                        TP_DECIMATE_FACTOR,
                        TP_SHIFT,
                        TP_RND,
                        TP_INPUT_WINDOW_VSIZE,
                        CASC_IN_TRUE,
                        CASC_OUT_FALSE,
                        TP_FIR_RANGE_LEN,
                        TP_KERNEL_POSITION,
                        TP_CASC_LEN,
                        USE_COEFF_RELOAD_FALSE,
                        2,
                        DUAL_IP_DUAL,
                        USE_STREAM_API,
                        TP_MODIFY_MARGIN_OFFSET,
                        TP_COEFF_PHASE,
                        TP_COEFF_PHASE_OFFSET,
                        TP_COEFF_PHASES,
                        TP_COEFF_PHASES_LEN,
                        TP_SAT> : public kernelFilterClass<TT_DATA,
                                                           TT_COEFF,
                                                           TP_FIR_LEN,
                                                           TP_DECIMATE_FACTOR,
                                                           TP_SHIFT,
                                                           TP_RND,
                                                           TP_INPUT_WINDOW_VSIZE,
                                                           CASC_IN_TRUE,
                                                           CASC_OUT_FALSE,
                                                           TP_FIR_RANGE_LEN,
                                                           TP_KERNEL_POSITION,
                                                           TP_CASC_LEN,
                                                           USE_COEFF_RELOAD_FALSE,
                                                           2,
                                                           DUAL_IP_DUAL,
                                                           USE_STREAM_API,
                                                           TP_MODIFY_MARGIN_OFFSET,
                                                           TP_COEFF_PHASE,
                                                           TP_COEFF_PHASE_OFFSET,
                                                           TP_COEFF_PHASES,
                                                           TP_COEFF_PHASES_LEN,
                                                           TP_SAT> {
   public:
    fir_decimate_asym(const TT_COEFF (&taps)[TP_FIR_LEN])
        : kernelFilterClass<TT_DATA,
                            TT_COEFF,
                            TP_FIR_LEN,
                            TP_DECIMATE_FACTOR,
                            TP_SHIFT,
                            TP_RND,
                            TP_INPUT_WINDOW_VSIZE,
                            CASC_IN_TRUE,
                            CASC_OUT_FALSE,
                            TP_FIR_RANGE_LEN,
                            TP_KERNEL_POSITION,
                            TP_CASC_LEN,
                            USE_COEFF_RELOAD_FALSE,
                            2,
                            DUAL_IP_DUAL,
                            USE_STREAM_API,
                            TP_MODIFY_MARGIN_OFFSET,
                            TP_COEFF_PHASE,
                            TP_COEFF_PHASE_OFFSET,
                            TP_COEFF_PHASES,
                            TP_COEFF_PHASES_LEN,
                            TP_SAT>(taps) {}

    static void registerKernelClass() { REGISTER_FUNCTION(fir_decimate_asym::filter); }

    void filter(input_stream<TT_DATA>* inStream,
                input_stream<TT_DATA>* inStream2,
                input_cascade_cacc* inCascade,
                output_stream<TT_DATA>* outStream,
                output_stream<TT_DATA>* outStream2);
};

//-----------------------------------------------------------------------------------------------------
// Partially specialized classes for cascaded interface - first kernel. Static coefficients
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_DECIMATE_FACTOR,
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

class fir_decimate_asym<TT_DATA,
                        TT_COEFF,
                        TP_FIR_LEN,
                        TP_DECIMATE_FACTOR,
                        TP_SHIFT,
                        TP_RND,
                        TP_INPUT_WINDOW_VSIZE,
                        CASC_IN_FALSE,
                        CASC_OUT_TRUE,
                        TP_FIR_RANGE_LEN,
                        TP_KERNEL_POSITION,
                        TP_CASC_LEN,
                        USE_COEFF_RELOAD_FALSE,
                        1,
                        DUAL_IP_DUAL,
                        USE_STREAM_API,
                        TP_MODIFY_MARGIN_OFFSET,
                        TP_COEFF_PHASE,
                        TP_COEFF_PHASE_OFFSET,
                        TP_COEFF_PHASES,
                        TP_COEFF_PHASES_LEN,
                        TP_SAT> : public kernelFilterClass<TT_DATA,
                                                           TT_COEFF,
                                                           TP_FIR_LEN,
                                                           TP_DECIMATE_FACTOR,
                                                           TP_SHIFT,
                                                           TP_RND,
                                                           TP_INPUT_WINDOW_VSIZE,
                                                           CASC_IN_FALSE,
                                                           CASC_OUT_TRUE,
                                                           TP_FIR_RANGE_LEN,
                                                           TP_KERNEL_POSITION,
                                                           TP_CASC_LEN,
                                                           USE_COEFF_RELOAD_FALSE,
                                                           1,
                                                           DUAL_IP_DUAL,
                                                           USE_STREAM_API,
                                                           TP_MODIFY_MARGIN_OFFSET,
                                                           TP_COEFF_PHASE,
                                                           TP_COEFF_PHASE_OFFSET,
                                                           TP_COEFF_PHASES,
                                                           TP_COEFF_PHASES_LEN,
                                                           TP_SAT> {
   public:
    fir_decimate_asym(const TT_COEFF (&taps)[TP_FIR_LEN])
        : kernelFilterClass<TT_DATA,
                            TT_COEFF,
                            TP_FIR_LEN,
                            TP_DECIMATE_FACTOR,
                            TP_SHIFT,
                            TP_RND,
                            TP_INPUT_WINDOW_VSIZE,
                            CASC_IN_FALSE,
                            CASC_OUT_TRUE,
                            TP_FIR_RANGE_LEN,
                            TP_KERNEL_POSITION,
                            TP_CASC_LEN,
                            USE_COEFF_RELOAD_FALSE,
                            1,
                            DUAL_IP_DUAL,
                            USE_STREAM_API,
                            TP_MODIFY_MARGIN_OFFSET,
                            TP_COEFF_PHASE,
                            TP_COEFF_PHASE_OFFSET,
                            TP_COEFF_PHASES,
                            TP_COEFF_PHASES_LEN,
                            TP_SAT>(taps) {}

    static void registerKernelClass() { REGISTER_FUNCTION(fir_decimate_asym::filter); }

    void filter(input_stream<TT_DATA>* inStream, input_stream<TT_DATA>* inStream2, output_cascade_cacc* outCascade);
};

//-----------------------------------------------------------------------------------------------------
// Partially specialized classes for cascaded interface - middle kernel. Static coefficients
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_DECIMATE_FACTOR,
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

class fir_decimate_asym<TT_DATA,
                        TT_COEFF,
                        TP_FIR_LEN,
                        TP_DECIMATE_FACTOR,
                        TP_SHIFT,
                        TP_RND,
                        TP_INPUT_WINDOW_VSIZE,
                        CASC_IN_TRUE,
                        CASC_OUT_TRUE,
                        TP_FIR_RANGE_LEN,
                        TP_KERNEL_POSITION,
                        TP_CASC_LEN,
                        USE_COEFF_RELOAD_FALSE,
                        1,
                        DUAL_IP_DUAL,
                        USE_STREAM_API,
                        TP_MODIFY_MARGIN_OFFSET,
                        TP_COEFF_PHASE,
                        TP_COEFF_PHASE_OFFSET,
                        TP_COEFF_PHASES,
                        TP_COEFF_PHASES_LEN,
                        TP_SAT> : public kernelFilterClass<TT_DATA,
                                                           TT_COEFF,
                                                           TP_FIR_LEN,
                                                           TP_DECIMATE_FACTOR,
                                                           TP_SHIFT,
                                                           TP_RND,
                                                           TP_INPUT_WINDOW_VSIZE,
                                                           CASC_IN_TRUE,
                                                           CASC_OUT_TRUE,
                                                           TP_FIR_RANGE_LEN,
                                                           TP_KERNEL_POSITION,
                                                           TP_CASC_LEN,
                                                           USE_COEFF_RELOAD_FALSE,
                                                           1,
                                                           DUAL_IP_DUAL,
                                                           USE_STREAM_API,
                                                           TP_MODIFY_MARGIN_OFFSET,
                                                           TP_COEFF_PHASE,
                                                           TP_COEFF_PHASE_OFFSET,
                                                           TP_COEFF_PHASES,
                                                           TP_COEFF_PHASES_LEN,
                                                           TP_SAT> {
   public:
    fir_decimate_asym(const TT_COEFF (&taps)[TP_FIR_LEN])
        : kernelFilterClass<TT_DATA,
                            TT_COEFF,
                            TP_FIR_LEN,
                            TP_DECIMATE_FACTOR,
                            TP_SHIFT,
                            TP_RND,
                            TP_INPUT_WINDOW_VSIZE,
                            CASC_IN_TRUE,
                            CASC_OUT_TRUE,
                            TP_FIR_RANGE_LEN,
                            TP_KERNEL_POSITION,
                            TP_CASC_LEN,
                            USE_COEFF_RELOAD_FALSE,
                            1,
                            DUAL_IP_DUAL,
                            USE_STREAM_API,
                            TP_MODIFY_MARGIN_OFFSET,
                            TP_COEFF_PHASE,
                            TP_COEFF_PHASE_OFFSET,
                            TP_COEFF_PHASES,
                            TP_COEFF_PHASES_LEN,
                            TP_SAT>(taps) {}

    static void registerKernelClass() { REGISTER_FUNCTION(fir_decimate_asym::filter); }

    void filter(input_stream<TT_DATA>* inStream,
                input_stream<TT_DATA>* inStream2,
                input_cascade_cacc* inCascade,
                output_cascade_cacc* outCascade);
};

//-----------------------------------------------------------------------------------------------------
// Partially specialized classes for cascaded interface - final kernel. Re-loadable coefficients
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_DECIMATE_FACTOR,
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

class fir_decimate_asym<TT_DATA,
                        TT_COEFF,
                        TP_FIR_LEN,
                        TP_DECIMATE_FACTOR,
                        TP_SHIFT,
                        TP_RND,
                        TP_INPUT_WINDOW_VSIZE,
                        CASC_IN_TRUE,
                        CASC_OUT_FALSE,
                        TP_FIR_RANGE_LEN,
                        TP_KERNEL_POSITION,
                        TP_CASC_LEN,
                        USE_COEFF_RELOAD_TRUE,
                        1,
                        DUAL_IP_DUAL,
                        USE_STREAM_API,
                        TP_MODIFY_MARGIN_OFFSET,
                        TP_COEFF_PHASE,
                        TP_COEFF_PHASE_OFFSET,
                        TP_COEFF_PHASES,
                        TP_COEFF_PHASES_LEN,
                        TP_SAT> : public kernelFilterClass<TT_DATA,
                                                           TT_COEFF,
                                                           TP_FIR_LEN,
                                                           TP_DECIMATE_FACTOR,
                                                           TP_SHIFT,
                                                           TP_RND,
                                                           TP_INPUT_WINDOW_VSIZE,
                                                           CASC_IN_TRUE,
                                                           CASC_OUT_FALSE,
                                                           TP_FIR_RANGE_LEN,
                                                           TP_KERNEL_POSITION,
                                                           TP_CASC_LEN,
                                                           USE_COEFF_RELOAD_TRUE,
                                                           1,
                                                           DUAL_IP_DUAL,
                                                           USE_STREAM_API,
                                                           TP_MODIFY_MARGIN_OFFSET,
                                                           TP_COEFF_PHASE,
                                                           TP_COEFF_PHASE_OFFSET,
                                                           TP_COEFF_PHASES,
                                                           TP_COEFF_PHASES_LEN,
                                                           TP_SAT> {
   public:
    fir_decimate_asym()
        : kernelFilterClass<TT_DATA,
                            TT_COEFF,
                            TP_FIR_LEN,
                            TP_DECIMATE_FACTOR,
                            TP_SHIFT,
                            TP_RND,
                            TP_INPUT_WINDOW_VSIZE,
                            CASC_IN_TRUE,
                            CASC_OUT_FALSE,
                            TP_FIR_RANGE_LEN,
                            TP_KERNEL_POSITION,
                            TP_CASC_LEN,
                            USE_COEFF_RELOAD_TRUE,
                            1,
                            DUAL_IP_DUAL,
                            USE_STREAM_API,
                            TP_MODIFY_MARGIN_OFFSET,
                            TP_COEFF_PHASE,
                            TP_COEFF_PHASE_OFFSET,
                            TP_COEFF_PHASES,
                            TP_COEFF_PHASES_LEN,
                            TP_SAT>() {}
    fir_decimate_asym(const TT_COEFF (&taps)[TP_FIR_LEN])
        : kernelFilterClass<TT_DATA,
                            TT_COEFF,
                            TP_FIR_LEN,
                            TP_DECIMATE_FACTOR,
                            TP_SHIFT,
                            TP_RND,
                            TP_INPUT_WINDOW_VSIZE,
                            CASC_IN_TRUE,
                            CASC_OUT_FALSE,
                            TP_FIR_RANGE_LEN,
                            TP_KERNEL_POSITION,
                            TP_CASC_LEN,
                            USE_COEFF_RELOAD_TRUE,
                            1,
                            DUAL_IP_DUAL,
                            USE_STREAM_API,
                            TP_MODIFY_MARGIN_OFFSET,
                            TP_COEFF_PHASE,
                            TP_COEFF_PHASE_OFFSET,
                            TP_COEFF_PHASES,
                            TP_COEFF_PHASES_LEN,
                            TP_SAT>(taps) {}

    static void registerKernelClass() { REGISTER_FUNCTION(fir_decimate_asym::filter); }

    void filter(input_stream<TT_DATA>* inStream,
                input_stream<TT_DATA>* inStream2,
                input_cascade_cacc* inCascade,
                output_stream<TT_DATA>* outStream);
};

// Partially specialized classes for cascaded interface - final kernel. Re-loadable coefficients, dual output
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_DECIMATE_FACTOR,
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

class fir_decimate_asym<TT_DATA,
                        TT_COEFF,
                        TP_FIR_LEN,
                        TP_DECIMATE_FACTOR,
                        TP_SHIFT,
                        TP_RND,
                        TP_INPUT_WINDOW_VSIZE,
                        CASC_IN_TRUE,
                        CASC_OUT_FALSE,
                        TP_FIR_RANGE_LEN,
                        TP_KERNEL_POSITION,
                        TP_CASC_LEN,
                        USE_COEFF_RELOAD_TRUE,
                        2,
                        DUAL_IP_DUAL,
                        USE_STREAM_API,
                        TP_MODIFY_MARGIN_OFFSET,
                        TP_COEFF_PHASE,
                        TP_COEFF_PHASE_OFFSET,
                        TP_COEFF_PHASES,
                        TP_COEFF_PHASES_LEN,
                        TP_SAT> : public kernelFilterClass<TT_DATA,
                                                           TT_COEFF,
                                                           TP_FIR_LEN,
                                                           TP_DECIMATE_FACTOR,
                                                           TP_SHIFT,
                                                           TP_RND,
                                                           TP_INPUT_WINDOW_VSIZE,
                                                           CASC_IN_TRUE,
                                                           CASC_OUT_FALSE,
                                                           TP_FIR_RANGE_LEN,
                                                           TP_KERNEL_POSITION,
                                                           TP_CASC_LEN,
                                                           USE_COEFF_RELOAD_TRUE,
                                                           2,
                                                           DUAL_IP_DUAL,
                                                           USE_STREAM_API,
                                                           TP_MODIFY_MARGIN_OFFSET,
                                                           TP_COEFF_PHASE,
                                                           TP_COEFF_PHASE_OFFSET,
                                                           TP_COEFF_PHASES,
                                                           TP_COEFF_PHASES_LEN,
                                                           TP_SAT> {
   public:
    fir_decimate_asym()
        : kernelFilterClass<TT_DATA,
                            TT_COEFF,
                            TP_FIR_LEN,
                            TP_DECIMATE_FACTOR,
                            TP_SHIFT,
                            TP_RND,
                            TP_INPUT_WINDOW_VSIZE,
                            CASC_IN_TRUE,
                            CASC_OUT_FALSE,
                            TP_FIR_RANGE_LEN,
                            TP_KERNEL_POSITION,
                            TP_CASC_LEN,
                            USE_COEFF_RELOAD_TRUE,
                            2,
                            DUAL_IP_DUAL,
                            USE_STREAM_API,
                            TP_MODIFY_MARGIN_OFFSET,
                            TP_COEFF_PHASE,
                            TP_COEFF_PHASE_OFFSET,
                            TP_COEFF_PHASES,
                            TP_COEFF_PHASES_LEN,
                            TP_SAT>() {}
    fir_decimate_asym(const TT_COEFF (&taps)[TP_FIR_LEN])
        : kernelFilterClass<TT_DATA,
                            TT_COEFF,
                            TP_FIR_LEN,
                            TP_DECIMATE_FACTOR,
                            TP_SHIFT,
                            TP_RND,
                            TP_INPUT_WINDOW_VSIZE,
                            CASC_IN_TRUE,
                            CASC_OUT_FALSE,
                            TP_FIR_RANGE_LEN,
                            TP_KERNEL_POSITION,
                            TP_CASC_LEN,
                            USE_COEFF_RELOAD_TRUE,
                            2,
                            DUAL_IP_DUAL,
                            USE_STREAM_API,
                            TP_MODIFY_MARGIN_OFFSET,
                            TP_COEFF_PHASE,
                            TP_COEFF_PHASE_OFFSET,
                            TP_COEFF_PHASES,
                            TP_COEFF_PHASES_LEN,
                            TP_SAT>(taps) {}

    static void registerKernelClass() { REGISTER_FUNCTION(fir_decimate_asym::filter); }

    void filter(input_stream<TT_DATA>* inStream,
                input_stream<TT_DATA>* inStream2,
                input_cascade_cacc* inCascade,
                output_stream<TT_DATA>* outStream,
                output_stream<TT_DATA>* outStream2);
};

//-----------------------------------------------------------------------------------------------------
// Partially specialized classes for cascaded interface - first kernel. Re-loadable coefficients
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_DECIMATE_FACTOR,
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

class fir_decimate_asym<TT_DATA,
                        TT_COEFF,
                        TP_FIR_LEN,
                        TP_DECIMATE_FACTOR,
                        TP_SHIFT,
                        TP_RND,
                        TP_INPUT_WINDOW_VSIZE,
                        CASC_IN_FALSE,
                        CASC_OUT_TRUE,
                        TP_FIR_RANGE_LEN,
                        TP_KERNEL_POSITION,
                        TP_CASC_LEN,
                        USE_COEFF_RELOAD_TRUE,
                        1,
                        DUAL_IP_DUAL,
                        USE_STREAM_API,
                        TP_MODIFY_MARGIN_OFFSET,
                        TP_COEFF_PHASE,
                        TP_COEFF_PHASE_OFFSET,
                        TP_COEFF_PHASES,
                        TP_COEFF_PHASES_LEN,
                        TP_SAT> : public kernelFilterClass<TT_DATA,
                                                           TT_COEFF,
                                                           TP_FIR_LEN,
                                                           TP_DECIMATE_FACTOR,
                                                           TP_SHIFT,
                                                           TP_RND,
                                                           TP_INPUT_WINDOW_VSIZE,
                                                           CASC_IN_FALSE,
                                                           CASC_OUT_TRUE,
                                                           TP_FIR_RANGE_LEN,
                                                           TP_KERNEL_POSITION,
                                                           TP_CASC_LEN,
                                                           USE_COEFF_RELOAD_TRUE,
                                                           1,
                                                           DUAL_IP_DUAL,
                                                           USE_STREAM_API,
                                                           TP_MODIFY_MARGIN_OFFSET,
                                                           TP_COEFF_PHASE,
                                                           TP_COEFF_PHASE_OFFSET,
                                                           TP_COEFF_PHASES,
                                                           TP_COEFF_PHASES_LEN,
                                                           TP_SAT> {
   public:
    fir_decimate_asym()
        : kernelFilterClass<TT_DATA,
                            TT_COEFF,
                            TP_FIR_LEN,
                            TP_DECIMATE_FACTOR,
                            TP_SHIFT,
                            TP_RND,
                            TP_INPUT_WINDOW_VSIZE,
                            CASC_IN_FALSE,
                            CASC_OUT_TRUE,
                            TP_FIR_RANGE_LEN,
                            TP_KERNEL_POSITION,
                            TP_CASC_LEN,
                            USE_COEFF_RELOAD_TRUE,
                            1,
                            DUAL_IP_DUAL,
                            USE_STREAM_API,
                            TP_MODIFY_MARGIN_OFFSET,
                            TP_COEFF_PHASE,
                            TP_COEFF_PHASE_OFFSET,
                            TP_COEFF_PHASES,
                            TP_COEFF_PHASES_LEN,
                            TP_SAT>() {}
    fir_decimate_asym(const TT_COEFF (&taps)[TP_FIR_LEN])
        : kernelFilterClass<TT_DATA,
                            TT_COEFF,
                            TP_FIR_LEN,
                            TP_DECIMATE_FACTOR,
                            TP_SHIFT,
                            TP_RND,
                            TP_INPUT_WINDOW_VSIZE,
                            CASC_IN_FALSE,
                            CASC_OUT_TRUE,
                            TP_FIR_RANGE_LEN,
                            TP_KERNEL_POSITION,
                            TP_CASC_LEN,
                            USE_COEFF_RELOAD_TRUE,
                            1,
                            DUAL_IP_DUAL,
                            USE_STREAM_API,
                            TP_MODIFY_MARGIN_OFFSET,
                            TP_COEFF_PHASE,
                            TP_COEFF_PHASE_OFFSET,
                            TP_COEFF_PHASES,
                            TP_COEFF_PHASES_LEN,
                            TP_SAT>(taps) {}

    static void registerKernelClass() { REGISTER_FUNCTION(fir_decimate_asym::filter); }

    void filter(input_stream<TT_DATA>* inStream,
                input_stream<TT_DATA>* inStream2,
                output_cascade_cacc* outCascade,
                const TT_COEFF (&inTaps)[TP_COEFF_PHASES_LEN]);
};

//-----------------------------------------------------------------------------------------------------
// Partially specialized classes for cascaded interface - middle kernel. Reeloadable coefficients
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_DECIMATE_FACTOR,
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

class fir_decimate_asym<TT_DATA,
                        TT_COEFF,
                        TP_FIR_LEN,
                        TP_DECIMATE_FACTOR,
                        TP_SHIFT,
                        TP_RND,
                        TP_INPUT_WINDOW_VSIZE,
                        CASC_IN_TRUE,
                        CASC_OUT_TRUE,
                        TP_FIR_RANGE_LEN,
                        TP_KERNEL_POSITION,
                        TP_CASC_LEN,
                        USE_COEFF_RELOAD_TRUE,
                        1,
                        DUAL_IP_DUAL,
                        USE_STREAM_API,
                        TP_MODIFY_MARGIN_OFFSET,
                        TP_COEFF_PHASE,
                        TP_COEFF_PHASE_OFFSET,
                        TP_COEFF_PHASES,
                        TP_COEFF_PHASES_LEN,
                        TP_SAT> : public kernelFilterClass<TT_DATA,
                                                           TT_COEFF,
                                                           TP_FIR_LEN,
                                                           TP_DECIMATE_FACTOR,
                                                           TP_SHIFT,
                                                           TP_RND,
                                                           TP_INPUT_WINDOW_VSIZE,
                                                           CASC_IN_TRUE,
                                                           CASC_OUT_TRUE,
                                                           TP_FIR_RANGE_LEN,
                                                           TP_KERNEL_POSITION,
                                                           TP_CASC_LEN,
                                                           USE_COEFF_RELOAD_TRUE,
                                                           1,
                                                           DUAL_IP_DUAL,
                                                           USE_STREAM_API,
                                                           TP_MODIFY_MARGIN_OFFSET,
                                                           TP_COEFF_PHASE,
                                                           TP_COEFF_PHASE_OFFSET,
                                                           TP_COEFF_PHASES,
                                                           TP_COEFF_PHASES_LEN,
                                                           TP_SAT> {
   public:
    fir_decimate_asym()
        : kernelFilterClass<TT_DATA,
                            TT_COEFF,
                            TP_FIR_LEN,
                            TP_DECIMATE_FACTOR,
                            TP_SHIFT,
                            TP_RND,
                            TP_INPUT_WINDOW_VSIZE,
                            CASC_IN_TRUE,
                            CASC_OUT_TRUE,
                            TP_FIR_RANGE_LEN,
                            TP_KERNEL_POSITION,
                            TP_CASC_LEN,
                            USE_COEFF_RELOAD_TRUE,
                            1,
                            DUAL_IP_DUAL,
                            USE_STREAM_API,
                            TP_MODIFY_MARGIN_OFFSET,
                            TP_COEFF_PHASE,
                            TP_COEFF_PHASE_OFFSET,
                            TP_COEFF_PHASES,
                            TP_COEFF_PHASES_LEN,
                            TP_SAT>() {}
    fir_decimate_asym(const TT_COEFF (&taps)[TP_FIR_LEN])
        : kernelFilterClass<TT_DATA,
                            TT_COEFF,
                            TP_FIR_LEN,
                            TP_DECIMATE_FACTOR,
                            TP_SHIFT,
                            TP_RND,
                            TP_INPUT_WINDOW_VSIZE,
                            CASC_IN_TRUE,
                            CASC_OUT_TRUE,
                            TP_FIR_RANGE_LEN,
                            TP_KERNEL_POSITION,
                            TP_CASC_LEN,
                            USE_COEFF_RELOAD_TRUE,
                            1,
                            DUAL_IP_DUAL,
                            USE_STREAM_API,
                            TP_MODIFY_MARGIN_OFFSET,
                            TP_COEFF_PHASE,
                            TP_COEFF_PHASE_OFFSET,
                            TP_COEFF_PHASES,
                            TP_COEFF_PHASES_LEN,
                            TP_SAT>(taps) {}

    static void registerKernelClass() { REGISTER_FUNCTION(fir_decimate_asym::filter); }

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
#endif // _DSPLIB_FIR_DECIMATE_ASYM_HPP_
