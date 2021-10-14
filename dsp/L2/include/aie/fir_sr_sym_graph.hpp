/*
 * Copyright 2021 Xilinx, Inc.
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
#ifndef _DSPLIB_FIR_SR_SYM_GRAPH_HPP_
#define _DSPLIB_FIR_SR_SYM_GRAPH_HPP_
/*
The file captures the definition of the 'L2' graph level class for
the Single Rate Symmetrical FIR library element.
*/
/**
 * @file fir_sr_sym_graph.hpp
 *
 **/

#include <adf.h>
#include <vector>
#include "fir_sr_sym.hpp"

using namespace adf;

namespace xf {
namespace dsp {
namespace aie {
namespace fir {
namespace sr_sym {
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
// Recursive kernel creation, static/reloadable coefficients
template <int dim,
          typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_CASC_LEN,
          unsigned int TP_USE_COEFF_RELOAD = 0,
          unsigned int TP_API = 0>
class create_casc_kernel_recur {
   public:
    static void create(kernel (&firKernels)[TP_CASC_LEN], const std::vector<TT_COEFF>& taps) {
        firKernels[dim - 1] =
            kernel::create_object<fir_sr_sym<TT_DATA, TT_COEFF, TP_FIR_LEN, TP_SHIFT, TP_RND, TP_INPUT_WINDOW_VSIZE,
                                             true, true, fnFirRangeSym<TP_FIR_LEN, TP_CASC_LEN, dim - 1>(), dim - 1,
                                             TP_CASC_LEN, TP_USE_COEFF_RELOAD, 1, TP_API> >(taps);
        create_casc_kernel_recur<dim - 1, TT_DATA, TT_COEFF, TP_FIR_LEN, TP_SHIFT, TP_RND, TP_INPUT_WINDOW_VSIZE,
                                 TP_CASC_LEN, TP_USE_COEFF_RELOAD, TP_API>::create(firKernels, taps);
    }
};
// Recursive kernel creation, static coefficients
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_CASC_LEN,
          unsigned int TP_USE_COEFF_RELOAD,
          unsigned int TP_API>
class create_casc_kernel_recur<1,
                               TT_DATA,
                               TT_COEFF,
                               TP_FIR_LEN,
                               TP_SHIFT,
                               TP_RND,
                               TP_INPUT_WINDOW_VSIZE,
                               TP_CASC_LEN,
                               TP_USE_COEFF_RELOAD,
                               TP_API> {
   public:
    static void create(kernel (&firKernels)[TP_CASC_LEN], const std::vector<TT_COEFF>& taps) {
        firKernels[0] = kernel::create_object<
            fir_sr_sym<TT_DATA, TT_COEFF, TP_FIR_LEN, TP_SHIFT, TP_RND, TP_INPUT_WINDOW_VSIZE, false, true,
                       fnFirRangeSym<TP_FIR_LEN, TP_CASC_LEN, 0>(), 0, TP_CASC_LEN, TP_USE_COEFF_RELOAD, 1, TP_API> >(
            taps);
    }
};
// Recursive kernel creation, mid-cascade, reloadable coefficients,
template <int dim,
          typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_CASC_LEN,
          unsigned int TP_API>
class create_casc_kernel_recur<dim,
                               TT_DATA,
                               TT_COEFF,
                               TP_FIR_LEN,
                               TP_SHIFT,
                               TP_RND,
                               TP_INPUT_WINDOW_VSIZE,
                               TP_CASC_LEN,
                               USE_COEFF_RELOAD_TRUE,
                               TP_API> {
   public:
    static void create(kernel (&firKernels)[TP_CASC_LEN]) {
        firKernels[dim - 1] =
            kernel::create_object<fir_sr_sym<TT_DATA, TT_COEFF, TP_FIR_LEN, TP_SHIFT, TP_RND, TP_INPUT_WINDOW_VSIZE,
                                             true, true, fnFirRangeSym<TP_FIR_LEN, TP_CASC_LEN, dim - 1>(), dim - 1,
                                             TP_CASC_LEN, USE_COEFF_RELOAD_TRUE, 1, TP_API> >();
        create_casc_kernel_recur<dim - 1, TT_DATA, TT_COEFF, TP_FIR_LEN, TP_SHIFT, TP_RND, TP_INPUT_WINDOW_VSIZE,
                                 TP_CASC_LEN, USE_COEFF_RELOAD_TRUE, TP_API>::create(firKernels);
    }
};
// Recursive kernel creation, first kernel in cascade, reloadable coefficients
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_CASC_LEN,
          unsigned int TP_API>
class create_casc_kernel_recur<1,
                               TT_DATA,
                               TT_COEFF,
                               TP_FIR_LEN,
                               TP_SHIFT,
                               TP_RND,
                               TP_INPUT_WINDOW_VSIZE,
                               TP_CASC_LEN,
                               USE_COEFF_RELOAD_TRUE,
                               TP_API> {
   public:
    static void create(kernel (&firKernels)[TP_CASC_LEN]) {
        firKernels[0] =
            kernel::create_object<fir_sr_sym<TT_DATA, TT_COEFF, TP_FIR_LEN, TP_SHIFT, TP_RND, TP_INPUT_WINDOW_VSIZE,
                                             false, true, fnFirRangeSym<TP_FIR_LEN, TP_CASC_LEN, 0>(), 0, TP_CASC_LEN,
                                             USE_COEFF_RELOAD_TRUE, 1, TP_API> >();
    }
};
// Kernel creation, last kernel, static/reloadable coefficients
template <int dim,
          typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_CASC_LEN,
          unsigned int TP_USE_COEFF_RELOAD = 0,
          unsigned int TP_NUM_OUTPUTS = 1, // Only the last kernel supports multiple outputs
          unsigned int TP_API = 0>         // All kernels take IN API
class create_casc_kernel {
   public:
    static void create(kernel (&firKernels)[TP_CASC_LEN], const std::vector<TT_COEFF>& taps) {
        firKernels[dim - 1] =
            kernel::create_object<fir_sr_sym<TT_DATA, TT_COEFF, TP_FIR_LEN, TP_SHIFT, TP_RND, TP_INPUT_WINDOW_VSIZE,
                                             true, false, fnFirRangeRemSym<TP_FIR_LEN, TP_CASC_LEN, dim - 1>(), dim - 1,
                                             TP_CASC_LEN, TP_USE_COEFF_RELOAD, TP_NUM_OUTPUTS, TP_API> >(taps);
        create_casc_kernel_recur<dim - 1, TT_DATA, TT_COEFF, TP_FIR_LEN, TP_SHIFT, TP_RND, TP_INPUT_WINDOW_VSIZE,
                                 TP_CASC_LEN, TP_USE_COEFF_RELOAD, TP_API>::create(firKernels, taps);
    }
};
// Kernel creation, last kernel, reloadable coefficients
template <int dim,
          typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_CASC_LEN,
          unsigned int TP_NUM_OUTPUTS,
          unsigned int TP_API>
class create_casc_kernel<dim,
                         TT_DATA,
                         TT_COEFF,
                         TP_FIR_LEN,
                         TP_SHIFT,
                         TP_RND,
                         TP_INPUT_WINDOW_VSIZE,
                         TP_CASC_LEN,
                         USE_COEFF_RELOAD_TRUE,
                         TP_NUM_OUTPUTS,
                         TP_API> {
   public:
    static void create(kernel (&firKernels)[TP_CASC_LEN]) {
        firKernels[dim - 1] =
            kernel::create_object<fir_sr_sym<TT_DATA, TT_COEFF, TP_FIR_LEN, TP_SHIFT, TP_RND, TP_INPUT_WINDOW_VSIZE,
                                             true, false, fnFirRangeRemSym<TP_FIR_LEN, TP_CASC_LEN, dim - 1>(), dim - 1,
                                             TP_CASC_LEN, USE_COEFF_RELOAD_TRUE, TP_NUM_OUTPUTS, TP_API> >();
        create_casc_kernel_recur<dim - 1, TT_DATA, TT_COEFF, TP_FIR_LEN, TP_SHIFT, TP_RND, TP_INPUT_WINDOW_VSIZE,
                                 TP_CASC_LEN, USE_COEFF_RELOAD_TRUE, TP_API>::create(firKernels);
    }
};
// Kernel creation, single kernel, static coefficients
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_USE_COEFF_RELOAD,
          unsigned int TP_NUM_OUTPUTS,
          unsigned int TP_API>
class create_casc_kernel<1,
                         TT_DATA,
                         TT_COEFF,
                         TP_FIR_LEN,
                         TP_SHIFT,
                         TP_RND,
                         TP_INPUT_WINDOW_VSIZE,
                         1,
                         TP_USE_COEFF_RELOAD,
                         TP_NUM_OUTPUTS,
                         TP_API> {
   public:
    static constexpr unsigned int dim = 1;
    static constexpr unsigned int TP_CASC_LEN = 1;
    static void create(kernel (&firKernels)[TP_CASC_LEN], const std::vector<TT_COEFF>& taps) {
        firKernels[dim - 1] =
            kernel::create_object<fir_sr_sym<TT_DATA, TT_COEFF, TP_FIR_LEN, TP_SHIFT, TP_RND, TP_INPUT_WINDOW_VSIZE,
                                             false, false, fnFirRangeRemSym<TP_FIR_LEN, TP_CASC_LEN, dim - 1>(),
                                             dim - 1, TP_CASC_LEN, TP_USE_COEFF_RELOAD, TP_NUM_OUTPUTS, TP_API> >(taps);
    }
};
// Kernel creation, single kernel, reloadable coefficients
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_NUM_OUTPUTS,
          unsigned int TP_API>
class create_casc_kernel<1,
                         TT_DATA,
                         TT_COEFF,
                         TP_FIR_LEN,
                         TP_SHIFT,
                         TP_RND,
                         TP_INPUT_WINDOW_VSIZE,
                         1,
                         USE_COEFF_RELOAD_TRUE,
                         TP_NUM_OUTPUTS,
                         TP_API> {
   public:
    static constexpr unsigned int dim = 1;
    static constexpr unsigned int TP_CASC_LEN = 1;
    static void create(kernel (&firKernels)[TP_CASC_LEN]) {
        firKernels[dim - 1] =
            kernel::create_object<fir_sr_sym<TT_DATA, TT_COEFF, TP_FIR_LEN, TP_SHIFT, TP_RND, TP_INPUT_WINDOW_VSIZE,
                                             false, false, fnFirRangeRemSym<TP_FIR_LEN, TP_CASC_LEN, dim - 1>(),
                                             dim - 1, TP_CASC_LEN, USE_COEFF_RELOAD_TRUE, TP_NUM_OUTPUTS, TP_API> >();
    }
};
/**
 * @endcond
 */

template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_CASC_LEN = 1,
          unsigned int TP_USE_COEFF_RELOAD = 0, // 1 = use coeff reload, 0 = don't use coeff reload
          unsigned int TP_NUM_OUTPUTS = 1,
          unsigned int TP_API = 0 // 0 = Window, 1 = Stream
          >
class fir_sr_sym_base_graph : virtual public graph {
   private:
    static_assert(TP_CASC_LEN < 10, "ERROR: Unsupported Cascade length");

