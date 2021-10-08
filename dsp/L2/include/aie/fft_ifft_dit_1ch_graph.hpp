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
#ifndef _DSPLIB_FFT_IFFT_DIT_1CH_GRAPH_HPP_
#define _DSPLIB_FFT_IFFT_DIT_1CH_GRAPH_HPP_

#include <adf.h>
#include <vector>
#include "fft_ifft_dit_1ch.hpp"
#include "fft_r2comb.hpp"
#include "widget_api_cast.hpp"

using namespace adf;
using namespace xf::dsp::aie::widget::api_cast;
using namespace xf::dsp::aie::fft::r2comb;

// Note on heap. The FFT twiddles and internal scratch memories can be handled by the graph scope mechanism which allows
// the use of adjacent tile's memory, hence aleviating the 32kB memory limit.
// Without the graph scope mechanism, heap can be set explicitly, or set automatically using aiecompiler switch
// --xlopt=1
// The following #defines allow control over which mechansims is used, though this must be done in conjunction with the
// Makefile
#ifndef __X86SIM__
#define USE_GRAPH_SCOPE
#endif
//#define USE_EXPLICIT_HEAP

#ifdef USE_GRAPH_SCOPE
#include "fft_bufs.h" //Extern declarations of all twiddle factor stores and rank-temporary sample stores.
#endif                // USE_GRAPH_SCOPE

