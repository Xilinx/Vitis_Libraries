/*
 * Copyright 2022 Xilinx, Inc.
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
#include "fft_twiddle_lut_dit.h"
#include "fft_twiddle_lut_dit_cfloat.h"
#include "fft_r4_twiddles.h"

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
          unsigned int TP_ORIG_PAR_POWER>
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
                                TP_ORIG_PAR_POWER>::stagePreamble(TT_TWIDDLE** tw_table,
                                                                  TT_INTERNAL_DATA* tmp1_buf,
                                                                  TT_INTERNAL_DATA* tmp2_buf,
                                                                  input_window<TT_DATA>* __restrict inputx,
                                                                  output_window<TT_OUT_DATA>* __restrict outputy) {
    typedef typename std::conditional<std::is_same<TT_DATA, cint16>::value, cint32_t, TT_DATA>::type T_internalDataType;
    constexpr int minPtSizePwr = 4;
    constexpr int kOpsInWindow = TP_WINDOW_VSIZE / TP_POINT_SIZE;

    TT_DATA* xbuff = (TT_DATA*)inputx->ptr;
    TT_OUT_DATA* obuff = (TT_OUT_DATA*)outputy->ptr;
    bool inv;
    if
        constexpr(TP_FFT_NIFFT == 1) { inv = false; }
    else {
        inv = true;
    }

    // This code would be moved to the constructor preferably, but contains intrinsics, so cannot. Also, it may be
    // necessary if 2 kernels share a processor
    set_rnd(rnd_pos_inf); // Match the twiddle round mode of Matlab.
    set_sat();            // do saturate.

    if
        constexpr(TP_DYN_PT_SIZE == 1) {
            constexpr int kHeaderBytes = 32; // 256 bits.
            constexpr int kSamplesInHeader = kHeaderBytes / sizeof(TT_DATA);
            constexpr int kSamplesOutHeader = kHeaderBytes / sizeof(TT_OUT_DATA);
            TT_INTERNAL_DATA* myTmp2_buf = (TT_INTERNAL_DATA*)tmp2_buf; // when dynamic, xbuff header alters start of
                                                                        // data and tmp2_buf is a pointer to xbuff so
                                                                        // has to alter too.
            T_buff_256b<TT_DATA> header;
            TT_DATA headerVal;
            TT_OUT_DATA headerOpVal;
            T_buff_256b<TT_OUT_DATA> headerOp;
            //  ::aie::vector<TT_DATA,32/sizeof(TT_DATA)> headerRawVal;
            using in256VectorType = ::aie::vector<TT_DATA, kSamplesInHeader>;
            using outVectorType =
                ::aie::vector<TT_OUT_DATA, kSamplesInHeader>; // has to be same number of elements as input.
            using blankOutVectorType =
                ::aie::vector<TT_OUT_DATA, kSamplesOutHeader>; // has to be same number of elements as input.
            blankOutVectorType blankOp;
            in256VectorType* inPtr;
            outVectorType* outPtr;
            in256VectorType in256;
            outVectorType outVector;
            int ptSizePwr;
            int firstRank; // first for this particular point size

            ::aie::accum<cacc48, kSamplesInHeader> cacc384;

            blankOp = ::aie::zeros<TT_OUT_DATA, kSamplesOutHeader>();
            headerOp.val = ::aie::zeros<TT_OUT_DATA, kSamplesOutHeader>();
            header = window_readincr_256b(inputx);
            xbuff = (TT_DATA*)inputx->ptr;
            if
                constexpr(!fnUsePingPongIntBuffer<TT_DATA>()) myTmp2_buf = (TT_INTERNAL_DATA*)xbuff;
            else
                myTmp2_buf = (TT_INTERNAL_DATA*)tmp2_buf;
            headerVal = header.val.get(0);
            headerOpVal.real = headerVal.real; // copy/cast header to output one field at a time
            headerOpVal.imag = headerVal.imag;
            headerOp.val.set(headerOpVal, 0);
            inv = headerVal.real == 0 ? true : false;
            headerVal = header.val.get(1);
            headerOpVal.real = headerVal.real;
            headerOpVal.imag = headerVal.imag;
            headerOp.val.set(headerOpVal, 1);
            ptSizePwr = (int)headerVal.real - TP_ORIG_PAR_POWER;
            // firstRank is explained in a large comment where firstRank is defined in stockhamStages::calc (static
            // variant)
            if
                constexpr(std::is_same<TT_DATA, cfloat>::value) { firstRank = kPointSizePower - ptSizePwr; }
            else {
                firstRank = kPointSizePowerCeiled - ptSizePwr;
            }
            if ((ptSizePwr >= minPtSizePwr) && (ptSizePwr <= kPointSizePower)) {
                window_write(outputy, headerOp.val);
                window_incr(outputy, 32 / sizeof(TT_OUT_DATA));
                obuff = (TT_OUT_DATA*)outputy->ptr;
                if (TP_END_RANK <=
                    firstRank) { // i.e. no need to do any processing in this kernel as later kernels will do it all.
                    // copy input window to output window in 256bit chunks
                    inPtr = (in256VectorType*)inputx->ptr;
                    outPtr = (outVectorType*)outputy->ptr;
                    if
                        constexpr(std::is_same<TT_DATA, cfloat>::value) {
                            for (int i = 0; i < TP_WINDOW_VSIZE / kSamplesInHeader; i++) {
                                *outPtr++ = *inPtr++;
                            }
                        }
                    else {
                        // this clause handles the case with different TT_IN_DATA and TT_OUT_DATA
                        for (int i = 0; i < TP_WINDOW_VSIZE / kSamplesInHeader; i++) {
                            in256 = *inPtr++;
                            cacc384.from_vector(in256, 0);
                            outVector = cacc384.template to_vector<TT_OUT_DATA>(0);
                            *outPtr++ = outVector;
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
                headerOpVal = unitVector<TT_OUT_DATA>();              // set real to 1, imag to 0, 1 meaning invalid.
                headerOp.val.set(headerOpVal, kSamplesOutHeader - 1); // set status in the last sample of the header
                window_write(outputy, headerOp.val);
                window_incr(outputy, kSamplesOutHeader);

                // blank output window beyond header.
                blankOutVectorType* blankPtr;
                blankPtr = (blankOutVectorType*)outputy->ptr;
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
          unsigned int TP_ORIG_PAR_POWER // irrelevant to static specialization
          >
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
                                TP_ORIG_PAR_POWER>::calc(TT_DATA* __restrict xbuff,
                                                         TT_TWIDDLE** tw_table,
                                                         TT_INTERNAL_DATA* tmp1_buf,
                                                         TT_INTERNAL_DATA* tmp2_buf,
                                                         TT_OUT_DATA* __restrict obuff) {
    constexpr int kPointSizePower = fnPointSizePower<TP_POINT_SIZE>();
    bool inv = TP_FFT_NIFFT == 1 ? false : true;

    // This code should be moved to the constructor preferably
    set_rnd(rnd_pos_inf); // Match the twiddle round mode of Matlab.
    set_sat();            // do saturate.

    TT_INTERNAL_DATA* my_tmp2_buf = fnUsePingPongIntBuffer<TT_DATA>() ? tmp2_buf : (TT_INTERNAL_DATA*)xbuff;
    TT_INTERNAL_DATA* tmp_bufs[2] = {tmp1_buf, my_tmp2_buf}; // tmp2 is actually xbuff reused.
    unsigned int pingPong = 1;                               // use tmp_buf1 as initial output
    int tw_index = 0;
    int rank = 0;
    int r = 0; // r is an indication to the stage of rank.

    if
        constexpr(std::is_same<TT_DATA, cfloat>::value) {
            //-----------------------------------------------------------------------------
            // cfloat handling
            TT_DATA* inptr;
            TT_INTERNAL_DATA* outptr;

            opt_cfloat_stage<0, TP_POINT_SIZE, TP_START_RANK, TP_END_RANK>(xbuff, obuff, tmp_bufs, tw_table, pingPong,
                                                                           inv);
            opt_cfloat_stage<1, TP_POINT_SIZE, TP_START_RANK, TP_END_RANK>(xbuff, obuff, tmp_bufs, tw_table, pingPong,
                                                                           inv);
            opt_cfloat_stage<2, TP_POINT_SIZE, TP_START_RANK, TP_END_RANK>(xbuff, obuff, tmp_bufs, tw_table, pingPong,
                                                                           inv);
            opt_cfloat_stage<3, TP_POINT_SIZE, TP_START_RANK, TP_END_RANK>(xbuff, obuff, tmp_bufs, tw_table, pingPong,
                                                                           inv);
            opt_cfloat_stage<4, TP_POINT_SIZE, TP_START_RANK, TP_END_RANK>(xbuff, obuff, tmp_bufs, tw_table, pingPong,
                                                                           inv);
            opt_cfloat_stage<5, TP_POINT_SIZE, TP_START_RANK, TP_END_RANK>(xbuff, obuff, tmp_bufs, tw_table, pingPong,
                                                                           inv);
            opt_cfloat_stage<6, TP_POINT_SIZE, TP_START_RANK, TP_END_RANK>(xbuff, obuff, tmp_bufs, tw_table, pingPong,
                                                                           inv);
            opt_cfloat_stage<7, TP_POINT_SIZE, TP_START_RANK, TP_END_RANK>(xbuff, obuff, tmp_bufs, tw_table, pingPong,
                                                                           inv);
            opt_cfloat_stage<8, TP_POINT_SIZE, TP_START_RANK, TP_END_RANK>(xbuff, obuff, tmp_bufs, tw_table, pingPong,
                                                                           inv);
            opt_cfloat_stage<9, TP_POINT_SIZE, TP_START_RANK, TP_END_RANK>(xbuff, obuff, tmp_bufs, tw_table, pingPong,
                                                                           inv);
            opt_cfloat_stage<10, TP_POINT_SIZE, TP_START_RANK, TP_END_RANK>(xbuff, obuff, tmp_bufs, tw_table, pingPong,
                                                                            inv);
            opt_cfloat_stage<11, TP_POINT_SIZE, TP_START_RANK, TP_END_RANK>(xbuff, obuff, tmp_bufs, tw_table, pingPong,
                                                                            inv);
        }
    else { // integer types can use radix 4 stages

        constexpr int firstRank = kPointSizePowerCeiled - kPointSizePower;
        // The following code is a loop which is unrolled so that the loop variable is constant at compile time in each
        // iteration
        // The loop unrolls to describe the stages required for a maximum point size, rounded up to a multiple of 2 so
        // that each pass through the loop is a radix 4 stage.
        // In each iteration the code considers whether or not to execute a stage, since for small point sizes, early
        // stages with large 'r' factors are skipped.
        // Also, the code has to handle the case that for odd power point sizes the first stage will be radix2, so it
        // has to detect the first stage being one more than the current loop stage.
        // Twiddles are relative to the first stage of the point size, not the first stage of the loop, hence the
        // equation for tw index.
        // Accordingly when the first stage is r2, it will be the second r2 of the radix4 in this iteration, so stage+1
        // is necessary to describe the position.
        // A further complication is that data arrives as TT_DATA, is processed to internal stages as TT_INTERNAL_DATA
        // and output as TT_OUT_DATA.
        // Finally, the whole operation may be split over multiple kernels, which means the input or output of this
        // kernel may be TT_INTERNAL_DATA.
        // beyond finally(!), only stages in the remit of the kernel are executed.
        opt_int_stage<TT_DATA, TT_INTERNAL_DATA, TT_OUT_DATA, TT_TWIDDLE, 0, TP_POINT_SIZE, TP_SHIFT, TP_START_RANK,
                      TP_END_RANK, firstRank, kPointSizePowerCeiled, kPointSizeCeiled>(xbuff, obuff, tmp_bufs, tw_table,
                                                                                       pingPong, inv);
        opt_int_stage<TT_DATA, TT_INTERNAL_DATA, TT_OUT_DATA, TT_TWIDDLE, 2, TP_POINT_SIZE, TP_SHIFT, TP_START_RANK,
                      TP_END_RANK, firstRank, kPointSizePowerCeiled, kPointSizeCeiled>(xbuff, obuff, tmp_bufs, tw_table,
                                                                                       pingPong, inv);
        opt_int_stage<TT_DATA, TT_INTERNAL_DATA, TT_OUT_DATA, TT_TWIDDLE, 4, TP_POINT_SIZE, TP_SHIFT, TP_START_RANK,
                      TP_END_RANK, firstRank, kPointSizePowerCeiled, kPointSizeCeiled>(xbuff, obuff, tmp_bufs, tw_table,
                                                                                       pingPong, inv);
        opt_int_stage<TT_DATA, TT_INTERNAL_DATA, TT_OUT_DATA, TT_TWIDDLE, 6, TP_POINT_SIZE, TP_SHIFT, TP_START_RANK,
                      TP_END_RANK, firstRank, kPointSizePowerCeiled, kPointSizeCeiled>(xbuff, obuff, tmp_bufs, tw_table,
                                                                                       pingPong, inv);
        opt_int_stage<TT_DATA, TT_INTERNAL_DATA, TT_OUT_DATA, TT_TWIDDLE, 8, TP_POINT_SIZE, TP_SHIFT, TP_START_RANK,
                      TP_END_RANK, firstRank, kPointSizePowerCeiled, kPointSizeCeiled>(xbuff, obuff, tmp_bufs, tw_table,
                                                                                       pingPong, inv);
        opt_int_stage<TT_DATA, TT_INTERNAL_DATA, TT_OUT_DATA, TT_TWIDDLE, 10, TP_POINT_SIZE, TP_SHIFT, TP_START_RANK,
                      TP_END_RANK, firstRank, kPointSizePowerCeiled, kPointSizeCeiled>(xbuff, obuff, tmp_bufs, tw_table,
                                                                                       pingPong, inv);
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
          unsigned int TP_ORIG_PAR_POWER>
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
                                TP_ORIG_PAR_POWER>::calc(TT_DATA* __restrict xbuff,
                                                         TT_TWIDDLE** tw_table,
                                                         TT_INTERNAL_DATA* tmp1_buf,
                                                         TT_INTERNAL_DATA* tmp2_buf,
                                                         TT_OUT_DATA* __restrict obuff,
                                                         int ptSizePwr,
                                                         bool inv) {
    // This code cannot be moved to the constructor because its scope is the processor so would affect co-hosted kernels
    set_rnd(rnd_pos_inf); // Match the twiddle round mode of Matlab.
    set_sat();            // do saturate.

    TT_INTERNAL_DATA* my_tmp2_buf = fnUsePingPongIntBuffer<TT_DATA>() ? tmp2_buf : (TT_INTERNAL_DATA*)xbuff;
    TT_INTERNAL_DATA* tmp_bufs[2] = {tmp1_buf, my_tmp2_buf}; // tmp2 is actually xbuff reused.
    unsigned int pingPong = 1; // use tmp_buf1 as input or tmp_buf2? Initially tmp2 is input since this is xbuff
    int tw_index = 0;
    int rank = 0;
    int r = 0; // r is an indication to the stage of rank.
    unsigned int intR2Stages;
    unsigned int ptSize = 1 << ptSizePwr;
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

        opt_int_dyn_stage<TT_DATA, TT_INTERNAL_DATA, TT_OUT_DATA, TT_TWIDDLE, 0, TP_POINT_SIZE, TP_SHIFT, TP_START_RANK,
                          TP_END_RANK, kPointSizePower, kPointSizePowerCeiled, kPointSizeCeiled>(
            xbuff, obuff, tmp_bufs, tw_table, pingPong, inv, ptSizePwr);
        opt_int_dyn_stage<TT_DATA, TT_INTERNAL_DATA, TT_OUT_DATA, TT_TWIDDLE, 2, TP_POINT_SIZE, TP_SHIFT, TP_START_RANK,
                          TP_END_RANK, kPointSizePower, kPointSizePowerCeiled, kPointSizeCeiled>(
            xbuff, obuff, tmp_bufs, tw_table, pingPong, inv, ptSizePwr);
        opt_int_dyn_stage<TT_DATA, TT_INTERNAL_DATA, TT_OUT_DATA, TT_TWIDDLE, 4, TP_POINT_SIZE, TP_SHIFT, TP_START_RANK,
                          TP_END_RANK, kPointSizePower, kPointSizePowerCeiled, kPointSizeCeiled>(
            xbuff, obuff, tmp_bufs, tw_table, pingPong, inv, ptSizePwr);
        opt_int_dyn_stage<TT_DATA, TT_INTERNAL_DATA, TT_OUT_DATA, TT_TWIDDLE, 6, TP_POINT_SIZE, TP_SHIFT, TP_START_RANK,
                          TP_END_RANK, kPointSizePower, kPointSizePowerCeiled, kPointSizeCeiled>(
            xbuff, obuff, tmp_bufs, tw_table, pingPong, inv, ptSizePwr);
        opt_int_dyn_stage<TT_DATA, TT_INTERNAL_DATA, TT_OUT_DATA, TT_TWIDDLE, 8, TP_POINT_SIZE, TP_SHIFT, TP_START_RANK,
                          TP_END_RANK, kPointSizePower, kPointSizePowerCeiled, kPointSizeCeiled>(
            xbuff, obuff, tmp_bufs, tw_table, pingPong, inv, ptSizePwr);
        opt_int_dyn_stage<TT_DATA, TT_INTERNAL_DATA, TT_OUT_DATA, TT_TWIDDLE, 10, TP_POINT_SIZE, TP_SHIFT,
                          TP_START_RANK, TP_END_RANK, kPointSizePower, kPointSizePowerCeiled, kPointSizeCeiled>(
            xbuff, obuff, tmp_bufs, tw_table, pingPong, inv, ptSizePwr);

    } // end of float/integer handling

    if ((1 << ptSizePwr) < TP_POINT_SIZE) {
        using outVectType = ::aie::vector<TT_OUT_DATA, 32 / sizeof(TT_OUT_DATA)>;
        outVectType zerosOut = ::aie::zeros<TT_OUT_DATA, 32 / sizeof(TT_OUT_DATA)>();
        outVectType* outFillPtr = (outVectType*)obuff;
        outFillPtr += (1 << ptSizePwr) * sizeof(TT_OUT_DATA) / 32;
        int fillCycles = (TP_POINT_SIZE - (1 << ptSizePwr)) * sizeof(TT_OUT_DATA) / 32;
        for (int i = 0; i < fillCycles; i++) {
            *outFillPtr++ = zerosOut;
        }
    }
};

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
          unsigned int TP_ORIG_PAR_POWER>
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
                                TP_ORIG_PAR_POWER>::kernelFFT(input_window<TT_DATA>* __restrict inputx,
                                                              output_window<TT_OUT_DATA>* __restrict outputy){};

template <typename TT_DATA,
          typename TT_OUT_DATA,
          typename TT_TWIDDLE,
          unsigned int TP_FFT_NIFFT,
          unsigned int TP_SHIFT,
          unsigned int TP_START_RANK,
          unsigned int TP_END_RANK,
          unsigned int TP_DYN_PT_SIZE,
          unsigned int TP_WINDOW_VSIZE,
          unsigned int TP_ORIG_PAR_POWER>
INLINE_DECL void kernelFFTClass<TT_DATA,
                                TT_OUT_DATA,
                                TT_TWIDDLE,
                                FFT4096_SIZE,
                                TP_FFT_NIFFT,
                                TP_SHIFT,
                                TP_START_RANK,
                                TP_END_RANK,
                                TP_DYN_PT_SIZE,
                                TP_WINDOW_VSIZE,
                                TP_ORIG_PAR_POWER>::kernelFFT(input_window<TT_DATA>* __restrict inputx,
                                                              output_window<TT_OUT_DATA>* __restrict outputy) {
    typedef typename std::conditional<std::is_same<TT_DATA, cint16>::value, cint32_t, TT_DATA>::type T_internalDataType;

    static constexpr TT_TWIDDLE* __restrict tw1 =
        (std::is_same<TT_DATA, cfloat>::value ? (TT_TWIDDLE*)fft_lut_tw1_cfloat : (TT_TWIDDLE*)fft_lut_tw1);
    static constexpr TT_TWIDDLE* __restrict tw2 =
        (std::is_same<TT_DATA, cfloat>::value ? (TT_TWIDDLE*)fft_lut_tw2_cfloat : (TT_TWIDDLE*)fft_lut_tw2);
    static constexpr TT_TWIDDLE* __restrict tw4 =
        (std::is_same<TT_DATA, cfloat>::value ? (TT_TWIDDLE*)fft_lut_tw4_cfloat : (TT_TWIDDLE*)fft_lut_tw4);
    static constexpr TT_TWIDDLE* __restrict tw8 =
        (std::is_same<TT_DATA, cfloat>::value ? (TT_TWIDDLE*)fft_lut_tw8_cfloat : (TT_TWIDDLE*)fft_lut_tw8);
    static constexpr TT_TWIDDLE* __restrict tw16 =
        (std::is_same<TT_DATA, cfloat>::value ? (TT_TWIDDLE*)fft_lut_tw16_cfloat : (TT_TWIDDLE*)fft_lut_tw16);
    static constexpr TT_TWIDDLE* __restrict tw32 =
        (std::is_same<TT_DATA, cfloat>::value ? (TT_TWIDDLE*)fft_lut_tw32_cfloat : (TT_TWIDDLE*)fft_lut_tw32);
    static constexpr TT_TWIDDLE* __restrict tw64 =
        (std::is_same<TT_DATA, cfloat>::value ? (TT_TWIDDLE*)fft_lut_tw64_cfloat : (TT_TWIDDLE*)fft_lut_tw64);
    static constexpr TT_TWIDDLE* __restrict tw128 =
        (std::is_same<TT_DATA, cfloat>::value ? (TT_TWIDDLE*)fft_lut_tw128_cfloat : (TT_TWIDDLE*)fft_lut_tw128);
    static constexpr TT_TWIDDLE* __restrict tw256 =
        (std::is_same<TT_DATA, cfloat>::value ? (TT_TWIDDLE*)fft_lut_tw256_cfloat : (TT_TWIDDLE*)fft_lut_tw256);
    static constexpr TT_TWIDDLE* __restrict tw512 =
        (std::is_same<TT_DATA, cfloat>::value ? (TT_TWIDDLE*)fft_lut_tw512_cfloat : (TT_TWIDDLE*)fft_lut_tw512);
    static constexpr TT_TWIDDLE* __restrict tw1024 =
        (std::is_same<TT_DATA, cfloat>::value ? (TT_TWIDDLE*)fft_lut_tw1024_cfloat : (TT_TWIDDLE*)fft_lut_tw1024);
    static constexpr TT_TWIDDLE* __restrict tw2048 =
        (std::is_same<TT_DATA, cfloat>::value ? (TT_TWIDDLE*)fft_lut_tw2048_cfloat : (TT_TWIDDLE*)fft_lut_tw2048_half);

#ifdef __FFT_SPOOF_R4__
    printf("spoof_r4 \n");
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
#endif //__FFT_SPOOF_R4__
#ifdef __FFT_TRUE_R4__
    printf("true_r4 \n");
  static constexpr TT_TWIDDLE* __restrict tw1_2       = (std::is_same<TT_DATA,cfloat>::value ? NULL : (TT_TWIDDLE*)fft_lut_r4_1_2;
  static constexpr TT_TWIDDLE* __restrict tw2_4       = (std::is_same<TT_DATA,cfloat>::value ? NULL : (TT_TWIDDLE*)fft_lut_r4_2_4;
  static constexpr TT_TWIDDLE* __restrict tw4_8       = (std::is_same<TT_DATA,cfloat>::value ? NULL : (TT_TWIDDLE*)fft_lut_r4_4_8;
  static constexpr TT_TWIDDLE* __restrict tw8_16      = (std::is_same<TT_DATA,cfloat>::value ? NULL : (TT_TWIDDLE*)fft_lut_r4_8_16;
  static constexpr TT_TWIDDLE* __restrict tw16_32     = (std::is_same<TT_DATA,cfloat>::value ? NULL : (TT_TWIDDLE*)fft_lut_r4_16_32;
  static constexpr TT_TWIDDLE* __restrict tw32_64     = (std::is_same<TT_DATA,cfloat>::value ? NULL : (TT_TWIDDLE*)fft_lut_r4_32_64;
  static constexpr TT_TWIDDLE* __restrict tw64_128    = (std::is_same<TT_DATA,cfloat>::value ? NULL : (TT_TWIDDLE*)fft_lut_r4_64_128;
  static constexpr TT_TWIDDLE* __restrict tw128_256   = (std::is_same<TT_DATA,cfloat>::value ? NULL : (TT_TWIDDLE*)fft_lut_r4_128_256;
  static constexpr TT_TWIDDLE* __restrict tw256_512   = (std::is_same<TT_DATA,cfloat>::value ? NULL : (TT_TWIDDLE*)fft_lut_r4_256_512;
  static constexpr TT_TWIDDLE* __restrict tw512_1024  = (std::is_same<TT_DATA,cfloat>::value ? NULL : (TT_TWIDDLE*)fft_lut_r4_512_1024;
  static constexpr TT_TWIDDLE* __restrict tw1024_2048 = (std::is_same<TT_DATA,cfloat>::value ? NULL : (TT_TWIDDLE*)fft_lut_r4_1024_2048;
#endif //__FFT_TRUE_R4__

  static TT_TWIDDLE* tw_table[kMaxPointLog*2] = {tw1,tw2,tw4,tw8,tw16,tw32,tw64,tw128,tw256,tw512,tw1024,tw2048,
      tw1_2, tw2_4, tw4_8, tw8_16, tw16_32, tw32_64, tw64_128, tw128_256, tw256_512, tw512_1024, tw1024_2048, NULL};

  TT_DATA* xbuff = (TT_DATA*)inputx->ptr;
  T_internalDataType* tmp1_buf = (T_internalDataType*)fft_4096_tmp1;
  T_internalDataType* tmp2_buf = (T_internalDataType*)xbuff;

  stages.stagePreamble(tw_table, tmp1_buf, tmp2_buf, inputx, outputy);
};

template <typename TT_OUT_DATA,
          unsigned int TP_FFT_NIFFT,
          unsigned int TP_SHIFT,
          unsigned int TP_START_RANK,
          unsigned int TP_END_RANK,
          unsigned int TP_DYN_PT_SIZE,
          unsigned int TP_WINDOW_VSIZE,
          unsigned int TP_ORIG_PAR_POWER>
INLINE_DECL void kernelFFTClass<cint16,
                                TT_OUT_DATA,
                                cint16,
                                FFT4096_SIZE,
                                TP_FFT_NIFFT,
                                TP_SHIFT,
                                TP_START_RANK,
                                TP_END_RANK,
                                TP_DYN_PT_SIZE,
                                TP_WINDOW_VSIZE,
                                TP_ORIG_PAR_POWER>::kernelFFT(input_window<cint16>* __restrict inputx,
                                                              output_window<TT_OUT_DATA>* __restrict outputy) {
    typedef cint16 TT_DATA;              // essentially a constant for thie specialization
    typedef cint16 TT_TWIDDLE;           // essentially a constant for thie specialization
    typedef cint32_t T_internalDataType; // essentially a constant for thie specialization
    static constexpr cint16* __restrict tw1 = (cint16*)fft_lut_tw1;
    static constexpr cint16* __restrict tw2 = (cint16*)fft_lut_tw2;
    static constexpr cint16* __restrict tw4 = (cint16*)fft_lut_tw4;
    static constexpr cint16* __restrict tw8 = (cint16*)fft_lut_tw8;
    static constexpr cint16* __restrict tw16 = (cint16*)fft_lut_tw16;
    static constexpr cint16* __restrict tw32 = (cint16*)fft_lut_tw32;
    static constexpr cint16* __restrict tw64 = (cint16*)fft_lut_tw64;
    static constexpr cint16* __restrict tw128 = (cint16*)fft_lut_tw128;
    static constexpr cint16* __restrict tw256 = (cint16*)fft_lut_tw256;
    static constexpr cint16* __restrict tw512 = (cint16*)fft_lut_tw512;
    static constexpr cint16* __restrict tw1024 = (cint16*)fft_lut_tw1024;
    static constexpr cint16* __restrict tw2048 = (cint16*)fft_lut_tw2048_half;

#ifdef __FFT_SPOOF_R4__
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
#endif //__FFT_SPOOF_R4__
#ifdef __FFT_TRUE_R4__
    static constexpr TT_TWIDDLE* __restrict tw1_2 = (TT_TWIDDLE*)fft_lut_r4_1_2;
    static constexpr TT_TWIDDLE* __restrict tw2_4 = (TT_TWIDDLE*)fft_lut_r4_2_4;
    static constexpr TT_TWIDDLE* __restrict tw4_8 = (TT_TWIDDLE*)fft_lut_r4_4_8;
    static constexpr TT_TWIDDLE* __restrict tw8_16 = (TT_TWIDDLE*)fft_lut_r4_8_16;
    static constexpr TT_TWIDDLE* __restrict tw16_32 = (TT_TWIDDLE*)fft_lut_r4_16_32;
    static constexpr TT_TWIDDLE* __restrict tw32_64 = (TT_TWIDDLE*)fft_lut_r4_32_64;
    static constexpr TT_TWIDDLE* __restrict tw64_128 = (TT_TWIDDLE*)fft_lut_r4_64_128;
    static constexpr TT_TWIDDLE* __restrict tw128_256 = (TT_TWIDDLE*)fft_lut_r4_128_256;
    static constexpr TT_TWIDDLE* __restrict tw256_512 = (TT_TWIDDLE*)fft_lut_r4_256_512;
    static constexpr TT_TWIDDLE* __restrict tw512_1024 = (TT_TWIDDLE*)fft_lut_r4_512_1024;
    static constexpr TT_TWIDDLE* __restrict tw1024_2048 = (TT_TWIDDLE*)fft_lut_r4_1024_2048;
#endif //__FFT_TRUE_R4__

    static cint16* tw_table[kMaxPointLog * 2] = {
        tw1,   tw2,   tw4,   tw8,    tw16,    tw32,    tw64,     tw128,     tw256,     tw512,      tw1024,      tw2048,
        tw1_2, tw2_4, tw4_8, tw8_16, tw16_32, tw32_64, tw64_128, tw128_256, tw256_512, tw512_1024, tw1024_2048, NULL};

    cint16* xbuff = (cint16*)inputx->ptr;
    TT_OUT_DATA* obuff = (TT_OUT_DATA*)outputy->ptr;
    T_internalDataType* tmp1_buf = (T_internalDataType*)fft_4096_tmp1;
    T_internalDataType* tmp2_buf = (T_internalDataType*)fft_4096_tmp2;

    stages.stagePreamble(tw_table, tmp1_buf, tmp2_buf, inputx, outputy);
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
          unsigned int TP_ORIG_PAR_POWER>
INLINE_DECL void kernelFFTClass<TT_DATA,
                                TT_OUT_DATA,
                                TT_TWIDDLE,
                                FFT2048_SIZE,
                                TP_FFT_NIFFT,
                                TP_SHIFT,
                                TP_START_RANK,
                                TP_END_RANK,
                                TP_DYN_PT_SIZE,
                                TP_WINDOW_VSIZE,
                                TP_ORIG_PAR_POWER>::kernelFFT(input_window<TT_DATA>* __restrict inputx,
                                                              output_window<TT_OUT_DATA>* __restrict outputy) {
    typedef typename std::conditional<std::is_same<TT_DATA, cint16>::value, cint32_t, TT_DATA>::type T_internalDataType;
    static constexpr TT_TWIDDLE* __restrict tw1 =
        (std::is_same<TT_DATA, cfloat>::value ? (TT_TWIDDLE*)fft_lut_tw1_cfloat : (TT_TWIDDLE*)fft_lut_tw1);
    static constexpr TT_TWIDDLE* __restrict tw2 =
        (std::is_same<TT_DATA, cfloat>::value ? (TT_TWIDDLE*)fft_lut_tw2_cfloat : (TT_TWIDDLE*)fft_lut_tw2);
    static constexpr TT_TWIDDLE* __restrict tw4 =
        (std::is_same<TT_DATA, cfloat>::value ? (TT_TWIDDLE*)fft_lut_tw4_cfloat : (TT_TWIDDLE*)fft_lut_tw4);
    static constexpr TT_TWIDDLE* __restrict tw8 =
        (std::is_same<TT_DATA, cfloat>::value ? (TT_TWIDDLE*)fft_lut_tw8_cfloat : (TT_TWIDDLE*)fft_lut_tw8);
    static constexpr TT_TWIDDLE* __restrict tw16 =
        (std::is_same<TT_DATA, cfloat>::value ? (TT_TWIDDLE*)fft_lut_tw16_cfloat : (TT_TWIDDLE*)fft_lut_tw16);
    static constexpr TT_TWIDDLE* __restrict tw32 =
        (std::is_same<TT_DATA, cfloat>::value ? (TT_TWIDDLE*)fft_lut_tw32_cfloat : (TT_TWIDDLE*)fft_lut_tw32);
    static constexpr TT_TWIDDLE* __restrict tw64 =
        (std::is_same<TT_DATA, cfloat>::value ? (TT_TWIDDLE*)fft_lut_tw64_cfloat : (TT_TWIDDLE*)fft_lut_tw64);
    static constexpr TT_TWIDDLE* __restrict tw128 =
        (std::is_same<TT_DATA, cfloat>::value ? (TT_TWIDDLE*)fft_lut_tw128_cfloat : (TT_TWIDDLE*)fft_lut_tw128);
    static constexpr TT_TWIDDLE* __restrict tw256 =
        (std::is_same<TT_DATA, cfloat>::value ? (TT_TWIDDLE*)fft_lut_tw256_cfloat : (TT_TWIDDLE*)fft_lut_tw256);
    static constexpr TT_TWIDDLE* __restrict tw512 =
        (std::is_same<TT_DATA, cfloat>::value ? (TT_TWIDDLE*)fft_lut_tw512_cfloat : (TT_TWIDDLE*)fft_lut_tw512);
    static constexpr TT_TWIDDLE* __restrict tw1024 =
        (std::is_same<TT_DATA, cfloat>::value ? (TT_TWIDDLE*)fft_lut_tw1024_cfloat : (TT_TWIDDLE*)fft_lut_tw1024_half);

#ifdef __FFT_SPOOF_R4__
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
#endif //__FFT_SPOOF_R4__
#ifdef __FFT_TRUE_R4__
  static constexpr TT_TWIDDLE* __restrict tw1_2       = (std::is_same<TT_DATA,cfloat>::value ? NULL : (TT_TWIDDLE*)fft_lut_r4_1_2;
  static constexpr TT_TWIDDLE* __restrict tw2_4       = (std::is_same<TT_DATA,cfloat>::value ? NULL : (TT_TWIDDLE*)fft_lut_r4_2_4;
  static constexpr TT_TWIDDLE* __restrict tw4_8       = (std::is_same<TT_DATA,cfloat>::value ? NULL : (TT_TWIDDLE*)fft_lut_r4_4_8;
  static constexpr TT_TWIDDLE* __restrict tw8_16      = (std::is_same<TT_DATA,cfloat>::value ? NULL : (TT_TWIDDLE*)fft_lut_r4_8_16;
  static constexpr TT_TWIDDLE* __restrict tw16_32     = (std::is_same<TT_DATA,cfloat>::value ? NULL : (TT_TWIDDLE*)fft_lut_r4_16_32;
  static constexpr TT_TWIDDLE* __restrict tw32_64     = (std::is_same<TT_DATA,cfloat>::value ? NULL : (TT_TWIDDLE*)fft_lut_r4_32_64;
  static constexpr TT_TWIDDLE* __restrict tw64_128    = (std::is_same<TT_DATA,cfloat>::value ? NULL : (TT_TWIDDLE*)fft_lut_r4_64_128;
  static constexpr TT_TWIDDLE* __restrict tw128_256   = (std::is_same<TT_DATA,cfloat>::value ? NULL : (TT_TWIDDLE*)fft_lut_r4_128_256;
  static constexpr TT_TWIDDLE* __restrict tw256_512   = (std::is_same<TT_DATA,cfloat>::value ? NULL : (TT_TWIDDLE*)fft_lut_r4_256_512;
  static constexpr TT_TWIDDLE* __restrict tw512_1024  = (std::is_same<TT_DATA,cfloat>::value ? NULL : (TT_TWIDDLE*)fft_lut_r4_512_1024;
  static constexpr TT_TWIDDLE* __restrict tw1024_2048 = (std::is_same<TT_DATA,cfloat>::value ? NULL : (TT_TWIDDLE*)fft_lut_r4_1024_2048;
#endif //__FFT_TRUE_R4__

  static TT_TWIDDLE* tw_table[kMaxPointLog*2] = {tw1,tw2,tw4,tw8,tw16,tw32,tw64,tw128,tw256,tw512,tw1024,NULL,
      tw1_2, tw2_4, tw4_8, tw8_16, tw16_32, tw32_64, tw64_128, tw128_256, tw256_512, tw512_1024, tw1024_2048, NULL};

  TT_DATA* xbuff = (TT_DATA*)inputx->ptr;
  T_internalDataType* tmp1_buf = (T_internalDataType*)fft_2048_tmp1;
  T_internalDataType* tmp2_buf = (T_internalDataType*)xbuff;

  stages.stagePreamble(tw_table, tmp1_buf, tmp2_buf, inputx, outputy);
};

template <typename TT_OUT_DATA,
          unsigned int TP_FFT_NIFFT,
          unsigned int TP_SHIFT,
          unsigned int TP_START_RANK,
          unsigned int TP_END_RANK,
          unsigned int TP_DYN_PT_SIZE,
          unsigned int TP_WINDOW_VSIZE,
          unsigned int TP_ORIG_PAR_POWER>
INLINE_DECL void kernelFFTClass<cint16,
                                TT_OUT_DATA,
                                cint16,
                                FFT2048_SIZE,
                                TP_FFT_NIFFT,
                                TP_SHIFT,
                                TP_START_RANK,
                                TP_END_RANK,
                                TP_DYN_PT_SIZE,
                                TP_WINDOW_VSIZE,
                                TP_ORIG_PAR_POWER>::kernelFFT(input_window<cint16>* __restrict inputx,
                                                              output_window<TT_OUT_DATA>* __restrict outputy) {
    typedef cint16 TT_DATA;
    typedef cint16 TT_TWIDDLE; // essentially a constant for thie specialization
    typedef cint32_t T_internalDataType;
    static constexpr cint16* __restrict tw1 = (cint16*)fft_lut_tw1;
    static constexpr cint16* __restrict tw2 = (cint16*)fft_lut_tw2;
    static constexpr cint16* __restrict tw4 = (cint16*)fft_lut_tw4;
    static constexpr cint16* __restrict tw8 = (cint16*)fft_lut_tw8;
    static constexpr cint16* __restrict tw16 = (cint16*)fft_lut_tw16;
    static constexpr cint16* __restrict tw32 = (cint16*)fft_lut_tw32;
    static constexpr cint16* __restrict tw64 = (cint16*)fft_lut_tw64;
    static constexpr cint16* __restrict tw128 = (cint16*)fft_lut_tw128;
    static constexpr cint16* __restrict tw256 = (cint16*)fft_lut_tw256;
    static constexpr cint16* __restrict tw512 = (cint16*)fft_lut_tw512;
    static constexpr cint16* __restrict tw1024 = (cint16*)fft_lut_tw1024_half;

#ifdef __FFT_SPOOF_R4__
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
#endif //__FFT_SPOOF_R4__
#ifdef __FFT_TRUE_R4__
    static constexpr TT_TWIDDLE* __restrict tw1_2 = (TT_TWIDDLE*)fft_lut_r4_1_2;
    static constexpr TT_TWIDDLE* __restrict tw2_4 = (TT_TWIDDLE*)fft_lut_r4_2_4;
    static constexpr TT_TWIDDLE* __restrict tw4_8 = (TT_TWIDDLE*)fft_lut_r4_4_8;
    static constexpr TT_TWIDDLE* __restrict tw8_16 = (TT_TWIDDLE*)fft_lut_r4_8_16;
    static constexpr TT_TWIDDLE* __restrict tw16_32 = (TT_TWIDDLE*)fft_lut_r4_16_32;
    static constexpr TT_TWIDDLE* __restrict tw32_64 = (TT_TWIDDLE*)fft_lut_r4_32_64;
    static constexpr TT_TWIDDLE* __restrict tw64_128 = (TT_TWIDDLE*)fft_lut_r4_64_128;
    static constexpr TT_TWIDDLE* __restrict tw128_256 = (TT_TWIDDLE*)fft_lut_r4_128_256;
    static constexpr TT_TWIDDLE* __restrict tw256_512 = (TT_TWIDDLE*)fft_lut_r4_256_512;
    static constexpr TT_TWIDDLE* __restrict tw512_1024 = (TT_TWIDDLE*)fft_lut_r4_512_1024;
    static constexpr TT_TWIDDLE* __restrict tw1024_2048 = (TT_TWIDDLE*)fft_lut_r4_1024_2048;
#endif //__FFT_TRUE_R4__
    static cint16* tw_table[kMaxPointLog * 2] = {
        tw1,   tw2,   tw4,   tw8,    tw16,    tw32,    tw64,     tw128,     tw256,     tw512,      tw1024,      NULL,
        tw1_2, tw2_4, tw4_8, tw8_16, tw16_32, tw32_64, tw64_128, tw128_256, tw256_512, tw512_1024, tw1024_2048, NULL};

    cint16* xbuff = (cint16*)inputx->ptr;
    TT_OUT_DATA* obuff = (TT_OUT_DATA*)outputy->ptr;
    T_internalDataType* tmp1_buf = (T_internalDataType*)fft_2048_tmp1;
    T_internalDataType* tmp2_buf = (T_internalDataType*)fft_2048_tmp2;

    stages.stagePreamble(tw_table, tmp1_buf, tmp2_buf, inputx, outputy);
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
          unsigned int TP_ORIG_PAR_POWER>
INLINE_DECL void kernelFFTClass<TT_DATA,
                                TT_OUT_DATA,
                                TT_TWIDDLE,
                                FFT1024_SIZE,
                                TP_FFT_NIFFT,
                                TP_SHIFT,
                                TP_START_RANK,
                                TP_END_RANK,
                                TP_DYN_PT_SIZE,
                                TP_WINDOW_VSIZE,
                                TP_ORIG_PAR_POWER>::kernelFFT(input_window<TT_DATA>* __restrict inputx,
                                                              output_window<TT_OUT_DATA>* __restrict outputy) {
    typedef typename std::conditional<std::is_same<TT_DATA, cint16>::value, cint32_t, TT_DATA>::type T_internalDataType;
    static constexpr TT_TWIDDLE* __restrict tw1 =
        (std::is_same<TT_DATA, cfloat>::value ? (TT_TWIDDLE*)fft_lut_tw1_cfloat : (TT_TWIDDLE*)fft_lut_tw1);
    static constexpr TT_TWIDDLE* __restrict tw2 =
        (std::is_same<TT_DATA, cfloat>::value ? (TT_TWIDDLE*)fft_lut_tw2_cfloat : (TT_TWIDDLE*)fft_lut_tw2);
    static constexpr TT_TWIDDLE* __restrict tw4 =
        (std::is_same<TT_DATA, cfloat>::value ? (TT_TWIDDLE*)fft_lut_tw4_cfloat : (TT_TWIDDLE*)fft_lut_tw4);
    static constexpr TT_TWIDDLE* __restrict tw8 =
        (std::is_same<TT_DATA, cfloat>::value ? (TT_TWIDDLE*)fft_lut_tw8_cfloat : (TT_TWIDDLE*)fft_lut_tw8);
    static constexpr TT_TWIDDLE* __restrict tw16 =
        (std::is_same<TT_DATA, cfloat>::value ? (TT_TWIDDLE*)fft_lut_tw16_cfloat : (TT_TWIDDLE*)fft_lut_tw16);
    static constexpr TT_TWIDDLE* __restrict tw32 =
        (std::is_same<TT_DATA, cfloat>::value ? (TT_TWIDDLE*)fft_lut_tw32_cfloat : (TT_TWIDDLE*)fft_lut_tw32);
    static constexpr TT_TWIDDLE* __restrict tw64 =
        (std::is_same<TT_DATA, cfloat>::value ? (TT_TWIDDLE*)fft_lut_tw64_cfloat : (TT_TWIDDLE*)fft_lut_tw64);
    static constexpr TT_TWIDDLE* __restrict tw128 =
        (std::is_same<TT_DATA, cfloat>::value ? (TT_TWIDDLE*)fft_lut_tw128_cfloat : (TT_TWIDDLE*)fft_lut_tw128);
    static constexpr TT_TWIDDLE* __restrict tw256 =
        (std::is_same<TT_DATA, cfloat>::value ? (TT_TWIDDLE*)fft_lut_tw256_cfloat : (TT_TWIDDLE*)fft_lut_tw256);
    static constexpr TT_TWIDDLE* __restrict tw512 =
        (std::is_same<TT_DATA, cfloat>::value ? (TT_TWIDDLE*)fft_lut_tw512_cfloat : (TT_TWIDDLE*)fft_lut_tw512_half);

#ifdef __FFT_SPOOF_R4__
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
#endif //__FFT_SPOOF_R4__
#ifdef __FFT_TRUE_R4__
  static constexpr TT_TWIDDLE* __restrict tw1_2       = (std::is_same<TT_DATA,cfloat>::value ? NULL : (TT_TWIDDLE*)fft_lut_r4_1_2;
  static constexpr TT_TWIDDLE* __restrict tw2_4       = (std::is_same<TT_DATA,cfloat>::value ? NULL : (TT_TWIDDLE*)fft_lut_r4_2_4;
  static constexpr TT_TWIDDLE* __restrict tw4_8       = (std::is_same<TT_DATA,cfloat>::value ? NULL : (TT_TWIDDLE*)fft_lut_r4_4_8;
  static constexpr TT_TWIDDLE* __restrict tw8_16      = (std::is_same<TT_DATA,cfloat>::value ? NULL : (TT_TWIDDLE*)fft_lut_r4_8_16;
  static constexpr TT_TWIDDLE* __restrict tw16_32     = (std::is_same<TT_DATA,cfloat>::value ? NULL : (TT_TWIDDLE*)fft_lut_r4_16_32;
  static constexpr TT_TWIDDLE* __restrict tw32_64     = (std::is_same<TT_DATA,cfloat>::value ? NULL : (TT_TWIDDLE*)fft_lut_r4_32_64;
  static constexpr TT_TWIDDLE* __restrict tw64_128    = (std::is_same<TT_DATA,cfloat>::value ? NULL : (TT_TWIDDLE*)fft_lut_r4_64_128;
  static constexpr TT_TWIDDLE* __restrict tw128_256   = (std::is_same<TT_DATA,cfloat>::value ? NULL : (TT_TWIDDLE*)fft_lut_r4_128_256;
  static constexpr TT_TWIDDLE* __restrict tw256_512   = (std::is_same<TT_DATA,cfloat>::value ? NULL : (TT_TWIDDLE*)fft_lut_r4_256_512;
  static constexpr TT_TWIDDLE* __restrict tw512_1024  = (std::is_same<TT_DATA,cfloat>::value ? NULL : (TT_TWIDDLE*)fft_lut_r4_512_1024;
#endif //__FFT_TRUE_R4__
  static TT_TWIDDLE* tw_table[kMaxPointLog*2] = {tw1,tw2,tw4,tw8,tw16,tw32,tw64,tw128,tw256,tw512,NULL,NULL,
      tw1_2, tw2_4, tw4_8, tw8_16, tw16_32, tw32_64, tw64_128, tw128_256, tw256_512, tw512_1024, NULL, NULL};

  TT_DATA* xbuff = (TT_DATA*)inputx->ptr;
  T_internalDataType* tmp1_buf = (T_internalDataType*)fft_1024_tmp1;
  T_internalDataType* tmp2_buf = (T_internalDataType*)xbuff;

  stages.stagePreamble(tw_table, tmp1_buf, tmp2_buf, inputx, outputy);
};

template <typename TT_OUT_DATA,
          unsigned int TP_FFT_NIFFT,
          unsigned int TP_SHIFT,
          unsigned int TP_START_RANK,
          unsigned int TP_END_RANK,
          unsigned int TP_DYN_PT_SIZE,
          unsigned int TP_WINDOW_VSIZE,
          unsigned int TP_ORIG_PAR_POWER>
INLINE_DECL void kernelFFTClass<cint16,
                                TT_OUT_DATA,
                                cint16,
                                FFT1024_SIZE,
                                TP_FFT_NIFFT,
                                TP_SHIFT,
                                TP_START_RANK,
                                TP_END_RANK,
                                TP_DYN_PT_SIZE,
                                TP_WINDOW_VSIZE,
                                TP_ORIG_PAR_POWER>::kernelFFT(input_window<cint16>* __restrict inputx,
                                                              output_window<TT_OUT_DATA>* __restrict outputy) {
    typedef cint16 TT_DATA;
    typedef cint16 TT_TWIDDLE; // essentially a constant for thie specialization
    typedef cint32_t T_internalDataType;
    static constexpr cint16* __restrict tw1 = (cint16*)fft_lut_tw1;
    static constexpr cint16* __restrict tw2 = (cint16*)fft_lut_tw2;
    static constexpr cint16* __restrict tw4 = (cint16*)fft_lut_tw4;
    static constexpr cint16* __restrict tw8 = (cint16*)fft_lut_tw8;
    static constexpr cint16* __restrict tw16 = (cint16*)fft_lut_tw16;
    static constexpr cint16* __restrict tw32 = (cint16*)fft_lut_tw32;
    static constexpr cint16* __restrict tw64 = (cint16*)fft_lut_tw64;
    static constexpr cint16* __restrict tw128 = (cint16*)fft_lut_tw128;
    static constexpr cint16* __restrict tw256 = (cint16*)fft_lut_tw256;
    static constexpr cint16* __restrict tw512 = (cint16*)fft_lut_tw512_half;

#ifdef __FFT_SPOOF_R4__
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
#endif //__FFT_SPOOF_R4__
#ifdef __FFT_TRUE_R4__
    static constexpr TT_TWIDDLE* __restrict tw1_2 = (TT_TWIDDLE*)fft_lut_r4_1_2;
    static constexpr TT_TWIDDLE* __restrict tw2_4 = (TT_TWIDDLE*)fft_lut_r4_2_4;
    static constexpr TT_TWIDDLE* __restrict tw4_8 = (TT_TWIDDLE*)fft_lut_r4_4_8;
    static constexpr TT_TWIDDLE* __restrict tw8_16 = (TT_TWIDDLE*)fft_lut_r4_8_16;
    static constexpr TT_TWIDDLE* __restrict tw16_32 = (TT_TWIDDLE*)fft_lut_r4_16_32;
    static constexpr TT_TWIDDLE* __restrict tw32_64 = (TT_TWIDDLE*)fft_lut_r4_32_64;
    static constexpr TT_TWIDDLE* __restrict tw64_128 = (TT_TWIDDLE*)fft_lut_r4_64_128;
    static constexpr TT_TWIDDLE* __restrict tw128_256 = (TT_TWIDDLE*)fft_lut_r4_128_256;
    static constexpr TT_TWIDDLE* __restrict tw256_512 = (TT_TWIDDLE*)fft_lut_r4_256_512;
    static constexpr TT_TWIDDLE* __restrict tw512_1024 = (TT_TWIDDLE*)fft_lut_r4_512_1024;
#endif //__FFT_TRUE_R4__

    static cint16* tw_table[kMaxPointLog * 2] = {
        tw1,   tw2,   tw4,   tw8,    tw16,    tw32,    tw64,     tw128,     tw256,     tw512,      NULL, NULL,
        tw1_2, tw2_4, tw4_8, tw8_16, tw16_32, tw32_64, tw64_128, tw128_256, tw256_512, tw512_1024, NULL, NULL};

    cint16* xbuff = (cint16*)inputx->ptr;
    TT_OUT_DATA* obuff = (TT_OUT_DATA*)outputy->ptr;
    T_internalDataType* tmp1_buf = (T_internalDataType*)fft_1024_tmp1;
    T_internalDataType* tmp2_buf = (T_internalDataType*)fft_1024_tmp2;

    stages.stagePreamble(tw_table, tmp1_buf, tmp2_buf, inputx, outputy);
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
          unsigned int TP_ORIG_PAR_POWER>
INLINE_DECL void kernelFFTClass<TT_DATA,
                                TT_OUT_DATA,
                                TT_TWIDDLE,
                                FFT512_SIZE,
                                TP_FFT_NIFFT,
                                TP_SHIFT,
                                TP_START_RANK,
                                TP_END_RANK,
                                TP_DYN_PT_SIZE,
                                TP_WINDOW_VSIZE,
                                TP_ORIG_PAR_POWER>::kernelFFT(input_window<TT_DATA>* __restrict inputx,
                                                              output_window<TT_OUT_DATA>* __restrict outputy) {
    typedef typename std::conditional<std::is_same<TT_DATA, cint16>::value, cint32_t, TT_DATA>::type T_internalDataType;
    static constexpr TT_TWIDDLE* __restrict tw1 =
        (std::is_same<TT_DATA, cfloat>::value ? (TT_TWIDDLE*)fft_lut_tw1_cfloat : (TT_TWIDDLE*)fft_lut_tw1);
    static constexpr TT_TWIDDLE* __restrict tw2 =
        (std::is_same<TT_DATA, cfloat>::value ? (TT_TWIDDLE*)fft_lut_tw2_cfloat : (TT_TWIDDLE*)fft_lut_tw2);
    static constexpr TT_TWIDDLE* __restrict tw4 =
        (std::is_same<TT_DATA, cfloat>::value ? (TT_TWIDDLE*)fft_lut_tw4_cfloat : (TT_TWIDDLE*)fft_lut_tw4);
    static constexpr TT_TWIDDLE* __restrict tw8 =
        (std::is_same<TT_DATA, cfloat>::value ? (TT_TWIDDLE*)fft_lut_tw8_cfloat : (TT_TWIDDLE*)fft_lut_tw8);
    static constexpr TT_TWIDDLE* __restrict tw16 =
        (std::is_same<TT_DATA, cfloat>::value ? (TT_TWIDDLE*)fft_lut_tw16_cfloat : (TT_TWIDDLE*)fft_lut_tw16);
    static constexpr TT_TWIDDLE* __restrict tw32 =
        (std::is_same<TT_DATA, cfloat>::value ? (TT_TWIDDLE*)fft_lut_tw32_cfloat : (TT_TWIDDLE*)fft_lut_tw32);
    static constexpr TT_TWIDDLE* __restrict tw64 =
        (std::is_same<TT_DATA, cfloat>::value ? (TT_TWIDDLE*)fft_lut_tw64_cfloat : (TT_TWIDDLE*)fft_lut_tw64);
    static constexpr TT_TWIDDLE* __restrict tw128 =
        (std::is_same<TT_DATA, cfloat>::value ? (TT_TWIDDLE*)fft_lut_tw128_cfloat : (TT_TWIDDLE*)fft_lut_tw128);
    static constexpr TT_TWIDDLE* __restrict tw256 =
        (std::is_same<TT_DATA, cfloat>::value ? (TT_TWIDDLE*)fft_lut_tw256_cfloat : (TT_TWIDDLE*)fft_lut_tw256_half);

#ifdef __FFT_SPOOF_R4__
    static constexpr TT_TWIDDLE* __restrict tw1_2 = NULL;
    static constexpr TT_TWIDDLE* __restrict tw2_4 = NULL;
    static constexpr TT_TWIDDLE* __restrict tw4_8 = NULL;
    static constexpr TT_TWIDDLE* __restrict tw8_16 = NULL;
    static constexpr TT_TWIDDLE* __restrict tw16_32 = NULL;
    static constexpr TT_TWIDDLE* __restrict tw32_64 = NULL;
    static constexpr TT_TWIDDLE* __restrict tw64_128 = NULL;
    static constexpr TT_TWIDDLE* __restrict tw128_256 = NULL;
    static constexpr TT_TWIDDLE* __restrict tw256_512 = NULL;
#endif //__FFT_SPOOF_R4__
#ifdef __FFT_TRUE_R4__
  static constexpr TT_TWIDDLE* __restrict tw1_2       = (std::is_same<TT_DATA,cfloat>::value ? NULL : (TT_TWIDDLE*)fft_lut_r4_1_2;
  static constexpr TT_TWIDDLE* __restrict tw2_4       = (std::is_same<TT_DATA,cfloat>::value ? NULL : (TT_TWIDDLE*)fft_lut_r4_2_4;
  static constexpr TT_TWIDDLE* __restrict tw4_8       = (std::is_same<TT_DATA,cfloat>::value ? NULL : (TT_TWIDDLE*)fft_lut_r4_4_8;
  static constexpr TT_TWIDDLE* __restrict tw8_16      = (std::is_same<TT_DATA,cfloat>::value ? NULL : (TT_TWIDDLE*)fft_lut_r4_8_16;
  static constexpr TT_TWIDDLE* __restrict tw16_32     = (std::is_same<TT_DATA,cfloat>::value ? NULL : (TT_TWIDDLE*)fft_lut_r4_16_32;
  static constexpr TT_TWIDDLE* __restrict tw32_64     = (std::is_same<TT_DATA,cfloat>::value ? NULL : (TT_TWIDDLE*)fft_lut_r4_32_64;
  static constexpr TT_TWIDDLE* __restrict tw64_128    = (std::is_same<TT_DATA,cfloat>::value ? NULL : (TT_TWIDDLE*)fft_lut_r4_64_128;
  static constexpr TT_TWIDDLE* __restrict tw128_256   = (std::is_same<TT_DATA,cfloat>::value ? NULL : (TT_TWIDDLE*)fft_lut_r4_128_256;
  static constexpr TT_TWIDDLE* __restrict tw256_512   = (std::is_same<TT_DATA,cfloat>::value ? NULL : (TT_TWIDDLE*)fft_lut_r4_256_512;
#endif //__FFT_TRUE_R4_

  static TT_TWIDDLE* tw_table[kMaxPointLog*2] = {tw1,tw2,tw4,tw8,tw16,tw32,tw64,tw128,tw256,NULL,NULL,NULL,
      tw1_2, tw2_4, tw4_8, tw8_16, tw16_32, tw32_64, tw64_128, tw128_256, tw256_512, NULL, NULL, NULL};

  TT_DATA* xbuff = (TT_DATA*)inputx->ptr;
  T_internalDataType* tmp1_buf = (T_internalDataType*)fft_512_tmp1;
  T_internalDataType* tmp2_buf = (T_internalDataType*)xbuff;

  stages.stagePreamble(tw_table, tmp1_buf, tmp2_buf, inputx, outputy);
};

template <typename TT_OUT_DATA,
          unsigned int TP_FFT_NIFFT,
          unsigned int TP_SHIFT,
          unsigned int TP_START_RANK,
          unsigned int TP_END_RANK,
          unsigned int TP_DYN_PT_SIZE,
          unsigned int TP_WINDOW_VSIZE,
          unsigned int TP_ORIG_PAR_POWER>
INLINE_DECL void kernelFFTClass<cint16,
                                TT_OUT_DATA,
                                cint16,
                                FFT512_SIZE,
                                TP_FFT_NIFFT,
                                TP_SHIFT,
                                TP_START_RANK,
                                TP_END_RANK,
                                TP_DYN_PT_SIZE,
                                TP_WINDOW_VSIZE,
                                TP_ORIG_PAR_POWER>::kernelFFT(input_window<cint16>* __restrict inputx,
                                                              output_window<TT_OUT_DATA>* __restrict outputy) {
    typedef cint16 TT_DATA;
    typedef cint16 TT_TWIDDLE; // essentially a constant for thie specialization
    typedef cint32_t T_internalDataType;
    static constexpr cint16* __restrict tw1 = (cint16*)fft_lut_tw1;
    static constexpr cint16* __restrict tw2 = (cint16*)fft_lut_tw2;
    static constexpr cint16* __restrict tw4 = (cint16*)fft_lut_tw4;
    static constexpr cint16* __restrict tw8 = (cint16*)fft_lut_tw8;
    static constexpr cint16* __restrict tw16 = (cint16*)fft_lut_tw16;
    static constexpr cint16* __restrict tw32 = (cint16*)fft_lut_tw32;
    static constexpr cint16* __restrict tw64 = (cint16*)fft_lut_tw64;
    static constexpr cint16* __restrict tw128 = (cint16*)fft_lut_tw128;
    static constexpr cint16* __restrict tw256 = (cint16*)fft_lut_tw256_half;

#ifdef __FFT_SPOOF_R4__
    static constexpr TT_TWIDDLE* __restrict tw1_2 = NULL;
    static constexpr TT_TWIDDLE* __restrict tw2_4 = NULL;
    static constexpr TT_TWIDDLE* __restrict tw4_8 = NULL;
    static constexpr TT_TWIDDLE* __restrict tw8_16 = NULL;
    static constexpr TT_TWIDDLE* __restrict tw16_32 = NULL;
    static constexpr TT_TWIDDLE* __restrict tw32_64 = NULL;
    static constexpr TT_TWIDDLE* __restrict tw64_128 = NULL;
    static constexpr TT_TWIDDLE* __restrict tw128_256 = NULL;
    static constexpr TT_TWIDDLE* __restrict tw256_512 = NULL;
#endif //__FFT_SPOOF_R4__
#ifdef __FFT_TRUE_R4__
  static constexpr TT_TWIDDLE* __restrict tw1_2       = (std::is_same<TT_DATA,cfloat>::value ? NULL : (TT_TWIDDLE*)fft_lut_r4_1_2;
  static constexpr TT_TWIDDLE* __restrict tw2_4       = (std::is_same<TT_DATA,cfloat>::value ? NULL : (TT_TWIDDLE*)fft_lut_r4_2_4;
  static constexpr TT_TWIDDLE* __restrict tw4_8       = (std::is_same<TT_DATA,cfloat>::value ? NULL : (TT_TWIDDLE*)fft_lut_r4_4_8;
  static constexpr TT_TWIDDLE* __restrict tw8_16      = (std::is_same<TT_DATA,cfloat>::value ? NULL : (TT_TWIDDLE*)fft_lut_r4_8_16;
  static constexpr TT_TWIDDLE* __restrict tw16_32     = (std::is_same<TT_DATA,cfloat>::value ? NULL : (TT_TWIDDLE*)fft_lut_r4_16_32;
  static constexpr TT_TWIDDLE* __restrict tw32_64     = (std::is_same<TT_DATA,cfloat>::value ? NULL : (TT_TWIDDLE*)fft_lut_r4_32_64;
  static constexpr TT_TWIDDLE* __restrict tw64_128    = (std::is_same<TT_DATA,cfloat>::value ? NULL : (TT_TWIDDLE*)fft_lut_r4_64_128;
  static constexpr TT_TWIDDLE* __restrict tw128_256   = (std::is_same<TT_DATA,cfloat>::value ? NULL : (TT_TWIDDLE*)fft_lut_r4_128_256;
  static constexpr TT_TWIDDLE* __restrict tw256_512   = (std::is_same<TT_DATA,cfloat>::value ? NULL : (TT_TWIDDLE*)fft_lut_r4_256_512;
#endif //__FFT_TRUE_R4__

  static cint16* tw_table[kMaxPointLog*2] = {tw1,tw2,tw4,tw8,tw16,tw32,tw64,tw128,tw256,NULL,NULL,NULL,
      tw1_2, tw2_4, tw4_8, tw8_16, tw16_32, tw32_64, tw64_128, tw128_256, tw256_512, NULL, NULL, NULL};

  cint16* xbuff = (cint16*)inputx->ptr;
  TT_OUT_DATA* obuff = (TT_OUT_DATA*)outputy->ptr;
  T_internalDataType* tmp1_buf = (T_internalDataType*)fft_512_tmp1;
  T_internalDataType* tmp2_buf = (T_internalDataType*)fft_512_tmp2;

  stages.stagePreamble(tw_table, tmp1_buf, tmp2_buf, inputx, outputy);
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
          unsigned int TP_ORIG_PAR_POWER>
INLINE_DECL void kernelFFTClass<TT_DATA,
                                TT_OUT_DATA,
                                TT_TWIDDLE,
                                FFT256_SIZE,
                                TP_FFT_NIFFT,
                                TP_SHIFT,
                                TP_START_RANK,
                                TP_END_RANK,
                                TP_DYN_PT_SIZE,
                                TP_WINDOW_VSIZE,
                                TP_ORIG_PAR_POWER>::kernelFFT(input_window<TT_DATA>* __restrict inputx,
                                                              output_window<TT_OUT_DATA>* __restrict outputy) {
    typedef typename std::conditional<std::is_same<TT_DATA, cint16>::value, cint32_t, TT_DATA>::type T_internalDataType;
    static constexpr TT_TWIDDLE* __restrict tw1 =
        (std::is_same<TT_DATA, cfloat>::value ? (TT_TWIDDLE*)fft_lut_tw1_cfloat : (TT_TWIDDLE*)fft_lut_tw1);
    static constexpr TT_TWIDDLE* __restrict tw2 =
        (std::is_same<TT_DATA, cfloat>::value ? (TT_TWIDDLE*)fft_lut_tw2_cfloat : (TT_TWIDDLE*)fft_lut_tw2);
    static constexpr TT_TWIDDLE* __restrict tw4 =
        (std::is_same<TT_DATA, cfloat>::value ? (TT_TWIDDLE*)fft_lut_tw4_cfloat : (TT_TWIDDLE*)fft_lut_tw4);
    static constexpr TT_TWIDDLE* __restrict tw8 =
        (std::is_same<TT_DATA, cfloat>::value ? (TT_TWIDDLE*)fft_lut_tw8_cfloat : (TT_TWIDDLE*)fft_lut_tw8);
    static constexpr TT_TWIDDLE* __restrict tw16 =
        (std::is_same<TT_DATA, cfloat>::value ? (TT_TWIDDLE*)fft_lut_tw16_cfloat : (TT_TWIDDLE*)fft_lut_tw16);
    static constexpr TT_TWIDDLE* __restrict tw32 =
        (std::is_same<TT_DATA, cfloat>::value ? (TT_TWIDDLE*)fft_lut_tw32_cfloat : (TT_TWIDDLE*)fft_lut_tw32);
    static constexpr TT_TWIDDLE* __restrict tw64 =
        (std::is_same<TT_DATA, cfloat>::value ? (TT_TWIDDLE*)fft_lut_tw64_cfloat : (TT_TWIDDLE*)fft_lut_tw64);
    static constexpr TT_TWIDDLE* __restrict tw128 =
        (std::is_same<TT_DATA, cfloat>::value ? (TT_TWIDDLE*)fft_lut_tw128_cfloat : (TT_TWIDDLE*)fft_lut_tw128_half);

#ifdef __FFT_SPOOF_R4__
    static constexpr TT_TWIDDLE* __restrict tw1_2 = NULL;
    static constexpr TT_TWIDDLE* __restrict tw2_4 = NULL;
    static constexpr TT_TWIDDLE* __restrict tw4_8 = NULL;
    static constexpr TT_TWIDDLE* __restrict tw8_16 = NULL;
    static constexpr TT_TWIDDLE* __restrict tw16_32 = NULL;
    static constexpr TT_TWIDDLE* __restrict tw32_64 = NULL;
    static constexpr TT_TWIDDLE* __restrict tw64_128 = NULL;
    static constexpr TT_TWIDDLE* __restrict tw128_256 = NULL;
#endif //__FFT_SPOOF_R4__
#ifdef __FFT_TRUE_R4__
  static constexpr TT_TWIDDLE* __restrict tw1_2       = (std::is_same<TT_DATA,cfloat>::value ? NULL : (TT_TWIDDLE*)fft_lut_r4_1_2;
  static constexpr TT_TWIDDLE* __restrict tw2_4       = (std::is_same<TT_DATA,cfloat>::value ? NULL : (TT_TWIDDLE*)fft_lut_r4_2_4;
  static constexpr TT_TWIDDLE* __restrict tw4_8       = (std::is_same<TT_DATA,cfloat>::value ? NULL : (TT_TWIDDLE*)fft_lut_r4_4_8;
  static constexpr TT_TWIDDLE* __restrict tw8_16      = (std::is_same<TT_DATA,cfloat>::value ? NULL : (TT_TWIDDLE*)fft_lut_r4_8_16;
  static constexpr TT_TWIDDLE* __restrict tw16_32     = (std::is_same<TT_DATA,cfloat>::value ? NULL : (TT_TWIDDLE*)fft_lut_r4_16_32;
  static constexpr TT_TWIDDLE* __restrict tw32_64     = (std::is_same<TT_DATA,cfloat>::value ? NULL : (TT_TWIDDLE*)fft_lut_r4_32_64;
  static constexpr TT_TWIDDLE* __restrict tw64_128    = (std::is_same<TT_DATA,cfloat>::value ? NULL : (TT_TWIDDLE*)fft_lut_r4_64_128;
  static constexpr TT_TWIDDLE* __restrict tw128_256   = (std::is_same<TT_DATA,cfloat>::value ? NULL : (TT_TWIDDLE*)fft_lut_r4_128_256;
#endif //__FFT_TRUE_R4__

  static TT_TWIDDLE* tw_table[kMaxPointLog*2] = {tw1,tw2,tw4,tw8,tw16,tw32,tw64,tw128,NULL,NULL,NULL,NULL,
      tw1_2, tw2_4, tw4_8, tw8_16, tw16_32, tw32_64, tw64_128, tw128_256, NULL, NULL, NULL, NULL};

  TT_DATA* xbuff = (TT_DATA*)inputx->ptr;
  T_internalDataType* tmp1_buf = (T_internalDataType*)fft_256_tmp1;
  T_internalDataType* tmp2_buf = (T_internalDataType*)xbuff;

  stages.stagePreamble(tw_table, tmp1_buf, tmp2_buf, inputx, outputy);
};

template <typename TT_OUT_DATA,
          unsigned int TP_FFT_NIFFT,
          unsigned int TP_SHIFT,
          unsigned int TP_START_RANK,
          unsigned int TP_END_RANK,
          unsigned int TP_DYN_PT_SIZE,
          unsigned int TP_WINDOW_VSIZE,
          unsigned int TP_ORIG_PAR_POWER>
INLINE_DECL void kernelFFTClass<cint16,
                                TT_OUT_DATA,
                                cint16,
                                FFT256_SIZE,
                                TP_FFT_NIFFT,
                                TP_SHIFT,
                                TP_START_RANK,
                                TP_END_RANK,
                                TP_DYN_PT_SIZE,
                                TP_WINDOW_VSIZE,
                                TP_ORIG_PAR_POWER>::kernelFFT(input_window<cint16>* __restrict inputx,
                                                              output_window<TT_OUT_DATA>* __restrict outputy) {
    typedef cint16 TT_DATA;
    typedef cint16 TT_TWIDDLE; // essentially a constant for thie specialization
    typedef cint32_t T_internalDataType;
    static constexpr cint16* __restrict tw1 = (cint16*)fft_lut_tw1;
    static constexpr cint16* __restrict tw2 = (cint16*)fft_lut_tw2;
    static constexpr cint16* __restrict tw4 = (cint16*)fft_lut_tw4;
    static constexpr cint16* __restrict tw8 = (cint16*)fft_lut_tw8;
    static constexpr cint16* __restrict tw16 = (cint16*)fft_lut_tw16;
    static constexpr cint16* __restrict tw32 = (cint16*)fft_lut_tw32;
    static constexpr cint16* __restrict tw64 = (cint16*)fft_lut_tw64;
    static constexpr cint16* __restrict tw128 = (cint16*)fft_lut_tw128_half;

#ifdef __FFT_SPOOF_R4__
    static constexpr TT_TWIDDLE* __restrict tw1_2 = NULL;
    static constexpr TT_TWIDDLE* __restrict tw2_4 = NULL;
    static constexpr TT_TWIDDLE* __restrict tw4_8 = NULL;
    static constexpr TT_TWIDDLE* __restrict tw8_16 = NULL;
    static constexpr TT_TWIDDLE* __restrict tw16_32 = NULL;
    static constexpr TT_TWIDDLE* __restrict tw32_64 = NULL;
    static constexpr TT_TWIDDLE* __restrict tw64_128 = NULL;
    static constexpr TT_TWIDDLE* __restrict tw128_256 = NULL;
#endif //__FFT_SPOOF_R4__
#ifdef __FFT_TRUE_R4__
    static constexpr TT_TWIDDLE* __restrict tw1_2 = (TT_TWIDDLE*)fft_lut_r4_1_2;
    static constexpr TT_TWIDDLE* __restrict tw2_4 = (TT_TWIDDLE*)fft_lut_r4_2_4;
    static constexpr TT_TWIDDLE* __restrict tw4_8 = (TT_TWIDDLE*)fft_lut_r4_4_8;
    static constexpr TT_TWIDDLE* __restrict tw8_16 = (TT_TWIDDLE*)fft_lut_r4_8_16;
    static constexpr TT_TWIDDLE* __restrict tw16_32 = (TT_TWIDDLE*)fft_lut_r4_16_32;
    static constexpr TT_TWIDDLE* __restrict tw32_64 = (TT_TWIDDLE*)fft_lut_r4_32_64;
    static constexpr TT_TWIDDLE* __restrict tw64_128 = (TT_TWIDDLE*)fft_lut_r4_64_128;
    static constexpr TT_TWIDDLE* __restrict tw128_256 = (TT_TWIDDLE*)fft_lut_r4_128_256;
#endif //__FFT_TRUE_R4__

    static cint16* tw_table[kMaxPointLog * 2] = {tw1,     tw2,     tw4,      tw8,       tw16,  tw32,  tw64,  tw128,
                                                 NULL,    NULL,    NULL,     NULL,      tw1_2, tw2_4, tw4_8, tw8_16,
                                                 tw16_32, tw32_64, tw64_128, tw128_256, NULL,  NULL,  NULL,  NULL};

    cint16* xbuff = (cint16*)inputx->ptr;
    TT_OUT_DATA* obuff = (TT_OUT_DATA*)outputy->ptr;
    T_internalDataType* tmp1_buf = (T_internalDataType*)fft_256_tmp1;
    T_internalDataType* tmp2_buf = (T_internalDataType*)fft_256_tmp2;

    stages.stagePreamble(tw_table, tmp1_buf, tmp2_buf, inputx, outputy);
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
          unsigned int TP_ORIG_PAR_POWER>
INLINE_DECL void kernelFFTClass<TT_DATA,
                                TT_OUT_DATA,
                                TT_TWIDDLE,
                                FFT128_SIZE,
                                TP_FFT_NIFFT,
                                TP_SHIFT,
                                TP_START_RANK,
                                TP_END_RANK,
                                TP_DYN_PT_SIZE,
                                TP_WINDOW_VSIZE,
                                TP_ORIG_PAR_POWER>::kernelFFT(input_window<TT_DATA>* __restrict inputx,
                                                              output_window<TT_OUT_DATA>* __restrict outputy) {
    typedef typename std::conditional<std::is_same<TT_DATA, cint16>::value, cint32_t, TT_DATA>::type T_internalDataType;
    static constexpr TT_TWIDDLE* __restrict tw1 =
        (std::is_same<TT_DATA, cfloat>::value ? (TT_TWIDDLE*)fft_lut_tw1_cfloat : (TT_TWIDDLE*)fft_lut_tw1);
    static constexpr TT_TWIDDLE* __restrict tw2 =
        (std::is_same<TT_DATA, cfloat>::value ? (TT_TWIDDLE*)fft_lut_tw2_cfloat : (TT_TWIDDLE*)fft_lut_tw2);
    static constexpr TT_TWIDDLE* __restrict tw4 =
        (std::is_same<TT_DATA, cfloat>::value ? (TT_TWIDDLE*)fft_lut_tw4_cfloat : (TT_TWIDDLE*)fft_lut_tw4);
    static constexpr TT_TWIDDLE* __restrict tw8 =
        (std::is_same<TT_DATA, cfloat>::value ? (TT_TWIDDLE*)fft_lut_tw8_cfloat : (TT_TWIDDLE*)fft_lut_tw8);
    static constexpr TT_TWIDDLE* __restrict tw16 =
        (std::is_same<TT_DATA, cfloat>::value ? (TT_TWIDDLE*)fft_lut_tw16_cfloat : (TT_TWIDDLE*)fft_lut_tw16);
    static constexpr TT_TWIDDLE* __restrict tw32 =
        (std::is_same<TT_DATA, cfloat>::value ? (TT_TWIDDLE*)fft_lut_tw32_cfloat : (TT_TWIDDLE*)fft_lut_tw32);
    static constexpr TT_TWIDDLE* __restrict tw64 =
        (std::is_same<TT_DATA, cfloat>::value ? (TT_TWIDDLE*)fft_lut_tw64_cfloat : (TT_TWIDDLE*)fft_lut_tw64_half);

#ifdef __FFT_SPOOF_R4__
    static constexpr TT_TWIDDLE* __restrict tw1_2 = NULL;
    static constexpr TT_TWIDDLE* __restrict tw2_4 = NULL;
    static constexpr TT_TWIDDLE* __restrict tw4_8 = NULL;
    static constexpr TT_TWIDDLE* __restrict tw8_16 = NULL;
    static constexpr TT_TWIDDLE* __restrict tw16_32 = NULL;
    static constexpr TT_TWIDDLE* __restrict tw32_64 = NULL;
    static constexpr TT_TWIDDLE* __restrict tw64_128 = NULL;
#endif //__FFT_SPOOF_R4__
#ifdef __FFT_TRUE_R4__
  static constexpr TT_TWIDDLE* __restrict tw1_2       = (std::is_same<TT_DATA,cfloat>::value ? NULL : (TT_TWIDDLE*)fft_lut_r4_1_2;
  static constexpr TT_TWIDDLE* __restrict tw2_4       = (std::is_same<TT_DATA,cfloat>::value ? NULL : (TT_TWIDDLE*)fft_lut_r4_2_4;
  static constexpr TT_TWIDDLE* __restrict tw4_8       = (std::is_same<TT_DATA,cfloat>::value ? NULL : (TT_TWIDDLE*)fft_lut_r4_4_8;
  static constexpr TT_TWIDDLE* __restrict tw8_16      = (std::is_same<TT_DATA,cfloat>::value ? NULL : (TT_TWIDDLE*)fft_lut_r4_8_16;
  static constexpr TT_TWIDDLE* __restrict tw16_32     = (std::is_same<TT_DATA,cfloat>::value ? NULL : (TT_TWIDDLE*)fft_lut_r4_16_32;
  static constexpr TT_TWIDDLE* __restrict tw32_64     = (std::is_same<TT_DATA,cfloat>::value ? NULL : (TT_TWIDDLE*)fft_lut_r4_32_64;
  static constexpr TT_TWIDDLE* __restrict tw64_128    = (std::is_same<TT_DATA,cfloat>::value ? NULL : (TT_TWIDDLE*)fft_lut_r4_64_128;
#endif //__FFT_TRUE_R4__

  static TT_TWIDDLE* tw_table[kMaxPointLog*2] = {tw1,tw2,tw4,tw8,tw16,tw32,tw64,NULL,NULL,NULL,NULL,NULL,
      tw1_2, tw2_4, tw4_8, tw8_16, tw16_32, tw32_64, tw64_128,NULL,NULL,NULL,NULL,NULL};

  TT_DATA* xbuff = (TT_DATA*)inputx->ptr;
  T_internalDataType* tmp1_buf = (T_internalDataType*)fft_128_tmp1;
  T_internalDataType* tmp2_buf = (T_internalDataType*)xbuff;

  stages.stagePreamble(tw_table, tmp1_buf, tmp2_buf, inputx, outputy);
};

template <typename TT_OUT_DATA,
          unsigned int TP_FFT_NIFFT,
          unsigned int TP_SHIFT,
          unsigned int TP_START_RANK,
          unsigned int TP_END_RANK,
          unsigned int TP_DYN_PT_SIZE,
          unsigned int TP_WINDOW_VSIZE,
          unsigned int TP_ORIG_PAR_POWER>
INLINE_DECL void kernelFFTClass<cint16,
                                TT_OUT_DATA,
                                cint16,
                                FFT128_SIZE,
                                TP_FFT_NIFFT,
                                TP_SHIFT,
                                TP_START_RANK,
                                TP_END_RANK,
                                TP_DYN_PT_SIZE,
                                TP_WINDOW_VSIZE,
                                TP_ORIG_PAR_POWER>::kernelFFT(input_window<cint16>* __restrict inputx,
                                                              output_window<TT_OUT_DATA>* __restrict outputy) {
    typedef cint16 TT_DATA;
    typedef cint16 TT_TWIDDLE; // essentially a constant for thie specialization
    typedef cint32_t T_internalDataType;
    static constexpr cint16* __restrict tw1 = (cint16*)fft_lut_tw1;
    static constexpr cint16* __restrict tw2 = (cint16*)fft_lut_tw2;
    static constexpr cint16* __restrict tw4 = (cint16*)fft_lut_tw4;
    static constexpr cint16* __restrict tw8 = (cint16*)fft_lut_tw8;
    static constexpr cint16* __restrict tw16 = (cint16*)fft_lut_tw16;
    static constexpr cint16* __restrict tw32 = (cint16*)fft_lut_tw32;
    static constexpr cint16* __restrict tw64 = (cint16*)fft_lut_tw64_half;

#ifdef __FFT_SPOOF_R4__
    static constexpr TT_TWIDDLE* __restrict tw1_2 = NULL;
    static constexpr TT_TWIDDLE* __restrict tw2_4 = NULL;
    static constexpr TT_TWIDDLE* __restrict tw4_8 = NULL;
    static constexpr TT_TWIDDLE* __restrict tw8_16 = NULL;
    static constexpr TT_TWIDDLE* __restrict tw16_32 = NULL;
    static constexpr TT_TWIDDLE* __restrict tw32_64 = NULL;
    static constexpr TT_TWIDDLE* __restrict tw64_128 = NULL;
#endif //__FFT_SPOOF_R4__
#ifdef __FFT_TRUE_R4__
    static constexpr TT_TWIDDLE* __restrict tw1_2 = (TT_TWIDDLE*)fft_lut_r4_1_2;
    static constexpr TT_TWIDDLE* __restrict tw2_4 = (TT_TWIDDLE*)fft_lut_r4_2_4;
    static constexpr TT_TWIDDLE* __restrict tw4_8 = (TT_TWIDDLE*)fft_lut_r4_4_8;
    static constexpr TT_TWIDDLE* __restrict tw8_16 = (TT_TWIDDLE*)fft_lut_r4_8_16;
    static constexpr TT_TWIDDLE* __restrict tw16_32 = (TT_TWIDDLE*)fft_lut_r4_16_32;
    static constexpr TT_TWIDDLE* __restrict tw32_64 = (TT_TWIDDLE*)fft_lut_r4_32_64;
    static constexpr TT_TWIDDLE* __restrict tw64_128 = (TT_TWIDDLE*)fft_lut_r4_64_128;
#endif //__FFT_TRUE_R4__

    static cint16* tw_table[kMaxPointLog * 2] = {tw1,     tw2,     tw4,      tw8,  tw16,  tw32,  tw64,  NULL,
                                                 NULL,    NULL,    NULL,     NULL, tw1_2, tw2_4, tw4_8, tw8_16,
                                                 tw16_32, tw32_64, tw64_128, NULL, NULL,  NULL,  NULL,  NULL};

    cint16* xbuff = (cint16*)inputx->ptr;
    TT_OUT_DATA* obuff = (TT_OUT_DATA*)outputy->ptr;
    T_internalDataType* tmp1_buf = (T_internalDataType*)fft_128_tmp1;
    T_internalDataType* tmp2_buf = (T_internalDataType*)fft_128_tmp2;

    stages.stagePreamble(tw_table, tmp1_buf, tmp2_buf, inputx, outputy);
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
          unsigned int TP_ORIG_PAR_POWER>
INLINE_DECL void kernelFFTClass<TT_DATA,
                                TT_OUT_DATA,
                                TT_TWIDDLE,
                                FFT64_SIZE,
                                TP_FFT_NIFFT,
                                TP_SHIFT,
                                TP_START_RANK,
                                TP_END_RANK,
                                TP_DYN_PT_SIZE,
                                TP_WINDOW_VSIZE,
                                TP_ORIG_PAR_POWER>::kernelFFT(input_window<TT_DATA>* __restrict inputx,
                                                              output_window<TT_OUT_DATA>* __restrict outputy) {
    typedef typename std::conditional<std::is_same<TT_DATA, cint16>::value, cint32_t, TT_DATA>::type T_internalDataType;
    static constexpr TT_TWIDDLE* __restrict tw1 =
        (std::is_same<TT_DATA, cfloat>::value ? (TT_TWIDDLE*)fft_lut_tw1_cfloat : (TT_TWIDDLE*)fft_lut_tw1);
    static constexpr TT_TWIDDLE* __restrict tw2 =
        (std::is_same<TT_DATA, cfloat>::value ? (TT_TWIDDLE*)fft_lut_tw2_cfloat : (TT_TWIDDLE*)fft_lut_tw2);
    static constexpr TT_TWIDDLE* __restrict tw4 =
        (std::is_same<TT_DATA, cfloat>::value ? (TT_TWIDDLE*)fft_lut_tw4_cfloat : (TT_TWIDDLE*)fft_lut_tw4);
    static constexpr TT_TWIDDLE* __restrict tw8 =
        (std::is_same<TT_DATA, cfloat>::value ? (TT_TWIDDLE*)fft_lut_tw8_cfloat : (TT_TWIDDLE*)fft_lut_tw8);
    static constexpr TT_TWIDDLE* __restrict tw16 =
        (std::is_same<TT_DATA, cfloat>::value ? (TT_TWIDDLE*)fft_lut_tw16_cfloat : (TT_TWIDDLE*)fft_lut_tw16);
    static constexpr TT_TWIDDLE* __restrict tw32 =
        (std::is_same<TT_DATA, cfloat>::value ? (TT_TWIDDLE*)fft_lut_tw32_cfloat : (TT_TWIDDLE*)fft_lut_tw32_half);

#ifdef __FFT_SPOOF_R4__
    static constexpr TT_TWIDDLE* __restrict tw1_2 = NULL;
    static constexpr TT_TWIDDLE* __restrict tw2_4 = NULL;
    static constexpr TT_TWIDDLE* __restrict tw4_8 = NULL;
    static constexpr TT_TWIDDLE* __restrict tw8_16 = NULL;
    static constexpr TT_TWIDDLE* __restrict tw16_32 = NULL;
    static constexpr TT_TWIDDLE* __restrict tw32_64 = NULL;
#endif //__FFT_SPOOF_R4__
#ifdef __FFT_TRUE_R4__
  static constexpr TT_TWIDDLE* __restrict tw1_2       = (std::is_same<TT_DATA,cfloat>::value ? NULL : (TT_TWIDDLE*)fft_lut_r4_1_2;
  static constexpr TT_TWIDDLE* __restrict tw2_4       = (std::is_same<TT_DATA,cfloat>::value ? NULL : (TT_TWIDDLE*)fft_lut_r4_2_4;
  static constexpr TT_TWIDDLE* __restrict tw4_8       = (std::is_same<TT_DATA,cfloat>::value ? NULL : (TT_TWIDDLE*)fft_lut_r4_4_8;
  static constexpr TT_TWIDDLE* __restrict tw8_16      = (std::is_same<TT_DATA,cfloat>::value ? NULL : (TT_TWIDDLE*)fft_lut_r4_8_16;
  static constexpr TT_TWIDDLE* __restrict tw16_32     = (std::is_same<TT_DATA,cfloat>::value ? NULL : (TT_TWIDDLE*)fft_lut_r4_16_32;
  static constexpr TT_TWIDDLE* __restrict tw32_64     = (std::is_same<TT_DATA,cfloat>::value ? NULL : (TT_TWIDDLE*)fft_lut_r4_32_64;
#endif //__FFT_TRUE_R4__
 static TT_TWIDDLE* tw_table[kMaxPointLog*2] = {tw1,tw2,tw4,tw8,tw16,tw32,NULL,NULL,NULL,NULL,NULL,NULL,
      tw1_2, tw2_4, tw4_8, tw8_16, tw16_32, tw32_64, NULL,NULL,NULL,NULL,NULL,NULL};

  TT_DATA* xbuff = (TT_DATA*)inputx->ptr;
  T_internalDataType* tmp1_buf = (T_internalDataType*)fft_128_tmp1;
  T_internalDataType* tmp2_buf = (T_internalDataType*)xbuff;

  stages.stagePreamble(tw_table, tmp1_buf, tmp2_buf, inputx, outputy);
};

template <typename TT_OUT_DATA,
          unsigned int TP_FFT_NIFFT,
          unsigned int TP_SHIFT,
          unsigned int TP_START_RANK,
          unsigned int TP_END_RANK,
          unsigned int TP_DYN_PT_SIZE,
          unsigned int TP_WINDOW_VSIZE,
          unsigned int TP_ORIG_PAR_POWER>
INLINE_DECL void kernelFFTClass<cint16,
                                TT_OUT_DATA,
                                cint16,
                                FFT64_SIZE,
                                TP_FFT_NIFFT,
                                TP_SHIFT,
                                TP_START_RANK,
                                TP_END_RANK,
                                TP_DYN_PT_SIZE,
                                TP_WINDOW_VSIZE,
                                TP_ORIG_PAR_POWER>::kernelFFT(input_window<cint16>* __restrict inputx,
                                                              output_window<TT_OUT_DATA>* __restrict outputy) {
    typedef cint16 TT_DATA;
    typedef cint16 TT_TWIDDLE; // essentially a constant for thie specialization
    typedef cint32_t T_internalDataType;
    static constexpr cint16* __restrict tw1 = (cint16*)fft_lut_tw1;
    static constexpr cint16* __restrict tw2 = (cint16*)fft_lut_tw2;
    static constexpr cint16* __restrict tw4 = (cint16*)fft_lut_tw4;
    static constexpr cint16* __restrict tw8 = (cint16*)fft_lut_tw8;
    static constexpr cint16* __restrict tw16 = (cint16*)fft_lut_tw16;
    static constexpr cint16* __restrict tw32 = (cint16*)fft_lut_tw32_half;

#ifdef __FFT_SPOOF_R4__
    static constexpr TT_TWIDDLE* __restrict tw1_2 = NULL;
    static constexpr TT_TWIDDLE* __restrict tw2_4 = NULL;
    static constexpr TT_TWIDDLE* __restrict tw4_8 = NULL;
    static constexpr TT_TWIDDLE* __restrict tw8_16 = NULL;
    static constexpr TT_TWIDDLE* __restrict tw16_32 = NULL;
    static constexpr TT_TWIDDLE* __restrict tw32_64 = NULL;
#endif //__FFT_SPOOF_R4__
#ifdef __FFT_TRUE_R4__
    static constexpr TT_TWIDDLE* __restrict tw1_2 = (TT_TWIDDLE*)fft_lut_r4_1_2;
    static constexpr TT_TWIDDLE* __restrict tw2_4 = (TT_TWIDDLE*)fft_lut_r4_2_4;
    static constexpr TT_TWIDDLE* __restrict tw4_8 = (TT_TWIDDLE*)fft_lut_r4_4_8;
    static constexpr TT_TWIDDLE* __restrict tw8_16 = (TT_TWIDDLE*)fft_lut_r4_8_16;
    static constexpr TT_TWIDDLE* __restrict tw16_32 = (TT_TWIDDLE*)fft_lut_r4_16_32;
    static constexpr TT_TWIDDLE* __restrict tw32_64 = (TT_TWIDDLE*)fft_lut_r4_32_64;
#endif //__FFT_TRUE_R4__

    static cint16* tw_table[kMaxPointLog * 2] = {tw1,     tw2,     tw4,  tw8,  tw16,  tw32,  NULL,  NULL,
                                                 NULL,    NULL,    NULL, NULL, tw1_2, tw2_4, tw4_8, tw8_16,
                                                 tw16_32, tw32_64, NULL, NULL, NULL,  NULL,  NULL,  NULL};

    cint16* xbuff = (cint16*)inputx->ptr;
    TT_OUT_DATA* obuff = (TT_OUT_DATA*)outputy->ptr;
    T_internalDataType* tmp1_buf = (T_internalDataType*)fft_128_tmp1;
    T_internalDataType* tmp2_buf = (T_internalDataType*)fft_128_tmp2;

    stages.stagePreamble(tw_table, tmp1_buf, tmp2_buf, inputx, outputy);
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
          unsigned int TP_ORIG_PAR_POWER>
INLINE_DECL void kernelFFTClass<TT_DATA,
                                TT_OUT_DATA,
                                TT_TWIDDLE,
                                FFT32_SIZE,
                                TP_FFT_NIFFT,
                                TP_SHIFT,
                                TP_START_RANK,
                                TP_END_RANK,
                                TP_DYN_PT_SIZE,
                                TP_WINDOW_VSIZE,
                                TP_ORIG_PAR_POWER>::kernelFFT(input_window<TT_DATA>* __restrict inputx,
                                                              output_window<TT_OUT_DATA>* __restrict outputy) {
    typedef typename std::conditional<std::is_same<TT_DATA, cint16>::value, cint32_t, TT_DATA>::type T_internalDataType;
    static constexpr TT_TWIDDLE* __restrict tw1 =
        (std::is_same<TT_DATA, cfloat>::value ? (TT_TWIDDLE*)fft_lut_tw1_cfloat : (TT_TWIDDLE*)fft_lut_tw1);
    static constexpr TT_TWIDDLE* __restrict tw2 =
        (std::is_same<TT_DATA, cfloat>::value ? (TT_TWIDDLE*)fft_lut_tw2_cfloat : (TT_TWIDDLE*)fft_lut_tw2);
    static constexpr TT_TWIDDLE* __restrict tw4 =
        (std::is_same<TT_DATA, cfloat>::value ? (TT_TWIDDLE*)fft_lut_tw4_cfloat : (TT_TWIDDLE*)fft_lut_tw4);
    static constexpr TT_TWIDDLE* __restrict tw8 =
        (std::is_same<TT_DATA, cfloat>::value ? (TT_TWIDDLE*)fft_lut_tw8_cfloat : (TT_TWIDDLE*)fft_lut_tw8);
    static constexpr TT_TWIDDLE* __restrict tw16 =
        (std::is_same<TT_DATA, cfloat>::value ? (TT_TWIDDLE*)fft_lut_tw16_cfloat : (TT_TWIDDLE*)fft_lut_tw16_half);

#ifdef __FFT_SPOOF_R4__
    static constexpr TT_TWIDDLE* __restrict tw1_2 = NULL;
    static constexpr TT_TWIDDLE* __restrict tw2_4 = NULL;
    static constexpr TT_TWIDDLE* __restrict tw4_8 = NULL;
    static constexpr TT_TWIDDLE* __restrict tw8_16 = NULL;
    static constexpr TT_TWIDDLE* __restrict tw16_32 = NULL;
#endif //__FFT_SPOOF_R4__
#ifdef __FFT_TRUE_R4__
  static constexpr TT_TWIDDLE* __restrict tw1_2       = (std::is_same<TT_DATA,cfloat>::value ? NULL : (TT_TWIDDLE*)fft_lut_r4_1_2;
  static constexpr TT_TWIDDLE* __restrict tw2_4       = (std::is_same<TT_DATA,cfloat>::value ? NULL : (TT_TWIDDLE*)fft_lut_r4_2_4;
  static constexpr TT_TWIDDLE* __restrict tw4_8       = (std::is_same<TT_DATA,cfloat>::value ? NULL : (TT_TWIDDLE*)fft_lut_r4_4_8;
  static constexpr TT_TWIDDLE* __restrict tw8_16      = (std::is_same<TT_DATA,cfloat>::value ? NULL : (TT_TWIDDLE*)fft_lut_r4_8_16;
  static constexpr TT_TWIDDLE* __restrict tw16_32     = (std::is_same<TT_DATA,cfloat>::value ? NULL : (TT_TWIDDLE*)fft_lut_r4_16_32;
#endif //__FFT_TRUE_R4__

  static TT_TWIDDLE* tw_table[kMaxPointLog*2] = {tw1,tw2,tw4,tw8,tw16,NULL,NULL,NULL,NULL,NULL,NULL,NULL,
      tw1_2, tw2_4, tw4_8, tw8_16, tw16_32, NULL,NULL,NULL,NULL,NULL,NULL,NULL};

  TT_DATA* xbuff = (TT_DATA*)inputx->ptr;
  T_internalDataType* tmp1_buf = (T_internalDataType*)fft_128_tmp1;
  T_internalDataType* tmp2_buf = (T_internalDataType*)xbuff;

  stages.stagePreamble(tw_table, tmp1_buf, tmp2_buf, inputx, outputy);
};

template <typename TT_OUT_DATA,
          unsigned int TP_FFT_NIFFT,
          unsigned int TP_SHIFT,
          unsigned int TP_START_RANK,
          unsigned int TP_END_RANK,
          unsigned int TP_DYN_PT_SIZE,
          unsigned int TP_WINDOW_VSIZE,
          unsigned int TP_ORIG_PAR_POWER>
INLINE_DECL void kernelFFTClass<cint16,
                                TT_OUT_DATA,
                                cint16,
                                FFT32_SIZE,
                                TP_FFT_NIFFT,
                                TP_SHIFT,
                                TP_START_RANK,
                                TP_END_RANK,
                                TP_DYN_PT_SIZE,
                                TP_WINDOW_VSIZE,
                                TP_ORIG_PAR_POWER>::kernelFFT(input_window<cint16>* __restrict inputx,
                                                              output_window<TT_OUT_DATA>* __restrict outputy) {
    typedef cint16 TT_DATA;
    typedef cint16 TT_TWIDDLE; // essentially a constant for thie specialization
    typedef cint32_t T_internalDataType;
    static constexpr cint16* __restrict tw1 = (cint16*)fft_lut_tw1;
    static constexpr cint16* __restrict tw2 = (cint16*)fft_lut_tw2;
    static constexpr cint16* __restrict tw4 = (cint16*)fft_lut_tw4;
    static constexpr cint16* __restrict tw8 = (cint16*)fft_lut_tw8;
    static constexpr cint16* __restrict tw16 = (cint16*)fft_lut_tw16_half;

#ifdef __FFT_SPOOF_R4__
    static constexpr TT_TWIDDLE* __restrict tw1_2 = NULL;
    static constexpr TT_TWIDDLE* __restrict tw2_4 = NULL;
    static constexpr TT_TWIDDLE* __restrict tw4_8 = NULL;
    static constexpr TT_TWIDDLE* __restrict tw8_16 = NULL;
    static constexpr TT_TWIDDLE* __restrict tw16_32 = NULL;
#endif //__FFT_SPOOF_R4__
#ifdef __FFT_TRUE_R4__
    static constexpr TT_TWIDDLE* __restrict tw1_2 = (TT_TWIDDLE*)fft_lut_r4_1_2;
    static constexpr TT_TWIDDLE* __restrict tw2_4 = (TT_TWIDDLE*)fft_lut_r4_2_4;
    static constexpr TT_TWIDDLE* __restrict tw4_8 = (TT_TWIDDLE*)fft_lut_r4_4_8;
    static constexpr TT_TWIDDLE* __restrict tw8_16 = (TT_TWIDDLE*)fft_lut_r4_8_16;
    static constexpr TT_TWIDDLE* __restrict tw16_32 = (TT_TWIDDLE*)fft_lut_r4_16_32;
#endif //__FFT_TRUE_R4__

    static cint16* tw_table[kMaxPointLog * 2] = {tw1,     tw2,  tw4,  tw8,  tw16,  NULL,  NULL,  NULL,
                                                 NULL,    NULL, NULL, NULL, tw1_2, tw2_4, tw4_8, tw8_16,
                                                 tw16_32, NULL, NULL, NULL, NULL,  NULL,  NULL,  NULL};

    cint16* xbuff = (cint16*)inputx->ptr;
    TT_OUT_DATA* obuff = (TT_OUT_DATA*)outputy->ptr;
    T_internalDataType* tmp1_buf = (T_internalDataType*)fft_128_tmp1;
    T_internalDataType* tmp2_buf = (T_internalDataType*)fft_128_tmp2;

    stages.stagePreamble(tw_table, tmp1_buf, tmp2_buf, inputx, outputy);
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
          unsigned int TP_ORIG_PAR_POWER>
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
                                TP_ORIG_PAR_POWER>::kernelFFT(input_window<TT_DATA>* __restrict inputx,
                                                              output_window<TT_OUT_DATA>* __restrict outputy) {
    const unsigned int TP_POINT_SIZE = FFT16_SIZE;

    typedef typename std::conditional<std::is_same<TT_DATA, cint16>::value, cint32_t, TT_DATA>::type T_internalDataType;
    static constexpr TT_TWIDDLE* __restrict tw1 =
        (std::is_same<TT_DATA, cfloat>::value ? (TT_TWIDDLE*)fft_lut_tw1_cfloat : (TT_TWIDDLE*)fft_lut_tw1);
    static constexpr TT_TWIDDLE* __restrict tw2 =
        (std::is_same<TT_DATA, cfloat>::value ? (TT_TWIDDLE*)fft_lut_tw2_cfloat : (TT_TWIDDLE*)fft_lut_tw2);
    static constexpr TT_TWIDDLE* __restrict tw4 =
        (std::is_same<TT_DATA, cfloat>::value ? (TT_TWIDDLE*)fft_lut_tw4_cfloat : (TT_TWIDDLE*)fft_lut_tw4);
    static constexpr TT_TWIDDLE* __restrict tw8 =
        (std::is_same<TT_DATA, cfloat>::value ? (TT_TWIDDLE*)fft_lut_tw8_cfloat : (TT_TWIDDLE*)fft_lut_tw8_half);

#ifdef __FFT_SPOOF_R4__
    static constexpr TT_TWIDDLE* __restrict tw1_2 = NULL;
    static constexpr TT_TWIDDLE* __restrict tw2_4 = NULL;
    static constexpr TT_TWIDDLE* __restrict tw4_8 = NULL;
    static constexpr TT_TWIDDLE* __restrict tw8_16 = NULL;
    static constexpr TT_TWIDDLE* __restrict tw16_32 = NULL;
#endif //__FFT_SPOOF_R4__
#ifdef __FFT_TRUE_R4__
  static constexpr TT_TWIDDLE* __restrict tw1_2       = (std::is_same<TT_DATA,cfloat>::value ? NULL : (TT_TWIDDLE*)fft_lut_r4_1_2;
  static constexpr TT_TWIDDLE* __restrict tw2_4       = (std::is_same<TT_DATA,cfloat>::value ? NULL : (TT_TWIDDLE*)fft_lut_r4_2_4;
  static constexpr TT_TWIDDLE* __restrict tw4_8       = (std::is_same<TT_DATA,cfloat>::value ? NULL : (TT_TWIDDLE*)fft_lut_r4_4_8;
  static constexpr TT_TWIDDLE* __restrict tw8_16      = (std::is_same<TT_DATA,cfloat>::value ? NULL : (TT_TWIDDLE*)fft_lut_r4_8_16;
#endif //__FFT_TRUE_R4__

  static TT_TWIDDLE* tw_table[kMaxPointLog*2] = {tw1,tw2,tw4,tw8,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,
      tw1_2, tw2_4, tw4_8, tw8_16, NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL};

  T_internalDataType* ktmp1_buf = (T_internalDataType*)fft_128_tmp1; //works because cint32 is the same size as cfloat
  T_internalDataType* ktmp2_buf; //not initialized because input window is re-used for this storage

  TT_DATA* xbuff = (TT_DATA*)inputx->ptr;
  TT_OUT_DATA* obuff = (TT_OUT_DATA*)outputy->ptr;
  T_internalDataType* tmp1_buf = (T_internalDataType*)fft_128_tmp1;
  // The following break with the pattern for other point sizes is a workaround for a compiler issue.
  alignas(32) T_internalDataType  tmp2_buf[FFT16_SIZE];//must be 256 bit aligned
  alignas(32) T_internalDataType  tmp3_buf[FFT16_SIZE];//must be 256 bit aligned

  bool inv = TP_FFT_NIFFT == 1 ? false : true;

  //This code should be moved to the constructor preferably
  set_rnd(rnd_pos_inf); //Match the twiddle round mode of Matlab.
  set_sat();            //do saturate.
  for (int iter = 0; iter< TP_WINDOW_VSIZE/TP_POINT_SIZE; iter++)
  chess_prepare_for_pipelining
  chess_loop_range(TP_WINDOW_VSIZE/TP_POINT_SIZE,)
  {
        if
            constexpr(std::is_same<TT_DATA, cfloat>::value &&
                      (TP_END_RANK - TP_START_RANK == 4)) { // special case for uncascaded 16pt cfloat
                stage_radix2_dit<cfloat, cfloat, cfloat, 8>((cfloat*)xbuff, (cfloat*)tw1, FFT16_SIZE, 0,
                                                            (cfloat*)tmp1_buf, inv);
                stage_radix2_dit<cfloat, cfloat, cfloat, 4>((cfloat*)tmp1_buf, (cfloat*)tw2, FFT16_SIZE, 0,
                                                            (cfloat*)tmp2_buf, inv);
                stage_radix2_dit<cfloat, cfloat, cfloat, 2>((cfloat*)tmp2_buf, (cfloat*)tw4, FFT16_SIZE, 0,
                                                            (cfloat*)tmp3_buf, inv);
                stage_radix2_dit<cfloat, cfloat, cfloat, 1>((cfloat*)tmp3_buf, (cfloat*)tw8, FFT16_SIZE, TP_SHIFT,
                                                            (cfloat*)obuff, inv); // r is not used.
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
          unsigned int TP_ORIG_PAR_POWER>
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
                                TP_ORIG_PAR_POWER>::kernelFFT(input_window<cint16>* __restrict inputx,
                                                              output_window<TT_OUT_DATA>* __restrict outputy) {
    const unsigned int TP_POINT_SIZE = FFT16_SIZE;
    typedef cint16 TT_DATA;
    typedef cint16 TT_TWIDDLE; // essentially a constant for thie specialization
    typedef cint32_t T_internalDataType;
    static constexpr cint16* __restrict tw1 = (cint16*)fft_lut_tw1;
    static constexpr cint16* __restrict tw2 = (cint16*)fft_lut_tw2;
    static constexpr cint16* __restrict tw4 = (cint16*)fft_lut_tw4;
    static constexpr cint16* __restrict tw8 = (cint16*)fft_lut_tw8_half;

#ifdef __FFT_SPOOF_R4__
    static constexpr TT_TWIDDLE* __restrict tw1_2 = NULL;
    static constexpr TT_TWIDDLE* __restrict tw2_4 = NULL;
    static constexpr TT_TWIDDLE* __restrict tw4_8 = NULL;
    static constexpr TT_TWIDDLE* __restrict tw8_16 = NULL;
#endif //__FFT_SPOOF_R4__
#ifdef __FFT_TRUE_R4__
    static constexpr TT_TWIDDLE* __restrict tw1_2 = (TT_TWIDDLE*)fft_lut_r4_1_2;
    static constexpr TT_TWIDDLE* __restrict tw2_4 = (TT_TWIDDLE*)fft_lut_r4_2_4;
    static constexpr TT_TWIDDLE* __restrict tw4_8 = (TT_TWIDDLE*)fft_lut_r4_4_8;
    static constexpr TT_TWIDDLE* __restrict tw8_16 = (TT_TWIDDLE*)fft_lut_r4_8_16;
#endif //__FFT_TRUE_R4__

    static cint16* tw_table[kMaxPointLog * 2] = {tw1,  tw2,  tw4,  tw8,  NULL,  NULL,  NULL,  NULL,
                                                 NULL, NULL, NULL, NULL, tw1_2, tw2_4, tw4_8, tw8_16,
                                                 NULL, NULL, NULL, NULL, NULL,  NULL,  NULL,  NULL};

    T_internalDataType* ktmp1_buf = (T_internalDataType*)
        fft_128_tmp1; // all pt sizes 128 or smaller use 128 sample buffer to keep codebase smaller.
    T_internalDataType* ktmp2_buf = (T_internalDataType*)fft_128_tmp2;

    cint16* xbuff = (cint16*)inputx->ptr;
    TT_OUT_DATA* obuff = (TT_OUT_DATA*)outputy->ptr;
    T_internalDataType* tmp1_buf = (T_internalDataType*)ktmp1_buf;
    T_internalDataType* tmp2_buf = (T_internalDataType*)ktmp2_buf;
    bool inv;
    if
        constexpr(TP_FFT_NIFFT == 1) { inv = false; }
    else {
        inv = true;
    }

    // This code should be moved to the constructor preferably
    set_rnd(rnd_pos_inf); // Match the twiddle round mode of Matlab.
    set_sat();            // do saturate.
    for (int iter = 0; iter < TP_WINDOW_VSIZE / TP_POINT_SIZE; iter++)
        chess_prepare_for_pipelining chess_loop_range(TP_WINDOW_VSIZE / TP_POINT_SIZE, ) {
            stages.calc(xbuff, tw_table, tmp1_buf, tmp2_buf, obuff);
            xbuff += TP_POINT_SIZE;
            obuff += TP_POINT_SIZE;
        }
};

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
          unsigned int TP_ORIG_PAR_POWER>
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
                                    TP_ORIG_PAR_POWER>::fftMain(input_window<TT_DATA>* __restrict inWindow,
                                                                output_window<TT_OUT_DATA>* __restrict outWindow) {
    m_fftKernel.kernelFFT(inWindow, outWindow);
};
}
}
}
}
}
