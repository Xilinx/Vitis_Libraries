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
#ifndef _DSPLIB_FIR_DECIMATE_SYM_HPP_
#define _DSPLIB_FIR_DECIMATE_SYM_HPP_

/*
Symmetric Decimation FIR.
This file exists to capture the definition of the symmetric decimation FIR
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
   So as to achieve the impulse response from the filter which matches the coefficient set, the coefficient array
   usually has to be reversed
   to compensate for the action of the intrinsics. However, for symmetrical coefficient sets, this reversal would result
   in the original set, so is unnecessary.
   This decimator implementation solves all polyphases for an output in a single lane. For large decimation factors, or
   large number of lanes
   (as required by data and coefficient type), it is not always possible to accommodate the input data step between
   lanes required because the maximum
   offset between lanes in a single operation is limited to 15. Hence, the implementation may operate on fewer lanes per
   operation than the hardware supports.
*/

#include <adf.h>
#include "fir_utils.hpp"
#include "fir_decimate_sym_traits.hpp"
#include <vector>

//#define _DSPLIB_FIR_DECIMATE_SYM_HPP_DEBUG_

namespace xf {
namespace dsp {
namespace aie {
namespace fir {
namespace decimate_sym {
//-----------------------------------------------------------------------------------------------------
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
          unsigned int TP_DUAL_IP = 0,
          unsigned int TP_USE_COEFF_RELOAD = 0,
          unsigned int TP_NUM_OUTPUTS = 1,
          unsigned int TP_API = 0,
          unsigned int TP_SAT = 1>
class kernelFilterClass {
   private:
// Parameter value defensive and legality checks
#include "fir_decimate_sym_check_params.hpp"
    // There are additional defensive checks after architectural constants have been calculated.

    // constants derived from configuration parameters
    // First, determine the architecture to use. This is dependent on many factors and would be best captured as a
    // function,
    // but as yet aiecompiler does not support constexpr functions fully, so instead ?: syntax is used.
    static constexpr unsigned int m_kColumns1buff =
        fnNumColumnsDecSym<TT_DATA, TT_COEFF>(); // number of mult-adds per lane for main intrinsic
    static constexpr unsigned int m_kLanes1buff =
        fnNumLanesDecSym<TT_DATA, TT_COEFF>(); // number of lanes vector intrinsic would operate, when architecture is
                                               // optimized to use single data buffer.
    static constexpr unsigned int m_kFirLenCeilCols1buff =
        CEIL((TP_FIR_RANGE_LEN + 1) / kSymmetryFactor, m_kColumns1buff);
    static constexpr unsigned int m_kNumOps1buff = m_kFirLenCeilCols1buff / m_kColumns1buff;
    static constexpr unsigned int m_kDataLoadsInReg =
        fnDataLoadsInRegDecSym<TT_DATA>(); // 4;  //ratio of sbuff to load size.
    static constexpr unsigned int m_kDataLoadVsize1buff =
        fnLoadSizeDecSym<TT_DATA, TT_COEFF, kArch1BuffLowDFBasic>() /
        (sizeof(TT_DATA) * 8); // numerator is bits, sizeof is bytes, hence 8
    static constexpr unsigned int m_kVOutSize = fnVOutSizeDecSym<TT_DATA, TT_COEFF>(); // Output vector size.
    static constexpr unsigned int m_kSamplesInDataBuff1buff = kBuffSize128Byte / sizeof(TT_DATA);
    static constexpr unsigned int m_kCoeffRegVsize = kBuffSize32Byte / sizeof(TT_COEFF);
    static constexpr unsigned int m_kFirRangeOffset =
        fnFirRangeOffsetSym<TP_FIR_LEN, TP_CASC_LEN, TP_KERNEL_POSITION>(); // FIR Cascade Offset for this kernel
                                                                            // position
    static constexpr unsigned int m_kFirMarginOffset =
        fnFirMargin<TP_FIR_LEN, TT_DATA>() - TP_FIR_LEN + 1; // FIR Margin Offset.
    static constexpr unsigned int m_kFirInitOffset = m_kFirRangeOffset + m_kFirMarginOffset;
    static constexpr unsigned int m_kDataBuffXOffset =
        m_kFirInitOffset % (16 / sizeof(TT_DATA)); // Remainder of m_kFirInitOffset divided by 128bit

    static constexpr unsigned int m_kArchFirLen =
        TP_FIR_LEN; // Check if only the remainder of the FIR (TP_FIR_RANGE_LEN + m_Lanes - due to xoffset alignment)
                    // for last kernel in chain fits, otherwise check if full FIR (TP_FIR_LEN) fits.
    static constexpr unsigned int m_k1buffSupported =
        TP_CASC_LEN == 1 ? fnFirDecSym1buffSupport<TT_DATA, TT_COEFF>()
                         : NOT_SUPPORTED; // Supported for cascade length 1. Longer cascades introduce enough overhead
                                          // to make the arch inefficient.
    static constexpr unsigned int m_kUse1buff =
        (m_k1buffSupported == SUPPORTED) &&
                ((m_kDataBuffXOffset + m_kArchFirLen + ((m_kLanes1buff - 1) * TP_DECIMATE_FACTOR)) <=
                 m_kSamplesInDataBuff1buff) &&
                ((m_kArchFirLen + 1) / 2 <= m_kCoeffRegVsize)
            ? SUPPORTED
            : NOT_SUPPORTED;
    static constexpr unsigned int m_kRepeatFactor =
        TP_DECIMATE_FACTOR % 2 == 0 ? m_kDataLoadVsize1buff
                                    : m_kSamplesInDataBuff1buff / m_kVOutSize; // only FACTORS of 2 or 3 supported
    static constexpr unsigned int m_kUse1incrStrobe =
        (m_kArchFirLen + ((m_kLanes1buff - 1) * TP_DECIMATE_FACTOR) <
         m_kSamplesInDataBuff1buff - m_kDataLoadVsize1buff) &&
                (TP_INPUT_WINDOW_VSIZE % (m_kLanes1buff * m_kRepeatFactor * TP_DECIMATE_FACTOR) == 0)
            ? 1
            : 0;
    static constexpr unsigned int m_kArch =
        m_kUse1buff == 0 ? kArch2BuffLowDFBasic
                         : m_kUse1incrStrobe == 1 ? kArch1BuffLowDFIncrStrobe : kArch1BuffLowDFBasic;
    static constexpr unsigned int m_kColumns =
        fnNumColumnsDecSym<TT_DATA, TT_COEFF>(); // number of mult-adds per lane for main intrinsic
    static constexpr unsigned int m_kLanes = fnNumLanesDecSym<TT_DATA, TT_COEFF>(); // number of operations in parallel
                                                                                    // of this type combinations that
                                                                                    // the vector processor can do.
    static constexpr unsigned int m_kFirLenCeilCols = CEIL((TP_FIR_RANGE_LEN + 1) / kSymmetryFactor, m_kColumns);
    static constexpr unsigned int m_kNumOps = m_kFirLenCeilCols / m_kColumns;
    static constexpr unsigned int m_kDataLoadVsize =
        fnLoadSizeDecSym<TT_DATA, TT_COEFF, m_kArch>() /
        (sizeof(TT_DATA) * 8); // numerator is bits, sizeof is bytes, hence 8
    static constexpr unsigned int m_kSamplesInDataBuff = m_kDataLoadVsize * m_kDataLoadsInReg;
    static constexpr unsigned int m_kDataRegVsize = m_kDataLoadsInReg * m_kDataLoadVsize;
    static constexpr unsigned int m_kLsize =
        fnLsize<TP_INPUT_WINDOW_VSIZE, TP_DECIMATE_FACTOR, m_kVOutSize>(); // loop length, given that <m_kVOutSize>
                                                                           // samples are output per iteration of loop
    static constexpr unsigned int m_kFinalOpSkew =
        m_kColumns == 1 ? 0
                        : (m_kColumns - (((TP_FIR_RANGE_LEN + 1) / kSymmetryFactor)) % m_kColumns) %
                              m_kColumns; // Data buffer skew for CT operation. No skew for single column type combos.

    // The static assertion is after the initial resolution of constants because it is much easier to phrase using the
    // derived value m_kLanes.
    static_assert(TP_INPUT_WINDOW_VSIZE % (TP_DECIMATE_FACTOR * m_kLanes) == 0,
                  "TP_INPUT_WINDOW_VSIZE must be an integer multiple of TP_DECIMATE_FACTOR and the number of lanes in "
                  "the vector processor for this data and coefficient type");

    static_assert(TP_NUM_OUTPUTS > 0 && TP_NUM_OUTPUTS <= 2, "ERROR: only single or dual outputs are supported.");

    // v8cint16 is a convenient 256b type. This line also initialized the array to zero to ensure zero padding.
    static constexpr unsigned int m_kCoeffLoadSize = 256 / 8 / sizeof(TT_COEFF);

    // The coefficients array must include zero padding up to a multiple of the number of columns
    // the MAC intrinsic used to eliminate the accidental inclusion of terms beyond the FIR length.
    // Since this zero padding cannot be applied to the class-external coefficient array
    // the supplied taps are copied to an internal array, m_internalTaps, which can be padded.
    alignas(__ALIGN_BYTE_SIZE__) TT_COEFF
        m_internalTaps[CEIL((TP_FIR_RANGE_LEN + 1) / kSymmetryFactor, m_kCoeffLoadSize)];

    alignas(__ALIGN_BYTE_SIZE__) TT_COEFF
        m_oldInTaps[CEIL((TP_FIR_LEN + 1) / kSymmetryFactor,
                         m_kCoeffLoadSize)]; // Previous user input coefficients with zero padding
    bool m_coeffnEq;                         // Are coefficients sets equal?

    void filterSelectArch(T_inputIF<TP_CASC_IN, TT_DATA, TP_DUAL_IP> inInterface,
                          T_outputIF<TP_CASC_OUT, TT_DATA> outInterface);
    void filter1BuffLowDFBasic(T_inputIF<TP_CASC_IN, TT_DATA, TP_DUAL_IP> inInterface,
                               T_outputIF<TP_CASC_OUT, TT_DATA> outInterface);
    void filter1BuffLowDFIncrStrobe(T_inputIF<TP_CASC_IN, TT_DATA, TP_DUAL_IP> inInterface,
                                    T_outputIF<TP_CASC_OUT, TT_DATA> outInterface);
    void filter2BuffLowDFBasic(T_inputIF<TP_CASC_IN, TT_DATA, TP_DUAL_IP> inInterface,
                               T_outputIF<TP_CASC_OUT, TT_DATA> outInterface);
    unsigned int m_kDecimateOffsets, m_kDecimateOffsetsHi; // hi is for int16/int16 only.

