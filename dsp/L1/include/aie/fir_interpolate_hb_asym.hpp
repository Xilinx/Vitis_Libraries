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
#ifndef _DSPLIB_fir_interpolate_hb_asym_HPP_
#define _DSPLIB_fir_interpolate_hb_asym_HPP_

/*
  Halfband interpolating FIR.
  This file exists to capture the definition of the FIR filter kernel class.
  The class definition holds defensive checks on parameter range and other
  legality.
  The constructor definition is held in this class because this class must be
  accessible to graph level aie compilation.
  The main runtime filter function is captured elsewhere as it contains aie
  intrinsics which are not included in aie graph level compilation.
*/

#include <adf.h>
#include <assert.h>
#include <array>
#include <cstdint>
#include <vector>

#include "fir_utils.hpp"
#include "fir_interpolate_hb_traits.hpp"
#include "fir_sr_asym.hpp"
#if __HAS_SYM_PREADD__ == 1
#define getHbTaps(x) (((x + 1) / 4) + 1)
#else
#define getHbTaps(x) (((x + 1) / 2) + 1)
#endif

namespace xf {
namespace dsp {
namespace aie {
namespace fir {
namespace interpolate_hb_asym {
using namespace interpolate_hb;

enum eArchType { kArchBasic = 0, kArchIncLoads, kArchStream };

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
          unsigned int TP_UPSHIFT_CT,
          unsigned int TP_API,
          unsigned int TP_SAT>
class kernelFilterClass {
   private:
    // Parameter value defensive and legality checks
    static_assert(TP_FIR_RANGE_LEN >= FIR_LEN_MIN,
                  "ERROR: Illegal combination of design FIR length and cascade length, resulting in kernel FIR length "
                  "below minimum required value. ");
    static_assert(TP_SHIFT >= SHIFT_MIN && TP_SHIFT <= SHIFT_MAX, "ERROR: SHIFT is out of the supported range.");
    static_assert(TP_RND >= ROUND_MIN && TP_RND <= ROUND_MAX, "ERROR: RND is out of the supported range.");

    static_assert(fnEnumType<TT_DATA>() != enumUnknownType, "ERROR: TT_DATA is not a supported type.");
    static_assert(fnEnumType<TT_COEFF>() != enumUnknownType, "ERROR: TT_COEFF is not a supported type.");
    static_assert(fnTypeCheckDataCoeffFltInt<TT_DATA, TT_COEFF>() != 0,
                  "ERROR: a mix of float and integer types of TT_DATA and TT_COEFF is not supported.");

    static constexpr bool m_kHasCT =
        (TP_CASC_LEN % 2 == 0)
            ? (TP_KERNEL_POSITION == TP_CASC_LEN / 2 ? true : false)
            : (TP_KERNEL_POSITION == TP_CASC_LEN / 2
                   ? true
                   : false); // the center kernel calculates the center tap and passes it on to the cascade
    static constexpr unsigned int m_kNumTaps =
        (TP_KERNEL_POSITION == TP_CASC_LEN - 1)
            ? (TP_FIR_RANGE_LEN + 1) / 2
            : TP_FIR_RANGE_LEN / 2; // FIR_LEN is always an odd number, and the total number of non-zero and non CT taps
                                    // is (FIR_LEN+1)/2. This factor of needs to be counted in the last kernel.
    static constexpr unsigned int m_kTotTaps = (TP_FIR_LEN + 1) / kInterpolateFactor + 1;
    static constexpr unsigned int m_kFirAsymTaps = m_kTotTaps - 1; // total taps minus the 1 center tap
    static constexpr unsigned int m_kColumns = fnNumSymColsIntHb<TT_DATA, TT_COEFF>();
    static constexpr unsigned int m_kFirLenCeilCols = CEIL(m_kNumTaps, m_kColumns);
    static constexpr unsigned int m_kFirRangeOffset =
        fnFirRangeOffset<TP_FIR_LEN, TP_CASC_LEN, TP_KERNEL_POSITION, 2>() /
        kInterpolateFactor; // refers to fir range offset of the single rate filter that performs top polyphase
                            // computation
    static constexpr unsigned int m_kWinAccessByteSize = 32;
    static constexpr unsigned int m_kLanes =
        fnNumSymLanesIntHb<TT_DATA, TT_COEFF, TP_API, TP_UPSHIFT_CT>(); // number of operations in parallel of this type
                                                                        // combinations that the vector processor can
                                                                        // do.
    static constexpr unsigned int m_kNumOps =
        CEIL((TP_FIR_RANGE_LEN + 1) / kInterpolateFactor, m_kColumns) / m_kColumns;
    static constexpr unsigned int m_kLsize = TP_INPUT_WINDOW_VSIZE / m_kLanes; // loops required to consume input
    static constexpr unsigned int m_kDataLoadVsize = 256 / 8 / sizeof(TT_DATA);
    static constexpr unsigned int m_kVOutSize =
        m_kLanes * kInterpolateFactor; // This differs from kLanes for cint32/cint32
    static constexpr unsigned int m_kFirMarginLen = fnFirMargin<m_kFirAsymTaps, TT_DATA>();
    static constexpr unsigned int m_kFirMarginOffset =
        fnFirMargin<m_kFirAsymTaps, TT_DATA>() - (m_kFirAsymTaps) + 1; // FIR Margin Offset.
    static constexpr unsigned int m_kFirMarginRangeOffset =
        m_kFirMarginOffset + m_kFirRangeOffset; // FIR Margin Offset.
    static constexpr unsigned int m_kFirInitOffset = m_kFirMarginRangeOffset;
    static constexpr unsigned int m_kDataBuffXOffset =
        m_kFirInitOffset % (m_kWinAccessByteSize / sizeof(TT_DATA)); // Remainder of m_kFirInitOffset divided by 128bit
    static constexpr unsigned int m_kXDataLoadInitOffset =
        TRUNC(m_kFirInitOffset,
              (m_kWinAccessByteSize / sizeof(TT_DATA))); // Xbuff window offset, aligned to load vector size.
    static constexpr unsigned int m_kArchFirLen =
        m_kNumTaps + m_kDataBuffXOffset; // This will check if only the portion of the FIR (TP_FIR_RANGE_LEN +
                                         // m_kDataBuffXOffset - due to xoffset alignment) fits.
    static constexpr unsigned int m_kTotDataNeededPerOp =
        TP_API == 0
            ? (m_kArchFirLen + m_kDataLoadVsize * 2 - 1)
            : (m_kArchFirLen + m_kDataLoadVsize -
               1); // the factor of 2 comes because the incloads architecture performs two 256-bit reads in steady state

