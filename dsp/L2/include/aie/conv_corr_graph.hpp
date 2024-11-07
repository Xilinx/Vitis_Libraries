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

#ifndef _DSPLIB_CONV_CORR_GRAPH_HPP_
#define _DSPLIB_CONV_CORR_GRAPH_HPP_
/*
    The file captures the definition of the 'L2' graph level class for
    the Convolution / Correlation  function library element.
*/

/**
    * @file conv_corr_graph.hpp
    *
**/
#include <stdio.h>
#include <adf.h>
#include <vector>
#include <array>
#include <type_traits>
#include <assert.h>
#include <cstdint>
#include <adf/arch/aie_arch_properties.hpp>
#include "graph_utils.hpp"
#include "conv_corr.hpp"

using namespace adf;

namespace xf {
namespace dsp {
namespace aie {
namespace conv_corr {
/**
 * @defgroup conv_corr_graph Convolution / Correlation functions
 *
 * Convolution / Correlation
**/

/**
  * @cond NOCOMMENTS
  */

// Start of recursive kernel creation for cascaded conolution kernels
// This is the specialization for kernels in the middle of the cascade.

// phPos, kPos, TT_DATA_F, TT_DATA_G, TT_DATA_OUT, TP_FUNCT_TYPE, TP_COMPUTE_MODE, TP_F_LEN, TP_G_LEN, TP_SHIFT, TP_API,
// TP_RND,
// TP_SAT, TP_NUM_FRAMES, TP_CASC_LEN, TP_PHASES
template <int phPos,
          int kPos,
          typename TT_DATA_F,
          typename TT_DATA_G,
          typename TT_DATA_OUT,
          unsigned int TP_FUNCT_TYPE,
          unsigned int TP_COMPUTE_MODE,
          unsigned int TP_F_LEN,
          unsigned int TP_G_LEN,
          unsigned int TP_SHIFT,
          unsigned int TP_API,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_CASC_LEN,
          unsigned int TP_PHASES>
class create_casc_kernel_recur {
   public:
    static void create(kernel (&convcorrKernels)[TP_PHASES][TP_CASC_LEN]) {
        static constexpr unsigned int TP_KERNEL_POSITION = (kPos == 0) ? (TP_CASC_LEN - 1) : (kPos - 1);
        static constexpr unsigned int TP_PH_POSITION = (kPos == 0) ? (phPos - 1) : phPos;
        static constexpr bool CASC_OUT = (TP_KERNEL_POSITION == TP_CASC_LEN - 1) ? false : true;
        static constexpr bool CASC_IN = (TP_KERNEL_POSITION == 0) ? false : true;

        convcorrKernels[TP_PH_POSITION][TP_KERNEL_POSITION] =
            kernel::create_object<conv_corr<TT_DATA_F, TT_DATA_G, TT_DATA_OUT, TP_FUNCT_TYPE, TP_COMPUTE_MODE, TP_F_LEN,
                                            TP_G_LEN, TP_SHIFT, TP_API, TP_RND, TP_SAT, TP_NUM_FRAMES, TP_CASC_LEN,
                                            TP_PHASES, TP_KERNEL_POSITION, TP_PH_POSITION, CASC_IN, CASC_OUT> >();

        create_casc_kernel_recur<TP_PH_POSITION, TP_KERNEL_POSITION, TT_DATA_F, TT_DATA_G, TT_DATA_OUT, TP_FUNCT_TYPE,
                                 TP_COMPUTE_MODE, TP_F_LEN, TP_G_LEN, TP_SHIFT, TP_API, TP_RND, TP_SAT, TP_NUM_FRAMES,
                                 TP_CASC_LEN, TP_PHASES>::create(convcorrKernels);
    }
};

// Recursive convolution kernel creation
// This is the specialization for the end of recursion (first kernel in cascade)
template <typename TT_DATA_F,
          typename TT_DATA_G,
          typename TT_DATA_OUT,
          unsigned int TP_FUNCT_TYPE,
          unsigned int TP_COMPUTE_MODE,
          unsigned int TP_F_LEN,
          unsigned int TP_G_LEN,
          unsigned int TP_SHIFT,
          unsigned int TP_API,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_CASC_LEN,
          unsigned int TP_PHASES>
class create_casc_kernel_recur<0,
                               1,
                               TT_DATA_F,
                               TT_DATA_G,
                               TT_DATA_OUT,
                               TP_FUNCT_TYPE,
                               TP_COMPUTE_MODE,
                               TP_F_LEN,
                               TP_G_LEN,
                               TP_SHIFT,
                               TP_API,
                               TP_RND,
                               TP_SAT,
                               TP_NUM_FRAMES,
                               TP_CASC_LEN,
                               TP_PHASES> {
   public:
    static void create(kernel (&convcorrKernels)[TP_PHASES][TP_CASC_LEN]) {
        static constexpr unsigned int TP_KERNEL_POSITION = 0;
        static constexpr unsigned int TP_PH_POSITION = 0;
        convcorrKernels[0][0] =
            kernel::create_object<conv_corr<TT_DATA_F, TT_DATA_G, TT_DATA_OUT, TP_FUNCT_TYPE, TP_COMPUTE_MODE, TP_F_LEN,
                                            TP_G_LEN, TP_SHIFT, TP_API, TP_RND, TP_SAT, TP_NUM_FRAMES, TP_CASC_LEN,
                                            TP_PHASES, TP_KERNEL_POSITION, TP_PH_POSITION, false, true> >();
    }
};

// Convolution Kernel creation, entry to recursion, also end of cascade.
template <int phPos,
          int kPos,
          typename TT_DATA_F,
          typename TT_DATA_G,
          typename TT_DATA_OUT,
          unsigned int TP_FUNCT_TYPE,
          unsigned int TP_COMPUTE_MODE,
          unsigned int TP_F_LEN,
          unsigned int TP_G_LEN,
          unsigned int TP_SHIFT,
          unsigned int TP_API,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_CASC_LEN,
          unsigned int TP_PHASES>
class create_casc_kernel {
   public:
    static void create(kernel (&convcorrKernels)[TP_PHASES][TP_CASC_LEN]) {
        static constexpr unsigned int TP_KERNEL_POSITION = kPos - 1;
        static constexpr unsigned int TP_PH_POSITION = phPos - 1;
        convcorrKernels[phPos - 1][kPos - 1] =
            kernel::create_object<conv_corr<TT_DATA_F, TT_DATA_G, TT_DATA_OUT, TP_FUNCT_TYPE, TP_COMPUTE_MODE, TP_F_LEN,
                                            TP_G_LEN, TP_SHIFT, TP_API, TP_RND, TP_SAT, TP_NUM_FRAMES, TP_CASC_LEN,
                                            TP_PHASES, TP_KERNEL_POSITION, TP_PH_POSITION, true, false> >();

        create_casc_kernel_recur<phPos - 1, kPos - 1, TT_DATA_F, TT_DATA_G, TT_DATA_OUT, TP_FUNCT_TYPE, TP_COMPUTE_MODE,
                                 TP_F_LEN, TP_G_LEN, TP_SHIFT, TP_API, TP_RND, TP_SAT, TP_NUM_FRAMES, TP_CASC_LEN,
                                 TP_PHASES>::create(convcorrKernels);
    }
};

// Conv,Corr Kernel creation, Specialization for CASC_LEN=1
template <typename TT_DATA_F,
          typename TT_DATA_G,
          typename TT_DATA_OUT,
          unsigned int TP_FUNCT_TYPE,
          unsigned int TP_COMPUTE_MODE,
          unsigned int TP_F_LEN,
          unsigned int TP_G_LEN,
          unsigned int TP_SHIFT,
          unsigned int TP_API,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_CASC_LEN,
          unsigned int TP_PHASES>
class create_casc_kernel<1,
                         1,
                         TT_DATA_F,
                         TT_DATA_G,
                         TT_DATA_OUT,
                         TP_FUNCT_TYPE,
                         TP_COMPUTE_MODE,
                         TP_F_LEN,
                         TP_G_LEN,
                         TP_SHIFT,
                         TP_API,
                         TP_RND,
                         TP_SAT,
                         TP_NUM_FRAMES,
                         TP_CASC_LEN,
                         TP_PHASES> {
   public:
    static void create(kernel (&convcorrKernels)[1][1]) {
        static constexpr unsigned int TP_KERNEL_POSITION = 0;
        static constexpr unsigned int TP_PH_POSITION = 0;
        convcorrKernels[0][0] =
            kernel::create_object<conv_corr<TT_DATA_F, TT_DATA_G, TT_DATA_OUT, TP_FUNCT_TYPE, TP_COMPUTE_MODE, TP_F_LEN,
                                            TP_G_LEN, TP_SHIFT, TP_API, TP_RND, TP_SAT, TP_NUM_FRAMES, TP_CASC_LEN,
                                            TP_PHASES, TP_KERNEL_POSITION, TP_PH_POSITION, false, false> >();
    }
};

/**
* @endcond
*/

//--------------------------------------------------------------------------------------------------
// conv_corr_graph template
//--------------------------------------------------------------------------------------------------
/**
 * @ingroup conv_corr_graph
 * @brief Convolution / Correlation of input vectors F and G
 *
 * These are the templates to configure the convolution and correlation:
 * @tparam TT_DATA_F describes the type of individual data samples of signal F to input to the function. \n
 *         This is a typename and must be one of the following: \n
 *         int8, int16, int32, cint16, cint32.
 * @tparam TT_DATA_G describes the type of individual data samples of signal G to input to the function. \n
 *         This is a typename and must be one of the following: \n
 *         int8, int16, int32, cint16, cint32.
 * @tparam TT_DATA_OUT describes the type individual data samples at the function output. \n
 *         This is a typename and must be one of the following: \n
 *         int16, int32, cint32, cint32.
 *         Note: TT_DATA_OUT type should be complex if any input is complex.
 * @tparam TP_FUNCT_TYPE describes the selection of function i.e convolution or correlation. \n
 * @tparam TP_COMPUTE_MODE describes the computation mode of conv/corr. we have 3 modes which are
 *         - FULL  (0) -- Length of F Signal will be updated by padding with zeros of G len (G-1) i.e. Len_F = [ (G-1)
 *         + F + (G-1) ]
 *         - SAME  (1) -- Length of F Signal will be updated by padding with zeros of G len (G/2-1)  i.e. Len_F = [
 *         ((G/2)-1) + F + ((G/2)-1) ]
 *         - VALID (2) -- Length of F Signal will be updated by padding with zeros of F_load i.e. Len_F = [ F +
 *         data_load]
 * @tparam TP_F_LEN describes the number of samples in vector F. \n
 * @tparam TP_G_LEN describes the number of samples in vector G. \n
 * @tparam TP_SHIFT describes the number of bits to downshift. \n
 * @tparam TP_API describes whether to use streams (1) or windows (0). \n
 * @tparam TP_RND describes the selection of rounding to be applied during the
 *         shift down stage of processing. \n
 *         Although, TP_RND accepts unsigned integer values descriptive macros are recommended where
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
 *         \n
 *         Note: Rounding modes ``rnd_sym_floor`` and ``rnd_sym_ceil`` are only supported on AIE-ML device. \n
 * @tparam TP_SAT describes the selection of saturation to be applied during the shift down stage of processing. \n
 *         TP_SAT accepts unsigned integer values, where:
 *         - 0: none           = No saturation is performed and the value is truncated on the MSB side.
 *         - 1: saturate       = Default. Saturation rounds an n-bit signed value
 *         in the range [- ( 2^(n-1) ) : +2^(n-1) - 1 ].
 *         - 3: symmetric      = Controls symmetric saturation. Symmetric saturation rounds
 *         an n-bit signed value in the range [- ( 2^(n-1) -1 ) : +2^(n-1) - 1 ]. \n
 *
 * @tparam TP_NUM_FRAMES describes the number of frames of input data samples that occur
 *         within each input window of data. \n
 * @tparam TP_CASC_LEN describes the number of AIE processors to split the operation
 *         over. \n This allows resource to be traded for higher performance.
 *         TP_CASC_LEN must be in the range 1 (default) to 40.
 * @tparam TP_PHASES specifies the number of parallel input/output paths where samples are interleaved between paths,
 *         giving an overall higher throughput.   \n
 *         A TP_PHASES of 1 means just one output leg and 1 input phase, and is the backwards compatible option. \n
 *         The number of AIEs used is given by ``TP_PHASES*TP_CASC_LEN``. \n
 *
 **/

// TT_DATA_F, TT_DATA_G, TT_DATA_OUT, TP_FUNCT_TYPE, TP_COMPUTE_MODE, TP_F_LEN, TP_G_LEN, TP_SHIFT, TP_API, TP_RND,
// TP_SAT
// TP_NUM_FRAMES, TP_CASC_LEN, TP_PHASES
template <typename TT_DATA_F,
          typename TT_DATA_G,
          typename TT_DATA_OUT,
          unsigned int TP_FUNCT_TYPE,
          unsigned int TP_COMPUTE_MODE,
          unsigned int TP_F_LEN,
          unsigned int TP_G_LEN,
          unsigned int TP_SHIFT,
          unsigned int TP_API,
          unsigned int TP_RND = 0,
          unsigned int TP_SAT = 1,
          unsigned int TP_NUM_FRAMES = 1,
          unsigned int TP_CASC_LEN = 1,
          unsigned int TP_PHASES = 1>

class conv_corr_graph : public graph {
   public:
    /**
     * @cond NOCOMMENTS
     */
    // Parameter value defensive and legality checks