   public:
    // Access function for AIE Synthesizer
    static unsigned int get_m_kArch() { return m_kArch; };

    // FIR
    void filterKernel(T_inputIF<TP_CASC_IN, TT_DATA, TP_DUAL_IP> inInterface,
                      T_outputIF<TP_CASC_OUT, TT_DATA> outInterface);

    // with taps for reload
    void filterKernel(T_inputIF<TP_CASC_IN, TT_DATA, TP_DUAL_IP> inInterface,
                      T_outputIF<TP_CASC_OUT, TT_DATA> outInterface,
                      const TT_COEFF (&inTaps)[(TP_FIR_LEN + 1) / kSymmetryFactor]);

    void filterKernelRtp(T_inputIF<TP_CASC_IN, TT_DATA, TP_DUAL_IP> inInterface,
                         T_outputIF<TP_CASC_OUT, TT_DATA> outInterface);

    // Constructor
    kernelFilterClass(const TT_COEFF (&taps)[((TP_FIR_LEN + 1) / kSymmetryFactor)]) : m_internalTaps{} {
        setDecimateOffsets();
        firReload(taps);
    };

    // Constructor used for reloadable coefficients
    kernelFilterClass() : m_oldInTaps{}, m_internalTaps{} { setDecimateOffsets(); }

    // setDecimateOffsets
    void setDecimateOffsets() {
        switch (TP_DECIMATE_FACTOR) {
            case 2:
                m_kDecimateOffsets = 0xECA86420;
                break; // No point in hi because range is exceeded.
            case 3:
                m_kDecimateOffsets = 0x9630;
                break; // only good up to 4 lanes
            case 4:
                m_kDecimateOffsets = 0xC840;
                break; // only good up to 4 lanes
            case 5:
                m_kDecimateOffsets = 0xFA50;
                break; // only good up to 4 lanes
            default:
                break;
        }
    }; // setDecimateOffsets

    void firReload(const TT_COEFF* taps) {
        TT_COEFF* tapsPtr = (TT_COEFF*)taps;
        firReload(tapsPtr);
    }

    void firReload(TT_COEFF* taps) {
        TT_COEFF* tapsPtr = (TT_COEFF*)taps;
        TT_COEFF* __restrict m_internalTapsPtr = (TT_COEFF*)m_internalTaps;

        // Since the intrinsics can have columns, any values in memory beyond the end of the taps array could
        // contaminate the calculation.
        // To avoid this hazard, the class has its own taps array which is zero-padded to the column width for the type
        // of coefficient.
        int tapIndex;
        int tapIndexMax; // highest index of conventionally handled taps (excepting operation which includes centre tap
        int op;
        int column;
        // Coefficients are pre-arranged such that during filter execution they may simply be read from a lookup table.
        for (op = 0; op < m_kNumOps; ++op) {
            if ((op != m_kNumOps - 1) || (TP_FIR_RANGE_LEN % 2 == 0)) {
                for (column = 0; column < m_kColumns; ++column) {
                    tapIndex = (op * m_kColumns + column);
                    if (tapIndex < (TP_FIR_RANGE_LEN + 1) /
                                       kSymmetryFactor) { // symmetry and halfband mean only 1/4 coeffs matter.
                        m_internalTapsPtr[op * m_kColumns + column] =
                            tapsPtr[tapIndex + fnFirRangeOffsetSym<TP_FIR_LEN, TP_CASC_LEN, TP_KERNEL_POSITION>()];
                    } else {
                        m_internalTapsPtr[op * m_kColumns + column] = nullElem<TT_COEFF>(); // 0 for the type.
                    }
                }
            } else { // odd length filters have a centre tap which requires special handling.
                // The final operation, including the centre tap, requires special handling.
                // since the centre tap is in the final column, not all columns may be in use
                // so must be zeroed.
                tapIndexMax = ((m_kNumOps - 2) * m_kColumns + m_kColumns - 1);
                tapIndex = (TP_FIR_RANGE_LEN + 1) / kSymmetryFactor - 1; // centre tap position
                m_internalTapsPtr[(m_kNumOps - 1) * m_kColumns + m_kColumns - 1] =
                    tapsPtr[tapIndex + fnFirRangeOffsetSym<TP_FIR_LEN, TP_CASC_LEN, TP_KERNEL_POSITION>()]; // goes to
                                                                                                            // the last
                                                                                                            // column of
                                                                                                            // the last
                                                                                                            // op.
                tapIndex--;
                for (column = m_kColumns - 2; column >= 0; column--) {
                    if (tapIndex > tapIndexMax) {
                        m_internalTapsPtr[op * m_kColumns + column] =
                            tapsPtr[tapIndex + fnFirRangeOffsetSym<TP_FIR_LEN, TP_CASC_LEN, TP_KERNEL_POSITION>()];
                        tapIndex--;
                    } else {
                        m_internalTapsPtr[op * m_kColumns + column] = nullElem<TT_COEFF>();
                    }
                } // col
            }     // if else for centre tap
        }         // op
    }
};

//-----------------------------------------------------------------------------------------------------
// Cascade layer class and specializations

//-----------------------------------------------------------------------------------------------------
// Standalone kernel specialization with no cascade ports, Windowed. a single input, no reload, single output
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
          unsigned int TP_DUAL_IP = 0,
          unsigned int TP_USE_COEFF_RELOAD = 0,
          unsigned int TP_NUM_OUTPUTS = 1,
          unsigned int TP_API = 0,
          unsigned int TP_SAT = 1>
class fir_decimate_sym : public kernelFilterClass<TT_DATA,
                                                  TT_COEFF,
                                                  TP_FIR_LEN,
                                                  TP_DECIMATE_FACTOR,
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
                                                  USE_WINDOW_API,
                                                  TP_SAT> {
   private:
   public:
    // Constructor
    fir_decimate_sym(const TT_COEFF (&taps)[(TP_FIR_LEN + 1) / kSymmetryFactor])
        : kernelFilterClass<TT_DATA,
                            TT_COEFF,
                            TP_FIR_LEN,
                            TP_DECIMATE_FACTOR,
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
                            1,
                            USE_WINDOW_API,
                            TP_SAT>(taps) {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fir_decimate_sym::filter); }

    // FIR
    void filter(input_circular_buffer<TT_DATA,
                                      extents<inherited_extent>,
                                      margin<fnFirMargin<TP_FIR_LEN, TT_DATA>()> >& __restrict inWindow,
                output_circular_buffer<TT_DATA>& __restrict outWindow);
};

// Single kernel specialization. No cascade ports, Windowed. single input, with static coefficients, dual outputs
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_DECIMATE_FACTOR,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_SAT>
class fir_decimate_sym<TT_DATA,
                       TT_COEFF,
                       TP_FIR_LEN,
                       TP_DECIMATE_FACTOR,
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
                       USE_WINDOW_API,
                       TP_SAT> : public kernelFilterClass<TT_DATA,
                                                          TT_COEFF,
                                                          TP_FIR_LEN,
                                                          TP_DECIMATE_FACTOR,
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
                                                          USE_WINDOW_API,
                                                          TP_SAT> {
   private:
   public:
    // Constructor
    fir_decimate_sym(const TT_COEFF (&taps)[(TP_FIR_LEN + 1) / kSymmetryFactor])
        : kernelFilterClass<TT_DATA,
                            TT_COEFF,
                            TP_FIR_LEN,
                            TP_DECIMATE_FACTOR,
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
                            USE_WINDOW_API,
                            TP_SAT>(taps) {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fir_decimate_sym::filter); }

    // FIR
    void filter(input_circular_buffer<TT_DATA,
                                      extents<inherited_extent>,
                                      margin<fnFirMargin<TP_FIR_LEN, TT_DATA>()> >& __restrict inWindow,
                output_circular_buffer<TT_DATA>& __restrict outWindow,
                output_circular_buffer<TT_DATA>& __restrict outWindow2);
};

// Single kernel specialization. No cascade ports, Windowed. single input, with reload coefficients, single output
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_DECIMATE_FACTOR,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_SAT>
class fir_decimate_sym<TT_DATA,
                       TT_COEFF,
                       TP_FIR_LEN,
                       TP_DECIMATE_FACTOR,
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
                                                          TP_DECIMATE_FACTOR,
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
    fir_decimate_sym()
        : kernelFilterClass<TT_DATA,
                            TT_COEFF,
                            TP_FIR_LEN,
                            TP_DECIMATE_FACTOR,
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
    static void registerKernelClass() { REGISTER_FUNCTION(fir_decimate_sym::filter); }

    // FIR
    void filter(input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<fnFirMargin<TP_FIR_LEN, TT_DATA>()> >&
                    inWindow,
                output_circular_buffer<TT_DATA>& outWindow,
                const TT_COEFF (&inTaps)[(TP_FIR_LEN + 1) / kSymmetryFactor]);
};

// Single kernel specialization. No cascade ports, Windowed. single input, with reload coefficients, dual output
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_DECIMATE_FACTOR,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_SAT>
class fir_decimate_sym<TT_DATA,
                       TT_COEFF,
                       TP_FIR_LEN,
                       TP_DECIMATE_FACTOR,
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
                       USE_WINDOW_API,
                       TP_SAT> : public kernelFilterClass<TT_DATA,
                                                          TT_COEFF,
                                                          TP_FIR_LEN,
                                                          TP_DECIMATE_FACTOR,
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
                                                          USE_WINDOW_API,
                                                          TP_SAT> {
   private:
   public:
    // Constructor
    fir_decimate_sym()
        : kernelFilterClass<TT_DATA,
                            TT_COEFF,
                            TP_FIR_LEN,
                            TP_DECIMATE_FACTOR,
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
                            USE_WINDOW_API,
                            TP_SAT>() {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fir_decimate_sym::filter); }