    static constexpr unsigned int m_kZbuffSize = 32; // bytes
    static constexpr unsigned int m_kCoeffRegVsize = m_kZbuffSize / sizeof(TT_COEFF);
    static constexpr unsigned int m_kSamplesInBuff = 1024 / 8 / sizeof(TT_DATA);
    static constexpr unsigned int m_kDataLoadsInReg = m_kSamplesInBuff / m_kDataLoadVsize;
    static constexpr eArchType m_kArch =
        TP_API == 0 ? m_kTotDataNeededPerOp > m_kSamplesInBuff ? kArchBasic : kArchIncLoads : kArchStream;
    static constexpr unsigned int m_kInitialLoads =
        m_kArch == kArchIncLoads
            ? CEIL(m_kTotDataNeededPerOp, m_kDataLoadVsize) / m_kDataLoadVsize
            : CEIL(m_kDataBuffXOffset + m_kLanes + m_kColumns - 1, m_kDataLoadVsize) / m_kDataLoadVsize;
    ;
    static constexpr unsigned int m_kNumCTZeroes =
        (TP_FIR_LEN + 1) / 4; // first position at which CT multiplies with non-margin data
    static constexpr unsigned int ctPos =
        (m_kFirMarginLen - m_kXDataLoadInitOffset - ((TP_FIR_LEN) / 4)) % m_kSamplesInBuff;

    // this following variables calculate which is the right point at which enough data is available to calculate the
    // low polyphase so we can calculate the low polyphase before data gets overwritten.
    static constexpr unsigned int m_kInitDataNeeded =
        m_kDataBuffXOffset + m_kLanes - 1 + m_kColumns; // filter1buff specific.
    static constexpr unsigned int m_kMinDataNeededLowPh =
        (m_kFirMarginOffset + m_kNumCTZeroes + m_kLanes) - m_kXDataLoadInitOffset;
    static constexpr unsigned int m_kMinDataLoadedLowPh = CEIL(m_kMinDataNeededLowPh, m_kDataLoadVsize);
    static constexpr unsigned int m_kMinDataNeededHighPhForCT = m_kMinDataLoadedLowPh - m_kDataLoadVsize + 1;
    static constexpr unsigned int m_kMinOp =
        m_kMinDataNeededHighPhForCT < m_kInitDataNeeded ? 0 : m_kMinDataNeededHighPhForCT - m_kInitDataNeeded;
    static constexpr int m_kCTOp = CEIL(m_kMinOp, m_kColumns);

    // needed for stream architecture
    static constexpr int streamRptFactor = m_kSamplesInBuff / m_kLanes;
    static constexpr int marginLoadsMappedToBuff = (m_kFirMarginLen % m_kSamplesInBuff) / m_kDataLoadVsize;
    static constexpr int streamDataOffsetWithinBuff = (m_kFirInitOffset) % m_kSamplesInBuff;
    static constexpr int streamInitNullAccs =
        ((TP_FIR_LEN - TP_FIR_RANGE_LEN - m_kFirRangeOffset * 2 + 1) / 2 / m_kLanes);
    static constexpr int streamInitAccs = (CEIL(streamInitNullAccs, streamRptFactor) - streamInitNullAccs);
    static constexpr int lowPolyPhaseNullAccs =
        TP_FIR_LEN / 4 / m_kLanes; // refers to the position of center tap in the filter
    static constexpr int startDataLoads = marginLoadsMappedToBuff + streamInitAccs * m_kLanes / m_kDataLoadVsize;
    static constexpr int startDataOffset = streamDataOffsetWithinBuff % m_kSamplesInBuff;
    int doInit = (streamInitNullAccs == 0) ? 0 : 1;

    // Lower polyphase taps internal storage. Initialised to zeros.
    alignas(__ALIGN_BYTE_SIZE__) TT_COEFF
        m_internalTaps[CEIL(m_kNumTaps, m_kCoeffRegVsize)]; // Filter taps/coefficients
    alignas(__ALIGN_BYTE_SIZE__) TT_COEFF m_phaseTwoTap[kMaxColumns] = {
        nullElem<TT_COEFF>()}; // note, the array is initializeed, causing extra instructions during initialiation.

    alignas(__ALIGN_BYTE_SIZE__) TT_COEFF
        m_oldInTaps[CEIL(m_kTotTaps, m_kCoeffRegVsize)]; // Previous user input coefficients with zero padding
    bool m_coeffnEq;                                     // Are coefficients sets equal?

    void filterSelectArch(T_inputIF<TP_CASC_IN, TT_DATA, TP_DUAL_IP> inInterface,
                          T_outputIF<TP_CASC_OUT, TT_DATA> outInterface);
    void filterIncLoads(T_inputIF<TP_CASC_IN, TT_DATA, TP_DUAL_IP> inInterface,
                        T_outputIF<TP_CASC_OUT, TT_DATA> outInterface);
    void filterBasic(T_inputIF<TP_CASC_IN, TT_DATA, TP_DUAL_IP> inInterface,
                     T_outputIF<TP_CASC_OUT, TT_DATA> outInterface);
    void filterStream(T_inputIF<TP_CASC_IN, TT_DATA, TP_DUAL_IP> inInterface,
                      T_outputIF<TP_CASC_OUT, TT_DATA> outInterface);
    alignas(__ALIGN_BYTE_SIZE__) int delay[(1024 / 8) / sizeof(int)] = {
        0}; // 1024b buffer to retain margin data between calls

    // Additional defensive checks
    static_assert(TP_INPUT_WINDOW_VSIZE % m_kLanes == 0,
                  "ERROR: TP_INPUT_WINDOW_VSIZE must be an integer multiple of the number of lanes for this data type");
    static_assert(TP_API == 0 || m_kTotDataNeededPerOp <= m_kSamplesInBuff,
                  "ERROR: TP_FIR_RANGE_LEN exceeds max supported range for this data/coeff type combination. Increase "
                  "TP_CASC_LEN to split the workload over more kernels.");

   public:
    // Access function for AIE Synthesizer
    static unsigned int get_m_kArch() { return -1; };

    // Constructors
    kernelFilterClass() : m_oldInTaps{}, m_internalTaps{} {}

    kernelFilterClass(const TT_COEFF (&taps)[getHbTaps(TP_FIR_LEN)]) : m_internalTaps{} { firReload(taps); };

    void firReload(const TT_COEFF* taps) {
        TT_COEFF* tapsPtr = (TT_COEFF*)taps;
        firReload(tapsPtr);
    }

    void firReload(TT_COEFF* taps) { // printf("in the kernel constructor - %d", TP_FIR_RANGE_LEN);
        // Coefficients are pre-arranged, so thats
        for (int i = 0; i < CEIL(m_kTotTaps, kMaxColumns); i++) {
            if (i < m_kNumTaps) {
                unsigned int tapsAddress = (TP_FIR_LEN + 1) / 2 - 1 - m_kFirRangeOffset - i;
                m_internalTaps[i] = taps[tapsAddress];
            } else {
                m_internalTaps[i] = nullElem<TT_COEFF>();
            }
        }
        if (TP_UPSHIFT_CT == 0) {
            // The centre tap requires only one coefficient, but this is a vector processor with
            // multiple columns in the mul intrinsic, so the other columns must be zero'd
            m_phaseTwoTap[0] = taps[m_kTotTaps - 1];
        }
    }

