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
fft_window kernal code.
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
//#include "fft_window_traits.hpp"
#include "fft_window.hpp"
#include "fft_window_utils.hpp"
#include "kernel_api_utils.hpp"

//#define _DSPLIB_FFT_WINDOW_HPP_DEBUG_

namespace xf {
namespace dsp {
namespace aie {
namespace fft {
namespace windowfn {

template <typename T_D>
T_D INLINE_DECL unitVector(){};
template <>
cint16 INLINE_DECL unitVector<cint16>() {
    cint16 temp;
    temp.real = 1;
    temp.imag = 0;
    return temp;
};
template <>
cint32 INLINE_DECL unitVector<cint32>() {
    cint32 temp;
    temp.real = 1;
    temp.imag = 0;
    return temp;
};
template <>
cfloat INLINE_DECL unitVector<cfloat>() {
    cfloat temp = {0.0, 0.0};
    temp.real = 1.0;
    temp.imag = 0.0;
    return temp;
};

// Constructor for windowed (simple) config
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_POINT_SIZE,
          unsigned int TP_WINDOW_VSIZE,
          unsigned int TP_SHIFT,
          unsigned int TP_API,
          unsigned int TP_SSR,
          unsigned int TP_DYN_PT_SIZE,
          unsigned int TP_RND,
          unsigned int TP_SAT>
NOINLINE_DECL fft_window<TT_DATA,
                         TT_COEFF,
                         TP_POINT_SIZE,
                         TP_WINDOW_VSIZE,
                         TP_SHIFT,
                         TP_API,
                         TP_SSR,
                         TP_DYN_PT_SIZE,
                         TP_RND,
                         TP_SAT>::fft_window(const TT_COEFF (&kernel_weights)[TP_POINT_SIZE * (1 + TP_DYN_PT_SIZE)]) {
    // copy the primary table raw/
    memcpy(weights, &kernel_weights[0], (TP_POINT_SIZE * (1 + TP_DYN_PT_SIZE) * sizeof(TT_COEFF)));
    tableStarts[0] = 0;
    if
        constexpr(TP_DYN_PT_SIZE == 1) {
            int index = 1;
            int start = 0;
            for (int pt = TP_POINT_SIZE; pt >= 16; pt = pt >> 1) {
                start += pt;
                tableStarts[index++] = start;
            }
        }
};

// Constructor for streaming config
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_POINT_SIZE,
          unsigned int TP_WINDOW_VSIZE,
          unsigned int TP_SHIFT,
          unsigned int TP_SSR,
          unsigned int TP_DYN_PT_SIZE,
          unsigned int TP_RND,
          unsigned int TP_SAT>
NOINLINE_DECL
fft_window<TT_DATA, TT_COEFF, TP_POINT_SIZE, TP_WINDOW_VSIZE, TP_SHIFT, 1, TP_SSR, TP_DYN_PT_SIZE, TP_RND, TP_SAT>::
    fft_window(const TT_COEFF (&kernel_weights)[TP_POINT_SIZE * (1 + TP_DYN_PT_SIZE)]) {
    // fft_window(const std::array<TT_COEFF, TP_POINT_SIZE*(1 + TP_DYN_PT_SIZE)>& kernel_weights) {
    // The situation here is that samples arrive on stream 0 as samples 0, 2, 4, 6 and on
    // stream 1 as 1, 3, 5, 7, so we want window samples arranged as 0(msb), 2, 4, 6, 1, 3, 5, 7.
    // What this achieves is that the data samples do not need to be interleaved before
    // the multiply nor deinterleaved afterwards, all because the window samples
    // have been pre-compile-time interleaved in like fashion.
    using tally_t = typename std::conditional<std::is_same<TT_DATA, float>::value, float, int64>::type;
    static constexpr int kSamplesInStream = 16 / sizeof(TT_DATA);
    int offset;
    int tableBase = 0;
    int fromIdx;
    int toIdx;
    int tableStartsIdx = 0;
    int vecInPtSize = kVecInFrame;
    tally_t tally;
#if __STREAMS_PER_TILE__ == 2
    for (int ptSizePwr = kPtSizePwr; ptSizePwr >= (TP_DYN_PT_SIZE == 0 ? kPtSizePwr : kMinPtSizePwr); ptSizePwr--) {
        tableStarts[tableStartsIdx++] = tableBase;
        for (int vect = 0; vect < vecInPtSize; vect++) {
            for (int sampIdx = 0; sampIdx < kSamplesInVect; sampIdx++) {
                offset = ((sampIdx << 1) & (kSamplesInVect - 1)) + sampIdx / (kSamplesInVect / 2);
                toIdx = sampIdx + vect * kSamplesInVect + tableBase;
                fromIdx = (vect * kSamplesInVect + offset) + tableBase;
                weights[toIdx] = kernel_weights[fromIdx];
            }
        }
        tableBase += (1 << ptSizePwr);
        vecInPtSize = vecInPtSize >> 1;
    }
#else

    // copy the primary table raw/
    memcpy(weights, &kernel_weights[0], TP_POINT_SIZE * (1 + TP_DYN_PT_SIZE) * sizeof(TT_COEFF));
    tableStarts[0] = 0;
    if
        constexpr(TP_DYN_PT_SIZE == 1) {
            int index = 1;
            int start = 0;
            for (int pt = TP_POINT_SIZE; pt >= 16; pt = pt >> 1) {
                start += pt;
                tableStarts[index++] = start;
            }
        }

#endif
};

// Base specialization, used for static size window API configurations
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_POINT_SIZE,
          unsigned int TP_WINDOW_VSIZE,
          unsigned int TP_SHIFT,
          unsigned int TP_API,
          unsigned int TP_SSR,
          unsigned int TP_DYN_PT_SIZE,
          unsigned int TP_RND,
          unsigned int TP_SAT>
NOINLINE_DECL void fft_window<TT_DATA,
                              TT_COEFF,
                              TP_POINT_SIZE,
                              TP_WINDOW_VSIZE,
                              TP_SHIFT,
                              TP_API,
                              TP_SSR,
                              TP_DYN_PT_SIZE,
                              TP_RND,
                              TP_SAT>::fft_window_main(input_buffer<TT_DATA>& __restrict inWindow,
                                                       output_buffer<TT_DATA>& __restrict outWindow) {
    using dataVect_t = ::aie::vector<TT_DATA, kSamplesInVect>;
    using coeffVect_t = ::aie::vector<TT_COEFF, kSamplesInVect>;
    using accVect_t = ::aie::accum<typename tAccBaseTypeMul<TT_DATA, TT_COEFF>::type, kSamplesInVect>;
    dataVect_t dataVect;
    coeffVect_t* coeffVectPtr;
    coeffVect_t coeffVect;
    accVect_t acc;
    dataVect_t outVect;
    dataVect_t* inPtr = (dataVect_t*)inWindow.data();
    dataVect_t* outPtr = (dataVect_t*)outWindow.data();

    set_rnd_mode<TP_RND>();
    set_sat_mode<TP_SAT>();

    for (int frame = 0; frame < TP_WINDOW_VSIZE / TP_POINT_SIZE; frame++)
        chess_prepare_for_pipelining chess_loop_range(TP_WINDOW_VSIZE / TP_POINT_SIZE, ) {
            coeffVectPtr = (coeffVect_t*)(&this->weights[0]);
            //#pragma unroll (kVecInFrame)
            for (int vect = 0; vect < kVecInFrame; vect++) {
                // dataVect = window_readincr_v<kSamplesInVect>(inWindow);
                dataVect = *inPtr++;
                coeffVect = *coeffVectPtr++;
                acc = ::aie::mul(dataVect, coeffVect);
                outVect = acc.template to_vector<TT_DATA>(TP_SHIFT);
                *outPtr++ = outVect;
            }
        }
};

// Specialization for dynamic size window API configurations
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_POINT_SIZE,
          unsigned int TP_WINDOW_VSIZE,
          unsigned int TP_SHIFT,
          unsigned int TP_API,
          unsigned int TP_SSR,
          unsigned int TP_DYN_PT_SIZE,
          unsigned int TP_RND,
          unsigned int TP_SAT>
NOINLINE_DECL void fft_window<TT_DATA,
                              TT_COEFF,
                              TP_POINT_SIZE,
                              TP_WINDOW_VSIZE,
                              TP_SHIFT,
                              TP_API,
                              TP_SSR,
                              TP_DYN_PT_SIZE,
                              TP_RND,
                              TP_SAT>::fft_window_main_dyn(input_buffer<TT_DATA>& __restrict inWindow,
                                                           output_buffer<TT_DATA>& __restrict outWindow) {
    using dataVect_t = ::aie::vector<TT_DATA, kSamplesInVect>;
    using coeffVect_t = ::aie::vector<TT_COEFF, kSamplesInVect>;
    using accVect_t = ::aie::accum<typename tAccBaseType<TT_DATA, TT_COEFF>::type, kSamplesInVect>;
    dataVect_t blankVect = ::aie::zeros<TT_DATA, kSamplesInVect>();
    dataVect_t dataVect;
    coeffVect_t* coeffVectPtr;
    coeffVect_t coeffVect;
    accVect_t acc;
    dataVect_t outVect;
    int ptSizePwr;
    int ptSize;
    int tableBase;
    unsigned int vecInFrame;
    dataVect_t header;
    // T_buff_256b<TT_DATA> header;
    TT_DATA headerVal;
    dataVect_t* inPtr = (dataVect_t*)inWindow.data();
    dataVect_t* outPtr = (dataVect_t*)outWindow.data();

    set_rnd_mode<TP_RND>();
    set_sat_mode<TP_SAT>();

    // header = window_readincr_256b(inWindow);
    header = *inPtr++;
    //  headerVal = header.val.get(1);
    headerVal = header.get(1);
    ptSizePwr = (int)headerVal.real - kLogSSR;
    ptSize = (1 << ptSizePwr);
    tableBase = tableStarts[kPtSizePwr - ptSizePwr];

    if (ptSizePwr >= kMinPtSizePwr && ptSizePwr <= kMaxPtSizePwr) {
        //    window_writeincr(outWindow, header.val);
        *outPtr++ = header;
        vecInFrame = ptSize >> kLogSamplesInVect;
        for (int frame = 0; frame < TP_WINDOW_VSIZE / TP_POINT_SIZE; frame++)
            chess_prepare_for_pipelining chess_loop_range(TP_WINDOW_VSIZE / TP_POINT_SIZE, ) {
                coeffVectPtr = (coeffVect_t*)(&this->weights[tableBase]);
                //#pragma unroll (kVecInFrame)
                for (int vect = 0; vect < vecInFrame; vect++) {
                    // dataVect = window_readincr_v<kSamplesInVect>(inWindow);
                    dataVect = *inPtr++;
                    coeffVect = *coeffVectPtr++;
                    acc = ::aie::mul(dataVect, coeffVect);
                    outVect = acc.template to_vector<TT_DATA>(TP_SHIFT);
                    // window_writeincr(outWindow, outVect);
                    *outPtr++ = outVect;
                }
                for (int vect = vecInFrame; vect < kVecInFrame; vect++) {
                    dataVect = *inPtr++;
                    *outPtr++ = blankVect;
                }
            }
    } else {
        // indicate that the frame is invalid by setting the flag in the status field of the header.
        // header.val.set(unitVector<TT_DATA>(), std::is_same<TT_DATA,cint16>::value ? 7:3); //set the invalid flag in
        // the status location.
        header.set(unitVector<TT_DATA>(), kSamplesInVect - 1);
        // window_writeincr(outWindow, header.val);
        *outPtr++ = header;

        // write out blank window
        // TT_DATA* ybuff = (TT_DATA*)outWindow->ptr;
        using write_type = ::aie::vector<TT_DATA, __ALIGN_BYTE_SIZE__ / sizeof(TT_DATA)>;
        // write_type* blankDataPtr = (write_type*)(ybuff); //addition is in TT_DATA currency, then cast to 128b
        write_type* blankDataPtr = (write_type*)(outPtr);
        for (int i = 0; i < TP_WINDOW_VSIZE / (__ALIGN_BYTE_SIZE__ / sizeof(TT_DATA)); i++) {
            *blankDataPtr++ = ::aie::zeros<TT_DATA, __ALIGN_BYTE_SIZE__ / sizeof(TT_DATA)>();
        }
    }
};

#if __STREAMS_PER_TILE__ == 2
// Specialization, for stream API configurations (2 streams per tile).
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_POINT_SIZE,
          unsigned int TP_WINDOW_VSIZE,
          unsigned int TP_SHIFT,
          unsigned int TP_SSR,
          unsigned int TP_DYN_PT_SIZE,
          unsigned int TP_RND,
          unsigned int TP_SAT>
NOINLINE_DECL void
fft_window<TT_DATA, TT_COEFF, TP_POINT_SIZE, TP_WINDOW_VSIZE, TP_SHIFT, 1, TP_SSR, TP_DYN_PT_SIZE, TP_RND, TP_SAT>::
    fft_window_main(input_stream<TT_DATA>* __restrict inStream0,
                    input_stream<TT_DATA>* __restrict inStream1,
                    output_stream<TT_DATA>* __restrict outStream0,
                    output_stream<TT_DATA>* __restrict outStream1) {
    using dataVect_t = ::aie::vector<TT_DATA, kSamplesInVect>;
    using coeffVect_t = ::aie::vector<TT_COEFF, kSamplesInVect>;
    using accVect_t = ::aie::accum<typename tAccBaseType<TT_DATA, TT_COEFF>::type, kSamplesInVect>;
    static constexpr int kSamplesInStream = 16 / sizeof(TT_DATA);
    dataVect_t dataVect;
    T_buff_128b<TT_DATA> strm0data, strm1data;
    T_buff_128b<TT_DATA> out0data, out1data;
    coeffVect_t* coeffVectPtr;
    coeffVect_t coeffVect;
    accVect_t acc;
    dataVect_t outVect;

    set_rnd_mode<TP_RND>();
    set_sat_mode<TP_SAT>();

    for (int frame = 0; frame < TP_WINDOW_VSIZE / TP_POINT_SIZE; frame++)
        chess_prepare_for_pipelining chess_loop_range(TP_WINDOW_VSIZE / TP_POINT_SIZE, ) {
            coeffVectPtr = (coeffVect_t*)(&this->weights[0]);
            //    #pragma unroll (kVecInFrame)
            for (int vect = 0; vect < kVecInFrame; vect++)
            // chess_prepare_for_pipelining
            // chess_loop_range(kVecInFrame,)
            {
                strm0data = stream_readincr_128b(inStream0, 0); // 0? refers to physical stream number on tile.
                strm1data = stream_readincr_128b(inStream1, 1); // 1? refers to physical stream number on tile.
                dataVect.insert(0, strm0data.val);
                dataVect.insert(1, strm1data.val);
                coeffVect = *coeffVectPtr++;
                acc = ::aie::mul(dataVect, coeffVect);
                outVect = acc.template to_vector<TT_DATA>(TP_SHIFT);
                out0data.val = outVect.template extract<kSamplesInStream>(0);
                out1data.val = outVect.template extract<kSamplesInStream>(1);
                stream_writeincr_128b(outStream0, out0data, 0);
                stream_writeincr_128b(outStream1, out1data, 1);
            }
        }
};
#endif // __STREAMS_PER_TILE__ == 2

#if __STREAMS_PER_TILE__ == 2
// Specialization, for dynamic size stream API configurations (2 streams per tile).
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_POINT_SIZE,
          unsigned int TP_WINDOW_VSIZE,
          unsigned int TP_SHIFT,
          unsigned int TP_SSR,
          unsigned int TP_DYN_PT_SIZE,
          unsigned int TP_RND,
          unsigned int TP_SAT>
NOINLINE_DECL void
fft_window<TT_DATA, TT_COEFF, TP_POINT_SIZE, TP_WINDOW_VSIZE, TP_SHIFT, 1, TP_SSR, TP_DYN_PT_SIZE, TP_RND, TP_SAT>::
    fft_window_main_dyn(input_stream<TT_DATA>* __restrict inStream0,
                        input_stream<TT_DATA>* __restrict inStream1,
                        output_stream<TT_DATA>* __restrict outStream0,
                        output_stream<TT_DATA>* __restrict outStream1) {
    using dataVect_t = ::aie::vector<TT_DATA, kSamplesInVect>;
    using coeffVect_t = ::aie::vector<TT_COEFF, kSamplesInVect>;
    using accVect_t = ::aie::accum<typename tAccBaseType<TT_DATA, TT_COEFF>::type, kSamplesInVect>;
    static constexpr int kSamplesInStream = 16 / sizeof(TT_DATA);
    dataVect_t dataVect;
    T_buff_128b<TT_DATA> strm0data, strm1data;
    T_buff_128b<TT_DATA> out0data, out1data;
    coeffVect_t* coeffVectPtr;
    coeffVect_t coeffVect;
    accVect_t acc;
    dataVect_t outVect;
    int ptSizePwr;
    int ptSize;
    int tableBase;
    unsigned int vecInFrame;
    T_buff_128b<TT_DATA> header;
    TT_DATA headerVal;
    T_buff_128b<TT_DATA> blankData;

    set_rnd_mode<TP_RND>();
    set_sat_mode<TP_SAT>();

    blankData.val = ::aie::zeros<TT_DATA, 16 / sizeof(TT_DATA)>();
    header = stream_readincr_128b(inStream1, 1);
    header = stream_readincr_128b(inStream0, 0);
    stream_writeincr_128b(outStream1, header, 1);
    stream_writeincr_128b(outStream0, header, 0);
    headerVal = header.val.get(1);
    ptSizePwr = (int)headerVal.real - kLogSSR;
    ptSize = (1 << ptSizePwr);
    tableBase = tableStarts[kPtSizePwr - ptSizePwr];
    for (int i = 1; i < kSamplesInVect / kSamplesInStream; i++) { // read and ignore remainder of header
        header = stream_readincr_128b(inStream1, 1);
        header = stream_readincr_128b(inStream0, 0);
    }

    if (ptSizePwr >= kMinPtSizePwr && ptSizePwr <= kMaxPtSizePwr) {
        for (int i = 1; i < kSamplesInVect / kSamplesInStream; i++) { // write out remainder of header
            stream_writeincr_128b(outStream1, header, 1);
            stream_writeincr_128b(outStream0, header, 0);
        }
        vecInFrame = ptSize >> kLogSamplesInVect;
        for (int frame = 0; frame < TP_WINDOW_VSIZE / TP_POINT_SIZE; frame++)
            chess_prepare_for_pipelining chess_loop_range(TP_WINDOW_VSIZE / TP_POINT_SIZE, ) {
                coeffVectPtr = (coeffVect_t*)(&this->weights[tableBase]);
                //    #pragma unroll (kVecInFrame)
                for (int vect = 0; vect < vecInFrame; vect++)
                // chess_prepare_for_pipelining
                // chess_loop_range(kVecInFrame,)
                {
                    strm0data = stream_readincr_128b(inStream0, 0); // 0? refers to physical stream number on tile.
                    strm1data = stream_readincr_128b(inStream1, 1); // 1? refers to physical stream number on tile.
                    dataVect.insert(0, strm0data.val);
                    dataVect.insert(1, strm1data.val);
                    coeffVect = *coeffVectPtr++;
                    acc = ::aie::mul(dataVect, coeffVect);
                    outVect = acc.template to_vector<TT_DATA>(TP_SHIFT);
                    out0data.val = outVect.template extract<kSamplesInStream>(0);
                    out1data.val = outVect.template extract<kSamplesInStream>(1);
                    stream_writeincr_128b(outStream0, out0data, 0);
                    stream_writeincr_128b(outStream1, out1data, 1);
                }
                for (int vect = vecInFrame; vect < kVecInFrame; vect++) // fill in remainder of frame holder with blanks
                // chess_prepare_for_pipelining
                // chess_loop_range(kVecInFrame,)
                {
                    // read in and discard, while writing blanks.
                    strm0data = stream_readincr_128b(inStream0, 0); // 0? refers to physical stream number on tile.
                    strm1data = stream_readincr_128b(inStream1, 1); // 1? refers to physical stream number on tile.
                    stream_writeincr_128b(outStream0, blankData, 0);
                    stream_writeincr_128b(outStream1, blankData, 1);
                }
            }
    } else {
        // indicate that the frame is invalid by setting the flag in the status field of the header.
        for (int i = 1; i < kSamplesInVect / kSamplesInStream - 1; i++) { // blank all reserved fields in header
            stream_writeincr_128b(outStream0, blankData, 0);
            stream_writeincr_128b(outStream1, blankData, 1);
        }

        header.val.set(unitVector<TT_DATA>(),
                       std::is_same<TT_DATA, cint16>::value ? 3 : 1); // set the invalid flag in the status location.
        stream_writeincr_128b(outStream0, header, 0);
        stream_writeincr_128b(outStream1, header, 1);

        // write out blank window (and also read and discard the illegal input 'window')
        for (int i = 0; i < TP_WINDOW_VSIZE / (kSamplesInStream * 2); i++) {
            header = stream_readincr_128b(inStream1, 1); // read and discard the illegal 'window'
            header = stream_readincr_128b(inStream0, 0); // read and discard the illegal 'window'
            stream_writeincr_128b(outStream0, blankData, 0);
            stream_writeincr_128b(outStream1, blankData, 1);
        }
    }
};
#endif // __STREAMS_PER_TILE__ == 2

#if __STREAMS_PER_TILE__ == 1
// Specialization, for stream API configurations (1 stream per tile).
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_POINT_SIZE,
          unsigned int TP_WINDOW_VSIZE,
          unsigned int TP_SHIFT,
          unsigned int TP_SSR,
          unsigned int TP_DYN_PT_SIZE,
          unsigned int TP_RND,
          unsigned int TP_SAT>
NOINLINE_DECL void
fft_window<TT_DATA, TT_COEFF, TP_POINT_SIZE, TP_WINDOW_VSIZE, TP_SHIFT, 1, TP_SSR, TP_DYN_PT_SIZE, TP_RND, TP_SAT>::
    fft_window_main(input_stream<TT_DATA>* __restrict inStream0, output_stream<TT_DATA>* __restrict outStream0) {
    using dataVect_t = ::aie::vector<TT_DATA, kSamplesInVect>;
    using coeffVect_t = ::aie::vector<TT_COEFF, kSamplesInVect>;
    using accVect_t = ::aie::accum<typename tAccBaseType<TT_DATA, TT_COEFF>::type, kSamplesInVect>;
    constexpr int kSamplesIn128b = 128 / 8 / sizeof(TT_DATA);
    using t128VectorType = ::aie::vector<TT_DATA, kSamplesIn128b>;

    dataVect_t dataVect;
    t128VectorType strm0data, strm1data;
    t128VectorType out0data, out1data;
    coeffVect_t* coeffVectPtr;
    coeffVect_t coeffVect;
    accVect_t acc;
    dataVect_t outVect;

    set_rnd_mode<TP_RND>();
    set_sat_mode<TP_SAT>();

    for (int frame = 0; frame < TP_WINDOW_VSIZE / TP_POINT_SIZE; frame++)
        chess_prepare_for_pipelining chess_loop_range(TP_WINDOW_VSIZE / TP_POINT_SIZE, ) {
            coeffVectPtr = (coeffVect_t*)(&this->weights[0]);
            //    #pragma unroll (kVecInFrame)
            for (int vect = 0; vect < kVecInFrame; vect++)
            // chess_prepare_for_pipelining
            // chess_loop_range(kVecInFrame,)
            {
                strm0data = readincr_v<kSamplesIn128b>(inStream0); // 0? refers to physical stream number on tile.
                strm1data = readincr_v<kSamplesIn128b>(inStream0); // 0? refers to physical stream number on tile.
                dataVect.insert(0, strm0data);
                dataVect.insert(1, strm1data);
                coeffVect = *coeffVectPtr++;
                acc = ::aie::mul(dataVect, coeffVect);
                outVect = acc.template to_vector<TT_DATA>(TP_SHIFT);
                out0data = outVect.template extract<kSamplesIn128b>(0);
                out1data = outVect.template extract<kSamplesIn128b>(1);
                writeincr<aie_stream_resource_out::a, TT_DATA, kSamplesIn128b>(outStream0, out0data);
                writeincr<aie_stream_resource_out::a, TT_DATA, kSamplesIn128b>(outStream0, out1data);
            }
        }
};
#endif // __STREAMS_PER_TILE__ == 1

#if __STREAMS_PER_TILE__ == 1
// Specialization, for dynamic size stream API configurations (1 stream per tile).
// Note that this is cloned from the 2 stream case, and to keep edits to a minimum loop sizes are
// kept, simply duplicating reads and writes which would have gone to the second stream to another
// read or write of the first stream.
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_POINT_SIZE,
          unsigned int TP_WINDOW_VSIZE,
          unsigned int TP_SHIFT,
          unsigned int TP_SSR,
          unsigned int TP_DYN_PT_SIZE,
          unsigned int TP_RND,
          unsigned int TP_SAT>
NOINLINE_DECL void
fft_window<TT_DATA, TT_COEFF, TP_POINT_SIZE, TP_WINDOW_VSIZE, TP_SHIFT, 1, TP_SSR, TP_DYN_PT_SIZE, TP_RND, TP_SAT>::
    fft_window_main_dyn(input_stream<TT_DATA>* __restrict inStream0, output_stream<TT_DATA>* __restrict outStream0) {
    using dataVect_t = ::aie::vector<TT_DATA, kSamplesInVect>;
    using coeffVect_t = ::aie::vector<TT_COEFF, kSamplesInVect>;
    using accVect_t = ::aie::accum<typename tAccBaseType<TT_DATA, TT_COEFF>::type, kSamplesInVect>;
    constexpr int kSamplesIn128b = 128 / 8 / sizeof(TT_DATA);
    using t128VectorType = ::aie::vector<TT_DATA, kSamplesIn128b>;
    constexpr int kHeaderSize = __ALIGN_BYTE_SIZE__ / sizeof(TT_DATA);

    dataVect_t dataVect;
    t128VectorType strm0data, strm1data;
    t128VectorType out0data, out1data;
    coeffVect_t* coeffVectPtr;
    coeffVect_t coeffVect;
    accVect_t acc;
    dataVect_t outVect;
    int ptSizePwr;
    int ptSize;
    int tableBase;
    unsigned int vecInFrame;
    t128VectorType header;
    TT_DATA headerVal;
    t128VectorType blankData;

    set_rnd_mode<TP_RND>();
    set_sat_mode<TP_SAT>();

    printf("tracer1\n");
    blankData = ::aie::zeros<TT_DATA, kSamplesIn128b>();
    header = readincr_v<kSamplesIn128b>(inStream0); // 0? refers to physical stream number on tile.
    writeincr<aie_stream_resource_out::a, TT_DATA, kSamplesIn128b>(outStream0, header);
    headerVal = header.get(1);
    ptSizePwr = (int)headerVal.real - kLogSSR;
    ptSize = (1 << ptSizePwr);
    tableBase = tableStarts[kPtSizePwr - ptSizePwr];
    for (int i = 1; i < kHeaderSize / kSamplesIn128b; i++) {
        header = readincr_v<kSamplesIn128b>(inStream0); // 0? refers to physical stream number on tile.
    }
    for (int i = 1; i < kHeaderSize / kSamplesIn128b - 1; i++) { // write out reserved fields of header
        writeincr<aie_stream_resource_out::a, TT_DATA, kSamplesIn128b>(outStream0, blankData);
    }

    if (ptSizePwr >= kMinPtSizePwr && ptSizePwr <= kMaxPtSizePwr) {
        writeincr<aie_stream_resource_out::a, TT_DATA, kSamplesIn128b>(outStream0, header);
        vecInFrame = ptSize >> kLogSamplesInVect;
        for (int frame = 0; frame < TP_WINDOW_VSIZE / TP_POINT_SIZE; frame++)
            chess_prepare_for_pipelining chess_loop_range(TP_WINDOW_VSIZE / TP_POINT_SIZE, ) {
                coeffVectPtr = (coeffVect_t*)(&this->weights[tableBase]);
                //    #pragma unroll (kVecInFrame)
                for (int vect = 0; vect < vecInFrame; vect++)
                // chess_prepare_for_pipelining
                // chess_loop_range(kVecInFrame,)
                {
                    strm0data = readincr_v<kSamplesIn128b>(inStream0); // 0? refers to physical stream number on tile.
                    strm1data = readincr_v<kSamplesIn128b>(inStream0); // 0? refers to physical stream number on tile.
                    dataVect.insert(0, strm0data);
                    dataVect.insert(1, strm1data);
                    coeffVect = *coeffVectPtr++;
                    acc = ::aie::mul(dataVect, coeffVect);
                    outVect = acc.template to_vector<TT_DATA>(TP_SHIFT);
                    out0data = outVect.template extract<kSamplesIn128b>(0);
                    out1data = outVect.template extract<kSamplesIn128b>(1);
                    writeincr<aie_stream_resource_out::a, TT_DATA, kSamplesIn128b>(outStream0, out0data);
                    writeincr<aie_stream_resource_out::a, TT_DATA, kSamplesIn128b>(outStream0, out1data);
                }
                for (int vect = vecInFrame; vect < kVecInFrame; vect++) // fill in remainder of frame holder with blanks
                // chess_prepare_for_pipelining
                // chess_loop_range(kVecInFrame,)
                {
                    // read in and discard, while writing blanks.
                    strm0data = readincr_v<kSamplesIn128b>(inStream0); // 0? refers to physical stream number on tile.
                    strm1data = readincr_v<kSamplesIn128b>(inStream0); // 0? refers to physical stream number on tile.
                    writeincr<aie_stream_resource_out::a, TT_DATA, kSamplesIn128b>(outStream0, blankData);
                    writeincr<aie_stream_resource_out::a, TT_DATA, kSamplesIn128b>(outStream0, blankData);
                }
            }
    } else {
        // indicate that the frame is invalid by setting the flag in the status field of the header.
        header.set(unitVector<TT_DATA>(),
                   std::is_same<TT_DATA, cint16>::value ? 3 : 1); // set the invalid flag in the status location.
        writeincr<aie_stream_resource_out::a, TT_DATA, kSamplesIn128b>(outStream0, header);

        // write out blank window
        for (int i = 0; i < TP_WINDOW_VSIZE / (16 / sizeof(TT_DATA)); i++) {
            header = readincr_v<kSamplesIn128b>(inStream0); // read and discard illegal 'window'
            writeincr<aie_stream_resource_out::a, TT_DATA, kSamplesIn128b>(outStream0, blankData);
        }
    }
};
#endif // __STREAMS_PER_TILE__ == 1
}
}
}
}
}