    // FIR
    void filter(input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<fnFirMargin<TP_FIR_LEN, TT_DATA>()> >&
                    inWindow,
                output_circular_buffer<TT_DATA>& outWindow,
                output_circular_buffer<TT_DATA>& outWindow2,
                const TT_COEFF (&inTaps)[(TP_FIR_LEN + 1) / kSymmetryFactor]);
};

// Single kernel specialization. No cascade ports, Windowed. dual input, no reload, single output
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_DECIMATE_FACTOR,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_SAT>
class fir_decimate_sym<TT_DATA,
                       TT_COEFF,
                       TP_FIR_LEN,
                       TP_DECIMATE_FACTOR,
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
                       USE_WINDOW_API,
                       TP_SAT> : public kernelFilterClass<TT_DATA,
                                                          TT_COEFF,
                                                          TP_FIR_LEN,
                                                          TP_DECIMATE_FACTOR,
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
                                                          USE_WINDOW_API,
                                                          TP_SAT> {
   private:
   public:
    // Constructor
    fir_decimate_sym(const TT_COEFF (&taps)[(TP_FIR_LEN + 1) / kSymmetryFactor])
        : kernelFilterClass<TT_DATA,
                            TT_COEFF,
                            TP_FIR_LEN,
                            TP_DECIMATE_FACTOR,
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
                            USE_WINDOW_API,
                            TP_SAT>(taps) {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fir_decimate_sym::filter); }

    // FIR
    void filter(input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<fnFirMargin<TP_FIR_LEN, TT_DATA>()> >&
                    inWindow,
                input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<fnFirMargin<TP_FIR_LEN, TT_DATA>()> >&
                    inWindowReverse,
                output_circular_buffer<TT_DATA>& outWindow);
};

// Single kernel specialization. No cascade ports, Windowed. dual input, no reload, dual output
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_DECIMATE_FACTOR,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_SAT>
class fir_decimate_sym<TT_DATA,
                       TT_COEFF,
                       TP_FIR_LEN,
                       TP_DECIMATE_FACTOR,
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
                       USE_WINDOW_API,
                       TP_SAT> : public kernelFilterClass<TT_DATA,
                                                          TT_COEFF,
                                                          TP_FIR_LEN,
                                                          TP_DECIMATE_FACTOR,
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
                                                          USE_WINDOW_API,
                                                          TP_SAT> {
   private:
   public:
    // Constructor
    fir_decimate_sym(const TT_COEFF (&taps)[(TP_FIR_LEN + 1) / kSymmetryFactor])
        : kernelFilterClass<TT_DATA,
                            TT_COEFF,
                            TP_FIR_LEN,
                            TP_DECIMATE_FACTOR,
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
                            USE_WINDOW_API,
                            TP_SAT>(taps) {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fir_decimate_sym::filter); }

    // FIR
    void filter(input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<fnFirMargin<TP_FIR_LEN, TT_DATA>()> >&
                    inWindow,
                input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<fnFirMargin<TP_FIR_LEN, TT_DATA>()> >&
                    inWindowReverse,
                output_circular_buffer<TT_DATA>& outWindow,
                output_circular_buffer<TT_DATA>& outWindow2);
};

//-----------------------------------------------------------------------------------------------------
// Single kernel specialization. No cascade ports, Windowed. dual input, with reload, single output
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_DECIMATE_FACTOR,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_SAT>
class fir_decimate_sym<TT_DATA,
                       TT_COEFF,
                       TP_FIR_LEN,
                       TP_DECIMATE_FACTOR,
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
                       USE_WINDOW_API,
                       TP_SAT> : public kernelFilterClass<TT_DATA,
                                                          TT_COEFF,
                                                          TP_FIR_LEN,
                                                          TP_DECIMATE_FACTOR,
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
                                                          USE_WINDOW_API,
                                                          TP_SAT> {
   private:
   public:
    // Constructor
    fir_decimate_sym()
        : kernelFilterClass<TT_DATA,
                            TT_COEFF,
                            TP_FIR_LEN,
                            TP_DECIMATE_FACTOR,
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
                            USE_WINDOW_API,
                            TP_SAT>() {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fir_decimate_sym::filter); }

    // FIR
    void filter(input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<fnFirMargin<TP_FIR_LEN, TT_DATA>()> >&
                    inWindow,
                input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<fnFirMargin<TP_FIR_LEN, TT_DATA>()> >&
                    inWindowReverse,
                output_circular_buffer<TT_DATA>& outWindow,
                const TT_COEFF (&inTaps)[(TP_FIR_LEN + 1) / kSymmetryFactor]);
};

//-----------------------------------------------------------------------------------------------------
// Single kernel specialization. No cascade ports, Windowed. dual input, with reload, dual output
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_DECIMATE_FACTOR,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_SAT>
class fir_decimate_sym<TT_DATA,
                       TT_COEFF,
                       TP_FIR_LEN,
                       TP_DECIMATE_FACTOR,
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
                       USE_WINDOW_API,
                       TP_SAT> : public kernelFilterClass<TT_DATA,
                                                          TT_COEFF,
                                                          TP_FIR_LEN,
                                                          TP_DECIMATE_FACTOR,
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
                                                          USE_WINDOW_API,
                                                          TP_SAT> {
   private:
   public:
    // Constructor
    fir_decimate_sym()
        : kernelFilterClass<TT_DATA,
                            TT_COEFF,
                            TP_FIR_LEN,
                            TP_DECIMATE_FACTOR,
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
                            USE_WINDOW_API,
                            TP_SAT>() {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fir_decimate_sym::filter); }

    // FIR
    void filter(input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<fnFirMargin<TP_FIR_LEN, TT_DATA>()> >&
                    inWindow,
                input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<fnFirMargin<TP_FIR_LEN, TT_DATA>()> >&
                    inWindowReverse,
                output_circular_buffer<TT_DATA>& outWindow,
                output_circular_buffer<TT_DATA>& outWindow2,
                const TT_COEFF (&inTaps)[(TP_FIR_LEN + 1) / kSymmetryFactor]);
};

//-----------------------------------------------------------------------------------------------------
// Partially specialized classes for cascaded interface (final kernel in cascade), Windowed. single input, no reload,
// single output
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
          unsigned int TP_DUAL_IP,
          unsigned int TP_SAT>
class fir_decimate_sym<TT_DATA,
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
                       TP_DUAL_IP,
                       USE_COEFF_RELOAD_FALSE,
                       1,
                       USE_WINDOW_API,
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
                                                          TP_DUAL_IP,
                                                          USE_COEFF_RELOAD_FALSE,
                                                          1,
                                                          USE_WINDOW_API,
                                                          TP_SAT> {
   private:
   public:
    // Constructor
    fir_decimate_sym(const TT_COEFF (&taps)[(TP_FIR_LEN + 1) / kSymmetryFactor])
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
                            TP_DUAL_IP,
                            USE_COEFF_RELOAD_FALSE,
                            1,
                            USE_WINDOW_API,
                            TP_SAT>(taps) {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fir_decimate_sym::filter); }

    // FIR
    void filter(input_async_buffer<TT_DATA>& inWindow,
                input_stream_cacc48* inCascade,
                output_circular_buffer<TT_DATA>& __restrict outWindow);
};

// Partially specialized classes for cascaded interface (final kernel in cascade), Windowed. single input, no reload,
// dual output
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
          unsigned int TP_DUAL_IP,
          unsigned int TP_SAT>
class fir_decimate_sym<TT_DATA,
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
                       TP_DUAL_IP,
                       USE_COEFF_RELOAD_FALSE,
                       2,
                       USE_WINDOW_API,
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
                                                          TP_DUAL_IP,
                                                          USE_COEFF_RELOAD_FALSE,
                                                          2,
                                                          USE_WINDOW_API,
                                                          TP_SAT> {
   private:
   public:
    // Constructor
    fir_decimate_sym(const TT_COEFF (&taps)[(TP_FIR_LEN + 1) / kSymmetryFactor])
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
                            TP_DUAL_IP,
                            USE_COEFF_RELOAD_FALSE,
                            2,
                            USE_WINDOW_API,
                            TP_SAT>(taps) {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fir_decimate_sym::filter); }

    // FIR
    void filter(input_async_buffer<TT_DATA>& inWindow,
                input_stream_cacc48* inCascade,
                output_circular_buffer<TT_DATA>& __restrict outWindow,
                output_circular_buffer<TT_DATA>& __restrict outWindow2);
};

