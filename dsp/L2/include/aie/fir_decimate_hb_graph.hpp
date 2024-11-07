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
#ifndef _DSPLIB_FIR_DECIMATE_HB_GRAPH_HPP_
#define _DSPLIB_FIR_DECIMATE_HB_GRAPH_HPP_

/**
 * @file fir_decimate_hb_graph.hpp
 **/

// This file captures the definition of the 'L2' graph level class for FIR library element.

#include <adf.h>
#include <vector>
#include "fir_graph_utils.hpp"
#include "fir_decimate_hb.hpp"
#include "fir_decimate_hb_asym.hpp"
#include "fir_sr_asym.hpp"
#include "widget_api_cast.hpp"
#include "fir_common_traits.hpp"
#include "fir_sr_asym_graph.hpp"
#include <adf/arch/aie_arch_properties.hpp>

using namespace adf;
using namespace xf::dsp::aie::widget::api_cast;
namespace xf {
namespace dsp {
namespace aie {
namespace fir {
namespace decimate_hb {

/**
  * @cond NOCOMMENTS
  */
template <int dim, int FIR_LENGTH_ACT, typename ct_kernel_params = fir_params_defaults>
class ct_kernels {
   private:
    static_assert(dim >= 0, "ERROR: dim must be a positive integer");
    static constexpr unsigned int SSR = ct_kernel_params::BTP_SSR;
    static constexpr unsigned int modifyMarginOffset = -1 * ((dim + (FIR_LENGTH_ACT + 1) / 4) / SSR);

   public:
    struct ct_fir_params : public ct_kernel_params {
        static constexpr int BTP_MODIFY_MARGIN_OFFSET = modifyMarginOffset;
    };

    using kernelClass = typename fir::sr_asym::fir_sr_asym_tl<ct_fir_params>::parent_class;

    template <int dim_idx>
    using ct_kernels_class_lookup = ct_kernels<dim_idx, FIR_LENGTH_ACT, ct_kernel_params>;

    using type = ct_kernels_class_lookup<dim>;                                                            // thisClass
    using recurseClass = typename std::conditional_t<(dim > 0), ct_kernels_class_lookup<dim - 1>, empty>; // nextClass