    // FIR
    void filterKernel(T_inputIF<TP_CASC_IN, TT_DATA, TP_DUAL_IP> inInterface,
                      T_outputIF<TP_CASC_OUT, TT_DATA> outInterface);

    // with taps for reload
    void filterKernel(T_inputIF<TP_CASC_IN, TT_DATA, TP_DUAL_IP> inInterface,
                      T_outputIF<TP_CASC_OUT, TT_DATA> outInterface,
                      const TT_COEFF (&inTaps)[getHbTaps(TP_FIR_LEN)]);

    void filterKernelRtp(T_inputIF<TP_CASC_IN, TT_DATA, TP_DUAL_IP> inInterface,
                         T_outputIF<TP_CASC_OUT, TT_DATA> outInterface);
};

//-----------------------------------------------------------------------------------------------------
// Cascade layer class and specializations
// xf::dsp::aie::fir::interpolate_hb::kernelFilterClass<cint16, short, 11u, 15u, 0u, 128u, false, false, 11u, 0u, 1u,
// 0u, 0u, 1u, 0u, 0u, 0u>::filterKernel(xf::dsp::aie::fir::T_inputIF<false, cint16, 0u>,
// xf::dsp::aie::fir::T_outputIF<false, cint16>)
// <cint16, short, 11u, 15u, 0u, 128u, false, false, 13u, 0u, 1u, 0u, 0u, 1u, 0u, 0u, 0u>
//-----------------------------------------------------------------------------------------------------
// This is the main declaration of the fir_interpolate_hb_asym class, and is also used for the
// Standalone kernel specialization with no cascade ports, Windowed. a single input and no reload, single output
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
          unsigned int TP_UPSHIFT_CT = 0,
          unsigned int TP_API = 0,
          unsigned int TP_SAT = 1>
class fir_interpolate_hb_asym : public kernelFilterClass<TT_DATA,
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
                                                         DUAL_IP_SINGLE,
                                                         USE_COEFF_RELOAD_FALSE,
                                                         TP_NUM_OUTPUTS,
                                                         TP_UPSHIFT_CT,
                                                         USE_WINDOW_API,
                                                         TP_SAT>

{
   public:
    // Constructor
    fir_interpolate_hb_asym(const TT_COEFF (&taps)[getHbTaps(TP_FIR_LEN)])
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
                            DUAL_IP_SINGLE,
                            USE_COEFF_RELOAD_FALSE,
                            TP_NUM_OUTPUTS,
                            TP_UPSHIFT_CT,
                            USE_WINDOW_API,
                            TP_SAT>(taps) {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fir_interpolate_hb_asym::filter); }
    // FIR
    void filter(
        input_circular_buffer<TT_DATA,
                              extents<inherited_extent>,
                              margin<fnFirMargin<TP_FIR_LEN / kInterpolateFactor, TT_DATA>()> >& __restrict inWindow,
        output_circular_buffer<TT_DATA>& __restrict outWindow);
};

//-----------------------------------------------------------------------------------------------------
// Single kernel specialization. No cascade ports, Windowed. single input, static  coefficients, dual output
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_FIR_RANGE_LEN,
          unsigned int TP_UPSHIFT_CT,
          unsigned int TP_SAT>
class fir_interpolate_hb_asym<TT_DATA,
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
                              DUAL_IP_SINGLE,
                              USE_COEFF_RELOAD_FALSE,
                              2,
                              TP_UPSHIFT_CT,
                              USE_WINDOW_API,
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
                                                                 DUAL_IP_SINGLE,
                                                                 USE_COEFF_RELOAD_FALSE,
                                                                 2,
                                                                 TP_UPSHIFT_CT,
                                                                 USE_WINDOW_API,
                                                                 TP_SAT> {
   public:
    // Constructor
    fir_interpolate_hb_asym(const TT_COEFF (&taps)[getHbTaps(TP_FIR_LEN)])
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
                            DUAL_IP_SINGLE,
                            USE_COEFF_RELOAD_FALSE,
                            2,
                            TP_UPSHIFT_CT,
                            USE_WINDOW_API,
                            TP_SAT>(taps) {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fir_interpolate_hb_asym::filter); }
    // FIR
    void filter(
        input_circular_buffer<TT_DATA,
                              extents<inherited_extent>,
                              margin<fnFirMargin<TP_FIR_LEN / kInterpolateFactor, TT_DATA>()> >& __restrict inWindow,
        output_circular_buffer<TT_DATA>& __restrict outWindow,
        output_circular_buffer<TT_DATA>& __restrict outWindow2);
};

//-----------------------------------------------------------------------------------------------------
// Single kernel specialization. No cascade ports, Windowed. single input, with reload coefficients, single output
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_FIR_RANGE_LEN,
          unsigned int TP_UPSHIFT_CT,
          unsigned int TP_SAT>
class fir_interpolate_hb_asym<TT_DATA,
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
                              DUAL_IP_SINGLE,
                              USE_COEFF_RELOAD_TRUE,
                              1,
                              TP_UPSHIFT_CT,
                              USE_WINDOW_API,
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
                                                                 DUAL_IP_SINGLE,
                                                                 USE_COEFF_RELOAD_TRUE,
                                                                 1,
                                                                 TP_UPSHIFT_CT,
                                                                 USE_WINDOW_API,
                                                                 TP_SAT> {
   public:
    fir_interpolate_hb_asym()
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
                            DUAL_IP_SINGLE,
                            USE_COEFF_RELOAD_TRUE,
                            1,
                            TP_UPSHIFT_CT,
                            USE_WINDOW_API,
                            TP_SAT>() {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fir_interpolate_hb_asym::filter); }

    // FIR
    void filter(
        input_circular_buffer<TT_DATA,
                              extents<inherited_extent>,
                              margin<fnFirMargin<TP_FIR_LEN / kInterpolateFactor, TT_DATA>()> >& __restrict inWindow,
        output_circular_buffer<TT_DATA>& __restrict outWindow,
        const TT_COEFF (&inTaps)[getHbTaps(TP_FIR_LEN)]);
};

// Single kernel specialization. No cascade ports, Windowed. single input, with reload coefficients, dual output
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_FIR_RANGE_LEN,
          unsigned int TP_UPSHIFT_CT,
          unsigned int TP_SAT>
class fir_interpolate_hb_asym<TT_DATA,
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
                              DUAL_IP_SINGLE,
                              USE_COEFF_RELOAD_TRUE,
                              2,
                              TP_UPSHIFT_CT,
                              USE_WINDOW_API,
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
                                                                 DUAL_IP_SINGLE,
                                                                 USE_COEFF_RELOAD_TRUE,
                                                                 2,
                                                                 TP_UPSHIFT_CT,
                                                                 USE_WINDOW_API,
                                                                 TP_SAT> {
   public:
    fir_interpolate_hb_asym()
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
                            DUAL_IP_SINGLE,
                            USE_COEFF_RELOAD_TRUE,
                            2,
                            TP_UPSHIFT_CT,
                            USE_WINDOW_API,
                            TP_SAT>() {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fir_interpolate_hb_asym::filter); }

