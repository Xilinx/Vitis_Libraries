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
#ifndef _DSPLIB_fir_decimate_hb_asym_HPP_
#define _DSPLIB_fir_decimate_hb_asym_HPP_

/*
    Half band decimatiion FIR filter.
    This file exists to capture the definition of the FIR filter kernel class.
    The class definition holds defensive checks on parameter range and other
    legality.
    The constructor definition is held in this class because this class must be
    accessible to graph level aie compilation.
    The main runtime filter function is captured elsewhere as it contains aie
    intrinsics which are not included in aie graph level compilation.

    TT_      template type suffix
    TP_      template parameter suffix
*/

#include <adf.h>
#include "fir_utils.hpp"
#include "fir_decimate_hb_traits.hpp"
#include "fir_sr_asym.hpp"

#define K_BITS_IN_BYTE 8
#define K_MAX_PASSES 2

//#define _DSPLIB_FIR_DECIMATE_HB_ASYM_HPP_DEBUG_

namespace xf {
namespace dsp {
namespace aie {
namespace fir {
namespace decimate_hb_asym {
using namespace decimate_hb;

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
          unsigned int TP_DUAL_IP = 0,
          unsigned int TP_USE_COEFF_RELOAD = 0,
          unsigned int TP_NUM_OUTPUTS = 1,
          unsigned int TP_API = 0,
          unsigned int TP_SAT = 1>
class fir_decimate_hb_asym;

template <typename fp = fir_params_defaults>
class fir_decimate_hb_asym_tl {
   public:
    // Get kernel's FIR range Length
    template <int pos>
    static constexpr unsigned int getKernelFirRangeLen() {
        constexpr unsigned int firRangeLen = (pos + 1 == fp::BTP_CASC_LEN)
                                                 ? fnFirRangeRem<fp::BTP_FIR_LEN, fp::BTP_CASC_LEN, pos, 2>()
                                                 : fnFirRange<fp::BTP_FIR_LEN, fp::BTP_CASC_LEN, pos, 2>();

        return firRangeLen;
    };

    // Get kernel's FIR range Length
    static constexpr unsigned int getFirRangeLen() { return fp::BTP_FIR_RANGE_LEN; };

    // Get kernel's FIR Total Tap Length
    static constexpr unsigned int getTapLen() {
#if __HAS_SYM_PREADD__ == 1
        return (fp::BTP_FIR_LEN + 1) / 4 + 1;
#else
        return (fp::BTP_FIR_LEN + 1) / 2 + 1;
#endif
    };

    // Get kernel's FIR Total Tap Length
    static constexpr unsigned int getSSRMargin() {
        constexpr unsigned int margin = fnFirMargin<fp::BTP_FIR_LEN / fp::BTP_SSR, typename fp::BTT_DATA>();
        return margin;
    };
    template <int pos, int CLEN, int T_FIR_LEN, typename T_D, typename T_C>
    static constexpr unsigned int fnCheckIfFits() {
        constexpr int samplesInBuff = fnSamplesIn1024<T_D>();
        constexpr unsigned int firRangeLen = getKernelFirRangeLen<pos>();
        constexpr unsigned int numTaps = (pos == CLEN - 1) ? (firRangeLen + 1) / 2 : firRangeLen / 2;
        constexpr unsigned int dataLoadVSize = 256 / 8 / sizeof(T_D);
        constexpr unsigned int firRangeOffset = fnFirRangeOffset<T_FIR_LEN, CLEN, pos, 2>() / kDecimateFactor; //
        constexpr int streamInitNullLanes = ((T_FIR_LEN - firRangeLen - firRangeOffset * kDecimateFactor + 1) / 2);
        constexpr unsigned int lanes = fnNumLanes384<T_D, T_C>();
        constexpr int streamInitNullAccs = streamInitNullLanes / lanes;
        constexpr int streamRptFactor = samplesInBuff / lanes;
        constexpr int streamInitAccs = (CEIL(streamInitNullAccs, streamRptFactor) - streamInitNullAccs);
        constexpr int cascOffset = streamInitNullLanes - streamInitNullAccs * lanes;
        constexpr unsigned int m_kInitDataNeeded = numTaps + cascOffset + dataLoadVSize - 1;
        if
            constexpr(m_kInitDataNeeded > samplesInBuff) { return 0; }
        else {
            return 1;
        }
    };
    static constexpr unsigned int getDF() { return 1; };
    static constexpr unsigned int getIF() { return 1; };

    // Get FIR variant
    static constexpr eFIRVariant getFirType() { return eFIRVariant::kDecHB; };
    using parent_class = fir_decimate_hb_asym<typename fp::BTT_DATA,
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
                                              fp::BTP_DUAL_IP,
                                              fp::BTP_USE_COEFF_RELOAD,
                                              fp::BTP_NUM_OUTPUTS,
                                              fp::BTP_API,
                                              fp::BTP_SAT>;
};
//-----------------------------------------------------------------------------------------------------
// Half band decimation FIR class definition.
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
          unsigned int TP_DUAL_IP = 0,
          unsigned int TP_USE_COEFF_RELOAD = 0,
          unsigned int TP_NUM_OUTPUTS = 1,
          unsigned int TP_API = 0,
          unsigned int TP_SAT = 1>
class kernelFilterClass {
   private:
    // Parameter value defensive and legality checks
    static_assert(TP_FIR_RANGE_LEN >= FIR_LEN_MIN,
                  "ERROR: Illegal combination of design FIR length and cascade length, resulting in kernel FIR length "
                  "below minimum required value. ");
    static_assert(((TP_FIR_LEN + 1) % 4) == 0, "ERROR: TP_FIR_LEN must be 4N-1 where N is a positive integer.");
    static_assert(TP_SHIFT >= SHIFT_MIN && TP_SHIFT <= SHIFT_MAX, "ERROR: TP_SHIFT is out of the supported range.");
    static_assert(TP_RND >= ROUND_MIN && TP_RND <= ROUND_MAX, "ERROR: TP_RND is out of the supported range.");
    static_assert(fnEnumType<TT_DATA>() != enumUnknownType, "ERROR: TT_DATA is not a supported type.");
    static_assert(fnEnumType<TT_COEFF>() != enumUnknownType, "ERROR: TT_COEFF is not a supported type.");
    static_assert(fnTypeCheckDataCoeffSize<TT_DATA, TT_COEFF>() != 0,
                  "ERROR: TT_DATA type less precise than TT_COEFF is not supported.");
    static_assert(fnTypeCheckDataCoeffCmplx<TT_DATA, TT_COEFF>() != 0,
                  "ERROR: real TT_DATA with complex TT_COEFF is not supported.");
    static_assert(fnTypeCheckDataCoeffFltInt<TT_DATA, TT_COEFF>() != 0,
                  "ERROR: a mix of float and integer types of TT_DATA and TT_COEFF is not supported.");
    static_assert(fnUnsupportedTypeComboFirDecHb<TT_DATA, TT_COEFF>() != 0,
                  "ERROR: The combination of TT_DATA and TT_COEFF is not supported for this class.");
    static_assert(TP_NUM_OUTPUTS > 0 && TP_NUM_OUTPUTS <= 2, "ERROR: only single or dual outputs are supported.");
    static_assert(!(std::is_same<TT_DATA, cfloat>::value || std::is_same<TT_DATA, float>::value) || (TP_SHIFT == 0),
                  "ERROR: TP_SHIFT cannot be performed for TT_DATA=cfloat, so must be set to 0");
    static_assert(TP_SAT >= SAT_MODE_MIN && TP_SAT <= SAT_MODE_MAX, "ERROR: TP_SAT is out of supported range");
    static_assert(TP_SAT != 2, "ERROR: TP_SAT is invalid. Valid values of TP_SAT are 0, 1, and 3");

