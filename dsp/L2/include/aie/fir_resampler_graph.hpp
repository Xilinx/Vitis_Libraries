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
#ifndef _DSPLIB_FIR_RESAMPLER_GRAPH_HPP_
#define _DSPLIB_FIR_RESAMPLER_GRAPH_HPP_
/*
The file captures the definition of the 'L2' graph level class for
the FIR_RESAMPLER library element.
*/

#include <adf.h>
#include <vector>
#include "graph_utils.hpp"
#include <tuple>

#include "fir_resampler.hpp"
namespace xf {
namespace dsp {
namespace aie {
namespace fir {
namespace resampler {
using namespace adf;

enum IO_API { WINDOW = 0, STREAM };

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
          unsigned int TP_INTERPOLATE_FACTOR,
          unsigned int TP_DECIMATE_FACTOR,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_CASC_LEN,
          unsigned int TP_USE_COEFF_RELOAD = 0,
          unsigned int TP_DUAL_IP = 0,
          unsigned int TP_API = 0>
class create_casc_kernel_recur {
   public:
    static void create(kernel (&firKernels)[TP_CASC_LEN], const std::vector<TT_COEFF>& taps) {
        firKernels[dim - 1] = kernel::create_object<fir_resampler<
            TT_DATA, TT_COEFF, TP_FIR_LEN, TP_INTERPOLATE_FACTOR, TP_DECIMATE_FACTOR, TP_SHIFT, TP_RND,
            TP_INPUT_WINDOW_VSIZE, true, true,
            fnFirRange<CEIL(TP_FIR_LEN, TP_INTERPOLATE_FACTOR), TP_CASC_LEN, dim - 1, TP_INTERPOLATE_FACTOR>(), dim - 1,
            TP_CASC_LEN, TP_USE_COEFF_RELOAD, 1, TP_DUAL_IP, TP_API> >(taps);
        create_casc_kernel_recur<dim - 1, TT_DATA, TT_COEFF, TP_FIR_LEN, TP_INTERPOLATE_FACTOR, TP_DECIMATE_FACTOR,
                                 TP_SHIFT, TP_RND, TP_INPUT_WINDOW_VSIZE, TP_CASC_LEN, TP_USE_COEFF_RELOAD, TP_DUAL_IP,
                                 TP_API>::create(firKernels, taps);
    }
};
// Recursive kernel creation, static coefficients
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_INTERPOLATE_FACTOR,
          unsigned int TP_DECIMATE_FACTOR,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_CASC_LEN,
          unsigned int TP_DUAL_IP,
          unsigned int TP_API>
class create_casc_kernel_recur<1,
                               TT_DATA,
                               TT_COEFF,
                               TP_FIR_LEN,
                               TP_INTERPOLATE_FACTOR,
                               TP_DECIMATE_FACTOR,
                               TP_SHIFT,
                               TP_RND,
                               TP_INPUT_WINDOW_VSIZE,
                               TP_CASC_LEN,
                               USE_COEFF_RELOAD_FALSE,
                               TP_DUAL_IP,
                               TP_API> {
   public:
    static void create(kernel (&firKernels)[TP_CASC_LEN], const std::vector<TT_COEFF>& taps) {
        firKernels[0] = kernel::create_object<
            fir_resampler<TT_DATA, TT_COEFF, TP_FIR_LEN, TP_INTERPOLATE_FACTOR, TP_DECIMATE_FACTOR, TP_SHIFT, TP_RND,
                          TP_INPUT_WINDOW_VSIZE, false, true,
                          fnFirRange<CEIL(TP_FIR_LEN, TP_INTERPOLATE_FACTOR), TP_CASC_LEN, 0, TP_INTERPOLATE_FACTOR>(),
                          0, TP_CASC_LEN, USE_COEFF_RELOAD_FALSE, 1, TP_DUAL_IP, TP_API> >(taps);
    }
};
// Recursive kernel creation, reloadable coefficients
template <int dim,
          typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_INTERPOLATE_FACTOR,
          unsigned int TP_DECIMATE_FACTOR,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_CASC_LEN,
          unsigned int TP_DUAL_IP,
          unsigned int TP_API>
class create_casc_kernel_recur<dim,
                               TT_DATA,
                               TT_COEFF,
                               TP_FIR_LEN,
                               TP_INTERPOLATE_FACTOR,
                               TP_DECIMATE_FACTOR,
                               TP_SHIFT,
                               TP_RND,
                               TP_INPUT_WINDOW_VSIZE,
                               TP_CASC_LEN,
                               USE_COEFF_RELOAD_TRUE,
                               TP_DUAL_IP,
                               TP_API> {
   public:
    static void create(kernel (&firKernels)[TP_CASC_LEN]) {
        firKernels[dim - 1] = kernel::create_object<fir_resampler<
            TT_DATA, TT_COEFF, TP_FIR_LEN, TP_INTERPOLATE_FACTOR, TP_DECIMATE_FACTOR, TP_SHIFT, TP_RND,
            TP_INPUT_WINDOW_VSIZE, true, true,
            fnFirRange<CEIL(TP_FIR_LEN, TP_INTERPOLATE_FACTOR), TP_CASC_LEN, dim - 1, TP_INTERPOLATE_FACTOR>(), dim - 1,
            TP_CASC_LEN, USE_COEFF_RELOAD_TRUE, 1, TP_DUAL_IP, TP_API> >();
        create_casc_kernel_recur<dim - 1, TT_DATA, TT_COEFF, TP_FIR_LEN, TP_INTERPOLATE_FACTOR, TP_DECIMATE_FACTOR,
                                 TP_SHIFT, TP_RND, TP_INPUT_WINDOW_VSIZE, TP_CASC_LEN, USE_COEFF_RELOAD_TRUE,
                                 TP_DUAL_IP, TP_API>::create(firKernels);
    }
};
// Recursive kernel creation, reloadable coefficients
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_INTERPOLATE_FACTOR,
          unsigned int TP_DECIMATE_FACTOR,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_CASC_LEN,
          unsigned int TP_DUAL_IP,
          unsigned int TP_API>
class create_casc_kernel_recur<1,
                               TT_DATA,
                               TT_COEFF,
                               TP_FIR_LEN,
                               TP_INTERPOLATE_FACTOR,
                               TP_DECIMATE_FACTOR,
                               TP_SHIFT,
                               TP_RND,
                               TP_INPUT_WINDOW_VSIZE,
                               TP_CASC_LEN,
                               USE_COEFF_RELOAD_TRUE,
                               TP_DUAL_IP,
                               TP_API> {
   public:
    static void create(kernel (&firKernels)[TP_CASC_LEN]) {
        firKernels[0] = kernel::create_object<
            fir_resampler<TT_DATA, TT_COEFF, TP_FIR_LEN, TP_INTERPOLATE_FACTOR, TP_DECIMATE_FACTOR, TP_SHIFT, TP_RND,
                          TP_INPUT_WINDOW_VSIZE, false, true,
                          fnFirRange<CEIL(TP_FIR_LEN, TP_INTERPOLATE_FACTOR), TP_CASC_LEN, 0, TP_INTERPOLATE_FACTOR>(),
                          0, TP_CASC_LEN, USE_COEFF_RELOAD_TRUE, 1, TP_DUAL_IP, TP_API> >();
    }
};
// Kernel creation, static coefficients
template <int dim,
          typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_INTERPOLATE_FACTOR,
          unsigned int TP_DECIMATE_FACTOR,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_CASC_LEN,
          unsigned int TP_USE_COEFF_RELOAD = 0,
          unsigned int TP_NUM_OUTPUTS = 1,
          unsigned int TP_DUAL_IP = 0,
          unsigned int TP_API = 0>
class create_casc_kernel {
   public:
    static void create(kernel (&firKernels)[TP_CASC_LEN], const std::vector<TT_COEFF>& taps) {
        firKernels[dim - 1] = kernel::create_object<fir_resampler<
            TT_DATA, TT_COEFF, TP_FIR_LEN, TP_INTERPOLATE_FACTOR, TP_DECIMATE_FACTOR, TP_SHIFT, TP_RND,
            TP_INPUT_WINDOW_VSIZE, true, false,
            fnFirRangeRem<CEIL(TP_FIR_LEN, TP_INTERPOLATE_FACTOR), TP_CASC_LEN, dim - 1, TP_INTERPOLATE_FACTOR>(),
            dim - 1, TP_CASC_LEN, TP_USE_COEFF_RELOAD, TP_NUM_OUTPUTS, TP_DUAL_IP, TP_API> >(taps);
        create_casc_kernel_recur<dim - 1, TT_DATA, TT_COEFF, TP_FIR_LEN, TP_INTERPOLATE_FACTOR, TP_DECIMATE_FACTOR,
                                 TP_SHIFT, TP_RND, TP_INPUT_WINDOW_VSIZE, TP_CASC_LEN, TP_USE_COEFF_RELOAD, TP_DUAL_IP,
                                 TP_API>::create(firKernels, taps);
    }
};
// Kernel creation, reloadable coefficients
template <int dim,
          typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_INTERPOLATE_FACTOR,
          unsigned int TP_DECIMATE_FACTOR,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_CASC_LEN,
          unsigned int TP_NUM_OUTPUTS,
          unsigned int TP_DUAL_IP,
          unsigned int TP_API>
class create_casc_kernel<dim,
                         TT_DATA,
                         TT_COEFF,
                         TP_FIR_LEN,
                         TP_INTERPOLATE_FACTOR,
                         TP_DECIMATE_FACTOR,
                         TP_SHIFT,
                         TP_RND,
                         TP_INPUT_WINDOW_VSIZE,
                         TP_CASC_LEN,
                         USE_COEFF_RELOAD_TRUE,
                         TP_NUM_OUTPUTS,
                         TP_DUAL_IP,
                         TP_API> {
   public:
    static void create(kernel (&firKernels)[TP_CASC_LEN]) {
        firKernels[dim - 1] = kernel::create_object<fir_resampler<
            TT_DATA, TT_COEFF, TP_FIR_LEN, TP_INTERPOLATE_FACTOR, TP_DECIMATE_FACTOR, TP_SHIFT, TP_RND,
            TP_INPUT_WINDOW_VSIZE, true, false,
            fnFirRangeRem<CEIL(TP_FIR_LEN, TP_INTERPOLATE_FACTOR), TP_CASC_LEN, dim - 1, TP_INTERPOLATE_FACTOR>(),
            dim - 1, TP_CASC_LEN, USE_COEFF_RELOAD_TRUE, TP_NUM_OUTPUTS, TP_DUAL_IP, TP_API> >();
        create_casc_kernel_recur<dim - 1, TT_DATA, TT_COEFF, TP_FIR_LEN, TP_INTERPOLATE_FACTOR, TP_DECIMATE_FACTOR,
                                 TP_SHIFT, TP_RND, TP_INPUT_WINDOW_VSIZE, TP_CASC_LEN, USE_COEFF_RELOAD_TRUE,
                                 TP_DUAL_IP, TP_API>::create(firKernels);
    }
};
// Kernel creation, single kernel, static coefficients
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_INTERPOLATE_FACTOR,
          unsigned int TP_DECIMATE_FACTOR,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_NUM_OUTPUTS,
          unsigned int TP_DUAL_IP,
          unsigned int TP_API>
class create_casc_kernel<1,
                         TT_DATA,
                         TT_COEFF,
                         TP_FIR_LEN,
                         TP_INTERPOLATE_FACTOR,
                         TP_DECIMATE_FACTOR,
                         TP_SHIFT,
                         TP_RND,
                         TP_INPUT_WINDOW_VSIZE,
                         1,
                         USE_COEFF_RELOAD_FALSE,
                         TP_NUM_OUTPUTS,
                         TP_DUAL_IP,
                         TP_API> {
   public:
    static constexpr unsigned int dim = 1;
    static constexpr unsigned int TP_CASC_LEN = 1;
    static void create(kernel (&firKernels)[TP_CASC_LEN], const std::vector<TT_COEFF>& taps) {
        firKernels[dim - 1] = kernel::create_object<fir_resampler<
            TT_DATA, TT_COEFF, TP_FIR_LEN, TP_INTERPOLATE_FACTOR, TP_DECIMATE_FACTOR, TP_SHIFT, TP_RND,
            TP_INPUT_WINDOW_VSIZE, false, false,
            fnFirRangeRem<CEIL(TP_FIR_LEN, TP_INTERPOLATE_FACTOR), TP_CASC_LEN, dim - 1, TP_INTERPOLATE_FACTOR>(),
            dim - 1, TP_CASC_LEN, USE_COEFF_RELOAD_FALSE, TP_NUM_OUTPUTS, TP_DUAL_IP, TP_API> >(taps);
    }
};
// Kernel creation, single kernel, reloadable coefficients
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_INTERPOLATE_FACTOR,
          unsigned int TP_DECIMATE_FACTOR,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_NUM_OUTPUTS,
          unsigned int TP_DUAL_IP,
          unsigned int TP_API>
class create_casc_kernel<1,
                         TT_DATA,
                         TT_COEFF,
                         TP_FIR_LEN,
                         TP_INTERPOLATE_FACTOR,
                         TP_DECIMATE_FACTOR,
                         TP_SHIFT,
                         TP_RND,
                         TP_INPUT_WINDOW_VSIZE,
                         1,
                         USE_COEFF_RELOAD_TRUE,
                         TP_NUM_OUTPUTS,
                         TP_DUAL_IP,
                         TP_API> {
   public:
    static constexpr unsigned int dim = 1;
    static constexpr unsigned int TP_CASC_LEN = 1;
    static void create(kernel (&firKernels)[TP_CASC_LEN]) {
        firKernels[dim - 1] = kernel::create_object<fir_resampler<
            TT_DATA, TT_COEFF, TP_FIR_LEN, TP_INTERPOLATE_FACTOR, TP_DECIMATE_FACTOR, TP_SHIFT, TP_RND,
            TP_INPUT_WINDOW_VSIZE, false, false,
            fnFirRangeRem<CEIL(TP_FIR_LEN, TP_INTERPOLATE_FACTOR), TP_CASC_LEN, dim - 1, TP_INTERPOLATE_FACTOR>(),
            dim - 1, TP_CASC_LEN, USE_COEFF_RELOAD_TRUE, TP_NUM_OUTPUTS, TP_DUAL_IP, TP_API> >();
    }
};
/**
  * @endcond
  */

//--------------------------------------------------------------------------------------------------
// fir_resampler_graph template
//--------------------------------------------------------------------------------------------------

/**
 * @brief fir_resampler is a generic asymmetric FIR filter that can do fractional and integer interpolation and
 *decimation.
 *
 * These are the templates to configure the generic FIR class.
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
 *         in the filter.
 * @tparam TP_INTERPOLATE_FACTOR is an unsigned integer which describes the
 *         interpolation factor of the filter. TP_INTERPOLATE_FACTOR must be in the
 *         range 3 to 16.
 * @tparam TP_DECIMATE_FACTOR is an unsigned integer which describes the
 *         decimation factor of the filter. TP_DECIMATE_FACTOR must be in the
 *         range 2 to 16. The decimation factor should be less that the interpolation
 *         factor and should not be divisible factor of the interpolation factor.
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
 *         multipled by TP_INTERPOLATE_FACTOR and divided by TP_DECIMATE_FACTOR. \n
 *         In the instance this would lead to a fraction number of output samples,
 *         this would be rounded down. \n
 *         Note: Margin size should not be included in TP_INPUT_WINDOW_VSIZE.
 * @tparam TP_CASC_LEN describes the number of AIE processors to split the operation
 *         over. \n This allows resource to be traded for higher performance.
 *         TP_CASC_LEN must be in the range 1 (default) to 9.
 * @tparam TP_USE_COEFF_RELOAD allows the user to select if runtime coefficient
 *         reloading should be used. \n When defining the parameter:
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
 * @tparam TP_DUAL_IP allows 2 stream inputs to be connected to FIR, increasing available throughput. \n
 *         When set to 0, single stream will be connected as FIRs input. \n
 *         When set to 1, two stream inputs will be connected. \n
 *         In such case data should be organized in 128-bit interleaved pattern, e.g.: \n
 *         - samples 0-3 to be sent over stream0 for cint16 data type, \n
 *         - samples 4-7 to be sent over stream1 for cint16 data type. \n
 * @tparam TP_API specifies if the input/output interface should be window-based or stream-based.  \n
 *         The values supported are 0 (window API) or 1 (stream API).
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
          unsigned int TP_API = 0>
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
                  "increase the cascade legnth to accomodate the FIR design.");

    // Limit FIR length for reloadable coeffs. Reloadable coeffs need a storage space that contibutes to system memory
    // exceeding Memory Module size.
    static_assert(TP_USE_COEFF_RELOAD == 0 || TP_FIR_LEN <= kMaxTapsPerKernel,
                  "ERROR: Exceeded maximum supported FIR length with reloadable coefficients. Please limit the FIR "
                  "length or disable coefficient reload.");

    static constexpr unsigned int kMemoryModuleSize = 32768;
    static constexpr unsigned int inBufferSize = ((TP_FIR_LEN + TP_INPUT_WINDOW_VSIZE) * sizeof(TT_DATA));
    // Requested Input Window buffer exceeds memory module size
    static_assert(TP_API != 0 || inBufferSize < kMemoryModuleSize,
                  "ERROR: Input Window size (based on requrested window size and FIR length margin) exceeds Memory "
                  "Module size of 32kB");

    static constexpr unsigned int outBufferSize =
        (TP_INTERPOLATE_FACTOR * TP_INPUT_WINDOW_VSIZE * sizeof(TT_DATA) / TP_DECIMATE_FACTOR);
    // Requested Output Window buffer exceeds memory module size
    static_assert(TP_API != 0 || outBufferSize < kMemoryModuleSize,
                  "ERROR: Input Window size (based on requrested window size and FIR length margin) exceeds Memory "
                  "Module size of 32kB");

    std::array<connect<stream, stream>*, TP_CASC_LEN> net;
    std::array<connect<stream, stream>*, TP_CASC_LEN> net2;

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
                                           TP_API>;

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

    int calculate_fifo_depth(int kernelPos) {
        // In FIFO mode, FIFO size has to be a multiple of 128 bits.
        // In FIFO mode, BD length has to be >= 128 bytes!
        // Conservative assumptions need to be made here.
        // For an overall decimation factor, with instreasing decimation factor,
        // the amount of input stream data required to produce output samples also increases.
        // This strains the FIFOs closer to the begining of the chain comparably more
        // than the ones closest to the output.
        // For an overall interpolation design, with increasing interpolation factor,
        // the amount of output stream data produced also increases.
        // Last kernel in the chain is limited by output bandwidth, which puts pressure on input stream.
        // Upstream kernels send data through cascade interface and so to keep the data flow without backpressure,
        // kernels closer to output stream require more FIFO storage.
        //
        // Conservative assumptions need to be made here, as mapper may place multiple buffers in
        // each of the memory banks, that may introduce Memory conflicts.
        // On top of that, the placement of input source wrt brodcast kernel inputs may introduce significant routing
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

    void create_connections() {
        // make input connections
        if (TP_API == USE_WINDOW_API) {
            connect<window<TP_INPUT_WINDOW_VSIZE * sizeof(TT_DATA),
                           fnFirMargin<((TP_FIR_LEN + TP_INTERPOLATE_FACTOR - 1) / TP_INTERPOLATE_FACTOR), TT_DATA>() *
                               sizeof(TT_DATA)> >(in[0], m_firKernels[0].in[0]);

            for (int i = 1; i < TP_CASC_LEN; i++) {
                single_buffer(m_firKernels[i].in[0]);
                connect<
                    window<TP_INPUT_WINDOW_VSIZE * sizeof(TT_DATA) +
                           fnFirMargin<((TP_FIR_LEN + TP_INTERPOLATE_FACTOR - 1) / TP_INTERPOLATE_FACTOR), TT_DATA>() *
                               sizeof(TT_DATA)> >(async(m_firKernels[i - 1].out[1]), async(m_firKernels[i].in[0]));
            }

        } else if (TP_API == USE_STREAM_API) {
            for (int i = 0; i < TP_CASC_LEN; i++) {
                net[i] = new connect<stream, stream>(in[0], m_firKernels[i].in[0]);
                fifo_depth(*net[i]) = calculate_fifo_depth(i);
            }
        }

        // make input in2 connections
        if
            constexpr(TP_DUAL_IP == DUAL_IP_DUAL) {
                if (TP_API == USE_STREAM_API) {
                    for (int i = 0; i < TP_CASC_LEN; i++) {
                        net2[i] = new connect<stream, stream>(in2[0], m_firKernels[i].in[1]);
                        fifo_depth(*net2[i]) = calculate_fifo_depth(i);
                    }
                } else {
                    for (int i = 0; i < TP_CASC_LEN; i++) {
                        connect<window<TP_INPUT_WINDOW_VSIZE * sizeof(TT_DATA),
                                       fnFirMargin<TP_FIR_LEN, TT_DATA>() * sizeof(TT_DATA)> >(in2[0],
                                                                                               m_firKernels[i].in[1]);
                    }
                }
            }

        int rtpPortPos = TP_DUAL_IP == DUAL_IP_DUAL ? 2 : 1;
        // make conditional RTP connection
        if
            constexpr(TP_USE_COEFF_RELOAD == 1) { connect<parameter>(coeff[0], async(m_firKernels[0].in[rtpPortPos])); }

        // make cascade connections
        int cascPos = TP_DUAL_IP == 0 ? 1 : 2;
        for (int i = 1; i < TP_CASC_LEN; i++) {
            connect<cascade>(m_firKernels[i - 1].out[0], m_firKernels[i].in[cascPos]);
        }

        // make output connections
        if (TP_API == USE_WINDOW_API) {
            connect<window<TP_INTERPOLATE_FACTOR * TP_INPUT_WINDOW_VSIZE * sizeof(TT_DATA) / TP_DECIMATE_FACTOR> >(
                m_firKernels[TP_CASC_LEN - 1].out[0], out[0]);
        } else {
            connect<stream>(m_firKernels[TP_CASC_LEN - 1].out[0], out[0]);
        }

        if
            constexpr(TP_NUM_OUTPUTS == 2) {
                // make output connections
                if (TP_API == USE_WINDOW_API) {
                    connect<
                        window<TP_INTERPOLATE_FACTOR * TP_INPUT_WINDOW_VSIZE * sizeof(TT_DATA) / TP_DECIMATE_FACTOR> >(
                        m_firKernels[TP_CASC_LEN - 1].out[1], out2[0]);
                } else {
                    connect<stream>(m_firKernels[TP_CASC_LEN - 1].out[1], out2[0]);
                }
            }

        for (int i = 0; i < TP_CASC_LEN; i++) {
            // Specify mapping constraints
            runtime<ratio>(m_firKernels[i]) = 0.8;
            // Source files
            source(m_firKernels[i]) = "fir_resampler.cpp";
        }
    }
    kernel m_firKernels[TP_CASC_LEN];

   public:
    /**
     * The input data to the function. This input is either a window API of
     * samples of TT_DATA type or stream API (depending on TP_API).
     * Note: Margin is added internally to the graph, when connecting input port
     * with kernel port. Therefore, margin should not be added when connecting
     * graph to a higher level design unit.
     * Margin size (in Bytes) equals to TP_FIR_LEN rounded up to a nearest
     * multiple of 32 bytes.
     **/
    port_array<input, 1> in;

