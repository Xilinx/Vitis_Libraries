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
#ifndef _DSPLIB_FIR_INTERPOLATE_HB_GRAPH_HPP_
#define _DSPLIB_FIR_INTERPOLATE_HB_GRAPH_HPP_

/**
 * @file fir_interpolate_hb_graph.hpp
 * @brief The file captures the definition of the 'L2' graph level class for FIR library element.
 **/

#include <adf.h>
#include <vector>
#include "graph_utils.hpp"
#include "fir_interpolate_hb.hpp"
#include "widget_api_cast.hpp"
#include "fir_utils.hpp"

using namespace adf;
using namespace xf::dsp::aie::widget::api_cast;
namespace xf {
namespace dsp {
namespace aie {
namespace fir {
namespace interpolate_hb {

//---------------------------------------------------------------------------------------------------
// create_casc_kernel_recur
// Where the FIR function is split over multiple processors to increase throughput, recursion
// is used to generate the multiple kernels, rather than a for loop due to constraints of
// c++ template handling.
// For each such kernel, only a splice of the full array of coefficients is processed.
//---------------------------------------------------------------------------------------------------
/**
  * @cond NOCOMMENTS
  */
// Recursive kernel creation, static coefficients
template <int dim,
          typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_CASC_LEN,
          unsigned int TP_DUAL_IP,
          unsigned int TP_USE_COEFF_RELOAD = 0,
          unsigned int TP_UPSHIFT_CT = 0,
          unsigned int TP_API = 0>
class create_casc_kernel_recur {
   private:
    static constexpr unsigned int kDualIpEn =
        (TP_API == USE_STREAM_API)
            ? TP_DUAL_IP
            : 0; // cascaded kernels do not support dual inputs, unless input interface is a stream
   public:
    static void create(kernel (&firKernels)[TP_CASC_LEN], const std::vector<TT_COEFF>& taps) {
        firKernels[dim - 1] = kernel::create_object<
            fir_interpolate_hb<TT_DATA, TT_COEFF, TP_FIR_LEN, TP_SHIFT, TP_RND, TP_INPUT_WINDOW_VSIZE, true, true,
                               fnFirRangeSym<TP_FIR_LEN, TP_CASC_LEN, dim - 1>(), dim - 1, TP_CASC_LEN, kDualIpEn,
                               USE_COEFF_RELOAD_FALSE, 1, TP_UPSHIFT_CT, TP_API> >(taps);
        create_casc_kernel_recur<dim - 1, TT_DATA, TT_COEFF, TP_FIR_LEN, TP_SHIFT, TP_RND, TP_INPUT_WINDOW_VSIZE,
                                 TP_CASC_LEN, TP_DUAL_IP, USE_COEFF_RELOAD_FALSE, TP_UPSHIFT_CT,
                                 TP_API>::create(firKernels, taps);
    }
};
// first kernel creation, static coefficients
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_CASC_LEN,
          unsigned int TP_DUAL_IP,
          unsigned int TP_UPSHIFT_CT,
          unsigned int TP_API>
class create_casc_kernel_recur<1,
                               TT_DATA,
                               TT_COEFF,
                               TP_FIR_LEN,
                               TP_SHIFT,
                               TP_RND,
                               TP_INPUT_WINDOW_VSIZE,
                               TP_CASC_LEN,
                               TP_DUAL_IP,
                               USE_COEFF_RELOAD_FALSE,
                               TP_UPSHIFT_CT,
                               TP_API> {
   public:
    static void create(kernel (&firKernels)[TP_CASC_LEN], const std::vector<TT_COEFF>& taps) {
        firKernels[0] = kernel::create_object<
            fir_interpolate_hb<TT_DATA, TT_COEFF, TP_FIR_LEN, TP_SHIFT, TP_RND, TP_INPUT_WINDOW_VSIZE, false, true,
                               fnFirRangeSym<TP_FIR_LEN, TP_CASC_LEN, 0>(), 0, TP_CASC_LEN, TP_DUAL_IP,
                               USE_COEFF_RELOAD_FALSE, 1, TP_UPSHIFT_CT, TP_API> >(taps);
    }
};
// Recursive kernel creation, reloadable coefficients
template <int dim,
          typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_CASC_LEN,
          unsigned int TP_DUAL_IP,
          unsigned int TP_UPSHIFT_CT,
          unsigned int TP_API>
class create_casc_kernel_recur<dim,
                               TT_DATA,
                               TT_COEFF,
                               TP_FIR_LEN,
                               TP_SHIFT,
                               TP_RND,
                               TP_INPUT_WINDOW_VSIZE,
                               TP_CASC_LEN,
                               TP_DUAL_IP,
                               USE_COEFF_RELOAD_TRUE,
                               TP_UPSHIFT_CT,
                               TP_API> {
   private:
    static constexpr unsigned int kDualIpEn =
        (TP_API == USE_STREAM_API)
            ? TP_DUAL_IP
            : 0; // cascaded kernels do not support dual inputs, unless input interface is a stream

   public:
    static void create(kernel (&firKernels)[TP_CASC_LEN]) {
        firKernels[dim - 1] = kernel::create_object<
            fir_interpolate_hb<TT_DATA, TT_COEFF, TP_FIR_LEN, TP_SHIFT, TP_RND, TP_INPUT_WINDOW_VSIZE, true, true,
                               fnFirRangeSym<TP_FIR_LEN, TP_CASC_LEN, dim - 1>(), dim - 1, TP_CASC_LEN, kDualIpEn,
                               USE_COEFF_RELOAD_TRUE, 1, TP_UPSHIFT_CT, TP_API> >();
        create_casc_kernel_recur<dim - 1, TT_DATA, TT_COEFF, TP_FIR_LEN, TP_SHIFT, TP_RND, TP_INPUT_WINDOW_VSIZE,
                                 TP_CASC_LEN, TP_DUAL_IP, USE_COEFF_RELOAD_TRUE, TP_UPSHIFT_CT,
                                 TP_API>::create(firKernels);
    }
};
// first kernel creation, reloadable coefficients
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_CASC_LEN,
          unsigned int TP_DUAL_IP,
          unsigned int TP_UPSHIFT_CT,
          unsigned int TP_API>
class create_casc_kernel_recur<1,
                               TT_DATA,
                               TT_COEFF,
                               TP_FIR_LEN,
                               TP_SHIFT,
                               TP_RND,
                               TP_INPUT_WINDOW_VSIZE,
                               TP_CASC_LEN,
                               TP_DUAL_IP,
                               USE_COEFF_RELOAD_TRUE,
                               TP_UPSHIFT_CT,
                               TP_API> {
   public:
    static void create(kernel (&firKernels)[TP_CASC_LEN]) {
        firKernels[0] = kernel::create_object<
            fir_interpolate_hb<TT_DATA, TT_COEFF, TP_FIR_LEN, TP_SHIFT, TP_RND, TP_INPUT_WINDOW_VSIZE, false, true,
                               fnFirRangeSym<TP_FIR_LEN, TP_CASC_LEN, 0>(), 0, TP_CASC_LEN, TP_DUAL_IP,
                               USE_COEFF_RELOAD_TRUE, 1, TP_UPSHIFT_CT, TP_API> >();
    }
};

// Kernel creation, last kernel in cascade, static coefficients
template <int dim,
          typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_CASC_LEN,
          unsigned int TP_DUAL_IP,
          unsigned int TP_USE_COEFF_RELOAD = 0,
          unsigned int TP_NUM_OUTPUTS = 1,
          unsigned int TP_UPSHIFT_CT = 0,
          unsigned int TP_API = 0>
class create_casc_kernel {
   private:
    static constexpr unsigned int kDualIpEn =
        (TP_API == USE_STREAM_API)
            ? TP_DUAL_IP
            : 0; // cascaded kernels do not support dual inputs, unless input interface is a stream
   public:
    static void create(kernel (&firKernels)[TP_CASC_LEN],
                       const std::vector<TT_COEFF>& taps,
                       const unsigned ctUpshift = 0) {
        firKernels[dim - 1] = kernel::create_object<
            fir_interpolate_hb<TT_DATA, TT_COEFF, TP_FIR_LEN, TP_SHIFT, TP_RND, TP_INPUT_WINDOW_VSIZE, true, false,
                               fnFirRangeRemSym<TP_FIR_LEN, TP_CASC_LEN, dim - 1>(), dim - 1, TP_CASC_LEN, kDualIpEn,
                               USE_COEFF_RELOAD_FALSE, TP_NUM_OUTPUTS, TP_UPSHIFT_CT, TP_API> >(taps);
        create_casc_kernel_recur<dim - 1, TT_DATA, TT_COEFF, TP_FIR_LEN, TP_SHIFT, TP_RND, TP_INPUT_WINDOW_VSIZE,
                                 TP_CASC_LEN, TP_DUAL_IP, USE_COEFF_RELOAD_FALSE, TP_UPSHIFT_CT,
                                 TP_API>::create(firKernels, taps);
    }
};
// Kernel creation, last kernel in cascade, reloadable coefficients
template <int dim,
          typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_CASC_LEN,
          unsigned int TP_DUAL_IP,
          unsigned int TP_NUM_OUTPUTS,
          unsigned int TP_UPSHIFT_CT,
          unsigned int TP_API>
class create_casc_kernel<dim,
                         TT_DATA,
                         TT_COEFF,
                         TP_FIR_LEN,
                         TP_SHIFT,
                         TP_RND,
                         TP_INPUT_WINDOW_VSIZE,
                         TP_CASC_LEN,
                         TP_DUAL_IP,
                         USE_COEFF_RELOAD_TRUE,
                         TP_NUM_OUTPUTS,
                         TP_UPSHIFT_CT,
                         TP_API> {
   private:
    static constexpr unsigned int kDualIpEn =
        (TP_API == USE_STREAM_API)
            ? TP_DUAL_IP
            : 0; // cascaded kernels do not support dual inputs, unless input interface is a stream
   public:
    static void create(kernel (&firKernels)[TP_CASC_LEN]) {
        firKernels[dim - 1] = kernel::create_object<
            fir_interpolate_hb<TT_DATA, TT_COEFF, TP_FIR_LEN, TP_SHIFT, TP_RND, TP_INPUT_WINDOW_VSIZE, true, false,
                               fnFirRangeRemSym<TP_FIR_LEN, TP_CASC_LEN, dim - 1>(), dim - 1, TP_CASC_LEN, kDualIpEn,
                               USE_COEFF_RELOAD_TRUE, TP_NUM_OUTPUTS, TP_UPSHIFT_CT, TP_API> >();
        create_casc_kernel_recur<dim - 1, TT_DATA, TT_COEFF, TP_FIR_LEN, TP_SHIFT, TP_RND, TP_INPUT_WINDOW_VSIZE,
                                 TP_CASC_LEN, TP_DUAL_IP, USE_COEFF_RELOAD_TRUE, TP_UPSHIFT_CT,
                                 TP_API>::create(firKernels);
    }
};
// Kernel creation, only kernel in cascade, static coefficients
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_DUAL_IP,
          unsigned int TP_NUM_OUTPUTS,
          unsigned int TP_UPSHIFT_CT,
          unsigned int TP_API>
class create_casc_kernel<1,
                         TT_DATA,
                         TT_COEFF,
                         TP_FIR_LEN,
                         TP_SHIFT,
                         TP_RND,
                         TP_INPUT_WINDOW_VSIZE,
                         1,
                         TP_DUAL_IP,
                         USE_COEFF_RELOAD_FALSE,
                         TP_NUM_OUTPUTS,
                         TP_UPSHIFT_CT,
                         TP_API> {
   public:
    static constexpr unsigned int dim = 1;
    static constexpr unsigned int TP_CASC_LEN = 1;
    static void create(kernel (&firKernels)[TP_CASC_LEN], const std::vector<TT_COEFF>& taps) {
        firKernels[dim - 1] = kernel::create_object<
            fir_interpolate_hb<TT_DATA, TT_COEFF, TP_FIR_LEN, TP_SHIFT, TP_RND, TP_INPUT_WINDOW_VSIZE, false, false,
                               fnFirRangeRemSym<TP_FIR_LEN, TP_CASC_LEN, dim - 1>(), dim - 1, TP_CASC_LEN, TP_DUAL_IP,
                               USE_COEFF_RELOAD_FALSE, TP_NUM_OUTPUTS, TP_UPSHIFT_CT, TP_API> >(taps);
    }
};
// Kernel creation, only kernel in cascade, reloadable coefficients
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_DUAL_IP,
          unsigned int TP_NUM_OUTPUTS,
          unsigned int TP_UPSHIFT_CT,
          unsigned int TP_API>
class create_casc_kernel<1,
                         TT_DATA,
                         TT_COEFF,
                         TP_FIR_LEN,
                         TP_SHIFT,
                         TP_RND,
                         TP_INPUT_WINDOW_VSIZE,
                         1,
                         TP_DUAL_IP,
                         USE_COEFF_RELOAD_TRUE,
                         TP_NUM_OUTPUTS,
                         TP_UPSHIFT_CT,
                         TP_API> {
   public:
    static constexpr unsigned int dim = 1;
    static constexpr unsigned int TP_CASC_LEN = 1;
    static void create(kernel (&firKernels)[TP_CASC_LEN]) {
        firKernels[dim - 1] = kernel::create_object<
            fir_interpolate_hb<TT_DATA, TT_COEFF, TP_FIR_LEN, TP_SHIFT, TP_RND, TP_INPUT_WINDOW_VSIZE, false, false,
                               fnFirRangeRemSym<TP_FIR_LEN, TP_CASC_LEN, dim - 1>(), dim - 1, TP_CASC_LEN, TP_DUAL_IP,
                               USE_COEFF_RELOAD_TRUE, TP_NUM_OUTPUTS, TP_UPSHIFT_CT, TP_API> >();
    }
};
/**
  * @endcond
  */

//--------------------------------------------------------------------------------------------------
// fir_interpolate_hb_graph template
//--------------------------------------------------------------------------------------------------
/**
 * @brief fir_interpolate_hb is a Halfband Interpolation FIR filter
 *
 * These are the templates to configure the halfband interpolator FIR class.
 * @tparam TT_DATA describes the type of individual data samples input to and
 *         output from the filter function. This is a typename and must be one
 *         of the following: \n
 *         int16, cint16, int32, cint32, float, cfloat.
 * @tparam TT_COEFF describes the type of individual coefficients of the filter
 *         taps. \n It must be one of the same set of types listed for TT_DATA
 *         and must also satisfy the following rules:
 *         - Complex types are only supported when TT_DATA is also complex.
 *         - 32 bit types are only supported when TT_DATA is also a 32 bit type,
 *         - TT_COEFF must be an integer type if TT_DATA is an integer type
 *         - TT_COEFF must be a float type if TT_DATA is a float type.
 * @tparam TP_FIR_LEN is an unsigned integer which describes the number of taps
 *         in the filter. TP_FIR_LEN must be in the range 4 to 240 inclusive and
 *         must satisfy (TP_FIR_LEN +1)/4 = N where N is a positive integer.
 * @tparam TP_SHIFT describes power of 2 shift down applied to the accumulation of
 *         FIR terms before output. \n TP_SHIFT must be in the range 0 to 61.
 * @tparam TP_RND describes the selection of rounding to be applied during the
 *         shift down stage of processing. TP_RND must be in the range 0 to 7
 *         where
 *         - 0 = floor (truncate) eg. 3.8 Would become 3.
 *         - 1 = ceiling e.g. 3.2 would become 4.
 *         - 2 = round to positive infinity.
 *         - 3 = round to negative infinity.
 *         - 4 = round symmetrical to infinity.
 *         - 5 = round symmetrical to zero.
 *         - 6 = round convergent to even.
 *         - 7 = round convergent to odd. \n
 *         Modes 2 to 7 round to the nearest integer. They differ only in how
 *         they round for values of 0.5.
 * @tparam TP_INPUT_WINDOW_VSIZE describes the number of samples in the window API
 *         used for input to the filter function. \n
 *         The number of values in the output window will be TP_INPUT_WINDOW_VSIZE
 *         multiplied by 2 by virtue the halfband interpolation factor. \n
 *         Note: Margin size should not be included in TP_INPUT_WINDOW_VSIZE.
 * @tparam TP_CASC_LEN describes the number of AIE processors to split the operation
 *         over. \n This allows resource to be traded for higher performance.
 *         TP_CASC_LEN must be in the range 1 (default) to 9.
 * @tparam TP_DUAL_IP allows 2 input ports to be connected to FIR, increasing available throughput. \n
 *         Depending on TP_API, additional input ports functionality differs.
 *         If TP_API is set to use streams, then \n
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
 *         reloading should be used.   \n When defining the parameter:
 *         - 0 = static coefficients, defined in filter constructor,
 *         - 1 = reloadable coefficients, passed as argument to runtime function. \n
 *
 *         Note: when used, optional port: ``` port<input> coeff; ``` will be added to the FIR. \n
 * @tparam TP_NUM_OUTPUTS sets the number of ports over which the output is sent. \n
 *         This can be 1 or 2. It is set to 1 by default. \n
 *         Depending on TP_API, additional output ports functionality differs.
 *         For Windows API, additional output provides flexibility in connecting
 *         FIR output with multiple destinations.
 *         Additional output ``out2`` is an exact copy of the data of the output port ``out``. \n
 *
 *         With Stream API, the additional output port increases the FIR's throughput. \n
 *         Data is sent in a 128-bit interleaved pattern, e.g. : \n
 *         - samples 0-3 is sent over stream0 for cint16 data type, \n
 *         - samples 4-7 is sent over stream1 for cint16 data type. \n
 *
 *         Note: when used, optional port: ``` port<output> out2; ``` will be added to the FIR. \n
 * @tparam TP_UPSHIFT_CT upshift unit center tap. \n
 *         Uses provided center tap's value to upshift data sample.
 *         Note: when complex coefficient's are used, center tap's real part will be used for the upshift. \n
 *         Note: Upshift UCT is only supported with 16-bit coefficient types, i.e. int16 and cint16. \n
 *         TP_UPSHIFT_CT must be in the range 0 to 47.
 * @tparam TP_API specifies if the input/output interface should be window-based or stream-based.  \n
 *         The values supported are 0 (window API) or 1 (stream API).
 **/

template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_CASC_LEN = 1,
          unsigned int TP_DUAL_IP = 0,
          unsigned int TP_USE_COEFF_RELOAD = 0, // 1 = use coeff reload, 0 = don't use coeff reload
          unsigned int TP_NUM_OUTPUTS = 1,
          unsigned int TP_UPSHIFT_CT = 0,
          unsigned int TP_API = 0>
/**
 **/
class fir_interpolate_hb_graph : public graph {
   private:
    static_assert(TP_CASC_LEN <= 40, "ERROR: Unsupported Cascade length");

