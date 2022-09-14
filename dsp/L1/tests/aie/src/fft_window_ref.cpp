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
FFT Window reference model
*/

#include "fft_window_ref.hpp"
//#include "fir_ref_utils.hpp"
#include "fft_ref_utils.hpp"

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
T_D scalar_mult(T_D d, T_C k, const int shift) {
    T_D retVal;
    int64 temp;
    int64 roundConst = shift > 0 ? ((int64)1 << (shift - 1)) : 0;
    // printf ("d = %lld, %lld\n", d.real, d.imag);
    temp = (int64)d.real * (int64)k + roundConst;
    // printf ("temp = %lld\n", temp);
    temp >>= shift;
    // printf ("temp = %lld\n", temp);
    if (temp > std::numeric_limits<T_C>::max()) {
        temp = std::numeric_limits<T_C>::max();
    }
    if (temp < std::numeric_limits<T_C>::min()) {
        temp = std::numeric_limits<T_C>::min();
    }
    retVal.real = temp;
    temp = (int64)d.imag * (int64)k + roundConst;
    temp >>= shift;
    if (temp > std::numeric_limits<T_C>::max()) {
        temp = std::numeric_limits<T_C>::max();
    }
    if (temp < std::numeric_limits<T_C>::min()) {
        //  printf ("temp = %lld\n", temp);
        temp = std::numeric_limits<T_C>::min();
        //  printf ("temp = %lld\n", temp);
    }
    retVal.imag = temp;
    return retVal;
}

template <>
cfloat scalar_mult<cfloat, float>(cfloat d, float k, const int shift) {
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
          unsigned int TP_DYN_PT_SIZE>
void fft_window_ref<TT_DATA, TT_COEFF, TP_POINT_SIZE, TP_WINDOW_VSIZE, TP_SHIFT, TP_API, TP_SSR, TP_DYN_PT_SIZE>::
    fft_window_main(input_window<TT_DATA>* inWindow0, output_window<TT_DATA>* outWindow0) {
    TT_DATA d_in;
    TT_DATA d_out;
    TT_COEFF* coeff_base = &this->weights[0];
    unsigned int ptSize =
        TP_POINT_SIZE; // default to static point size value. May be overwritten if dynamic point size selected.
    int16 tableSelect = 0;
    TT_COEFF coeff;

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

            headerPtr = (TT_DATA*)inWindow0->ptr;
            header = *headerPtr++; // saved for later when output to outWindow
            window_writeincr(outWindow0, header);
            header = *headerPtr++; // saved for later when output to outWindow
            window_writeincr(outWindow0, header);
            ptSizePwr = (int32)header.real - kLogSSR; // Modified for case where FFT is a subframe processor. Then the
                                                      // header refers to the overall point size.
            ptSize = ((unsigned int)1) << ptSizePwr;
            tableSelect = (kPtSizePwr - ptSizePwr);
            coeff_base = &this->weights[this->tableStarts[tableSelect]];
            for (int i = 2; i < kHeaderSize - 1; i++) {
                window_writeincr(outWindow0, blankVector<TT_DATA>());
            }
            if ((ptSizePwr >= kMinPtSizePwr) && (ptSizePwr <= kMaxPtSizePwr)) {
                window_writeincr(outWindow0, blankVector<TT_DATA>()); // Status word. 0 indicated all ok.
            } else {
                window_writeincr(outWindow0, unitVector<TT_DATA>()); // Status word. 0 indicated all ok.
            }
            window_incr(inWindow0, kHeaderSize);
        }

    for (int frame = 0; frame < TP_WINDOW_VSIZE / TP_POINT_SIZE; frame++) {
        for (unsigned int i = 0; i < ptSize; i++) {
            d_in = window_readincr(inWindow0); // read input data
            coeff = coeff_base[i];
            d_out = scalar_mult<TT_DATA, TT_COEFF>(d_in, coeff, TP_SHIFT);
            window_writeincr(outWindow0, d_out);
        }
        for (unsigned int i = ptSize; i < TP_POINT_SIZE; i++) {
            d_in = window_readincr(inWindow0);                    // read input data just to flush out unused samples
            window_writeincr(outWindow0, blankVector<TT_DATA>()); // but write out zeros so that detritus is overwritten
                                                                  // (else verification is hard)
        }
    }
};

// Streaming specialization
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_POINT_SIZE,
          unsigned int TP_WINDOW_VSIZE,
          unsigned int TP_SHIFT,
          unsigned int TP_SSR,
          unsigned int TP_DYN_PT_SIZE>
void fft_window_ref<TT_DATA, TT_COEFF, TP_POINT_SIZE, TP_WINDOW_VSIZE, TP_SHIFT, 1, TP_SSR, TP_DYN_PT_SIZE>::
    fft_window_main(input_stream<TT_DATA>* inStream0,
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
            d_out0 = scalar_mult<TT_DATA, TT_COEFF>(d_in0, coeff0, TP_SHIFT);
            d_out1 = scalar_mult<TT_DATA, TT_COEFF>(d_in1, coeff1, TP_SHIFT);
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
}
}
}
}
}
