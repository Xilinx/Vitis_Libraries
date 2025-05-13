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
#ifndef _DSPLIB_MATRIX_VECTOR_MUL_HPP_
#define _DSPLIB_MATRIX_VECTOR_MUL_HPP_

/*
MATRIX_VECTOR_MUL
This file exists to capture the definition matrix_vector_mul kernel
class.
The class definition holds defensive checks on parameter range and other legality.
The constructor definition is held in this class because this class must be accessible to graph
level aie compilation.
The main runtime function is captured elsewhere as it contains aie intrinsics which are not
included in aie graph level compilation.
*/

/* Coding conventions
   TT_      template type suffix
   TP_      template parameter suffix
*/

/* Design Notes
*/

#include <adf.h>
#include <vector>

#include "matrix_vector_mul_traits.hpp"

#define IOBUFFER_B 0
#define STREAM_B 1
#define SINGLE_STREAM_B 0
#define DUAL_STREAM_B 1
#define SINGLE_STREAM_OUT 1
#define DUAL_STREAM_OUT 2

#define USE_MATRIX_RELOAD_TRUE 1
#define USE_MATRIX_RELOAD_FALSE 0

namespace xf {
namespace dsp {
namespace aie {
namespace blas {
namespace matrix_vector_mul {

//-----------------------------------------------------------------------------------------------------
template <typename TT_DATA_A,
          typename TT_DATA_B,
          unsigned int TP_DIM_A,
          unsigned int TP_DIM_B,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_CASC_LEN,
          unsigned int TP_USE_MATRIX_RELOAD = 0,
          unsigned int TP_API = 0,
          unsigned int TP_DUAL_IP = 0,
          unsigned int TP_NUM_OUTPUTS = 0,
          unsigned int TP_KERNEL_POSITION = 0,
          bool TP_CASC_IN = CASC_IN_FALSE,
          bool TP_CASC_OUT = CASC_OUT_FALSE>
class kernelMatVecMulClass {
   private:
    // Parameter value defensive and legality checks
    static_assert(TP_RND >= ROUND_MIN && TP_RND <= ROUND_MAX, "ERROR: TP_RND is out of the supported range.");
    static_assert(TP_SAT >= SAT_MODE_MIN && TP_SAT <= SAT_MODE_MAX, "ERROR: TP_SAT is out of supported range");
    static_assert(TP_SAT != 2, "ERROR: TT_DATA_A is not currently supported");
    static_assert(TP_SHIFT >= SHIFT_MIN && TP_SHIFT <= SHIFT_MAX, "ERROR: SHIFT is out of the supported range.");
    // static_assert(TP_DIM_A_LEADING == 1, "ERROR: Matrix-vector multiplication currently only supports matrix data
    // stored in a column major format. Row major matrices must be transposed to column major");
    static_assert((TP_DIM_A % (256 / 8 / sizeof(TT_DATA_A))) == 0,
                  "ERROR: TP_DIM_A must be a multiple of (256 / 8 / sizeof(TT_DATA_A). The input matrix data can be "
                  "zero-padded as required");
    static_assert((TP_DIM_B % (TP_CASC_LEN * 256 / 8 / sizeof(TT_DATA_B))) == 0,
                  "ERROR: TP_DIM_B must be a multiple of (256 / 8 / sizeof(TT_DATA_B) and TP_CASC_LEN. The input "
                  "vector data can be zero-padded as required ");
    static_assert(!(std::is_same<TT_DATA_A, cfloat>::value || std::is_same<TT_DATA_A, float>::value) || (TP_SHIFT == 0),
                  "ERROR: TP_SHIFT cannot be performed for TT_DATA=cfloat, so must be set to 0"); // only necessary to
                                                                                                  // check TT_DATA_A as
                                                                                                  // TT_DATA_B will also
                                                                                                  // be float or integer
                                                                                                  // to match TT_DATA_A.

    using TT_OUT = outType_t<TT_DATA_A, TT_DATA_B>;
    static constexpr int vecSampleNumA = mumLanesMatVec<TT_DATA_A, TT_DATA_B>().lanesInA;
    static constexpr int vecSampleNumB = mumLanesMatVec<TT_DATA_A, TT_DATA_B>().lanesInB;
    static constexpr int vecSampleNumOut = mumLanesMatVec<TT_DATA_A, TT_DATA_B>().lanesInOut;