    static void create_and_recurse(kernel firKernels[SSR],
                                   const std::vector<typename ct_kernel_params::BTT_COEFF>& taps) {
        printf("in the create ct kernels %d %d %d %d\n", dim, modifyMarginOffset, ct_kernel_params::BTP_FIR_LEN, SSR);
        firKernels[dim] = kernel::create_object<kernelClass>(taps);
        if
            constexpr(dim != 0) { recurseClass::create_and_recurse(firKernels, taps); }
    }
    static void create_and_recurse(kernel firKernels[SSR]) {
        printf("in the create ct kernels %d %d %d %d\n", dim, modifyMarginOffset, ct_kernel_params::BTP_FIR_LEN, SSR);
        firKernels[dim] = kernel::create_object<kernelClass>();
        if
            constexpr(dim != 0) { recurseClass::create_and_recurse(firKernels); }
    }
};
/**
  * @endcond
  */

/**
 * @brief fir_decimate_hb is a Halfband Decimation FIR filter
 *
 * @ingroup fir_graphs
 *
 * These are the templates to configure the halfband decimator FIR class.
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
 *         in the filter. \n
 *         TP_FIR_LEN must satisfy (TP_FIR_LEN +1)/4 = N where N is a positive integer.
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
 *         When TP_API is set to 1 and TP_SSR is set to 1, incoming samples are buffered in a similar manner.  \n
 *         When TP_API is set to 1 and TP_SSR > 1, samples are processed directly from the stream inputs and no
 *buffering takes place. \n
 *         In such case, maximum number of samples processed by the graph is limited to 32-bit value (4.294B samples per
 *iteration).  \n
 *         \n
 *         Note: For SSR configurations (TP_SSR>1), the input data must be split over multiple ports,
 *         where each successive sample is sent to a different input port in a round-robin fashion. \n
 *         As a result, each SSR input path will process a fraction of the frame defined by the TP_INPUT_WINDOW_VSIZE.
 *\n
 *         The number of values in the output window will be TP_INPUT_WINDOW_VSIZE
 *         divided by 2 by virtue the halfband decimation factor. \n
 *         \n
 *         Note: Margin size should not be included in TP_INPUT_WINDOW_VSIZE.
 * @tparam TP_CASC_LEN describes the number of AIE processors to split the operation
 *         over. \n This allows resource to be traded for higher performance.
 *         TP_CASC_LEN must be in the range 1 (default) to 40.
 * @tparam TP_DUAL_IP allows 2 input ports to be connected to FIR, increasing available throughput. \n
 *         Depending on TP_API, additional input ports functionality differs.
 *         If TP_API is set to use windows, then \n
 *         TP_DUAL_IP is an implementation trade-off between performance and data
 *         bank resource. \n
 *         When TP_DUAL_IP is set to 0, the FIR performance may be limited by load contention. \n
 *         When TP_DUAL_IP is set to 1, two ram banks are used for input. \n
 *         If TP_API is set to use streams, then: \n
 *         When TP_DUAL_IP is set to 0, single stream will be connected as FIRs input. \n
 *         When TP_DUAL_IP is set to 1, two stream inputs will be connected. \n
 *         In such case data should be organized in 128-bit interleaved pattern, e.g.: \n
 *         - samples 0-3 to be sent over stream0 for cint16 data type, \n
 *         - samples 4-7 to be sent over stream1 for cint16 data type. \n
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
 * @tparam TP_API specifies if the input/output interface should be window-based or stream-based.  \n
 *         The values supported are 0 (window API) or 1 (stream API).
 * @tparam TP_SSR specifies the number of parallel input/output paths where samples are interleaved between paths,
 *         giving an overall higher throughput.   \n
 *         A TP_SSR of 1 means just one output leg and 1 input phase, and is the backwards compatible option. \n
 *         The number of AIEs used is given by ``TP_SSR^2 * TP_CASC_LEN``. \n
 * @tparam TP_PARA_DECI_POLY specifies the number of distinct input data phases into which the input stream will be
 *split.
 *         In each stream computations are performed in parallel and the outputs are combined into a single output
 *stream. \n
 *         Currently, only TP_PARA_DECI_POLY=2 is supported for the halfband interpolators with SSR>1. SSR = 1 supports
 *TP_PARA_DECI_POLY=1 or 2.
 *
 *         TP_PARA_DECI_POLY = 2 result in two input polyphases. Each polyphase processes half of the input data stream.
 *Processed output data from both the polyphases
 *         are combined together to produce the overall output data stream.
 *         Hence, effectively the input to the filter is de-interleaved into TP_SSR * TP_PARA_DECI_POLY distinct input
 *data phases
 *         and produces TP_SSR distinct output data phases. \n
 *
 *         Overall, the first polyphase is implemented using a single rate asymmetric filter that is configured to
 *produce and consume data in parallel in
 *         TP_SSR phases, each phase can operate at maximum throughput depending on the configuration.
 *         The first polyphase uses TP_SSR^2 * TP_CASC_LEN kernels. \n
 *         The second polyphase simplifies into a single kernel that does a single tap because halfband decimators only
 *have one non-zero
 *         coefficient in the second coefficient phase. The second polyphase uses SSR kernels operating at maximum
 *throughput.
 *         Currently, only TP_PARA_INTERP_POLY=2 is supported for the halfband interpolators with SSR>1. SSR = 1
 *supports TP_PARA_INTERP_POLY=1 or 2.
 *         The overall theoretical output data rate is TP_SSR * TP_NUM_OUTPUTS * 1 GSa/s.
 *         The overall theoretical input data rate is TP_SSR * TP_PARA_DECI_POLY * (TP_DUAL_IP + 1) * 1GSa/s
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
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_CASC_LEN = 1,
          unsigned int TP_DUAL_IP = 0,
          unsigned int TP_USE_COEFF_RELOAD = 0,
          unsigned int TP_NUM_OUTPUTS = 1,
          unsigned int TP_API = 0,
          unsigned int TP_SSR = 1,
          unsigned int TP_PARA_DECI_POLY = 1,
          unsigned int TP_SAT = 1>
class fir_decimate_hb_graph : public graph {
   private:
    static constexpr unsigned int kMaxTapsPerKernel = 1024;

    static_assert(TP_CASC_LEN <= 40, "ERROR: Unsupported Cascade length");

    static_assert(TP_INPUT_WINDOW_VSIZE % TP_SSR == 0,
                  "ERROR: Unsupported frame size. TP_INPUT_WINDOW_VSIZE must be divisible by TP_SSR");

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

    static constexpr unsigned int kMemoryModuleSize = 32768;
    static constexpr unsigned int inBufferSize = ((TP_FIR_LEN + TP_INPUT_WINDOW_VSIZE / TP_SSR) * sizeof(TT_DATA));
    // Requested Input Window buffer exceeds memory module size
    static_assert(TP_API != 0 || inBufferSize < kMemoryModuleSize,
                  "ERROR: Input Window size (based on requested window size and FIR length margin) exceeds Memory "
                  "Module size of 32kB");
    static_assert(
        TP_SSR == 1 || TP_PARA_DECI_POLY == 2,
        "Please set TP_PARA_DECI_POLY=2 for SSR>1; SSR>1 and TP_PARA_DECI_POLY=1 are currently not supported");
    static_assert(TP_PARA_DECI_POLY == 1 || TP_PARA_DECI_POLY == 2, "TP_PARA_DECI_POLY can be set to 1 or 2 only.");
    static_assert(!(((TP_DUAL_IP == 1 && TP_NUM_OUTPUTS == 1) || (TP_DUAL_IP == 0 && TP_NUM_OUTPUTS == 2)) &&
                    TP_PARA_DECI_POLY == 2),
                  "ERROR: if SSR is enabled or TP_PARA_DECI_POLY=2 , DUAL inputs is only supported with DUAL_OUTPUTS");
    static constexpr bool TP_CASC_IN =
        TP_PARA_DECI_POLY == 2 ? CASC_IN_TRUE : CASC_IN_FALSE; // should be unsigned int if exposed on graph interface
    static constexpr unsigned int CASC_IN_PORT_POS = (TP_DUAL_IP == DUAL_IP_DUAL) ? 2 : 1;
    static_assert(!(get_input_streams_core_module() == 1 && (TP_API == 1) && (TP_DUAL_IP == 1)),
                  "Dual stream ports not supported on this device. Please set TP_DUAL_IP to 0.");
    static_assert(!(get_input_streams_core_module() == 1 && (TP_API == 1) && (TP_NUM_OUTPUTS == 2)),
                  "Dual stream ports not supported on this device. Please set TP_NUM_OUTPUTS to 1.");

    void create_connections() {
        // make input connections

        // TP_INPUT_WINDOW_VSIZE / TP_SSR. However, only SSR=1 is supported with windowed architectures

        if
            constexpr(TP_API == USE_WINDOW_API) {
                connect<>(in[0], m_firKernels[0].in[0]);
                dimensions(m_firKernels[0].in[0]) = {TP_INPUT_WINDOW_VSIZE};
                for (int i = 1; i < TP_CASC_LEN; i++) {
                    single_buffer(m_firKernels[i].in[0]);
                    connect<>(m_firKernels[i - 1].out[1], m_firKernels[i].in[0]);
                    dimensions(m_firKernels[i - 1].out[1]) = {TP_INPUT_WINDOW_VSIZE +
                                                              fnFirMargin<TP_FIR_LEN, TT_DATA>()};
                    dimensions(m_firKernels[i].in[0]) = {TP_INPUT_WINDOW_VSIZE + fnFirMargin<TP_FIR_LEN, TT_DATA>()};
                }
                if
                    constexpr(TP_DUAL_IP == DUAL_IP_DUAL) {
                        connect<>(in2[0], m_firKernels[0].in[1]);
                        dimensions(m_firKernels[0].in[1]) = {TP_INPUT_WINDOW_VSIZE};
                    }
            }
        else {
            if
                constexpr(TP_DUAL_IP == DUAL_IP_SINGLE) {
                    connect<>(in[0], m_firKernels[0].in[0]);
                    dimensions(m_firKernels[0].in[0]) = {TP_INPUT_WINDOW_VSIZE};
                    for (int i = 1; i < TP_CASC_LEN; i++) {
                        connect<>(in[0], m_firKernels[i].in[0]);
                        dimensions(m_firKernels[i].in[0]) = {TP_INPUT_WINDOW_VSIZE};
                    }
                }
            else if
                constexpr(TP_DUAL_IP == DUAL_IP_DUAL) {
                    kernel m_inWidgetKernel;

                    m_inWidgetKernel = kernel::create_object<
                        widget_api_cast<TT_DATA, USE_STREAM_API, USE_WINDOW_API, 2, TP_INPUT_WINDOW_VSIZE, 1, 0> >();
                    connect<stream>(in[0], m_inWidgetKernel.in[0]);
                    connect<stream>(in2[0], m_inWidgetKernel.in[1]);
                    connect<>(m_inWidgetKernel.out[0], m_firKernels[0].in[0]);
                    dimensions(m_inWidgetKernel.out[0]) = {TP_INPUT_WINDOW_VSIZE};
                    dimensions(m_firKernels[0].in[0]) = {TP_INPUT_WINDOW_VSIZE};
                    source(m_inWidgetKernel) = "widget_api_cast.cpp";
                    headers(m_inWidgetKernel) = {"widget_api_cast.hpp"};
                    runtime<ratio>(m_inWidgetKernel) = 0.8;

                    for (int i = 1; i < TP_CASC_LEN; i++) {
                        single_buffer(m_firKernels[i].in[0]);
                        connect<>(m_firKernels[i - 1].out[1], m_firKernels[i].in[0]);
                        dimensions(m_firKernels[i - 1].out[1]) = {TP_INPUT_WINDOW_VSIZE +
                                                                  fnFirMargin<TP_FIR_LEN, TT_DATA>()};
                        dimensions(m_firKernels[i].in[0]) = {TP_INPUT_WINDOW_VSIZE +
                                                             fnFirMargin<TP_FIR_LEN, TT_DATA>()};
                    }
                }
        }

        // conditional RTP connection
        int rtpPortPos = (TP_DUAL_IP == DUAL_IP_DUAL & TP_API == USE_WINDOW_API) ? 2 : 1;
        if
            constexpr(TP_USE_COEFF_RELOAD == 1) { connect<parameter>(coeff[0], async(m_firKernels[0].in[rtpPortPos])); }
        if (TP_API == 0) {
            // make cascade connections
            for (int i = 1; i < TP_CASC_LEN; i++) {
                connect<cascade>(m_firKernels[i - 1].out[0], m_firKernels[i].in[1]);
            }
        } else {
            for (int i = 1; i < TP_CASC_LEN; i++) {
                connect<cascade>(m_firKernels[i - 1].out[0], m_firKernels[i].in[1]);
            }
        }
        // make output connections
        if (TP_API == 0) {
            connect<>(m_firKernels[TP_CASC_LEN - 1].out[0], out[0]);
            dimensions(m_firKernels[TP_CASC_LEN - 1].out[0]) = {TP_INPUT_WINDOW_VSIZE / kDecimateFactor};
        } else {
            connect<stream>(m_firKernels[TP_CASC_LEN - 1].out[0], out[0]);
        }

        if
            constexpr(TP_NUM_OUTPUTS == 2) {
                if (TP_API == 0) {
                    connect<>(m_firKernels[TP_CASC_LEN - 1].out[1], out2[0]);
                    dimensions(m_firKernels[TP_CASC_LEN - 1].out[1]) = {TP_INPUT_WINDOW_VSIZE / kDecimateFactor};
                } else {
                    connect<stream>(m_firKernels[TP_CASC_LEN - 1].out[1], out2[0]);
                }
            }

        for (int i = 0; i < TP_CASC_LEN; i++) {
            // Specify mapping constraints
            runtime<ratio>(m_firKernels[i]) = 0.8;
// Source files
#if __HAS_SYM_PREADD__ == 1
            source(m_firKernels[i]) = "fir_decimate_hb.cpp";
#else
            source(m_firKernels[i]) = "fir_decimate_hb_asym.cpp";
#endif
        }
    }

    void connect_with_ssr() {
        // create kernels
        if
            constexpr(TP_PARA_DECI_POLY == 1) {
// Only SSR==1
#if __HAS_SYM_PREADD__ == 1
                create_connections();
#else
                aiemllastSSRKernel::create_connections(m_firKernels, &in[0], in2, &out[0], out2, coeff, net, net2,
                                                       casc_in, "fir_decimate_hb_asym.cpp");
#endif
            }
        else {
            printf("paradecipoly=2\n");
            // SSR>=1
            // printParams<sr_asym_graph_params<0, TP_PARA_DECI_POLY>>();

            // rearrange taps
            for (int i = 0; i < TP_SSR; i++) {
                connect<stream>(in[2 * i], in_dummy[i]);
                if
                    constexpr(TP_DUAL_IP == 1) { connect<stream>(in2[2 * i], in2_dummy[i]); }
            }
            // Only first kernels in chain are configured with RTP port.
            // Subsequent kernels in cascade chain expect it's coneffs to be passed through the cascade IF.
            // Since the first in chain is the CT one, a dummy one is passed to Asym kernels to mark the distinction.
            typename lastSSRKernelSrAsym::coeff_type coeff_dummy;
            lastSSRKernelSrAsym::create_connections(m_firKernels, &in_dummy[0], in2_dummy, &out[0], out2, coeff_dummy,
                                                    net, net2, casc_in);

            for (int i = 0; i < TP_SSR; i++) {
                // Specify mapping constraints
                runtime<ratio>(m_ct_firKernels[i]) = 0.8;
                // Source files
                source(m_ct_firKernels[i]) = "fir_sr_asym.cpp";

                unsigned int CT_SSROutputPath = (i + ((TP_FIR_LEN + 1) / 4)) % TP_SSR;
                printf("For CT phase, connecting input path %d to output path %d\n", i, CT_SSROutputPath);

                if
                    constexpr(TP_API == 1) {
                        connect<stream>(
                            in[2 * i + 1],
                            m_ct_firKernels[i]
                                .in[0]); // all the odd input data phases need to be connected to the ct kernels
                        if
                            constexpr(TP_DUAL_IP == 1) { connect<stream>(in2[2 * i + 1], m_ct_firKernels[i].in[1]); }
                    }
                else {
                    connect<>(in[2 * i + 1],
                              m_ct_firKernels[i]
                                  .in[0]); // all the odd input data phases need to be connected to the ct kernels
                    dimensions(m_ct_firKernels[i].in[0]) = {TP_INPUT_WINDOW_VSIZE / (TP_SSR * TP_PARA_DECI_POLY)};
                }
                connect<cascade>(m_ct_firKernels[i].out[0], casc_in[CT_SSROutputPath]);
                constexpr unsigned int RTP_PORT_POS = TP_DUAL_IP + 1;
                // TP_PARA_DECI_POLY only takes values in range 1 - 2.
                if
                    constexpr(TP_USE_COEFF_RELOAD == 1) {
                        connect<parameter>(coeff[i], async(m_ct_firKernels[i].in[RTP_PORT_POS]));
                    }
            }
        }
    }

    // call sr aym graph constructor
    template <int dim, unsigned int TP_POLY_SSR>
    struct hb_dec_graph_params : public fir_params_defaults {
        static constexpr int Bdim = dim;
        using BTT_DATA = TT_DATA;
        using BTT_COEFF = TT_COEFF;
        static constexpr int BTP_FIR_LEN = CEIL((TP_POLY_SSR != 1 ? (TP_FIR_LEN + 1) / 2 : TP_FIR_LEN), TP_SSR);
        static constexpr int BTP_SHIFT = TP_SHIFT;
        static constexpr int BTP_RND = TP_RND;
        static constexpr int BTP_INPUT_WINDOW_VSIZE = TP_INPUT_WINDOW_VSIZE / (TP_SSR * TP_POLY_SSR);
        static constexpr int BTP_CASC_LEN = TP_CASC_LEN;
        static constexpr int BTP_USE_COEFF_RELOAD = TP_USE_COEFF_RELOAD;
        static constexpr int BTP_NUM_OUTPUTS = TP_NUM_OUTPUTS;
        static constexpr int BTP_DUAL_IP = TP_DUAL_IP;
        static constexpr int BTP_API = TP_API;
        static constexpr int BTP_SSR = TP_SSR;
        static constexpr int BTP_PARA_DECI_POLY = TP_PARA_DECI_POLY;
        static constexpr int BTP_CASC_IN = TP_CASC_IN;
        static constexpr int BTP_POLY_SSR = TP_POLY_SSR;
        static constexpr int BTP_SAT = TP_SAT;
    };

    // call sr aym graph constructor
    template <int dim, unsigned int TP_POLY_SSR>
    struct sr_asym_graph_params : public fir_params_defaults {
        static constexpr int Bdim = dim;
        using BTT_DATA = TT_DATA;
        using BTT_COEFF = TT_COEFF;
        static constexpr int BTP_FIR_LEN = CEIL((TP_POLY_SSR != 1 ? (TP_FIR_LEN + 1) / 2 : TP_FIR_LEN), TP_SSR);
        static constexpr int BTP_SHIFT = TP_SHIFT;
        static constexpr int BTP_RND = TP_RND;
        static constexpr int BTP_INPUT_WINDOW_VSIZE = TP_INPUT_WINDOW_VSIZE / (TP_SSR * TP_POLY_SSR);
        static constexpr int BTP_CASC_LEN = TP_CASC_LEN;
        static constexpr int BTP_USE_COEFF_RELOAD = TP_USE_COEFF_RELOAD;
        static constexpr int BTP_NUM_OUTPUTS = TP_NUM_OUTPUTS;
        static constexpr int BTP_DUAL_IP = TP_DUAL_IP;
        static constexpr int BTP_API = TP_API;
        static constexpr int BTP_SSR = TP_SSR;
        static constexpr int BTP_COEFF_PHASE_OFFSET =
            1; // disregard the Center tap Coeff (BTP_COEFF_PHASES_LEN = BTP_FIR_LEN + 1)
        static constexpr int BTP_COEFF_PHASES = TP_SSR;
        static constexpr int BTP_COEFF_PHASES_LEN = CEIL((TP_FIR_LEN + 1) / 2, TP_SSR) + 1;
        static constexpr int BTP_CASC_IN = TP_CASC_IN;
        static constexpr int BTP_POLY_SSR = TP_POLY_SSR;
        static constexpr int BTP_SAT = TP_SAT;
    };

    // create single tap kernels
    template <unsigned int dim>
    struct ct_fir_params : public fir_params_defaults {
        using BTT_DATA = TT_DATA;
        using BTT_COEFF = TT_COEFF;
        static constexpr int BTP_FIR_LEN = 1;
        static constexpr int BTP_SHIFT = TP_SHIFT;
        static constexpr int BTP_RND = TP_RND;
        static constexpr int BTP_INPUT_WINDOW_VSIZE = TP_INPUT_WINDOW_VSIZE / (TP_SSR * TP_PARA_DECI_POLY);
        static constexpr int BTP_CASC_IN = 0;
        static constexpr int BTP_CASC_OUT = 1;
        static constexpr int BTP_FIR_RANGE_LEN = 1;
        static constexpr int BTP_KERNEL_POSITION = 0;
        static constexpr int BTP_CASC_LEN = 1;
        static constexpr int BTP_USE_COEFF_RELOAD = TP_USE_COEFF_RELOAD;
        static constexpr int BTP_NUM_OUTPUTS = TP_NUM_OUTPUTS;
        static constexpr int BTP_DUAL_IP = TP_DUAL_IP;
        static constexpr int BTP_API = TP_API;
        static constexpr int BTP_SSR = TP_SSR;
        static constexpr int BTP_COEFF_PHASE = 0;
        static constexpr int BTP_COEFF_PHASE_OFFSET = 0;
        static constexpr int BTP_COEFF_PHASES = 1;
        static constexpr int BTP_PARA_DECI_POLY = TP_PARA_DECI_POLY;
        static constexpr int BTP_COEFF_PHASES_LEN = CEIL((TP_FIR_LEN + 1) / 2, TP_SSR) + 1;
        static constexpr int BTP_MODIFY_MARGIN_OFFSET =
            -1 * ((TP_FIR_LEN / 4 - 1 + dim) / (TP_SSR * TP_PARA_DECI_POLY));
        static constexpr int BTP_SAT = TP_SAT;
    };
    template <int ssr_dim>
    using ssrKernelLookupSrAsym =
        ssr_kernels<sr_asym_graph_params<ssr_dim, TP_PARA_DECI_POLY>, sr_asym::fir_sr_asym_tl>;
    using lastSSRKernelSrAsym = ssrKernelLookupSrAsym<(TP_SSR * TP_SSR) - 1>;

    template <int ssr_dim>
    using ssrKernelLookup = ssr_kernels<hb_dec_graph_params<ssr_dim, TP_PARA_DECI_POLY>, fir_decimate_hb_tl>;
    using lastSSRKernel = ssrKernelLookup<(TP_SSR * TP_SSR) - 1>;

    using lastct_kernels = ct_kernels<TP_SSR - 1, TP_FIR_LEN, ct_fir_params<TP_SSR - 1> >;

    template <int dim>
    struct aieml_ssr_params : public fir_params_defaults {
        static constexpr int Bdim = dim;
        using BTT_DATA = TT_DATA;
        using BTT_COEFF = TT_COEFF;
        static constexpr int BTP_FIR_LEN = TP_FIR_LEN;
        static constexpr int BTP_SHIFT = TP_SHIFT;
        static constexpr int BTP_RND = TP_RND;
        static constexpr int BTP_INPUT_WINDOW_VSIZE = TP_INPUT_WINDOW_VSIZE / TP_SSR;
        static constexpr int BTP_DECIMATE_FACTOR = kDecimateFactor;
        static constexpr int BTP_CASC_LEN = TP_CASC_LEN;
        static constexpr int BTP_USE_COEFF_RELOAD = TP_USE_COEFF_RELOAD;
        static constexpr int BTP_NUM_OUTPUTS = TP_NUM_OUTPUTS;
        static constexpr int BTP_SSR = TP_SSR;
        static constexpr int BTP_DUAL_IP = TP_DUAL_IP;
        static constexpr int BTP_API = TP_API;
        static constexpr int BTP_CASC_IN = 0;
        static constexpr int BTP_CASC_OUT = 0;
        static constexpr int BTP_COEFF_PHASES = TP_SSR;
        static constexpr int BTP_COEFF_PHASES_LEN = BTP_FIR_LEN;
        static constexpr int BTP_POLY_SSR = TP_PARA_DECI_POLY;
        static constexpr int BTP_SAT = TP_SAT;
    };

    template <int ssr_dim>
    using aiemlssrKernelLookup = ssr_kernels<aieml_ssr_params<ssr_dim>, decimate_hb_asym::fir_decimate_hb_asym_tl>;
    using aiemllastSSRKernel = aiemlssrKernelLookup<(TP_SSR * TP_SSR) - 1>;
    // 3d array for storing net information.
    // address[inPhase][outPath][cascPos]
    std::array<std::array<std::array<connect<stream, stream>*, TP_CASC_LEN>, TP_SSR>, TP_SSR> net;
    std::array<std::array<std::array<connect<stream, stream>*, TP_CASC_LEN>, TP_SSR>, TP_SSR> net2;

    port_array<input, TP_SSR> in_dummy;
    port_conditional_array<input, (TP_DUAL_IP == 1), TP_SSR> in2_dummy;

    template <typename T_D>
    static constexpr unsigned int getMaxTapsPerKernel() {
        if
            constexpr(std::is_same<TT_DATA, int32>::value) { return 902; }
        else if
            constexpr(std::is_same<TT_DATA, float>::value) { return 500; }
        else {
            return 1024;
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
    port_conditional_array<input, (TP_CASC_IN == CASC_IN_TRUE), TP_SSR> casc_in;

   public:
    /**
     * The array of kernels that will be created and mapped onto AIE tiles.
     * Number of kernels (``TP_CASC_LEN * TP_SSR``) will be connected with each other by cascade interface.
     **/
    kernel m_firKernels[TP_SSR * TP_SSR * TP_CASC_LEN];