namespace xf {
namespace dsp {
namespace aie {
namespace fft {
namespace dit_1ch {
/**
 * @cond NOCOMMENTS
 */

//---------start of recursive kernel creation code.
// Recursive kernel creation, static coefficients
template <int dim,
          typename TT_DATA,
          typename TT_INT_DATA,
          typename TT_TWIDDLE,
          unsigned int TP_POINT_SIZE,
          unsigned int TP_FFT_NIFFT,
          unsigned int TP_SHIFT,
          unsigned int TP_CASC_LEN,
          unsigned int TP_END_RANK,
          unsigned int TP_RANKS_PER_KERNEL,
          unsigned int TP_DYN_PT_SIZE,
          unsigned int TP_WINDOW_VSIZE = TP_POINT_SIZE>
class create_casc_kernel_recur {
   public:
    static void create(kernel (&fftKernels)[TP_CASC_LEN]) {
        static constexpr int kRawStartRank = (int)TP_END_RANK - (int)TP_RANKS_PER_KERNEL;
        static constexpr unsigned int TP_START_RANK = kRawStartRank < 0 ? 0 : kRawStartRank;

        fftKernels[dim - 1] = kernel::create_object<
            fft_ifft_dit_1ch<TT_INT_DATA, TT_INT_DATA, TT_TWIDDLE, TP_POINT_SIZE, TP_FFT_NIFFT, TP_SHIFT, TP_START_RANK,
                             TP_END_RANK, TP_DYN_PT_SIZE, TP_WINDOW_VSIZE> >();
        create_casc_kernel_recur<dim - 1, TT_DATA, TT_INT_DATA, TT_TWIDDLE, TP_POINT_SIZE, TP_FFT_NIFFT, TP_SHIFT,
                                 TP_CASC_LEN, TP_START_RANK, TP_RANKS_PER_KERNEL, TP_DYN_PT_SIZE,
                                 TP_WINDOW_VSIZE>::create(fftKernels);
    }
};
// Recursive fft kernel creation, static coefficients
template <typename TT_DATA,
          typename TT_INT_DATA,
          typename TT_TWIDDLE,
          unsigned int TP_POINT_SIZE,
          unsigned int TP_FFT_NIFFT,
          unsigned int TP_SHIFT,
          unsigned int TP_CASC_LEN,
          unsigned int TP_END_RANK,
          unsigned int TP_RANKS_PER_KERNEL,
          unsigned int TP_DYN_PT_SIZE,
          unsigned int TP_WINDOW_VSIZE>
class create_casc_kernel_recur<1,
                               TT_DATA,
                               TT_INT_DATA,
                               TT_TWIDDLE,
                               TP_POINT_SIZE,
                               TP_FFT_NIFFT,
                               TP_SHIFT,
                               TP_CASC_LEN,
                               TP_END_RANK,
                               TP_RANKS_PER_KERNEL,
                               TP_DYN_PT_SIZE,
                               TP_WINDOW_VSIZE> {
   public:
    static void create(kernel (&fftKernels)[TP_CASC_LEN]) {
        static constexpr unsigned int TP_START_RANK = 0;
        fftKernels[0] = kernel::create_object<
            fft_ifft_dit_1ch<TT_DATA, TT_INT_DATA, TT_TWIDDLE, TP_POINT_SIZE, TP_FFT_NIFFT, TP_SHIFT, TP_START_RANK,
                             TP_END_RANK, TP_DYN_PT_SIZE, TP_WINDOW_VSIZE> >();
    }
};
// fft Kernel creation, entry to recursion, also end of cascade. For integer types
template <int dim,
          typename TT_DATA, // type for I/O
          typename TT_TWIDDLE,
          unsigned int TP_POINT_SIZE,
          unsigned int TP_FFT_NIFFT,
          unsigned int TP_SHIFT,
          unsigned int TP_CASC_LEN,
          unsigned int TP_DYN_PT_SIZE,
          unsigned int TP_WINDOW_VSIZE = TP_POINT_SIZE>
class create_casc_kernel {
   public:
    typedef typename std::conditional<std::is_same<TT_DATA, cint16>::value, cint32_t, TT_DATA>::type T_internalDataType;
    static void create(kernel (&fftKernels)[TP_CASC_LEN]) {
        static constexpr unsigned int TP_END_RANK_RAW =
            (TP_POINT_SIZE == 4096)
                ? 12
                : (TP_POINT_SIZE == 2048)
                      ? 11
                      : (TP_POINT_SIZE == 1024)
                            ? 10
                            : (TP_POINT_SIZE == 512)
                                  ? 9
                                  : (TP_POINT_SIZE == 256)
                                        ? 8
                                        : (TP_POINT_SIZE == 128)
                                              ? 7
                                              : (TP_POINT_SIZE == 64)
                                                    ? 6
                                                    : (TP_POINT_SIZE == 32)
                                                          ? 5
                                                          : (TP_POINT_SIZE == 16) ? 4
                                                                                  : 0; // 0 is an error trap effectively
        static constexpr unsigned int kIntConfig = std::is_same<TT_DATA, cfloat>::value ? 0 : 1;
        static constexpr unsigned int TP_END_RANK = (kIntConfig == 1) ? fnCeil<TP_END_RANK_RAW, 2>() : TP_END_RANK_RAW;
        static_assert(fnCheckCascLen<TT_DATA, TP_END_RANK, TP_CASC_LEN>(), "Error: TP_CASC_LEN is invalid");
        static_assert(
            fnCheckCascLen2<TT_DATA, TP_POINT_SIZE, TP_CASC_LEN>(),
            "Error: 16 point float FFT does not support cascade"); // due to need for ping/pang/pong complication
        static constexpr int kRawRanksPerKernel =
            std::is_same<TT_DATA, cfloat>::value ? (TP_END_RANK / TP_CASC_LEN) : (TP_END_RANK / TP_CASC_LEN / 2) * 2;
        static constexpr unsigned int TP_RANKS_PER_KERNEL = std::is_same<TT_DATA, cfloat>::value
                                                                ? kRawRanksPerKernel
                                                                : (kRawRanksPerKernel < 2 ? 2 : kRawRanksPerKernel);
        static constexpr unsigned int TP_START_RANK = (int)TP_END_RANK - (int)TP_RANKS_PER_KERNEL;
        fftKernels[dim - 1] = kernel::create_object<
            fft_ifft_dit_1ch<T_internalDataType, TT_DATA, TT_TWIDDLE, TP_POINT_SIZE, TP_FFT_NIFFT, TP_SHIFT,
                             TP_START_RANK, TP_END_RANK, TP_DYN_PT_SIZE, TP_WINDOW_VSIZE> >();
        create_casc_kernel_recur<dim - 1, TT_DATA, T_internalDataType, TT_TWIDDLE, TP_POINT_SIZE, TP_FFT_NIFFT,
                                 TP_SHIFT, TP_CASC_LEN, TP_START_RANK, TP_RANKS_PER_KERNEL, TP_DYN_PT_SIZE,
                                 TP_WINDOW_VSIZE>::create(fftKernels);
    }
};
// fft Kernel creation, entry to recursion, also end of cascade. For integer types - for single kernel
template <typename TT_DATA, // type for I/O
          typename TT_TWIDDLE,
          unsigned int TP_POINT_SIZE,
          unsigned int TP_FFT_NIFFT,
          unsigned int TP_SHIFT,
          unsigned int TP_DYN_PT_SIZE,
          unsigned int TP_WINDOW_VSIZE>
class create_casc_kernel<1,
                         TT_DATA,
                         TT_TWIDDLE,
                         TP_POINT_SIZE,
                         TP_FFT_NIFFT,
                         TP_SHIFT,
                         1,
                         TP_DYN_PT_SIZE,
                         TP_WINDOW_VSIZE> {
   public:
    static void create(kernel (&fftKernels)[1]) {
        static constexpr unsigned int TP_END_RANK_RAW =
            (TP_POINT_SIZE == 4096)
                ? 12
                : (TP_POINT_SIZE == 2048)
                      ? 11
                      : (TP_POINT_SIZE == 1024)
                            ? 10
                            : (TP_POINT_SIZE == 512)
                                  ? 9
                                  : (TP_POINT_SIZE == 256)
                                        ? 8
                                        : (TP_POINT_SIZE == 128)
                                              ? 7
                                              : (TP_POINT_SIZE == 64)
                                                    ? 6
                                                    : (TP_POINT_SIZE == 32)
                                                          ? 5
                                                          : (TP_POINT_SIZE == 16) ? 4
                                                                                  : 0; // 0 is an error trap effectively
        static constexpr unsigned int kIntConfig = std::is_same<TT_DATA, cfloat>::value ? 0 : 1;
        static constexpr unsigned int TP_END_RANK = (kIntConfig == 1) ? fnCeil<TP_END_RANK_RAW, 2>() : TP_END_RANK_RAW;
        static constexpr unsigned int TP_START_RANK = 0;
        fftKernels[0] =
            kernel::create_object<fft_ifft_dit_1ch<TT_DATA, TT_DATA, TT_TWIDDLE, TP_POINT_SIZE, TP_FFT_NIFFT, TP_SHIFT,
                                                   TP_START_RANK, TP_END_RANK, TP_DYN_PT_SIZE, TP_WINDOW_VSIZE> >();
    }
};

template <typename TT_DATA,
          typename TT_TWIDDLE,
          unsigned int TP_POINT_SIZE,
          unsigned int TP_FFT_NIFFT,
          unsigned int TP_SHIFT,
          unsigned int TP_WINDOW_VSIZE,
          unsigned int TP_PARALLEL_POWER,
          unsigned int TP_INDEX>
class create_r2comb_kernels {
   public:
    static constexpr int kParallel_factor = 1 << TP_PARALLEL_POWER;
    static void create(kernel (&m_r2Comb)[kParallel_factor]) {
        m_r2Comb[TP_INDEX] =
            kernel::create_object<fft_r2comb<TT_DATA, TT_TWIDDLE, TP_POINT_SIZE, TP_FFT_NIFFT, TP_SHIFT,
                                             TP_WINDOW_VSIZE, TP_PARALLEL_POWER, TP_INDEX> >();
        create_r2comb_kernels<TT_DATA, TT_TWIDDLE, TP_POINT_SIZE, TP_FFT_NIFFT, TP_SHIFT, TP_WINDOW_VSIZE,
                              TP_PARALLEL_POWER, (TP_INDEX - 1)>::create(m_r2Comb);
    }
};

template <typename TT_DATA,
          typename TT_TWIDDLE,
          unsigned int TP_POINT_SIZE,
          unsigned int TP_FFT_NIFFT,
          unsigned int TP_SHIFT,
          unsigned int TP_WINDOW_VSIZE,
          unsigned int TP_PARALLEL_POWER>
class create_r2comb_kernels<TT_DATA,
                            TT_TWIDDLE,
                            TP_POINT_SIZE,
                            TP_FFT_NIFFT,
                            TP_SHIFT,
                            TP_WINDOW_VSIZE,
                            TP_PARALLEL_POWER,
                            0> {
   public:
    static constexpr int kParallel_factor = 1 << TP_PARALLEL_POWER;
    static void create(kernel (&m_r2Comb)[kParallel_factor]) {
        m_r2Comb[0] = kernel::create_object<fft_r2comb<TT_DATA, TT_TWIDDLE, TP_POINT_SIZE, TP_FFT_NIFFT, TP_SHIFT,
                                                       TP_WINDOW_VSIZE, TP_PARALLEL_POWER, 0> >();
    }
};

template <typename TT_DATA, unsigned int TP_WINDOW_VSIZE, unsigned int TP_PARALLEL_POWER, unsigned int TP_INDEX>
class create_combInWidget_kernels {
   public:
    static constexpr int kParallel_factor = 1 << TP_PARALLEL_POWER;
    static void create(kernel (&m_combInKernel)[kParallel_factor]) {
        m_combInKernel[TP_INDEX] = kernel::create_object<
            widget_api_cast<TT_DATA, kStreamAPI, kWindowAPI, 2, TP_WINDOW_VSIZE, 1, kSampleIntlv> >();
        create_combInWidget_kernels<TT_DATA, TP_WINDOW_VSIZE, TP_PARALLEL_POWER, (TP_INDEX - 1)>::create(
            m_combInKernel);
    }
};

template <typename TT_DATA, unsigned int TP_WINDOW_VSIZE, unsigned int TP_PARALLEL_POWER>
class create_combInWidget_kernels<TT_DATA, TP_WINDOW_VSIZE, TP_PARALLEL_POWER, 0> {
   public:
    static constexpr int kParallel_factor = 1 << TP_PARALLEL_POWER;
    static void create(kernel (&m_combInKernel)[kParallel_factor]) {
        m_combInKernel[0] = kernel::create_object<
            widget_api_cast<TT_DATA, kStreamAPI, kWindowAPI, 2, TP_WINDOW_VSIZE, 1, kSampleIntlv> >();
    }
};

template <typename TT_DATA, unsigned int TP_WINDOW_VSIZE, unsigned int TP_PARALLEL_POWER, unsigned int TP_INDEX>
class create_combOutWidget_kernels {
   public:
    static constexpr int kParallel_factor = 1 << TP_PARALLEL_POWER;
    static void create(kernel (&m_combOutKernel)[kParallel_factor]) {
        m_combOutKernel[TP_INDEX] =
            kernel::create_object<widget_api_cast<TT_DATA, kWindowAPI, kStreamAPI, 1, TP_WINDOW_VSIZE, 2, kSplit> >();
        create_combOutWidget_kernels<TT_DATA, TP_WINDOW_VSIZE, TP_PARALLEL_POWER, (TP_INDEX - 1)>::create(
            m_combOutKernel);
    }
};

template <typename TT_DATA, unsigned int TP_WINDOW_VSIZE, unsigned int TP_PARALLEL_POWER>
class create_combOutWidget_kernels<TT_DATA, TP_WINDOW_VSIZE, TP_PARALLEL_POWER, 0> {
   public:
    static constexpr int kParallel_factor = 1 << TP_PARALLEL_POWER;
    static void create(kernel (&m_combOutKernel)[kParallel_factor]) {
        m_combOutKernel[0] =
            kernel::create_object<widget_api_cast<TT_DATA, kWindowAPI, kStreamAPI, 1, TP_WINDOW_VSIZE, 2, kSplit> >();
    }
};

//---------End of recursive code.

/**
  * @endcond
  */

//--------------------------------------------------------------------------------------------------
// fft_dit_1ch template
//--------------------------------------------------------------------------------------------------
/**
 * @brief fft_dit_1ch is a single-channel, decimation-in-time, fixed point size FFT
 *
 * These are the templates to configure the single-channel decimation-in-time class.
 * @tparam TT_DATA describes the type of individual data samples input to and
 *         output from the transform function. This is a typename and must be one
 *         of the following: \n
 *         int16, cint16, int32, cint32, float, cfloat.
 * @tparam TT_TWIDDLE describes the type of twiddle factors of the transform. \n
 *         It must be one of the following: cint16, cint32, cfloat
 *         and must also satisfy the following rules:
 *         - 32 bit types are only supported when TT_DATA is also a 32 bit type,
 *         - TT_TWIDDLE must be an integer type if TT_DATA is an integer type
 *         - TT_TWIDDLE must be cfloat type if TT_DATA is a float type.
 * @tparam TP_POINT_SIZE is an unsigned integer which describes the number of point
 *         size of the transform. \n This must be 2^N where N is an integer in the range
 *         4 to 16 inclusive. \n When TP_DYN_PT_SIZE is set, TP_POINT_SIZE describes the maximum
 *         point size possible.
 * @tparam TP_FFT_NIFFT selects whether the transform to perform is an FFT (1) or IFFT (0).
 * @tparam TP_SHIFT selects the power of 2 to scale the result by prior to output.
 * @tparam TP_CASC_LEN selects the number of kernels the FFT will be divided over in series
 *         to improve throughput
 * @tparam TP_DYN_PT_SIZE selects whether (1) or not (0) to use run-time point size determination. \n
 *         When set, each frame of data must be preceeded, in the window, by a 256 bit header. \n
 *         The output frame will also be preceeded by a 256 bit vector which is a copy of the input
 *         vector, but for the top byte, which is 0 to indicate a legal frame or 1 to indicate an illegal
 *         frame. \n
 *         The lowest significance byte of the input header field describes forward (non-zero) or
 *         inverse(0) direction. \n
 *         The second least significant byte  8 bits of this field describe the Radix 2 power of the following
 *         frame. e.g. for a 512 point size, this field would hold 9, as 2^9 = 512. \n Any value below 4 or
 *         greater than log2(TP_POINT_SIZE) is considered illegal. \n When this occurs the top byte of the
 *         output header will be set to 1 and the output samples will be set to 0 for a frame of TP_POINT_SIZE
 * @tparam TP_WINDOW_VSIZE is an unsigned integer which describes the number of samples in the input window. \n
 *         By default, TP_WINDOW_SIZE is set ot match TP_POINT_SIZE. \n
 *         TP_WINDOW_SIZE may be set to be an integer multiple of the TP_POINT_SIZE, in which case
 *         multiple FFT iterations will be performed on a given input window, resulting in multiple
 *         iterations of output samples, reducing the numer of times the kernel needs to be triggered to
 *         process a given number of input data samples. \n
 *         As a result, the overheads inferred during kernel triggering are reduced and overall performance
 *         is increased.
 * @tparam TP_API is an unsigned integer to select window (0) or stream (1) interfaces.
 * @tparam TP_PARALLEL_POWER is an unsigned integer to describe how many subframe processors to use. \n
 *         The default is 1. This may be set to 4 or 16 to increase throughput.
  **/
template <typename TT_DATA,
          typename TT_TWIDDLE,
          unsigned int TP_POINT_SIZE,
          unsigned int TP_FFT_NIFFT,
          unsigned int TP_SHIFT,
          unsigned int TP_CASC_LEN = 1,
          unsigned int TP_DYN_PT_SIZE = 0, // backwards compatible default
          unsigned int TP_WINDOW_VSIZE = TP_POINT_SIZE>
/**
 **/
class fft_ifft_dit_1ch_base_graph : public graph {
   public:
    // declare FFT Kernel array
    kernel m_fftKernels[TP_CASC_LEN];
    kernel* getKernels() { return m_fftKernels; };

#ifdef USE_GRAPH_SCOPE
    parameter fft_buf1;

