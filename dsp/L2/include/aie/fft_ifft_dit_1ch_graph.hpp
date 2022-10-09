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
#ifndef _DSPLIB_FFT_IFFT_DIT_1CH_GRAPH_HPP_
#define _DSPLIB_FFT_IFFT_DIT_1CH_GRAPH_HPP_

#include <adf.h>
#include <vector>
#include "graph_utils.hpp"
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
// Recursive kernel creatio for cascaded fft kernels
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
          unsigned int TP_WINDOW_VSIZE = TP_POINT_SIZE,
          unsigned int TP_ORIG_PAR_POWER = 0>
class create_casc_kernel_recur {
   public:
    static void create(kernel (&fftKernels)[TP_CASC_LEN]) {
        static constexpr int kRawStartRank = (int)TP_END_RANK - (int)TP_RANKS_PER_KERNEL;
        static constexpr unsigned int TP_START_RANK = kRawStartRank < 0 ? 0 : kRawStartRank;

        fftKernels[dim - 1] = kernel::create_object<
            fft_ifft_dit_1ch<TT_INT_DATA, TT_INT_DATA, TT_TWIDDLE, TP_POINT_SIZE, TP_FFT_NIFFT, TP_SHIFT, TP_START_RANK,
                             TP_END_RANK, TP_DYN_PT_SIZE, TP_WINDOW_VSIZE, TP_ORIG_PAR_POWER> >();
        create_casc_kernel_recur<dim - 1, TT_DATA, TT_INT_DATA, TT_TWIDDLE, TP_POINT_SIZE, TP_FFT_NIFFT, TP_SHIFT,
                                 TP_CASC_LEN, TP_START_RANK, TP_RANKS_PER_KERNEL, TP_DYN_PT_SIZE, TP_WINDOW_VSIZE,
                                 TP_ORIG_PAR_POWER>::create(fftKernels);
    }
};
// Recursive fft kernel creation
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
          unsigned int TP_WINDOW_VSIZE,
          unsigned int TP_ORIG_PAR_POWER>
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
                               TP_WINDOW_VSIZE,
                               TP_ORIG_PAR_POWER> {
   public:
    static void create(kernel (&fftKernels)[TP_CASC_LEN]) {
        static constexpr unsigned int TP_START_RANK = 0;
        fftKernels[0] = kernel::create_object<
            fft_ifft_dit_1ch<TT_DATA, TT_INT_DATA, TT_TWIDDLE, TP_POINT_SIZE, TP_FFT_NIFFT, TP_SHIFT, TP_START_RANK,
                             TP_END_RANK, TP_DYN_PT_SIZE, TP_WINDOW_VSIZE, TP_ORIG_PAR_POWER> >();
    }
};
// fft Kernel creation, entry to recursion, also end of cascade.
template <int dim,
          typename TT_DATA, // type for I/O
          typename TT_TWIDDLE,
          unsigned int TP_POINT_SIZE,
          unsigned int TP_FFT_NIFFT,
          unsigned int TP_SHIFT,
          unsigned int TP_CASC_LEN,
          unsigned int TP_DYN_PT_SIZE,
          unsigned int TP_WINDOW_VSIZE = TP_POINT_SIZE,
          unsigned int TP_ORIG_PAR_POWER = 0> // invalid default because this must be set
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
                             TP_START_RANK, TP_END_RANK, TP_DYN_PT_SIZE, TP_WINDOW_VSIZE, TP_ORIG_PAR_POWER> >();
        create_casc_kernel_recur<dim - 1, TT_DATA, T_internalDataType, TT_TWIDDLE, TP_POINT_SIZE, TP_FFT_NIFFT,
                                 TP_SHIFT, TP_CASC_LEN, TP_START_RANK, TP_RANKS_PER_KERNEL, TP_DYN_PT_SIZE,
                                 TP_WINDOW_VSIZE, TP_ORIG_PAR_POWER>::create(fftKernels);
    }
};
// fft Kernel creation, entry to recursion, also end of cascade.
template <typename TT_DATA, // type for I/O
          typename TT_TWIDDLE,
          unsigned int TP_POINT_SIZE,
          unsigned int TP_FFT_NIFFT,
          unsigned int TP_SHIFT,
          unsigned int TP_DYN_PT_SIZE,
          unsigned int TP_WINDOW_VSIZE,
          unsigned int TP_ORIG_PAR_POWER>