/*
// Partially specialized classes for cascaded interface (final kernel in cascade), dual input, no reload, single output
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
unsigned int TP_SAT>
class fir_decimate_sym<TT_DATA, TT_COEFF, TP_FIR_LEN, TP_DECIMATE_FACTOR, TP_SHIFT, TP_RND, TP_INPUT_WINDOW_VSIZE,
            CASC_IN_TRUE, CASC_OUT_FALSE, TP_FIR_RANGE_LEN, TP_KERNEL_POSITION, TP_CASC_LEN, DUAL_IP_DUAL,
USE_COEFF_RELOAD_FALSE, 1, USE_WINDOW_API, TP_SAT> :
public kernelFilterClass<TT_DATA, TT_COEFF, TP_FIR_LEN, TP_DECIMATE_FACTOR, TP_SHIFT, TP_RND, TP_INPUT_WINDOW_VSIZE,
            CASC_IN_TRUE, CASC_OUT_FALSE, TP_FIR_RANGE_LEN, TP_KERNEL_POSITION, TP_CASC_LEN, DUAL_IP_DUAL,
USE_COEFF_RELOAD_FALSE, 1, USE_WINDOW_API, TP_SAT>
{
private:
public:
// Constructor
fir_decimate_sym(const TT_COEFF (&taps)[(TP_FIR_LEN+1)/kSymmetryFactor]) :
kernelFilterClass<TT_DATA, TT_COEFF, TP_FIR_LEN, TP_DECIMATE_FACTOR, TP_SHIFT, TP_RND, TP_INPUT_WINDOW_VSIZE,
            CASC_IN_TRUE, CASC_OUT_FALSE, TP_FIR_RANGE_LEN, TP_KERNEL_POSITION, TP_CASC_LEN, DUAL_IP_DUAL,
USE_COEFF_RELOAD_FALSE, 1, USE_WINDOW_API, TP_SAT>
            (taps)
{
}

// Register Kernel Class
static void registerKernelClass()
{
REGISTER_FUNCTION(fir_decimate_sym::filter);
}

// FIR
void filter(input_window<TT_DATA>  *inWindow,
      input_window<TT_DATA>  *inWindowReverse,
      input_stream_cacc48    *inCascade,
      output_window<TT_DATA>* __restrict outWindow);
};

// Partially specialized classes for cascaded interface (final kernel in cascade), dual input, no reload, dual output
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
unsigned int TP_SAT>
class fir_decimate_sym<TT_DATA, TT_COEFF, TP_FIR_LEN, TP_DECIMATE_FACTOR, TP_SHIFT, TP_RND, TP_INPUT_WINDOW_VSIZE,
            CASC_IN_TRUE, CASC_OUT_FALSE, TP_FIR_RANGE_LEN, TP_KERNEL_POSITION, TP_CASC_LEN, DUAL_IP_DUAL,
USE_COEFF_RELOAD_FALSE, 2, USE_WINDOW_API, TP_SAT> :
public kernelFilterClass<TT_DATA, TT_COEFF, TP_FIR_LEN, TP_DECIMATE_FACTOR, TP_SHIFT, TP_RND, TP_INPUT_WINDOW_VSIZE,
            CASC_IN_TRUE, CASC_OUT_FALSE, TP_FIR_RANGE_LEN, TP_KERNEL_POSITION, TP_CASC_LEN, DUAL_IP_DUAL,
USE_COEFF_RELOAD_FALSE, 2, USE_WINDOW_API, TP_SAT>
{
private:
public:
// Constructor
fir_decimate_sym(const TT_COEFF (&taps)[(TP_FIR_LEN+1)/kSymmetryFactor]) :
kernelFilterClass<TT_DATA, TT_COEFF, TP_FIR_LEN, TP_DECIMATE_FACTOR, TP_SHIFT, TP_RND, TP_INPUT_WINDOW_VSIZE,
            CASC_IN_TRUE, CASC_OUT_FALSE, TP_FIR_RANGE_LEN, TP_KERNEL_POSITION, TP_CASC_LEN, DUAL_IP_DUAL,
USE_COEFF_RELOAD_FALSE, 2, USE_WINDOW_API, TP_SAT>
            (taps)
{
}

// Register Kernel Class
static void registerKernelClass()
{
REGISTER_FUNCTION(fir_decimate_sym::filter);
}

// FIR
void filter(input_window<TT_DATA>  *inWindow,
      input_window<TT_DATA>  *inWindowReverse,
      input_stream_cacc48    *inCascade,
      output_window<TT_DATA>* __restrict outWindow,
      output_window<TT_DATA>* __restrict outWindow2);
};
*/

// Partially specialized classes for cascaded interface (final kernel in cascade), Windowed. single/dual input, with
// reload, single output
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
          unsigned int TP_DUAL_IP,
          unsigned int TP_SAT>
class fir_decimate_sym<TT_DATA,
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
                       TP_DUAL_IP,
                       USE_COEFF_RELOAD_TRUE,
                       1,
                       USE_WINDOW_API,
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
                                                          TP_DUAL_IP,
                                                          USE_COEFF_RELOAD_TRUE,
                                                          1,
                                                          USE_WINDOW_API,
                                                          TP_SAT> {
   private:
   public:
    // Constructor
    fir_decimate_sym()
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
                            TP_DUAL_IP,
                            USE_COEFF_RELOAD_TRUE,
                            1,
                            USE_WINDOW_API,
                            TP_SAT>() {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fir_decimate_sym::filter); }

    // FIR
    void filter(input_async_buffer<TT_DATA>& inWindow,
                input_stream_cacc48* inCascade,
                output_circular_buffer<TT_DATA>& __restrict outWindow);
};

// Partially specialized classes for cascaded interface (final kernel in cascade), Windowed. single/dual input, with
// reload, dual output
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
          unsigned int TP_DUAL_IP,
          unsigned int TP_SAT>
class fir_decimate_sym<TT_DATA,
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
                       TP_DUAL_IP,
                       USE_COEFF_RELOAD_TRUE,
                       2,
                       USE_WINDOW_API,
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
                                                          TP_DUAL_IP,
                                                          USE_COEFF_RELOAD_TRUE,
                                                          2,
                                                          USE_WINDOW_API,
                                                          TP_SAT> {
   private:
   public:
    // Constructor
    fir_decimate_sym()
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
                            TP_DUAL_IP,
                            USE_COEFF_RELOAD_TRUE,
                            2,
                            USE_WINDOW_API,
                            TP_SAT>() {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fir_decimate_sym::filter); }

    // FIR
    void filter(input_async_buffer<TT_DATA>& inWindow,
                input_stream_cacc48* inCascade,
                output_circular_buffer<TT_DATA>& __restrict outWindow,
                output_circular_buffer<TT_DATA>& __restrict outWindow2);
};

/*
// Partially specialized classes for cascaded interface (final kernel in cascade), dual input, with reload, single
output
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
unsigned int TP_SAT>
class fir_decimate_sym<TT_DATA, TT_COEFF, TP_FIR_LEN, TP_DECIMATE_FACTOR, TP_SHIFT, TP_RND, TP_INPUT_WINDOW_VSIZE,
             CASC_IN_TRUE, CASC_OUT_FALSE, TP_FIR_RANGE_LEN, TP_KERNEL_POSITION, TP_CASC_LEN, DUAL_IP_DUAL,
USE_COEFF_RELOAD_TRUE, 1, USE_WINDOW_API, TP_SAT> :
public kernelFilterClass<TT_DATA, TT_COEFF, TP_FIR_LEN, TP_DECIMATE_FACTOR, TP_SHIFT, TP_RND, TP_INPUT_WINDOW_VSIZE,
                   CASC_IN_TRUE, CASC_OUT_FALSE, TP_FIR_RANGE_LEN, TP_KERNEL_POSITION, TP_CASC_LEN, DUAL_IP_DUAL,
USE_COEFF_RELOAD_TRUE, 1, USE_WINDOW_API, TP_SAT>
{
private:
public:
// Constructor
fir_decimate_sym() :
kernelFilterClass<TT_DATA, TT_COEFF, TP_FIR_LEN, TP_DECIMATE_FACTOR, TP_SHIFT, TP_RND, TP_INPUT_WINDOW_VSIZE,
            CASC_IN_TRUE, CASC_OUT_FALSE, TP_FIR_RANGE_LEN, TP_KERNEL_POSITION, TP_CASC_LEN, DUAL_IP_DUAL,
USE_COEFF_RELOAD_TRUE, 1, USE_WINDOW_API, TP_SAT>()
{
}

// Register Kernel Class
static void registerKernelClass()
{
REGISTER_FUNCTION(fir_decimate_sym::filter);
}

// FIR
void filter(input_window<TT_DATA>  *inWindow,
      input_window<TT_DATA>  *inWindowReverse,
      input_stream_cacc48    *inCascade,
      output_window<TT_DATA>* __restrict outWindow);
};

// Partially specialized classes for cascaded interface (final kernel in cascade), dual input, with reload, dual output
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
unsigned int TP_SAT>
class fir_decimate_sym<TT_DATA, TT_COEFF, TP_FIR_LEN, TP_DECIMATE_FACTOR, TP_SHIFT, TP_RND, TP_INPUT_WINDOW_VSIZE,
             CASC_IN_TRUE, CASC_OUT_FALSE, TP_FIR_RANGE_LEN, TP_KERNEL_POSITION, TP_CASC_LEN, DUAL_IP_DUAL,
USE_COEFF_RELOAD_TRUE, 2, USE_WINDOW_API, TP_SAT> :
public kernelFilterClass<TT_DATA, TT_COEFF, TP_FIR_LEN, TP_DECIMATE_FACTOR, TP_SHIFT, TP_RND, TP_INPUT_WINDOW_VSIZE,
                   CASC_IN_TRUE, CASC_OUT_FALSE, TP_FIR_RANGE_LEN, TP_KERNEL_POSITION, TP_CASC_LEN, DUAL_IP_DUAL,
USE_COEFF_RELOAD_TRUE, 2, USE_WINDOW_API, TP_SAT>
{
private:
public:
// Constructor
fir_decimate_sym() :
kernelFilterClass<TT_DATA, TT_COEFF, TP_FIR_LEN, TP_DECIMATE_FACTOR, TP_SHIFT, TP_RND, TP_INPUT_WINDOW_VSIZE,
            CASC_IN_TRUE, CASC_OUT_FALSE, TP_FIR_RANGE_LEN, TP_KERNEL_POSITION, TP_CASC_LEN, DUAL_IP_DUAL,
USE_COEFF_RELOAD_TRUE, 2, USE_WINDOW_API, TP_SAT>()
{
}

// Register Kernel Class
static void registerKernelClass()
{
REGISTER_FUNCTION(fir_decimate_sym::filter);
}

// FIR
void filter(input_window<TT_DATA>  *inWindow,
      input_window<TT_DATA>  *inWindowReverse,
      input_stream_cacc48    *inCascade,
      output_window<TT_DATA>* __restrict outWindow,
      output_window<TT_DATA>* __restrict outWindow2);
};
*/

//-----------------------------------------------------------------------------------------------------
// Partially specialized classes for cascaded interface (First kernel in cascade), Windowed. single input, no reload
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
          unsigned int TP_NUM_OUTPUTS,
          unsigned int TP_SAT>
class fir_decimate_sym<TT_DATA,
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
                       DUAL_IP_SINGLE,
                       USE_COEFF_RELOAD_FALSE,
                       TP_NUM_OUTPUTS,
                       USE_WINDOW_API,
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
                                                          DUAL_IP_SINGLE,
                                                          USE_COEFF_RELOAD_FALSE,
                                                          TP_NUM_OUTPUTS,
                                                          USE_WINDOW_API,
                                                          TP_SAT> {
   private:
   public:
    // Constructor
    fir_decimate_sym(const TT_COEFF (&taps)[(TP_FIR_LEN + 1) / kSymmetryFactor])
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
                            DUAL_IP_SINGLE,
                            USE_COEFF_RELOAD_FALSE,
                            TP_NUM_OUTPUTS,
                            USE_WINDOW_API,
                            TP_SAT>(taps) {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fir_decimate_sym::filter); }

    // FIR
    void filter(input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<fnFirMargin<TP_FIR_LEN, TT_DATA>()> >&
                    inWindow,
                output_stream_cacc48* outCascade,
                output_async_buffer<TT_DATA>& broadcastWindow);
};