    // defensive check for Input data types
    static_assert(fnCheckInputDataTypes<TT_DATA_F, TT_DATA_G, TP_API>(),
                  " Assertion Failed : \n"
                  "             ERROR: TT_DATA_F and TT_DATA_G are not a supported combination ");

    // defensive check for Output data type
    static_assert(fnCheckDataTypeOfOutput<TT_DATA_F, TT_DATA_G, TT_DATA_OUT>(),
                  " Assertion Failed : \n"
                  "              ERROR:TT_DATA_OUT should be complex if any input data type is complex.");

    // defensive check for Function type i.e either CONV or CORR
    static_assert(TP_FUNCT_TYPE <= 1,
                  " Assertion Failed : \n"
                  "              ERROR: 'TP_FUNCT_TYPE' must be \n "
                  "                     0-CORRELATION or \n "
                  "                     1-CONVOLUTION for both AIE-1 and AIE-2");

    // defensive check for CONV/CORR compute mode i.e. full/same/valid
    static_assert(fnCheckIsComputeModeValid<TP_COMPUTE_MODE, TP_API>(),
                  " Assertion Failed : \n"
                  "              ERROR: [TP_API = 1 'STREAM']  'TP_COMPUTE_MODE' always 2-VALID_MODE only \n"
                  "                     [TP_API = 0 'IOBuffer']  ] \n "
                  "                    'TP_COMPUTE_MODE' must be 0-FULL_MODE  or 1-SAME_MODE or 2-VALID_MODE \n ");

