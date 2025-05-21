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
#ifndef _DSPLIB_FIR_RESAMPLER_GRAPH_HPP_
#define _DSPLIB_FIR_RESAMPLER_GRAPH_HPP_
/*
The file captures the definition of the 'L2' graph level class for
the FIR_RESAMPLER library element.
*/

#include <adf.h>
#include <vector>
#include "fir_graph_utils.hpp"
#include <tuple>
#include "fir_common_traits.hpp"
#include "fir_resampler.hpp"
#include "fir_decomposer_utils.hpp"

namespace xf {
namespace dsp {
namespace aie {
namespace fir {
namespace resampler {
using namespace adf;

//--------------------------------------------------------------------------------------------------
// fir_resampler_graph template
//--------------------------------------------------------------------------------------------------

/**
 * @brief fir_resampler is a generic asymmetric FIR filter that can do fractional and integer interpolation and
 *decimation.
 *
 * @ingroup fir_graphs
 *
 * These are the templates to configure the generic FIR class.
 * @tparam TT_DATA describes the type of individual data samples input to and
 *         output from the filter function. \n
 *         This is a typename and must be one of the following: \n
 *         int16, cint16, int32, cint32, float, cfloat.
 * @tparam TT_COEFF describes the type of individual coefficients of the filter
 *         taps. \n It must be one of the same set of types listed for TT_DATA
 *         and must also satisfy the following rules:
 *         - Complex types are only supported when ``TT_DATA`` is also complex.
 *         - ``TT_COEFF`` must be an integer type if TT_DATA is an integer type
 *         - ``TT_COEFF`` must be a float type if TT_DATA is a float type.
 * @tparam TP_FIR_LEN is an unsigned integer which describes the number of taps
 *         in the filter.
 * @tparam TP_INTERPOLATE_FACTOR is an unsigned integer which describes the
 *         interpolation factor of the filter. \n
 *         TP_INTERPOLATE_FACTOR must be in the
 *         range 3 to 16.
 * @tparam TP_DECIMATE_FACTOR is an unsigned integer which describes the
 *         decimation factor of the filter. TP_DECIMATE_FACTOR must be in the
 *         range 2 to 16.
 * @tparam TP_SHIFT describes power of 2 shift down applied to the accumulation of
 *         FIR terms before output. \n ``TP_SHIFT`` must be in the range 0 to 59 (61 for AIE1).
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
 *         Note: Rounding modes ``rnd_sym_floor`` and ``rnd_sym_ceil`` are only supported on AIE-ML and AIE-MLv2 device.
\n
 * @tparam TP_INPUT_WINDOW_VSIZE describes the number of samples processed by the graph
 *         in a single iteration run.  \n
 *         When TP_API is set to 0, samples are buffered and stored in a ping-pong window buffer mapped onto Memory
 *Group banks. \n
 *         As a result, maximum number of samples processed by the graph is limited by the size of Memory Group. \n
 *         When TP_API is set to 1, samples are processed directly from the stream inputs and no buffering takes place.
 *\n
 *         In such case, maximum number of samples processed by the graph is limited to 32-bit value (4.294B samples per
 *iteration).  \n
 *         TP_INPUT_WINDOW_VSIZE must be an integer multiple of TP_DECIMATE_FACTOR.
 *         The number of values in the output window will be TP_INPUT_WINDOW_VSIZE
 *         multiplied by TP_INTERPOLATE_FACTOR and divided by TP_DECIMATE_FACTOR. \n
 *         The resulting output window size must be a multiple of 256bits. \n
 *         \n
 *         Note: Margin size should not be included in TP_INPUT_WINDOW_VSIZE.
 * @tparam TP_CASC_LEN describes the number of AIE processors to split the operation
 *         over. \n This allows resource to be traded for higher performance.
 *         TP_CASC_LEN must be in the range 1 (default) to 40.
 * @tparam TP_USE_COEFF_RELOAD allows the user to select if runtime coefficient
 *         reloading should be used. \n When defining the parameter:
 *         - 0 = static coefficients, defined in filter constructor,
 *         - 1 = reloadable coefficients, passed as argument to runtime function. \n
 *         \n
 *         Note: when used, async port: ```port_conditional_array<input, (TP_USE_COEFF_RELOAD == 1), TP_SSR> coeff;```
 *         will be added to the FIR. \n
 *         \n
 *         Note: the size of the port array is equal to the total number of output paths  (TP_SSR).  \n
 *         Each port should contain the same taps array content, i.e. each additional port must be a duplicate of the
 *         coefficient array. \n
 * @tparam TP_NUM_OUTPUTS sets the number of ports over which the output is sent. \n
 *         This can be 1 or 2. It is set to 1 by default. \n
 *         Depending on TP_API, additional output ports functionality differs.
 *         For Windows API, additional output provides flexibility in connecting
 *         FIR output with multiple destinations.
 *         Additional output ``out2`` is an exact copy of the data of the output port ``out``. \n
 *         \n
 *         With Stream API, the additional output port increases the FIR's throughput. \n
 *         Data is sent in a 128-bit interleaved pattern, e.g. : \n
 *         - samples 0-3 is sent over stream0 for cint16 data type, \n
 *         - samples 4-7 is sent over stream1 for cint16 data type. \n
 *         \n
 *         Note: when used, optional port: ``` port<output> out2; ``` will be added to the FIR. \n
 * @tparam TP_DUAL_IP allows 2 stream inputs to be connected to FIR, increasing available throughput. \n
 *         When set to 0, single stream will be connected as FIRs input. \n
 *         When set to 1, two stream inputs will be connected. \n
 *         In such case data should be organized in 128-bit interleaved pattern, e.g.: \n
 *         - samples 0-3 to be sent over stream0 for cint16 data type, \n
 *         - samples 4-7 to be sent over stream1 for cint16 data type. \n
 * @tparam TP_API specifies if the input/output interface should be window-based or stream-based.  \n
 *         The values supported are 0 (window API) or 1 (stream API).
 * @tparam TP_SSR specifies the number of parallel input paths where samples are interleaved between paths,
 giving an overall higher throughput.   \n
 *         An SSR of 1 means just one input path, and is the backwards compatible option.
 *         SSR Decomposition is currently unavailable. Please set TP_SSR to 1 and use
 *         TP_PARA_INTERP_POLY/TP_PARA_DECI_POLY for Super Sample operation modes.
 * @tparam TP_PARA_INTERP_POLY sets the number of interpolator polyphases over which the coefficients will be split to
 enable parallel computation of the outputs. \n
 *         The polyphases are executed in parallel, output data is produced by each polyphase directly. \n
 *         TP_PARA_INTERP_POLY does not affect the number of input data paths. \n
 *         There will be TP_SSR input phases irrespective of the value of TP_PARA_INTERP_POLY. \n
 *         TP_PARA_INTERP_POLY = TP_INTERPOLATE_FACTOR will result in an interpolate factor of polyphases,
 * where each polyphase is a single rate filters. \n
 *          TP_PARA_INTERP_POLY < TP_INTERPOLATE_FACTOR will result in the kernels in the polyphase branches operating
 *          as interpolators. \n
 *         The number of AIEs used is given by TP_PARA_INTERP_POLY * TP_SSR^2 * TP_CASC_LEN. \n
 *
 * @tparam TP_PARA_DECI_POLY specifies the number of decimator polyphases that will be split up
 * and executed in a series of pipelined cascade stages, resulting in additional input paths. \n
 *         A TP_PARA_DECI_POLY of 1 means just one input leg, and is the backwards compatible option. \n
 *         TP_PARA_DECI_POLY = TP_DECIMATE_FACTOR will result in an decimate factor of polyphases,
 * operating as independent single rate filters connected by cascades.
 *         The number of AIEs used is given by TP_PARA_DECI_POLY * TP_SSR^2 * TP_CASC_LEN. \n
 * @tparam TP_SAT describes the selection of saturation to be applied during the shift down stage of processing. \n
 *         TP_SAT accepts unsigned integer values, where:
 *         - 0: none           = No saturation is performed and the value is truncated on the MSB side.
 *         - 1: saturate       = Default. Saturation rounds an n-bit signed value
 *         in the range [- ( 2^(n-1) ) : +2^(n-1) - 1 ].
 *         - 3: symmetric      = Controls symmetric saturation. Symmetric saturation rounds
 *         an n-bit signed value in the range [- ( 2^(n-1) -1 ) : +2^(n-1) - 1 ]. \n
 *
**/

template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_INTERPOLATE_FACTOR,
          unsigned int TP_DECIMATE_FACTOR,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_CASC_LEN = 1,
          unsigned int TP_USE_COEFF_RELOAD = 0,
          unsigned int TP_NUM_OUTPUTS = 1,
          unsigned int TP_DUAL_IP = 0,
          unsigned int TP_API = 0,
          unsigned int TP_SSR = 1,
          unsigned int TP_PARA_INTERP_POLY = 1,
          unsigned int TP_PARA_DECI_POLY = 1,
          unsigned int TP_SAT = 1>
class fir_resampler_graph : public graph {
   private:
    static_assert(TP_CASC_LEN <= 40, "ERROR: Unsupported Cascade length");

