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
#ifndef _DSPLIB_FIR_TDM_HPP_
#define _DSPLIB_FIR_TDM_HPP_

#include <adf.h>
#include <vector>
#include "fir_utils.hpp"

#if __HAS_ACCUM_PERMUTES__ == 1
#define __FIR_TDM_USE_48BIT_ACC__ 1
#else
#define __FIR_TDM_USE_48BIT_ACC__ 0
#endif

#include "fir_tdm_traits.hpp"

namespace xf {
namespace dsp {
namespace aie {
namespace fir {
namespace tdm {

template <typename TT_DATA,
          typename TT_OUT_DATA,
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
class fir_tdm;

template <typename fp = fir_params_defaults>
class fir_tdm_tl {
   public:
    using parent_class = fir_tdm<typename fp::BTT_DATA,
                                 typename fp::BTT_OUT_DATA,
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
                ? fnFirRangeRem<fp::BTP_FIR_LEN, fp::BTP_CASC_LEN, pos, parent_class::get_ColMultiple()>()
                : fnFirRange<fp::BTP_FIR_LEN, fp::BTP_CASC_LEN, pos, parent_class::get_ColMultiple()>();
        return firRangeLen;
    };

    template <int pos, int CLEN, int T_FIR_LEN, typename T_D, typename T_C, unsigned int SSR>
    static constexpr unsigned int fnCheckIfFits() {
        return 1;
    }

    // Get kernel's FIR range Length
    static constexpr unsigned int getFirRangeLen() { return fp::BTP_FIR_RANGE_LEN; };
    // static constexpr unsigned int getFirRangeLen() {
    //     return CEIL(getKernelFirRangeLen<fp::BTP_KERNEL_POSITION>() * fp::BTP_TDM_CHANNELS, fp::BTP_SSR) /
    //     fp::BTP_SSR;
    // };

    // Get kernel's FIR Total Tap Length
    static constexpr unsigned int getTapLen() {
        return CEIL(fp::BTP_FIR_LEN * fp::BTP_TDM_CHANNELS, fp::BTP_SSR) / fp::BTP_SSR;
        // return CEIL(getKernelFirRangeLen<fp::BTP_KERNEL_POSITION>() * fp::BTP_TDM_CHANNELS, fp::BTP_SSR) /
        // fp::BTP_SSR;
    };

    // Get kernel's FIR Total Tap Length
    static constexpr unsigned int getSSRMargin() { return parent_class::get_margin(); };

    static constexpr unsigned int getDF() { return 1; };
    static constexpr unsigned int getIF() { return 1; };

    static constexpr unsigned int getLanes() { return parent_class::get_Lanes(); };
    static constexpr unsigned int getInternalBufferSize() { return parent_class::get_internalBufferSize(); };
    static constexpr unsigned int getInternalTapsSize() { return parent_class::get_internalTapsSize(); };
    static constexpr unsigned int isInternalMarginEnabled() { return parent_class::isInternalMarginEnabled(); };
    static constexpr unsigned int getColMultiple() { return parent_class::get_ColMultiple(); };
    static constexpr unsigned int getFirRangeOffset() { return parent_class::get_FirRangeOffset(); };
    static constexpr unsigned int getFirCoeffOffset() { return parent_class::get_FirCoeffOffset(); };

    // Get FIR variant
    static constexpr eFIRVariant getFirType() { return eFIRVariant::kTDM; };
    // Get FIR source file
    static const char* getFirSource() { return "fir_tdm.cpp"; };
};
//-----------------------------------------------------------------------------------------------------
template <typename TT_DATA,
          typename TT_OUT_DATA,
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
    static_assert(TP_NUM_OUTPUTS > 0 && TP_NUM_OUTPUTS <= 2, "ERROR: only single or dual outputs are supported.");
#if __SUPPORTS_CFLOAT__ == 1
    static_assert(!(std::is_same<TT_DATA, cfloat>::value || std::is_same<TT_DATA, float>::value) || (TP_SHIFT == 0),
                  "ERROR: TP_SHIFT cannot be performed for TT_DATA=cfloat, so must be set to 0");
#else
    static_assert(!(std::is_same<TT_DATA, float>::value) || (TP_SHIFT == 0),
                  "ERROR: TP_SHIFT cannot be performed for TT_DATA=cfloat, so must be set to 0");
#endif

