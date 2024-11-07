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
#ifndef _DSPLIB_FIR_SR_ASYM_GRAPH_HPP_
#define _DSPLIB_FIR_SR_ASYM_GRAPH_HPP_
/*
The file captures the definition of the 'L2' graph level class for
the Single Rate Asymmetrical FIR library element.
*/
/**
 * @file fir_sr_asym_graph.hpp
 *
 **/

#include <adf.h>
#include <vector>
#include "fir_graph_utils.hpp"
#include <tuple>
#include <typeinfo>
#include "fir_sr_asym.hpp"
#include "fir_common_traits.hpp"
#include "fir_utils.hpp"
#include <adf/arch/aie_arch_properties.hpp>

namespace xf {
namespace dsp {
namespace aie {
namespace fir {
namespace sr_asym {
using namespace adf;

//--------------------------------------------------------------------------------------------------
// fir_sr_asym_graph template
//--------------------------------------------------------------------------------------------------
/**
 * @brief fir_sr_asym is a Asymmetric Single Rate FIR filter
 *
 * @ingroup fir_graphs
 *
 * These are the templates to configure the Asymmetric Single Rate FIR class.
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
 *         Note: Rounding modes ``rnd_sym_floor`` and ``rnd_sym_ceil`` are only supported on AIE-ML device. \n
 * @tparam TP_INPUT_WINDOW_VSIZE describes the number of samples processed by the graph
 *         in a single iteration run.  \n
 *         When TP_API is set to 0, samples are buffered and stored in a ping-pong window buffer mapped onto Memory
 *Group banks. \n
 *         As a result, maximum number of samples processed by the graph is limited by the size of Memory Group. \n
 *         When TP_API is set to 1, samples are processed directly from the stream inputs and no buffering takes place.
 *\n
 *         In such case, maximum number of samples processed by the graph is limited to 32-bit value (4.294B samples per
 *iteration).  \n
 *         \n
 *         Note: For SSR configurations (TP_SSR>1), the input data must be split over multiple ports,
 *         where each successive sample is sent to a different input port in a round-robin fashion. \n
 *         As a result, each SSR input path will process a fraction of the frame defined by the TP_INPUT_WINDOW_VSIZE.
 *\n

 *         Note: Margin size should not be included in TP_INPUT_WINDOW_VSIZE.
 * @tparam TP_CASC_LEN describes the number of AIE processors to split the operation
 *         over.  \n This allows resource to be traded for higher performance.
 *         TP_CASC_LEN must be in the range 1 (default) to 40.
 * @tparam TP_USE_COEFF_RELOAD allows the user to select if runtime coefficient
 *         reloading should be used. \n When defining the parameter:
 *         - 0 = static coefficients, defined in filter constructor,
 *         - 1 = reloadable coefficients, passed as argument to runtime function. \n
 *         \n
 *         Note: when used, async port: ```port_conditional_array<input, (TP_USE_COEFF_RELOAD == 1), TP_SSR> coeff;```
 *         will be added to the FIR. \n
 *         \n
 *         Note: the size of the port array is equal to the total number of output paths  (TP_SSR). \n
 *         Each port should contain the same taps array content, i.e. each additional port must be a duplicate of the
 *         coefficient array. \n
 * @tparam TP_NUM_OUTPUTS sets the number of ports to broadcast the output to. \n
 *         \n
 *         Note: when used, optional port: ``` std::array<port<output>, TP_SSR> out2; ``` will be added to the FIR. \n
 *         \n
 *         Note: For Windows API, additional output an exact copy of the data. \n
 *         Stream API interleaves the output data with a 128-bit pattern, e.g.: \n
 *         - samples 0-3 to be sent over stream0 for cint16 data type, \n
 *         - samples 4-7 to be sent over stream1 for cint16 data type. \n
 * @tparam TP_DUAL_IP allows 2 stream inputs to be connected to FIR, increasing available throughput. \n
 *         When set to 0, single stream will be connected as FIRs input. \n
 *         When set to 1, two stream inputs will be connected. \n
 *         In such case data should be organized in 128-bit interleaved pattern, e.g.: \n
 *         - samples 0-3 to be sent over stream0 for cint16 data type, \n
 *         - samples 4-7 to be sent over stream1 for cint16 data type. \n
 *         \n
 *         Note: Dual input streams offer no throughput gain if only single output stream would be used.
 *         Therefore, dual input streams are only supported with 2 output streams. \n
 *         \n
 *         Note: Dual input ports offer no throughput gain if port api is windows.
 *         Therefore, dual input ports are only supported with streams and not windows.
 * @tparam TP_API specifies if the input/output interface should be window-based or stream-based.  \n
 *         The values supported are 0 (window API) or 1 (stream API).
 * @tparam TP_SSR specifies the number of parallel input/output paths where samples are interleaved between paths,
 *         giving an overall higher throughput.   \n
 *         A TP_SSR of 1 means just one output leg and 1 input phase, and is the backwards compatible option. \n
 *         The number of AIEs used is given by ``TP_SSR^2 * TP_CASC_LEN``. \n
 * @tparam TP_SAT describes the selection of saturation to be applied during the shift down stage of processing. \n
 *         TP_SAT accepts unsigned integer values, where:
 *         - 0: none           = No saturation is performed and the value is truncated on the MSB side.
 *         - 1: saturate       = Default. Saturation rounds an n-bit signed value
 *         in the range [- ( 2^(n-1) ) : +2^(n-1) - 1 ].
 *         - 3: symmetric      = Controls symmetric saturation. Symmetric saturation rounds
 *         an n-bit signed value in the range [- ( 2^(n-1) -1 ) : +2^(n-1) - 1 ]. \n
 **/

template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_CASC_LEN = 1,
          unsigned int TP_USE_COEFF_RELOAD = 0,
          unsigned int TP_NUM_OUTPUTS = 1,
          unsigned int TP_DUAL_IP = 0,
          unsigned int TP_API = 0,
          unsigned int TP_SSR = 1,
          unsigned int TP_SAT = 1>
/**
 **/
class fir_sr_asym_graph : public graph {
   private:
    static constexpr bool TP_CASC_IN = CASC_IN_FALSE;   // should be unsigned int if exposed on graph interface
    static constexpr bool TP_CASC_OUT = CASC_OUT_FALSE; // should be unsigned int if exposed on graph interface
    // 3d array for storing net information.
    // address[inPhase][outPath][cascPos]
    std::array<std::array<std::array<connect<stream, stream>*, TP_CASC_LEN>, TP_SSR>, TP_SSR> net;
    std::array<std::array<std::array<connect<stream, stream>*, TP_CASC_LEN>, TP_SSR>, TP_SSR> net2;