    /**
     * The array of kernels that will be created and mapped onto AIE tiles,
     * to process Center tap on a parallel polyphase (TP_PARA_DECI_POLY == 2).
     * Number of kernels (``TP_SSR``) will be connected with each other by cascade interface.
     **/
    kernel m_ct_firKernels[TP_SSR];

    /**
     * The input data array to the function. This input array is either a window API of
     * samples of TT_DATA type or stream API (depending on TP_API).
     * Note: Margin is added internally to the graph, when connecting input port
     * with kernel port. Therefore, margin should not be added when connecting
     * graph to a higher level design unit.
     * Margin size (in Bytes) equals to TP_FIR_LEN rounded up to a nearest
     * multiple of 32 bytes.
     **/
    port_array<input, TP_SSR * TP_PARA_DECI_POLY> in;

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
    port_conditional_array<input, (TP_DUAL_IP == 1), TP_SSR * TP_PARA_DECI_POLY> in2;

    /**
     * The conditional array of input async ports used to pass run-time programmable (RTP) coefficients.
     * This port_conditional_array is (generated when TP_USE_COEFF_RELOAD == 1) an array of input ports, which size is
     *defined by TP_SSR.
     * Each port in the array holds a duplicate of the coefficient array, required to connect to each SSR input path.
     * Size of the coefficient array is dependent on the TP_SSR.
     * - When TP_SSR = 1, the taps array must be supplied in a compressed form for
     *                   this halfband application, i.e.  \n
     *                   taps[] = {c0, c2, c4, ..., cN, cCT} where  \n
     *                   N = (TP_FIR_LEN+1)/4 and
     *                   cCT is the center tap. \n
     *                   For example, a 7-tap halfband decimator might use coeffs
     *                   (1, 0, 2, 5, 2, 0, 1).  \n This would be input as
     *                   coeff[]= {1,2,5} since the context of halfband decimation
     *                   allows the remaining coefficients to be inferred.
     * - When TP_SSR > 1, the taps array must be partially uncompressed and symmetry must be removed.
     *                   For example, a 7-tap halfband decimator might use coeffs
     *                   (1, 0, 2, 5, 2, 0, 1).  \n This would be input as
     *                   coeff[]= {1,2,5,2,1}.
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
     * Access function to get pointer to kernel (or first kernel in a chained configuration).
     **/
    kernel* getKernels() { return m_firKernels; };

