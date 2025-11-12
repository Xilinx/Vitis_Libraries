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
cumsum kernal code.
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
//#include "cumsum_traits.hpp"
#include "cumsum.hpp"
#include "cumsum_utils.hpp"
#include "fir_utils.hpp"
#include "kernel_api_utils.hpp"

//#define _DSPLIB_CUMSUM_HPP_DEBUG_

namespace xf {
namespace dsp {
namespace aie {
namespace cumsum {

// This is the basic (general) implementation of cumsum mode 0 on AIE1.
template <typename TT_DATA,
          typename TT_OUT_DATA,
          unsigned int TP_DIM_A,
          unsigned int TP_DIM_B,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_MODE,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_SAT>
INLINE_DECL void
cumsumBase<TT_DATA, TT_OUT_DATA, TP_DIM_A, TP_DIM_B, TP_NUM_FRAMES, TP_MODE, TP_SHIFT, TP_RND, TP_SAT>::
    cumsumBase_mode0(input_buffer<TT_DATA>& __restrict inWindow, output_buffer<TT_OUT_DATA>& __restrict outWindow) {
    static constexpr int kSamplesInLoadVect = __MAX_READ_WRITE__ / 8 / sizeof(TT_OUT_DATA);
    static constexpr int kSamplesInMacVect = __MAX_READ_WRITE__ / 8 / sizeof(TT_OUT_DATA);
    static constexpr int kFullVectsInRow = CEIL(TP_DIM_A, kSamplesInMacVect) / kSamplesInMacVect;
    using accBaseType = typename tcumsumAccBaseType<TT_OUT_DATA>::type;
    using bigDataVect_t = ::aie::vector<TT_DATA, 2 * kSamplesInMacVect>;
    using dataOutVect_t = ::aie::vector<TT_OUT_DATA, kSamplesInMacVect>;
    using coeffType = typename tcumsumCoeffBaseType<TT_OUT_DATA>::type;

    coeffType unity = cumsumUnity<coeffType>();

    auto itr = ::aie::begin_vector<kSamplesInLoadVect>(inWindow);
    auto itw = ::aie::begin_vector<kSamplesInLoadVect>(outWindow);
    bigDataVect_t buff_i = ::aie::zeros<TT_DATA, 2 * kSamplesInMacVect>();
    ;
    bigDataVect_t revBuff = ::aie::zeros<TT_DATA, 2 * kSamplesInMacVect>();
    ;
    bigDataVect_t zeroBuff = ::aie::zeros<TT_DATA, 2 * kSamplesInMacVect>();
    ;
    ::aie::accum<accBaseType, kSamplesInMacVect> acc0;
    ::aie::vector<coeffType, kSamplesInMacVect> coeff = ::aie::broadcast<coeffType, kSamplesInMacVect>(unity);
    ::aie::vector<TT_OUT_DATA, kSamplesInMacVect> result; // used for output
    dataOutVect_t outVect;

    // collapse the TP_NUM_FRAMES and TP_DIM_B loops to one loop because with cumsum along the first dimension there is
    // no
    // difference between rows and frames, so collapsing the loops into one will eliminate an unnecessary loop in
    // microcode.
    for (int frame = 0; frame < TP_NUM_FRAMES * TP_DIM_B; frame++)
        chess_prepare_for_pipelining chess_loop_range(TP_NUM_FRAMES * TP_DIM_B, ) {
            acc0 = ::aie::zeros<accBaseType, kSamplesInMacVect>();
            for (unsigned rr = 0; rr < kFullVectsInRow; rr++)
                chess_prepare_for_pipelining chess_loop_range(kFullVectsInRow, ) {
                    // buff_i = *itr++;
                    buff_i.insert(1, *itr); // dont increment itr yet since it is read for revBuff too.

                    // second variant of sliding mac in the AIE API reference.
                    acc0 = sliding_mac<kSamplesInMacVect, kSamplesInMacVect, 1 /*CoeffStep*/, 1 /*DataStepX*/,
                                       1 /*DataStepY*/>(acc0, coeff, 0 /*CoeffStart */, buff_i, 1 /*Datastart */);

                    result = acc0.template to_vector<TT_OUT_DATA>(TP_SHIFT);
                    *itw++ = result;
                    revBuff.insert(0, *itr++);
                    acc0 = sliding_mac<kSamplesInMacVect, kSamplesInMacVect, 1 /*CoeffStep*/, 1 /*DataStepX*/,
                                       1 /*DataStepY*/>(acc0, coeff, 0 /*CoeffStart */, revBuff, 1 /*Datastart */);
                } // rr
        }         // frame loop
};

// This implementation uses sliding mul of double width where the first half
// produced the desired output and the second half the 'top-up' required
// to set the accumulator values for the next set of data inputs.
template <typename TT_DATA,
          typename TT_OUT_DATA,
          unsigned int TP_DIM_A,
          unsigned int TP_DIM_B,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_MODE,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_SAT>
INLINE_DECL void
cumsumBase<TT_DATA, TT_OUT_DATA, TP_DIM_A, TP_DIM_B, TP_NUM_FRAMES, TP_MODE, TP_SHIFT, TP_RND, TP_SAT>::
    cumsumBase_mode0_accacc(input_buffer<TT_DATA>& __restrict inWindow,
                            output_buffer<TT_OUT_DATA>& __restrict outWindow) {
    static constexpr int kSamplesInLoadVect = __MAX_READ_WRITE__ / 8 / sizeof(TT_OUT_DATA);
    static constexpr int kSamplesInMacVect = kSamplesInLoadVect;
    static constexpr int kFullVectsInRow = CEIL(TP_DIM_A, kSamplesInMacVect) / kSamplesInMacVect;
    using accBaseType = typename tcumsumAccBaseType<TT_OUT_DATA>::type;
    using coeffBaseType = typename tcumsumCoeffBaseType<TT_OUT_DATA>::type;
    using bigaccType = ::aie::accum<accBaseType, 2 * kSamplesInMacVect>;
    using smallaccType = ::aie::accum<accBaseType, kSamplesInMacVect>;
    using bigDataVect_t = ::aie::vector<TT_DATA, 2 * kSamplesInMacVect>;
    using dataOutVect_t = ::aie::vector<TT_OUT_DATA, 2 * kSamplesInMacVect>;
    using coeffType = ::aie::vector<coeffBaseType, kSamplesInMacVect>;

    coeffBaseType unity = cumsumUnity<coeffBaseType>();

    auto itr = ::aie::begin_vector<kSamplesInLoadVect>(inWindow);
    auto itw = ::aie::begin_vector<kSamplesInLoadVect>(outWindow);
    bigDataVect_t buff_i = ::aie::zeros<TT_DATA, 2 * kSamplesInMacVect>();
    bigDataVect_t revBuff = ::aie::zeros<TT_DATA, 2 * kSamplesInMacVect>();
    bigaccType acc0;
    smallaccType acc1;
    bigaccType acc2 = ::aie::zeros<accBaseType, 2 * kSamplesInMacVect>();
    coeffType coeff = ::aie::broadcast<coeffBaseType, kSamplesInMacVect>(unity);
    dataOutVect_t result;
    ::aie::vector<TT_OUT_DATA, kSamplesInMacVect> acc1int;

    // This implementation uses sliding mul so as to utilize Vector mac columns when available.
    // It uses a trick for resetting the accumulation to the final total of the last lane at the end.
    // E.g. the data vector loaded is 1, 2, 3, 4. This is loaded in to a double size vector giving 12340000.
    // This is then accumulated with a slide of 1 over 4 points. The acc then holds (hex) 136A9740.
    // The first half 136A is output.
    // The second half (acc1) is extracted and inserted to a double length reg acc2 giving 97400000
    // This is then added to acc0 giving AAAA9740. Finally the second half of acc0 is zeroed giving
    // AAAA0000 which is the desired state for the next iteration of the inner loop.

    // collapse the TP_NUM_FRAMES and TP_DIM_B loops to one loop because with cumsum along the first dimension there is
    // no
    // difference between rows and frames, so collapsing the loops into one will eliminate an unnecessary loop in
    // microcode.
    for (int frame = 0; frame < TP_NUM_FRAMES * TP_DIM_B; frame++)
        chess_prepare_for_pipelining chess_loop_range(TP_NUM_FRAMES * TP_DIM_B, ) {
            acc0 = ::aie::zeros<accBaseType, 2 * kSamplesInMacVect>();
            for (unsigned rr = 0; rr < kFullVectsInRow; rr++)
                chess_prepare_for_pipelining chess_loop_range(kFullVectsInRow, ) {
                    buff_i.insert(1, *itr++); // increment since there is no revBuff.

                    // second variant of sliding mac in the AIE API reference.
                    acc0 = sliding_mac<2 * kSamplesInMacVect, kSamplesInMacVect, 1 /*CoeffStep*/, 1 /*DataStepX*/,
                                       1 /*DataStepY*/>(acc0, coeff, 0 /*CoeffStart */, buff_i, 1 /*Datastart */);

                    result = acc0.template to_vector<TT_OUT_DATA>(TP_SHIFT);
                    *itw++ = result.template extract<kSamplesInLoadVect>(0);
                    //*itw++ = result.template extract<kSamplesInLoadVect>(1);
                    //*itw++ = result;

                    acc1 = acc0.template extract<kSamplesInLoadVect>(1);
                    acc2.insert(0, acc1);
                    acc0 = ::aie::add(acc0, acc2);
                    acc0.insert(
                        1, ::aie::zeros<accBaseType, kSamplesInMacVect>()); // clear the second half of the reg so that

                } // rr
        }         // frame loop
};

template <typename TT_DATA,
          typename TT_OUT_DATA,
          unsigned int TP_DIM_A,
          unsigned int TP_DIM_B,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_MODE,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_SAT>
INLINE_DECL void
cumsumBase<TT_DATA, TT_OUT_DATA, TP_DIM_A, TP_DIM_B, TP_NUM_FRAMES, TP_MODE, TP_SHIFT, TP_RND, TP_SAT>::
    cumsumBase_mode1(input_buffer<TT_DATA>& __restrict inWindow, output_buffer<TT_OUT_DATA>& __restrict outWindow) {
    static constexpr int kSamplesInVect = __MAX_READ_WRITE__ / 8 / sizeof(TT_OUT_DATA);

    using dataVect_t = ::aie::vector<TT_DATA, kSamplesInVect>;
    using dataOutVect_t = ::aie::vector<TT_OUT_DATA, kSamplesInVect>;
    using accBaseType = typename tcumsumAccBaseType<TT_OUT_DATA>::type;
    using accVect_t = ::aie::accum<accBaseType, kSamplesInVect>;
    static constexpr int kFullVectsInRow =
        CEIL(TP_DIM_A, kSamplesInVect) / kSamplesInVect; // includes the vector with runt samples
    static constexpr int kVectsInRow =
        (TP_DIM_A /
         kSamplesInVect); // number of complete vectors in a row. Does not include the vector with runt samples.
    static constexpr uint32 kMask =
        (1 << (TP_DIM_A % kSamplesInVect)) - 1; // 0's for all elements which must be blanked/
    static constexpr::aie::mask<kSamplesInVect> kMyMask = ::aie::mask<kSamplesInVect>::from_uint32(kMask);
    accVect_t zeroAcc = ::aie::zeros<accBaseType, kSamplesInVect>();
    dataVect_t dataVect;
    accVect_t acc;
    dataOutVect_t outVect;
    dataVect_t* inPtr = (dataVect_t*)inWindow.data();
    TT_DATA* inDebugPtr = (TT_DATA*)inWindow.data();
    dataOutVect_t* outPtr = (dataOutVect_t*)outWindow.data();
    int idx;

    for (int frame = 0; frame < TP_NUM_FRAMES; frame++) chess_prepare_for_pipelining chess_loop_range(TP_NUM_FRAMES, ) {
            for (int i = 0; i < kVectsInRow; i++) {
                acc = zeroAcc;
                for (int k = 0; k < TP_DIM_B; k++) chess_prepare_for_pipelining chess_loop_range(TP_DIM_B, ) {
                        idx = i + k * kFullVectsInRow + frame * kFullVectsInRow * TP_DIM_B;
                        dataVect = inPtr[idx];
                        acc = ::aie::add(acc, dataVect);
                        outPtr[idx] = acc.template to_vector<TT_OUT_DATA>(TP_SHIFT);
                    }
            }
            for (int i = kVectsInRow; i < kFullVectsInRow; i++) {
                acc = zeroAcc;
                for (int k = 0; k < TP_DIM_B; k++) chess_prepare_for_pipelining chess_loop_range(TP_DIM_B, ) {
                        dataVect = inPtr[i + k * kFullVectsInRow + frame * kFullVectsInRow * TP_DIM_B];
                        dataVect = select(nullElem<TT_DATA>(), dataVect, kMyMask);
                        acc = ::aie::add(acc, dataVect);
                        outPtr[i + k * kFullVectsInRow + frame * kFullVectsInRow * TP_DIM_B] =
                            acc.template to_vector<TT_OUT_DATA>(TP_SHIFT);
                    }
            }
        }
};

// cumsum for AIE variants which support acc+acc
// Note this specialization is fast, but is limited to range TT_OUT_DATA, not the corresponding accum type
template <typename TT_DATA,
          typename TT_OUT_DATA,
          unsigned int TP_DIM_A,
          unsigned int TP_DIM_B,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_MODE,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_SAT>
INLINE_DECL void
cumsumBase<TT_DATA, TT_OUT_DATA, TP_DIM_A, TP_DIM_B, TP_NUM_FRAMES, TP_MODE, TP_SHIFT, TP_RND, TP_SAT>::
    cumsumBase_mode2(input_buffer<TT_DATA>& __restrict inWindow, output_buffer<TT_OUT_DATA>& __restrict outWindow) {
    static constexpr int kSamplesInLoadVect = __MAX_READ_WRITE__ / 8 / sizeof(TT_OUT_DATA);
    static constexpr int kSamplesInMacVect = 2 * kSamplesInLoadVect;
    static constexpr int kFullVectsInRow = CEIL(TP_DIM_A, kSamplesInMacVect) / kSamplesInMacVect;

    using accBaseType = typename tcumsumAccBaseType<TT_OUT_DATA>::type;
    using dataOutVect_t = ::aie::vector<TT_OUT_DATA, kSamplesInMacVect>;

    auto itr = ::aie::begin_vector<kSamplesInLoadVect>(inWindow);
    auto itw = ::aie::begin_vector<kSamplesInLoadVect>(outWindow);
    ::aie::vector<TT_DATA, kSamplesInMacVect> buff_i;
    ::aie::vector<TT_DATA, kSamplesInMacVect> shiftedBuff;
    //::aie::vector<TT_DATA,kSamplesInMacVect> prev = ::aie::zeros<TT_DATA,kSamplesInMacVect>();
    ::aie::accum<accBaseType, kSamplesInMacVect> prev;
    ::aie::vector<int16, kSamplesInMacVect> unity =
        ::aie::broadcast<int16, kSamplesInMacVect>(1); // TODO - need templatize type lookup
    ::aie::vector<TT_DATA, kSamplesInMacVect> zero = ::aie::zeros<TT_DATA, kSamplesInMacVect>();
    ::aie::accum<accBaseType, kSamplesInMacVect> acc0;
    ::aie::accum<accBaseType, kSamplesInMacVect> acc1;
    ::aie::accum<accBaseType, kSamplesInMacVect> acc2;
    dataOutVect_t result;

    for (int frame = 0; frame < TP_NUM_FRAMES * TP_DIM_B;
         frame++) // for this direction of accumulation there's no difference between rows and frames
        chess_prepare_for_pipelining chess_loop_range(TP_NUM_FRAMES, ) {
            prev = ::aie::zeros<accBaseType, kSamplesInMacVect>();
            for (unsigned rr = 0; rr < kFullVectsInRow; rr++)
                chess_prepare_for_pipelining chess_loop_range(kFullVectsInRow, ) {
                    buff_i.insert(0, *itr++);
                    buff_i.insert(1, *itr++);
                    acc0 = ::aie::add(prev, buff_i); //              acc0.from_vector(::aie::add(buff_i,prev));
                    shiftedBuff = ::aie::shuffle_up_fill(buff_i, zero, 1);
                    if
                        constexpr(isFloat<TT_DATA>()) { acc1 = ::aie::from_vector<accBaseType>(shiftedBuff); }
                    else {
                        acc1 = ::aie::mul<accBaseType>(shiftedBuff, unity);
                    }
                    for (unsigned ss = 2; ss < kSamplesInMacVect; ss += 2) chess_flatten_loop {
                            if
                                constexpr(isFloat<TT_DATA>()) {
                                    acc0 = ::aie::add(acc0, ::aie::shuffle_up_fill(buff_i, zero, ss));
                                    acc1 = ::aie::add(acc1, ::aie::shuffle_up_fill(buff_i, zero, ss + 1));
                                }
                            else {
                                acc0 = ::aie::mac(acc0, ::aie::shuffle_up_fill(buff_i, zero, ss), unity);
                                acc1 = ::aie::mac(acc1, ::aie::shuffle_up_fill(buff_i, zero, ss + 1), unity);
                            }
                        }
                    acc2 = ::aie::add(acc0, acc1);
                    result = acc2.template to_vector<TT_OUT_DATA>();
                    prev = ::aie::from_vector<accBaseType>(::aie::broadcast<TT_OUT_DATA, kSamplesInMacVect>(
                                                               (TT_OUT_DATA)result.get(kSamplesInMacVect - 1)),
                                                           0);

                    *itw++ = result.template extract<kSamplesInLoadVect>(0);
                    *itw++ = result.template extract<kSamplesInLoadVect>(1);
                } // rr

        } // end of frame loop
};

// cumsumBase main - effectively a switch statement to all variants.
template <typename TT_DATA,
          typename TT_OUT_DATA,
          unsigned int TP_DIM_A,
          unsigned int TP_DIM_B,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_MODE,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_SAT>
INLINE_DECL void
cumsumBase<TT_DATA, TT_OUT_DATA, TP_DIM_A, TP_DIM_B, TP_NUM_FRAMES, TP_MODE, TP_SHIFT, TP_RND, TP_SAT>::cumsumBase_main(
    input_buffer<TT_DATA>& __restrict inWindow, output_buffer<TT_OUT_DATA>& __restrict outWindow) {
    if
        constexpr(TP_MODE == 0) {
#if __SUPPORTS_ACC_ACC_ADD__ == 1 // acc + acc is AIE-ML feature
            cumsumBase_mode0_accacc(inWindow, outWindow);
#else
            cumsumBase_mode0(inWindow, outWindow);
#endif //__SUPPORTS_ACC_ACC_ADD__
        }
    else if
        constexpr(TP_MODE == 1) { cumsumBase_mode1(inWindow, outWindow); }
    else {
        cumsumBase_mode2(inWindow, outWindow);
    }
};

//---------------------------------------------------
// cumsum entry class
template <typename TT_DATA,
          typename TT_OUT_DATA,
          unsigned int TP_DIM_A,
          unsigned int TP_DIM_B,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_MODE,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_SAT>
NOINLINE_DECL void
cumsum<TT_DATA, TT_OUT_DATA, TP_DIM_A, TP_DIM_B, TP_NUM_FRAMES, TP_MODE, TP_SHIFT, TP_RND, TP_SAT>::cumsum_main(
    input_buffer<TT_DATA>& __restrict inWindow, output_buffer<TT_OUT_DATA>& __restrict outWindow) {
    set_rnd_mode<TP_RND>();
    set_sat_mode<TP_SAT>();

    this->cumsumBase_main(inWindow, outWindow);
}; // cumsum_main
}
}
}
}
/* ----------Code experiments


    v16acc48 oacc  = null_v16acc48;
    v16int16 xbuff = null_v16int16;
    v16int16 result = null_v16int16;
    v16int8  zbuff = null_v16int8;
    for (unsigned rr=0; rr < kFullVectsInRow; rr++)
          chess_prepare_for_pipelining chess_loop_range(kFullVectsInRow, )
            {
              xbuff = *itr++;
              //buff_i.insert(0,*itr++);
              //buff_i.insert(1,*itr++);
              acc0 = ::aie::add(prev,buff_i); //acc0.from_vector(::aie::add(buff_i,prev));
              for (unsigned ss=1; ss < kSamplesInMacVect; ss++)  chess_prepare_for_pipelining
chess_loop_range(kSamplesInMacVect, ) {
                  oacc = mac16(oacc, xbuff, 0, 0x0000, 0x0000, 0, 0x1010, zbuff, 0x6420, 0xECA8, 2, 0x2110);
                  //acc0 = add(acc0,::aie::shuffle_up_fill(buff_i,zero,ss));
              }
              //              result = acc0.template to_vector<TT_OUT_DATA>();
              result = srs(oacc, 0, TP_SAT, TP_RND);
              *itw++ = result;
                             *itw++ = result.template extract<kSamplesInLoadVect>(0);
                             *itw++ = result.template extract<kSamplesInLoadVect>(1);
              prev =
::aie::from_vector<accBaseType>(::aie::broadcast<TT_DATA,kSamplesInMacVect>(result.get(kSamplesInMacVect-1)),0);
            } // rr
      }

//-----------------------------------------

    // This method achieves very poor QoR - presumably because it translates to multi-column additions, but has to mask
the columns.
        for (unsigned rr=0; rr < kFullVectsInRow; rr++)
          chess_prepare_for_pipelining chess_loop_range(kFullVectsInRow, )
            {
              //buff_i = *itr++;
              buff_i.insert(0,*itr++);
              buff_i.insert(1,*itr++);
              acc0 = ::aie::add(prev,buff_i); //acc0.from_vector(::aie::add(buff_i,prev));
              for (unsigned ss=1; ss < kSamplesInMacVect; ss++) chess_prepare_for_pipelining
chess_loop_range(kSamplesInMacVect, ) {
                acc0 = ::aie::add(acc0,::aie::shuffle_up_fill(buff_i,zero,ss  ));
              }

              result = acc0.template to_vector<TT_OUT_DATA>();
              prev =
::aie::from_vector<accBaseType>(::aie::broadcast<TT_OUT_DATA,kSamplesInMacVect>(result.get(kSamplesInMacVect-1)),0);
                                                *itw++ = result;
              if constexpr(TP_SHIFT>0) {
                  result = acc0.template to_vector<TT_OUT_DATA>(TP_SHIFT);
                }
              *itw++ = result.template extract<kSamplesInLoadVect>(0);
              *itw++ = result.template extract<kSamplesInLoadVect>(1);
            } // rr

//-------------------------------------------------------------
//scalar variant - achieves ~194Msa/s for int16
       int32 acc = 0;
       TT_OUT_DATA outVal;
       TT_DATA* inPtr = inWindow.data();
       TT_OUT_DATA* outPtr = outWindow.data();
       for (unsigned rr=0; rr < kFullVectsInRow*kSamplesInMacVect; rr++)
         chess_prepare_for_pipelining chess_loop_range(kFullVectsInRow*kSamplesInMacVect, )
         {
           acc += (int32)*inPtr++;
           if constexpr (TP_SHIFT>0) {
               outVal = acc >> TP_SHIFT;
               *outPtr++ = outVal;
           } else {
             *outPtr++ = (TT_OUT_DATA)acc;
           }
         }

//Base variant
template <typename TT_DATA,
          typename TT_OUT_DATA,
          unsigned int TP_DIM_A,
          unsigned int TP_DIM_B,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_MODE,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_SAT>
INLINE_DECL void cumsumBase<TT_DATA,
                            TT_OUT_DATA,
                            TP_DIM_A,
                            TP_DIM_B,
                            TP_NUM_FRAMES,
                            TP_MODE,
                            TP_SHIFT,
                            TP_RND,
                            TP_SAT>::cumsumBase_mode0_orig_base(input_buffer<TT_DATA>& __restrict inWindow,
                                                                output_buffer<TT_OUT_DATA>& __restrict outWindow) {

    static constexpr int kSamplesInLoadVect = 256/8/sizeof(TT_OUT_DATA);//256 for ML
    static constexpr int kSamplesInMacVect = 512/8/sizeof(TT_OUT_DATA);//512 for ML
    static constexpr int kFullVectsInRow = CEIL(TP_DIM_A,kSamplesInMacVect)/kSamplesInMacVect;
    using accBaseType = typename tcumsumAccBaseType<TT_OUT_DATA>::type;
    using dataOutVect_t = ::aie::vector<TT_OUT_DATA, kSamplesInMacVect>;

    auto itr = ::aie::begin_vector<kSamplesInLoadVect>(inWindow);
    auto itw = ::aie::begin_vector<kSamplesInLoadVect>(outWindow);
    ::aie::vector<TT_DATA,kSamplesInMacVect> buff_i;
    //::aie::vector<TT_DATA,kSamplesInMacVect> prev = ::aie::zeros<TT_DATA,kSamplesInMacVect>();
    ::aie::accum<accBaseType,kSamplesInMacVect> prev;
    ::aie::vector<int16,kSamplesInMacVect> unity = ::aie::broadcast<int16,kSamplesInMacVect>(1); //TODO - need
templatize type lookup
    ::aie::vector<TT_DATA,kSamplesInMacVect> zero = ::aie::zeros<TT_DATA,kSamplesInMacVect>();
    ::aie::accum<accBaseType,kSamplesInMacVect>  acc0;
    ::aie::accum<accBaseType,kSamplesInMacVect>  acc1;
    ::aie::accum<accBaseType,kSamplesInMacVect>  acc2;
    dataOutVect_t result;

    for (int frame = 0; frame < TP_NUM_FRAMES*TP_DIM_B; frame++) //for this direction of accumulation there's no
difference between rows and frames
      chess_prepare_for_pipelining chess_loop_range(TP_NUM_FRAMES, )
      {
        prev = ::aie::zeros<accBaseType,kSamplesInMacVect>();
        for (unsigned rr=0; rr < kFullVectsInRow; rr++)
          chess_prepare_for_pipelining chess_loop_range(kFullVectsInRow, )
          {
            //buff_i = *itr++;
            buff_i.insert(0,*itr++);
            buff_i.insert(1,*itr++);
            acc0 = ::aie::add(prev,buff_i); //acc0.from_vector(::aie::add(buff_i,prev));
            for (unsigned ss=1; ss < kSamplesInMacVect; ss++) chess_prepare_for_pipelining
chess_loop_range(kSamplesInMacVect, ) {
                acc0 = ::aie::add(acc0,::aie::shuffle_up_fill(buff_i,zero,ss  ));
              }

            result = acc0.template to_vector<TT_OUT_DATA>();
            prev =
::aie::from_vector<accBaseType>(::aie::broadcast<TT_OUT_DATA,kSamplesInMacVect>(result.get(kSamplesInMacVect-1)),0);
            // *itw++ = result;
            if constexpr(TP_SHIFT>0) {
                result = acc0.template to_vector<TT_OUT_DATA>(TP_SHIFT);
              }
            *itw++ = result.template extract<kSamplesInLoadVect>(0);
            *itw++ = result.template extract<kSamplesInLoadVect>(1);
          } // rr
      }// frame loop
};

//---------------------------------------------------------
intrinsic variant
template <typename TT_DATA,
          typename TT_OUT_DATA,
          unsigned int TP_DIM_A,
          unsigned int TP_DIM_B,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_MODE,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_SAT>
INLINE_DECL void cumsumBase<TT_DATA,
                            TT_OUT_DATA,
                            TP_DIM_A,
                            TP_DIM_B,
                            TP_NUM_FRAMES,
                            TP_MODE,
                            TP_SHIFT,
                            TP_RND,
                            TP_SAT>::cumsumBase_mode0_int32(input_buffer<TT_DATA>& __restrict inWindow,
                                                            output_buffer<TT_OUT_DATA>& __restrict outWindow) {

  v8int32* inRd = (v8int32*)inWindow.data();
  v8int32 inval;
  v8int32* outWr = (v8int32*)outWindow.data();
  v8acc80 acc;
  ::aie::accum<acc80,8>  accapi;
  v16int32 xbuff = null_v16int32();
  v8int32 out;
  ::aie::vector<int32,8> outapi;
  int xstart = 8;
  unsigned int xoffsets = 0;//0x3210;
  unsigned xstep = 1;
  int16 zbuff_init[16] = { 0,0,0,0, 0,0,0,0, 1,1,1,1, 1,1,1,1 };
  v16int16 zbuff = *(v16int16*)zbuff_init;
  int zstart = 8;
  int zoffsets = 0x76543210;
  int zstep = -1;
  int32 prev;
  int32 outsamp;

    for (int frame = 0; frame < TP_NUM_FRAMES*TP_DIM_B; frame++) //for this direction of accumulation there's no
difference between rows and frames
      chess_prepare_for_pipelining chess_loop_range(TP_NUM_FRAMES, )
      {
        acc = null_v8acc80();

        for (int k = 0; k < CEIL(TP_DIM_A,8)/8; k++) {
          //        acc = null_v8acc80;
          inval = *inRd++;
          for (int m = 0 ; m<8; m++) {
            outsamp = ext_elem(inval,m);
            //printf("inval[%d] = %d ",m,outsamp);
          }
          //printf("\n");
          xbuff = upd_w(xbuff, 1, inval);
#pragma unroll (8)
          for (int i = 0; i<4; i++) {
            acc = lmac8(acc, xbuff, 8+2*i, xoffsets, xstep, zbuff, 8-2*i, zoffsets, zstep);

          }

          out = srs(acc, 0);
          *outWr++ = out;
          prev = ext_elem(out,7);
          printf("prev= %d\n",prev);
          outapi = ::aie::broadcast<int32,8>(prev);
          out = (v8int32)outapi;
          accapi = ::aie::from_vector<acc80>(outapi,0);
          acc = (v8acc80)accapi;
          out = srs(acc, 0);
        }
      }// frame loop
};




 */
