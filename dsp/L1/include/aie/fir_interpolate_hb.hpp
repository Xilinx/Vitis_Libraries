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
#ifndef _DSPLIB_FIR_INTERPOLATE_HB_HPP_
#define _DSPLIB_FIR_INTERPOLATE_HB_HPP_

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

namespace xf {
namespace dsp {
namespace aie {
namespace fir {
namespace interpolate_hb {
/* Halfband Interpolating FIR class definition

Note on Coefficient reversal. The AIE processor intrinsics naturally sum data and coefficients in the same order,
but the conventional definition of a FIR has data and coefficient indices in opposite order. For some other
variants of FIR filter the order of coefficients is therefore reversed during construction to yield conventional
FIR behaviour, but due to the fact that a halfband filter is symmetrical, this is not necessary here.

Coding conventions
  TT_      template type suffix
  TP_      template parameter suffix
*/
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
class kernelFilterClass {
   private:
    // Parameter value defensive and legality checks
    static_assert(TP_FIR_RANGE_LEN >= FIR_LEN_MIN,
                  "ERROR: Illegal combination of design FIR length and cascade length, resulting in kernel FIR length "
                  "below minimum required value. ");
    static_assert(TP_SHIFT >= SHIFT_MIN && TP_SHIFT <= SHIFT_MAX, "ERROR: SHIFT is out of the supported range.");
    static_assert(TP_RND >= ROUND_MIN && TP_RND <= ROUND_MAX, "ERROR: TP_RND is out of the supported range.");
    static_assert(TP_SAT >= SAT_MODE_MIN && TP_SAT <= SAT_MODE_MAX, "ERROR: TP_SAT is out of supported range");
    static_assert(TP_SAT != 2, "ERROR: TP_SAT is invalid. Valid values of TP_SAT are 0, 1, and 3");
    static_assert(((TP_FIR_LEN + 1) % 4) == 0, "ERROR: TP_FIR_LEN must be 4N-1 where N is a positive integer.");
    static_assert(fnEnumType<TT_DATA>() != enumUnknownType, "ERROR: TT_DATA is not a supported type.");
    static_assert(fnEnumType<TT_COEFF>() != enumUnknownType, "ERROR: TT_COEFF is not a supported type.");
    static_assert(fnTypeCheckDataCoeffSize<TT_DATA, TT_COEFF>() != 0,
                  "ERROR: TT_DATA type less precise than TT_COEFF is not supported.");
    static_assert(fnTypeCheckDataCoeffCmplx<TT_DATA, TT_COEFF>() != 0,
                  "ERROR: real TT_DATA with complex TT_COEFF is not supported.");
    static_assert(fnTypeCheckDataCoeffFltInt<TT_DATA, TT_COEFF>() != 0,
                  "ERROR: a mix of float and integer types of TT_DATA and TT_COEFF is not supported.");
    static_assert(TP_NUM_OUTPUTS > 0 && TP_NUM_OUTPUTS <= 2, "ERROR: only single or dual outputs are supported.");
    static_assert(TP_UPSHIFT_CT == 0 || fnUpshiftCTSupport<TT_DATA, TT_COEFF>() == SUPPORTED,
                  "ERROR: Unsupported data/coeff type combination. Upshift CT is only available for 16-bit integer "
                  "combinations.");
    static_assert(!(std::is_same<TT_DATA, cfloat>::value || std::is_same<TT_DATA, float>::value) || (TP_SHIFT == 0),
                  "ERROR: TP_SHIFT cannot be performed for TT_DATA=cfloat, so must be set to 0");
    // There are additional defensive checks after architectural constants have been calculated.

    static constexpr unsigned int m_kWinAccessByteSize =
        16; // 16 Bytes. The memory data path is min 128-bits wide for vector operations
    static constexpr unsigned int m_kColumns = fnNumSymColsIntHb<TT_DATA, TT_COEFF>();
    static constexpr unsigned int m_kZbuffSize = 32; // bytes
    static constexpr unsigned int m_kCoeffRegVsize = m_kZbuffSize / sizeof(TT_COEFF);
    static constexpr unsigned int m_kLanes =
        fnNumSymLanesIntHb<TT_DATA,
                           TT_COEFF,
                           TP_UPSHIFT_CT>(); // kMaxMacs/(sizeof(TT_DATA)*sizeof(TT_COEFF)*m_kColumns); //number of
                                             // operations in parallel of this type combinations that the vector
                                             // processor can do.
    static constexpr unsigned int m_kLsize = TP_INPUT_WINDOW_VSIZE / m_kLanes; // loops required to consume input
    static constexpr unsigned int m_kNumOps =
        CEIL((TP_FIR_RANGE_LEN + 1) / kInterpolateFactor / kSymmetryFactor, m_kColumns) / m_kColumns;

    // Data offsets (margin padding, cascade, etc.)
    static constexpr unsigned int m_kFirMarginLen = (TP_FIR_LEN + 1) / kInterpolateFactor;
    static constexpr unsigned int m_kFirRangeOffset =
        fnFirRangeOffsetSym<TP_FIR_LEN, TP_CASC_LEN, TP_KERNEL_POSITION>() /
        kInterpolateFactor; // FIR Cascade Offset for this kernel position
    static constexpr unsigned int m_kFirMarginOffset =
        fnFirMargin<m_kFirMarginLen, TT_DATA>() - m_kFirMarginLen + 1; // FIR Margin Offset.
    static constexpr unsigned int m_kFirInitOffset = m_kFirRangeOffset + m_kFirMarginOffset;
    static constexpr unsigned int m_kDataBuffXOffset =
        m_kFirInitOffset % (m_kWinAccessByteSize / sizeof(TT_DATA)); // Remainder of m_kFirInitOffset divided by 128bit
    // Reverse direction on data
    static constexpr unsigned int m_kCentreTapDataCol =
        m_kDataBuffXOffset +
        (TP_FIR_RANGE_LEN / kInterpolateFactor - (TP_FIR_RANGE_LEN / kInterpolateFactor) / kSymmetryFactor);
    // Forward direction on coeff
    static constexpr unsigned int m_kCentreTapCoeffCol = TP_FIR_RANGE_LEN / kInterpolateFactor / kSymmetryFactor;