    static constexpr unsigned int m_kNumAsymPh1Taps =
        (TP_KERNEL_POSITION == TP_CASC_LEN - 1) ? (TP_FIR_RANGE_LEN + 1) / 2 : TP_FIR_RANGE_LEN / 2;
    static constexpr unsigned int m_kTotTaps =
        (TP_FIR_LEN + 1) / kHbFactor + 1;                         // factor of 2 is because of half-band zero-ed taps
    static constexpr unsigned int m_kTotPh1Taps = m_kTotTaps - 1; //
    static constexpr unsigned int m_kDataReadGranByte = __MIN_REGSIZE__; // Data reads occur on (128/256 bit boundary)
    static constexpr unsigned int m_kFirRangeOffset =
        fnFirRangeOffset<TP_FIR_LEN, TP_CASC_LEN, TP_KERNEL_POSITION, kHbFactor>() / kDecimateFactor; //
    static constexpr unsigned int m_kFirMarginLen = fnFirMargin<TP_FIR_LEN, TT_DATA>();
    static constexpr unsigned int m_kFirMarginOffset =
        fnFirMargin<TP_FIR_LEN, TT_DATA>() / kDecimateFactor - m_kTotPh1Taps + 1; // FIR Margin Offset.
    static constexpr unsigned int m_kFirInitOffset = (m_kFirRangeOffset + m_kFirMarginOffset) * kDecimateFactor; //
    static constexpr unsigned int m_kDataWindowOffset =
        TRUNC(m_kFirInitOffset, (m_kDataReadGranByte / 8 / sizeof(TT_DATA))); // Window offset - increments by 128bit
    static constexpr unsigned int m_kDataBuffXOffset =
        (m_kFirInitOffset % (m_kDataReadGranByte / 8 / sizeof(TT_DATA))) /
        2; // Remainder of m_kFirInitOffset divided by 128bit

    static constexpr unsigned int m_kArch = 0;
    static constexpr unsigned int m_kXbuffByteSize = 256;
    static constexpr unsigned int m_kLanes = fnNumLanes384<TT_DATA, TT_COEFF>();
    static constexpr unsigned int m_kColumns = fnNumCols<TT_DATA, TT_COEFF>();
    static constexpr unsigned int m_kZbuffSize = __MIN_REGSIZE__; // bytes
    static constexpr unsigned int m_kCoeffRegVsize = m_kZbuffSize / 8 / sizeof(TT_COEFF);
    static constexpr unsigned int m_kNumOps =
        CEIL((TP_FIR_RANGE_LEN + 1) / kDecimateFactor / kSymmetryFactor + 1, m_kColumns) /
        m_kColumns; //+1 for centre tap
    static constexpr unsigned int m_kDataLoadVsize = 256 / 8 / sizeof(TT_DATA);
    static constexpr unsigned int m_kSamplesInDataBuff = 1024 / 8 / sizeof(TT_DATA);
    static constexpr unsigned int m_kVOutSize = m_kLanes;
    static constexpr unsigned int m_kLsize =
        TP_INPUT_WINDOW_VSIZE / (m_kVOutSize * kDecimateFactor); // loops required to consume input
    static constexpr unsigned int m_kDataRegVsize =
        m_kXbuffByteSize / sizeof(TT_DATA); // sbuff samples, for small architecture
    static constexpr unsigned int m_kFirLenCeilCols = CEIL(m_kNumAsymPh1Taps, m_kColumns);
    static constexpr unsigned int m_kDataLoadsInReg = m_kSamplesInDataBuff / m_kDataLoadVsize;

    static constexpr unsigned int m_kInitDataNeeded =
        (m_kDataBuffXOffset + m_kLanes - 1 + m_kColumns) * kDecimateFactor;                                          //
    static constexpr unsigned int m_kInitialLoadsSt1 = CEIL(m_kInitDataNeeded, m_kDataLoadVsize) / m_kDataLoadVsize; //
    static constexpr unsigned int m_kInitialLoads = CEIL(m_kInitialLoadsSt1, 2);                                     //

    // The following variables calculate which is the right op at which enough data is available to calculate the low
    // polyphase so we can calculate the low polyphase.
    // This cannot just be done in the last op because the data might get overwritten.
    static constexpr unsigned int m_kNumCTZeroes = (TP_FIR_LEN + 1) / 4;
    static constexpr unsigned int m_kMinDataNeededLowPh =
        kDecimateFactor * (m_kFirMarginOffset + m_kNumCTZeroes + m_kLanes - 1) - m_kDataWindowOffset;
    static constexpr unsigned int m_kMinDataLoadedLowPh = CEIL(m_kMinDataNeededLowPh, m_kDataLoadVsize);
    static constexpr unsigned int m_kMinHighPhDataNeeded = m_kMinDataLoadedLowPh - m_kDataLoadVsize + 1;
    static constexpr int m_kMinOp =
        m_kMinHighPhDataNeeded < m_kInitDataNeeded
            ? 0
            : CEIL((m_kMinHighPhDataNeeded - m_kInitDataNeeded), kDecimateFactor) / kDecimateFactor;
    static constexpr int m_kCTOp = CEIL(m_kMinOp, m_kColumns);
    static constexpr bool m_kHasCT =
        (TP_CASC_LEN % 2 == 0)
            ? (TP_KERNEL_POSITION == (TP_CASC_LEN / 2 - 1)) ? true : false
            : (TP_KERNEL_POSITION == TP_CASC_LEN / 2)
                  ? true
                  : false; // the center kernel calculates the center tap and passes it on to the cascade
    alignas(__ALIGN_BYTE_SIZE__) TT_COEFF
        m_internalTaps[CEIL(m_kNumAsymPh1Taps, m_kCoeffRegVsize)]; // Filter taps/coefficients
    alignas(__ALIGN_BYTE_SIZE__) TT_COEFF m_phaseTwoTap[m_kCoeffRegVsize] = {
        nullElem<TT_COEFF>()}; // note, the array is initializeed, causing extra instructions during initialiation.

    // stream variables