    // twiddle table
    parameter fft_lut1, fft_lut1a, fft_lut1b; // large twiddle tables
    parameter fft_lut2, fft_lut2a, fft_lut2b;
    parameter fft_lut3, fft_lut3a, fft_lut3b;
    parameter fft_lut4, fft_lut4a, fft_lut4b;

    // interrank store
    parameter fft_buf4096;
    parameter fft_buf2048;
    parameter fft_buf1024;
    parameter fft_buf512;
    parameter fft_buf256;
    parameter fft_buf128;
#endif // USE_GRAPH_SCOPE

    /**
     * @brief This is the constructor function for the Single channel DIT FFT graph.
     **/
    // Constructor
    fft_ifft_dit_1ch_base_graph() {
        typedef
            typename std::conditional<std::is_same<TT_DATA, cint16>::value, cint32_t, TT_DATA>::type T_internalDataType;

        // Create kernel class(s)
        create_casc_kernel<TP_CASC_LEN, TT_DATA, TT_TWIDDLE, TP_POINT_SIZE, TP_FFT_NIFFT, TP_SHIFT, TP_CASC_LEN,
                           TP_DYN_PT_SIZE, TP_WINDOW_VSIZE>::create(m_fftKernels);

        // Make kernel to kernel window connections
        for (int k = 0; k < TP_CASC_LEN - 1; ++k) {
            connect<window<TP_WINDOW_VSIZE * sizeof(T_internalDataType) + TP_DYN_PT_SIZE * 32> >(
                m_fftKernels[k].out[0], m_fftKernels[k + 1].in[0]);
        }

#ifdef USE_GRAPH_SCOPE
        // TODO - at present all twiddles connect to all kernels in a cascade, but this could be optimized for static
        // pointsize since each twiddle table need only go to one kernel
        // Connect twiddle Lookups
        // Note that this switch statement does NOT use break statements. That is deliberate. The top case executes all
        // cases.
        if (std::is_same<TT_DATA, cfloat>::value) {
            if (TP_POINT_SIZE == 4096) {
                fft_lut1 = parameter::array(fft_lut_tw2048_cfloat);
                for (int k = 0; k < TP_CASC_LEN; ++k) {
                    connect<>(fft_lut1, m_fftKernels[k]);
                }
            }
            if (TP_POINT_SIZE >= 2048) {
                fft_lut2 = parameter::array(fft_lut_tw1024_cfloat);
                for (int k = 0; k < TP_CASC_LEN; ++k) {
                    connect<>(fft_lut2, m_fftKernels[k]);
                }
            }
            if (TP_POINT_SIZE >= 1024) {
                fft_lut3 = parameter::array(fft_lut_tw512_cfloat);
                for (int k = 0; k < TP_CASC_LEN; ++k) {
                    connect<>(fft_lut3, m_fftKernels[k]);
                }
            }
            if (TP_POINT_SIZE >= 512) {
                fft_lut4 = parameter::array(fft_lut_tw256_cfloat);
                for (int k = 0; k < TP_CASC_LEN; ++k) {
                    connect<>(fft_lut4, m_fftKernels[k]);
                }
            }
        } else {
            if (TP_POINT_SIZE == 4096) {
                fft_lut1 = parameter::array(fft_lut_tw2048_half);
                for (int k = 0; k < TP_CASC_LEN; ++k) {
                    connect<>(fft_lut1, m_fftKernels[k]);
                }
                fft_lut2 = parameter::array(fft_lut_tw1024);
                for (int k = 0; k < TP_CASC_LEN; ++k) {
                    connect<>(fft_lut2, m_fftKernels[k]);
                }
                fft_lut3 = parameter::array(fft_lut_tw512);
                for (int k = 0; k < TP_CASC_LEN; ++k) {
                    connect<>(fft_lut3, m_fftKernels[k]);
                }
                fft_lut4 = parameter::array(fft_lut_tw256);
                for (int k = 0; k < TP_CASC_LEN; ++k) {
                    connect<>(fft_lut4, m_fftKernels[k]);
                }
            }
            if (TP_POINT_SIZE == 2048) {
                fft_lut2 = parameter::array(fft_lut_tw1024_half);
                for (int k = 0; k < TP_CASC_LEN; ++k) {
                    connect<>(fft_lut2, m_fftKernels[k]);
                }
                fft_lut3 = parameter::array(fft_lut_tw512);
                for (int k = 0; k < TP_CASC_LEN; ++k) {
                    connect<>(fft_lut3, m_fftKernels[k]);
                }
                fft_lut4 = parameter::array(fft_lut_tw256);
                for (int k = 0; k < TP_CASC_LEN; ++k) {
                    connect<>(fft_lut4, m_fftKernels[k]);
                }
            }
            if (TP_POINT_SIZE == 1024) {
                fft_lut3 = parameter::array(fft_lut_tw512_half);
                for (int k = 0; k < TP_CASC_LEN; ++k) {
                    connect<>(fft_lut3, m_fftKernels[k]);
                }
                fft_lut4 = parameter::array(fft_lut_tw256);
                for (int k = 0; k < TP_CASC_LEN; ++k) {
                    connect<>(fft_lut4, m_fftKernels[k]);
                }
            }
            if (TP_POINT_SIZE == 512) {
                fft_lut4 = parameter::array(fft_lut_tw256_half);
                for (int k = 0; k < TP_CASC_LEN; ++k) {
                    connect<>(fft_lut4, m_fftKernels[k]);
                }
            }
        }

        // Connect inter-rank temporary storage
        fft_buf4096 = parameter::array(fft_4096_tmp1);
        fft_buf2048 = parameter::array(fft_2048_tmp1);
        fft_buf1024 = parameter::array(fft_1024_tmp1);
        fft_buf512 = parameter::array(fft_512_tmp1);
        fft_buf256 = parameter::array(fft_256_tmp1);
        fft_buf128 = parameter::array(fft_128_tmp1);
        switch (TP_POINT_SIZE) {
            case 4096:
                fft_buf1 = fft_buf4096;
                break;
            case 2048:
                fft_buf1 = fft_buf2048;
                break;
            case 1024:
                fft_buf1 = fft_buf1024;
                break;
            case 512:
                fft_buf1 = fft_buf512;
                break;
            case 256:
                fft_buf1 = fft_buf256;
                break;
            default:
                fft_buf1 = fft_buf128;
                break;
        }
        for (int k = 0; k < TP_CASC_LEN; ++k) {
            connect<>(fft_buf1, m_fftKernels[k]);
        }
#endif // USE_GRAPH_SCOPE

#ifndef USE_GRAPH_SCOPE
#ifdef USE_EXPLICIT_HEAP
        // heap_size is defined in the derived fft_ifft_dit_1ch_baseports_graph because it requires knowledge of the
        // API.
        for (int k = 0; k < TP_CASC_LEN; ++k) {
            int heapSize = 0;
#ifndef USE_GRAPH_SCOPE
            heapSize = fnHeapSize<TT_DATA, TT_TWIDDLE, TP_POINT_SIZE>();
#endif // USE_GRAPH_SCOPE
#ifdef USE_GRAPH_SCOPE
            heapSize = 1024; // sundry items
#endif                       // USE_GRAPH_SCOPE
            heap_size(this->m_fftKernels[k]) = heapSize;
        }
#endif // USE_EXPLICIT_HEAP
#endif // USE_GRAPH_SCOPE

        for (int k = 0; k < TP_CASC_LEN; ++k) {
            // Specify mapping constraints
            runtime<ratio>(m_fftKernels[k]) = 0.3;

            // Source files
            source(m_fftKernels[k]) = "fft_ifft_dit_1ch.cpp";
            headers(m_fftKernels[k]) = {"fft_ifft_dit_1ch.hpp"};
        }

        for (int k = 1; k < TP_CASC_LEN; ++k) {
            // Navigate mapper to avoid multiple cascaded FFT kernels placement on the same AIE tile, as this defeats
            // the purpose of cascading FFT.
            not_equal(location<kernel>(m_fftKernels[k - 1]), location<kernel>(m_fftKernels[k]));
        }
    };
};

//------------------------------------------------
// inheritance - used because the base class handles buffer association. This derived class handles IO ports.
template <typename TT_DATA,
          typename TT_TWIDDLE,
          unsigned int TP_POINT_SIZE,
          unsigned int TP_FFT_NIFFT,
          unsigned int TP_SHIFT,
          unsigned int TP_CASC_LEN = 1,
          unsigned int TP_DYN_PT_SIZE = 0, // backwards compatible default
          unsigned int TP_WINDOW_VSIZE = TP_POINT_SIZE,
          unsigned int TP_API = kWindowAPI>
class fft_ifft_dit_1ch_baseports_graph : public fft_ifft_dit_1ch_base_graph<TT_DATA,
                                                                            TT_TWIDDLE,
                                                                            TP_POINT_SIZE,
                                                                            TP_FFT_NIFFT,
                                                                            TP_SHIFT,
                                                                            TP_CASC_LEN,
                                                                            TP_DYN_PT_SIZE,
                                                                            TP_WINDOW_VSIZE> {
   public:
    // This is the default, for windowed ports.
    /**
   * The input data to the function. This input is a window API of
   * samples of TT_DATA type. The number of samples in the window is
   * described by TP_POINT_SIZE.
   **/
    port<input> in[1];
    /**
   * A window API of TP_POINT_SIZE samples of TT_DATA type.
   **/
    port<output> out[1];

    // Constructor
    fft_ifft_dit_1ch_baseports_graph() {
        // Make data connections
        // Size of window is in Bytes.
        connect<window<TP_WINDOW_VSIZE * sizeof(TT_DATA) + TP_DYN_PT_SIZE * 32> >(in[0], this->m_fftKernels[0].in[0]);
        connect<window<TP_WINDOW_VSIZE * sizeof(TT_DATA) + TP_DYN_PT_SIZE * 32> >(
            this->m_fftKernels[TP_CASC_LEN - 1].out[0], out[0]);
    };
};

template <typename TT_DATA,
          typename TT_TWIDDLE,
          unsigned int TP_POINT_SIZE,
          unsigned int TP_FFT_NIFFT,
          unsigned int TP_SHIFT,
          unsigned int TP_CASC_LEN,
          unsigned int TP_DYN_PT_SIZE,
          unsigned int TP_WINDOW_VSIZE>
class fft_ifft_dit_1ch_baseports_graph<TT_DATA,
                                       TT_TWIDDLE,
                                       TP_POINT_SIZE,
                                       TP_FFT_NIFFT,
                                       TP_SHIFT,
                                       TP_CASC_LEN,
                                       TP_DYN_PT_SIZE,
                                       TP_WINDOW_VSIZE,
                                       kStreamAPI> : public fft_ifft_dit_1ch_base_graph<TT_DATA,
                                                                                        TT_TWIDDLE,
                                                                                        TP_POINT_SIZE,
                                                                                        TP_FFT_NIFFT,
                                                                                        TP_SHIFT,
                                                                                        TP_CASC_LEN,
                                                                                        TP_DYN_PT_SIZE,
                                                                                        TP_WINDOW_VSIZE> {
    // This is the specialization for streaming ports.
   public:
    /**
   * I/O is two parallel streams each TT_DATA type.
   **/
    port<input> in[2];
    port<output> out[2];

    kernel m_inWidgetKernel =
        kernel::create_object<widget_api_cast<TT_DATA,
                                              kStreamAPI,
                                              kWindowAPI,
                                              2,
                                              TP_WINDOW_VSIZE + TP_DYN_PT_SIZE * 32 / sizeof(TT_DATA),
                                              1,
                                              kSampleIntlv> >();
    kernel m_outWidgetKernel =
        kernel::create_object<widget_api_cast<TT_DATA,
                                              kWindowAPI,
                                              kStreamAPI,
                                              1,
                                              TP_WINDOW_VSIZE + TP_DYN_PT_SIZE * 32 / sizeof(TT_DATA),
                                              2,
                                              kSplit> >();

    // Constructor
    fft_ifft_dit_1ch_baseports_graph() {
        // Make data connections
        // Size of window is in Bytes.
        connect<stream>(in[0], m_inWidgetKernel.in[0]);
        connect<stream>(in[1], m_inWidgetKernel.in[1]);
        connect<window<TP_WINDOW_VSIZE * sizeof(TT_DATA) + TP_DYN_PT_SIZE * 32> >(m_inWidgetKernel.out[0],
                                                                                  this->m_fftKernels[0].in[0]);
        connect<window<TP_WINDOW_VSIZE * sizeof(TT_DATA) + TP_DYN_PT_SIZE * 32> >(
            this->m_fftKernels[TP_CASC_LEN - 1].out[0], m_outWidgetKernel.in[0]);
        connect<stream>(m_outWidgetKernel.out[0], out[0]);
        connect<stream>(m_outWidgetKernel.out[1], out[1]);

        // Source files
        source(m_inWidgetKernel) = "widget_api_cast.cpp";
        headers(m_inWidgetKernel) = {"widget_api_cast.hpp"};
        source(m_outWidgetKernel) = "widget_api_cast.cpp";
        headers(m_outWidgetKernel) = {"widget_api_cast.hpp"};
        runtime<ratio>(m_inWidgetKernel) = 0.3;
        runtime<ratio>(m_outWidgetKernel) = 0.3;
#ifndef USE_GRAPH_SCOPE
#ifdef USE_EXPLICIT_HEAP
        heap_size(m_inWidgetKernel) = 100;
        heap_size(m_outWidgetKernel) = 100;
#endif // USE_EXPLICIT_HEAP
#endif // USE_GRAPH_SCOPE
    };
};

/**
 * @cond NOCOMMENTS
 */

//------------------------------------------------
// inheritance - used because only cint16 requires two internal buffers for sample storage
// fft_ifft_dit_1ch_graph is the top level of a monolithic FFT. Above this are heterogeneous FFTs (combos
// of monolithic FFT and combiner stages.)
template <typename TT_DATA,
          typename TT_TWIDDLE,
          unsigned int TP_POINT_SIZE,
          unsigned int TP_FFT_NIFFT,
          unsigned int TP_SHIFT,
          unsigned int TP_CASC_LEN = 1,
          unsigned int TP_DYN_PT_SIZE = 0, // backwards compatible default
          unsigned int TP_WINDOW_VSIZE = TP_POINT_SIZE,
          unsigned int TP_API = kWindowAPI>
class fft_ifft_dit_1ch_mono_graph : public fft_ifft_dit_1ch_baseports_graph<TT_DATA,
                                                                            TT_TWIDDLE,
                                                                            TP_POINT_SIZE,
                                                                            TP_FFT_NIFFT,
                                                                            TP_SHIFT,
                                                                            TP_CASC_LEN,
                                                                            TP_DYN_PT_SIZE,
                                                                            TP_WINDOW_VSIZE,
                                                                            TP_API> {
    // This is the default for cint32 and cfloat
};

// Default inheritance for cint16, PT_SIZE 128 or below- to include second temporary buffer.
template <typename TT_TWIDDLE,
          unsigned int TP_POINT_SIZE,
          unsigned int TP_FFT_NIFFT,
          unsigned int TP_SHIFT,
          unsigned int TP_CASC_LEN,
          unsigned int TP_DYN_PT_SIZE,
          unsigned int TP_WINDOW_VSIZE,
          unsigned int TP_API>
class fft_ifft_dit_1ch_mono_graph<cint16,
                                  TT_TWIDDLE,
                                  TP_POINT_SIZE,
                                  TP_FFT_NIFFT,
                                  TP_SHIFT,
                                  TP_CASC_LEN,
                                  TP_DYN_PT_SIZE,
                                  TP_WINDOW_VSIZE,
                                  TP_API> : public fft_ifft_dit_1ch_baseports_graph<cint16,
                                                                                    TT_TWIDDLE,
                                                                                    TP_POINT_SIZE,
                                                                                    TP_FFT_NIFFT,
                                                                                    TP_SHIFT,
                                                                                    TP_CASC_LEN,
                                                                                    TP_DYN_PT_SIZE,
                                                                                    TP_WINDOW_VSIZE,
                                                                                    TP_API> {
   public:
#ifdef USE_GRAPH_SCOPE
    parameter fft_buf4096;
    parameter fft_buf2048;
    parameter fft_buf1024;
    parameter fft_buf512;
    parameter fft_buf256;
    parameter fft_buf128;
    parameter fft_buf2;
    fft_ifft_dit_1ch_mono_graph() {
        fft_buf4096 = parameter::array(fft_4096_tmp2);
        fft_buf2048 = parameter::array(fft_2048_tmp2);
        fft_buf1024 = parameter::array(fft_1024_tmp2);
        fft_buf512 = parameter::array(fft_512_tmp2);
        fft_buf256 = parameter::array(fft_256_tmp2);
        fft_buf128 = parameter::array(fft_128_tmp2);
        switch (TP_POINT_SIZE) {
            case 4096:
                fft_buf2 = fft_buf4096;
                break;
            case 2048:
                fft_buf2 = fft_buf2048;
                break;
            case 1024:
                fft_buf2 = fft_buf1024;
                break;
            case 512:
                fft_buf2 = fft_buf512;
                break;
            case 256:
                fft_buf2 = fft_buf256;
                break;
            default:
                fft_buf2 = fft_buf128;
                break;
        }
        for (int k = 0; k < TP_CASC_LEN; k++) {
            connect<>(fft_buf2, this->m_fftKernels[k]);
        }
    }
#endif // USE_GRAPH_SCOPE
};

template <typename TT_DATA,
          typename TT_TWIDDLE,
          unsigned int TP_POINT_SIZE,
          unsigned int TP_FFT_NIFFT = 1,
          unsigned int TP_SHIFT = 0,
          unsigned int TP_CASC_LEN = 1,
          unsigned int TP_DYN_PT_SIZE = 0,
          unsigned int TP_WINDOW_VSIZE = TP_POINT_SIZE,
          unsigned int TP_API = 0,
          unsigned int TP_PARALLEL_POWER = 0>
class fft_ifft_dit_1ch_graph : public graph {
   public:
    static_assert(TP_API == kStreamAPI, "Error: Only Stream interface is supported for parallel FFT");
    static_assert(TP_PARALLEL_POWER >= 1 && TP_PARALLEL_POWER < 9,
                  "Error: TP_PARALLEL_POWER is out of supported range");