    /**
    * @brief Access function to get kernel's architecture (or first kernel's architecture in a chained configuration).
    **/
    unsigned int getKernelArchs() {
        constexpr unsigned int firRange = (TP_CASC_LEN == 1) ? TP_FIR_LEN : fnFirRangeSym<TP_FIR_LEN, TP_CASC_LEN, 0>();
        // return the architecture for first kernel in the design (only one for single kernel designs).
        // First kernel will always be the slowest of the kernels and so it will reflect on the designs performance
        // best.
        return fir_decimate_hb<TT_DATA, TT_COEFF, TP_FIR_LEN, TP_SHIFT, TP_RND, (TP_INPUT_WINDOW_VSIZE / TP_SSR), false,
                               true, firRange, 0, TP_CASC_LEN, TP_DUAL_IP, TP_USE_COEFF_RELOAD, TP_NUM_OUTPUTS, TP_API,
                               TP_SAT>::get_m_kArch();
    };

    /**
     * @brief This is the constructor function for the FIR graph with static coefficients.
     * @param[in] taps   a reference to the std::vector array of taps values of type TT_COEFF. \n
     *                   The taps array must be supplied in a compressed form for
     *                   this halfband application, i.e.  \n
     *                   taps[] = {c0, c2, c4, ..., cN, cCT} where  \n
     *                   N = (TP_FIR_LEN+1)/4 and
     *                   cCT is the center tap. \n
     *                   For example, a 7-tap halfband decimator might use coeffs
     *                   (1, 0, 2, 5, 2, 0, 1).  \n This would be input as
     *                   taps[]= {1,2,5} since the context of halfband decimation
     *                   allows the remaining coefficients to be inferred.
     **/
    fir_decimate_hb_graph(const std::vector<TT_COEFF>& taps) {
        // create kernels
        if
            constexpr(TP_PARA_DECI_POLY == 1) {
// Only SSR==1
#if __HAS_SYM_PREADD__ == 1
                lastSSRKernel::create_and_recurse(m_firKernels, taps);
#else
                std::vector<TT_COEFF> asymTaps =
                    aiemllastSSRKernel::convert_sym_taps_to_asym(((TP_FIR_LEN + 1) / 2), taps);
                asymTaps.push_back(taps.at((TP_FIR_LEN + 1) / 4));
                aiemllastSSRKernel::create_and_recurse(m_firKernels, asymTaps);
#endif
            }
        else {
            // SSR>=1
            // rearrange taps
            std::vector<TT_COEFF> srTaps = lastSSRKernelSrAsym::convert_sym_taps_to_asym(((TP_FIR_LEN + 1) / 2), taps);
            lastSSRKernelSrAsym::create_and_recurse(m_firKernels, srTaps);
            std::vector<TT_COEFF> ct_vector;
            ct_vector.push_back(taps.at((TP_FIR_LEN + 1) / 4));
            lastct_kernels::create_and_recurse(m_ct_firKernels, ct_vector);
        }
        connect_with_ssr();
    }

