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
#ifndef _DSPLIB_FIR_INTERPOLATE_ASYM_GRAPH_HPP_
#define _DSPLIB_FIR_INTERPOLATE_ASYM_GRAPH_HPP_

// This file holds the definition of the Asymmetric Interpolation FIR graph class
/**
 * @file fir_interpolate_asym_graph.hpp
 *
 **/

#include <adf.h>
#include <vector>
#include "fir_graph_utils.hpp"

#include "fir_interpolate_asym.hpp"
#include "fir_common_traits.hpp"
#include "fir_decomposer_utils.hpp"
#include "widget_api_cast.hpp"

namespace xf {
namespace dsp {
namespace aie {
namespace fir {
namespace interpolate_asym {

using namespace adf;

/**
 * @brief fir_interpolate_asym is an Asymmetric Interpolation FIR filter
 *
 * @ingroup fir_graphs
 *
 * These are the templates to configure the Asymmetric Interpolation FIR class.
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
 *         range 1 to 16.
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
 Group banks. \n
 *         As a result, maximum number of samples processed by the graph is limited by the size of Memory Group. \n
 *         When TP_API is set to 1, samples are processed directly from the stream inputs and no buffering takes place.
 \n
 *         In such case, maximum number of samples processed by the graph is limited to 32-bit value (4.294B samples per
 iteration).  \n
 *         \n
 *         Note: For SSR configurations (TP_SSR>1), the input data must be split over multiple ports,
 *         where each successive sample is sent to a different input port in a round-robin fashion. \n
 *         As a result, each SSR input path will process a fraction of the frame defined by the TP_INPUT_WINDOW_VSIZE.
 \n
 *         The number of values in the output window will be TP_INPUT_WINDOW_VSIZE
 *         multiplied by TP_INTERPOLATE_FACTOR. \n
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
 *         Note: when used, async port: ```port_conditional_array<input, (TP_USE_COEFF_RELOAD == 1), TP_SSR *
 TP_PARA_INTERP_POLY> coeff;``` will be added to the FIR. \n
 *         \n
 *         Note: the size of the port array is equal to the total number of output paths  (TP_SSR *
 TP_PARA_INTERP_POLY).  \n
 *         Each port should contain the same taps array content, i.e. each additional port must be a duplicate of the
 coefficient array. \n
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
 * @tparam TP_PARA_INTERP_POLY sets the number of interpolator polyphases over which the coefficients will be split to
 enable parallel computation of the outputs. \n
 *         The polyphases are executed in parallel, output data is produced by each polyphase directly. \n
 *         TP_PARA_INTERP_POLY does not affect the number of input data paths. \n
 *         There will be TP_SSR input phases irrespective of the value of TP_PARA_INTERP_POLY. \n
 *         TP_PARA_INTERP_POLY = TP_INTERPOLATE_FACTOR will result in an interpolate factor of polyphases,
 * where each kernel is a single rate filters. \n
 *          TP_PARA_INTERP_POLY < TP_INTERPOLATE_FACTOR will result in the kernels in the polyphase branches operating
 as
 * independent interpolators. \n
 * @tparam TP_SAT describes the selection of saturation to be applied during the shift down stage of processing. \n
 *         TP_SAT accepts unsigned integer values, where:
 *         - 0: none           = No saturation is performed and the value is truncated on the MSB side.
 *         - 1: saturate       = Default. Saturation rounds an n-bit signed value
 *         in the range [- ( 2^(n-1) ) : +2^(n-1) - 1 ].
 *         - 3: symmetric      = Controls symmetric saturation. Symmetric saturation rounds
 *         an n-bit signed value in the range [- ( 2^(n-1) -1 ) : +2^(n-1) - 1 ]. \n
 *         TP_PARA_INTERP_POLY can be used in combination with TP_SSR.
 *         The number of AIEs used is given by TP_PARA_INTERP_POLY*TP_SSR^2 * TP_CASC_LEN. \n
 *
 **/

template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_INTERPOLATE_FACTOR,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_CASC_LEN = 1,
          unsigned int TP_USE_COEFF_RELOAD = 0,
          unsigned int TP_DUAL_IP = 0,
          unsigned int TP_NUM_OUTPUTS = 1,
          unsigned int TP_API = 0,
          unsigned int TP_SSR = 1,
          unsigned int TP_PARA_INTERP_POLY = 1,
          unsigned int TP_SAT = 1>
class fir_interpolate_asym_graph : public graph {
   private:
#if (__STREAMS_PER_TILE__ == 1)
    // Note: TP_SSR decomposition uses more tiles than TP_PARA_INTERP_POLY decomposition to offer equivalent gain in
    // input/output bandwidth increase. Please use parallel polyphases decomposition in the first instance.
    static_assert(TP_SSR == 1 || (TP_PARA_INTERP_POLY == TP_INTERPOLATE_FACTOR),
                  "ERROR: TP_SSR mode not availalble "
                  "when design is not fully decomposed. "
                  "Please use Parallel polyphase decomposition (TP_PARA_INTERP_POLY == TP_INTERPOLATE_FACTOR) before "
                  "increasing T_SSR");
    // Device doesn't support dual input streams
    static_assert(TP_API == 0 || TP_DUAL_IP == 0, "ERROR: Dual input stream ports not supported on this device.");
    static_assert(TP_API == 0 || TP_NUM_OUTPUTS == 1,
                  "ERROR: Multiple output stream ports not supported on this device.");

#endif
    static constexpr unsigned int TP_CASC_IN = CASC_IN_FALSE;