    static constexpr unsigned int kMaxTapsPerKernel = 1024;
    // Limit FIR length per kernel. Longer FIRs may exceed Program Memory and/or system memory combined with window
    // buffers may exceed Memory Module size
    static_assert(TP_FIR_LEN / TP_CASC_LEN <= kMaxTapsPerKernel,
                  "ERROR: Requested FIR length and Cascade length exceeds supported number of taps per kernel. Please "
                  "increase the cascade legnth to accomodate the FIR design.");

    // Limit FIR length for reloadable coeffs. Reloadable coeffs need a storage space that contibutes to system memory
    // exceeding Memory Module size.
    static_assert(TP_USE_COEFF_RELOAD == 0 || TP_FIR_LEN <= kMaxTapsPerKernel,
                  "ERROR: Exceeded maximum supported FIR length with reloadable coefficients. Please limit the FIR "
                  "length or disable coefficient reload.");

    static constexpr unsigned int kMemoryModuleSize = 32768;
    static constexpr unsigned int bufferSize = ((TP_FIR_LEN + TP_INPUT_WINDOW_VSIZE) * sizeof(TT_DATA));
    // Requested Window buffer exceeds memory module size
    static_assert(TP_API != 0 || bufferSize < kMemoryModuleSize,
                  "ERROR: Input Window size (based on requrested window size and FIR length margin) exceeds Memory "
                  "Module size of 32kB");

