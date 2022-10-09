/*
 * Copyright 2022 Xilinx, Inc.
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
#ifndef _DSPLIB_fir_resampler_HPP_
#define _DSPLIB_fir_resampler_HPP_

/*
Resampler FIR.
This file exists to capture the definition of the resampler FIR
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
#include "fir_resampler_traits.hpp"

#include <vector>
#include <numeric> //for lcm calc
#include <array>   //for phase arrays

namespace xf {
namespace dsp {
namespace aie {
namespace fir {
namespace resampler {

template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_INTERPOLATE_FACTOR,
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
          unsigned int TP_API = 0>
class fir_resampler;

template <typename fp = fir_params_defaults>
class fir_resampler_tl {
   public:
    // Get kernel's FIR range Length
    template <int pos>
    static constexpr unsigned int getKernelFirRangeLen() {
        constexpr unsigned int firRangeLen = pos + 1 == fp::BTP_CASC_LEN
                                                 ? fnFirRangeRem<CEIL(fp::BTP_FIR_LEN, fp::BTP_INTERPOLATE_FACTOR),
                                                                 fp::BTP_CASC_LEN, pos, fp::BTP_INTERPOLATE_FACTOR>()
                                                 : fnFirRange<CEIL(fp::BTP_FIR_LEN, fp::BTP_INTERPOLATE_FACTOR),
                                                              fp::BTP_CASC_LEN, pos, fp::BTP_INTERPOLATE_FACTOR>();
        return firRangeLen;
    };

    template <int pos, int CLEN, int T_FIR_LEN, typename T_D, typename T_C, unsigned int T_DF, unsigned int T_IF>
    static constexpr unsigned int fnCheckIfFits() {
        constexpr int samplesInBuff = fnSamplesIn1024<T_D>();
        constexpr unsigned int fir_range_len = getKernelFirRangeLen<pos>();
        constexpr unsigned int m_kPolyLen = (fir_range_len + T_IF - 1) / T_IF;
        constexpr unsigned int m_kFirLenCeil = CEIL(T_FIR_LEN, T_IF);
        constexpr unsigned int m_kFirCoeffOffset =
            fnFirRangeOffset<m_kFirLenCeil, CLEN, pos, T_IF>(); // FIR Cascade Coeff Offset for this kernel position
        constexpr unsigned int m_kFirRangeOffset =
            m_kFirCoeffOffset / T_IF; // FIR Cascade Data Offset for this kernel position
        constexpr unsigned int m_kFirMarginLen = m_kFirLenCeil / T_IF;
        constexpr unsigned int m_kFirMarginOffset =
            fnFirMargin<m_kFirMarginLen, T_D>() - m_kFirMarginLen + 1; // FIR Margin Offset.
        constexpr unsigned int m_kWinAccessByteSize =
            fnWinAccessByteSize<T_D, T_C>(); // The memory data path is min 128-bits wide for vector operations
        constexpr unsigned int m_kFirInitOffset = m_kFirRangeOffset + m_kFirMarginOffset;
        constexpr unsigned int m_kDataBuffXOffset =
            m_kFirInitOffset % (m_kWinAccessByteSize / sizeof(T_D)); // Remainder of m_kFirInitOffset divided by 128bit
        constexpr unsigned int m_kLanes = fnNumLanesResampler<T_D, T_C, 1>(); // number of operations in parallel of
                                                                              // this type combinations that the vector
                                                                              // processor can do.
        constexpr unsigned int m_kNumSamplesRequiredForNLanes = (m_kLanes * T_DF + (T_IF - 1)) / T_IF;
        constexpr unsigned int m_kInitDataNeeded = m_kDataBuffXOffset + m_kPolyLen + m_kNumSamplesRequiredForNLanes - 1;

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
            fnFirMargin<((fp::BTP_FIR_LEN + fp::BTP_INTERPOLATE_FACTOR - 1) / fp::BTP_INTERPOLATE_FACTOR),
                        typename fp::BTT_DATA>();
        return margin;
    };

    static constexpr unsigned int getDF() { return fp::BTP_DECIMATE_FACTOR; };
    static constexpr unsigned int getIF() { return fp::BTP_INTERPOLATE_FACTOR; };

    // Get FIR variant
    static constexpr eFIRVariant getFirType() { return eFIRVariant::kSrAsym; };
    // Get FIR source file
    static const char* getFirSource() { return "fir_resampler.cpp"; };

    using parent_class = fir_resampler<typename fp::BTT_DATA,
                                       typename fp::BTT_COEFF,
                                       fp::BTP_FIR_LEN,
                                       fp::BTP_INTERPOLATE_FACTOR,
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
                                       fp::BTP_API>;
};
//-----------------------------------------------------------------------------------------------------
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_INTERPOLATE_FACTOR,
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
          unsigned int TP_API = 0>
class kernelFilterClass {
   private:
    // Parameter value defensive and legality checks
    static_assert(TP_INTERPOLATE_FACTOR >= INTERPOLATE_FACTOR_MIN && TP_INTERPOLATE_FACTOR <= INTERPOLATE_FACTOR_MAX,
                  "ERROR: TP_INTERPOLATE_FACTOR is out of the supported range.");
    static_assert(TP_DECIMATE_FACTOR > 0 && TP_DECIMATE_FACTOR <= INTERPOLATE_FACTOR_MAX,
                  "ERROR: TP_DECIMATE_FACTOR is out of supported range.");
    // static_assert(TP_FIR_LEN <= FIR_LEN_MAX,"ERROR: Max supported FIR length exceeded. ");
    // static_assert(TP_FIR_RANGE_LEN >= FIR_LEN_MIN,"ERROR: Illegal combination of design FIR length and cascade
    // length, resulting in kernel FIR length below minimum required value. ");
    static_assert(TP_SHIFT >= SHIFT_MIN && TP_SHIFT <= SHIFT_MAX, "ERROR: TP_SHIFT is out of the supported range.");
    static_assert(TP_RND >= ROUND_MIN && TP_RND <= ROUND_MAX, "ERROR: TP_RND is out of the supported range.");
    static_assert(fnEnumType<TT_DATA>() != enumUnknownType, "ERROR: TT_DATA is not a supported type.");
    static_assert(fnEnumType<TT_COEFF>() != enumUnknownType, "ERROR: TT_COEFF is not a supported type.");
    // static_assert(fnFirInterpFractTypeSupport<TT_DATA, TT_COEFF>() != 0, "ERROR: This library element currently
    // supports TT_DATA of cint16 and TT_COEFF of int16.");
    static_assert(fnTypeCheckDataCoeffSize<TT_DATA, TT_COEFF>() != 0,
                  "ERROR: TT_DATA type less precise than TT_COEFF is not supported.");
    static_assert(fnTypeCheckDataCoeffCmplx<TT_DATA, TT_COEFF>() != 0,
                  "ERROR: real TT_DATA with complex TT_COEFF is not supported.");
    static_assert(fnTypeCheckDataCoeffFltInt<TT_DATA, TT_COEFF>() != 0,
                  "ERROR: a mix of float and integer types of TT_DATA and TT_COEFF is not supported.");
    static_assert((((TP_INPUT_WINDOW_VSIZE * TP_INTERPOLATE_FACTOR) % TP_DECIMATE_FACTOR) == 0),
                  "Number of input samples must give an integer number of output samples based on Interpolate Factor "
                  "and Decimate Factor");
    static_assert((((TP_INPUT_WINDOW_VSIZE * sizeof(TT_DATA)) % (128 / 8)) == 0),
                  "Number of input samples must align to 128 bits.");
    static_assert(TP_NUM_OUTPUTS > 0 && TP_NUM_OUTPUTS <= 2, "ERROR: only single or dual outputs are supported.");
    static_assert(!(std::is_same<TT_DATA, cfloat>::value || std::is_same<TT_DATA, float>::value) || (TP_SHIFT == 0),
                  "ERROR: TP_SHIFT cannot be performed for TT_DATA=cfloat, so must be set to 0");

    static constexpr unsigned int m_kColumns =
        fnNumColumnsResampler<TT_DATA, TT_COEFF, TP_API>(); // number of mult-adds per lane for main intrinsic
    static constexpr unsigned int m_kLanes =
        fnNumLanesResampler<TT_DATA, TT_COEFF, TP_API>(); // number of operations in parallel of this type combinations
                                                          // that the vector processor can do.
    static constexpr unsigned int m_kFirLenCeil = CEIL(TP_FIR_LEN, TP_INTERPOLATE_FACTOR);
    static constexpr unsigned int m_kFirMarginLen = m_kFirLenCeil / TP_INTERPOLATE_FACTOR;
    static constexpr unsigned int m_kPolyLen = (TP_FIR_RANGE_LEN + TP_INTERPOLATE_FACTOR - 1) / TP_INTERPOLATE_FACTOR;
    static constexpr unsigned int m_kFirLenCeilCols = CEIL(m_kPolyLen, m_kColumns);
    static constexpr unsigned int m_kNumOps = CEIL(m_kPolyLen, m_kColumns) / m_kColumns;
    // static constexpr unsigned int  m_kStreamReadWidth   = (TP_DECIMATE_FACTOR%2==0) ? 256 :
    // fnStreamReadWidth<TT_DATA, TT_COEFF>();
    static constexpr unsigned int m_kStreamReadWidth = fnStreamReadWidth<TT_DATA, TT_COEFF>();
    static constexpr unsigned int m_kStreamLoadVsize =
        m_kStreamReadWidth / 8 /
        sizeof(TT_DATA); // vector size of a single stream read command (reading from single or 2 streams)
    static constexpr unsigned int m_kDataLoadVsize =
        TP_API == 1
            ? m_kStreamLoadVsize
            : fnDataLoadVsizeResampler<TT_DATA>(); // vector size of single data load command, e.g. upd_w or stream read
    static constexpr unsigned int m_kSamplesInBuff = fnSamplesIn1024<TT_DATA>();
    static constexpr unsigned int m_kDataLoadsInReg =
        m_kSamplesInBuff / m_kDataLoadVsize; // ratio of sbuff to load size.

    static constexpr unsigned int m_kZbuffSize = 256 / 8; // kZbuffSize (256bit) - const for all data/coeff types
    static constexpr unsigned int m_kCoeffRegVsize = m_kZbuffSize / sizeof(TT_COEFF);
    static constexpr unsigned int m_kMultsPerCycle = m_kLanes * m_kColumns;
    static constexpr unsigned int m_kVOutSize = fnVOutSizeResampler<TT_DATA, TT_COEFF, TP_API>();
    static constexpr unsigned int m_kFirCoeffOffset =
        fnFirRangeOffset<m_kFirLenCeil, TP_CASC_LEN, TP_KERNEL_POSITION, TP_INTERPOLATE_FACTOR>(); // FIR Cascade Coeff
                                                                                                   // Offset for this
                                                                                                   // kernel position
    static constexpr unsigned int m_kFirRangeOffset =
        m_kFirCoeffOffset / TP_INTERPOLATE_FACTOR; // FIR Cascade Data Offset for this kernel position
    static constexpr unsigned int m_kFirMarginOffset =
        fnFirMargin<m_kFirMarginLen, TT_DATA>() - m_kFirMarginLen + 1; // FIR Margin Offset.
    static constexpr unsigned int m_kWinAccessByteSize =
        fnWinAccessByteSize<TT_DATA, TT_COEFF>(); // The memory data path is min 128-bits wide for vector operations
    static constexpr unsigned int m_kFirInitOffset = m_kFirRangeOffset + m_kFirMarginOffset;
    static constexpr unsigned int m_kDataBuffXOffset =
        m_kFirInitOffset % (m_kWinAccessByteSize / sizeof(TT_DATA)); // Remainder of m_kFirInitOffset divided by 128bit
    static constexpr eArchType m_kArch = TP_API == USE_STREAM_API ? kArchStream : kArchBasic;
    static constexpr unsigned int m_kNumSamplesRequiredForNLanes =
        (m_kLanes * TP_DECIMATE_FACTOR + (TP_INTERPOLATE_FACTOR - 1)) / TP_INTERPOLATE_FACTOR;
    static constexpr unsigned int m_kCFloatDataType = std::is_same<TT_DATA, cfloat>::value ? 1 : 0;
    static constexpr unsigned int m_kInitDataNeeded =
        m_kDataBuffXOffset + m_kPolyLen + m_kNumSamplesRequiredForNLanes - 1;

    // INLINE_DECL definition of the struct which is declared in traits.
    static constexpr firParamsTrait params{
        sizeof(TT_DATA),                         // dataSizeBytes;
        TP_FIR_LEN,                              // firLen ;
        m_kDataLoadVsize,                        // loadSize ;
        m_kSamplesInBuff,                        // dataBuffSamples ;
        m_kWinAccessByteSize,                    // alignWindowReadBytes ;
        m_kFirMarginLen - 1 - m_kFirRangeOffset, //  marginOffsetIndex ;
        0,                                       //  rangeOffsetIndex ;
    };
    // the lowest common multiple defines how many output samples it would take
    // to have lane 0 with polyphase 0 again. Divide by lanes to get number of
    // vector outputs.
    static constexpr unsigned int m_kPolyphaseLaneAliasInternal =
        (my_lcm(TP_INTERPOLATE_FACTOR, m_kLanes) / m_kLanes >= 1) ? (my_lcm(TP_INTERPOLATE_FACTOR, m_kLanes) / m_kLanes)
                                                                  : 1;
    // Takes into account the buffer loads and window decrements required at the end of each phase.
    static constexpr unsigned int m_kPolyphaseLaneAlias = calculateLaneAlias(params,
                                                                             TP_DECIMATE_FACTOR,
                                                                             TP_INTERPOLATE_FACTOR,
                                                                             m_kLanes,
                                                                             m_kColumns,
                                                                             m_kNumOps,
                                                                             m_kPolyphaseLaneAliasInternal);

    // If interp fits within one set of lanes, then we don't need to duplicate
    // coeff storage into phases, as this can be acheived through zoffsets only.
    static constexpr unsigned int m_kPolyphaseCoeffAlias =
        TP_INTERPOLATE_FACTOR <= m_kLanes ? 1 : m_kPolyphaseLaneAlias;

    static constexpr unsigned int m_kNumOutputs =
        ((unsigned int)((TP_INPUT_WINDOW_VSIZE * TP_INTERPOLATE_FACTOR) / TP_DECIMATE_FACTOR));
    static constexpr unsigned int m_kLsize =
        m_kNumOutputs /
        (m_kPolyphaseLaneAlias *
         m_kVOutSize); // loop length, given that <m_kVOutSize> samples are output per m_kPolyphaseLaneAlias loop

    // Typically, we need less data than coefficients for each operation
    // we're going to need MIN(interp,lanes)*num_cols coefficients and MIN(lanes,rndUp(((LCM*interp)/(Deci*lanes)))
    static_assert(sizeof(TT_DATA) * m_kLanes <= m_kZbuffSize,
                  "ERROR: Invalid assumption in archtecture. Can't fit enough data into selected (Z) buffer.");

    // Coefficient Load Size - number of samples in 256-bits
    static constexpr unsigned int m_kCoeffLoadSize = 256 / 8 / sizeof(TT_COEFF);
    alignas(32) TT_COEFF
        m_oldInTaps[CEIL(TP_FIR_LEN, m_kCoeffLoadSize)]; // Previous user input coefficients with zero padding
    bool m_coeffnEq;                                     // Are coefficients sets equal?

    template <typename T>
    using polyphaseArray = std::array<T, m_kPolyphaseLaneAlias>;

    static constexpr polyphaseArray<unsigned int> m_kDataNeededPhase =
        getDataNeeded<m_kPolyphaseLaneAlias>(params, TP_DECIMATE_FACTOR, TP_INTERPOLATE_FACTOR, m_kLanes, m_kColumns);
    static constexpr polyphaseArray<unsigned int> m_kInitialLoads =
        getInitialLoads<m_kPolyphaseLaneAlias>(params, TP_DECIMATE_FACTOR, TP_INTERPOLATE_FACTOR, m_kLanes, m_kColumns);
    static constexpr polyphaseArray<int> xstartPhase =
        getXStarts<m_kPolyphaseLaneAlias>(params, TP_DECIMATE_FACTOR, TP_INTERPOLATE_FACTOR, m_kLanes, m_kColumns);
    static constexpr polyphaseArray<int> windowDecPhase = getWindowDecrements<m_kPolyphaseLaneAlias>(
        params, TP_DECIMATE_FACTOR, TP_INTERPOLATE_FACTOR, m_kLanes, m_kColumns, m_kNumOps);

    static constexpr polyphaseArray<unsigned int> xoffsets =
        getXOffsets<m_kPolyphaseLaneAlias>(TP_DECIMATE_FACTOR, TP_INTERPOLATE_FACTOR, m_kLanes);
    static constexpr polyphaseArray<unsigned int> zoffsets =
        getZOffsets<m_kPolyphaseLaneAlias>(TP_INTERPOLATE_FACTOR, m_kLanes);

    static constexpr int m_kRepeatFactor = 16;
    static constexpr int marginLoadsMappedToBuff =
        (fnFirMargin<m_kFirMarginLen, TT_DATA>() % m_kSamplesInBuff) / m_kStreamLoadVsize;
    static constexpr int streamDataOffsetWithinBuff = (m_kFirInitOffset) % m_kSamplesInBuff;
    static constexpr int emptyInitLanes =
        CEIL((m_kFirLenCeil - TP_FIR_RANGE_LEN - m_kFirRangeOffset * TP_INTERPOLATE_FACTOR), TP_DECIMATE_FACTOR) /
        TP_DECIMATE_FACTOR;
    static constexpr int streamInitNullAccs =
        (emptyInitLanes / m_kVOutSize); // Number of Null Mac Vectors sent as partial prouducts over cascade.
    static constexpr int streamInitAccStrobes =
        (CEIL(streamInitNullAccs, m_kRepeatFactor) -
         CEIL(streamInitNullAccs, m_kPolyphaseLaneAlias) / m_kPolyphaseLaneAlias); // Number of non-Null Mac Vector
                                                                                   // strobe iterations needed to make
                                                                                   // up a full m_kRepeatFactor loop.

    static_assert(
        m_kNumSamplesRequiredForNLanes + m_kColumns - 1 <= kXYBuffSize,
        "ERROR: the overall decimation rate requires more samples than can be fit within the vector register.");
    static_assert(m_kNumSamplesRequiredForNLanes <= 16,
                  "ERROR: the overall decimation rate requires more integer samples than can be indexed within the "
                  "vector register (xoffsets up to 16 samples).");
    static_assert(!(m_kCFloatDataType && m_kNumSamplesRequiredForNLanes > 8),
                  "ERROR: the overall decimation rate requires more complex floating-point data samples than can be "
                  "indexed within the vector register (xoffsets up to 8 samples).");
    static_assert(m_kNumOutputs % (m_kVOutSize) == 0,
                  "ERROR: output window size must be a multiple of number of lanes. ");
    static_assert(m_kNumOutputs % (m_kPolyphaseLaneAlias * m_kVOutSize) == 0,
                  "ERROR: due to architectural optimisation, this window size is not currently supported. Please use a "
                  "TP_INPUT_WINDOW_VSIZE that will give a number of output samples which is a multiple of Lanes and "
                  "m_kPolyphaseLaneAlias.");
    static_assert(!(m_kArch == kArchStream && m_kInitDataNeeded > m_kSamplesInBuff),
                  "ERROR: TP_FIR_RANGE_LEN exceeds max supported range for this data/coeff type combination. Increase "
                  "TP_CASC_LEN to split the workload over more kernels.");
    static_assert(!(m_kArch == kArchStream && m_kLsize % m_kRepeatFactor != 0),
                  "ERROR: For optimal design, inner loop size must schedule multiple iterations of vector operations. "
                  "Please use a TP_INPUT_WINDOW_VSIZE that results in a m_kLsize being a multiple of m_kRepeatFactor.");

    alignas(32) int delay[32] = {0};
    int doInit = (TP_CASC_LEN == 1 || emptyInitLanes == 0) ? 0 : 1;

    // The coefficients array must include zero padding up to a multiple of the number of columns
    // the MAC intrinsic used to eliminate the accidental inclusion of terms beyond the FIR length.
    // Since this zero padding cannot be applied to the class-external coefficient array
    // the supplied taps are copied to an internal array, m_internalTaps2, which can be padded.
    using tapsArray = TT_COEFF[m_kPolyphaseCoeffAlias][m_kNumOps][m_kColumns][m_kLanes];
    alignas(32) tapsArray m_internalTaps; // Filter taps/coefficients
    alignas(32) TT_COEFF
        m_internalTaps2[m_kPolyphaseCoeffAlias * m_kNumOps * m_kColumns * m_kLanes]; // Filter taps/coefficients

    // Filter kernel architecture
    void filterBasic(T_inputIF<TP_CASC_IN, TT_DATA, TP_DUAL_IP> inInterface,
                     T_outputIF<TP_CASC_OUT, TT_DATA> outInterface);
    void filterStream(T_inputIF<TP_CASC_IN, TT_DATA, TP_DUAL_IP> inInterface,
                      T_outputIF<TP_CASC_OUT, TT_DATA> outInterface);
    void filterSelectArch(T_inputIF<TP_CASC_IN, TT_DATA, TP_DUAL_IP> inInterface,
                          T_outputIF<TP_CASC_OUT, TT_DATA> outInterface);

   public:
    // Access function for AIE Synthesizer
    static unsigned int get_m_kArch() { return 0; }; // no distinct architectures yet

    static unsigned int get_polyphaseLaneAlias() { return m_kPolyphaseLaneAlias; };

    // Constructors
    kernelFilterClass() : m_oldInTaps{}, m_internalTaps2{}, m_internalTaps{} {}

    kernelFilterClass(const TT_COEFF (&taps)[TP_FIR_LEN]) : m_internalTaps2{}, m_internalTaps{} {
        // Loads taps/coefficients
        firReload(taps);
    };

    void firReload(const TT_COEFF* taps) {
        TT_COEFF* tapsPtr = (TT_COEFF*)taps;
        firReload(tapsPtr);
    };

    void firReload(TT_COEFF* taps) {
        constexpr unsigned int I = TP_INTERPOLATE_FACTOR;
        constexpr unsigned int D = TP_DECIMATE_FACTOR;
        // Loads taps/coefficients
        // Need nColumns more coefficients each op (including padding)
        // there are FirLenCeilCols/InterpRate columns split over numOps at nColumns per op
        for (int coeffPhase = 0; coeffPhase < m_kPolyphaseCoeffAlias; coeffPhase++) {
            for (int op = 0; op < m_kNumOps; op++) {
                for (int col = 0; col < m_kColumns; col++) {
                    // polyphaseI
                    // When interpolation rate is > number of lanes, we use coeffPhase
                    for (int poly = 0; poly < m_kLanes; poly++) {
                        // non-reversed indexes
                        // Reorder coefficients so that they are in the order they will be
                        // used in due to decimate factor. This means that zoffsets will
                        // always be 01234012,34012340 regardless of decimation factor.
                        // Instead of 04321043,21..
                        // If you draw the polyphase diagram, this is the cascade column index for each polyphase.
                        int polyPhaseCol = op * m_kColumns + col;

                        int polyphaseIndex = (((coeffPhase * m_kLanes + poly) * D) % TP_INTERPOLATE_FACTOR);
                        // We could modulus poly by interpRate, but we're
                        // already going to pad taps array for values over interpRate.
                        int tapIndexFwd = polyphaseIndex + polyPhaseCol * I;
                        // Coefficient reversal, retaining order of polyphases
                        // int tapIndexRev = TP_FIR_LEN +
                        int tapIndexRev = CEIL(TP_FIR_LEN, TP_INTERPOLATE_FACTOR) + polyphaseIndex -
                                          (polyPhaseCol + 1) * I - m_kFirCoeffOffset;

                        if (poly < TP_INTERPOLATE_FACTOR &&
                            // tapIndexRev >= TP_FIR_LEN - m_kFirCoeffOffset - TP_FIR_RANGE_LEN &&
                            tapIndexRev >= CEIL(TP_FIR_LEN, TP_INTERPOLATE_FACTOR) - m_kFirCoeffOffset -
                                               CEIL(TP_FIR_RANGE_LEN, TP_INTERPOLATE_FACTOR) &&
                            tapIndexRev < TP_FIR_LEN) {
                            m_internalTaps[coeffPhase][op][col][poly] = taps[tapIndexRev];
                            // m_internalTaps2[coeffPhase*m_kNumOps*m_kColumns*m_kLanes + op*m_kColumns*m_kLanes +
                            // col*m_kLanes + poly] = taps[tapIndexRev];
                        } else {
                            // padding, interpRate doesn't fit into m_kLanes
                            // or fir_len/interp doesn't fit into m_kColummns
                            // or fir_len doesn't fit into interpolate factor
                            // This padding is necessary in order to have coef reads at a
                            // 256b boundary.
                            m_internalTaps[coeffPhase][op][col][poly] = nullElem<TT_COEFF>(); // 0 for the type.
                            // m_internalTaps2[coeffPhase*m_kNumOps*m_kColumns*m_kLanes + op*m_kColumns*m_kLanes +
                            // col*m_kLanes + poly] = nullElem<TT_COEFF>(); //0 for the type.
                        }

                        // if (m_internalTaps[coeffPhase][op][col][poly] !=
                        // m_internalTaps2[coeffPhase*m_kNumOps*m_kColumns*m_kLanes + op*m_kColumns*m_kLanes +
                        // col*m_kLanes + poly]) {
                        //     printf("ERROR: COEFFS incorrectly created");
                        // }
                    }
                }
            }
        }
    }

    // Filter kernel for static coefficient designs
    void filterKernel(T_inputIF<TP_CASC_IN, TT_DATA, TP_DUAL_IP> inInterface,
                      T_outputIF<TP_CASC_OUT, TT_DATA> outInterface);

    // Filter kernel for reloadable coefficient designs
    void filterKernel(T_inputIF<TP_CASC_IN, TT_DATA, TP_DUAL_IP> inInterface,
                      T_outputIF<TP_CASC_OUT, TT_DATA> outInterface,
                      const TT_COEFF (&inTaps)[TP_FIR_LEN]);
    void filterKernelRtp(T_inputIF<TP_CASC_IN, TT_DATA, TP_DUAL_IP> inInterface,
                         T_outputIF<TP_CASC_OUT, TT_DATA> outInterface);
};

//-----------------------------------------------------------------------------------------------------
// Single kernel base specialization. No cascade ports. Static coefficients
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_INTERPOLATE_FACTOR,
          unsigned int TP_DECIMATE_FACTOR,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          bool TP_CASC_IN,
          bool TP_CASC_OUT,
          unsigned int TP_FIR_RANGE_LEN,
          unsigned int TP_KERNEL_POSITION,
          unsigned int TP_CASC_LEN,
          unsigned int TP_USE_COEFF_RELOAD, // 1 = use coeff reload, 0 = don't use coeff reload
          unsigned int TP_NUM_OUTPUTS,
          unsigned int TP_DUAL_IP,
          unsigned int TP_API>
class fir_resampler : public kernelFilterClass<TT_DATA,
                                               TT_COEFF,
                                               TP_FIR_LEN,
                                               TP_INTERPOLATE_FACTOR,
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
                                               TP_API> {
   public:
    // Constructor
    fir_resampler(const TT_COEFF (&taps)[TP_FIR_LEN])
        : kernelFilterClass<TT_DATA,
                            TT_COEFF,
                            TP_FIR_LEN,
                            TP_INTERPOLATE_FACTOR,
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
                            TP_API>(taps) {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fir_resampler::filter); }

    // FIR
    void filter(input_window<TT_DATA>* inWindow, output_window<TT_DATA>* outWindow);
};

// Single kernel specialization. No cascade ports. Static coefficients, dual output
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_INTERPOLATE_FACTOR,
          unsigned int TP_DECIMATE_FACTOR,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_FIR_RANGE_LEN>

class fir_resampler<TT_DATA,
                    TT_COEFF,
                    TP_FIR_LEN,
                    TP_INTERPOLATE_FACTOR,
                    TP_DECIMATE_FACTOR,
                    TP_SHIFT,
                    TP_RND,
                    TP_INPUT_WINDOW_VSIZE,
                    false,
                    false,
                    TP_FIR_RANGE_LEN,
                    0,
                    1,
                    USE_COEFF_RELOAD_FALSE,
                    2,
                    DUAL_IP_SINGLE,
                    USE_WINDOW_API> : public kernelFilterClass<TT_DATA,
                                                               TT_COEFF,
                                                               TP_FIR_LEN,
                                                               TP_INTERPOLATE_FACTOR,
                                                               TP_DECIMATE_FACTOR,
                                                               TP_SHIFT,
                                                               TP_RND,
                                                               TP_INPUT_WINDOW_VSIZE,
                                                               false,
                                                               false,
                                                               TP_FIR_RANGE_LEN,
                                                               0,
                                                               1,
                                                               USE_COEFF_RELOAD_FALSE,
                                                               2,
                                                               DUAL_IP_SINGLE,
                                                               USE_WINDOW_API> {
   public:
    // Constructor
    fir_resampler(const TT_COEFF (&taps)[TP_FIR_LEN])
        : kernelFilterClass<TT_DATA,
                            TT_COEFF,
                            TP_FIR_LEN,
                            TP_INTERPOLATE_FACTOR,
                            TP_DECIMATE_FACTOR,
                            TP_SHIFT,
                            TP_RND,
                            TP_INPUT_WINDOW_VSIZE,
                            false,
                            false,
                            TP_FIR_RANGE_LEN,
                            0,
                            1,
                            USE_COEFF_RELOAD_FALSE,
                            2,
                            DUAL_IP_SINGLE,
                            USE_WINDOW_API>(taps) {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fir_resampler::filter); }

    // FIR
    void filter(input_window<TT_DATA>* inWindow, output_window<TT_DATA>* outWindow, output_window<TT_DATA>* outWindow2);
};

//-----------------------------------------------------------------------------------------------------
// Single kernel specialization. No cascade ports. Using coefficient reload, single output
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_INTERPOLATE_FACTOR,
          unsigned int TP_DECIMATE_FACTOR,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_FIR_RANGE_LEN,
          unsigned int TP_KERNEL_POSITION,
          unsigned int TP_CASC_LEN>

class fir_resampler<TT_DATA,
                    TT_COEFF,
                    TP_FIR_LEN,
                    TP_INTERPOLATE_FACTOR,
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
                    USE_WINDOW_API> : public kernelFilterClass<TT_DATA,
                                                               TT_COEFF,
                                                               TP_FIR_LEN,
                                                               TP_INTERPOLATE_FACTOR,
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
                                                               USE_WINDOW_API> {
   public:
    // Constructor
    fir_resampler()
        : kernelFilterClass<TT_DATA,
                            TT_COEFF,
                            TP_FIR_LEN,
                            TP_INTERPOLATE_FACTOR,
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
                            USE_WINDOW_API>() {}
    fir_resampler(const TT_COEFF (&taps)[TP_FIR_LEN])
        : kernelFilterClass<TT_DATA,
                            TT_COEFF,
                            TP_FIR_LEN,
                            TP_INTERPOLATE_FACTOR,
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
                            USE_WINDOW_API>(taps) {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fir_resampler::filter); }

    // FIR
    void filter(input_window<TT_DATA>* __restrict inWindow,
                output_window<TT_DATA>* __restrict outWindow,
                const TT_COEFF (&inTaps)[TP_FIR_LEN]);
};

// Single kernel specialization. No cascade ports. Using coefficient reload, dual output
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_INTERPOLATE_FACTOR,
          unsigned int TP_DECIMATE_FACTOR,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_FIR_RANGE_LEN,
          unsigned int TP_KERNEL_POSITION,
          unsigned int TP_CASC_LEN>

class fir_resampler<TT_DATA,
                    TT_COEFF,
                    TP_FIR_LEN,
                    TP_INTERPOLATE_FACTOR,
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
                    USE_WINDOW_API> : public kernelFilterClass<TT_DATA,
                                                               TT_COEFF,
                                                               TP_FIR_LEN,
                                                               TP_INTERPOLATE_FACTOR,
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
                                                               USE_WINDOW_API> {
   public:
    // Constructor
    fir_resampler()
        : kernelFilterClass<TT_DATA,
                            TT_COEFF,
                            TP_FIR_LEN,
                            TP_INTERPOLATE_FACTOR,
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
                            USE_WINDOW_API>() {}
    fir_resampler(const TT_COEFF (&taps)[TP_FIR_LEN])
        : kernelFilterClass<TT_DATA,
                            TT_COEFF,
                            TP_FIR_LEN,
                            TP_INTERPOLATE_FACTOR,
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
                            USE_WINDOW_API>(taps) {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fir_resampler::filter); }

    // FIR
    void filter(input_window<TT_DATA>* __restrict inWindow,
                output_window<TT_DATA>* __restrict outWindow,
                output_window<TT_DATA>* __restrict outWindow2,
                const TT_COEFF (&inTaps)[TP_FIR_LEN]);
};

//-----------------------------------------------------------------------------------------------------
// Partially specialized classes for cascaded interface - final kernel. Static coefficients single output
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_INTERPOLATE_FACTOR,
          unsigned int TP_DECIMATE_FACTOR,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_FIR_RANGE_LEN,
          unsigned int TP_KERNEL_POSITION,
          unsigned int TP_CASC_LEN>

class fir_resampler<TT_DATA,
                    TT_COEFF,
                    TP_FIR_LEN,
                    TP_INTERPOLATE_FACTOR,
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
                    USE_WINDOW_API> : public kernelFilterClass<TT_DATA,
                                                               TT_COEFF,
                                                               TP_FIR_LEN,
                                                               TP_INTERPOLATE_FACTOR,
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
                                                               USE_WINDOW_API> {
   public:
    // Constructor
    fir_resampler(const TT_COEFF (&taps)[TP_FIR_LEN])
        : kernelFilterClass<TT_DATA,
                            TT_COEFF,
                            TP_FIR_LEN,
                            TP_INTERPOLATE_FACTOR,
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
                            USE_WINDOW_API>(taps) {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fir_resampler::filter); }

    // FIR
    void filter(input_window<TT_DATA>* inWindow, input_stream_cacc48* inCascade, output_window<TT_DATA>* outWindow);
};

// Partially specialized classes for cascaded interface - final kernel. Static coefficients, dual output
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_INTERPOLATE_FACTOR,
          unsigned int TP_DECIMATE_FACTOR,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_FIR_RANGE_LEN,
          unsigned int TP_KERNEL_POSITION,
          unsigned int TP_CASC_LEN>

class fir_resampler<TT_DATA,
                    TT_COEFF,
                    TP_FIR_LEN,
                    TP_INTERPOLATE_FACTOR,
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
                    USE_WINDOW_API> : public kernelFilterClass<TT_DATA,
                                                               TT_COEFF,
                                                               TP_FIR_LEN,
                                                               TP_INTERPOLATE_FACTOR,
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
                                                               USE_WINDOW_API> {
   public:
    // Constructor
    fir_resampler(const TT_COEFF (&taps)[TP_FIR_LEN])
        : kernelFilterClass<TT_DATA,
                            TT_COEFF,
                            TP_FIR_LEN,
                            TP_INTERPOLATE_FACTOR,
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
                            USE_WINDOW_API>(taps) {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fir_resampler::filter); }

    // FIR
    void filter(input_window<TT_DATA>* inWindow,
                input_stream_cacc48* inCascade,
                output_window<TT_DATA>* outWindow,
                output_window<TT_DATA>* outWindow2);
};

//-----------------------------------------------------------------------------------------------------
// Partially specialized classes for cascaded interface - first kernel. Static coefficients
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_INTERPOLATE_FACTOR,
          unsigned int TP_DECIMATE_FACTOR,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_FIR_RANGE_LEN,
          unsigned int TP_KERNEL_POSITION,
          unsigned int TP_CASC_LEN>

class fir_resampler<TT_DATA,
                    TT_COEFF,
                    TP_FIR_LEN,
                    TP_INTERPOLATE_FACTOR,
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
                    USE_WINDOW_API> : public kernelFilterClass<TT_DATA,
                                                               TT_COEFF,
                                                               TP_FIR_LEN,
                                                               TP_INTERPOLATE_FACTOR,
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
                                                               USE_WINDOW_API> {
   public:
    // Constructor
    fir_resampler(const TT_COEFF (&taps)[TP_FIR_LEN])
        : kernelFilterClass<TT_DATA,
                            TT_COEFF,
                            TP_FIR_LEN,
                            TP_INTERPOLATE_FACTOR,
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
                            USE_WINDOW_API>(taps) {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fir_resampler::filter); }

    // FIR
    void filter(input_window<TT_DATA>* inWindow,
                output_stream_cacc48* outCascade,
                output_window<TT_DATA>* broadcastWindow);
};

//-----------------------------------------------------------------------------------------------------
// Partially specialized classes for cascaded interface - middle kernel. Static coefficients
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_INTERPOLATE_FACTOR,
          unsigned int TP_DECIMATE_FACTOR,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_FIR_RANGE_LEN,
          unsigned int TP_KERNEL_POSITION,
          unsigned int TP_CASC_LEN>

class fir_resampler<TT_DATA,
                    TT_COEFF,
                    TP_FIR_LEN,
                    TP_INTERPOLATE_FACTOR,
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
                    USE_WINDOW_API> : public kernelFilterClass<TT_DATA,
                                                               TT_COEFF,
                                                               TP_FIR_LEN,
                                                               TP_INTERPOLATE_FACTOR,
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
                                                               USE_WINDOW_API> {
   public:
    // Constructor
    fir_resampler(const TT_COEFF (&taps)[TP_FIR_LEN])
        : kernelFilterClass<TT_DATA,
                            TT_COEFF,
                            TP_FIR_LEN,
                            TP_INTERPOLATE_FACTOR,
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
                            USE_WINDOW_API>(taps) {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fir_resampler::filter); }

    // FIR
    void filter(input_window<TT_DATA>* inWindow,
                input_stream_cacc48* inCascade,
                output_stream_cacc48* outCascade,
                output_window<TT_DATA>* broadcastWindow);
};

//-----------------------------------------------------------------------------------------------------
// Partially specialized classes for cascaded interface - final kernel. Reloadable coefficients
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_INTERPOLATE_FACTOR,
          unsigned int TP_DECIMATE_FACTOR,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_FIR_RANGE_LEN,
          unsigned int TP_KERNEL_POSITION,
          unsigned int TP_CASC_LEN>

class fir_resampler<TT_DATA,
                    TT_COEFF,
                    TP_FIR_LEN,
                    TP_INTERPOLATE_FACTOR,
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
                    USE_WINDOW_API> : public kernelFilterClass<TT_DATA,
                                                               TT_COEFF,
                                                               TP_FIR_LEN,
                                                               TP_INTERPOLATE_FACTOR,
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
                                                               USE_WINDOW_API> {
   public:
    // Constructor
    fir_resampler()
        : kernelFilterClass<TT_DATA,
                            TT_COEFF,
                            TP_FIR_LEN,
                            TP_INTERPOLATE_FACTOR,
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
                            USE_WINDOW_API>() {}
    fir_resampler(const TT_COEFF (&taps)[TP_FIR_LEN])
        : kernelFilterClass<TT_DATA,
                            TT_COEFF,
                            TP_FIR_LEN,
                            TP_INTERPOLATE_FACTOR,
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
                            USE_WINDOW_API>(taps) {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fir_resampler::filter); }

    // FIR
    void filter(input_window<TT_DATA>* inWindow, input_stream_cacc48* inCascade, output_window<TT_DATA>* outWindow);
};

// Partially specialized classes for cascaded interface - final kernel. Reloadable coefficients, dual output
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_INTERPOLATE_FACTOR,
          unsigned int TP_DECIMATE_FACTOR,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_FIR_RANGE_LEN,
          unsigned int TP_KERNEL_POSITION,
          unsigned int TP_CASC_LEN>

class fir_resampler<TT_DATA,
                    TT_COEFF,
                    TP_FIR_LEN,
                    TP_INTERPOLATE_FACTOR,
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
                    USE_WINDOW_API> : public kernelFilterClass<TT_DATA,
                                                               TT_COEFF,
                                                               TP_FIR_LEN,
                                                               TP_INTERPOLATE_FACTOR,
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
                                                               USE_WINDOW_API> {
   public:
    // Constructor
    fir_resampler()
        : kernelFilterClass<TT_DATA,
                            TT_COEFF,
                            TP_FIR_LEN,
                            TP_INTERPOLATE_FACTOR,
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
                            USE_WINDOW_API>() {}
    fir_resampler(const TT_COEFF (&taps)[TP_FIR_LEN])
        : kernelFilterClass<TT_DATA,
                            TT_COEFF,
                            TP_FIR_LEN,
                            TP_INTERPOLATE_FACTOR,
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
                            USE_WINDOW_API>(taps) {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fir_resampler::filter); }

    // FIR
    void filter(input_window<TT_DATA>* inWindow,
                input_stream_cacc48* inCascade,
                output_window<TT_DATA>* outWindow,
                output_window<TT_DATA>* outWindow2);
};

//-----------------------------------------------------------------------------------------------------
// Partially specialized classes for cascaded interface - first kernel. Reloadable coefficients
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_INTERPOLATE_FACTOR,
          unsigned int TP_DECIMATE_FACTOR,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_FIR_RANGE_LEN,
          unsigned int TP_KERNEL_POSITION,
          unsigned int TP_CASC_LEN>

class fir_resampler<TT_DATA,
                    TT_COEFF,
                    TP_FIR_LEN,
                    TP_INTERPOLATE_FACTOR,
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
                    USE_WINDOW_API> : public kernelFilterClass<TT_DATA,
                                                               TT_COEFF,
                                                               TP_FIR_LEN,
                                                               TP_INTERPOLATE_FACTOR,
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
                                                               USE_WINDOW_API> {
   public:
    // Constructor
    fir_resampler()
        : kernelFilterClass<TT_DATA,
                            TT_COEFF,
                            TP_FIR_LEN,
                            TP_INTERPOLATE_FACTOR,
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
                            USE_WINDOW_API>() {}
    fir_resampler(const TT_COEFF (&taps)[TP_FIR_LEN])
        : kernelFilterClass<TT_DATA,
                            TT_COEFF,
                            TP_FIR_LEN,
                            TP_INTERPOLATE_FACTOR,
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
                            USE_WINDOW_API>(taps) {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fir_resampler::filter); }

    // FIR
    void filter(input_window<TT_DATA>* inWindow,
                output_stream_cacc48* outCascade,
                output_window<TT_DATA>* broadcastWindow,
                const TT_COEFF (&inTaps)[TP_FIR_LEN]);
};

//-----------------------------------------------------------------------------------------------------
// Partially specialized classes for cascaded interface - middle kernel. Reeloadable coefficients
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_INTERPOLATE_FACTOR,
          unsigned int TP_DECIMATE_FACTOR,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_FIR_RANGE_LEN,
          unsigned int TP_KERNEL_POSITION,
          unsigned int TP_CASC_LEN>

class fir_resampler<TT_DATA,
                    TT_COEFF,
                    TP_FIR_LEN,
                    TP_INTERPOLATE_FACTOR,
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
                    USE_WINDOW_API> : public kernelFilterClass<TT_DATA,
                                                               TT_COEFF,
                                                               TP_FIR_LEN,
                                                               TP_INTERPOLATE_FACTOR,
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
                                                               USE_WINDOW_API> {
   public:
    // Constructor
    fir_resampler()
        : kernelFilterClass<TT_DATA,
                            TT_COEFF,
                            TP_FIR_LEN,
                            TP_INTERPOLATE_FACTOR,
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
                            USE_WINDOW_API>() {}
    fir_resampler(const TT_COEFF (&taps)[TP_FIR_LEN])
        : kernelFilterClass<TT_DATA,
                            TT_COEFF,
                            TP_FIR_LEN,
                            TP_INTERPOLATE_FACTOR,
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
                            USE_WINDOW_API>(taps) {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fir_resampler::filter); }

    // FIR
    void filter(input_window<TT_DATA>* inWindow,
                input_stream_cacc48* inCascade,
                output_stream_cacc48* outCascade,
                output_window<TT_DATA>* broadcastWindow);
};

// ----------------------------------------------------------------------------
// ---------------------------------- STREAM ----------------------------------
// ----------------------------------------------------------------------------

//-----------------------------------------------------------------------------------------------------
// Single kernel base specialization. No cascade ports. Static coefficients

// Single kernel specialization. No cascade ports. Static coefficients, dual output
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_INTERPOLATE_FACTOR,
          unsigned int TP_DECIMATE_FACTOR,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_FIR_RANGE_LEN>

class fir_resampler<TT_DATA,
                    TT_COEFF,
                    TP_FIR_LEN,
                    TP_INTERPOLATE_FACTOR,
                    TP_DECIMATE_FACTOR,
                    TP_SHIFT,
                    TP_RND,
                    TP_INPUT_WINDOW_VSIZE,
                    false,
                    false,
                    TP_FIR_RANGE_LEN,
                    0,
                    1,
                    USE_COEFF_RELOAD_FALSE,
                    1,
                    DUAL_IP_SINGLE,
                    USE_STREAM_API> : public kernelFilterClass<TT_DATA,
                                                               TT_COEFF,
                                                               TP_FIR_LEN,
                                                               TP_INTERPOLATE_FACTOR,
                                                               TP_DECIMATE_FACTOR,
                                                               TP_SHIFT,
                                                               TP_RND,
                                                               TP_INPUT_WINDOW_VSIZE,
                                                               false,
                                                               false,
                                                               TP_FIR_RANGE_LEN,
                                                               0,
                                                               1,
                                                               USE_COEFF_RELOAD_FALSE,
                                                               1,
                                                               DUAL_IP_SINGLE,
                                                               USE_STREAM_API> {
   public:
    // Constructor
    fir_resampler(const TT_COEFF (&taps)[TP_FIR_LEN])
        : kernelFilterClass<TT_DATA,
                            TT_COEFF,
                            TP_FIR_LEN,
                            TP_INTERPOLATE_FACTOR,
                            TP_DECIMATE_FACTOR,
                            TP_SHIFT,
                            TP_RND,
                            TP_INPUT_WINDOW_VSIZE,
                            false,
                            false,
                            TP_FIR_RANGE_LEN,
                            0,
                            1,
                            USE_COEFF_RELOAD_FALSE,
                            1,
                            DUAL_IP_SINGLE,
                            USE_STREAM_API>(taps) {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fir_resampler::filter); }

    // FIR
    void filter(input_stream<TT_DATA>* inStream, output_stream<TT_DATA>* outStream);
};
// Single kernel specialization. No cascade ports. Static coefficients, dual output
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_INTERPOLATE_FACTOR,
          unsigned int TP_DECIMATE_FACTOR,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_FIR_RANGE_LEN>

class fir_resampler<TT_DATA,
                    TT_COEFF,
                    TP_FIR_LEN,
                    TP_INTERPOLATE_FACTOR,
                    TP_DECIMATE_FACTOR,
                    TP_SHIFT,
                    TP_RND,
                    TP_INPUT_WINDOW_VSIZE,
                    false,
                    false,
                    TP_FIR_RANGE_LEN,
                    0,
                    1,
                    USE_COEFF_RELOAD_FALSE,
                    2,
                    DUAL_IP_SINGLE,
                    USE_STREAM_API> : public kernelFilterClass<TT_DATA,
                                                               TT_COEFF,
                                                               TP_FIR_LEN,
                                                               TP_INTERPOLATE_FACTOR,
                                                               TP_DECIMATE_FACTOR,
                                                               TP_SHIFT,
                                                               TP_RND,
                                                               TP_INPUT_WINDOW_VSIZE,
                                                               false,
                                                               false,
                                                               TP_FIR_RANGE_LEN,
                                                               0,
                                                               1,
                                                               USE_COEFF_RELOAD_FALSE,
                                                               2,
                                                               DUAL_IP_SINGLE,
                                                               USE_STREAM_API> {
   public:
    // Constructor
    fir_resampler(const TT_COEFF (&taps)[TP_FIR_LEN])
        : kernelFilterClass<TT_DATA,
                            TT_COEFF,
                            TP_FIR_LEN,
                            TP_INTERPOLATE_FACTOR,
                            TP_DECIMATE_FACTOR,
                            TP_SHIFT,
                            TP_RND,
                            TP_INPUT_WINDOW_VSIZE,
                            false,
                            false,
                            TP_FIR_RANGE_LEN,
                            0,
                            1,
                            USE_COEFF_RELOAD_FALSE,
                            2,
                            DUAL_IP_SINGLE,
                            USE_STREAM_API>(taps) {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fir_resampler::filter); }

    // FIR
    void filter(input_stream<TT_DATA>* inStream, output_stream<TT_DATA>* outStream, output_stream<TT_DATA>* outStream2);
};

//-----------------------------------------------------------------------------------------------------
// Single kernel specialization. No cascade ports. Using coefficient reload, single output
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_INTERPOLATE_FACTOR,
          unsigned int TP_DECIMATE_FACTOR,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_FIR_RANGE_LEN,
          unsigned int TP_KERNEL_POSITION,
          unsigned int TP_CASC_LEN>

class fir_resampler<TT_DATA,
                    TT_COEFF,
                    TP_FIR_LEN,
                    TP_INTERPOLATE_FACTOR,
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
                    USE_STREAM_API> : public kernelFilterClass<TT_DATA,
                                                               TT_COEFF,
                                                               TP_FIR_LEN,
                                                               TP_INTERPOLATE_FACTOR,
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
                                                               USE_STREAM_API> {
   public:
    // Constructor
    fir_resampler()
        : kernelFilterClass<TT_DATA,
                            TT_COEFF,
                            TP_FIR_LEN,
                            TP_INTERPOLATE_FACTOR,
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
                            USE_STREAM_API>() {}
    fir_resampler(const TT_COEFF (&taps)[TP_FIR_LEN])
        : kernelFilterClass<TT_DATA,
                            TT_COEFF,
                            TP_FIR_LEN,
                            TP_INTERPOLATE_FACTOR,
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
                            USE_STREAM_API>(taps) {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fir_resampler::filter); }

    // FIR
    void filter(input_stream<TT_DATA>* inStream,
                output_stream<TT_DATA>* outStream,
                const TT_COEFF (&inTaps)[TP_FIR_LEN]);
};

// Single kernel specialization. No cascade ports. Using coefficient reload, dual output
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_INTERPOLATE_FACTOR,
          unsigned int TP_DECIMATE_FACTOR,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_FIR_RANGE_LEN,
          unsigned int TP_KERNEL_POSITION,
          unsigned int TP_CASC_LEN>

class fir_resampler<TT_DATA,
                    TT_COEFF,
                    TP_FIR_LEN,
                    TP_INTERPOLATE_FACTOR,
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
                    USE_STREAM_API> : public kernelFilterClass<TT_DATA,
                                                               TT_COEFF,
                                                               TP_FIR_LEN,
                                                               TP_INTERPOLATE_FACTOR,
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
                                                               USE_STREAM_API> {
   public:
    // Constructor
    fir_resampler()
        : kernelFilterClass<TT_DATA,
                            TT_COEFF,
                            TP_FIR_LEN,
                            TP_INTERPOLATE_FACTOR,
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
                            USE_STREAM_API>() {}
    fir_resampler(const TT_COEFF (&taps)[TP_FIR_LEN])
        : kernelFilterClass<TT_DATA,
                            TT_COEFF,
                            TP_FIR_LEN,
                            TP_INTERPOLATE_FACTOR,
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
                            USE_STREAM_API>(taps) {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fir_resampler::filter); }

    // FIR
    void filter(input_stream<TT_DATA>* inStream,
                output_stream<TT_DATA>* outStream,
                output_stream<TT_DATA>* outStream2,
                const TT_COEFF (&inTaps)[TP_FIR_LEN]);
};

//-----------------------------------------------------------------------------------------------------
// Partially specialized classes for cascaded interface - final kernel. Static coefficients single output
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_INTERPOLATE_FACTOR,
          unsigned int TP_DECIMATE_FACTOR,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_FIR_RANGE_LEN,
          unsigned int TP_KERNEL_POSITION,
          unsigned int TP_CASC_LEN>

class fir_resampler<TT_DATA,
                    TT_COEFF,
                    TP_FIR_LEN,
                    TP_INTERPOLATE_FACTOR,
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
                    USE_STREAM_API> : public kernelFilterClass<TT_DATA,
                                                               TT_COEFF,
                                                               TP_FIR_LEN,
                                                               TP_INTERPOLATE_FACTOR,
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
                                                               USE_STREAM_API> {
   public:
    // Constructor
    fir_resampler(const TT_COEFF (&taps)[TP_FIR_LEN])
        : kernelFilterClass<TT_DATA,
                            TT_COEFF,
                            TP_FIR_LEN,
                            TP_INTERPOLATE_FACTOR,
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
                            USE_STREAM_API>(taps) {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fir_resampler::filter); }

    // FIR
    void filter(input_stream<TT_DATA>* inStream, input_stream_cacc48* inCascade, output_stream<TT_DATA>* outStream);
};

// Partially specialized classes for cascaded interface - final kernel. Static coefficients, dual output
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_INTERPOLATE_FACTOR,
          unsigned int TP_DECIMATE_FACTOR,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_FIR_RANGE_LEN,
          unsigned int TP_KERNEL_POSITION,
          unsigned int TP_CASC_LEN>

class fir_resampler<TT_DATA,
                    TT_COEFF,
                    TP_FIR_LEN,
                    TP_INTERPOLATE_FACTOR,
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
                    USE_STREAM_API> : public kernelFilterClass<TT_DATA,
                                                               TT_COEFF,
                                                               TP_FIR_LEN,
                                                               TP_INTERPOLATE_FACTOR,
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
                                                               USE_STREAM_API> {
   public:
    // Constructor
    fir_resampler(const TT_COEFF (&taps)[TP_FIR_LEN])
        : kernelFilterClass<TT_DATA,
                            TT_COEFF,
                            TP_FIR_LEN,
                            TP_INTERPOLATE_FACTOR,
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
                            USE_STREAM_API>(taps) {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fir_resampler::filter); }

    // FIR
    void filter(input_stream<TT_DATA>* inStream,
                input_stream_cacc48* inCascade,
                output_stream<TT_DATA>* outStream,
                output_stream<TT_DATA>* outStream2);
};

//-----------------------------------------------------------------------------------------------------
// Partially specialized classes for cascaded interface - first kernel. Static coefficients
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_INTERPOLATE_FACTOR,
          unsigned int TP_DECIMATE_FACTOR,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_FIR_RANGE_LEN,
          unsigned int TP_KERNEL_POSITION,
          unsigned int TP_CASC_LEN>

class fir_resampler<TT_DATA,
                    TT_COEFF,
                    TP_FIR_LEN,
                    TP_INTERPOLATE_FACTOR,
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
                    USE_STREAM_API> : public kernelFilterClass<TT_DATA,
                                                               TT_COEFF,
                                                               TP_FIR_LEN,
                                                               TP_INTERPOLATE_FACTOR,
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
                                                               USE_STREAM_API> {
   public:
    // Constructor
    fir_resampler(const TT_COEFF (&taps)[TP_FIR_LEN])
        : kernelFilterClass<TT_DATA,
                            TT_COEFF,
                            TP_FIR_LEN,
                            TP_INTERPOLATE_FACTOR,
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
                            USE_STREAM_API>(taps) {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fir_resampler::filter); }

    // FIR
    void filter(input_stream<TT_DATA>* inStream, output_stream_cacc48* outCascade);
};

//-----------------------------------------------------------------------------------------------------
// Partially specialized classes for cascaded interface - middle kernel. Static coefficients
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_INTERPOLATE_FACTOR,
          unsigned int TP_DECIMATE_FACTOR,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_FIR_RANGE_LEN,
          unsigned int TP_KERNEL_POSITION,
          unsigned int TP_CASC_LEN>

class fir_resampler<TT_DATA,
                    TT_COEFF,
                    TP_FIR_LEN,
                    TP_INTERPOLATE_FACTOR,
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
                    USE_STREAM_API> : public kernelFilterClass<TT_DATA,
                                                               TT_COEFF,
                                                               TP_FIR_LEN,
                                                               TP_INTERPOLATE_FACTOR,
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
                                                               USE_STREAM_API> {
   public:
    // Constructor
    fir_resampler(const TT_COEFF (&taps)[TP_FIR_LEN])
        : kernelFilterClass<TT_DATA,
                            TT_COEFF,
                            TP_FIR_LEN,
                            TP_INTERPOLATE_FACTOR,
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
                            USE_STREAM_API>(taps) {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fir_resampler::filter); }

    // FIR
    void filter(input_stream<TT_DATA>* inStream, input_stream_cacc48* inCascade, output_stream_cacc48* outCascade);
};

//-----------------------------------------------------------------------------------------------------
// Partially specialized classes for cascaded interface - final kernel. Reloadable coefficients
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_INTERPOLATE_FACTOR,
          unsigned int TP_DECIMATE_FACTOR,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_FIR_RANGE_LEN,
          unsigned int TP_KERNEL_POSITION,
          unsigned int TP_CASC_LEN>

class fir_resampler<TT_DATA,
                    TT_COEFF,
                    TP_FIR_LEN,
                    TP_INTERPOLATE_FACTOR,
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
                    USE_STREAM_API> : public kernelFilterClass<TT_DATA,
                                                               TT_COEFF,
                                                               TP_FIR_LEN,
                                                               TP_INTERPOLATE_FACTOR,
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
                                                               USE_STREAM_API> {
   public:
    // Constructor
    fir_resampler()
        : kernelFilterClass<TT_DATA,
                            TT_COEFF,
                            TP_FIR_LEN,
                            TP_INTERPOLATE_FACTOR,
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
                            USE_STREAM_API>() {}
    fir_resampler(const TT_COEFF (&taps)[TP_FIR_LEN])
        : kernelFilterClass<TT_DATA,
                            TT_COEFF,
                            TP_FIR_LEN,
                            TP_INTERPOLATE_FACTOR,
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
                            USE_STREAM_API>(taps) {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fir_resampler::filter); }

    // FIR
    void filter(input_stream<TT_DATA>* inStream, input_stream_cacc48* inCascade, output_stream<TT_DATA>* outStream);
};

// Partially specialized classes for cascaded interface - final kernel. Reloadable coefficients, dual output
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_INTERPOLATE_FACTOR,
          unsigned int TP_DECIMATE_FACTOR,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_FIR_RANGE_LEN,
          unsigned int TP_KERNEL_POSITION,
          unsigned int TP_CASC_LEN>

class fir_resampler<TT_DATA,
                    TT_COEFF,
                    TP_FIR_LEN,
                    TP_INTERPOLATE_FACTOR,
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
                    USE_STREAM_API> : public kernelFilterClass<TT_DATA,
                                                               TT_COEFF,
                                                               TP_FIR_LEN,
                                                               TP_INTERPOLATE_FACTOR,
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
                                                               USE_STREAM_API> {
   public:
    // Constructor
    fir_resampler()
        : kernelFilterClass<TT_DATA,
                            TT_COEFF,
                            TP_FIR_LEN,
                            TP_INTERPOLATE_FACTOR,
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
                            USE_STREAM_API>() {}
    fir_resampler(const TT_COEFF (&taps)[TP_FIR_LEN])
        : kernelFilterClass<TT_DATA,
                            TT_COEFF,
                            TP_FIR_LEN,
                            TP_INTERPOLATE_FACTOR,
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
                            USE_STREAM_API>(taps) {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fir_resampler::filter); }

    // FIR
    void filter(input_stream<TT_DATA>* inStream,
                input_stream_cacc48* inCascade,
                output_stream<TT_DATA>* outStream,
                output_stream<TT_DATA>* outStream2);
};

//-----------------------------------------------------------------------------------------------------
// Partially specialized classes for cascaded interface - first kernel. Reloadable coefficients
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_INTERPOLATE_FACTOR,
          unsigned int TP_DECIMATE_FACTOR,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_FIR_RANGE_LEN,
          unsigned int TP_KERNEL_POSITION,
          unsigned int TP_CASC_LEN>

class fir_resampler<TT_DATA,
                    TT_COEFF,
                    TP_FIR_LEN,
                    TP_INTERPOLATE_FACTOR,
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
                    USE_STREAM_API> : public kernelFilterClass<TT_DATA,
                                                               TT_COEFF,
                                                               TP_FIR_LEN,
                                                               TP_INTERPOLATE_FACTOR,
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
                                                               USE_STREAM_API> {
   public:
    // Constructor
    fir_resampler()
        : kernelFilterClass<TT_DATA,
                            TT_COEFF,
                            TP_FIR_LEN,
                            TP_INTERPOLATE_FACTOR,
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
                            USE_STREAM_API>() {}
    fir_resampler(const TT_COEFF (&taps)[TP_FIR_LEN])
        : kernelFilterClass<TT_DATA,
                            TT_COEFF,
                            TP_FIR_LEN,
                            TP_INTERPOLATE_FACTOR,
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
                            USE_STREAM_API>(taps) {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fir_resampler::filter); }

    // FIR
    void filter(input_stream<TT_DATA>* inStream,
                output_stream_cacc48* outCascade,
                const TT_COEFF (&inTaps)[TP_FIR_LEN]);
};

//-----------------------------------------------------------------------------------------------------
// Partially specialized classes for cascaded interface - middle kernel. Reeloadable coefficients
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_INTERPOLATE_FACTOR,
          unsigned int TP_DECIMATE_FACTOR,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_FIR_RANGE_LEN,
          unsigned int TP_KERNEL_POSITION,
          unsigned int TP_CASC_LEN>

class fir_resampler<TT_DATA,
                    TT_COEFF,
                    TP_FIR_LEN,
                    TP_INTERPOLATE_FACTOR,
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
                    USE_STREAM_API> : public kernelFilterClass<TT_DATA,
                                                               TT_COEFF,
                                                               TP_FIR_LEN,
                                                               TP_INTERPOLATE_FACTOR,
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
                                                               USE_STREAM_API> {
   public:
    // Constructor
    fir_resampler()
        : kernelFilterClass<TT_DATA,
                            TT_COEFF,
                            TP_FIR_LEN,
                            TP_INTERPOLATE_FACTOR,
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
                            USE_STREAM_API>() {}
    fir_resampler(const TT_COEFF (&taps)[TP_FIR_LEN])
        : kernelFilterClass<TT_DATA,
                            TT_COEFF,
                            TP_FIR_LEN,
                            TP_INTERPOLATE_FACTOR,
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
                            USE_STREAM_API>(taps) {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fir_resampler::filter); }

    // FIR
    void filter(input_stream<TT_DATA>* inStream, input_stream_cacc48* inCascade, output_stream_cacc48* outCascade);
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
          unsigned int TP_INTERPOLATE_FACTOR,
          unsigned int TP_DECIMATE_FACTOR,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_FIR_RANGE_LEN>

class fir_resampler<TT_DATA,
                    TT_COEFF,
                    TP_FIR_LEN,
                    TP_INTERPOLATE_FACTOR,
                    TP_DECIMATE_FACTOR,
                    TP_SHIFT,
                    TP_RND,
                    TP_INPUT_WINDOW_VSIZE,
                    false,
                    false,
                    TP_FIR_RANGE_LEN,
                    0,
                    1,
                    USE_COEFF_RELOAD_FALSE,
                    1,
                    DUAL_IP_DUAL,
                    USE_STREAM_API> : public kernelFilterClass<TT_DATA,
                                                               TT_COEFF,
                                                               TP_FIR_LEN,
                                                               TP_INTERPOLATE_FACTOR,
                                                               TP_DECIMATE_FACTOR,
                                                               TP_SHIFT,
                                                               TP_RND,
                                                               TP_INPUT_WINDOW_VSIZE,
                                                               false,
                                                               false,
                                                               TP_FIR_RANGE_LEN,
                                                               0,
                                                               1,
                                                               USE_COEFF_RELOAD_FALSE,
                                                               1,
                                                               DUAL_IP_DUAL,
                                                               USE_STREAM_API> {
   public:
    // Constructor
    fir_resampler(const TT_COEFF (&taps)[TP_FIR_LEN])
        : kernelFilterClass<TT_DATA,
                            TT_COEFF,
                            TP_FIR_LEN,
                            TP_INTERPOLATE_FACTOR,
                            TP_DECIMATE_FACTOR,
                            TP_SHIFT,
                            TP_RND,
                            TP_INPUT_WINDOW_VSIZE,
                            false,
                            false,
                            TP_FIR_RANGE_LEN,
                            0,
                            1,
                            USE_COEFF_RELOAD_FALSE,
                            1,
                            DUAL_IP_DUAL,
                            USE_STREAM_API>(taps) {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fir_resampler::filter); }

    // FIR
    void filter(input_stream<TT_DATA>* inStream, input_stream<TT_DATA>* inStream2, output_stream<TT_DATA>* outStream);
};
// Single kernel specialization. No cascade ports. Static coefficients, dual output
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_INTERPOLATE_FACTOR,
          unsigned int TP_DECIMATE_FACTOR,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_FIR_RANGE_LEN>

class fir_resampler<TT_DATA,
                    TT_COEFF,
                    TP_FIR_LEN,
                    TP_INTERPOLATE_FACTOR,
                    TP_DECIMATE_FACTOR,
                    TP_SHIFT,
                    TP_RND,
                    TP_INPUT_WINDOW_VSIZE,
                    false,
                    false,
                    TP_FIR_RANGE_LEN,
                    0,
                    1,
                    USE_COEFF_RELOAD_FALSE,
                    2,
                    DUAL_IP_DUAL,
                    USE_STREAM_API> : public kernelFilterClass<TT_DATA,
                                                               TT_COEFF,
                                                               TP_FIR_LEN,
                                                               TP_INTERPOLATE_FACTOR,
                                                               TP_DECIMATE_FACTOR,
                                                               TP_SHIFT,
                                                               TP_RND,
                                                               TP_INPUT_WINDOW_VSIZE,
                                                               false,
                                                               false,
                                                               TP_FIR_RANGE_LEN,
                                                               0,
                                                               1,
                                                               USE_COEFF_RELOAD_FALSE,
                                                               2,
                                                               DUAL_IP_DUAL,
                                                               USE_STREAM_API> {
   public:
    // Constructor
    fir_resampler(const TT_COEFF (&taps)[TP_FIR_LEN])
        : kernelFilterClass<TT_DATA,
                            TT_COEFF,
                            TP_FIR_LEN,
                            TP_INTERPOLATE_FACTOR,
                            TP_DECIMATE_FACTOR,
                            TP_SHIFT,
                            TP_RND,
                            TP_INPUT_WINDOW_VSIZE,
                            false,
                            false,
                            TP_FIR_RANGE_LEN,
                            0,
                            1,
                            USE_COEFF_RELOAD_FALSE,
                            2,
                            DUAL_IP_DUAL,
                            USE_STREAM_API>(taps) {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fir_resampler::filter); }

    // FIR
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
          unsigned int TP_INTERPOLATE_FACTOR,
          unsigned int TP_DECIMATE_FACTOR,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_FIR_RANGE_LEN,
          unsigned int TP_KERNEL_POSITION,
          unsigned int TP_CASC_LEN>

class fir_resampler<TT_DATA,
                    TT_COEFF,
                    TP_FIR_LEN,
                    TP_INTERPOLATE_FACTOR,
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
                    USE_STREAM_API> : public kernelFilterClass<TT_DATA,
                                                               TT_COEFF,
                                                               TP_FIR_LEN,
                                                               TP_INTERPOLATE_FACTOR,
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
                                                               USE_STREAM_API> {
   public:
    // Constructor
    fir_resampler()
        : kernelFilterClass<TT_DATA,
                            TT_COEFF,
                            TP_FIR_LEN,
                            TP_INTERPOLATE_FACTOR,
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
                            USE_STREAM_API>() {}
    fir_resampler(const TT_COEFF (&taps)[TP_FIR_LEN])
        : kernelFilterClass<TT_DATA,
                            TT_COEFF,
                            TP_FIR_LEN,
                            TP_INTERPOLATE_FACTOR,
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
                            USE_STREAM_API>(taps) {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fir_resampler::filter); }

    // FIR
    void filter(input_stream<TT_DATA>* inStream,
                input_stream<TT_DATA>* inStream2,
                output_stream<TT_DATA>* outStream,
                const TT_COEFF (&inTaps)[TP_FIR_LEN]);
};

// Single kernel specialization. No cascade ports. Using coefficient reload, dual output
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_INTERPOLATE_FACTOR,
          unsigned int TP_DECIMATE_FACTOR,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_FIR_RANGE_LEN,
          unsigned int TP_KERNEL_POSITION,
          unsigned int TP_CASC_LEN>

class fir_resampler<TT_DATA,
                    TT_COEFF,
                    TP_FIR_LEN,
                    TP_INTERPOLATE_FACTOR,
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
                    USE_STREAM_API> : public kernelFilterClass<TT_DATA,
                                                               TT_COEFF,
                                                               TP_FIR_LEN,
                                                               TP_INTERPOLATE_FACTOR,
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
                                                               USE_STREAM_API> {
   public:
    // Constructor
    fir_resampler()
        : kernelFilterClass<TT_DATA,
                            TT_COEFF,
                            TP_FIR_LEN,
                            TP_INTERPOLATE_FACTOR,
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
                            USE_STREAM_API>() {}
    fir_resampler(const TT_COEFF (&taps)[TP_FIR_LEN])
        : kernelFilterClass<TT_DATA,
                            TT_COEFF,
                            TP_FIR_LEN,
                            TP_INTERPOLATE_FACTOR,
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
                            USE_STREAM_API>(taps) {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fir_resampler::filter); }

    // FIR
    void filter(input_stream<TT_DATA>* inStream,
                input_stream<TT_DATA>* inStream2,
                output_stream<TT_DATA>* outStream,
                output_stream<TT_DATA>* outStream2,
                const TT_COEFF (&inTaps)[TP_FIR_LEN]);
};

//-----------------------------------------------------------------------------------------------------
// Partially specialized classes for cascaded interface - final kernel. Static coefficients single output
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_INTERPOLATE_FACTOR,
          unsigned int TP_DECIMATE_FACTOR,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_FIR_RANGE_LEN,
          unsigned int TP_KERNEL_POSITION,
          unsigned int TP_CASC_LEN>

class fir_resampler<TT_DATA,
                    TT_COEFF,
                    TP_FIR_LEN,
                    TP_INTERPOLATE_FACTOR,
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
                    USE_STREAM_API> : public kernelFilterClass<TT_DATA,
                                                               TT_COEFF,
                                                               TP_FIR_LEN,
                                                               TP_INTERPOLATE_FACTOR,
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
                                                               USE_STREAM_API> {
   public:
    // Constructor
    fir_resampler(const TT_COEFF (&taps)[TP_FIR_LEN])
        : kernelFilterClass<TT_DATA,
                            TT_COEFF,
                            TP_FIR_LEN,
                            TP_INTERPOLATE_FACTOR,
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
                            USE_STREAM_API>(taps) {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fir_resampler::filter); }

    // FIR
    void filter(input_stream<TT_DATA>* inStream,
                input_stream<TT_DATA>* inStream2,
                input_stream_cacc48* inCascade,
                output_stream<TT_DATA>* outStream);
};

// Partially specialized classes for cascaded interface - final kernel. Static coefficients, dual output
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_INTERPOLATE_FACTOR,
          unsigned int TP_DECIMATE_FACTOR,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_FIR_RANGE_LEN,
          unsigned int TP_KERNEL_POSITION,
          unsigned int TP_CASC_LEN>

class fir_resampler<TT_DATA,
                    TT_COEFF,
                    TP_FIR_LEN,
                    TP_INTERPOLATE_FACTOR,
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
                    USE_STREAM_API> : public kernelFilterClass<TT_DATA,
                                                               TT_COEFF,
                                                               TP_FIR_LEN,
                                                               TP_INTERPOLATE_FACTOR,
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
                                                               USE_STREAM_API> {
   public:
    // Constructor
    fir_resampler(const TT_COEFF (&taps)[TP_FIR_LEN])
        : kernelFilterClass<TT_DATA,
                            TT_COEFF,
                            TP_FIR_LEN,
                            TP_INTERPOLATE_FACTOR,
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
                            USE_STREAM_API>(taps) {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fir_resampler::filter); }

    // FIR
    void filter(input_stream<TT_DATA>* inStream,
                input_stream<TT_DATA>* inStream2,
                input_stream_cacc48* inCascade,
                output_stream<TT_DATA>* outStream,
                output_stream<TT_DATA>* outStream2);
};

//-----------------------------------------------------------------------------------------------------
// Partially specialized classes for cascaded interface - first kernel. Static coefficients
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_INTERPOLATE_FACTOR,
          unsigned int TP_DECIMATE_FACTOR,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_FIR_RANGE_LEN,
          unsigned int TP_KERNEL_POSITION,
          unsigned int TP_CASC_LEN>

class fir_resampler<TT_DATA,
                    TT_COEFF,
                    TP_FIR_LEN,
                    TP_INTERPOLATE_FACTOR,
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
                    USE_STREAM_API> : public kernelFilterClass<TT_DATA,
                                                               TT_COEFF,
                                                               TP_FIR_LEN,
                                                               TP_INTERPOLATE_FACTOR,
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
                                                               USE_STREAM_API> {
   public:
    // Constructor
    fir_resampler(const TT_COEFF (&taps)[TP_FIR_LEN])
        : kernelFilterClass<TT_DATA,
                            TT_COEFF,
                            TP_FIR_LEN,
                            TP_INTERPOLATE_FACTOR,
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
                            USE_STREAM_API>(taps) {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fir_resampler::filter); }

    // FIR
    void filter(input_stream<TT_DATA>* inStream, input_stream<TT_DATA>* inStream2, output_stream_cacc48* outCascade);
};

//-----------------------------------------------------------------------------------------------------
// Partially specialized classes for cascaded interface - middle kernel. Static coefficients
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_INTERPOLATE_FACTOR,
          unsigned int TP_DECIMATE_FACTOR,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_FIR_RANGE_LEN,
          unsigned int TP_KERNEL_POSITION,
          unsigned int TP_CASC_LEN>

class fir_resampler<TT_DATA,
                    TT_COEFF,
                    TP_FIR_LEN,
                    TP_INTERPOLATE_FACTOR,
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
                    USE_STREAM_API> : public kernelFilterClass<TT_DATA,
                                                               TT_COEFF,
                                                               TP_FIR_LEN,
                                                               TP_INTERPOLATE_FACTOR,
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
                                                               USE_STREAM_API> {
   public:
    // Constructor
    fir_resampler(const TT_COEFF (&taps)[TP_FIR_LEN])
        : kernelFilterClass<TT_DATA,
                            TT_COEFF,
                            TP_FIR_LEN,
                            TP_INTERPOLATE_FACTOR,
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
                            USE_STREAM_API>(taps) {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fir_resampler::filter); }

    // FIR
    void filter(input_stream<TT_DATA>* inStream,
                input_stream<TT_DATA>* inStream2,
                input_stream_cacc48* inCascade,
                output_stream_cacc48* outCascade);
};

//-----------------------------------------------------------------------------------------------------
// Partially specialized classes for cascaded interface - final kernel. Reloadable coefficients
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_INTERPOLATE_FACTOR,
          unsigned int TP_DECIMATE_FACTOR,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_FIR_RANGE_LEN,
          unsigned int TP_KERNEL_POSITION,
          unsigned int TP_CASC_LEN>

class fir_resampler<TT_DATA,
                    TT_COEFF,
                    TP_FIR_LEN,
                    TP_INTERPOLATE_FACTOR,
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
                    USE_STREAM_API> : public kernelFilterClass<TT_DATA,
                                                               TT_COEFF,
                                                               TP_FIR_LEN,
                                                               TP_INTERPOLATE_FACTOR,
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
                                                               USE_STREAM_API> {
   public:
    // Constructor
    fir_resampler()
        : kernelFilterClass<TT_DATA,
                            TT_COEFF,
                            TP_FIR_LEN,
                            TP_INTERPOLATE_FACTOR,
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
                            USE_STREAM_API>() {}
    fir_resampler(const TT_COEFF (&taps)[TP_FIR_LEN])
        : kernelFilterClass<TT_DATA,
                            TT_COEFF,
                            TP_FIR_LEN,
                            TP_INTERPOLATE_FACTOR,
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
                            USE_STREAM_API>(taps) {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fir_resampler::filter); }

    // FIR
    void filter(input_stream<TT_DATA>* inStream,
                input_stream<TT_DATA>* inStream2,
                input_stream_cacc48* inCascade,
                output_stream<TT_DATA>* outStream);
};

// Partially specialized classes for cascaded interface - final kernel. Reloadable coefficients, dual output
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_INTERPOLATE_FACTOR,
          unsigned int TP_DECIMATE_FACTOR,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_FIR_RANGE_LEN,
          unsigned int TP_KERNEL_POSITION,
          unsigned int TP_CASC_LEN>

class fir_resampler<TT_DATA,
                    TT_COEFF,
                    TP_FIR_LEN,
                    TP_INTERPOLATE_FACTOR,
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
                    USE_STREAM_API> : public kernelFilterClass<TT_DATA,
                                                               TT_COEFF,
                                                               TP_FIR_LEN,
                                                               TP_INTERPOLATE_FACTOR,
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
                                                               USE_STREAM_API> {
   public:
    // Constructor
    fir_resampler()
        : kernelFilterClass<TT_DATA,
                            TT_COEFF,
                            TP_FIR_LEN,
                            TP_INTERPOLATE_FACTOR,
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
                            USE_STREAM_API>() {}
    fir_resampler(const TT_COEFF (&taps)[TP_FIR_LEN])
        : kernelFilterClass<TT_DATA,
                            TT_COEFF,
                            TP_FIR_LEN,
                            TP_INTERPOLATE_FACTOR,
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
                            USE_STREAM_API>(taps) {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fir_resampler::filter); }

    // FIR
    void filter(input_stream<TT_DATA>* inStream,
                input_stream<TT_DATA>* inStream2,
                input_stream_cacc48* inCascade,
                output_stream<TT_DATA>* outStream,
                output_stream<TT_DATA>* outStream2);
};

//-----------------------------------------------------------------------------------------------------
// Partially specialized classes for cascaded interface - first kernel. Reloadable coefficients
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_INTERPOLATE_FACTOR,
          unsigned int TP_DECIMATE_FACTOR,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_FIR_RANGE_LEN,
          unsigned int TP_KERNEL_POSITION,
          unsigned int TP_CASC_LEN>

class fir_resampler<TT_DATA,
                    TT_COEFF,
                    TP_FIR_LEN,
                    TP_INTERPOLATE_FACTOR,
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
                    USE_STREAM_API> : public kernelFilterClass<TT_DATA,
                                                               TT_COEFF,
                                                               TP_FIR_LEN,
                                                               TP_INTERPOLATE_FACTOR,
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
                                                               USE_STREAM_API> {
   public:
    // Constructor
    fir_resampler()
        : kernelFilterClass<TT_DATA,
                            TT_COEFF,
                            TP_FIR_LEN,
                            TP_INTERPOLATE_FACTOR,
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
                            USE_STREAM_API>() {}
    fir_resampler(const TT_COEFF (&taps)[TP_FIR_LEN])
        : kernelFilterClass<TT_DATA,
                            TT_COEFF,
                            TP_FIR_LEN,
                            TP_INTERPOLATE_FACTOR,
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
                            USE_STREAM_API>(taps) {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fir_resampler::filter); }

    // FIR
    void filter(input_stream<TT_DATA>* inStream,
                input_stream<TT_DATA>* inStream2,
                output_stream_cacc48* outCascade,
                const TT_COEFF (&inTaps)[TP_FIR_LEN]);
};

//-----------------------------------------------------------------------------------------------------
// Partially specialized classes for cascaded interface - middle kernel. Reeloadable coefficients
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_INTERPOLATE_FACTOR,
          unsigned int TP_DECIMATE_FACTOR,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_FIR_RANGE_LEN,
          unsigned int TP_KERNEL_POSITION,
          unsigned int TP_CASC_LEN>

class fir_resampler<TT_DATA,
                    TT_COEFF,
                    TP_FIR_LEN,
                    TP_INTERPOLATE_FACTOR,
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
                    USE_STREAM_API> : public kernelFilterClass<TT_DATA,
                                                               TT_COEFF,
                                                               TP_FIR_LEN,
                                                               TP_INTERPOLATE_FACTOR,
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
                                                               USE_STREAM_API> {
   public:
    // Constructor
    fir_resampler()
        : kernelFilterClass<TT_DATA,
                            TT_COEFF,
                            TP_FIR_LEN,
                            TP_INTERPOLATE_FACTOR,
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
                            USE_STREAM_API>() {}
    fir_resampler(const TT_COEFF (&taps)[TP_FIR_LEN])
        : kernelFilterClass<TT_DATA,
                            TT_COEFF,
                            TP_FIR_LEN,
                            TP_INTERPOLATE_FACTOR,
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
                            USE_STREAM_API>(taps) {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fir_resampler::filter); }

    // FIR
    void filter(input_stream<TT_DATA>* inStream,
                input_stream<TT_DATA>* inStream2,
                input_stream_cacc48* inCascade,
                output_stream_cacc48* outCascade);
};
}
}
}
}
}
#endif // _DSPLIB_fir_resampler_HPP_
