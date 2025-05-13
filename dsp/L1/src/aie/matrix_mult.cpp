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
/*
This file holds the body of the kernel class for the Asymmetric Interpolation FIR.
Unlike single rate implementations, this interpolation FIR calculates sets of output
vectors in parallel such that the number of lanes in total is a multiple of the
interpolation factor.

Coding conventions
  TT_      template type suffix
  TP_      template parameter suffix
*/

#include <adf.h>

//#include <aie_api/aie_adf.hpp>
#ifndef __NEW_WINDOW_H__
#endif
// if we use 1kb registers -> aie api uses 2x512b registers for 1024b so we need this for QoR
#ifndef __AIE_API_USE_NATIVE_1024B_VECTOR__
#define __AIE_API_USE_NATIVE_1024B_VECTOR__
#endif
#include "aie_api/aie_adf.hpp"

#include "matrix_mult.hpp" //hence including matrix_mult_traits.hpp too
#include "kernel_api_utils.hpp"

//#define _DSPLIB_MATRIX_MULT_HPP_DEBUG_

namespace xf {
namespace dsp {
namespace aie {
namespace blas {
namespace matrix_mult {

// aie_api is external to xf::dsp::aie namespace
namespace aie = ::aie;

//-----------------------------------------------------------------------------------------------------
//#TEMPLATE_FUNCTION_DEFINITION
template <typename TT_DATA_A,
          typename TT_DATA_B,
          typename TT_OUT_DATA,
          unsigned int TP_DIM_A,
          unsigned int TP_DIM_AB,
          unsigned int TP_DIM_B,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_DIM_A_LEADING, // = ROW_MAJOR,
          unsigned int TP_DIM_B_LEADING, // = COL_MAJOR,
          unsigned int TP_DIM_OUT_LEADING,
          unsigned int TP_INPUT_WINDOW_VSIZE_A,
          unsigned int TP_INPUT_WINDOW_VSIZE_B,
          bool TP_CASC_IN,
          bool TP_CASC_OUT,
          unsigned int TP_DIM_A_RANGE,
          unsigned int TP_DIM_AB_RANGE,
          unsigned int TP_DIM_B_RANGE,
          unsigned int TP_KERNEL_POSITION,
          unsigned int TP_CASC_LEN>
INLINE_DECL void kernelMatMultClass<
    TT_DATA_A,
    TT_DATA_B,
    TT_OUT_DATA,
    TP_DIM_A,
    TP_DIM_AB,
    TP_DIM_B,
    TP_SHIFT,
    TP_RND,
    TP_SAT,
    TP_DIM_A_LEADING,
    TP_DIM_B_LEADING,
    TP_DIM_OUT_LEADING,
    TP_INPUT_WINDOW_VSIZE_A,
    TP_INPUT_WINDOW_VSIZE_B,
    TP_CASC_IN,
    TP_CASC_OUT,
    TP_DIM_A_RANGE,
    TP_DIM_AB_RANGE,
    TP_DIM_B_RANGE,
    TP_KERNEL_POSITION,
    TP_CASC_LEN>::matMultKernel(T_inputIF<TP_CASC_IN, TT_DATA_A, TT_DATA_B> inInterface,
                                T_outputIF<TP_CASC_OUT, TT_DATA_A, TT_DATA_B, TT_OUT_DATA> outInterface) {
    // This function hides exposure of the implementation choice from the user.
    matMult_impl1(inInterface, outInterface);
};

template <typename TT_DATA_A,
          typename TT_DATA_B,
          typename TT_OUT_DATA,
          unsigned int TP_DIM_A,
          unsigned int TP_DIM_AB,
          unsigned int TP_DIM_B,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_DIM_A_LEADING, // = ROW_MAJOR,
          unsigned int TP_DIM_B_LEADING, // = COL_MAJOR,
          unsigned int TP_DIM_OUT_LEADING,
          unsigned int TP_INPUT_WINDOW_VSIZE_A,
          unsigned int TP_INPUT_WINDOW_VSIZE_B,
          bool TP_CASC_IN,
          bool TP_CASC_OUT,
          unsigned int TP_DIM_A_RANGE,
          unsigned int TP_DIM_AB_RANGE,
          unsigned int TP_DIM_B_RANGE,
          unsigned int TP_KERNEL_POSITION,
          unsigned int TP_CASC_LEN>
INLINE_DECL void kernelMatMultClass<
    TT_DATA_A,
    TT_DATA_B,
    TT_OUT_DATA,
    TP_DIM_A,
    TP_DIM_AB,
    TP_DIM_B,
    TP_SHIFT,
    TP_RND,
    TP_SAT,
    TP_DIM_A_LEADING,
    TP_DIM_B_LEADING,
    TP_DIM_OUT_LEADING,
    TP_INPUT_WINDOW_VSIZE_A,
    TP_INPUT_WINDOW_VSIZE_B,
    TP_CASC_IN,
    TP_CASC_OUT,
    TP_DIM_A_RANGE,
    TP_DIM_AB_RANGE,
    TP_DIM_B_RANGE,
    TP_KERNEL_POSITION,
    TP_CASC_LEN>::matMult_impl1(T_inputIF<TP_CASC_IN, TT_DATA_A, TT_DATA_B> inInterface,
                                T_outputIF<TP_CASC_OUT, TT_DATA_A, TT_DATA_B, TT_OUT_DATA> outInterface) {
    set_rnd_mode<TP_RND>();
    set_sat_mode<TP_SAT>();
    constexpr unsigned int M = tilingScheme.Atile;
    constexpr unsigned int N = tilingScheme.ABtile;
    constexpr unsigned int K = tilingScheme.Btile;
    constexpr unsigned int sizeTileA = M * N;
    constexpr unsigned int sizeTileB = N * K;
    constexpr unsigned int sizeTileC = M * K;
    constexpr bool parallelA =
        ((TP_DIM_A / M) % 2 == 0 &&
         TP_DIM_A > M); // TODO: deal with odd numbers better (last loop iteration has no parallel A)
    constexpr bool parallelB = ((TP_DIM_B / K) % 2 == 0 && TP_DIM_B > K);
    using MMUL = aie::mmul<M, N, K, TT_DATA_A, TT_DATA_B, accType_t<TT_DATA_A, TT_DATA_B> >;

    constexpr unsigned int numAReg = (parallelA) ? 2 : 1;
    constexpr unsigned int numBReg = (parallelB) ? 2 : 1;

    // 0 disable pipelining, 1 do not decrement loop count, 2 decrement loop count once and duplicate loop to POSTAMBLE,
    // 3 decrement loop count once for PREAMBLE, 4 decrement twice for PREAMBLE & POSTAMBLE
    const unsigned int non_leaf_loop_sol = 4;
    TT_DATA_A* inputAPtr = (TT_DATA_A*)inInterface.inWindowA;
    TT_DATA_B* inputBPtr = (TT_DATA_B*)inInterface.inWindowB;

    TT_OUT_DATA* tiledOutWindowPtr;
    TT_OUT_DATA* outPtr = (TT_OUT_DATA*)outInterface.outWindow;

    if
        constexpr(TP_CASC_OUT == CASC_OUT_FALSE) { tiledOutWindowPtr = (TT_OUT_DATA*)outInterface.outWindow; }
    else {
        tiledOutWindowPtr = nullptr;
    }

    for (unsigned AChunk = 0; AChunk < TP_DIM_A / M; AChunk += numAReg)
        chess_prepare_for_pipelining chess_loop_count((TP_DIM_A / M) / numAReg) // TP_DIM_A)
        {
            // window pointer for output
            TT_OUT_DATA* __restrict pC1 = (TP_CASC_OUT == CASC_OUT_FALSE)
                                              ? tiledOutWindowPtr + ((AChunk * TP_DIM_B / K + 0) * sizeTileC)
                                              : nullptr;
            TT_OUT_DATA* __restrict pC2 = (TP_CASC_OUT == CASC_OUT_FALSE && parallelA)
                                              ? tiledOutWindowPtr + (((AChunk + 1) * TP_DIM_B / K + 0) * sizeTileC)
                                              : nullptr;

            for (unsigned BChunk = 0; BChunk < TP_DIM_B / K; BChunk += numBReg)
                chess_prepare_for_pipelining chess_pipeline_non_leaf_loop_solution(non_leaf_loop_sol)
                    chess_loop_count((TP_DIM_B / K) / numBReg) // TP_DIM_B/K)
                {
                    const TT_DATA_A* __restrict pA1 = inputAPtr + ((AChunk * TP_DIM_AB / N + 0) * sizeTileA);
                    const TT_DATA_A* __restrict pA2;
                    if
                        constexpr(parallelA) { pA2 = inputAPtr + (((AChunk + 1) * TP_DIM_AB / N + 0) * sizeTileA); }
                    const TT_DATA_B* __restrict pB1 = inputBPtr + ((0 * TP_DIM_B / K + BChunk) * sizeTileB);
                    const TT_DATA_B* __restrict pB2;
                    if
                        constexpr(parallelB) { pB2 = inputBPtr + ((0 * TP_DIM_B / K + (BChunk + 1)) * sizeTileB); }

                    aie::vector<TT_DATA_A, sizeTileA> A0 = aie::load_v<sizeTileA>(pA1);
                    pA1 += sizeTileA;
                    aie::vector<TT_DATA_A, sizeTileA> A1;
                    if
                        constexpr(parallelA) {
                            A1 = aie::load_v<sizeTileA>(pA2);
                            pA2 += sizeTileA;
                        }

                    aie::vector<TT_DATA_B, sizeTileB> B0 = aie::load_v<sizeTileB>(pB1);
                    pB1 += sizeTileB * TP_DIM_B / K;
                    aie::vector<TT_DATA_B, sizeTileB> B1;
                    if
                        constexpr(parallelB) {
                            B1 = aie::load_v<sizeTileB>(pB2);
                            pB2 += sizeTileB * TP_DIM_B / K;
                        }

                    // initial muls
                    MMUL C00;
                    MMUL C01;
                    MMUL C10;
                    MMUL C11;
                    if
                        constexpr(TP_CASC_IN == CASC_IN_TRUE) {
                            // hoping AIE API readincr_v will infer the lanes.
                            // guess for sizeTileC
                            // could also try to replae sizeTileC with MMUL::accum_type::size()   or just
                            // MMUL::accum_type::Elems
                            C00 = MMUL(readincr_v<sizeTileC>(inInterface.inCascade));
                            if
                                constexpr(parallelB) C01 = MMUL(readincr_v<sizeTileC>(inInterface.inCascade));
                            if
                                constexpr(parallelA) C10 = MMUL(readincr_v<sizeTileC>(inInterface.inCascade));
                            if
                                constexpr(parallelA && parallelB) C11 =
                                    MMUL(readincr_v<sizeTileC>(inInterface.inCascade));
                            C00.mac(A0, B0);
                            if
                                constexpr(parallelB) C01.mac(A0, B1);
                            if
                                constexpr(parallelA) C10.mac(A1, B0);
                            if
                                constexpr(parallelA && parallelB) C11.mac(A1, B1);
                        }
                    else {
                        C00.mul(A0, B0);
                        if
                            constexpr(parallelB) C01.mul(A0, B1);
                        if
                            constexpr(parallelA) C10.mul(A1, B0);
                        if
                            constexpr(parallelA && parallelB) C11.mul(A1, B1);
                    }

                    //#pragma unroll((TP_DIM_AB/N -1))
                    for (unsigned i = 1; i < TP_DIM_AB / N; ++i)
                        chess_loop_count((TP_DIM_AB / N - 1)) chess_prepare_for_pipelining {
                            A0 = aie::load_v<sizeTileA>(pA1);
                            pA1 += sizeTileA;
                            if
                                constexpr(parallelA) {
                                    A1 = aie::load_v<sizeTileA>(pA2);
                                    pA2 += sizeTileA;
                                }

                            B0 = aie::load_v<sizeTileB>(pB1);
                            pB1 += sizeTileB * TP_DIM_B / K;
                            if
                                constexpr(parallelB) {
                                    B1 = aie::load_v<sizeTileB>(pB2);
                                    pB2 += sizeTileB * TP_DIM_B / K;
                                }

                            C00.mac(A0, B0);
                            if
                                constexpr(parallelB) { C01.mac(A0, B1); }
                            if
                                constexpr(parallelA) { C10.mac(A1, B0); }
                            if
                                constexpr(parallelA && parallelB) { C11.mac(A1, B1); }
                        }

                    if
                        constexpr(TP_CASC_OUT == CASC_OUT_FALSE) {
                            aie::store_v(pC1, C00.template to_vector<TT_OUT_DATA>(TP_SHIFT));
                            pC1 += sizeTileC;
                            if
                                constexpr(parallelB) {
                                    aie::store_v(pC1, C01.template to_vector<TT_OUT_DATA>(TP_SHIFT));
                                    pC1 += sizeTileC;
                                }
                            if
                                constexpr(parallelA) {
                                    aie::store_v(pC2, C10.template to_vector<TT_OUT_DATA>(TP_SHIFT));
                                    pC2 += sizeTileC;
                                }
                            if
                                constexpr(parallelA && parallelB) {
                                    aie::store_v(pC2, C11.template to_vector<TT_OUT_DATA>(TP_SHIFT));
                                    pC2 += sizeTileC;
                                }
                        }
                    else {
                        writeincr(outInterface.outWindow, C00.to_accum());
                        if
                            constexpr(parallelB) { writeincr(outInterface.outWindow, C01.to_accum()); }
                        if
                            constexpr(parallelA) { writeincr(outInterface.outWindow, C10.to_accum()); }
                        if
                            constexpr(parallelA && parallelB) { writeincr(outInterface.outWindow, C11.to_accum()); }
                    }
                }
        }
}

// function overloaded with cascade interface variations
// This is a specialization of the main class for when there is only one kernel for the whole function.
//-----------------------------------------------------------------------------------------------------
template <typename TT_DATA_A,
          typename TT_DATA_B,
          typename TT_OUT_DATA,
          unsigned int TP_DIM_A,
          unsigned int TP_DIM_AB,
          unsigned int TP_DIM_B,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_DIM_A_LEADING, // = ROW_MAJOR,
          unsigned int TP_DIM_B_LEADING, // = COL_MAJOR,
          unsigned int TP_DIM_OUT_LEADING,
          unsigned int TP_INPUT_WINDOW_VSIZE_A,
          unsigned int TP_INPUT_WINDOW_VSIZE_B,
          bool TP_CASC_IN,
          bool TP_CASC_OUT,
          unsigned int TP_DIM_A_RANGE,
          unsigned int TP_DIM_AB_RANGE,
          unsigned int TP_DIM_B_RANGE,
          unsigned int TP_KERNEL_POSITION,
          unsigned int TP_CASC_LEN>
NOINLINE_DECL void matrix_mult<TT_DATA_A,
                               TT_DATA_B,
                               TT_OUT_DATA,
                               TP_DIM_A,
                               TP_DIM_AB,
                               TP_DIM_B,
                               TP_SHIFT,
                               TP_RND,
                               TP_SAT,
                               TP_DIM_A_LEADING,
                               TP_DIM_B_LEADING,
                               TP_DIM_OUT_LEADING,
                               TP_INPUT_WINDOW_VSIZE_A,
                               TP_INPUT_WINDOW_VSIZE_B,
                               TP_CASC_IN,
                               TP_CASC_OUT,
                               TP_DIM_A_RANGE,
                               TP_DIM_AB_RANGE,
                               TP_DIM_B_RANGE,
                               TP_KERNEL_POSITION,
                               TP_CASC_LEN>::matMult(input_buffer<TT_DATA_A>& __restrict inWindowA,
                                                     input_buffer<TT_DATA_B>& __restrict inWindowB,
                                                     output_buffer<TT_OUT_DATA>& __restrict outWindow) {
    T_inputIF<CASC_IN_FALSE, TT_DATA_A, TT_DATA_B> inInterface;
    T_outputIF<CASC_OUT_FALSE, TT_DATA_A, TT_DATA_B, TT_OUT_DATA> outInterface;
    inInterface.inWindowA = (void*)inWindowA.data();
    inInterface.inWindowB = (void*)inWindowB.data();
    outInterface.outWindow = outWindow.data();
    this->matMultKernel(inInterface, outInterface);
};

// function overloaded with cascade interface variations
// This is a specialization of the main class for the final kernel in a cascade chain.
//-----------------------------------------------------------------------------------------------------
template <typename TT_DATA_A,
          typename TT_DATA_B,
          typename TT_OUT_DATA,
          unsigned int TP_DIM_A,
          unsigned int TP_DIM_AB,
          unsigned int TP_DIM_B,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_DIM_A_LEADING, // = ROW_MAJOR,
          unsigned int TP_DIM_B_LEADING, // = COL_MAJOR,
          unsigned int TP_DIM_OUT_LEADING,
          unsigned int TP_INPUT_WINDOW_VSIZE_A,
          unsigned int TP_INPUT_WINDOW_VSIZE_B,
          unsigned int TP_DIM_A_RANGE,
          unsigned int TP_DIM_AB_RANGE,
          unsigned int TP_DIM_B_RANGE,
          unsigned int TP_KERNEL_POSITION,
          unsigned int TP_CASC_LEN>
void matrix_mult<TT_DATA_A,
                 TT_DATA_B,
                 TT_OUT_DATA,
                 TP_DIM_A,
                 TP_DIM_AB,
                 TP_DIM_B,
                 TP_SHIFT,
                 TP_RND,
                 TP_SAT,
                 TP_DIM_A_LEADING,
                 TP_DIM_B_LEADING,
                 TP_DIM_OUT_LEADING,
                 TP_INPUT_WINDOW_VSIZE_A,
                 TP_INPUT_WINDOW_VSIZE_B,
                 CASC_IN_TRUE,
                 CASC_OUT_FALSE,
                 TP_DIM_A_RANGE,
                 TP_DIM_AB_RANGE,
                 TP_DIM_B_RANGE,
                 TP_KERNEL_POSITION,
                 TP_CASC_LEN>::matMult(input_buffer<TT_DATA_A>& __restrict inWindowA,
                                       input_buffer<TT_DATA_B>& __restrict inWindowB,
                                       input_cascade<accType_t<TT_DATA_A, TT_DATA_B> >* inCascade,
                                       output_buffer<TT_OUT_DATA>& __restrict outWindow) {
    T_inputIF<CASC_IN_TRUE, TT_DATA_A, TT_DATA_B> inInterface;
    T_outputIF<CASC_OUT_FALSE, TT_DATA_A, TT_DATA_B, TT_OUT_DATA> outInterface;
    inInterface.inWindowA = (void*)inWindowA.data();
    inInterface.inWindowB = (void*)inWindowB.data();
    inInterface.inCascade = inCascade;
    outInterface.outWindow = outWindow.data();
    this->matMultKernel(inInterface, outInterface);
};

// function overloaded with cascade interface variations
// This is a specialization of the main class for the first kernel in a cascade chain.
//-----------------------------------------------------------------------------------------------------
template <typename TT_DATA_A,
          typename TT_DATA_B,
          typename TT_OUT_DATA,
          unsigned int TP_DIM_A,
          unsigned int TP_DIM_AB,
          unsigned int TP_DIM_B,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_DIM_A_LEADING, // = ROW_MAJOR,
          unsigned int TP_DIM_B_LEADING, // = COL_MAJOR,
          unsigned int TP_DIM_OUT_LEADING,
          unsigned int TP_INPUT_WINDOW_VSIZE_A,
          unsigned int TP_INPUT_WINDOW_VSIZE_B,
          unsigned int TP_DIM_A_RANGE,
          unsigned int TP_DIM_AB_RANGE,
          unsigned int TP_DIM_B_RANGE,
          unsigned int TP_KERNEL_POSITION,
          unsigned int TP_CASC_LEN>
void matrix_mult<TT_DATA_A,
                 TT_DATA_B,
                 TT_OUT_DATA,
                 TP_DIM_A,
                 TP_DIM_AB,
                 TP_DIM_B,
                 TP_SHIFT,
                 TP_RND,
                 TP_SAT,
                 TP_DIM_A_LEADING,
                 TP_DIM_B_LEADING,
                 TP_DIM_OUT_LEADING,
                 TP_INPUT_WINDOW_VSIZE_A,
                 TP_INPUT_WINDOW_VSIZE_B,
                 CASC_IN_FALSE,
                 CASC_OUT_TRUE,
                 TP_DIM_A_RANGE,
                 TP_DIM_AB_RANGE,
                 TP_DIM_B_RANGE,
                 TP_KERNEL_POSITION,
                 TP_CASC_LEN>::matMult(input_buffer<TT_DATA_A>& __restrict inWindowA,
                                       input_buffer<TT_DATA_B>& __restrict inWindowB,
                                       output_cascade<accType_t<TT_DATA_A, TT_DATA_B> >* outCascade) {
    T_inputIF<CASC_IN_FALSE, TT_DATA_A, TT_DATA_B> inInterface;
    T_outputIF<CASC_OUT_TRUE, TT_DATA_A, TT_DATA_B, TT_OUT_DATA> outInterface;
    inInterface.inWindowA = (void*)inWindowA.data();
    inInterface.inWindowB = (void*)inWindowB.data();
    outInterface.outWindow = outCascade; // toodo rename outWindow to just outPort
    this->matMultKernel(inInterface, outInterface);
};

// function overloaded with cascade interface variations
// This is a specialization of the main class for any kernel within a cascade chain, but neither first nor last.
//-----------------------------------------------------------------------------------------------------
template <typename TT_DATA_A,
          typename TT_DATA_B,
          typename TT_OUT_DATA,
          unsigned int TP_DIM_A,
          unsigned int TP_DIM_AB,
          unsigned int TP_DIM_B,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_DIM_A_LEADING, // = ROW_MAJOR,
          unsigned int TP_DIM_B_LEADING, // = COL_MAJOR,
          unsigned int TP_DIM_OUT_LEADING,
          unsigned int TP_INPUT_WINDOW_VSIZE_A,
          unsigned int TP_INPUT_WINDOW_VSIZE_B,
          unsigned int TP_DIM_A_RANGE,
          unsigned int TP_DIM_AB_RANGE,
          unsigned int TP_DIM_B_RANGE,
          unsigned int TP_KERNEL_POSITION,
          unsigned int TP_CASC_LEN>
void matrix_mult<TT_DATA_A,
                 TT_DATA_B,
                 TT_OUT_DATA,
                 TP_DIM_A,
                 TP_DIM_AB,
                 TP_DIM_B,
                 TP_SHIFT,
                 TP_RND,
                 TP_SAT,
                 TP_DIM_A_LEADING,
                 TP_DIM_B_LEADING,
                 TP_DIM_OUT_LEADING,
                 TP_INPUT_WINDOW_VSIZE_A,
                 TP_INPUT_WINDOW_VSIZE_B,
                 CASC_IN_TRUE,
                 CASC_OUT_TRUE,
                 TP_DIM_A_RANGE,
                 TP_DIM_AB_RANGE,
                 TP_DIM_B_RANGE,
                 TP_KERNEL_POSITION,
                 TP_CASC_LEN>::matMult(input_buffer<TT_DATA_A>& inWindowA,
                                       input_buffer<TT_DATA_B>& inWindowB,
                                       input_cascade<accType_t<TT_DATA_A, TT_DATA_B> >* inCascade,
                                       output_cascade<accType_t<TT_DATA_A, TT_DATA_B> >* outCascade) {
    T_inputIF<CASC_IN_TRUE, TT_DATA_A, TT_DATA_B> inInterface;
    T_outputIF<CASC_OUT_TRUE, TT_DATA_A, TT_DATA_B, TT_OUT_DATA> outInterface;
    inInterface.inWindowA = (void*)inWindowA.data();
    inInterface.inWindowB = (void*)inWindowB.data();
    inInterface.inCascade = inCascade;
    outInterface.outWindow = outCascade;
    this->matMultKernel(inInterface, outInterface);
};
}
}
}
}
}
