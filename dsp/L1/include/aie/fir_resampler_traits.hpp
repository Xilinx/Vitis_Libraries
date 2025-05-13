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
#ifndef _DSPLIB_fir_resampler_TRAITS_HPP_
#define _DSPLIB_fir_resampler_TRAITS_HPP_

#include <array> //for phase arrays

#include "fir_utils.hpp"

namespace xf {
namespace dsp {
namespace aie {
namespace fir {
namespace resampler {
/*
Resampler FIR traits.
This file contains sets of overloaded, templatized and specialized templatized functions which
encapsulate properties of the intrinsics used by the main kernal class. Specifically,
this file does not contain any vector types or intrinsics since it is required for construction
and therefore must be suitable for the aie compiler graph-level compilation.
*/
// Defensive checks
template <typename TT_DATA, typename TT_COEFF>
INLINE_DECL constexpr unsigned int fnFirInterpFractTypeSupport() {
    return fnUnsupportedTypeCombo<TT_DATA, TT_COEFF>();
}; // Int16 data int16 coeff type combo unsupported, due to xquare.

struct firParamsTrait {
    // typename dataType = TT_DATA;
    unsigned int dataSizeBytes;
    unsigned int firLen;
    unsigned int loadSize;
    unsigned int dataBuffSamples;
    unsigned int alignWindowReadBytes;
    unsigned int marginOffsetIndex;
    unsigned int rangeOffset;
};

enum eArchType { kArchBasic, kArchStream, kArchPhaseParallel, kArchStreamPhaseParallel };

// function to return the number of lanes for a type combo
template <typename TT_DATA, typename TT_COEFF, unsigned int TP_API = 0>
INLINE_DECL constexpr unsigned int fnNumLanesResampler() {
    return (TP_API == 0 ? fnNumLanes<TT_DATA, TT_COEFF>() : fnNumLanes384<TT_DATA, TT_COEFF>());
};

// function to return the number of columns for a type combo for the intrinsics used for this single rate asym FIR
template <typename TT_DATA, typename TT_COEFF, unsigned int TP_API = 0>
INLINE_DECL constexpr unsigned int fnNumColumnsResampler() {
    return (TP_API == 0 ? fnNumCols<TT_DATA, TT_COEFF>() : fnNumCols384<TT_DATA, TT_COEFF>());
};
template <>
INLINE_DECL constexpr unsigned int fnNumColumnsResampler<int16, int32>() {
    return 1;
}; // only single columns can be used due to MAC operating on 256-bit coeff buffer.

// Obsolete
template <typename TT_DATA>
INLINE_DECL constexpr unsigned int fnDataLoadsInRegResampler() {
    // To fill a full 1024-bit input vector register using a 256-bit upd_w command it takes 4 load operations.
    // Always return 4, the exception (handled by explicit template instantiation)
    // would be needed it a different command was used (e.g. upd_v, upd_x).
    // Or when using 128-bit streams.
    return kXYBuffSize / kUpdWSize;
}

// Parameter to constant resolution functions
template <typename TT_DATA>
INLINE_DECL constexpr unsigned int fnDataLoadVsizeResampler() {
    return (kUpdWSize / sizeof(TT_DATA));
}

// // Function returns window access size, the memory data path is either 128-bits or 256-bits wide for vector
// operations.
// template<typename TT_DATA, typename TT_COEFF>
// INLINE_DECL constexpr unsigned int fnWinAccessByteSize()
// {
//   return 16;
// };

// function to return the number of samples in an output vector for a type combo
template <typename TT_DATA, typename TT_COEFF, unsigned int TP_API>
INLINE_DECL constexpr unsigned int fnVOutSizeResampler() {
    return fnNumLanesResampler<TT_DATA, TT_COEFF, TP_API>();
};

// Get the first data sample required for an arbritary output
constexpr unsigned int getSampleIndexOnLane(
    unsigned int D, unsigned int I, unsigned int nLanes, unsigned int laneIndex, unsigned int phaseIndex) {
    unsigned int trueIndex = (phaseIndex * nLanes + laneIndex);
    unsigned int trueSample = (unsigned int)((trueIndex * D) / I);
    return trueSample;
}
constexpr unsigned int getSampleIndexOnLaneCol(unsigned int D,
                                               unsigned int I,
                                               unsigned int nLanes,
                                               unsigned int laneIndex,
                                               unsigned int phaseIndex,
                                               unsigned int colIndex) {
    return colIndex + getSampleIndexOnLane(D, I, nLanes, laneIndex, phaseIndex);
}

// Work out the samples that are required to be loaded into the buffer for a given operation, taking into account window
// read granularity.
constexpr std::array<int, 2> getBuffMinMax(firParamsTrait params,
                                           unsigned int D,
                                           unsigned int I,
                                           unsigned int nLanes,
                                           unsigned int nCols,
                                           unsigned int phaseIndex,
                                           unsigned int opIndex) {
    // For some reason this function only works if these are signed...
    int firLen = params.firLen;
    int dataSizeBytes = params.dataSizeBytes;
    int buffSampleSize = params.dataBuffSamples; //(1024/8)/dataSizeBytes;
    int alignRead =
        params.alignWindowReadBytes / dataSizeBytes; //(128/8)/dataSizeBytes; // alignment granularity of AIE.
    int loadSize = params.loadSize;                  //(256/8)/dataSizeBytes;
    int marginOffsetIndex = params.marginOffsetIndex;
    // these are min needed for first opIndex only. Our dataNeeded and dataLoaded accurately handle subsequent ops.
    int minSampleNeeded = (int)(getSampleIndexOnLaneCol(D, I, nLanes, 0, phaseIndex, 0)) - marginOffsetIndex;
    int maxSampleNeeded =
        (int)(getSampleIndexOnLaneCol(D, I, nLanes, nLanes - 1, phaseIndex, (opIndex * nCols + (nCols - 1)))) -
        marginOffsetIndex;

    // Align sample needed to windRead Granularity
    int minSbuff =
        ((int)((minSampleNeeded < 0 ? minSampleNeeded - (alignRead - 1) : minSampleNeeded) / alignRead)) * alignRead;
    // maxSampleNeeded, align to window read access granularity.
    int maxSbuff =
        ((int)((maxSampleNeeded < 0 ? (maxSampleNeeded + 1) : (maxSampleNeeded + 1 + (alignRead - 1))) / alignRead)) *
        alignRead;
    // Align Sbuff to actual loadSize.
    maxSbuff = minSbuff + ((int)(((maxSbuff - minSbuff) + (loadSize - 1)) / loadSize)) * loadSize;

    minSbuff = ((maxSbuff - minSbuff) > buffSampleSize) ? (maxSbuff - buffSampleSize) : minSbuff;

    return {minSbuff, maxSbuff};
}

// Work out what data we need in the buffer for each phase and hence figure out the required window decrement/incrment
// TODO: Optimisation to detect when buffers overlap, then only load the additional data that's required.  would likely
// reduce the number of phases required.
// (this is trickier because we need to preserve data already in the buffer, so using different xindex)
template <unsigned int polyphaseLaneAlias>
constexpr std::array<int, polyphaseLaneAlias> getWindowDecrements(
    firParamsTrait params, unsigned int D, unsigned int I, unsigned int nLanes, unsigned int nCols, unsigned int nOps) {
    std::array<int, polyphaseLaneAlias> windowDecs = {0};
    std::array<int, 2> buffMinMaxInitial = getBuffMinMax(params, D, I, nLanes, nCols, 0, 0);
    std::array<int, 2> buffMinMaxPrevFinal =
        getBuffMinMax(params, D, I, nLanes, nCols, polyphaseLaneAlias - 1, nOps - 1);
    const unsigned int MAX = 1;
    const unsigned int MIN = 0;
    int nextPhaseChunkStarting = (int)((polyphaseLaneAlias * nLanes * D) / I);
    // A negative dec would indicate an incr
    windowDecs[polyphaseLaneAlias - 1] = buffMinMaxPrevFinal[MAX] - (buffMinMaxInitial[MIN] + nextPhaseChunkStarting);

    for (unsigned int phase = 1; phase < polyphaseLaneAlias; phase++) {
        std::array<int, 2> buffMinMaxInitialOp = getBuffMinMax(params, D, I, nLanes, nCols, phase, 0);
        std::array<int, 2> buffMinMaxPrevFinalOp = getBuffMinMax(params, D, I, nLanes, nCols, phase - 1, nOps - 1);
        windowDecs[phase - 1] = buffMinMaxPrevFinalOp[MAX] - buffMinMaxInitialOp[MIN];
    }
    const std::array<int, polyphaseLaneAlias> windowDecsRet = windowDecs;
    return windowDecsRet;
}

// Very Similar to getWindowDecrements, we calculate the window decrements until the window decrements fit nicely into
// the number of phases and windowRead/Write granularity.
constexpr unsigned int calculateLaneAlias(firParamsTrait params,
                                          unsigned int D,
                                          unsigned int I,
                                          unsigned int nLanes,
                                          unsigned int nCols,
                                          unsigned int nOps,
                                          unsigned int polyAlias) {
    const unsigned int MAX = 1;
    const unsigned int MIN = 0;
    const unsigned int alignWindowReadSamples = params.alignWindowReadBytes / params.dataSizeBytes;
    // try multiples of polyphaseLaneAlias until the last window decrement aligns to windowDecrementGranularity.
    for (unsigned int polyphaseLaneAlias = polyAlias; polyphaseLaneAlias < polyAlias * 16;
         polyphaseLaneAlias += polyAlias) {
        std::array<int, 2> buffMinMaxInitial = getBuffMinMax(params, D, I, nLanes, nCols, 0, 0);
        std::array<int, 2> buffMinMaxPrevFinal =
            getBuffMinMax(params, D, I, nLanes, nCols, polyphaseLaneAlias - 1, nOps - 1);
        int nextPhaseChunkStarting = (int)((polyphaseLaneAlias * nLanes * D) / I);
        int lastWindowDecs = buffMinMaxPrevFinal[MAX] - (buffMinMaxInitial[MIN] + nextPhaseChunkStarting);
        if (lastWindowDecs % alignWindowReadSamples == 0) {
            return polyphaseLaneAlias;
        }
    }
    // couldn't find a reasonable solution (less than a multiple of 16 of polyphaseLaneAlias) - return 0 for a static
    // assert.
    return 0;
}

// Based on the data in the buffers for a given op, calculate initial loads
template <unsigned int polyphaseLaneAlias>
constexpr std::array<unsigned int, polyphaseLaneAlias> getInitialLoads(
    firParamsTrait params, unsigned int D, unsigned int I, unsigned int nLanes, unsigned int nCols) {
    std::array<unsigned int, polyphaseLaneAlias> initLoads = {0};
    unsigned int dataSizeBytes = params.dataSizeBytes;
    unsigned int loadSize = params.loadSize; //(256/8)/dataSizeBytes;
    const unsigned int MAX = 1;
    const unsigned int MIN = 0;
    // A negative dec would indicate an incr
    for (unsigned int phase = 0; phase < polyphaseLaneAlias; phase++) {
        std::array<int, 2> buffMinMaxInitial = getBuffMinMax(params, D, I, nLanes, nCols, phase, 0);
        initLoads[phase] = (buffMinMaxInitial[MAX] - buffMinMaxInitial[MIN]) / loadSize;
    }
    return initLoads;
}
// buffer alignment doesn't match the sample needed, so we need phase-varying xstart.
template <unsigned int polyphaseLaneAlias>
constexpr std::array<int, polyphaseLaneAlias> getXStarts(
    firParamsTrait params, unsigned int D, unsigned int I, unsigned int nLanes, unsigned int nCols) {
    std::array<int, polyphaseLaneAlias> xstarts = {0};
    unsigned int dataSizeBytes = params.dataSizeBytes;
    unsigned int loadSize = params.loadSize; //(256/8)/dataSizeBytes;
    const unsigned int MAX = 1;
    const unsigned int MIN = 0;
    // int minSampleNeeded;
    unsigned int firLen = params.firLen;
    int marginOffsetIndex = params.marginOffsetIndex;
    // A negative dec would indicate an incr
    for (unsigned int phase = 0; phase < polyphaseLaneAlias; phase++) {
        int minSampleNeeded = (int)(getSampleIndexOnLaneCol(D, I, nLanes, 0, phase, 0)) - marginOffsetIndex;
        std::array<int, 2> buffMinMaxInitial = getBuffMinMax(params, D, I, nLanes, nCols, phase, 0);
        xstarts[phase] = (minSampleNeeded - buffMinMaxInitial[MIN]);
    }
    return xstarts;
}
// returns the initial data needed starting value.
template <unsigned int polyphaseLaneAlias>
constexpr std::array<unsigned int, polyphaseLaneAlias> getDataNeeded(
    firParamsTrait params, unsigned int D, unsigned int I, unsigned int nLanes, unsigned int nCols) {
    std::array<unsigned int, polyphaseLaneAlias> dataNeeded = {0};
    unsigned int dataSizeBytes = params.dataSizeBytes;
    unsigned int loadSize = params.loadSize; // todo Link to current loadSize
    const unsigned int MAX = 1;
    const unsigned int MIN = 0;
    // int minSampleNeeded,maxSampleNeeded;
    unsigned int firLen = params.firLen; // todo
    int marginOffsetIndex = params.marginOffsetIndex;
    // A negative dec would indicate an incr
    for (unsigned int phase = 0; phase < polyphaseLaneAlias; phase++) {
        int minSampleNeeded = (int)(getSampleIndexOnLaneCol(D, I, nLanes, 0, phase, 0)) - marginOffsetIndex;
        int maxSampleNeeded =
            (int)(getSampleIndexOnLaneCol(D, I, nLanes, nLanes - 1, phase, (nCols - 1))) - marginOffsetIndex;
        dataNeeded[phase] = maxSampleNeeded - minSampleNeeded + 1;
    }
    return dataNeeded;
}

// 4b sample offset for each lane
template <unsigned int polyphaseLaneAlias>
constexpr std::array<unsigned int, polyphaseLaneAlias> getXOffsets(unsigned int D,
                                                                   unsigned int I,
                                                                   unsigned int nLanes) {
    std::array<unsigned int, polyphaseLaneAlias> acc = {0};
#if __HAS_ACCUM_PERMUTES__ == 1
    // static_assert(nLanes<=8, "ERROR: Unsupported data type. Exceeding 32-bit offset range.");
    // int minSampleNeeded,maxSampleNeeded;
    // A negative dec would indicate an incr
    for (unsigned int phase = 0; phase < polyphaseLaneAlias; phase++) {
        int min = getSampleIndexOnLane(D, I, nLanes, 0, phase);
        for (unsigned int lane = 0; lane < nLanes; lane++) {
            acc[phase] += (getSampleIndexOnLane(D, I, nLanes, lane, phase) - min)
                          << 4 * lane; // left shift in order to have values at 4b offset
        }
    }
#endif
    return acc;
}
// 4b coeff offset for each lane
template <unsigned int polyphaseLaneAlias>
constexpr std::array<unsigned int, polyphaseLaneAlias> getZOffsets(unsigned int I, unsigned int nLanes) {
    std::array<unsigned int, polyphaseLaneAlias> acc = {0};
#if __HAS_ACCUM_PERMUTES__ == 1
    // static_assert(nLanes<=8, "ERROR: Unsupported data type. Exceeding 32-bit offset range.");

    unsigned int alreadyAccounted = I > nLanes ? 0 : 1; // simple hack to enable/disable phase alias on coeff offsets.
    for (unsigned int phase = 0; phase < polyphaseLaneAlias; phase++) {
        for (unsigned int lane = 0; lane < nLanes; lane++) {
            acc[phase] += ((phase * nLanes * alreadyAccounted + lane) % I)
                          << 4 * lane; // left shift in order to have values at 4b offset
        }
    }
#endif

    return acc;
}

// recursive until remainder is 0.
constexpr unsigned int my_gcd(unsigned int m, unsigned int n) {
    return (n == 0 ? m : my_gcd(n, m % n));
}
// This is pretty much the same as in std library
constexpr unsigned int my_lcm(unsigned int m, unsigned int n) {
    return ((m / my_gcd(m, n)) * n);
}
}
}
}
}
}
#endif // _DSPLIB_fir_resampler_TRAITS_HPP_