    static constexpr int kParallel_factor = 1 << TP_PARALLEL_POWER;
    static constexpr int kWindowSize = TP_WINDOW_VSIZE >> TP_PARALLEL_POWER;
    static constexpr int kNextParallelPower = TP_PARALLEL_POWER - 1;

    static constexpr int kR2Shift = TP_SHIFT > 0 ? 1 : 0;
    static constexpr int kFFTsubShift = TP_SHIFT > 0 ? TP_SHIFT - 1 : 0;

    port<input> in[2 * kParallel_factor]; // 2 streams per lane
    port<output> out[2 * kParallel_factor];

    parameter r2comb_tw_lut;

    kernel m_combInKernel[kParallel_factor];
    kernel m_r2Comb[kParallel_factor];
    kernel m_combOutKernel[kParallel_factor];

    fft_ifft_dit_1ch_graph<TT_DATA,
                           TT_TWIDDLE,
                           (TP_POINT_SIZE >> 1),
                           TP_FFT_NIFFT,
                           kFFTsubShift,
                           TP_CASC_LEN,
                           TP_DYN_PT_SIZE,
                           (TP_WINDOW_VSIZE >> 1),
                           kStreamAPI,
                           kNextParallelPower>
        FFTsubframe[2]; // fractal or recursive decomposition