   public:
    /**
     * @brief The input data to the function. This input is a window API of
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
     * @brief A window API of TP_INPUT_WINDOW_VSIZE samples of TT_DATA type.
     **/
    port<output> out;

    kernel m_firKernels[TP_CASC_LEN];
    /**
     * @brief Access function to get pointer to kernel (or first kernel in a chained configuration).
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
        return fir_sr_sym<TT_DATA, TT_COEFF, TP_FIR_LEN, TP_SHIFT, TP_RND, TP_INPUT_WINDOW_VSIZE, false, true, firRange,
                          0, TP_CASC_LEN, TP_USE_COEFF_RELOAD, TP_NUM_OUTPUTS, TP_API>::get_m_kArch();
    };

    fir_sr_sym_base_graph(const std::vector<TT_COEFF>& taps) {
        printf("== class fir_sr_sym_base_graph : ");
        create_casc_kernel<TP_CASC_LEN, TT_DATA, TT_COEFF, TP_FIR_LEN, TP_SHIFT, TP_RND, TP_INPUT_WINDOW_VSIZE,
                           TP_CASC_LEN, USE_COEFF_RELOAD_FALSE, TP_NUM_OUTPUTS, TP_API>::create(m_firKernels, taps);
        fir_sr_sym_base_connections();
    };
    fir_sr_sym_base_graph() {
        printf("== class fir_sr_sym_base_graph : ");
        create_casc_kernel<TP_CASC_LEN, TT_DATA, TT_COEFF, TP_FIR_LEN, TP_SHIFT, TP_RND, TP_INPUT_WINDOW_VSIZE,
                           TP_CASC_LEN, USE_COEFF_RELOAD_TRUE, TP_NUM_OUTPUTS, TP_API>::create(m_firKernels);
        fir_sr_sym_base_connections();
    };

    void fir_sr_sym_base_connections() {
        // make input connections
        connect<window<TP_INPUT_WINDOW_VSIZE * sizeof(TT_DATA), fnFirMargin<TP_FIR_LEN, TT_DATA>() * sizeof(TT_DATA)> >(
            in, m_firKernels[0].in[0]);
        if (TP_API == 0) {
            for (int i = 1; i < TP_CASC_LEN; i++) {
                single_buffer(m_firKernels[i].in[0]);
                connect<window<TP_INPUT_WINDOW_VSIZE * sizeof(TT_DATA) +
                               fnFirMargin<TP_FIR_LEN, TT_DATA>() * sizeof(TT_DATA)> >(
                    async(m_firKernels[i - 1].out[1]), async(m_firKernels[i].in[0]));
            }
        } else {
            for (int i = 1; i < TP_CASC_LEN; i++) {
                connect<window<TP_INPUT_WINDOW_VSIZE * sizeof(TT_DATA),
                               fnFirMargin<TP_FIR_LEN, TT_DATA>() * sizeof(TT_DATA)> >(in, m_firKernels[i].in[0]);
            }
        }

        // make cascade connections
        for (int i = 1; i < TP_CASC_LEN; i++) {
            connect<cascade>(m_firKernels[i - 1].out[0], m_firKernels[i].in[1]);
        }

        // make output connections
        if (TP_API == 0) {
            connect<window<TP_INPUT_WINDOW_VSIZE * sizeof(TT_DATA)> >(m_firKernels[TP_CASC_LEN - 1].out[0], out);
        } else {
            connect<stream>(m_firKernels[TP_CASC_LEN - 1].out[0], out);
        }

        for (int i = 0; i < TP_CASC_LEN; i++) {
            // Specify mapping constraints
            runtime<ratio>(m_firKernels[i]) = 0.8;
            // Source files
            source(m_firKernels[i]) = "fir_sr_sym.cpp";
        }
    }
};

template <unsigned int TP_OUTPUT_WINDOW_BYTESIZE,
          unsigned int TP_CASC_LEN = 1,
          unsigned int TP_NUM_OUTPUTS = 2,
          unsigned int TP_API = 0 // 0 = Window, 1 = Stream
          >
class conditional_out_graph : virtual public graph {
   private:
   public:
    port<output> out2;

    conditional_out_graph() { printf("== add  port<output> out2 \n"); };

    void conditioanl_out_connections(kernel* firKernels) {
        // make output connections
        if (TP_API == 0) {
            connect<window<TP_OUTPUT_WINDOW_BYTESIZE> >(firKernels[TP_CASC_LEN - 1].out[1], out2);
        } else {
            connect<stream>(firKernels[TP_CASC_LEN - 1].out[1], out2);
        }
    }
};

template <unsigned int TP_OUTPUT_WINDOW_BYTESIZE, unsigned int TP_CASC_LEN, unsigned int TP_API>
class conditional_out_graph<TP_OUTPUT_WINDOW_BYTESIZE, TP_CASC_LEN, 1, TP_API> : virtual public graph {
   private:
   public:
    conditional_out_graph(){};

    void conditioanl_out_connections(kernel* firKernels) {
        // Do nothing
    }
};

template <unsigned int TP_USE_RTP = 1, // 1 = use rtp, 0 = don't use rtp
          unsigned int TP_PORT_POS = 1>
class conditioanl_rtp_graph : virtual public graph {
   private:
   public:
    port<input> coeff;

    conditioanl_rtp_graph() { printf("== add  port<input> coeff \n"); };

    void conditioanl_rtp_connections(kernel* firKernels) {
        // make RTP connection
        connect<parameter>(coeff, async(firKernels[0].in[TP_PORT_POS]));
    }
};

template <unsigned int TP_PORT_POS>
class conditioanl_rtp_graph<0, TP_PORT_POS> : virtual public graph {
   private:
   public:
    conditioanl_rtp_graph(){};

    void conditioanl_rtp_connections(kernel* firKernels) {
        // Do nothing
    }
};

//--------------------------------------------------------------------------------------------------
// fir_sr_sym_graph template
//--------------------------------------------------------------------------------------------------
/**
 * @brief fir_sr_sym is a Symmetrical Single Rate FIR filter
 *
 * These are the templates to configure the Symmetric Single Rate FIR class.
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
 *         also by virtue the single rate nature of this function. \n
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
 * @tparam TP_NUM_OUTPUTS sets the number of ports to broadcast the output to. \n
 *         Note: when used, optional port: ``` port<output> out2; ``` will be added to the FIR. \n
 * @tparam TP_API specifies if the output interface should be window-based or stream-based.  \n
 *         The values supported are 0 (window API) or 1 (stream API). \n
 *         Note: due to the data buffering requirement imposed through symmetry, input interface is always set to
 *window. \n
 *         Auto-infeffed DMA stream-to-window conversion is applied when FIR is connected with an input stream.
 **/
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_CASC_LEN = 1,
          unsigned int TP_USE_COEFF_RELOAD = 0, // 1 = use coeff reload, 0 = don't use coeff reload
          unsigned int TP_NUM_OUTPUTS = 1,
          unsigned int TP_API = 0 // 0 = Window, 1 = Stream
          >
/**
 **/
class fir_sr_sym_graph
    : public fir_sr_sym_base_graph<TT_DATA,
                                   TT_COEFF,
                                   TP_FIR_LEN,
                                   TP_SHIFT,
                                   TP_RND,
                                   TP_INPUT_WINDOW_VSIZE,
                                   TP_CASC_LEN,
                                   TP_USE_COEFF_RELOAD,
                                   TP_NUM_OUTPUTS,
                                   TP_API>,
      public conditional_out_graph<TP_INPUT_WINDOW_VSIZE * sizeof(TT_DATA), TP_CASC_LEN, TP_NUM_OUTPUTS, TP_API>,
      public conditioanl_rtp_graph<TP_USE_COEFF_RELOAD> {
    static_assert(TP_CASC_LEN < 10, "ERROR: Unsupported Cascade length");

   public:
    /**
     * @brief This is the constructor function for the Symmetric Singlr Rate FIR graph.
     * Constructor has no arguments.
     * To be used with TP_USE_COEFF_RELOAD=1, taps needs to be passed through RTP
     **/
    fir_sr_sym_graph()
        : fir_sr_sym_base_graph<TT_DATA,
                                TT_COEFF,
                                TP_FIR_LEN,
                                TP_SHIFT,
                                TP_RND,
                                TP_INPUT_WINDOW_VSIZE,
                                TP_CASC_LEN,
                                TP_USE_COEFF_RELOAD,
                                TP_NUM_OUTPUTS,
                                TP_API>() {
        create_connections();
    };
    /**
     * @brief This is the constructor function for the Symmetric Singlr Rate FIR graph.
     * Constructor has the following arguments
     * @arg taps         a reference to the std::vector array of taps values of type TT_COEFF. \n
     *                   The taps array need only be supplied for the first half
     *                   of the filter length plus the center tap for odd lengths i.e. \n
     *                   taps[] = {c0, c1, c2, ..., cN [, cCT]} where \n
     *                   N = TP_FIR_LEN/2 and cCT is the center tap where TP_FIR_LEN
     *                   is odd. \n
     *                   For example, a 7-tap filter might use coeffs
     *                   (1, 3, 2, 5, 2, 3, 1). \n This could be input as
     *                   taps[]= {1,3,2,5} since the context of symmetry allows
     *                   the remaining coefficients to be inferred. \n
     **/
    fir_sr_sym_graph(const std::vector<TT_COEFF>& taps)
        : fir_sr_sym_base_graph<TT_DATA,
                                TT_COEFF,
                                TP_FIR_LEN,
                                TP_SHIFT,
                                TP_RND,
                                TP_INPUT_WINDOW_VSIZE,
                                TP_CASC_LEN,
                                TP_USE_COEFF_RELOAD,
                                TP_NUM_OUTPUTS,
                                TP_API>(taps) {
        create_connections();
    };

    void create_connections() {
        printf("== class fir_sr_sym_graph  \n");
        if (TP_NUM_OUTPUTS == 2) {
            this->conditioanl_out_connections(this->getKernels());
        }
        if (TP_USE_COEFF_RELOAD == USE_COEFF_RELOAD_TRUE) {
            this->conditioanl_rtp_connections(this->getKernels());
        }
    };
    /**
     * @brief Access function to get pointer to kernel (or first kernel in a chained configuration).
     * No arguments required.
     **/

    kernel* getKernels() { return this->m_firKernels; };
};

/**
  * @endcond
  */
}
}
}
}
}

#endif // _DSPLIB_FIR_SR_SYM_GRAPH_HPP_