// Partially specialized classes for cascaded interface (First kernel in cascade), Windowed. dual input, no reload
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
          unsigned int TP_NUM_OUTPUTS,
          unsigned int TP_SAT>
class fir_decimate_sym<TT_DATA,
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
                       DUAL_IP_DUAL,
                       USE_COEFF_RELOAD_FALSE,
                       TP_NUM_OUTPUTS,
                       USE_WINDOW_API,
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
                                                          DUAL_IP_DUAL,
                                                          USE_COEFF_RELOAD_FALSE,
                                                          TP_NUM_OUTPUTS,
                                                          USE_WINDOW_API,
                                                          TP_SAT> {
   private:
   public:
    // Constructor
    fir_decimate_sym(const TT_COEFF (&taps)[(TP_FIR_LEN + 1) / kSymmetryFactor])
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
                            DUAL_IP_DUAL,
                            USE_COEFF_RELOAD_FALSE,
                            TP_NUM_OUTPUTS,
                            USE_WINDOW_API,
                            TP_SAT>(taps) {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fir_decimate_sym::filter); }

    // FIR
    void filter(input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<fnFirMargin<TP_FIR_LEN, TT_DATA>()> >&
                    inWindow,
                input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<fnFirMargin<TP_FIR_LEN, TT_DATA>()> >&
                    inWindowReverse,
                output_stream_cacc48* outCascade,
                output_async_buffer<TT_DATA>& broadcastWindow);
};

// Partially specialized classes for cascaded interface (First kernel in cascade), Windowed. single input, with reload
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
          unsigned int TP_NUM_OUTPUTS,
          unsigned int TP_SAT>
class fir_decimate_sym<TT_DATA,
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
                       DUAL_IP_SINGLE,
                       USE_COEFF_RELOAD_TRUE,
                       TP_NUM_OUTPUTS,
                       USE_WINDOW_API,
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
                                                          DUAL_IP_SINGLE,
                                                          USE_COEFF_RELOAD_TRUE,
                                                          TP_NUM_OUTPUTS,
                                                          USE_WINDOW_API,
                                                          TP_SAT> {
   private:
   public:
    // Constructor
    fir_decimate_sym()
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
                            DUAL_IP_SINGLE,
                            USE_COEFF_RELOAD_TRUE,
                            TP_NUM_OUTPUTS,
                            USE_WINDOW_API,
                            TP_SAT>() {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fir_decimate_sym::filter); }

    // FIR
    void filter(input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<fnFirMargin<TP_FIR_LEN, TT_DATA>()> >&
                    inWindow,
                output_stream_cacc48* outCascade,
                output_async_buffer<TT_DATA>& broadcastWindow,
                const TT_COEFF (&inTaps)[(TP_FIR_LEN + 1) / kSymmetryFactor]);
};

// Partially specialized classes for cascaded interface (First kernel in cascade), Windowed. dual input, with reload
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
          unsigned int TP_NUM_OUTPUTS,
          unsigned int TP_SAT>
class fir_decimate_sym<TT_DATA,
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
                       DUAL_IP_DUAL,
                       USE_COEFF_RELOAD_TRUE,
                       TP_NUM_OUTPUTS,
                       USE_WINDOW_API,
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
                                                          DUAL_IP_DUAL,
                                                          USE_COEFF_RELOAD_TRUE,
                                                          TP_NUM_OUTPUTS,
                                                          USE_WINDOW_API,
                                                          TP_SAT> {
   private:
   public:
    // Constructor
    fir_decimate_sym()
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
                            DUAL_IP_DUAL,
                            USE_COEFF_RELOAD_TRUE,
                            TP_NUM_OUTPUTS,
                            USE_WINDOW_API,
                            TP_SAT>() {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fir_decimate_sym::filter); }

    // FIR
    void filter(input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<fnFirMargin<TP_FIR_LEN, TT_DATA>()> >&
                    inWindow,
                input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<fnFirMargin<TP_FIR_LEN, TT_DATA>()> >&
                    inWindowReverse,
                output_stream_cacc48* outCascade,
                output_async_buffer<TT_DATA>& broadcastWindow,
                const TT_COEFF (&inTaps)[(TP_FIR_LEN + 1) / kSymmetryFactor]);
};

//-----------------------------------------------------------------------------------------------------
// Partially specialized classes for cascaded interface (middle kernels in cascade), Windowed. single/dual  input, no
// reload
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
          unsigned int TP_DUAL_IP,
          unsigned int TP_NUM_OUTPUTS,
          unsigned int TP_SAT>
class fir_decimate_sym<TT_DATA,
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
                       TP_DUAL_IP,
                       USE_COEFF_RELOAD_FALSE,
                       TP_NUM_OUTPUTS,
                       USE_WINDOW_API,
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
                                                          TP_DUAL_IP,
                                                          USE_COEFF_RELOAD_FALSE,
                                                          TP_NUM_OUTPUTS,
                                                          USE_WINDOW_API,
                                                          TP_SAT> {
   private:
   public:
    // Constructor
    fir_decimate_sym(const TT_COEFF (&taps)[(TP_FIR_LEN + 1) / kSymmetryFactor])
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
                            TP_DUAL_IP,
                            USE_COEFF_RELOAD_FALSE,
                            TP_NUM_OUTPUTS,
                            USE_WINDOW_API,
                            TP_SAT>(taps) {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fir_decimate_sym::filter); }

    // FIR
    void filter(input_async_buffer<TT_DATA>& inWindow,
                input_stream_cacc48* inCascade,
                output_stream_cacc48* outCascade,
                output_async_buffer<TT_DATA>& broadcastWindow);
};

/*
// Partially specialized classes for cascaded interface (middle kernels in cascade), dual input, no reload
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
unsigned int TP_NUM_OUTPUTS,
unsigned int TP_SAT>
class fir_decimate_sym<TT_DATA, TT_COEFF, TP_FIR_LEN, TP_DECIMATE_FACTOR, TP_SHIFT, TP_RND, TP_INPUT_WINDOW_VSIZE,
            CASC_IN_TRUE, CASC_OUT_TRUE, TP_FIR_RANGE_LEN, TP_KERNEL_POSITION, TP_CASC_LEN, DUAL_IP_DUAL,
USE_COEFF_RELOAD_FALSE, TP_NUM_OUTPUTS, USE_WINDOW_API, TP_SAT> :
public kernelFilterClass<TT_DATA, TT_COEFF, TP_FIR_LEN, TP_DECIMATE_FACTOR, TP_SHIFT, TP_RND, TP_INPUT_WINDOW_VSIZE,
            CASC_IN_TRUE, CASC_OUT_TRUE, TP_FIR_RANGE_LEN, TP_KERNEL_POSITION, TP_CASC_LEN, DUAL_IP_DUAL,
USE_COEFF_RELOAD_FALSE, TP_NUM_OUTPUTS, USE_WINDOW_API, TP_SAT>
{
private:
public:
// Constructor
fir_decimate_sym(const TT_COEFF (&taps)[(TP_FIR_LEN+1)/kSymmetryFactor]):
kernelFilterClass<TT_DATA, TT_COEFF, TP_FIR_LEN, TP_DECIMATE_FACTOR, TP_SHIFT, TP_RND, TP_INPUT_WINDOW_VSIZE,
            CASC_IN_TRUE, CASC_OUT_TRUE, TP_FIR_RANGE_LEN, TP_KERNEL_POSITION, TP_CASC_LEN, DUAL_IP_DUAL,
USE_COEFF_RELOAD_FALSE, TP_NUM_OUTPUTS, USE_WINDOW_API, TP_SAT>
            (taps)
{
}

// Register Kernel Class
static void registerKernelClass()
{
REGISTER_FUNCTION(fir_decimate_sym::filter);
}

// FIR
void filter(input_window<TT_DATA> *inWindow,
      input_window<TT_DATA> *inWindowReverse,
      input_stream_cacc48   *inCascade,
      output_stream_cacc48  *outCascade,
      output_window<TT_DATA> *broadcastWindow);
};
*/

// Partially specialized classes for cascaded interface (middle kernels in cascade), single/dual input, with reload
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
          unsigned int TP_DUAL_IP,
          unsigned int TP_NUM_OUTPUTS,
          unsigned int TP_SAT>
class fir_decimate_sym<TT_DATA,
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
                       TP_DUAL_IP,
                       USE_COEFF_RELOAD_TRUE,
                       TP_NUM_OUTPUTS,
                       USE_WINDOW_API,
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
                                                          TP_DUAL_IP,
                                                          USE_COEFF_RELOAD_TRUE,
                                                          TP_NUM_OUTPUTS,
                                                          USE_WINDOW_API,
                                                          TP_SAT> {
   private:
   public:
    // Constructor
    fir_decimate_sym()
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
                            TP_DUAL_IP,
                            USE_COEFF_RELOAD_TRUE,
                            TP_NUM_OUTPUTS,
                            USE_WINDOW_API,
                            TP_SAT>() {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fir_decimate_sym::filter); }

    // FIR
    void filter(input_async_buffer<TT_DATA>& inWindow,
                input_stream_cacc48* inCascade,
                output_stream_cacc48* outCascade,
                output_async_buffer<TT_DATA>& broadcastWindow);
};