    static constexpr unsigned int m_k1buffSupported =
        (TP_UPSHIFT_CT == 1 && TP_CASC_LEN != 1) ? NOT_SUPPORTED : SUPPORTED; // Upshift CT only supported for cascade
                                                                              // length 1. Longer cascades introduce
                                                                              // enough overhead to make the arch
                                                                              // inefficient.
    static constexpr unsigned int m_kSamplesIn128ByteBuff = kBuffSize128Byte / sizeof(TT_DATA); // 1024b reg is 128bytes
    static constexpr unsigned int m_kCoeffsNeeded = (TP_FIR_RANGE_LEN + 1) / 4;                 // need that many coeffs
    static constexpr unsigned int m_kInitialDataNeed1buff =
        (m_kDataBuffXOffset + (TP_FIR_LEN + 1) / kInterpolateFactor) + m_kLanes + m_kColumns -
        1; // final plus one is index to width conversion
    static constexpr eArchType m_kArch =
        (m_k1buffSupported == NOT_SUPPORTED || m_kInitialDataNeed1buff >= m_kSamplesIn128ByteBuff ||
         m_kCoeffsNeeded > m_kCoeffRegVsize)
            ? kArch2Buff
            : kArch1Buff;
    static constexpr eArchType m_kArchZigZag =
        (fnSupportZigZag<TT_DATA, TT_COEFF, TP_UPSHIFT_CT>() == 1 && TP_CASC_LEN == 1)
            ? kArch2BuffZigZag
            : kArch2Buff; // ZigZag architecture supports UCT mode with cint16/int16 data/coeff type combination
    static constexpr unsigned int m_kDataLoadsInReg =
        fnDataLoadsInReg<m_kArch, TT_DATA, TT_COEFF>(); // 4 for 2 buff arch, 8 for 1 buff arch, when 128-bit loads are
                                                        // used, else 4.
    static constexpr unsigned int m_kDataLoadVsize =
        m_kArch == kArch1Buff ? (32 / sizeof(TT_DATA)) : (16 / sizeof(TT_DATA));
    static constexpr unsigned int m_kXDataLoadInitOffset =
        TRUNC((m_kFirInitOffset),
              (m_kWinAccessByteSize / sizeof(TT_DATA))); // Xbuff window offset, aligned to load vector size.
    static constexpr unsigned int m_kSamplesInDataBuff = m_kDataLoadVsize * m_kDataLoadsInReg;
    static constexpr unsigned int m_kVOutSize =
        m_kDataLoadVsize * kInterpolateFactor; // This differs from kLanes for cint32/cint32
    static constexpr unsigned int m_kXbuffSize =
        m_kArch == kArch1Buff ? 1024 / 8 : 512 / 8; // kXbuffSize in Bytes (1024bit) - const for all data/coeff types
    static constexpr unsigned int m_kDataRegVsize =
        m_kXbuffSize / sizeof(TT_DATA); // sbuff samples, for small architecture
    static constexpr unsigned int m_kyStart =
        m_kFirMarginOffset + (TP_FIR_LEN) / 2 - m_kFirRangeOffset; // starting sample of the reverse direction buffer.
                                                                   // Kernel 0, always aligned to 32Byte boundary (due
                                                                   // to padded margin)
    static constexpr unsigned int m_kyStart2buff =
        (m_kyStart) % m_kDataLoadVsize; // ystart offset from 128-bit (m_kDataLoadVsize) aligned buffer data.
    static constexpr unsigned int m_kInitialLoads1buff =
        CEIL(m_kInitialDataNeed1buff, m_kDataLoadVsize) / m_kDataLoadVsize;
    static constexpr unsigned int m_kInitialLoadsX =
        CEIL(m_kDataBuffXOffset + m_kLanes + m_kColumns - 1, m_kDataLoadVsize) / m_kDataLoadVsize;
    static constexpr unsigned int m_kInitialLoadsY =
        CEIL(m_kyStart2buff + m_kLanes + m_kColumns - 1, m_kDataLoadVsize) /
        m_kDataLoadVsize; // number of Ybuffef initial loads
    static constexpr unsigned int m_kYInitOffsetMargin = TRUNC((m_kFirMarginOffset), m_kDataLoadVsize);
    static constexpr unsigned int m_kYInitOffsetFIR =
        ((TP_FIR_LEN / 2 / m_kDataLoadVsize) + m_kInitialLoadsY - 1 + (m_kLanes > m_kDataLoadVsize ? 1 : 0)) *
        m_kDataLoadVsize; // Offset required to start loading initial data
    static constexpr unsigned int m_kYInitOffsetCascade = CEIL((m_kFirRangeOffset), m_kDataLoadVsize); //
    static constexpr unsigned int m_kYDataLoadInitOffset =
        TRUNC(m_kyStart + m_kLanes - 1, m_kDataLoadVsize); // Offset required to start loading initial data
    static constexpr unsigned int m_kySpliceStart =
        TRUNC((m_kYDataLoadInitOffset), m_kDataLoadVsize) / m_kDataLoadVsize; //
    static constexpr unsigned int m_kRepeatFactor = (m_kArchZigZag == kArch2BuffZigZag && m_kArch == kArch2Buff)
                                                        ? m_kDataLoadsInReg / 2
                                                        : 1; // enough to repeat m_kDataLoadsInReg/2 - as the buff gets
                                                             // one load on zig and one on zag, i.e. =2 for 1024bit
                                                             // buffers, =1 for 512 bit buffers.