    // FIR
    void filter(
        input_circular_buffer<TT_DATA,
                              extents<inherited_extent>,
                              margin<fnFirMargin<TP_FIR_LEN / kInterpolateFactor, TT_DATA>()> >& __restrict inWindow,
        output_circular_buffer<TT_DATA>& __restrict outWindow,
        output_circular_buffer<TT_DATA>& __restrict outWindow2,
        const TT_COEFF (&inTaps)[getHbTaps(TP_FIR_LEN)]);
};

//-----------------------------------------------------------------------------------------------------
// Single kernel specialization. No cascade ports, Windowed. dual input, no reload, single output
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_FIR_RANGE_LEN,
          unsigned int TP_UPSHIFT_CT,
          unsigned int TP_SAT>
class fir_interpolate_hb_asym<TT_DATA,
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
                              DUAL_IP_DUAL,
                              USE_COEFF_RELOAD_FALSE,
                              1,
                              TP_UPSHIFT_CT,
                              USE_WINDOW_API,
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
                                                                 DUAL_IP_DUAL,
                                                                 USE_COEFF_RELOAD_FALSE,
                                                                 1,
                                                                 TP_UPSHIFT_CT,
                                                                 USE_WINDOW_API,
                                                                 TP_SAT>

{
   public:
    // Constructor
    fir_interpolate_hb_asym(const TT_COEFF (&taps)[getHbTaps(TP_FIR_LEN)])
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
                            DUAL_IP_DUAL,
                            USE_COEFF_RELOAD_FALSE,
                            1,
                            TP_UPSHIFT_CT,
                            USE_WINDOW_API,
                            TP_SAT>(taps) {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fir_interpolate_hb_asym::filter); }

    // FIR
    void filter(
        input_circular_buffer<TT_DATA,
                              extents<inherited_extent>,
                              margin<fnFirMargin<TP_FIR_LEN / kInterpolateFactor, TT_DATA>()> >& __restrict inWindow,
        input_circular_buffer<TT_DATA,
                              extents<inherited_extent>,
                              margin<fnFirMargin<TP_FIR_LEN / kInterpolateFactor, TT_DATA>()> >& __restrict inWindowRev,
        output_circular_buffer<TT_DATA>& __restrict outWindow);
};

// Single kernel specialization. No cascade ports, Windowed. dual input, no reload, dual output
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_FIR_RANGE_LEN,
          unsigned int TP_UPSHIFT_CT,
          unsigned int TP_SAT>
class fir_interpolate_hb_asym<TT_DATA,
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
                              DUAL_IP_DUAL,
                              USE_COEFF_RELOAD_FALSE,
                              2,
                              TP_UPSHIFT_CT,
                              USE_WINDOW_API,
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
                                                                 DUAL_IP_DUAL,
                                                                 USE_COEFF_RELOAD_FALSE,
                                                                 2,
                                                                 TP_UPSHIFT_CT,
                                                                 USE_WINDOW_API,
                                                                 TP_SAT>

{
   public:
    // Constructor
    fir_interpolate_hb_asym(const TT_COEFF (&taps)[getHbTaps(TP_FIR_LEN)])
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
                            DUAL_IP_DUAL,
                            USE_COEFF_RELOAD_FALSE,
                            2,
                            TP_UPSHIFT_CT,
                            USE_WINDOW_API,
                            TP_SAT>(taps) {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fir_interpolate_hb_asym::filter); }

    // FIR
    void filter(
        input_circular_buffer<TT_DATA,
                              extents<inherited_extent>,
                              margin<fnFirMargin<TP_FIR_LEN / kInterpolateFactor, TT_DATA>()> >& __restrict inWindow,
        input_circular_buffer<TT_DATA,
                              extents<inherited_extent>,
                              margin<fnFirMargin<TP_FIR_LEN / kInterpolateFactor, TT_DATA>()> >& __restrict inWindowRev,
        output_circular_buffer<TT_DATA>& __restrict outWindow,
        output_circular_buffer<TT_DATA>& __restrict outWindow2);
};

//-----------------------------------------------------------------------------------------------------
// Single kernel specialization. No cascade ports, Windowed. dual input, with reload, single output
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_FIR_RANGE_LEN,
          unsigned int TP_UPSHIFT_CT,
          unsigned int TP_SAT>
class fir_interpolate_hb_asym<TT_DATA,
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
                              DUAL_IP_DUAL,
                              USE_COEFF_RELOAD_TRUE,
                              1,
                              TP_UPSHIFT_CT,
                              USE_WINDOW_API,
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
                                                                 DUAL_IP_DUAL,
                                                                 USE_COEFF_RELOAD_TRUE,
                                                                 1,
                                                                 TP_UPSHIFT_CT,
                                                                 USE_WINDOW_API,
                                                                 TP_SAT>

{
   public:
    // Constructor
    fir_interpolate_hb_asym()
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
                            DUAL_IP_DUAL,
                            USE_COEFF_RELOAD_TRUE,
                            1,
                            TP_UPSHIFT_CT,
                            USE_WINDOW_API,
                            TP_SAT>() {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fir_interpolate_hb_asym::filter); }

    // FIR
    void filter(
        input_circular_buffer<TT_DATA,
                              extents<inherited_extent>,
                              margin<fnFirMargin<TP_FIR_LEN / kInterpolateFactor, TT_DATA>()> >& __restrict inWindow,
        input_circular_buffer<TT_DATA,
                              extents<inherited_extent>,
                              margin<fnFirMargin<TP_FIR_LEN / kInterpolateFactor, TT_DATA>()> >& __restrict inWindowRev,
        output_circular_buffer<TT_DATA>& __restrict outWindow,
        const TT_COEFF (&inTaps)[getHbTaps(TP_FIR_LEN)]);
};

// Single kernel specialization. No cascade ports, Windowed. dual input, with reload, dual output
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_FIR_RANGE_LEN,
          unsigned int TP_UPSHIFT_CT,
          unsigned int TP_SAT>
class fir_interpolate_hb_asym<TT_DATA,
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
                              DUAL_IP_DUAL,
                              USE_COEFF_RELOAD_TRUE,
                              2,
                              TP_UPSHIFT_CT,
                              USE_WINDOW_API,
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
                                                                 DUAL_IP_DUAL,
                                                                 USE_COEFF_RELOAD_TRUE,
                                                                 2,
                                                                 TP_UPSHIFT_CT,
                                                                 USE_WINDOW_API,
                                                                 TP_SAT>