    // Dual input ports offer no throughput gain if port api is windows.
    // Therefore, dual input ports are only supported with streams and not windows.
    static_assert(TP_API == USE_STREAM_API || TP_DUAL_IP == DUAL_IP_SINGLE,
                  "ERROR: Dual input ports only supported when port API is a stream. ");

    static constexpr unsigned int kMaxTapsPerKernel = 256;
    // Limit FIR length per kernel. Longer FIRs may exceed Program Memory and/or system memory combined with window
    // buffers may exceed Memory Module size
    static_assert(TP_FIR_LEN / TP_CASC_LEN <= kMaxTapsPerKernel,
                  "ERROR: Requested FIR length and Cascade length exceeds supported number of taps per kernel. Please "
                  "increase the cascade length to accommodate the FIR design.");

    // Limit FIR length for reloadable coeffs. Reloadable coeffs need a storage space that contributes to system memory
    // exceeding Memory Module size.
    static_assert(TP_USE_COEFF_RELOAD == 0 || TP_FIR_LEN <= kMaxTapsPerKernel,
                  "ERROR: Exceeded maximum supported FIR length with reloadable coefficients. Please limit the FIR "
                  "length or disable coefficient reload.");

    static constexpr unsigned int kMemoryModuleSize = __DATA_MEM_BYTES__;
    static constexpr unsigned int inBufferSize = ((TP_FIR_LEN + TP_INPUT_WINDOW_VSIZE) * sizeof(TT_DATA));
    // Requested Input Window buffer exceeds memory module size
    static_assert(TP_API != 0 || inBufferSize <= kMemoryModuleSize,
                  "ERROR: Input Window size (based on requested window size and FIR length margin) exceeds Memory "
                  "Module size of 32kB for AIE-1 and 64kB for AIE-ML devices.");