    static_assert(TP_CASC_LEN <= 40, "ERROR: Unsupported Cascade length");

    static_assert(TP_INPUT_WINDOW_VSIZE % TP_SSR == 0,
                  "ERROR: Unsupported frame size. TP_INPUT_WINDOW_VSIZE must be divisible by TP_SSR");
    // Dual input ports offer no throughput gain if port api is windows.
    // Therefore, dual input ports are only supported with streams and not windows.
    static_assert(TP_API == USE_STREAM_API || TP_DUAL_IP == DUAL_IP_SINGLE,
                  "ERROR: Dual input ports only supported when port API is a stream. ");

    static constexpr unsigned int kMaxTapsPerKernel = 256;
    // Limit FIR length per kernel. Longer FIRs may exceed Program Memory and/or system memory combined with window
    // buffers may exceed Memory Module size
    static_assert((CEIL(TP_FIR_LEN, TP_SSR) / TP_SSR) / TP_CASC_LEN <= kMaxTapsPerKernel,
                  "ERROR: Requested FIR length and Cascade length exceeds supported number of taps per kernel. Please "
                  "increase the cascade length to accommodate the FIR design.");

    // Limit FIR length for reloadable coeffs. Reloadable coeffs need a storage space that contributes to system memory
    // exceeding Memory Module size.
    static_assert(TP_USE_COEFF_RELOAD == 0 ||
                      CEIL(TP_FIR_LEN, (TP_SSR * TP_CASC_LEN)) / TP_SSR / TP_CASC_LEN <= kMaxTapsPerKernel,
                  "ERROR: Exceeded maximum supported FIR length with reloadable coefficients. Please limit the FIR "
                  "length or disable coefficient reload.");

    static constexpr unsigned int kMemoryModuleSize = __DATA_MEM_BYTES__;
    static constexpr unsigned int inBufferSize = ((TP_FIR_LEN + TP_INPUT_WINDOW_VSIZE) * sizeof(TT_DATA));
    // Requested Input Window buffer exceeds memory module size
    static_assert(TP_API != 0 || inBufferSize < kMemoryModuleSize,
                  "ERROR: Input Window size (based on requested window size and FIR length margin) exceeds Memory "
                  "Module size of 32kB for AIE-1 and 64kB for AIE-ML devices.");

    static constexpr unsigned int outBufferSize = (TP_INTERPOLATE_FACTOR * TP_INPUT_WINDOW_VSIZE * sizeof(TT_DATA));
    // Requested Output Window buffer exceeds memory module size
    static_assert(TP_API != 0 || outBufferSize < kMemoryModuleSize,
                  "ERROR: Input Window size (based on requested window size and FIR length margin) exceeds Memory "
                  "Module size of 32kB for AIE-1 and 64kB for AIE-ML devices.");
    // SSR is only supported for streaming API
    static_assert(TP_API == 1 || TP_SSR == 1,
                  "ERROR: SSR > 1 is only supported for streaming API. Set TP_API = 1 to enable streaming API");