    // typedef typename std::conditional_t<std::is_same<TT_COEFF, int32>::value,
    //                                     int16,
    //                                     std::conditional_t<std::is_same<TT_COEFF, cint32>::value, cint16, TT_COEFF> >
    //     TT_COEFF_TEST;
    static constexpr unsigned int m_kVOutSize = fnVOutSizeTdm<TT_DATA, TT_COEFF>();
    static constexpr int TP_NUM_FRAMES = TP_INPUT_WINDOW_VSIZE / TP_TDM_CHANNELS;
    static constexpr int TP_TDM_LOOP_SIZE = TP_TDM_CHANNELS / m_kVOutSize;
    static_assert(TP_TDM_CHANNELS % m_kVOutSize == 0,
                  "ERROR: Number of TDM Channels split over SSR paths (TP_TDM_CHANNELS / TP_SSR) must be a integer "
                  "multiple of lanes vector processor operates on (8, 16 or 32).");

// always assume data >= coeff. otherwise, we'd need to load data at 0.5 rate, i.e. every second iteration + add offset
#if __HAS_ACCUM_PERMUTES__ == 1
    // cint16/int16 combo can be overloaded with 2 column MUL/MACs.
    static constexpr unsigned int columnMultiple =
        (std::is_same<TT_DATA, cint16>::value && std::is_same<TT_COEFF, int16>::value) ? 2 : 1;
    static constexpr unsigned int coeffToDataMultiple = 1;
#else
    static constexpr unsigned int columnMultiple = 1;
    // When 512-bits of coeffs are needed for 256-bits of data, it takes 2 clock cycles to fetch data for a VMAC.
    // If 2 operations are unrolled and separate pointers added for coeff reads, can we squeeze 6 operations in 3 clock
    // cycles, rather than 4?
    // No, as the 4 coeff reads would need to be scheduled in 3 clock cycles, causing memory conflict, defeating the
    // purpose.

    // How about operating on multiple frames?
    // Yes, that would work. Read coeffs once. Read data for first frame and then for second frame.
    // 512-bits of data and 512-bits of coeffs for 2 VMACs.
    // static constexpr unsigned int coeffToDataMultiple =
    // (TP_NUM_FRAMES % 2 == 0
    // && std::is_same<TT_COEFF, cint32>::value) ? 2 : 1;
    // && std::is_same<TT_DATA, cint16>::value) ? 2 : 1;
    // && std::is_same<TT_DATA, cint16>::value && std::is_same<TT_COEFF, cint32>::value) ? 2 : 1;
    // Does it not make sense to enable it for all data types?
    static constexpr unsigned int coeffToDataMultiple = (TP_NUM_FRAMES % 2 == 0) ? 2 : 1;
#endif

// Unrolling may offer better pipelining opportunities, at the cost of increased program memory.
// However, unrolling doesn't always offer best performance.
// When loop count is around forUnroolThreshold or more, it is best not to unroll and allow a nested loop.
#if __HAS_ACCUM_PERMUTES__ == 1
    static constexpr unsigned int firUnrollLoop = TP_FIR_RANGE_LEN / columnMultiple;
#else
    static constexpr unsigned int forUnroolThreshold = 16;
    static constexpr unsigned int firUnrollLoop =
        (TP_FIR_RANGE_LEN / columnMultiple >= forUnroolThreshold) ? 1 : TP_FIR_RANGE_LEN / columnMultiple;
#endif // _DSPLIB_FIR_TDM_HPP_DEBUG_

    static constexpr int kSamplesInVectData = columnMultiple * m_kVOutSize;
    static constexpr int kSamplesInVectCoeff = columnMultiple * m_kVOutSize;
    static constexpr int kSamplesInVectAcc = m_kVOutSize;

    static constexpr unsigned int m_kLanes =
        fnNumLanesTdm<TT_DATA, TT_COEFF>(); // number of operations in parallel of this type combinations
                                            // that the vector processor can do.
    static constexpr unsigned int m_kFirRangeOffset =
        fnFirRangeOffset<TP_FIR_LEN,
                         TP_CASC_LEN,
                         TP_KERNEL_POSITION,
                         columnMultiple>() +
        TP_MODIFY_MARGIN_OFFSET; // FIR Cascade Offset for this kernel position

    // Coeffs are reversed. Need to apply the offset from the back, so that the last kernel starts with a zero.
    static constexpr unsigned int m_kFirCoeffOffset = TP_FIR_LEN - TP_FIR_RANGE_LEN - m_kFirRangeOffset;