    static constexpr unsigned int outBufferSize =
        (TP_INTERPOLATE_FACTOR * TP_INPUT_WINDOW_VSIZE * sizeof(TT_DATA) / TP_DECIMATE_FACTOR);
    // Requested Output Window buffer exceeds memory module size
    static_assert(TP_API != 0 || outBufferSize <= kMemoryModuleSize,
                  "ERROR: Input Window size (based on requested window size and FIR length margin) exceeds Memory "
                  "Module size of 32kB");
    // SSR is decomposition is currently unavailable
    // static_assert(TP_SSR == 1,
    //               "ERROR: SSR Decomposition is currently unavailable. Please set TP_SSR to 1 and use
    //               TP_PARA_INTERP_POLY/TP_PARA_DECI_POLY for Super Sample operation modes.");

    static constexpr unsigned int TP_CASC_IN = 0;
    static constexpr unsigned int TP_CASC_OUT = 0;

    // 3d array for storing net information.
    // address[inPhase][outPath][cascPos]
    std::array<std::array<std::array<std::array<std::array<connect<stream, stream>*, TP_CASC_LEN>, TP_SSR>, TP_SSR>,
                          TP_PARA_DECI_POLY>,
               TP_PARA_INTERP_POLY>
        net;
    std::array<std::array<std::array<std::array<std::array<connect<stream, stream>*, TP_CASC_LEN>, TP_SSR>, TP_SSR>,
                          TP_PARA_DECI_POLY>,
               TP_PARA_INTERP_POLY>
        net2;

    static constexpr unsigned int firRange =
        (TP_CASC_LEN == 1)
            ? TP_FIR_LEN
            : fnFirRange<CEIL(TP_FIR_LEN, TP_INTERPOLATE_FACTOR), TP_CASC_LEN, 0, TP_INTERPOLATE_FACTOR>();
    using firstKernelClass = fir_resampler<TT_DATA,
                                           TT_COEFF,
                                           TP_FIR_LEN,
                                           TP_INTERPOLATE_FACTOR,
                                           TP_DECIMATE_FACTOR,
                                           TP_SHIFT,
                                           TP_RND,
                                           TP_INPUT_WINDOW_VSIZE,
                                           false,
                                           true,
                                           firRange,
                                           0,
                                           TP_CASC_LEN,
                                           TP_USE_COEFF_RELOAD,
                                           TP_NUM_OUTPUTS,
                                           TP_DUAL_IP,
                                           TP_API,
                                           TP_SAT>;

