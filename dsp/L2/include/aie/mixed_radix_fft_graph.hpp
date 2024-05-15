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
#ifndef _DSPLIB_MIXED_RADIX_FFT_GRAPH_HPP_
#define _DSPLIB_MIXED_RADIX_FFT_GRAPH_HPP_

#include <adf.h>
#include <vector>
#include <adf/arch/aie_arch_properties.hpp> //for get_input_streams_core_module()
#include "graph_utils.hpp"                  //for port_array
#include "widget_api_cast.hpp"
#include "mixed_radix_fft.hpp"
#include "fir_utils.hpp" // for CEIL function outside of L2 flow

using namespace ::xf::dsp::aie::widget::api_cast;
using namespace adf;

//#define _MIXED_RADIX_GRAPH_DEBUG_

namespace xf {
namespace dsp {
namespace aie {
namespace fft {
namespace mixed_radix_fft {

// Kernels need to be created by recursion because the template parameters vary by loop index
// The recursion here is complex.
// The entry point is create_casc_kernel. This has a specialization for a CASC_LEN of one as that is also the end of
// recursion.
// If the entry class needs to recurse (CASC_LEN>1) then it instances create_casc_kernel_recur which will call itself
// recursively.
// create_casc_kernel_recur also has a specialization for the end of recursion.
// In each class the tables for twiddle and twiddle pointers are created from the reference of what is passed to them,
// since each kernel needs a unique instance of the memory.
// An optimization would be to supply each kernel only with the twiddles that it will use. At present, each kernel gets
// all twiddles.
// The determination of START_RANK and END_RANK is also recursive.
// On entry, END_RANK is simply the number of RANKs.
// The create_casc_kernel divides the END_RANK by the CASC_LEN (rounded down) to get the number of ranks this kernel
// will perform, so START_RANK = END_RANK - END_RANK/TP_CASC_LEN.
// However, the next kernel's END_RANK is the previous kernel's START_RANK (recursion goes backwards along the cascade
// so that the last kernel in recursion is easily detected as index 0).
// And the CASC_POS passed to this next kernel is the current CASC_POS -1.
// E.g. For 8 ranks and 3 kernels, END_RANK is 8 and CASC_LEN=3, so the last kernel in the chain has START_RANK = 8-8/3
// = 6.
// The next to last kernel will have START_RANK = 6-6/2 = 3.
// The first kernel will have START_RANK = 0 (no need to calc), which the calc would return as 3-3/1 = 0 anyway.
// Hence, the number of ranks per kernel in this example would be 3 for the first 2 and 2 for the last.
//----------------
// TT_DATA handling.
// The 4 specializations of create_casc_kernel effectively boil down to 'last in chain', 'middle in chain', 'start of
// chain' and 'single kernel'.
// Since the input and output of the chain are TT_DATA, but internal connections can be different (t_internaldata), this
// breakdown is convenient
// since each specialization knows its place, so can determine if the kernel in question needs TT_DATA or t_internaldata
// for in and out, plus only TT_DATA
// needs to be passed down recursion.

// middle kernel in chain
template <int TP_DIM,
          typename TT_DATA, // type for I/O
          typename TT_TWIDDLE,
          unsigned int TP_POINT_SIZE,
          unsigned int TP_FFT_NIFFT,
          unsigned int TP_SHIFT,
          unsigned int TP_CASC_LEN,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_WINDOW_VSIZE,
          unsigned int TP_END_RANK,
          unsigned int TP_KTWIDDLETABLESIZE,
          unsigned int TP_KNUMMAXTABLES>
class create_casc_kernel_recur {
   public:
    typedef typename std::conditional<std::is_same<TT_DATA, cint16>::value, cint32_t, TT_DATA>::type T_internalDataType;
    static void create(kernel (&fftKernels)[TP_CASC_LEN], TT_TWIDDLE* twTable, int* twiddlePtrs) {
        static constexpr unsigned int TP_RANKS_PER_KERNEL = TP_END_RANK / TP_DIM;
        static constexpr unsigned int TP_START_RANK = (int)TP_END_RANK - (int)TP_RANKS_PER_KERNEL;

        std::vector<TT_TWIDDLE> m_twiddleTable;
        m_twiddleTable.resize(TP_KTWIDDLETABLESIZE);
        std::vector<int> m_twiddlePtrs; // index in m_twiddleTable of the start of each atomic table
        m_twiddlePtrs.resize(TP_KNUMMAXTABLES);
        for (int i = 0; i < TP_KTWIDDLETABLESIZE; i++) m_twiddleTable[i] = twTable[i];
        for (int i = 0; i < TP_KNUMMAXTABLES; i++) m_twiddlePtrs[i] = twiddlePtrs[i];

        std::vector<T_internalDataType> tmpBuff0;
        std::vector<T_internalDataType> tmpBuff1;
        tmpBuff0.resize(TP_POINT_SIZE);
        tmpBuff1.resize(TP_POINT_SIZE);

        fftKernels[TP_DIM - 1] = kernel::create_object<
            mixed_radix_fft<T_internalDataType, T_internalDataType, TT_TWIDDLE, TP_POINT_SIZE, TP_FFT_NIFFT, TP_SHIFT,
                            TP_RND, TP_SAT, TP_WINDOW_VSIZE, TP_START_RANK, TP_END_RANK> >(
            tmpBuff0, tmpBuff1, m_twiddleTable, m_twiddlePtrs);
        create_casc_kernel_recur<TP_DIM - 1, TT_DATA, TT_TWIDDLE, TP_POINT_SIZE, TP_FFT_NIFFT, TP_SHIFT, TP_CASC_LEN,
                                 TP_RND, TP_SAT, TP_WINDOW_VSIZE, TP_START_RANK, TP_KTWIDDLETABLESIZE,
                                 TP_KNUMMAXTABLES>::create(fftKernels, twTable, twiddlePtrs);

    } // constructor
};    // create_casc_kernel_recur class

// first kernel in chain
template <typename TT_DATA, // type for I/O
          typename TT_TWIDDLE,
          unsigned int TP_POINT_SIZE,
          unsigned int TP_FFT_NIFFT,
          unsigned int TP_SHIFT,
          unsigned int TP_CASC_LEN,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_WINDOW_VSIZE,
          unsigned int TP_END_RANK,
          unsigned int TP_KTWIDDLETABLESIZE,
          unsigned int TP_KNUMMAXTABLES>
class create_casc_kernel_recur<1,
                               TT_DATA,
                               TT_TWIDDLE,
                               TP_POINT_SIZE,
                               TP_FFT_NIFFT,
                               TP_SHIFT,
                               TP_CASC_LEN,
                               TP_RND,
                               TP_SAT,
                               TP_WINDOW_VSIZE,
                               TP_END_RANK,
                               TP_KTWIDDLETABLESIZE,
                               TP_KNUMMAXTABLES> {
   public:
    typedef typename std::conditional<std::is_same<TT_DATA, cint16>::value, cint32_t, TT_DATA>::type T_internalDataType;
    static void create(kernel (&fftKernels)[TP_CASC_LEN], TT_TWIDDLE* twTable, int* twiddlePtrs) {
        static constexpr unsigned int TP_START_RANK = 0;

        std::vector<TT_TWIDDLE> m_twiddleTable;
        m_twiddleTable.resize(TP_KTWIDDLETABLESIZE);
        std::vector<int> m_twiddlePtrs; // index in m_twiddleTable of the start of each atomic table
        m_twiddlePtrs.resize(TP_KNUMMAXTABLES);
        for (int i = 0; i < TP_KTWIDDLETABLESIZE; i++) m_twiddleTable[i] = twTable[i];
        for (int i = 0; i < TP_KNUMMAXTABLES; i++) m_twiddlePtrs[i] = twiddlePtrs[i];

        std::vector<T_internalDataType> tmpBuff0;
        std::vector<T_internalDataType> tmpBuff1;
        tmpBuff0.resize(TP_POINT_SIZE);
        tmpBuff1.resize(TP_POINT_SIZE);

        fftKernels[0] = kernel::create_object<
            mixed_radix_fft<TT_DATA, T_internalDataType, TT_TWIDDLE, TP_POINT_SIZE, TP_FFT_NIFFT, TP_SHIFT, TP_RND,
                            TP_SAT, TP_WINDOW_VSIZE, TP_START_RANK, TP_END_RANK> >(tmpBuff0, tmpBuff1, m_twiddleTable,
                                                                                   m_twiddlePtrs);

    } // constructor
};    // create_casc_kernel_recur class

// end of chain specialization.
template <int TP_DIM,
          typename TT_DATA, // type for I/O
          typename TT_TWIDDLE,
          unsigned int TP_POINT_SIZE,
          unsigned int TP_FFT_NIFFT,
          unsigned int TP_SHIFT,
          unsigned int TP_CASC_LEN,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_WINDOW_VSIZE,
          unsigned int TP_END_RANK,
          unsigned int TP_KTWIDDLETABLESIZE,
          unsigned int TP_KNUMMAXTABLES>
class create_casc_kernel {
   public:
    typedef typename std::conditional<std::is_same<TT_DATA, cint16>::value, cint32_t, TT_DATA>::type T_internalDataType;
    static void create(kernel (&fftKernels)[TP_CASC_LEN], TT_TWIDDLE* twTable, int* twiddlePtrs) {
        static constexpr unsigned int TP_RANKS_PER_KERNEL = TP_END_RANK / (TP_DIM);
        static constexpr unsigned int TP_START_RANK = (int)TP_END_RANK - (int)TP_RANKS_PER_KERNEL;

        std::vector<TT_TWIDDLE> m_twiddleTable;
        m_twiddleTable.resize(TP_KTWIDDLETABLESIZE);
        std::vector<int> m_twiddlePtrs; // index in m_twiddleTable of the start of each atomic table
        m_twiddlePtrs.resize(TP_KNUMMAXTABLES);
        for (int i = 0; i < TP_KTWIDDLETABLESIZE; i++) m_twiddleTable[i] = twTable[i];
        for (int i = 0; i < TP_KNUMMAXTABLES; i++) m_twiddlePtrs[i] = twiddlePtrs[i];

        std::vector<T_internalDataType> tmpBuff0;
        std::vector<T_internalDataType> tmpBuff1;
        tmpBuff0.resize(TP_POINT_SIZE);
        tmpBuff1.resize(TP_POINT_SIZE);

        fftKernels[TP_DIM - 1] = kernel::create_object<
            mixed_radix_fft<T_internalDataType, TT_DATA, TT_TWIDDLE, TP_POINT_SIZE, TP_FFT_NIFFT, TP_SHIFT, TP_RND,
                            TP_SAT, TP_WINDOW_VSIZE, TP_START_RANK, TP_END_RANK> >(tmpBuff0, tmpBuff1, m_twiddleTable,
                                                                                   m_twiddlePtrs);
        create_casc_kernel_recur<TP_DIM - 1, TT_DATA, TT_TWIDDLE, TP_POINT_SIZE, TP_FFT_NIFFT, TP_SHIFT, TP_CASC_LEN,
                                 TP_RND, TP_SAT, TP_WINDOW_VSIZE, TP_START_RANK, TP_KTWIDDLETABLESIZE,
                                 TP_KNUMMAXTABLES>::create(fftKernels, twTable, twiddlePtrs);

    } // constructor
};    // create_casc_kernel class

// Single kernel case
template <typename TT_DATA, // type for I/O
          typename TT_TWIDDLE,
          unsigned int TP_POINT_SIZE,
          unsigned int TP_FFT_NIFFT,
          unsigned int TP_SHIFT,
          unsigned int TP_CASC_LEN,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_WINDOW_VSIZE,
          unsigned int TP_END_RANK,
          unsigned int TP_KTWIDDLETABLESIZE,
          unsigned int TP_KNUMMAXTABLES>
class create_casc_kernel<1,
                         TT_DATA,
                         TT_TWIDDLE,
                         TP_POINT_SIZE,
                         TP_FFT_NIFFT,
                         TP_SHIFT,
                         TP_CASC_LEN,
                         TP_RND,
                         TP_SAT,
                         TP_WINDOW_VSIZE,
                         TP_END_RANK,
                         TP_KTWIDDLETABLESIZE,
                         TP_KNUMMAXTABLES> {
   public:
    typedef typename std::conditional<std::is_same<TT_DATA, cint16>::value, cint32_t, TT_DATA>::type T_internalDataType;
    static void create(kernel (&fftKernels)[TP_CASC_LEN], TT_TWIDDLE* twTable, int* twiddlePtrs) {
        static constexpr unsigned int TP_START_RANK = 0;

        std::vector<TT_TWIDDLE> m_twiddleTable;
        m_twiddleTable.resize(TP_KTWIDDLETABLESIZE);
        std::vector<int> m_twiddlePtrs; // index in m_twiddleTable of the start of each atomic table
        m_twiddlePtrs.resize(TP_KNUMMAXTABLES);
        for (int i = 0; i < TP_KTWIDDLETABLESIZE; i++) m_twiddleTable[i] = twTable[i];
        for (int i = 0; i < TP_KNUMMAXTABLES; i++) m_twiddlePtrs[i] = twiddlePtrs[i];

        std::vector<T_internalDataType> tmpBuff0;
        std::vector<T_internalDataType> tmpBuff1;
        tmpBuff0.resize(TP_POINT_SIZE);
        tmpBuff1.resize(TP_POINT_SIZE);

        printf("Creating single kernel\n");
        fftKernels[0] =
            kernel::create_object<mixed_radix_fft<TT_DATA, TT_DATA, TT_TWIDDLE, TP_POINT_SIZE, TP_FFT_NIFFT, TP_SHIFT,
                                                  TP_RND, TP_SAT, TP_WINDOW_VSIZE, TP_START_RANK, TP_END_RANK> >(
                tmpBuff0, tmpBuff1, m_twiddleTable, m_twiddlePtrs);

    } // constructor
};    // create_casc_kernel class

//--------------------------------------------------------------------------------------------------
// mixed_radix_fft template parameters
//--------------------------------------------------------------------------------------------------
/**
 * @ingroup fft_graphs
 *
 * @brief mixed_radix_fft is a single-channel, decimation-in-time, fixed point size FFT including radix3 or radix5
 *stages.
 *
 *
 * These are the templates to configure the single-channel decimation-in-time class.
 * @tparam TT_DATA describes the type of individual data samples input to and
 *         output from the transform function. \n
 *         This is a typename and must be one of the following: \n
 *         int16, cint16, int32, cint32, float, cfloat.
 * @tparam TT_TWIDDLE describes the type of twiddle factors of the transform. \n
 *         It must be one of the following: cint16, cint32, cfloat
 *         and must also satisfy the following rules:
 *         - 32 bit types are only supported when TT_DATA is also a 32 bit type,
 *         - TT_TWIDDLE must be an integer type if TT_DATA is an integer type
 *         - TT_TWIDDLE must be cfloat type if TT_DATA is a float type.
 * @tparam TP_POINT_SIZE is an unsigned integer which describes the number of samples in
 *         the transform. \n This must be 2^N where N is an integer in the range
 *         4 to 16 inclusive. \n When TP_DYN_PT_SIZE is set, TP_POINT_SIZE describes the maximum
 *         point size possible.
 * @tparam TP_FFT_NIFFT selects whether the transform to perform is an FFT (1) or IFFT (0).
 * @tparam TP_SHIFT selects the power of 2 to scale the result by prior to output.
 * @tparam TP_RND selects the rounding mode.
 *         - rnd_floor      = Truncate LSB, always round down (towards negative infinity).
 *         - rnd_ceil       = Always round up (towards positive infinity).
 *         - rnd_sym_floor  = Truncate LSB, always round towards 0.
 *         - rnd_sym_ceil   = Always round up towards infinity.
 *         - rnd_pos_inf    = Round halfway towards positive infinity.
 *         - rnd_neg_inf    = Round halfway towards negative infinity.
 *         - rnd_sym_inf    = Round halfway towards infinity (away from zero).
 *         - rnd_sym_zero   = Round halfway towards zero (away from infinity).
 *         - rnd_conv_even  = Round halfway towards nearest even number.
 *         - rnd_conv_odd   = Round halfway towards nearest odd number. \n
 *         No rounding is performed on ceil or floor mode variants. \n
 *         Other modes round to the nearest integer. They differ only in how
 *         they round for values of 0.5. \n
 *
 *         Note: Rounding modes ``rnd_sym_floor`` and ``rnd_sym_ceil`` will only be supported on AIE-ML device. \n
 * @tparam TP_SAT describes the selection of saturation to be applied during the shift down stage of processing. \n
 *         TP_SAT accepts unsigned integer values, where:
 *         - 0: none           = No saturation is performed and the value is truncated on the MSB side.
 *         - 1: saturate       = Default. Saturation rounds an n-bit signed value
 *         in the range [- ( 2^(n-1) ) : +2^(n-1) - 1 ].
 *         - 3: symmetric      = Controls symmetric saturation. Symmetric saturation rounds
 *         an n-bit signed value in the range [- ( 2^(n-1) -1 ) : +2^(n-1) - 1 ]. \n
 * @tparam TP_WINDOW_VSIZE is an unsigned integer which describes the number of samples to be processed in each call \n
 *         to the function.  \n
 *         By default, TP_WINDOW_SIZE is set to match TP_POINT_SIZE. \n
 *         TP_WINDOW_SIZE may be set to be an integer multiple of the TP_POINT_SIZE, in which case \n
 *         multiple FFT operations will be performed on a given input window, one calculation per frame of data in \n
 *         the iobuffer, resulting in multiple output frames. This feature of packing multiple frames of data \n
 *         into a single call of the function reduces the number of kernel calls and as a result, reduces the losses \n
 *         due to overheads in the kernel call mechanism. As a result, overall performance is increased.
 * @tparam TP_CASC_LEN selects the number of kernels the FFT will be divided over in series
 *         to improve throughput
 * @tparam TP_API is an unsigned integer to select window (0) or stream (1) interfaces.
 *         When stream I/O is selected, one sample is taken from, or output to, a stream and the next sample
 *         from or two the next stream. Two streams minimum are used. In this example, even samples are
 *         read from input stream[0] and odd samples from input stream[1].
 **/
template <typename TT_DATA,
          typename TT_TWIDDLE,
          unsigned int TP_POINT_SIZE,
          unsigned int TP_FFT_NIFFT,
          unsigned int TP_SHIFT,
          unsigned int TP_RND = 4,
          unsigned int TP_SAT = 1,
          unsigned int TP_WINDOW_VSIZE = TP_POINT_SIZE, // to support multiple frames in an iobuffer
          unsigned int TP_CASC_LEN = 1,                 // single kernel operation only to begin with
          unsigned int TP_API = 0                       // 0 = iobuffer 1 = stream
          >
class mixed_radix_fft_graph : public graph {
   public:
    static_assert(TP_RND != rnd_sym_floor && TP_RND != rnd_sym_ceil && TP_RND != rnd_floor && TP_RND != rnd_ceil,
                  "Error: mixed radix FFT does not support TP_RND set to floor, ceil, symmetric floor, and symmetric "
                  "ceil. Please set TP_RND to any of the other rounding modes. The mapping of integers to rounding "
                  "modes is device dependent. Please refer to documentation for more information.");

