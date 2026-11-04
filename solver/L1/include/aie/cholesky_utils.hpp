/*
 * Copyright (C) 2025-2026, Advanced Micro Devices, Inc.
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
#ifndef _DSPLIB_CHOLESKY_UTILS_HPP_
#define _DSPLIB_CHOLESKY_UTILS_HPP_

/*
This file contains sets of overloaded, templatized and specialized templatized functions for use
by the main kernel class and run-time function. These functions are separate from the traits file
because they are purely for kernel use, not graph level compilation.
*/

#include <stdio.h>
#include <adf.h>
#include "cholesky_traits.hpp"

namespace xf {
namespace solver {
namespace aie {
namespace cholesky {

template <typename TT>
INLINE_DECL float getReal(TT& val) {
    return val;
};
template <>
INLINE_DECL float getReal(cfloat& val) {
    return val.real;
};

template <typename TT>
INLINE_DECL TT getScalarAsType(float& val) { // Base case when TT_DATA = cfloat;
    return {val, 0.0};
}
template <>
INLINE_DECL float getScalarAsType(float& val) {
    return val;
}

template <typename TT, unsigned int kVecSampleNum>
INLINE_DECL::aie::accum<accType_t<TT>, kVecSampleNum> msc_with_conj(
    typename ::aie::accum<accType_t<TT>, kVecSampleNum>& accum,
    typename DATA_VECT_T(TT, kVecSampleNum) & diagRowVect,
    typename DATA_VECT_T(TT, kVecSampleNum) & diagColVect,
    int& k) {
    if
        constexpr(isComplex<TT>()) { return ::aie::msc(accum, ::aie::op_conj(diagRowVect[k]), diagColVect); }
    else {
        return ::aie::msc(accum, diagRowVect[k], diagColVect);
    }
};

template <typename TT, unsigned int kVecSampleNum>
INLINE_DECL void writeScalarToPort(output_stream<TT>* __restrict(&port), TT& scalar) {
    writeincr(port, scalar);
};
template <typename TT, unsigned int kVecSampleNum>
INLINE_DECL void writeScalarToPort(output_cascade<TT>* __restrict(&port), TT& scalar) {
    using dataVect_t = ::aie::vector<TT, kVecSampleNum>;
    dataVect_t packetVect;
    packetVect[0] = scalar;
    writeincr(port, packetVect);
};

template <typename TT, unsigned int kVecSampleNum>
INLINE_DECL TT readScalarFromPort(input_stream<TT>* __restrict(&port)) {
    return readincr(port);
};
template <typename TT, unsigned int kVecSampleNum>
INLINE_DECL TT readScalarFromPort(input_cascade<TT>* __restrict(&port)) {
    using dataVect_t = ::aie::vector<TT, kVecSampleNum>;
    dataVect_t packetVector = readincr_v<kVecSampleNum>(port);
    return packetVector[0];
};

template <typename TT, unsigned int kNumVecsPerDim, unsigned int kVecSampleNum>
INLINE_DECL void matrixBlockEliminations(
    DATA_VECT_T(TT, kVecSampleNum) *
        __restrict(&diagColPtr), // These multiply together and could come from different tiles.
    DATA_VECT_T(TT, kVecSampleNum) * __restrict(&diagRowPtr),
    DATA_VECT_T(TT, kVecSampleNum) * __restrict(&inPtr)) {
    constexpr unsigned int kNumAccs = fnNumAccs<TT>();
    using dataVect_t = ::aie::vector<TT, kVecSampleNum>;
    using dataAcc_t = ::aie::accum<accType_t<TT>, kVecSampleNum>;
    DATA_VECT_T(TT, kVecSampleNum)* __restrict readPtr = inPtr;
    // Per-k accumulators make independence across unrolled k-iterations explicit to the
    // scheduler. The count is capped at the device's float/cfloat accumulator budget
    // (fnNumAccs) so the array fits the register file - critical on AIE1 where float
    // accumulators share the vector registers with data.
    dataAcc_t accums[kNumAccs];

    for (int j = 0; j < kNumVecsPerDim; j++) chess_prepare_for_pipelining chess_loop_count(kNumVecsPerDim) {
            unsigned int vectBlockIdx = j * kVecSampleNum * kNumVecsPerDim;
            dataVect_t diagRowVect = diagRowPtr[j];

            for (int i = 0; i < kNumVecsPerDim; i++) chess_prepare_for_pipelining chess_loop_count(kNumVecsPerDim) {
                    dataVect_t diagColVect = diagColPtr[i];

#pragma unroll(kVecSampleNum)
                    for (int k = 0; k < kVecSampleNum; k++) {
                        unsigned int vectIdx = vectBlockIdx + k * kNumVecsPerDim;
                        // k % kNumAccs is compile-time per unrolled iteration; each accumulator slot
                        // is loaded and stored within one k-iteration, so cycling is correctness-safe.
                        dataAcc_t& accum = accums[k % kNumAccs];
                        accum.from_vector(readPtr[vectIdx]);
                        accum = msc_with_conj<TT, kVecSampleNum>(accum, diagRowVect, diagColVect, k);
                        inPtr[vectIdx] = accum.template to_vector<TT>();
                    }
                    vectBlockIdx++;
                }
        }
};

template <typename TT, unsigned int kNumVecsPerDim, unsigned int kVecSampleNum>
INLINE_DECL void colBlockEliminations(
    DATA_VECT_T(TT, kVecSampleNum) *
        __restrict(&diagColPtr), // These multiply together and could come from different tiles.
    DATA_VECT_T(TT, kVecSampleNum) * __restrict(&diagRowPtr),
    DATA_VECT_T(TT, kVecSampleNum) * __restrict(&inPtr),
    int& colStart,
    int& rowStart) {
    constexpr unsigned int kNumAccs = fnNumAccs<TT>();
    using dataVect_t = ::aie::vector<TT, kVecSampleNum>;
    using dataAcc_t = ::aie::accum<accType_t<TT>, kVecSampleNum>;

    DATA_VECT_T(TT, kVecSampleNum)* __restrict readPtr = inPtr;
    dataVect_t diagRowVect = diagRowPtr[colStart];
    // Per-k accumulators: each k-iteration operates on a distinct vectIdx — no cross-k
    // dependency. Capped at the device's float/cfloat accumulator budget (fnNumAccs) so the
    // array fits the register file - critical on AIE1 where float accumulators share the
    // vector registers with data.
    dataAcc_t accums[kNumAccs];
    unsigned int vectBlockIdx = colStart * kVecSampleNum * kNumVecsPerDim + rowStart;

    // i-iterations access disjoint inPtr locations (vectBlockIdx advances by 1 per i),
    // so there is no cross-iteration data dependency and the loop is pipelineable.
    for (int i = rowStart; i < kNumVecsPerDim; i++) chess_prepare_for_pipelining {
            dataVect_t diagColVect = diagColPtr[i];

#pragma unroll(kVecSampleNum)
            for (int k = 0; k < kVecSampleNum; k++) {
                unsigned int vectIdx = vectBlockIdx + k * kNumVecsPerDim;
                // k % kNumAccs is compile-time per unrolled iteration; each accumulator slot
                // is loaded and stored within one k-iteration, so cycling is correctness-safe.
                dataAcc_t& accum = accums[k % kNumAccs];
                accum.from_vector(readPtr[vectIdx]);
                accum = msc_with_conj<TT, kVecSampleNum>(accum, diagRowVect, diagColVect, k);
                inPtr[vectIdx] = accum.template to_vector<TT>();
            }
            vectBlockIdx++;
        }
};
}
}
}
}

#endif // _DSPLIB_CHOLESKY_UTILS_HPP_