    /**
         * @brief This is the constructor function for the FIR graph with reloadable coefficients.
     **/
    fir_decimate_hb_graph() {
        // create kernels
        if
            constexpr(TP_PARA_DECI_POLY == 1) {
// Only SSR==1
#if __HAS_SYM_PREADD__ == 1
                lastSSRKernel::create_and_recurse(m_firKernels);
#else
                aiemllastSSRKernel::create_and_recurse(m_firKernels);
#endif
            }
        else {
            // SSR>=1
            // rearrange taps
            printf("coeff phases length = %d\n", CEIL((TP_FIR_LEN + 1) / 2, TP_SSR) + 1);
            lastSSRKernelSrAsym::create_and_recurse(m_firKernels);
            lastct_kernels::create_and_recurse(m_ct_firKernels);
        }
        connect_with_ssr();
    }

    template <unsigned int CL>
    struct tmp_ssr_params : public aieml_ssr_params<0> {
        static constexpr unsigned int BTP_FIR_LEN = CEIL((TP_FIR_LEN + 1) / 2, TP_SSR) / TP_SSR;
        static constexpr unsigned int BTP_CASC_LEN = CL;
    };

    template <int pos, int CLEN, int T_FIR_LEN, typename T_D, typename T_C>
    static constexpr unsigned int clRecurser() {
        if
            constexpr(decimate_hb_asym::fir_decimate_hb_asym_tl<tmp_ssr_params<CLEN> >::template fnCheckIfFits<
                          pos, CLEN, T_FIR_LEN, T_D, T_C>() == 1) {
                if
                    constexpr(pos == 0) { return CLEN; }
                else {
                    return clRecurser<pos - 1, CLEN, T_FIR_LEN, T_D, T_C>();
                }
            }
        else {
            return clRecurser<CLEN, CLEN + 1, T_FIR_LEN, T_D, T_C>();
        }
    }