    // declare MIXED_RADIX_FFT Kernel array
    kernel m_mixed_radix_fftKernels[TP_CASC_LEN];
    kernel* getKernels() { return m_mixed_radix_fftKernels; };
    static constexpr int kStreamsPerTile = get_input_streams_core_module();        // a device trait
    static constexpr int m_kNumPorts = TP_API == kWindowAPI ? 1 : kStreamsPerTile; // 1 for iobuffer, 2 for streams

    /**
     * The input data to the function.
     * I/O  is an iobuffer of TT_DATA type.
     **/
    port_array<input, m_kNumPorts> in; // iobuffer only

    /**
     * The output data from the function.
     * I/O  is an iobuffer of  TT_DATA type.
     **/
    port_array<output, m_kNumPorts> out;

    // decompose TP_POINT_SIZE in product of factors 5^r5stages * 3^r3stages * 2^r2stages * 4^r4stages
    // or TP_POINT_SIZE = r5factor * r3factor * r2factor * r4factor
    static constexpr int m_kR5Stages = fnGetNumStages<TP_POINT_SIZE, 5>();
    static constexpr int m_kR3Stages = fnGetNumStages<TP_POINT_SIZE, 3>();
    static constexpr int m_kR4Stages = fnGetNumStages<TP_POINT_SIZE, 4>();
    static constexpr int m_kR2Stages = fnGetNumStages<(TP_POINT_SIZE >> (2 * m_kR4Stages)), 2>();
    static constexpr int m_kR5factor = fnGetRadixFactor<TP_POINT_SIZE, 5>();
    static constexpr int m_kR3factor = fnGetRadixFactor<TP_POINT_SIZE, 3>();
    static constexpr int m_kR4factor = fnGetRadixFactor<TP_POINT_SIZE, 4>();
    static constexpr int m_kR2factor = fnGetRadixFactor<(TP_POINT_SIZE >> (2 * m_kR4Stages)), 2>();

