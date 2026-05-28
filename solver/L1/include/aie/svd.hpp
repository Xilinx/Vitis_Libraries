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
#ifndef _DSPLIB_SVD_HPP_
#define _DSPLIB_SVD_HPP_

/*
SVD kernel classes.
Graph-scoped scratch buffer avoids stack spilling for working arrays.
*/

/* Coding conventions
   TT_      template type suffix
   TP_      template parameter suffix
*/

#include <adf.h>
#include <vector>

#include "svd_traits.hpp"
#include "aie_api/aie.hpp"
#include "single_mul_out_types.hpp"
#include "single_mul_acc_types.hpp"

using namespace adf;
namespace xf {
namespace solver {
namespace aie {
namespace svd {

typedef float T_outSDataType;

// =====================================================================
// Scratch buffer size computation (shared by graph and kernel)
// =====================================================================
template <typename TT_DATA, unsigned int TP_DIM_COLS>
struct SvdScratchSize {
    static constexpr bool kIsComplex = fnIsComplexDataType<TT_DATA>();
    static constexpr unsigned int kVecSize = fnVecSampleNum<TT_DATA>();
    static constexpr unsigned int kSchedCols = TP_DIM_COLS + (TP_DIM_COLS % 2);
    static constexpr unsigned int kPairsPerSet = kSchedCols / 2;
    // Pad each scratch slice to IO vector width so vector stream writes
    // (writeincr of v8float / v4cfloat) are aligned and in-bounds.
    static constexpr unsigned int kVecSizeIO = fnVecSampleNumIO<TT_DATA>();
    static constexpr unsigned int kPaddedPairsPerSet =
        ((kPairsPerSet + kVecSizeIO - 1) / kVecSizeIO) * kVecSizeIO;
    // Layout: dotII[PP] + dotJJ[PP] + dotIJ[PP] + c_arr[PP] + s_arr[PP] + phase_real[PP] + phase_imag[PP]
    static constexpr unsigned int kSize = 7 * kPaddedPairsPerSet;
};

// =====================================================================
// Compile-time pair generation (round-robin tournament)
// =====================================================================
namespace detail {
template <unsigned int N>
struct SetOrganizer {
    static_assert(N % 2 == 0, "Parallel pairs requires even dimensions");
    static constexpr int numPairs = (N * (N - 1)) / 2;
    static constexpr int pairsPerSet = N / 2;
    static constexpr int numSets = N - 1;

    struct SetArray {
        uint8_t data[numPairs][2];
        constexpr SetArray() : data{} {
            int globalIdx = 0;
            for (int round = 0; round < numSets; round++) {
                data[globalIdx][0] = 0;
                data[globalIdx][1] = static_cast<uint8_t>(1 + (round % (N - 1)));
                globalIdx++;
                for (int k = 1; k < pairsPerSet; k++) {
                    uint8_t a = static_cast<uint8_t>(1 + ((k + round) % (N - 1)));
                    uint8_t b = static_cast<uint8_t>(1 + ((N - 1 - k + round) % (N - 1)));
                    if (a > b) { uint8_t tmp = a; a = b; b = tmp; }
                    data[globalIdx][0] = a;
                    data[globalIdx][1] = b;
                    globalIdx++;
                }
            }
        }
    };
    static constexpr SetArray sets = SetArray();
};
}

template <unsigned int TP_DIM_COLS>
struct ParallelPairSets {
    static_assert(TP_DIM_COLS % 2 == 0, "Parallel pairs currently only supports even TP_DIM_COLS");
    static constexpr int numPairs = (TP_DIM_COLS * (TP_DIM_COLS - 1)) / 2;
    static constexpr int pairsPerSet = TP_DIM_COLS / 2;
    static constexpr int numSets = TP_DIM_COLS - 1;
    static constexpr auto& pairs = detail::SetOrganizer<TP_DIM_COLS>::sets.data;
};

// =====================================================================
// Single kernel (TP_CASC_LEN=1)
// =====================================================================
template <typename TT_DATA, unsigned int TP_DIM_ROWS, unsigned int TP_DIM_COLS,
          unsigned int TP_PASSES, unsigned int TP_CASC_LEN, unsigned int TP_KERNEL_POSITION>
class SVDecomposition {
   public:
    static constexpr unsigned int kScratchSize = SvdScratchSize<TT_DATA, TP_DIM_COLS>::kSize;
    float (&scratchBuf)[kScratchSize];