{
   public:
    // Constructor
    fir_interpolate_hb_asym()
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
                            DUAL_IP_DUAL,
                            USE_COEFF_RELOAD_TRUE,
                            2,
                            TP_UPSHIFT_CT,
                            USE_WINDOW_API,
                            TP_SAT>() {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fir_interpolate_hb_asym::filter); }

    // FIR
    void filter(
        input_circular_buffer<TT_DATA,
                              extents<inherited_extent>,
                              margin<fnFirMargin<TP_FIR_LEN / kInterpolateFactor, TT_DATA>()> >& __restrict inWindow,
        input_circular_buffer<TT_DATA,
                              extents<inherited_extent>,
                              margin<fnFirMargin<TP_FIR_LEN / kInterpolateFactor, TT_DATA>()> >& __restrict inWindowRev,
        output_circular_buffer<TT_DATA>& __restrict outWindow,
        output_circular_buffer<TT_DATA>& __restrict outWindow2,
        const TT_COEFF (&inTaps)[getHbTaps(TP_FIR_LEN)]);
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
          unsigned int TP_UPSHIFT_CT,
          unsigned int TP_SAT>
class fir_interpolate_hb_asym<TT_DATA,
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
                              TP_UPSHIFT_CT,
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
                                                                 TP_UPSHIFT_CT,
                                                                 USE_WINDOW_API,
                                                                 TP_SAT>

{
   public:
    // Constructor
    fir_interpolate_hb_asym(const TT_COEFF (&taps)[getHbTaps(TP_FIR_LEN)])
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
                            TP_UPSHIFT_CT,
                            USE_WINDOW_API,
                            TP_SAT>(taps) {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fir_interpolate_hb_asym::filter); }
    // FIR
    void filter(input_async_buffer<TT_DATA, extents<inherited_extent> >& inWindow,
                input_stream_cacc48* inCascade,
                output_circular_buffer<TT_DATA>& __restrict outWindow);
};

// Partially specialized classes for cascaded interface (final kernel in cascade), Windowed. single input, no reload,
// dual output
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
          unsigned int TP_UPSHIFT_CT,
          unsigned int TP_SAT>
class fir_interpolate_hb_asym<TT_DATA,
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
                              2,
                              TP_UPSHIFT_CT,
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
                                                                 2,
                                                                 TP_UPSHIFT_CT,
                                                                 USE_WINDOW_API,
                                                                 TP_SAT>

{
   public:
    // Constructor
    fir_interpolate_hb_asym(const TT_COEFF (&taps)[getHbTaps(TP_FIR_LEN)])
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
                            2,
                            TP_UPSHIFT_CT,
                            USE_WINDOW_API,
                            TP_SAT>(taps) {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fir_interpolate_hb_asym::filter); }
    // FIR
    void filter(input_async_buffer<TT_DATA, extents<inherited_extent> >& inWindow,
                input_stream_cacc48* inCascade,
                output_circular_buffer<TT_DATA>& __restrict outWindow,
                output_circular_buffer<TT_DATA>& __restrict outWindow2);
};

// Partially specialized classes for cascaded interface (final kernel in cascade), Windowed. single/dual input, reload,
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
          unsigned int TP_UPSHIFT_CT,
          unsigned int TP_SAT>
class fir_interpolate_hb_asym<TT_DATA,
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
                              TP_UPSHIFT_CT,
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
                                                                 TP_UPSHIFT_CT,
                                                                 USE_WINDOW_API,
                                                                 TP_SAT>

{
   public:
    // Constructor
    fir_interpolate_hb_asym()
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
                            TP_UPSHIFT_CT,
                            USE_WINDOW_API,
                            TP_SAT>() {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fir_interpolate_hb_asym::filter); }
    // FIR
    void filter(input_async_buffer<TT_DATA, extents<inherited_extent> >& inWindow,
                input_stream_cacc48* inCascade,
                output_circular_buffer<TT_DATA>& __restrict outWindow);
};

// Partially specialized classes for cascaded interface (final kernel in cascade), Windowed. single/dual input, reload,
// dual output
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
          unsigned int TP_UPSHIFT_CT,
          unsigned int TP_SAT>
class fir_interpolate_hb_asym<TT_DATA,
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
                              2,
                              TP_UPSHIFT_CT,
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
                                                                 2,
                                                                 TP_UPSHIFT_CT,
                                                                 USE_WINDOW_API,
                                                                 TP_SAT>

{
   public:
    // Constructor
    fir_interpolate_hb_asym()
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
                            2,
                            TP_UPSHIFT_CT,
                            USE_WINDOW_API,
                            TP_SAT>() {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fir_interpolate_hb_asym::filter); }
    // FIR
    void filter(input_async_buffer<TT_DATA, extents<inherited_extent> >& inWindow,
                input_stream_cacc48* inCascade,
                output_circular_buffer<TT_DATA>& __restrict outWindow,
                output_circular_buffer<TT_DATA>& __restrict outWindow2);
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
          unsigned int TP_UPSHIFT_CT,
          unsigned int TP_SAT>
class fir_interpolate_hb_asym<TT_DATA,
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
                              DUAL_IP_SINGLE,
                              USE_COEFF_RELOAD_FALSE,
                              1,
                              TP_UPSHIFT_CT,
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
                                                                 DUAL_IP_SINGLE,
                                                                 USE_COEFF_RELOAD_FALSE,
                                                                 1,
                                                                 TP_UPSHIFT_CT,
                                                                 USE_WINDOW_API,
                                                                 TP_SAT>

{
   public:
    // Constructor
    fir_interpolate_hb_asym(const TT_COEFF (&taps)[getHbTaps(TP_FIR_LEN)])
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
                            DUAL_IP_SINGLE,
                            USE_COEFF_RELOAD_FALSE,
                            1,
                            TP_UPSHIFT_CT,
                            USE_WINDOW_API,
                            TP_SAT>(taps) {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fir_interpolate_hb_asym::filter); }
    // FIR
    void filter(input_circular_buffer<TT_DATA,
                                      extents<inherited_extent>,
                                      margin<fnFirMargin<TP_FIR_LEN / kInterpolateFactor, TT_DATA>()> >& inWindow,
                output_stream_cacc48* outCascade,
                output_async_buffer<TT_DATA>& broadcastWindow);
};

// Partially specialized classes for cascaded interface (First kernel in cascade), Windowed. dual input, no reload
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_FIR_RANGE_LEN,
          unsigned int TP_KERNEL_POSITION,
          unsigned int TP_CASC_LEN,
          unsigned int TP_UPSHIFT_CT,
          unsigned int TP_SAT>
class fir_interpolate_hb_asym<TT_DATA,
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
                              DUAL_IP_DUAL,
                              USE_COEFF_RELOAD_FALSE,
                              1,
                              TP_UPSHIFT_CT,
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
                                                                 DUAL_IP_DUAL,
                                                                 USE_COEFF_RELOAD_FALSE,
                                                                 1,
                                                                 TP_UPSHIFT_CT,
                                                                 USE_WINDOW_API,
                                                                 TP_SAT>