    /**
     * The output data from the function. This output is either a window API of
     * samples of TT_DATA type or stream API (depending on TP_API).
     * Number of output samples is determined by interpolation & decimation factors (if present).
     **/
    port_array<output, 1> out;

    /**
     * The conditional input data to the function.
     * This input is (generated when TP_DUAL_IP == 1) either a window API of
     * samples of TT_DATA type or stream API (depending on TP_API).
     *
     **/
    port_conditional_array<input, (TP_DUAL_IP == 1), 1> in2;

    /**
     * The conditional coefficient data to the function.
     * This port is (generated when TP_USE_COEFF_RELOAD == 1) an array of coefficients of TT_COEFF type.
     *
     **/
    port_conditional_array<input, (TP_USE_COEFF_RELOAD == 1), 1> coeff;

    /**
     * The output data from the function.
     * This output is (generated when TP_NUM_OUTPUTS == 2) either a window API of
     * samples of TT_DATA type or stream API (depending on TP_API).
     * Number of output samples is determined by interpolation & decimation factors (if present).
     **/
    port_conditional_array<output, (TP_NUM_OUTPUTS == 2), 1> out2;

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
     **/
    connect<stream, stream>* getInNet(int cascadePosition) { return net[cascadePosition]; };

    /**
     * Access function to get pointer to net of the ``` in2 ``` port,
     * when port is being generated, i.e. when TP_DUAL_IP == 1.
     * Nets only get assigned when streaming interface is being broadcast, i.e.
     * nets only get used when TP_API == 1 and TP_CASC_LEN > 1
     * @param[in] cascadePosition   an index to the kernel's position in the cascade.
     **/
    connect<stream, stream>* getIn2Net(int cascadePosition) { return net2[cascadePosition]; };