    static constexpr int streamRptFactor = m_kSamplesInDataBuff / m_kLanes;
    static constexpr int streamInitNullLanes =
        ((TP_FIR_LEN - TP_FIR_RANGE_LEN - m_kFirRangeOffset * kDecimateFactor + 1) / 2);
    static constexpr int streamInitNullAccs = streamInitNullLanes / m_kLanes;
    static constexpr int streamInitAccs = CEIL(streamInitNullAccs, streamRptFactor) - streamInitNullAccs;
    static constexpr int cascOffset = streamInitNullLanes - streamInitNullAccs * m_kLanes;
    static constexpr int m_kDataBuffXOffsetStrm = m_kSamplesInDataBuff - m_kNumAsymPh1Taps + 1 - cascOffset;
    static constexpr unsigned int lowPhOffset =
        (m_kSamplesInDataBuff - m_kNumCTZeroes + streamInitNullAccs * m_kLanes) % m_kSamplesInDataBuff;
    static constexpr unsigned int mainLoopLowPhOffset = lowPhOffset + streamInitAccs * m_kLanes;
    static constexpr unsigned int samplesNeededInBuff = m_kDataLoadVsize + m_kNumAsymPh1Taps + cascOffset - 1;
    alignas(__ALIGN_BYTE_SIZE__) int storePh1Data[(1024 / 8) / sizeof(int)] = {
        0}; // 1024b buffer to retain margin data between calls
    alignas(__ALIGN_BYTE_SIZE__) int storePh2Data[(1024 / 8) / sizeof(int)] = {
        0}; // 1024b buffer to retain margin data between calls
    alignas(__ALIGN_BYTE_SIZE__) int delay[(1024 / 8) / sizeof(int)] = {
        0}; // 1024b buffer to retain margin data between calls
    int doInit = 1;
    // Constants for coefficient reload
    static constexpr unsigned int m_kCoeffLoadSize = 256 / 8 / sizeof(TT_COEFF);
    alignas(__ALIGN_BYTE_SIZE__) TT_COEFF
        m_oldInTaps[CEIL((TP_FIR_LEN + 1) / 4 + 1,
                         m_kCoeffLoadSize)]; // Previous user input coefficients with zero padding
    bool m_coeffnEq;                         // Are coefficients sets equal?
    void filterSelectArch(T_inputIF<TP_CASC_IN, TT_DATA, TP_DUAL_IP> inInterface,
                          T_outputIF<TP_CASC_OUT, TT_DATA> outInterface);
    // FIR filter function variant utilizing single 1024-bit buffer.
    void filterBasic(T_inputIF<TP_CASC_IN, TT_DATA, TP_DUAL_IP> inInterface,
                     T_outputIF<TP_CASC_OUT, TT_DATA> outInterface);
    void filterStream(T_inputIF<TP_CASC_IN, TT_DATA, TP_DUAL_IP> inInterface,
                      T_outputIF<TP_CASC_OUT, TT_DATA> outInterface);

    struct ssr_params : public fir_params_defaults {
        using BTT_DATA = TT_DATA;
        using BTT_COEFF = TT_COEFF;
        static constexpr unsigned int BTP_FIR_LEN = TP_FIR_LEN;
        static constexpr unsigned int BTP_CASC_LEN = TP_CASC_LEN;
        static constexpr unsigned int BTP_SHIFT = TP_SHIFT;
        static constexpr unsigned int BTP_RND = TP_RND;
        static constexpr unsigned int BTP_INPUT_WINDOW_VSIZE = TP_INPUT_WINDOW_VSIZE;
    };

    // Additional defensive checks
    // window size must be a multiple of lanes* decimation factor (to ensure that output window is a multiple of lanes)
    // * 2 (because most archs have a minimum repeat factor of 2.
    static_assert(!((TP_API == USE_STREAM_API) &&
                    (fir_decimate_hb_asym_tl<ssr_params>::
                         template fnCheckIfFits<TP_KERNEL_POSITION, TP_CASC_LEN, TP_FIR_LEN, TT_DATA, TT_COEFF>() ==
                     0)),
                  "ERROR: TP_FIR_RANGE_LEN exceeds max supported range for this data/coeff type combination. Increase "
                  "TP_CASC_LEN to split the workload over more kernels.");
    static_assert(TP_INPUT_WINDOW_VSIZE % (2 * kDecimateFactor * m_kLanes) == 0,
                  "ERROR: TP_INPUT_WINDOW_VSIZE must be an integer multiple of 512 bits. Since TP_INPUT_WINDOW_VSIZE "
                  "is in terms of samples, 512 is divided by the size of the data type here. e.g for cint16 (32 bits), "
                  "TP_INPUT_WINDOW_VSIZE must be a multiple of 16.");

   public:
    // Access function for AIE Synthesizer
    static unsigned int get_m_kArch() { return m_kArch; };

    // Constructor used for reloadable coefficients
    kernelFilterClass() : m_oldInTaps{}, m_internalTaps{} {}

    // Constractor used for static coefficients
    kernelFilterClass(const TT_COEFF (&taps)[(TP_FIR_LEN + 1) / 2 + 1]) : m_internalTaps{} { firReload(taps); }

    void firReload(const TT_COEFF* taps) {
        TT_COEFF* tapsPtr = (TT_COEFF*)taps;
        firReload(tapsPtr);
    }

    void firReload(TT_COEFF* taps) {
        for (int i = 0; i < CEIL(m_kNumAsymPh1Taps, kMaxColumns); i++) {
            if (i < m_kNumAsymPh1Taps) {
                unsigned int tapsAddress = (TP_FIR_LEN + 1) / 2 - 1 - m_kFirRangeOffset - i;
                m_internalTaps[i] = taps[tapsAddress];
            } else {
                m_internalTaps[i] = nullElem<TT_COEFF>();
            }
        }
        // The centre tap requires only one coefficient, but this is a vector processor with
        // multiple columns in the mul intrinsic, so the other columns must be zero'd
        m_phaseTwoTap[0] = taps[m_kTotTaps - 1];
    }

    // FIR
    void filterKernel(T_inputIF<TP_CASC_IN, TT_DATA, TP_DUAL_IP> inInterface,
                      T_outputIF<TP_CASC_OUT, TT_DATA> outInterface);

    // with taps for reload
    void filterKernel(T_inputIF<TP_CASC_IN, TT_DATA, TP_DUAL_IP> inInterface,
                      T_outputIF<TP_CASC_OUT, TT_DATA> outInterface,
                      const TT_COEFF (&inTaps)[(TP_FIR_LEN + 1) / 2 + 1]);

    void filterKernelRtp(T_inputIF<TP_CASC_IN, TT_DATA, TP_DUAL_IP> inInterface,
                         T_outputIF<TP_CASC_OUT, TT_DATA> outInterface);
};

//-----------------------------------------------------------------------------------------------------
// Cascade layer class and specializations

//-----------------------------------------------------------------------------------------------------
// This is the main declaration of the windowed fir_decimate_hb_asym class, and is also used for the
// Standalone kernel specialization with no cascade ports, Windowed. a single input, no reload, single output
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
          unsigned int TP_DUAL_IP,
          unsigned int TP_USE_COEFF_RELOAD,
          unsigned int TP_NUM_OUTPUTS,
          unsigned int TP_API,
          unsigned int TP_SAT>
class fir_decimate_hb_asym : public kernelFilterClass<TT_DATA,
                                                      TT_COEFF,
                                                      TP_FIR_LEN,
                                                      TP_SHIFT,
                                                      TP_RND,
                                                      TP_INPUT_WINDOW_VSIZE,
                                                      CASC_IN_FALSE,
                                                      CASC_OUT_FALSE,
                                                      TP_FIR_LEN,
                                                      0,
                                                      1,
                                                      TP_DUAL_IP,
                                                      TP_USE_COEFF_RELOAD,
                                                      TP_NUM_OUTPUTS,
                                                      USE_WINDOW_API,
                                                      TP_SAT> {
   private:
   public:
    // Constructor
    fir_decimate_hb_asym(const TT_COEFF (&taps)[(TP_FIR_LEN + 1) / 2 + 1])
        : kernelFilterClass<TT_DATA,
                            TT_COEFF,
                            TP_FIR_LEN,
                            TP_SHIFT,
                            TP_RND,
                            TP_INPUT_WINDOW_VSIZE,
                            CASC_IN_FALSE,
                            CASC_OUT_FALSE,
                            TP_FIR_LEN,
                            0,
                            1,
                            TP_DUAL_IP,
                            TP_USE_COEFF_RELOAD,
                            TP_NUM_OUTPUTS,
                            USE_WINDOW_API,
                            TP_SAT>(taps) {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fir_decimate_hb_asym::filter); }