class create_casc_kernel<1,
                         TT_DATA,
                         TT_TWIDDLE,
                         TP_POINT_SIZE,
                         TP_FFT_NIFFT,
                         TP_SHIFT,
                         1,
                         TP_DYN_PT_SIZE,
                         TP_WINDOW_VSIZE,
                         TP_ORIG_PAR_POWER> {
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
        fftKernels[0] = kernel::create_object<
            fft_ifft_dit_1ch<TT_DATA, TT_DATA, TT_TWIDDLE, TP_POINT_SIZE, TP_FFT_NIFFT, TP_SHIFT, TP_START_RANK,
                             TP_END_RANK, TP_DYN_PT_SIZE, TP_WINDOW_VSIZE, TP_ORIG_PAR_POWER> >();
    }
};

template <typename TT_DATA,
          typename TT_TWIDDLE,
          unsigned int TP_POINT_SIZE,
          unsigned int TP_FFT_NIFFT,
          unsigned int TP_SHIFT,
          unsigned int TP_DYN_PT_SIZE,
          unsigned int TP_WINDOW_VSIZE,
          unsigned int TP_PARALLEL_POWER,
          unsigned int TP_INDEX,
          unsigned int TP_ORIG_PAR_POWER>
class create_r2comb_kernels {
   public:
    static constexpr int kParallel_factor = 1 << TP_PARALLEL_POWER;
    static void create(kernel (&m_r2Comb)[kParallel_factor]) {
        m_r2Comb[TP_INDEX] =
            kernel::create_object<fft_r2comb<TT_DATA, TT_TWIDDLE, TP_POINT_SIZE, TP_FFT_NIFFT, TP_SHIFT, TP_DYN_PT_SIZE,
                                             TP_WINDOW_VSIZE, TP_PARALLEL_POWER, TP_INDEX, TP_ORIG_PAR_POWER> >();
        create_r2comb_kernels<TT_DATA, TT_TWIDDLE, TP_POINT_SIZE, TP_FFT_NIFFT, TP_SHIFT, TP_DYN_PT_SIZE,
                              TP_WINDOW_VSIZE, TP_PARALLEL_POWER, (TP_INDEX - 1), TP_ORIG_PAR_POWER>::create(m_r2Comb);
    }
};

template <typename TT_DATA,
          typename TT_TWIDDLE,
          unsigned int TP_POINT_SIZE,
          unsigned int TP_FFT_NIFFT,
          unsigned int TP_SHIFT,
          unsigned int TP_DYN_PT_SIZE,
          unsigned int TP_WINDOW_VSIZE,
          unsigned int TP_PARALLEL_POWER,
          unsigned int TP_ORIG_PAR_POWER>
class create_r2comb_kernels<TT_DATA,
                            TT_TWIDDLE,
                            TP_POINT_SIZE,
                            TP_FFT_NIFFT,
                            TP_SHIFT,
                            TP_DYN_PT_SIZE,
                            TP_WINDOW_VSIZE,
                            TP_PARALLEL_POWER,
                            0,
                            TP_ORIG_PAR_POWER> {
   public:
    static constexpr int kParallel_factor = 1 << TP_PARALLEL_POWER;
    static void create(kernel (&m_r2Comb)[kParallel_factor]) {
        m_r2Comb[0] =
            kernel::create_object<fft_r2comb<TT_DATA, TT_TWIDDLE, TP_POINT_SIZE, TP_FFT_NIFFT, TP_SHIFT, TP_DYN_PT_SIZE,
                                             TP_WINDOW_VSIZE, TP_PARALLEL_POWER, 0, TP_ORIG_PAR_POWER> >();
    }
};

template <typename TT_DATA,
          unsigned int TP_WINDOW_VSIZE,
          unsigned int TP_PARALLEL_POWER,
          unsigned int TP_INDEX,
          unsigned int TP_HEADER_BYTES>
class create_combInWidget_kernels {
   public:
    static constexpr int kParallel_factor = 1 << TP_PARALLEL_POWER;
    static void create(kernel (&m_combInKernel)[kParallel_factor]) {
        m_combInKernel[TP_INDEX] = kernel::create_object<
            widget_api_cast<TT_DATA, kStreamAPI, kWindowAPI, 2, TP_WINDOW_VSIZE, 1, kSampleIntlv, TP_HEADER_BYTES> >();
        create_combInWidget_kernels<TT_DATA, TP_WINDOW_VSIZE, TP_PARALLEL_POWER, (TP_INDEX - 1),
                                    TP_HEADER_BYTES>::create(m_combInKernel);
    }
};

