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
#ifndef _DSPLIB_FIR_INTERPOLATE_FRACT_ASYM_GRAPH_HPP_
#define _DSPLIB_FIR_INTERPOLATE_FRACT_ASYM_GRAPH_HPP_

// This file holds the definition of the Asymmetric Fractional Interpolator FIR graph class
/**
 * @file fir_interpolate_fract_asym_graph.hpp
 *
 **/

#include <adf.h>
#include <vector>
#include "fir_graph_utils.hpp"

#include "fir_resampler.hpp"
namespace xf {
namespace dsp {
namespace aie {
namespace fir {
namespace interpolate_fract_asym {

using namespace adf;
using namespace resampler;

void deprecation_warning() {
    printf(
        "\nWARNING: The FIR fractional interpolator library element has been superseded by the FIR resampler. Please "
        "use the resampler. This library element will be deprecated and removed in the 2022.2 release timeframe.\n");
}
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
          unsigned int TP_USE_COEFF_RELOAD = 0>
class create_casc_kernel_recur {
   public:
    static void create(kernel (&firKernels)[TP_CASC_LEN], const std::vector<TT_COEFF>& taps) {
        firKernels[dim - 1] = kernel::create_object<fir_resampler<
            TT_DATA, TT_COEFF, TP_FIR_LEN, TP_INTERPOLATE_FACTOR, TP_DECIMATE_FACTOR, TP_SHIFT, TP_RND,
            TP_INPUT_WINDOW_VSIZE, true, true,
            fnFirRange<CEIL(TP_FIR_LEN, TP_INTERPOLATE_FACTOR), TP_CASC_LEN, dim - 1, TP_INTERPOLATE_FACTOR>(), dim - 1,
            TP_CASC_LEN, TP_USE_COEFF_RELOAD, 1> >(taps);
        create_casc_kernel_recur<dim - 1, TT_DATA, TT_COEFF, TP_FIR_LEN, TP_INTERPOLATE_FACTOR, TP_DECIMATE_FACTOR,
                                 TP_SHIFT, TP_RND, TP_INPUT_WINDOW_VSIZE, TP_CASC_LEN,
                                 TP_USE_COEFF_RELOAD>::create(firKernels, taps);
        printf("Created %d\n", dim);
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
          unsigned int TP_CASC_LEN>
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
                               USE_COEFF_RELOAD_FALSE> {
   public:
    static void create(kernel (&firKernels)[TP_CASC_LEN], const std::vector<TT_COEFF>& taps) {
        firKernels[0] = kernel::create_object<
            fir_resampler<TT_DATA, TT_COEFF, TP_FIR_LEN, TP_INTERPOLATE_FACTOR, TP_DECIMATE_FACTOR, TP_SHIFT, TP_RND,
                          TP_INPUT_WINDOW_VSIZE, false, true,
                          fnFirRange<CEIL(TP_FIR_LEN, TP_INTERPOLATE_FACTOR), TP_CASC_LEN, 0, TP_INTERPOLATE_FACTOR>(),
                          0, TP_CASC_LEN, USE_COEFF_RELOAD_FALSE, 1> >(taps);
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
          unsigned int TP_CASC_LEN>
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
                               USE_COEFF_RELOAD_TRUE> {
   public:
    static void create(kernel (&firKernels)[TP_CASC_LEN]) {
        firKernels[dim - 1] = kernel::create_object<fir_resampler<
            TT_DATA, TT_COEFF, TP_FIR_LEN, TP_INTERPOLATE_FACTOR, TP_DECIMATE_FACTOR, TP_SHIFT, TP_RND,
            TP_INPUT_WINDOW_VSIZE, true, true,
            fnFirRange<CEIL(TP_FIR_LEN, TP_INTERPOLATE_FACTOR), TP_CASC_LEN, dim - 1, TP_INTERPOLATE_FACTOR>(), dim - 1,
            TP_CASC_LEN, USE_COEFF_RELOAD_TRUE, 1> >();
        create_casc_kernel_recur<dim - 1, TT_DATA, TT_COEFF, TP_FIR_LEN, TP_INTERPOLATE_FACTOR, TP_DECIMATE_FACTOR,
                                 TP_SHIFT, TP_RND, TP_INPUT_WINDOW_VSIZE, TP_CASC_LEN,
                                 USE_COEFF_RELOAD_TRUE>::create(firKernels);
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
          unsigned int TP_CASC_LEN>
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
                               USE_COEFF_RELOAD_TRUE> {
   public:
    static void create(kernel (&firKernels)[TP_CASC_LEN]) {
        firKernels[0] = kernel::create_object<
            fir_resampler<TT_DATA, TT_COEFF, TP_FIR_LEN, TP_INTERPOLATE_FACTOR, TP_DECIMATE_FACTOR, TP_SHIFT, TP_RND,
                          TP_INPUT_WINDOW_VSIZE, false, true,
                          fnFirRange<CEIL(TP_FIR_LEN, TP_INTERPOLATE_FACTOR), TP_CASC_LEN, 0, TP_INTERPOLATE_FACTOR>(),
                          0, TP_CASC_LEN, USE_COEFF_RELOAD_TRUE, 1> >();
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
          unsigned int TP_NUM_OUTPUTS = 1>
class create_casc_kernel {
   public:
    static void create(kernel (&firKernels)[TP_CASC_LEN], const std::vector<TT_COEFF>& taps) {
        firKernels[dim - 1] = kernel::create_object<fir_resampler<
            TT_DATA, TT_COEFF, TP_FIR_LEN, TP_INTERPOLATE_FACTOR, TP_DECIMATE_FACTOR, TP_SHIFT, TP_RND,
            TP_INPUT_WINDOW_VSIZE, true, false,
            fnFirRangeRem<CEIL(TP_FIR_LEN, TP_INTERPOLATE_FACTOR), TP_CASC_LEN, dim - 1, TP_INTERPOLATE_FACTOR>(),
            dim - 1, TP_CASC_LEN, TP_USE_COEFF_RELOAD, TP_NUM_OUTPUTS> >(taps);
        create_casc_kernel_recur<dim - 1, TT_DATA, TT_COEFF, TP_FIR_LEN, TP_INTERPOLATE_FACTOR, TP_DECIMATE_FACTOR,
                                 TP_SHIFT, TP_RND, TP_INPUT_WINDOW_VSIZE, TP_CASC_LEN,
                                 TP_USE_COEFF_RELOAD>::create(firKernels, taps);
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
          unsigned int TP_NUM_OUTPUTS>
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
                         TP_NUM_OUTPUTS> {
   public:
    static void create(kernel (&firKernels)[TP_CASC_LEN]) {
        firKernels[dim - 1] = kernel::create_object<fir_resampler<
            TT_DATA, TT_COEFF, TP_FIR_LEN, TP_INTERPOLATE_FACTOR, TP_DECIMATE_FACTOR, TP_SHIFT, TP_RND,
            TP_INPUT_WINDOW_VSIZE, true, false,
            fnFirRangeRem<CEIL(TP_FIR_LEN, TP_INTERPOLATE_FACTOR), TP_CASC_LEN, dim - 1, TP_INTERPOLATE_FACTOR>(),
            dim - 1, TP_CASC_LEN, USE_COEFF_RELOAD_TRUE, TP_NUM_OUTPUTS> >();
        create_casc_kernel_recur<dim - 1, TT_DATA, TT_COEFF, TP_FIR_LEN, TP_INTERPOLATE_FACTOR, TP_DECIMATE_FACTOR,
                                 TP_SHIFT, TP_RND, TP_INPUT_WINDOW_VSIZE, TP_CASC_LEN,
                                 USE_COEFF_RELOAD_TRUE>::create(firKernels);
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
          unsigned int TP_NUM_OUTPUTS>
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
                         TP_NUM_OUTPUTS> {
   public:
    static constexpr unsigned int dim = 1;
    static constexpr unsigned int TP_CASC_LEN = 1;
    static void create(kernel (&firKernels)[TP_CASC_LEN], const std::vector<TT_COEFF>& taps) {
        firKernels[dim - 1] = kernel::create_object<fir_resampler<
            TT_DATA, TT_COEFF, TP_FIR_LEN, TP_INTERPOLATE_FACTOR, TP_DECIMATE_FACTOR, TP_SHIFT, TP_RND,
            TP_INPUT_WINDOW_VSIZE, false, false,
            fnFirRangeRem<CEIL(TP_FIR_LEN, TP_INTERPOLATE_FACTOR), TP_CASC_LEN, dim - 1, TP_INTERPOLATE_FACTOR>(),
            dim - 1, TP_CASC_LEN, USE_COEFF_RELOAD_FALSE, TP_NUM_OUTPUTS> >(taps);
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
          unsigned int TP_NUM_OUTPUTS>
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
                         TP_NUM_OUTPUTS> {
   public:
    static constexpr unsigned int dim = 1;
    static constexpr unsigned int TP_CASC_LEN = 1;
    static void create(kernel (&firKernels)[TP_CASC_LEN]) {
        firKernels[dim - 1] = kernel::create_object<fir_resampler<
            TT_DATA, TT_COEFF, TP_FIR_LEN, TP_INTERPOLATE_FACTOR, TP_DECIMATE_FACTOR, TP_SHIFT, TP_RND,
            TP_INPUT_WINDOW_VSIZE, false, false,
            fnFirRangeRem<CEIL(TP_FIR_LEN, TP_INTERPOLATE_FACTOR), TP_CASC_LEN, dim - 1, TP_INTERPOLATE_FACTOR>(),
            dim - 1, TP_CASC_LEN, USE_COEFF_RELOAD_TRUE, TP_NUM_OUTPUTS> >();
    }
};
/**
  * @endcond
  */

/**
 * @brief fir_resampler is an Asymmetric Fractional Interpolation FIR filter
 *
 * These are the templates to configure the Asymmetric Fractional Interpolation FIR class.
 * @tparam TT_DATA describes the type of individual data samples input to and
 *         output from the filter function. \n
 *         This is a typename and must be one of the following: \n
 *         int16, cint16, int32, cint32, float, cfloat.
 * @tparam TT_COEFF describes the type of individual coefficients of the filter
 *         taps. \n It must be one of the same set of types listed for TT_DATA
 *         and must also satisfy the following rules:
 *         - Complex types are only supported when ``TT_DATA`` is also complex.
 *         - 32 bit types are only supported when TT_DATA is also a 32 bit type,
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
 *         range 2 to 16. The decimation factor should be less that the interpolation
 *         factor and should not be divisible factor of the interpolation factor.
 * @tparam TP_SHIFT describes power of 2 shift down applied to the accumulation of
 *         FIR terms before output. \n ``TP_SHIFT`` must be in the range 0 to 59 (61 for AIE1).
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
 *         multiplied by TP_INTERPOLATE_FACTOR and divided by TP_DECIMATE_FACTOR. \n
 *         In the instance this would lead to a fraction number of output samples,
 *         this would be rounded down. \n
 *         \n
 *         Note: Margin size should not be included in TP_INPUT_WINDOW_VSIZE.
 * @tparam TP_CASC_LEN describes the number of AIE processors to split the operation
 *         over. \n This allows resource to be traded for higher performance.
 *         TP_CASC_LEN must be in the range 1 (default) to 9.
 * @tparam TP_USE_COEFF_RELOAD allows the user to select if runtime coefficient
 *         reloading should be used. \n When defining the parameter:
 *         - 0 = static coefficients, defined in filter constructor,
 *         - 1 = reloadable coefficients, passed as argument to runtime function. \n
 *         \n
 *         Note: when used, optional port: ``` port<input> coeff;``` will be added to the FIR. \n
 * @tparam TP_NUM_OUTPUTS sets the number of ports to broadcast the output to. \n
 *         \n
 *         Note: when used, optional port: ``` port<output> out2; ``` will be added to the FIR. \n
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
          unsigned int TP_NUM_OUTPUTS = 1>
class fir_interpolate_fract_asym_graph : public graph {
   private:
    static_assert(TP_CASC_LEN < 10, "ERROR: Unsupported Cascade length");

   public:
    /**
     * The input data to the function. This input is a window API of
     * samples of TT_DATA type. The number of samples in the window is
     * described by TP_INPUT_WINDOW_VSIZE.
     * Note: Margin is added internally to the graph, when connecting input port
     * with kernel port. Therefore, margin should not be added when connecting
     * graph to a higher level design unit.
     * Margin size (in Bytes) equals to TP_FIR_LEN rounded up to a nearest
     * multiple of 32 bytes.
     **/
    port<input> in;
    /**
     * A window API of TP_INPUT_WINDOW_VSIZE*TP_INTERPOLATE_FACTOR/TP_DECIMATE_FACTOR samples of TT_DATA type.
     **/
    port<output> out;
    /**
      * @cond NOCOMMENTS
      */
    kernel m_firKernels[TP_CASC_LEN];
    /**
      * @endcond
      */

    /**
     * Access function to get pointer to kernel (or first kernel in a chained configuration).
     **/

    kernel* getKernels() { return m_firKernels; };
    unsigned int getKernelArchs() {
        constexpr unsigned int firRange =
            (TP_CASC_LEN == 1)
                ? TP_FIR_LEN
                : fnFirRange<CEIL(TP_FIR_LEN, TP_INTERPOLATE_FACTOR), TP_CASC_LEN, 0, TP_INTERPOLATE_FACTOR>();
        // return the architecture for first kernel in the design (only one for single kernel designs).
        // First kernel will always be the slowest of the kernels and so it will reflect on the designs performance
        // best.
        return fir_resampler<TT_DATA, TT_COEFF, TP_FIR_LEN, TP_INTERPOLATE_FACTOR, TP_DECIMATE_FACTOR, TP_SHIFT, TP_RND,
                             TP_INPUT_WINDOW_VSIZE, false, true, firRange, 0, TP_CASC_LEN, TP_USE_COEFF_RELOAD,
                             TP_NUM_OUTPUTS>::get_m_kArch();
    };

    // constructor
    /**
     * @brief This is the constructor function for the asymmetrical decimator FIR graph with static coefficients.
     * @param[in] taps   a reference to the std::vector array of taps values of type TT_COEFF.
     **/
    fir_interpolate_fract_asym_graph(const std::vector<TT_COEFF>& taps) {
        deprecation_warning();
        // create kernels
        create_casc_kernel<TP_CASC_LEN, TT_DATA, TT_COEFF, TP_FIR_LEN, TP_INTERPOLATE_FACTOR, TP_DECIMATE_FACTOR,
                           TP_SHIFT, TP_RND, TP_INPUT_WINDOW_VSIZE, TP_CASC_LEN>::create(m_firKernels, taps);

        // make input connections
        // TODO: verify margin matches kernel class
        connect<window<TP_INPUT_WINDOW_VSIZE * sizeof(TT_DATA),
                       fnFirMargin<((TP_FIR_LEN + TP_INTERPOLATE_FACTOR - 1) / TP_INTERPOLATE_FACTOR), TT_DATA>() *
                           sizeof(TT_DATA)> >(in, m_firKernels[0].in[0]);
        for (int i = 1; i < TP_CASC_LEN; i++) {
            single_buffer(m_firKernels[i].in[0]);
            connect<window<TP_INPUT_WINDOW_VSIZE * sizeof(TT_DATA) +
                           fnFirMargin<((TP_FIR_LEN + TP_INTERPOLATE_FACTOR - 1) / TP_INTERPOLATE_FACTOR), TT_DATA>() *
                               sizeof(TT_DATA)> >(async(m_firKernels[i - 1].out[1]), async(m_firKernels[i].in[0]));
        }
        // make cascade connections
        for (int i = 1; i < TP_CASC_LEN; i++) {
            connect<cascade>(m_firKernels[i - 1].out[0], m_firKernels[i].in[1]);
        }

        // make output connections
        connect<window<TP_INTERPOLATE_FACTOR * TP_INPUT_WINDOW_VSIZE * sizeof(TT_DATA) / TP_DECIMATE_FACTOR> >(
            m_firKernels[TP_CASC_LEN - 1].out[0], out);

        for (int i = 0; i < TP_CASC_LEN; i++) {
            // Specify mapping constraints
            runtime<ratio>(m_firKernels[i]) = 0.8;
            // Source files
            source(m_firKernels[i]) = "fir_resampler.cpp";
        }
    }
};

/**
  * @cond NOCOMMENTS
  */
// Specialized template  using static coefficients, dual output
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_INTERPOLATE_FACTOR,
          unsigned int TP_DECIMATE_FACTOR,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_CASC_LEN>
class fir_interpolate_fract_asym_graph<TT_DATA,
                                       TT_COEFF,
                                       TP_FIR_LEN,
                                       TP_INTERPOLATE_FACTOR,
                                       TP_DECIMATE_FACTOR,
                                       TP_SHIFT,
                                       TP_RND,
                                       TP_INPUT_WINDOW_VSIZE,
                                       TP_CASC_LEN,
                                       USE_COEFF_RELOAD_FALSE,
                                       2> : public graph {
   private:
    static_assert(TP_CASC_LEN < 10, "ERROR: Unsupported Cascade length");

   public:
    port<input> in;
    port<output> out;
    /**
      * @endcond
      */
    /**
     * A second output window API of TP_INPUT_WINDOW_VSIZE*TP_INTERPOLATE_FACTOR/TP_DECIMATE_FACTOR samples of TT_DATA
     *type.
     **/
    port<output> out2;
    /**
      * @cond NOCOMMENTS
      */
    kernel m_firKernels[TP_CASC_LEN];
    // Access function for AIE synthesizer
    kernel* getKernels() { return m_firKernels; };
    unsigned int getKernelArchs() {
        constexpr unsigned int firRange =
            (TP_CASC_LEN == 1)
                ? TP_FIR_LEN
                : fnFirRange<CEIL(TP_FIR_LEN, TP_INTERPOLATE_FACTOR), TP_CASC_LEN, 0, TP_INTERPOLATE_FACTOR>();
        // return the architecture for first kernel in the design (only one for single kernel designs).
        // First kernel will always be the slowest of the kernels and so it will reflect on the designs performance
        // best.
        return fir_resampler<TT_DATA, TT_COEFF, TP_FIR_LEN, TP_INTERPOLATE_FACTOR, TP_DECIMATE_FACTOR, TP_SHIFT, TP_RND,
                             TP_INPUT_WINDOW_VSIZE, false, true, firRange, 0, TP_CASC_LEN, USE_COEFF_RELOAD_FALSE,
                             2>::get_m_kArch();
    };

    fir_interpolate_fract_asym_graph(const std::vector<TT_COEFF>& taps) {
        deprecation_warning();
        // create kernels
        create_casc_kernel<TP_CASC_LEN, TT_DATA, TT_COEFF, TP_FIR_LEN, TP_INTERPOLATE_FACTOR, TP_DECIMATE_FACTOR,
                           TP_SHIFT, TP_RND, TP_INPUT_WINDOW_VSIZE, TP_CASC_LEN, USE_COEFF_RELOAD_FALSE,
                           2>::create(m_firKernels, taps);

        // make input connections
        connect<window<TP_INPUT_WINDOW_VSIZE * sizeof(TT_DATA),
                       fnFirMargin<((TP_FIR_LEN + TP_INTERPOLATE_FACTOR - 1) / TP_INTERPOLATE_FACTOR), TT_DATA>() *
                           sizeof(TT_DATA)> >(in, m_firKernels[0].in[0]);
        for (int i = 1; i < TP_CASC_LEN; i++) {
            single_buffer(m_firKernels[i].in[0]);
            connect<window<TP_INPUT_WINDOW_VSIZE * sizeof(TT_DATA) +
                           fnFirMargin<((TP_FIR_LEN + TP_INTERPOLATE_FACTOR - 1) / TP_INTERPOLATE_FACTOR), TT_DATA>() *
                               sizeof(TT_DATA)> >(async(m_firKernels[i - 1].out[1]), async(m_firKernels[i].in[0]));
        }

        // make cascade connections
        for (int i = 1; i < TP_CASC_LEN; i++) {
            connect<cascade>(m_firKernels[i - 1].out[0], m_firKernels[i].in[1]);
        }

        // make output connections
        connect<window<TP_INTERPOLATE_FACTOR * TP_INPUT_WINDOW_VSIZE * sizeof(TT_DATA) / TP_DECIMATE_FACTOR> >(
            m_firKernels[TP_CASC_LEN - 1].out[0], out);
        connect<window<TP_INTERPOLATE_FACTOR * TP_INPUT_WINDOW_VSIZE * sizeof(TT_DATA) / TP_DECIMATE_FACTOR> >(
            m_firKernels[TP_CASC_LEN - 1].out[1], out2);

        for (int i = 0; i < TP_CASC_LEN; i++) {
            // Specify mapping constraints
            runtime<ratio>(m_firKernels[i]) = 0.8;
            // Source files
            source(m_firKernels[i]) = "fir_resampler.cpp";
        }
    }
};

// Specialized template  using runtime reloadable coefficients, single output
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_INTERPOLATE_FACTOR,
          unsigned int TP_DECIMATE_FACTOR,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_CASC_LEN>
class fir_interpolate_fract_asym_graph<TT_DATA,
                                       TT_COEFF,
                                       TP_FIR_LEN,
                                       TP_INTERPOLATE_FACTOR,
                                       TP_DECIMATE_FACTOR,
                                       TP_SHIFT,
                                       TP_RND,
                                       TP_INPUT_WINDOW_VSIZE,
                                       TP_CASC_LEN,
                                       USE_COEFF_RELOAD_TRUE,
                                       1> : public graph {
   public:
    port<input> in;
    port<output> out;
    port<input> coeff;

    kernel m_firKernels[TP_CASC_LEN];
    // Access function for AIE synthesizer
    kernel* getKernels() { return m_firKernels; };
    unsigned int getKernelArchs() {
        constexpr unsigned int firRange =
            (TP_CASC_LEN == 1)
                ? TP_FIR_LEN
                : fnFirRange<CEIL(TP_FIR_LEN, TP_INTERPOLATE_FACTOR), TP_CASC_LEN, 0, TP_INTERPOLATE_FACTOR>();
        // return the architecture for first kernel in the design (only one for single kernel designs).
        // First kernel will always be the slowest of the kernels and so it will reflect on the designs performance
        // best.
        return fir_resampler<TT_DATA, TT_COEFF, TP_FIR_LEN, TP_INTERPOLATE_FACTOR, TP_DECIMATE_FACTOR, TP_SHIFT, TP_RND,
                             TP_INPUT_WINDOW_VSIZE, false, true, firRange, 0, TP_CASC_LEN, USE_COEFF_RELOAD_TRUE,
                             1>::get_m_kArch();
    };

    // constructor
    fir_interpolate_fract_asym_graph() {
        deprecation_warning();
        // create kernels
        create_casc_kernel<TP_CASC_LEN, TT_DATA, TT_COEFF, TP_FIR_LEN, TP_INTERPOLATE_FACTOR, TP_DECIMATE_FACTOR,
                           TP_SHIFT, TP_RND, TP_INPUT_WINDOW_VSIZE, TP_CASC_LEN, USE_COEFF_RELOAD_TRUE,
                           1>::create(m_firKernels);

        // make input connections
        connect<window<TP_INPUT_WINDOW_VSIZE * sizeof(TT_DATA),
                       fnFirMargin<((TP_FIR_LEN + TP_INTERPOLATE_FACTOR - 1) / TP_INTERPOLATE_FACTOR), TT_DATA>() *
                           sizeof(TT_DATA)> >(in, m_firKernels[0].in[0]);
        for (int i = 1; i < TP_CASC_LEN; i++) {
            single_buffer(m_firKernels[i].in[0]);
            connect<window<TP_INPUT_WINDOW_VSIZE * sizeof(TT_DATA) +
                           fnFirMargin<((TP_FIR_LEN + TP_INTERPOLATE_FACTOR - 1) / TP_INTERPOLATE_FACTOR), TT_DATA>() *
                               sizeof(TT_DATA)> >(async(m_firKernels[i - 1].out[1]), async(m_firKernels[i].in[0]));
        }

        // make cascade connections
        for (int i = 1; i < TP_CASC_LEN; i++) {
            connect<cascade>(m_firKernels[i - 1].out[0], m_firKernels[i].in[1]);
        }

        // coefficient connections between reload kernels
        connect<parameter>(async(coeff), async(m_firKernels[0].in[1]));

        // make output connections
        connect<window<TP_INTERPOLATE_FACTOR * TP_INPUT_WINDOW_VSIZE * sizeof(TT_DATA) / TP_DECIMATE_FACTOR> >(
            m_firKernels[TP_CASC_LEN - 1].out[0], out);

        for (int i = 0; i < TP_CASC_LEN; i++) {
            // Specify mapping constraints
            runtime<ratio>(m_firKernels[i]) = 0.8;
            // Source files
            source(m_firKernels[i]) = "fir_resampler.cpp";
        }
    }
};

// Specialized template  using runtime reloadable coefficients, dual output
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_INTERPOLATE_FACTOR,
          unsigned int TP_DECIMATE_FACTOR,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_CASC_LEN>
class fir_interpolate_fract_asym_graph<TT_DATA,
                                       TT_COEFF,
                                       TP_FIR_LEN,
                                       TP_INTERPOLATE_FACTOR,
                                       TP_DECIMATE_FACTOR,
                                       TP_SHIFT,
                                       TP_RND,
                                       TP_INPUT_WINDOW_VSIZE,
                                       TP_CASC_LEN,
                                       USE_COEFF_RELOAD_TRUE,
                                       2> : public graph {
   public:
    port<input> in;
    port<output> out;
    port<output> out2;
    port<input> coeff;

    kernel m_firKernels[TP_CASC_LEN];
    // Access function for AIE synthesizer
    kernel* getKernels() { return m_firKernels; };
    unsigned int getKernelArchs() {
        constexpr unsigned int firRange =
            (TP_CASC_LEN == 1)
                ? TP_FIR_LEN
                : fnFirRange<CEIL(TP_FIR_LEN, TP_INTERPOLATE_FACTOR), TP_CASC_LEN, 0, TP_INTERPOLATE_FACTOR>();
        // return the architecture for first kernel in the design (only one for single kernel designs).
        // First kernel will always be the slowest of the kernels and so it will reflect on the designs performance
        // best.
        return fir_resampler<TT_DATA, TT_COEFF, TP_FIR_LEN, TP_INTERPOLATE_FACTOR, TP_DECIMATE_FACTOR, TP_SHIFT, TP_RND,
                             TP_INPUT_WINDOW_VSIZE, false, true, firRange, 0, TP_CASC_LEN, USE_COEFF_RELOAD_TRUE,
                             2>::get_m_kArch();
    };

    // constructor
    fir_interpolate_fract_asym_graph() {
        deprecation_warning();
        // create kernels
        create_casc_kernel<TP_CASC_LEN, TT_DATA, TT_COEFF, TP_FIR_LEN, TP_INTERPOLATE_FACTOR, TP_DECIMATE_FACTOR,
                           TP_SHIFT, TP_RND, TP_INPUT_WINDOW_VSIZE, TP_CASC_LEN, USE_COEFF_RELOAD_TRUE,
                           2>::create(m_firKernels);

        // make input connections
        connect<window<TP_INPUT_WINDOW_VSIZE * sizeof(TT_DATA),
                       fnFirMargin<((TP_FIR_LEN + TP_INTERPOLATE_FACTOR - 1) / TP_INTERPOLATE_FACTOR), TT_DATA>() *
                           sizeof(TT_DATA)> >(in, m_firKernels[0].in[0]);
        for (int i = 1; i < TP_CASC_LEN; i++) {
            single_buffer(m_firKernels[i].in[0]);
            connect<window<TP_INPUT_WINDOW_VSIZE * sizeof(TT_DATA) +
                           fnFirMargin<((TP_FIR_LEN + TP_INTERPOLATE_FACTOR - 1) / TP_INTERPOLATE_FACTOR), TT_DATA>() *
                               sizeof(TT_DATA)> >(async(m_firKernels[i - 1].out[1]), async(m_firKernels[i].in[0]));
        }

        // make cascade connections
        for (int i = 1; i < TP_CASC_LEN; i++) {
            connect<cascade>(m_firKernels[i - 1].out[0], m_firKernels[i].in[1]);
        }

        // coefficient connections between reload kernels
        connect<parameter>(async(coeff), async(m_firKernels[0].in[1]));

        // make output connections
        connect<window<TP_INTERPOLATE_FACTOR * TP_INPUT_WINDOW_VSIZE * sizeof(TT_DATA) / TP_DECIMATE_FACTOR> >(
            m_firKernels[TP_CASC_LEN - 1].out[0], out);
        connect<window<TP_INTERPOLATE_FACTOR * TP_INPUT_WINDOW_VSIZE * sizeof(TT_DATA) / TP_DECIMATE_FACTOR> >(
            m_firKernels[TP_CASC_LEN - 1].out[1], out2);

        for (int i = 0; i < TP_CASC_LEN; i++) {
            // Specify mapping constraints
            runtime<ratio>(m_firKernels[i]) = 0.8;
            // Source files
            source(m_firKernels[i]) = "fir_resampler.cpp";
        }
    }
};

/**
  * @endcond
  */
}
}
}
}
}

#endif // _DSPLIB_FIR_INTERPOLATE_FRACT_ASYM_GRAPH_HPP_