    void create_connections() {
        // make input connections
        if
            constexpr(TP_API == USE_WINDOW_API) {
                connect<window<TP_INPUT_WINDOW_VSIZE * sizeof(TT_DATA),
                               fnFirMargin<TP_FIR_LEN / kInterpolateFactor, TT_DATA>() * sizeof(TT_DATA)> >(
                    in, m_firKernels[0].in[0]);
                for (int i = 1; i < TP_CASC_LEN; i++) {
                    single_buffer(m_firKernels[i].in[0]);
                    connect<window<TP_INPUT_WINDOW_VSIZE * sizeof(TT_DATA) +
                                   fnFirMargin<TP_FIR_LEN / kInterpolateFactor, TT_DATA>() * sizeof(TT_DATA)> >(
                        async(m_firKernels[i - 1].out[1]), async(m_firKernels[i].in[0]));
                }
                if
                    constexpr(TP_DUAL_IP == DUAL_IP_DUAL) {
                        connect<window<TP_INPUT_WINDOW_VSIZE * sizeof(TT_DATA),
                                       fnFirMargin<TP_FIR_LEN / kInterpolateFactor, TT_DATA>() * sizeof(TT_DATA)> >(
                            in2, m_firKernels[0].in[1]);
                    }
            }
        else {
            if
                constexpr(TP_DUAL_IP == DUAL_IP_SINGLE) {
                    connect<window<TP_INPUT_WINDOW_VSIZE * sizeof(TT_DATA),
                                   fnFirMargin<TP_FIR_LEN / kInterpolateFactor, TT_DATA>() * sizeof(TT_DATA)> >(
                        in, m_firKernels[0].in[0]);
                    for (int i = 1; i < TP_CASC_LEN; i++) {
                        connect<window<TP_INPUT_WINDOW_VSIZE * sizeof(TT_DATA),
                                       fnFirMargin<TP_FIR_LEN / kInterpolateFactor, TT_DATA>() * sizeof(TT_DATA)> >(
                            in, m_firKernels[i].in[0]);
                    }
                }
            else if
                constexpr(TP_DUAL_IP == DUAL_IP_DUAL) {
                    kernel m_inWidgetKernel;
                    m_inWidgetKernel = kernel::create_object<
                        widget_api_cast<TT_DATA, USE_STREAM_API, USE_WINDOW_API, 2, TP_INPUT_WINDOW_VSIZE, 1, 0> >();
                    connect<stream>(in, m_inWidgetKernel.in[0]);
                    connect<stream>(in2, m_inWidgetKernel.in[1]);
                    connect<window<TP_INPUT_WINDOW_VSIZE * sizeof(TT_DATA),
                                   fnFirMargin<TP_FIR_LEN / kInterpolateFactor, TT_DATA>() * sizeof(TT_DATA)> >(
                        m_inWidgetKernel.out[0], m_firKernels[0].in[0]);
                    source(m_inWidgetKernel) = "widget_api_cast.cpp";
                    headers(m_inWidgetKernel) = {"widget_api_cast.hpp"};
                    runtime<ratio>(m_inWidgetKernel) = 0.8;

                    for (int i = 1; i < TP_CASC_LEN; i++) {
                        single_buffer(m_firKernels[i].in[0]);
                        connect<window<TP_INPUT_WINDOW_VSIZE * sizeof(TT_DATA) +
                                       fnFirMargin<TP_FIR_LEN, TT_DATA>() * sizeof(TT_DATA)> >(
                            async(m_firKernels[i - 1].out[1]), async(m_firKernels[i].in[0]));
                    }
                }
        }

        int rtpPortPos = (TP_DUAL_IP == DUAL_IP_DUAL & TP_API == USE_WINDOW_API) ? 2 : 1;
        if
            constexpr(TP_USE_COEFF_RELOAD == 1) { connect<parameter>(coeff, async(m_firKernels[0].in[rtpPortPos])); }

        // make cascade connections
        for (int i = 1; i < TP_CASC_LEN; i++) {
            connect<cascade>(m_firKernels[i - 1].out[0], m_firKernels[i].in[1]);
        }
        // make output connections
        if (TP_API == 0) {
            connect<window<kInterpolateFactor * TP_INPUT_WINDOW_VSIZE * sizeof(TT_DATA)> >(
                m_firKernels[TP_CASC_LEN - 1].out[0], out);
        } else {
            connect<stream>(m_firKernels[TP_CASC_LEN - 1].out[0], out);
        }

        if
            constexpr(TP_NUM_OUTPUTS == 2) {
                if
                    constexpr(TP_API == USE_WINDOW_API) {
                        connect<window<kInterpolateFactor * TP_INPUT_WINDOW_VSIZE * sizeof(TT_DATA)> >(
                            m_firKernels[TP_CASC_LEN - 1].out[1], out2);
                    }
                else {
                    connect<stream>(m_firKernels[TP_CASC_LEN - 1].out[1], out2);
                }
            }
        for (int i = 0; i < TP_CASC_LEN; i++) {
            // Specify mapping constraints
            runtime<ratio>(m_firKernels[i]) = 0.8;
            // Source files
            source(m_firKernels[i]) = "fir_interpolate_hb.cpp";
        }
    }