    // static_assert(((TP_INTERPOLATE_FACTOR / TP_PARA_INTERP_POLY) != TP_SSR) || TP_SSR == 1,
    //               "ERROR: Currently, we do not support SSR=(INTERPOLATE_FACTOR/TP_PARA_INTERP_POLY). Please set SSR
    //               to "
    //               "the next higher value "
    //               "to get as high a performance");

    // static_assert(TP_PARA_INTERP_POLY == 1 || TP_USE_COEFF_RELOAD == 0,
    //               "ERROR: Reloadable coefficients not supported with multiple parallel interpolation polyphases.");
    using casc_net_array = std::array<connect<stream, stream>*, TP_CASC_LEN>;
    using ssr_net_array = std::array<std::array<casc_net_array, TP_SSR>, TP_SSR>;
    // a 2-D array for decimator and interpolator polyphases.
    using polyphase_net_array = std::array<std::array<ssr_net_array, 1>, TP_PARA_INTERP_POLY>;

    polyphase_net_array net;
    polyphase_net_array net2;

    /**
     * Base value FIFO Depth, in words (32-bits).
     * Used with streaming interface.
     * During graph construction, FIFO constraint is applied to all broadcast input nets, which are created as part of a
     *cascaded design connection.
     * Constraint is added after the input port has been broadcast and consists of:
     * - a base value of FIFO depth. This minimum value is applied to all broadcast nets.
     * - a variable part, where each net will be assigned an additional value based on interpolation and position in the
     *cascade.
     **/
    int baseFifoDepth = 32;
    int cascPos = TP_DUAL_IP == 0 ? 1 : 2;

    int calculate_fifo_depth(int kernelPos) {
        // In FIFO mode, FIFO size has to be a multiple of 128 bits.
        // In FIFO mode, BD length has to be >= 128 bytes!
        // Conservative assumptions need to be made here.
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
        int fifoStep = (TP_CASC_LEN - kernelPos + 1) * (TP_INTERPOLATE_FACTOR);

        int fifoDepth = baseFifoDepth + 32 * fifoStep;
        // limit size at a single memory bank - 8kB
        const int memBankSize = 2048; // 32-bit words
        int fifoDepthCap = fifoDepth < memBankSize ? fifoDepth : memBankSize;
        return fifoDepthCap;
    }

    static constexpr unsigned int lastSSRDim = (TP_SSR * TP_SSR) - 1;

    struct ssr_params : public fir_params_defaults {
        static constexpr int Bdim = lastSSRDim;
        using BTT_DATA = TT_DATA;
        using BTT_COEFF = TT_COEFF;
        static constexpr unsigned int BTP_FIR_LEN = TP_FIR_LEN;
        static constexpr unsigned int BTP_FIR_RANGE_LEN = TP_INTERPOLATE_FACTOR;
        static constexpr unsigned int BTP_INTERPOLATE_FACTOR = TP_INTERPOLATE_FACTOR;
        static constexpr unsigned int BTP_SHIFT = TP_SHIFT;
        static constexpr unsigned int BTP_RND = TP_RND;
        static constexpr unsigned int BTP_INPUT_WINDOW_VSIZE = TP_INPUT_WINDOW_VSIZE / TP_SSR;
        static constexpr unsigned int BTP_CASC_LEN = TP_CASC_LEN;
        static constexpr unsigned int BTP_USE_COEFF_RELOAD = TP_USE_COEFF_RELOAD;
        static constexpr unsigned int BTP_NUM_OUTPUTS = TP_NUM_OUTPUTS;
        static constexpr unsigned int BTP_DUAL_IP = TP_DUAL_IP;
        static constexpr unsigned int BTP_API = TP_API;
        static constexpr unsigned int BTP_SSR = TP_SSR;
        static constexpr unsigned int BTP_COEFF_PHASES = TP_SSR;
        static constexpr unsigned int BTP_COEFF_PHASES_LEN = TP_FIR_LEN;
        static constexpr unsigned int BTP_PARA_INTERP_POLY = TP_PARA_INTERP_POLY;
        static constexpr int BTP_MODIFY_MARGIN_OFFSET = 0;
        static constexpr unsigned int BTP_SAT = TP_SAT;
    };

    // src file might not be interpolate_asym - use decomposer utility to get sourcefile.
    static constexpr const char* srcFileName = decomposer::getSourceFile<ssr_params>();