    fft_ifft_dit_1ch_graph() {
        // create kernels and subgraphs
        create_combInWidget_kernels<TT_DATA, kWindowSize, TP_PARALLEL_POWER, kParallel_factor - 1>::create(
            m_combInKernel);
        create_r2comb_kernels<TT_DATA, TT_TWIDDLE, TP_POINT_SIZE, TP_FFT_NIFFT, kR2Shift, kWindowSize,
                              TP_PARALLEL_POWER, kParallel_factor - 1>::create(m_r2Comb);
        create_combOutWidget_kernels<TT_DATA, kWindowSize, TP_PARALLEL_POWER, kParallel_factor - 1>::create(
            m_combOutKernel);
        // make connections
        for (int i = 0; i < kParallel_factor; i++) {
            connect<stream>(in[2 * i], FFTsubframe[0].in[i]);     // stream connection
            connect<stream>(in[2 * i + 1], FFTsubframe[1].in[i]); // stream connection
            connect<stream>(FFTsubframe[0].out[i], m_combInKernel[i].in[0]);
            connect<stream>(FFTsubframe[1].out[i], m_combInKernel[i].in[1]);
            connect<window<kWindowSize * sizeof(TT_DATA)> >(m_combInKernel[i].out[0], m_r2Comb[i].in[0]);
            connect<window<kWindowSize * sizeof(TT_DATA)> >(m_r2Comb[i].out[0], m_combOutKernel[i].in[0]);
            connect<stream>(m_combOutKernel[i].out[0], out[i]);
            connect<stream>(m_combOutKernel[i].out[1], out[i + kParallel_factor]);
        }

        // Associate kernels with Source files and set runtime ratio
        for (int i = 0; i < kParallel_factor; i++) {
            source(m_combInKernel[i]) = "widget_api_cast.cpp";
            source(m_r2Comb[i]) = "fft_r2comb.cpp";
            source(m_combOutKernel[i]) = "widget_api_cast.cpp";
            headers(m_combInKernel[i]) = {"widget_api_cast.hpp"};
            headers(m_r2Comb[i]) = {"fft_r2comb.hpp"};
            headers(m_combOutKernel[i]) = {"widget_api_cast.hpp"};
            runtime<ratio>(m_combInKernel[i]) = 0.3;
            runtime<ratio>(m_r2Comb[i]) = 0.3;
            runtime<ratio>(m_combOutKernel[i]) = 0.3;
#ifndef USE_GRAPH_SCOPE
#ifdef USE_EXPLICIT_HEAP
            heap_size(m_combInKernel[i]) = 100;
            heap_size(m_combOutKernel[i]) = 100;
#endif // USE_EXPLICIT_HEAP
#endif // USE_GRAPH_SCOPE
            // As yet, the r2comb memory has not been moved to use graph scope.
            heap_size(m_r2Comb[i]) = sizeof(TT_TWIDDLE) * (TP_POINT_SIZE >> (TP_PARALLEL_POWER + 1)) + 100;
        }
    };

}; // class

// specialization for trivial mapping, i.e. single (monolithic) FFT, window API
template <typename TT_DATA,
          typename TT_TWIDDLE,
          unsigned int TP_POINT_SIZE,
          unsigned int TP_FFT_NIFFT,
          unsigned int TP_SHIFT,
          unsigned int TP_CASC_LEN,
          unsigned int TP_DYN_PT_SIZE,
          unsigned int TP_WINDOW_VSIZE>
class fft_ifft_dit_1ch_graph<TT_DATA,
                             TT_TWIDDLE,
                             TP_POINT_SIZE,
                             TP_FFT_NIFFT,
                             TP_SHIFT,
                             TP_CASC_LEN,
                             TP_DYN_PT_SIZE,
                             TP_WINDOW_VSIZE,
                             kWindowAPI,
                             0> : public graph {
   public:
    port<input> in[1];
    port<output> out[1];
    fft_ifft_dit_1ch_mono_graph<TT_DATA,
                                TT_TWIDDLE,
                                TP_POINT_SIZE,
                                TP_FFT_NIFFT,
                                TP_SHIFT,
                                TP_CASC_LEN,
                                TP_DYN_PT_SIZE,
                                TP_WINDOW_VSIZE,
                                kWindowAPI>
        FFTwinproc;
    kernel* getKernels() { return FFTwinproc.m_fftKernels; };

    fft_ifft_dit_1ch_graph() {
        connect<>(in[0], FFTwinproc.in[0]);
        connect<>(FFTwinproc.out[0], out[0]);
    };
};
// specialization for single FFT, stream API
template <typename TT_DATA,
          typename TT_TWIDDLE,
          unsigned int TP_POINT_SIZE,
          unsigned int TP_FFT_NIFFT,
          unsigned int TP_SHIFT,
          unsigned int TP_CASC_LEN,
          unsigned int TP_DYN_PT_SIZE,
          unsigned int TP_WINDOW_VSIZE>
class fft_ifft_dit_1ch_graph<TT_DATA,
                             TT_TWIDDLE,
                             TP_POINT_SIZE,
                             TP_FFT_NIFFT,
                             TP_SHIFT,
                             TP_CASC_LEN,
                             TP_DYN_PT_SIZE,
                             TP_WINDOW_VSIZE,
                             kStreamAPI,
                             0> : public graph {
   public:
    port<input> in[2]; // dual streams
    port<output> out[2];
    fft_ifft_dit_1ch_mono_graph<TT_DATA,
                                TT_TWIDDLE,
                                TP_POINT_SIZE,
                                TP_FFT_NIFFT,
                                TP_SHIFT,
                                TP_CASC_LEN,
                                TP_DYN_PT_SIZE,
                                TP_WINDOW_VSIZE,
                                kStreamAPI>
        FFTstrproc;
    fft_ifft_dit_1ch_graph() {
        connect<>(in[0], FFTstrproc.in[0]);
        connect<>(in[1], FFTstrproc.in[1]);
        connect<>(FFTstrproc.out[0], out[0]);
        connect<>(FFTstrproc.out[1], out[1]);
    };
};

/**
  * @endcond
  */

} // namespace dit_1ch
} // namespace fft
} // namespace aie
} // namespace dsp
} // namespace xf

#endif // _DSPLIB_FFT_IFFT_DIT_1CH_GRAPH_HPP_