    // FIR
    void filter(input_circular_buffer<TT_DATA,
                                      extents<inherited_extent>,
                                      margin<fnFirMargin<TP_FIR_LEN, TT_DATA>()> >& __restrict inWindow,
                output_circular_buffer<TT_DATA>& __restrict outWindow);
};

//-----------------------------------------------------------------------------------------------------
// Single kernel specialization. No cascade ports, Windowed. single input, with reload coefficients, single output
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_SAT>
class fir_decimate_hb_asym<TT_DATA,
                           TT_COEFF,
                           TP_FIR_LEN,
                           TP_SHIFT,
                           TP_RND,
                           TP_INPUT_WINDOW_VSIZE,
                           CASC_IN_FALSE,
                           CASC_OUT_FALSE,
                           TP_FIR_LEN,
                           0,
                           1,
                           DUAL_IP_SINGLE,
                           USE_COEFF_RELOAD_TRUE,
                           1,
                           USE_WINDOW_API,
                           TP_SAT> : public kernelFilterClass<TT_DATA,
                                                              TT_COEFF,
                                                              TP_FIR_LEN,
                                                              TP_SHIFT,
                                                              TP_RND,
                                                              TP_INPUT_WINDOW_VSIZE,
                                                              CASC_IN_FALSE,
                                                              CASC_OUT_FALSE,
                                                              TP_FIR_LEN,
                                                              0,
                                                              1,
                                                              DUAL_IP_SINGLE,
                                                              USE_COEFF_RELOAD_TRUE,
                                                              1,
                                                              USE_WINDOW_API,
                                                              TP_SAT> {
   private:
   public:
    // Constructor
    fir_decimate_hb_asym()
        : kernelFilterClass<TT_DATA,
                            TT_COEFF,
                            TP_FIR_LEN,
                            TP_SHIFT,
                            TP_RND,
                            TP_INPUT_WINDOW_VSIZE,
                            CASC_IN_FALSE,
                            CASC_OUT_FALSE,
                            TP_FIR_LEN,
                            0,
                            1,
                            DUAL_IP_SINGLE,
                            USE_COEFF_RELOAD_TRUE,
                            1,
                            USE_WINDOW_API,
                            TP_SAT>() {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fir_decimate_hb_asym::filter); }

    // FIR
    void filter(input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<fnFirMargin<TP_FIR_LEN, TT_DATA>()> >&
                    inWindow,
                output_circular_buffer<TT_DATA>& outWindow,
                const TT_COEFF (&inTaps)[(TP_FIR_LEN + 1) / 2 + 1]);
};

//-----------------------------------------------------------------------------------------------------
// Partially specialized classes for cascaded interface (final kernel in cascade), Windowed. single input, no reload,
// single output
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_FIR_RANGE_LEN,
          unsigned int TP_KERNEL_POSITION,
          unsigned int TP_CASC_LEN,
          unsigned int TP_DUAL_IP,
          unsigned int TP_SAT>
class fir_decimate_hb_asym<TT_DATA,
                           TT_COEFF,
                           TP_FIR_LEN,
                           TP_SHIFT,
                           TP_RND,
                           TP_INPUT_WINDOW_VSIZE,
                           CASC_IN_TRUE,
                           CASC_OUT_FALSE,
                           TP_FIR_RANGE_LEN,
                           TP_KERNEL_POSITION,
                           TP_CASC_LEN,
                           TP_DUAL_IP,
                           USE_COEFF_RELOAD_FALSE,
                           1,
                           USE_WINDOW_API,
                           TP_SAT> : public kernelFilterClass<TT_DATA,
                                                              TT_COEFF,
                                                              TP_FIR_LEN,
                                                              TP_SHIFT,
                                                              TP_RND,
                                                              TP_INPUT_WINDOW_VSIZE,
                                                              CASC_IN_TRUE,
                                                              CASC_OUT_FALSE,
                                                              TP_FIR_RANGE_LEN,
                                                              TP_KERNEL_POSITION,
                                                              TP_CASC_LEN,
                                                              TP_DUAL_IP,
                                                              USE_COEFF_RELOAD_FALSE,
                                                              1,
                                                              USE_WINDOW_API,
                                                              TP_SAT> {
   private:
   public:
    // Constructor
    fir_decimate_hb_asym(const TT_COEFF (&taps)[(TP_FIR_LEN + 1) / 2 + 1])
        : kernelFilterClass<TT_DATA,
                            TT_COEFF,
                            TP_FIR_LEN,
                            TP_SHIFT,
                            TP_RND,
                            TP_INPUT_WINDOW_VSIZE,
                            CASC_IN_TRUE,
                            CASC_OUT_FALSE,
                            TP_FIR_RANGE_LEN,
                            TP_KERNEL_POSITION,
                            TP_CASC_LEN,
                            TP_DUAL_IP,
                            USE_COEFF_RELOAD_FALSE,
                            1,
                            USE_WINDOW_API,
                            TP_SAT>(taps) {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fir_decimate_hb_asym::filter); }

    // FIR
    void filter(input_async_buffer<TT_DATA>& inWindow,
                input_cascade_cacc* inCascade,
                output_circular_buffer<TT_DATA>& __restrict outWindow);
};

// Partially specialized classes for cascaded interface (final kernel in cascade), Windowed. single/dual input, with
// reload, single output
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_FIR_RANGE_LEN,
          unsigned int TP_KERNEL_POSITION,
          unsigned int TP_CASC_LEN,
          unsigned int TP_DUAL_IP,
          unsigned int TP_SAT>
class fir_decimate_hb_asym<TT_DATA,
                           TT_COEFF,
                           TP_FIR_LEN,
                           TP_SHIFT,
                           TP_RND,
                           TP_INPUT_WINDOW_VSIZE,
                           CASC_IN_TRUE,
                           CASC_OUT_FALSE,
                           TP_FIR_RANGE_LEN,
                           TP_KERNEL_POSITION,
                           TP_CASC_LEN,
                           TP_DUAL_IP,
                           USE_COEFF_RELOAD_TRUE,
                           1,
                           USE_WINDOW_API,
                           TP_SAT> : public kernelFilterClass<TT_DATA,
                                                              TT_COEFF,
                                                              TP_FIR_LEN,
                                                              TP_SHIFT,
                                                              TP_RND,
                                                              TP_INPUT_WINDOW_VSIZE,
                                                              CASC_IN_TRUE,
                                                              CASC_OUT_FALSE,
                                                              TP_FIR_RANGE_LEN,
                                                              TP_KERNEL_POSITION,
                                                              TP_CASC_LEN,
                                                              TP_DUAL_IP,
                                                              USE_COEFF_RELOAD_TRUE,
                                                              1,
                                                              USE_WINDOW_API,
                                                              TP_SAT> {
   private:
   public:
    // Constructor
    fir_decimate_hb_asym()
        : kernelFilterClass<TT_DATA,
                            TT_COEFF,
                            TP_FIR_LEN,
                            TP_SHIFT,
                            TP_RND,
                            TP_INPUT_WINDOW_VSIZE,
                            CASC_IN_TRUE,
                            CASC_OUT_FALSE,
                            TP_FIR_RANGE_LEN,
                            TP_KERNEL_POSITION,
                            TP_CASC_LEN,
                            TP_DUAL_IP,
                            USE_COEFF_RELOAD_TRUE,
                            1,
                            USE_WINDOW_API,
                            TP_SAT>() {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fir_decimate_hb_asym::filter); }