    // Operate on multiple frames in parallel, when possible.
    // Optimized to reduce data loads, handy when 512-bits of data and 256-bits of coeffs are needed on each clock
    // cycle.
    static constexpr unsigned int useEvenFrames = (TP_NUM_FRAMES % 2 == 0 && columnMultiple == 2) ? 1 : 0;
    // TDM FIR Margin = (TP_FIR_LEN-1)*TP_TDM_CHANNELS
    // or set to 0, if handled with internal buffer.
    static constexpr unsigned int enableInternalMargin = __HAS_ACCUM_PERMUTES__ ? 1 : 0;
    static constexpr unsigned int m_kFirMargin =
        fnTDMFirMargin<TP_FIR_LEN, TT_DATA, TP_TDM_CHANNELS, TP_INPUT_WINDOW_VSIZE, enableInternalMargin>();
    static constexpr eArchType m_ArchInternalMargin =
        useEvenFrames == 1 ? kArchInternalMarginEvenFrames : kArchInternalMargin;
    static constexpr eArchType m_ArchExternalMargin =
        useEvenFrames == 1 ? kArchExternalMarginEvenFrames : kArchExternalMargin;
    static constexpr eArchType m_kArch = m_kFirMargin == 0 ? m_ArchInternalMargin : m_ArchExternalMargin;

    // Margin frame iteration state
    int marginFrame = 0;
    // TT_COEFF* __restrict m_inTapsPtr;

    // Need to store  margin for ALL FIR computation, not just the FIR_RANGE_LEN.
    static constexpr unsigned int internalBufferFrames = (TP_FIR_LEN + useEvenFrames);
    static constexpr unsigned int internalBufferSize =
        (m_kFirMargin == 0) ? (internalBufferFrames * TP_TDM_CHANNELS) : 32;
    static constexpr unsigned int internalTapsSize = TP_FIR_RANGE_LEN * TP_TDM_CHANNELS;
    // (TP_FIR_LEN-1)*TP_TDM_CHANNELS - margin samples + TP_TDM_CHANNELS new samples.
    alignas(__ALIGN_BYTE_SIZE__) TT_DATA (&m_inputBuffer)[internalBufferSize] = {0};
    // alignas(__ALIGN_BYTE_SIZE__) TT_COEFF (&m_internalTaps)[internalTapsSize];
    // requires #include <optional>
    // alignas(__ALIGN_BYTE_SIZE__) std::optional<TT_COEFF> (&m_internalTaps)[internalTapsSize];
    // alignas(__ALIGN_BYTE_SIZE__) std::optional<std::array<TT_COEFF, internalTapsSize>> &m_internalTaps; // Optional
    // array of shorts

    static constexpr kernelPositionState m_kKernelPosEnum = getKernelPositionState(TP_KERNEL_POSITION, TP_CASC_LEN);

    // Additional defensive checks
    static_assert(TP_INPUT_WINDOW_VSIZE % m_kLanes == 0,
                  "ERROR: TP_INPUT_WINDOW_VSIZE must be an integer multiple of the number of lanes for this data type");
    // Coefficient Load Size - number of samples in a cascade read/write
    static constexpr unsigned int m_kCoeffLoadSize = SCD_SIZE / 8 / sizeof(TT_COEFF);

    void filterSelectArch(T_inputIF<TP_CASC_IN, TT_DATA, TP_DUAL_IP> inInterface,
                          T_outputIF<TP_CASC_OUT, TT_OUT_DATA> outInterface);
    // Implementations
    void filterBasic(T_inputIF<TP_CASC_IN, TT_DATA, TP_DUAL_IP> inInterface,
                     T_outputIF<TP_CASC_OUT, TT_OUT_DATA> outInterface);
    // Implementations
    void filterInternalMargin(T_inputIF<TP_CASC_IN, TT_DATA, TP_DUAL_IP> inInterface,
                              T_outputIF<TP_CASC_OUT, TT_OUT_DATA> outInterface);
    void filterInternalMarginEvenFrames(T_inputIF<TP_CASC_IN, TT_DATA, TP_DUAL_IP> inInterface,
                                        T_outputIF<TP_CASC_OUT, TT_OUT_DATA> outInterface);

   public:
    TT_COEFF* __restrict m_inTapsPtr;
    TT_COEFF* __restrict m_internalTaps;

    static eArchType get_m_kArch() { return m_kArch; };
    void setInTapsPtr(const TT_COEFF (&inTaps)[internalBufferSize]) { m_inTapsPtr = (TT_COEFF*)inTaps; };

