/*
 * Copyright (C) 2019-2022, Xilinx, Inc.
 * Copyright (C) 2022-2024, Advanced Micro Devices, Inc.
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
FFT Window reference model
*/
#include "device_defs.h"
#include "fft_window_ref.hpp"
#include "fir_ref_utils.hpp"
#include "fft_ref_utils.hpp"
//#define _DSPLIB_FFT_WINDOW_REF_DEBUG_

namespace xf {
namespace dsp {
namespace aie {
namespace fft {
namespace windowfn {

// unitVector cannot be in fft_ref_utils because that is used by 2 different kernels, so leads to multiple definition.
template <typename T_D>
constexpr T_D unitVector(){};
template <>
constexpr cint16 unitVector<cint16>() {
    cint16 temp;
    temp.real = 1;
    temp.imag = 0;
    return temp;
};
template <>
constexpr cint32 unitVector<cint32>() {
    cint32 temp;
    temp.real = 1;
    temp.imag = 0;
    return temp;
};
template <>
constexpr cfloat unitVector<cfloat>() {
    cfloat temp;
    temp.real = 1.0;
    temp.imag = 0.0;
    return temp;
};

template <typename T_D>
constexpr T_D blankVector(){};
template <>
constexpr cint16 blankVector<cint16>() {
    cint16 temp;
    temp.real = 0;
    temp.imag = 0;
    return temp;
};
template <>
constexpr cint32 blankVector<cint32>() {
    cint32 temp;
    temp.real = 0;
    temp.imag = 0;
    return temp;
};
template <>
constexpr cfloat blankVector<cfloat>() {
    cfloat temp;
    temp.real = 0.0;
    temp.imag = 0.0;
    return temp;
};

template <typename T_D, typename T_C>
T_D scalar_mult(T_D d, T_C k, const int shift, unsigned int t_rnd, unsigned int t_sat) {
    T_D retVal;
    int64 temp;
    T_accRef<T_D> acc;

    acc.real = (int64)d.real * (int64)k;
    acc.imag = (int64)d.imag * (int64)k;

    roundAcc(t_rnd, shift, acc);
    saturateAcc(acc, t_sat);

    retVal.real = acc.real;
    retVal.imag = acc.imag;

    return retVal;
}

template <>
cfloat scalar_mult<cfloat, float>(cfloat d, float k, const int shift, unsigned int t_rnd, unsigned int t_sat) {
    cfloat retVal;
    retVal.real = (float)d.real * (float)k; // no rounding or shift for floats
    retVal.imag = (float)d.imag * (float)k;
    return retVal;
}

// FFT window - default/base 'specialization' for both static and dynamic point size
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
void fft_window_ref<TT_DATA,
                    TT_COEFF,
                    TP_POINT_SIZE,
                    TP_WINDOW_VSIZE,
                    TP_SHIFT,
                    TP_API,
                    TP_SSR,
                    TP_DYN_PT_SIZE,
                    TP_RND,
                    TP_SAT>::fft_window_main(input_buffer<TT_DATA>& inWindow0, output_buffer<TT_DATA>& outWindow0) {
    TT_DATA d_in;
    TT_DATA d_out;
    TT_COEFF* coeff_base = &this->weights[0];
    unsigned int ptSize =
        TP_POINT_SIZE; // default to static point size value. May be overwritten if dynamic point size selected.
    int16 tableSelect = 0;
    TT_COEFF coeff;
    TT_DATA* inPtr = (TT_DATA*)inWindow0.data();
    TT_DATA* outPtr = (TT_DATA*)outWindow0.data();

    if
        constexpr(TP_DYN_PT_SIZE == 1) {
            constexpr unsigned int kPtSizePwr = fnGetPointSizePower<TP_POINT_SIZE>();
            constexpr unsigned int kMaxPtSizePwr = 16; // largest is 64k = 1<<16;
            constexpr unsigned int kMinPtSizePwr = 4;  // smallest is 16 = 1<<4;
            constexpr unsigned int kHeaderSize =
                32 / (sizeof(TT_DATA)); // dynamic point size header size (256bits or 32 bytes) in terms of samples
            TT_DATA header;
            int16 ptSizePwr; // default to static point size value. May be overwritten if dynamic point size selected.

            // headerPtr = (TT_DATA*)inWindow0->ptr;
            header = *inPtr++; // saved for later when output to outWindow
            // window_writeincr(outWindow0, header);
            *outPtr++ = header;
            header = *inPtr++; // saved for later when output to outWindow
            // window_writeincr(outWindow0, header);
            *outPtr++ = header;
            ptSizePwr = (int32)header.real - kLogSSR; // Modified for case where FFT is a subframe processor. Then the
                                                      // header refers to the overall point size.
            ptSize = ((unsigned int)1) << ptSizePwr;
            tableSelect = (kPtSizePwr - ptSizePwr);
            coeff_base = &this->weights[this->tableStarts[tableSelect]];
            for (int i = 2; i < kHeaderSize - 1; i++) {
                // window_writeincr(outWindow0, blankVector<TT_DATA>());
                *outPtr++ = blankVector<TT_DATA>();
            }
            if ((ptSizePwr >= kMinPtSizePwr) && (ptSizePwr <= kMaxPtSizePwr)) {
                // window_writeincr(outWindow0, blankVector<TT_DATA>()); //Status word. 0 indicated all ok.
                *outPtr++ = blankVector<TT_DATA>();
            } else {
                // window_writeincr(outWindow0, unitVector<TT_DATA>()); //Status word. 0 indicated all ok.
                *outPtr++ = unitVector<TT_DATA>();
            }
            inPtr += kHeaderSize - 2; // two reads already;
            // window_incr(inWindow0,kHeaderSize);
        }

    for (int frame = 0; frame < TP_WINDOW_VSIZE / TP_POINT_SIZE; frame++) {
        for (unsigned int i = 0; i < ptSize; i++) {
            // d_in = window_readincr(inWindow0);  //read input data
            d_in = *inPtr++;
            coeff = coeff_base[i];
            d_out = scalar_mult<TT_DATA, TT_COEFF>(d_in, coeff, TP_SHIFT, TP_RND, TP_SAT);
            // window_writeincr(outWindow0, d_out) ;
            *outPtr++ = d_out;
        }
        for (unsigned int i = ptSize; i < TP_POINT_SIZE; i++) {
            // d_in = window_readincr(inWindow0);  //read input data just to flush out unused samples
            d_in = *inPtr++;
            // window_writeincr(outWindow0, blankVector<TT_DATA>()) ;//but write out zeros so that detritus is
            // overwritten (else verification is hard)
            *outPtr++ = blankVector<TT_DATA>();
        }
    }
};

#if __STREAMS_PER_TILE__ == 2
// Streaming specialization (2 streams)
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_POINT_SIZE,
          unsigned int TP_WINDOW_VSIZE,
          unsigned int TP_SHIFT,
          unsigned int TP_SSR,
          unsigned int TP_DYN_PT_SIZE,
          unsigned int TP_RND,
          unsigned int TP_SAT>
void fft_window_ref<TT_DATA,
                    TT_COEFF,
                    TP_POINT_SIZE,
                    TP_WINDOW_VSIZE,
                    TP_SHIFT,
                    1,
                    TP_SSR,
                    TP_DYN_PT_SIZE,
                    TP_RND,
                    TP_SAT>::fft_window_main(input_stream<TT_DATA>* inStream0,
                                             input_stream<TT_DATA>* inStream1,
                                             output_stream<TT_DATA>* outStream0,
                                             output_stream<TT_DATA>* outStream1) {
    TT_DATA d_in0;
    TT_DATA d_out0;
    TT_DATA d_in1;
    TT_DATA d_out1;

    TT_COEFF* coeff_base = &this->weights[0];
    unsigned int ptSize =
        TP_POINT_SIZE; // default to static point size value. May be overwritten if dynamic point size selected.
    int16 tableSelect = 0;
    TT_COEFF coeff0, coeff1;

    if
        constexpr(TP_DYN_PT_SIZE == 1) {
            constexpr unsigned int kPtSizePwr = fnGetPointSizePower<TP_POINT_SIZE>();
            constexpr unsigned int kMaxPtSizePwr = 16; // largest is 64k = 1<<16;
            constexpr unsigned int kMinPtSizePwr = 4;  // smallest is 16 = 1<<4;
            constexpr unsigned int kHeaderSize =
                32 / (sizeof(TT_DATA)); // dynamic point size header size (256bits or 32 bytes) in terms of samples
            TT_DATA* headerPtr;
            TT_DATA header;
            int16 ptSizePwr; // default to static point size value. May be overwritten if dynamic point size selected.

            // first field is direction - irrelevant to fft_window, so ignore.
            writeincr(outStream0, readincr(inStream0));
            writeincr(outStream1, readincr(inStream1));

            // second field is point size power, but only need to read it from one port. Just pass through the other.
            writeincr(outStream1, readincr(inStream1));
            d_in0 = readincr(inStream0);
            writeincr(outStream0, d_in0);
            ptSizePwr = (int32)d_in0.real - kLogSSR;
            ptSize = ((unsigned int)1) << ptSizePwr;
            tableSelect = kPtSizePwr - ptSizePwr;
            coeff_base = &this->weights[this->tableStarts[tableSelect]];

            // copy the rest of the header to output streams
            for (int i = 2; i < kHeaderSize - 1; i++) {
                writeincr(outStream0, readincr(inStream0));
                writeincr(outStream1, readincr(inStream1));
            }
            d_in0 = readincr(inStream0); // read input data status field
            d_in1 = readincr(inStream1); // read input data statis field
            //...but set the status accordingly
            if ((ptSizePwr >= kMinPtSizePwr) && (ptSizePwr <= kMaxPtSizePwr)) {
                writeincr(outStream0, blankVector<TT_DATA>()); // Status word. 0 indicated all ok.
                writeincr(outStream1, blankVector<TT_DATA>()); // Status word. 0 indicated all ok.
            } else {
                writeincr(outStream0, unitVector<TT_DATA>()); // Status word. 0 indicated all ok.
                writeincr(outStream1, unitVector<TT_DATA>()); // Status word. 0 indicated all ok.
            }
        }

    for (int frame = 0; frame < TP_WINDOW_VSIZE / TP_POINT_SIZE; frame++) {
        for (unsigned int i = 0; i < ptSize / 2; i++) { // /2 because there are 2 streams
            d_in0 = readincr(inStream0);                // read input data
            d_in1 = readincr(inStream1);                // read input data
            coeff0 = coeff_base[i * 2];
            coeff1 = coeff_base[(i * 2 + 1)];
            d_out0 = scalar_mult<TT_DATA, TT_COEFF>(d_in0, coeff0, TP_SHIFT, TP_RND, TP_SAT);
            d_out1 = scalar_mult<TT_DATA, TT_COEFF>(d_in1, coeff1, TP_SHIFT, TP_RND, TP_SAT);
            writeincr(outStream0, d_out0);
            writeincr(outStream1, d_out1);
        }
        for (unsigned int i = ptSize / 2; i < TP_POINT_SIZE / 2; i++) { // /2 because there are 2 streams
            d_in0 = readincr(inStream0);                                // read input data
            d_in1 = readincr(inStream1);                                // read input data

            writeincr(outStream0, blankVector<TT_DATA>());
            writeincr(outStream1, blankVector<TT_DATA>());
        }
    }
};
#endif // __STREAMS_PER_TILE__ == 2

#if __STREAMS_PER_TILE__ == 1
// Streaming specialization (1 stream)
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_POINT_SIZE,
          unsigned int TP_WINDOW_VSIZE,
          unsigned int TP_SHIFT,
          unsigned int TP_SSR,
          unsigned int TP_DYN_PT_SIZE,
          unsigned int TP_RND,
          unsigned int TP_SAT>
void fft_window_ref<TT_DATA,
                    TT_COEFF,
                    TP_POINT_SIZE,
                    TP_WINDOW_VSIZE,
                    TP_SHIFT,
                    1,
                    TP_SSR,
                    TP_DYN_PT_SIZE,
                    TP_RND,
                    TP_SAT>::fft_window_main(input_stream<TT_DATA>* inStream0, output_stream<TT_DATA>* outStream0) {
    TT_DATA d_in0;
    TT_DATA d_out0;
    TT_DATA d_in1;
    TT_DATA d_out1;

    TT_COEFF* coeff_base = &this->weights[0];
    unsigned int ptSize =
        TP_POINT_SIZE; // default to static point size value. May be overwritten if dynamic point size selected.
    int16 tableSelect = 0;
    TT_COEFF coeff0, coeff1;

    if
        constexpr(TP_DYN_PT_SIZE == 1) {
            constexpr unsigned int kPtSizePwr = fnGetPointSizePower<TP_POINT_SIZE>();
            constexpr unsigned int kMaxPtSizePwr = 16; // largest is 64k = 1<<16;
            constexpr unsigned int kMinPtSizePwr = 4;  // smallest is 16 = 1<<4;
            constexpr unsigned int kHeaderSize =
                32 / (sizeof(TT_DATA)); // dynamic point size header size (256bits or 32 bytes) in terms of samples
            TT_DATA* headerPtr;
            TT_DATA header;
            int16 ptSizePwr; // default to static point size value. May be overwritten if dynamic point size selected.

            // first field is direction - irrelevant to fft_window, so ignore.
            writeincr(outStream0, readincr(inStream0));

            // second field is point size power, but only need to read it from one port. Just pass through the other.
            d_in0 = readincr(inStream0);
            writeincr(outStream0, d_in0);
            ptSizePwr = (int32)d_in0.real - kLogSSR;
            ptSize = ((unsigned int)1) << ptSizePwr;
            tableSelect = kPtSizePwr - ptSizePwr;
            coeff_base = &this->weights[this->tableStarts[tableSelect]];

            // copy the rest of the header to output streams
            for (int i = 2; i < kHeaderSize - 1; i++) {
                writeincr(outStream0, readincr(inStream0));
            }
            d_in0 = readincr(inStream0); // read input data status field
            //...but set the status accordingly
            if ((ptSizePwr >= kMinPtSizePwr) && (ptSizePwr <= kMaxPtSizePwr)) {
                writeincr(outStream0, blankVector<TT_DATA>()); // Status word. 0 indicated all ok.
            } else {
                writeincr(outStream0, unitVector<TT_DATA>()); // Status word. 0 indicated all ok.
            }
        }

    // This code is cloned from the case with 2 streams, with a minimal mod for single stream.
    // It could be rewritten for single stream operation.
    for (int frame = 0; frame < TP_WINDOW_VSIZE / TP_POINT_SIZE; frame++) {
        for (unsigned int i = 0; i < ptSize / 2; i++) { // /2 because there are 2 streams
            d_in0 = readincr(inStream0);                // read input data
            d_in1 = readincr(inStream0);                // read input data
            coeff0 = coeff_base[i * 2];
            coeff1 = coeff_base[(i * 2 + 1)];
            d_out0 = scalar_mult<TT_DATA, TT_COEFF>(d_in0, coeff0, TP_SHIFT, TP_RND, TP_SAT);
            d_out1 = scalar_mult<TT_DATA, TT_COEFF>(d_in1, coeff1, TP_SHIFT, TP_RND, TP_SAT);
            writeincr(outStream0, d_out0);
            writeincr(outStream0, d_out1);
        }
        for (unsigned int i = ptSize / 2; i < TP_POINT_SIZE / 2; i++) { // /2 because there are 2 streams
            d_in0 = readincr(inStream0);                                // read input data
            d_in1 = readincr(inStream0);                                // read input data

            writeincr(outStream0, blankVector<TT_DATA>());
            writeincr(outStream0, blankVector<TT_DATA>());
        }
    }
};
#endif // __STREAMS_PER_TILE__ == 1
}
}
}
}
}