{
   public:
    // Constructor
    fir_interpolate_hb_asym(const TT_COEFF (&taps)[getHbTaps(TP_FIR_LEN)])
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
                            DUAL_IP_DUAL,
                            USE_COEFF_RELOAD_FALSE,
                            1,
                            TP_UPSHIFT_CT,
                            USE_WINDOW_API,
                            TP_SAT>(taps) {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fir_interpolate_hb_asym::filter); }

    // FIR
    void filter(
        input_circular_buffer<TT_DATA,
                              extents<inherited_extent>,
                              margin<fnFirMargin<TP_FIR_LEN / kInterpolateFactor, TT_DATA>()> >& inWindow,
        input_circular_buffer<TT_DATA,
                              extents<inherited_extent>,
                              margin<fnFirMargin<TP_FIR_LEN / kInterpolateFactor, TT_DATA>()> >& inWindowReverse,
        output_stream_cacc48* outCascade,
        output_async_buffer<TT_DATA>& broadcastWindow);
};

// Partially specialized classes for cascaded interface (First kernel in cascade), Windowed. single input, reload
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_FIR_RANGE_LEN,
          unsigned int TP_KERNEL_POSITION,
          unsigned int TP_CASC_LEN,
          unsigned int TP_UPSHIFT_CT,
          unsigned int TP_SAT>
class fir_interpolate_hb_asym<TT_DATA,
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
                              DUAL_IP_SINGLE,
                              USE_COEFF_RELOAD_TRUE,
                              1,
                              TP_UPSHIFT_CT,
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
                                                                 DUAL_IP_SINGLE,
                                                                 USE_COEFF_RELOAD_TRUE,
                                                                 1,
                                                                 TP_UPSHIFT_CT,
                                                                 USE_WINDOW_API,
                                                                 TP_SAT>

{
   public:
    // Constructor
    fir_interpolate_hb_asym()
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
                            DUAL_IP_SINGLE,
                            USE_COEFF_RELOAD_TRUE,
                            1,
                            TP_UPSHIFT_CT,
                            USE_WINDOW_API,
                            TP_SAT>() {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fir_interpolate_hb_asym::filter); }

    // FIR
    void filter(input_circular_buffer<TT_DATA,
                                      extents<inherited_extent>,
                                      margin<fnFirMargin<TP_FIR_LEN / kInterpolateFactor, TT_DATA>()> >& inWindow,
                output_stream_cacc48* outCascade,
                output_async_buffer<TT_DATA>& broadcastWindow,
                const TT_COEFF (&inTaps)[getHbTaps(TP_FIR_LEN)]);
};

// Partially specialized classes for cascaded interface (First kernel in cascade), Windowed. dual input, reload
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_FIR_RANGE_LEN,
          unsigned int TP_KERNEL_POSITION,
          unsigned int TP_CASC_LEN,
          unsigned int TP_UPSHIFT_CT,
          unsigned int TP_SAT>
class fir_interpolate_hb_asym<TT_DATA,
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
                              DUAL_IP_DUAL,
                              USE_COEFF_RELOAD_TRUE,
                              1,
                              TP_UPSHIFT_CT,
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
                                                                 DUAL_IP_DUAL,
                                                                 USE_COEFF_RELOAD_TRUE,
                                                                 1,
                                                                 TP_UPSHIFT_CT,
                                                                 USE_WINDOW_API,
                                                                 TP_SAT>

{
   public:
    // Constructor
    fir_interpolate_hb_asym()
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
                            DUAL_IP_DUAL,
                            USE_COEFF_RELOAD_TRUE,
                            1,
                            TP_UPSHIFT_CT,
                            USE_WINDOW_API,
                            TP_SAT>() {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fir_interpolate_hb_asym::filter); }

    // FIR
    void filter(
        input_circular_buffer<TT_DATA,
                              extents<inherited_extent>,
                              margin<fnFirMargin<TP_FIR_LEN / kInterpolateFactor, TT_DATA>()> >& inWindow,
        input_circular_buffer<TT_DATA,
                              extents<inherited_extent>,
                              margin<fnFirMargin<TP_FIR_LEN / kInterpolateFactor, TT_DATA>()> >& inWindowReverse,
        output_stream_cacc48* outCascade,
        output_async_buffer<TT_DATA>& broadcastWindow,
        const TT_COEFF (&inTaps)[getHbTaps(TP_FIR_LEN)]);
};

//-----------------------------------------------------------------------------------------------------
// Partially specialized classes for cascaded interface (middle kernels in cascade), Windowed. single/dual input, no
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
          unsigned int TP_UPSHIFT_CT,
          unsigned int TP_SAT>
class fir_interpolate_hb_asym<TT_DATA,
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
                              TP_UPSHIFT_CT,
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
                                                                 TP_UPSHIFT_CT,
                                                                 USE_WINDOW_API,
                                                                 TP_SAT>

{
   public:
    // Constructor
    fir_interpolate_hb_asym(const TT_COEFF (&taps)[getHbTaps(TP_FIR_LEN)])
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
                            TP_UPSHIFT_CT,
                            USE_WINDOW_API,
                            TP_SAT>(taps) {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fir_interpolate_hb_asym::filter); }
    // FIR
    void filter(input_async_buffer<TT_DATA>& inWindow,
                input_stream_cacc48* inCascade,
                output_stream_cacc48* outCascade,
                output_async_buffer<TT_DATA>& broadcastWindow);
};
// Partially specialized classes for cascaded interface (middle kernels in cascade), Windowed. single/dual input, reload
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
          unsigned int TP_UPSHIFT_CT,
          unsigned int TP_SAT>
class fir_interpolate_hb_asym<TT_DATA,
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
                              TP_UPSHIFT_CT,
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
                                                                 TP_UPSHIFT_CT,
                                                                 USE_WINDOW_API,
                                                                 TP_SAT>

{
   private:
   public:
    // Constructor
    fir_interpolate_hb_asym()
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
                            TP_UPSHIFT_CT,
                            USE_WINDOW_API,
                            TP_SAT>() {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fir_interpolate_hb_asym::filter); }

    // FIR
    void filter(input_async_buffer<TT_DATA>& inWindow,
                input_stream_cacc48* inCascade,
                output_stream_cacc48* outCascade,
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
          unsigned int TP_UPSHIFT_CT,
          unsigned int TP_SAT>
class fir_interpolate_hb_asym<TT_DATA,
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
                              TP_UPSHIFT_CT,
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
                                                                 TP_UPSHIFT_CT,
                                                                 USE_STREAM_API,
                                                                 TP_SAT> {
   public:
    // Constructor
    fir_interpolate_hb_asym(const TT_COEFF (&taps)[getHbTaps(TP_FIR_LEN)])
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
                            TP_UPSHIFT_CT,
                            USE_STREAM_API,
                            TP_SAT>(taps) {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fir_interpolate_hb_asym::filter); }

    // FIR
    void filter(input_stream<TT_DATA>* inStream, output_stream<TT_DATA>* outStream);
};