    // This figure is mostly guesswork and will likely need changed for specfic systems.
    // If this is lower, then SSR designs (ssr2 casc2) typically suffer from stream stalls, which ruins QoR.
    static constexpr unsigned int fifo_depth_multiple = 40;

    /**
     * Base value FIFO Depth, in words (32-bits).
     * Used with streaming interface.
     * During graph construction, FIFO constraint is applied to all broadcast input nets, which are created as part of a
     *cascaded design connection.
     * Constraint is added after the input port has been broadcast and consists of:
     * - a base value of FIFO depth. This minimum value is applied to all broadcast nets.
     * - a variable part, where each net will be assigned an additional value based on position in the cascade.
     **/
    int baseFifoDepth = 32;

    int calculate_fifo_depth(int kernelPos) {
        // In FIFO mode, FIFO size has to be a multiple of 128 bits.
        // In FIFO mode, BD length has to be >= 128 bytes!
        // Conservative assumptions need to be made here.
        // For an overall decimation factor, with increasing decimation factor,
        // the amount of input stream data required to produce output samples also increases.
        // This strains the FIFOs closer to the beginning of the chain comparably more
        // than the ones closest to the output.
        //
        // Conservative assumptions need to be made here, as mapper may place multiple buffers in
        // each of the memory banks, that may introduce Memory conflicts.
        // On top of that, the placement of input source wrt broadcast kernel inputs may introduce significant routing
        // delays.
        // which may have an adverse effect on the amount of FIFO storage available for filter design purposes.
        int fifoStep = (TP_CASC_LEN - kernelPos + 1);

        int fifoDepth = baseFifoDepth + fifo_depth_multiple * fifoStep;
        // limit size at a single memory bank - 8kB
        const int memBankSize = 2048; // 32-bit words
        int fifoDepthCap = fifoDepth < memBankSize ? fifoDepth : memBankSize;
        return fifoDepthCap;
    }