    // Coefficient Load Size - number of samples in 256-bits
    static constexpr unsigned int m_kCoeffLoadSize = 256 / 8 / sizeof(TT_COEFF);

    // Lower polyphase taps internal storage. Initialised to zeros.
    alignas(__ALIGN_BYTE_SIZE__) TT_COEFF
        m_internalTaps[CEIL((TP_FIR_LEN + 1) / 4 + 1, m_kCoeffLoadSize)]; // Filter taps/coefficients
    alignas(__ALIGN_BYTE_SIZE__) TT_COEFF m_phaseTwoTap[kMaxColumns] = {
        nullElem<TT_COEFF>()}; // note, the array is initializeed, causing extra instructions during initialiation.
    int16 m_ctShift;           // Upshift Center tap

    alignas(__ALIGN_BYTE_SIZE__) TT_COEFF
        m_oldInTaps[CEIL((TP_FIR_LEN + 1) / 4 + 1,
                         m_kCoeffLoadSize)]; // Previous user input coefficients with zero padding
    bool m_coeffnEq;                         // Are coefficients sets equal?

    void filterSelectArch(T_inputIF<TP_CASC_IN, TT_DATA, TP_DUAL_IP> inInterface,
                          T_outputIF<TP_CASC_OUT, TT_DATA> outInterface);
    void filter1buff(T_inputIF<TP_CASC_IN, TT_DATA, TP_DUAL_IP> inInterface,
                     T_outputIF<TP_CASC_OUT, TT_DATA> outInterface);
    void filter2buff(T_inputIF<TP_CASC_IN, TT_DATA, TP_DUAL_IP> inInterface,
                     T_outputIF<TP_CASC_OUT, TT_DATA> outInterface);
    void filter2buffZigZag(T_inputIF<TP_CASC_IN, TT_DATA, TP_DUAL_IP> inInterface,
                           T_outputIF<TP_CASC_OUT, TT_DATA> outInterface); // Not yet used.

    // Additional defensive checks
    static_assert(TP_INPUT_WINDOW_VSIZE % m_kLanes == 0,
                  "ERROR: TP_INPUT_WINDOW_VSIZE must be an integer multiple of the number of lanes for this data type");

   public:
    // Access function for AIE Synthesizer
    static unsigned int get_m_kArch() {
        return ((m_kArchZigZag == kArch2BuffZigZag) && (m_kArch == kArch2Buff)) ? kArch2BuffZigZag : m_kArch;
    };

    // Constructors
    kernelFilterClass() : m_oldInTaps{}, m_internalTaps{} {}

    kernelFilterClass(const TT_COEFF (&taps)[(TP_FIR_LEN + 1) / 4 + 1]) : m_internalTaps{} { firReload(taps); };

    void firReload(const TT_COEFF* taps) {
        TT_COEFF* tapsPtr = (TT_COEFF*)taps;
        firReload(tapsPtr);
    }

    void firReload(TT_COEFF* taps) {
        // The below may be accessed by a restrict pointer (to allow pipelined vector read).
        // In RTP cases the creation of the array is close to reading that array.
        // To ensure sufficient gap, remove restrict qualifier (done) or force addidion of gap with e.g. noinline.

        // Coefficients are pre-arranged, so that
        for (int i = 0; i < CEIL(((TP_FIR_RANGE_LEN + 1) / 4), kMaxColumns); i++) {
            if (i < ((TP_FIR_RANGE_LEN + 1) / 4)) {
                m_internalTaps[i] = taps[fnFirRangeOffsetSym<TP_FIR_LEN, TP_CASC_LEN, TP_KERNEL_POSITION>() / 2 + i];
            } else {
                m_internalTaps[i] = nullElem<TT_COEFF>();
            }
        }
        if (TP_UPSHIFT_CT == 0) {
            // The centre tap requires only one coefficient, but this is a vector processor with
            // multiple columns in the mul intrinsic, so the other columns must be zero'd
            m_phaseTwoTap[0] = taps[(TP_FIR_LEN + 1) / 4];
        } else {
            // Upshift Center tap.
            // Only supported for 16-bit integers. Accepted range: 0 - 16. Extracted from Real part when Complex numbers
            // used, hence the cast.
            m_ctShift = getUpshiftCt(taps[(TP_FIR_LEN + 1) / 4]);
        }
    }

    // FIR
    void filterKernel(T_inputIF<TP_CASC_IN, TT_DATA, TP_DUAL_IP> inInterface,
                      T_outputIF<TP_CASC_OUT, TT_DATA> outInterface);

    // with taps for reload
    void filterKernel(T_inputIF<TP_CASC_IN, TT_DATA, TP_DUAL_IP> inInterface,
                      T_outputIF<TP_CASC_OUT, TT_DATA> outInterface,
                      const TT_COEFF (&inTaps)[(TP_FIR_LEN + 1) / 4 + 1]);