    template <unsigned int CL>
    struct tmp_ssr_params : public ssr_params {
        static constexpr unsigned int BTP_FIR_LEN = CEIL(TP_FIR_LEN, TP_SSR) / TP_SSR;
        static constexpr unsigned int BTP_CASC_LEN = CL;
    };

    template <int pos, int CLEN, int T_FIR_LEN, typename T_D, typename T_C, unsigned int IP_PORTS, unsigned int T_IF>
    static constexpr unsigned int clRecurser() {
        if
            constexpr(fir_interpolate_asym_tl<tmp_ssr_params<CLEN> >::template fnCheckIfFits<pos, CLEN, T_FIR_LEN, T_D,
                                                                                             T_C, IP_PORTS, T_IF>() ==
                      1) {
                if
                    constexpr(pos == 0) { return CLEN; }
                else {
                    return clRecurser<pos - 1, CLEN, T_FIR_LEN, T_D, T_C, IP_PORTS, T_IF>();
                }
            }
        else {
            return clRecurser<CLEN, CLEN + 1, T_FIR_LEN, T_D, T_C, IP_PORTS, T_IF>();
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
     * OUT_SSR defines the number of output paths, equal to ``TP_SSR * TP_PARA_INTERP_POLY``.
     **/
    static constexpr unsigned int OUT_SSR = TP_SSR * TP_PARA_INTERP_POLY;

    /**
     * The input data to the function. This input is either a window API of
     * samples of TT_DATA type or stream API (depending on TP_API).
     * Note: Margin is added internally to the graph, when connecting input port
     * with kernel port. Therefore, margin should not be added when connecting
     * graph to a higher level design unit.
     * Margin size (in Bytes) equals to TP_FIR_LEN rounded up to a nearest
     * multiple of 32 bytes.
     **/
    port_array<input, TP_SSR> in;

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
    port_conditional_array<input, (TP_DUAL_IP == 1), TP_SSR> in2;

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
     * The array of kernels that will be created and mapped onto AIE tiles.
     * Number of kernels (``TP_CASC_LEN * TP_SSR``) will be connected with each other by cascade interface.
     **/
    kernel m_firKernels[TP_CASC_LEN * TP_SSR * TP_SSR * TP_PARA_INTERP_POLY];
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
     * @param[in] ssrOutPathIndex      an index to the output data Path.
     * @param[in] ssrInPhaseIndex     an index to the input data Phase
     * @param[in] paraPolyphaseOutIdx an index to the parallel output polyphase
     **/
    connect<stream, stream>* getInNet(int cascadePosition,
                                      int ssrOutPathIndex = 0,
                                      int ssrInPhaseIndex = 0,
                                      int paraPolyphaseOutIdx = 0) {
        return net[paraPolyphaseOutIdx][0][ssrOutPathIndex][ssrInPhaseIndex][cascadePosition];
    };

    /**
     * Access function to get pointer to net of the ``` in2 ``` port,
     * when port is being generated, i.e. when TP_DUAL_IP == 1.
     * Nets only get assigned when streaming interface is being broadcast, i.e.
     * nets only get used when TP_API == 1 and TP_CASC_LEN > 1
     * @param[in] cascadePosition   an index to the kernel's position in the cascade.
     * @param[in] ssrOutPathIndex      an index to the output data Path.
     * @param[in] ssrInPhaseIndex     an index to the input data Phase
     * @param[in] paraPolyphaseOutIdx an index to the parallel output polyphase
     **/
    connect<stream, stream>* getIn2Net(int cascadePosition,
                                       int ssrOutPathIndex = 0,
                                       int ssrInPhaseIndex = 0,
                                       int paraPolyphaseOutIdx = 0) {
        return net2[paraPolyphaseOutIdx][0][ssrOutPathIndex][ssrInPhaseIndex][cascadePosition];
    };

    /**
    * @brief Access function to get kernel's architecture (or first kernel's architecture in a chained configuration).
    **/
    unsigned int getKernelArchs() {
        constexpr unsigned int rnd = TP_SSR * TP_INTERPOLATE_FACTOR;
        constexpr unsigned int firLenPerSSR = CEIL(TP_FIR_LEN, rnd) / TP_SSR;
        constexpr unsigned int firRange =
            (TP_CASC_LEN == 1) ? firLenPerSSR : fnFirRange<firLenPerSSR, TP_CASC_LEN, 0, TP_INTERPOLATE_FACTOR>();
        // return the architecture for first kernel in the design (only one for single kernel designs).
        // First kernel will always be the slowest of the kernels and so it will reflect on the designs performance
        // best.
        return fir_interpolate_asym<TT_DATA, TT_COEFF, firLenPerSSR, TP_INTERPOLATE_FACTOR, TP_SHIFT, TP_RND,
                                    (TP_INPUT_WINDOW_VSIZE / TP_SSR), false, true, firRange, 0, TP_CASC_LEN,
                                    TP_USE_COEFF_RELOAD, TP_DUAL_IP, TP_NUM_OUTPUTS, TP_API, TP_SAT>::get_m_kArch();
    };

    /**
     * @brief This is the constructor function for the Asymmetric Interpolation FIR graph.
     * @param[in] taps   a reference to the std::vector array of taps values of type TT_COEFF.
     **/
    fir_interpolate_asym_graph(const std::vector<TT_COEFF>& taps) {
        decomposer::polyphase_decomposer<ssr_params>::create(m_firKernels, taps);
        decomposer::polyphase_decomposer<ssr_params>::create_connections(m_firKernels, &in[0], in2, &out[0], out2,
                                                                         coeff, net, net2, casc_in, srcFileName);
    }

    /**
     * @brief This is the constructor function for the FIR graph with reloadable coefficients.
     **/
    fir_interpolate_asym_graph() {
        decomposer::polyphase_decomposer<ssr_params>::create(m_firKernels);
        decomposer::polyphase_decomposer<ssr_params>::create_connections(m_firKernels, &in[0], in2, &out[0], out2,
                                                                         coeff, net, net2, casc_in, srcFileName);
    }

    /**
    * @brief Access function to get Graphs minimum cascade length for a given configuration.
    * @tparam T_FIR_LEN tap length of the fir filter
    * @tparam T_API interface type : 0 - window, 1 - stream
    * @tparam T_D data type
    * @tparam T_C coeff type
    * @tparam T_IF interpolation factor
    * @tparam SSR parallelism factor set for super sample rate operation
    * @tparam IP_PORTS single/dual input ports : 0 - single, 1 - dual
    **/
    template <int T_FIR_LEN,
              int T_API,
              typename T_D,
              typename T_C,
              unsigned int T_IF,
              unsigned int SSR,
              unsigned int IP_PORTS>
    static constexpr unsigned int getMinCascLen() {
        constexpr int kMaxTaps = getMaxTapsPerKernel<T_API, T_D>();
        constexpr int firLenPerSSR = CEIL(T_FIR_LEN, SSR) / SSR;
        constexpr int cLenMin = xf::dsp::aie::getMinCascLen<firLenPerSSR / T_IF, kMaxTaps>();
        if
            constexpr(T_API == 0) { return cLenMin; }
        else {
            return clRecurser<cLenMin - 1, cLenMin, firLenPerSSR, T_D, T_C, IP_PORTS, T_IF>();
        }
        return cLenMin;
    };

    /**
    * @brief Access function to get graph's cascade length to obtain maximum performance for streaming configurations.
    * @tparam T_FIR_LEN tap length of the fir filter
    * @tparam T_D data type
    * @tparam T_C coeff type
    * @tparam T_API interface type : 0 - window, 1 - stream
    * @tparam T_IF interpolation factor
    * @tparam SSR parallelism factor set for super sample rate operation
    * @tparam INT_POLY number of polyphases set for super sample rate operation
    **/
    template <int T_FIR_LEN,
              typename T_D,
              typename T_C,
              int T_API,
              unsigned int T_IF,
              unsigned int SSR,
              unsigned int INT_POLY>
    static constexpr unsigned int getOptCascLen() {
        constexpr int kMaxTaps = getMaxTapsPerKernel<T_API, T_D>();
        constexpr int kRawOptTaps = getOptTapsPerKernel<T_D, T_C, 1>();
        constexpr int firLenPerSSR = CEIL(T_FIR_LEN, SSR) / SSR;
        constexpr int tapsPerSSR = firLenPerSSR / (T_IF / INT_POLY);
        return xf::dsp::aie::getOptCascLen<kMaxTaps, kRawOptTaps, tapsPerSSR>();
    };
};
}
}
}
}
} // namespaces
#endif //_DSPLIB_FIR_INTERPOLATE_ASYM_GRAPH_HPP_