    /**
     * Base value FIFO Depth, in words (32-bits).
     * Used with streaming interface.
     * During graph construction, FIFO constraint is applied to all broadcast input nets, which are created as part of a
     *cascaded design connection.
     * Constraint is added after the input port has been broadcast and consists of:
     * - a base value of FIFO depth. This minimum value is applied to all broadcast nets.
     * - a variable part, where each net will be assigned an additional value based on interpolation/decimation and
     *position in the cascade.
     **/
    int baseFifoDepth = 32;
    int MARGIN_BYTESIZE =
        fnFirMargin<((TP_FIR_LEN + TP_INTERPOLATE_FACTOR - 1) / TP_INTERPOLATE_FACTOR), TT_DATA>() * sizeof(TT_DATA);

    int calculate_fifo_depth(int kernelPos) {
        // In FIFO mode, FIFO size has to be a multiple of 128 bits.
        // In FIFO mode, BD length has to be >= 128 bytes!
        // Conservative assumptions need to be made here.
        // For an overall decimation factor, with increasing decimation factor,
        // the amount of input stream data required to produce output samples also increases.
        // This strains the FIFOs closer to the beginning of the chain comparably more
        // than the ones closest to the output.
        // For an overall interpolation design, with increasing interpolation factor,
        // the amount of output stream data produced also increases.
        // Last kernel in the chain is limited by output bandwidth, which puts pressure on input stream.
        // Upstream kernels send data through cascade interface and so to keep the data flow without backpressure,
        // kernels closer to output stream require more FIFO storage.
        //
        // Conservative assumptions need to be made here, as mapper may place multiple buffers in
        // each of the memory banks, that may introduce Memory conflicts.
        // On top of that, the placement of input source wrt broadcast kernel inputs may introduce significant routing
        // delays.
        // which may have an adverse effect on the amount of FIFO storage available for filter design purposes.
        int fifoStep =
            (TP_INTERPOLATE_FACTOR < TP_DECIMATE_FACTOR
                 ? (TP_CASC_LEN - kernelPos + 1) * CEIL(TP_INTERPOLATE_FACTOR, TP_DECIMATE_FACTOR) / TP_DECIMATE_FACTOR
                 : (kernelPos + 1) * CEIL(TP_DECIMATE_FACTOR, TP_INTERPOLATE_FACTOR) / TP_INTERPOLATE_FACTOR);

        int fifoDepth = baseFifoDepth + 32 * fifoStep;
        // limit size at a single memory bank - 8kB
        const int memBankSize = 2048; // 32-bit words
        int fifoDepthCap = fifoDepth < memBankSize ? fifoDepth : memBankSize;
        return fifoDepthCap;
    }

    static constexpr unsigned int rnd = TP_DECIMATE_FACTOR * TP_SSR;