template <typename TT_DATA, unsigned int TP_WINDOW_VSIZE, unsigned int TP_PARALLEL_POWER, unsigned int TP_HEADER_BYTES>
class create_combInWidget_kernels<TT_DATA, TP_WINDOW_VSIZE, TP_PARALLEL_POWER, 0, TP_HEADER_BYTES> {
   public:
    static constexpr int kParallel_factor = 1 << TP_PARALLEL_POWER;
    static void create(kernel (&m_combInKernel)[kParallel_factor]) {
        m_combInKernel[0] = kernel::create_object<
            widget_api_cast<TT_DATA, kStreamAPI, kWindowAPI, 2, TP_WINDOW_VSIZE, 1, kSampleIntlv, TP_HEADER_BYTES> >();
    }
};

template <typename TT_DATA,
          unsigned int TP_WINDOW_VSIZE,
          unsigned int TP_PARALLEL_POWER,
          unsigned int TP_INDEX,
          unsigned int TP_HEADER_BYTES>
class create_combOutWidget_kernels {
   public:
    static constexpr int kParallel_factor = 1 << TP_PARALLEL_POWER;
    static void create(kernel (&m_combOutKernel)[kParallel_factor]) {
        m_combOutKernel[TP_INDEX] = kernel::create_object<
            widget_api_cast<TT_DATA, kWindowAPI, kStreamAPI, 1, TP_WINDOW_VSIZE, 2, kSampleIntlv, TP_HEADER_BYTES> >();
        create_combOutWidget_kernels<TT_DATA, TP_WINDOW_VSIZE, TP_PARALLEL_POWER, (TP_INDEX - 1),
                                     TP_HEADER_BYTES>::create(m_combOutKernel);
    }
};

template <typename TT_DATA, unsigned int TP_WINDOW_VSIZE, unsigned int TP_PARALLEL_POWER, unsigned int TP_HEADER_BYTES>
class create_combOutWidget_kernels<TT_DATA, TP_WINDOW_VSIZE, TP_PARALLEL_POWER, 0, TP_HEADER_BYTES> {
   public:
    static constexpr int kParallel_factor = 1 << TP_PARALLEL_POWER;
    static void create(kernel (&m_combOutKernel)[kParallel_factor]) {
        m_combOutKernel[0] = kernel::create_object<
            widget_api_cast<TT_DATA, kWindowAPI, kStreamAPI, 1, TP_WINDOW_VSIZE, 2, kSampleIntlv, TP_HEADER_BYTES> >();
    }
};

//---------End of recursive code.

/**
  * @endcond
  */

/**
 * @cond NOCOMMENTS
 */