    /**
     * @cond NOCOMMENTS
     */
    /**
    * @brief Access function to get Graphs minimum cascade length for a given configuration.
    **/
    static unsigned int getMinCascadeLength() {
        unsigned int minCascLen = CEIL(TP_FIR_LEN, kMaxTapsPerKernel) / kMaxTapsPerKernel;

        return minCascLen;
    };
    /**
     * @endcond
     */

    /**
    * @brief Access function to get kernel's architecture (or first kernel's architecture in a chained configuration).
    **/
    unsigned int getKernelArchs() {
        // return the architecture for first kernel in the design (only one for single kernel designs).
        // First kernel will always be the slowest of the kernels and so it will reflect on the designs performance
        // best.
        return firstKernelClass::get_m_kArch();
    };

    /**
     * Access function to get kernels PoliphaseLaneAlias.
     **/
    unsigned int getPolyphaseLaneAlias() {
        // return PoliphaseLaneAlias
        return firstKernelClass::get_polyphaseLaneAlias();
    };

    /**
     * @brief This is the constructor function for the FIR graph with static coefficients.
     * @param[in] taps   a reference to the std::vector array of taps values of type TT_COEFF.
     **/
    fir_resampler_graph(const std::vector<TT_COEFF>& taps) : net{}, net2{} {
        // create kernels
        create_casc_kernel<TP_CASC_LEN, TT_DATA, TT_COEFF, TP_FIR_LEN, TP_INTERPOLATE_FACTOR, TP_DECIMATE_FACTOR,
                           TP_SHIFT, TP_RND, TP_INPUT_WINDOW_VSIZE, TP_CASC_LEN, TP_USE_COEFF_RELOAD, TP_NUM_OUTPUTS,
                           TP_DUAL_IP, TP_API>::create(m_firKernels, taps);
        create_connections();
    };

    /**
     * @brief This is the constructor function for the FIR graph with reloadable coefficients.
     **/
    fir_resampler_graph() : net{}, net2{} {
        // create kernels
        create_casc_kernel<TP_CASC_LEN, TT_DATA, TT_COEFF, TP_FIR_LEN, TP_INTERPOLATE_FACTOR, TP_DECIMATE_FACTOR,
                           TP_SHIFT, TP_RND, TP_INPUT_WINDOW_VSIZE, TP_CASC_LEN, TP_USE_COEFF_RELOAD, TP_NUM_OUTPUTS,
                           TP_DUAL_IP, TP_API>::create(m_firKernels);
        create_connections();
    };
};
}
}
}
}
} // namespace braces
#endif //_DSPLIB_FIR_RESAMPLER_GRAPH_HPP_