    template <int dim>
    struct ssr_params : public fir_params_defaults {
        static constexpr int Bdim = dim;
        using BTT_DATA = TT_DATA;
        using BTT_COEFF = TT_COEFF;
        static constexpr unsigned int BTP_FIR_LEN = TP_FIR_LEN;
        static constexpr unsigned int BTP_FIR_RANGE_LEN =
            TRUNC((CEIL(TP_FIR_LEN, rnd) / (TP_CASC_LEN * TP_SSR * TP_PARA_DECI_POLY)),
                  TP_DECIMATE_FACTOR / TP_PARA_DECI_POLY);
        static constexpr unsigned int BTP_DECIMATE_FACTOR = TP_DECIMATE_FACTOR;
        static constexpr unsigned int BTP_INTERPOLATE_FACTOR = TP_INTERPOLATE_FACTOR;
        static constexpr unsigned int BTP_SHIFT = TP_SHIFT;
        static constexpr unsigned int BTP_RND = TP_RND;
        static constexpr unsigned int BTP_INPUT_WINDOW_VSIZE = TP_INPUT_WINDOW_VSIZE / (TP_SSR * TP_PARA_DECI_POLY);
        static constexpr unsigned int BTP_CASC_LEN = TP_CASC_LEN;
        static constexpr unsigned int BTP_USE_COEFF_RELOAD = TP_USE_COEFF_RELOAD;
        static constexpr unsigned int BTP_NUM_OUTPUTS = TP_NUM_OUTPUTS;
        static constexpr unsigned int BTP_DUAL_IP = TP_DUAL_IP;
        static constexpr unsigned int BTP_API = TP_API;
        static constexpr unsigned int BTP_SSR = TP_SSR;
        static constexpr unsigned int BTP_PARA_DECI_POLY = TP_PARA_DECI_POLY;
        static constexpr unsigned int BTP_COEFF_PHASES = TP_SSR;
        static constexpr unsigned int BTP_COEFF_PHASES_LEN = TP_FIR_LEN;
        static constexpr unsigned int BTP_PARA_INTERP_POLY = TP_PARA_INTERP_POLY;
        static constexpr int BTP_MODIFY_MARGIN_OFFSET = 0;
        static constexpr unsigned int BTP_SAT = TP_SAT;
    };
    static constexpr unsigned int lastSSRDim = (TP_SSR * TP_SSR) - 1;

    static constexpr const char* srcFileName = decomposer::getSourceFile<ssr_params<lastSSRDim> >();

    using lastSSRKernel = ssr_kernels<ssr_params<lastSSRDim>, fir_resampler_tl>;

    template <unsigned int CL>
    struct tmp_ssr_params : public ssr_params<0> {
        static constexpr unsigned int BTP_FIR_LEN = CEIL(TP_FIR_LEN, rnd) / TP_SSR;
        static constexpr unsigned int BTP_CASC_LEN = CL;
    };

    template <int pos, int CLEN, int T_FIR_LEN, typename T_D, typename T_C, unsigned int T_IF, unsigned int T_DF>
    static constexpr unsigned int clRecurser() {
        if
            constexpr(fir_resampler_tl<tmp_ssr_params<CLEN> >::template fnCheckIfFits<pos, CLEN, T_FIR_LEN, T_D, T_C,
                                                                                      T_DF, T_IF>() == 1) {
                if
                    constexpr(pos == 0) { return CLEN; }
                else {
                    return clRecurser<pos - 1, CLEN, T_FIR_LEN, T_D, T_C, T_DF, T_IF>();
                }
            }
        else {
            return clRecurser<CLEN, CLEN + 1, T_FIR_LEN, T_D, T_C, T_DF, T_IF>();
        }
    }

    template <unsigned int T_API, typename T_D>
    static constexpr unsigned int getMaxTapsPerKernel() {
        if
            constexpr(T_API == 0) { return 256; }
        else {
            constexpr unsigned int m_kSamplesInBuff = fnSamplesIn1024<T_D>();
            constexpr unsigned int m_kDataLoadVsize = (32 / sizeof(T_D));
            return m_kSamplesInBuff - m_kDataLoadVsize;
        }
    }

    template <typename T_D, typename T_C, int T_PORTS>
    static constexpr unsigned int getOptTapsPerKernel() {
        unsigned int optTaps = getOptTapsPerKernelSrAsym<T_D, T_C, T_PORTS>();
        return optTaps;
    };

    /**
     * The conditional input array data to the function.
     * This input is (generated when TP_CASC_IN == CASC_IN_TRUE) either a cascade input.
     **/
    port_conditional_array<output, (TP_CASC_IN == CASC_IN_TRUE), TP_SSR> casc_in;

   public:
    /**
     * Size of the Input port array in SSR operation mode
     **/
    static constexpr unsigned int IN_SSR = TP_SSR * TP_PARA_DECI_POLY;

    /**
     * OUT_SSR defines the number of output paths, equal to ``TP_SSR * TP_PARA_INTERP_POLY``.
     **/

    static constexpr unsigned int OUT_SSR = TP_SSR * TP_PARA_INTERP_POLY;

    /**
     * The array of kernels that will be created and mapped onto AIE tiles.
     * Number of kernels (``TP_CASC_LEN * TP_SSR``) will be connected with each other by cascade interface.
     **/
    kernel m_firKernels[TP_CASC_LEN * TP_SSR * TP_SSR * TP_PARA_DECI_POLY * TP_PARA_INTERP_POLY];