/*
// Partially specialized classes for cascaded interface (middle kernels in cascade), dual input, with reload
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
unsigned int TP_NUM_OUTPUTS,
unsigned int TP_SAT>
class fir_decimate_sym<TT_DATA, TT_COEFF, TP_FIR_LEN, TP_DECIMATE_FACTOR, TP_SHIFT, TP_RND, TP_INPUT_WINDOW_VSIZE,
            CASC_IN_TRUE, CASC_OUT_TRUE, TP_FIR_RANGE_LEN, TP_KERNEL_POSITION, TP_CASC_LEN, DUAL_IP_DUAL,
USE_COEFF_RELOAD_TRUE, TP_NUM_OUTPUTS, USE_WINDOW_API, TP_SAT> :
public kernelFilterClass<TT_DATA, TT_COEFF, TP_FIR_LEN, TP_DECIMATE_FACTOR, TP_SHIFT, TP_RND, TP_INPUT_WINDOW_VSIZE,
            CASC_IN_TRUE, CASC_OUT_TRUE, TP_FIR_RANGE_LEN, TP_KERNEL_POSITION, TP_CASC_LEN, DUAL_IP_DUAL,
USE_COEFF_RELOAD_TRUE, TP_NUM_OUTPUTS, USE_WINDOW_API, TP_SAT>
{
private:
public:
// Constructor
fir_decimate_sym():
kernelFilterClass<TT_DATA, TT_COEFF, TP_FIR_LEN, TP_DECIMATE_FACTOR, TP_SHIFT, TP_RND, TP_INPUT_WINDOW_VSIZE,
            CASC_IN_TRUE, CASC_OUT_TRUE, TP_FIR_RANGE_LEN, TP_KERNEL_POSITION, TP_CASC_LEN, DUAL_IP_DUAL,
USE_COEFF_RELOAD_TRUE, TP_NUM_OUTPUTS, USE_WINDOW_API, TP_SAT>()
{
}

// Register Kernel Class
static void registerKernelClass()
{
REGISTER_FUNCTION(fir_decimate_sym::filter);
}

// FIR
void filter(input_window<TT_DATA> *inWindow,
      input_window<TT_DATA> *inWindowReverse,
      input_stream_cacc48   *inCascade,
      output_stream_cacc48  *outCascade,
      output_window<TT_DATA> *broadcastWindow);
};
*/

// ----------------------------------------------------------------------------
// ---------------------------------- STREAM ----------------------------------
// ----------------------------------------------------------------------------

// Single kernel specialization. No cascade ports. Streaming. Static coefficients. Single Output
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_DECIMATE_FACTOR,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_DUAL_IP,
          unsigned int TP_SAT>
class fir_decimate_sym<TT_DATA,
                       TT_COEFF,
                       TP_FIR_LEN,
                       TP_DECIMATE_FACTOR,
                       TP_SHIFT,
                       TP_RND,
                       TP_INPUT_WINDOW_VSIZE,
                       CASC_IN_FALSE,
                       CASC_OUT_FALSE,
                       TP_FIR_LEN,
                       0,
                       1,
                       TP_DUAL_IP,
                       USE_COEFF_RELOAD_FALSE,
                       1,
                       USE_STREAM_API,
                       TP_SAT> : public kernelFilterClass<TT_DATA,
                                                          TT_COEFF,
                                                          TP_FIR_LEN,
                                                          TP_DECIMATE_FACTOR,
                                                          TP_SHIFT,
                                                          TP_RND,
                                                          TP_INPUT_WINDOW_VSIZE,
                                                          CASC_IN_FALSE,
                                                          CASC_OUT_FALSE,
                                                          TP_FIR_LEN,
                                                          0,
                                                          1,
                                                          TP_DUAL_IP,
                                                          USE_COEFF_RELOAD_FALSE,
                                                          1,
                                                          USE_STREAM_API,
                                                          TP_SAT> {
   private:
   public:
    // Constructor
    fir_decimate_sym(const TT_COEFF (&taps)[(TP_FIR_LEN + 1) / kSymmetryFactor])
        : kernelFilterClass<TT_DATA,
                            TT_COEFF,
                            TP_FIR_LEN,
                            TP_DECIMATE_FACTOR,
                            TP_SHIFT,
                            TP_RND,
                            TP_INPUT_WINDOW_VSIZE,
                            CASC_IN_FALSE,
                            CASC_OUT_FALSE,
                            TP_FIR_LEN,
                            0,
                            1,
                            TP_DUAL_IP,
                            USE_COEFF_RELOAD_FALSE,
                            1,
                            USE_STREAM_API,
                            TP_SAT>(taps) {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fir_decimate_sym::filter); }

    // FIR
    void filter(input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<fnFirMargin<TP_FIR_LEN, TT_DATA>()> >&
                    inWindow,
                output_stream<TT_DATA>* restrict outStream);
};

// Single kernel specialization. No cascade ports. Streaming. Static coefficients. Dual Output
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_DECIMATE_FACTOR,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_DUAL_IP,
          unsigned int TP_SAT>
class fir_decimate_sym<TT_DATA,
                       TT_COEFF,
                       TP_FIR_LEN,
                       TP_DECIMATE_FACTOR,
                       TP_SHIFT,
                       TP_RND,
                       TP_INPUT_WINDOW_VSIZE,
                       CASC_IN_FALSE,
                       CASC_OUT_FALSE,
                       TP_FIR_LEN,
                       0,
                       1,
                       TP_DUAL_IP,
                       USE_COEFF_RELOAD_FALSE,
                       2,
                       USE_STREAM_API,
                       TP_SAT> : public kernelFilterClass<TT_DATA,
                                                          TT_COEFF,
                                                          TP_FIR_LEN,
                                                          TP_DECIMATE_FACTOR,
                                                          TP_SHIFT,
                                                          TP_RND,
                                                          TP_INPUT_WINDOW_VSIZE,
                                                          CASC_IN_FALSE,
                                                          CASC_OUT_FALSE,
                                                          TP_FIR_LEN,
                                                          0,
                                                          1,
                                                          TP_DUAL_IP,
                                                          USE_COEFF_RELOAD_FALSE,
                                                          2,
                                                          USE_STREAM_API,
                                                          TP_SAT> {
   private:
   public:
    // Constructor
    fir_decimate_sym(const TT_COEFF (&taps)[(TP_FIR_LEN + 1) / kSymmetryFactor])
        : kernelFilterClass<TT_DATA,
                            TT_COEFF,
                            TP_FIR_LEN,
                            TP_DECIMATE_FACTOR,
                            TP_SHIFT,
                            TP_RND,
                            TP_INPUT_WINDOW_VSIZE,
                            CASC_IN_FALSE,
                            CASC_OUT_FALSE,
                            TP_FIR_LEN,
                            0,
                            1,
                            TP_DUAL_IP,
                            USE_COEFF_RELOAD_FALSE,
                            2,
                            USE_STREAM_API,
                            TP_SAT>(taps) {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fir_decimate_sym::filter); }

    // FIR
    void filter(input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<fnFirMargin<TP_FIR_LEN, TT_DATA>()> >&
                    inWindow,
                output_stream<TT_DATA>* restrict outStream,
                output_stream<TT_DATA>* restrict outStream2);
};

// Single kernel specialization. No cascade ports. Streaming. Reloadable coefficients. Single Output
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_DECIMATE_FACTOR,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_DUAL_IP,
          unsigned int TP_SAT>
class fir_decimate_sym<TT_DATA,
                       TT_COEFF,
                       TP_FIR_LEN,
                       TP_DECIMATE_FACTOR,
                       TP_SHIFT,
                       TP_RND,
                       TP_INPUT_WINDOW_VSIZE,
                       CASC_IN_FALSE,
                       CASC_OUT_FALSE,
                       TP_FIR_LEN,
                       0,
                       1,
                       TP_DUAL_IP,
                       USE_COEFF_RELOAD_TRUE,
                       1,
                       USE_STREAM_API,
                       TP_SAT> : public kernelFilterClass<TT_DATA,
                                                          TT_COEFF,
                                                          TP_FIR_LEN,
                                                          TP_DECIMATE_FACTOR,
                                                          TP_SHIFT,
                                                          TP_RND,
                                                          TP_INPUT_WINDOW_VSIZE,
                                                          CASC_IN_FALSE,
                                                          CASC_OUT_FALSE,
                                                          TP_FIR_LEN,
                                                          0,
                                                          1,
                                                          TP_DUAL_IP,
                                                          USE_COEFF_RELOAD_TRUE,
                                                          1,
                                                          USE_STREAM_API,
                                                          TP_SAT> {
   private:
   public:
    // Constructor
    fir_decimate_sym()
        : kernelFilterClass<TT_DATA,
                            TT_COEFF,
                            TP_FIR_LEN,
                            TP_DECIMATE_FACTOR,
                            TP_SHIFT,
                            TP_RND,
                            TP_INPUT_WINDOW_VSIZE,
                            CASC_IN_FALSE,
                            CASC_OUT_FALSE,
                            TP_FIR_LEN,
                            0,
                            1,
                            TP_DUAL_IP,
                            USE_COEFF_RELOAD_TRUE,
                            1,
                            USE_STREAM_API,
                            TP_SAT>() {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fir_decimate_sym::filter); }

    // FIR
    void filter(input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<fnFirMargin<TP_FIR_LEN, TT_DATA>()> >&
                    inWindow,
                output_stream<TT_DATA>* outStream,
                const TT_COEFF (&inTaps)[(TP_FIR_LEN + 1) / kSymmetryFactor]);
};

// Single kernel specialization. No cascade ports. Streaming. Reloadable coefficients. Dual Output
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_DECIMATE_FACTOR,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_DUAL_IP,
          unsigned int TP_SAT>
class fir_decimate_sym<TT_DATA,
                       TT_COEFF,
                       TP_FIR_LEN,
                       TP_DECIMATE_FACTOR,
                       TP_SHIFT,
                       TP_RND,
                       TP_INPUT_WINDOW_VSIZE,
                       CASC_IN_FALSE,
                       CASC_OUT_FALSE,
                       TP_FIR_LEN,
                       0,
                       1,
                       TP_DUAL_IP,
                       USE_COEFF_RELOAD_TRUE,
                       2,
                       USE_STREAM_API,
                       TP_SAT> : public kernelFilterClass<TT_DATA,
                                                          TT_COEFF,
                                                          TP_FIR_LEN,
                                                          TP_DECIMATE_FACTOR,
                                                          TP_SHIFT,
                                                          TP_RND,
                                                          TP_INPUT_WINDOW_VSIZE,
                                                          CASC_IN_FALSE,
                                                          CASC_OUT_FALSE,
                                                          TP_FIR_LEN,
                                                          0,
                                                          1,
                                                          TP_DUAL_IP,
                                                          USE_COEFF_RELOAD_TRUE,
                                                          2,
                                                          USE_STREAM_API,
                                                          TP_SAT> {
   private:
   public:
    // Constructor
    fir_decimate_sym()
        : kernelFilterClass<TT_DATA,
                            TT_COEFF,
                            TP_FIR_LEN,
                            TP_DECIMATE_FACTOR,
                            TP_SHIFT,
                            TP_RND,
                            TP_INPUT_WINDOW_VSIZE,
                            CASC_IN_FALSE,
                            CASC_OUT_FALSE,
                            TP_FIR_LEN,
                            0,
                            1,
                            TP_DUAL_IP,
                            USE_COEFF_RELOAD_TRUE,
                            2,
                            USE_STREAM_API,
                            TP_SAT>() {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fir_decimate_sym::filter); }

    // FIR
    void filter(input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<fnFirMargin<TP_FIR_LEN, TT_DATA>()> >&
                    inWindow,
                output_stream<TT_DATA>* outStream,
                output_stream<TT_DATA>* outStream2,
                const TT_COEFF (&inTaps)[(TP_FIR_LEN + 1) / kSymmetryFactor]);
};