   public:
    kernel m_firKernels[TP_CASC_LEN];

    /**
     * The input data to the function. This input is either a window API of
     * samples of TT_DATA type or stream API (depending on TP_API).
     * Note: Margin is added internally to the graph, when connecting input port
     * with kernel port. Therefore, margin should not be added when connecting
     * graph to a higher level design unit.
     * Margin size (in Bytes) equals to TP_FIR_LEN rounded up to a nearest
     * multiple of 32 bytes.
     **/
    port<input> in;

    /**
     * The output data from the function. This output is either a window API of
     * samples of TT_DATA type or stream API (depending on TP_API).
     * Number of output samples is determined by interpolation & decimation factors (if present).
     **/
    port<output> out;

    /**
     * The conditional input data to the function.
     * This input is (generated when TP_DUAL_IP == 1) either a window API of
     * samples of TT_DATA type or stream API (depending on TP_API).
     *
     **/
    port_conditional<input, (TP_DUAL_IP == 1)> in2;

    /**
     * The conditional coefficient data to the function.
     * This port is (generated when TP_USE_COEFF_RELOAD == 1) an array of coefficients of TT_COEFF type.
     *
     **/
    port_conditional<input, (TP_USE_COEFF_RELOAD == 1)> coeff;

    /**
     * The output data from the function.
     * This output is (generated when TP_NUM_OUTPUTS == 2) either a window API of
     * samples of TT_DATA type or stream API (depending on TP_API).
     * Number of output samples is determined by interpolation & decimation factors (if present).
     **/
    port_conditional<output, (TP_NUM_OUTPUTS == 2)> out2;

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
        return fir_interpolate_hb<TT_DATA, TT_COEFF, TP_FIR_LEN, TP_SHIFT, TP_RND, TP_INPUT_WINDOW_VSIZE, false, true,
                                  firRange, 0, TP_CASC_LEN, TP_DUAL_IP, TP_USE_COEFF_RELOAD, TP_NUM_OUTPUTS,
                                  TP_UPSHIFT_CT, TP_API>::get_m_kArch();
    };

