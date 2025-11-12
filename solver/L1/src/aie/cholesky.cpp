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
/*
cholesky kernel code.
This file captures the body of run-time code for the kernel class.

Coding conventions
  TT_      template type suffix
  TP_      template parameter suffix
*/

#pragma once
#include <adf.h>
#include <cstring>

#define __AIE_API_USE_NATIVE_1024B_VECTOR__
#include "aie_api/aie_adf.hpp"
#include "aie_api/utils.hpp"
#include "cholesky.hpp"
#include "kernel_api_utils.hpp"
#include "cholesky_utils.hpp"
#include "cholesky_traits.hpp"

namespace xf {
namespace solver {
namespace aie {
namespace cholesky {

//--------------------------------------------------------------------
// Base specialization, used for static size window API configurations
template <typename TT_DATA,
          unsigned int TP_DIM,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_X,
          unsigned int TP_Y,
          unsigned int TP_GRID_DIM>
NOINLINE_DECL void
cholesky<TT_DATA, TP_DIM, TP_NUM_FRAMES, TP_X, TP_Y, TP_GRID_DIM>::cholesky_main(
        input_buffer<TT_DATA>& __restrict inWindow, 
        output_buffer<TT_DATA>& __restrict outWindow) {
    using dataVect_t = ::aie::vector<TT_DATA, kVecSampleNum>;

    dataVect_t* inPtr = (dataVect_t*)inWindow.data();
    dataVect_t* outPtr = (dataVect_t*)outWindow.data();
    dataVect_t* diagColPtr = (dataVect_t*)outWindow.data();
    TT_DATA* diagPtr = (TT_DATA*)inWindow.data();
    
    for (int frame = 0; frame < TP_NUM_FRAMES; frame++) 
    chess_prepare_for_pipelining chess_loop_count(TP_NUM_FRAMES) {


        // * SHRINKING ROWS RUNTIME
        for (int diag = 0; diag < TP_DIM; diag++)
        chess_prepare_for_pipelining chess_loop_count(TP_DIM) {

            int diagChunk = diag / kVecSampleNum;
            int diagLocal = diag & (kVecSampleNum-1);

            // * Multiplying current row with reciprocal square root of its diagonal...
            float diagInvSqrt = ::aie::invsqrt(getReal<TT_DATA>( diagPtr[diag*TP_DIM + diag] ));
            for (int j = diagChunk; j < kNumVecsPerDim; j++) {
                dataVect_t diagColVect = ::aie::mul(diagInvSqrt, inPtr[IDX(diag, j)]);
                outPtr[IDX(diag, j)] = diagColVect;
            }
            chess_memory_fence();
            
            firstColBlockEliminations<TT_DATA, kNumVecsPerDim>(diagColPtr, diagColPtr, inPtr, diagChunk, diagChunk, diagLocal);
            // Performing eliminations on other columns...
            for (int chunk = diagChunk+1; chunk < kNumVecsPerDim; chunk++) {
                colBlockEliminations<TT_DATA, kNumVecsPerDim>(diagColPtr, diagColPtr, inPtr, chunk, chunk);
            }
            diagColPtr += kNumVecsPerDim;
        }


        diagPtr += TP_DIM * TP_DIM;
        inPtr += kNumVecsPerDim * TP_DIM;
        outPtr += kNumVecsPerDim * TP_DIM;
    }
};

// ************** DIAGONAL KERNEL FUNCTION **************

template <typename TT_DATA,
          unsigned int TP_DIM,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_X,
          unsigned int TP_Y,
          unsigned int TP_GRID_DIM>
NOINLINE_DECL void
cholesky<TT_DATA, TP_DIM, TP_NUM_FRAMES, TP_X, TP_Y, TP_GRID_DIM>::cholesky_diagKernel(
        input_buffer<TT_DATA>& __restrict inWindow, 
        output_buffer<TT_DATA>& __restrict outWindow,
        T_communicationIF ports) {
    using dataVect_t = ::aie::vector<TT_DATA, kVecSampleNum>;

    dataVect_t*             inPtr           = (dataVect_t*)inWindow.data();
    dataVect_t*             outPtr          = (dataVect_t*)outWindow.data();
    dataVect_t*             diagColHomePtr  = outPtr;
    dataVect_t* __restrict  diagColBuffPtr  = (dataVect_t*)diagColBuffer;
    TT_DATA*                diagPtr         = (TT_DATA*)inPtr;

    for (int frame = 0; frame < TP_NUM_FRAMES; frame++) 
    chess_prepare_for_pipelining chess_loop_count( TP_NUM_FRAMES ) {


        // * diag has NOT yet reached our kernel
        for (int diag = 0; diag < kNumPriorKernels * TP_DIM; diag++)
        chess_prepare_for_pipelining chess_loop_count( kNumPriorKernels * TP_DIM ) {
            
            for (int i = 0; i < kNumVecsPerDim; i++)
            chess_prepare_for_pipelining chess_loop_count( kNumVecsPerDim ) {

                dataVect_t diagColVect = readincr_v<kVecSampleNum>(ports.inLeft);
                diagColBuffPtr[i] = diagColVect;

                if constexpr(TP_Y + 1 < TP_GRID_DIM) {
                    writeincr(ports.outDown, diagColVect);  // For the below kernel, this is the diagRowVect.
                }
            }
            chess_memory_fence();

            for (int chunk = 0; chunk < kNumVecsPerDim; chunk++)
            chess_prepare_for_pipelining chess_loop_count( kNumVecsPerDim ) {
                colBlockEliminations<TT_DATA, kNumVecsPerDim>(diagColBuffPtr, diagColBuffPtr, inPtr, chunk, chunk);
            }
        }

        // * diag HAS reached our kernel
        for (int diag = 0; diag < TP_DIM; diag++) 
        chess_prepare_for_pipelining chess_loop_count(TP_DIM) {
            chess_memory_fence();

            int diagChunk = diag / kVecSampleNum;
            int diagLocal = diag & (kVecSampleNum-1);

            float diagInvSqrt = ::aie::invsqrt(getReal<TT_DATA>( diagPtr[diag*TP_DIM + diag] ));
            if constexpr(TP_Y + 1 < TP_GRID_DIM) {  // if there are kernels below us...
                TT_DATA diagInvSqrtCompatible = getScalarAsType<TT_DATA>(diagInvSqrt);
                writeScalarToPort<TT_DATA>(ports.outDown, diagInvSqrtCompatible);
            }

            for (int j = diagChunk; j < kNumVecsPerDim; j++) {
                dataVect_t diagColVect = ::aie::mul(diagInvSqrt, inPtr[IDX(diag, j)]);
                outPtr[IDX(diag, j)] = diagColVect;

                if constexpr(TP_Y + 1 < TP_GRID_DIM) {  // if there are kernels below us...
                    writeincr(ports.outDown, diagColVect);
                }
            }
            chess_memory_fence();

            firstColBlockEliminations<TT_DATA, kNumVecsPerDim>(diagColHomePtr, diagColHomePtr, inPtr, diagChunk, diagChunk, diagLocal);
            // Performing eliminations on other columns...
            for (int chunk = diagChunk+1; chunk < kNumVecsPerDim; chunk++) {
                colBlockEliminations<TT_DATA, kNumVecsPerDim>(diagColHomePtr, diagColHomePtr, inPtr, chunk, chunk);
            }
            diagColHomePtr += kNumVecsPerDim;
        }

        diagPtr += TP_DIM * TP_DIM;
        inPtr += kNumVecsPerDim * TP_DIM;
        outPtr += kNumVecsPerDim * TP_DIM;
    }
};


// ************** LOWER KERNEL FUNCTION **************

template <typename TT_DATA,
          unsigned int TP_DIM,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_X,
          unsigned int TP_Y,
          unsigned int TP_GRID_DIM>
NOINLINE_DECL void
cholesky<TT_DATA, TP_DIM, TP_NUM_FRAMES, TP_X, TP_Y, TP_GRID_DIM>::cholesky_lowerKernel(
        input_buffer<TT_DATA>& inWindow, 
        output_buffer<TT_DATA>& outWindow,
        T_communicationIF ports) {
    using dataVect_t = ::aie::vector<TT_DATA, kVecSampleNum>;

    dataVect_t* __restrict  inPtr           = (dataVect_t*)inWindow.data();
    dataVect_t*             outPtr          = (dataVect_t*)outWindow.data();
    dataVect_t*             diagColHomePtr  = outPtr;
    dataVect_t* __restrict  diagColBuffPtr  = (dataVect_t*)diagColBuffer;
    dataVect_t* __restrict  diagRowBuffPtr  = (dataVect_t*)diagRowBuffer;

    for (int frame = 0; frame < TP_NUM_FRAMES; frame++) 
    chess_prepare_for_pipelining chess_loop_count(TP_NUM_FRAMES) {

        
        // * diag has NOT yet reached our kernel
        for (int diag = 0; diag < kNumPriorKernels * TP_DIM; diag++)
        chess_prepare_for_pipelining chess_loop_count( kNumPriorKernels * TP_DIM ) {

            int diagChunk = diag / kVecSampleNum;

            for (int i = 0; i < kNumVecsPerDim; i++)
            chess_prepare_for_pipelining chess_loop_count( kNumVecsPerDim ) {
                dataVect_t diagColVect = readincr_v<kVecSampleNum>(ports.inLeft);
                diagColBuffPtr[i] = diagColVect;
                writeincr(ports.outRight, diagColVect);
            }

            for (int i = 0; i < kNumVecsPerDim; i++)
            chess_prepare_for_pipelining chess_loop_count( kNumVecsPerDim ) {
                dataVect_t diagRowVect = readincr_v<kVecSampleNum>(ports.inUp);
                diagRowBuffPtr[i] = diagRowVect;
                if constexpr(TP_Y + 1 < TP_GRID_DIM) { // if there are kernels below us...
                    writeincr(ports.outDown, diagRowVect);
                }
            }
            chess_memory_fence();
            matrixBlockEliminations<TT_DATA, kNumVecsPerDim>(diagColBuffPtr, diagRowBuffPtr, inPtr);
        }

        // * diag HAS reached our kernel
        for (int diag = 0; diag < TP_DIM; diag++) 
        chess_prepare_for_pipelining chess_loop_count(TP_DIM) {
            chess_memory_fence();

            int diagChunk = diag / kVecSampleNum;
            int diagLocal = diag & (kVecSampleNum-1);

            TT_DATA diagInvSqrtCompatible = readScalarFromPort<TT_DATA>(ports.inUp);
            float diagInvSqrt = getReal<TT_DATA>(diagInvSqrtCompatible);
            if constexpr(TP_Y + 1 < TP_GRID_DIM) {  // if there are kernels below us...
                writeScalarToPort<TT_DATA>(ports.outDown, diagInvSqrtCompatible);
            }

            for (int i = diagChunk; i < kNumVecsPerDim; i++) {
                dataVect_t diagRowVect = readincr_v<kVecSampleNum>(ports.inUp);
                diagRowBuffPtr[i] = diagRowVect;
                if constexpr(TP_Y + 1 < TP_GRID_DIM) { // if there are kernels below us...
                    writeincr(ports.outDown, diagRowVect);
                }
            }

            for (int i = 0; i < kNumVecsPerDim; i++)
            chess_prepare_for_pipelining chess_loop_count(kNumVecsPerDim) {
                dataVect_t diagColVect = ::aie::mul(diagInvSqrt, inPtr[IDX(diag, i)]);
                outPtr[IDX(diag, i)] = diagColVect;
                writeincr(ports.outRight, diagColVect);
            }
            chess_memory_fence();

            int rowStart = 0;
            firstColBlockEliminations<TT_DATA, kNumVecsPerDim>(diagColHomePtr, diagRowBuffPtr, inPtr, diagChunk, rowStart, diagLocal);
            for (int colStart = diagChunk+1; colStart < kNumVecsPerDim; colStart++) {
                colBlockEliminations<TT_DATA, kNumVecsPerDim>(diagColHomePtr, diagRowBuffPtr, inPtr, colStart, rowStart);
            }
            diagColHomePtr += kNumVecsPerDim;
        }

        inPtr += kNumVecsPerDim * TP_DIM;
        outPtr += kNumVecsPerDim * TP_DIM;
    }
};


// ************** DIAGONAL ENTRY-POINT FUNCTIONS **************

template <typename TT_DATA,
          unsigned int TP_DIM,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_X,
          unsigned int TP_Y,
          unsigned int TP_GRID_DIM>
NOINLINE_DECL void
cholesky<TT_DATA, TP_DIM, TP_NUM_FRAMES, TP_X, TP_Y, TP_GRID_DIM>::cholesky_diagKernel_topLeft(
        input_buffer<TT_DATA>& __restrict inWindow, 
        output_buffer<TT_DATA>& __restrict outWindow,
        outputPortDown_t* outDown) {

    T_communicationIF ports;
    ports.outDown = outDown;
    this->cholesky_diagKernel(inWindow, outWindow, ports);
};

template <typename TT_DATA,
          unsigned int TP_DIM,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_X,
          unsigned int TP_Y,
          unsigned int TP_GRID_DIM>
NOINLINE_DECL void
cholesky<TT_DATA, TP_DIM, TP_NUM_FRAMES, TP_X, TP_Y, TP_GRID_DIM>::cholesky_diagKernel_middle(
        input_buffer<TT_DATA>& __restrict inWindow, 
        output_buffer<TT_DATA>& __restrict outWindow,
        inputPortLeft_t* inLeft,
        outputPortDown_t* outDown) {

    T_communicationIF ports;
    ports.inLeft = inLeft;
    ports.outDown = outDown;
    this->cholesky_diagKernel(inWindow, outWindow, ports);
};

template <typename TT_DATA,
          unsigned int TP_DIM,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_X,
          unsigned int TP_Y,
          unsigned int TP_GRID_DIM>
NOINLINE_DECL void
cholesky<TT_DATA, TP_DIM, TP_NUM_FRAMES, TP_X, TP_Y, TP_GRID_DIM>::cholesky_diagKernel_botRight(
        input_buffer<TT_DATA>& __restrict inWindow, 
        output_buffer<TT_DATA>& __restrict outWindow,
        inputPortLeft_t* inLeft) {

    T_communicationIF ports;
    ports.inLeft = inLeft;
    this->cholesky_diagKernel(inWindow, outWindow, ports);
};


// ************** LOWER ENTRY-POINT FUNCTIONS **************

template <typename TT_DATA,
          unsigned int TP_DIM,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_X,
          unsigned int TP_Y,
          unsigned int TP_GRID_DIM>
NOINLINE_DECL void
cholesky<TT_DATA, TP_DIM, TP_NUM_FRAMES, TP_X, TP_Y, TP_GRID_DIM>::cholesky_lowerKernel_leftEdge(
        input_buffer<TT_DATA>& __restrict inWindow, 
        output_buffer<TT_DATA>& __restrict outWindow,
        inputPortUp_t* inUp,
        outputPortRight_t* outRight,
        outputPortDown_t* outDown) {

    T_communicationIF ports;
    ports.inUp = inUp;
    ports.outRight = outRight;
    ports.outDown = outDown;
    this->cholesky_lowerKernel(inWindow, outWindow, ports);
};

template <typename TT_DATA,
          unsigned int TP_DIM,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_X,
          unsigned int TP_Y,
          unsigned int TP_GRID_DIM>
NOINLINE_DECL void
cholesky<TT_DATA, TP_DIM, TP_NUM_FRAMES, TP_X, TP_Y, TP_GRID_DIM>::cholesky_lowerKernel_botLeft(
        input_buffer<TT_DATA>& __restrict inWindow, 
        output_buffer<TT_DATA>& __restrict outWindow,
        inputPortUp_t* inUp,
        outputPortRight_t* outRight) {

    T_communicationIF ports;
    ports.inUp = inUp;
    ports.outRight = outRight;
    this->cholesky_lowerKernel(inWindow, outWindow, ports);
};

template <typename TT_DATA,
          unsigned int TP_DIM,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_X,
          unsigned int TP_Y,
          unsigned int TP_GRID_DIM>
NOINLINE_DECL void
cholesky<TT_DATA, TP_DIM, TP_NUM_FRAMES, TP_X, TP_Y, TP_GRID_DIM>::cholesky_lowerKernel_botEdge(
        input_buffer<TT_DATA>& __restrict inWindow, 
        output_buffer<TT_DATA>& __restrict outWindow,
        inputPortLeft_t* inLeft,
        inputPortUp_t* inUp,
        outputPortRight_t* outRight) {

    T_communicationIF ports;
    ports.inLeft = inLeft;
    ports.inUp = inUp;
    ports.outRight = outRight;
    this->cholesky_lowerKernel(inWindow, outWindow, ports);
};

template <typename TT_DATA,
          unsigned int TP_DIM,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_X,
          unsigned int TP_Y,
          unsigned int TP_GRID_DIM>
NOINLINE_DECL void
cholesky<TT_DATA, TP_DIM, TP_NUM_FRAMES, TP_X, TP_Y, TP_GRID_DIM>::cholesky_lowerKernel_nonEdge(
        input_buffer<TT_DATA>& __restrict inWindow, 
        output_buffer<TT_DATA>& __restrict outWindow,
        inputPortLeft_t* inLeft,
        inputPortUp_t* inUp,
        outputPortRight_t* outRight,
        outputPortDown_t* outDown) {

    T_communicationIF ports;
    ports.inLeft = inLeft;
    ports.inUp = inUp;
    ports.outRight = outRight;
    ports.outDown = outDown;
    this->cholesky_lowerKernel(inWindow, outWindow, ports);
};

}
}
}
}