    static constexpr unsigned int get_Lanes() { return m_kVOutSize; };
    static constexpr unsigned int get_CoeffLanes() { return kSamplesInVectCoeff; };
    static constexpr unsigned int get_internalBufferSize() { return internalBufferSize; };
    static constexpr unsigned int get_internalTapsSize() { return internalTapsSize; };
    static constexpr unsigned int get_margin() { return m_kFirMargin; };
    static constexpr unsigned int isInternalMarginEnabled() { return enableInternalMargin; };
    // need to switch graph beh if multiple columns are used.
    static constexpr unsigned int get_ColMultiple() { return columnMultiple; };
    static constexpr unsigned int get_FirRangeOffset() { return m_kFirRangeOffset; };
    static constexpr unsigned int get_FirCoeffOffset() { return m_kFirCoeffOffset; };

    // Constructor
    kernelFilterClass(TT_COEFF (&taps)[internalTapsSize], TT_DATA (&inputBuffer)[get_internalBufferSize()])
        : m_internalTaps{taps}, m_inputBuffer{inputBuffer} {}

    // RTP Constructor
    kernelFilterClass(TT_DATA (&inputBuffer)[get_internalBufferSize()])
        : m_internalTaps(nullptr), m_inputBuffer{inputBuffer} {}

    // FIR
    void filterKernel(T_inputIF<TP_CASC_IN, TT_DATA, TP_DUAL_IP> inInterface,
                      T_outputIF<TP_CASC_OUT, TT_OUT_DATA> outInterface);
};

//-----------------------------------------------------------------------------------------------------
// Single kernel base specialization. No cascade ports. Static coefficients
template <typename TT_DATA,
          typename TT_OUT_DATA,
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
class fir_tdm : public kernelFilterClass<TT_DATA,
                                         TT_OUT_DATA,
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
    // Constructor
    using thisKernelFilterClass = kernelFilterClass<TT_DATA,
                                                    TT_OUT_DATA,
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

    static constexpr unsigned int internalTapsSize = thisKernelFilterClass::get_internalTapsSize();
    static constexpr unsigned int internalBufferSize = thisKernelFilterClass::get_internalBufferSize();
    TT_COEFF (&internalTaps)[internalTapsSize];
    TT_DATA (&internalBuffer)[internalBufferSize];
    fir_tdm(TT_COEFF (&taps)[internalTapsSize], TT_DATA (&inputBuffer)[internalBufferSize])
        : internalTaps(taps), internalBuffer(inputBuffer), thisKernelFilterClass(taps, inputBuffer){};

    // Register Kernel Class
    static void registerKernelClass() {
        if
            constexpr(TP_CASC_LEN == 1) { REGISTER_FUNCTION(fir_tdm::filter); }
        else if
            constexpr(TP_KERNEL_POSITION == 0) { REGISTER_FUNCTION(fir_tdm::filterFirst); }
        else if
            constexpr(TP_KERNEL_POSITION == TP_CASC_LEN - 1) { REGISTER_FUNCTION(fir_tdm::filterLast); }
        else {
            REGISTER_FUNCTION(fir_tdm::filterMiddle);
        }
        REGISTER_PARAMETER(internalBuffer);
        REGISTER_PARAMETER(internalTaps);
    }