    /**
     * @brief This is the constructor function for the FIR graph with static coefficients.
     * @param[in] taps   a reference to the std::vector array of taps values of type TT_COEFF. \n
     *                   The taps array must be supplied in a compressed form for
     *                   this halfband application, i.e. \n
     *                   taps[] = {c0, c2, c4, ..., cN, cCT} where \n
     *                   N = (TP_FIR_LEN+1)/4 and
     *                   cCT is the center tap. \n
     *                   For example, a 7-tap halfband interpolator might use coeffs
     *                   (1, 0, 2, 5, 2, 0, 1). \n This would be input as
     *                   taps[]= {1,2,5} since the context of halfband interpolation
     *                   allows the remaining coefficients to be inferred. \n
     **/
    fir_interpolate_hb_graph(const std::vector<TT_COEFF>& taps) {
        // create kernels
        create_casc_kernel<TP_CASC_LEN, TT_DATA, TT_COEFF, TP_FIR_LEN, TP_SHIFT, TP_RND, TP_INPUT_WINDOW_VSIZE,
                           TP_CASC_LEN, TP_DUAL_IP, USE_COEFF_RELOAD_FALSE, TP_NUM_OUTPUTS, TP_UPSHIFT_CT,
                           TP_API>::create(m_firKernels, taps);
        create_connections();
    }

    /**
     * @brief This is the constructor function for the FIR graph with reloadable coefficients.
     **/
    fir_interpolate_hb_graph() {
        // create kernels
        create_casc_kernel<TP_CASC_LEN, TT_DATA, TT_COEFF, TP_FIR_LEN, TP_SHIFT, TP_RND, TP_INPUT_WINDOW_VSIZE,
                           TP_CASC_LEN, TP_DUAL_IP, USE_COEFF_RELOAD_TRUE, TP_NUM_OUTPUTS, TP_UPSHIFT_CT,
                           TP_API>::create(m_firKernels);
        create_connections();
    }
};
}
}
}
}
}

#endif // _DSPLIB_FIR_INTERPOLATE_HB_GRAPH_HPP_