    void filterKernelRtp(T_inputIF<TP_CASC_IN, TT_DATA, TP_DUAL_IP> inInterface,
                         T_outputIF<TP_CASC_OUT, TT_DATA> outInterface);
};

//-----------------------------------------------------------------------------------------------------
// Cascade layer class and specializations

//-----------------------------------------------------------------------------------------------------
// This is the main declaration of the fir_interpolate_hb class, and is also used for the
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
class fir_interpolate_hb : public kernelFilterClass<TT_DATA,
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
                                                    USE_COEFF_RELOAD_FALSE,
                                                    TP_NUM_OUTPUTS,
                                                    TP_UPSHIFT_CT,
                                                    USE_WINDOW_API,
                                                    TP_SAT>

{
   public:
    // Constructor
    fir_interpolate_hb(const TT_COEFF (&taps)[(TP_FIR_LEN + 1) / 4 + 1])
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
                            USE_COEFF_RELOAD_FALSE,
                            TP_NUM_OUTPUTS,
                            TP_UPSHIFT_CT,
                            USE_WINDOW_API,
                            TP_SAT>(taps) {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fir_interpolate_hb::filter); }
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
          unsigned int TP_UPSHIFT_CT,
          unsigned int TP_SAT>
class fir_interpolate_hb<TT_DATA,
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
                                                            TP_FIR_LEN,
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
    fir_interpolate_hb(const TT_COEFF (&taps)[(TP_FIR_LEN + 1) / 4 + 1])
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
                            USE_COEFF_RELOAD_FALSE,
                            2,
                            TP_UPSHIFT_CT,
                            USE_WINDOW_API,
                            TP_SAT>(taps) {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fir_interpolate_hb::filter); }
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
          unsigned int TP_UPSHIFT_CT,
          unsigned int TP_SAT>
class fir_interpolate_hb<TT_DATA,
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
                                                            TP_FIR_LEN,
                                                            0,
                                                            1,
                                                            DUAL_IP_SINGLE,
                                                            USE_COEFF_RELOAD_TRUE,
                                                            1,
                                                            TP_UPSHIFT_CT,
                                                            USE_WINDOW_API,
                                                            TP_SAT> {
   public:
    fir_interpolate_hb()
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
                            TP_UPSHIFT_CT,
                            USE_WINDOW_API,
                            TP_SAT>() {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fir_interpolate_hb::filter); }

    // FIR
    void filter(
        input_circular_buffer<TT_DATA,
                              extents<inherited_extent>,
                              margin<fnFirMargin<TP_FIR_LEN / kInterpolateFactor, TT_DATA>()> >& __restrict inWindow,
        output_circular_buffer<TT_DATA>& __restrict outWindow,
        const TT_COEFF (&inTaps)[(TP_FIR_LEN + 1) / 4 + 1]);
};

// Single kernel specialization. No cascade ports, Windowed. single input, with reload coefficients, dual output
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_UPSHIFT_CT,
          unsigned int TP_SAT>
class fir_interpolate_hb<TT_DATA,
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
                                                            TP_FIR_LEN,
                                                            0,
                                                            1,
                                                            DUAL_IP_SINGLE,
                                                            USE_COEFF_RELOAD_TRUE,
                                                            2,
                                                            TP_UPSHIFT_CT,
                                                            USE_WINDOW_API,
                                                            TP_SAT> {
   public:
    fir_interpolate_hb()
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
                            2,
                            TP_UPSHIFT_CT,
                            USE_WINDOW_API,
                            TP_SAT>() {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fir_interpolate_hb::filter); }

    // FIR
    void filter(
        input_circular_buffer<TT_DATA,
                              extents<inherited_extent>,
                              margin<fnFirMargin<TP_FIR_LEN / kInterpolateFactor, TT_DATA>()> >& __restrict inWindow,
        output_circular_buffer<TT_DATA>& __restrict outWindow,
        output_circular_buffer<TT_DATA>& __restrict outWindow2,
        const TT_COEFF (&inTaps)[(TP_FIR_LEN + 1) / 4 + 1]);
};

//-----------------------------------------------------------------------------------------------------
// Single kernel specialization. No cascade ports, Windowed. dual input, no reload, single output
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_UPSHIFT_CT,
          unsigned int TP_SAT>
class fir_interpolate_hb<TT_DATA,
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
                                                            TP_FIR_LEN,
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
    fir_interpolate_hb(const TT_COEFF (&taps)[(TP_FIR_LEN + 1) / 4 + 1])
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
                            DUAL_IP_DUAL,
                            USE_COEFF_RELOAD_FALSE,
                            1,
                            TP_UPSHIFT_CT,
                            USE_WINDOW_API,
                            TP_SAT>(taps) {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fir_interpolate_hb::filter); }

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
          unsigned int TP_UPSHIFT_CT,
          unsigned int TP_SAT>
class fir_interpolate_hb<TT_DATA,
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
                                                            TP_FIR_LEN,
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
    fir_interpolate_hb(const TT_COEFF (&taps)[(TP_FIR_LEN + 1) / 4 + 1])
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
                            DUAL_IP_DUAL,
                            USE_COEFF_RELOAD_FALSE,
                            2,
                            TP_UPSHIFT_CT,
                            USE_WINDOW_API,
                            TP_SAT>(taps) {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fir_interpolate_hb::filter); }

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
          unsigned int TP_UPSHIFT_CT,
          unsigned int TP_SAT>
class fir_interpolate_hb<TT_DATA,
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
                                                            TP_FIR_LEN,
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
    fir_interpolate_hb()
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
                            DUAL_IP_DUAL,
                            USE_COEFF_RELOAD_TRUE,
                            1,
                            TP_UPSHIFT_CT,
                            USE_WINDOW_API,
                            TP_SAT>() {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fir_interpolate_hb::filter); }

    // FIR
    void filter(
        input_circular_buffer<TT_DATA,
                              extents<inherited_extent>,
                              margin<fnFirMargin<TP_FIR_LEN / kInterpolateFactor, TT_DATA>()> >& __restrict inWindow,
        input_circular_buffer<TT_DATA,
                              extents<inherited_extent>,
                              margin<fnFirMargin<TP_FIR_LEN / kInterpolateFactor, TT_DATA>()> >& __restrict inWindowRev,
        output_circular_buffer<TT_DATA>& __restrict outWindow,
        const TT_COEFF (&inTaps)[(TP_FIR_LEN + 1) / 4 + 1]);
};

