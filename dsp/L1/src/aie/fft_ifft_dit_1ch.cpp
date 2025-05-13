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
FFT/IFFT DIT single channel kernal code.
This file captures the body of run-time code for the kernal class.

Coding conventions
  TT_      template type suffix
  TP_      template parameter suffix
*/

//#include <adf.h>
#include <stdio.h>

using namespace std;

#include "device_defs.h"

// if we use 1kb registers -> aie api uses 2x512b registers for 1024b so we need this for QoR
#define __AIE_API_USE_NATIVE_1024B_VECTOR__
#include "aie_api/aie_adf.hpp"

#include "fft_com_inc.h"
#include "fft_ifft_dit_1ch.hpp"
#include "kernel_api_utils.hpp"
#include "fft_ifft_dit_1ch_utils.hpp"
#include "fft_kernel_bufs.h"
//#include "fft_twiddle_lut_dit.h"
//#include "fft_twiddle_lut_dit_cfloat.h"
#include "fft_twiddle_lut_dit_all.h"
//#include "fft_r4_twiddles.h"
//#include "fft_r4_twiddles_cint32.h"
#include "fft_r4_twiddles_all.h"

//#define _DSPLIB_FFT_IFFT_DIT_1CH_HPP_DEBUG_

namespace xf {
namespace dsp {
namespace aie {
namespace fft {
namespace dit_1ch {

// Stockham stages kernel common - this function is defered to the stockham object of the kernel class because it
// requires no specialization
// whereas the kernel class itself does, so this is just a way to avoid code duplication
template <typename TT_DATA,
          typename TT_OUT_DATA,
          typename TT_TWIDDLE,
          typename TT_INTERNAL_DATA,
          unsigned int TP_POINT_SIZE,
          unsigned int TP_FFT_NIFFT,
          unsigned int TP_SHIFT,
          unsigned int TP_START_RANK,
          unsigned int TP_END_RANK,
          unsigned int TP_DYN_PT_SIZE,
          unsigned int TP_WINDOW_VSIZE,
          unsigned int TP_ORIG_PAR_POWER,
          unsigned int TP_TWIDDLE_MODE>
INLINE_DECL void stockhamStages<TT_DATA,
                                TT_OUT_DATA,
                                TT_TWIDDLE,
                                TT_INTERNAL_DATA,
                                TP_POINT_SIZE,
                                TP_FFT_NIFFT,
                                TP_SHIFT,
                                TP_START_RANK,
                                TP_END_RANK,
                                TP_DYN_PT_SIZE,
                                TP_WINDOW_VSIZE,
                                TP_ORIG_PAR_POWER,
                                TP_TWIDDLE_MODE>::stagePreamble(TT_TWIDDLE** tw_table,
                                                                TT_INTERNAL_DATA* tmp1_buf,
                                                                TT_INTERNAL_DATA* tmp2_buf,
                                                                TT_DATA* __restrict inptr,
                                                                TT_OUT_DATA* __restrict outptr) {
    typedef typename std::conditional<std::is_same<TT_DATA, cint16>::value, cint32_t, TT_DATA>::type T_internalDataType;

#if __FFT_R4_IMPL__ == 0
    // TODO - minPtSizePwr = 4, temporarily set to 5 for debug
    // constexpr int minPtSizePwr = (TP_DYN_PT_SIZE == 1) && (std::is_same<TT_TWIDDLE, cint32>::value)? 5:4;
    constexpr int minPtSizePwr = 4;
#endif //__FFT_R4_IMPL__ == 0
#if __FFT_R4_IMPL__ == 1
#if __ALIGN_BYTE_SIZE__ == 32
    constexpr int minPtSizePwr = 5;
#else
    constexpr int minPtSizePwr = 6;
#endif //__ALIGN_BYTE_SIZE__
#endif //__FFT_R4_IMPL__ == 1
    constexpr int kOpsInWindow = TP_WINDOW_VSIZE / TP_POINT_SIZE;

    TT_DATA* xbuff = inptr;
    TT_OUT_DATA* obuff = outptr;
    bool inv;
    if
        constexpr(TP_FFT_NIFFT == 1) { inv = false; }
    else {
        inv = true;
    }

    if
        constexpr(TP_DYN_PT_SIZE == 1) {
            constexpr int kHeaderBytes = __ALIGN_BYTE_SIZE__; // 256 bits for AIE and AIE-ML, 512 for AIE-MLv2.
            constexpr int kSamplesInHeader = kHeaderBytes / sizeof(TT_DATA);
            constexpr int kSamplesOutHeader = kHeaderBytes / sizeof(TT_OUT_DATA);
            TT_INTERNAL_DATA* myTmp2_buf = (TT_INTERNAL_DATA*)tmp2_buf; // when dynamic, xbuff header alters start of
                                                                        // data and tmp2_buf is a pointer to xbuff so
                                                                        // has to alter too.
            TT_DATA headerVal;
            TT_OUT_DATA headerOpVal;
            using inHeaderVectorType = ::aie::vector<TT_DATA, kSamplesInHeader>;
            using outVectorType =
                ::aie::vector<TT_OUT_DATA, kSamplesInHeader>; // has to be same number of elements as input.
            using outHeaderVectorType =
                ::aie::vector<TT_OUT_DATA, kSamplesOutHeader>; // has to be same number of elements as input.
            inHeaderVectorType header;
            outHeaderVectorType headerOp;
            outHeaderVectorType blankOp;
            inHeaderVectorType* inPtr;
            outVectorType* outputPtr;
            inHeaderVectorType inHeader;
            outHeaderVectorType* outHeaderPtr;
            outVectorType outVector;
            int ptSizePwr;
            unsigned int ptSize;
            int firstRank; // first for this particular point size

            ::aie::accum<cacc48, kSamplesInHeader> cacc384;

            blankOp = ::aie::zeros<TT_OUT_DATA, kSamplesOutHeader>();
            headerOp = ::aie::zeros<TT_OUT_DATA, kSamplesOutHeader>();
            xbuff = (TT_DATA*)inptr;
            inPtr = (inHeaderVectorType*)xbuff;
            header = *inPtr;
            xbuff += kHeaderBytes / sizeof(TT_DATA); // increment by header size bits.
            if
                constexpr(!fnUsePingPongIntBuffer<TT_DATA>()) myTmp2_buf = (TT_INTERNAL_DATA*)xbuff;
            else
                myTmp2_buf = (TT_INTERNAL_DATA*)tmp2_buf;
            headerVal = header.get(0);
            headerOpVal.real = headerVal.real; // copy/cast header to output one field at a time
            headerOpVal.imag = headerVal.imag;
            headerOp.set(headerOpVal, 0);
            inv = headerVal.real == 0 ? true : false;
            headerVal = header.get(1);
            headerOpVal.real = headerVal.real;
            headerOpVal.imag = headerVal.imag;
            headerOp.set(headerOpVal, 1);
            ptSizePwr = (int)headerVal.real - TP_ORIG_PAR_POWER;
            ptSize = 1 << ptSizePwr;
            // firstRank is explained in a large comment where firstRank is defined in stockhamStages::calc (static
            // variant)
            if
                constexpr(std::is_same<TT_DATA, cfloat>::value) { firstRank = kPointSizePower - ptSizePwr; }
            else {
                firstRank = kPointSizePowerCeiled - ptSizePwr;
            }
            // obuff = outptr;
            if ((ptSizePwr >= minPtSizePwr) && (ptSizePwr <= kPointSizePower)) {
                outHeaderPtr = (outHeaderVectorType*)obuff;
                *outHeaderPtr = headerOp;
                obuff += kHeaderBytes / sizeof(TT_OUT_DATA);
                if (TP_END_RANK <=
                    firstRank) { // i.e. no need to do any processing in this kernel as later kernels will do it all.
                    // copy input window to output window in Headerbit chunks
                    inPtr = (inHeaderVectorType*)xbuff;
                    outputPtr = (outVectorType*)obuff;
                    if
                        constexpr(std::is_same<TT_DATA, cfloat>::value) {
                            for (int i = 0; i < TP_WINDOW_VSIZE / kSamplesInHeader; i++) {
                                *outputPtr++ = *inPtr++;
                            }
                        }
                    else {
                        // this clause handles the case with different TT_IN_DATA and TT_OUT_DATA
                        for (int i = 0; i < TP_WINDOW_VSIZE / kSamplesInHeader; i++) {
                            inHeader = *inPtr++;
                            cacc384.from_vector(inHeader, 0);
                            outVector = cacc384.template to_vector<TT_OUT_DATA>(0);
                            *outputPtr++ = outVector;
                        }
                    }

                } else {
                    for (int iter = 0; iter < kOpsInWindow; iter++) {
                        if
                            constexpr(!fnUsePingPongIntBuffer<TT_DATA>()) myTmp2_buf = (TT_INTERNAL_DATA*)xbuff;
                        else
                            myTmp2_buf = (TT_INTERNAL_DATA*)tmp2_buf;
                        calc(xbuff, (TT_TWIDDLE**)tw_table, (T_internalDataType*)tmp1_buf,
                             (T_internalDataType*)myTmp2_buf, obuff, ptSizePwr, inv); // dynamic variant of calc
                        obuff += TP_POINT_SIZE;
                        xbuff += TP_POINT_SIZE;
                    }
                }
            } else { // illegal point size, so flag error in status field and fill output window with zeros.
                headerOpVal = unitVector<TT_OUT_DATA>();          // set real to 1, imag to 0, 1 meaning invalid.
                headerOp.set(headerOpVal, kSamplesOutHeader - 1); // set status in the last sample of the header
                outHeaderPtr = (outHeaderVectorType*)obuff;
                *outHeaderPtr = headerOp;
                obuff += kHeaderBytes / sizeof(TT_OUT_DATA);

                // blank output window beyond header.
                outHeaderVectorType* blankPtr;
                blankPtr = (outHeaderVectorType*)obuff;
                for (int i = 0; i < TP_WINDOW_VSIZE / kSamplesOutHeader; i++) {
                    *blankPtr++ = blankOp;
                }
            }
        }
    else { // else for TP_DYN_PT_SIZE == 1
        for (int iter = 0; iter < kOpsInWindow; iter++) chess_prepare_for_pipelining chess_loop_range(kOpsInWindow, ) {
                calc(xbuff, (TT_TWIDDLE**)tw_table, (T_internalDataType*)tmp1_buf, (T_internalDataType*)tmp2_buf,
                     obuff);
                obuff += TP_POINT_SIZE;
                xbuff += TP_POINT_SIZE;
            }
    }
};
// stockhamStages calc body. Static TP_POINT_SIZE variant
// This is a helper class which allows the many variants of the kernel class to call a single body of code for the fft
// stages.
template <typename TT_DATA,
          typename TT_OUT_DATA,
          typename TT_TWIDDLE,
          typename TT_INTERNAL_DATA,
          unsigned int TP_POINT_SIZE,
          unsigned int TP_FFT_NIFFT,
          unsigned int TP_SHIFT,
          unsigned int TP_START_RANK,
          unsigned int TP_END_RANK,
          unsigned int TP_DYN_PT_SIZE,
          unsigned int TP_WINDOW_VSIZE,
          unsigned int TP_ORIG_PAR_POWER, // irrelevant to static specialization
          unsigned int TP_TWIDDLE_MODE>
INLINE_DECL void stockhamStages<TT_DATA,
                                TT_OUT_DATA,
                                TT_TWIDDLE,
                                TT_INTERNAL_DATA,
                                TP_POINT_SIZE,
                                TP_FFT_NIFFT,
                                TP_SHIFT,
                                TP_START_RANK,
                                TP_END_RANK,
                                TP_DYN_PT_SIZE,
                                TP_WINDOW_VSIZE,
                                TP_ORIG_PAR_POWER,
                                TP_TWIDDLE_MODE>::calc(TT_DATA* __restrict xbuff,
                                                       TT_TWIDDLE** tw_table,
                                                       TT_INTERNAL_DATA* tmp1_buf,
                                                       TT_INTERNAL_DATA* tmp2_buf,
                                                       TT_OUT_DATA* __restrict obuff) {
    constexpr int kPointSizePower = fnPointSizePower<TP_POINT_SIZE>();
    bool inv = TP_FFT_NIFFT == 1 ? false : true;

    // Note on internal buffers.
    // The FFT is calculated over a series of stages of radix 2 or 4. If a kernel calculates N stages, then a basic
    // implementation would be to read
    // the input buffer and write results to an internal buffer A. Then, for the next stage, read from A and write to B
    // and so on.
    // Since the input buffer or A in the description above is free at the end of the stages in question, they are free
    // to be re-used.
    // Therefore, there is never a need for any more than internal buffers A and B because for each stage you would be
    // reading from one and writing
    // to the other. Ultimately the output buffer is written to.
    // Now, with TT_DATA=cint16, the input and output buffers are half the size of the internal buffers because the data
    // type used internally is cint32,
    // so here 2 internal buffers are always required.
    // For TT_DATA = cint32 or cfloat, the input buffer xfbuf can always be used as one of the two internal buffers.
    // Further, for kernels which have an odd number of stages, the output buffer can be used as one of the two internal
    // buffers too.

    // Reuse the output buffer as one of the two scratchpad pingpong buffers?
    TT_INTERNAL_DATA* my_tmp1_buf = fnReuseOutputBuffer<TT_OUT_DATA, TP_START_RANK, TP_END_RANK, TP_DYN_PT_SIZE>()
                                        ? (TT_INTERNAL_DATA*)obuff
                                        : tmp1_buf;

    // Reuse the input buffer as one of the two scratchpad pingpong buffers?
    TT_INTERNAL_DATA* my_tmp2_buf = fnUsePingPongIntBuffer<TT_DATA>() ? tmp2_buf : (TT_INTERNAL_DATA*)xbuff;

    // The kernel will ping pong between two buffers. Which two is resolved above.
    TT_INTERNAL_DATA* tmp_bufs[2] = {my_tmp1_buf,
                                     my_tmp2_buf}; // tmp2 is may be xbuff reused., tmp1 may be obuff reused
    unsigned int pingPong = 1;                     // use my_tmp_buf1 as initial output - i.e. output of first stage
    int tw_index = 0;
    int rank = 0;
    int r = 0; // r is an indication to the stage of rank.

    if
        constexpr(std::is_same<TT_DATA, cfloat>::value) {
            //-----------------------------------------------------------------------------
            // cfloat handling
            TT_DATA* inptr;
            TT_INTERNAL_DATA* outptr;

            if
                constexpr(TP_WINDOW_VSIZE <= 64) chess_memory_fence();
            opt_cfloat_stage<0, TP_POINT_SIZE, TP_START_RANK, TP_END_RANK>(xbuff, obuff, tmp_bufs, tw_table, pingPong,
                                                                           inv);
            if
                constexpr(TP_WINDOW_VSIZE <= 64) chess_memory_fence();
            opt_cfloat_stage<1, TP_POINT_SIZE, TP_START_RANK, TP_END_RANK>(xbuff, obuff, tmp_bufs, tw_table, pingPong,
                                                                           inv);
            if
                constexpr(TP_WINDOW_VSIZE <= 64) chess_memory_fence();
            opt_cfloat_stage<2, TP_POINT_SIZE, TP_START_RANK, TP_END_RANK>(xbuff, obuff, tmp_bufs, tw_table, pingPong,
                                                                           inv);
            if
                constexpr(TP_WINDOW_VSIZE <= 64) chess_memory_fence();
            opt_cfloat_stage<3, TP_POINT_SIZE, TP_START_RANK, TP_END_RANK>(xbuff, obuff, tmp_bufs, tw_table, pingPong,
                                                                           inv);
            if
                constexpr(TP_WINDOW_VSIZE <= 64) chess_memory_fence();
            opt_cfloat_stage<4, TP_POINT_SIZE, TP_START_RANK, TP_END_RANK>(xbuff, obuff, tmp_bufs, tw_table, pingPong,
                                                                           inv);
            if
                constexpr(TP_WINDOW_VSIZE <= 64) chess_memory_fence();
            opt_cfloat_stage<5, TP_POINT_SIZE, TP_START_RANK, TP_END_RANK>(xbuff, obuff, tmp_bufs, tw_table, pingPong,
                                                                           inv);
            if
                constexpr(TP_WINDOW_VSIZE <= 64) chess_memory_fence();
            opt_cfloat_stage<6, TP_POINT_SIZE, TP_START_RANK, TP_END_RANK>(xbuff, obuff, tmp_bufs, tw_table, pingPong,
                                                                           inv);
            if
                constexpr(TP_WINDOW_VSIZE <= 64) chess_memory_fence();
            opt_cfloat_stage<7, TP_POINT_SIZE, TP_START_RANK, TP_END_RANK>(xbuff, obuff, tmp_bufs, tw_table, pingPong,
                                                                           inv);
            if
                constexpr(TP_WINDOW_VSIZE <= 64) chess_memory_fence();
            opt_cfloat_stage<8, TP_POINT_SIZE, TP_START_RANK, TP_END_RANK>(xbuff, obuff, tmp_bufs, tw_table, pingPong,
                                                                           inv);
            if
                constexpr(TP_WINDOW_VSIZE <= 64) chess_memory_fence();
            opt_cfloat_stage<9, TP_POINT_SIZE, TP_START_RANK, TP_END_RANK>(xbuff, obuff, tmp_bufs, tw_table, pingPong,
                                                                           inv);
            if
                constexpr(TP_WINDOW_VSIZE <= 64) chess_memory_fence();
            opt_cfloat_stage<10, TP_POINT_SIZE, TP_START_RANK, TP_END_RANK>(xbuff, obuff, tmp_bufs, tw_table, pingPong,
                                                                            inv);
            if
                constexpr(TP_WINDOW_VSIZE <= 64) chess_memory_fence();
            opt_cfloat_stage<11, TP_POINT_SIZE, TP_START_RANK, TP_END_RANK>(xbuff, obuff, tmp_bufs, tw_table, pingPong,
                                                                            inv);
        }
    else { // integer types can use radix 4 stages

        constexpr int firstRank = kPointSizePowerCeiled - kPointSizePower;
        // The following code is a loop which is unrolled so that the loop variable is constant at compile time in each
        // iteration
        // The loop unrolls to describe the stages required for a maximum point size, rounded up to a multiple of 2 so
        // that each
        // pass through the loop is a radix 4 stage.
        // In each iteration the code considers whether or not to execute a stage, since for small point sizes, early
        // stages
        // with large 'r' factors are skipped.
        // Also, the code has to handle the case that for odd power point sizes the first stage will be radix2, so it
        // has to
        // detect the first stage being one more than the current loop stage.
        // Twiddles are relative to the first stage of the point size, not the first stage of the loop, hence the
        // equation for
        // tw index.
        // Accordingly when the first stage is r2, it will be the second r2 of the radix4 in this iteration, so stage+1
        // is
        // necessary to describe the position.
        // A further complication is that data arrives as TT_DATA, is processed to internal stages as TT_INTERNAL_DATA
        // and
        // output as TT_OUT_DATA.
        // Finally, the whole operation may be split over multiple kernels, which means the input or output of this
        // kernel may
        // be TT_INTERNAL_DATA.
        // beyond finally(!), only stages in the remit of the kernel are executed.

        if
            constexpr(TP_WINDOW_VSIZE <= 64) chess_memory_fence();
        opt_int_stage<TT_DATA, TT_INTERNAL_DATA, TT_OUT_DATA, TT_TWIDDLE, TP_TWIDDLE_MODE, 0, TP_POINT_SIZE, TP_SHIFT,
                      TP_START_RANK, TP_END_RANK, firstRank, kPointSizePowerCeiled, kPointSizeCeiled>(
            xbuff, obuff, tmp_bufs, tw_table, pingPong, inv);
        if
            constexpr(TP_WINDOW_VSIZE <= 64) chess_memory_fence();
        opt_int_stage<TT_DATA, TT_INTERNAL_DATA, TT_OUT_DATA, TT_TWIDDLE, TP_TWIDDLE_MODE, 2, TP_POINT_SIZE, TP_SHIFT,
                      TP_START_RANK, TP_END_RANK, firstRank, kPointSizePowerCeiled, kPointSizeCeiled>(
            xbuff, obuff, tmp_bufs, tw_table, pingPong, inv);
        if
            constexpr(TP_WINDOW_VSIZE <= 64) chess_memory_fence();
        opt_int_stage<TT_DATA, TT_INTERNAL_DATA, TT_OUT_DATA, TT_TWIDDLE, TP_TWIDDLE_MODE, 4, TP_POINT_SIZE, TP_SHIFT,
                      TP_START_RANK, TP_END_RANK, firstRank, kPointSizePowerCeiled, kPointSizeCeiled>(
            xbuff, obuff, tmp_bufs, tw_table, pingPong, inv);
        if
            constexpr(TP_WINDOW_VSIZE <= 64) chess_memory_fence();
        opt_int_stage<TT_DATA, TT_INTERNAL_DATA, TT_OUT_DATA, TT_TWIDDLE, TP_TWIDDLE_MODE, 6, TP_POINT_SIZE, TP_SHIFT,
                      TP_START_RANK, TP_END_RANK, firstRank, kPointSizePowerCeiled, kPointSizeCeiled>(
            xbuff, obuff, tmp_bufs, tw_table, pingPong, inv);
        if
            constexpr(TP_WINDOW_VSIZE <= 64) chess_memory_fence();
        opt_int_stage<TT_DATA, TT_INTERNAL_DATA, TT_OUT_DATA, TT_TWIDDLE, TP_TWIDDLE_MODE, 8, TP_POINT_SIZE, TP_SHIFT,
                      TP_START_RANK, TP_END_RANK, firstRank, kPointSizePowerCeiled, kPointSizeCeiled>(
            xbuff, obuff, tmp_bufs, tw_table, pingPong, inv);
        if
            constexpr(TP_WINDOW_VSIZE <= 64) chess_memory_fence();
        opt_int_stage<TT_DATA, TT_INTERNAL_DATA, TT_OUT_DATA, TT_TWIDDLE, TP_TWIDDLE_MODE, 10, TP_POINT_SIZE, TP_SHIFT,
                      TP_START_RANK, TP_END_RANK, firstRank, kPointSizePowerCeiled, kPointSizeCeiled>(
            xbuff, obuff, tmp_bufs, tw_table, pingPong, inv);
    } // end of if float or int handling
};

// stockhamStages calc body. Dynamic variant.
template <typename TT_DATA,
          typename TT_OUT_DATA,
          typename TT_TWIDDLE,
          typename TT_INTERNAL_DATA,
          unsigned int TP_POINT_SIZE,
          unsigned int TP_FFT_NIFFT,
          unsigned int TP_SHIFT,
          unsigned int TP_START_RANK,
          unsigned int TP_END_RANK,
          unsigned int TP_DYN_PT_SIZE,
          unsigned int TP_WINDOW_VSIZE,
          unsigned int TP_ORIG_PAR_POWER,
          unsigned int TP_TWIDDLE_MODE>
INLINE_DECL void stockhamStages<TT_DATA,
                                TT_OUT_DATA,
                                TT_TWIDDLE,
                                TT_INTERNAL_DATA,
                                TP_POINT_SIZE,
                                TP_FFT_NIFFT,
                                TP_SHIFT,
                                TP_START_RANK,
                                TP_END_RANK,
                                TP_DYN_PT_SIZE,
                                TP_WINDOW_VSIZE,
                                TP_ORIG_PAR_POWER,
                                TP_TWIDDLE_MODE>::calc(TT_DATA* __restrict xbuff,
                                                       TT_TWIDDLE** tw_table,
                                                       TT_INTERNAL_DATA* tmp1_buf,
                                                       TT_INTERNAL_DATA* tmp2_buf,
                                                       TT_OUT_DATA* __restrict obuff,
                                                       int ptSizePwr,
                                                       bool inv) {
    // Reuse the output buffer as one of the two scratchpad pingpong buffers?
    TT_INTERNAL_DATA* my_tmp1_buf = fnReuseOutputBuffer<TT_DATA, TP_START_RANK, TP_END_RANK, TP_DYN_PT_SIZE>()
                                        ? (TT_INTERNAL_DATA*)obuff
                                        : tmp1_buf;
    TT_INTERNAL_DATA* my_tmp2_buf = fnUsePingPongIntBuffer<TT_DATA>() ? tmp2_buf : (TT_INTERNAL_DATA*)xbuff;
    TT_INTERNAL_DATA* tmp_bufs[2] = {my_tmp1_buf, my_tmp2_buf}; // tmp2 is actually xbuff reused.
    unsigned int pingPong = 1; // use tmp_buf1 as input or tmp_buf2? Initially tmp2 is input since this is xbuff
    int tw_index = 0;
    int rank = 0;
    int r = 0; // r is an indication to the stage of rank.
    unsigned int intR2Stages;
    unsigned int ptSize = (unsigned int)1 << ptSizePwr;
    unsigned int myStart = TP_START_RANK;
    unsigned int myEnd = TP_END_RANK;
    unsigned int firstRank;

    if
        constexpr(std::is_same<TT_DATA, cfloat>::value) {
            //-----------------------------------------------------------------------------
            // cfloat handling
            opt_cfloat_dyn_stage<0, TP_POINT_SIZE, TP_START_RANK, TP_END_RANK, kPointSizePower>(
                xbuff, obuff, tmp_bufs, tw_table, pingPong, inv, ptSizePwr);
            opt_cfloat_dyn_stage<1, TP_POINT_SIZE, TP_START_RANK, TP_END_RANK, kPointSizePower>(
                xbuff, obuff, tmp_bufs, tw_table, pingPong, inv, ptSizePwr);
            opt_cfloat_dyn_stage<2, TP_POINT_SIZE, TP_START_RANK, TP_END_RANK, kPointSizePower>(
                xbuff, obuff, tmp_bufs, tw_table, pingPong, inv, ptSizePwr);
            opt_cfloat_dyn_stage<3, TP_POINT_SIZE, TP_START_RANK, TP_END_RANK, kPointSizePower>(
                xbuff, obuff, tmp_bufs, tw_table, pingPong, inv, ptSizePwr);
            opt_cfloat_dyn_stage<4, TP_POINT_SIZE, TP_START_RANK, TP_END_RANK, kPointSizePower>(
                xbuff, obuff, tmp_bufs, tw_table, pingPong, inv, ptSizePwr);
            opt_cfloat_dyn_stage<5, TP_POINT_SIZE, TP_START_RANK, TP_END_RANK, kPointSizePower>(
                xbuff, obuff, tmp_bufs, tw_table, pingPong, inv, ptSizePwr);
            opt_cfloat_dyn_stage<6, TP_POINT_SIZE, TP_START_RANK, TP_END_RANK, kPointSizePower>(
                xbuff, obuff, tmp_bufs, tw_table, pingPong, inv, ptSizePwr);
            opt_cfloat_dyn_stage<7, TP_POINT_SIZE, TP_START_RANK, TP_END_RANK, kPointSizePower>(
                xbuff, obuff, tmp_bufs, tw_table, pingPong, inv, ptSizePwr);
            opt_cfloat_dyn_stage<8, TP_POINT_SIZE, TP_START_RANK, TP_END_RANK, kPointSizePower>(
                xbuff, obuff, tmp_bufs, tw_table, pingPong, inv, ptSizePwr);
            opt_cfloat_dyn_stage<9, TP_POINT_SIZE, TP_START_RANK, TP_END_RANK, kPointSizePower>(
                xbuff, obuff, tmp_bufs, tw_table, pingPong, inv, ptSizePwr);
            opt_cfloat_dyn_stage<10, TP_POINT_SIZE, TP_START_RANK, TP_END_RANK, kPointSizePower>(
                xbuff, obuff, tmp_bufs, tw_table, pingPong, inv, ptSizePwr);
            opt_cfloat_dyn_stage<11, TP_POINT_SIZE, TP_START_RANK, TP_END_RANK, kPointSizePower>(
                xbuff, obuff, tmp_bufs, tw_table, pingPong, inv, ptSizePwr);
        }
    else { // integer types can use radix 4 stages
        //------------------------------------------------------------------------------------------------
        // cint handling, dynamic variant
        ptSize = ((unsigned int)1 << ptSizePwr); // without this, x86 zeros ptSize.

        opt_int_dyn_stage<TT_DATA, TT_INTERNAL_DATA, TT_OUT_DATA, TT_TWIDDLE, TP_TWIDDLE_MODE, 0, TP_POINT_SIZE,
                          TP_SHIFT, TP_START_RANK, TP_END_RANK, kPointSizePower, kPointSizePowerCeiled,
                          kPointSizeCeiled>(xbuff, obuff, tmp_bufs, tw_table, pingPong, inv, ptSizePwr);
        opt_int_dyn_stage<TT_DATA, TT_INTERNAL_DATA, TT_OUT_DATA, TT_TWIDDLE, TP_TWIDDLE_MODE, 2, TP_POINT_SIZE,
                          TP_SHIFT, TP_START_RANK, TP_END_RANK, kPointSizePower, kPointSizePowerCeiled,
                          kPointSizeCeiled>(xbuff, obuff, tmp_bufs, tw_table, pingPong, inv, ptSizePwr);
        opt_int_dyn_stage<TT_DATA, TT_INTERNAL_DATA, TT_OUT_DATA, TT_TWIDDLE, TP_TWIDDLE_MODE, 4, TP_POINT_SIZE,
                          TP_SHIFT, TP_START_RANK, TP_END_RANK, kPointSizePower, kPointSizePowerCeiled,
                          kPointSizeCeiled>(xbuff, obuff, tmp_bufs, tw_table, pingPong, inv, ptSizePwr);
        opt_int_dyn_stage<TT_DATA, TT_INTERNAL_DATA, TT_OUT_DATA, TT_TWIDDLE, TP_TWIDDLE_MODE, 6, TP_POINT_SIZE,
                          TP_SHIFT, TP_START_RANK, TP_END_RANK, kPointSizePower, kPointSizePowerCeiled,
                          kPointSizeCeiled>(xbuff, obuff, tmp_bufs, tw_table, pingPong, inv, ptSizePwr);
        opt_int_dyn_stage<TT_DATA, TT_INTERNAL_DATA, TT_OUT_DATA, TT_TWIDDLE, TP_TWIDDLE_MODE, 8, TP_POINT_SIZE,
                          TP_SHIFT, TP_START_RANK, TP_END_RANK, kPointSizePower, kPointSizePowerCeiled,
                          kPointSizeCeiled>(xbuff, obuff, tmp_bufs, tw_table, pingPong, inv, ptSizePwr);
        opt_int_dyn_stage<TT_DATA, TT_INTERNAL_DATA, TT_OUT_DATA, TT_TWIDDLE, TP_TWIDDLE_MODE, 10, TP_POINT_SIZE,
                          TP_SHIFT, TP_START_RANK, TP_END_RANK, kPointSizePower, kPointSizePowerCeiled,
                          kPointSizeCeiled>(xbuff, obuff, tmp_bufs, tw_table, pingPong, inv, ptSizePwr);

    } // end of float/integer handling

    constexpr int koutVSize = __ALIGN_BYTE_SIZE__ / sizeof(TT_OUT_DATA);

    if (ptSize < TP_POINT_SIZE) {
        using outVectType = ::aie::vector<TT_OUT_DATA, koutVSize>;
        outVectType zerosOut = ::aie::zeros<TT_OUT_DATA, koutVSize>();
        //        outVectType* outFillPtr = (outVectType*)obuff;
        // outFillPtr += ptSize / koutVSize;
        outVectType* outFillPtr = (outVectType*)&obuff[ptSize];

        int fillCycles = (TP_POINT_SIZE - ptSize) / koutVSize;
        for (int i = 0; i < fillCycles; i++) {
            *outFillPtr++ = zerosOut;
        }
    }
    chess_memory_fence();
};

template <typename T_TW, int T_PT, int T_TWPT, int T_TW_MODE, int TP_DYN_PT_SIZE, int TP_START_RANK, int TP_END_RANK>
INLINE_DECL constexpr T_TW* fnGetTwPtr() {
    constexpr int kOddPowerAdj = std::is_same<T_TW, cfloat>::value ? 0 : fnOddPower<T_PT>();
    // This function would be better phrased with for loops rather than nested ifs, but since the tables are unique
    // identifiers it is not possible.
    // Also, for integers, TP_END_RANK is rounded up to Even, so point size 128 has TP_END_RANK=8, as does 256.
    if
        constexpr(std::is_same<T_TW, cfloat>::value) {
            if
                constexpr(T_TWPT == 1 && T_TWPT < T_PT && (TP_DYN_PT_SIZE == 1 || TP_START_RANK <= 0) &&
                          TP_END_RANK > 0) return (T_TW*)fft_lut_tw1_cfloat;
            else if
                constexpr(T_TWPT == 2 && T_TWPT < T_PT && (TP_DYN_PT_SIZE == 1 || TP_START_RANK <= 1) &&
                          TP_END_RANK > 1) return (T_TW*)fft_lut_tw2_cfloat;
            else if
                constexpr(T_TWPT == 4 && T_TWPT < T_PT && (TP_DYN_PT_SIZE == 1 || TP_START_RANK <= 2) &&
                          TP_END_RANK > 2) return (T_TW*)fft_lut_tw4_cfloat;
            else if
                constexpr(T_TWPT == 8 && T_TWPT < T_PT && (TP_DYN_PT_SIZE == 1 || TP_START_RANK <= 3) &&
                          TP_END_RANK > 3) return (T_TW*)fft_lut_tw8_cfloat;
            else if
                constexpr(T_TWPT == 16 && T_TWPT < T_PT && (TP_DYN_PT_SIZE == 1 || TP_START_RANK <= 4) &&
                          TP_END_RANK > 4) return (T_TW*)fft_lut_tw16_cfloat;
            else if
                constexpr(T_TWPT == 32 && T_TWPT < T_PT && (TP_DYN_PT_SIZE == 1 || TP_START_RANK <= 5) &&
                          TP_END_RANK > 5) return (T_TW*)fft_lut_tw32_cfloat;
            else if
                constexpr(T_TWPT == 64 && T_TWPT < T_PT && (TP_DYN_PT_SIZE == 1 || TP_START_RANK <= 6) &&
                          TP_END_RANK > 6) return (T_TW*)fft_lut_tw64_cfloat;
            else if
                constexpr(T_TWPT == 128 && T_TWPT < T_PT && (TP_DYN_PT_SIZE == 1 || TP_START_RANK <= 7) &&
                          TP_END_RANK > 7) return (T_TW*)fft_lut_tw128_cfloat;
            else if
                constexpr(T_TWPT == 256 && T_TWPT < T_PT && (TP_DYN_PT_SIZE == 1 || TP_START_RANK <= 8) &&
                          TP_END_RANK > 8) return (T_TW*)fft_lut_tw256_cfloat;
            else if
                constexpr(T_TWPT == 512 && T_TWPT < T_PT && (TP_DYN_PT_SIZE == 1 || TP_START_RANK <= 9) &&
                          TP_END_RANK > 9) return (T_TW*)fft_lut_tw512_cfloat;
            else if
                constexpr(T_TWPT == 1024 && T_TWPT < T_PT && (TP_DYN_PT_SIZE == 1 || TP_START_RANK <= 10) &&
                          TP_END_RANK > 10) return (T_TW*)fft_lut_tw1024_cfloat;
            else if
                constexpr(T_TWPT == 2048 && T_TWPT < T_PT && (TP_DYN_PT_SIZE == 1 || TP_START_RANK <= 11) &&
                          TP_END_RANK > 11) return (T_TW*)fft_lut_tw2048_cfloat; // Placeholder Not supported in AIE1
            else
                return NULL;
        }
    if
        constexpr(std::is_same<T_TW, cint32>::value) {
            // Also, for integers, TP_END_RANK is rounded up to Even, so point size 128 has TP_END_RANK=8, as does 256.
            // TP_START_RANK is similarly affected. This is why there is -1 in the clause
            if
                constexpr(T_TW_MODE == 0) {
                    if
                        constexpr(T_TWPT == 1 && T_TWPT < T_PT &&
                                  (TP_DYN_PT_SIZE == 1 || TP_START_RANK - kOddPowerAdj <= 0) &&
                                  TP_END_RANK - kOddPowerAdj > 0) return (T_TW*)fft_lut_tw1_cint32;
                    else if
                        constexpr(T_TWPT == 2 && T_TWPT < T_PT &&
                                  (TP_DYN_PT_SIZE == 1 || TP_START_RANK - kOddPowerAdj <= 1) &&
                                  TP_END_RANK - kOddPowerAdj > 1) return (T_TW*)fft_lut_tw2_cint32;
                    else if
                        constexpr(T_TWPT == 4 && T_TWPT < T_PT &&
                                  (TP_DYN_PT_SIZE == 1 || TP_START_RANK - kOddPowerAdj <= 2) &&
                                  TP_END_RANK - kOddPowerAdj > 2) return (T_TW*)fft_lut_tw4_cint32;
                    else if
                        constexpr(T_TWPT == 8 && T_TWPT < T_PT &&
                                  (TP_DYN_PT_SIZE == 1 || TP_START_RANK - kOddPowerAdj <= 3) &&
                                  TP_END_RANK - kOddPowerAdj > 3) return T_TWPT == T_PT / 2
                            ? (T_TW*)fft_lut_tw8_cint32_half
                            : (T_TW*)fft_lut_tw8_cint32;
                    else if
                        constexpr(T_TWPT == 16 && T_TWPT < T_PT &&
                                  (TP_DYN_PT_SIZE == 1 || TP_START_RANK - kOddPowerAdj <= 4) &&
                                  TP_END_RANK - kOddPowerAdj > 4) return T_TWPT == T_PT / 2
                            ? (T_TW*)fft_lut_tw16_cint32_half
                            : (T_TW*)fft_lut_tw16_cint32;
                    else if
                        constexpr(T_TWPT == 32 && T_TWPT < T_PT &&
                                  (TP_DYN_PT_SIZE == 1 || TP_START_RANK - kOddPowerAdj <= 5) &&
                                  TP_END_RANK - kOddPowerAdj > 5) return T_TWPT == T_PT / 2
                            ? (T_TW*)fft_lut_tw32_cint32_half
                            : (T_TW*)fft_lut_tw32_cint32;
                    else if
                        constexpr(T_TWPT == 64 && T_TWPT < T_PT &&
                                  (TP_DYN_PT_SIZE == 1 || TP_START_RANK - kOddPowerAdj <= 6) &&
                                  TP_END_RANK - kOddPowerAdj > 6) return T_TWPT == T_PT / 2
                            ? (T_TW*)fft_lut_tw64_cint32_half
                            : (T_TW*)fft_lut_tw64_cint32;
                    else if
                        constexpr(T_TWPT == 128 && T_TWPT < T_PT &&
                                  (TP_DYN_PT_SIZE == 1 || TP_START_RANK - kOddPowerAdj <= 7) &&
                                  TP_END_RANK - kOddPowerAdj > 7) return T_TWPT == T_PT / 2
                            ? (T_TW*)fft_lut_tw128_cint32_half
                            : (T_TW*)fft_lut_tw128_cint32;
                    else if
                        constexpr(T_TWPT == 256 && T_TWPT < T_PT &&
                                  (TP_DYN_PT_SIZE == 1 || TP_START_RANK - kOddPowerAdj <= 8) &&
                                  TP_END_RANK - kOddPowerAdj > 8) return T_TWPT == T_PT / 2
                            ? (T_TW*)fft_lut_tw256_cint32_half
                            : (T_TW*)fft_lut_tw256_cint32;
                    else if
                        constexpr(T_TWPT == 512 && T_TWPT < T_PT &&
                                  (TP_DYN_PT_SIZE == 1 || TP_START_RANK - kOddPowerAdj <= 9) &&
                                  TP_END_RANK - kOddPowerAdj > 9) return T_TWPT == T_PT / 2
                            ? (T_TW*)fft_lut_tw512_cint32_half
                            : (T_TW*)fft_lut_tw512_cint32;
                    else if
                        constexpr(T_TWPT == 1024 && T_TWPT < T_PT &&
                                  (TP_DYN_PT_SIZE == 1 || TP_START_RANK - kOddPowerAdj <= 10) &&
                                  TP_END_RANK - kOddPowerAdj > 10) return T_TWPT == T_PT / 2
                            ? (T_TW*)fft_lut_tw1024_cint32_half
                            : (T_TW*)fft_lut_tw1024_cint32;
                    else if
                        constexpr(T_TWPT == 2048 && T_TWPT < T_PT &&
                                  (TP_DYN_PT_SIZE == 1 || TP_START_RANK - kOddPowerAdj <= 11) &&
                                  TP_END_RANK - kOddPowerAdj > 11) return T_TWPT == T_PT / 2
                            ? (T_TW*)fft_lut_tw2048_cint32_half
                            : (T_TW*)fft_lut_tw2048_cint32;
                    else
                        return NULL;
                }
            else {
                if
                    constexpr(T_TWPT == 1 && T_TWPT < T_PT &&
                              (TP_DYN_PT_SIZE == 1 || TP_START_RANK - kOddPowerAdj <= 0) &&
                              TP_END_RANK - kOddPowerAdj > 0) return (T_TW*)fft_lut_tw1_cint31;
                else if
                    constexpr(T_TWPT == 2 && T_TWPT < T_PT &&
                              (TP_DYN_PT_SIZE == 1 || TP_START_RANK - kOddPowerAdj <= 1) &&
                              TP_END_RANK - kOddPowerAdj > 1) return (T_TW*)fft_lut_tw2_cint31;
                else if
                    constexpr(T_TWPT == 4 && T_TWPT < T_PT &&
                              (TP_DYN_PT_SIZE == 1 || TP_START_RANK - kOddPowerAdj <= 2) &&
                              TP_END_RANK - kOddPowerAdj > 2) return (T_TW*)fft_lut_tw4_cint31;
                else if
                    constexpr(T_TWPT == 8 && T_TWPT < T_PT &&
                              (TP_DYN_PT_SIZE == 1 || TP_START_RANK - kOddPowerAdj <= 3) &&
                              TP_END_RANK - kOddPowerAdj > 3) return T_TWPT == T_PT / 2
                        ? (T_TW*)fft_lut_tw8_cint31_half
                        : (T_TW*)fft_lut_tw8_cint31;
                else if
                    constexpr(T_TWPT == 16 && T_TWPT < T_PT &&
                              (TP_DYN_PT_SIZE == 1 || TP_START_RANK - kOddPowerAdj <= 4) &&
                              TP_END_RANK - kOddPowerAdj > 4) return T_TWPT == T_PT / 2
                        ? (T_TW*)fft_lut_tw16_cint31_half
                        : (T_TW*)fft_lut_tw16_cint31;
                else if
                    constexpr(T_TWPT == 32 && T_TWPT < T_PT &&
                              (TP_DYN_PT_SIZE == 1 || TP_START_RANK - kOddPowerAdj <= 5) &&
                              TP_END_RANK - kOddPowerAdj > 5) return T_TWPT == T_PT / 2
                        ? (T_TW*)fft_lut_tw32_cint31_half
                        : (T_TW*)fft_lut_tw32_cint31;
                else if
                    constexpr(T_TWPT == 64 && T_TWPT < T_PT &&
                              (TP_DYN_PT_SIZE == 1 || TP_START_RANK - kOddPowerAdj <= 6) &&
                              TP_END_RANK - kOddPowerAdj > 6) return T_TWPT == T_PT / 2
                        ? (T_TW*)fft_lut_tw64_cint31_half
                        : (T_TW*)fft_lut_tw64_cint31;
                else if
                    constexpr(T_TWPT == 128 && T_TWPT < T_PT &&
                              (TP_DYN_PT_SIZE == 1 || TP_START_RANK - kOddPowerAdj <= 7) &&
                              TP_END_RANK - kOddPowerAdj > 7) return T_TWPT == T_PT / 2
                        ? (T_TW*)fft_lut_tw128_cint31_half
                        : (T_TW*)fft_lut_tw128_cint31;
                else if
                    constexpr(T_TWPT == 256 && T_TWPT < T_PT &&
                              (TP_DYN_PT_SIZE == 1 || TP_START_RANK - kOddPowerAdj <= 8) &&
                              TP_END_RANK - kOddPowerAdj > 8) return T_TWPT == T_PT / 2
                        ? (T_TW*)fft_lut_tw256_cint31_half
                        : (T_TW*)fft_lut_tw256_cint31;
                else if
                    constexpr(T_TWPT == 512 && T_TWPT < T_PT &&
                              (TP_DYN_PT_SIZE == 1 || TP_START_RANK - kOddPowerAdj <= 9) &&
                              TP_END_RANK - kOddPowerAdj > 9) return T_TWPT == T_PT / 2
                        ? (T_TW*)fft_lut_tw512_cint31_half
                        : (T_TW*)fft_lut_tw512_cint31;
                else if
                    constexpr(T_TWPT == 1024 && T_TWPT < T_PT &&
                              (TP_DYN_PT_SIZE == 1 || TP_START_RANK - kOddPowerAdj <= 10) &&
                              TP_END_RANK - kOddPowerAdj > 10) return T_TWPT == T_PT / 2
                        ? (T_TW*)fft_lut_tw1024_cint31_half
                        : (T_TW*)fft_lut_tw1024_cint31;
                else if
                    constexpr(T_TWPT == 2048 && T_TWPT < T_PT &&
                              (TP_DYN_PT_SIZE == 1 || TP_START_RANK - kOddPowerAdj <= 11) &&
                              TP_END_RANK - kOddPowerAdj > 11) return T_TWPT == T_PT / 2
                        ? (T_TW*)fft_lut_tw2048_cint31_half
                        : (T_TW*)fft_lut_tw2048_cint31;
                else
                    return NULL;
            }
        }
    if
        constexpr(std::is_same<T_TW, cint16>::value) {
            if
                constexpr(T_TW_MODE == 0) {
                    if
                        constexpr(T_TWPT == 1 && T_TWPT < T_PT &&
                                  (TP_DYN_PT_SIZE == 1 || TP_START_RANK - kOddPowerAdj <= 0) &&
                                  TP_END_RANK - kOddPowerAdj > 0) return (T_TW*)fft_lut_tw1_cint16;
                    else if
                        constexpr(T_TWPT == 2 && T_TWPT < T_PT &&
                                  (TP_DYN_PT_SIZE == 1 || TP_START_RANK - kOddPowerAdj <= 1) &&
                                  TP_END_RANK - kOddPowerAdj > 1) return (T_TW*)fft_lut_tw2_cint16;
                    else if
                        constexpr(T_TWPT == 4 && T_TWPT < T_PT &&
                                  (TP_DYN_PT_SIZE == 1 || TP_START_RANK - kOddPowerAdj <= 2) &&
                                  TP_END_RANK - kOddPowerAdj > 2) return (T_TW*)fft_lut_tw4_cint16;
                    else if
                        constexpr(T_TWPT == 8 && T_TWPT < T_PT &&
                                  (TP_DYN_PT_SIZE == 1 || TP_START_RANK - kOddPowerAdj <= 3) &&
                                  TP_END_RANK - kOddPowerAdj > 3) return T_TWPT == T_PT / 2
                            ? (T_TW*)fft_lut_tw8_cint16_half
                            : (T_TW*)fft_lut_tw8_cint16;
                    else if
                        constexpr(T_TWPT == 16 && T_TWPT < T_PT &&
                                  (TP_DYN_PT_SIZE == 1 || TP_START_RANK - kOddPowerAdj <= 4) &&
                                  TP_END_RANK - kOddPowerAdj > 4) return T_TWPT == T_PT / 2
                            ? (T_TW*)fft_lut_tw16_cint16_half
                            : (T_TW*)fft_lut_tw16_cint16;
                    else if
                        constexpr(T_TWPT == 32 && T_TWPT < T_PT &&
                                  (TP_DYN_PT_SIZE == 1 || TP_START_RANK - kOddPowerAdj <= 5) &&
                                  TP_END_RANK - kOddPowerAdj > 5) return T_TWPT == T_PT / 2
                            ? (T_TW*)fft_lut_tw32_cint16_half
                            : (T_TW*)fft_lut_tw32_cint16;
                    else if
                        constexpr(T_TWPT == 64 && T_TWPT < T_PT &&
                                  (TP_DYN_PT_SIZE == 1 || TP_START_RANK - kOddPowerAdj <= 6) &&
                                  TP_END_RANK - kOddPowerAdj > 6) return T_TWPT == T_PT / 2
                            ? (T_TW*)fft_lut_tw64_cint16_half
                            : (T_TW*)fft_lut_tw64_cint16;
                    else if
                        constexpr(T_TWPT == 128 && T_TWPT < T_PT &&
                                  (TP_DYN_PT_SIZE == 1 || TP_START_RANK - kOddPowerAdj <= 7) &&
                                  TP_END_RANK - kOddPowerAdj > 7) return T_TWPT == T_PT / 2
                            ? (T_TW*)fft_lut_tw128_cint16_half
                            : (T_TW*)fft_lut_tw128_cint16;
                    else if
                        constexpr(T_TWPT == 256 && T_TWPT < T_PT &&
                                  (TP_DYN_PT_SIZE == 1 || TP_START_RANK - kOddPowerAdj <= 8) &&
                                  TP_END_RANK - kOddPowerAdj > 8) return T_TWPT == T_PT / 2
                            ? (T_TW*)fft_lut_tw256_cint16_half
                            : (T_TW*)fft_lut_tw256_cint16;
                    else if
                        constexpr(T_TWPT == 512 && T_TWPT < T_PT &&
                                  (TP_DYN_PT_SIZE == 1 || TP_START_RANK - kOddPowerAdj <= 9) &&
                                  TP_END_RANK - kOddPowerAdj > 9) return T_TWPT == T_PT / 2
                            ? (T_TW*)fft_lut_tw512_cint16_half
                            : (T_TW*)fft_lut_tw512_cint16;
                    else if
                        constexpr(T_TWPT == 1024 && T_TWPT < T_PT &&
                                  (TP_DYN_PT_SIZE == 1 || TP_START_RANK - kOddPowerAdj <= 10) &&
                                  TP_END_RANK - kOddPowerAdj > 10) return T_TWPT == T_PT / 2
                            ? (T_TW*)fft_lut_tw1024_cint16_half
                            : (T_TW*)fft_lut_tw1024_cint16;
                    else if
                        constexpr(T_TWPT == 2048 && T_TWPT < T_PT &&
                                  (TP_DYN_PT_SIZE == 1 || TP_START_RANK - kOddPowerAdj <= 11) &&
                                  TP_END_RANK - kOddPowerAdj > 11) return T_TWPT == T_PT / 2
                            ? (T_TW*)fft_lut_tw2048_cint16_half
                            : (T_TW*)fft_lut_tw2048_cint16;
                    else
                        return NULL;
                }
            else {
                if
                    constexpr(T_TWPT == 1 && T_TWPT < T_PT &&
                              (TP_DYN_PT_SIZE == 1 || TP_START_RANK - kOddPowerAdj <= 0) &&
                              TP_END_RANK - kOddPowerAdj > 0) return (T_TW*)fft_lut_tw1_cint15;
                else if
                    constexpr(T_TWPT == 2 && T_TWPT < T_PT &&
                              (TP_DYN_PT_SIZE == 1 || TP_START_RANK - kOddPowerAdj <= 1) &&
                              TP_END_RANK - kOddPowerAdj > 1) return (T_TW*)fft_lut_tw2_cint15;
                else if
                    constexpr(T_TWPT == 4 && T_TWPT < T_PT &&
                              (TP_DYN_PT_SIZE == 1 || TP_START_RANK - kOddPowerAdj <= 2) &&
                              TP_END_RANK - kOddPowerAdj > 2) return (T_TW*)fft_lut_tw4_cint15;
                else if
                    constexpr(T_TWPT == 8 && T_TWPT < T_PT &&
                              (TP_DYN_PT_SIZE == 1 || TP_START_RANK - kOddPowerAdj <= 3) &&
                              TP_END_RANK - kOddPowerAdj > 3) return T_TWPT == T_PT / 2
                        ? (T_TW*)fft_lut_tw8_cint15_half
                        : (T_TW*)fft_lut_tw8_cint15;
                else if
                    constexpr(T_TWPT == 16 && T_TWPT < T_PT &&
                              (TP_DYN_PT_SIZE == 1 || TP_START_RANK - kOddPowerAdj <= 4) &&
                              TP_END_RANK - kOddPowerAdj > 4) return T_TWPT == T_PT / 2
                        ? (T_TW*)fft_lut_tw16_cint15_half
                        : (T_TW*)fft_lut_tw16_cint15;
                else if
                    constexpr(T_TWPT == 32 && T_TWPT < T_PT &&
                              (TP_DYN_PT_SIZE == 1 || TP_START_RANK - kOddPowerAdj <= 5) &&
                              TP_END_RANK - kOddPowerAdj > 5) return T_TWPT == T_PT / 2
                        ? (T_TW*)fft_lut_tw32_cint15_half
                        : (T_TW*)fft_lut_tw32_cint15;
                else if
                    constexpr(T_TWPT == 64 && T_TWPT < T_PT &&
                              (TP_DYN_PT_SIZE == 1 || TP_START_RANK - kOddPowerAdj <= 6) &&
                              TP_END_RANK - kOddPowerAdj > 6) return T_TWPT == T_PT / 2
                        ? (T_TW*)fft_lut_tw64_cint15_half
                        : (T_TW*)fft_lut_tw64_cint15;
                else if
                    constexpr(T_TWPT == 128 && T_TWPT < T_PT &&
                              (TP_DYN_PT_SIZE == 1 || TP_START_RANK - kOddPowerAdj <= 7) &&
                              TP_END_RANK - kOddPowerAdj > 7) return T_TWPT == T_PT / 2
                        ? (T_TW*)fft_lut_tw128_cint15_half
                        : (T_TW*)fft_lut_tw128_cint15;
                else if
                    constexpr(T_TWPT == 256 && T_TWPT < T_PT &&
                              (TP_DYN_PT_SIZE == 1 || TP_START_RANK - kOddPowerAdj <= 8) &&
                              TP_END_RANK - kOddPowerAdj > 8) return T_TWPT == T_PT / 2
                        ? (T_TW*)fft_lut_tw256_cint15_half
                        : (T_TW*)fft_lut_tw256_cint15;
                else if
                    constexpr(T_TWPT == 512 && T_TWPT < T_PT &&
                              (TP_DYN_PT_SIZE == 1 || TP_START_RANK - kOddPowerAdj <= 9) &&
                              TP_END_RANK - kOddPowerAdj > 9) return T_TWPT == T_PT / 2
                        ? (T_TW*)fft_lut_tw512_cint15_half
                        : (T_TW*)fft_lut_tw512_cint15;
                else if
                    constexpr(T_TWPT == 1024 && T_TWPT < T_PT &&
                              (TP_DYN_PT_SIZE == 1 || TP_START_RANK - kOddPowerAdj <= 10) &&
                              TP_END_RANK - kOddPowerAdj > 10) return T_TWPT == T_PT / 2
                        ? (T_TW*)fft_lut_tw1024_cint15_half
                        : (T_TW*)fft_lut_tw1024_cint15;
                else if
                    constexpr(T_TWPT == 2048 && T_TWPT < T_PT &&
                              (TP_DYN_PT_SIZE == 1 || TP_START_RANK - kOddPowerAdj <= 11) &&
                              TP_END_RANK - kOddPowerAdj > 11) return T_TWPT == T_PT / 2
                        ? (T_TW*)fft_lut_tw2048_cint15_half
                        : (T_TW*)fft_lut_tw2048_cint15;
                else
                    return NULL;
            }
        }
}
template <typename T_TW, int T_PT, int T_TWPT, int T_TW_MODE, int TP_DYN_PT_SIZE, int TP_START_RANK, int TP_END_RANK>
INLINE_DECL constexpr T_TW* fnGetR4TwPtr() {
    constexpr int kOddPowerAdj = std::is_same<T_TW, cfloat>::value ? 0 : fnOddPower<T_PT>();
    if
        constexpr(std::is_same<T_TW, cfloat>::value) { return NULL; }
    if
        constexpr(std::is_same<T_TW, cint32>::value) {
            if
                constexpr(T_TW_MODE == 0) {
                    if
                        constexpr(T_TWPT == 1 && T_TWPT <= T_PT &&
                                  (TP_DYN_PT_SIZE == 1 || TP_START_RANK - kOddPowerAdj <= 0) &&
                                  TP_END_RANK > 0) return (T_TW*)fft_lut_cint32_r4_1_2;
                    else if
                        constexpr(T_TWPT == 2 && T_TWPT <= T_PT &&
                                  (TP_DYN_PT_SIZE == 1 || TP_START_RANK - kOddPowerAdj <= 1) &&
                                  TP_END_RANK > 1) return (T_TW*)fft_lut_cint32_r4_2_4;
                    else if
                        constexpr(T_TWPT == 4 && T_TWPT <= T_PT &&
                                  (TP_DYN_PT_SIZE == 1 || TP_START_RANK - kOddPowerAdj <= 2) &&
                                  TP_END_RANK > 2) return (T_TW*)fft_lut_cint32_r4_4_8;
                    else if
                        constexpr(T_TWPT == 8 && T_TWPT <= T_PT &&
                                  (TP_DYN_PT_SIZE == 1 || TP_START_RANK - kOddPowerAdj <= 3) &&
                                  TP_END_RANK > 3) return (T_TW*)fft_lut_cint32_r4_8_16;
                    else if
                        constexpr(T_TWPT == 16 && T_TWPT <= T_PT &&
                                  (TP_DYN_PT_SIZE == 1 || TP_START_RANK - kOddPowerAdj <= 4) &&
                                  TP_END_RANK > 4) return (T_TW*)fft_lut_cint32_r4_16_32;
                    else if
                        constexpr(T_TWPT == 32 && T_TWPT <= T_PT &&
                                  (TP_DYN_PT_SIZE == 1 || TP_START_RANK - kOddPowerAdj <= 5) &&
                                  TP_END_RANK > 5) return (T_TW*)fft_lut_cint32_r4_32_64;
                    else if
                        constexpr(T_TWPT == 64 && T_TWPT <= T_PT &&
                                  (TP_DYN_PT_SIZE == 1 || TP_START_RANK - kOddPowerAdj <= 6) &&
                                  TP_END_RANK > 6) return (T_TW*)fft_lut_cint32_r4_64_128;
                    else if
                        constexpr(T_TWPT == 128 && T_TWPT <= T_PT &&
                                  (TP_DYN_PT_SIZE == 1 || TP_START_RANK - kOddPowerAdj <= 7) &&
                                  TP_END_RANK > 7) return (T_TW*)fft_lut_cint32_r4_128_256;
                    else if
                        constexpr(T_TWPT == 256 && T_TWPT <= T_PT &&
                                  (TP_DYN_PT_SIZE == 1 || TP_START_RANK - kOddPowerAdj <= 8) &&
                                  TP_END_RANK > 8) return (T_TW*)fft_lut_cint32_r4_256_512;
                    else if
                        constexpr(T_TWPT == 512 && T_TWPT <= T_PT &&
                                  (TP_DYN_PT_SIZE == 1 || TP_START_RANK - kOddPowerAdj <= 9) &&
                                  TP_END_RANK > 9) return (T_TW*)fft_lut_cint32_r4_512_1024;
                    else if
                        constexpr(T_TWPT == 1024 && T_TWPT <= T_PT &&
                                  (TP_DYN_PT_SIZE == 1 || TP_START_RANK - kOddPowerAdj <= 10) &&
                                  TP_END_RANK > 10) return (T_TW*)fft_lut_cint32_r4_1024_2048;
                    else
                        return NULL;
                }
            else {
                if
                    constexpr(T_TWPT == 1 && T_TWPT <= T_PT &&
                              (TP_DYN_PT_SIZE == 1 || TP_START_RANK - kOddPowerAdj <= 0) &&
                              TP_END_RANK > 0) return (T_TW*)fft_lut_cint31_r4_1_2;
                else if
                    constexpr(T_TWPT == 2 && T_TWPT <= T_PT &&
                              (TP_DYN_PT_SIZE == 1 || TP_START_RANK - kOddPowerAdj <= 1) &&
                              TP_END_RANK > 1) return (T_TW*)fft_lut_cint31_r4_2_4;
                else if
                    constexpr(T_TWPT == 4 && T_TWPT <= T_PT &&
                              (TP_DYN_PT_SIZE == 1 || TP_START_RANK - kOddPowerAdj <= 2) &&
                              TP_END_RANK > 2) return (T_TW*)fft_lut_cint31_r4_4_8;
                else if
                    constexpr(T_TWPT == 8 && T_TWPT <= T_PT &&
                              (TP_DYN_PT_SIZE == 1 || TP_START_RANK - kOddPowerAdj <= 3) &&
                              TP_END_RANK > 3) return (T_TW*)fft_lut_cint31_r4_8_16;
                else if
                    constexpr(T_TWPT == 16 && T_TWPT <= T_PT &&
                              (TP_DYN_PT_SIZE == 1 || TP_START_RANK - kOddPowerAdj <= 4) &&
                              TP_END_RANK > 4) return (T_TW*)fft_lut_cint31_r4_16_32;
                else if
                    constexpr(T_TWPT == 32 && T_TWPT <= T_PT &&
                              (TP_DYN_PT_SIZE == 1 || TP_START_RANK - kOddPowerAdj <= 5) &&
                              TP_END_RANK > 5) return (T_TW*)fft_lut_cint31_r4_32_64;
                else if
                    constexpr(T_TWPT == 64 && T_TWPT <= T_PT &&
                              (TP_DYN_PT_SIZE == 1 || TP_START_RANK - kOddPowerAdj <= 6) &&
                              TP_END_RANK > 6) return (T_TW*)fft_lut_cint31_r4_64_128;
                else if
                    constexpr(T_TWPT == 128 && T_TWPT <= T_PT &&
                              (TP_DYN_PT_SIZE == 1 || TP_START_RANK - kOddPowerAdj <= 7) &&
                              TP_END_RANK > 7) return (T_TW*)fft_lut_cint31_r4_128_256;
                else if
                    constexpr(T_TWPT == 256 && T_TWPT <= T_PT &&
                              (TP_DYN_PT_SIZE == 1 || TP_START_RANK - kOddPowerAdj <= 8) &&
                              TP_END_RANK > 8) return (T_TW*)fft_lut_cint31_r4_256_512;
                else if
                    constexpr(T_TWPT == 512 && T_TWPT <= T_PT &&
                              (TP_DYN_PT_SIZE == 1 || TP_START_RANK - kOddPowerAdj <= 9) &&
                              TP_END_RANK > 9) return (T_TW*)fft_lut_cint31_r4_512_1024;
                else if
                    constexpr(T_TWPT == 1024 && T_TWPT <= T_PT &&
                              (TP_DYN_PT_SIZE == 1 || TP_START_RANK - kOddPowerAdj <= 10) &&
                              TP_END_RANK > 10) return (T_TW*)fft_lut_cint31_r4_1024_2048;
                else
                    return NULL;
            }
        }
    if
        constexpr(std::is_same<T_TW, cint16>::value) {
            if
                constexpr(T_TW_MODE == 0) {
                    if
                        constexpr(T_TWPT == 1 && T_TWPT <= T_PT &&
                                  (TP_DYN_PT_SIZE == 1 || TP_START_RANK - kOddPowerAdj <= 0) &&
                                  TP_END_RANK > 0) return (T_TW*)fft_lut_cint16_r4_1_2;
                    else if
                        constexpr(T_TWPT == 2 && T_TWPT <= T_PT &&
                                  (TP_DYN_PT_SIZE == 1 || TP_START_RANK - kOddPowerAdj <= 1) &&
                                  TP_END_RANK > 1) return (T_TW*)fft_lut_cint16_r4_2_4;
                    else if
                        constexpr(T_TWPT == 4 && T_TWPT <= T_PT &&
                                  (TP_DYN_PT_SIZE == 1 || TP_START_RANK - kOddPowerAdj <= 2) &&
                                  TP_END_RANK > 2) return (T_TW*)fft_lut_cint16_r4_4_8;
                    else if
                        constexpr(T_TWPT == 8 && T_TWPT <= T_PT &&
                                  (TP_DYN_PT_SIZE == 1 || TP_START_RANK - kOddPowerAdj <= 3) &&
                                  TP_END_RANK > 3) return (T_TW*)fft_lut_cint16_r4_8_16;
                    else if
                        constexpr(T_TWPT == 16 && T_TWPT <= T_PT &&
                                  (TP_DYN_PT_SIZE == 1 || TP_START_RANK - kOddPowerAdj <= 4) &&
                                  TP_END_RANK > 4) return (T_TW*)fft_lut_cint16_r4_16_32;
                    else if
                        constexpr(T_TWPT == 32 && T_TWPT <= T_PT &&
                                  (TP_DYN_PT_SIZE == 1 || TP_START_RANK - kOddPowerAdj <= 5) &&
                                  TP_END_RANK > 5) return (T_TW*)fft_lut_cint16_r4_32_64;
                    else if
                        constexpr(T_TWPT == 64 && T_TWPT <= T_PT &&
                                  (TP_DYN_PT_SIZE == 1 || TP_START_RANK - kOddPowerAdj <= 6) &&
                                  TP_END_RANK > 6) return (T_TW*)fft_lut_cint16_r4_64_128;
                    else if
                        constexpr(T_TWPT == 128 && T_TWPT <= T_PT &&
                                  (TP_DYN_PT_SIZE == 1 || TP_START_RANK - kOddPowerAdj <= 7) &&
                                  TP_END_RANK > 7) return (T_TW*)fft_lut_cint16_r4_128_256;
                    else if
                        constexpr(T_TWPT == 256 && T_TWPT <= T_PT &&
                                  (TP_DYN_PT_SIZE == 1 || TP_START_RANK - kOddPowerAdj <= 8) &&
                                  TP_END_RANK > 8) return (T_TW*)fft_lut_cint16_r4_256_512;
                    else if
                        constexpr(T_TWPT == 512 && T_TWPT <= T_PT &&
                                  (TP_DYN_PT_SIZE == 1 || TP_START_RANK - kOddPowerAdj <= 9) &&
                                  TP_END_RANK > 9) return (T_TW*)fft_lut_cint16_r4_512_1024;
                    else if
                        constexpr(T_TWPT == 1024 && T_TWPT <= T_PT &&
                                  (TP_DYN_PT_SIZE == 1 || TP_START_RANK - kOddPowerAdj <= 10) &&
                                  TP_END_RANK > 10) return (T_TW*)fft_lut_cint16_r4_1024_2048;
                    else
                        return NULL;
                }
            else {
                if
                    constexpr(T_TWPT == 1 && T_TWPT <= T_PT &&
                              (TP_DYN_PT_SIZE == 1 || TP_START_RANK - kOddPowerAdj <= 0) &&
                              TP_END_RANK > 0) return (T_TW*)fft_lut_cint15_r4_1_2;
                else if
                    constexpr(T_TWPT == 2 && T_TWPT <= T_PT &&
                              (TP_DYN_PT_SIZE == 1 || TP_START_RANK - kOddPowerAdj <= 1) &&
                              TP_END_RANK > 1) return (T_TW*)fft_lut_cint15_r4_2_4;
                else if
                    constexpr(T_TWPT == 4 && T_TWPT <= T_PT &&
                              (TP_DYN_PT_SIZE == 1 || TP_START_RANK - kOddPowerAdj <= 2) &&
                              TP_END_RANK > 2) return (T_TW*)fft_lut_cint15_r4_4_8;
                else if
                    constexpr(T_TWPT == 8 && T_TWPT <= T_PT &&
                              (TP_DYN_PT_SIZE == 1 || TP_START_RANK - kOddPowerAdj <= 3) &&
                              TP_END_RANK > 3) return (T_TW*)fft_lut_cint15_r4_8_16;
                else if
                    constexpr(T_TWPT == 16 && T_TWPT <= T_PT &&
                              (TP_DYN_PT_SIZE == 1 || TP_START_RANK - kOddPowerAdj <= 4) &&
                              TP_END_RANK > 4) return (T_TW*)fft_lut_cint15_r4_16_32;
                else if
                    constexpr(T_TWPT == 32 && T_TWPT <= T_PT &&
                              (TP_DYN_PT_SIZE == 1 || TP_START_RANK - kOddPowerAdj <= 5) &&
                              TP_END_RANK > 5) return (T_TW*)fft_lut_cint15_r4_32_64;
                else if
                    constexpr(T_TWPT == 64 && T_TWPT <= T_PT &&
                              (TP_DYN_PT_SIZE == 1 || TP_START_RANK - kOddPowerAdj <= 6) &&
                              TP_END_RANK > 6) return (T_TW*)fft_lut_cint15_r4_64_128;
                else if
                    constexpr(T_TWPT == 128 && T_TWPT <= T_PT &&
                              (TP_DYN_PT_SIZE == 1 || TP_START_RANK - kOddPowerAdj <= 7) &&
                              TP_END_RANK > 7) return (T_TW*)fft_lut_cint15_r4_128_256;
                else if
                    constexpr(T_TWPT == 256 && T_TWPT <= T_PT &&
                              (TP_DYN_PT_SIZE == 1 || TP_START_RANK - kOddPowerAdj <= 8) &&
                              TP_END_RANK > 8) return (T_TW*)fft_lut_cint15_r4_256_512;
                else if
                    constexpr(T_TWPT == 512 && T_TWPT <= T_PT &&
                              (TP_DYN_PT_SIZE == 1 || TP_START_RANK - kOddPowerAdj <= 9) &&
                              TP_END_RANK > 9) return (T_TW*)fft_lut_cint15_r4_512_1024;
                else if
                    constexpr(T_TWPT == 1024 && T_TWPT <= T_PT &&
                              (TP_DYN_PT_SIZE == 1 || TP_START_RANK - kOddPowerAdj <= 10) &&
                              TP_END_RANK > 10) return (T_TW*)fft_lut_cint15_r4_1024_2048;
                else
                    return NULL;
            }
        }
}

// FFT/iFFT DIT single channel function - base of specialization .
//-----------------------------------------------------------------------------------------------------
template <typename TT_DATA,
          typename TT_OUT_DATA,
          typename TT_TWIDDLE,
          unsigned int TP_POINT_SIZE,
          unsigned int TP_FFT_NIFFT,
          unsigned int TP_SHIFT,
          unsigned int TP_START_RANK,
          unsigned int TP_END_RANK,
          unsigned int TP_DYN_PT_SIZE,
          unsigned int TP_WINDOW_VSIZE,
          unsigned int TP_ORIG_PAR_POWER,
          unsigned int TP_TWIDDLE_MODE>
INLINE_DECL void kernelFFTClass<TT_DATA,
                                TT_OUT_DATA,
                                TT_TWIDDLE,
                                TP_POINT_SIZE,
                                TP_FFT_NIFFT,
                                TP_SHIFT,
                                TP_START_RANK,
                                TP_END_RANK,
                                TP_DYN_PT_SIZE,
                                TP_WINDOW_VSIZE,
                                TP_ORIG_PAR_POWER,
                                TP_TWIDDLE_MODE>::kernelFFT(TT_DATA* __restrict inptr, TT_OUT_DATA* __restrict outptr) {
    typedef typename std::conditional<std::is_same<TT_DATA, cint16>::value, cint32_t, TT_DATA>::type T_internalDataType;

    static constexpr TT_TWIDDLE* __restrict tw1 =
        fnGetTwPtr<TT_TWIDDLE, TP_POINT_SIZE, 1, TP_TWIDDLE_MODE, TP_DYN_PT_SIZE, TP_START_RANK, TP_END_RANK>();
    static constexpr TT_TWIDDLE* __restrict tw2 =
        fnGetTwPtr<TT_TWIDDLE, TP_POINT_SIZE, 2, TP_TWIDDLE_MODE, TP_DYN_PT_SIZE, TP_START_RANK, TP_END_RANK>();
    static constexpr TT_TWIDDLE* __restrict tw4 =
        fnGetTwPtr<TT_TWIDDLE, TP_POINT_SIZE, 4, TP_TWIDDLE_MODE, TP_DYN_PT_SIZE, TP_START_RANK, TP_END_RANK>();
    static constexpr TT_TWIDDLE* __restrict tw8 =
        fnGetTwPtr<TT_TWIDDLE, TP_POINT_SIZE, 8, TP_TWIDDLE_MODE, TP_DYN_PT_SIZE, TP_START_RANK, TP_END_RANK>();
    static constexpr TT_TWIDDLE* __restrict tw16 =
        fnGetTwPtr<TT_TWIDDLE, TP_POINT_SIZE, 16, TP_TWIDDLE_MODE, TP_DYN_PT_SIZE, TP_START_RANK, TP_END_RANK>();
    static constexpr TT_TWIDDLE* __restrict tw32 =
        fnGetTwPtr<TT_TWIDDLE, TP_POINT_SIZE, 32, TP_TWIDDLE_MODE, TP_DYN_PT_SIZE, TP_START_RANK, TP_END_RANK>();
    static constexpr TT_TWIDDLE* __restrict tw64 =
        fnGetTwPtr<TT_TWIDDLE, TP_POINT_SIZE, 64, TP_TWIDDLE_MODE, TP_DYN_PT_SIZE, TP_START_RANK, TP_END_RANK>();
    static constexpr TT_TWIDDLE* __restrict tw128 =
        fnGetTwPtr<TT_TWIDDLE, TP_POINT_SIZE, 128, TP_TWIDDLE_MODE, TP_DYN_PT_SIZE, TP_START_RANK, TP_END_RANK>();
    static constexpr TT_TWIDDLE* __restrict tw256 =
        fnGetTwPtr<TT_TWIDDLE, TP_POINT_SIZE, 256, TP_TWIDDLE_MODE, TP_DYN_PT_SIZE, TP_START_RANK, TP_END_RANK>();
    static constexpr TT_TWIDDLE* __restrict tw512 =
        fnGetTwPtr<TT_TWIDDLE, TP_POINT_SIZE, 512, TP_TWIDDLE_MODE, TP_DYN_PT_SIZE, TP_START_RANK, TP_END_RANK>();
    static constexpr TT_TWIDDLE* __restrict tw1024 =
        fnGetTwPtr<TT_TWIDDLE, TP_POINT_SIZE, 1024, TP_TWIDDLE_MODE, TP_DYN_PT_SIZE, TP_START_RANK, TP_END_RANK>();
    static constexpr TT_TWIDDLE* __restrict tw2048 =
        fnGetTwPtr<TT_TWIDDLE, TP_POINT_SIZE, 2048, TP_TWIDDLE_MODE, TP_DYN_PT_SIZE, TP_START_RANK, TP_END_RANK>();

#if __FFT_R4_IMPL__ == 0
    static constexpr TT_TWIDDLE* __restrict tw1_2 = NULL;
    static constexpr TT_TWIDDLE* __restrict tw2_4 = NULL;
    static constexpr TT_TWIDDLE* __restrict tw4_8 = NULL;
    static constexpr TT_TWIDDLE* __restrict tw8_16 = NULL;
    static constexpr TT_TWIDDLE* __restrict tw16_32 = NULL;
    static constexpr TT_TWIDDLE* __restrict tw32_64 = NULL;
    static constexpr TT_TWIDDLE* __restrict tw64_128 = NULL;
    static constexpr TT_TWIDDLE* __restrict tw128_256 = NULL;
    static constexpr TT_TWIDDLE* __restrict tw256_512 = NULL;
    static constexpr TT_TWIDDLE* __restrict tw512_1024 = NULL;
    static constexpr TT_TWIDDLE* __restrict tw1024_2048 = NULL;
#endif //__FFT_R4_IMPL__ == 0
#if __FFT_R4_IMPL__ == 1
    static constexpr TT_TWIDDLE* __restrict tw1_2 =
        fnGetR4TwPtr<TT_TWIDDLE, TP_POINT_SIZE, 1, TP_TWIDDLE_MODE, TP_DYN_PT_SIZE, TP_START_RANK,
                     TP_END_RANK>(); //(TT_TWIDDLE*)fft_lut_r4_1_2;
    static constexpr TT_TWIDDLE* __restrict tw2_4 =
        fnGetR4TwPtr<TT_TWIDDLE, TP_POINT_SIZE, 2, TP_TWIDDLE_MODE, TP_DYN_PT_SIZE, TP_START_RANK,
                     TP_END_RANK>(); //(TT_TWIDDLE*)fft_lut_r4_2_4;
    static constexpr TT_TWIDDLE* __restrict tw4_8 =
        fnGetR4TwPtr<TT_TWIDDLE, TP_POINT_SIZE, 4, TP_TWIDDLE_MODE, TP_DYN_PT_SIZE, TP_START_RANK,
                     TP_END_RANK>(); //(TT_TWIDDLE*)fft_lut_r4_4_8;
    static constexpr TT_TWIDDLE* __restrict tw8_16 =
        fnGetR4TwPtr<TT_TWIDDLE, TP_POINT_SIZE, 8, TP_TWIDDLE_MODE, TP_DYN_PT_SIZE, TP_START_RANK,
                     TP_END_RANK>(); //(TT_TWIDDLE*)fft_lut_r4_8_16;
    static constexpr TT_TWIDDLE* __restrict tw16_32 =
        fnGetR4TwPtr<TT_TWIDDLE, TP_POINT_SIZE, 16, TP_TWIDDLE_MODE, TP_DYN_PT_SIZE, TP_START_RANK,
                     TP_END_RANK>(); //(TT_TWIDDLE*)fft_lut_r4_16_32;
    static constexpr TT_TWIDDLE* __restrict tw32_64 =
        fnGetR4TwPtr<TT_TWIDDLE, TP_POINT_SIZE, 32, TP_TWIDDLE_MODE, TP_DYN_PT_SIZE, TP_START_RANK,
                     TP_END_RANK>(); //(TT_TWIDDLE*)fft_lut_r4_32_64;
    static constexpr TT_TWIDDLE* __restrict tw64_128 =
        fnGetR4TwPtr<TT_TWIDDLE, TP_POINT_SIZE, 64, TP_TWIDDLE_MODE, TP_DYN_PT_SIZE, TP_START_RANK,
                     TP_END_RANK>(); //(TT_TWIDDLE*)fft_lut_r4_64_128;
    static constexpr TT_TWIDDLE* __restrict tw128_256 =
        fnGetR4TwPtr<TT_TWIDDLE, TP_POINT_SIZE, 128, TP_TWIDDLE_MODE, TP_DYN_PT_SIZE, TP_START_RANK,
                     TP_END_RANK>(); //(TT_TWIDDLE*)fft_lut_r4_128_256;
    static constexpr TT_TWIDDLE* __restrict tw256_512 =
        fnGetR4TwPtr<TT_TWIDDLE, TP_POINT_SIZE, 256, TP_TWIDDLE_MODE, TP_DYN_PT_SIZE, TP_START_RANK,
                     TP_END_RANK>(); //(TT_TWIDDLE*)fft_lut_r4_256_512;
    static constexpr TT_TWIDDLE* __restrict tw512_1024 =
        fnGetR4TwPtr<TT_TWIDDLE, TP_POINT_SIZE, 512, TP_TWIDDLE_MODE, TP_DYN_PT_SIZE, TP_START_RANK,
                     TP_END_RANK>(); //(TT_TWIDDLE*)fft_lut_r4_512_1024;
    static constexpr TT_TWIDDLE* __restrict tw1024_2048 =
        fnGetR4TwPtr<TT_TWIDDLE, TP_POINT_SIZE, 1024, TP_TWIDDLE_MODE, TP_DYN_PT_SIZE, TP_START_RANK,
                     TP_END_RANK>(); //(TT_TWIDDLE*)fft_lut_r4_1024_2048;
#endif                               //__FFT_R4_IMPL__ == 1

    alignas(__ALIGN_BYTE_SIZE__) static TT_TWIDDLE* tw_table[kMaxPointLog * 2] = {
        tw1,   tw2,   tw4,   tw8,    tw16,    tw32,    tw64,     tw128,     tw256,     tw512,      tw1024,      tw2048,
        tw1_2, tw2_4, tw4_8, tw8_16, tw16_32, tw32_64, tw64_128, tw128_256, tw256_512, tw512_1024, tw1024_2048, NULL};

    TT_DATA* xbuff = inptr;
    T_internalDataType* tmp1_buf = NULL;
    T_internalDataType* tmp2_buf = NULL;

    // assign tmp1_buf
    if
        constexpr(TP_POINT_SIZE == 4096) { tmp1_buf = (T_internalDataType*)fft_4096_tmp1; }
    else if
        constexpr(TP_POINT_SIZE == 2048) { tmp1_buf = (T_internalDataType*)fft_2048_tmp1; }
    else if
        constexpr(TP_POINT_SIZE == 1024) { tmp1_buf = (T_internalDataType*)fft_1024_tmp1; }
    else if
        constexpr(TP_POINT_SIZE == 512) { tmp1_buf = (T_internalDataType*)fft_512_tmp1; }
    else if
        constexpr(TP_POINT_SIZE == 256) { tmp1_buf = (T_internalDataType*)fft_256_tmp1; }
    else {
        tmp1_buf = (T_internalDataType*)fft_128_tmp1;
    }

    // assign tmp2_buf
    if
        constexpr(std::is_same<TT_DATA, cint16>::value) {
            if
                constexpr(TP_POINT_SIZE == 4096) { tmp2_buf = (T_internalDataType*)fft_4096_tmp2; }
            else if
                constexpr(TP_POINT_SIZE == 2048) { tmp2_buf = (T_internalDataType*)fft_2048_tmp2; }
            else if
                constexpr(TP_POINT_SIZE == 1024) { tmp2_buf = (T_internalDataType*)fft_1024_tmp2; }
            else if
                constexpr(TP_POINT_SIZE == 512) { tmp2_buf = (T_internalDataType*)fft_512_tmp2; }
            else if
                constexpr(TP_POINT_SIZE == 256) { tmp2_buf = (T_internalDataType*)fft_256_tmp2; }
            else {
                tmp2_buf = (T_internalDataType*)fft_128_tmp2;
            }
        }
    else {
        tmp2_buf = (T_internalDataType*)xbuff;
    }
    stages.stagePreamble(tw_table, tmp1_buf, tmp2_buf, inptr, outptr);
};

template <typename TT_DATA,
          typename TT_OUT_DATA,
          typename TT_TWIDDLE,
          unsigned int TP_FFT_NIFFT,
          unsigned int TP_SHIFT,
          unsigned int TP_START_RANK,
          unsigned int TP_END_RANK,
          unsigned int TP_DYN_PT_SIZE,
          unsigned int TP_WINDOW_VSIZE,
          unsigned int TP_ORIG_PAR_POWER,
          unsigned int TP_TWIDDLE_MODE>
INLINE_DECL void kernelFFTClass<TT_DATA,
                                TT_OUT_DATA,
                                TT_TWIDDLE,
                                FFT16_SIZE,
                                TP_FFT_NIFFT,
                                TP_SHIFT,
                                TP_START_RANK,
                                TP_END_RANK,
                                TP_DYN_PT_SIZE,
                                TP_WINDOW_VSIZE,
                                TP_ORIG_PAR_POWER,
                                TP_TWIDDLE_MODE>::kernelFFT(TT_DATA* __restrict inptr, TT_OUT_DATA* __restrict outptr) {
    const unsigned int TP_POINT_SIZE = FFT16_SIZE;

    typedef typename std::conditional<std::is_same<TT_DATA, cint16>::value, cint32_t, TT_DATA>::type T_internalDataType;
    static constexpr TT_TWIDDLE* __restrict tw1 =
        fnGetTwPtr<TT_TWIDDLE, TP_POINT_SIZE, 1, TP_TWIDDLE_MODE, TP_DYN_PT_SIZE, TP_START_RANK, TP_END_RANK>();
    static constexpr TT_TWIDDLE* __restrict tw2 =
        fnGetTwPtr<TT_TWIDDLE, TP_POINT_SIZE, 2, TP_TWIDDLE_MODE, TP_DYN_PT_SIZE, TP_START_RANK, TP_END_RANK>();
    static constexpr TT_TWIDDLE* __restrict tw4 =
        fnGetTwPtr<TT_TWIDDLE, TP_POINT_SIZE, 4, TP_TWIDDLE_MODE, TP_DYN_PT_SIZE, TP_START_RANK, TP_END_RANK>();
    static constexpr TT_TWIDDLE* __restrict tw8 =
        fnGetTwPtr<TT_TWIDDLE, TP_POINT_SIZE, 8, TP_TWIDDLE_MODE, TP_DYN_PT_SIZE, TP_START_RANK, TP_END_RANK>();

#if __FFT_R4_IMPL__ == 0
    static constexpr TT_TWIDDLE* __restrict tw1_2 = NULL;
    static constexpr TT_TWIDDLE* __restrict tw2_4 = NULL;
    static constexpr TT_TWIDDLE* __restrict tw4_8 = NULL;
    static constexpr TT_TWIDDLE* __restrict tw8_16 = NULL;
    static constexpr TT_TWIDDLE* __restrict tw16_32 = NULL;
#endif //__FFT_R4_IMPL__ == 0
#if __FFT_R4_IMPL__ == 1
    static constexpr TT_TWIDDLE* __restrict tw1_2 =
        fnGetR4TwPtr<TT_TWIDDLE, TP_POINT_SIZE, 1, TP_TWIDDLE_MODE, TP_DYN_PT_SIZE, TP_START_RANK, TP_END_RANK>();
    static constexpr TT_TWIDDLE* __restrict tw2_4 =
        fnGetR4TwPtr<TT_TWIDDLE, TP_POINT_SIZE, 2, TP_TWIDDLE_MODE, TP_DYN_PT_SIZE, TP_START_RANK, TP_END_RANK>();
    static constexpr TT_TWIDDLE* __restrict tw4_8 =
        fnGetR4TwPtr<TT_TWIDDLE, TP_POINT_SIZE, 4, TP_TWIDDLE_MODE, TP_DYN_PT_SIZE, TP_START_RANK, TP_END_RANK>();
    static constexpr TT_TWIDDLE* __restrict tw8_16 =
        fnGetR4TwPtr<TT_TWIDDLE, TP_POINT_SIZE, 8, TP_TWIDDLE_MODE, TP_DYN_PT_SIZE, TP_START_RANK, TP_END_RANK>();
#endif //__FFT_R4_IMPL__ == 1

    alignas(__ALIGN_BYTE_SIZE__) static TT_TWIDDLE* tw_table[kMaxPointLog * 2] = {
        tw1,   tw2,   tw4,   tw8,    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
        tw1_2, tw2_4, tw4_8, tw8_16, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};

    T_internalDataType* ktmp1_buf = (T_internalDataType*)fft_128_tmp1; // works because cint32 is the same size as
                                                                       // cfloat
    T_internalDataType* ktmp2_buf; // not initialized because input window is re-used for this storage

    TT_DATA* xbuff = inptr;
    TT_OUT_DATA* obuff = outptr;
    T_internalDataType* tmp1_buf = (T_internalDataType*)fft_128_tmp1;
    // The following break with the pattern for other point sizes is a workaround for a compiler issue.
    alignas(__ALIGN_BYTE_SIZE__) T_internalDataType tmp2_buf[FFT16_SIZE]; // must be 256 bit aligned
    alignas(__ALIGN_BYTE_SIZE__) T_internalDataType tmp3_buf[FFT16_SIZE]; // must be 256 bit aligned

    bool inv = TP_FFT_NIFFT == 1 ? false : true;

    for (int iter = 0; iter < TP_WINDOW_VSIZE / TP_POINT_SIZE; iter++)
        chess_prepare_for_pipelining chess_loop_range(TP_WINDOW_VSIZE / TP_POINT_SIZE, ) {
            if
                constexpr(std::is_same<TT_DATA, cfloat>::value &&
                          (TP_END_RANK - TP_START_RANK == 4)) { // special case for uncascaded 16pt cfloat
                    stage_radix2_dit<cfloat, cfloat, cfloat, 8, 0 /*TP_TWIDDLE_MODE*/>(
                        (cfloat*)xbuff, (cfloat*)tw1, FFT16_SIZE, 0, (cfloat*)tmp1_buf, inv);
                    stage_radix2_dit<cfloat, cfloat, cfloat, 4, 0 /*TP_TWIDDLE_MODE*/>(
                        (cfloat*)tmp1_buf, (cfloat*)tw2, FFT16_SIZE, 0, (cfloat*)tmp2_buf, inv);
                    stage_radix2_dit<cfloat, cfloat, cfloat, 2, 0 /*TP_TWIDDLE_MODE*/>(
                        (cfloat*)tmp2_buf, (cfloat*)tw4, FFT16_SIZE, 0, (cfloat*)tmp3_buf, inv);
                    stage_radix2_dit<cfloat, cfloat, cfloat, 1, 0 /*TP_TWIDDLE_MODE*/>(
                        (cfloat*)tmp3_buf, (cfloat*)tw8, FFT16_SIZE, TP_SHIFT, (cfloat*)obuff, inv); // r is not used.
                }
            else {
                stages.calc(xbuff, tw_table, tmp1_buf, tmp2_buf, obuff);
            }
            xbuff += TP_POINT_SIZE;
            obuff += TP_POINT_SIZE;
        }
};

template <typename TT_OUT_DATA,
          unsigned int TP_FFT_NIFFT,
          unsigned int TP_SHIFT,
          unsigned int TP_START_RANK,
          unsigned int TP_END_RANK,
          unsigned int TP_DYN_PT_SIZE,
          unsigned int TP_WINDOW_VSIZE,
          unsigned int TP_ORIG_PAR_POWER,
          unsigned int TP_TWIDDLE_MODE>
INLINE_DECL void kernelFFTClass<cint16,
                                TT_OUT_DATA,
                                cint16,
                                FFT16_SIZE,
                                TP_FFT_NIFFT,
                                TP_SHIFT,
                                TP_START_RANK,
                                TP_END_RANK,
                                TP_DYN_PT_SIZE,
                                TP_WINDOW_VSIZE,
                                TP_ORIG_PAR_POWER,
                                TP_TWIDDLE_MODE>::kernelFFT(cint16* __restrict inptr, TT_OUT_DATA* __restrict outptr) {
    const unsigned int TP_POINT_SIZE = FFT16_SIZE;
    typedef cint16 TT_DATA;
    typedef cint16 TT_TWIDDLE; // essentially a constant for thie specialization
    typedef cint32_t T_internalDataType;
    static constexpr cint16* __restrict tw1 =
        fnGetTwPtr<TT_TWIDDLE, TP_POINT_SIZE, 1, TP_TWIDDLE_MODE, TP_DYN_PT_SIZE, TP_START_RANK, TP_END_RANK>();
    static constexpr cint16* __restrict tw2 =
        fnGetTwPtr<TT_TWIDDLE, TP_POINT_SIZE, 2, TP_TWIDDLE_MODE, TP_DYN_PT_SIZE, TP_START_RANK, TP_END_RANK>();
    static constexpr cint16* __restrict tw4 =
        fnGetTwPtr<TT_TWIDDLE, TP_POINT_SIZE, 4, TP_TWIDDLE_MODE, TP_DYN_PT_SIZE, TP_START_RANK, TP_END_RANK>();
    static constexpr cint16* __restrict tw8 =
        fnGetTwPtr<TT_TWIDDLE, TP_POINT_SIZE, 8, TP_TWIDDLE_MODE, TP_DYN_PT_SIZE, TP_START_RANK, TP_END_RANK>();

#if __FFT_R4_IMPL__ == 0
    static constexpr TT_TWIDDLE* __restrict tw1_2 = NULL;
    static constexpr TT_TWIDDLE* __restrict tw2_4 = NULL;
    static constexpr TT_TWIDDLE* __restrict tw4_8 = NULL;
    static constexpr TT_TWIDDLE* __restrict tw8_16 = NULL;
#endif //__FFT_R4_IMPL__ == 0
#if __FFT_R4_IMPL__ == 1
    static constexpr TT_TWIDDLE* __restrict tw1_2 =
        fnGetR4TwPtr<TT_TWIDDLE, TP_POINT_SIZE, 1, TP_TWIDDLE_MODE, TP_DYN_PT_SIZE, TP_START_RANK, TP_END_RANK>();
    static constexpr TT_TWIDDLE* __restrict tw2_4 =
        fnGetR4TwPtr<TT_TWIDDLE, TP_POINT_SIZE, 2, TP_TWIDDLE_MODE, TP_DYN_PT_SIZE, TP_START_RANK, TP_END_RANK>();
    static constexpr TT_TWIDDLE* __restrict tw4_8 =
        fnGetR4TwPtr<TT_TWIDDLE, TP_POINT_SIZE, 4, TP_TWIDDLE_MODE, TP_DYN_PT_SIZE, TP_START_RANK, TP_END_RANK>();
    static constexpr TT_TWIDDLE* __restrict tw8_16 =
        fnGetR4TwPtr<TT_TWIDDLE, TP_POINT_SIZE, 8, TP_TWIDDLE_MODE, TP_DYN_PT_SIZE, TP_START_RANK, TP_END_RANK>();
#endif //__FFT_R4_IMPL__ == 1

    alignas(__ALIGN_BYTE_SIZE__) static cint16* tw_table[kMaxPointLog * 2] = {
        tw1,   tw2,   tw4,   tw8,    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
        tw1_2, tw2_4, tw4_8, tw8_16, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};

    T_internalDataType* ktmp1_buf = (T_internalDataType*)
        fft_128_tmp1; // all pt sizes 128 or smaller use 128 sample buffer to keep codebase smaller.
    T_internalDataType* ktmp2_buf = (T_internalDataType*)fft_128_tmp2;

    cint16* xbuff = inptr;
    TT_OUT_DATA* obuff = outptr;
    T_internalDataType* tmp1_buf = (T_internalDataType*)ktmp1_buf;
    T_internalDataType* tmp2_buf = (T_internalDataType*)ktmp2_buf;
    bool inv;
    if
        constexpr(TP_FFT_NIFFT == 1) { inv = false; }
    else {
        inv = true;
    }

    for (int iter = 0; iter < TP_WINDOW_VSIZE / TP_POINT_SIZE; iter++)
        chess_prepare_for_pipelining chess_loop_range(TP_WINDOW_VSIZE / TP_POINT_SIZE, ) {
            stages.calc(xbuff, tw_table, tmp1_buf, tmp2_buf, obuff);
            xbuff += TP_POINT_SIZE;
            obuff += TP_POINT_SIZE;
        }
};

//-----------------------------------------------------------------------------------------------------
// top level kernel entry function
template <typename TT_DATA,
          typename TT_OUT_DATA,
          typename TT_TWIDDLE,
          unsigned int TP_POINT_SIZE,
          unsigned int TP_FFT_NIFFT,
          unsigned int TP_SHIFT,
          unsigned int TP_START_RANK,
          unsigned int TP_END_RANK,
          unsigned int TP_DYN_PT_SIZE,
          unsigned int TP_WINDOW_VSIZE,
          unsigned int TP_IN_API,
          unsigned int TP_OUT_API,
          unsigned int TP_ORIG_PAR_POWER,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_TWIDDLE_MODE>
NOINLINE_DECL void fft_ifft_dit_1ch<TT_DATA,
                                    TT_OUT_DATA,
                                    TT_TWIDDLE,
                                    TP_POINT_SIZE,
                                    TP_FFT_NIFFT,
                                    TP_SHIFT,
                                    TP_START_RANK,
                                    TP_END_RANK,
                                    TP_DYN_PT_SIZE,
                                    TP_WINDOW_VSIZE,
                                    TP_IN_API,
                                    TP_OUT_API,
                                    TP_ORIG_PAR_POWER,
                                    TP_RND,
                                    TP_SAT,
                                    TP_TWIDDLE_MODE>::fftMain(input_buffer<TT_DATA>& __restrict inWindow,
                                                              output_buffer<TT_OUT_DATA>& __restrict outWindow) {
    TT_DATA* inptr = (TT_DATA*)inWindow.data();
    TT_OUT_DATA* outptr = (TT_OUT_DATA*)outWindow.data();

    set_rnd_mode<TP_RND>();
    set_sat_mode<TP_SAT>();

    m_fftKernel.kernelFFT(inptr, outptr);
};

#if __STREAMS_PER_TILE__ == 2
// specialization for streams in, window/buffer out
template <typename TT_DATA,
          typename TT_OUT_DATA,
          typename TT_TWIDDLE,
          unsigned int TP_POINT_SIZE,
          unsigned int TP_FFT_NIFFT,
          unsigned int TP_SHIFT,
          unsigned int TP_START_RANK,
          unsigned int TP_END_RANK,
          unsigned int TP_DYN_PT_SIZE,
          unsigned int TP_WINDOW_VSIZE,
          unsigned int TP_ORIG_PAR_POWER,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_TWIDDLE_MODE>
NOINLINE_DECL void fft_ifft_dit_1ch<TT_DATA,
                                    TT_OUT_DATA,
                                    TT_TWIDDLE,
                                    TP_POINT_SIZE,
                                    TP_FFT_NIFFT,
                                    TP_SHIFT,
                                    TP_START_RANK,
                                    TP_END_RANK,
                                    TP_DYN_PT_SIZE,
                                    TP_WINDOW_VSIZE,
                                    kStreamAPI,
                                    kWindowAPI,
                                    TP_ORIG_PAR_POWER,
                                    TP_RND,
                                    TP_SAT,
                                    TP_TWIDDLE_MODE>::fftMain(input_stream<TT_DATA>* __restrict inStream0,
                                                              input_stream<TT_DATA>* __restrict inStream1,
                                                              output_buffer<TT_OUT_DATA>& __restrict outWindow) {
    TT_DATA* __restrict inPtr = &inBuff[0];
    TT_OUT_DATA* __restrict outPtr = (TT_OUT_DATA*)outWindow.data();

    set_rnd_mode<TP_RND>();
    set_sat_mode<TP_SAT>();

    readStreamIn<TT_DATA, TP_DYN_PT_SIZE, TP_WINDOW_VSIZE>(inStream0, inStream1, inPtr);
    chess_memory_fence();
    m_fftKernel.kernelFFT(inPtr, outPtr);
};
#else  //_STREAMS_PER_TILE__ == 2

// specialization / overload for single streams in, window/buffer out
template <typename TT_DATA,
          typename TT_OUT_DATA,
          typename TT_TWIDDLE,
          unsigned int TP_POINT_SIZE,
          unsigned int TP_FFT_NIFFT,
          unsigned int TP_SHIFT,
          unsigned int TP_START_RANK,
          unsigned int TP_END_RANK,
          unsigned int TP_DYN_PT_SIZE,
          unsigned int TP_WINDOW_VSIZE,
          unsigned int TP_ORIG_PAR_POWER,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_TWIDDLE_MODE>
NOINLINE_DECL void fft_ifft_dit_1ch<TT_DATA,
                                    TT_OUT_DATA,
                                    TT_TWIDDLE,
                                    TP_POINT_SIZE,
                                    TP_FFT_NIFFT,
                                    TP_SHIFT,
                                    TP_START_RANK,
                                    TP_END_RANK,
                                    TP_DYN_PT_SIZE,
                                    TP_WINDOW_VSIZE,
                                    kStreamAPI,
                                    kWindowAPI,
                                    TP_ORIG_PAR_POWER,
                                    TP_RND,
                                    TP_SAT,
                                    TP_TWIDDLE_MODE>::fftMain(input_stream<TT_DATA>* __restrict inStream0,
                                                              output_buffer<TT_OUT_DATA>& __restrict outWindow) {
    TT_DATA* __restrict inPtr = &inBuff[0];
    TT_OUT_DATA* __restrict outPtr = (TT_OUT_DATA*)outWindow.data();

    set_rnd_mode<TP_RND>();
    set_sat_mode<TP_SAT>();

    readStreamIn<TT_DATA, TP_DYN_PT_SIZE, TP_WINDOW_VSIZE>(inStream0, inPtr);
    chess_memory_fence();
    m_fftKernel.kernelFFT(inPtr, outPtr);
};
#endif //_STREAMS_PER_TILE__ == 2

#if __STREAMS_PER_TILE__ == 2
// specialization for window/buffer in, streams out
template <typename TT_DATA,
          typename TT_OUT_DATA,
          typename TT_TWIDDLE,
          unsigned int TP_POINT_SIZE,
          unsigned int TP_FFT_NIFFT,
          unsigned int TP_SHIFT,
          unsigned int TP_START_RANK,
          unsigned int TP_END_RANK,
          unsigned int TP_DYN_PT_SIZE,
          unsigned int TP_WINDOW_VSIZE,
          unsigned int TP_ORIG_PAR_POWER,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_TWIDDLE_MODE>
NOINLINE_DECL void fft_ifft_dit_1ch<TT_DATA,
                                    TT_OUT_DATA,
                                    TT_TWIDDLE,
                                    TP_POINT_SIZE,
                                    TP_FFT_NIFFT,
                                    TP_SHIFT,
                                    TP_START_RANK,
                                    TP_END_RANK,
                                    TP_DYN_PT_SIZE,
                                    TP_WINDOW_VSIZE,
                                    kWindowAPI,
                                    kStreamAPI,
                                    TP_ORIG_PAR_POWER,
                                    TP_RND,
                                    TP_SAT,
                                    TP_TWIDDLE_MODE>::fftMain(input_buffer<TT_DATA>& __restrict inWindow,
                                                              output_stream<TT_OUT_DATA>* __restrict outStream0,
                                                              output_stream<TT_OUT_DATA>* __restrict outStream1) {
    TT_DATA* __restrict inPtr = (TT_DATA*)inWindow.data();
    TT_OUT_DATA* __restrict outPtr = &outBuff[0];

    set_rnd_mode<TP_RND>();
    set_sat_mode<TP_SAT>();

    m_fftKernel.kernelFFT(inPtr, outPtr);
    chess_memory_fence();
    writeStreamOut<TT_OUT_DATA, TP_DYN_PT_SIZE, TP_WINDOW_VSIZE>(outStream0, outStream1, outPtr);
};
#else  //_STREAMS_PER_TILE__ == 2

// specialization/overload for window/buffer in, single stream out
template <typename TT_DATA,
          typename TT_OUT_DATA,
          typename TT_TWIDDLE,
          unsigned int TP_POINT_SIZE,
          unsigned int TP_FFT_NIFFT,
          unsigned int TP_SHIFT,
          unsigned int TP_START_RANK,
          unsigned int TP_END_RANK,
          unsigned int TP_DYN_PT_SIZE,
          unsigned int TP_WINDOW_VSIZE,
          unsigned int TP_ORIG_PAR_POWER,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_TWIDDLE_MODE>
NOINLINE_DECL void fft_ifft_dit_1ch<TT_DATA,
                                    TT_OUT_DATA,
                                    TT_TWIDDLE,
                                    TP_POINT_SIZE,
                                    TP_FFT_NIFFT,
                                    TP_SHIFT,
                                    TP_START_RANK,
                                    TP_END_RANK,
                                    TP_DYN_PT_SIZE,
                                    TP_WINDOW_VSIZE,
                                    kWindowAPI,
                                    kStreamAPI,
                                    TP_ORIG_PAR_POWER,
                                    TP_RND,
                                    TP_SAT,
                                    TP_TWIDDLE_MODE>::fftMain(input_buffer<TT_DATA>& __restrict inWindow,
                                                              output_stream<TT_OUT_DATA>* __restrict outStream0) {
    TT_DATA* __restrict inPtr = (TT_DATA*)inWindow.data();
    TT_OUT_DATA* __restrict outPtr = &outBuff[0];

    set_rnd_mode<TP_RND>();
    set_sat_mode<TP_SAT>();

    m_fftKernel.kernelFFT(inPtr, outPtr);
    chess_memory_fence();
    writeStreamOut<TT_OUT_DATA, TP_DYN_PT_SIZE, TP_WINDOW_VSIZE>(outStream0, outPtr);
};
#endif //_STREAMS_PER_TILE__ == 2

#if __STREAMS_PER_TILE__ == 2
// specialization for streams in, streams out
template <typename TT_DATA,
          typename TT_OUT_DATA,
          typename TT_TWIDDLE,
          unsigned int TP_POINT_SIZE,
          unsigned int TP_FFT_NIFFT,
          unsigned int TP_SHIFT,
          unsigned int TP_START_RANK,
          unsigned int TP_END_RANK,
          unsigned int TP_DYN_PT_SIZE,
          unsigned int TP_WINDOW_VSIZE,
          unsigned int TP_ORIG_PAR_POWER,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_TWIDDLE_MODE>
NOINLINE_DECL void fft_ifft_dit_1ch<TT_DATA,
                                    TT_OUT_DATA,
                                    TT_TWIDDLE,
                                    TP_POINT_SIZE,
                                    TP_FFT_NIFFT,
                                    TP_SHIFT,
                                    TP_START_RANK,
                                    TP_END_RANK,
                                    TP_DYN_PT_SIZE,
                                    TP_WINDOW_VSIZE,
                                    kStreamAPI,
                                    kStreamAPI,
                                    TP_ORIG_PAR_POWER,
                                    TP_RND,
                                    TP_SAT,
                                    TP_TWIDDLE_MODE>::fftMain(input_stream<TT_DATA>* __restrict inStream0,
                                                              input_stream<TT_DATA>* __restrict inStream1,
                                                              output_stream<TT_OUT_DATA>* __restrict outStream0,
                                                              output_stream<TT_OUT_DATA>* __restrict outStream1) {
    TT_DATA* __restrict inPtr = &inBuff[0];
    TT_OUT_DATA* __restrict outPtr = &outBuff[0];

    set_rnd_mode<TP_RND>();
    set_sat_mode<TP_SAT>();

    readStreamIn<TT_DATA, TP_DYN_PT_SIZE, TP_WINDOW_VSIZE>(inStream0, inStream1, inPtr);
    chess_memory_fence();
    m_fftKernel.kernelFFT(inPtr, outPtr);
    chess_memory_fence();
    writeStreamOut<TT_OUT_DATA, TP_DYN_PT_SIZE, TP_WINDOW_VSIZE>(outStream0, outStream1, outPtr);
};
#else  //_STREAMS_PER_TILE__ == 2

// specialization/overload for single streams in, single stream out
template <typename TT_DATA,
          typename TT_OUT_DATA,
          typename TT_TWIDDLE,
          unsigned int TP_POINT_SIZE,
          unsigned int TP_FFT_NIFFT,
          unsigned int TP_SHIFT,
          unsigned int TP_START_RANK,
          unsigned int TP_END_RANK,
          unsigned int TP_DYN_PT_SIZE,
          unsigned int TP_WINDOW_VSIZE,
          unsigned int TP_ORIG_PAR_POWER,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_TWIDDLE_MODE>
NOINLINE_DECL void fft_ifft_dit_1ch<TT_DATA,
                                    TT_OUT_DATA,
                                    TT_TWIDDLE,
                                    TP_POINT_SIZE,
                                    TP_FFT_NIFFT,
                                    TP_SHIFT,
                                    TP_START_RANK,
                                    TP_END_RANK,
                                    TP_DYN_PT_SIZE,
                                    TP_WINDOW_VSIZE,
                                    kStreamAPI,
                                    kStreamAPI,
                                    TP_ORIG_PAR_POWER,
                                    TP_RND,
                                    TP_SAT,
                                    TP_TWIDDLE_MODE>::fftMain(input_stream<TT_DATA>* __restrict inStream0,
                                                              output_stream<TT_OUT_DATA>* __restrict outStream0) {
    TT_DATA* __restrict inPtr = &inBuff[0];
    TT_OUT_DATA* __restrict outPtr = &outBuff[0];

    set_rnd_mode<TP_RND>();
    set_sat_mode<TP_SAT>();

    readStreamIn<TT_DATA, TP_DYN_PT_SIZE, TP_WINDOW_VSIZE>(inStream0, inPtr);
    chess_memory_fence();
    m_fftKernel.kernelFFT(inPtr, outPtr);
    chess_memory_fence();
    writeStreamOut<TT_OUT_DATA, TP_DYN_PT_SIZE, TP_WINDOW_VSIZE>(outStream0, outPtr);
};
#endif //_STREAMS_PER_TILE__ == 2

// specialization/overload for single streams in, casc/stream out
template <typename TT_DATA,
          typename TT_OUT_DATA,
          typename TT_TWIDDLE,
          unsigned int TP_POINT_SIZE,
          unsigned int TP_FFT_NIFFT,
          unsigned int TP_SHIFT,
          unsigned int TP_START_RANK,
          unsigned int TP_END_RANK,
          unsigned int TP_DYN_PT_SIZE,
          unsigned int TP_WINDOW_VSIZE,
          unsigned int TP_ORIG_PAR_POWER,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_TWIDDLE_MODE>
NOINLINE_DECL void fft_ifft_dit_1ch<
    TT_DATA,
    TT_OUT_DATA,
    TT_TWIDDLE,
    TP_POINT_SIZE,
    TP_FFT_NIFFT,
    TP_SHIFT,
    TP_START_RANK,
    TP_END_RANK,
    TP_DYN_PT_SIZE,
    TP_WINDOW_VSIZE,
    kStreamAPI,
    kCascStreamAPI,
    TP_ORIG_PAR_POWER,
    TP_RND,
    TP_SAT,
    TP_TWIDDLE_MODE>::fftMain(input_stream<TT_DATA>* __restrict inStream0,
                              output_cascade<typename t_accType<TT_DATA>::type>* __restrict outStream0, // Cascade
                              output_stream<TT_OUT_DATA>* __restrict outStream1) {
    TT_DATA* __restrict inPtr = &inBuff[0];
    TT_OUT_DATA* __restrict outPtr = &outBuff[0];

    set_rnd_mode<TP_RND>();
    set_sat_mode<TP_SAT>();

    readStreamIn<TT_DATA, TP_DYN_PT_SIZE, TP_WINDOW_VSIZE>(inStream0, inPtr);
    chess_memory_fence();
    m_fftKernel.kernelFFT(inPtr, outPtr);
    chess_memory_fence();
    writeCascStreamOut<TT_OUT_DATA, TP_DYN_PT_SIZE, TP_WINDOW_VSIZE>(outStream0, outStream1, outPtr);
};

// specialization/overload for iobuffer in, casc/stream out
template <typename TT_DATA,
          typename TT_OUT_DATA,
          typename TT_TWIDDLE,
          unsigned int TP_POINT_SIZE,
          unsigned int TP_FFT_NIFFT,
          unsigned int TP_SHIFT,
          unsigned int TP_START_RANK,
          unsigned int TP_END_RANK,
          unsigned int TP_DYN_PT_SIZE,
          unsigned int TP_WINDOW_VSIZE,
          unsigned int TP_ORIG_PAR_POWER,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_TWIDDLE_MODE>
NOINLINE_DECL void fft_ifft_dit_1ch<
    TT_DATA,
    TT_OUT_DATA,
    TT_TWIDDLE,
    TP_POINT_SIZE,
    TP_FFT_NIFFT,
    TP_SHIFT,
    TP_START_RANK,
    TP_END_RANK,
    TP_DYN_PT_SIZE,
    TP_WINDOW_VSIZE,
    kWindowAPI,
    kCascStreamAPI,
    TP_ORIG_PAR_POWER,
    TP_RND,
    TP_SAT,
    TP_TWIDDLE_MODE>::fftMain(input_buffer<TT_DATA>& __restrict inWindow0,
                              output_cascade<typename t_accType<TT_DATA>::type>* __restrict outStream0, // Cascade
                              output_stream<TT_OUT_DATA>* __restrict outStream1) {
    TT_DATA* __restrict inPtr = (TT_DATA*)inWindow0.data();
    TT_OUT_DATA* __restrict outPtr = &outBuff[0];

    set_rnd_mode<TP_RND>();
    set_sat_mode<TP_SAT>();

    m_fftKernel.kernelFFT(inPtr, outPtr);
    chess_memory_fence();
    writeCascStreamOut<TT_OUT_DATA, TP_DYN_PT_SIZE, TP_WINDOW_VSIZE>(outStream0, outStream1, outPtr);
};

// specialization/overload for single streams in, stream/casc out
template <typename TT_DATA,
          typename TT_OUT_DATA,
          typename TT_TWIDDLE,
          unsigned int TP_POINT_SIZE,
          unsigned int TP_FFT_NIFFT,
          unsigned int TP_SHIFT,
          unsigned int TP_START_RANK,
          unsigned int TP_END_RANK,
          unsigned int TP_DYN_PT_SIZE,
          unsigned int TP_WINDOW_VSIZE,
          unsigned int TP_ORIG_PAR_POWER,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_TWIDDLE_MODE>
NOINLINE_DECL void fft_ifft_dit_1ch<
    TT_DATA,
    TT_OUT_DATA,
    TT_TWIDDLE,
    TP_POINT_SIZE,
    TP_FFT_NIFFT,
    TP_SHIFT,
    TP_START_RANK,
    TP_END_RANK,
    TP_DYN_PT_SIZE,
    TP_WINDOW_VSIZE,
    kStreamAPI,
    kStreamCascAPI,
    TP_ORIG_PAR_POWER,
    TP_RND,
    TP_SAT,
    TP_TWIDDLE_MODE>::fftMain(input_stream<TT_DATA>* __restrict inStream0,
                              output_stream<TT_OUT_DATA>* __restrict outStream0,
                              output_cascade<typename t_accType<TT_DATA>::type>* __restrict outStream1 // Cascade
                              ) {
    TT_DATA* __restrict inPtr = &inBuff[0];
    TT_OUT_DATA* __restrict outPtr = &outBuff[0];

    set_rnd_mode<TP_RND>();
    set_sat_mode<TP_SAT>();

    readStreamIn<TT_DATA, TP_DYN_PT_SIZE, TP_WINDOW_VSIZE>(inStream0, inPtr);
    chess_memory_fence();
    m_fftKernel.kernelFFT(inPtr, outPtr);
    chess_memory_fence();
    writeStreamCascOut<TT_OUT_DATA, TP_DYN_PT_SIZE, TP_WINDOW_VSIZE>(outStream0, outStream1, outPtr);
};

// specialization/overload for iobuffer in, stream/casc out
template <typename TT_DATA,
          typename TT_OUT_DATA,
          typename TT_TWIDDLE,
          unsigned int TP_POINT_SIZE,
          unsigned int TP_FFT_NIFFT,
          unsigned int TP_SHIFT,
          unsigned int TP_START_RANK,
          unsigned int TP_END_RANK,
          unsigned int TP_DYN_PT_SIZE,
          unsigned int TP_WINDOW_VSIZE,
          unsigned int TP_ORIG_PAR_POWER,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_TWIDDLE_MODE>
NOINLINE_DECL void fft_ifft_dit_1ch<
    TT_DATA,
    TT_OUT_DATA,
    TT_TWIDDLE,
    TP_POINT_SIZE,
    TP_FFT_NIFFT,
    TP_SHIFT,
    TP_START_RANK,
    TP_END_RANK,
    TP_DYN_PT_SIZE,
    TP_WINDOW_VSIZE,
    kWindowAPI,
    kStreamCascAPI,
    TP_ORIG_PAR_POWER,
    TP_RND,
    TP_SAT,
    TP_TWIDDLE_MODE>::fftMain(input_buffer<TT_DATA>& __restrict inWindow0,
                              output_stream<TT_OUT_DATA>* __restrict outStream0,
                              output_cascade<typename t_accType<TT_DATA>::type>* __restrict outStream1 // Cascade
                              ) {
    TT_DATA* __restrict inPtr = (TT_DATA*)inWindow0.data();
    TT_OUT_DATA* __restrict outPtr = &outBuff[0];

    set_rnd_mode<TP_RND>();
    set_sat_mode<TP_SAT>();

    m_fftKernel.kernelFFT(inPtr, outPtr);
    chess_memory_fence();
    writeStreamCascOut<TT_OUT_DATA, TP_DYN_PT_SIZE, TP_WINDOW_VSIZE>(outStream0, outStream1, outPtr);
};
}
}
}
}
}
