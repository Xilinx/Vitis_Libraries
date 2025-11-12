/*
 * Copyright (C) 2025, Advanced Micro Devices, Inc.
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

// #define _DSPLIB_CHOLESKY_RUNTIME_DEBUG_

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

template <typename TT>
INLINE_DECL DATA_VECT_T(TT) getConj(typename DATA_VECT_T(TT)& vectInput) {
    return ::aie::conj(vectInput);
};
template <>
INLINE_DECL DATA_VECT_T(float) getConj(typename DATA_VECT_T(float)& vectInput) {
    return vectInput;
};


template <typename TT>
INLINE_DECL void writeScalarToPort(output_stream<TT>* __restrict (&port), TT& scalar) {
    writeincr(port, scalar);
};
template <typename TT>
INLINE_DECL void writeScalarToPort(output_cascade<TT>* __restrict (&port), TT& scalar) {
    using dataVect_t = ::aie::vector<TT, fnVecSampleNum<TT>()>;
    dataVect_t packetVect;
    packetVect[0] = scalar;
    writeincr(port, packetVect);
};

template <typename TT>
INLINE_DECL TT readScalarFromPort(input_stream<TT>* __restrict (&port)) {
    return readincr(port);

};
template <typename TT>
INLINE_DECL TT readScalarFromPort(input_cascade<TT>* __restrict (&port)) {
    using dataVect_t = ::aie::vector<TT, fnVecSampleNum<TT>()>;
    dataVect_t packetVector = readincr_v<fnVecSampleNum<TT>()>(port);
    return packetVector[0];
};



template <typename TT, unsigned int kNumVecsPerDim>
INLINE_DECL void matrixBlockEliminations(   DATA_VECT_T(TT)* __restrict (&diagColPtr), // These multiply together and could come from different tiles.
                                            DATA_VECT_T(TT)* __restrict (&diagRowPtr),
                                            DATA_VECT_T(TT)* __restrict (&inPtr)) {
    constexpr unsigned int kVecSampleNum = fnVecSampleNum<TT>();
    using dataVect_t = ::aie::vector<TT, kVecSampleNum>;
    using dataAcc_t = ::aie::accum<accType_t<TT>, kVecSampleNum>;
    dataAcc_t accum;
    
    for (int j = 0; j < kNumVecsPerDim; j++)
    chess_prepare_for_pipelining chess_loop_count( kNumVecsPerDim ) {

        unsigned int vectBlockIdx = j * kVecSampleNum * kNumVecsPerDim;
        dataVect_t diagRowVect = getConj<TT>(diagRowPtr[j]);

        for (int i = 0; i < kNumVecsPerDim; i++)
        chess_prepare_for_pipelining chess_loop_count( kNumVecsPerDim ) {

            dataVect_t diagColVect = diagColPtr[i];

            #pragma unroll( kVecSampleNum )
            for (int k = 0; k < kVecSampleNum; k++) {
                unsigned int vectIdx = vectBlockIdx + k * kNumVecsPerDim;
                accum.from_vector(inPtr[vectIdx]);
                accum = ::aie::msc(accum, diagRowVect[k], diagColVect); //TODO: utilise msc with conjugate.
                inPtr[vectIdx] = accum.template to_vector<TT>();
            }
            vectBlockIdx++;
        }
    }
};


template <typename TT, unsigned int kNumVecsPerDim>
INLINE_DECL void firstColBlockEliminations( DATA_VECT_T(TT)* __restrict (&diagColPtr),  // These multiply together and could come from different tiles.
                                            DATA_VECT_T(TT)* __restrict (&diagRowPtr),
                                            DATA_VECT_T(TT)* __restrict (&inPtr),
                                            int& colStart,
                                            int& rowStart,
                                            int& diagLocal) {
    constexpr unsigned int kVecSampleNum = fnVecSampleNum<TT>();
    using dataVect_t = ::aie::vector<TT, kVecSampleNum>;
    using dataAcc_t = ::aie::accum<accType_t<TT>, kVecSampleNum>;

    dataVect_t diagRowVect = getConj<TT>(diagRowPtr[colStart]);
    dataAcc_t accum;
    unsigned int vectBlockIdx = colStart * kVecSampleNum * kNumVecsPerDim + rowStart;
    
    for (int i = rowStart; i < kNumVecsPerDim; i++) {   // ! not pipelineable
        dataVect_t diagColVect = diagColPtr[i];
        
        for (int k = diagLocal+1; k < kVecSampleNum; k++) { // ! not unrollable
            unsigned int vectIdx = vectBlockIdx + k * kNumVecsPerDim;
            accum.from_vector(inPtr[vectIdx]);
            accum = ::aie::msc(accum, diagRowVect[k], diagColVect);
            inPtr[vectIdx] = accum.template to_vector<TT>();
        }
        vectBlockIdx++;
    }
};

template <typename TT, unsigned int kNumVecsPerDim>
INLINE_DECL void colBlockEliminations(  DATA_VECT_T(TT)* __restrict (&diagColPtr),  // These multiply together and could come from different tiles.
                                        DATA_VECT_T(TT)* __restrict (&diagRowPtr),
                                        DATA_VECT_T(TT)* __restrict (&inPtr),
                                        int& colStart,
                                        int& rowStart) {
    constexpr unsigned int kVecSampleNum = fnVecSampleNum<TT>();
    using dataVect_t = ::aie::vector<TT, kVecSampleNum>;
    using dataAcc_t = ::aie::accum<accType_t<TT>, kVecSampleNum>;

    dataVect_t diagRowVect = getConj<TT>(diagRowPtr[colStart]);
    dataAcc_t accum;
    unsigned int vectBlockIdx = colStart * kVecSampleNum * kNumVecsPerDim + rowStart;
    
    for (int i = rowStart; i < kNumVecsPerDim; i++) {   // ! not pipelineable
        dataVect_t diagColVect = diagColPtr[i];
        
        #pragma unroll( kVecSampleNum )
        for (int k = 0; k < kVecSampleNum; k++) {
            unsigned int vectIdx = vectBlockIdx + k * kNumVecsPerDim;
            accum.from_vector(inPtr[vectIdx]);
            accum = ::aie::msc(accum, diagRowVect[k], diagColVect);
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