    /**
     * The input data to the function. This input is either a window API of
     * samples of TT_DATA type or stream API (depending on TP_API).
     * Note: Margin is added internally to the graph, when connecting input port
     * with kernel port. Therefore, margin should not be added when connecting
     * graph to a higher level design unit.
     * Margin size (in Bytes) equals to TP_FIR_LEN rounded up to a nearest
     * multiple of 32 bytes.
     **/
    port_array<input, IN_SSR> in;

    /**
     * The output data from the function. This output is either a window API of
     * samples of TT_DATA type or stream API (depending on TP_API).
     * Number of output samples is determined by interpolation & decimation factors (if present).
     **/
    port_array<output, OUT_SSR> out;

    /**
     * The conditional input data to the function.
     * This input is (generated when TP_DUAL_IP == 1) either a window API of
     * samples of TT_DATA type or stream API (depending on TP_API).
     *
     **/
    port_conditional_array<input, (TP_DUAL_IP == 1), IN_SSR> in2;

    /**
     * The conditional array of input async ports used to pass run-time programmable (RTP) coefficients.
     * This port_conditional_array is (generated when TP_USE_COEFF_RELOAD == 1) an array of input ports, which size is
     *defined by TP_SSR.
     * Each port in the array holds a duplicate of the coefficient array, required to connect to each SSR input path.
     **/
    port_conditional_array<input, (TP_USE_COEFF_RELOAD == 1), TP_SSR * TP_PARA_INTERP_POLY> coeff;

    /**
     * The output data from the function.
     * This output is (generated when TP_NUM_OUTPUTS == 2) either a window API of
     * samples of TT_DATA type or stream API (depending on TP_API).
     * Number of output samples is determined by interpolation & decimation factors (if present).
     **/
    port_conditional_array<output, (TP_NUM_OUTPUTS == 2), OUT_SSR> out2;

    /**
     * Access function to get pointer to kernel (or first kernel in a chained configuration).
     **/
    kernel* getKernels() { return m_firKernels; };

    /**
     * Access function to get pointer to an indexed kernel.
     * @param[in] cascadePosition   an index to the kernel's position in the cascade.
     **/
    kernel* getKernels(int cascadePosition) { return m_firKernels + cascadePosition; };

    /**
     * Access function to get pointer to net of the ``` in ``` port.
     * Nets only get assigned when streaming interface is being broadcast, i.e.
     * nets only get used when TP_API == 1 and TP_CASC_LEN > 1
     * @param[in] cascadePosition   an index to the kernel's position in the cascade.
     * @param[in] ssrOutPathIndex     an index to the output data SSR Path.
     * @param[in] ssrInPhaseIndex     an index to the input data SSR Phase
     * @param[in] paraDeciPolyIndex   an index to the input data Parallel Decimation Polyphase
     * @param[in] paraInterpPolyIndex an index to the input data Parallel Interpolation Polyphase
     **/
    connect<stream, stream>* getInNet(
        int cascadePosition, int ssrInPhaseIndex, int ssrOutPathIndex, int paraDeciPolyIndex, int paraInterpPolyIndex) {
        return net[paraInterpPolyIndex][paraDeciPolyIndex][ssrOutPathIndex][ssrInPhaseIndex][cascadePosition];
    };

    connect<stream, stream>* getInNet(int cascadePosition) { return getInNet(cascadePosition, 0, 0, 0, 0); };

    /**
     * Access function to get pointer to net of the ``` in2 ``` port,
     * when port is being generated, i.e. when TP_DUAL_IP == 1.
     * Nets only get assigned when streaming interface is being broadcast, i.e.
     * nets only get used when TP_API == 1 and TP_CASC_LEN > 1
     * @param[in] cascadePosition     an index to the kernel's position in the cascade.
     * @param[in] ssrOutPathIndex     an index to the output data SSR Path.
     * @param[in] ssrInPhaseIndex     an index to the input data SSR Phase
     * @param[in] paraDeciPolyIndex   an index to the input data Parallel Decimation Polyphase
     * @param[in] paraInterpPolyIndex an index to the input data Parallel Interpolation Polyphase
     **/
    connect<stream, stream>* getIn2Net(
        int cascadePosition, int ssrInPhaseIndex, int ssrOutPathIndex, int paraDeciPolyIndex, int paraInterpPolyIndex) {
        return net2[paraInterpPolyIndex][paraDeciPolyIndex][ssrOutPathIndex][ssrInPhaseIndex][cascadePosition];
    };