    // defensive check for G sig length should be always less than or equal to F Sig Length
    // defensive check for TP_G_LEN which should be multiples of (phases*4)) when Stream only Processing happening
    static_assert(fnCheckGLen<TT_DATA_F, TP_F_LEN, TP_G_LEN, TP_PHASES, TP_API>(),
                  " Assertion Failed : \n            ERROR: 'TP_G_LEN' should be always less than or equal to TP_F_LEN "
                  "as per con_corr requirement \n"
                  "                   For API-1[STREAM] [COND-1] : TP_G_LEN must be in the range <8,256> when stream "
                  "only processing AND \n  "
                  "                                    ([COND-2] : TP_G_LEN must be equal to (TP_PHASES*4) OR \n "
                  "                                     [COND-3] : TP_G_LEN must be multiples of (TP_PHASES*8) when "
                  "TP_G_LEN is greater than (TP_PHASES*4)).  \n");

    // defensive check for num of frames i.e., TP_NUM_FRAMES restricted to 1 to maintain the better performance for time
    // being.
    static_assert(TP_NUM_FRAMES == 1,
                  " Assertion Failed : \n"
                  "              ERROR: 'TP_NUM_FRAMES' should be always equal to 1 (ONE) for current version to "
                  "maintain the same performance of conv/corr.");