//-----------------------------------------------------------------------------------------------------
// Cascaded Kernels - Final Kernel. Static coefficients. Streaming. Single Input. Single Output
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
          unsigned int TP_DUAL_IP,
          unsigned int TP_SAT>
class fir_decimate_sym<TT_DATA,
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
                       TP_DUAL_IP,
                       USE_COEFF_RELOAD_FALSE,
                       1,
                       USE_STREAM_API,
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
                                                          TP_DUAL_IP,
                                                          USE_COEFF_RELOAD_FALSE,
                                                          1,
                                                          USE_STREAM_API,
                                                          TP_SAT> {
   private:
   public:
    // Constructor
    fir_decimate_sym(const TT_COEFF (&taps)[(TP_FIR_LEN + 1) / kSymmetryFactor])
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
                            TP_DUAL_IP,
                            USE_COEFF_RELOAD_FALSE,
                            1,
                            USE_STREAM_API,
                            TP_SAT>(taps) {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fir_decimate_sym::filter); }

    // FIR
    void filter(input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<fnFirMargin<TP_FIR_LEN, TT_DATA>()> >&
                    inWindow,
                input_stream_cacc48* inCascade,
                output_stream<TT_DATA>* restrict outStream);
};

// Cascaded Kernels - Final Kernel. Static coefficients. Streaming. Dual Input. Single Output
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
          unsigned int TP_SAT>
class fir_decimate_sym<TT_DATA,
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
                       DUAL_IP_DUAL,
                       USE_COEFF_RELOAD_FALSE,
                       1,
                       USE_STREAM_API,
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
                                                          DUAL_IP_DUAL,
                                                          USE_COEFF_RELOAD_FALSE,
                                                          1,
                                                          USE_STREAM_API,
                                                          TP_SAT> {
   private:
   public:
    // Constructor
    fir_decimate_sym(const TT_COEFF (&taps)[(TP_FIR_LEN + 1) / kSymmetryFactor])
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
                            DUAL_IP_DUAL,
                            USE_COEFF_RELOAD_FALSE,
                            1,
                            USE_STREAM_API,
                            TP_SAT>(taps) {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fir_decimate_sym::filter); }

    // FIR
    void filter(input_async_buffer<TT_DATA>& inWindow,
                input_stream_cacc48* inCascade,
                output_stream<TT_DATA>* restrict outStream);
};

// Cascaded Kernels - Final Kernel. Static coefficients. Streaming. Single Input. Dual Output
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
          unsigned int TP_DUAL_IP,
          unsigned int TP_SAT>
class fir_decimate_sym<TT_DATA,
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
                       TP_DUAL_IP,
                       USE_COEFF_RELOAD_FALSE,
                       2,
                       USE_STREAM_API,
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
                                                          TP_DUAL_IP,
                                                          USE_COEFF_RELOAD_FALSE,
                                                          2,
                                                          USE_STREAM_API,
                                                          TP_SAT> {
   private:
   public:
    // Constructor
    fir_decimate_sym(const TT_COEFF (&taps)[(TP_FIR_LEN + 1) / kSymmetryFactor])
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
                            TP_DUAL_IP,
                            USE_COEFF_RELOAD_FALSE,
                            2,
                            USE_STREAM_API,
                            TP_SAT>(taps) {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fir_decimate_sym::filter); }

    // FIR
    void filter(input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<fnFirMargin<TP_FIR_LEN, TT_DATA>()> >&
                    inWindow,
                input_stream_cacc48* inCascade,
                output_stream<TT_DATA>* restrict outStrean,
                output_stream<TT_DATA>* restrict outStream2);
};

// Cascaded Kernels - Final Kernel. Static coefficients. Streaming. Dual Input. Dual Output
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
          unsigned int TP_SAT>
class fir_decimate_sym<TT_DATA,
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
                       DUAL_IP_DUAL,
                       USE_COEFF_RELOAD_FALSE,
                       2,
                       USE_STREAM_API,
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
                                                          DUAL_IP_DUAL,
                                                          USE_COEFF_RELOAD_FALSE,
                                                          2,
                                                          USE_STREAM_API,
                                                          TP_SAT> {
   private:
   public:
    // Constructor
    fir_decimate_sym(const TT_COEFF (&taps)[(TP_FIR_LEN + 1) / kSymmetryFactor])
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
                            DUAL_IP_DUAL,
                            USE_COEFF_RELOAD_FALSE,
                            2,
                            USE_STREAM_API,
                            TP_SAT>(taps) {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fir_decimate_sym::filter); }

    // FIR
    void filter(input_async_buffer<TT_DATA>& inWindow,
                input_stream_cacc48* inCascade,
                output_stream<TT_DATA>* restrict outStrean,
                output_stream<TT_DATA>* restrict outStream2);
};

//-----------------------------------------------------------------------------------------------------
// Cascaded Kernels - First Kernel. Streaming. Static coefficients. Single Input
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
          unsigned int TP_NUM_OUTPUTS,
          unsigned int TP_SAT>
class fir_decimate_sym<TT_DATA,
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
                       DUAL_IP_SINGLE,
                       USE_COEFF_RELOAD_FALSE,
                       TP_NUM_OUTPUTS,
                       USE_STREAM_API,
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
                                                          DUAL_IP_SINGLE,
                                                          USE_COEFF_RELOAD_FALSE,
                                                          TP_NUM_OUTPUTS,
                                                          USE_STREAM_API,
                                                          TP_SAT> {
   private:
   public:
    // Constructor
    fir_decimate_sym(const TT_COEFF (&taps)[(TP_FIR_LEN + 1) / kSymmetryFactor])
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
                            DUAL_IP_SINGLE,
                            USE_COEFF_RELOAD_FALSE,
                            TP_NUM_OUTPUTS,
                            USE_STREAM_API,
                            TP_SAT>(taps) {}

    static void registerKernelClass() { REGISTER_FUNCTION(fir_decimate_sym::filter); }

    void filter(input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<fnFirMargin<TP_FIR_LEN, TT_DATA>()> >&
                    inWindow,
                output_stream_cacc48* outCascade);
};

//-----------------------------------------------------------------------------------------------------
// Cascaded Kernels - First Kernel. Streaming. Static coefficients. Dual Input
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
          unsigned int TP_NUM_OUTPUTS,
          unsigned int TP_SAT>
class fir_decimate_sym<TT_DATA,
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
                       DUAL_IP_DUAL,
                       USE_COEFF_RELOAD_FALSE,
                       TP_NUM_OUTPUTS,
                       USE_STREAM_API,
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
                                                          DUAL_IP_DUAL,
                                                          USE_COEFF_RELOAD_FALSE,
                                                          TP_NUM_OUTPUTS,
                                                          USE_STREAM_API,
                                                          TP_SAT> {
   private:
   public:
    // Constructor
    fir_decimate_sym(const TT_COEFF (&taps)[(TP_FIR_LEN + 1) / kSymmetryFactor])
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
                            DUAL_IP_DUAL,
                            USE_COEFF_RELOAD_FALSE,
                            TP_NUM_OUTPUTS,
                            USE_STREAM_API,
                            TP_SAT>(taps) {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fir_decimate_sym::filter); }

    // FIR
    void filter(input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<fnFirMargin<TP_FIR_LEN, TT_DATA>()> >&
                    inWindow,
                output_stream_cacc48* outCascade,
                output_async_buffer<TT_DATA>& broadcastWindow);
};

//-----------------------------------------------------------------------------------------------------
// Cascaded Kernels - Middle Kernel. Streaming. Static coefficients. Single Input
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
          unsigned int TP_NUM_OUTPUTS,
          unsigned int TP_SAT>
class fir_decimate_sym<TT_DATA,
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
                       DUAL_IP_SINGLE,
                       USE_COEFF_RELOAD_FALSE,
                       TP_NUM_OUTPUTS,
                       USE_STREAM_API,
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
                                                          DUAL_IP_SINGLE,
                                                          USE_COEFF_RELOAD_FALSE,
                                                          TP_NUM_OUTPUTS,
                                                          USE_STREAM_API,
                                                          TP_SAT> {
   public:
    // Constructor
    fir_decimate_sym(const TT_COEFF (&taps)[(TP_FIR_LEN + 1) / kSymmetryFactor])
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
                            DUAL_IP_SINGLE,
                            USE_COEFF_RELOAD_FALSE,
                            TP_NUM_OUTPUTS,
                            USE_STREAM_API,
                            TP_SAT>(taps) {}

    static void registerKernelClass() { REGISTER_FUNCTION(fir_decimate_sym::filter); }

    void filter(input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<fnFirMargin<TP_FIR_LEN, TT_DATA>()> >&
                    inWindow,
                input_stream_cacc48* inCascade,
                output_stream_cacc48* outCascade);
};
//-----------------------------------------------------------------------------------------------------
// Cascaded Kernels - Middle Kernel. Streaming. Static coefficients. Dual Input
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
          unsigned int TP_NUM_OUTPUTS,
          unsigned int TP_SAT>