//-----------------------------------------------------------------------------------------------------
// Single kernel specialization. No cascade ports. Streaming. Static coefficients. Dual Output
//-----------------------------------------------------------------------------------------------------
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_FIR_RANGE_LEN,
          unsigned int TP_DUAL_IP,
          unsigned int TP_UPSHIFT_CT,
          unsigned int TP_SAT>
class fir_interpolate_hb_asym<TT_DATA,
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
                              2,
                              TP_UPSHIFT_CT,
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
                                                                 2,
                                                                 TP_UPSHIFT_CT,
                                                                 USE_STREAM_API,
                                                                 TP_SAT> {
   public:
    // Constructor
    fir_interpolate_hb_asym(const TT_COEFF (&taps)[getHbTaps(TP_FIR_LEN)])
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
                            2,
                            TP_UPSHIFT_CT,
                            USE_STREAM_API,
                            TP_SAT>(taps) {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fir_interpolate_hb_asym::filter); }

    // FIR
    void filter(input_stream<TT_DATA>* inStream, output_stream<TT_DATA>* outStream, output_stream<TT_DATA>* outStream2);
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
          unsigned int TP_UPSHIFT_CT,
          unsigned int TP_SAT>
class fir_interpolate_hb_asym<TT_DATA,
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
                              TP_UPSHIFT_CT,
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
                                                                 TP_UPSHIFT_CT,
                                                                 USE_STREAM_API,
                                                                 TP_SAT> {
   public:
    // Constructor
    fir_interpolate_hb_asym()
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
                            TP_UPSHIFT_CT,
                            USE_STREAM_API,
                            TP_SAT>() {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fir_interpolate_hb_asym::filter); }

    // FIR
    void filter(input_stream<TT_DATA>* inStream,
                output_stream<TT_DATA>* outStream,
                const TT_COEFF (&inTaps)[getHbTaps(TP_FIR_LEN)]);
};

//-----------------------------------------------------------------------------------------------------
// Single kernel specialization. No cascade ports. Streaming. Reloadable coefficients. Dual Output
//-----------------------------------------------------------------------------------------------------
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_FIR_RANGE_LEN,
          unsigned int TP_DUAL_IP,
          unsigned int TP_UPSHIFT_CT,
          unsigned int TP_SAT>
class fir_interpolate_hb_asym<TT_DATA,
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
                              2,
                              TP_UPSHIFT_CT,
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
                                                                 2,
                                                                 TP_UPSHIFT_CT,
                                                                 USE_STREAM_API,
                                                                 TP_SAT> {
   public:
    // Constructor
    fir_interpolate_hb_asym()
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
                            2,
                            TP_UPSHIFT_CT,
                            USE_STREAM_API,
                            TP_SAT>() {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fir_interpolate_hb_asym::filter); }

    // FIR
    void filter(input_stream<TT_DATA>* inStream,
                output_stream<TT_DATA>* outStream,
                output_stream<TT_DATA>* outStream2,
                const TT_COEFF (&inTaps)[getHbTaps(TP_FIR_LEN)]);
};

//-----------------------------------------------------------------------------------------------------
// Cascaded Kernels - Final Kernel. Static coefficients. Streaming. Single input. Single Output
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
          unsigned int TP_UPSHIFT_CT,
          unsigned int TP_SAT>
class fir_interpolate_hb_asym<TT_DATA,
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
                              TP_UPSHIFT_CT,
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
                                                                 TP_UPSHIFT_CT,
                                                                 USE_STREAM_API,
                                                                 TP_SAT> {
   public:
    // Constructor
    fir_interpolate_hb_asym(const TT_COEFF (&taps)[getHbTaps(TP_FIR_LEN)])
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
                            TP_UPSHIFT_CT,
                            USE_STREAM_API,
                            TP_SAT>(taps) {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fir_interpolate_hb_asym::filter); }

    // FIR
    void filter(input_stream<TT_DATA>* inStream, input_stream_cacc48* inCascade, output_stream<TT_DATA>* outStream);
};

//-----------------------------------------------------------------------------------------------------
// Cascaded Kernels - Final Kernel. Static coefficients. Streaming. Single Input. Dual Output
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
          unsigned int TP_UPSHIFT_CT,
          unsigned int TP_SAT>
class fir_interpolate_hb_asym<TT_DATA,
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
                              2,
                              TP_UPSHIFT_CT,
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
                                                                 2,
                                                                 TP_UPSHIFT_CT,
                                                                 USE_STREAM_API,
                                                                 TP_SAT> {
   public:
    // Constructor
    fir_interpolate_hb_asym(const TT_COEFF (&taps)[getHbTaps(TP_FIR_LEN)])
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
                            2,
                            TP_UPSHIFT_CT,
                            USE_STREAM_API,
                            TP_SAT>(taps) {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fir_interpolate_hb_asym::filter); }

    // FIR
    void filter(input_stream<TT_DATA>* inStream,
                input_stream_cacc48* inCascade,
                output_stream<TT_DATA>* outStream,
                output_stream<TT_DATA>* outStream2);
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
          unsigned int TP_UPSHIFT_CT,
          unsigned int TP_SAT>
class fir_interpolate_hb_asym<TT_DATA,
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
                              USE_COEFF_RELOAD_FALSE,
                              1,
                              TP_UPSHIFT_CT,
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
                                                                 USE_COEFF_RELOAD_FALSE,
                                                                 1,
                                                                 TP_UPSHIFT_CT,
                                                                 USE_STREAM_API,
                                                                 TP_SAT> {
   public:
    // Constructor
    fir_interpolate_hb_asym(const TT_COEFF (&taps)[getHbTaps(TP_FIR_LEN)])
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
                            USE_COEFF_RELOAD_FALSE,
                            1,
                            TP_UPSHIFT_CT,
                            USE_STREAM_API,
                            TP_SAT>(taps) {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fir_interpolate_hb_asym::filter); }

    // FIR
    void filter(input_stream<TT_DATA>* inStream, output_stream_cacc48* outCascade);
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
          unsigned int TP_UPSHIFT_CT,
          unsigned int TP_SAT>
class fir_interpolate_hb_asym<TT_DATA,
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
                              USE_COEFF_RELOAD_FALSE,
                              1,
                              TP_UPSHIFT_CT,
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
                                                                 USE_COEFF_RELOAD_FALSE,
                                                                 1,
                                                                 TP_UPSHIFT_CT,
                                                                 USE_STREAM_API,
                                                                 TP_SAT> {
   public:
    // Constructor
    fir_interpolate_hb_asym(const TT_COEFF (&taps)[getHbTaps(TP_FIR_LEN)])
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
                            USE_COEFF_RELOAD_FALSE,
                            1,
                            TP_UPSHIFT_CT,
                            USE_STREAM_API,
                            TP_SAT>(taps) {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fir_interpolate_hb_asym::filter); }

    // FIR
    void filter(input_stream<TT_DATA>* inStream, input_stream_cacc48* inCascade, output_stream_cacc48* outCascade);
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
          unsigned int TP_UPSHIFT_CT,
          unsigned int TP_SAT>