    // defensive check for Lengths of F and G should be in the given range of Min and Max
    static_assert(fnCheckLenOfData<TT_DATA_F, TP_F_LEN, TP_API>(),
                  " Assertion Failed : \n "
                  "             ERROR: TP_F_LEN should be granuality of Min data_load on AIE i.e. "
                  "[(256/samplesize<TT_DATA_F>())] \n                   [int8   - (32*N) ] \n                   [int16 "
                  " - (16*N) ] \n                   [int32  - (8*N)  ] \n                   [cint16 - (8*N)  ] \n      "
                  "             [cint32 - (4*N)  ] \n            where N is Integer > 1] and \n            TP_F_LEN "
                  "should be greater than or equal to minimum length [((256/samplesize<TT_DATA_F>())*2)] based on "
                  "given data type i.e.\n                 '[Data Type-    MIN    MAX]' \n                 "
                  "'--------------------------' \n                 '[int8     -    64    8192]' \n                 "
                  "'[int16    -    32    4096]' \n                 '[int32    -    16    2048]' \n                 "
                  "'[cint16   -    16    2048]' \n                 '[cint32   -    8     1024]' ");

    static_assert(fnCheckLenOfData<TT_DATA_G, TP_G_LEN, TP_API>(),
                  " Assertion Failed : \n "
                  "             ERROR: TP_G_LEN should be granuality of Min data_load on AIE i.e. "
                  "[(256/samplesize<TT_DATA_G>())] \n                   [int8   - (32*N) ] \n                   [int16 "
                  " - (16*N) ] \n                   [int32  - (8*N)  ] \n                   [cint16 - (8*N)  ] \n      "
                  "             [cint32 - (4*N)  ] \n            where N is Integer > 1] and \n            TP_G_LEN "
                  "should be greater than or equal to minimum length [((256/samplesize<TT_DATA_G>())*2)] based on "
                  "given data type i.e.\n                 '[Data Type-    MIN    MAX]' \n                 "
                  "'--------------------------' \n                 '[int8     -    64    8192]' \n                 "
                  "'[int16    -    32    4096]' \n                 '[int32    -    16    2048]' \n                 "
                  "'[cint16   -    16    2048]' \n                 '[cint32   -    8     1024]' ");