    static_assert(m_kR5factor* m_kR3factor* m_kR4factor* m_kR2factor == TP_POINT_SIZE,
                  "ERROR: TP_POINT_SIZE failed to factorize");

    static constexpr int m_kTotalStages = m_kR5Stages + m_kR3Stages + m_kR2Stages + m_kR4Stages;
    static_assert(m_kTotalStages >= TP_CASC_LEN, "Error: TP_CASC_LEN is greater than the number of stages required");

    typedef typename std::conditional<std::is_same<TT_DATA, cint16>::value, cint32_t, TT_DATA>::type T_internalDataType;
    static constexpr int m_ktwiddleTableSize = fnGetTwiddleTableSize<TT_TWIDDLE, TP_POINT_SIZE>();

    /**
     * @brief This is the constructor function for the Mixed Radix FFT graph.
     * Constructor has no arguments.
     **/
    mixed_radix_fft_graph() {
        std::vector<T_internalDataType> tmpBuff0;
        std::vector<T_internalDataType> tmpBuff1;
        tmpBuff0.resize(TP_POINT_SIZE);
        tmpBuff1.resize(TP_POINT_SIZE);

        std::vector<TT_TWIDDLE> m_twiddleTable;
        m_twiddleTable.resize(m_ktwiddleTableSize);
        std::vector<int> m_twiddlePtrs; // index in m_twiddleTable of the start of each atomic table
        m_twiddlePtrs.resize(kNumMaxTables);

        typedef typename std::conditional<std::is_same<TT_TWIDDLE, cfloat>::value, float, int16>::type T_twiddlebase;
        int twiddleScale =
            std::is_same<TT_TWIDDLE, cfloat>::value ? 0 : 32768; // cint16 twiddles are expressed as FIX1_15
        double reald, imagd;
        T_twiddlebase realVal;
        T_twiddlebase imagVal;

        // First, create an array which tells the radix of each stage
        int radixOfStage[m_kTotalStages];
        int ptSizeOfStage[m_kTotalStages];
        int tally = 1;
        // Populate twiddle table and twiddle pointer array
        for (int stage = 0; stage < m_kR5Stages; stage++) {
            radixOfStage[stage] = 5;
            tally *= 5;
            ptSizeOfStage[stage] = tally;
        }
        for (int stage = m_kR5Stages; stage < m_kR5Stages + m_kR3Stages; stage++) {
            radixOfStage[stage] = 3;
            tally *= 3;
            ptSizeOfStage[stage] = tally;
        }
        for (int stage = m_kR5Stages + m_kR3Stages; stage < m_kR5Stages + m_kR3Stages + m_kR2Stages; stage++) {
            radixOfStage[stage] = 2;
            tally *= 2;
            ptSizeOfStage[stage] = tally;
        }
        for (int stage = m_kR5Stages + m_kR3Stages + m_kR2Stages;
             stage < m_kR5Stages + m_kR3Stages + m_kR2Stages + m_kR4Stages; stage++) {
            radixOfStage[stage] = 4;
            tally *= 4;
            ptSizeOfStage[stage] = tally;
        }

        int masterIdx = 0; // track entry in m_twiddleTable
        int ptrPtr = 0;    // track entry in m_twiddlePtrs
        int alignment = 32 / sizeof(TT_TWIDDLE);
        for (int stage = 0; stage < m_kR5Stages + m_kR3Stages + m_kR2Stages; stage++) { // radix4 breaks the pattern
            for (int leg = 1; leg < radixOfStage[stage]; leg++) {
                m_twiddlePtrs[ptrPtr++] = masterIdx; // base of table for this leg

                for (int index = 0; index < ptSizeOfStage[stage] / radixOfStage[stage]; index++) {
                    // double phase = M_PI * (double)(index * leg) / (double)(ptSizeOfStage[stage] /
                    // radixOfStage[stage]); // W index * 2pi.
                    double phase = M_PI * (double)(2 * index * leg) / (double)(ptSizeOfStage[stage]); // W index * 2pi.
                    reald = round(cos(phase) * twiddleScale); // cast to follow is a floor operation
                    imagd = round(-sin(phase) * twiddleScale);
                    realVal = (T_twiddlebase)(reald > 32767.0 ? 32767.0 : reald);
                    imagVal = (T_twiddlebase)(imagd > 32767.0 ? 32767.0 : imagd);
                    TT_TWIDDLE val;
                    val.real = realVal;
                    val.imag = imagVal;
                    m_twiddleTable[masterIdx] = val;

                    masterIdx++;
                }
                masterIdx = CEIL(masterIdx, alignment); // skip up to next aligned place for next leg
            }
        }

        //---------------------------------------
        // Radix4 twiddles - they break the mould.

        for (int stage = m_kR5Stages + m_kR3Stages + m_kR2Stages; stage < m_kTotalStages;
             stage++) { // radix4 breaks the pattern

            //------First leg---------
            // first leg twiddle is tw(ptSize/4)
            m_twiddlePtrs[ptrPtr++] = masterIdx; // base of table for this leg

            for (int index = 0; index < ptSizeOfStage[stage] / radixOfStage[stage]; index++) {
                double phase =
                    (double)(M_PI * index) / (double)(ptSizeOfStage[stage] / radixOfStage[stage]); // W index * 2pi.
                reald = round(cos(phase) * twiddleScale);
                imagd = round(-sin(phase) * twiddleScale);
                realVal = (T_twiddlebase)(reald > 32767.0 ? 32767.0 : reald);
                imagVal = (T_twiddlebase)(imagd > 32767.0 ? 32767.0 : imagd);
                TT_TWIDDLE val;
                val.real = realVal;
                val.imag = imagVal;
                m_twiddleTable[masterIdx] = val;

                masterIdx++;
            }
            masterIdx = CEIL(masterIdx, alignment); // skip up to next aligned place for next leg
            // end of first r4 leg

            //----------------------------------
            // second leg twiddle is tw(ptSize/2)
            m_twiddlePtrs[ptrPtr++] = masterIdx; // base of table for this leg

#if __FFT_R4_IMPL__ == 0
            constexpr int secondLegMult = 2;
#else
            constexpr int secondLegMult = 1;
#endif

            for (int index = 0; index < secondLegMult * ptSizeOfStage[stage] / radixOfStage[stage]; index++) {
                double phase =
                    M_PI * (double)(index) / (double)(2 * ptSizeOfStage[stage] / radixOfStage[stage]); // W index * 2pi.
                reald = round(cos(phase) * twiddleScale);
                imagd = round(-sin(phase) * twiddleScale);
                realVal = (T_twiddlebase)(reald > 32767.0 ? 32767.0 : reald);
                imagVal = (T_twiddlebase)(imagd > 32767.0 ? 32767.0 : imagd);
                TT_TWIDDLE val;
                val.real = realVal;
                val.imag = imagVal;
                m_twiddleTable[masterIdx] = val;

                masterIdx++;
            }
            masterIdx = CEIL(masterIdx, alignment); // skip up to next aligned place for next leg
                                                    // end of second r4 leg

#if __FFT_R4_IMPL__ == 1
            // third leg only for AIE-ML Radix4
            //----------------------------------
            // 3rd leg twiddle is tw(3*ptSize/4)
            m_twiddlePtrs[ptrPtr++] = masterIdx; // base of table for this leg

            for (int index = 0; index < ptSizeOfStage[stage] / radixOfStage[stage]; index++) {
                double phase = M_PI * (double)(3 * index) /
                               (double)(2 * ptSizeOfStage[stage] / radixOfStage[stage]); // W index * 2pi.
                reald = round(cos(phase) * twiddleScale);
                imagd = round(-sin(phase) * twiddleScale);
                realVal = (T_twiddlebase)(reald > 32767.0 ? 32767.0 : reald);
                imagVal = (T_twiddlebase)(imagd > 32767.0 ? 32767.0 : imagd);
                TT_TWIDDLE val;
                val.real = realVal;
                val.imag = imagVal;
                m_twiddleTable[masterIdx] = val;

                masterIdx++;
            }
            masterIdx = CEIL(masterIdx, alignment); // skip up to next aligned place for next leg
                                                    // end of second r4 leg

#endif //__FFT_R4_IMPL__ == 1
        }

        // Create kernel classes
        create_casc_kernel<TP_CASC_LEN, TT_DATA, TT_TWIDDLE, TP_POINT_SIZE, TP_FFT_NIFFT, TP_SHIFT, TP_CASC_LEN, TP_RND,
                           TP_SAT, TP_WINDOW_VSIZE, m_kTotalStages, m_ktwiddleTableSize,
                           kNumMaxTables>::create(m_mixed_radix_fftKernels, &m_twiddleTable[0], &m_twiddlePtrs[0]);

        // Connections
        if
            constexpr(TP_API == kWindowAPI) {
                // input
                connect(in[0], m_mixed_radix_fftKernels[0].in[0]);
                dimensions(m_mixed_radix_fftKernels[0].in[0]) = {TP_WINDOW_VSIZE};

                // connect final kernel output to output of the graph
                connect(m_mixed_radix_fftKernels[TP_CASC_LEN - 1].out[0], out[0]);
                dimensions(m_mixed_radix_fftKernels[TP_CASC_LEN - 1].out[0]) = {TP_WINDOW_VSIZE};
            }
        else {
            kernel m_inWidgetKernel =
                kernel::create_object<widget_api_cast<TT_DATA, kStreamAPI, kWindowAPI, kStreamsPerTile, TP_WINDOW_VSIZE,
                                                      1, kSampleIntlv, 0 /* kHeaderBytes*/> >();
            kernel m_outWidgetKernel =
                kernel::create_object<widget_api_cast<TT_DATA, kWindowAPI, kStreamAPI, 1, TP_WINDOW_VSIZE,
                                                      kStreamsPerTile, kSampleIntlv, 0 /*kHeaderBytes*/> >();
            connect(in[0], m_inWidgetKernel.in[0]);
            if
                constexpr(kStreamsPerTile > 1) { connect(in[1], m_inWidgetKernel.in[1]); }
            connect(m_inWidgetKernel.out[0], m_mixed_radix_fftKernels[0].in[0]);
            dimensions(m_inWidgetKernel.out[0]) = {TP_WINDOW_VSIZE};
            dimensions(m_mixed_radix_fftKernels[0].in[0]) = {TP_WINDOW_VSIZE};

            connect(m_mixed_radix_fftKernels[TP_CASC_LEN - 1].out[0], m_outWidgetKernel.in[0]);
            dimensions(m_mixed_radix_fftKernels[TP_CASC_LEN - 1].out[0]) = {TP_WINDOW_VSIZE};
            dimensions(m_outWidgetKernel.in[0]) = {TP_WINDOW_VSIZE};
            connect(m_outWidgetKernel.out[0], out[0]);
            if
                constexpr(kStreamsPerTile > 1) { connect(m_outWidgetKernel.out[1], out[1]); }

            // Specify mapping constraints
            source(m_inWidgetKernel) = "widget_api_cast.cpp";
            headers(m_inWidgetKernel) = {"widget_api_cast.hpp"};
            runtime<ratio>(m_inWidgetKernel) = 0.1;
            source(m_outWidgetKernel) = "widget_api_cast.cpp";
            headers(m_outWidgetKernel) = {"widget_api_cast.hpp"};
            runtime<ratio>(m_outWidgetKernel) = 0.1;
        }

        // kernel 'cascade' connections.
        for (int k = 1; k < TP_CASC_LEN; k++) {
            connect(m_mixed_radix_fftKernels[k - 1].out[0], m_mixed_radix_fftKernels[k].in[0]);
            dimensions(m_mixed_radix_fftKernels[k - 1].out[0]) = {TP_WINDOW_VSIZE};
            dimensions(m_mixed_radix_fftKernels[k].in[0]) = {TP_WINDOW_VSIZE};
        }

        for (int k = 0; k < TP_CASC_LEN; k++) {
            // Specify mapping constraints
            runtime<ratio>(m_mixed_radix_fftKernels[k]) = 0.7;

            // Source files
            source(m_mixed_radix_fftKernels[k]) = "mixed_radix_fft.cpp";
            headers(m_mixed_radix_fftKernels[k]) = {"mixed_radix_fft.hpp"};
        }
    };
};

} // namespace mixed_radix_fft
} // namespace fft
} // namespace aie
} // namespace dsp
} // namespace xf

#endif // _DSPLIB_MIXED_RADIX_FFT_GRAPH_HPP_