class fir_interpolate_hb_asym<TT_DATA,
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
                              TP_UPSHIFT_CT,
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
                                                                 TP_UPSHIFT_CT,
                                                                 USE_STREAM_API,
                                                                 TP_SAT> {
   public:
    // Constructor
    fir_interpolate_hb_asym()
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
                            TP_UPSHIFT_CT,
                            USE_STREAM_API,
                            TP_SAT>() {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fir_interpolate_hb_asym::filter); }

    // FIR
    void filter(input_stream<TT_DATA>* inStream, input_stream_cacc48* inCascade, output_stream<TT_DATA>* outStream);
};

//-----------------------------------------------------------------------------------------------------
// Cascaded Kernels - Final Kernel. Streaming. Reloadable coefficients. Single input. Dual Output
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
          unsigned int TP_UPSHIFT_CT,
          unsigned int TP_SAT>
class fir_interpolate_hb_asym<TT_DATA,
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
                              2,
                              TP_UPSHIFT_CT,
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
                                                                 2,
                                                                 TP_UPSHIFT_CT,
                                                                 USE_STREAM_API,
                                                                 TP_SAT> {
   public:
    // Constructor
    fir_interpolate_hb_asym()
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
                            2,
                            TP_UPSHIFT_CT,
                            USE_STREAM_API,
                            TP_SAT>() {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fir_interpolate_hb_asym::filter); }

    // FIR
    void filter(input_stream<TT_DATA>* inStream,
                input_stream_cacc48* inCascade,
                output_stream<TT_DATA>* outStream,
                output_stream<TT_DATA>* outStream2);
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
          unsigned int TP_UPSHIFT_CT,
          unsigned int TP_SAT>
class fir_interpolate_hb_asym<TT_DATA,
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
                              TP_UPSHIFT_CT,
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
                                                                 TP_UPSHIFT_CT,
                                                                 USE_STREAM_API,
                                                                 TP_SAT> {
   public:
    // Constructor
    fir_interpolate_hb_asym()
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
                            TP_UPSHIFT_CT,
                            USE_STREAM_API,
                            TP_SAT>() {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fir_interpolate_hb_asym::filter); }

    // FIR
    void filter(input_stream<TT_DATA>* inStream,
                output_stream_cacc48* outCascade,
                const TT_COEFF (&inTaps)[getHbTaps(TP_FIR_LEN)]);
};

//-----------------------------------------------------------------------------------------------------
// Cascaded Kernels - Middle Kernel. Streaming. Reloadable coefficients. Single Input
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
          unsigned int TP_UPSHIFT_CT,
          unsigned int TP_SAT>
class fir_interpolate_hb_asym<TT_DATA,
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
                              0,
                              USE_COEFF_RELOAD_TRUE,
                              1,
                              TP_UPSHIFT_CT,
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
                                                                 0,
                                                                 USE_COEFF_RELOAD_TRUE,
                                                                 1,
                                                                 TP_UPSHIFT_CT,
                                                                 USE_STREAM_API,
                                                                 TP_SAT> {
   public:
    // Constructor
    fir_interpolate_hb_asym()
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
                            0,
                            USE_COEFF_RELOAD_TRUE,
                            1,
                            TP_UPSHIFT_CT,
                            USE_STREAM_API,
                            TP_SAT>() {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fir_interpolate_hb_asym::filter); }

    // FIR
    void filter(input_stream<TT_DATA>* inStream, input_stream_cacc48* inCascade, output_stream_cacc48* outCascade);
};

template <typename fp = fir_params_defaults>
class fir_interpolate_hb_asym_tl : public fir_interpolate_hb_asym<typename fp::BTT_DATA,
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
                                                                  fp::BTP_UPSHIFT_CT,
                                                                  fp::BTP_API,
                                                                  fp::BTP_SAT> {
   public:
    // Get kernel's FIR range Length
    template <int pos>
    static constexpr unsigned int getKernelFirRangeLen() {
        constexpr unsigned int hasCT = (fp::BTP_KERNEL_POSITION == fp::BTP_CASC_LEN / 2) ? 1 : 0;
#if __HAS_SYM_PREADD__ == 1
        constexpr unsigned int firRangeLen =
            pos + 1 == fp::BTP_CASC_LEN
                ? (fp::BTP_POLY_SSR == 1
                       ? fnFirRangeRemSym<fp::BTP_FIR_LEN, fp::BTP_CASC_LEN, pos>()
                       : sr_asym::fnFirRangeRemAsym<fp::BTP_FIR_LEN, fp::BTP_CASC_LEN, pos, typename fp::BTT_DATA,
                                                    typename fp::BTT_COEFF, fp::BTP_API>())
                : (fp::BTP_POLY_SSR == 1
                       ? fnFirRangeSym<fp::BTP_FIR_LEN, fp::BTP_CASC_LEN, pos>()
                       : sr_asym::fnFirRangeAsym<fp::BTP_FIR_LEN, fp::BTP_CASC_LEN, pos, typename fp::BTT_DATA,
                                                 typename fp::BTT_COEFF, fp::BTP_API>());
        return firRangeLen;
#else
        constexpr unsigned int firRangeLen = (pos + 1 == fp::BTP_CASC_LEN)
                                                 ? fnFirRangeRem<fp::BTP_FIR_LEN, fp::BTP_CASC_LEN, pos, 2>()
                                                 : fnFirRange<fp::BTP_FIR_LEN, fp::BTP_CASC_LEN, pos, 2>();

        return firRangeLen;
#endif
    };

    // Get kernel's FIR range Length
    static constexpr unsigned int getFirRangeLen() { return fp::BTP_FIR_RANGE_LEN; };

    // Get kernel's FIR Total Tap Length
    static constexpr unsigned int getTapLen() { return getHbTaps(fp::BTP_FIR_LEN); }
    //(fp::BTP_FIR_LEN+1)/4 + 1;};

    // Get kernel's FIR Total Tap Length
    static constexpr unsigned int getSSRMargin() {
        constexpr unsigned int margin = fnFirMargin<fp::BTP_FIR_LEN / fp::BTP_SSR, typename fp::BTT_DATA>();
        return margin;
    };

    static constexpr unsigned int getDF() { return 1; };
    static constexpr unsigned int getIF() { return 1; };

    // Get FIR variant
    static constexpr eFIRVariant getFirType() { return eFIRVariant::kIntHB; };
    using parent_class = fir_interpolate_hb_asym<typename fp::BTT_DATA,
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
                                                 fp::BTP_UPSHIFT_CT,
                                                 fp::BTP_API,
                                                 fp::BTP_SAT>;
};
}
}
}
}
}

#endif // _DSPLIB_fir_interpolate_hb_asym_HPP_
