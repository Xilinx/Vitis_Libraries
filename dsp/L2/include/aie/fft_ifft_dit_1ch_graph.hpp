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
#ifndef _DSPLIB_FFT_IFFT_DIT_1CH_GRAPH_HPP_
#define _DSPLIB_FFT_IFFT_DIT_1CH_GRAPH_HPP_

#include <adf.h>
#include <vector>
#include <adf/arch/aie_arch_properties.hpp>
#include "graph_utils.hpp"
#include "fft_ifft_dit_1ch.hpp"
#include "fft_r2comb.hpp"
#include "widget_api_cast_traits.hpp" //for kWindowAPI etc

using namespace adf;
using namespace xf::dsp::aie::widget::api_cast;
using namespace xf::dsp::aie::fft::r2comb;

// Use Graph scoped buffer definitions for both haie and x86 targets.
// #ifndef __X86SIM__
#define USE_GRAPH_SCOPE
// #endif

#ifdef USE_GRAPH_SCOPE
#include "fft_bufs.h" //Extern declarations of all twiddle factor stores and rank-temporary sample stores.
#endif                // USE_GRAPH_SCOPE

namespace xf {
namespace dsp {
namespace aie {
namespace fft {
namespace dit_1ch {

/**
 * @defgroup fft_graphs FFT Graph
 *
 * The FFT graph is offered as a template class that is available with 2 template specializations,
 * that offer varied features and interfaces:
 * - window interface (TP_API == 0) or
 * - stream interface (TP_API == 1).
 *
 */

/**
 *
 * @brief Defines Window/IO Buffer API
 *
 * @ingroup fft_graphs
 */
static constexpr unsigned int kWindowAPI = 0;

/**
 *
 * @brief Defines Stream Buffer API
 *
 * @ingroup fft_graphs
 */
static constexpr unsigned int kStreamAPI = 1;

/**
 * @cond NOCOMMENTS
 */

// Utility function to determine the output API of an FFT atom (atom? - single kernel FFT which may be part of a larger
// graph FFT).
// AIE1 is easy - outAPI = TP_API for single kernel  and kStream otherwise.
// For AIE2, the column of FFTs alternate between (cascade, stream) and (stream, cascade) for the 2 outputs.
// Utility function to get API for R2 input
template <unsigned int TP_API, unsigned int TP_ORIG_PAR_POWER, unsigned int TP_PARALLEL_POWER, unsigned int TP_INDEX>
constexpr unsigned int fnGetFFTSubframeOutAPI() {
    if (TP_ORIG_PAR_POWER == 0) {
        return TP_API;
    } else {
        if
            constexpr(get_input_streams_core_module() == 2) { return kStreamAPI; }
        else {
            if
                constexpr(TP_INDEX % 2 == 0) { return kCascStreamAPI; }
            else {
                return kStreamCascAPI;
            }
        }
    }
}

// Utility function to get API for R2 input
template <unsigned int TP_API, unsigned int TP_ORIG_PAR_POWER, unsigned int TP_PARALLEL_POWER, unsigned int TP_INDEX>
constexpr unsigned int fnGetR2InAPI() {
    if
        constexpr(get_input_streams_core_module() == 2) { return kStreamAPI; }
    else {
        if
            constexpr((TP_INDEX >> (TP_PARALLEL_POWER - 1)) % 2 == 0) { return kCascStreamAPI; }
        else {
            return kStreamCascAPI;
        }
    }
}

// Utility function to get API for R2 output
template <unsigned int TP_API, unsigned int TP_ORIG_PAR_POWER, unsigned int TP_PARALLEL_POWER, unsigned int TP_INDEX>
constexpr unsigned int fnGetR2OutAPI() {
    if
        constexpr(TP_ORIG_PAR_POWER == TP_PARALLEL_POWER) { return TP_API; }
    else {
        if
            constexpr(get_input_streams_core_module() == 2) { return kStreamAPI; }
        else {
            if
                constexpr((TP_INDEX >> TP_PARALLEL_POWER) % 2 == 0) { return kCascStreamAPI; }
            else {
                return kStreamCascAPI;
            }
        }
    }
}

//---------start of recursive kernel creation code.
// Recursive kernel creation for cascaded fft kernels
// This is the specialization for kernels in the middle of the cascade.
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
          unsigned int TP_IN_API = 0,
          unsigned int TP_ORIG_PAR_POWER = 0,
          unsigned int TP_RND = 0,
          unsigned int TP_SAT = 1,
          unsigned int TP_TWIDDLE_MODE = 0>
class create_casc_kernel_recur {
   public:
    // graph scoping of large (>256 twiddle tables)
    static void create(kernel (&fftKernels)[TP_CASC_LEN], int* startRanks, int* endRanks) {
        static constexpr int kRawRanksPerKernel =
            std::is_same<TT_DATA, cfloat>::value ? (TP_END_RANK / dim) : (TP_END_RANK / dim / 2) * 2;
        static constexpr unsigned int kRanksPerKernel = std::is_same<TT_DATA, cfloat>::value
                                                            ? kRawRanksPerKernel
                                                            : (kRawRanksPerKernel < 2 ? 2 : kRawRanksPerKernel);
        static constexpr int kRawStartRank = (int)TP_END_RANK - (int)kRanksPerKernel;
        static constexpr unsigned int TP_START_RANK = kRawStartRank < 0 ? 0 : kRawStartRank;

        startRanks[dim - 1] = TP_START_RANK;
        endRanks[dim - 1] = TP_END_RANK;

        fftKernels[dim - 1] = kernel::create_object<
            fft_ifft_dit_1ch<TT_INT_DATA, TT_INT_DATA, TT_TWIDDLE, TP_POINT_SIZE, TP_FFT_NIFFT, TP_SHIFT, TP_START_RANK,
                             TP_END_RANK, TP_DYN_PT_SIZE, TP_WINDOW_VSIZE, 0, 0, TP_ORIG_PAR_POWER, TP_RND, TP_SAT,
                             TP_TWIDDLE_MODE> >(); // 0's are for window connection for internal cascade connections
        create_casc_kernel_recur<dim - 1, TT_DATA, TT_INT_DATA, TT_TWIDDLE, TP_POINT_SIZE, TP_FFT_NIFFT, TP_SHIFT,
                                 TP_CASC_LEN, TP_START_RANK, kRanksPerKernel, TP_DYN_PT_SIZE, TP_WINDOW_VSIZE,
                                 TP_IN_API, TP_ORIG_PAR_POWER, TP_RND, TP_SAT, TP_TWIDDLE_MODE>::create(fftKernels,
                                                                                                        startRanks,
                                                                                                        endRanks);

    } // end of create
};
// Recursive fft kernel creation
// This is the specialization for the end of recursion (first kernel in cascade)
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
          unsigned int TP_IN_API,
          unsigned int TP_ORIG_PAR_POWER,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_TWIDDLE_MODE>
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
                               TP_IN_API,
                               TP_ORIG_PAR_POWER,
                               TP_RND,
                               TP_SAT,
                               TP_TWIDDLE_MODE> {
   public:
    static void create(kernel (&fftKernels)[TP_CASC_LEN], int* startRanks, int* endRanks) {
        static constexpr unsigned int TP_START_RANK = 0;
        if
            constexpr(TP_IN_API != 0) { // AIE2 clause needed?
                std::vector<TT_DATA> inBuff;
                inBuff.resize(TP_WINDOW_VSIZE + TP_DYN_PT_SIZE * __ALIGN_BYTE_SIZE__ / sizeof(TT_DATA));
                fftKernels[0] = kernel::create_object<fft_ifft_dit_1ch<
                    TT_DATA, TT_INT_DATA, TT_TWIDDLE, TP_POINT_SIZE, TP_FFT_NIFFT, TP_SHIFT, TP_START_RANK, TP_END_RANK,
                    TP_DYN_PT_SIZE, TP_WINDOW_VSIZE, TP_IN_API, 0 /*window for cascade connection*/, TP_ORIG_PAR_POWER,
                    TP_RND, TP_SAT, TP_TWIDDLE_MODE> >(inBuff);
            }
        else {
            fftKernels[0] =
                kernel::create_object<fft_ifft_dit_1ch<TT_DATA, TT_INT_DATA, TT_TWIDDLE, TP_POINT_SIZE, TP_FFT_NIFFT,
                                                       TP_SHIFT, TP_START_RANK, TP_END_RANK, TP_DYN_PT_SIZE,
                                                       TP_WINDOW_VSIZE, TP_IN_API, 0 /*window for cascade connection*/,
                                                       TP_ORIG_PAR_POWER, TP_RND, TP_SAT, TP_TWIDDLE_MODE> >();
        }

        startRanks[0] = TP_START_RANK;
        endRanks[0] = TP_END_RANK;

    } // end of create
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
          unsigned int TP_IN_API = 0,
          unsigned int TP_OUT_API = 0,
          unsigned int TP_ORIG_PAR_POWER = 0, // invalid default because this must be set
          unsigned int TP_RND = 0,
          unsigned int TP_SAT = 1,
          unsigned int TP_TWIDDLE_MODE = 0,
          typename TT_OUT_DATA = TT_DATA>
class create_casc_kernel {
   public:
    typedef typename std::conditional<std::is_same<TT_DATA, cint16>::value, cint32_t, TT_DATA>::type T_internalDataType;
    static void create(kernel (&fftKernels)[TP_CASC_LEN], int* startRanks, int* endRanks) {
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

        startRanks[dim - 1] = TP_START_RANK;
        endRanks[dim - 1] = TP_END_RANK;

        if
            constexpr(TP_OUT_API != 0) {
                std::vector<TT_OUT_DATA> outBuff;
                outBuff.resize(TP_WINDOW_VSIZE + TP_DYN_PT_SIZE * __ALIGN_BYTE_SIZE__ / sizeof(TT_OUT_DATA));
                fftKernels[dim - 1] = kernel::create_object<fft_ifft_dit_1ch<
                    T_internalDataType, TT_OUT_DATA, TT_TWIDDLE, TP_POINT_SIZE, TP_FFT_NIFFT, TP_SHIFT, TP_START_RANK,
                    TP_END_RANK, TP_DYN_PT_SIZE, TP_WINDOW_VSIZE, 0 /*window for cascade connection*/, TP_OUT_API,
                    TP_ORIG_PAR_POWER, TP_RND, TP_SAT, TP_TWIDDLE_MODE> >(outBuff);
            }
        else {
            fftKernels[dim - 1] = kernel::create_object<fft_ifft_dit_1ch<
                T_internalDataType, TT_OUT_DATA, TT_TWIDDLE, TP_POINT_SIZE, TP_FFT_NIFFT, TP_SHIFT, TP_START_RANK,
                TP_END_RANK, TP_DYN_PT_SIZE, TP_WINDOW_VSIZE, 0 /*window for cascade connection*/, TP_OUT_API,
                TP_ORIG_PAR_POWER, TP_RND, TP_SAT, TP_TWIDDLE_MODE> >();
        }
        create_casc_kernel_recur<dim - 1, TT_DATA, T_internalDataType, TT_TWIDDLE, TP_POINT_SIZE, TP_FFT_NIFFT,
                                 TP_SHIFT, TP_CASC_LEN, TP_START_RANK, TP_RANKS_PER_KERNEL, TP_DYN_PT_SIZE,
                                 TP_WINDOW_VSIZE, TP_IN_API, TP_ORIG_PAR_POWER, TP_RND, TP_SAT,
                                 TP_TWIDDLE_MODE>::create(fftKernels, startRanks, endRanks);

    } // end of create
};
// fft Kernel creation, Specialization for CASC_LEN=1
template <typename TT_DATA, // type for I/O
          typename TT_TWIDDLE,
          unsigned int TP_POINT_SIZE,
          unsigned int TP_FFT_NIFFT,
          unsigned int TP_SHIFT,
          unsigned int TP_DYN_PT_SIZE,
          unsigned int TP_WINDOW_VSIZE,
          unsigned int TP_IN_API,
          unsigned int TP_OUT_API,
          unsigned int TP_ORIG_PAR_POWER,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_TWIDDLE_MODE,
          typename TT_OUT_DATA>
class create_casc_kernel<1,
                         TT_DATA,
                         TT_TWIDDLE,
                         TP_POINT_SIZE,
                         TP_FFT_NIFFT,
                         TP_SHIFT,
                         1,
                         TP_DYN_PT_SIZE,
                         TP_WINDOW_VSIZE,
                         TP_IN_API,
                         TP_OUT_API,
                         TP_ORIG_PAR_POWER,
                         TP_RND,
                         TP_SAT,
                         TP_TWIDDLE_MODE,
                         TT_OUT_DATA> {
   public:
    static void create(kernel (&fftKernels)[1], int* startRanks, int* endRanks) {
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
        if
            constexpr(TP_IN_API != 0) { //
                std::vector<TT_DATA> inBuff;
                inBuff.resize(TP_WINDOW_VSIZE + TP_DYN_PT_SIZE * __ALIGN_BYTE_SIZE__ / sizeof(TT_DATA));
                if
                    constexpr(TP_OUT_API != 0) {
                        std::vector<TT_OUT_DATA> outBuff;
                        outBuff.resize(TP_WINDOW_VSIZE + TP_DYN_PT_SIZE * __ALIGN_BYTE_SIZE__ / sizeof(TT_OUT_DATA));
                        fftKernels[0] = kernel::create_object<
                            fft_ifft_dit_1ch<TT_DATA, TT_OUT_DATA, TT_TWIDDLE, TP_POINT_SIZE, TP_FFT_NIFFT, TP_SHIFT,
                                             TP_START_RANK, TP_END_RANK, TP_DYN_PT_SIZE, TP_WINDOW_VSIZE, TP_IN_API,
                                             TP_OUT_API, TP_ORIG_PAR_POWER, TP_RND, TP_SAT, TP_TWIDDLE_MODE> >(inBuff,
                                                                                                               outBuff);
                    }
                else {
                    fftKernels[0] = kernel::create_object<
                        fft_ifft_dit_1ch<TT_DATA, TT_OUT_DATA, TT_TWIDDLE, TP_POINT_SIZE, TP_FFT_NIFFT, TP_SHIFT,
                                         TP_START_RANK, TP_END_RANK, TP_DYN_PT_SIZE, TP_WINDOW_VSIZE, TP_IN_API,
                                         TP_OUT_API, TP_ORIG_PAR_POWER, TP_RND, TP_SAT, TP_TWIDDLE_MODE> >(inBuff);
                }
            }
        else {
            if
                constexpr(TP_OUT_API != 0) {
                    std::vector<TT_OUT_DATA> outBuff;
                    outBuff.resize(TP_WINDOW_VSIZE + TP_DYN_PT_SIZE * __ALIGN_BYTE_SIZE__ / sizeof(TT_OUT_DATA));
                    fftKernels[0] = kernel::create_object<
                        fft_ifft_dit_1ch<TT_DATA, TT_OUT_DATA, TT_TWIDDLE, TP_POINT_SIZE, TP_FFT_NIFFT, TP_SHIFT,
                                         TP_START_RANK, TP_END_RANK, TP_DYN_PT_SIZE, TP_WINDOW_VSIZE, TP_IN_API,
                                         TP_OUT_API, TP_ORIG_PAR_POWER, TP_RND, TP_SAT, TP_TWIDDLE_MODE> >(outBuff);
                }
            else {
                fftKernels[0] = kernel::create_object<
                    fft_ifft_dit_1ch<TT_DATA, TT_OUT_DATA, TT_TWIDDLE, TP_POINT_SIZE, TP_FFT_NIFFT, TP_SHIFT,
                                     TP_START_RANK, TP_END_RANK, TP_DYN_PT_SIZE, TP_WINDOW_VSIZE, TP_IN_API, TP_OUT_API,
                                     TP_ORIG_PAR_POWER, TP_RND, TP_SAT, TP_TWIDDLE_MODE> >();
            }
        }
        startRanks[0] = TP_START_RANK;
        endRanks[0] = TP_END_RANK;

    } // end of create
};

//-----------------------------------------
// Recursive classes for r2 combiner creation

// Base specialization
template <typename TT_DATA,
          typename TT_TWIDDLE,
          unsigned int TP_POINT_SIZE,
          unsigned int TP_FFT_NIFFT,
          unsigned int TP_SHIFT,
          unsigned int TP_DYN_PT_SIZE,
          unsigned int TP_WINDOW_VSIZE,
          unsigned int TP_PARALLEL_POWER,
          unsigned int TP_INDEX,
          unsigned int TP_INDEX_BASE, // the wider context. Allows calculation of TP_INDEX in overall design, not just
                                      // this level of recursion.
          unsigned int TP_ORIG_PAR_POWER,
          unsigned int TP_API,
          unsigned int TP_USE_WIDGETS,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_TWIDDLE_MODE>
class create_r2comb_kernels {
   public:
    static constexpr int kParallel_factor = 1 << TP_PARALLEL_POWER;
    static constexpr unsigned int kInAPI =
        fnGetR2InAPI<TP_API, TP_ORIG_PAR_POWER, TP_PARALLEL_POWER, TP_INDEX_BASE + TP_INDEX>();
    static constexpr unsigned int kOutAPI =
        fnGetR2OutAPI<TP_API, TP_ORIG_PAR_POWER, TP_PARALLEL_POWER, TP_INDEX_BASE + TP_INDEX>();
    static void create(kernel (&m_r2Comb)[kParallel_factor]) {
        // Memories for stream to window and window to stream conversion.
        std::vector<TT_DATA> inBuff, outBuff;
        inBuff.resize(TP_WINDOW_VSIZE + TP_DYN_PT_SIZE * __ALIGN_BYTE_SIZE__ / sizeof(TT_DATA));

        if (kOutAPI == kWindowAPI) { // no need for outbuff if output is a window anyway
            m_r2Comb[TP_INDEX] =
                kernel::create_object<fft_r2comb<TT_DATA, TT_TWIDDLE, TP_POINT_SIZE, TP_FFT_NIFFT, TP_SHIFT,
                                                 TP_DYN_PT_SIZE, TP_WINDOW_VSIZE, TP_PARALLEL_POWER, TP_INDEX,
                                                 TP_ORIG_PAR_POWER, kInAPI, kOutAPI, TP_RND, TP_SAT, TP_TWIDDLE_MODE> >(
                    inBuff);
        } else { // need and output buffer too.
            outBuff.resize(TP_WINDOW_VSIZE + TP_DYN_PT_SIZE * __ALIGN_BYTE_SIZE__ / sizeof(TT_DATA));
            m_r2Comb[TP_INDEX] =
                kernel::create_object<fft_r2comb<TT_DATA, TT_TWIDDLE, TP_POINT_SIZE, TP_FFT_NIFFT, TP_SHIFT,
                                                 TP_DYN_PT_SIZE, TP_WINDOW_VSIZE, TP_PARALLEL_POWER, TP_INDEX,
                                                 TP_ORIG_PAR_POWER, kInAPI, kOutAPI, TP_RND, TP_SAT, TP_TWIDDLE_MODE> >(
                    inBuff, outBuff);
        }
        create_r2comb_kernels<TT_DATA, TT_TWIDDLE, TP_POINT_SIZE, TP_FFT_NIFFT, TP_SHIFT, TP_DYN_PT_SIZE,
                              TP_WINDOW_VSIZE, TP_PARALLEL_POWER, (TP_INDEX - 1), TP_INDEX_BASE, TP_ORIG_PAR_POWER,
                              TP_API, TP_USE_WIDGETS, TP_RND, TP_SAT, TP_TWIDDLE_MODE>::create(m_r2Comb);
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
          unsigned int TP_INDEX_BASE, // the wider context. Allows calculation of TP_INDEX in overal design, not just
                                      // this level of recursion.
          unsigned int TP_ORIG_PAR_POWER,
          unsigned int TP_API,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_TWIDDLE_MODE>
class create_r2comb_kernels<TT_DATA,
                            TT_TWIDDLE,
                            TP_POINT_SIZE,
                            TP_FFT_NIFFT,
                            TP_SHIFT,
                            TP_DYN_PT_SIZE,
                            TP_WINDOW_VSIZE,
                            TP_PARALLEL_POWER,
                            TP_INDEX,
                            TP_INDEX_BASE,
                            TP_ORIG_PAR_POWER,
                            TP_API,
                            1 /*USE_WIDGETS*/,
                            TP_RND,
                            TP_SAT,
                            TP_TWIDDLE_MODE> {
   public:
    static constexpr int kParallel_factor = 1 << TP_PARALLEL_POWER;
    static constexpr unsigned int kInAPI = 0;
    static constexpr unsigned int kOutAPI = 0;
    static void create(kernel (&m_r2Comb)[kParallel_factor]) {
        m_r2Comb[TP_INDEX] =
            kernel::create_object<fft_r2comb<TT_DATA, TT_TWIDDLE, TP_POINT_SIZE, TP_FFT_NIFFT, TP_SHIFT, TP_DYN_PT_SIZE,
                                             TP_WINDOW_VSIZE, TP_PARALLEL_POWER, TP_INDEX, TP_ORIG_PAR_POWER, kInAPI,
                                             kOutAPI, TP_RND, TP_SAT, TP_TWIDDLE_MODE> >();
        create_r2comb_kernels<TT_DATA, TT_TWIDDLE, TP_POINT_SIZE, TP_FFT_NIFFT, TP_SHIFT, TP_DYN_PT_SIZE,
                              TP_WINDOW_VSIZE, TP_PARALLEL_POWER, (TP_INDEX - 1), TP_INDEX_BASE, TP_ORIG_PAR_POWER,
                              TP_API, 1, TP_RND, TP_SAT, TP_TWIDDLE_MODE>::create(m_r2Comb);
    }
};

// end of r2comb recursion without widgets
template <typename TT_DATA,
          typename TT_TWIDDLE,
          unsigned int TP_POINT_SIZE,
          unsigned int TP_FFT_NIFFT,
          unsigned int TP_SHIFT,
          unsigned int TP_DYN_PT_SIZE,
          unsigned int TP_WINDOW_VSIZE,
          unsigned int TP_PARALLEL_POWER,
          unsigned int TP_INDEX_BASE,
          unsigned int TP_ORIG_PAR_POWER,
          unsigned int TP_API,
          unsigned int TP_USE_WIDGETS,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_TWIDDLE_MODE>
class create_r2comb_kernels<TT_DATA,
                            TT_TWIDDLE,
                            TP_POINT_SIZE,
                            TP_FFT_NIFFT,
                            TP_SHIFT,
                            TP_DYN_PT_SIZE,
                            TP_WINDOW_VSIZE,
                            TP_PARALLEL_POWER,
                            0,
                            TP_INDEX_BASE,
                            TP_ORIG_PAR_POWER,
                            TP_API,
                            TP_USE_WIDGETS,
                            TP_RND,
                            TP_SAT,
                            TP_TWIDDLE_MODE> {
   public:
    static constexpr unsigned int TP_INDEX = 0; // for this specialization (end of recursion)
    static constexpr unsigned int kParallel_factor = 1 << TP_PARALLEL_POWER;
    static constexpr unsigned int kInAPI =
        fnGetR2InAPI<TP_API, TP_ORIG_PAR_POWER, TP_PARALLEL_POWER, TP_INDEX_BASE + TP_INDEX>();
    static constexpr unsigned int kOutAPI =
        fnGetR2OutAPI<TP_API, TP_ORIG_PAR_POWER, TP_PARALLEL_POWER, TP_INDEX_BASE + TP_INDEX>();

    static void create(kernel (&m_r2Comb)[kParallel_factor]) {
        // Memories for stream to window and window to stream conversion.
        std::vector<TT_DATA> inBuff, outBuff;
        inBuff.resize(TP_WINDOW_VSIZE + TP_DYN_PT_SIZE * __ALIGN_BYTE_SIZE__ / sizeof(TT_DATA));

        if (kOutAPI == kWindowAPI) { // no need for outbuff if output is a window anyway
            m_r2Comb[0] = kernel::create_object<
                fft_r2comb<TT_DATA, TT_TWIDDLE, TP_POINT_SIZE, TP_FFT_NIFFT, TP_SHIFT, TP_DYN_PT_SIZE, TP_WINDOW_VSIZE,
                           TP_PARALLEL_POWER, 0, TP_ORIG_PAR_POWER, kInAPI, kOutAPI, TP_RND, TP_SAT, TP_TWIDDLE_MODE> >(
                inBuff);
        } else { // need and output buffer if non-window output
            outBuff.resize(TP_WINDOW_VSIZE + TP_DYN_PT_SIZE * __ALIGN_BYTE_SIZE__ / sizeof(TT_DATA));
            m_r2Comb[0] = kernel::create_object<
                fft_r2comb<TT_DATA, TT_TWIDDLE, TP_POINT_SIZE, TP_FFT_NIFFT, TP_SHIFT, TP_DYN_PT_SIZE, TP_WINDOW_VSIZE,
                           TP_PARALLEL_POWER, 0, TP_ORIG_PAR_POWER, kInAPI, kOutAPI, TP_RND, TP_SAT, TP_TWIDDLE_MODE> >(
                inBuff, outBuff);
        }
    }
};

// end of r2comb recursion with widgets
template <typename TT_DATA,
          typename TT_TWIDDLE,
          unsigned int TP_POINT_SIZE,
          unsigned int TP_FFT_NIFFT,
          unsigned int TP_SHIFT,
          unsigned int TP_DYN_PT_SIZE,
          unsigned int TP_WINDOW_VSIZE,
          unsigned int TP_PARALLEL_POWER,
          unsigned int TP_INDEX_BASE,
          unsigned int TP_ORIG_PAR_POWER,
          unsigned int TP_API,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_TWIDDLE_MODE>
class create_r2comb_kernels<TT_DATA,
                            TT_TWIDDLE,
                            TP_POINT_SIZE,
                            TP_FFT_NIFFT,
                            TP_SHIFT,
                            TP_DYN_PT_SIZE,
                            TP_WINDOW_VSIZE,
                            TP_PARALLEL_POWER,
                            0,
                            TP_INDEX_BASE,
                            TP_ORIG_PAR_POWER,
                            TP_API,
                            1 /*USE_WIDGETS*/,
                            TP_RND,
                            TP_SAT,
                            TP_TWIDDLE_MODE> {
   public:
    static constexpr unsigned int TP_INDEX = 0; // for this specialization (end of recursion)
    static constexpr unsigned int kParallel_factor = 1 << TP_PARALLEL_POWER;
    static constexpr unsigned int kInAPI = 0;
    static constexpr unsigned int kOutAPI = 0;

    static void create(kernel (&m_r2Comb)[kParallel_factor]) {
        m_r2Comb[0] = kernel::create_object<
            fft_r2comb<TT_DATA, TT_TWIDDLE, TP_POINT_SIZE, TP_FFT_NIFFT, TP_SHIFT, TP_DYN_PT_SIZE, TP_WINDOW_VSIZE,
                       TP_PARALLEL_POWER, 0, TP_ORIG_PAR_POWER, kInAPI, kOutAPI, TP_RND, TP_SAT, TP_TWIDDLE_MODE> >();
    }
};

template <typename TT_DATA,
          unsigned int TP_WINDOW_VSIZE,
          unsigned int TP_PARALLEL_POWER,
          unsigned int TP_INDEX,
          unsigned int TP_INDEX_BASE, // the wider context. Allows calculation of TP_INDEX in overall design, not just
                                      // this level of recursion.
          unsigned int TP_ORIG_PAR_POWER,
          unsigned int TP_API,
          unsigned int TP_HEADER_BYTES>
class create_combInWidget_kernels {
   public:
    static constexpr int kParallel_factor = 1 << TP_PARALLEL_POWER;
    static constexpr int kStreamsPerTile = get_input_streams_core_module(); // a device trait
    static constexpr unsigned int kInAPI =
        fnGetR2InAPI<TP_API, TP_ORIG_PAR_POWER, TP_PARALLEL_POWER, TP_INDEX_BASE + TP_INDEX>();

    static void create(kernel (&m_combInKernel)[kParallel_factor]) {
        m_combInKernel[TP_INDEX] = kernel::create_object<
            widget_api_cast<TT_DATA, kInAPI, kWindowAPI, 2, TP_WINDOW_VSIZE, 1, kSampleIntlv, TP_HEADER_BYTES> >();
        create_combInWidget_kernels<TT_DATA, TP_WINDOW_VSIZE, TP_PARALLEL_POWER, (TP_INDEX - 1), TP_INDEX_BASE,
                                    TP_ORIG_PAR_POWER, TP_API, TP_HEADER_BYTES>::create(m_combInKernel);
    }
};

// end of combInWidget recursion
template <typename TT_DATA,
          unsigned int TP_WINDOW_VSIZE,
          unsigned int TP_PARALLEL_POWER,
          unsigned int TP_INDEX_BASE, // the wider context. Allows calculation of TP_INDEX in overal design, not just
                                      // this level of recursion.
          unsigned int TP_ORIG_PAR_POWER,
          unsigned int TP_API,
          unsigned int TP_HEADER_BYTES>
class create_combInWidget_kernels<TT_DATA,
                                  TP_WINDOW_VSIZE,
                                  TP_PARALLEL_POWER,
                                  0 /*TP_INDEX*/,
                                  TP_INDEX_BASE,
                                  TP_ORIG_PAR_POWER,
                                  TP_API,
                                  TP_HEADER_BYTES> {
   public:
    static constexpr int TP_INDEX = 0; // end of recursion
    static constexpr int kParallel_factor = 1 << TP_PARALLEL_POWER;
    static constexpr int kStreamsPerTile = get_input_streams_core_module(); // a device trait
    static constexpr unsigned int kInAPI =
        fnGetR2InAPI<TP_API, TP_ORIG_PAR_POWER, TP_PARALLEL_POWER, TP_INDEX_BASE + TP_INDEX>();

    static void create(kernel (&m_combInKernel)[kParallel_factor]) {
        m_combInKernel[0] = kernel::create_object<
            widget_api_cast<TT_DATA, kInAPI, kWindowAPI, 2, TP_WINDOW_VSIZE, 1, kSampleIntlv, TP_HEADER_BYTES> >();
    }
};

template <typename TT_DATA,
          unsigned int TP_WINDOW_VSIZE,
          unsigned int TP_PARALLEL_POWER,
          unsigned int TP_INDEX,
          unsigned int TP_INDEX_BASE, // the wider context. Allows calculation of TP_INDEX in overall design, not just
                                      // this level of recursion.
          unsigned int TP_HEADER_BYTES,
          unsigned int TP_ORIG_PAR_POWER,
          unsigned int TP_API>
class create_combOutWidget_kernels {
   public:
    static constexpr int kParallel_factor = 1 << TP_PARALLEL_POWER;
    static constexpr int kStreamsPerTile = get_input_streams_core_module(); // a device trait
    static constexpr unsigned int kOutAPI =
        fnGetR2OutAPI<TP_API, TP_ORIG_PAR_POWER, TP_PARALLEL_POWER, TP_INDEX_BASE + TP_INDEX>();
    static constexpr int kPortsOut = TP_PARALLEL_POWER == TP_ORIG_PAR_POWER ? kStreamsPerTile : 2;

    static void create(kernel (&m_combOutKernel)[kParallel_factor]) {
        m_combOutKernel[TP_INDEX] =
            kernel::create_object<widget_api_cast<TT_DATA, kWindowAPI, kOutAPI, 1, TP_WINDOW_VSIZE, kPortsOut,
                                                  kSampleIntlv, TP_HEADER_BYTES> >();
        create_combOutWidget_kernels<TT_DATA, TP_WINDOW_VSIZE, TP_PARALLEL_POWER, (TP_INDEX - 1), TP_INDEX_BASE,
                                     TP_HEADER_BYTES, TP_ORIG_PAR_POWER, TP_API>::create(m_combOutKernel);
    }
};

template <typename TT_DATA,
          unsigned int TP_WINDOW_VSIZE,
          unsigned int TP_PARALLEL_POWER,
          unsigned int TP_INDEX_BASE, // the wider context. Allows calculation of TP_INDEX in overal design, not just
                                      // this level of recursion.
          unsigned int TP_HEADER_BYTES,
          unsigned int TP_ORIG_PAR_POWER,
          unsigned int TP_API>
class create_combOutWidget_kernels<TT_DATA,
                                   TP_WINDOW_VSIZE,
                                   TP_PARALLEL_POWER,
                                   0,
                                   TP_INDEX_BASE,
                                   TP_HEADER_BYTES,
                                   TP_ORIG_PAR_POWER,
                                   TP_API> {
   public:
    static constexpr unsigned int TP_INDEX = 0; // for this specialization (end of recursion)
    static constexpr int kParallel_factor = 1 << TP_PARALLEL_POWER;
    static constexpr int kStreamsPerTile = get_input_streams_core_module(); // a device trait
    static constexpr unsigned int kOutAPI =
        fnGetR2OutAPI<TP_API, TP_ORIG_PAR_POWER, TP_PARALLEL_POWER, TP_INDEX_BASE + TP_INDEX>();
    static constexpr int kPortsOut = TP_PARALLEL_POWER == TP_ORIG_PAR_POWER ? kStreamsPerTile : 2;

    static void create(kernel (&m_combOutKernel)[kParallel_factor]) {
        m_combOutKernel[0] = kernel::create_object<widget_api_cast<TT_DATA, kWindowAPI, kOutAPI, 1, TP_WINDOW_VSIZE,
                                                                   kPortsOut, kSampleIntlv, TP_HEADER_BYTES> >();
    }
};

//---------End of recursive code.

/**
  * @endcond
  */

/**
 * @cond NOCOMMENTS
 */
// This base of inheritance associates twiddle tables with each kernel.
template <typename TT_DATA,
          typename TT_TWIDDLE,
          unsigned int TP_POINT_SIZE,
          unsigned int TP_FFT_NIFFT,
          unsigned int TP_SHIFT,
          unsigned int TP_CASC_LEN = 1,
          unsigned int TP_DYN_PT_SIZE = 0, // backwards compatible default
          unsigned int TP_WINDOW_VSIZE = TP_POINT_SIZE,
          unsigned int TP_IN_API = 0,
          unsigned int TP_OUT_API = 0,
          unsigned int TP_ORIG_PAR_POWER = 0, // invalid default because this must be set.
          unsigned int TP_RND = 0,
          unsigned int TP_SAT = 1,
          unsigned int TP_TWIDDLE_MODE = 0,
          typename TT_OUT_DATA = TT_DATA>
class fft_ifft_dit_1ch_base_graph : public graph {
   public:
    // declare FFT Kernel array
    kernel m_fftKernels[TP_CASC_LEN];
    kernel* getKernels() { return m_fftKernels; };
    int startRanks[TP_CASC_LEN];
    int endRanks[TP_CASC_LEN];

#ifdef USE_GRAPH_SCOPE
    parameter fft_buf1[TP_CASC_LEN];

    // twiddle table graph scoping .
    parameter fft_lut1[TP_CASC_LEN];
    parameter fft_lut2[TP_CASC_LEN];
    parameter fft_lut3[TP_CASC_LEN];
    parameter fft_lut4[TP_CASC_LEN];

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
        // When streaming, a 'widget' function interlaces streams to buffer (window) for input and conversely for the
        // output.
        // These buffers are created here and passed to the cascade create to associate with the first and last kernels.
        // If not streaming, these buffers are not connected so should disappear.
        constexpr int bufferbytes = TP_WINDOW_VSIZE * sizeof(TT_DATA) + __ALIGN_BYTE_SIZE__ * TP_DYN_PT_SIZE;

        static constexpr int kHeaderBytes = TP_DYN_PT_SIZE > 0 ? __ALIGN_BYTE_SIZE__ : 0;
        typedef
            typename std::conditional<std::is_same<TT_DATA, cint16>::value, cint32_t, TT_DATA>::type T_internalDataType;

        // Create kernel class(s)
        create_casc_kernel<TP_CASC_LEN, TT_DATA, TT_TWIDDLE, TP_POINT_SIZE, TP_FFT_NIFFT, TP_SHIFT, TP_CASC_LEN,
                           TP_DYN_PT_SIZE, TP_WINDOW_VSIZE, TP_IN_API, TP_OUT_API, TP_ORIG_PAR_POWER, TP_RND, TP_SAT,
                           TP_TWIDDLE_MODE, TT_OUT_DATA>::create(m_fftKernels, startRanks, endRanks);

        // Make kernel to kernel window connections
        for (int k = 0; k < TP_CASC_LEN - 1; ++k) {
            connect(m_fftKernels[k].out[0], m_fftKernels[k + 1].in[0]);
            dimensions(m_fftKernels[k].out[0]) = {
                TP_WINDOW_VSIZE + kHeaderBytes / sizeof(T_internalDataType)}; // dimension is in units of data type
            dimensions(m_fftKernels[k + 1].in[0]) = {
                TP_WINDOW_VSIZE + kHeaderBytes / sizeof(T_internalDataType)}; // dimension is in units of data type

            // Removing this restriction as it hammers throughput to the point where cascade is ineffective. Also, it is
            // futile to attempt to calculate the point
            // at which memory limit will be exceeded especially with multiple AIE variants
            //      if (TP_POINT_SIZE*sizeof(TT_DATA) >= 8192) { //This would exceed data memory limits for AIE1.
            //        single_buffer(m_fftKernels[k + 1].in[0]);
            //      }
        }

#ifdef USE_GRAPH_SCOPE

        // Twiddle graph scoping
        // Attach graph scoped tables as required

        constexpr int kOddPowerAdj = std::is_same<TT_TWIDDLE, cfloat>::value ? 0 : fnOddPower<TP_POINT_SIZE>();

        for (int k = 0; k < TP_CASC_LEN; ++k) {
            if (std::is_same<TT_TWIDDLE, cfloat>::value) {
                if (TP_POINT_SIZE == 4096 && (TP_DYN_PT_SIZE == 1 || startRanks[k] <= 11) && endRanks[k] > 11) {
                    fft_lut1[k] = parameter::array(fft_lut_tw2048_cfloat);
                    connect<>(fft_lut1[k], m_fftKernels[k]);
                }
                if (TP_POINT_SIZE >= 2048 && (TP_DYN_PT_SIZE == 1 || startRanks[k] <= 10) && endRanks[k] > 10) {
                    fft_lut2[k] = parameter::array(fft_lut_tw1024_cfloat);
                    connect<>(fft_lut2[k], m_fftKernels[k]);
                }
                if (TP_POINT_SIZE >= 1024 && (TP_DYN_PT_SIZE == 1 || startRanks[k] <= 9) && endRanks[k] > 9) {
                    fft_lut3[k] = parameter::array(fft_lut_tw512_cfloat);
                    connect<>(fft_lut3[k], m_fftKernels[k]);
                }
                if (TP_POINT_SIZE >= 512 && (TP_DYN_PT_SIZE == 1 || startRanks[k] <= 8) && endRanks[k] > 8) {
                    fft_lut4[k] = parameter::array(fft_lut_tw256_cfloat);
                    connect<>(fft_lut4[k], m_fftKernels[k]);
                }
                // Integer section differs from cfloat because of half table optimization i.e. largest twiddle table is
                // half-size.
            } else if (std::is_same<TT_TWIDDLE, cint16>::value) {
                if (TP_POINT_SIZE == 4096 && (TP_DYN_PT_SIZE == 1 || startRanks[k] - kOddPowerAdj <= 11) &&
                    endRanks[k] - kOddPowerAdj > 11) {
                    if (TP_TWIDDLE_MODE == 0) {
                        fft_lut1[k] = parameter::array(fft_lut_tw2048_cint16_half);
                    } else {
                        fft_lut1[k] = parameter::array(fft_lut_tw2048_cint15_half);
                    }
                    connect<>(fft_lut1[k], m_fftKernels[k]);
                }

                if (TP_POINT_SIZE >= 2048 && (TP_DYN_PT_SIZE == 1 || startRanks[k] - kOddPowerAdj <= 10) &&
                    endRanks[k] - kOddPowerAdj > 10) {
                    if (TP_POINT_SIZE == 2048) {
                        if (TP_TWIDDLE_MODE == 0) {
                            fft_lut2[k] = parameter::array(fft_lut_tw1024_cint16_half);
                        } else {
                            fft_lut2[k] = parameter::array(fft_lut_tw1024_cint15_half);
                        }
                    } else {
                        if (TP_TWIDDLE_MODE == 0) {
                            fft_lut2[k] = parameter::array(fft_lut_tw1024_cint16);
                        } else {
                            fft_lut2[k] = parameter::array(fft_lut_tw1024_cint15);
                        }
                    }
                    connect<>(fft_lut2[k], m_fftKernels[k]);
                }
                if (TP_POINT_SIZE >= 1024 && (TP_DYN_PT_SIZE == 1 || startRanks[k] - kOddPowerAdj <= 9) &&
                    endRanks[k] - kOddPowerAdj > 9) {
                    if (TP_POINT_SIZE == 1024) {
                        if (TP_TWIDDLE_MODE == 0) {
                            fft_lut3[k] = parameter::array(fft_lut_tw512_cint16_half);
                        } else {
                            fft_lut3[k] = parameter::array(fft_lut_tw512_cint15_half);
                        }
                    } else {
                        if (TP_TWIDDLE_MODE == 0) {
                            fft_lut3[k] = parameter::array(fft_lut_tw512_cint16);
                        } else {
                            fft_lut3[k] = parameter::array(fft_lut_tw512_cint15);
                        }
                    }
                    connect<>(fft_lut3[k], m_fftKernels[k]);
                }
                if (TP_POINT_SIZE >= 512 && (TP_DYN_PT_SIZE == 1 || startRanks[k] - kOddPowerAdj <= 8) &&
                    endRanks[k] - kOddPowerAdj > 8) {
                    if (TP_POINT_SIZE == 512) {
                        if (TP_TWIDDLE_MODE == 0) {
                            fft_lut4[k] = parameter::array(fft_lut_tw256_cint16_half);
                        } else {
                            fft_lut4[k] = parameter::array(fft_lut_tw256_cint15_half);
                        }
                    } else {
                        if (TP_TWIDDLE_MODE == 0) {
                            fft_lut4[k] = parameter::array(fft_lut_tw256_cint16);
                        } else {
                            fft_lut4[k] = parameter::array(fft_lut_tw256_cint15);
                        }
                    }
                    connect<>(fft_lut4[k], m_fftKernels[k]);
                }
            } else if (std::is_same<TT_TWIDDLE, cint32>::value) {
                if (TP_POINT_SIZE == 4096 && (TP_DYN_PT_SIZE == 1 || startRanks[k] - kOddPowerAdj <= 11) &&
                    endRanks[k] - kOddPowerAdj > 11) {
                    if (TP_TWIDDLE_MODE == 0) {
                        fft_lut1[k] = parameter::array(fft_lut_tw2048_cint32_half);
                    } else {
                        fft_lut1[k] = parameter::array(fft_lut_tw2048_cint31_half);
                    }
                    connect<>(fft_lut1[k], m_fftKernels[k]);
                }
                if (TP_POINT_SIZE >= 2048 && (TP_DYN_PT_SIZE == 1 || startRanks[k] - kOddPowerAdj <= 10) &&
                    endRanks[k] - kOddPowerAdj > 10) {
                    if (TP_POINT_SIZE == 2048) {
                        if (TP_TWIDDLE_MODE == 0) {
                            fft_lut2[k] = parameter::array(fft_lut_tw1024_cint32_half);
                        } else {
                            fft_lut2[k] = parameter::array(fft_lut_tw1024_cint31_half);
                        }
                    } else {
                        if (TP_TWIDDLE_MODE == 0) {
                            fft_lut2[k] = parameter::array(fft_lut_tw1024_cint32);
                        } else {
                            fft_lut2[k] = parameter::array(fft_lut_tw1024_cint31);
                        }
                    }
                    connect<>(fft_lut2[k], m_fftKernels[k]);
                }
                if (TP_POINT_SIZE >= 1024 && (TP_DYN_PT_SIZE == 1 || startRanks[k] - kOddPowerAdj <= 9) &&
                    endRanks[k] - kOddPowerAdj > 9) {
                    if (TP_POINT_SIZE == 1024) {
                        if (TP_TWIDDLE_MODE == 0) {
                            fft_lut3[k] = parameter::array(fft_lut_tw512_cint32_half);
                        } else {
                            fft_lut3[k] = parameter::array(fft_lut_tw512_cint31_half);
                        }
                    } else {
                        if (TP_TWIDDLE_MODE == 0) {
                            fft_lut3[k] = parameter::array(fft_lut_tw512_cint32);
                        } else {
                            fft_lut3[k] = parameter::array(fft_lut_tw512_cint31);
                        }
                    }
                    connect<>(fft_lut3[k], m_fftKernels[k]);
                }
                if (TP_POINT_SIZE >= 512 && (TP_DYN_PT_SIZE == 1 || startRanks[k] - kOddPowerAdj <= 8) &&
                    endRanks[k] - kOddPowerAdj > 8) {
                    if (TP_POINT_SIZE == 512) {
                        if (TP_TWIDDLE_MODE == 0) {
                            fft_lut4[k] = parameter::array(fft_lut_tw256_cint32_half);
                        } else {
                            fft_lut4[k] = parameter::array(fft_lut_tw256_cint31_half);
                        }
                    } else {
                        if (TP_TWIDDLE_MODE == 0) {
                            fft_lut4[k] = parameter::array(fft_lut_tw256_cint32);
                        } else {
                            fft_lut4[k] = parameter::array(fft_lut_tw256_cint31);
                        }
                    }
                    connect<>(fft_lut4[k], m_fftKernels[k]);
                }
            } // end of cint32 clause
        }     // end of casc_len loop for twiddle graph scoping

        // Connect inter-rank temporary storage
        fft_buf4096 = parameter::array(fft_4096_tmp1);
        fft_buf2048 = parameter::array(fft_2048_tmp1);
        fft_buf1024 = parameter::array(fft_1024_tmp1);
        fft_buf512 = parameter::array(fft_512_tmp1);
        fft_buf256 = parameter::array(fft_256_tmp1);
        fft_buf128 = parameter::array(fft_128_tmp1);
        for (int k = 0; k < TP_CASC_LEN; ++k) {
            int numStages = std::is_same<TT_DATA, cfloat>::value ? (endRanks[k] - startRanks[k])
                                                                 : (endRanks[k] / 2 - startRanks[k] / 2);
            bool evenStages = false;
            if (numStages % 2 == 0 || TP_DYN_PT_SIZE == 1) { // When dynamic, point size can be even or odd at runtime
                                                             // so it is not safe to numStages since that is based on
                                                             // max point size.
                evenStages = true;
            }
            // Can we re-use the output buffer? Yes if the output is cint32/cfloat, no if the output is cint16 and no if
            // the number of stages is even.
            // So do we need to allocate a temp buffer? Yes if cint16 and this is the last kernel (the one with the
            // output) or the number of stages is even
            if ((std::is_same<TT_DATA, cint16>::value && k == TP_CASC_LEN - 1) || evenStages) {
                switch (TP_POINT_SIZE) {
                    case 4096:
                        fft_buf1[k] = fft_buf4096;
                        break;
                    case 2048:
                        fft_buf1[k] = fft_buf2048;
                        break;
                    case 1024:
                        fft_buf1[k] = fft_buf1024;
                        break;
                    case 512:
                        fft_buf1[k] = fft_buf512;
                        break;
                    case 256:
                        fft_buf1[k] = fft_buf256;
                        break;
                    default:
                        fft_buf1[k] = fft_buf128;
                        break;
                }
                connect<>(fft_buf1[k], m_fftKernels[k]);
            }
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
            runtime<ratio>(m_fftKernels[k]) = 0.7;

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
// Base of specialization. This default acts as specialization for iobuffer to iobuffer
template <typename TT_DATA,
          typename TT_TWIDDLE,
          unsigned int TP_POINT_SIZE,
          unsigned int TP_FFT_NIFFT,
          unsigned int TP_SHIFT,
          unsigned int TP_CASC_LEN = 1,
          unsigned int TP_DYN_PT_SIZE = 0, // backwards compatible default
          unsigned int TP_WINDOW_VSIZE = TP_POINT_SIZE,
          unsigned int TP_IN_API = kWindowAPI,
          unsigned int TP_OUT_API = kWindowAPI,
          unsigned int TP_USE_WIDGETS = 0,
          unsigned int TP_ORIG_PAR_POWER = 0, // invalid default because this must be set.
          unsigned int TP_RND = 0,
          unsigned int TP_SAT = 1,
          unsigned int TP_TWIDDLE_MODE = 0,
          typename TT_OUT_DATA = TT_DATA>
class fft_ifft_dit_1ch_baseports_graph : public fft_ifft_dit_1ch_base_graph<TT_DATA,
                                                                            TT_TWIDDLE,
                                                                            TP_POINT_SIZE,
                                                                            TP_FFT_NIFFT,
                                                                            TP_SHIFT,
                                                                            TP_CASC_LEN,
                                                                            TP_DYN_PT_SIZE,
                                                                            TP_WINDOW_VSIZE,
                                                                            TP_IN_API,
                                                                            TP_OUT_API,
                                                                            TP_ORIG_PAR_POWER,
                                                                            TP_RND,
                                                                            TP_SAT,
                                                                            TP_TWIDDLE_MODE,
                                                                            TT_OUT_DATA> {
   public:
    port_array<input, 1> in;
    port_array<output, 1> out;

    // Constructor
    fft_ifft_dit_1ch_baseports_graph() {
        // Make data connections
        static constexpr int kHeaderBytes = TP_DYN_PT_SIZE > 0 ? __ALIGN_BYTE_SIZE__ : 0;
        connect(in[0], this->m_fftKernels[0].in[0]);
        dimensions(this->m_fftKernels[0].in[0]) = {TP_WINDOW_VSIZE + kHeaderBytes / sizeof(TT_DATA)};
        connect(this->m_fftKernels[TP_CASC_LEN - 1].out[0], out[0]);
        dimensions(this->m_fftKernels[TP_CASC_LEN - 1].out[0]) = {TP_WINDOW_VSIZE + kHeaderBytes / sizeof(TT_OUT_DATA)};
    };
};

// This is the specialization for stream to stream ports.
template <typename TT_DATA,
          typename TT_TWIDDLE,
          unsigned int TP_POINT_SIZE,
          unsigned int TP_FFT_NIFFT,
          unsigned int TP_SHIFT,
          unsigned int TP_CASC_LEN,
          unsigned int TP_DYN_PT_SIZE,
          unsigned int TP_WINDOW_VSIZE,
          unsigned int TP_USE_WIDGETS,
          unsigned int TP_ORIG_PAR_POWER,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_TWIDDLE_MODE,
          typename TT_OUT_DATA>
class fft_ifft_dit_1ch_baseports_graph<TT_DATA,
                                       TT_TWIDDLE,
                                       TP_POINT_SIZE,
                                       TP_FFT_NIFFT,
                                       TP_SHIFT,
                                       TP_CASC_LEN,
                                       TP_DYN_PT_SIZE,
                                       TP_WINDOW_VSIZE,
                                       kStreamAPI,
                                       kStreamAPI,
                                       TP_USE_WIDGETS,
                                       TP_ORIG_PAR_POWER,
                                       TP_RND,
                                       TP_SAT,
                                       TP_TWIDDLE_MODE,
                                       TT_OUT_DATA> : public fft_ifft_dit_1ch_base_graph<TT_DATA,
                                                                                         TT_TWIDDLE,
                                                                                         TP_POINT_SIZE,
                                                                                         TP_FFT_NIFFT,
                                                                                         TP_SHIFT,
                                                                                         TP_CASC_LEN,
                                                                                         TP_DYN_PT_SIZE,
                                                                                         TP_WINDOW_VSIZE,
                                                                                         kStreamAPI,
                                                                                         kStreamAPI,
                                                                                         TP_ORIG_PAR_POWER,
                                                                                         TP_RND,
                                                                                         TP_SAT,
                                                                                         TP_TWIDDLE_MODE,
                                                                                         TT_OUT_DATA> {
   public:
    static constexpr int kStreamsPerTile = get_input_streams_core_module(); // a device trait

    /**
     * I/O is two parallel streams each TT_DATA type for AIE1 and 1 stream if AIE2.
     **/
    port_array<input, kStreamsPerTile> in;
    port_array<output, kStreamsPerTile> out;

    fft_ifft_dit_1ch_baseports_graph() {
        // Make data connections
        for (int i = 0; i < kStreamsPerTile; i++) {
            connect<stream>(in[i], this->m_fftKernels[0].in[i]);
            connect<stream>(this->m_fftKernels[TP_CASC_LEN - 1].out[i], out[i]);
        }
    };
};

// This is the specialization for stream to stream ports. using widgets
template <typename TT_DATA,
          typename TT_TWIDDLE,
          unsigned int TP_POINT_SIZE,
          unsigned int TP_FFT_NIFFT,
          unsigned int TP_SHIFT,
          unsigned int TP_CASC_LEN,
          unsigned int TP_DYN_PT_SIZE,
          unsigned int TP_WINDOW_VSIZE,
          unsigned int TP_ORIG_PAR_POWER,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_TWIDDLE_MODE,
          typename TT_OUT_DATA>
class fft_ifft_dit_1ch_baseports_graph<TT_DATA,
                                       TT_TWIDDLE,
                                       TP_POINT_SIZE,
                                       TP_FFT_NIFFT,
                                       TP_SHIFT,
                                       TP_CASC_LEN,
                                       TP_DYN_PT_SIZE,
                                       TP_WINDOW_VSIZE,
                                       kStreamAPI,
                                       kStreamAPI,
                                       1 /*USE_WIDGETS*/,
                                       TP_ORIG_PAR_POWER,
                                       TP_RND,
                                       TP_SAT,
                                       TP_TWIDDLE_MODE,
                                       TT_OUT_DATA> : public fft_ifft_dit_1ch_base_graph<TT_DATA,
                                                                                         TT_TWIDDLE,
                                                                                         TP_POINT_SIZE,
                                                                                         TP_FFT_NIFFT,
                                                                                         TP_SHIFT,
                                                                                         TP_CASC_LEN,
                                                                                         TP_DYN_PT_SIZE,
                                                                                         TP_WINDOW_VSIZE,
                                                                                         kWindowAPI,
                                                                                         kWindowAPI,
                                                                                         TP_ORIG_PAR_POWER,
                                                                                         TP_RND,
                                                                                         TP_SAT,
                                                                                         TP_TWIDDLE_MODE,
                                                                                         TT_OUT_DATA> {
   public:
    static constexpr int kHeaderBytes = TP_DYN_PT_SIZE > 0 ? __ALIGN_BYTE_SIZE__ : 0;
    static constexpr int kStreamsPerTile = get_input_streams_core_module(); // a device trait

    /**
     * I/O is two parallel streams each TT_DATA type for AIE1 and 1 stream if AIE2.
     **/
    port_array<input, kStreamsPerTile> in;
    port_array<output, kStreamsPerTile> out;

    kernel m_inWidgetKernel = kernel::create_object<widget_api_cast<TT_DATA,
                                                                    kStreamAPI,
                                                                    kWindowAPI,
                                                                    kStreamsPerTile,
                                                                    TP_WINDOW_VSIZE,
                                                                    1,
                                                                    kSampleIntlv,
                                                                    kHeaderBytes> >();
    kernel m_outWidgetKernel = kernel::create_object<widget_api_cast<TT_OUT_DATA,
                                                                     kWindowAPI,
                                                                     kStreamAPI,
                                                                     1,
                                                                     TP_WINDOW_VSIZE,
                                                                     kStreamsPerTile,
                                                                     kSampleIntlv,
                                                                     kHeaderBytes> >();

    fft_ifft_dit_1ch_baseports_graph() {
        connect<>(m_inWidgetKernel.out[0], this->m_fftKernels[0].in[0]);
        dimensions(m_inWidgetKernel.out[0]) = {TP_WINDOW_VSIZE + kHeaderBytes / sizeof(TT_DATA)};
        dimensions(this->m_fftKernels[0].in[0]) = {TP_WINDOW_VSIZE + kHeaderBytes / sizeof(TT_DATA)};
        connect<>(this->m_fftKernels[TP_CASC_LEN - 1].out[0], m_outWidgetKernel.in[0]);
        dimensions(this->m_fftKernels[TP_CASC_LEN - 1].out[0]) = {TP_WINDOW_VSIZE + kHeaderBytes / sizeof(TT_OUT_DATA)};
        dimensions(m_outWidgetKernel.in[0]) = {TP_WINDOW_VSIZE + kHeaderBytes / sizeof(TT_OUT_DATA)};
        // Make data connections
        for (int i = 0; i < kStreamsPerTile; i++) {
            connect<stream>(in[i], m_inWidgetKernel.in[i]);
            connect<stream>(m_outWidgetKernel.out[i], out[i]);
        }

        source(m_inWidgetKernel) = "widget_api_cast.cpp";
        headers(m_inWidgetKernel) = {"widget_api_cast.hpp"};
        runtime<ratio>(m_inWidgetKernel) = 0.9;
        source(m_outWidgetKernel) = "widget_api_cast.cpp";
        headers(m_outWidgetKernel) = {"widget_api_cast.hpp"};
        runtime<ratio>(m_outWidgetKernel) = 0.9;
    };
};

// This is the specialization for window to 2 streams
template <typename TT_DATA,
          typename TT_TWIDDLE,
          unsigned int TP_POINT_SIZE,
          unsigned int TP_FFT_NIFFT,
          unsigned int TP_SHIFT,
          unsigned int TP_CASC_LEN,
          unsigned int TP_DYN_PT_SIZE,
          unsigned int TP_WINDOW_VSIZE,
          unsigned int TP_USE_WIDGETS,
          unsigned int TP_ORIG_PAR_POWER,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_TWIDDLE_MODE,
          typename TT_OUT_DATA>
class fft_ifft_dit_1ch_baseports_graph<TT_DATA,
                                       TT_TWIDDLE,
                                       TP_POINT_SIZE,
                                       TP_FFT_NIFFT,
                                       TP_SHIFT,
                                       TP_CASC_LEN,
                                       TP_DYN_PT_SIZE,
                                       TP_WINDOW_VSIZE,
                                       kWindowAPI,
                                       kStreamAPI,
                                       TP_USE_WIDGETS,
                                       TP_ORIG_PAR_POWER,
                                       TP_RND,
                                       TP_SAT,
                                       TP_TWIDDLE_MODE,
                                       TT_OUT_DATA> : public fft_ifft_dit_1ch_base_graph<TT_DATA,
                                                                                         TT_TWIDDLE,
                                                                                         TP_POINT_SIZE,
                                                                                         TP_FFT_NIFFT,
                                                                                         TP_SHIFT,
                                                                                         TP_CASC_LEN,
                                                                                         TP_DYN_PT_SIZE,
                                                                                         TP_WINDOW_VSIZE,
                                                                                         kWindowAPI,
                                                                                         kStreamAPI,
                                                                                         TP_ORIG_PAR_POWER,
                                                                                         TP_RND,
                                                                                         TP_SAT,
                                                                                         TP_TWIDDLE_MODE,
                                                                                         TT_OUT_DATA> {
   public:
    static constexpr int kHeaderBytes = TP_DYN_PT_SIZE > 0 ? __ALIGN_BYTE_SIZE__ : 0;
    /**
     * I/O is an iobuffer in of TT_DATA type and for output 2 streams.
     **/
    port_array<input, 1> in;
    port_array<output, 2> out;

    fft_ifft_dit_1ch_baseports_graph() {
        // Make data connections
        connect<>(in[0], this->m_fftKernels[0].in[0]);
        dimensions(this->m_fftKernels[0].in[0]) = {TP_WINDOW_VSIZE + kHeaderBytes / sizeof(TT_DATA)};
        connect<stream>(this->m_fftKernels[TP_CASC_LEN - 1].out[0], out[0]);
        connect<stream>(this->m_fftKernels[TP_CASC_LEN - 1].out[1], out[1]);
    };
};

// This is the specialization for window to 2 streams using a widget
template <typename TT_DATA,
          typename TT_TWIDDLE,
          unsigned int TP_POINT_SIZE,
          unsigned int TP_FFT_NIFFT,
          unsigned int TP_SHIFT,
          unsigned int TP_CASC_LEN,
          unsigned int TP_DYN_PT_SIZE,
          unsigned int TP_WINDOW_VSIZE,
          unsigned int TP_ORIG_PAR_POWER,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_TWIDDLE_MODE,
          typename TT_OUT_DATA>
class fft_ifft_dit_1ch_baseports_graph<TT_DATA,
                                       TT_TWIDDLE,
                                       TP_POINT_SIZE,
                                       TP_FFT_NIFFT,
                                       TP_SHIFT,
                                       TP_CASC_LEN,
                                       TP_DYN_PT_SIZE,
                                       TP_WINDOW_VSIZE,
                                       kWindowAPI,
                                       kStreamAPI,
                                       1 /*USE_WIDGET*/,
                                       TP_ORIG_PAR_POWER,
                                       TP_RND,
                                       TP_SAT,
                                       TP_TWIDDLE_MODE,
                                       TT_OUT_DATA>
    : public fft_ifft_dit_1ch_base_graph<TT_DATA,
                                         TT_TWIDDLE,
                                         TP_POINT_SIZE,
                                         TP_FFT_NIFFT,
                                         TP_SHIFT,
                                         TP_CASC_LEN,
                                         TP_DYN_PT_SIZE,
                                         TP_WINDOW_VSIZE,
                                         kWindowAPI,
                                         kWindowAPI,
                                         TP_ORIG_PAR_POWER,
                                         TP_RND,
                                         TP_SAT,
                                         TP_TWIDDLE_MODE,
                                         TT_OUT_DATA> // note output of FFT is iobuffer
{
   public:
    static constexpr int kHeaderBytes = TP_DYN_PT_SIZE > 0 ? __ALIGN_BYTE_SIZE__ : 0;
    /**
     * I/O is an iobuffer in of TT_DATA type and for output 2 streams.
     **/
    port_array<input, 1> in;
    port_array<output, 2> out;

    kernel m_outWidgetKernel = kernel::create_object<
        widget_api_cast<TT_OUT_DATA, kWindowAPI, kStreamAPI, 1, TP_WINDOW_VSIZE, 2, kSampleIntlv, kHeaderBytes> >();

    fft_ifft_dit_1ch_baseports_graph() {
        // Make data connections
        connect<>(in[0], this->m_fftKernels[0].in[0]);
        dimensions(this->m_fftKernels[0].in[0]) = {TP_WINDOW_VSIZE + kHeaderBytes / sizeof(TT_DATA)};
        connect<>(this->m_fftKernels[TP_CASC_LEN - 1].out[0], m_outWidgetKernel.in[0]);
        dimensions(this->m_fftKernels[TP_CASC_LEN - 1].out[0]) = {TP_WINDOW_VSIZE + kHeaderBytes / sizeof(TT_OUT_DATA)};
        dimensions(m_outWidgetKernel.in[0]) = {TP_WINDOW_VSIZE + kHeaderBytes / sizeof(TT_OUT_DATA)};
        connect<stream>(this->m_outWidgetKernel.out[0], out[0]);
        connect<stream>(this->m_outWidgetKernel.out[1], out[1]);

        source(m_outWidgetKernel) = "widget_api_cast.cpp";
        headers(m_outWidgetKernel) = {"widget_api_cast.hpp"};
        runtime<ratio>(m_outWidgetKernel) = 0.9;
    };
};

// This is the specialization for window to casc/stream
template <typename TT_DATA,
          typename TT_TWIDDLE,
          unsigned int TP_POINT_SIZE,
          unsigned int TP_FFT_NIFFT,
          unsigned int TP_SHIFT,
          unsigned int TP_CASC_LEN,
          unsigned int TP_DYN_PT_SIZE,
          unsigned int TP_WINDOW_VSIZE,
          unsigned int TP_USE_WIDGETS,
          unsigned int TP_ORIG_PAR_POWER,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_TWIDDLE_MODE,
          typename TT_OUT_DATA>
class fft_ifft_dit_1ch_baseports_graph<TT_DATA,
                                       TT_TWIDDLE,
                                       TP_POINT_SIZE,
                                       TP_FFT_NIFFT,
                                       TP_SHIFT,
                                       TP_CASC_LEN,
                                       TP_DYN_PT_SIZE,
                                       TP_WINDOW_VSIZE,
                                       kWindowAPI,
                                       kCascStreamAPI,
                                       TP_USE_WIDGETS,
                                       TP_ORIG_PAR_POWER,
                                       TP_RND,
                                       TP_SAT,
                                       TP_TWIDDLE_MODE,
                                       TT_OUT_DATA> : public fft_ifft_dit_1ch_base_graph<TT_DATA,
                                                                                         TT_TWIDDLE,
                                                                                         TP_POINT_SIZE,
                                                                                         TP_FFT_NIFFT,
                                                                                         TP_SHIFT,
                                                                                         TP_CASC_LEN,
                                                                                         TP_DYN_PT_SIZE,
                                                                                         TP_WINDOW_VSIZE,
                                                                                         kWindowAPI,
                                                                                         kCascStreamAPI,
                                                                                         TP_ORIG_PAR_POWER,
                                                                                         TP_RND,
                                                                                         TP_SAT,
                                                                                         TP_TWIDDLE_MODE,
                                                                                         TT_OUT_DATA> {
   public:
    static constexpr int kHeaderBytes = TP_DYN_PT_SIZE > 0 ? __ALIGN_BYTE_SIZE__ : 0;
    /**
     * I/O is an iobuffer in of TT_DATA type and for output a cascade and stream.
     **/
    port_array<input, 1> in;
    port_array<output, 2> out;

    fft_ifft_dit_1ch_baseports_graph() {
        // Make data connections
        connect<>(in[0], this->m_fftKernels[0].in[0]);
        dimensions(this->m_fftKernels[0].in[0]) = {TP_WINDOW_VSIZE + kHeaderBytes / sizeof(TT_DATA)};
        connect<cascade>(this->m_fftKernels[TP_CASC_LEN - 1].out[0], out[0]);
        connect<stream>(this->m_fftKernels[TP_CASC_LEN - 1].out[1], out[1]);
    };
};

// This is the specialization for window to casc/stream using a widget
template <typename TT_DATA,
          typename TT_TWIDDLE,
          unsigned int TP_POINT_SIZE,
          unsigned int TP_FFT_NIFFT,
          unsigned int TP_SHIFT,
          unsigned int TP_CASC_LEN,
          unsigned int TP_DYN_PT_SIZE,
          unsigned int TP_WINDOW_VSIZE,
          unsigned int TP_ORIG_PAR_POWER,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_TWIDDLE_MODE,
          typename TT_OUT_DATA>
class fft_ifft_dit_1ch_baseports_graph<TT_DATA,
                                       TT_TWIDDLE,
                                       TP_POINT_SIZE,
                                       TP_FFT_NIFFT,
                                       TP_SHIFT,
                                       TP_CASC_LEN,
                                       TP_DYN_PT_SIZE,
                                       TP_WINDOW_VSIZE,
                                       kWindowAPI,
                                       kCascStreamAPI,
                                       1 /*TP_USE_WIDGETS*/,
                                       TP_ORIG_PAR_POWER,
                                       TP_RND,
                                       TP_SAT,
                                       TP_TWIDDLE_MODE,
                                       TT_OUT_DATA> : public fft_ifft_dit_1ch_base_graph<TT_DATA,
                                                                                         TT_TWIDDLE,
                                                                                         TP_POINT_SIZE,
                                                                                         TP_FFT_NIFFT,
                                                                                         TP_SHIFT,
                                                                                         TP_CASC_LEN,
                                                                                         TP_DYN_PT_SIZE,
                                                                                         TP_WINDOW_VSIZE,
                                                                                         kWindowAPI,
                                                                                         kWindowAPI,
                                                                                         TP_ORIG_PAR_POWER,
                                                                                         TP_RND,
                                                                                         TP_SAT,
                                                                                         TP_TWIDDLE_MODE,
                                                                                         TT_OUT_DATA> {
   public:
    static constexpr int kHeaderBytes = TP_DYN_PT_SIZE > 0 ? __ALIGN_BYTE_SIZE__ : 0;
    /**
     * I/O is an iobuffer in of TT_DATA type and for output a cascade and stream.
     **/
    port_array<input, 1> in;
    port_array<output, 2> out;

    kernel m_outWidgetKernel = kernel::create_object<
        widget_api_cast<TT_OUT_DATA, kWindowAPI, kCascStreamAPI, 1, TP_WINDOW_VSIZE, 2, kSampleIntlv, kHeaderBytes> >();

    fft_ifft_dit_1ch_baseports_graph() {
        // Make data connections
        connect<>(in[0], this->m_fftKernels[0].in[0]);
        dimensions(this->m_fftKernels[0].in[0]) = {TP_WINDOW_VSIZE + kHeaderBytes / sizeof(TT_DATA)};
        connect<>(this->m_fftKernels[TP_CASC_LEN - 1].out[0], m_outWidgetKernel.in[0]);
        dimensions(this->m_fftKernels[TP_CASC_LEN - 1].out[0]) = {TP_WINDOW_VSIZE + kHeaderBytes / sizeof(TT_OUT_DATA)};
        dimensions(m_outWidgetKernel.in[0]) = {TP_WINDOW_VSIZE + kHeaderBytes / sizeof(TT_OUT_DATA)};
        connect<cascade>(m_outWidgetKernel.out[0], out[0]);
        connect<stream>(m_outWidgetKernel.out[1], out[1]);

        source(m_outWidgetKernel) = "widget_api_cast.cpp";
        headers(m_outWidgetKernel) = {"widget_api_cast.hpp"};
        runtime<ratio>(m_outWidgetKernel) = 0.9;
    };
};

// This is the specialization for window to stream/casc
template <typename TT_DATA,
          typename TT_TWIDDLE,
          unsigned int TP_POINT_SIZE,
          unsigned int TP_FFT_NIFFT,
          unsigned int TP_SHIFT,
          unsigned int TP_CASC_LEN,
          unsigned int TP_DYN_PT_SIZE,
          unsigned int TP_WINDOW_VSIZE,
          unsigned int TP_USE_WIDGETS,
          unsigned int TP_ORIG_PAR_POWER,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_TWIDDLE_MODE,
          typename TT_OUT_DATA>
class fft_ifft_dit_1ch_baseports_graph<TT_DATA,
                                       TT_TWIDDLE,
                                       TP_POINT_SIZE,
                                       TP_FFT_NIFFT,
                                       TP_SHIFT,
                                       TP_CASC_LEN,
                                       TP_DYN_PT_SIZE,
                                       TP_WINDOW_VSIZE,
                                       kWindowAPI,
                                       kStreamCascAPI,
                                       TP_USE_WIDGETS,
                                       TP_ORIG_PAR_POWER,
                                       TP_RND,
                                       TP_SAT,
                                       TP_TWIDDLE_MODE,
                                       TT_OUT_DATA> : public fft_ifft_dit_1ch_base_graph<TT_DATA,
                                                                                         TT_TWIDDLE,
                                                                                         TP_POINT_SIZE,
                                                                                         TP_FFT_NIFFT,
                                                                                         TP_SHIFT,
                                                                                         TP_CASC_LEN,
                                                                                         TP_DYN_PT_SIZE,
                                                                                         TP_WINDOW_VSIZE,
                                                                                         kWindowAPI,
                                                                                         kStreamCascAPI,
                                                                                         TP_ORIG_PAR_POWER,
                                                                                         TP_RND,
                                                                                         TP_SAT,
                                                                                         TP_TWIDDLE_MODE,
                                                                                         TT_OUT_DATA> {
   public:
    static constexpr int kHeaderBytes = TP_DYN_PT_SIZE > 0 ? __ALIGN_BYTE_SIZE__ : 0;
    /**
     * I/O is an iobuffer in of TT_DATA type and for output a cascade and stream.
     **/
    port_array<input, 1> in;
    port_array<output, 2> out;

    fft_ifft_dit_1ch_baseports_graph() {
        // Make data connections
        connect<>(in[0], this->m_fftKernels[0].in[0]);
        dimensions(this->m_fftKernels[0].in[0]) = {TP_WINDOW_VSIZE + kHeaderBytes / sizeof(TT_DATA)};
        connect<stream>(this->m_fftKernels[TP_CASC_LEN - 1].out[0], out[0]);
        connect<cascade>(this->m_fftKernels[TP_CASC_LEN - 1].out[1], out[1]);
    };
};

// This is the specialization for window to stream/casc using a widget
template <typename TT_DATA,
          typename TT_TWIDDLE,
          unsigned int TP_POINT_SIZE,
          unsigned int TP_FFT_NIFFT,
          unsigned int TP_SHIFT,
          unsigned int TP_CASC_LEN,
          unsigned int TP_DYN_PT_SIZE,
          unsigned int TP_WINDOW_VSIZE,
          unsigned int TP_ORIG_PAR_POWER,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_TWIDDLE_MODE,
          typename TT_OUT_DATA>
class fft_ifft_dit_1ch_baseports_graph<TT_DATA,
                                       TT_TWIDDLE,
                                       TP_POINT_SIZE,
                                       TP_FFT_NIFFT,
                                       TP_SHIFT,
                                       TP_CASC_LEN,
                                       TP_DYN_PT_SIZE,
                                       TP_WINDOW_VSIZE,
                                       kWindowAPI,
                                       kStreamCascAPI,
                                       1 /*TP_USE_WIDGETS*/,
                                       TP_ORIG_PAR_POWER,
                                       TP_RND,
                                       TP_SAT,
                                       TP_TWIDDLE_MODE,
                                       TT_OUT_DATA> : public fft_ifft_dit_1ch_base_graph<TT_DATA,
                                                                                         TT_TWIDDLE,
                                                                                         TP_POINT_SIZE,
                                                                                         TP_FFT_NIFFT,
                                                                                         TP_SHIFT,
                                                                                         TP_CASC_LEN,
                                                                                         TP_DYN_PT_SIZE,
                                                                                         TP_WINDOW_VSIZE,
                                                                                         kWindowAPI,
                                                                                         kWindowAPI,
                                                                                         TP_ORIG_PAR_POWER,
                                                                                         TP_RND,
                                                                                         TP_SAT,
                                                                                         TP_TWIDDLE_MODE,
                                                                                         TT_OUT_DATA> {
   public:
    static constexpr int kHeaderBytes = TP_DYN_PT_SIZE > 0 ? __ALIGN_BYTE_SIZE__ : 0;
    /**
     * I/O is an iobuffer in of TT_DATA type and for output a cascade and stream.
     **/
    port_array<input, 1> in;
    port_array<output, 2> out;

    kernel m_outWidgetKernel = kernel::create_object<
        widget_api_cast<TT_OUT_DATA, kWindowAPI, kStreamCascAPI, 1, TP_WINDOW_VSIZE, 2, kSampleIntlv, kHeaderBytes> >();

    fft_ifft_dit_1ch_baseports_graph() {
        // Make data connections
        connect<>(in[0], this->m_fftKernels[0].in[0]);
        dimensions(this->m_fftKernels[0].in[0]) = {TP_WINDOW_VSIZE + kHeaderBytes / sizeof(TT_DATA)};
        connect<>(this->m_fftKernels[TP_CASC_LEN - 1].out[0], m_outWidgetKernel.in[0]);
        dimensions(this->m_fftKernels[TP_CASC_LEN - 1].out[0]) = {TP_WINDOW_VSIZE + kHeaderBytes / sizeof(TT_OUT_DATA)};
        dimensions(m_outWidgetKernel.in[0]) = {TP_WINDOW_VSIZE + kHeaderBytes / sizeof(TT_OUT_DATA)};
        connect<stream>(m_outWidgetKernel.out[0], out[0]);
        connect<cascade>(m_outWidgetKernel.out[1], out[1]);

        source(m_outWidgetKernel) = "widget_api_cast.cpp";
        headers(m_outWidgetKernel) = {"widget_api_cast.hpp"};
        runtime<ratio>(m_outWidgetKernel) = 0.9;
    };
};

// This is the specialization for stream to casc/stream
template <typename TT_DATA,
          typename TT_TWIDDLE,
          unsigned int TP_POINT_SIZE,
          unsigned int TP_FFT_NIFFT,
          unsigned int TP_SHIFT,
          unsigned int TP_CASC_LEN,
          unsigned int TP_DYN_PT_SIZE,
          unsigned int TP_WINDOW_VSIZE,
          unsigned int TP_USE_WIDGETS,
          unsigned int TP_ORIG_PAR_POWER,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_TWIDDLE_MODE,
          typename TT_OUT_DATA>
class fft_ifft_dit_1ch_baseports_graph<TT_DATA,
                                       TT_TWIDDLE,
                                       TP_POINT_SIZE,
                                       TP_FFT_NIFFT,
                                       TP_SHIFT,
                                       TP_CASC_LEN,
                                       TP_DYN_PT_SIZE,
                                       TP_WINDOW_VSIZE,
                                       kStreamAPI,
                                       kCascStreamAPI,
                                       TP_USE_WIDGETS,
                                       TP_ORIG_PAR_POWER,
                                       TP_RND,
                                       TP_SAT,
                                       TP_TWIDDLE_MODE,
                                       TT_OUT_DATA> : public fft_ifft_dit_1ch_base_graph<TT_DATA,
                                                                                         TT_TWIDDLE,
                                                                                         TP_POINT_SIZE,
                                                                                         TP_FFT_NIFFT,
                                                                                         TP_SHIFT,
                                                                                         TP_CASC_LEN,
                                                                                         TP_DYN_PT_SIZE,
                                                                                         TP_WINDOW_VSIZE,
                                                                                         kStreamAPI,
                                                                                         kCascStreamAPI,
                                                                                         TP_ORIG_PAR_POWER,
                                                                                         TP_RND,
                                                                                         TP_SAT,
                                                                                         TP_TWIDDLE_MODE,
                                                                                         TT_OUT_DATA> {
   public:
    /**
     * I/O is an iobuffer in of TT_DATA type and for output a cascade and stream.
     **/
    port_array<input, 1> in;
    port_array<output, 2> out;

    fft_ifft_dit_1ch_baseports_graph() {
        // Make data connections
        connect<stream>(in[0], this->m_fftKernels[0].in[0]);
        connect<cascade>(this->m_fftKernels[TP_CASC_LEN - 1].out[0], out[0]);
        connect<stream>(this->m_fftKernels[TP_CASC_LEN - 1].out[1], out[1]);
    };
};

// This is the specialization for stream to casc/stream using widget
template <typename TT_DATA,
          typename TT_TWIDDLE,
          unsigned int TP_POINT_SIZE,
          unsigned int TP_FFT_NIFFT,
          unsigned int TP_SHIFT,
          unsigned int TP_CASC_LEN,
          unsigned int TP_DYN_PT_SIZE,
          unsigned int TP_WINDOW_VSIZE,
          unsigned int TP_ORIG_PAR_POWER,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_TWIDDLE_MODE,
          typename TT_OUT_DATA>
class fft_ifft_dit_1ch_baseports_graph<TT_DATA,
                                       TT_TWIDDLE,
                                       TP_POINT_SIZE,
                                       TP_FFT_NIFFT,
                                       TP_SHIFT,
                                       TP_CASC_LEN,
                                       TP_DYN_PT_SIZE,
                                       TP_WINDOW_VSIZE,
                                       kStreamAPI,
                                       kCascStreamAPI,
                                       1 /*TP_USE_WIDGETS*/,
                                       TP_ORIG_PAR_POWER,
                                       TP_RND,
                                       TP_SAT,
                                       TP_TWIDDLE_MODE,
                                       TT_OUT_DATA> : public fft_ifft_dit_1ch_base_graph<TT_DATA,
                                                                                         TT_TWIDDLE,
                                                                                         TP_POINT_SIZE,
                                                                                         TP_FFT_NIFFT,
                                                                                         TP_SHIFT,
                                                                                         TP_CASC_LEN,
                                                                                         TP_DYN_PT_SIZE,
                                                                                         TP_WINDOW_VSIZE,
                                                                                         kWindowAPI,
                                                                                         kWindowAPI,
                                                                                         TP_ORIG_PAR_POWER,
                                                                                         TP_RND,
                                                                                         TP_SAT,
                                                                                         TP_TWIDDLE_MODE,
                                                                                         TT_OUT_DATA> {
   public:
    static constexpr int kHeaderBytes = TP_DYN_PT_SIZE > 0 ? __ALIGN_BYTE_SIZE__ : 0;
    /**
     * I/O is an iobuffer in of TT_DATA type and for output a cascade and stream.
     **/
    port_array<input, 1> in;
    port_array<output, 2> out;

    kernel m_inWidgetKernel = kernel::create_object<widget_api_cast<TT_DATA,
                                                                    kStreamAPI,
                                                                    kWindowAPI,
                                                                    1 /*kStreamsPerTile*/,
                                                                    TP_WINDOW_VSIZE,
                                                                    1,
                                                                    kSampleIntlv,
                                                                    kHeaderBytes> >();
    kernel m_outWidgetKernel = kernel::create_object<
        widget_api_cast<TT_OUT_DATA, kWindowAPI, kCascStreamAPI, 1, TP_WINDOW_VSIZE, 2, kSampleIntlv, kHeaderBytes> >();

    fft_ifft_dit_1ch_baseports_graph() {
        // Make data connections
        connect<stream>(in[0], m_inWidgetKernel.in[0]);
        connect<>(m_inWidgetKernel.out[0], this->m_fftKernels[0].in[0]);
        dimensions(m_inWidgetKernel.out[0]) = {TP_WINDOW_VSIZE + kHeaderBytes / sizeof(TT_DATA)};
        dimensions(this->m_fftKernels[0].in[0]) = {TP_WINDOW_VSIZE + kHeaderBytes / sizeof(TT_DATA)};
        connect<>(this->m_fftKernels[TP_CASC_LEN - 1].out[0], m_outWidgetKernel.in[0]);
        dimensions(m_outWidgetKernel.in[0]) = {TP_WINDOW_VSIZE + kHeaderBytes / sizeof(TT_OUT_DATA)};
        dimensions(this->m_fftKernels[TP_CASC_LEN - 1].out[0]) = {TP_WINDOW_VSIZE + kHeaderBytes / sizeof(TT_OUT_DATA)};
        connect<cascade>(m_outWidgetKernel.out[0], out[0]);
        connect<stream>(m_outWidgetKernel.out[1], out[1]);

        source(m_inWidgetKernel) = "widget_api_cast.cpp";
        headers(m_inWidgetKernel) = {"widget_api_cast.hpp"};
        runtime<ratio>(m_inWidgetKernel) = 0.9;
        source(m_outWidgetKernel) = "widget_api_cast.cpp";
        headers(m_outWidgetKernel) = {"widget_api_cast.hpp"};
        runtime<ratio>(m_outWidgetKernel) = 0.9;
    };
};

// This is the specialization for stream to stream/casc
template <typename TT_DATA,
          typename TT_TWIDDLE,
          unsigned int TP_POINT_SIZE,
          unsigned int TP_FFT_NIFFT,
          unsigned int TP_SHIFT,
          unsigned int TP_CASC_LEN,
          unsigned int TP_DYN_PT_SIZE,
          unsigned int TP_WINDOW_VSIZE,
          unsigned int TP_USE_WIDGETS,
          unsigned int TP_ORIG_PAR_POWER,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_TWIDDLE_MODE,
          typename TT_OUT_DATA>
class fft_ifft_dit_1ch_baseports_graph<TT_DATA,
                                       TT_TWIDDLE,
                                       TP_POINT_SIZE,
                                       TP_FFT_NIFFT,
                                       TP_SHIFT,
                                       TP_CASC_LEN,
                                       TP_DYN_PT_SIZE,
                                       TP_WINDOW_VSIZE,
                                       kStreamAPI,
                                       kStreamCascAPI,
                                       TP_USE_WIDGETS,
                                       TP_ORIG_PAR_POWER,
                                       TP_RND,
                                       TP_SAT,
                                       TP_TWIDDLE_MODE,
                                       TT_OUT_DATA> : public fft_ifft_dit_1ch_base_graph<TT_DATA,
                                                                                         TT_TWIDDLE,
                                                                                         TP_POINT_SIZE,
                                                                                         TP_FFT_NIFFT,
                                                                                         TP_SHIFT,
                                                                                         TP_CASC_LEN,
                                                                                         TP_DYN_PT_SIZE,
                                                                                         TP_WINDOW_VSIZE,
                                                                                         kStreamAPI,
                                                                                         kStreamCascAPI,
                                                                                         TP_ORIG_PAR_POWER,
                                                                                         TP_RND,
                                                                                         TP_SAT,
                                                                                         TP_TWIDDLE_MODE,
                                                                                         TT_OUT_DATA> {
   public:
    /**
     * I/O is an iobuffer in of TT_DATA type and for output a cascade and stream.
     **/
    port_array<input, 1> in;
    port_array<output, 2> out;

    fft_ifft_dit_1ch_baseports_graph() {
        // Make data connections
        connect<stream>(in[0], this->m_fftKernels[0].in[0]);
        connect<stream>(this->m_fftKernels[TP_CASC_LEN - 1].out[0], out[0]);
        connect<cascade>(this->m_fftKernels[TP_CASC_LEN - 1].out[1], out[1]);
    };
};

// This is the specialization for stream to stream/casc using widget
template <typename TT_DATA,
          typename TT_TWIDDLE,
          unsigned int TP_POINT_SIZE,
          unsigned int TP_FFT_NIFFT,
          unsigned int TP_SHIFT,
          unsigned int TP_CASC_LEN,
          unsigned int TP_DYN_PT_SIZE,
          unsigned int TP_WINDOW_VSIZE,
          unsigned int TP_ORIG_PAR_POWER,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_TWIDDLE_MODE,
          typename TT_OUT_DATA>
class fft_ifft_dit_1ch_baseports_graph<TT_DATA,
                                       TT_TWIDDLE,
                                       TP_POINT_SIZE,
                                       TP_FFT_NIFFT,
                                       TP_SHIFT,
                                       TP_CASC_LEN,
                                       TP_DYN_PT_SIZE,
                                       TP_WINDOW_VSIZE,
                                       kStreamAPI,
                                       kStreamCascAPI,
                                       1 /*TP_USE_WIDGETS*/,
                                       TP_ORIG_PAR_POWER,
                                       TP_RND,
                                       TP_SAT,
                                       TP_TWIDDLE_MODE,
                                       TT_OUT_DATA> : public fft_ifft_dit_1ch_base_graph<TT_DATA,
                                                                                         TT_TWIDDLE,
                                                                                         TP_POINT_SIZE,
                                                                                         TP_FFT_NIFFT,
                                                                                         TP_SHIFT,
                                                                                         TP_CASC_LEN,
                                                                                         TP_DYN_PT_SIZE,
                                                                                         TP_WINDOW_VSIZE,
                                                                                         kWindowAPI,
                                                                                         kWindowAPI,
                                                                                         TP_ORIG_PAR_POWER,
                                                                                         TP_RND,
                                                                                         TP_SAT,
                                                                                         TP_TWIDDLE_MODE,
                                                                                         TT_OUT_DATA> {
   public:
    static constexpr int kHeaderBytes = TP_DYN_PT_SIZE > 0 ? __ALIGN_BYTE_SIZE__ : 0;
    /**
     * I/O is an iobuffer in of TT_DATA type and for output a cascade and stream.
     **/
    port_array<input, 1> in;
    port_array<output, 2> out;

    kernel m_inWidgetKernel = kernel::create_object<widget_api_cast<TT_DATA,
                                                                    kStreamAPI,
                                                                    kWindowAPI,
                                                                    1 /*kStreamsPerTile*/,
                                                                    TP_WINDOW_VSIZE,
                                                                    1,
                                                                    kSampleIntlv,
                                                                    kHeaderBytes> >();
    kernel m_outWidgetKernel = kernel::create_object<
        widget_api_cast<TT_OUT_DATA, kWindowAPI, kStreamCascAPI, 1, TP_WINDOW_VSIZE, 2, kSampleIntlv, kHeaderBytes> >();

    fft_ifft_dit_1ch_baseports_graph() {
        // Make data connections
        connect<stream>(in[0], m_inWidgetKernel.in[0]);
        connect<>(m_inWidgetKernel.out[0], this->m_fftKernels[0].in[0]);
        dimensions(m_inWidgetKernel.out[0]) = {TP_WINDOW_VSIZE + kHeaderBytes / sizeof(TT_DATA)};
        dimensions(this->m_fftKernels[0].in[0]) = {TP_WINDOW_VSIZE + kHeaderBytes / sizeof(TT_DATA)};
        connect<>(this->m_fftKernels[TP_CASC_LEN - 1].out[0], m_outWidgetKernel.in[0]);
        dimensions(this->m_fftKernels[TP_CASC_LEN - 1].out[0]) = {TP_WINDOW_VSIZE + kHeaderBytes / sizeof(TT_OUT_DATA)};
        dimensions(m_outWidgetKernel.in[0]) = {TP_WINDOW_VSIZE + kHeaderBytes / sizeof(TT_OUT_DATA)};
        connect<stream>(m_outWidgetKernel.out[0], out[0]);
        connect<cascade>(m_outWidgetKernel.out[1], out[1]);

        source(m_inWidgetKernel) = "widget_api_cast.cpp";
        headers(m_inWidgetKernel) = {"widget_api_cast.hpp"};
        runtime<ratio>(m_inWidgetKernel) = 0.9;
        source(m_outWidgetKernel) = "widget_api_cast.cpp";
        headers(m_outWidgetKernel) = {"widget_api_cast.hpp"};
        runtime<ratio>(m_outWidgetKernel) = 0.9;
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
          unsigned int TP_IN_API = kWindowAPI,
          unsigned int TP_OUT_API = kWindowAPI,
          unsigned int TP_USE_WIDGETS = 0,
          unsigned int TP_ORIG_PAR_POWER = 0, // invalid default because this must be set.
          unsigned int TP_RND = 0,
          unsigned int TP_SAT = 1,
          unsigned int TP_TWIDDLE_MODE = 0,
          typename TT_OUT_DATA = TT_DATA>
class fft_ifft_dit_1ch_mono_graph : public fft_ifft_dit_1ch_baseports_graph<TT_DATA,
                                                                            TT_TWIDDLE,
                                                                            TP_POINT_SIZE,
                                                                            TP_FFT_NIFFT,
                                                                            TP_SHIFT,
                                                                            TP_CASC_LEN,
                                                                            TP_DYN_PT_SIZE,
                                                                            TP_WINDOW_VSIZE,
                                                                            TP_IN_API,
                                                                            TP_OUT_API,
                                                                            TP_USE_WIDGETS,
                                                                            TP_ORIG_PAR_POWER,
                                                                            TP_RND,
                                                                            TP_SAT,
                                                                            TP_TWIDDLE_MODE,
                                                                            TT_OUT_DATA> {
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
          unsigned int TP_IN_API,
          unsigned int TP_OUT_API,
          unsigned int TP_USE_WIDGETS,
          unsigned int TP_ORIG_PAR_POWER,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_TWIDDLE_MODE,
          typename TT_OUT_DATA>
class fft_ifft_dit_1ch_mono_graph<cint16,
                                  TT_TWIDDLE,
                                  TP_POINT_SIZE,
                                  TP_FFT_NIFFT,
                                  TP_SHIFT,
                                  TP_CASC_LEN,
                                  TP_DYN_PT_SIZE,
                                  TP_WINDOW_VSIZE,
                                  TP_IN_API,
                                  TP_OUT_API,
                                  TP_USE_WIDGETS,
                                  TP_ORIG_PAR_POWER,
                                  TP_RND,
                                  TP_SAT,
                                  TP_TWIDDLE_MODE,
                                  TT_OUT_DATA> : public fft_ifft_dit_1ch_baseports_graph<cint16,
                                                                                         TT_TWIDDLE,
                                                                                         TP_POINT_SIZE,
                                                                                         TP_FFT_NIFFT,
                                                                                         TP_SHIFT,
                                                                                         TP_CASC_LEN,
                                                                                         TP_DYN_PT_SIZE,
                                                                                         TP_WINDOW_VSIZE,
                                                                                         TP_IN_API,
                                                                                         TP_OUT_API,
                                                                                         TP_USE_WIDGETS,
                                                                                         TP_ORIG_PAR_POWER,
                                                                                         TP_RND,
                                                                                         TP_SAT,
                                                                                         TP_TWIDDLE_MODE,
                                                                                         TT_OUT_DATA> {
   public:
#ifdef USE_GRAPH_SCOPE
    parameter fft_buf4096;
    parameter fft_buf2048;
    parameter fft_buf1024;
    parameter fft_buf512;
    parameter fft_buf256;
    parameter fft_buf128;
    parameter fft_buf2[TP_CASC_LEN];
    fft_ifft_dit_1ch_mono_graph() {
        fft_buf4096 = parameter::array(fft_4096_tmp2);
        fft_buf2048 = parameter::array(fft_2048_tmp2);
        fft_buf1024 = parameter::array(fft_1024_tmp2);
        fft_buf512 = parameter::array(fft_512_tmp2);
        fft_buf256 = parameter::array(fft_256_tmp2);
        fft_buf128 = parameter::array(fft_128_tmp2);
        for (int k = 0; k < TP_CASC_LEN; ++k) {
            // Only the first kernel in a chain will see TT_DATA == cint16 input. All others will see cint32, so could
            // reuse the input buffer.
            // We only need to allocate a graph scoped buffer for the first kernel.
            if (k == 0) {
                switch (TP_POINT_SIZE) {
                    case 4096:
                        fft_buf2[k] = fft_buf4096;
                        break;
                    case 2048:
                        fft_buf2[k] = fft_buf2048;
                        break;
                    case 1024:
                        fft_buf2[k] = fft_buf1024;
                        break;
                    case 512:
                        fft_buf2[k] = fft_buf512;
                        break;
                    case 256:
                        fft_buf2[k] = fft_buf256;
                        break;
                    default:
                        fft_buf2[k] = fft_buf128;
                        break;
                }
                connect<>(fft_buf2[k], this->m_fftKernels[k]);
            }
        }
    }
#endif // USE_GRAPH_SCOPE
};

/**
  * @endcond
  */

//--------------------------------------------------------------------------------------------------
// fft_dit_1ch template
//--------------------------------------------------------------------------------------------------
/**
 * @ingroup fft_graphs
 *
 * @brief fft_dit_1ch is a single-channel, decimation-in-time FFT.
 *
 * These are the templates to configure the single-channel decimation-in-time class.
 * @tparam TT_DATA describes the type of individual data samples input to the transform function. \n
 *         This is a typename and must be one of the following: \n
 *         cint16, cint32, cfloat.
 *         For real-only operation, consider use of the widget_real2complex library element.
 * @tparam TT_TWIDDLE describes the type of twiddle factors of the transform. \n
 *         It must be one of the following: cint16, cint32, cfloat
 *         and must also satisfy the following rules:
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
 *         to the function. When TP_DYN_PT_SIZE is set to 1 the actual window size will be larger than TP_WINDOW_VSIZE\n
 *         because the header is not included in TP_WINDOW_VSIZE. \n
 *         By default, TP_WINDOW_SIZE is set to match TP_POINT_SIZE. \n
 *         TP_WINDOW_SIZE may be set to be an integer multiple of the TP_POINT_SIZE, in which case
 *         multiple FFT iterations will be performed on a given input window, resulting in multiple
 *         iterations of output samples, reducing the numer of times the kernel needs to be triggered to
 *         process a given number of input data samples. \n
 *         As a result, the overheads inferred during kernel triggering are reduced and overall performance
 *         is increased.
 * @tparam TP_API is an unsigned integer to select window (0) or stream (1) interfaces. \n
 *         When stream I/O is selected, one sample is taken from, or output to, a stream and the next sample
 *         from or two the next stream. Two streams minimum are used. In this example, even samples are
 *         read from input stream[0] and odd samples from input stream[1].
 * @tparam TP_PARALLEL_POWER is an unsigned integer to describe N where 2^N is the numbers of subframe processors
 *         to use, so as to achieve higher throughput. \n
 *         The default is 0. With TP_PARALLEL_POWER set to 2, 4 subframe processors will be used, each of which
 *         takes 2 streams or one iobuffer in for a total of 8 streams or 4 iobuffers input and output.
 *         Sample[p] must be written to stream or iobuffer number [p modulus q] where q is the number of ports.
 *         A reciprocal operation must be performed on output ports.
 * @tparam TP_USE_WIDGETS is an unsigned integer to control the use of widgets for configurations which either use
 *         TP_API=1 or TP_PARALLEL_POWER>0.  \n
 *         Designs with streaming IO (TP_API=1) and/or multiple subframe processors (TP_PARALLEL_POWER>0)
 *         will use streams internally, even if not externally. \n The default is not to use widgets
 *         but to have the stream to window conversion performed as part of the FFT kernel or R2combiner kernel.
 *         Using widget kernels allows this conversion to be placed in a separate tile and so boost performance
 *         at the expense of more tiles being used.
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
 *         Note that the FFT does not support floor and ceiling forms of rounding as these result in very poor
 *         accuracy due to the form of internal calculations.
 *         Rounding modes differ only in how they round for values of 0.5. \n
 *         \n
 * @tparam TP_SAT describes the selection of saturation to be applied during the shift down stage of processing. \n
 *         TP_SAT accepts unsigned integer values, where:
 *         - 0: none           = No saturation is performed and the value is simply cropped (aliased).
 *         - 1: saturate       = Default. Saturation rounds an n-bit signed value
 *         in the range [- ( 2^(n-1) ) : +2^(n-1) - 1 ].
 *         - 3: symmetric      = Controls symmetric saturation. Symmetric saturation rounds an n-bit signed value in the
 *range [- ( 2^(n-1) -1 ) : +2^(n-1) - 1 ]. \n
 * @tparam TP_TWIDDLE_MODE describes the magnitude of integer twiddles. It has no effect for cfloat. \n
 *         - 0: Max amplitude. Values at 2^15 (for TT_TWIDDLE=cint16) and 2^31 (TT_TWIDDLE=cint32) will saturate and so
 *introduce errors
 *         - 1: 0.5 amplitude. Twiddle values are 1/2 that of mode 0 so as to avoid twiddle saturation. However,
 *twiddles are one bit less precise versus mode 0.
 * @tparam TT_OUT_DATA describes the type of individual data samples output from the transform function. \n
 *         This is a typename and must be cint16 / cint32 if TT_DATA is cint16 / cint32 or cfloat if TT_DATA is cfloat.
 * @tparam TP_INDEX
 *         This parameter is for internal use regarding the recursion of the parallel power feature. \n
 *         It is recommended
 *         to miss this parameter from the configuration and rely instead on default values. If this parameter is set
 *         by the user, the behaviour of the library unit is undefined.
 * @tparam TP_ORIG_PAR_POWER
 *         This parameter is for internal use regarding the recursion of the parallel power feature. \n
 *         It is recommended
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
          unsigned int TP_USE_WIDGETS = 0,
          unsigned int TP_RND = 4,
          unsigned int TP_SAT = 1,
          unsigned int TP_TWIDDLE_MODE = 0,
          typename TT_OUT_DATA = TT_DATA,
          // template params above this comment are user params. Params below are for recursive calls. Any new user
          // parameters should go above this comment.
          unsigned int TP_INDEX = 0,
          unsigned int TP_ORIG_PAR_POWER = TP_PARALLEL_POWER>
class fft_ifft_dit_1ch_graph : public graph {
   public:
    // static_assert(TP_API == kStreamAPI, "Error: Only Stream interface is supported for parallel FFT"); //Not any
    // more, as of 23.1
    static_assert(TP_PARALLEL_POWER >= 1 && TP_PARALLEL_POWER < 9,
                  "Error: TP_PARALLEL_POWER is out of supported range");
#ifdef __FFT_NO_RND_SYM_CEIL_FLOOR__
    static_assert(
        TP_RND != rnd_sym_floor && TP_RND != rnd_sym_ceil,
        "Error: FFT does not support TP_RND set to floor. Please set TP_RND to any of the other rounding modes."
        "The mapping of integers to rounding modes is device dependent."
        "Please refer to documentation for more information.");
#endif
#ifdef __FFT_NO_RND_CEIL_FLOOR__
    static_assert(
        TP_RND != rnd_floor && TP_RND != rnd_ceil,
        "Error: FFT does not support TP_RND set to floor. Please set TP_RND to any of the other rounding modes."
        "The mapping of integers to rounding modes is device dependent."
        "Please refer to documentation for more information.");
#endif
    static_assert(!(std::is_same<TT_TWIDDLE, cint32>::value) || __SUPPORTS_32B_TW__ == 1,
                  "ERROR: This variant of AIE does not support TT_TWIDDLE = cint32");

    static_assert((std::is_same<TT_DATA, cint16>::value) || (std::is_same<TT_DATA, cint32>::value) ||
                      (std::is_same<TT_DATA, cfloat>::value),
                  "ERROR: TT_DATA is not supported");

    static constexpr int kParallel_factor = 1 << TP_PARALLEL_POWER;
    static constexpr int kWindowSize = TP_WINDOW_VSIZE >> TP_PARALLEL_POWER;
    static constexpr int kNextParallelPower = TP_PARALLEL_POWER - 1;

    static constexpr int kR2Shift = TP_SHIFT > 0 ? 1 : 0;
    static constexpr int kFFTsubShift = TP_SHIFT > 0 ? TP_SHIFT - 1 : 0;

    static constexpr int kHeaderBytes = TP_DYN_PT_SIZE > 0 ? __ALIGN_BYTE_SIZE__ : 0;
    static constexpr int kStreamsPerTile = get_input_streams_core_module(); // a device trait
    static constexpr int kPortsPerTile = (TP_API == 0) ? 1 : kStreamsPerTile;
    static constexpr int kOutputPorts =
        (TP_PARALLEL_POWER == TP_ORIG_PAR_POWER)
            ? kPortsPerTile * kParallel_factor
            : 2 * kParallel_factor; // only at the top level do we see an exception with single ports out.

    /**
     * The input data to the function.
     * I/O  is two parallel streams or one IObuffer each TT_DATA type.
     **/

    port_array<input, kPortsPerTile * kParallel_factor> in;

    /**
     * The output data from the function.
     * I/O  is two parallel streams or one IObuffer each TT_DATA type.
     **/
    port_array<output, kOutputPorts> out;

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
     * FFT is split into 2 subframe processors with a stage of r2combiners connecting subframe processors.
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
                           TP_API,
                           kNextParallelPower,
                           TP_USE_WIDGETS,
                           TP_RND,
                           TP_SAT,
                           TP_TWIDDLE_MODE,
                           TT_OUT_DATA,
                           TP_INDEX,
                           TP_ORIG_PAR_POWER>
        FFTsubframe0;

    /**
     * FFT recursive decomposition. Subframe1.
     * FFT is split into 2 subframe processors with a stage of r2combiners connecting subframe processors.
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
                           TP_API,
                           kNextParallelPower,
                           TP_USE_WIDGETS,
                           TP_RND,
                           TP_SAT,
                           TP_TWIDDLE_MODE,
                           TT_OUT_DATA,
                           TP_INDEX + kParallel_factor / 2,
                           TP_ORIG_PAR_POWER>
        FFTsubframe1;

    /**
     * @brief This is the constructor function for the Single channel DIT FFT graph.
     * No arguments required
     **/
    fft_ifft_dit_1ch_graph() {
        // create kernels and subgraphs
        if
            constexpr(TP_USE_WIDGETS == 0) {
                create_r2comb_kernels<TT_OUT_DATA, TT_TWIDDLE, TP_POINT_SIZE, TP_FFT_NIFFT, kR2Shift, TP_DYN_PT_SIZE,
                                      kWindowSize, TP_PARALLEL_POWER, kParallel_factor - 1, TP_INDEX, TP_ORIG_PAR_POWER,
                                      TP_API, TP_USE_WIDGETS, TP_RND, TP_SAT, TP_TWIDDLE_MODE>::create(m_r2Comb);
            }
        else {
            create_combInWidget_kernels<TT_OUT_DATA, kWindowSize, TP_PARALLEL_POWER, kParallel_factor - 1, TP_INDEX,
                                        TP_ORIG_PAR_POWER, TP_API, kHeaderBytes>::create(m_combInKernel);
            create_r2comb_kernels<TT_OUT_DATA, TT_TWIDDLE, TP_POINT_SIZE, TP_FFT_NIFFT, kR2Shift, TP_DYN_PT_SIZE,
                                  kWindowSize, TP_PARALLEL_POWER, kParallel_factor - 1, TP_INDEX, TP_ORIG_PAR_POWER,
                                  kWindowAPI, TP_USE_WIDGETS, TP_RND, TP_SAT, TP_TWIDDLE_MODE>::create(m_r2Comb);
            if
                constexpr(TP_API == 1 || TP_PARALLEL_POWER < TP_ORIG_PAR_POWER) {
                    create_combOutWidget_kernels<TT_OUT_DATA, kWindowSize, TP_PARALLEL_POWER, kParallel_factor - 1,
                                                 TP_INDEX, kHeaderBytes, TP_ORIG_PAR_POWER,
                                                 TP_API>::create(m_combOutKernel);
                }
        }
        // make input connections
        if
            constexpr(TP_API == 0) {
                for (int i = 0; i < kParallel_factor / 2; i++) {
                    connect<>(in[2 * i], FFTsubframe0.in[i]);
                    connect<>(in[2 * i + 1], FFTsubframe1.in[i]);
                }
            }
        else { // TP_API == 1 (stream(s))
            for (int i = 0; i < kStreamsPerTile * kParallel_factor / 2; i++) {
                connect<stream>(in[2 * i], FFTsubframe0.in[i]);
                connect<stream>(in[2 * i + 1], FFTsubframe1.in[i]);
            }
        }

        // internal connections
        if
            constexpr(kStreamsPerTile == 2) { // AIE1
                for (int i = 0; i < kParallel_factor; i++) {
                    if
                        constexpr(TP_USE_WIDGETS == 0) {
                            connect<stream>(FFTsubframe0.out[i], m_r2Comb[i].in[0]);
                            connect<stream>(FFTsubframe1.out[i], m_r2Comb[i].in[1]);
                        }
                    else {
                        connect<stream>(FFTsubframe0.out[i], m_combInKernel[i].in[0]);
                        connect<stream>(FFTsubframe1.out[i], m_combInKernel[i].in[1]);
                        connect<>(m_combInKernel[i].out[0], m_r2Comb[i].in[0]);
                        dimensions(m_combInKernel[i].out[0]) = {kWindowSize + kHeaderBytes / sizeof(TT_OUT_DATA)};
                        dimensions(m_r2Comb[i].in[0]) = {kWindowSize + kHeaderBytes / sizeof(TT_OUT_DATA)};
                    }
                }
            }
        else {
            for (int i = 0; i < kParallel_factor; i++) {
                if (i < kParallel_factor / 2) { // i.e. top half
                    if
                        constexpr(TP_USE_WIDGETS == 0) {
                            connect<cascade>(FFTsubframe0.out[i], m_r2Comb[i].in[0]);
                            connect<stream>(FFTsubframe1.out[i], m_r2Comb[i].in[1]);
                        }
                    else {
                        connect<cascade>(FFTsubframe0.out[i], m_combInKernel[i].in[0]);
                        connect<stream>(FFTsubframe1.out[i], m_combInKernel[i].in[1]);
                        connect<>(m_combInKernel[i].out[0], m_r2Comb[i].in[0]);
                        dimensions(m_combInKernel[i].out[0]) = {kWindowSize + kHeaderBytes / sizeof(TT_OUT_DATA)};
                        dimensions(m_r2Comb[i].in[0]) = {kWindowSize + kHeaderBytes / sizeof(TT_OUT_DATA)};
                    }
                } else { // bottom half
                    if
                        constexpr(TP_USE_WIDGETS == 0) {
                            connect<stream>(FFTsubframe0.out[i], m_r2Comb[i].in[0]);
                            connect<cascade>(FFTsubframe1.out[i], m_r2Comb[i].in[1]);
                        }
                    else {
                        connect<stream>(FFTsubframe0.out[i], m_combInKernel[i].in[0]);
                        connect<cascade>(FFTsubframe1.out[i], m_combInKernel[i].in[1]);
                        connect<>(m_combInKernel[i].out[0], m_r2Comb[i].in[0]);
                        dimensions(m_combInKernel[i].out[0]) = {kWindowSize + kHeaderBytes / sizeof(TT_OUT_DATA)};
                        dimensions(m_r2Comb[i].in[0]) = {kWindowSize + kHeaderBytes / sizeof(TT_OUT_DATA)};
                    }
                }
            }
        }

        // output connections
        if (TP_PARALLEL_POWER == TP_ORIG_PAR_POWER) { // i.e. top level, so outputs are really outputs
            if (TP_API == kStreamAPI) {
                for (int i = 0; i < kParallel_factor; i++) {
                    if
                        constexpr(TP_USE_WIDGETS == 0) {
                            connect<stream>(m_r2Comb[i].out[0], out[i]);
                            if (kStreamsPerTile == 2) {
                                connect<stream>(m_r2Comb[i].out[1], out[i + kParallel_factor]);
                            }
                        }
                    else {
                        connect<>(m_r2Comb[i].out[0], m_combOutKernel[i].in[0]);
                        dimensions(m_r2Comb[i].out[0]) = {kWindowSize + kHeaderBytes / sizeof(TT_OUT_DATA)};
                        dimensions(m_combOutKernel[i].in[0]) = {kWindowSize + kHeaderBytes / sizeof(TT_OUT_DATA)};
                        connect<stream>(m_combOutKernel[i].out[0], out[i]);
                        if (kStreamsPerTile == 2) {
                            connect<stream>(m_combOutKernel[i].out[1], out[i + kParallel_factor]);
                        }
                    }
                }
            } else {
                for (int i = 0; i < kParallel_factor; i++) {
                    connect<>(m_r2Comb[i].out[0], out[i]);
                    dimensions(m_r2Comb[i].out[0]) = {kWindowSize + kHeaderBytes / sizeof(TT_OUT_DATA)};
                }
            }
        } else { // not top level, so outputs are internal trellis connections

            if (kStreamsPerTile == 2) {
                for (int i = 0; i < kParallel_factor; i++) {
                    if
                        constexpr(TP_USE_WIDGETS == 0) {
                            connect<stream>(m_r2Comb[i].out[0], out[i]);
                            connect<stream>(m_r2Comb[i].out[1], out[i + kParallel_factor]);
                        }
                    else {
                        connect(m_r2Comb[i].out[0], m_combOutKernel[i].in[0]);
                        dimensions(m_r2Comb[i].out[0]) = {kWindowSize + kHeaderBytes / sizeof(TT_OUT_DATA)};
                        dimensions(m_combOutKernel[i].in[0]) = {kWindowSize + kHeaderBytes / sizeof(TT_OUT_DATA)};
                        connect<stream>(m_combOutKernel[i].out[0], out[i]);
                        connect<stream>(m_combOutKernel[i].out[1], out[i + kParallel_factor]);
                    }
                }
            } else {
                for (int i = 0; i < kParallel_factor; i++) {
                    if ((TP_INDEX >> TP_PARALLEL_POWER) % 2 == 0) { // i.e. top half
                        if
                            constexpr(TP_USE_WIDGETS == 0) {
                                connect<cascade>(m_r2Comb[i].out[0], out[i]);
                                connect<stream>(m_r2Comb[i].out[1], out[i + kParallel_factor]);
                            }
                        else {
                            connect(m_r2Comb[i].out[0], m_combOutKernel[i].in[0]);
                            dimensions(m_r2Comb[i].out[0]) = {kWindowSize + kHeaderBytes / sizeof(TT_OUT_DATA)};
                            dimensions(m_combOutKernel[i].in[0]) = {kWindowSize + kHeaderBytes / sizeof(TT_OUT_DATA)};
                            connect<cascade>(m_combOutKernel[i].out[0], out[i]);
                            connect<stream>(m_combOutKernel[i].out[1], out[i + kParallel_factor]);
                        }
                    } else {
                        if
                            constexpr(TP_USE_WIDGETS == 0) {
                                connect<stream>(m_r2Comb[i].out[0], out[i]);
                                connect<cascade>(m_r2Comb[i].out[1], out[i + kParallel_factor]);
                            }
                        else {
                            connect(m_r2Comb[i].out[0], m_combOutKernel[i].in[0]);
                            dimensions(m_r2Comb[i].out[0]) = {kWindowSize + kHeaderBytes / sizeof(TT_OUT_DATA)};
                            dimensions(m_combOutKernel[i].in[0]) = {kWindowSize + kHeaderBytes / sizeof(TT_OUT_DATA)};
                            connect<stream>(m_combOutKernel[i].out[0], out[i]);
                            connect<cascade>(m_combOutKernel[i].out[1], out[i + kParallel_factor]);
                        }
                    }
                }
            }
        }

        // Associate kernels with Source files and set runtime ratio
        for (int i = 0; i < kParallel_factor; i++) {
            source(m_r2Comb[i]) = "fft_r2comb.cpp";
            headers(m_r2Comb[i]) = {"fft_r2comb.hpp"};
            runtime<ratio>(m_r2Comb[i]) = 0.9;
            if
                constexpr(TP_USE_WIDGETS == 1) {
                    source(m_combInKernel[i]) = "widget_api_cast.cpp";
                    headers(m_combInKernel[i]) = {"widget_api_cast.hpp"};
                    runtime<ratio>(m_combInKernel[i]) = 0.9;
                    source(m_combOutKernel[i]) = "widget_api_cast.cpp";
                    headers(m_combOutKernel[i]) = {"widget_api_cast.hpp"};
                    runtime<ratio>(m_combOutKernel[i]) = 0.9;
                }
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
 * Window interface (kWindowAPI = 0) FFT graph is offered with a single input windowed, ping-pong buffer.
 * Dynamic point size is available (TP_DYN_PT_SIZE = 1), which allows a runtime configurable FFT point size.
 * In addition, window buffer size can be an integer multiple of the FFT's point size ( TP_WINDOW_VSIZE = N *
 *TP_POINT_SIZE).
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
          unsigned int TP_USE_WIDGETS,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_TWIDDLE_MODE,
          typename TT_OUT_DATA,
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
                             TP_USE_WIDGETS,
                             TP_RND,
                             TP_SAT,
                             TP_TWIDDLE_MODE,
                             TT_OUT_DATA,
                             TP_INDEX,
                             TP_ORIG_PAR_POWER> : public graph {
   public:
    static constexpr unsigned int TP_PARALLEL_POWER = 0; // for this specialization
    static constexpr int kHeaderBytes = TP_DYN_PT_SIZE > 0 ? __ALIGN_BYTE_SIZE__ : 0;
    static constexpr unsigned int kOutAPI =
        fnGetFFTSubframeOutAPI<kWindowAPI, TP_ORIG_PAR_POWER, TP_PARALLEL_POWER, TP_INDEX>(); // 0=win, 1=stream,
                                                                                              // 2=casc/strm,
                                                                                              // 3=strm/casc
    static constexpr unsigned int kNumOutPorts = (kOutAPI == 0) ? 1 : 2;

#ifdef __FFT_NO_RND_SYM_CEIL_FLOOR__
    static_assert(
        TP_RND != rnd_sym_floor && TP_RND != rnd_sym_ceil,
        "Error: FFT does not support TP_RND set to floor. Please set TP_RND to any of the other rounding modes."
        "The mapping of integers to rounding modes is device dependent."
        "Please refer to documentation for more information.");
#endif
#ifdef __FFT_NO_RND_CEIL_FLOOR__
    static_assert(
        TP_RND != rnd_floor && TP_RND != rnd_ceil,
        "Error: FFT does not support TP_RND set to floor. Please set TP_RND to any of the other rounding modes."
        "The mapping of integers to rounding modes is device dependent."
        "Please refer to documentation for more information.");
#endif

    static_assert(!(std::is_same<TT_TWIDDLE, cint32>::value) || __SUPPORTS_32B_TW__ == 1,
                  "ERROR: This variant of AIE does not support TT_TWIDDLE = cint32");

    /**
     * The input data to the function. This input is a window API of
     * samples of TT_DATA type. The number of samples in the window is
     * described by TP_POINT_SIZE.
     **/
    port_array<input, 1> in;

    /**
     * A window API of TP_POINT_SIZE samples of TT_DATA type.
     **/
    port_array<output, kNumOutPorts> out;

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
                                kOutAPI,
                                TP_USE_WIDGETS,
                                TP_ORIG_PAR_POWER,
                                TP_RND,
                                TP_SAT,
                                TP_TWIDDLE_MODE,
                                TT_OUT_DATA>
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
        if
            constexpr(kOutAPI == 0) {
                connect<>(FFTwinproc.out[0], out[0]);
                //            dimensions(FFTwinproc.out[0]) = {TP_WINDOW_VSIZE + kHeaderBytes / sizeof(TT_OUT_DATA)};
            }
        else {
            connect<>(FFTwinproc.out[0], out[0]);
            connect<>(FFTwinproc.out[1], out[1]);
        }
    };
};

//--------------------------------------------------------------------------------------------------
// fft_dit_1ch template specialization
//--------------------------------------------------------------------------------------------------
/**
 * @ingroup fft_graphs
 *
 * @brief fft_dit_1ch template specialization for single FFT, stream API (kStreamAPI = 0).
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
          unsigned int TP_USE_WIDGETS,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_TWIDDLE_MODE,
          typename TT_OUT_DATA,
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
                             TP_USE_WIDGETS,
                             TP_RND,
                             TP_SAT,
                             TP_TWIDDLE_MODE,
                             TT_OUT_DATA,
                             TP_INDEX,
                             TP_ORIG_PAR_POWER> : public graph {
   public:
    static constexpr unsigned int TP_PARALLEL_POWER = 0; // for this specialization
    static constexpr unsigned int kOutAPI =
        fnGetFFTSubframeOutAPI<kStreamAPI, TP_ORIG_PAR_POWER, TP_PARALLEL_POWER, TP_INDEX>();
    static constexpr unsigned int kNumOutPorts = (kOutAPI == 0) ? 1 : 2;
    static constexpr int kStreamsPerTile = get_input_streams_core_module();
#ifdef __FFT_NO_RND_SYM_CEIL_FLOOR__
    static_assert(
        TP_RND != rnd_sym_floor && TP_RND != rnd_sym_ceil,
        "Error: FFT does not support TP_RND set to floor. Please set TP_RND to any of the other rounding modes."
        "The mapping of integers to rounding modes is device dependent."
        "Please refer to documentation for more information.");
#endif
#ifdef __FFT_NO_RND_CEIL_FLOOR__
    static_assert(
        TP_RND != rnd_floor && TP_RND != rnd_ceil,
        "Error: FFT does not support TP_RND set to floor. Please set TP_RND to any of the other rounding modes."
        "The mapping of integers to rounding modes is device dependent."
        "Please refer to documentation for more information.");
#endif

    /**
       * The input data to the function.
       * I/O  is two parallel streams each TT_DATA type.
       **/
    port_array<input, kStreamsPerTile> in; // dual streams

    /**
     * The output data from the function.
     * I/O  is two parallel streams each TT_DATA type.
     **/
    port_array<output, kNumOutPorts> out;

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
                                kOutAPI,
                                TP_USE_WIDGETS,
                                TP_ORIG_PAR_POWER,
                                TP_RND,
                                TP_SAT,
                                TP_TWIDDLE_MODE,
                                TT_OUT_DATA>
        FFTstrproc;

    /**
     * @brief This is the constructor function for the Single channel DIT FFT graph.
     * No arguments required
     **/
    fft_ifft_dit_1ch_graph() {
        for (int i = 0; i < kStreamsPerTile; i++) {
            connect<>(in[i], FFTstrproc.in[i]);
        }
        // for case that this is the top level, output ports according to kStreamsPerTile.
        if (TP_ORIG_PAR_POWER == 0) {
            for (int i = 0; i < kStreamsPerTile; i++) {
                connect<>(FFTstrproc.out[i], out[i]);
            }
        } else { // if not top level, these are trellis connections.
            for (int i = 0; i < kNumOutPorts; i++) {
                connect<>(FFTstrproc.out[i], out[i]);
            }
        }
    };
};

} // namespace dit_1ch
} // namespace fft
} // namespace aie
} // namespace dsp
} // namespace xf

#endif // _DSPLIB_FFT_IFFT_DIT_1CH_GRAPH_HPP_
