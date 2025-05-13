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
#ifndef _DSPLIB_MATRIX_MULT_UNTILER_CPP_
#define _DSPLIB_MATRIX_MULT_UNTILER_CPP_

#include <adf.h>

#ifndef __NEW_WINDOW_H__
#endif
// if we use 1kb registers -> aie api uses 2x512b registers for 1024b so we need this for QoR
#ifndef __AIE_API_USE_NATIVE_1024B_VECTOR__
#define __AIE_API_USE_NATIVE_1024B_VECTOR__
#endif
#include "aie_api/aie_adf.hpp"
#include "matrix_mult_untiler.hpp"
#include "matrix_mult_tiler_common.hpp"

#ifndef ROW_MAJOR
#define ROW_MAJOR 0
#endif
#ifndef COL_MAJOR
#define COL_MAJOR 1
#endif

// #define MATMUL_DEBUG

namespace xf {
namespace dsp {
namespace aie {
namespace blas {
namespace matrix_mult {

constexpr unsigned int makeUnTileShuffleOffsets(
    unsigned M, unsigned N, unsigned vectorSize, unsigned leadingDim, bool hiLo) { //, unsigned int loadSize) {
    const unsigned tileSize = (M * N);
    const unsigned tilesPerVector = vectorSize / tileSize;
    const unsigned mIncr = N;
    const unsigned nIncr = 1;
    const unsigned tileIncr = tileSize;

    const unsigned nLaneIncr = (leadingDim == ROW_MAJOR) ? 1 : M;
    const unsigned tileLaneIncr =
        (leadingDim == ROW_MAJOR)
            ? N
            : tileSize; // if rowMajor just continue N  across tiles, otherwise treat each tile indendantly.
    const unsigned mLaneIncr = (leadingDim == ROW_MAJOR) ? N * tilesPerVector : 1; // if row major, we're trying to get
                                                                                   // as many columns contiguous so rows
                                                                                   // are furthest apart. if col_major,
                                                                                   // rows are stored contiguously

    unsigned long int ret = 0;
    // rowMajor
    for (unsigned mIndex = 0; mIndex < M; ++mIndex) {
        for (unsigned tileIndex = 0; tileIndex < tilesPerVector; ++tileIndex) {
            for (unsigned nIndex = 0; nIndex < N; ++nIndex) {
                const unsigned laneIndex = nIndex * nLaneIncr + tileIndex * tileLaneIncr + mIndex * mLaneIncr;
                const unsigned sampleIndex = nIndex * nIncr + tileIndex * tileSize + mIndex * mIncr;

                if (laneIndex < 8 && !hiLo) {
                    ret += sampleIndex << (4 * (laneIndex % 8));
                } else if (laneIndex >= 8 && hiLo) {
                    ret += sampleIndex << (4 * (laneIndex % 8));
                }
            }
        }
    }
    return ret;
}
constexpr loHi getUnTileShuffleOffsets(unsigned M, unsigned N, unsigned vectorSize, unsigned leadingDim) {
    loHi ret = {.lo = makeUnTileShuffleOffsets(M, N, vectorSize, leadingDim, false),
                .hi = makeUnTileShuffleOffsets(M, N, vectorSize, leadingDim, true)};
    return ret;
}

constexpr loHi getUnTileShuffleOffsetsInt16(unsigned M, unsigned N, unsigned vectorSize, unsigned leadingDim) {
    // we don't support colMajor untiling with int16

    unsigned int andMask = 0x0F0F0F0F; // every odd offset is handled differently for int16
    // 3 is from M-1 (M assumed to be 4 when N=2).
    unsigned int orMask = N == 2 ? 0x30303030 : 0x0; // don't do anything for N=4
    unsigned int offLo = (makeUnTileShuffleOffsets(M, N / 2, vectorSize / 2, leadingDim, false) & andMask) | orMask;
    unsigned int offHi = (makeUnTileShuffleOffsets(M, N / 2, vectorSize / 2, leadingDim, true) & andMask) | orMask;
    loHi ret = {.lo = offLo, .hi = offHi};
    return ret;
}

template <typename T_D, unsigned inRow, unsigned inCol, unsigned hasMulLoadsPerRow>
static constexpr int getVecSize() {
    constexpr unsigned minVBuffSizeforType = (512 / 8) / sizeof(T_D); // not sure why this is 512 bits?
    if
        constexpr(minVBuffSizeforType > (inRow * inCol)) { return inRow * inCol; }
    else if (!((hasMulLoadsPerRow) && (minVBuffSizeforType <= inCol) && (inCol % minVBuffSizeforType != 0))) {
        return minVBuffSizeforType;
    } else {
        int vSize = minVBuffSizeforType;
        while (inCol % vSize != 0) {
            vSize /= 2;
        }
        return vSize;
    }
}

template <unsigned M, unsigned N, unsigned inRow, unsigned inCol, unsigned leadingDim, typename T_D>
static void doUnTile(T_D* __restrict inPtr, T_D* outPtr) {
    constexpr unsigned minGranularity = (128 / 8) / sizeof(T_D);
    constexpr unsigned loadSize = (N >= minGranularity) ? N : minGranularity;
    // constexpr unsigned loadSize = (N >= minGranularity && (N < 8)) ? N : minGranularity;
    constexpr unsigned hasMulLoadsPerRow = (loadSize <= N && leadingDim == ROW_MAJOR);
    // constexpr unsigned vectorSize = getVecSize<T_D, inRow, inCol, hasMulLoadsPerRow>();
    constexpr unsigned minVBuffSizeforType = (512 / 8) / sizeof(T_D);
    constexpr unsigned vectorSize = (minVBuffSizeforType < (M * N)) ? 2 * minVBuffSizeforType : minVBuffSizeforType;
    //
    // static_assert((vectorSize * sizeof(T_D) < 128), "Error: This DATA_TYPE combination does not support the given
    // matrix sizes. Consider padding up the matrices to the closest multiples of tiling sizes.");

    // static_assert(N >= minGranularity, "Granularity is awkward");
    static_assert(vectorSize <= (1024 / 8) / sizeof(T_D), "calculated vector size too large for vector register.");
    static_assert(!(leadingDim == COL_MAJOR && std::is_same_v<T_D, int16>),
                  "UnTiling is not supported for int16 matrix if matrix is column major.");
    const unsigned largeTile = (M * N) * (inCol / N);
    loHi offsets = std::is_same_v<T_D, int16> ? getUnTileShuffleOffsetsInt16(M, N, vectorSize, leadingDim)
                                              : getUnTileShuffleOffsets(M, N, vectorSize, leadingDim);

    const unsigned loadsPerVector = vectorSize / loadSize;
    const unsigned tilesPerVector = vectorSize / (M * N);
    const unsigned colsPerLoad =
        (loadSize <= N)
            ? loadSize
            : N * std::max((loadSize / (M * N)),
                           (unsigned)1); // deal with edge case where can load multiple full tiles with a single load
    const unsigned rowsPerLoad = (loadSize <= N) ? 1 : (((loadSize / N) - 1) % M) + 1;

    // We move pointers between loads to optimise for col/row major.
    const unsigned colsPerVector =
        (hasMulLoadsPerRow) ? (vectorSize <= inCol) ? vectorSize : inCol : tilesPerVector * N;
    const unsigned rowsPerVector = (hasMulLoadsPerRow) ? (vectorSize <= inCol) ? 1 : vectorSize / inCol : M; // todo?

    const unsigned vectorsPerCol = inRow / rowsPerVector;
    const unsigned vectorsPerRow = inCol / colsPerVector;

    // Loop through a row first if row major
    const unsigned outerLoopCount = (leadingDim == ROW_MAJOR) ? vectorsPerCol : vectorsPerRow;
    const unsigned innerLoopCount = (leadingDim == ROW_MAJOR) ? vectorsPerRow : vectorsPerCol;

    const unsigned innerLoopIncr = (leadingDim == ROW_MAJOR) ? colsPerVector * M : rowsPerVector * inCol;
    const unsigned outerLoopIncr = (leadingDim == ROW_MAJOR) ? rowsPerVector * inCol : colsPerVector * M;

    const unsigned outerDimPerVector = (leadingDim == ROW_MAJOR) ? rowsPerVector : colsPerVector;
    const unsigned innerDimPerVector = (leadingDim == ROW_MAJOR) ? colsPerVector : rowsPerVector;

    const unsigned storeSize = loadSize; // May optimise store size to use 256b where possible.
    const unsigned outerDimStoreIncr = (leadingDim == ROW_MAJOR) ? inCol : inRow;
    const unsigned innerDimStoreIncr = storeSize;

    const bool shuffleIsNeeded = ((leadingDim == COL_MAJOR) || ((leadingDim == ROW_MAJOR) && (loadSize > N)));
    const int shuffleVSizeAllowed = (1024 / 8) / sizeof(T_D);
    // static_assert(!(shuffleIsNeeded && vectorSize != shuffleVSizeAllowed), "Untiling does not support these matrix
    // dimensions for this data type combination.");

    for (unsigned outerDimIdx = 0; outerDimIdx < outerLoopCount; ++outerDimIdx)
        chess_loop_count((outerLoopCount)) chess_prepare_for_pipelining {
            const unsigned ptrOuterBase = (leadingDim == ROW_MAJOR)
                                              ? (rowsPerVector < M)
                                                    ? (((outerDimIdx * rowsPerVector) % M) * N) +
                                                          (((outerDimIdx * rowsPerVector) / M) * (inCol * M))
                                                    : outerDimIdx * outerLoopIncr
                                              : outerDimIdx * outerLoopIncr;
            for (unsigned innerDimIdx = 0; innerDimIdx < innerLoopCount; ++innerDimIdx)
                chess_loop_count((innerLoopCount)) chess_prepare_for_pipelining {
                    const unsigned ptrInnerBase = innerDimIdx * innerLoopIncr;

                    aie::vector<T_D, vectorSize> vec;
                    using vType = aie::vector<T_D, vectorSize>;
                    using vTypeHalf = aie::vector<T_D, vectorSize / 2>;

#pragma unroll((loadsPerVector))
                    for (unsigned loadIdx = 0; loadIdx < loadsPerVector; ++loadIdx) {
                        const unsigned innerLoadPtr =
                            (leadingDim == ROW_MAJOR)
                                ? (loadSize <= N)
                                      ? loadSize * (loadIdx % ((N / loadSize) * rowsPerVector)) +
                                            (loadIdx / ((N / loadSize) * rowsPerVector)) * M * N
                                      : // skip over the rows; only load columns within a single row.
                                      loadSize * loadIdx
                                : // colMajor
                                (loadSize <= N * M)
                                    ?
                                    // potenially could do something different for loadSize<N
                                    // previously was inCol * M for outerTile increment - changed to go to the next tile
                                    // out of simplicity.
                                    loadSize * (loadIdx % ((M * N) / loadSize)) +
                                        (loadIdx / ((M * N) / loadSize)) * (M * N)
                                    : // within one tile, just load the whole tile. outwith that tile, move to the next
                                      // tile. - This could be simplied to just loadSize * idx
                                    loadSize * loadIdx; // unlikely

                        const unsigned loadPtr = innerLoadPtr + ptrInnerBase + ptrOuterBase;
                        vec.insert(loadIdx, aie::load_v<loadSize>(inPtr + loadPtr));
                    }

                    if
                        constexpr(shuffleIsNeeded) {
#ifdef __SUPPORTS_ACC64__
                            if
                                constexpr(leadingDim == ROW_MAJOR) {
                                    // Interleave zip needed for ROW_MAJOR when there are two tiles per vector (int16
                                    // 4x4 tile)
                                    vTypeHalf vecA = vec.template extract<vectorSize / 2>(0);
                                    vTypeHalf vecB = vec.template extract<vectorSize / 2>(1);
                                    std::pair<vTypeHalf, vTypeHalf> vecZip = aie::interleave_zip(vecA, vecB, N);
                                    vec = aie::concat(vecZip.first, vecZip.second);
                                }
                            else {
                                // vTypeHalf vecA = aie::filter_even(vec, 1);
                                // vTypeHalf vecB = aie::filter_odd(vec, 1);
                                // myprint(vecA, true, "vecA: ");
                                // myprint(vecB, true, "vecB: ");
                                // vec = aie::concat(vecA, vecB);
                                vec = aie::transpose(vec, M, N);
                            }
#else
                            vec = doShuffle(vec, 0, offsets);
#endif //__SUPPORTS_ACC64__
                        }
#pragma unroll((outerDimPerVector))
                    for (unsigned outerStoreIdx = 0; outerStoreIdx < outerDimPerVector; ++outerStoreIdx) {
                        const unsigned storeOuterPtr = outerStoreIdx * outerDimStoreIncr;
#pragma unroll((std::max(innerDimPerVector / storeSize, (unsigned) 1)))
                        for (unsigned innerStoreIdx = 0;
                             innerStoreIdx < std::max(innerDimPerVector / storeSize, (unsigned)1); ++innerStoreIdx) {
                            // If we don't shuffle and still load multiple outerDims, then we need to skip over that.
                            const unsigned sliceIdx =
                                (!shuffleIsNeeded && outerDimPerVector > 1)
                                    ? innerStoreIdx * outerDimPerVector + outerStoreIdx
                                    : innerStoreIdx + outerStoreIdx * (innerDimPerVector / storeSize);
                            const unsigned storePtr = innerStoreIdx * storeSize + storeOuterPtr +
                                                      innerDimIdx * innerDimPerVector +
                                                      outerDimIdx * outerDimPerVector * outerDimStoreIncr;

                            // store direct to window
                            aie::store_v(outPtr + storePtr, vec.template extract<storeSize>(sliceIdx));
                        }
                    }
                }
        }
}

namespace aie = ::aie;
template <unsigned M, unsigned N, unsigned inRow, unsigned inCol, unsigned leadingDim, typename T_D>
void untilerKernelClass<M, N, inRow, inCol, leadingDim, T_D>::unTile(input_buffer<T_D>& __restrict inWindow,
                                                                     output_buffer<T_D>& __restrict outWindow) {
    doUnTile<M, N, inRow, inCol, leadingDim, T_D>((T_D*)inWindow.data(), (T_D*)outWindow.data());
};
}
}
}
}
}

#endif