class fir_decimate_sym<TT_DATA,
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
                       DUAL_IP_DUAL,
                       USE_COEFF_RELOAD_FALSE,
                       TP_NUM_OUTPUTS,
                       USE_STREAM_API,
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
                                                          DUAL_IP_DUAL,
                                                          USE_COEFF_RELOAD_FALSE,
                                                          TP_NUM_OUTPUTS,
                                                          USE_STREAM_API,
                                                          TP_SAT> {
   public:
    // Constructor
    fir_decimate_sym(const TT_COEFF (&taps)[(TP_FIR_LEN + 1) / kSymmetryFactor])
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
                            DUAL_IP_DUAL,
                            USE_COEFF_RELOAD_FALSE,
                            TP_NUM_OUTPUTS,
                            USE_STREAM_API,
                            TP_SAT>(taps) {}

    static void registerKernelClass() { REGISTER_FUNCTION(fir_decimate_sym::filter); }

    void filter(input_async_buffer<TT_DATA>& inWindow,
                input_stream_cacc48* inCascade,
                output_stream_cacc48* outCascade,
                output_async_buffer<TT_DATA>& broadcastWindow);
};

// Cascaded Kernels - Final Kernel. Streaming. Reloadable coefficients. Single input. Single Output
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
          unsigned int TP_DUAL_IP,
          unsigned int TP_SAT>
class fir_decimate_sym<TT_DATA,
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
                       TP_DUAL_IP,
                       USE_COEFF_RELOAD_TRUE,
                       1,
                       USE_STREAM_API,
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
                                                          TP_DUAL_IP,
                                                          USE_COEFF_RELOAD_TRUE,
                                                          1,
                                                          USE_STREAM_API,
                                                          TP_SAT> {
   private:
   public:
    // Constructor
    fir_decimate_sym()
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
                            TP_DUAL_IP,
                            USE_COEFF_RELOAD_TRUE,
                            1,
                            USE_STREAM_API,
                            TP_SAT>() {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fir_decimate_sym::filter); }

    // FIR
    void filter(input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<fnFirMargin<TP_FIR_LEN, TT_DATA>()> >&
                    inWindow,
                input_stream_cacc48* inCascade,
                output_stream<TT_DATA>* outStream);
};

// Cascaded Kernels - Final Kernel. Streaming. Reloadable coefficients. Dual input. Single Output
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
          unsigned int TP_SAT>
class fir_decimate_sym<TT_DATA,
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
                       DUAL_IP_DUAL,
                       USE_COEFF_RELOAD_TRUE,
                       1,
                       USE_STREAM_API,
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
                                                          DUAL_IP_DUAL,
                                                          USE_COEFF_RELOAD_TRUE,
                                                          1,
                                                          USE_STREAM_API,
                                                          TP_SAT> {
   private:
   public:
    // Constructor
    fir_decimate_sym()
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
                            DUAL_IP_DUAL,
                            USE_COEFF_RELOAD_TRUE,
                            1,
                            USE_STREAM_API,
                            TP_SAT>() {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fir_decimate_sym::filter); }

    // FIR
    void filter(input_async_buffer<TT_DATA>& inWindow,
                input_stream_cacc48* inCascade,
                output_stream<TT_DATA>* outStream);
};

// Cascaded Kernels - Final Kernel. Streaming. Reloadable coefficients. Single input. Dual Output
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
          unsigned int TP_DUAL_IP,
          unsigned int TP_SAT>
class fir_decimate_sym<TT_DATA,
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
                       TP_DUAL_IP,
                       USE_COEFF_RELOAD_TRUE,
                       2,
                       USE_STREAM_API,
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
                                                          TP_DUAL_IP,
                                                          USE_COEFF_RELOAD_TRUE,
                                                          2,
                                                          USE_STREAM_API,
                                                          TP_SAT> {
   private:
   public:
    // Constructor
    fir_decimate_sym()
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
                            TP_DUAL_IP,
                            USE_COEFF_RELOAD_TRUE,
                            2,
                            USE_STREAM_API,
                            TP_SAT>() {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fir_decimate_sym::filter); }

    // FIR
    void filter(input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<fnFirMargin<TP_FIR_LEN, TT_DATA>()> >&
                    inWindow,
                input_stream_cacc48* inCascade,
                output_stream<TT_DATA>* restrict outStream,
                output_stream<TT_DATA>* restrict outStream2);
};

// Cascaded Kernels - Final Kernel. Streaming. Reloadable coefficients. Dual input. Dual Output
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
          unsigned int TP_SAT>
class fir_decimate_sym<TT_DATA,
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
                       DUAL_IP_DUAL,
                       USE_COEFF_RELOAD_TRUE,
                       2,
                       USE_STREAM_API,
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
                                                          DUAL_IP_DUAL,
                                                          USE_COEFF_RELOAD_TRUE,
                                                          2,
                                                          USE_STREAM_API,
                                                          TP_SAT> {
   private:
   public:
    // Constructor
    fir_decimate_sym()
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
                            DUAL_IP_DUAL,
                            USE_COEFF_RELOAD_TRUE,
                            2,
                            USE_STREAM_API,
                            TP_SAT>() {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fir_decimate_sym::filter); }

    // FIR
    void filter(input_async_buffer<TT_DATA>& inWindow,
                input_stream_cacc48* inCascade,
                output_stream<TT_DATA>* restrict outStream,
                output_stream<TT_DATA>* restrict outStream2);
};

// Cascaded Kernels - First Kernel. Streaming. Reloadable coefficients. Single Input
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
          unsigned int TP_NUM_OUTPUTS,
          unsigned int TP_SAT>
class fir_decimate_sym<TT_DATA,
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
                       DUAL_IP_SINGLE,
                       USE_COEFF_RELOAD_TRUE,
                       TP_NUM_OUTPUTS,
                       USE_STREAM_API,
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
                                                          DUAL_IP_SINGLE,
                                                          USE_COEFF_RELOAD_TRUE,
                                                          TP_NUM_OUTPUTS,
                                                          USE_STREAM_API,
                                                          TP_SAT> {
   private:
   public:
    // Constructor
    fir_decimate_sym()
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
                            DUAL_IP_SINGLE,
                            USE_COEFF_RELOAD_TRUE,
                            TP_NUM_OUTPUTS,
                            USE_STREAM_API,
                            TP_SAT>() {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fir_decimate_sym::filter); }

    // FIR
    void filter(input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<fnFirMargin<TP_FIR_LEN, TT_DATA>()> >&
                    inWindow,
                output_stream_cacc48* outCascade,
                const TT_COEFF (&inTaps)[(TP_FIR_LEN + 1) / kSymmetryFactor]);
};

// Cascaded Kernels - First Kernel. Streaming. Reloadable coefficients. Dual Input
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
          unsigned int TP_NUM_OUTPUTS,
          unsigned int TP_SAT>
class fir_decimate_sym<TT_DATA,
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
                       DUAL_IP_DUAL,
                       USE_COEFF_RELOAD_TRUE,
                       TP_NUM_OUTPUTS,
                       USE_STREAM_API,
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
                                                          DUAL_IP_DUAL,
                                                          USE_COEFF_RELOAD_TRUE,
                                                          TP_NUM_OUTPUTS,
                                                          USE_STREAM_API,
                                                          TP_SAT> {
   private:
   public:
    // Constructor
    fir_decimate_sym()
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
                            DUAL_IP_DUAL,
                            USE_COEFF_RELOAD_TRUE,
                            TP_NUM_OUTPUTS,
                            USE_STREAM_API,
                            TP_SAT>() {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fir_decimate_sym::filter); }

    // FIR
    void filter(input_circular_buffer<TT_DATA,
                                      extents<inherited_extent>,
                                      margin<fnFirMargin<TP_FIR_LEN, TT_DATA>()> >& __restrict inWindow,
                output_stream_cacc48* outCascade,
                output_async_buffer<TT_DATA>& broadcastWindow,
                const TT_COEFF (&inTaps)[(TP_FIR_LEN + 1) / kSymmetryFactor]);
};

// Cascaded Kernels - Middle Kernel. Streaming. Reloadable coefficients. Single input
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
          unsigned int TP_NUM_OUTPUTS,
          unsigned int TP_SAT>
class fir_decimate_sym<TT_DATA,
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
                       DUAL_IP_SINGLE,
                       USE_COEFF_RELOAD_TRUE,
                       TP_NUM_OUTPUTS,
                       USE_STREAM_API,
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
                                                          DUAL_IP_SINGLE,
                                                          USE_COEFF_RELOAD_TRUE,
                                                          TP_NUM_OUTPUTS,
                                                          USE_STREAM_API,
                                                          TP_SAT> {
   private:
   public:
    // Constructor
    fir_decimate_sym()
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
                            DUAL_IP_SINGLE,
                            USE_COEFF_RELOAD_TRUE,
                            TP_NUM_OUTPUTS,
                            USE_STREAM_API,
                            TP_SAT>() {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fir_decimate_sym::filter); }

    // FIR
    void filter(input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<fnFirMargin<TP_FIR_LEN, TT_DATA>()> >&
                    inWindow,
                input_stream_cacc48* inCascade,
                output_stream_cacc48* outCascade);
};

// Cascaded Kernels - Middle Kernel. Streaming. Reloadable coefficients. Dual Input
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
          unsigned int TP_NUM_OUTPUTS,
          unsigned int TP_SAT>
class fir_decimate_sym<TT_DATA,
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
                       DUAL_IP_DUAL,
                       USE_COEFF_RELOAD_TRUE,
                       TP_NUM_OUTPUTS,
                       USE_STREAM_API,
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
                                                          DUAL_IP_DUAL,
                                                          USE_COEFF_RELOAD_TRUE,
                                                          TP_NUM_OUTPUTS,
                                                          USE_STREAM_API,
                                                          TP_SAT> {
   private:
   public:
    // Constructor
    fir_decimate_sym()
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
                            DUAL_IP_DUAL,
                            USE_COEFF_RELOAD_TRUE,
                            TP_NUM_OUTPUTS,
                            USE_STREAM_API,
                            TP_SAT>() {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fir_decimate_sym::filter); }

    // FIR
    void filter(input_async_buffer<TT_DATA>& inWindow,
                input_stream_cacc48* inCascade,
                output_stream_cacc48* outCascade,
                output_async_buffer<TT_DATA>& broadcastWindow);
};
}
}
}
}
}

#endif // _DSPLIB_FIR_DECIMATE_SYM_HPP_