    template <int dim, int firlen = TP_FIR_LEN>
    struct ssr_params : public fir_params_defaults {
        static constexpr int Bdim = dim;
        using BTT_DATA = TT_DATA;
        using BTT_COEFF = TT_COEFF;
        static constexpr int BTP_FIR_LEN = firlen;
        static constexpr int BTP_SHIFT = TP_SHIFT;
        static constexpr int BTP_RND = TP_RND;
        static constexpr int BTP_INPUT_WINDOW_VSIZE = TP_INPUT_WINDOW_VSIZE / TP_SSR;
        static constexpr int BTP_CASC_LEN = TP_CASC_LEN;
        static constexpr int BTP_USE_COEFF_RELOAD = TP_USE_COEFF_RELOAD;
        static constexpr int BTP_NUM_OUTPUTS = TP_NUM_OUTPUTS;
        static constexpr int BTP_DUAL_IP = TP_DUAL_IP;
        static constexpr int BTP_API = TP_API;
        static constexpr int BTP_SSR = TP_SSR;
        static constexpr int BTP_COEFF_PHASES = TP_SSR;
        static constexpr int BTP_COEFF_PHASES_LEN = firlen;
        static constexpr int BTP_CASC_IN = TP_CASC_IN;
        static constexpr int BTP_CASC_OUT = TP_CASC_OUT;
        static constexpr int BTP_FIR_RANGE_LEN = 1;
        static constexpr int BTP_SAT = TP_SAT;
    };
    template <int ssr_dim>
    using ssrKernelLookup = ssr_kernels<ssr_params<ssr_dim>, fir_sr_asym_tl>;
    using lastSSRKernel = ssrKernelLookup<(TP_SSR * TP_SSR) - 1>; // TP_SSR^2

    struct first_casc_params : public ssr_params<0> {
        static constexpr int Bdim = 0;
        static constexpr int BTP_FIR_LEN = CEIL(TP_FIR_LEN, TP_SSR) / TP_SSR;
        static constexpr int BTP_FIR_RANGE_LEN =
            fir_sr_asym_tl<ssr_params<0, BTP_FIR_LEN> >::template getKernelFirRangeLen<0>();
    };

    using first_casc_kernel_in_first_ssr = fir_sr_asym_tl<first_casc_params>;

    static_assert(!(get_input_streams_core_module() == 1 && (TP_API == 1) && (TP_DUAL_IP == 1)),
                  "Dual stream ports not supported on this device. Please set TP_DUAL_IP to 0.");
    static_assert(!(get_input_streams_core_module() == 1 && (TP_API == 1) && (TP_NUM_OUTPUTS == 2)),
                  "Dual stream ports not supported on this device. Please set TP_NUM_OUTPUTS to 1.");
    static_assert(TP_SSR >= 1, "ERROR: TP_SSR must be 1 or higher");

    static_assert(TP_FIR_LEN % TP_SSR == 0, "TP_FIR LEN must be divisible by TP_SSR"); //
    // static_assert(TP_USE_COEFF_RELOAD != 2 || (TP_FIR_LEN % TP_SSR == 0), "TP_FIR LEN must be divisible by TP_SSR, at
    // least for the header based coefficient reload."); //
    static_assert(TP_CASC_LEN <= 40, "ERROR: Unsupported Cascade length");