    // FIR TDM - single kernel
    void filter(input_circular_buffer<TT_DATA,
                                      extents<inherited_extent>,
                                      margin<thisKernelFilterClass::get_margin()> >& __restrict inWindow,
                output_circular_buffer<TT_OUT_DATA>& __restrict outWindow);
    // FIR First in a cascade
    void filterFirst(input_circular_buffer<TT_DATA,
                                           extents<inherited_extent>,
                                           margin<thisKernelFilterClass::get_margin()> >& __restrict inWindow,
                     output_cascade_cacc* outCascade);
    // FIR Middle in the cascade
    void filterMiddle(input_circular_buffer<TT_DATA,
                                            extents<inherited_extent>,
                                            margin<thisKernelFilterClass::get_margin()> >& __restrict inWindow,
                      input_cascade_cacc* inCascade,
                      output_cascade_cacc* outCascade);
    // FIR Last
    void filterLast(input_circular_buffer<TT_DATA,
                                          extents<inherited_extent>,
                                          margin<thisKernelFilterClass::get_margin()> >& __restrict inWindow,
                    input_cascade_cacc* inCascade,
                    output_circular_buffer<TT_OUT_DATA>& __restrict outWindow);
};

//-----------------------------------------------------------------------------------------------------
// Single kernel base specialization. No cascade ports. Static coefficients
template <typename TT_DATA,
          typename TT_OUT_DATA,
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
          unsigned int TP_DUAL_IP,
          unsigned int TP_API,
          int TP_MODIFY_MARGIN_OFFSET,
          unsigned int TP_COEFF_PHASE,
          unsigned int TP_COEFF_PHASE_OFFSET,
          unsigned int TP_COEFF_PHASES,
          unsigned int TP_COEFF_PHASES_LEN,
          unsigned int TP_SAT>
class fir_tdm<TT_DATA,
              TT_OUT_DATA,
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
              USE_COEFF_RELOAD_TRUE,
              TP_NUM_OUTPUTS,
              TP_DUAL_IP,
              TP_API,
              TP_MODIFY_MARGIN_OFFSET,
              TP_COEFF_PHASE,
              TP_COEFF_PHASE_OFFSET,
              TP_COEFF_PHASES,
              TP_COEFF_PHASES_LEN,
              TP_SAT> : public kernelFilterClass<TT_DATA,
                                                 TT_OUT_DATA,
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
                                                 USE_COEFF_RELOAD_TRUE,
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
    // Constructor
    using thisKernelFilterClass = kernelFilterClass<TT_DATA,
                                                    TT_OUT_DATA,
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
                                                    USE_COEFF_RELOAD_TRUE,
                                                    TP_NUM_OUTPUTS,
                                                    TP_DUAL_IP,
                                                    TP_API,
                                                    TP_MODIFY_MARGIN_OFFSET,
                                                    TP_COEFF_PHASE,
                                                    TP_COEFF_PHASE_OFFSET,
                                                    TP_COEFF_PHASES,
                                                    TP_COEFF_PHASES_LEN,
                                                    TP_SAT>;

    static constexpr unsigned int internalTapsSize = thisKernelFilterClass::get_internalTapsSize();
    static constexpr unsigned int internalBufferSize = thisKernelFilterClass::get_internalBufferSize();
    TT_DATA (&internalBuffer)[internalBufferSize];

    // RTP Conscturtor - no internap taps
    fir_tdm(TT_DATA (&inputBuffer)[internalBufferSize])
        : internalBuffer(inputBuffer), thisKernelFilterClass(inputBuffer){};

    // Register Kernel Class
    static void registerKernelClass() {
        if
            constexpr(TP_CASC_LEN == 1) { REGISTER_FUNCTION(fir_tdm::filterRtp); }
        else if
            constexpr(TP_KERNEL_POSITION == 0) { REGISTER_FUNCTION(fir_tdm::filterFirstRtp); }
        else if
            constexpr(TP_KERNEL_POSITION == TP_CASC_LEN - 1) { REGISTER_FUNCTION(fir_tdm::filterLastRtp); }
        else {
            REGISTER_FUNCTION(fir_tdm::filterMiddleRtp);
        }

        REGISTER_PARAMETER(internalBuffer);
    }

    // FIR RTP - single kernel
    void filterRtp(input_circular_buffer<TT_DATA,
                                         extents<inherited_extent>,
                                         margin<thisKernelFilterClass::get_margin()> >& __restrict inWindow,
                   output_circular_buffer<TT_OUT_DATA>& __restrict outWindow,
                   const TT_COEFF (&inTaps)[internalTapsSize]);

    // FIR RTP First
    void filterFirstRtp(input_circular_buffer<TT_DATA,
                                              extents<inherited_extent>,
                                              margin<thisKernelFilterClass::get_margin()> >& __restrict inWindow,
                        output_cascade_cacc* outCascade,
                        const TT_COEFF (&inTaps)[internalTapsSize]);

    // FIR RTP Middle
    void filterMiddleRtp(input_circular_buffer<TT_DATA,
                                               extents<inherited_extent>,
                                               margin<thisKernelFilterClass::get_margin()> >& __restrict inWindow,
                         input_cascade_cacc* inCascade,
                         output_cascade_cacc* outCascade,
                         const TT_COEFF (&inTaps)[internalTapsSize]);

    // FIR RTP last
    void filterLastRtp(input_circular_buffer<TT_DATA,
                                             extents<inherited_extent>,
                                             margin<thisKernelFilterClass::get_margin()> >& __restrict inWindow,
                       input_cascade_cacc* inCascade,
                       output_circular_buffer<TT_OUT_DATA>& __restrict outWindow,
                       const TT_COEFF (&inTaps)[internalTapsSize]);
};
}
}
}
}
}

#endif // _DSPLIB_FIR_TDM_HPP_
