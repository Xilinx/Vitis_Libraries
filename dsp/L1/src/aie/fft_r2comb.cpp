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
FFT/IFFT DIT single channel R2 combiner stage kernel code.
This file captures the body of run-time code for the kernel class.

Coding conventions
  TT_      template type suffix
  TP_      template parameter suffix
*/

#include <adf.h>
#include <stdio.h>

using namespace std;

//#define _DSPLIB_FFT_R2COMB_HPP_DEBUG_

// if we use 1kb registers -> aie api uses 2x512b registers for 1024b so we need this for QoR
#define __AIE_API_USE_NATIVE_1024B_VECTOR__
#include "aie_api/aie_adf.hpp"

#include "fft_com_inc.h"
#include "fft_r2comb.hpp"
#include "kernel_api_utils.hpp"
#include "fft_ifft_dit_1ch_traits.hpp"
#include "fft_ifft_dit_1ch_utils.hpp"
#include "fft_r2comb_twiddle_lut.hpp"

namespace xf {
namespace dsp {
namespace aie {
namespace fft {
namespace r2comb {

using namespace xf::dsp::aie::fft::dit_1ch;

template <typename TT_TWIDDLE>
INLINE_DECL constexpr TT_TWIDDLE null_tw(){};
template <>
INLINE_DECL constexpr cint16 null_tw<cint16>() {
    return {0, 0};
};
template <>
INLINE_DECL constexpr cfloat null_tw<cfloat>() {
    return {0.0, 0.0};
};

// function to create offsets for multiple twiddle tables stored in one array for the Dynamic Point size case.
template <int TP_POINT_SIZE>
constexpr std::array<int, 12> fnGetR2TwStarts() {
    std::array<int, 12> twStarts = {0};
    int index = 0;
    int offset = 0;
    for (int i = TP_POINT_SIZE; i >= 8; i = (i >> 1)) {
        twStarts[index++] = offset;
        offset += i;
    }
    return twStarts;
}

// function to create R2 twiddle lookup table at compile-time
template <typename TT_TWIDDLE, int TP_INDEX, int TP_POINT_SIZE, int TP_PARALLEL_POWER, int TP_DYN_PT_SIZE>
constexpr std::array<TT_TWIDDLE, ((TP_POINT_SIZE / (2 - TP_DYN_PT_SIZE)) >> TP_PARALLEL_POWER)>
fnGetR2CombTwTable() // admittedly ugly way of saying twiddle table is 2x for dynamic case
{
    constexpr TT_TWIDDLE kzero = null_tw<TT_TWIDDLE>();
    constexpr int kTwiddleTableSize = (TP_POINT_SIZE / (2 - TP_DYN_PT_SIZE)) >> TP_PARALLEL_POWER;
    std::array<TT_TWIDDLE, kTwiddleTableSize> twiddles = {kzero};
    constexpr TT_TWIDDLE* twiddle_master = fnGetR2TwiddleMasterBase<TT_TWIDDLE>();

    if
        constexpr(TP_DYN_PT_SIZE == 1) {
            int tableIndex = 0;
            int tableFactor = 1; // 1 for max size table, moving up by power of 2 for each
            for (int tableSize = (TP_POINT_SIZE / 2) >> TP_PARALLEL_POWER; tableSize >= 8;
                 tableSize = (tableSize >> 1)) {
                int idx = (2 * kR2MasterTableSize / TP_POINT_SIZE) * TP_INDEX * tableFactor;
                int stride = ((2 * kR2MasterTableSize / TP_POINT_SIZE) << TP_PARALLEL_POWER) * tableFactor;
                for (int i = 0; i < tableSize; i++) {
                    twiddles[tableIndex++] = twiddle_master[idx];
                    idx += stride;
                }
                tableFactor <<= 1;
            }
        }
    else {
        constexpr int tableSize = (TP_POINT_SIZE / 2) >> TP_PARALLEL_POWER;
        int idx = (2 * kR2MasterTableSize / TP_POINT_SIZE) * TP_INDEX;
        int stride = (2 * kR2MasterTableSize / TP_POINT_SIZE) << TP_PARALLEL_POWER;
        for (int i = 0; i < tableSize; i++) {
            twiddles[i] = twiddle_master[idx];
            idx += stride;
        }
    }
    return twiddles;
};

//-----------------------------------------------------------------------------------------------------
template <typename TT_DATA,
          typename TT_TWIDDLE,
          unsigned int TP_POINT_SIZE,
          unsigned int TP_FFT_NIFFT,
          unsigned int TP_SHIFT,
          unsigned int TP_DYN_PT_SIZE,
          unsigned int TP_WINDOW_VSIZE,
          unsigned int TP_PARALLEL_POWER,
          unsigned int TP_INDEX,
          unsigned int TP_ORIG_PAR_POWER>
NOINLINE_DECL void fft_r2comb<TT_DATA,
                              TT_TWIDDLE,
                              TP_POINT_SIZE,
                              TP_FFT_NIFFT,
                              TP_SHIFT,
                              TP_DYN_PT_SIZE,
                              TP_WINDOW_VSIZE,
                              TP_PARALLEL_POWER,
                              TP_INDEX,
                              TP_ORIG_PAR_POWER>::fft_r2comb_main(input_window<TT_DATA>* __restrict inWindow,
                                                                  output_window<TT_DATA>* __restrict outWindow) {
    static constexpr int kTableFactor = (TP_DYN_PT_SIZE == 1) ? 1 : 2; // for dynamic point size, the max point size
                                                                       // table is not alone. We need 1/2, 1/4, etc,
                                                                       // hence twice the storage
    alignas(32) static constexpr std::array<TT_TWIDDLE, ((TP_POINT_SIZE / kTableFactor) >> TP_PARALLEL_POWER)>
        twiddles = fnGetR2CombTwTable<TT_TWIDDLE, TP_INDEX, TP_POINT_SIZE, TP_PARALLEL_POWER, TP_DYN_PT_SIZE>();
    static constexpr std::array<int, 12> twiddleStarts = fnGetR2TwStarts<(TP_POINT_SIZE >> (TP_PARALLEL_POWER + 1))>();

    set_rnd(rnd_pos_inf); // Match the twiddle round mode of Matlab.
    set_sat();            // do saturate.

    bool inv;

    if
        constexpr(TP_DYN_PT_SIZE == 1) {
            static constexpr unsigned int kPtSizePwr = fnPointSizePower<TP_POINT_SIZE>();
            static constexpr unsigned int kminPtSizePwr = 4;
            T_buff_256b<TT_DATA> header;
            TT_DATA headerVal;
            T_buff_256b<TT_DATA> blankOp;
            int ptSizePwr;
            int ptSize;

            blankOp.val = ::aie::zeros<TT_DATA, 32 / sizeof(TT_DATA)>();
            header = window_readincr_256b(inWindow);
            headerVal = header.val.get(0);
            inv = headerVal.real == 0 ? true : false;
            headerVal = header.val.get(1);
            ptSizePwr = (int)headerVal.real - (TP_ORIG_PAR_POWER - TP_PARALLEL_POWER);
            ptSize = (1 << ptSizePwr);

            headerVal = header.val.get(std::is_same<TT_DATA, cint16>::value ? 7 : 3);

            if (headerVal.real == 0 && ptSizePwr >= kminPtSizePwr) {
                window_writeincr(outWindow, header.val);
                // perform the R2 stage here.
                TT_DATA* xbuff = (TT_DATA*)inWindow->ptr;
                TT_DATA* ybuff = (TT_DATA*)outWindow->ptr;
                int n = (ptSize >> TP_PARALLEL_POWER);
                unsigned shift = TP_SHIFT + 15;
                int tw_base = kPtSizePwr - ptSizePwr;

                for (int i = 0; i < TP_WINDOW_VSIZE; i += (TP_POINT_SIZE >> TP_PARALLEL_POWER)) {
                    r2comb_dit<TT_DATA, TT_TWIDDLE>(xbuff + i, (TT_TWIDDLE*)(&twiddles[twiddleStarts[tw_base]]), n,
                                                    0 /* r  */, TP_SHIFT + 15, ybuff + i, inv);

                    // blank the remainder of the frame holder
                    using write_type = ::aie::vector<TT_DATA, 128 / 8 / sizeof(TT_DATA)>;
                    write_type* blankDataPtr =
                        (write_type*)(ybuff + n); // addition is in TT_DATA currency, then cast to 128b
                    for (int i = n; i < (TP_POINT_SIZE >> TP_PARALLEL_POWER); i += (16 / sizeof(TT_DATA))) {
                        *blankDataPtr++ = ::aie::zeros<TT_DATA, 16 / sizeof(TT_DATA)>();
                    }
                }

            } else { // illegal framesize or invalid incoming
                header.val.set(unitVector<TT_DATA>(), std::is_same<TT_DATA, cint16>::value
                                                          ? 7
                                                          : 3); // set the invalid flag in the status location.
                window_writeincr(outWindow, header.val);
                // write out blank window

                TT_DATA* ybuff = (TT_DATA*)outWindow->ptr;
                using write_type = ::aie::vector<TT_DATA, 128 / 8 / sizeof(TT_DATA)>;
                write_type* blankDataPtr = (write_type*)(ybuff); // addition is in TT_DATA currency, then cast to 128b
                for (int i = 0; i < TP_WINDOW_VSIZE / (16 / sizeof(TT_DATA)); i++) {
                    *blankDataPtr++ = ::aie::zeros<TT_DATA, 16 / sizeof(TT_DATA)>();
                }
            }
        }
    else { // Static point size case

        if
            constexpr(TP_FFT_NIFFT == 1) { inv = false; }
        else {
            inv = true;
        }
        // perform the R2 stage here.
        TT_DATA* xbuff = (TT_DATA*)inWindow->ptr;
        TT_DATA* ybuff = (TT_DATA*)outWindow->ptr;
        constexpr int n = (TP_POINT_SIZE >> TP_PARALLEL_POWER);
        unsigned shift = TP_SHIFT + 15;

        for (int i = 0; i < TP_WINDOW_VSIZE; i += n) {
            r2comb_dit<TT_DATA, TT_TWIDDLE>(xbuff + i, (TT_TWIDDLE*)(&twiddles[0]), n, 0 /* r  */, TP_SHIFT + 15,
                                            ybuff + i, inv);
        }
    }
};
}
}
}
}
}