    // FIR
    void filter(input_async_buffer<TT_DATA>& inWindow,
                input_cascade_cacc* inCascade,
                output_circular_buffer<TT_DATA>& __restrict outWindow);
};

//-----------------------------------------------------------------------------------------------------
// Partially specialized classes for cascaded interface (First kernel in cascade), Windowed. single input, no reload
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_FIR_RANGE_LEN,
          unsigned int TP_KERNEL_POSITION,
          unsigned int TP_CASC_LEN,
          unsigned int TP_DUAL_IP,
          unsigned int TP_SAT>
class fir_decimate_hb_asym<TT_DATA,
                           TT_COEFF,
                           TP_FIR_LEN,
                           TP_SHIFT,
                           TP_RND,
                           TP_INPUT_WINDOW_VSIZE,
                           CASC_IN_FALSE,
                           CASC_OUT_TRUE,
                           TP_FIR_RANGE_LEN,
                           TP_KERNEL_POSITION,
                           TP_CASC_LEN,
                           TP_DUAL_IP,
                           USE_COEFF_RELOAD_FALSE,
                           1,
                           USE_WINDOW_API,
                           TP_SAT> : public kernelFilterClass<TT_DATA,
                                                              TT_COEFF,
                                                              TP_FIR_LEN,
                                                              TP_SHIFT,
                                                              TP_RND,
                                                              TP_INPUT_WINDOW_VSIZE,
                                                              CASC_IN_FALSE,
                                                              CASC_OUT_TRUE,
                                                              TP_FIR_RANGE_LEN,
                                                              TP_KERNEL_POSITION,
                                                              TP_CASC_LEN,
                                                              TP_DUAL_IP,
                                                              USE_COEFF_RELOAD_FALSE,
                                                              1,
                                                              USE_WINDOW_API,
                                                              TP_SAT> {
   private:
   public:
    // Constructor
    fir_decimate_hb_asym(const TT_COEFF (&taps)[(TP_FIR_LEN + 1) / 2 + 1])
        : kernelFilterClass<TT_DATA,
                            TT_COEFF,
                            TP_FIR_LEN,
                            TP_SHIFT,
                            TP_RND,
                            TP_INPUT_WINDOW_VSIZE,
                            CASC_IN_FALSE,
                            CASC_OUT_TRUE,
                            TP_FIR_RANGE_LEN,
                            TP_KERNEL_POSITION,
                            TP_CASC_LEN,
                            TP_DUAL_IP,
                            USE_COEFF_RELOAD_FALSE,
                            1,
                            USE_WINDOW_API,
                            TP_SAT>(taps) {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fir_decimate_hb_asym::filter); }

    // FIR
    void filter(input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<fnFirMargin<TP_FIR_LEN, TT_DATA>()> >&
                    inWindow,
                output_cascade_cacc* outCascade,
                output_async_buffer<TT_DATA>& broadcastWindow);
};

// Partially specialized classes for cascaded interface (First kernel in cascade), Windowed. single input, with reload
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_FIR_RANGE_LEN,
          unsigned int TP_KERNEL_POSITION,
          unsigned int TP_CASC_LEN,
          unsigned int TP_DUAL_IP,
          unsigned int TP_SAT>
class fir_decimate_hb_asym<TT_DATA,
                           TT_COEFF,
                           TP_FIR_LEN,
                           TP_SHIFT,
                           TP_RND,
                           TP_INPUT_WINDOW_VSIZE,
                           CASC_IN_FALSE,
                           CASC_OUT_TRUE,
                           TP_FIR_RANGE_LEN,
                           TP_KERNEL_POSITION,
                           TP_CASC_LEN,
                           TP_DUAL_IP,
                           USE_COEFF_RELOAD_TRUE,
                           1,
                           USE_WINDOW_API,
                           TP_SAT> : public kernelFilterClass<TT_DATA,
                                                              TT_COEFF,
                                                              TP_FIR_LEN,
                                                              TP_SHIFT,
                                                              TP_RND,
                                                              TP_INPUT_WINDOW_VSIZE,
                                                              CASC_IN_FALSE,
                                                              CASC_OUT_TRUE,
                                                              TP_FIR_RANGE_LEN,
                                                              TP_KERNEL_POSITION,
                                                              TP_CASC_LEN,
                                                              TP_DUAL_IP,
                                                              USE_COEFF_RELOAD_TRUE,
                                                              1,
                                                              USE_WINDOW_API,
                                                              TP_SAT> {
   private:
   public:
    // Constructor
    fir_decimate_hb_asym()
        : kernelFilterClass<TT_DATA,
                            TT_COEFF,
                            TP_FIR_LEN,
                            TP_SHIFT,
                            TP_RND,
                            TP_INPUT_WINDOW_VSIZE,
                            CASC_IN_FALSE,
                            CASC_OUT_TRUE,
                            TP_FIR_RANGE_LEN,
                            TP_KERNEL_POSITION,
                            TP_CASC_LEN,
                            TP_DUAL_IP,
                            USE_COEFF_RELOAD_TRUE,
                            1,
                            USE_WINDOW_API,
                            TP_SAT>() {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fir_decimate_hb_asym::filter); }

    // FIR
    void filter(input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<fnFirMargin<TP_FIR_LEN, TT_DATA>()> >&
                    inWindow,
                output_cascade_cacc* outCascade,
                output_async_buffer<TT_DATA>& broadcastWindow,
                const TT_COEFF (&inTaps)[(TP_FIR_LEN + 1) / 2 + 1]);
};

//-----------------------------------------------------------------------------------------------------
// Partially specialized classes for cascaded interface (middle kernels in cascade), Windowed. single/dual  input, no
// reload
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_FIR_RANGE_LEN,
          unsigned int TP_KERNEL_POSITION,
          unsigned int TP_CASC_LEN,
          unsigned int TP_DUAL_IP,
          unsigned int TP_SAT>
class fir_decimate_hb_asym<TT_DATA,
                           TT_COEFF,
                           TP_FIR_LEN,
                           TP_SHIFT,
                           TP_RND,
                           TP_INPUT_WINDOW_VSIZE,
                           CASC_IN_TRUE,
                           CASC_OUT_TRUE,
                           TP_FIR_RANGE_LEN,
                           TP_KERNEL_POSITION,
                           TP_CASC_LEN,
                           TP_DUAL_IP,
                           USE_COEFF_RELOAD_FALSE,
                           1,
                           USE_WINDOW_API,
                           TP_SAT> : public kernelFilterClass<TT_DATA,
                                                              TT_COEFF,
                                                              TP_FIR_LEN,
                                                              TP_SHIFT,
                                                              TP_RND,
                                                              TP_INPUT_WINDOW_VSIZE,
                                                              CASC_IN_TRUE,
                                                              CASC_OUT_TRUE,
                                                              TP_FIR_RANGE_LEN,
                                                              TP_KERNEL_POSITION,
                                                              TP_CASC_LEN,
                                                              TP_DUAL_IP,
                                                              USE_COEFF_RELOAD_FALSE,
                                                              1,
                                                              USE_WINDOW_API,
                                                              TP_SAT> {
   private:
   public:
    // Constructor
    fir_decimate_hb_asym(const TT_COEFF (&taps)[(TP_FIR_LEN + 1) / 2 + 1])
        : kernelFilterClass<TT_DATA,
                            TT_COEFF,
                            TP_FIR_LEN,
                            TP_SHIFT,
                            TP_RND,
                            TP_INPUT_WINDOW_VSIZE,
                            CASC_IN_TRUE,
                            CASC_OUT_TRUE,
                            TP_FIR_RANGE_LEN,
                            TP_KERNEL_POSITION,
                            TP_CASC_LEN,
                            TP_DUAL_IP,
                            USE_COEFF_RELOAD_FALSE,
                            1,
                            USE_WINDOW_API,
                            TP_SAT>(taps) {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fir_decimate_hb_asym::filter); }

    // FIR
    void filter(input_async_buffer<TT_DATA>& inWindow,
                input_cascade_cacc* inCascade,
                output_cascade_cacc* outCascade,
                output_async_buffer<TT_DATA>& broadcastWindow);
};

