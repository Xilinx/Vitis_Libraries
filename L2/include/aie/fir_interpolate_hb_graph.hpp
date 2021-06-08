#ifndef _DSPLIB_FIR_INTERPOLATE_HB_GRAPH_HPP_
#define _DSPLIB_FIR_INTERPOLATE_HB_GRAPH_HPP_

/**
 * @file fir_interpolate_hb_graph.hpp
 * @brief The file captures the definition of the 'L2' graph level class for FIR library element.
 **/

#include <adf.h>
#include <vector>
#include "fir_interpolate_hb.hpp"
#include "fir_utils.hpp"

using namespace adf;

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
          unsigned int TP_UPSHIFT_CT = 0>
class create_casc_kernel_recur {
   private:
    static constexpr unsigned int kDualIpEn =
        0; // cascaded kernels do not support dual inputs, unless input interface is a stream
   public:
    static void create(kernel (&firKernels)[TP_CASC_LEN], const std::vector<TT_COEFF>& taps) {
        firKernels[dim - 1] = kernel::create_object<
            fir_interpolate_hb<TT_DATA, TT_COEFF, TP_FIR_LEN, TP_SHIFT, TP_RND, TP_INPUT_WINDOW_VSIZE, true, true,
                               fnFirRangeSym<TP_FIR_LEN, TP_CASC_LEN, dim - 1>(), dim - 1, TP_CASC_LEN, kDualIpEn,
                               USE_COEFF_RELOAD_FALSE, 1, TP_UPSHIFT_CT> >(taps);
        create_casc_kernel_recur<dim - 1, TT_DATA, TT_COEFF, TP_FIR_LEN, TP_SHIFT, TP_RND, TP_INPUT_WINDOW_VSIZE,
                                 TP_CASC_LEN, TP_DUAL_IP, TP_UPSHIFT_CT>::create(firKernels, taps);
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
          unsigned int TP_UPSHIFT_CT>
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
                               TP_UPSHIFT_CT> {
   public:
    static void create(kernel (&firKernels)[TP_CASC_LEN], const std::vector<TT_COEFF>& taps) {
        firKernels[0] = kernel::create_object<
            fir_interpolate_hb<TT_DATA, TT_COEFF, TP_FIR_LEN, TP_SHIFT, TP_RND, TP_INPUT_WINDOW_VSIZE, false, true,
                               fnFirRangeSym<TP_FIR_LEN, TP_CASC_LEN, 0>(), 0, TP_CASC_LEN, TP_DUAL_IP,
                               USE_COEFF_RELOAD_FALSE, 1, TP_UPSHIFT_CT> >(taps);
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
          unsigned int TP_UPSHIFT_CT>
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
                               TP_UPSHIFT_CT> {
   private:
    static constexpr unsigned int kDualIpEn =
        0; // cascaded kernels do not support dual inputs, unless input interface is a stream

   public:
    static void create(kernel (&firKernels)[TP_CASC_LEN]) {
        firKernels[dim - 1] = kernel::create_object<
            fir_interpolate_hb<TT_DATA, TT_COEFF, TP_FIR_LEN, TP_SHIFT, TP_RND, TP_INPUT_WINDOW_VSIZE, true, true,
                               fnFirRangeSym<TP_FIR_LEN, TP_CASC_LEN, dim - 1>(), dim - 1, TP_CASC_LEN, kDualIpEn,
                               USE_COEFF_RELOAD_TRUE, 1, TP_UPSHIFT_CT> >();
        create_casc_kernel_recur<dim - 1, TT_DATA, TT_COEFF, TP_FIR_LEN, TP_SHIFT, TP_RND, TP_INPUT_WINDOW_VSIZE,
                                 TP_CASC_LEN, TP_DUAL_IP, USE_COEFF_RELOAD_TRUE, TP_UPSHIFT_CT>::create(firKernels);
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
          unsigned int TP_UPSHIFT_CT>
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
                               TP_UPSHIFT_CT> {
   public:
    static void create(kernel (&firKernels)[TP_CASC_LEN]) {
        firKernels[0] = kernel::create_object<
            fir_interpolate_hb<TT_DATA, TT_COEFF, TP_FIR_LEN, TP_SHIFT, TP_RND, TP_INPUT_WINDOW_VSIZE, false, true,
                               fnFirRangeSym<TP_FIR_LEN, TP_CASC_LEN, 0>(), 0, TP_CASC_LEN, TP_DUAL_IP,
                               USE_COEFF_RELOAD_TRUE, 1, TP_UPSHIFT_CT> >();
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
          unsigned int TP_UPSHIFT_CT = 0>
class create_casc_kernel {
   private:
    static constexpr unsigned int kDualIpEn =
        0; // cascaded kernels do not support dual inputs, unless input interface is a stream
   public:
    static void create(kernel (&firKernels)[TP_CASC_LEN],
                       const std::vector<TT_COEFF>& taps,
                       const unsigned ctUpshift = 0) {
        firKernels[dim - 1] = kernel::create_object<
            fir_interpolate_hb<TT_DATA, TT_COEFF, TP_FIR_LEN, TP_SHIFT, TP_RND, TP_INPUT_WINDOW_VSIZE, true, false,
                               fnFirRangeRemSym<TP_FIR_LEN, TP_CASC_LEN, dim - 1>(), dim - 1, TP_CASC_LEN, kDualIpEn,
                               USE_COEFF_RELOAD_FALSE, TP_NUM_OUTPUTS, TP_UPSHIFT_CT> >(taps);
        create_casc_kernel_recur<dim - 1, TT_DATA, TT_COEFF, TP_FIR_LEN, TP_SHIFT, TP_RND, TP_INPUT_WINDOW_VSIZE,
                                 TP_CASC_LEN, TP_DUAL_IP, USE_COEFF_RELOAD_FALSE, TP_UPSHIFT_CT>::create(firKernels,
                                                                                                         taps);
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
          unsigned int TP_UPSHIFT_CT>
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
                         TP_UPSHIFT_CT> {
   private:
    static constexpr unsigned int kDualIpEn =
        0; // cascaded kernels do not support dual inputs, unless input interface is a stream
   public:
    static void create(kernel (&firKernels)[TP_CASC_LEN]) {
        firKernels[dim - 1] = kernel::create_object<
            fir_interpolate_hb<TT_DATA, TT_COEFF, TP_FIR_LEN, TP_SHIFT, TP_RND, TP_INPUT_WINDOW_VSIZE, true, false,
                               fnFirRangeRemSym<TP_FIR_LEN, TP_CASC_LEN, dim - 1>(), dim - 1, TP_CASC_LEN, kDualIpEn,
                               USE_COEFF_RELOAD_TRUE, TP_NUM_OUTPUTS, TP_UPSHIFT_CT> >();
        create_casc_kernel_recur<dim - 1, TT_DATA, TT_COEFF, TP_FIR_LEN, TP_SHIFT, TP_RND, TP_INPUT_WINDOW_VSIZE,
                                 TP_CASC_LEN, TP_DUAL_IP, USE_COEFF_RELOAD_TRUE, TP_UPSHIFT_CT>::create(firKernels);
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
          unsigned int TP_UPSHIFT_CT>
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
                         TP_UPSHIFT_CT> {
   public:
    static constexpr unsigned int dim = 1;
    static constexpr unsigned int TP_CASC_LEN = 1;
    static void create(kernel (&firKernels)[TP_CASC_LEN], const std::vector<TT_COEFF>& taps) {
        firKernels[dim - 1] = kernel::create_object<
            fir_interpolate_hb<TT_DATA, TT_COEFF, TP_FIR_LEN, TP_SHIFT, TP_RND, TP_INPUT_WINDOW_VSIZE, false, false,
                               fnFirRangeRemSym<TP_FIR_LEN, TP_CASC_LEN, dim - 1>(), dim - 1, TP_CASC_LEN, TP_DUAL_IP,
                               USE_COEFF_RELOAD_FALSE, TP_NUM_OUTPUTS, TP_UPSHIFT_CT> >(taps);
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
          unsigned int TP_UPSHIFT_CT>
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
                         TP_UPSHIFT_CT> {
   public:
    static constexpr unsigned int dim = 1;
    static constexpr unsigned int TP_CASC_LEN = 1;
    static void create(kernel (&firKernels)[TP_CASC_LEN]) {
        firKernels[dim - 1] = kernel::create_object<
            fir_interpolate_hb<TT_DATA, TT_COEFF, TP_FIR_LEN, TP_SHIFT, TP_RND, TP_INPUT_WINDOW_VSIZE, false, false,
                               fnFirRangeRemSym<TP_FIR_LEN, TP_CASC_LEN, dim - 1>(), dim - 1, TP_CASC_LEN, TP_DUAL_IP,
                               USE_COEFF_RELOAD_TRUE, TP_NUM_OUTPUTS, TP_UPSHIFT_CT> >();
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
 *         of the following:
 *         int16, cint16, int32, cint32, float, cfloat.
 * @tparam TT_COEFF describes the type of individual coefficients of the filter
 *         taps. It must be one of the same set of types listed for TT_DATA
 *         and must also satisfy the following rules:
 *         - Complex types are only supported when TT_DATA is also complex.
 *         - 32 bit types are only supported when TT_DATA is also a 32 bit type,
 *         - TT_COEFF must be an integer type if TT_DATA is an integer type
 *         - TT_COEFF must be a float type if TT_DATA is a float type.
 * @tparam TP_FIR_LEN is an unsigned integer which describes the number of taps
 *         in the filter. TP_FIR_LEN must be in the range 4 to 240 inclusive and
 *         must satisfy (TP_FIR_LEN +1)/4 = N where N is a positive integer.
 * @tparam TP_SHIFT is describes power of 2 shift down applied to the accumulation of
 *         FIR terms before output. TP_SHIFT must be in the range 0 to 61.
 * @tparam TP_RND describes the selection of rounding to be applied during the
 *         shift down stage of processing. TP_RND must be in the range 0 to 7
 *         where
 *         0 = floor (truncate) eg. 3.8 Would become 3.
 *         1 = ceiling e.g. 3.2 would become 4.
 *         2 = round to positive infinity.
 *         3 = round to negative infinity.
 *         4 = round symmetrical to infinity.
 *         5 = round symmetrical to zero.
 *         6 = round convergent to even.
 *         7 = round convergent to odd.
 *         Modes 2 to 7 round to the nearest integer. They differ only in how
 *         they round for values of 0.5.
 * @tparam TP_INPUT_WINDOW_VSIZE describes the number of samples in the window API
 *         used for input to the filter function.
 *         The number of values in the output window will be TP_INPUT_WINDOW_VSIZE
 *         multiplied by 2 by virtue the halfband interpolation factor.
 *         Note: Margin size should not be included in TP_INPUT_WINDOW_VSIZE.
 * @tparam TP_CASC_LEN describes the number of AIE processors to split the operation
 *         over. This allows resource to be traded for higher performance.
 *         TP_CASC_LEN must be in the range 1 (default) to 9.
 * @tparam TP_DUAL_IP is an implementation trade-off between performance and data
 *         bank resource. When set to 0, the FIR performance may be limited
 *         by load contention. When set to 1, two ram banks are used for input.
 * @tparam TP_USE_COEFF_RELOAD allows the user to select if runtime coefficient
 *         reloading should be used. This currently is only available for single
 *         kernel filters. When defining the parameter:
 *         - 0 = static coefficients, defined in filter constructor
 *         - 1 = reloadable coefficients, passed as argument to runtime function
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
          unsigned int TP_UPSHIFT_CT = 0>
/**
 * This is the class for the halfband interpolator FIR graph
 **/
class fir_interpolate_hb_graph : public graph {
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
     * A window API of TP_INPUT_WINDOW_VSIZE*2 samples of TT_DATA type.
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

    /**
     * @brief This is the constructor function for the halfband interpolator FIR graph.
     * @param[in] taps - a pointer to the array of taps values of type TT_COEFF.
     *                   The taps array must be supplied in a compressed form for
     *                   this halfband application, i.e.
     *                   taps[] = {c0, c2, c4, ..., cN, cCT} where
     *                   N = (TP_FIR_LEN+1)/4 and
     *                   cCT is the center tap.
     *                   For example, a 7-tap halfband interpolator might use coeffs
     *                   (1, 0, 2, 5, 2, 0, 1). This would be input as
     *                   taps[]= {1,2,5} since the context of halfband interpolation
     *                   allows the remaining coefficients to be inferred.
     **/
    fir_interpolate_hb_graph(const std::vector<TT_COEFF>& taps) {
        // create kernels
        create_casc_kernel<TP_CASC_LEN, TT_DATA, TT_COEFF, TP_FIR_LEN, TP_SHIFT, TP_RND, TP_INPUT_WINDOW_VSIZE,
                           TP_CASC_LEN, TP_DUAL_IP, TP_USE_COEFF_RELOAD, TP_NUM_OUTPUTS,
                           TP_UPSHIFT_CT>::create(m_firKernels, taps);

        // make input connections
        connect<window<TP_INPUT_WINDOW_VSIZE * sizeof(TT_DATA),
                       fnFirMargin<TP_FIR_LEN / kInterpolateFactor, TT_DATA>() * sizeof(TT_DATA)> >(
            in, m_firKernels[0].in[0]);
        for (int i = 1; i < TP_CASC_LEN; i++) {
            single_buffer(m_firKernels[i].in[0]);
            connect<window<TP_INPUT_WINDOW_VSIZE * sizeof(TT_DATA) +
                           fnFirMargin<TP_FIR_LEN / kInterpolateFactor, TT_DATA>() * sizeof(TT_DATA)> >(
                async(m_firKernels[i - 1].out[1]), async(m_firKernels[i].in[0]));
        }

        // make cascade connections
        for (int i = 1; i < TP_CASC_LEN; i++) {
            connect<cascade>(m_firKernels[i - 1].out[0], m_firKernels[i].in[1]);
        }

        // make output connections
        connect<window<kInterpolateFactor * TP_INPUT_WINDOW_VSIZE * sizeof(TT_DATA)> >(
            m_firKernels[TP_CASC_LEN - 1].out[0], out);

        for (int i = 0; i < TP_CASC_LEN; i++) {
            // Specify mapping constraints
            runtime<ratio>(m_firKernels[i]) = 0.8;
            // Source files
            source(m_firKernels[i]) = "fir_interpolate_hb.cpp";
        }
    }
};

/**
  * @cond NOCOMMENTS
  */
// Specialized SR FIR type template for single input, static coefficients, dual output
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_CASC_LEN,
          unsigned int TP_UPSHIFT_CT>
class fir_interpolate_hb_graph<TT_DATA,
                               TT_COEFF,
                               TP_FIR_LEN,
                               TP_SHIFT,
                               TP_RND,
                               TP_INPUT_WINDOW_VSIZE,
                               TP_CASC_LEN,
                               DUAL_IP_SINGLE,
                               USE_COEFF_RELOAD_FALSE,
                               2,
                               TP_UPSHIFT_CT> : public graph {
   private:
    static_assert(TP_CASC_LEN < 10, "ERROR: Unsupported Cascade length");

   public:
    port<input> in;
    port<output> out;
    port<output> out2;

    kernel m_firKernels[TP_CASC_LEN];
    // Access function for AIE synthesizer
    kernel* getKernels() { return m_firKernels; };
    fir_interpolate_hb_graph(const std::vector<TT_COEFF>& taps) {
        // create kernels
        create_casc_kernel<TP_CASC_LEN, TT_DATA, TT_COEFF, TP_FIR_LEN, TP_SHIFT, TP_RND, TP_INPUT_WINDOW_VSIZE,
                           TP_CASC_LEN, DUAL_IP_SINGLE, USE_COEFF_RELOAD_FALSE, 2, TP_UPSHIFT_CT>::create(m_firKernels,
                                                                                                          taps);

        // make input connections
        connect<window<TP_INPUT_WINDOW_VSIZE * sizeof(TT_DATA),
                       fnFirMargin<TP_FIR_LEN / kInterpolateFactor, TT_DATA>() * sizeof(TT_DATA)> >(
            in, m_firKernels[0].in[0]);
        for (int i = 1; i < TP_CASC_LEN; i++) {
            single_buffer(m_firKernels[i].in[0]);
            connect<window<TP_INPUT_WINDOW_VSIZE * sizeof(TT_DATA) +
                           fnFirMargin<TP_FIR_LEN / kInterpolateFactor, TT_DATA>() * sizeof(TT_DATA)> >(
                async(m_firKernels[i - 1].out[1]), async(m_firKernels[i].in[0]));
        }

        // make cascade connections
        for (int i = 1; i < TP_CASC_LEN; i++) {
            connect<cascade>(m_firKernels[i - 1].out[0], m_firKernels[i].in[1]);
        }

        // make output connections
        connect<window<kInterpolateFactor * TP_INPUT_WINDOW_VSIZE * sizeof(TT_DATA)> >(
            m_firKernels[TP_CASC_LEN - 1].out[0], out);
        connect<window<kInterpolateFactor * TP_INPUT_WINDOW_VSIZE * sizeof(TT_DATA)> >(
            m_firKernels[TP_CASC_LEN - 1].out[1], out2);

        for (int i = 0; i < TP_CASC_LEN; i++) {
            // Specify mapping constraints
            runtime<ratio>(m_firKernels[i]) = 0.8;
            // Source files
            source(m_firKernels[i]) = "fir_interpolate_hb.cpp";
        }
    }
};

// Specialized SR FIR type template for DUAL input,  static coefficients, single output
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_CASC_LEN,
          unsigned int TP_UPSHIFT_CT>
class fir_interpolate_hb_graph<TT_DATA,
                               TT_COEFF,
                               TP_FIR_LEN,
                               TP_SHIFT,
                               TP_RND,
                               TP_INPUT_WINDOW_VSIZE,
                               TP_CASC_LEN,
                               DUAL_IP_DUAL,
                               USE_COEFF_RELOAD_FALSE,
                               1,
                               TP_UPSHIFT_CT> : public graph {
   public:
    port<input> in;
    port<input> in2;
    port<output> out;

    kernel m_firKernels[TP_CASC_LEN];
    // Access function for AIE synthesizer
    kernel* getKernels() { return m_firKernels; };

    // constructor
    fir_interpolate_hb_graph(const std::vector<TT_COEFF>& taps) {
        // create kernels
        create_casc_kernel<TP_CASC_LEN, TT_DATA, TT_COEFF, TP_FIR_LEN, TP_SHIFT, TP_RND, TP_INPUT_WINDOW_VSIZE,
                           TP_CASC_LEN, DUAL_IP_DUAL, USE_COEFF_RELOAD_FALSE, 1, TP_UPSHIFT_CT>::create(m_firKernels,
                                                                                                        taps);

        // make input connections
        connect<window<TP_INPUT_WINDOW_VSIZE * sizeof(TT_DATA),
                       fnFirMargin<TP_FIR_LEN / kInterpolateFactor, TT_DATA>() * sizeof(TT_DATA)> >(
            in, m_firKernels[0].in[0]);
        connect<window<TP_INPUT_WINDOW_VSIZE * sizeof(TT_DATA),
                       fnFirMargin<TP_FIR_LEN / kInterpolateFactor, TT_DATA>() * sizeof(TT_DATA)> >(
            in2, m_firKernels[0].in[1]);
        for (int i = 1; i < TP_CASC_LEN; i++) {
            single_buffer(m_firKernels[i].in[0]);
            connect<window<TP_INPUT_WINDOW_VSIZE * sizeof(TT_DATA) +
                           fnFirMargin<TP_FIR_LEN / kInterpolateFactor, TT_DATA>() * sizeof(TT_DATA)> >(
                async(m_firKernels[i - 1].out[1]), async(m_firKernels[i].in[0]));
        }

        // make cascade connections
        for (int i = 1; i < TP_CASC_LEN; i++) {
            connect<cascade>(m_firKernels[i - 1].out[0], m_firKernels[i].in[1]);
        }

        // make output connections
        connect<window<kInterpolateFactor * TP_INPUT_WINDOW_VSIZE * sizeof(TT_DATA)> >(
            m_firKernels[TP_CASC_LEN - 1].out[0], out);

        for (int i = 0; i < TP_CASC_LEN; i++) {
            // Specify mapping constraints
            runtime<ratio>(m_firKernels[i]) = 0.8;
            // Source files
            source(m_firKernels[i]) = "fir_interpolate_hb.cpp";
        }
    }
};

// Specialized SR FIR type template for DUAL input,  static coefficients, dual output
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_CASC_LEN,
          unsigned int TP_UPSHIFT_CT>
class fir_interpolate_hb_graph<TT_DATA,
                               TT_COEFF,
                               TP_FIR_LEN,
                               TP_SHIFT,
                               TP_RND,
                               TP_INPUT_WINDOW_VSIZE,
                               TP_CASC_LEN,
                               DUAL_IP_DUAL,
                               USE_COEFF_RELOAD_FALSE,
                               2,
                               TP_UPSHIFT_CT> : public graph {
   public:
    port<input> in;
    port<input> in2;
    port<output> out;
    port<output> out2;

    kernel m_firKernels[TP_CASC_LEN];
    // Access function for AIE synthesizer
    kernel* getKernels() { return m_firKernels; };

    // constructor
    fir_interpolate_hb_graph(const std::vector<TT_COEFF>& taps) {
        // create kernels
        create_casc_kernel<TP_CASC_LEN, TT_DATA, TT_COEFF, TP_FIR_LEN, TP_SHIFT, TP_RND, TP_INPUT_WINDOW_VSIZE,
                           TP_CASC_LEN, DUAL_IP_DUAL, USE_COEFF_RELOAD_FALSE, 2, TP_UPSHIFT_CT>::create(m_firKernels,
                                                                                                        taps);

        // make input connections
        connect<window<TP_INPUT_WINDOW_VSIZE * sizeof(TT_DATA),
                       fnFirMargin<TP_FIR_LEN / kInterpolateFactor, TT_DATA>() * sizeof(TT_DATA)> >(
            in, m_firKernels[0].in[0]);
        connect<window<TP_INPUT_WINDOW_VSIZE * sizeof(TT_DATA),
                       fnFirMargin<TP_FIR_LEN / kInterpolateFactor, TT_DATA>() * sizeof(TT_DATA)> >(
            in2, m_firKernels[0].in[1]);
        for (int i = 1; i < TP_CASC_LEN; i++) {
            single_buffer(m_firKernels[i].in[0]);
            connect<window<TP_INPUT_WINDOW_VSIZE * sizeof(TT_DATA) +
                           fnFirMargin<TP_FIR_LEN / kInterpolateFactor, TT_DATA>() * sizeof(TT_DATA)> >(
                async(m_firKernels[i - 1].out[1]), async(m_firKernels[i].in[0]));
        }

        // make cascade connections
        for (int i = 1; i < TP_CASC_LEN; i++) {
            connect<cascade>(m_firKernels[i - 1].out[0], m_firKernels[i].in[1]);
        }

        // make output connections
        connect<window<kInterpolateFactor * TP_INPUT_WINDOW_VSIZE * sizeof(TT_DATA)> >(
            m_firKernels[TP_CASC_LEN - 1].out[0], out);
        connect<window<kInterpolateFactor * TP_INPUT_WINDOW_VSIZE * sizeof(TT_DATA)> >(
            m_firKernels[TP_CASC_LEN - 1].out[1], out2);

        for (int i = 0; i < TP_CASC_LEN; i++) {
            // Specify mapping constraints
            runtime<ratio>(m_firKernels[i]) = 0.8;
            // Source files
            source(m_firKernels[i]) = "fir_interpolate_hb.cpp";
        }
    }
};

// Specialized SR FIR type template for single input, reloadable coefficients, single output
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_CASC_LEN,
          unsigned int TP_UPSHIFT_CT>
class fir_interpolate_hb_graph<TT_DATA,
                               TT_COEFF,
                               TP_FIR_LEN,
                               TP_SHIFT,
                               TP_RND,
                               TP_INPUT_WINDOW_VSIZE,
                               TP_CASC_LEN,
                               DUAL_IP_SINGLE,
                               USE_COEFF_RELOAD_TRUE,
                               1,
                               TP_UPSHIFT_CT> : public graph {
   public:
    port<input> in;
    port<output> out;
    /**
     * @endcond
     */
    /**
     * A Run-time Parameter API containing the set of coefficient values. A change to these values will be detected and
     *will cause a
     * reload of the coefficients within the kernel or kernels to be used on the next data window.
     * This port is present only when TP_USE_COEFF_RELOAD is set to 1.
     **/
    port<input> coeff;
    /**
     * @cond NOCOMMENTS
     */

    kernel m_firKernels[TP_CASC_LEN];

    // Access function for AIE synthesizer
    kernel* getKernels() { return m_firKernels; };

    // constructor
    fir_interpolate_hb_graph() {
        // create kernels
        create_casc_kernel<TP_CASC_LEN, TT_DATA, TT_COEFF, TP_FIR_LEN, TP_SHIFT, TP_RND, TP_INPUT_WINDOW_VSIZE,
                           TP_CASC_LEN, DUAL_IP_SINGLE, USE_COEFF_RELOAD_TRUE, 1, TP_UPSHIFT_CT>::create(m_firKernels);

        // make input connections
        connect<window<TP_INPUT_WINDOW_VSIZE * sizeof(TT_DATA),
                       fnFirMargin<TP_FIR_LEN / kInterpolateFactor, TT_DATA>() * sizeof(TT_DATA)> >(
            in, m_firKernels[0].in[0]);
        for (int i = 1; i < TP_CASC_LEN; i++) {
            single_buffer(m_firKernels[i].in[0]);
            connect<window<TP_INPUT_WINDOW_VSIZE * sizeof(TT_DATA) +
                           fnFirMargin<TP_FIR_LEN / kInterpolateFactor, TT_DATA>() * sizeof(TT_DATA)> >(
                async(m_firKernels[i - 1].out[1]), async(m_firKernels[i].in[0]));
        }

        // make cascade connections
        for (int i = 1; i < TP_CASC_LEN; i++) {
            connect<cascade>(m_firKernels[i - 1].out[0], m_firKernels[i].in[1]);
        }

        // make RTP connection
        connect<parameter>(coeff, async(m_firKernels[0].in[1]));

        // make output connections
        connect<window<kInterpolateFactor * TP_INPUT_WINDOW_VSIZE * sizeof(TT_DATA)> >(
            m_firKernels[TP_CASC_LEN - 1].out[0], out);

        for (int i = 0; i < TP_CASC_LEN; i++) {
            // Specify mapping constraints
            runtime<ratio>(m_firKernels[i]) = 0.8;
            // Source files
            source(m_firKernels[i]) = "fir_interpolate_hb.cpp";
        }
    }
};

// Specialized SR FIR type template for single input, reloadable coefficients, dual output
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_CASC_LEN,
          unsigned int TP_UPSHIFT_CT>
class fir_interpolate_hb_graph<TT_DATA,
                               TT_COEFF,
                               TP_FIR_LEN,
                               TP_SHIFT,
                               TP_RND,
                               TP_INPUT_WINDOW_VSIZE,
                               TP_CASC_LEN,
                               DUAL_IP_SINGLE,
                               USE_COEFF_RELOAD_TRUE,
                               2,
                               TP_UPSHIFT_CT> : public graph {
   public:
    port<input> in;
    port<output> out;
    port<output> out2;
    port<input> coeff;

    kernel m_firKernels[TP_CASC_LEN];

    // Access function for AIE synthesizer
    kernel* getKernels() { return m_firKernels; };

    // constructor
    fir_interpolate_hb_graph() {
        // create kernels
        create_casc_kernel<TP_CASC_LEN, TT_DATA, TT_COEFF, TP_FIR_LEN, TP_SHIFT, TP_RND, TP_INPUT_WINDOW_VSIZE,
                           TP_CASC_LEN, DUAL_IP_SINGLE, USE_COEFF_RELOAD_TRUE, 2, TP_UPSHIFT_CT>::create(m_firKernels);

        // make input connections
        connect<window<TP_INPUT_WINDOW_VSIZE * sizeof(TT_DATA),
                       fnFirMargin<TP_FIR_LEN / kInterpolateFactor, TT_DATA>() * sizeof(TT_DATA)> >(
            in, m_firKernels[0].in[0]);
        for (int i = 1; i < TP_CASC_LEN; i++) {
            single_buffer(m_firKernels[i].in[0]);
            connect<window<TP_INPUT_WINDOW_VSIZE * sizeof(TT_DATA) +
                           fnFirMargin<TP_FIR_LEN / kInterpolateFactor, TT_DATA>() * sizeof(TT_DATA)> >(
                async(m_firKernels[i - 1].out[1]), async(m_firKernels[i].in[0]));
        }

        // make cascade connections
        for (int i = 1; i < TP_CASC_LEN; i++) {
            connect<cascade>(m_firKernels[i - 1].out[0], m_firKernels[i].in[1]);
        }

        // make RTP connection
        connect<parameter>(coeff, async(m_firKernels[0].in[1]));

        // make output connections
        connect<window<kInterpolateFactor * TP_INPUT_WINDOW_VSIZE * sizeof(TT_DATA)> >(
            m_firKernels[TP_CASC_LEN - 1].out[0], out);
        connect<window<kInterpolateFactor * TP_INPUT_WINDOW_VSIZE * sizeof(TT_DATA)> >(
            m_firKernels[TP_CASC_LEN - 1].out[1], out2);

        for (int i = 0; i < TP_CASC_LEN; i++) {
            // Specify mapping constraints
            runtime<ratio>(m_firKernels[i]) = 0.8;
            // Source files
            source(m_firKernels[i]) = "fir_interpolate_hb.cpp";
        }
    }
};

// Specialized SR FIR type template for DUAL input, reloadable coefficients, single output
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_CASC_LEN,
          unsigned int TP_UPSHIFT_CT>
class fir_interpolate_hb_graph<TT_DATA,
                               TT_COEFF,
                               TP_FIR_LEN,
                               TP_SHIFT,
                               TP_RND,
                               TP_INPUT_WINDOW_VSIZE,
                               TP_CASC_LEN,
                               DUAL_IP_DUAL,
                               USE_COEFF_RELOAD_TRUE,
                               1,
                               TP_UPSHIFT_CT> : public graph {
   public:
    port<input> in;
    port<input> in2;
    port<output> out;
    port<input> coeff;

    kernel m_firKernels[TP_CASC_LEN];

    // Access function for AIE synthesizer
    kernel* getKernels() { return m_firKernels; };

    // constructor
    fir_interpolate_hb_graph() {
        // create kernels
        create_casc_kernel<TP_CASC_LEN, TT_DATA, TT_COEFF, TP_FIR_LEN, TP_SHIFT, TP_RND, TP_INPUT_WINDOW_VSIZE,
                           TP_CASC_LEN, DUAL_IP_DUAL, USE_COEFF_RELOAD_TRUE, 1, TP_UPSHIFT_CT>::create(m_firKernels);

        // make input connections
        connect<window<TP_INPUT_WINDOW_VSIZE * sizeof(TT_DATA),
                       fnFirMargin<TP_FIR_LEN / kInterpolateFactor, TT_DATA>() * sizeof(TT_DATA)> >(
            in, m_firKernels[0].in[0]);
        connect<window<TP_INPUT_WINDOW_VSIZE * sizeof(TT_DATA),
                       fnFirMargin<TP_FIR_LEN / kInterpolateFactor, TT_DATA>() * sizeof(TT_DATA)> >(
            in2, m_firKernels[0].in[1]);
        for (int i = 1; i < TP_CASC_LEN; i++) {
            single_buffer(m_firKernels[i].in[0]);
            connect<window<TP_INPUT_WINDOW_VSIZE * sizeof(TT_DATA) +
                           fnFirMargin<TP_FIR_LEN / kInterpolateFactor, TT_DATA>() * sizeof(TT_DATA)> >(
                async(m_firKernels[i - 1].out[1]), async(m_firKernels[i].in[0]));
        }

        // make cascade connections
        for (int i = 1; i < TP_CASC_LEN; i++) {
            connect<cascade>(m_firKernels[i - 1].out[0], m_firKernels[i].in[1]);
        }

        // make RTP connection
        connect<parameter>(coeff, async(m_firKernels[0].in[2]));

        // make output connections
        connect<window<kInterpolateFactor * TP_INPUT_WINDOW_VSIZE * sizeof(TT_DATA)> >(
            m_firKernels[TP_CASC_LEN - 1].out[0], out);

        for (int i = 0; i < TP_CASC_LEN; i++) {
            // Specify mapping constraints
            runtime<ratio>(m_firKernels[i]) = 0.8;
            // Source files
            source(m_firKernels[i]) = "fir_interpolate_hb.cpp";
        }
    }
};

// Specialized SR FIR type template for DUAL input, reloadable coefficients, dual output
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_CASC_LEN,
          unsigned int TP_UPSHIFT_CT>
class fir_interpolate_hb_graph<TT_DATA,
                               TT_COEFF,
                               TP_FIR_LEN,
                               TP_SHIFT,
                               TP_RND,
                               TP_INPUT_WINDOW_VSIZE,
                               TP_CASC_LEN,
                               DUAL_IP_DUAL,
                               USE_COEFF_RELOAD_TRUE,
                               2,
                               TP_UPSHIFT_CT> : public graph {
   public:
    port<input> in;
    port<input> in2;
    port<output> out;
    port<output> out2;
    port<input> coeff;

    kernel m_firKernels[TP_CASC_LEN];

    // Access function for AIE synthesizer
    kernel* getKernels() { return m_firKernels; };

    // constructor
    fir_interpolate_hb_graph() {
        // create kernels
        create_casc_kernel<TP_CASC_LEN, TT_DATA, TT_COEFF, TP_FIR_LEN, TP_SHIFT, TP_RND, TP_INPUT_WINDOW_VSIZE,
                           TP_CASC_LEN, DUAL_IP_DUAL, USE_COEFF_RELOAD_TRUE, 2, TP_UPSHIFT_CT>::create(m_firKernels);

        // make input connections
        connect<window<TP_INPUT_WINDOW_VSIZE * sizeof(TT_DATA),
                       fnFirMargin<TP_FIR_LEN / kInterpolateFactor, TT_DATA>() * sizeof(TT_DATA)> >(
            in, m_firKernels[0].in[0]);
        connect<window<TP_INPUT_WINDOW_VSIZE * sizeof(TT_DATA),
                       fnFirMargin<TP_FIR_LEN / kInterpolateFactor, TT_DATA>() * sizeof(TT_DATA)> >(
            in2, m_firKernels[0].in[1]);
        for (int i = 1; i < TP_CASC_LEN; i++) {
            single_buffer(m_firKernels[i].in[0]);
            connect<window<TP_INPUT_WINDOW_VSIZE * sizeof(TT_DATA) +
                           fnFirMargin<TP_FIR_LEN / kInterpolateFactor, TT_DATA>() * sizeof(TT_DATA)> >(
                async(m_firKernels[i - 1].out[1]), async(m_firKernels[i].in[0]));
        }

        // make cascade connections
        for (int i = 1; i < TP_CASC_LEN; i++) {
            connect<cascade>(m_firKernels[i - 1].out[0], m_firKernels[i].in[1]);
        }

        // make RTP connection
        connect<parameter>(coeff, async(m_firKernels[0].in[2]));

        // make output connections
        connect<window<kInterpolateFactor * TP_INPUT_WINDOW_VSIZE * sizeof(TT_DATA)> >(
            m_firKernels[TP_CASC_LEN - 1].out[0], out);
        connect<window<kInterpolateFactor * TP_INPUT_WINDOW_VSIZE * sizeof(TT_DATA)> >(
            m_firKernels[TP_CASC_LEN - 1].out[1], out2);

        for (int i = 0; i < TP_CASC_LEN; i++) {
            // Specify mapping constraints
            runtime<ratio>(m_firKernels[i]) = 0.8;
            // Source files
            source(m_firKernels[i]) = "fir_interpolate_hb.cpp";
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

#endif // _DSPLIB_FIR_INTERPOLATE_HB_GRAPH_HPP_

/*  (c) Copyright 2020 Xilinx, Inc. All rights reserved.

    This file contains confidential and proprietary information
    of Xilinx, Inc. and is protected under U.S. and
    international copyright and other intellectual property
    laws.

    DISCLAIMER
    This disclaimer is not a license and does not grant any
    rights to the materials distributed herewith. Except as
    otherwise provided in a valid license issued to you by
    Xilinx, and to the maximum extent permitted by applicable
    law: (1) THESE MATERIALS ARE MADE AVAILABLE "AS IS" AND
    WITH ALL FAULTS, AND XILINX HEREBY DISCLAIMS ALL WARRANTIES
    AND CONDITIONS, EXPRESS, IMPLIED, OR STATUTORY, INCLUDING
    BUT NOT LIMITED TO WARRANTIES OF MERCHANTABILITY, NON-
    INFRINGEMENT, OR FITNESS FOR ANY PARTICULAR PURPOSE; and
    (2) Xilinx shall not be liable (whether in contract or tort,
    including negligence, or under any other theory of
    liability) for any loss or damage of any kind or nature
    related to, arising under or in connection with these
    materials, including for any direct, or any indirect,
    special, incidental, or consequential loss or damage
    (including loss of data, profits, goodwill, or any type of
    loss or damage suffered as a result of any action brought
    by a third party) even if such damage or loss was
    reasonably foreseeable or Xilinx had been advised of the
    possibility of the same.

    CRITICAL APPLICATIONS
    Xilinx products are not designed or intended to be fail-
    safe, or for use in any application requiring fail-safe
    performance, such as life-support or safety devices or
    systems, Class III medical devices, nuclear facilities,
    applications related to the deployment of airbags, or any
    other applications that could lead to death, personal
    injury, or severe property or environmental damage
    (individually and collectively, "Critical
    Applications"). Customer assumes the sole risk and
    liability of any use of Xilinx products in Critical
    Applications, subject only to applicable laws and
    regulations governing limitations on product liability.

    THIS COPYRIGHT NOTICE AND DISCLAIMER MUST BE RETAINED AS
    PART OF THIS FILE AT ALL TIMES.                       */