    static constexpr int loadsPerColA = TP_DIM_A / vecSampleNumA;
    static constexpr int loadsPerMatrix = loadsPerColA * (TP_DIM_B / TP_CASC_LEN);
    static constexpr int loadsPerVectorB = (TP_DIM_B / TP_CASC_LEN) / vecSampleNumB;
    static constexpr unsigned int streamVectorBuffSize = (TP_DIM_B / TP_CASC_LEN) == (768 / 8 / sizeof(TT_DATA_B))
                                                             ? (1024 / 8 / sizeof(TT_DATA_B))
                                                             : TP_DIM_B / TP_CASC_LEN;
#ifdef __SUPPORTS_ACC64__
    static constexpr int castBtoA = 0;
#else
    static constexpr int castBtoA =
        (std::is_same<TT_DATA_A, cint32>::value && std::is_same<TT_DATA_B, cint16>::value) ||
        (std::is_same<TT_DATA_A, int32>::value && std::is_same<TT_DATA_B, int16>::value);
#endif //__SUPPORTS_ACC64__

    static constexpr unsigned int streamLoadSize = 128 / 8 / sizeof(TT_DATA_B);
    static constexpr unsigned int streamWriteOutSize = 128 / 8 / sizeof(TT_OUT);
    // fn get reg size
    static constexpr unsigned int streamLoadsRequired = (TP_DIM_B / TP_CASC_LEN) / streamLoadSize;

    void matVecMulSelectArch(T_inputIF<TT_DATA_A, TT_DATA_B> inInterface,
                             T_outputIF<TT_DATA_A, TT_DATA_B> outInterface);
    // Implementations
    void matVecMulBasic(T_inputIF<TT_DATA_A, TT_DATA_B> inInterface, T_outputIF<TT_DATA_A, TT_DATA_B> outInterface);
    // Vector B Stream Implementation
    void matVecMulStream(T_inputIF<TT_DATA_A, TT_DATA_B> inInterface, T_outputIF<TT_DATA_A, TT_DATA_B> outInterface);

   public:
    TT_DATA_A* __restrict m_inMatrixPtr;
    void setInMatrixPtr(const TT_DATA_A (&inMatrixA)[TP_DIM_A * TP_DIM_B / (TP_CASC_LEN)]) {
        m_inMatrixPtr = (TT_DATA_A*)inMatrixA;
    };

    // Constructor
    kernelMatVecMulClass() {}