// Partially specialized classes for cascaded interface (middle kernels in cascade), Windowed. single/dual input, with
// reload
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_FIR_RANGE_LEN,
          unsigned int TP_KERNEL_POSITION,
          unsigned int TP_CASC_LEN,
          unsigned int TP_DUAL_IP,
          unsigned int TP_SAT>
class fir_decimate_hb_asym<TT_DATA,
                           TT_COEFF,
                           TP_FIR_LEN,
                           TP_SHIFT,
                           TP_RND,
                           TP_INPUT_WINDOW_VSIZE,
                           CASC_IN_TRUE,
                           CASC_OUT_TRUE,
                           TP_FIR_RANGE_LEN,
                           TP_KERNEL_POSITION,
                           TP_CASC_LEN,
                           TP_DUAL_IP,
                           USE_COEFF_RELOAD_TRUE,
                           1,
                           USE_WINDOW_API,
                           TP_SAT> : public kernelFilterClass<TT_DATA,
                                                              TT_COEFF,
                                                              TP_FIR_LEN,
                                                              TP_SHIFT,
                                                              TP_RND,
                                                              TP_INPUT_WINDOW_VSIZE,
                                                              CASC_IN_TRUE,
                                                              CASC_OUT_TRUE,
                                                              TP_FIR_RANGE_LEN,
                                                              TP_KERNEL_POSITION,
                                                              TP_CASC_LEN,
                                                              TP_DUAL_IP,
                                                              USE_COEFF_RELOAD_TRUE,
                                                              1,
                                                              USE_WINDOW_API,
                                                              TP_SAT> {
   private:
   public:
    // Constructor
    fir_decimate_hb_asym()
        : kernelFilterClass<TT_DATA,
                            TT_COEFF,
                            TP_FIR_LEN,
                            TP_SHIFT,
                            TP_RND,
                            TP_INPUT_WINDOW_VSIZE,
                            CASC_IN_TRUE,
                            CASC_OUT_TRUE,
                            TP_FIR_RANGE_LEN,
                            TP_KERNEL_POSITION,
                            TP_CASC_LEN,
                            TP_DUAL_IP,
                            USE_COEFF_RELOAD_TRUE,
                            1,
                            USE_WINDOW_API,
                            TP_SAT>() {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fir_decimate_hb_asym::filter); }

    // FIR
    void filter(input_async_buffer<TT_DATA>& inWindow,
                input_cascade_cacc* inCascade,
                output_cascade_cacc* outCascade,
                output_async_buffer<TT_DATA>& broadcastWindow);
};

// ----------------------------------------------------------------------------
// ---------------------------------- STREAM ----------------------------------
// ----------------------------------------------------------------------------

//-----------------------------------------------------------------------------------------------------
// Single kernel specialization. No cascade ports. Streaming. Static coefficients. Single Output
//-----------------------------------------------------------------------------------------------------
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_FIR_RANGE_LEN,
          unsigned int TP_DUAL_IP,
          unsigned int TP_SAT>
class fir_decimate_hb_asym<TT_DATA,
                           TT_COEFF,
                           TP_FIR_LEN,
                           TP_SHIFT,
                           TP_RND,
                           TP_INPUT_WINDOW_VSIZE,
                           CASC_IN_FALSE,
                           CASC_OUT_FALSE,
                           TP_FIR_RANGE_LEN,
                           0,
                           1,
                           TP_DUAL_IP,
                           USE_COEFF_RELOAD_FALSE,
                           1,
                           USE_STREAM_API,
                           TP_SAT> : public kernelFilterClass<TT_DATA,
                                                              TT_COEFF,
                                                              TP_FIR_LEN,
                                                              TP_SHIFT,
                                                              TP_RND,
                                                              TP_INPUT_WINDOW_VSIZE,
                                                              CASC_IN_FALSE,
                                                              CASC_OUT_FALSE,
                                                              TP_FIR_RANGE_LEN,
                                                              0,
                                                              1,
                                                              TP_DUAL_IP,
                                                              USE_COEFF_RELOAD_FALSE,
                                                              1,
                                                              USE_STREAM_API,
                                                              TP_SAT> {
   public:
    // Constructor
    fir_decimate_hb_asym(const TT_COEFF (&taps)[(TP_FIR_LEN + 1) / 2 + 1])
        : kernelFilterClass<TT_DATA,
                            TT_COEFF,
                            TP_FIR_LEN,
                            TP_SHIFT,
                            TP_RND,
                            TP_INPUT_WINDOW_VSIZE,
                            CASC_IN_FALSE,
                            CASC_OUT_FALSE,
                            TP_FIR_RANGE_LEN,
                            0,
                            1,
                            TP_DUAL_IP,
                            USE_COEFF_RELOAD_FALSE,
                            1,
                            USE_STREAM_API,
                            TP_SAT>(taps) {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fir_decimate_hb_asym::filter); }

    // FIR
    void filter(input_stream<TT_DATA>* inStream, output_stream<TT_DATA>* outStream);
};

//-----------------------------------------------------------------------------------------------------
// Single kernel specialization. No cascade ports. Streaming. Reloadable coefficients. Single Output
//-----------------------------------------------------------------------------------------------------
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_FIR_RANGE_LEN,
          unsigned int TP_DUAL_IP,
          unsigned int TP_SAT>
class fir_decimate_hb_asym<TT_DATA,
                           TT_COEFF,
                           TP_FIR_LEN,
                           TP_SHIFT,
                           TP_RND,
                           TP_INPUT_WINDOW_VSIZE,
                           CASC_IN_FALSE,
                           CASC_OUT_FALSE,
                           TP_FIR_RANGE_LEN,
                           0,
                           1,
                           TP_DUAL_IP,
                           USE_COEFF_RELOAD_TRUE,
                           1,
                           USE_STREAM_API,
                           TP_SAT> : public kernelFilterClass<TT_DATA,
                                                              TT_COEFF,
                                                              TP_FIR_LEN,
                                                              TP_SHIFT,
                                                              TP_RND,
                                                              TP_INPUT_WINDOW_VSIZE,
                                                              CASC_IN_FALSE,
                                                              CASC_OUT_FALSE,
                                                              TP_FIR_RANGE_LEN,
                                                              0,
                                                              1,
                                                              TP_DUAL_IP,
                                                              USE_COEFF_RELOAD_TRUE,
                                                              1,
                                                              USE_STREAM_API,
                                                              TP_SAT> {
   public:
    // Constructor
    fir_decimate_hb_asym()
        : kernelFilterClass<TT_DATA,
                            TT_COEFF,
                            TP_FIR_LEN,
                            TP_SHIFT,
                            TP_RND,
                            TP_INPUT_WINDOW_VSIZE,
                            CASC_IN_FALSE,
                            CASC_OUT_FALSE,
                            TP_FIR_RANGE_LEN,
                            0,
                            1,
                            TP_DUAL_IP,
                            USE_COEFF_RELOAD_TRUE,
                            1,
                            USE_STREAM_API,
                            TP_SAT>() {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fir_decimate_hb_asym::filter); }

    // FIR
    void filter(input_stream<TT_DATA>* inStream,
                output_stream<TT_DATA>* outStream,
                const TT_COEFF (&inTaps)[(TP_FIR_LEN + 1) / 2 + 1]);
};