    static_assert(TP_INPUT_WINDOW_VSIZE % TP_SSR == 0,
                  "ERROR: Unsupported frame size. TP_INPUT_WINDOW_VSIZE must be divisible by TP_SSR");
    // Dual input streams offer no throughput gain if only single output stream would be used.
    // Therefore, dual input streams are only supported with 2 output streams.
    static_assert(TP_NUM_OUTPUTS > TP_DUAL_IP,
                  "ERROR: Dual input streams only supported when number of output streams is also 2. ");
    // Dual input ports offer no throughput gain if port api is windows.
    // Therefore, dual input ports are only supported with streams and not windows.
    static_assert(TP_API == USE_STREAM_API || TP_DUAL_IP == DUAL_IP_SINGLE,
                  "ERROR: Dual input ports only supported when port API is a stream. ");
    // Dual input ports offer no throughput gain if port api is windows.
    // Therefore, dual input ports are only supported with streams and not windows.
    static_assert(not(TP_API == USE_WINDOW_API && TP_SSR > 1 && TP_NUM_OUTPUTS == 2),
                  "ERROR: Dual output ports is not supported when port API is a window and SSR > 1. ");

    static constexpr unsigned int kMaxTapsPerKernel = 256;
    // Limit FIR length per kernel. Longer FIRs may exceed Program Memory and/or system memory combined with window
    // buffers may exceed Memory Module size
    static_assert((TP_FIR_LEN / TP_SSR) / TP_CASC_LEN <= kMaxTapsPerKernel,
                  "ERROR: Requested FIR length and Cascade length exceeds supported number of taps per kernel. Please "
                  "increase the cascade length to accommodate the FIR design.");

    // Limit FIR length for reloadable coeffs. Reloadable coeffs need a storage space that contributes to system memory
    // exceeding Memory Module size.
    static_assert(TP_USE_COEFF_RELOAD == 0 || (TP_FIR_LEN / TP_SSR) <= kMaxTapsPerKernel,
                  "ERROR: Exceeded maximum supported FIR length with reloadable coefficients. Please limit the FIR "
                  "length or disable coefficient reload.");
    // static_assert(TP_USE_COEFF_RELOAD != 1 || (TP_USE_COEFF_RELOAD == 1 && TP_SSR == 1),"ERROR: Coefficient reload is
    // not supported for SSR configurations.");
    static_assert(TP_USE_COEFF_RELOAD != 2 || TP_API == USE_STREAM_API,
                  "ERROR: Unsupported TP_USE_COEFF_RELOAD and TP_API combination. Header based coeff reload "
                  "(TP_USE_COEFF_RELOAD == 2) only supported with Streaming API (TP_API == 1)."); //

    static constexpr unsigned int kMemoryModuleSize = 32768;
    static constexpr unsigned int bufferSize =
        (((TP_FIR_LEN / TP_SSR) + TP_INPUT_WINDOW_VSIZE / TP_SSR) * sizeof(TT_DATA));
    // Requested Window buffer exceeds memory module size
    static_assert(TP_API != 0 || bufferSize < kMemoryModuleSize,
                  "ERROR: Input Window size (based on requested window size and FIR length margin) exceeds Memory "
                  "Module size of 32kB");

    template <unsigned int CL>
    struct tmp_ssr_params : public ssr_params<0> {
        static constexpr unsigned int BTP_FIR_LEN = CEIL(TP_FIR_LEN, TP_SSR) / TP_SSR;
        static constexpr unsigned int BTP_CASC_LEN = CL;
    };