template <typename TT_DATA,
          typename TT_TWIDDLE,
          unsigned int TP_POINT_SIZE,
          unsigned int TP_FFT_NIFFT,
          unsigned int TP_SHIFT,
          unsigned int TP_CASC_LEN = 1,
          unsigned int TP_DYN_PT_SIZE = 0, // backwards compatible default
          unsigned int TP_WINDOW_VSIZE = TP_POINT_SIZE,
          unsigned int TP_ORIG_PAR_POWER = 0> // invalid default because this must be set.
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

    // Constructor
    fft_ifft_dit_1ch_base_graph() {
        static constexpr int kHeaderBytes = TP_DYN_PT_SIZE > 0 ? 32 : 0;
        typedef
            typename std::conditional<std::is_same<TT_DATA, cint16>::value, cint32_t, TT_DATA>::type T_internalDataType;

        // Create kernel class(s)
        create_casc_kernel<TP_CASC_LEN, TT_DATA, TT_TWIDDLE, TP_POINT_SIZE, TP_FFT_NIFFT, TP_SHIFT, TP_CASC_LEN,
                           TP_DYN_PT_SIZE, TP_WINDOW_VSIZE, TP_ORIG_PAR_POWER>::create(m_fftKernels);

        // Make kernel to kernel window connections
        for (int k = 0; k < TP_CASC_LEN - 1; ++k) {
            connect<window<TP_WINDOW_VSIZE * sizeof(T_internalDataType) + kHeaderBytes> >(m_fftKernels[k].out[0],
                                                                                          m_fftKernels[k + 1].in[0]);
            if (TP_POINT_SIZE * sizeof(TT_DATA) >= 8192) { // This would exceed data memory limits for AIE1.
                single_buffer(m_fftKernels[k + 1].in[0]);
            }
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
          unsigned int TP_API = kWindowAPI,
          unsigned int TP_ORIG_PAR_POWER = 0> // invalid default because this must be set.
class fft_ifft_dit_1ch_baseports_graph : public fft_ifft_dit_1ch_base_graph<TT_DATA,
                                                                            TT_TWIDDLE,
                                                                            TP_POINT_SIZE,
                                                                            TP_FFT_NIFFT,
                                                                            TP_SHIFT,
                                                                            TP_CASC_LEN,
                                                                            TP_DYN_PT_SIZE,
                                                                            TP_WINDOW_VSIZE,
                                                                            TP_ORIG_PAR_POWER> {
   public:
    port_array<input, 1> in;
    port_array<output, 1> out;

    // Constructor
    fft_ifft_dit_1ch_baseports_graph() {
        // Make data connections
        // Size of window is in Bytes.
        static constexpr int kHeaderBytes = TP_DYN_PT_SIZE > 0 ? 32 : 0;
        connect<window<TP_WINDOW_VSIZE * sizeof(TT_DATA) + kHeaderBytes> >(in[0], this->m_fftKernels[0].in[0]);
        connect<window<TP_WINDOW_VSIZE * sizeof(TT_DATA) + kHeaderBytes> >(this->m_fftKernels[TP_CASC_LEN - 1].out[0],
                                                                           out[0]);
    };
};

template <typename TT_DATA,
          typename TT_TWIDDLE,
          unsigned int TP_POINT_SIZE,
          unsigned int TP_FFT_NIFFT,
          unsigned int TP_SHIFT,
          unsigned int TP_CASC_LEN,
          unsigned int TP_DYN_PT_SIZE,
          unsigned int TP_WINDOW_VSIZE,
          unsigned int TP_ORIG_PAR_POWER>
class fft_ifft_dit_1ch_baseports_graph<TT_DATA,
                                       TT_TWIDDLE,
                                       TP_POINT_SIZE,
                                       TP_FFT_NIFFT,
                                       TP_SHIFT,
                                       TP_CASC_LEN,
                                       TP_DYN_PT_SIZE,
                                       TP_WINDOW_VSIZE,
                                       kStreamAPI,
                                       TP_ORIG_PAR_POWER> : public fft_ifft_dit_1ch_base_graph<TT_DATA,
                                                                                               TT_TWIDDLE,
                                                                                               TP_POINT_SIZE,
                                                                                               TP_FFT_NIFFT,
                                                                                               TP_SHIFT,
                                                                                               TP_CASC_LEN,
                                                                                               TP_DYN_PT_SIZE,
                                                                                               TP_WINDOW_VSIZE,
                                                                                               TP_ORIG_PAR_POWER> {
    // This is the specialization for streaming ports.
   public:
    /**
   * I/O is two parallel streams each TT_DATA type.
   **/
    port_array<input, 2> in;
    port_array<output, 2> out;

    static constexpr int kHeaderBytes = TP_DYN_PT_SIZE > 0 ? 32 : 0;

    kernel m_inWidgetKernel = kernel::create_object<
        widget_api_cast<TT_DATA, kStreamAPI, kWindowAPI, 2, TP_WINDOW_VSIZE, 1, kSampleIntlv, kHeaderBytes> >();
    kernel m_outWidgetKernel = kernel::create_object<
        widget_api_cast<TT_DATA, kWindowAPI, kStreamAPI, 1, TP_WINDOW_VSIZE, 2, kSampleIntlv, kHeaderBytes> >();

    // Constructor
    fft_ifft_dit_1ch_baseports_graph() {
        // Make data connections
        // Size of window is in Bytes.
        connect<stream>(in[0], m_inWidgetKernel.in[0]);
        connect<stream>(in[1], m_inWidgetKernel.in[1]);
        connect<window<TP_WINDOW_VSIZE * sizeof(TT_DATA) + kHeaderBytes> >(m_inWidgetKernel.out[0],
                                                                           this->m_fftKernels[0].in[0]);
        connect<window<TP_WINDOW_VSIZE * sizeof(TT_DATA) + kHeaderBytes> >(this->m_fftKernels[TP_CASC_LEN - 1].out[0],
                                                                           m_outWidgetKernel.in[0]);
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
          unsigned int TP_API = kWindowAPI,
          unsigned int TP_ORIG_PAR_POWER = 0> // invalid default because this must be set.
class fft_ifft_dit_1ch_mono_graph : public fft_ifft_dit_1ch_baseports_graph<TT_DATA,
                                                                            TT_TWIDDLE,
                                                                            TP_POINT_SIZE,
                                                                            TP_FFT_NIFFT,
                                                                            TP_SHIFT,
                                                                            TP_CASC_LEN,
                                                                            TP_DYN_PT_SIZE,
                                                                            TP_WINDOW_VSIZE,
                                                                            TP_API,
                                                                            TP_ORIG_PAR_POWER> {
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
          unsigned int TP_API,
          unsigned int TP_ORIG_PAR_POWER>
class fft_ifft_dit_1ch_mono_graph<cint16,
                                  TT_TWIDDLE,
                                  TP_POINT_SIZE,
                                  TP_FFT_NIFFT,
                                  TP_SHIFT,
                                  TP_CASC_LEN,
                                  TP_DYN_PT_SIZE,
                                  TP_WINDOW_VSIZE,
                                  TP_API,
                                  TP_ORIG_PAR_POWER> : public fft_ifft_dit_1ch_baseports_graph<cint16,
                                                                                               TT_TWIDDLE,
                                                                                               TP_POINT_SIZE,
                                                                                               TP_FFT_NIFFT,
                                                                                               TP_SHIFT,
                                                                                               TP_CASC_LEN,
                                                                                               TP_DYN_PT_SIZE,
                                                                                               TP_WINDOW_VSIZE,
                                                                                               TP_API,
                                                                                               TP_ORIG_PAR_POWER> {
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

/**
  * @endcond
  */

/**
 * @defgroup fft_graphs FFT IFFT
 *
 * The FFT/IFFT graph is offered as a template class that is available with 2 template specializations,
 * that offer varied features and interfaces:
 * - window interface (TP_API == 0) or
 * - stream interface (TP_API == 1).
 *
 */

//--------------------------------------------------------------------------------------------------
// fft_dit_1ch template
//--------------------------------------------------------------------------------------------------
/**
 * @ingroup fft_graphs
 *
 * @brief fft_dit_1ch is a single-channel, decimation-in-time, fixed point size FFT.
 *
 * This class definition is only used with stream interfaces (TP_API == 1).
 * Stream interface FFT graph is offered with a dual input stream configuration,
 * which interleaves data samples betwwen the streams.
 * Stream interface FFT implementation is capable of supporting parallel computation (TP_PARALLEL_POWER > 0).
 * Dynamic point size, with a header embedded in the data stream.
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
 * @tparam TP_POINT_SIZE is an unsigned integer which describes the number of samples in
 *         the transform. \n This must be 2^N where N is an integer in the range
 *         4 to 16 inclusive. \n When TP_DYN_PT_SIZE is set, TP_POINT_SIZE describes the maximum
 *         point size possible.
 * @tparam TP_FFT_NIFFT selects whether the transform to perform is an FFT (1) or IFFT (0).
 * @tparam TP_SHIFT selects the power of 2 to scale the result by prior to output.
 * @tparam TP_CASC_LEN selects the number of kernels the FFT will be divided over in series
 *         to improve throughput
 * @tparam TP_DYN_PT_SIZE selects whether (1) or not (0) to use run-time point size determination. \n
 *         When set, each window of data must be preceeded, in the window, by a 256 bit header. \n
 *         This header is 8 samples when TT_DATA is cint16 and 4 samples otherwise.\n
 *         The real part of the first sample indicates the forward (1) or inverse (0) transform. \n
 *         The real part of the second sample indicates the Radix2 power of the point size. \n
 *         e.g. for a 512 point size, this field would hold 9, as 2^9 = 512.
 *         The second least significant byte  8 bits of this field describe the Radix 2 power of the following \n
 *         frame. e.g. for a 512 point size, this field would hold 9, as 2^9 = 512. \n
 *         Any value below 4 or greater than log2(TP_POINT_SIZE) is considered illegal. \n
 *         The output window will also be preceeded by a 256 bit vector which is a copy of the input \n
 *         vector, but for the real part of the top sample, which is 0 to indicate a legal frame or 1 to \n
 *         indicate an illegal frame. \n
 *         When TP_PARALLEL_POWER is greater than 0, the header must be applied before each window of data \n
 *         for every port of the design and will appears before each window of data on the output ports. \n
 *         Note that the minimum point size of 16 applies to each lane when in parallel mode, so a configuration \n
 *         of point size 256 with TP_PARALLEL_POWER = 2 will have 4 lanes each with a minimum of 16 so the minimum \n
 *         legal point size here is 64.
 * @tparam TP_WINDOW_VSIZE is an unsigned integer which describes the number of samples to be processed in each call \n
 *         to the function. When TP_DYN_PT_SIZE is set to 1 the actual window size will be larger than TP_WINDOW_VSIZE
 *\n
 *         because the header is not included in TP_WINDOW_VSIZE. \n
 *         By default, TP_WINDOW_SIZE is set to match TP_POINT_SIZE. \n
 *         TP_WINDOW_SIZE may be set to be an integer multiple of the TP_POINT_SIZE, in which case
 *         multiple FFT iterations will be performed on a given input window, resulting in multiple
 *         iterations of output samples, reducing the numer of times the kernel needs to be triggered to
 *         process a given number of input data samples. \n
 *         As a result, the overheads inferred during kernel triggering are reduced and overall performance
 *         is increased.
 * @tparam TP_API is an unsigned integer to select window (0) or stream (1) interfaces.
 *         When stream I/O is selected, one sample is taken from, or output to, a stream and the next sample
 *         from or two the next stream. Two streams mimimum are used. In this example, even samples are
 *         read from input stream[0] and odd samples from input stream[1].
 * @tparam TP_PARALLEL_POWER is an unsigned integer to describe N where 2^N is the numbers of subframe processors
 *         to use, so as to achieve higher throughput. \n
 *         The default is 0. With TP_PARALLEL_POWER set to 2, 4 subframe processors will be used, each of which
 *         takes 2 streams in for a total of 8 streams input and output. Sample[p] must be written to stream[p modulus
 *q]
 *         where q is the number of streams.
 * @tparam TP_INDEX
 *         This parameter is for internal use regarding the recursion of the parallel power feature. It is recommended
 *         to miss this parameter from the configuration and rely instead on default values. If this parameter is set
 *         by the user, the behaviour of the library unit is undefined.
 * @tparam TP_ORIG_PAR_POWER
 *         This parameter is for internal use regarding the recursion of the parallel power feature. It is recommended
 *         to miss this parameter from the configuration and rely instead on default values. If this parameter is set
 *         by the user, the behaviour of the library unit is undefined.
 *
  **/
template <typename TT_DATA,
          typename TT_TWIDDLE,
          unsigned int TP_POINT_SIZE,
          unsigned int TP_FFT_NIFFT = 1,
          unsigned int TP_SHIFT = 0,
          unsigned int TP_CASC_LEN = 1,
          unsigned int TP_DYN_PT_SIZE = 0,
          unsigned int TP_WINDOW_VSIZE = TP_POINT_SIZE,
          unsigned int TP_API = 0,
          unsigned int TP_PARALLEL_POWER = 0,
          unsigned int TP_INDEX = 0,
          unsigned int TP_ORIG_PAR_POWER = TP_PARALLEL_POWER>
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

    static constexpr int kHeaderBytes = TP_DYN_PT_SIZE > 0 ? 32 : 0;

    /**
     * The input data to the function.
     * I/O  is two parallel streams each TT_DATA type.
     **/

    port_array<input, 2 * kParallel_factor> in;

    /**
     * The output data from the function.
     * I/O  is two parallel streams each TT_DATA type.
     **/
    port_array<output, 2 * kParallel_factor> out;

    parameter r2comb_tw_lut;

    /**
     * FFT recursive decomposition. Widget kernel
     * Widgets are used to reorder data as per FFT algorithm requirements.
     **/
    kernel m_combInKernel[kParallel_factor];

    /**
     * FFT recursive decomposition. R2Combiner kernel
     * R2combiner kernels connect 2 subframe processors with next stage's r2combiner kernels
     **/
    kernel m_r2Comb[kParallel_factor];

    /**
     * FFT recursive decomposition. Widget kernel
     * Widgets are used to reorder data as per FFT algorithm requirements.
     **/
    kernel m_combOutKernel[kParallel_factor];

    /**
     * FFT recursive decomposition. Subframe0
     * FFT is split into 2 subframe processors with a stage of r2cominers connecting subframe processors.
     * This is a recursive call with decrementing TP_PARALLEL_POWER template parameter.
     **/
    fft_ifft_dit_1ch_graph<TT_DATA,
                           TT_TWIDDLE,
                           (TP_POINT_SIZE >> 1),
                           TP_FFT_NIFFT,
                           kFFTsubShift,
                           TP_CASC_LEN,
                           TP_DYN_PT_SIZE,
                           (TP_WINDOW_VSIZE >> 1),
                           kStreamAPI,
                           kNextParallelPower,
                           TP_INDEX,
                           TP_ORIG_PAR_POWER>
        FFTsubframe0;

    /**
     * FFT recursive decomposition. Subframe1.
     * FFT is split into 2 subframe processors with a stage of r2cominers connecting subframe processors.
     * This is a recursive call with decrementing TP_PARALLEL_POWER template parameter.
     **/
    fft_ifft_dit_1ch_graph<TT_DATA,
                           TT_TWIDDLE,
                           (TP_POINT_SIZE >> 1),
                           TP_FFT_NIFFT,
                           kFFTsubShift,
                           TP_CASC_LEN,
                           TP_DYN_PT_SIZE,
                           (TP_WINDOW_VSIZE >> 1),
                           kStreamAPI,
                           kNextParallelPower,
                           TP_INDEX + kParallel_factor / 2,
                           TP_ORIG_PAR_POWER>
        FFTsubframe1;

    /**
     * @brief This is the constructor function for the Single channel DIT FFT graph.
     * No arguments required
     **/
    fft_ifft_dit_1ch_graph() {
        // create kernels and subgraphs
        create_combInWidget_kernels<TT_DATA, kWindowSize, TP_PARALLEL_POWER, kParallel_factor - 1,
                                    kHeaderBytes>::create(m_combInKernel);
        create_r2comb_kernels<TT_DATA, TT_TWIDDLE, TP_POINT_SIZE, TP_FFT_NIFFT, kR2Shift, TP_DYN_PT_SIZE, kWindowSize,
                              TP_PARALLEL_POWER, kParallel_factor - 1, TP_ORIG_PAR_POWER>::create(m_r2Comb);
        create_combOutWidget_kernels<TT_DATA, kWindowSize, TP_PARALLEL_POWER, kParallel_factor - 1,
                                     kHeaderBytes>::create(m_combOutKernel);
        // make connections
        for (int i = 0; i < kParallel_factor; i++) {
            connect<stream>(in[2 * i], FFTsubframe0.in[i]);     // stream connection
            connect<stream>(in[2 * i + 1], FFTsubframe1.in[i]); // stream connection
            connect<stream>(FFTsubframe0.out[i], m_combInKernel[i].in[0]);
            connect<stream>(FFTsubframe1.out[i], m_combInKernel[i].in[1]);
            connect<window<kWindowSize * sizeof(TT_DATA) + kHeaderBytes> >(m_combInKernel[i].out[0], m_r2Comb[i].in[0]);
            connect<window<kWindowSize * sizeof(TT_DATA) + kHeaderBytes> >(m_r2Comb[i].out[0],
                                                                           m_combOutKernel[i].in[0]);
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
            // heap_size(m_r2Comb[i])= sizeof(TT_TWIDDLE) * (TP_POINT_SIZE>>(TP_PARALLEL_POWER+1)) + 100;
        }
    };

}; // class

//--------------------------------------------------------------------------------------------------
// fft_dit_1ch template specialization
//--------------------------------------------------------------------------------------------------
/**
 * @ingroup fft_graphs
 *
 * @brief fft_dit_1ch template specialization for single (monolithic) FFT, window API
 *
 * Window interface FFT graph is offered with a single input windowed, ping-pong buffer.
 * Window interface FFT implementation does not support parallel computation (TP_PARALLEL_POWER = 0 only).
 * However, dynamic point size is available (TP_DYN_PT_SIZE = 1), which allows a window buffer size to be an integer
 * multiple of the FFT's point size ( TP_WINDOW_VSIZE = N * TP_POINT_SIZE).
 * Feature offers performance improvements, particularly with small FFT graphs.
**/
template <typename TT_DATA,
          typename TT_TWIDDLE,
          unsigned int TP_POINT_SIZE,
          unsigned int TP_FFT_NIFFT,
          unsigned int TP_SHIFT,
          unsigned int TP_CASC_LEN,
          unsigned int TP_DYN_PT_SIZE,
          unsigned int TP_WINDOW_VSIZE,
          unsigned int TP_INDEX,
          unsigned int TP_ORIG_PAR_POWER>
class fft_ifft_dit_1ch_graph<TT_DATA,
                             TT_TWIDDLE,
                             TP_POINT_SIZE,
                             TP_FFT_NIFFT,
                             TP_SHIFT,
                             TP_CASC_LEN,
                             TP_DYN_PT_SIZE,
                             TP_WINDOW_VSIZE,
                             kWindowAPI,
                             0,
                             TP_INDEX,
                             TP_ORIG_PAR_POWER> : public graph {
   public:
    /**
     * The input data to the function. This input is a window API of
     * samples of TT_DATA type. The number of samples in the window is
     * described by TP_POINT_SIZE.
     **/
    port_array<input, 1> in;

    /**
     * A window API of TP_POINT_SIZE samples of TT_DATA type.
     **/
    port_array<output, 1> out;

    /**
     * Monolithic FFT block.
     **/
    fft_ifft_dit_1ch_mono_graph<TT_DATA,
                                TT_TWIDDLE,
                                TP_POINT_SIZE,
                                TP_FFT_NIFFT,
                                TP_SHIFT,
                                TP_CASC_LEN,
                                TP_DYN_PT_SIZE,
                                TP_WINDOW_VSIZE,
                                kWindowAPI,
                                TP_ORIG_PAR_POWER>
        FFTwinproc;

    /**
     * Access function to get pointer to kernel (or first kernel in a chained configuration).
     **/
    kernel* getKernels() { return FFTwinproc.m_fftKernels; };

    /**
     * @brief This is the constructor function for the Single channel DIT FFT graph.
     * No arguments required
     **/
    fft_ifft_dit_1ch_graph() {
        connect<>(in[0], FFTwinproc.in[0]);
        connect<>(FFTwinproc.out[0], out[0]);
    };
};

//--------------------------------------------------------------------------------------------------
// fft_dit_1ch template specialization
//--------------------------------------------------------------------------------------------------
/**
 * @ingroup fft_graphs
 *
 * @brief fft_dit_1ch template specialization for single FFT, stream API.
 * This FFT block is the last call (specialization TP_PARALLEL_POWER = 0 )
 * of a recursive SSR FFT call, i.e.
 * this is the last subframe processor called when TP_PARALLEL_POWER >= 1.
**/
template <typename TT_DATA,
          typename TT_TWIDDLE,
          unsigned int TP_POINT_SIZE,
          unsigned int TP_FFT_NIFFT,
          unsigned int TP_SHIFT,
          unsigned int TP_CASC_LEN,
          unsigned int TP_DYN_PT_SIZE,
          unsigned int TP_WINDOW_VSIZE,
          unsigned int TP_INDEX,
          unsigned int TP_ORIG_PAR_POWER>
class fft_ifft_dit_1ch_graph<TT_DATA,
                             TT_TWIDDLE,
                             TP_POINT_SIZE,
                             TP_FFT_NIFFT,
                             TP_SHIFT,
                             TP_CASC_LEN,
                             TP_DYN_PT_SIZE,
                             TP_WINDOW_VSIZE,
                             kStreamAPI,
                             0,
                             TP_INDEX,
                             TP_ORIG_PAR_POWER> : public graph {
   public:
    /**
     * The input data to the function.
     * I/O  is two parallel streams each TT_DATA type.
     **/
    port_array<input, 2> in; // dual streams

    /**
     * The output data from the function.
     * I/O  is two parallel streams each TT_DATA type.
     **/
    port_array<output, 2> out;

    /**
     * Monolithic FFT block.
     **/
    fft_ifft_dit_1ch_mono_graph<TT_DATA,
                                TT_TWIDDLE,
                                TP_POINT_SIZE,
                                TP_FFT_NIFFT,
                                TP_SHIFT,
                                TP_CASC_LEN,
                                TP_DYN_PT_SIZE,
                                TP_WINDOW_VSIZE,
                                kStreamAPI,
                                TP_ORIG_PAR_POWER>
        FFTstrproc;

    /**
     * @brief This is the constructor function for the Single channel DIT FFT graph.
     * No arguments required
     **/
    fft_ifft_dit_1ch_graph() {
        connect<>(in[0], FFTstrproc.in[0]);
        connect<>(in[1], FFTstrproc.in[1]);
        connect<>(FFTstrproc.out[0], out[0]);
        connect<>(FFTstrproc.out[1], out[1]);
    };
};

} // namespace dit_1ch
} // namespace fft
} // namespace aie
} // namespace dsp
} // namespace xf

#endif // _DSPLIB_FFT_IFFT_DIT_1CH_GRAPH_HPP_