//-----------------------------------------------------------------------------------------------------
// Cascaded Kernels - Final Kernel. Static coefficients. Streaming. Single Input. Single Output
//-----------------------------------------------------------------------------------------------------
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_FIR_RANGE_LEN,
          unsigned int TP_KERNEL_POSITION,
          unsigned int TP_CASC_LEN,
          unsigned int TP_DUAL_IP,
          unsigned int TP_SAT>
class fir_decimate_hb_asym<TT_DATA,
                           TT_COEFF,
                           TP_FIR_LEN,
                           TP_SHIFT,
                           TP_RND,
                           TP_INPUT_WINDOW_VSIZE,
                           CASC_IN_TRUE,
                           CASC_OUT_FALSE,
                           TP_FIR_RANGE_LEN,
                           TP_KERNEL_POSITION,
                           TP_CASC_LEN,
                           TP_DUAL_IP,
                           USE_COEFF_RELOAD_FALSE,
                           1,
                           USE_STREAM_API,
                           TP_SAT> : public kernelFilterClass<TT_DATA,
                                                              TT_COEFF,
                                                              TP_FIR_LEN,
                                                              TP_SHIFT,
                                                              TP_RND,
                                                              TP_INPUT_WINDOW_VSIZE,
                                                              CASC_IN_TRUE,
                                                              CASC_OUT_FALSE,
                                                              TP_FIR_RANGE_LEN,
                                                              TP_KERNEL_POSITION,
                                                              TP_CASC_LEN,
                                                              TP_DUAL_IP,
                                                              USE_COEFF_RELOAD_FALSE,
                                                              1,
                                                              USE_STREAM_API,
                                                              TP_SAT> {
   public:
    // Constructor
    fir_decimate_hb_asym(const TT_COEFF (&taps)[(TP_FIR_LEN + 1) / 2 + 1])
        : kernelFilterClass<TT_DATA,
                            TT_COEFF,
                            TP_FIR_LEN,
                            TP_SHIFT,
                            TP_RND,
                            TP_INPUT_WINDOW_VSIZE,
                            CASC_IN_TRUE,
                            CASC_OUT_FALSE,
                            TP_FIR_RANGE_LEN,
                            TP_KERNEL_POSITION,
                            TP_CASC_LEN,
                            TP_DUAL_IP,
                            USE_COEFF_RELOAD_FALSE,
                            1,
                            USE_STREAM_API,
                            TP_SAT>(taps) {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fir_decimate_hb_asym::filter); }

    // FIR
    void filter(input_stream<TT_DATA>* inStream, input_cascade_cacc* inCascade, output_stream<TT_DATA>* outStream);
};

//-----------------------------------------------------------------------------------------------------
// Cascaded Kernels - First Kernel. Streaming. Static coefficients. Single Input
//-----------------------------------------------------------------------------------------------------
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_FIR_RANGE_LEN,
          unsigned int TP_CASC_LEN,
          unsigned int TP_DUAL_IP,
          unsigned int TP_SAT>
class fir_decimate_hb_asym<TT_DATA,
                           TT_COEFF,
                           TP_FIR_LEN,
                           TP_SHIFT,
                           TP_RND,
                           TP_INPUT_WINDOW_VSIZE,
                           CASC_IN_FALSE,
                           CASC_OUT_TRUE,
                           TP_FIR_RANGE_LEN,
                           0,
                           TP_CASC_LEN,
                           TP_DUAL_IP,
                           USE_COEFF_RELOAD_FALSE,
                           1,
                           USE_STREAM_API,
                           TP_SAT> : public kernelFilterClass<TT_DATA,
                                                              TT_COEFF,
                                                              TP_FIR_LEN,
                                                              TP_SHIFT,
                                                              TP_RND,
                                                              TP_INPUT_WINDOW_VSIZE,
                                                              CASC_IN_FALSE,
                                                              CASC_OUT_TRUE,
                                                              TP_FIR_RANGE_LEN,
                                                              0,
                                                              TP_CASC_LEN,
                                                              TP_DUAL_IP,
                                                              USE_COEFF_RELOAD_FALSE,
                                                              1,
                                                              USE_STREAM_API,
                                                              TP_SAT> {
   public:
    // Constructor
    fir_decimate_hb_asym(const TT_COEFF (&taps)[(TP_FIR_LEN + 1) / 2 + 1])
        : kernelFilterClass<TT_DATA,
                            TT_COEFF,
                            TP_FIR_LEN,
                            TP_SHIFT,
                            TP_RND,
                            TP_INPUT_WINDOW_VSIZE,
                            CASC_IN_FALSE,
                            CASC_OUT_TRUE,
                            TP_FIR_RANGE_LEN,
                            0,
                            TP_CASC_LEN,
                            TP_DUAL_IP,
                            USE_COEFF_RELOAD_FALSE,
                            1,
                            USE_STREAM_API,
                            TP_SAT>(taps) {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fir_decimate_hb_asym::filter); }

    // FIR
    void filter(input_stream<TT_DATA>* inStream, output_cascade_cacc* outCascade);
};

//-----------------------------------------------------------------------------------------------------
// Cascaded Kernels - Middle Kernel. Streaming. Static coefficients. Single Input
//-----------------------------------------------------------------------------------------------------
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_FIR_RANGE_LEN,
          unsigned int TP_KERNEL_POSITION,
          unsigned int TP_CASC_LEN,
          unsigned int TP_DUAL_IP,
          unsigned int TP_SAT>
class fir_decimate_hb_asym<TT_DATA,
                           TT_COEFF,
                           TP_FIR_LEN,
                           TP_SHIFT,
                           TP_RND,
                           TP_INPUT_WINDOW_VSIZE,
                           CASC_IN_TRUE,
                           CASC_OUT_TRUE,
                           TP_FIR_RANGE_LEN,
                           TP_KERNEL_POSITION,
                           TP_CASC_LEN,
                           TP_DUAL_IP,
                           USE_COEFF_RELOAD_FALSE,
                           1,
                           USE_STREAM_API,
                           TP_SAT> : public kernelFilterClass<TT_DATA,
                                                              TT_COEFF,
                                                              TP_FIR_LEN,
                                                              TP_SHIFT,
                                                              TP_RND,
                                                              TP_INPUT_WINDOW_VSIZE,
                                                              CASC_IN_TRUE,
                                                              CASC_OUT_TRUE,
                                                              TP_FIR_RANGE_LEN,
                                                              TP_KERNEL_POSITION,
                                                              TP_CASC_LEN,
                                                              TP_DUAL_IP,
                                                              USE_COEFF_RELOAD_FALSE,
                                                              1,
                                                              USE_STREAM_API,
                                                              TP_SAT> {
   public:
    // Constructor
    fir_decimate_hb_asym(const TT_COEFF (&taps)[(TP_FIR_LEN + 1) / 2 + 1])
        : kernelFilterClass<TT_DATA,
                            TT_COEFF,
                            TP_FIR_LEN,
                            TP_SHIFT,
                            TP_RND,
                            TP_INPUT_WINDOW_VSIZE,
                            CASC_IN_TRUE,
                            CASC_OUT_TRUE,
                            TP_FIR_RANGE_LEN,
                            TP_KERNEL_POSITION,
                            TP_CASC_LEN,
                            TP_DUAL_IP,
                            USE_COEFF_RELOAD_FALSE,
                            1,
                            USE_STREAM_API,
                            TP_SAT>(taps) {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fir_decimate_hb_asym::filter); }

    // FIR
    void filter(input_stream<TT_DATA>* inStream, input_cascade_cacc* inCascade, output_cascade_cacc* outCascade);
};