// Single kernel specialization. No cascade ports, Windowed. dual input, with reload, dual output
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_UPSHIFT_CT,
          unsigned int TP_SAT>
class fir_interpolate_hb<TT_DATA,
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
                                                            TP_FIR_LEN,
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
    fir_interpolate_hb()
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
                            DUAL_IP_DUAL,
                            USE_COEFF_RELOAD_TRUE,
                            2,
                            TP_UPSHIFT_CT,
                            USE_WINDOW_API,
                            TP_SAT>() {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fir_interpolate_hb::filter); }

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
        const TT_COEFF (&inTaps)[(TP_FIR_LEN + 1) / 4 + 1]);
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
class fir_interpolate_hb<TT_DATA,
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
    fir_interpolate_hb(const TT_COEFF (&taps)[(TP_FIR_LEN + 1) / 4 + 1])
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
    static void registerKernelClass() { REGISTER_FUNCTION(fir_interpolate_hb::filter); }
    // FIR
    void filter(input_async_buffer<TT_DATA, extents<inherited_extent> >& inWindow,
                input_cascade_cacc* inCascade,
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
class fir_interpolate_hb<TT_DATA,
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
    fir_interpolate_hb(const TT_COEFF (&taps)[(TP_FIR_LEN + 1) / 4 + 1])
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
    static void registerKernelClass() { REGISTER_FUNCTION(fir_interpolate_hb::filter); }
    // FIR
    void filter(input_async_buffer<TT_DATA, extents<inherited_extent> >& inWindow,
                input_cascade_cacc* inCascade,
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
class fir_interpolate_hb<TT_DATA,
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
    fir_interpolate_hb()
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
    static void registerKernelClass() { REGISTER_FUNCTION(fir_interpolate_hb::filter); }
    // FIR
    void filter(input_async_buffer<TT_DATA, extents<inherited_extent> >& inWindow,
                input_cascade_cacc* inCascade,
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
class fir_interpolate_hb<TT_DATA,
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
    fir_interpolate_hb()
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
    static void registerKernelClass() { REGISTER_FUNCTION(fir_interpolate_hb::filter); }
    // FIR
    void filter(input_async_buffer<TT_DATA, extents<inherited_extent> >& inWindow,
                input_cascade_cacc* inCascade,
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
class fir_interpolate_hb<TT_DATA,
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
    fir_interpolate_hb(const TT_COEFF (&taps)[(TP_FIR_LEN + 1) / 4 + 1])
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
    static void registerKernelClass() { REGISTER_FUNCTION(fir_interpolate_hb::filter); }
    // FIR
    void filter(input_circular_buffer<TT_DATA,
                                      extents<inherited_extent>,
                                      margin<fnFirMargin<TP_FIR_LEN / kInterpolateFactor, TT_DATA>()> >& inWindow,
                output_cascade_cacc* outCascade,
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
class fir_interpolate_hb<TT_DATA,
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
    fir_interpolate_hb(const TT_COEFF (&taps)[(TP_FIR_LEN + 1) / 4 + 1])
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
    static void registerKernelClass() { REGISTER_FUNCTION(fir_interpolate_hb::filter); }

    // FIR
    void filter(
        input_circular_buffer<TT_DATA,
                              extents<inherited_extent>,
                              margin<fnFirMargin<TP_FIR_LEN / kInterpolateFactor, TT_DATA>()> >& inWindow,
        input_circular_buffer<TT_DATA,
                              extents<inherited_extent>,
                              margin<fnFirMargin<TP_FIR_LEN / kInterpolateFactor, TT_DATA>()> >& inWindowReverse,
        output_cascade_cacc* outCascade,
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
class fir_interpolate_hb<TT_DATA,
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
    fir_interpolate_hb()
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
    static void registerKernelClass() { REGISTER_FUNCTION(fir_interpolate_hb::filter); }

    // FIR
    void filter(input_circular_buffer<TT_DATA,
                                      extents<inherited_extent>,
                                      margin<fnFirMargin<TP_FIR_LEN / kInterpolateFactor, TT_DATA>()> >& inWindow,
                output_cascade_cacc* outCascade,
                output_async_buffer<TT_DATA>& broadcastWindow,
                const TT_COEFF (&inTaps)[(TP_FIR_LEN + 1) / 4 + 1]);
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
class fir_interpolate_hb<TT_DATA,
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
    fir_interpolate_hb()
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
    static void registerKernelClass() { REGISTER_FUNCTION(fir_interpolate_hb::filter); }

    // FIR
    void filter(
        input_circular_buffer<TT_DATA,
                              extents<inherited_extent>,
                              margin<fnFirMargin<TP_FIR_LEN / kInterpolateFactor, TT_DATA>()> >& inWindow,
        input_circular_buffer<TT_DATA,
                              extents<inherited_extent>,
                              margin<fnFirMargin<TP_FIR_LEN / kInterpolateFactor, TT_DATA>()> >& inWindowReverse,
        output_cascade_cacc* outCascade,
        output_async_buffer<TT_DATA>& broadcastWindow,
        const TT_COEFF (&inTaps)[(TP_FIR_LEN + 1) / 4 + 1]);
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
class fir_interpolate_hb<TT_DATA,
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
    fir_interpolate_hb(const TT_COEFF (&taps)[(TP_FIR_LEN + 1) / 4 + 1])
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
    static void registerKernelClass() { REGISTER_FUNCTION(fir_interpolate_hb::filter); }
    // FIR
    void filter(input_async_buffer<TT_DATA>& inWindow,
                input_cascade_cacc* inCascade,
                output_cascade_cacc* outCascade,
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
class fir_interpolate_hb<TT_DATA,
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
    fir_interpolate_hb()
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
    static void registerKernelClass() { REGISTER_FUNCTION(fir_interpolate_hb::filter); }

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
          unsigned int TP_UPSHIFT_CT,
          unsigned int TP_SAT>
class fir_interpolate_hb<TT_DATA,
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
    fir_interpolate_hb(const TT_COEFF (&taps)[(TP_FIR_LEN + 1) / 4 + 1])
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
    static void registerKernelClass() { REGISTER_FUNCTION(fir_interpolate_hb::filter); }

    // FIR
    void filter(input_circular_buffer<TT_DATA,
                                      extents<inherited_extent>,
                                      margin<fnFirMargin<TP_FIR_LEN / kInterpolateFactor, TT_DATA>()> >& inWindow,
                output_stream<TT_DATA>* outStream);
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
class fir_interpolate_hb<TT_DATA,
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
    fir_interpolate_hb(const TT_COEFF (&taps)[(TP_FIR_LEN + 1) / 4 + 1])
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
    static void registerKernelClass() { REGISTER_FUNCTION(fir_interpolate_hb::filter); }

    // FIR
    void filter(input_circular_buffer<TT_DATA,
                                      extents<inherited_extent>,
                                      margin<fnFirMargin<TP_FIR_LEN / kInterpolateFactor, TT_DATA>()> >& inWindow,
                output_stream<TT_DATA>* outStream,
                output_stream<TT_DATA>* outStream2);
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
class fir_interpolate_hb<TT_DATA,
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
    fir_interpolate_hb()
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
    static void registerKernelClass() { REGISTER_FUNCTION(fir_interpolate_hb::filter); }

    // FIR
    void filter(input_circular_buffer<TT_DATA,
                                      extents<inherited_extent>,
                                      margin<fnFirMargin<TP_FIR_LEN / kInterpolateFactor, TT_DATA>()> >& inWindow,
                output_stream<TT_DATA>* outStream,
                const TT_COEFF (&inTaps)[(TP_FIR_LEN + 1) / 4 + 1]);
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
class fir_interpolate_hb<TT_DATA,
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
    fir_interpolate_hb()
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
    static void registerKernelClass() { REGISTER_FUNCTION(fir_interpolate_hb::filter); }

    // FIR
    void filter(input_circular_buffer<TT_DATA,
                                      extents<inherited_extent>,
                                      margin<fnFirMargin<TP_FIR_LEN / kInterpolateFactor, TT_DATA>()> >& inWindow,
                output_stream<TT_DATA>* outStream,
                output_stream<TT_DATA>* outStream2,
                const TT_COEFF (&inTaps)[(TP_FIR_LEN + 1) / 4 + 1]);
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
class fir_interpolate_hb<TT_DATA,
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
    fir_interpolate_hb(const TT_COEFF (&taps)[(TP_FIR_LEN + 1) / 4 + 1])
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
    static void registerKernelClass() { REGISTER_FUNCTION(fir_interpolate_hb::filter); }

    // FIR
    void filter(input_circular_buffer<TT_DATA,
                                      extents<inherited_extent>,
                                      margin<fnFirMargin<TP_FIR_LEN / kInterpolateFactor, TT_DATA>()> >& inWindow,
                input_cascade_cacc* inCascade,
                output_stream<TT_DATA>* outStream);
};

//-----------------------------------------------------------------------------------------------------
// Cascaded Kernels - Final Kernel. Static coefficients. Streaming. Dual input. Single Output
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
class fir_interpolate_hb<TT_DATA,
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
                         DUAL_IP_DUAL,
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
                                                            DUAL_IP_DUAL,
                                                            USE_COEFF_RELOAD_FALSE,
                                                            1,
                                                            TP_UPSHIFT_CT,
                                                            USE_STREAM_API,
                                                            TP_SAT> {
   public:
    // Constructor
    fir_interpolate_hb(const TT_COEFF (&taps)[(TP_FIR_LEN + 1) / 4 + 1])
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
                            DUAL_IP_DUAL,
                            USE_COEFF_RELOAD_FALSE,
                            1,
                            TP_UPSHIFT_CT,
                            USE_STREAM_API,
                            TP_SAT>(taps) {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fir_interpolate_hb::filter); }

    // FIR
    void filter(input_async_buffer<TT_DATA>& inWindow,
                input_cascade_cacc* inCascade,
                output_stream<TT_DATA>* outStream);
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
class fir_interpolate_hb<TT_DATA,
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
    fir_interpolate_hb(const TT_COEFF (&taps)[(TP_FIR_LEN + 1) / 4 + 1])
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
    static void registerKernelClass() { REGISTER_FUNCTION(fir_interpolate_hb::filter); }

    // FIR
    void filter(input_circular_buffer<TT_DATA,
                                      extents<inherited_extent>,
                                      margin<fnFirMargin<TP_FIR_LEN / kInterpolateFactor, TT_DATA>()> >& inWindow,
                input_cascade_cacc* inCascade,
                output_stream<TT_DATA>* outStream,
                output_stream<TT_DATA>* outStream2);
};

//-----------------------------------------------------------------------------------------------------
// Cascaded Kernels - Final Kernel. Static coefficients. Streaming. Dual Input. Dual Output
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
class fir_interpolate_hb<TT_DATA,
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
                         DUAL_IP_DUAL,
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
                                                            DUAL_IP_DUAL,
                                                            USE_COEFF_RELOAD_FALSE,
                                                            2,
                                                            TP_UPSHIFT_CT,
                                                            USE_STREAM_API,
                                                            TP_SAT> {
   public:
    // Constructor
    fir_interpolate_hb(const TT_COEFF (&taps)[(TP_FIR_LEN + 1) / 4 + 1])
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
                            DUAL_IP_DUAL,
                            USE_COEFF_RELOAD_FALSE,
                            2,
                            TP_UPSHIFT_CT,
                            USE_STREAM_API,
                            TP_SAT>(taps) {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fir_interpolate_hb::filter); }

    // FIR
    void filter(input_async_buffer<TT_DATA>& inWindow,
                input_cascade_cacc* inCascade,
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
class fir_interpolate_hb<TT_DATA,
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
    fir_interpolate_hb(const TT_COEFF (&taps)[(TP_FIR_LEN + 1) / 4 + 1])
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
    static void registerKernelClass() { REGISTER_FUNCTION(fir_interpolate_hb::filter); }

    // FIR
    void filter(input_circular_buffer<TT_DATA,
                                      extents<inherited_extent>,
                                      margin<fnFirMargin<TP_FIR_LEN / kInterpolateFactor, TT_DATA>()> >& inWindow,
                output_cascade_cacc* outCascade);
};

//-----------------------------------------------------------------------------------------------------
// Cascaded Kernels - First Kernel. Streaming. Static coefficients. Dual Input
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
class fir_interpolate_hb<TT_DATA,
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
                         DUAL_IP_DUAL,
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
                                                            DUAL_IP_DUAL,
                                                            USE_COEFF_RELOAD_FALSE,
                                                            1,
                                                            TP_UPSHIFT_CT,
                                                            USE_STREAM_API,
                                                            TP_SAT> {
   public:
    // Constructor
    fir_interpolate_hb(const TT_COEFF (&taps)[(TP_FIR_LEN + 1) / 4 + 1])
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
                            DUAL_IP_DUAL,
                            USE_COEFF_RELOAD_FALSE,
                            1,
                            TP_UPSHIFT_CT,
                            USE_STREAM_API,
                            TP_SAT>(taps) {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fir_interpolate_hb::filter); }

    // FIR
    // Note: dual input streams go via widget and are converted to one window, but this is broadcast
    void filter(input_circular_buffer<TT_DATA,
                                      extents<inherited_extent>,
                                      margin<fnFirMargin<TP_FIR_LEN / kInterpolateFactor, TT_DATA>()> >& inWindow,
                output_cascade_cacc* outCascade,
                output_async_buffer<TT_DATA>& broadcastWindow);
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
class fir_interpolate_hb<TT_DATA,
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
    fir_interpolate_hb(const TT_COEFF (&taps)[(TP_FIR_LEN + 1) / 4 + 1])
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
    static void registerKernelClass() { REGISTER_FUNCTION(fir_interpolate_hb::filter); }

    // FIR
    void filter(input_circular_buffer<TT_DATA,
                                      extents<inherited_extent>,
                                      margin<fnFirMargin<TP_FIR_LEN / kInterpolateFactor, TT_DATA>()> >& inWindow,
                input_cascade_cacc* inCascade,
                output_cascade_cacc* outCascade);
};

//-----------------------------------------------------------------------------------------------------
// Cascaded Kernels - Middle Kernel. Streaming. Static coefficients. Dual Input
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
class fir_interpolate_hb<TT_DATA,
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
                         DUAL_IP_DUAL,
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
                                                            DUAL_IP_DUAL,
                                                            USE_COEFF_RELOAD_FALSE,
                                                            1,
                                                            TP_UPSHIFT_CT,
                                                            USE_STREAM_API,
                                                            TP_SAT> {
   public:
    // Constructor
    fir_interpolate_hb(const TT_COEFF (&taps)[(TP_FIR_LEN + 1) / 4 + 1])
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
                            DUAL_IP_DUAL,
                            USE_COEFF_RELOAD_FALSE,
                            1,
                            TP_UPSHIFT_CT,
                            USE_STREAM_API,
                            TP_SAT>(taps) {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fir_interpolate_hb::filter); }

    // FIR
    void filter(input_async_buffer<TT_DATA>& inWindow,
                input_cascade_cacc* inCascade,
                output_cascade_cacc* outCascade,
                output_async_buffer<TT_DATA>& broadcastWindow);
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
class fir_interpolate_hb<TT_DATA,
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
    fir_interpolate_hb()
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
    static void registerKernelClass() { REGISTER_FUNCTION(fir_interpolate_hb::filter); }

    // FIR
    void filter(input_circular_buffer<TT_DATA,
                                      extents<inherited_extent>,
                                      margin<fnFirMargin<TP_FIR_LEN / kInterpolateFactor, TT_DATA>()> >& inWindow,
                input_cascade_cacc* inCascade,
                output_stream<TT_DATA>* outStream);
};

//-----------------------------------------------------------------------------------------------------
// Cascaded Kernels - Final Kernel. Streaming. Reloadable coefficients. Dual input. Single Output
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
class fir_interpolate_hb<TT_DATA,
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
                         DUAL_IP_DUAL,
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
                                                            DUAL_IP_DUAL,
                                                            USE_COEFF_RELOAD_TRUE,
                                                            1,
                                                            TP_UPSHIFT_CT,
                                                            USE_STREAM_API,
                                                            TP_SAT> {
   public:
    // Constructor
    fir_interpolate_hb()
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
                            DUAL_IP_DUAL,
                            USE_COEFF_RELOAD_TRUE,
                            1,
                            TP_UPSHIFT_CT,
                            USE_STREAM_API,
                            TP_SAT>() {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fir_interpolate_hb::filter); }

    // FIR
    void filter(input_async_buffer<TT_DATA>& inWindow,
                input_cascade_cacc* inCascade,
                output_stream<TT_DATA>* outStream);
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
class fir_interpolate_hb<TT_DATA,
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
    fir_interpolate_hb()
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
    static void registerKernelClass() { REGISTER_FUNCTION(fir_interpolate_hb::filter); }

    // FIR
    void filter(input_circular_buffer<TT_DATA,
                                      extents<inherited_extent>,
                                      margin<fnFirMargin<TP_FIR_LEN / kInterpolateFactor, TT_DATA>()> >& inWindow,
                input_cascade_cacc* inCascade,
                output_stream<TT_DATA>* outStream,
                output_stream<TT_DATA>* outStream2);
};

//-----------------------------------------------------------------------------------------------------
// Cascaded Kernels - Final Kernel. Streaming. Reloadable coefficients. Dual input. Dual Output
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
class fir_interpolate_hb<TT_DATA,
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
                         DUAL_IP_DUAL,
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
                                                            DUAL_IP_DUAL,
                                                            USE_COEFF_RELOAD_TRUE,
                                                            2,
                                                            TP_UPSHIFT_CT,
                                                            USE_STREAM_API,
                                                            TP_SAT> {
   public:
    // Constructor
    fir_interpolate_hb()
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
                            DUAL_IP_DUAL,
                            USE_COEFF_RELOAD_TRUE,
                            2,
                            TP_UPSHIFT_CT,
                            USE_STREAM_API,
                            TP_SAT>() {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fir_interpolate_hb::filter); }

    // FIR
    void filter(input_async_buffer<TT_DATA>& inWindow,
                input_cascade_cacc* inCascade,
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
class fir_interpolate_hb<TT_DATA,
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
    fir_interpolate_hb()
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
    static void registerKernelClass() { REGISTER_FUNCTION(fir_interpolate_hb::filter); }

    // FIR
    void filter(input_circular_buffer<TT_DATA,
                                      extents<inherited_extent>,
                                      margin<fnFirMargin<TP_FIR_LEN / kInterpolateFactor, TT_DATA>()> >& inWindow,
                output_cascade_cacc* outCascade,
                const TT_COEFF (&inTaps)[(TP_FIR_LEN + 1) / 4 + 1]);
};

//-----------------------------------------------------------------------------------------------------
// Cascaded Kernels - First Kernel. Streaming. Reloadable coefficients. Dual Input
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
class fir_interpolate_hb<TT_DATA,
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
                         DUAL_IP_DUAL,
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
                                                            DUAL_IP_DUAL,
                                                            USE_COEFF_RELOAD_TRUE,
                                                            1,
                                                            TP_UPSHIFT_CT,
                                                            USE_STREAM_API,
                                                            TP_SAT> {
   public:
    // Constructor
    fir_interpolate_hb()
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
                            DUAL_IP_DUAL,
                            USE_COEFF_RELOAD_TRUE,
                            1,
                            TP_UPSHIFT_CT,
                            USE_STREAM_API,
                            TP_SAT>() {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fir_interpolate_hb::filter); }

    // FIR
    void filter(input_circular_buffer<TT_DATA,
                                      extents<inherited_extent>,
                                      margin<fnFirMargin<TP_FIR_LEN / kInterpolateFactor, TT_DATA>()> >& inWindow,
                output_cascade_cacc* outCascade,
                output_async_buffer<TT_DATA>& broadcastWindow,
                const TT_COEFF (&inTaps)[(TP_FIR_LEN + 1) / 4 + 1]);
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
class fir_interpolate_hb<TT_DATA,
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
    fir_interpolate_hb()
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
    static void registerKernelClass() { REGISTER_FUNCTION(fir_interpolate_hb::filter); }

    // FIR
    void filter(input_circular_buffer<TT_DATA,
                                      extents<inherited_extent>,
                                      margin<fnFirMargin<TP_FIR_LEN / kInterpolateFactor, TT_DATA>()> >& inWindow,
                input_cascade_cacc* inCascade,
                output_cascade_cacc* outCascade);
};

//-----------------------------------------------------------------------------------------------------
// Cascaded Kernels - Middle Kernel. Streaming. Reloadable coefficients. Dual Input
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
class fir_interpolate_hb<TT_DATA,
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
                         DUAL_IP_DUAL,
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
                                                            DUAL_IP_DUAL,
                                                            USE_COEFF_RELOAD_TRUE,
                                                            1,
                                                            TP_UPSHIFT_CT,
                                                            USE_STREAM_API,
                                                            TP_SAT> {
   public:
    // Constructor
    fir_interpolate_hb()
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
                            DUAL_IP_DUAL,
                            USE_COEFF_RELOAD_TRUE,
                            1,
                            TP_UPSHIFT_CT,
                            USE_STREAM_API,
                            TP_SAT>() {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fir_interpolate_hb::filter); }

    // FIR
    void filter(input_async_buffer<TT_DATA>& inWindow,
                input_cascade_cacc* inCascade,
                output_cascade_cacc* outCascade,
                output_async_buffer<TT_DATA>& broadcastWindow);
};

template <typename fp = fir_params_defaults>
class fir_interpolate_hb_tl : public fir_interpolate_hb<typename fp::BTT_DATA,
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
    };

    // Get kernel's FIR range Length
    static constexpr unsigned int getFirRangeLen() { return fp::BTP_FIR_RANGE_LEN; };

    // Get kernel's FIR Total Tap Length
    static constexpr unsigned int getTapLen() { return (fp::BTP_FIR_LEN + 1) / 4 + 1; };

    // Get kernel's FIR Total Tap Length
    static constexpr unsigned int getSSRMargin() {
        constexpr unsigned int margin = fnFirMargin<fp::BTP_FIR_LEN / fp::BTP_SSR, typename fp::BTT_DATA>();
        return margin;
    };

    static constexpr unsigned int getDF() { return 1; };
    static constexpr unsigned int getIF() { return 1; };

    // Get FIR variant
    static constexpr eFIRVariant getFirType() { return eFIRVariant::kIntHB; };
    using parent_class = fir_interpolate_hb<typename fp::BTT_DATA,
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

#endif // _DSPLIB_fir_interpolate_hb_HPP_