    /**
    * @brief Access function to get Graphs minimum cascade length for a given configuration.
    * @tparam T_FIR_LEN tap length of the fir filter
    * @tparam T_D data type
    * @tparam T_C coeff type
    * @tparam SSR parallelism factor set for super sample rate operation
    * @tparam T_API interface type : 0 - window, 1 - stream
    **/
    template <int T_FIR_LEN, typename T_D, typename T_C, int SSR, int T_API>
    static constexpr unsigned int getMinCascLen() {
        if
            constexpr(SSR == 1) {
                constexpr int kMaxTaps = getMaxTapsPerKernel<T_D>();
                constexpr int asymFirLen = (T_FIR_LEN + 1) / 2;
                constexpr int firLenPerSSR = CEIL(asymFirLen, SSR) / SSR;
                constexpr int cLenMin = xf::dsp::aie::getMinCascLen<firLenPerSSR, kMaxTaps>();
#if __HAS_SYM_PREADD__ == 1
                return cLenMin;
#else
                if
                    constexpr(T_API == 0) { return cLenMin; }
                else {
                    return clRecurser<cLenMin - 1, cLenMin, firLenPerSSR, T_D, T_C>();
                }
#endif
            }
        else {
            using asym_graph =
                sr_asym::fir_sr_asym_graph<cint16, int16, 8, 0, 0, 128>; // making an arbitrary template list to call
                                                                         // sr_asym, can be avoided if we have default
                                                                         // template parameters for all arguments
            return asym_graph::getMinCascLen<T_FIR_LEN, T_API, T_D, T_C, SSR>();
        }
    };

    /**
    * @brief Access function to get graph's cascade length to obtain maximum performance for streaming configurations.
    * @tparam T_FIR_LEN tap length of the fir filter
    * @tparam T_D data type
    * @tparam T_C coeff type
    * @tparam T_PORTS single/dual input and output ports. 1 : single, 2 : dual
    *
    **/
    template <int T_FIR_LEN, typename T_D, typename T_C, int T_PORTS>
    static constexpr unsigned int getOptCascLen() {
        constexpr int kMaxTaps = getMaxTapsPerKernel<T_D>();
        constexpr int kRawOptTaps = getOptTapsPerKernel<T_D, T_C, T_PORTS>();
        return xf::dsp::aie::getOptCascLen<kMaxTaps, kRawOptTaps, ((T_FIR_LEN + 1) / 2)>();
    };
};
}
}
}
}
}

#endif // _DSPLIB_FIR_DECIMATE_HB_GRAPH_HPP_
