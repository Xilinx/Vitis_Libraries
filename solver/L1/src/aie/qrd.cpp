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
/*
qrd kernel code.
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
#include "kernel_api_utils.hpp"
#include "qrd.hpp"
#include "qrd_utils.hpp"

using namespace ::xf::dsp::aie;

// #include "qrd_traits.hpp"
// #include "qrd_utils.hpp"

namespace xf {
namespace solver {
namespace aie {
namespace qrd {

template <typename TT_DATA,
          unsigned int TP_DIM_ROWS,
          unsigned int TP_DIM_COLS,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_CASC_LEN,
          unsigned int TP_DIM_COLS_DIST,
          unsigned int TP_DIM_COLS_TOTAL,
          bool TP_CASC_IN,
          bool TP_CASC_OUT>
INLINE_DECL void qrd_kernel<TT_DATA,
                            TP_DIM_ROWS,
                            TP_DIM_COLS,
                            TP_NUM_FRAMES,
                            TP_CASC_LEN,
                            TP_DIM_COLS_DIST,
                            TP_DIM_COLS_TOTAL,
                            TP_CASC_IN,
                            TP_CASC_OUT>::qrd_mgs(T_inputIF<TP_CASC_IN, TT_DATA>& inInterface,
                                                  T_outputIF<TP_CASC_OUT, TT_DATA>& outInterface,
                                                  int& frame_id) {
    using acc_t = accType_t<TT_DATA>;
    using vect_t = ::aie::vector<TT_DATA, vecSampleNum>;
    using acc_vect_t = ::aie::accum<acc_t, vecSampleNum>;

    acc_vect_t acc_proj, acc_q, acc_norm;
    // Second accumulators for the split-accumulator (N=2) float fast-path; see
    // the projection/norm reductions below. Unused on the cfloat / small-trip path.
    acc_vect_t acc_proj1, acc_norm1;

    vect_t* inAPtr = (vect_t*)inInterface.inWindowA.data();
    vect_t* outQPtr = (vect_t*)outInterface.outWindowQ.data();
    TT_DATA* outRPtr_s = (TT_DATA*)outInterface.outWindowR.data();
    vect_t* VPtr = VPtr = (vect_t*)inInterface.inWindowA.data();

    vect_t a_vect, v_vect, v_vect_conj, q_vect_pre, q_vect_pre_casc, q_vect;
    vect_t project_vector;

    TT_DATA norm_val, norm_val_inv;
    TT_DATA dot_product_val;

    if
        constexpr(TP_CASC_IN == CASC_IN_TRUE) {
            VPtr = (vect_t*)outInterface.outWindowQ.data() +
                   (m_kRowChunkNum * TP_DIM_COLS *
                    frame_id); // v has calculated values from the qrd_casc_msg function, keep projecting on tp of it
        }
    else if
        constexpr(TP_CASC_IN == CASC_IN_FALSE) {
            VPtr = (vect_t*)inInterface.inWindowA.data() +
                   (m_kRowChunkNum * TP_DIM_COLS * frame_id); // v needs to be calculated from the a input, to save
                                                              // copying of the input read, use the input buffer and
                                                              // keep saving on its location.
        }

    outRPtr_s = (TT_DATA*)outInterface.outWindowR.data() + (TP_DIM_COLS * TP_DIM_COLS_TOTAL * frame_id) +
                TP_DIM_COLS_DIST; // norm value for the first vector
    outQPtr = (vect_t*)outInterface.outWindowQ.data() + (m_kRowChunkNum * TP_DIM_COLS * frame_id);
    inAPtr = VPtr; // MGS: projections must read the updated v (same buffer as VPtr), not the original A

    for (int vect = 0; vect < TP_DIM_COLS; vect++) chess_prepare_for_pipelining chess_loop_range(TP_DIM_COLS, ) {
            for (int vect_pre = 0; vect_pre < vect; vect_pre++) {
                // Projection dot product q_prev . a. Two reduction paths:
                //   float, m_kRowChunkNum>=4: split-accumulator (N=2) breaks the single
                //     fpmac feedback recurrence so the compiler can hide MAC latency,
                //     lowering the effective II. Pointers still advance m_kRowChunkNum total.
                //   cfloat or small trip count: original single-accumulator path (op_conj,
                //     no code growth — keeps PM bounded on cascaded cfloat configs).
                if
                    constexpr(XF_SOLVER_QRD_USE_AIE_ML_REDUCTION_FAST_PATH && !isComplex<TT_DATA>() &&
                              m_kRowChunkNum >= 4) {
                        acc_proj = ::aie::mul(*outQPtr++, *inAPtr++);
                        acc_proj1 = ::aie::mul(*outQPtr++, *inAPtr++);
                        int row_chunk = 2;
                        for (; row_chunk + 1 < m_kRowChunkNum; row_chunk += 2) chess_prepare_for_pipelining {
                                acc_proj = ::aie::mac(acc_proj, *outQPtr++, *inAPtr++);
                                acc_proj1 = ::aie::mac(acc_proj1, *outQPtr++, *inAPtr++);
                            }
                        for (; row_chunk < m_kRowChunkNum; row_chunk++) { // scalar remainder (odd count)
                            acc_proj = ::aie::mac(acc_proj, *outQPtr++, *inAPtr++);
                        }
                        auto pv0 = acc_proj.template to_vector<TT_DATA>(0);
                        auto pv1 = acc_proj1.template to_vector<TT_DATA>(0);
                        dot_product_val = ::aie::reduce_add(::aie::add(pv0, pv1));
                    }
                else {
                    // init acc_proj
                    if
                        constexpr(isComplex<TT_DATA>()) {
                            acc_proj = ::aie::mul(::aie::op_conj(*outQPtr++), *inAPtr++);
                        }
                    else {
                        acc_proj = ::aie::mul(*outQPtr++, *inAPtr++);
                    }

                    // rest of the loop
                    for (int row_chunk = 1; row_chunk < m_kRowChunkNum; row_chunk++)
                        chess_prepare_for_pipelining chess_loop_range(m_kRowChunkNum - 1, ) {
                            if
                                constexpr(isComplex<TT_DATA>()) {
                                    acc_proj = ::aie::mac(acc_proj, ::aie::op_conj(*outQPtr++), *inAPtr++);
                                }
                            else {
                                acc_proj = ::aie::mac(acc_proj, *outQPtr++, *inAPtr++);
                            }
                        }
                    dot_product_val = ::aie::reduce_add(
                        acc_proj.template to_vector<TT_DATA>(0)); // reduce the accumulated value to a scalar value
                }
                *outRPtr_s++ = dot_product_val;     // use the scalar pointer to save the dot product value
                inAPtr = inAPtr - m_kRowChunkNum;   // reset the input pointer to the start of the A vector
                outQPtr = outQPtr - m_kRowChunkNum; // reset the output pointer to the start of the Q vector

                for (int row_chunk = 0; row_chunk < m_kRowChunkNum; row_chunk++)
                    chess_prepare_for_pipelining chess_loop_range(m_kRowChunkNum, ) {
                        project_vector = mul(dot_product_val, *outQPtr++);
                        v_vect = sub(*VPtr, project_vector);
                        *VPtr++ = v_vect; // save the v-vector to the input buffer
                    }
                VPtr = VPtr - m_kRowChunkNum;
            } // end of vect_pre

            // Column norm-squared sum(v.v). Split-accumulator (N=2) on the float path
            // for the same recurrence-breaking reason as the projection above; cfloat /
            // small trip counts keep the original single-accumulator path.
            if
                constexpr(XF_SOLVER_QRD_USE_AIE_ML_REDUCTION_FAST_PATH && !isComplex<TT_DATA>() &&
                          m_kRowChunkNum >= 4) {
                    vect_t n0 = *VPtr++;
                    vect_t n1 = *VPtr++;
                    acc_norm = ::aie::mul(n0, n0);
                    acc_norm1 = ::aie::mul(n1, n1);
                    int row_chunk = 2;
                    for (; row_chunk + 1 < m_kRowChunkNum; row_chunk += 2) chess_prepare_for_pipelining {
                            n0 = *VPtr++;
                            n1 = *VPtr++;
                            acc_norm = ::aie::mac(acc_norm, n0, n0);
                            acc_norm1 = ::aie::mac(acc_norm1, n1, n1);
                        }
                    for (; row_chunk < m_kRowChunkNum; row_chunk++) { // scalar remainder (odd count)
                        n0 = *VPtr++;
                        acc_norm = ::aie::mac(acc_norm, n0, n0);
                    }
                    auto nv0 = acc_norm.template to_vector<TT_DATA>(0);
                    auto nv1 = acc_norm1.template to_vector<TT_DATA>(0);
                    dot_product_val = ::aie::reduce_add(::aie::add(nv0, nv1));
                }
            else {
                // init acc_norm
                v_vect = *VPtr++;
                if
                    constexpr(isComplex<TT_DATA>()) { acc_norm = ::aie::mul(::aie::op_conj(v_vect), v_vect); }
                else {
                    acc_norm = ::aie::mul(v_vect, v_vect);
                }

                // rest of the loop
                for (int row_chunk = 1; row_chunk < m_kRowChunkNum; row_chunk++)
                    chess_prepare_for_pipelining chess_loop_range(m_kRowChunkNum - 1, ) {
                        v_vect = *VPtr++;
                        if
                            constexpr(isComplex<TT_DATA>()) {
                                acc_norm = ::aie::mac(acc_norm, ::aie::op_conj(v_vect), v_vect);
                            }
                        else {
                            acc_norm = ::aie::mac(acc_norm, v_vect, v_vect);
                        }
                    }
                dot_product_val = ::aie::reduce_add(
                    acc_norm.template to_vector<TT_DATA>(0)); // reduce the accumulated value to a scalar
            }
            VPtr = VPtr - m_kRowChunkNum;
            // The single-invsqrt normalization is faster on AIE-ML; retain the
            // separate sqrt/inverse sequence on AIE1 and AIE-MLv2.
            if
                constexpr(XF_SOLVER_QRD_USE_AIE_ML_REDUCTION_FAST_PATH) {
                    calc_norm_and_inv(dot_product_val, norm_val, norm_val_inv);
                }
            else {
                calc_sqrt(dot_product_val, norm_val);
                calc_inv(norm_val, norm_val_inv);
            }
            *outRPtr_s = norm_val; // we are at the diagonal element of the R matrix

            for (int row_chunk = 0; row_chunk < m_kRowChunkNum; row_chunk++)
                chess_prepare_for_pipelining chess_loop_range(m_kRowChunkNum, ) {
                    acc_q = ::aie::mul<acc_t>(*VPtr++, norm_val_inv);
                    q_vect = acc_q.template to_vector<TT_DATA>(0);
                    *outQPtr++ = q_vect;

                    if
                        constexpr(TP_CASC_OUT == CASC_OUT_TRUE) {
                            writeincr(outInterface.outCascade, q_vect); // send out the Q results
                        }
                }
            inAPtr = inAPtr + m_kRowChunkNum;                   // move to the next column of A
            outRPtr_s = outRPtr_s + (TP_DIM_COLS_TOTAL - vect); // go to the next R location
            outQPtr =
                (vect_t*)outInterface.outWindowQ.data() +
                (m_kRowChunkNum * TP_DIM_COLS * frame_id); // reset the output pointer to the start of the Q vector
        }                                                  // end of vect
};

template <typename TT_DATA,
          unsigned int TP_DIM_ROWS,
          unsigned int TP_DIM_COLS,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_CASC_LEN,
          unsigned int TP_DIM_COLS_DIST,
          unsigned int TP_DIM_COLS_TOTAL,
          bool TP_CASC_IN,
          bool TP_CASC_OUT>
INLINE_DECL void qrd_kernel<TT_DATA,
                            TP_DIM_ROWS,
                            TP_DIM_COLS,
                            TP_NUM_FRAMES,
                            TP_CASC_LEN,
                            TP_DIM_COLS_DIST,
                            TP_DIM_COLS_TOTAL,
                            TP_CASC_IN,
                            TP_CASC_OUT>::qrd_mgs_casc(T_inputIF<TP_CASC_IN, TT_DATA>& inInterface,
                                                       T_outputIF<TP_CASC_OUT, TT_DATA>& outInterface) {
    using acc_t = accType_t<TT_DATA>;
    using vect_t = ::aie::vector<TT_DATA, vecSampleNum>;
    using acc_vect_t = ::aie::accum<acc_t, vecSampleNum>;

    acc_vect_t acc_proj;
    acc_vect_t acc_proj1; // second accumulator for the split-accumulator float fast-path
    vect_t q_vect_prev, q_vect_read;

    vect_t* inAPtr = (vect_t*)inInterface.inWindowA.data();
    vect_t* outQPtr = (vect_t*)outInterface.outWindowQ.data();
    vect_t* outRPtr_v = (vect_t*)outInterface.outWindowR.data();
    TT_DATA* outRPtr_s = (TT_DATA*)outInterface.outWindowR.data();
    vect_t* QrdCascPtr = (vect_t*)&QrdCascData[0];

    vect_t a_vect, v_vect_intermediate, q_vect_prev_casc;
    vect_t blankVect = ::aie::zeros<TT_DATA, vecSampleNum>(); // to initialise acc
    TT_DATA dot_product_val;

    // fill output buffer with zeros
    for (int vect = 0; vect < TP_DIM_COLS * TP_NUM_FRAMES; vect++) {
        for (int col_chunk = 0; col_chunk < m_kColChunkNum; col_chunk++) {
            *outRPtr_v++ = blankVect;
        }
    }

    for (int frame = 0; frame < TP_NUM_FRAMES; frame++) chess_prepare_for_pipelining chess_loop_range(TP_NUM_FRAMES, ) {
            inAPtr = (vect_t*)inInterface.inWindowA.data() + (m_kRowChunkNum * TP_DIM_COLS * frame);
            outQPtr = (vect_t*)outInterface.outWindowQ.data() + (m_kRowChunkNum * TP_DIM_COLS * frame);
            outRPtr_s = (TT_DATA*)outInterface.outWindowR.data() + (TP_DIM_COLS * TP_DIM_COLS_TOTAL * frame);

            for (int vect_prev = 0; vect_prev < TP_DIM_COLS_DIST; vect_prev++)
                chess_prepare_for_pipelining chess_loop_range(TP_DIM_COLS_DIST, ) {
                    for (int row_chunk = 0; row_chunk < m_kRowChunkNum; row_chunk++)
                        chess_prepare_for_pipelining chess_loop_range(m_kRowChunkNum, ) {
                            q_vect_read = readincr_v<vecSampleNum>(
                                inInterface.inCascade); // read the incoming Q vector from the cascade
                            *QrdCascPtr++ = q_vect_read;
                            if
                                constexpr(TP_CASC_OUT == CASC_OUT_TRUE) { // bypass for the next kernel operation
                                    writeincr(outInterface.outCascade, q_vect_read);
                                }
                        }
                    QrdCascPtr = (vect_t*)&QrdCascData[0]; // reset the pointer to the start of the QrdCascData buffer

                    for (int vect = 0; vect < TP_DIM_COLS; vect++)
                        chess_prepare_for_pipelining chess_loop_range(TP_DIM_COLS, ) {
                            // MGS: project onto the partially-updated v (held in the Q buffer after the
                            // first incoming q), not the original A column, which would be Classical GS.
                            vect_t* dotPtr = (vect_prev == 0) ? inAPtr : outQPtr;
                            // Projection dot product (pure-local: reads QrdCasc cache + v, no stream
                            // ops inside). Split-accumulator (N=2) on the float path breaks the fpmac
                            // feedback recurrence; cfloat / small trip count keep the original path.
                            if
                                constexpr(XF_SOLVER_QRD_USE_AIE_ML_REDUCTION_FAST_PATH && !isComplex<TT_DATA>() &&
                                          m_kRowChunkNum >= 4) {
                                    acc_proj = ::aie::mul(*QrdCascPtr++, *dotPtr++);
                                    acc_proj1 = ::aie::mul(*QrdCascPtr++, *dotPtr++);
                                    int row_chunk = 2;
                                    for (; row_chunk + 1 < m_kRowChunkNum; row_chunk += 2)
                                        chess_prepare_for_pipelining {
                                            acc_proj = ::aie::mac(acc_proj, *QrdCascPtr++, *dotPtr++);
                                            acc_proj1 = ::aie::mac(acc_proj1, *QrdCascPtr++, *dotPtr++);
                                        }
                                    for (; row_chunk < m_kRowChunkNum; row_chunk++) { // scalar remainder (odd count)
                                        acc_proj = ::aie::mac(acc_proj, *QrdCascPtr++, *dotPtr++);
                                    }
                                    auto pv0 = acc_proj.template to_vector<TT_DATA>(0);
                                    auto pv1 = acc_proj1.template to_vector<TT_DATA>(0);
                                    dot_product_val = ::aie::reduce_add(::aie::add(pv0, pv1));
                                }
                            else {
                                // init acc_proj
                                if
                                    constexpr(isComplex<TT_DATA>()) {
                                        acc_proj = ::aie::mul(::aie::op_conj(*QrdCascPtr++), *dotPtr++);
                                    }
                                else {
                                    acc_proj = ::aie::mul(*QrdCascPtr++, *dotPtr++);
                                }

                                // rest of the loop
                                for (int row_chunk = 1; row_chunk < m_kRowChunkNum; row_chunk++)
                                    chess_prepare_for_pipelining chess_loop_range(m_kRowChunkNum - 1, ) {
                                        if
                                            constexpr(isComplex<TT_DATA>()) {
                                                acc_proj =
                                                    ::aie::mac(acc_proj, ::aie::op_conj(*QrdCascPtr++), *dotPtr++);
                                            }
                                        else {
                                            acc_proj = ::aie::mac(acc_proj, *QrdCascPtr++, *dotPtr++);
                                        }
                                    }
                                dot_product_val = ::aie::reduce_add(acc_proj.template to_vector<TT_DATA>(
                                    0)); // reduce the accumulated value to a scalar value
                            }
                            QrdCascPtr =
                                (vect_t*)&QrdCascData[0]; // reset the pointer to the start of the QrdCascData buffer

                            for (int row_chunk = 0; row_chunk < m_kRowChunkNum; row_chunk++)
                                chess_prepare_for_pipelining chess_loop_range(m_kRowChunkNum, ) {
                                    v_vect_intermediate = (vect_prev == 0) ? *inAPtr++ : *outQPtr;
                                    vect_t project_vector = ::aie::mul(dot_product_val, *QrdCascPtr++);
                                    v_vect_intermediate = ::aie::sub(v_vect_intermediate, project_vector);
                                    *outQPtr++ = v_vect_intermediate; // save the v-vector to the Q buffer
                                }
                            QrdCascPtr =
                                (vect_t*)&QrdCascData[0]; // reset the pointer to the start of the QrdCascData buffer
                            *outRPtr_s = dot_product_val; // use the scalar pointer to save the dot product value
                            outRPtr_s = outRPtr_s + TP_DIM_COLS_TOTAL; // go to the R location of the next column

                        } // end of vect
                    inAPtr = (vect_t*)inInterface.inWindowA.data() + (m_kRowChunkNum * TP_DIM_COLS * frame);
                    outQPtr = (vect_t*)outInterface.outWindowQ.data() + (m_kRowChunkNum * TP_DIM_COLS * frame);
                    outRPtr_s = (TT_DATA*)outInterface.outWindowR.data() + (TP_DIM_COLS * TP_DIM_COLS_TOTAL * frame) +
                                (vect_prev + 1);

                } // end of vect_prev
            qrd_mgs(inInterface, outInterface,
                    frame); // process the rest of the vectors using the traditional QRD algorithm
        }                   // end of frames
}

template <typename TT_DATA,
          unsigned int TP_DIM_ROWS,
          unsigned int TP_DIM_COLS,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_CASC_LEN,
          unsigned int TP_DIM_COLS_DIST,
          unsigned int TP_DIM_COLS_TOTAL,
          bool TP_CASC_IN,
          bool TP_CASC_OUT>
INLINE_DECL void qrd_kernel<TT_DATA,
                            TP_DIM_ROWS,
                            TP_DIM_COLS,
                            TP_NUM_FRAMES,
                            TP_CASC_LEN,
                            TP_DIM_COLS_DIST,
                            TP_DIM_COLS_TOTAL,
                            TP_CASC_IN,
                            TP_CASC_OUT>::qrd_mgs_first_kernel(T_inputIF<TP_CASC_IN, TT_DATA>& inInterface,
                                                               T_outputIF<TP_CASC_OUT, TT_DATA>& outInterface) {
    using vect_t = ::aie::vector<TT_DATA, vecSampleNum>;
    vect_t* outRPtr_v = (vect_t*)outInterface.outWindowR.data();
    vect_t blankVect = ::aie::zeros<TT_DATA, vecSampleNum>(); // to initialise acc

    // fill output buffer with zeros
    for (int vect = 0; vect < TP_DIM_COLS * TP_NUM_FRAMES; vect++) {
        for (int col_chunk = 0; col_chunk < m_kColChunkNum; col_chunk++) {
            *outRPtr_v++ = blankVect;
        }
    }

    for (int frame = 0; frame < TP_NUM_FRAMES; frame++) chess_prepare_for_pipelining chess_loop_range(TP_NUM_FRAMES, ) {
            qrd_mgs(inInterface, outInterface, frame); // process the input using the QRD function
        }
}

// Base specialization, used for static size window API configurations
template <typename TT_DATA,
          unsigned int TP_DIM_ROWS,
          unsigned int TP_DIM_COLS,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_CASC_LEN,
          unsigned int TP_DIM_COLS_DIST,
          unsigned int TP_DIM_COLS_TOTAL,
          bool TP_CASC_IN,
          bool TP_CASC_OUT>
NOINLINE_DECL void qrd<TT_DATA,
                       TP_DIM_ROWS,
                       TP_DIM_COLS,
                       TP_NUM_FRAMES,
                       TP_CASC_LEN,
                       TP_DIM_COLS_DIST,
                       TP_DIM_COLS_TOTAL,
                       TP_CASC_IN,
                       TP_CASC_OUT>::qrd_main(input_buffer<TT_DATA>& __restrict inWindowA,
                                              output_buffer<TT_DATA>& __restrict outWindowQ,
                                              output_buffer<TT_DATA>& __restrict outWindowR) {
    T_inputIF<TP_CASC_IN, TT_DATA> inInterface(inWindowA);
    T_outputIF<TP_CASC_OUT, TT_DATA> outInterface(outWindowQ, outWindowR);
    this->qrd_mgs_first_kernel(inInterface, outInterface);
}

template <typename TT_DATA,
          unsigned int TP_DIM_ROWS,
          unsigned int TP_DIM_COLS,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_CASC_LEN,
          unsigned int TP_DIM_COLS_DIST,
          unsigned int TP_DIM_COLS_TOTAL>
NOINLINE_DECL void qrd<TT_DATA,
                       TP_DIM_ROWS,
                       TP_DIM_COLS,
                       TP_NUM_FRAMES,
                       TP_CASC_LEN,
                       TP_DIM_COLS_DIST,
                       TP_DIM_COLS_TOTAL,
                       CASC_IN_TRUE,
                       CASC_OUT_TRUE>::qrd_main(input_buffer<TT_DATA>& __restrict inWindowA,
                                                input_cascade<TT_DATA>* __restrict inCascade,
                                                output_buffer<TT_DATA>& __restrict outWindowQ,
                                                output_buffer<TT_DATA>& __restrict outWindowR,
                                                output_cascade<TT_DATA>* __restrict outCascade) {
    T_inputIF<CASC_IN_TRUE, TT_DATA> inInterface(inWindowA);
    inInterface.inCascade = inCascade;
    T_outputIF<CASC_OUT_TRUE, TT_DATA> outInterface(outWindowQ, outWindowR);
    outInterface.outCascade = outCascade;
    this->qrd_mgs_casc(inInterface, outInterface);
};

template <typename TT_DATA,
          unsigned int TP_DIM_ROWS,
          unsigned int TP_DIM_COLS,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_CASC_LEN,
          unsigned int TP_DIM_COLS_DIST,
          unsigned int TP_DIM_COLS_TOTAL>
NOINLINE_DECL void qrd<TT_DATA,
                       TP_DIM_ROWS,
                       TP_DIM_COLS,
                       TP_NUM_FRAMES,
                       TP_CASC_LEN,
                       TP_DIM_COLS_DIST,
                       TP_DIM_COLS_TOTAL,
                       CASC_IN_TRUE,
                       CASC_OUT_FALSE>::
    qrd_main(

        input_buffer<TT_DATA>& __restrict inWindowA,
        input_cascade<TT_DATA>* __restrict inCascade,
        output_buffer<TT_DATA>& __restrict outWindowQ,
        output_buffer<TT_DATA>& __restrict outWindowR) {
    T_inputIF<CASC_IN_TRUE, TT_DATA> inInterface(inWindowA);
    inInterface.inCascade = inCascade;
    T_outputIF<CASC_OUT_FALSE, TT_DATA> outInterface(outWindowQ, outWindowR);
    this->qrd_mgs_casc(inInterface, outInterface);
};

template <typename TT_DATA,
          unsigned int TP_DIM_ROWS,
          unsigned int TP_DIM_COLS,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_CASC_LEN,
          unsigned int TP_DIM_COLS_DIST,
          unsigned int TP_DIM_COLS_TOTAL>
NOINLINE_DECL void qrd<TT_DATA,
                       TP_DIM_ROWS,
                       TP_DIM_COLS,
                       TP_NUM_FRAMES,
                       TP_CASC_LEN,
                       TP_DIM_COLS_DIST,
                       TP_DIM_COLS_TOTAL,
                       CASC_IN_FALSE,
                       CASC_OUT_TRUE>::qrd_main(input_buffer<TT_DATA>& __restrict inWindowA,
                                                output_buffer<TT_DATA>& __restrict outWindowQ,
                                                output_buffer<TT_DATA>& __restrict outWindowR,
                                                output_cascade<TT_DATA>* __restrict outCascade) {
    T_inputIF<CASC_IN_FALSE, TT_DATA> inInterface(inWindowA);
    T_outputIF<CASC_OUT_TRUE, TT_DATA> outInterface(outWindowQ, outWindowR);
    outInterface.outCascade = outCascade;
    this->qrd_mgs_first_kernel(inInterface, outInterface);
};
}
}
}
}