    // defensive check for scaling factor should be in the range.
    static_assert(fnValidateShiftFloat<TP_SHIFT, TT_DATA_F>(), "ERROR: TP_SHIFT must be 0 for float types.");
    static_assert(fnValidateShiftRange<TP_SHIFT>(), "ERROR: TP_SHIFT is out of the supported range.");

    // defensive check for API Port is whether iobuffer or stream (stream not supported in current version).
    static_assert(fnCheckIsStreamSupportedbyArch<TP_API>(),
                  " Assertion Failed : \n"
                  "            ERROR: TP_API must be \n "
                  "                           -0 for 'iobuffer' \n "
                  "                           -1 for 'stream only' \n ");

    // defensive check for ROUND Mode which should be in the range i.e. ROUND_MIN <TP_RND< ROUND_MAX
    static_assert(fnValidateRoundMode<TP_RND>(), "ERROR: Illegal round mode.");

    // defensive check for SATURATION Mode which should be in the range i.e. SAT_MODE_MIN <TP_SAT< SAT_MODE_MAX
    static_assert(fnValidateSatMode<TP_SAT>(), "ERROR: Illegal saturation mode.");

    // STREAM BASED PROCESSING: DEFENSIVE CHECKS
    // defensive check for (TP_G_LEN/TP_CASC_LEN)should be multiples of (Lanes*Points) when Stream only Processing
    // happening
    static_assert(fnCheckCascLen<TP_G_LEN, TP_CASC_LEN, TP_PHASES, TP_API>(),
                  "Assertion Failed : \n"
                  "            ERROR: TP_CASC_LEN should be equal to (TP_G_LEN/8) or \n"
                  "            TP_CASC_LEN should be (TP_G_LEN/16) or (TP_G_LEN/32) for less throughput requirement\n");

    // defensive check for PHASES which should be power of 2
    static_assert(
        fnCheckPhases<TP_G_LEN, TP_CASC_LEN, TP_PHASES, TP_API>(),
        "Assertion Failed : \n"
        "            ERROR: TP_PHASES can be greater than 1 only when TP_CASC_LEN should be equal to (TP_G_LEN/8) \n "
        "                  TP_PHASES is always 1 for both TP_API=0 and TP_API==1 \n ");

    /**
    * @endcond
    */
    /**
        * The input F data to the function.
    **/
    port_array<input, TP_PHASES> inF;
    /**
        * The input G data to the function.
    **/
    input_port inG;
    /**
        * The output data of conv_corr.
    **/
    port_array<output, TP_PHASES> out;

    /**
        * The array of kernels that will be created and mapped onto AIE tiles.
    **/
    kernel m_conv_corr[TP_PHASES][TP_CASC_LEN];