    connect<stream, stream>* getIn2Net(int cascadePosition) { return getIn2Net(cascadePosition, 0, 0, 0, 0); };

    /**
    * @brief Access function to get kernel's architecture (or first kernel's architecture in a chained configuration).
    **/
    unsigned int getKernelArchs() {
        // return the architecture for first kernel in the design (only one for single kernel designs).
        // First kernel will always be the slowest of the kernels and so it will reflect on the designs performance
        // best.
        return 0;
        // return firstKernelClass::get_m_kArch();
    };

    /**
     * Access function to get kernels PoliphaseLaneAlias.
     **/
    unsigned int getPolyphaseLaneAlias() {
        // return PoliphaseLaneAlias
        return 0;
        // return firstKernelClass::get_polyphaseLaneAlias();
    };

    /**
     * @brief This is the constructor function for the FIR graph with static coefficients.
     * @param[in] taps   a reference to the std::vector array of taps values of type TT_COEFF.
     **/
    fir_resampler_graph(const std::vector<TT_COEFF>& taps) : net{}, net2{} {
        decomposer::polyphase_decomposer<ssr_params<lastSSRDim> >::create(m_firKernels, taps);
        decomposer::polyphase_decomposer<ssr_params<lastSSRDim> >::create_connections(
            m_firKernels, &in[0], in2, &out[0], out2, coeff, net, net2, casc_in, srcFileName);
    };

    /**
     * @brief This is the constructor function for the FIR graph with reloadable coefficients.
     **/
    fir_resampler_graph() : net{}, net2{} {
        decomposer::polyphase_decomposer<ssr_params<lastSSRDim> >::create(m_firKernels);
        decomposer::polyphase_decomposer<ssr_params<lastSSRDim> >::create_connections(
            m_firKernels, &in[0], in2, &out[0], out2, coeff, net, net2, casc_in, srcFileName);
    };

    /**
    * @brief Access function to get Graphs minimum cascade length for a given configuration.
    * @tparam T_FIR_LEN tap length of the fir filter
    * @tparam T_API interface type : 0 - window, 1 - stream
    * @tparam T_D data type
    * @tparam T_C coeff type
    * @tparam T_IF interpolation factor
    * @tparam T_DF decimation factor
    * @tparam SSR Super Sample Rate operation - defaults to: 1.
    **/
    template <int T_FIR_LEN,
              int T_API,
              typename T_D,
              typename T_C,
              unsigned int T_IF,
              unsigned int T_DF,
              unsigned int SSR = 1>
    static constexpr unsigned int getMinCascLen() {
        constexpr int kMaxTaps = getMaxTapsPerKernel<T_API, T_D>();
        constexpr int firLenPerSSR = CEIL(T_FIR_LEN, SSR) / SSR;
        constexpr int cLenMin = xf::dsp::aie::getMinCascLen<firLenPerSSR * T_DF / T_IF, kMaxTaps>();
        if
            constexpr(T_API == 0) { return cLenMin; }
        else {
            return clRecurser<cLenMin - 1, cLenMin, firLenPerSSR, T_D, T_C, T_DF, T_IF>();
        }
        return cLenMin;
    };

    /**
    * @brief Access function to get graph's cascade length to obtain maximum performance for streaming configurations.
    * @tparam T_FIR_LEN tap length of the fir filter
    * @tparam T_D data type
    * @tparam T_C coeff type
    * @tparam T_API interface type : 0 - window, 1 - stream
    * @tparam T_PORTS single/dual input and output ports. 1 : single, 2 : dual
    * @tparam T_IF interpolation factor
    * @tparam T_DF decimation factor
    **/
    template <int T_FIR_LEN, typename T_D, typename T_C, int T_API, int T_PORTS, unsigned int T_IF, unsigned int T_DF>
    static constexpr unsigned int getOptCascLen() {
        constexpr int kMaxTaps = getMaxTapsPerKernel<T_API, T_D>();
        constexpr int kRawOptTaps = getOptTapsPerKernel<T_D, T_C, T_PORTS>();
        return xf::dsp::aie::getOptCascLen<kMaxTaps, kRawOptTaps, (T_FIR_LEN * T_DF / T_IF)>();
    };
};
}
}
}
}
} // namespace braces
#endif //_DSPLIB_FIR_RESAMPLER_GRAPH_HPP_