    // MATRIX_VECTOR_MUL
    void matVecMulKernel(T_inputIF<TT_DATA_A, TT_DATA_B> inInterface, T_outputIF<TT_DATA_A, TT_DATA_B> outInterface);
};

//-----------------------------------------------------------------------------------------------------
// iobuffer A, iobuffer B, iobuffer Out

template <typename TT_DATA_A,
          typename TT_DATA_B,
          unsigned int TP_DIM_A,
          unsigned int TP_DIM_B,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_CASC_LEN,
          unsigned int TP_USE_MATRIX_RELOAD,
          unsigned int TP_API,
          unsigned int TP_DUAL_IP,
          unsigned int TP_NUM_OUTPUTS,
          unsigned int TP_KERNEL_POSITION,
          bool TP_CASC_IN,
          bool TP_CASC_OUT>
class matrix_vector_mul : public kernelMatVecMulClass<TT_DATA_A,
                                                      TT_DATA_B,
                                                      TP_DIM_A,
                                                      TP_DIM_B,
                                                      TP_SHIFT,
                                                      TP_RND,
                                                      TP_SAT,
                                                      TP_NUM_FRAMES,
                                                      TP_CASC_LEN,
                                                      TP_USE_MATRIX_RELOAD,
                                                      TP_API,
                                                      TP_DUAL_IP,
                                                      TP_NUM_OUTPUTS,
                                                      TP_KERNEL_POSITION,
                                                      TP_CASC_IN,
                                                      TP_CASC_OUT> {
   public:
    // Constructor
    matrix_vector_mul(){};

    // Register Kernel Class
    static void registerKernelClass() {
        if
            constexpr(TP_CASC_LEN == 1) { REGISTER_FUNCTION(matrix_vector_mul::matVecMul); }
        else if
            constexpr(TP_KERNEL_POSITION == 0) { REGISTER_FUNCTION(matrix_vector_mul::matVecMulFirst); }
        else if
            constexpr(TP_KERNEL_POSITION == TP_CASC_LEN - 1) { REGISTER_FUNCTION(matrix_vector_mul::matVecMulLast); }
        else {
            REGISTER_FUNCTION(matrix_vector_mul::matVecMulMiddle);
        }
    }
    using TT_OUT = outType_t<TT_DATA_A, TT_DATA_B>;

    // GEMV- single kernel
    void matVecMul(input_buffer<TT_DATA_A>& __restrict inWindowA,
                   input_buffer<TT_DATA_B>& __restrict inWindowB,
                   output_buffer<TT_OUT>& __restrict outWindow);
    // GEMV First in a cascade
    void matVecMulFirst(input_buffer<TT_DATA_A>& __restrict inWindowA,
                        input_buffer<TT_DATA_B>& __restrict inWindowB,
                        output_cascade<accType_t<TT_DATA_A, TT_DATA_B> >* outCascade);
    // GEMV Middle in the cascade
    void matVecMulMiddle(input_buffer<TT_DATA_A>& __restrict inWindowA,
                         input_buffer<TT_DATA_B>& __restrict inWindowB,
                         input_cascade<accType_t<TT_DATA_A, TT_DATA_B> >* inCascade,
                         output_cascade<accType_t<TT_DATA_A, TT_DATA_B> >* outCascade);
    // GEMV Last
    void matVecMulLast(input_buffer<TT_DATA_A>& __restrict inWindowA,
                       input_buffer<TT_DATA_B>& __restrict inWindowB,
                       input_cascade<accType_t<TT_DATA_A, TT_DATA_B> >* inCascade,
                       output_buffer<TT_OUT>& __restrict outWindow);
};
//-----------------------------------------------------------------------------------------------------
// RTP A, iobuffer B, iobuffer Out
template <typename TT_DATA_A,
          typename TT_DATA_B,
          unsigned int TP_DIM_A,
          unsigned int TP_DIM_B,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_CASC_LEN,
          //   unsigned int TP_USE_MATRIX_RELOAD,
          //   unsigned int TP_API,
          unsigned int TP_DUAL_IP,
          unsigned int TP_NUM_OUTPUTS,
          unsigned int TP_KERNEL_POSITION,
          bool TP_CASC_IN,
          bool TP_CASC_OUT>
class matrix_vector_mul<TT_DATA_A,
                        TT_DATA_B,
                        TP_DIM_A,
                        TP_DIM_B,
                        TP_SHIFT,
                        TP_RND,
                        TP_SAT,
                        TP_NUM_FRAMES,
                        TP_CASC_LEN,
                        USE_MATRIX_RELOAD_TRUE,
                        IOBUFFER_B,
                        TP_DUAL_IP,
                        TP_NUM_OUTPUTS,
                        TP_KERNEL_POSITION,
                        TP_CASC_IN,
                        TP_CASC_OUT> : public kernelMatVecMulClass<TT_DATA_A,
                                                                   TT_DATA_B,
                                                                   TP_DIM_A,
                                                                   TP_DIM_B,
                                                                   TP_SHIFT,
                                                                   TP_RND,
                                                                   TP_SAT,
                                                                   TP_NUM_FRAMES,
                                                                   TP_CASC_LEN,
                                                                   USE_MATRIX_RELOAD_TRUE,
                                                                   IOBUFFER_B,
                                                                   TP_DUAL_IP,
                                                                   TP_NUM_OUTPUTS,
                                                                   TP_KERNEL_POSITION,
                                                                   TP_CASC_IN,
                                                                   TP_CASC_OUT> {
   public:
    // No ssr here, as TP_DIM_A = TP_DIM_A / TP_SSR was given in graph
    static constexpr unsigned int matrixASize = TP_DIM_A * TP_DIM_B * TP_NUM_FRAMES / (TP_CASC_LEN);
    // Constructor
    matrix_vector_mul(){};
    // Register Kernel Class
    static void registerKernelClass() {
        if
            constexpr(TP_CASC_LEN == 1) { REGISTER_FUNCTION(matrix_vector_mul::matVecMulRtp); }
        else if
            constexpr(TP_KERNEL_POSITION == 0) { REGISTER_FUNCTION(matrix_vector_mul::matVecMulFirstRtp); }
        else if
            constexpr(TP_KERNEL_POSITION == TP_CASC_LEN - 1) { REGISTER_FUNCTION(matrix_vector_mul::matVecMulLastRtp); }
        else {
            REGISTER_FUNCTION(matrix_vector_mul::matVecMulMiddleRtp);
        }
    }
    using TT_OUT = outType_t<TT_DATA_A, TT_DATA_B>;

    // GEMV - single kernel
    void matVecMulRtp(const TT_DATA_A (&inMatrixA)[matrixASize],     // i0
                      input_buffer<TT_DATA_B>& __restrict inWindowB, // i1
                      output_buffer<TT_OUT>& __restrict outWindow);  // o0
    // GEMV First in a cascade
    void matVecMulFirstRtp(const TT_DATA_A (&inMatrixA)[matrixASize],                     // i0
                           input_buffer<TT_DATA_B>& __restrict inWindowB,                 // i1
                           output_cascade<accType_t<TT_DATA_A, TT_DATA_B> >* outCascade); // o0
    // GEMV Middle in the cascade
    void matVecMulMiddleRtp(const TT_DATA_A (&inMatrixA)[matrixASize],                     // i0
                            input_buffer<TT_DATA_B>& __restrict inWindowB,                 // i1
                            input_cascade<accType_t<TT_DATA_A, TT_DATA_B> >* inCascade,    // i2
                            output_cascade<accType_t<TT_DATA_A, TT_DATA_B> >* outCascade); // o0
    // GEMV Last
    void matVecMulLastRtp(const TT_DATA_A (&inMatrixA)[matrixASize],                  // i0
                          input_buffer<TT_DATA_B>& __restrict inWindowB,              // i1
                          input_cascade<accType_t<TT_DATA_A, TT_DATA_B> >* inCascade, // i2
                          output_buffer<TT_OUT>& __restrict outWindow);               // o0
};
//-----------------------------------------------------------------------------------------------------
// RTP A, 1 stream B, 1 stream Out
template <typename TT_DATA_A,
          typename TT_DATA_B,
          unsigned int TP_DIM_A,
          unsigned int TP_DIM_B,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_CASC_LEN,
          //   unsigned int TP_USE_MATRIX_RELOAD,
          //   unsigned int TP_API,
          //   unsigned int TP_DUAL_IP,
          //   unsigned int TP_NUM_OUTPUTS,
          unsigned int TP_KERNEL_POSITION,
          bool TP_CASC_IN,
          bool TP_CASC_OUT>
class matrix_vector_mul<TT_DATA_A,
                        TT_DATA_B,
                        TP_DIM_A,
                        TP_DIM_B,
                        TP_SHIFT,
                        TP_RND,
                        TP_SAT,
                        TP_NUM_FRAMES,
                        TP_CASC_LEN,
                        USE_MATRIX_RELOAD_TRUE,
                        STREAM_B,
                        SINGLE_STREAM_B,
                        SINGLE_STREAM_OUT,
                        TP_KERNEL_POSITION,
                        TP_CASC_IN,
                        TP_CASC_OUT> : public kernelMatVecMulClass<TT_DATA_A,
                                                                   TT_DATA_B,
                                                                   TP_DIM_A,
                                                                   TP_DIM_B,
                                                                   TP_SHIFT,
                                                                   TP_RND,
                                                                   TP_SAT,
                                                                   TP_NUM_FRAMES,
                                                                   TP_CASC_LEN,
                                                                   USE_MATRIX_RELOAD_TRUE,
                                                                   STREAM_B,
                                                                   SINGLE_STREAM_B,
                                                                   SINGLE_STREAM_OUT,
                                                                   TP_KERNEL_POSITION,
                                                                   TP_CASC_IN,
                                                                   TP_CASC_OUT> {
   public:
    // No ssr here, as TP_DIM_A = TP_DIM_A / TP_SSR was given in graph
    // Constructor
    static constexpr unsigned int matrixASize = TP_DIM_A * TP_DIM_B * TP_NUM_FRAMES / (TP_CASC_LEN);

    matrix_vector_mul(){};
    // Register Kernel Class
    static void registerKernelClass() {
        if
            constexpr(TP_CASC_LEN == 1) { REGISTER_FUNCTION(matrix_vector_mul::matVecMulRtpStream); }
        else if
            constexpr(TP_KERNEL_POSITION == 0) { REGISTER_FUNCTION(matrix_vector_mul::matVecMulFirstRtpStream); }
        else if
            constexpr(TP_KERNEL_POSITION == TP_CASC_LEN - 1) {
                REGISTER_FUNCTION(matrix_vector_mul::matVecMulLastRtpStream);
            }
        else {
            REGISTER_FUNCTION(matrix_vector_mul::matVecMulMiddleRtpStream);
        }
    }
    using TT_OUT = outType_t<TT_DATA_A, TT_DATA_B>;

    // GEMV single kernel
    void matVecMulRtpStream(const TT_DATA_A (&inMatrixA)[matrixASize],
                            input_stream<TT_DATA_B>* __restrict inStreamB,
                            output_stream<TT_OUT>* __restrict outStream);
    // GEMV First in a cascade
    void matVecMulFirstRtpStream(const TT_DATA_A (&inMatrixA)[matrixASize],
                                 input_stream<TT_DATA_B>* __restrict inStreamB,
                                 output_cascade<accType_t<TT_DATA_A, TT_DATA_B> >* outCascade);
    // GEMV Middle in the cascade
    void matVecMulMiddleRtpStream(const TT_DATA_A (&inMatrixA)[matrixASize],
                                  input_stream<TT_DATA_B>* __restrict inStreamB,
                                  input_cascade<accType_t<TT_DATA_A, TT_DATA_B> >* inCascade,
                                  output_cascade<accType_t<TT_DATA_A, TT_DATA_B> >* outCascade);
    // GEMV Last
    void matVecMulLastRtpStream(const TT_DATA_A (&inMatrixA)[matrixASize],
                                input_stream<TT_DATA_B>* __restrict inStreamB,
                                input_cascade<accType_t<TT_DATA_A, TT_DATA_B> >* inCascade,
                                output_stream<TT_OUT>* __restrict outStream);
};
//-----------------------------------------------------------------------------------------------------
// RTP A, 2 stream B, 2 stream Out
template <typename TT_DATA_A,
          typename TT_DATA_B,
          unsigned int TP_DIM_A,
          unsigned int TP_DIM_B,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_CASC_LEN,
          //   unsigned int TP_USE_MATRIX_RELOAD,
          //   unsigned int TP_API,
          //   unsigned int TP_DUAL_IP,
          //   unsigned int TP_NUM_OUTPUTS,
          unsigned int TP_KERNEL_POSITION,
          bool TP_CASC_IN,
          bool TP_CASC_OUT>
class matrix_vector_mul<TT_DATA_A,
                        TT_DATA_B,
                        TP_DIM_A,
                        TP_DIM_B,
                        TP_SHIFT,
                        TP_RND,
                        TP_SAT,
                        TP_NUM_FRAMES,
                        TP_CASC_LEN,
                        USE_MATRIX_RELOAD_TRUE,
                        STREAM_B,
                        DUAL_STREAM_B,
                        DUAL_STREAM_OUT,
                        TP_KERNEL_POSITION,
                        TP_CASC_IN,
                        TP_CASC_OUT> : public kernelMatVecMulClass<TT_DATA_A,
                                                                   TT_DATA_B,
                                                                   TP_DIM_A,
                                                                   TP_DIM_B,
                                                                   TP_SHIFT,
                                                                   TP_RND,
                                                                   TP_SAT,
                                                                   TP_NUM_FRAMES,
                                                                   TP_CASC_LEN,
                                                                   USE_MATRIX_RELOAD_TRUE,
                                                                   STREAM_B,
                                                                   DUAL_STREAM_B,
                                                                   DUAL_STREAM_OUT,
                                                                   TP_KERNEL_POSITION,
                                                                   TP_CASC_IN,
                                                                   TP_CASC_OUT> {
   public:
    // No ssr here, as TP_DIM_A = TP_DIM_A / TP_SSR was given in graph
    // Constructor
    static constexpr unsigned int matrixASize = TP_DIM_A * TP_DIM_B * TP_NUM_FRAMES / (TP_CASC_LEN);

    matrix_vector_mul(){};
    // Register Kernel Class
    static void registerKernelClass() {
        if
            constexpr(TP_CASC_LEN == 1) { REGISTER_FUNCTION(matrix_vector_mul::matVecMulRtpStream); }
        else if
            constexpr(TP_KERNEL_POSITION == 0) { REGISTER_FUNCTION(matrix_vector_mul::matVecMulFirstRtpStream); }
        else if
            constexpr(TP_KERNEL_POSITION == TP_CASC_LEN - 1) {
                REGISTER_FUNCTION(matrix_vector_mul::matVecMulLastRtpStream);
            }
        else {
            REGISTER_FUNCTION(matrix_vector_mul::matVecMulMiddleRtpStream);
        }
    }
    using TT_OUT = outType_t<TT_DATA_A, TT_DATA_B>;

    // GEMV single kernel
    void matVecMulRtpStream(const TT_DATA_A (&inMatrixA)[matrixASize],      // in[0]
                            input_stream<TT_DATA_B>* __restrict inStreamB,  // in[1]
                            input_stream<TT_DATA_B>* __restrict inStreamB2, // in[2]
                            output_stream<TT_OUT>* __restrict outStream,    // out[0]
                            output_stream<TT_OUT>* __restrict outStream2);  // out[1]
    // GEMV First in a cascade
    void matVecMulFirstRtpStream(const TT_DATA_A (&inMatrixA)[matrixASize],                     // in[0]
                                 input_stream<TT_DATA_B>* __restrict inStreamB,                 // in[1]
                                 input_stream<TT_DATA_B>* __restrict inStreamB2,                // in[2]
                                 output_cascade<accType_t<TT_DATA_A, TT_DATA_B> >* outCascade); // out[0]
    // GEMV Middle in the cascade
    void matVecMulMiddleRtpStream(const TT_DATA_A (&inMatrixA)[matrixASize],                     // in[0]
                                  input_stream<TT_DATA_B>* __restrict inStreamB,                 // in[1]
                                  input_stream<TT_DATA_B>* __restrict inStreamB2,                // in[2]
                                  input_cascade<accType_t<TT_DATA_A, TT_DATA_B> >* inCascade,    // in[3]
                                  output_cascade<accType_t<TT_DATA_A, TT_DATA_B> >* outCascade); // out[0]
    // GEMV Last
    void matVecMulLastRtpStream(const TT_DATA_A (&inMatrixA)[matrixASize],                  // in[0]
                                input_stream<TT_DATA_B>* __restrict inStreamB,              // in[1]
                                input_stream<TT_DATA_B>* __restrict inStreamB2,             // in[2]
                                input_cascade<accType_t<TT_DATA_A, TT_DATA_B> >* inCascade, // in[3]
                                output_stream<TT_OUT>* __restrict outStream,                // out[0]
                                output_stream<TT_OUT>* __restrict outStream2);              // out[1]
};
//-----------------------------------------------------------------------------------------------------
// RTP A, 1 stream B, 2 stream Out
template <typename TT_DATA_A,
          typename TT_DATA_B,
          unsigned int TP_DIM_A,
          unsigned int TP_DIM_B,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_CASC_LEN,
          //   unsigned int TP_USE_MATRIX_RELOAD,
          //   unsigned int TP_API,
          //   unsigned int TP_DUAL_IP,
          //   unsigned int TP_NUM_OUTPUTS,
          unsigned int TP_KERNEL_POSITION,
          bool TP_CASC_IN,
          bool TP_CASC_OUT>
class matrix_vector_mul<TT_DATA_A,
                        TT_DATA_B,
                        TP_DIM_A,
                        TP_DIM_B,
                        TP_SHIFT,
                        TP_RND,
                        TP_SAT,
                        TP_NUM_FRAMES,
                        TP_CASC_LEN,
                        USE_MATRIX_RELOAD_TRUE,
                        STREAM_B,
                        SINGLE_STREAM_B,
                        DUAL_STREAM_OUT,
                        TP_KERNEL_POSITION,
                        TP_CASC_IN,
                        TP_CASC_OUT> : public kernelMatVecMulClass<TT_DATA_A,
                                                                   TT_DATA_B,
                                                                   TP_DIM_A,
                                                                   TP_DIM_B,
                                                                   TP_SHIFT,
                                                                   TP_RND,
                                                                   TP_SAT,
                                                                   TP_NUM_FRAMES,
                                                                   TP_CASC_LEN,
                                                                   USE_MATRIX_RELOAD_TRUE,
                                                                   STREAM_B,
                                                                   SINGLE_STREAM_B,
                                                                   DUAL_STREAM_OUT,
                                                                   TP_KERNEL_POSITION,
                                                                   TP_CASC_IN,
                                                                   TP_CASC_OUT> {
   public:
    // No ssr here, as TP_DIM_A = TP_DIM_A / TP_SSR was given in graph
    // Constructor
    static constexpr unsigned int matrixASize = TP_DIM_A * TP_DIM_B * TP_NUM_FRAMES / (TP_CASC_LEN);

    matrix_vector_mul(){};
    // Register Kernel Class
    static void registerKernelClass() {
        if
            constexpr(TP_CASC_LEN == 1) { REGISTER_FUNCTION(matrix_vector_mul::matVecMulRtpStream); }
        else if
            constexpr(TP_KERNEL_POSITION == 0) { REGISTER_FUNCTION(matrix_vector_mul::matVecMulFirstRtpStream); }
        else if
            constexpr(TP_KERNEL_POSITION == TP_CASC_LEN - 1) {
                REGISTER_FUNCTION(matrix_vector_mul::matVecMulLastRtpStream);
            }
        else {
            REGISTER_FUNCTION(matrix_vector_mul::matVecMulMiddleRtpStream);
        }
    }
    using TT_OUT = outType_t<TT_DATA_A, TT_DATA_B>;

    // GEMV single kernel
    void matVecMulRtpStream(const TT_DATA_A (&inMatrixA)[matrixASize],     // in[0]
                            input_stream<TT_DATA_B>* __restrict inStreamB, // in[1]
                            output_stream<TT_OUT>* __restrict outStream,   // out[0]
                            output_stream<TT_OUT>* __restrict outStream2); // out[1]
    // GEMV First in a cascade
    void matVecMulFirstRtpStream(const TT_DATA_A (&inMatrixA)[matrixASize],                     // in[0]
                                 input_stream<TT_DATA_B>* __restrict inStreamB,                 // in[1]
                                 output_cascade<accType_t<TT_DATA_A, TT_DATA_B> >* outCascade); // out[0]
    // GEMV Middle in the cascade
    void matVecMulMiddleRtpStream(const TT_DATA_A (&inMatrixA)[matrixASize],                     // in[0]
                                  input_stream<TT_DATA_B>* __restrict inStreamB,                 // in[1]
                                  input_cascade<accType_t<TT_DATA_A, TT_DATA_B> >* inCascade,    // in[2]
                                  output_cascade<accType_t<TT_DATA_A, TT_DATA_B> >* outCascade); // out[0]
    // GEMV Last
    void matVecMulLastRtpStream(const TT_DATA_A (&inMatrixA)[matrixASize],                  // in[0]
                                input_stream<TT_DATA_B>* __restrict inStreamB,              // in[1]
                                input_cascade<accType_t<TT_DATA_A, TT_DATA_B> >* inCascade, // in[2]
                                output_stream<TT_OUT>* __restrict outStream,                // out[0]
                                output_stream<TT_OUT>* __restrict outStream2);              // out[1]
};
//-----------------------------------------------------------------------------------------------------
// RTP A, 2 stream B, 1 stream Out
template <typename TT_DATA_A,
          typename TT_DATA_B,
          unsigned int TP_DIM_A,
          unsigned int TP_DIM_B,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_CASC_LEN,
          //   unsigned int TP_USE_MATRIX_RELOAD,
          //   unsigned int TP_API,
          //   unsigned int TP_DUAL_IP,
          //   unsigned int TP_NUM_OUTPUTS,
          unsigned int TP_KERNEL_POSITION,
          bool TP_CASC_IN,
          bool TP_CASC_OUT>
class matrix_vector_mul<TT_DATA_A,
                        TT_DATA_B,
                        TP_DIM_A,
                        TP_DIM_B,
                        TP_SHIFT,
                        TP_RND,
                        TP_SAT,
                        TP_NUM_FRAMES,
                        TP_CASC_LEN,
                        USE_MATRIX_RELOAD_TRUE,
                        STREAM_B,
                        DUAL_STREAM_B,
                        SINGLE_STREAM_OUT,
                        TP_KERNEL_POSITION,
                        TP_CASC_IN,
                        TP_CASC_OUT> : public kernelMatVecMulClass<TT_DATA_A,
                                                                   TT_DATA_B,
                                                                   TP_DIM_A,
                                                                   TP_DIM_B,
                                                                   TP_SHIFT,
                                                                   TP_RND,
                                                                   TP_SAT,
                                                                   TP_NUM_FRAMES,
                                                                   TP_CASC_LEN,
                                                                   USE_MATRIX_RELOAD_TRUE,
                                                                   STREAM_B,
                                                                   DUAL_STREAM_B,
                                                                   SINGLE_STREAM_OUT,
                                                                   TP_KERNEL_POSITION,
                                                                   TP_CASC_IN,
                                                                   TP_CASC_OUT> {
   public:
    // No ssr here, as TP_DIM_A = TP_DIM_A / TP_SSR was given in graph
    // Constructor
    static constexpr unsigned int matrixASize = TP_DIM_A * TP_DIM_B * TP_NUM_FRAMES / (TP_CASC_LEN);

    matrix_vector_mul(){};
    // Register Kernel Class
    static void registerKernelClass() {
        if
            constexpr(TP_CASC_LEN == 1) { REGISTER_FUNCTION(matrix_vector_mul::matVecMulRtpStream); }
        else if
            constexpr(TP_KERNEL_POSITION == 0) { REGISTER_FUNCTION(matrix_vector_mul::matVecMulFirstRtpStream); }
        else if
            constexpr(TP_KERNEL_POSITION == TP_CASC_LEN - 1) {
                REGISTER_FUNCTION(matrix_vector_mul::matVecMulLastRtpStream);
            }
        else {
            REGISTER_FUNCTION(matrix_vector_mul::matVecMulMiddleRtpStream);
        }
    }
    using TT_OUT = outType_t<TT_DATA_A, TT_DATA_B>;

    // GEMV single kernel
    void matVecMulRtpStream(const TT_DATA_A (&inMatrixA)[matrixASize],      // in[0]
                            input_stream<TT_DATA_B>* __restrict inStreamB,  // in[1]
                            input_stream<TT_DATA_B>* __restrict inStreamB2, // in[2]
                            output_stream<TT_OUT>* __restrict outStream);   // out[0])
    // GEMV First in a cascade
    void matVecMulFirstRtpStream(const TT_DATA_A (&inMatrixA)[matrixASize],                     // in[0]
                                 input_stream<TT_DATA_B>* __restrict inStreamB,                 // in[1]
                                 input_stream<TT_DATA_B>* __restrict inStreamB2,                // in[2]
                                 output_cascade<accType_t<TT_DATA_A, TT_DATA_B> >* outCascade); // out[0]
    // GEMV Middle in the cascade
    void matVecMulMiddleRtpStream(const TT_DATA_A (&inMatrixA)[matrixASize],                     // in[0]
                                  input_stream<TT_DATA_B>* __restrict inStreamB,                 // in[1]
                                  input_stream<TT_DATA_B>* __restrict inStreamB2,                // in[2]
                                  input_cascade<accType_t<TT_DATA_A, TT_DATA_B> >* inCascade,    // in[3]
                                  output_cascade<accType_t<TT_DATA_A, TT_DATA_B> >* outCascade); // out[0]
    // GEMV Last
    void matVecMulLastRtpStream(const TT_DATA_A (&inMatrixA)[matrixASize],                  // in[0]
                                input_stream<TT_DATA_B>* __restrict inStreamB,              // in[1]
                                input_stream<TT_DATA_B>* __restrict inStreamB2,             // in[2]
                                input_cascade<accType_t<TT_DATA_A, TT_DATA_B> >* inCascade, // in[3]
                                output_stream<TT_OUT>* __restrict outStream);               // out[0]
};
}
}
}
}
}
#endif // _DSPLIB_MATRIX_VECTOR_MUL_HPP_