    /**
        * @brief This is the constructor function for the conv_corr graph.
    **/
    conv_corr_graph() {
        if
            constexpr(TP_API == 0) {
                m_conv_corr[0][0] = kernel::create_object<
                    conv_corr<TT_DATA_F, TT_DATA_G, TT_DATA_OUT, TP_FUNCT_TYPE, TP_COMPUTE_MODE, TP_F_LEN, TP_G_LEN,
                              TP_SHIFT, TP_API, TP_RND, TP_SAT, TP_NUM_FRAMES, TP_CASC_LEN, TP_PHASES, 0, 0> >();

                // Loop Count
                static constexpr int kLanes = getNumofLanes<TT_DATA_F, TT_DATA_G>();
                static constexpr int kLoopCount = getLoopCount<TP_COMPUTE_MODE, TP_F_LEN, TP_G_LEN>();
                static constexpr int kAieLoopCount = (CEIL(kLoopCount, kLanes) / kLanes);
                static constexpr int kOutLen = (kAieLoopCount * kLanes);
                static constexpr int kpaddedFsigLen =
                    getPaddedLength<TT_DATA_F, TT_DATA_G, TP_COMPUTE_MODE, TP_F_LEN, TP_G_LEN>();

                // make connections
                connect<>(inF[0], m_conv_corr[0][0].in[0]);
                connect<>(inG, m_conv_corr[0][0].in[1]);
                dimensions(m_conv_corr[0][0].in[0]) = {kpaddedFsigLen * TP_NUM_FRAMES};
                dimensions(m_conv_corr[0][0].in[1]) = {TP_G_LEN * TP_NUM_FRAMES};

                // connect final kernel output to output of the graph
                connect<>(m_conv_corr[0][0].out[0], out[0]);
                dimensions(m_conv_corr[0][0].out[0]) = {kOutLen * TP_NUM_FRAMES};

                // Specify mapping constraints
                runtime<ratio>(m_conv_corr[0][0]) =
                    0.1; // Nominal figure. The real figure requires knowledge of the sample rate.
                // Source files
                source(m_conv_corr[0][0]) = "conv_corr.cpp";
                headers(m_conv_corr[0][0]) = {"conv_corr.hpp"};
            }
        else {
            static constexpr unsigned int m_kMuls = getNumofMULs<TT_DATA_F, TT_DATA_G>();
            static constexpr int Lanes = 4;                // Num of Lanes
            static constexpr int Points = m_kMuls / Lanes; // Num of Points
            static constexpr int streampercore_var = ((Lanes * Points * TP_PHASES) >> 1);
            static constexpr int streams_per_core = (TP_G_LEN > streampercore_var) ? 1 : 2;
            static constexpr int phases = TP_PHASES - 1;
            int netval, netval1, xinit, xinit8, xstart, dir, xoff;

            // Create kernel classes
            create_casc_kernel<TP_PHASES, TP_CASC_LEN, TT_DATA_F, TT_DATA_G, TT_DATA_OUT, TP_FUNCT_TYPE,
                               TP_COMPUTE_MODE, TP_F_LEN, TP_G_LEN, TP_SHIFT, TP_API, TP_RND, TP_SAT, TP_NUM_FRAMES,
                               TP_CASC_LEN, TP_PHASES>::create(m_conv_corr);

            // Outer Loop that iterates for all phases
            for (unsigned int i = 0; i < TP_PHASES; i++) {
                // Make Connections
                // Inner loop that iterates for cascade length
                for (unsigned int j = 0; j < TP_CASC_LEN; j++) {
                    // connect cascaded kernels
                    if (TP_CASC_LEN > 1 && j > 0) {
                        connect<cascade>(m_conv_corr[i][j - 1].out[0], m_conv_corr[i][j].in[2]);
                    }
                    // connect input data to each kernel
                    netval = i + j;
                    netval1 = TP_PHASES * TP_CASC_LEN + i + j;

                    connect<stream> netval(inF[(i + ((j << (streams_per_core - 1)) & phases)) & phases],
                                           m_conv_corr[i][j].in[0]);
                    fifo_depth(netval) = 1024;

                    connect<stream> netval1(inF[(i + (((j << (streams_per_core - 1)) + 1) & phases)) & phases],
                                            m_conv_corr[i][j].in[1]);
                    fifo_depth(netval1) = 1024;

                    runtime<ratio>(m_conv_corr[i][j]) = 0.9;
                    // Source files
                    source(m_conv_corr[i][j]) = "conv_corr.cpp";
                    headers(m_conv_corr[i][j]) = {"conv_corr.hpp"};
                }

                connect(inG, m_conv_corr[i][0].in[2]);
                dimensions(m_conv_corr[i][0].in[2]) = {TP_G_LEN};
                // connect final kernel output to output of the graph
                connect<stream>(m_conv_corr[i][TP_CASC_LEN - 1].out[0], out[i]);
            }
        }
    }; // constructor
};

} // namespace conv_corr
} // namespace aie
} // namespace dsp
} // namespace xf

#endif // _DSPLIB_CONV_CORR_GRAPH_HPP_