    template <int pos, int CLEN, int T_FIR_LEN, typename T_D, typename T_C, unsigned int SSR>
    static constexpr unsigned int clRecurser() {
        if
            constexpr(
                fir_sr_asym_tl<tmp_ssr_params<CLEN> >::template fnCheckIfFits<pos, CLEN, T_FIR_LEN, T_D, T_C, SSR>() ==
                1) {
                if
                    constexpr(pos == 0) { return CLEN; }
                else {
                    return clRecurser<pos - 1, CLEN, T_FIR_LEN, T_D, T_C, SSR>();
                }
            }
        else {
            return clRecurser<CLEN, CLEN + 1, T_FIR_LEN, T_D, T_C, SSR>();
        }
    }

    template <unsigned int T_API, typename T_D, typename T_C>
    static constexpr unsigned int getMaxTapsPerKernel() {
        if
            constexpr(T_API == 0) { return 256; }
        else {
            constexpr unsigned int m_kSamplesInBuff = fnSamplesIn1024<T_D>();
            constexpr unsigned int m_kDataLoadVsize = fnDataLoadVsizeSrAsym<T_D, T_C, T_API>();
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
     * The array of kernels that will be created and mapped onto AIE tiles.
     * Number of kernels (``TP_CASC_LEN * TP_SSR``) will be connected with each other by cascade interface.
     **/
    kernel m_firKernels[TP_SSR * TP_SSR * TP_CASC_LEN];

    /**
     * The input data array to the function. This input array is either a window API of
     * samples of TT_DATA type or stream API (depending on TP_API).
     * Note: Margin is added internally to the graph, when connecting input port
     * with kernel port. Therefore, margin should not be added when connecting
     * graph to a higher level design unit.
     * Margin size (in Bytes) equals to TP_FIR_LEN rounded up to a nearest
     * multiple of 32 bytes.
     **/
    port_array<input, TP_SSR> in;

    /**
     * The output data array from the function. This output is either a window API of
     * samples of TT_DATA type or stream API (depending on TP_API).
     * Number of output samples is determined by interpolation & decimation factors (if present).
     **/
    port_array<output, TP_SSR> out;

    /**
     * The conditional input array data to the function.
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
    port_conditional_array<input, (TP_USE_COEFF_RELOAD == 1), TP_SSR> coeff;

    /**
     * The output data array from the function.
     * This output is (generated when TP_NUM_OUTPUTS == 2) either a window API of
     * samples of TT_DATA type or stream API (depending on TP_API).
     * Number of output samples is determined by interpolation & decimation factors (if present).
     **/
    port_conditional_array<output, (TP_NUM_OUTPUTS == 2), TP_SSR> out2;

    /**
     * Access function to get pointer to kernel (or first kernel in a chained and/or SSR configurations).
     * No arguments required.
     **/
    kernel* getKernels() { return m_firKernels; };

    /**
     * Access function to get pointer to an indexed kernel.
     * @param[in] ssrOutPathIndex      an index to the output data Path.
     * @param[in] ssrInPhaseIndex     an index to the input data Phase
     * @param[in] cascadePosition   an index to the kernel's position in the cascade.
     **/
    kernel* getKernels(int ssrOutPathIndex, int ssrInPhaseIndex, int cascadePosition) {
        return m_firKernels + ssrOutPathIndex * TP_SSR * TP_CASC_LEN + ssrInPhaseIndex * TP_CASC_LEN + cascadePosition;
    };

    /**
     * Access function to get pointer to net of the ``` in ``` port.
     * Nets only get assigned when streaming interface is being broadcast, i.e.
     * nets only get used when TP_API == 1 and TP_CASC_LEN > 1 or TP_SSR > 1
     * @param[in] ssrOutPathIndex      an index to the output data Path.
     * @param[in] ssrInPhaseIndex     an index to the input data Phase
     * @param[in] cascadePosition   an index to the kernel's position in the cascade.
     **/
    connect<stream, stream>* getInNet(int ssrOutPathIndex, int ssrInPhaseIndex, int cascadePosition) {
        return net[ssrOutPathIndex][ssrInPhaseIndex][cascadePosition];
    };

    /**
     * Access function to get pointer to net of the ``` in2 ``` port,
     * when port is being generated, i.e. when TP_DUAL_IP == 1.
     * Nets only get assigned when streaming interface is being broadcast, i.e.
     * nets only get used when TP_API == 1 and TP_CASC_LEN > 1 or TP_SSR > 1
     * @param[in] ssrOutPathIndex      an index to the output data Path.
     * @param[in] ssrInPhaseIndex     an index to the input data Phase
     * @param[in] cascadePosition   an index to the kernel's position in the cascade.
     **/
    connect<stream, stream>* getIn2Net(int ssrOutPathIndex, int ssrInPhaseIndex, int cascadePosition) {
        return net2[ssrOutPathIndex][ssrInPhaseIndex][cascadePosition];
    };

    /**
    * @brief Access function to get kernel's architecture (or first kernel's architecture in a chained configuration).
    **/
    unsigned int getKernelArchs() {
        // return the architecture for first kernel in the design (only one for single kernel designs).
        // First kernel will always be the slowest of the kernels and so it will reflect on the designs performance
        // best.
        return first_casc_kernel_in_first_ssr::parent_class::get_m_kArch();
    };

    /**
     * @brief This is the constructor function for the FIR graph with reloadable coefficients. \n
     * Constructor has no args. To be used with TP_USE_COEFF_RELOAD=1, taps needs to be passed through RTP
     **/
    fir_sr_asym_graph() {
        printParams<ssr_params<0> >();

        printf("== class fir_sr_asym_base_graph (reloadable): \n");
        lastSSRKernel::create_and_recurse(m_firKernels);
        lastSSRKernel::create_connections(m_firKernels, &in[0], in2, &out[0], out2, coeff, net, net2, casc_in);
    };

    /**
     * @brief This is the constructor function for the FIR graph with static coefficients.
     * @param[in] taps   a reference to the std::vector array of taps values of type TT_COEFF.
     **/
    fir_sr_asym_graph(const std::vector<TT_COEFF>& taps) {
        printf("== class fir_sr_asym_base_graph (static): \n");
        lastSSRKernel::create_and_recurse(m_firKernels, taps);
        lastSSRKernel::create_connections(m_firKernels, &in[0], in2, &out[0], out2, coeff, net, net2, casc_in);
    };

    /**
    * @brief Access function to get Graph's minimum cascade length for a given configuration.
    * @tparam T_FIR_LEN tap length of the fir filter
    * @tparam T_API interface type : 0 - window, 1 - stream
    * @tparam T_D data type
    * @tparam T_C coeff type
    * @tparam SSR parallelism factor set for super sample rate operation
    **/
    template <int T_FIR_LEN, int T_API, typename T_D, typename T_C, unsigned int SSR>
    static constexpr unsigned int getMinCascLen() {
        constexpr int kMaxTaps = getMaxTapsPerKernel<T_API, T_D, T_C>();
        constexpr int firLenPerSSR = CEIL(T_FIR_LEN, SSR) / SSR;
        constexpr int cLenMin = xf::dsp::aie::getMinCascLen<firLenPerSSR, kMaxTaps>();
        if
            constexpr(T_API == 0) { return cLenMin; }
        else {
            return clRecurser<cLenMin - 1, cLenMin, firLenPerSSR, T_D, T_C, SSR>();
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
    **/
    template <int T_FIR_LEN, typename T_D, typename T_C, int T_PORTS, unsigned int SSR>
    static constexpr unsigned int getOptCascLen() {
        constexpr int kMaxTaps = getMaxTapsPerKernel<1, T_D, T_C>();
        constexpr int kRawOptTaps = getOptTapsPerKernel<T_D, T_C, T_PORTS>();
        return xf::dsp::aie::getOptCascLen<kMaxTaps, kRawOptTaps, T_FIR_LEN, SSR>();
    };
};
}
}
}
}
} // namespace braces
#endif //_DSPLIB_FIR_SR_ASYM_GRAPH_HPP_
