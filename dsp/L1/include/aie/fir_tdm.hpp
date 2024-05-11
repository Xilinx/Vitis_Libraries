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
#ifndef _DSPLIB_FIR_TDM_HPP_
#define _DSPLIB_FIR_TDM_HPP_

#define __FIR_TDM_USE_48BIT_ACC__ 1

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
    static constexpr unsigned int getSSRMargin() { return parent_class::get_margin(); };

    static constexpr unsigned int getDF() { return 1; };
    static constexpr unsigned int getIF() { return 1; };

    static constexpr unsigned int getLanes() { return parent_class::get_Lanes(); };
    static constexpr unsigned int getInternalBufferSize() { return parent_class::get_internalBufferSize(); };
    static constexpr unsigned int getColMultiple() { return parent_class::get_ColMultiple(); };

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
    static_assert(TP_NUM_OUTPUTS > 0 && TP_NUM_OUTPUTS <= 2, "ERROR: only single or dual outputs are supported.");
#if __SUPPORTS_CFLOAT__ == 1
    static_assert(!(std::is_same<TT_DATA, cfloat>::value || std::is_same<TT_DATA, float>::value) || (TP_SHIFT == 0),
                  "ERROR: TP_SHIFT cannot be performed for TT_DATA=cfloat, so must be set to 0");
#else
    static_assert(!(std::is_same<TT_DATA, float>::value) || (TP_SHIFT == 0),
                  "ERROR: TP_SHIFT cannot be performed for TT_DATA=cfloat, so must be set to 0");
#endif

    typedef typename std::conditional_t<std::is_same<TT_COEFF, int32>::value,
                                        int16,
                                        std::conditional_t<std::is_same<TT_COEFF, cint32>::value, cint16, TT_COEFF> >
        TT_COEFF_TEST;
    static constexpr unsigned int m_kVOutSize = fnVOutSizeTdm<TT_DATA, TT_COEFF_TEST, TP_API>();
    static constexpr int TP_NUM_FRAMES = TP_INPUT_WINDOW_VSIZE / TP_TDM_CHANNELS;
    static constexpr int TP_TDM_LOOP_SIZE = TP_TDM_CHANNELS / m_kVOutSize;
    static_assert(
        TP_TDM_CHANNELS % m_kVOutSize == 0,
        "ERROR: TP_TDM_CHANNELS must be a integer multiple of lanes vector processor operates on (8, 16 or 32).");

// always assume data >= coeff. otherwise, we'd need to load data at 0.5 rate, i.e. every second iteration + add offset
#if __HAS_ACCUM_PERMUTES__ == 1
    // cint16/int16 combo can be overloaded with 2 column MUL/MACs.
    static constexpr unsigned int columnMultiple =
        (std::is_same<TT_DATA, cint16>::value && std::is_same<TT_COEFF, int16>::value) ? 2 : 1;
#else
    static constexpr unsigned int columnMultiple = 1;
#endif

    static constexpr int kSamplesInVectData = columnMultiple * m_kVOutSize;
    static constexpr int kSamplesInVectCoeff = columnMultiple * m_kVOutSize;
    static constexpr int kSamplesInVectAcc = m_kVOutSize;

    static constexpr unsigned int m_kLanes =
        fnNumLanesTdm<TT_DATA, TT_COEFF, TP_API>(); // number of operations in parallel of this type combinations
                                                    // that the vector processor can do.
    static constexpr unsigned int m_kFirRangeOffset =
        fnFirRangeOffsetAsym<TP_FIR_RANGE_LEN,
                             TP_CASC_LEN,
                             TP_KERNEL_POSITION,
                             TT_DATA,
                             TT_COEFF,
                             TP_API>() +
        TP_MODIFY_MARGIN_OFFSET; // FIR Cascade Offset for this kernel position

    // TDM FIR Margin = (TP_FIR_LEN-1)*TP_TDM_CHANNELS
    // or set to 0, if handled with internal buffer.
    static constexpr unsigned int enableInternalMargin = __HAS_ACCUM_PERMUTES__ ? 1 : 0;
    static constexpr unsigned int m_kFirMargin =
        fnTDMFirMargin<TP_FIR_LEN, TT_DATA, TP_TDM_CHANNELS, TP_INPUT_WINDOW_VSIZE, enableInternalMargin>();
    static constexpr eArchType m_kArch = m_kFirMargin == 0 ? kArchInternalMargin : kArchBasic;

    // Margin frame iteration state
    int marginFrame = 0;

    static constexpr unsigned int internalBufferSize = (m_kFirMargin == 0) ? (TP_FIR_LEN * TP_TDM_CHANNELS) : 32;
    // (TP_FIR_LEN-1)*TP_TDM_CHANNELS - margin samples + TP_TDM_CHANNELS new samples.
    TT_DATA (&m_inputBuffer)[internalBufferSize] = {0};
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
    // Implementations
    void filterInternalMargin(T_inputIF<TP_CASC_IN, TT_DATA, TP_DUAL_IP> inInterface,
                              T_outputIF<TP_CASC_OUT, TT_DATA> outInterface);

   public:
    static eArchType get_m_kArch() { return m_kArch; };

    static constexpr unsigned int get_Lanes() { return m_kVOutSize; };
    static constexpr unsigned int get_CoeffLanes() { return kSamplesInVectCoeff; };
    static constexpr unsigned int get_internalBufferSize() { return internalBufferSize; };
    static constexpr unsigned int get_margin() { return m_kFirMargin; };
    static constexpr unsigned int isInternalMarginEnabled() { return enableInternalMargin; };
    // need to switch graph beh if multiple columns are used.
    static unsigned int get_ColMultiple() { return columnMultiple; };

    // Constructor
    kernelFilterClass(TT_COEFF (&taps)[TP_FIR_LEN * TP_TDM_CHANNELS], TT_DATA (&inputBuffer)[get_internalBufferSize()])
        : m_internalTaps{taps}, m_inputBuffer{inputBuffer} {
        for (int i = 0; i < TP_FIR_LEN * TP_TDM_CHANNELS; i++) {
        }
    }

    // FIR
    void filterKernel(T_inputIF<TP_CASC_IN, TT_DATA, TP_DUAL_IP> inInterface,
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
    // (TP_FIR_LEN-1)*TP_TDM_CHANNELS - margin samples + TP_INPUT_WINDOW_VSIZE new samples, which are integer multiples
    // of TP_TDM_CHANNELS
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

    TT_DATA (&internalBuffer)[thisKernelFilterClass::get_internalBufferSize()];
    fir_tdm(TT_COEFF (&taps)[TP_FIR_LEN * TP_TDM_CHANNELS],
            TT_DATA (&inputBuffer)[thisKernelFilterClass::get_internalBufferSize()])
        : internalTaps(taps), internalBuffer(inputBuffer), thisKernelFilterClass(taps, inputBuffer) {}

    // Register Kernel Class
    static void registerKernelClass() {
        REGISTER_FUNCTION(fir_tdm::filter);
        REGISTER_PARAMETER(internalBuffer);
        REGISTER_PARAMETER(internalTaps);
    }

    // FIR
    void filter(input_circular_buffer<TT_DATA,
                                      extents<inherited_extent>,
                                      margin<thisKernelFilterClass::get_margin()> >& __restrict inWindow,
                output_circular_buffer<TT_DATA>& __restrict outWindow);
};
}
}
}
}
}

#endif // _DSPLIB_FIR_TDM_HPP_