    SVDecomposition(float (&buf)[kScratchSize]) : scratchBuf(buf) {}

    static void registerKernelClass() {
        REGISTER_FUNCTION(SVDecomposition::svdMain);
        REGISTER_PARAMETER(scratchBuf);
    }

    void svdMain(input_buffer<TT_DATA>& __restrict inWindow,
                 output_buffer<TT_DATA>& __restrict outUWindow,
                 output_buffer<T_outSDataType>& __restrict outSWindow,
                 output_buffer<TT_DATA>& __restrict outVWindow);
};

// =====================================================================
// First kernel (receives feedback stream from last, sends to next)
// =====================================================================
template <typename TT_DATA, unsigned int TP_DIM_ROWS, unsigned int TP_DIM_COLS,
          unsigned int TP_PASSES, unsigned int TP_CASC_LEN, unsigned int TP_KERNEL_POSITION>
class SVDecomposition_First {
   public:
    static constexpr unsigned int kScratchSize = SvdScratchSize<TT_DATA, TP_DIM_COLS>::kSize;
    float (&scratchBuf)[kScratchSize];

    SVDecomposition_First(float (&buf)[kScratchSize]) : scratchBuf(buf) {}

    static void registerKernelClass() {
        REGISTER_FUNCTION(SVDecomposition_First::svdMain);
        REGISTER_PARAMETER(scratchBuf);
    }

    void svdMain(input_buffer<TT_DATA>& __restrict inWindow,
                 input_stream<TT_DATA>* inStream,
                 output_buffer<TT_DATA>& __restrict outUWindow,
                 output_buffer<T_outSDataType>& __restrict outSWindow,
                 output_buffer<TT_DATA>& __restrict outVWindow,
                 output_stream<TT_DATA>* outStream);
};

// =====================================================================
// Middle kernel (receives stream, sends stream)
// =====================================================================
template <typename TT_DATA, unsigned int TP_DIM_ROWS, unsigned int TP_DIM_COLS,
          unsigned int TP_PASSES, unsigned int TP_CASC_LEN, unsigned int TP_KERNEL_POSITION>
class SVDecomposition_Middle {
   public:
    static constexpr unsigned int kScratchSize = SvdScratchSize<TT_DATA, TP_DIM_COLS>::kSize;
    float (&scratchBuf)[kScratchSize];

    SVDecomposition_Middle(float (&buf)[kScratchSize]) : scratchBuf(buf) {}

    static void registerKernelClass() {
        REGISTER_FUNCTION(SVDecomposition_Middle::svdMain);
        REGISTER_PARAMETER(scratchBuf);
    }

    void svdMain(input_buffer<TT_DATA>& __restrict inWindow,
                 input_stream<TT_DATA>* inStream,
                 output_buffer<TT_DATA>& __restrict outUWindow,
                 output_buffer<T_outSDataType>& __restrict outSWindow,
                 output_buffer<TT_DATA>& __restrict outVWindow,
                 output_stream<TT_DATA>* outStream);
};

// =====================================================================
// Last kernel (receives stream, sends back to first)
// =====================================================================
template <typename TT_DATA, unsigned int TP_DIM_ROWS, unsigned int TP_DIM_COLS,
          unsigned int TP_PASSES, unsigned int TP_CASC_LEN, unsigned int TP_KERNEL_POSITION>
class SVDecomposition_Last {
   public:
    static constexpr unsigned int kScratchSize = SvdScratchSize<TT_DATA, TP_DIM_COLS>::kSize;
    float (&scratchBuf)[kScratchSize];

    SVDecomposition_Last(float (&buf)[kScratchSize]) : scratchBuf(buf) {}

    static void registerKernelClass() {
        REGISTER_FUNCTION(SVDecomposition_Last::svdMain);
        REGISTER_PARAMETER(scratchBuf);
    }

    void svdMain(input_buffer<TT_DATA>& __restrict inWindow,
                 input_stream<TT_DATA>* inStream,
                 output_buffer<TT_DATA>& __restrict outUWindow,
                 output_buffer<T_outSDataType>& __restrict outSWindow,
                 output_buffer<TT_DATA>& __restrict outVWindow,
                 output_stream<TT_DATA>* outStream);
};

} // namespace svd
} // namespace aie
} // namespace solver
} // namespace xf
#endif // _DSPLIB_SVD_HPP_
