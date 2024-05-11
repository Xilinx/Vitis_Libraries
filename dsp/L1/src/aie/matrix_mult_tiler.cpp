/*
 * Copyright (C) 2019-2022, Xilinx, Inc.
 * Copyright (C) 2022-2024, Advanced Micro Devices, Inc.
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
#ifndef _DSPLIB_MATRIX_MULT_TILER_CPP_
#define _DSPLIB_MATRIX_MULT_TILER_CPP_

#include <adf.h>

#ifndef __NEW_WINDOW_H__
#endif
// if we use 1kb registers -> aie api uses 2x512b registers for 1024b so we need this for QoR
#ifndef __AIE_API_USE_NATIVE_1024B_VECTOR__
#define __AIE_API_USE_NATIVE_1024B_VECTOR__
#endif
#include "aie_api/aie_adf.hpp"
#include "matrix_mult_tiler.hpp"
#include "matrix_mult_tiler_common.hpp"

// #define _DSPLIB_MATRIX_MULT_TILER_HPP_DEBUG_

// #define MATMUL_DEBUG

#ifndef ROW_MAJOR
#define ROW_MAJOR 0
#endif
#ifndef COL_MAJOR
#define COL_MAJOR 1
#endif
namespace xf {
namespace dsp {
namespace aie {
namespace blas {
namespace matrix_mult {

constexpr unsigned int makeShuffleOffsets(
    unsigned M, unsigned N, unsigned vectorSize, unsigned leadingDim, bool hiLo) //, unsigned int loadSize) {
{
    unsigned int loadSize = vectorSize == 8 ? 2 : 4;
    unsigned long int ret = 0;
    unsigned mIncr = (leadingDim == ROW_MAJOR) ? (loadSize < N) ? (N / loadSize) * loadSize : loadSize : 1; // transpose
    // We first load enough for M, then we load for N when we're tranpose.
    unsigned nIncr = (leadingDim == ROW_MAJOR) ? 1 : (loadSize < M) ? (M / loadSize) * loadSize : loadSize; // tranpose
    for (unsigned int tileIndex = 0; tileIndex < vectorSize / (M * N); ++tileIndex) {
        // This has two parts. If N is smaller than the loadsize, then we first deal with those extra samples.
        // Then we deal with the remaining samples.
        const unsigned tileStartIndex =
            (leadingDim == ROW_MAJOR) ? ((tileIndex * N) % loadSize + ((tileIndex * N) / loadSize) * (M * loadSize))
                                      : ((tileIndex % ((vectorSize / loadSize) / N)) * N * loadSize +
                                         tileIndex / ((vectorSize / loadSize) / N) * M);

        for (unsigned int mIndex = 0; mIndex < M; ++mIndex) {
            for (unsigned int nIndex = 0; nIndex < N; ++nIndex) {
                const unsigned laneIndex = nIndex + mIndex * N + tileIndex * M * N;
                // This effectively deals with tranpose at the offsets level.
                const unsigned sampleIndex = (tileStartIndex + mIndex * mIncr + nIndex * nIncr);
                if (laneIndex < 8 && !hiLo) {
                    ret += ((unsigned long)sampleIndex) << (4 * (laneIndex % 8));
                } else if (laneIndex >= 8 && hiLo) {
                    ret += ((unsigned long)sampleIndex) << (4 * (laneIndex % 8));
                }
            }
        }
    }
    return ret;
}
constexpr loHi getShuffleOffsets(unsigned M, unsigned N, unsigned vectorSize, unsigned leadingDim) {
    loHi ret = {
        .lo = makeShuffleOffsets(M, N, vectorSize, leadingDim, false),
        .hi = makeShuffleOffsets(M, N, vectorSize, leadingDim, true),
    };
    return ret;
}
constexpr loHi getShuffleOffsetsInt16(const unsigned M,
                                      const unsigned N,
                                      const unsigned vectorSize,
                                      const unsigned leadingDim) {
    unsigned int andMask = 0x0F0F0F0F; // every odd offset is handled differently for int16
    // This assumes that the loadsize is 8
    // An offset of 3 on odd index corresponds to previus row's odd index +1 + 2*offset
    // So if we have
    // I J
    // X Y
    // X = J+1 + Off*2; Y = X+1
    // We only have N=2 or N=4
    unsigned int orMask = N == 2 ? 0x30303030 : 0x0; // don't do anything for N=4
    unsigned int offLo = (makeShuffleOffsets(M, N / 2, vectorSize / 2, leadingDim, false) & andMask) | orMask;
    unsigned int offHi = (makeShuffleOffsets(M, N / 2, vectorSize / 2, leadingDim, true) & andMask) | orMask;
    // Hardcoding possible combinations, assumed N=2
    if (leadingDim == COL_MAJOR) {
        offLo = (M == 4) ? 0x39383130 : (M == 2) ? 0x39313830 : 0;
        offHi = (M == 4) ? 0x3B3A3332 : (M == 2) ? 0xB3333A32 : 0;
    }
    loHi ret = {
        .lo = offLo,
        .hi = offHi,
        .square = (leadingDim == ROW_MAJOR) ? (unsigned)0x3210
                                            : (unsigned)0x3120, // Need permute to grab sample from different 32b chunk
    };
    return ret;
}

template <unsigned M, unsigned N, unsigned inRow, unsigned inCol, unsigned leadingDim, typename T_D>
static void doTile(T_D* inPtr, T_D* outPtr) {
    constexpr unsigned minGranularity = (128 / 8) / sizeof(T_D);
    constexpr unsigned loadSize = (N >= minGranularity) ? N : minGranularity;
    constexpr unsigned minVBuffSizeforType = (512 / 8) / sizeof(T_D);
    constexpr unsigned vectorSize = minVBuffSizeforType;

    // static_assert(N >= minGranularity, "Granularity is awkward");
    static_assert(vectorSize <= (1024 / 8) / sizeof(T_D),
                  "ERROR: calculated vector size too large for vector register.");
    static_assert(!(N == 4 && leadingDim == COL_MAJOR && std::is_same_v<T_D, int16>),
                  "ERROR: Tiling is not supported for column major int16 matrix.");

    constexpr unsigned int sizeTileA = M * N;
    // Window size in terms of elements
    const unsigned windowSizeA = inRow * inCol;
    const unsigned largeTile = (M * N) * (inCol / N);

    // This will get optimised away if not used.
    // Offsets for the shuffle intrinsic
    const loHi offsets = std::is_same_v<T_D, int16> ? getShuffleOffsetsInt16(M, N, vectorSize, leadingDim)
                                                    : getShuffleOffsets(M, N, vectorSize, leadingDim);
    const unsigned loadsPerVector = vectorSize / loadSize;

    const unsigned columnsPerVector =
        (leadingDim == ROW_MAJOR)
            ? (vectorSize / M <= inCol) ? loadSize * (loadsPerVector) / M : inCol
            : (loadSize >= inCol) ? inCol : (loadSize >= M) ? (loadsPerVector) : (loadsPerVector) / (M / loadSize);

    const unsigned rowsPerVector = vectorSize / columnsPerVector;
    const unsigned colElementDist = (leadingDim == ROW_MAJOR) ? 1 : inRow;
    const unsigned rowElementDist = (leadingDim == ROW_MAJOR) ? inCol : 1;
    const unsigned rowTilesPerVector = std::max((rowsPerVector / M), (unsigned)1);
    static_assert(
        inRow * inCol >= vectorSize,
        "ERROR: Matrix is too small to implement tiling. A single matrix must consume at least 512 bits of memory.");
    static_assert(((inRow * inCol) % minVBuffSizeforType == 0) || (leadingDim == 0),
                  "ERROR: Column major matrices must be a multiple of 512-bits to implement tiling");
    const unsigned vectorsPerCol = inRow / rowsPerVector;
    const unsigned vectorsPerRow = inCol / columnsPerVector;
    for (unsigned rowPos = 0; rowPos < vectorsPerCol; ++rowPos)
        chess_loop_count((vectorsPerCol)) chess_prepare_for_pipelining {
            unsigned outI = rowPos * largeTile;
            const unsigned rowIndex = rowPos * rowElementDist * rowsPerVector;
            // printf("new LargeTile\n");
            // Travel down the column
            for (unsigned colPos = 0; colPos < vectorsPerRow; ++colPos)
                chess_loop_count(vectorsPerRow) chess_prepare_for_pipelining {
                    // printf("rowPos=%d, colPos=%d\n",rowPos, colPos );
                    const unsigned colIndex = colPos * colElementDist * columnsPerVector;
                    aie::vector<T_D, vectorSize> chunk;
#pragma unroll((loadsPerVector))
                    for (unsigned i = 0; i < (loadsPerVector); ++i) {
                        // Todo: deal with the possibility that inCol < loadSize for COL_MAJOR
                        const unsigned loadPos =
                            (leadingDim == ROW_MAJOR)
                                ? (loadSize >= inCol) ? i * loadSize : (i % M) * inCol + (i / M) * loadSize
                                : (loadSize >= M) ? (inRow * (i % inCol) + (i / inCol) * M)
                                                  : inRow * (i / (M / loadSize)) + (i % (M / loadSize)) * loadSize;
                        const unsigned pointerLoc = colIndex + rowIndex + loadPos;

                        chunk.insert(i, aie::load_v<loadSize>(inPtr + pointerLoc));
                    }
                    // If N is 4
                    // shuffle16(chunk, 0, 0xD951C840)
                    // shuffle16(chunk, 2, 0xD951C840)
                    // initialise to chunk
                    aie::vector<T_D, vectorSize> mychunk;
                    // If N is 2 & loadsize=4
                    if
                        constexpr(N < minGranularity || leadingDim == COL_MAJOR) {
                            mychunk = doShuffle(chunk, 0, offsets);
                        }
                    else {
                        mychunk = chunk;
                    }
                    const unsigned resultIndexBase =
                        vectorSize * (vectorsPerRow)*rowPos + (vectorSize / rowTilesPerVector) * colPos;
                    outI += vectorSize;
// If we end up loading more rows than we need for a single tile in a vector, need to store this somewhere else

#pragma unroll((rowTilesPerVector))
                    for (unsigned tile = 0; tile < rowTilesPerVector; ++tile) {
                        const unsigned resultIndexPos = resultIndexBase + tile * largeTile;
                        aie::store_v(outPtr + resultIndexPos,
                                     mychunk.template extract<vectorSize / rowTilesPerVector>(tile));
                    }
                }
        }
}
template <unsigned M, unsigned N, unsigned inRow, unsigned inCol, unsigned leadingDim, typename T_D>
static void shuffleTile(T_D* inPtr, T_D* outPtr) {
    constexpr unsigned minGranularity = (128 / 8) / sizeof(T_D);
    constexpr unsigned loadSize = (leadingDim == ROW_MAJOR) ? (N >= minGranularity) ? N : minGranularity
                                                            : (M >= minGranularity) ? M : minGranularity;
    constexpr unsigned minVBuffSizeforType = (512 / 8) / sizeof(T_D);
    // double vector size if tile doesn't fit in 512 buffer
    // constexpr unsigned vectorSize = (M*N >= minVBuffSizeforType) ? (2*minVBuffSizeforType) : minVBuffSizeforType;
    constexpr unsigned vectorSize = (minVBuffSizeforType < (M * N)) ? 2 * minVBuffSizeforType : minVBuffSizeforType;
    // static_assert(N >= minGranularity, "Granularity is awkward");
    static_assert(vectorSize <= (1024 / 8) / sizeof(T_D),
                  "ERROR: calculated vector size too large for vector register.");
    static_assert(!(N == 4 && leadingDim == COL_MAJOR && std::is_same_v<T_D, int16>),
                  "ERROR: Tiling is not supported for column major int16 matrix.");
    constexpr unsigned int sizeTileA = M * N;
    // Window size in terms of elements
    const unsigned windowSizeA = inRow * inCol;
    const unsigned largeTile = (M * N) * (inCol / N);
    const unsigned loadsPerVector = vectorSize / loadSize;
    const unsigned columnsPerVector =
        (leadingDim == ROW_MAJOR)
            ? (vectorSize / M <= inCol) ? loadSize * (loadsPerVector) / M : // vectorSize/loadSize
                  inCol
            : (loadSize >= inCol) ? inCol : (loadSize >= M) ? (loadsPerVector) : (loadsPerVector) / (M / loadSize);
    const unsigned rowsPerVector =
        (leadingDim == ROW_MAJOR) ? (vectorSize / M <= inCol) ? M : vectorSize / inCol : (loadSize >= M) ? loadSize : M;
    const unsigned colElementDist = (leadingDim == ROW_MAJOR) ? 1 : inRow;
    const unsigned rowElementDist = (leadingDim == ROW_MAJOR) ? inCol : 1;
    const unsigned rowTilesPerVector = std::max((rowsPerVector / M), (unsigned)1);
    static_assert(
        inRow * inCol >= vectorSize,
        "ERROR: Matrix is too small to implement tiling. A single matrix must consume at least 512 bits of memory.");
    const unsigned vectorsPerCol = inRow / rowsPerVector;
    const unsigned vectorsPerRow = inCol / columnsPerVector;
    using vTypeFull = aie::vector<T_D, vectorSize>;
    using vTypeHalf = aie::vector<T_D, vectorSize / 2>;
    using vTypeQuarter = aie::vector<T_D, vectorSize / 4>;
    using vTypeLoad = aie::vector<T_D, loadSize>;
    vTypeLoad readVal;
    const unsigned tilesPerLoad = loadSize / N;
    const unsigned loadsPerRow = columnsPerVector / loadSize;
    const unsigned tilesPerCol = rowsPerVector / M;
    const unsigned tilesPerRow = columnsPerVector / N;
    const unsigned tilesInVector = vectorSize / (N * M);
    for (unsigned rowPos = 0; rowPos < vectorsPerCol; ++rowPos)
        chess_loop_count((vectorsPerCol)) chess_prepare_for_pipelining {
            unsigned outI = rowPos * largeTile;
            unsigned rowIndex = rowPos * rowElementDist * rowsPerVector;
            // printf("new LargeTile\n");
            // Travel down the column
            for (unsigned colPos = 0; colPos < vectorsPerRow; ++colPos)
                chess_loop_count(vectorsPerRow) chess_prepare_for_pipelining {
                    // printf("rowPos=%d, colPos=%d\n",rowPos, colPos );
                    unsigned colIndex = colPos * colElementDist * columnsPerVector;
                    aie::vector<T_D, vectorSize> chunk;
#pragma unroll((loadsPerVector))
                    for (unsigned i = 0; i < (loadsPerVector); ++i) {
                        // Todo: deal with the possibility that inCol < loadSize for COL_MAJOR
                        unsigned loadPos =
                            (leadingDim == ROW_MAJOR)
                                ? (loadSize >= inCol) ? i * loadSize : (i % M) * inCol + (i / M) * loadSize
                                : (loadSize >= M) ? inRow * i
                                                  : inRow * (i / (M / loadSize)) + (i % (M / loadSize)) * loadSize;
                        unsigned pointerLoc = colIndex + rowIndex + loadPos;
                        readVal = aie::load_v<loadSize>(inPtr + pointerLoc);
                        chunk.insert(i, readVal);
                    }
                    // initialise to chunk
                    vTypeFull mychunk;

                    if
                        constexpr(N < minGranularity || leadingDim == COL_MAJOR) {
                            // mychunk = doShuffle(chunk, 0, offsets);
                            if
                                constexpr(leadingDim == ROW_MAJOR) {
                                    if
                                        constexpr(tilesInVector == 4 && tilesPerLoad == 4) {
                                            vTypeHalf chunkA = aie::filter_even(chunk, loadSize / 2);
                                            vTypeHalf chunkB = aie::filter_odd(chunk, loadSize / 2);
                                            vTypeQuarter chunkA1 = aie::filter_even(chunkA, N);
                                            vTypeQuarter chunkA2 = aie::filter_odd(chunkA, N);
                                            vTypeQuarter chunkB1 = aie::filter_even(chunkB, N);
                                            vTypeQuarter chunkB2 = aie::filter_odd(chunkB, N);
                                            mychunk = aie::concat(chunkA1, chunkA2, chunkB1, chunkB2);
                                        }
                                    else if
                                        constexpr(tilesInVector == 4 && tilesPerLoad == 2) {
                                            // loadSize/colsPerVector
                                            // rowsPerVector / M
                                            vTypeHalf chunkA = chunk.template extract<vectorSize / 2>(0);
                                            vTypeHalf chunkB = chunk.template extract<vectorSize / 2>(1);
                                            vTypeQuarter chunkA1 = aie::filter_even(chunkA, N);
                                            vTypeQuarter chunkA2 = aie::filter_odd(chunkA, N);
                                            vTypeQuarter chunkB1 = aie::filter_even(chunkB, N);
                                            vTypeQuarter chunkB2 = aie::filter_odd(chunkB, N);
                                            mychunk = aie::concat(chunkA1, chunkA2, chunkB1, chunkB2);
                                        }
                                    else if
                                        constexpr(tilesInVector == 2 && tilesPerLoad == 2) {
                                            vTypeHalf chunkA = aie::filter_even(chunk, N);
                                            vTypeHalf chunkB = aie::filter_odd(chunk, N);
                                            mychunk = aie::concat(chunkA, chunkB);
                                        }
                                    else {
                                        mychunk = chunk;
                                    }
                                }
                            if
                                constexpr(leadingDim == COL_MAJOR) {
                                    vTypeFull chunkT;
                                    if
                                        constexpr(tilesInVector == 1) {
                                            chunkT = (M > 1) ? aie::transpose(chunk, N, M) : chunk;
                                        }
                                    else {
                                        chunkT = aie::transpose(chunk, columnsPerVector, rowsPerVector);
                                    }
                                    if
                                        constexpr(tilesInVector == 4) {
                                            vTypeHalf chunkA = chunkT.template extract<vectorSize / 2>(0);
                                            vTypeHalf chunkB = chunkT.template extract<vectorSize / 2>(1);
                                            vTypeQuarter chunkA1 = aie::filter_even(chunkA, N);
                                            vTypeQuarter chunkA2 = aie::filter_odd(chunkA, N);
                                            vTypeQuarter chunkB1 = aie::filter_even(chunkB, N);
                                            vTypeQuarter chunkB2 = aie::filter_odd(chunkB, N);
                                            mychunk = aie::concat(chunkA1, chunkA2, chunkB1, chunkB2);
                                        }
                                    else if
                                        constexpr(tilesInVector == 2) {
                                            vTypeHalf chunkA = aie::filter_even(chunkT, N);
                                            vTypeHalf chunkB = aie::filter_odd(chunkT, N);
                                            mychunk = aie::concat(chunkA, chunkB);
                                        }
                                    else {
                                        mychunk = chunkT;
                                    };
                                }
                        }
                    else {
                        mychunk = chunk;
                    }

                    const unsigned resultIndexBase =
                        vectorSize * (vectorsPerRow)*rowPos + (vectorSize / rowTilesPerVector) * colPos;
                    outI += vectorSize;

// If we end up loading more rows than we need for a single tile in a vector, need to store this somewhere else
#pragma unroll((rowTilesPerVector))
                    for (unsigned tile = 0; tile < rowTilesPerVector; ++tile) {
                        const unsigned resultIndexPos = resultIndexBase + tile * largeTile;
                        aie::store_v(outPtr + resultIndexPos,
                                     mychunk.template extract<vectorSize / rowTilesPerVector>(tile));
                    }
                }
        }
}

namespace aie = ::aie;
template <unsigned M, unsigned N, unsigned inRow, unsigned inCol, unsigned leadingDim, typename T_D>
void tilerKernelClass<M, N, inRow, inCol, leadingDim, T_D>::tile(input_buffer<T_D>& __restrict inWindow,
                                                                 output_buffer<T_D>& __restrict outWindow) {
#ifdef __SUPPORTS_ACC64__
    shuffleTile<M, N, inRow, inCol, leadingDim, T_D>((T_D*)inWindow.data(), (T_D*)outWindow.data());
#else
    doTile<M, N, inRow, inCol, leadingDim, T_D>((T_D*)inWindow.data(), (T_D*)outWindow.data());
#endif //__SUPPORTS_ACC64__
};
}
}
}
}
}

#endif