//-----------------------------------------------------------------------------------------------------
// Cascaded Kernels - Final Kernel. Streaming. Reloadable coefficients. Single input. Single Output
//-----------------------------------------------------------------------------------------------------
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_FIR_RANGE_LEN,
          unsigned int TP_KERNEL_POSITION,
          unsigned int TP_CASC_LEN,
          unsigned int TP_DUAL_IP,
          unsigned int TP_SAT>
class fir_decimate_hb_asym<TT_DATA,
                           TT_COEFF,
                           TP_FIR_LEN,
                           TP_SHIFT,
                           TP_RND,
                           TP_INPUT_WINDOW_VSIZE,
                           CASC_IN_TRUE,
                           CASC_OUT_FALSE,
                           TP_FIR_RANGE_LEN,
                           TP_KERNEL_POSITION,
                           TP_CASC_LEN,
                           TP_DUAL_IP,
                           USE_COEFF_RELOAD_TRUE,
                           1,
                           USE_STREAM_API,
                           TP_SAT> : public kernelFilterClass<TT_DATA,
                                                              TT_COEFF,
                                                              TP_FIR_LEN,
                                                              TP_SHIFT,
                                                              TP_RND,
                                                              TP_INPUT_WINDOW_VSIZE,
                                                              CASC_IN_TRUE,
                                                              CASC_OUT_FALSE,
                                                              TP_FIR_RANGE_LEN,
                                                              TP_KERNEL_POSITION,
                                                              TP_CASC_LEN,
                                                              TP_DUAL_IP,
                                                              USE_COEFF_RELOAD_TRUE,
                                                              1,
                                                              USE_STREAM_API,
                                                              TP_SAT> {
   public:
    // Constructor
    fir_decimate_hb_asym()
        : kernelFilterClass<TT_DATA,
                            TT_COEFF,
                            TP_FIR_LEN,
                            TP_SHIFT,
                            TP_RND,
                            TP_INPUT_WINDOW_VSIZE,
                            CASC_IN_TRUE,
                            CASC_OUT_FALSE,
                            TP_FIR_RANGE_LEN,
                            TP_KERNEL_POSITION,
                            TP_CASC_LEN,
                            TP_DUAL_IP,
                            USE_COEFF_RELOAD_TRUE,
                            1,
                            USE_STREAM_API,
                            TP_SAT>() {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fir_decimate_hb_asym::filter); }

    // FIR
    void filter(input_stream<TT_DATA>* inStream, input_cascade_cacc* inCascade, output_stream<TT_DATA>* outStream);
};

//-----------------------------------------------------------------------------------------------------
// Cascaded Kernels - First Kernel. Streaming. Reloadable coefficients. Single Input
//-----------------------------------------------------------------------------------------------------
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_FIR_RANGE_LEN,
          unsigned int TP_CASC_LEN,
          unsigned int TP_SAT>
class fir_decimate_hb_asym<TT_DATA,
                           TT_COEFF,
                           TP_FIR_LEN,
                           TP_SHIFT,
                           TP_RND,
                           TP_INPUT_WINDOW_VSIZE,
                           CASC_IN_FALSE,
                           CASC_OUT_TRUE,
                           TP_FIR_RANGE_LEN,
                           0,
                           TP_CASC_LEN,
                           DUAL_IP_SINGLE,
                           USE_COEFF_RELOAD_TRUE,
                           1,
                           USE_STREAM_API,
                           TP_SAT> : public kernelFilterClass<TT_DATA,
                                                              TT_COEFF,
                                                              TP_FIR_LEN,
                                                              TP_SHIFT,
                                                              TP_RND,
                                                              TP_INPUT_WINDOW_VSIZE,
                                                              CASC_IN_FALSE,
                                                              CASC_OUT_TRUE,
                                                              TP_FIR_RANGE_LEN,
                                                              0,
                                                              TP_CASC_LEN,
                                                              DUAL_IP_SINGLE,
                                                              USE_COEFF_RELOAD_TRUE,
                                                              1,
                                                              USE_STREAM_API,
                                                              TP_SAT> {
   public:
    // Constructor
    fir_decimate_hb_asym()
        : kernelFilterClass<TT_DATA,
                            TT_COEFF,
                            TP_FIR_LEN,
                            TP_SHIFT,
                            TP_RND,
                            TP_INPUT_WINDOW_VSIZE,
                            CASC_IN_FALSE,
                            CASC_OUT_TRUE,
                            TP_FIR_RANGE_LEN,
                            0,
                            TP_CASC_LEN,
                            DUAL_IP_SINGLE,
                            USE_COEFF_RELOAD_TRUE,
                            1,
                            USE_STREAM_API,
                            TP_SAT>() {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fir_decimate_hb_asym::filter); }

    // FIR
    void filter(input_stream<TT_DATA>* inStream,
                output_cascade_cacc* outCascade,
                const TT_COEFF (&inTaps)[(TP_FIR_LEN + 1) / 2 + 1]);
};

//-----------------------------------------------------------------------------------------------------
// Cascaded Kernels - Middle Kernel. Streaming. Reloadable coefficients. Single input
//-----------------------------------------------------------------------------------------------------
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_FIR_RANGE_LEN,
          unsigned int TP_KERNEL_POSITION,
          unsigned int TP_CASC_LEN,
          unsigned int TP_SAT>
class fir_decimate_hb_asym<TT_DATA,
                           TT_COEFF,
                           TP_FIR_LEN,
                           TP_SHIFT,
                           TP_RND,
                           TP_INPUT_WINDOW_VSIZE,
                           CASC_IN_TRUE,
                           CASC_OUT_TRUE,
                           TP_FIR_RANGE_LEN,
                           TP_KERNEL_POSITION,
                           TP_CASC_LEN,
                           DUAL_IP_SINGLE,
                           USE_COEFF_RELOAD_TRUE,
                           1,
                           USE_STREAM_API,
                           TP_SAT> : public kernelFilterClass<TT_DATA,
                                                              TT_COEFF,
                                                              TP_FIR_LEN,
                                                              TP_SHIFT,
                                                              TP_RND,
                                                              TP_INPUT_WINDOW_VSIZE,
                                                              CASC_IN_TRUE,
                                                              CASC_OUT_TRUE,
                                                              TP_FIR_RANGE_LEN,
                                                              TP_KERNEL_POSITION,
                                                              TP_CASC_LEN,
                                                              DUAL_IP_SINGLE,
                                                              USE_COEFF_RELOAD_TRUE,
                                                              1,
                                                              USE_STREAM_API,
                                                              TP_SAT> {
   public:
    // Constructor
    fir_decimate_hb_asym()
        : kernelFilterClass<TT_DATA,
                            TT_COEFF,
                            TP_FIR_LEN,
                            TP_SHIFT,
                            TP_RND,
                            TP_INPUT_WINDOW_VSIZE,
                            CASC_IN_TRUE,
                            CASC_OUT_TRUE,
                            TP_FIR_RANGE_LEN,
                            TP_KERNEL_POSITION,
                            TP_CASC_LEN,
                            DUAL_IP_SINGLE,
                            USE_COEFF_RELOAD_TRUE,
                            1,
                            USE_STREAM_API,
                            TP_SAT>() {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fir_decimate_hb_asym::filter); }

    // FIR
    void filter(input_stream<TT_DATA>* inStream, input_cascade_cacc* inCascade, output_cascade_cacc* outCascade);
};
}
}
}
}
}
#endif // _DSPLIB_fir_decimate_hb_asym_HPP_
