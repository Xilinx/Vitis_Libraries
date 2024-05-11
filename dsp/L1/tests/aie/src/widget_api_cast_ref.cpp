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
Widget API Cast reference model
*/

#include <adf.h>
#include "aie_api/aie_adf.hpp"
#include "widget_api_cast_ref.hpp"
#include "fir_ref_utils.hpp"

namespace xf {
namespace dsp {
namespace aie {
namespace widget {
namespace api_cast {

// Widget API Cast - default/base 'specialization' - window to window copy (may help routing)
template <typename TT_DATA,
          unsigned int TP_IN_API,
          unsigned int TP_OUT_API,
          unsigned int TP_NUM_INPUTS,
          unsigned int TP_WINDOW_VSIZE,
          unsigned int TP_NUM_OUTPUT_CLONES,
          unsigned int TP_PATTERN,
          unsigned int TP_HEADER_BYTES>
void widget_api_cast_ref<TT_DATA,
                         TP_IN_API,
                         TP_OUT_API,
                         TP_NUM_INPUTS,
                         TP_WINDOW_VSIZE,
                         TP_NUM_OUTPUT_CLONES,
                         TP_PATTERN,
                         TP_HEADER_BYTES>::transferData(input_buffer<TT_DATA>& inWindow0,
                                                        output_buffer<TT_DATA>& outWindow0) {
    TT_DATA d_in;
    TT_DATA* inPtr = inWindow0.data();
    TT_DATA* outPtr = outWindow0.data();

    for (unsigned int i = 0; i < TP_WINDOW_VSIZE + TP_HEADER_BYTES / sizeof(TT_DATA); i++) {
        *outPtr++ = *inPtr++;
    }
};

// Widget API Cast - dual cloned output 'specialization'
template <typename TT_DATA, unsigned int TP_WINDOW_VSIZE, unsigned int TP_PATTERN, unsigned int TP_HEADER_BYTES>
void widget_api_cast_ref<TT_DATA, kWindowAPI, kWindowAPI, 1, TP_WINDOW_VSIZE, 2, TP_PATTERN, TP_HEADER_BYTES>::
    transferData(input_buffer<TT_DATA>& inWindow0,
                 output_buffer<TT_DATA>& outWindow0,
                 output_buffer<TT_DATA>& outWindow1) {
    TT_DATA d_in;
    TT_DATA* inPtr = inWindow0.data();
    TT_DATA* outPtr0 = outWindow0.data();
    TT_DATA* outPtr1 = outWindow1.data();

    for (unsigned int i = 0; i < TP_WINDOW_VSIZE + TP_HEADER_BYTES / sizeof(TT_DATA); i++) {
        d_in = *inPtr++; // read input data
        *outPtr0++ = d_in;
        *outPtr1++ = d_in;
    }
};

// Widget API Cast - triple output 'specialization'
template <typename TT_DATA, unsigned int TP_WINDOW_VSIZE, unsigned int TP_PATTERN, unsigned int TP_HEADER_BYTES>
void widget_api_cast_ref<TT_DATA, kWindowAPI, kWindowAPI, 1, TP_WINDOW_VSIZE, 3, TP_PATTERN, TP_HEADER_BYTES>::
    transferData(input_buffer<TT_DATA>& inWindow0,
                 output_buffer<TT_DATA>& outWindow0,
                 output_buffer<TT_DATA>& outWindow1,
                 output_buffer<TT_DATA>& outWindow2) {
    TT_DATA d_in;
    TT_DATA* inPtr = inWindow0.data();
    TT_DATA* outPtr0 = outWindow0.data();
    TT_DATA* outPtr1 = outWindow1.data();
    TT_DATA* outPtr2 = outWindow2.data();

    for (unsigned int i = 0; i < TP_WINDOW_VSIZE + TP_HEADER_BYTES / sizeof(TT_DATA); i++) {
        d_in = *inPtr++; // read input data
        *outPtr0++ = d_in;
        *outPtr1++ = d_in;
        *outPtr2++ = d_in;
    }
};

// Widget API Cast - 1 stream input 1 window output
template <typename TT_DATA, unsigned int TP_WINDOW_VSIZE, unsigned int TP_PATTERN, unsigned int TP_HEADER_BYTES>
void widget_api_cast_ref<TT_DATA, kStreamAPI, kWindowAPI, 1, TP_WINDOW_VSIZE, 1, TP_PATTERN, TP_HEADER_BYTES>::
    transferData(input_stream<TT_DATA>* inStream0, output_buffer<TT_DATA>& outWindow0) {
    TT_DATA d_in;
    TT_DATA* outPtr0 = outWindow0.data();

    for (unsigned int i = 0; i < TP_WINDOW_VSIZE + TP_HEADER_BYTES / sizeof(TT_DATA); i++) {
        d_in = readincr(inStream0); // read input data
        *outPtr0++ = d_in;
    }
};

// Widget API Cast - 1 stream input 1 window output int16
template <unsigned int TP_WINDOW_VSIZE, unsigned int TP_PATTERN, unsigned int TP_HEADER_BYTES>
void widget_api_cast_ref<int16, kStreamAPI, kWindowAPI, 1, TP_WINDOW_VSIZE, 1, TP_PATTERN, TP_HEADER_BYTES>::
    transferData(input_stream<int16>* inStream0, output_buffer<int16>& outWindow0) {
    cint16 d_in;
    cint16* outPtr0 = (cint16*)outWindow0.data();

    // stream reads are 32b which is 2x int16, so cint16 is used in place of TT_DATA.
    for (unsigned int i = 0; i < (TP_WINDOW_VSIZE + TP_HEADER_BYTES / sizeof(int16)) / 2;
         i++) {                                            // /2? see comment above
        d_in = readincr((input_stream<cint16>*)inStream0); // read input data
        *outPtr0++ = d_in;
    }
};

// Widget API Cast - 1 stream input 2 window output
template <typename TT_DATA, unsigned int TP_WINDOW_VSIZE, unsigned int TP_PATTERN, unsigned int TP_HEADER_BYTES>
void widget_api_cast_ref<TT_DATA, kStreamAPI, kWindowAPI, 1, TP_WINDOW_VSIZE, 2, TP_PATTERN, TP_HEADER_BYTES>::
    transferData(input_stream<TT_DATA>* inStream0,
                 output_buffer<TT_DATA>& outWindow0,
                 output_buffer<TT_DATA>& outWindow1) {
    TT_DATA d_in;
    TT_DATA* outPtr0 = outWindow0.data();
    TT_DATA* outPtr1 = outWindow1.data();

    for (unsigned int i = 0; i < TP_WINDOW_VSIZE + TP_HEADER_BYTES / sizeof(TT_DATA); i++) {
        d_in = readincr(inStream0); // read input data
        *outPtr0++ = d_in;
        *outPtr1++ = d_in;
    }
};

// Widget API Cast - 1 stream input 2 window output int16
template <unsigned int TP_WINDOW_VSIZE, unsigned int TP_PATTERN, unsigned int TP_HEADER_BYTES>
void widget_api_cast_ref<int16, kStreamAPI, kWindowAPI, 1, TP_WINDOW_VSIZE, 2, TP_PATTERN, TP_HEADER_BYTES>::
    transferData(input_stream<int16>* inStream0, output_buffer<int16>& outWindow0, output_buffer<int16>& outWindow1) {
    cint16 d_in;
    cint16* outPtr0 = (cint16*)outWindow0.data();
    cint16* outPtr1 = (cint16*)outWindow1.data();

    // stream reads are 32b which is 2x int16, so cint16 is used in place of TT_DATA.
    for (unsigned int i = 0; i < (TP_WINDOW_VSIZE + TP_HEADER_BYTES / sizeof(int16)) / 2; i++) {
        d_in = readincr((input_stream<cint16>*)inStream0); // read input data
        *outPtr0++ = d_in;
        *outPtr1++ = d_in;
    }
};

// Widget API Cast - 1 stream input 3 window output
template <typename TT_DATA, unsigned int TP_WINDOW_VSIZE, unsigned int TP_PATTERN, unsigned int TP_HEADER_BYTES>
void widget_api_cast_ref<TT_DATA, kStreamAPI, kWindowAPI, 1, TP_WINDOW_VSIZE, 3, TP_PATTERN, TP_HEADER_BYTES>::
    transferData(input_stream<TT_DATA>* inStream0,
                 output_buffer<TT_DATA>& outWindow0,
                 output_buffer<TT_DATA>& outWindow1,
                 output_buffer<TT_DATA>& outWindow2) {
    TT_DATA d_in;
    TT_DATA* outPtr0 = outWindow0.data();
    TT_DATA* outPtr1 = outWindow1.data();
    TT_DATA* outPtr2 = outWindow2.data();

    for (unsigned int i = 0; i < TP_WINDOW_VSIZE + TP_HEADER_BYTES / sizeof(TT_DATA); i++) {
        d_in = readincr(inStream0); // read input data
        *outPtr0++ = d_in;
        *outPtr1++ = d_in;
        *outPtr2++ = d_in;
    }
};

// Widget API Cast - 1 stream input 3 window output int16
template <unsigned int TP_WINDOW_VSIZE, unsigned int TP_PATTERN, unsigned int TP_HEADER_BYTES>
void widget_api_cast_ref<int16, kStreamAPI, kWindowAPI, 1, TP_WINDOW_VSIZE, 3, TP_PATTERN, TP_HEADER_BYTES>::
    transferData(input_stream<int16>* inStream0,
                 output_buffer<int16>& outWindow0,
                 output_buffer<int16>& outWindow1,
                 output_buffer<int16>& outWindow2) {
    cint16 d_in;
    cint16* outPtr0 = (cint16*)outWindow0.data();
    cint16* outPtr1 = (cint16*)outWindow1.data();
    cint16* outPtr2 = (cint16*)outWindow2.data();

    // stream reads are 32b which is 2x int16, so cint16 is used in place of TT_DATA.
    for (unsigned int i = 0; i < (TP_WINDOW_VSIZE + TP_HEADER_BYTES / sizeof(int16)) / 2; i++) {
        d_in = readincr((input_stream<cint16>*)inStream0); // read input data
        *outPtr0++ = d_in;
        *outPtr1++ = d_in;
        *outPtr2++ = d_in;
    }
};

// Widget API Cast - 1 stream input 4 window output
template <typename TT_DATA, unsigned int TP_WINDOW_VSIZE, unsigned int TP_PATTERN, unsigned int TP_HEADER_BYTES>
void widget_api_cast_ref<TT_DATA, kStreamAPI, kWindowAPI, 1, TP_WINDOW_VSIZE, 4, TP_PATTERN, TP_HEADER_BYTES>::
    transferData(input_stream<TT_DATA>* inStream0,
                 output_buffer<TT_DATA>& outWindow0,
                 output_buffer<TT_DATA>& outWindow1,
                 output_buffer<TT_DATA>& outWindow2,
                 output_buffer<TT_DATA>& outWindow3) {
    TT_DATA d_in;
    TT_DATA* outPtr0 = outWindow0.data();
    TT_DATA* outPtr1 = outWindow1.data();
    TT_DATA* outPtr2 = outWindow2.data();
    TT_DATA* outPtr3 = outWindow3.data();

    for (unsigned int i = 0; i < TP_WINDOW_VSIZE + TP_HEADER_BYTES / sizeof(TT_DATA); i++) {
        d_in = readincr(inStream0); // read input data
        *outPtr0++ = d_in;
        *outPtr1++ = d_in;
        *outPtr2++ = d_in;
        *outPtr3++ = d_in;
    }
};

// Widget API Cast - 1 stream input 4 window output int16
template <unsigned int TP_WINDOW_VSIZE, unsigned int TP_PATTERN, unsigned int TP_HEADER_BYTES>
void widget_api_cast_ref<int16, kStreamAPI, kWindowAPI, 1, TP_WINDOW_VSIZE, 4, TP_PATTERN, TP_HEADER_BYTES>::
    transferData(input_stream<int16>* inStream0,
                 output_buffer<int16>& outWindow0,
                 output_buffer<int16>& outWindow1,
                 output_buffer<int16>& outWindow2,
                 output_buffer<int16>& outWindow3) {
    cint16 d_in;
    cint16* outPtr0 = (cint16*)outWindow0.data();
    cint16* outPtr1 = (cint16*)outWindow1.data();
    cint16* outPtr2 = (cint16*)outWindow2.data();
    cint16* outPtr3 = (cint16*)outWindow3.data();

    for (unsigned int i = 0; i < (TP_WINDOW_VSIZE + TP_HEADER_BYTES / sizeof(int16)) / 2; i++) {
        d_in = readincr((input_stream<cint16>*)inStream0); // read input data
        *outPtr0++ = d_in;
        *outPtr1++ = d_in;
        *outPtr2++ = d_in;
        *outPtr3++ = d_in;
    }
};

// Dual stream in
// Widget API Cast - 2 stream input 1 window output
template <typename TT_DATA, unsigned int TP_WINDOW_VSIZE, unsigned int TP_PATTERN, unsigned int TP_HEADER_BYTES>
void widget_api_cast_ref<TT_DATA, kStreamAPI, kWindowAPI, 2, TP_WINDOW_VSIZE, 1, TP_PATTERN, TP_HEADER_BYTES>::
    transferData(input_stream<TT_DATA>* inStream0,
                 input_stream<TT_DATA>* inStream1,
                 output_buffer<TT_DATA>& outWindow0) {
    TT_DATA d_in, d_in2;
    TT_DATA* outPtr0 = outWindow0.data();
    constexpr unsigned int kWriteSize = 256 / 8; // in bytes
    constexpr unsigned int kStreamReadSize = 128 / 8;
    constexpr unsigned int kNumStreams = 2;
    constexpr unsigned int kSampleSize = sizeof(TT_DATA);
    constexpr unsigned int Lsize = TP_WINDOW_VSIZE * kSampleSize / (kWriteSize);

    // header handling
    if
        constexpr(TP_HEADER_BYTES > 0) {
            for (int i = 0; i < TP_HEADER_BYTES / sizeof(TT_DATA); i++) {
                d_in = readincr(inStream1); // read just to empty it
                d_in = readincr(inStream0); // read header from one stream only
                *outPtr0++ = d_in;
            }
        }

    if
        constexpr(TP_PATTERN == 0) {
            for (unsigned int i = 0; i < Lsize; i++) {
                for (int k = 0; k < kStreamReadSize / sizeof(TT_DATA); k++) {
                    d_in = readincr(inStream0); // read input data
                    *outPtr0++ = d_in;
                }
                for (int k = 0; k < 128 / 8 / sizeof(TT_DATA); k++) {
                    d_in = readincr(inStream1); // read input data
                    *outPtr0++ = d_in;
                }
            }
            if (TP_WINDOW_VSIZE * kSampleSize / (kWriteSize / 2) % 2 == 1) { // odd number of chunks
                for (int k = 0; k < kStreamReadSize / sizeof(TT_DATA); k++) {
                    d_in = readincr(inStream0); // read input data
                    *outPtr0++ = d_in;
                }
            }
        }
    else if
        constexpr(TP_PATTERN == kSampleIntlv) {
            for (unsigned int i = 0; i < TP_WINDOW_VSIZE / 2; i++) {
                d_in = readincr(inStream0); // read input data
                *outPtr0++ = d_in;
                d_in = readincr(inStream1); // read input data
                *outPtr0++ = d_in;
            }
        }
    else if
        constexpr(TP_PATTERN == kSplit) { // merge, in this direction.
            TT_DATA* upperHalfPtr;
            TT_DATA* lowerHalfPtr;
            lowerHalfPtr = (TT_DATA*)outWindow0.data();
            upperHalfPtr = (TT_DATA*)(lowerHalfPtr + TP_WINDOW_VSIZE / 2); // ptr arithmetic currency here is in
                                                                           // samples.
            for (unsigned int i = 0; i < TP_WINDOW_VSIZE / 2; i++) {
                d_in = readincr(inStream0);  // read input data
                d_in2 = readincr(inStream1); // read input data
                *lowerHalfPtr++ = d_in;
                *upperHalfPtr++ = d_in2;
            }
        }
};

// Widget API Cast - 2 stream input 2 window output
template <typename TT_DATA, unsigned int TP_WINDOW_VSIZE, unsigned int TP_PATTERN, unsigned int TP_HEADER_BYTES>
void widget_api_cast_ref<TT_DATA, kStreamAPI, kWindowAPI, 2, TP_WINDOW_VSIZE, 2, TP_PATTERN, TP_HEADER_BYTES>::
    transferData(input_stream<TT_DATA>* inStream0,
                 input_stream<TT_DATA>* inStream1,
                 output_buffer<TT_DATA>& outWindow0,
                 output_buffer<TT_DATA>& outWindow1) {
    TT_DATA d_in;
    TT_DATA* outPtr0 = outWindow0.data();
    TT_DATA* outPtr1 = outWindow1.data();
    constexpr unsigned int kWriteSize = 256 / 8; // in bytes
    constexpr unsigned int kStreamReadSize = 128 / 8;
    constexpr unsigned int kNumStreams = 2;
    constexpr unsigned int kSampleSize = sizeof(TT_DATA);
    constexpr unsigned int Lsize = TP_WINDOW_VSIZE * kSampleSize / (kWriteSize);

    // header handling
    if
        constexpr(TP_HEADER_BYTES > 0) {
            for (int i = 0; i < TP_HEADER_BYTES / sizeof(TT_DATA); i++) {
                d_in = readincr(inStream1); // read just to empty it
                d_in = readincr(inStream0); // read header from one stream only
                *outPtr0++ = d_in;
                *outPtr1++ = d_in;
            }
        }

    if
        constexpr(TP_PATTERN == 0) {
            for (unsigned int i = 0; i < Lsize; i++) {
                for (int k = 0; k < kStreamReadSize / sizeof(TT_DATA); k++) {
                    d_in = readincr(inStream0); // read input data
                    *outPtr0++ = d_in;
                    *outPtr1++ = d_in;
                }
                for (int k = 0; k < 128 / 8 / sizeof(TT_DATA); k++) {
                    d_in = readincr(inStream1); // read input data
                    *outPtr0++ = d_in;
                    *outPtr1++ = d_in;
                }
            }
            if (TP_WINDOW_VSIZE * kSampleSize / (kWriteSize / 2) % 2 == 1) { // odd number of chunks
                for (int k = 0; k < kStreamReadSize / sizeof(TT_DATA); k++) {
                    d_in = readincr(inStream0); // read input data
                    *outPtr0++ = d_in;
                    *outPtr1++ = d_in;
                }
            }
        }
    else if
        constexpr(TP_PATTERN == kSampleIntlv) {
            for (unsigned int i = 0; i < TP_WINDOW_VSIZE / 2; i++) {
                d_in = readincr(inStream0); // read input data
                *outPtr0++ = d_in;
                *outPtr1++ = d_in;
                d_in = readincr(inStream1); // read input data
                *outPtr0++ = d_in;
                *outPtr1++ = d_in;
            }
        }
    else if
        constexpr(TP_PATTERN == kSplit) {
            for (unsigned int i = 0; i < TP_WINDOW_VSIZE / 2; i++) {
                d_in = readincr(inStream0); // read input data
                *outPtr0++ = d_in;
                *outPtr1++ = d_in;
            }
            for (unsigned int i = 0; i < TP_WINDOW_VSIZE / 2; i++) {
                d_in = readincr(inStream1); // read input data
                *outPtr0++ = d_in;
                *outPtr1++ = d_in;
            }
        }
};

// Widget API Cast - 2 stream input 3 window output
template <typename TT_DATA, unsigned int TP_WINDOW_VSIZE, unsigned int TP_PATTERN, unsigned int TP_HEADER_BYTES>
void widget_api_cast_ref<TT_DATA, kStreamAPI, kWindowAPI, 2, TP_WINDOW_VSIZE, 3, TP_PATTERN, TP_HEADER_BYTES>::
    transferData(input_stream<TT_DATA>* inStream0,
                 input_stream<TT_DATA>* inStream1,
                 output_buffer<TT_DATA>& outWindow0,
                 output_buffer<TT_DATA>& outWindow1,
                 output_buffer<TT_DATA>& outWindow2) {
    TT_DATA d_in;
    TT_DATA* outPtr0 = outWindow0.data();
    TT_DATA* outPtr1 = outWindow1.data();
    TT_DATA* outPtr2 = outWindow2.data();
    constexpr unsigned int kWriteSize = 256 / 8; // in bytes
    constexpr unsigned int kStreamReadSize = 128 / 8;
    constexpr unsigned int kNumStreams = 2;
    constexpr unsigned int kSampleSize = sizeof(TT_DATA);
    constexpr unsigned int Lsize = TP_WINDOW_VSIZE * kSampleSize / (kWriteSize);

    if
        constexpr(TP_HEADER_BYTES > 0) {
            for (int i = 0; i < TP_HEADER_BYTES / sizeof(TT_DATA); i++) {
                d_in = readincr(inStream1); // read just to empty it
                d_in = readincr(inStream0); // read header from one stream only
                *outPtr0++ = d_in;
                *outPtr1++ = d_in;
                *outPtr2++ = d_in;
            }
        }

    if
        constexpr(TP_PATTERN == 0) {
            for (unsigned int i = 0; i < Lsize; i++) {
                for (int k = 0; k < kStreamReadSize / sizeof(TT_DATA); k++) {
                    d_in = readincr(inStream0); // read input data
                    *outPtr0++ = d_in;
                    *outPtr1++ = d_in;
                    *outPtr2++ = d_in;
                }
                for (int k = 0; k < 128 / 8 / sizeof(TT_DATA); k++) {
                    d_in = readincr(inStream1); // read input data
                    *outPtr0++ = d_in;
                    *outPtr1++ = d_in;
                    *outPtr2++ = d_in;
                }
            }
            if (TP_WINDOW_VSIZE * kSampleSize / (kWriteSize / 2) % 2 == 1) { // odd number of chunks
                for (int k = 0; k < kStreamReadSize / sizeof(TT_DATA); k++) {
                    d_in = readincr(inStream0); // read input data
                    *outPtr0++ = d_in;
                    *outPtr1++ = d_in;
                    *outPtr2++ = d_in;
                }
            }
        }
    else if
        constexpr(TP_PATTERN == kSampleIntlv) {
            for (unsigned int i = 0; i < TP_WINDOW_VSIZE / 2; i++) {
                d_in = readincr(inStream0); // read input data
                *outPtr0++ = d_in;
                *outPtr1++ = d_in;
                *outPtr2++ = d_in;
                d_in = readincr(inStream1); // read input data
                *outPtr0++ = d_in;
                *outPtr1++ = d_in;
                *outPtr2++ = d_in;
            }
        }
    else if
        constexpr(TP_PATTERN == kSplit) {
            for (unsigned int i = 0; i < TP_WINDOW_VSIZE / 2; i++) {
                d_in = readincr(inStream0); // read input data
                *outPtr0++ = d_in;
                *outPtr1++ = d_in;
                *outPtr2++ = d_in;
            }
            for (unsigned int i = 0; i < TP_WINDOW_VSIZE / 2; i++) {
                d_in = readincr(inStream1); // read input data
                *outPtr0++ = d_in;
                *outPtr1++ = d_in;
                *outPtr2++ = d_in;
            }
        }
};

// Widget API Cast - 2 stream input 4 window output
template <typename TT_DATA, unsigned int TP_WINDOW_VSIZE, unsigned int TP_PATTERN, unsigned int TP_HEADER_BYTES>
void widget_api_cast_ref<TT_DATA, kStreamAPI, kWindowAPI, 2, TP_WINDOW_VSIZE, 4, TP_PATTERN, TP_HEADER_BYTES>::
    transferData(input_stream<TT_DATA>* inStream0,
                 input_stream<TT_DATA>* inStream1,
                 output_buffer<TT_DATA>& outWindow0,
                 output_buffer<TT_DATA>& outWindow1,
                 output_buffer<TT_DATA>& outWindow2,
                 output_buffer<TT_DATA>& outWindow3) {
    TT_DATA d_in;
    TT_DATA* outPtr0 = outWindow0.data();
    TT_DATA* outPtr1 = outWindow1.data();
    TT_DATA* outPtr2 = outWindow2.data();
    TT_DATA* outPtr3 = outWindow3.data();
    constexpr unsigned int kWriteSize = 256 / 8; // in bytes
    constexpr unsigned int kStreamReadSize = 128 / 8;
    constexpr unsigned int kNumStreams = 2;
    constexpr unsigned int kSampleSize = sizeof(TT_DATA);
    constexpr unsigned int Lsize = TP_WINDOW_VSIZE * kSampleSize / (kWriteSize);

    if
        constexpr(TP_HEADER_BYTES > 0) {
            for (int i = 0; i < TP_HEADER_BYTES / sizeof(TT_DATA); i++) {
                d_in = readincr(inStream1); // read just to empty it
                d_in = readincr(inStream0); // read header from one stream only
                *outPtr0++ = d_in;
                *outPtr1++ = d_in;
                *outPtr2++ = d_in;
                *outPtr3++ = d_in;
            }
        }

    if
        constexpr(TP_PATTERN == 0) {
            for (unsigned int i = 0; i < Lsize; i++) {
                for (int k = 0; k < kStreamReadSize / sizeof(TT_DATA); k++) {
                    d_in = readincr(inStream0); // read input data
                    *outPtr0++ = d_in;
                    *outPtr1++ = d_in;
                    *outPtr2++ = d_in;
                    *outPtr3++ = d_in;
                }
                for (int k = 0; k < 128 / 8 / sizeof(TT_DATA); k++) {
                    d_in = readincr(inStream1); // read input data
                    *outPtr0++ = d_in;
                    *outPtr1++ = d_in;
                    *outPtr2++ = d_in;
                    *outPtr3++ = d_in;
                }
            }
            if (TP_WINDOW_VSIZE * kSampleSize / (kWriteSize / 2) % 2 == 1) { // odd number of chunks
                for (int k = 0; k < kStreamReadSize / sizeof(TT_DATA); k++) {
                    d_in = readincr(inStream0); // read input data
                    *outPtr0++ = d_in;
                    *outPtr1++ = d_in;
                    *outPtr2++ = d_in;
                    *outPtr3++ = d_in;
                }
            }
        }
    else if
        constexpr(TP_PATTERN == kSampleIntlv) {
            for (unsigned int i = 0; i < TP_WINDOW_VSIZE / 2; i++) {
                d_in = readincr(inStream0); // read input data
                *outPtr0++ = d_in;
                *outPtr1++ = d_in;
                *outPtr2++ = d_in;
                *outPtr3++ = d_in;
                d_in = readincr(inStream1); // read input data
                *outPtr0++ = d_in;
                *outPtr1++ = d_in;
                *outPtr2++ = d_in;
                *outPtr3++ = d_in;
            }
        }
    else if
        constexpr(TP_PATTERN == kSplit) {
            for (unsigned int i = 0; i < TP_WINDOW_VSIZE / 2; i++) {
                d_in = readincr(inStream0); // read input data
                *outPtr0++ = d_in;
                *outPtr1++ = d_in;
                *outPtr2++ = d_in;
                *outPtr3++ = d_in;
            }
            for (unsigned int i = 0; i < TP_WINDOW_VSIZE / 2; i++) {
                d_in = readincr(inStream1); // read input data
                *outPtr0++ = d_in;
                *outPtr1++ = d_in;
                *outPtr2++ = d_in;
                *outPtr3++ = d_in;
            }
        }
};

// Widget API Cast - window to stream, 1 to 1
template <typename TT_DATA, unsigned int TP_WINDOW_VSIZE, unsigned int TP_PATTERN, unsigned int TP_HEADER_BYTES>
void widget_api_cast_ref<TT_DATA, kWindowAPI, kStreamAPI, 1, TP_WINDOW_VSIZE, 1, TP_PATTERN, TP_HEADER_BYTES>::
    transferData(input_buffer<TT_DATA>& inWindow0, output_stream<TT_DATA>* outStream0) {
    TT_DATA d_in;
    TT_DATA* inPtr = inWindow0.data();

    for (unsigned int i = 0; i < TP_WINDOW_VSIZE + TP_HEADER_BYTES / sizeof(TT_DATA); i++) {
        d_in = *inPtr++; // read input data
        writeincr(outStream0, d_in);
    }
};

// Widget API Cast - window to stream, 1 to 1 int16
template <unsigned int TP_WINDOW_VSIZE, unsigned int TP_PATTERN, unsigned int TP_HEADER_BYTES>
void widget_api_cast_ref<int16, kWindowAPI, kStreamAPI, 1, TP_WINDOW_VSIZE, 1, TP_PATTERN, TP_HEADER_BYTES>::
    transferData(input_buffer<int16>& inWindow0, output_stream<int16>* outStream0) {
    cint16 d_in;
    cint16* inPtr = (cint16*)inWindow0.data();

    for (unsigned int i = 0; i < (TP_WINDOW_VSIZE + TP_HEADER_BYTES / sizeof(int16)) / 2; i++) {
        d_in = *inPtr++; // read input data
        writeincr((output_stream<cint16>*)outStream0, d_in);
    }
};

// Widget API Cast - window to stream, 1 to 2
template <typename TT_DATA, unsigned int TP_WINDOW_VSIZE, unsigned int TP_PATTERN, unsigned int TP_HEADER_BYTES>
void widget_api_cast_ref<TT_DATA, kWindowAPI, kStreamAPI, 1, TP_WINDOW_VSIZE, 2, TP_PATTERN, TP_HEADER_BYTES>::
    transferData(input_buffer<TT_DATA>& inWindow0,
                 output_stream<TT_DATA>* outStream0,
                 output_stream<TT_DATA>* outStream1) {
    TT_DATA d_in;
    TT_DATA* inPtr = inWindow0.data();

    constexpr unsigned int kSamplesIn128b = 16 / sizeof(TT_DATA);
    constexpr unsigned int Lsize = (TP_WINDOW_VSIZE) / (2 * kSamplesIn128b);
    // header handling - clone to both outputs
    if
        constexpr(TP_HEADER_BYTES > 0) {
            for (int i = 0; i < TP_HEADER_BYTES / sizeof(TT_DATA); i++) {
                d_in = *inPtr++; // read input data samplewise
                writeincr(outStream0, d_in);
                writeincr(outStream1, d_in);
            }
        }

    if
        constexpr(TP_PATTERN == 0) {
            for (unsigned int i = 0; i < Lsize; i++) {
                for (int i = 0; i < kSamplesIn128b; i++) {
                    d_in = *inPtr++; // read input data samplewise
                    writeincr(outStream0, d_in);
                }
                for (int i = 0; i < kSamplesIn128b; i++) {
                    d_in = *inPtr++; // read input data
                    writeincr(outStream1, d_in);
                }
            }
            if (TP_WINDOW_VSIZE / kSamplesIn128b % 2 == 1) {
                d_in = *inPtr++; // read input data samplewise
                writeincr(outStream0, d_in);
            }
        }
    else if
        constexpr(TP_PATTERN == kSampleIntlv) {
            for (int i = 0; i < TP_WINDOW_VSIZE / 2; i++) {
                d_in = *inPtr++; // read input data samplewise
                writeincr(outStream0, d_in);
                d_in = *inPtr++; // read input data samplewise
                writeincr(outStream1, d_in);
            }
        }
    else if
        constexpr(TP_PATTERN == kSplit) {
            for (int i = 0; i < TP_WINDOW_VSIZE / 2; i++) {
                d_in = *inPtr++; // read input data samplewise
                writeincr(outStream0, d_in);
            }
            for (int i = 0; i < TP_WINDOW_VSIZE / 2; i++) {
                d_in = *inPtr++; // read input data samplewise
                writeincr(outStream1, d_in);
            }
        }
};

// AIE2 functions Casc/Stream or Stream/Casc to/from iobuffer

#ifdef __SUPPORTS_ACC64__
// Widget API Cast - Casc/Stream input 1 window output
template <typename TT_DATA, unsigned int TP_WINDOW_VSIZE, unsigned int TP_PATTERN, unsigned int TP_HEADER_BYTES>
void widget_api_cast_ref<TT_DATA, kCascStreamAPI, kWindowAPI, 2, TP_WINDOW_VSIZE, 1, TP_PATTERN, TP_HEADER_BYTES>::
    transferData(input_stream<cacc64>* inStream0,
                 input_stream<TT_DATA>* inStream1,
                 output_buffer<TT_DATA>& outWindow0) {
    TT_DATA d_in, d_in2;
    TT_DATA* outPtr0 = outWindow0.data();
    constexpr unsigned int kWriteSize = 256 / 8; // in bytes
    constexpr unsigned int kStreamReadSize = 128 / 8;
    constexpr unsigned int kNumStreams = 2;
    constexpr unsigned int kSampleSize = sizeof(TT_DATA);
    constexpr unsigned int Lsize = TP_WINDOW_VSIZE * kSampleSize / (kWriteSize);
    constexpr unsigned int kCascadeWidth = 4; // samples.

    using accVect_t = ::aie::accum<cacc64, kCascadeWidth>;
    //  using accVect_t   = ::aie::detail::accum<::aie::detail::AccumClass::CInt,
    //                                         //fnAccClass<TT_DATA>(), //int, cint, FP or CFP
    //                                         64, //acc sample width
    //                                         kCascadeWidth>; //both cint16 and cint32 use 4 * cacc64kVectSize>;
    using dataVect_t = ::aie::vector<TT_DATA, kCascadeWidth>;
    accVect_t cascAcc;
    dataVect_t cascData;

    // header handling
    if
        constexpr(TP_HEADER_BYTES > 0) {
            for (int i = 0; i < TP_HEADER_BYTES / sizeof(TT_DATA); i++) {
                if (i % kCascadeWidth == 0)
                    cascAcc = readincr_v4(inStream0); // read and ignore - the header can come from the stream
                d_in = readincr(inStream1);           // read header from one stream only
                *outPtr0++ = d_in;
            }
        }

    if
        constexpr(TP_PATTERN == 0) {
            // not supported (not required for FFT)
        }
    else if
        constexpr(TP_PATTERN == kSampleIntlv) {
            for (int v = 0; v < (TP_WINDOW_VSIZE / 2) / kCascadeWidth; v++) {
                cascAcc = readincr_v4(inStream0);
                cascData = cascAcc.template to_vector<TT_DATA>(0);
                for (int i = 0; i < kCascadeWidth; i++) {
                    d_in = cascData[i]; // read input data
                    *outPtr0++ = d_in;
                    d_in = readincr(inStream1); // read input data
                    *outPtr0++ = d_in;
                }
            }
        }
    else if
        constexpr(TP_PATTERN == kSplit) { // merge, in this direction.
            // not supported (not required for FFT)
        }
};
#endif //__SUPPORTS_ACC64__

#ifdef __SUPPORTS_ACC64__
// Widget API Cast - Stream/Casc input 1 window output
template <typename TT_DATA, unsigned int TP_WINDOW_VSIZE, unsigned int TP_PATTERN, unsigned int TP_HEADER_BYTES>
void widget_api_cast_ref<TT_DATA, kStreamCascAPI, kWindowAPI, 2, TP_WINDOW_VSIZE, 1, TP_PATTERN, TP_HEADER_BYTES>::
    transferData(input_stream<TT_DATA>* inStream0,
                 input_stream<cacc64>* inStream1,
                 output_buffer<TT_DATA>& outWindow0) {
    TT_DATA d_in, d_in2;
    TT_DATA* outPtr0 = outWindow0.data();
    constexpr unsigned int kWriteSize = 256 / 8; // in bytes
    constexpr unsigned int kStreamReadSize = 128 / 8;
    constexpr unsigned int kNumStreams = 2;
    constexpr unsigned int kSampleSize = sizeof(TT_DATA);
    constexpr unsigned int Lsize = TP_WINDOW_VSIZE * kSampleSize / (kWriteSize);
    constexpr unsigned int kCascadeWidth = 4; // samples.

    using accVect_t = ::aie::accum<cacc64, kCascadeWidth>;
    //  using accVect_t   = ::aie::detail::accum<::aie::detail::AccumClass::CInt,
    //                                         //fnAccClass<TT_DATA>(), //int, cint, FP or CFP
    //                                         64, //acc sample width
    //                                         kCascadeWidth>; //both cint16 and cint32 use 4 * cacc64kVectSize>;
    using dataVect_t = ::aie::vector<TT_DATA, kCascadeWidth>;
    accVect_t cascAcc;
    dataVect_t cascData;

    // header handling
    if
        constexpr(TP_HEADER_BYTES > 0) {
            for (int i = 0; i < TP_HEADER_BYTES / sizeof(TT_DATA); i++) {
                if (i % kCascadeWidth == 0)
                    cascAcc = readincr_v4(inStream1); // read and ignore - the header can come from the stream
                d_in = readincr(inStream0);           // read header from one stream only
                *outPtr0++ = d_in;
            }
        }

    if
        constexpr(TP_PATTERN == kDefaultIntlv) { // 128b granularity interleave
            // not supported (not required for FFT)
        }
    else if
        constexpr(TP_PATTERN == kSampleIntlv) {
            for (int v = 0; v < (TP_WINDOW_VSIZE / 2) / kCascadeWidth; v++) {
                cascAcc = readincr_v4(inStream1); // read and ignore - the header can come from the stream
                cascData = cascAcc.template to_vector<TT_DATA>(0);
                for (int i = 0; i < kCascadeWidth; i++) {
                    d_in = readincr(inStream0);
                    *outPtr0++ = d_in;
                    d_in = cascData[i];
                    *outPtr0++ = d_in;
                }
            }
        }
    else if
        constexpr(TP_PATTERN == kSplit) { // merge, in this direction.
            // not supported (not required for FFT)
        }
};
#endif //__SUPPORTS_ACC64__

#ifdef __SUPPORTS_ACC64__
// Widget API Cast - 1 window input Casc/Stream output
template <typename TT_DATA, unsigned int TP_WINDOW_VSIZE, unsigned int TP_PATTERN, unsigned int TP_HEADER_BYTES>
void widget_api_cast_ref<TT_DATA, kWindowAPI, kCascStreamAPI, 1, TP_WINDOW_VSIZE, 2, TP_PATTERN, TP_HEADER_BYTES>::
    transferData(input_buffer<TT_DATA>& inWindow0,
                 output_stream<cacc64>* outStream0,
                 output_stream<TT_DATA>* outStream1) {
    constexpr unsigned int kCascadeWidth = 4;                        // samples.
    constexpr unsigned int kStreamWidth = 128 / 8 / sizeof(TT_DATA); // samples.
    using accVect_t = ::aie::accum<cacc64, kCascadeWidth>;
    //  using accVect_t   = ::aie::detail::accum<::aie::detail::AccumClass::CInt,
    //                                         //fnAccClass<TT_DATA>(), //int, cint, FP or CFP
    //                                         64, //acc sample width
    //                                         kCascadeWidth>; //both cint16 and cint32 use 4 * cacc64kVectSize>;
    using dataVect_t = ::aie::vector<TT_DATA, kCascadeWidth>;
    using streamVect_t = ::aie::vector<TT_DATA, kStreamWidth>;

    accVect_t cascAcc;
    dataVect_t cascData;
    streamVect_t streamData;
    TT_DATA* inPtr = inWindow0.data();
    TT_DATA temp;

    constexpr unsigned int kSamplesIn128b = 16 / sizeof(TT_DATA);
    constexpr unsigned int Lsize = (TP_WINDOW_VSIZE) / (2 * kSamplesIn128b);
    // header handling - clone to both outputs
    if
        constexpr(TP_HEADER_BYTES > 0) {
            for (int i = 0; i < TP_HEADER_BYTES / sizeof(TT_DATA); i++) {
                temp = *inPtr++;
                streamData[i % kStreamWidth] = temp; // read input data samplewise
                cascData[i % kCascadeWidth] = temp;
                if (i % kCascadeWidth == kCascadeWidth - 1) {
                    cascAcc = ::aie::from_vector<cacc64>(cascData);
                    writeincr(outStream0, cascAcc);
                }
                if (i % kStreamWidth == kStreamWidth - 1) {
                    writeincr(outStream1, streamData);
                }
            }
        }

    if
        constexpr(TP_PATTERN == 0) {
            // not supported - not needed by FFT
        }
    else if
        constexpr(TP_PATTERN == kSampleIntlv) {
            for (int i = 0; i < TP_WINDOW_VSIZE; i++) {
                if (i % 2 == 0) { // deinterlace
                    cascData[(i / 2) % kCascadeWidth] = *inPtr++;
                    if ((i / 2) % kCascadeWidth == kCascadeWidth - 1) {
                        cascAcc = ::aie::from_vector<cacc64>(cascData);
                        writeincr(outStream0, cascAcc);
                    }
                } else {
                    streamData[(i / 2) % kStreamWidth] = *inPtr++; // read input data samplewise
                    if ((i / 2) % kStreamWidth == kStreamWidth - 1) {
                        writeincr(outStream1, streamData);
                    }
                }
            }
        }
    else if
        constexpr(TP_PATTERN == kSplit) {
            // Not supported - not needed by FFT
        }
};
#endif //__SUPPORTS_ACC64__

#ifdef __SUPPORTS_ACC64__
// Widget API Cast - 1 window input Stream/Casc output
template <typename TT_DATA, unsigned int TP_WINDOW_VSIZE, unsigned int TP_PATTERN, unsigned int TP_HEADER_BYTES>
void widget_api_cast_ref<TT_DATA, kWindowAPI, kStreamCascAPI, 1, TP_WINDOW_VSIZE, 2, TP_PATTERN, TP_HEADER_BYTES>::
    transferData(input_buffer<TT_DATA>& inWindow0,
                 output_stream<TT_DATA>* outStream0,
                 output_stream<cacc64>* outStream1) {
    constexpr unsigned int kCascadeWidth = 4;                        // samples.
    constexpr unsigned int kStreamWidth = 128 / 8 / sizeof(TT_DATA); // samples.
    using accVect_t = ::aie::accum<cacc64, kCascadeWidth>;
    //  using accVect_t   = ::aie::detail::accum<::aie::detail::AccumClass::CInt,
    //                                         //fnAccClass<TT_DATA>(), //int, cint, FP or CFP
    //                                         64, //acc sample width
    //                                         kCascadeWidth>; //both cint16 and cint32 use 4 * cacc64kVectSize>;
    using dataVect_t = ::aie::vector<TT_DATA, kCascadeWidth>;
    using streamVect_t = ::aie::vector<TT_DATA, kStreamWidth>;

    accVect_t cascAcc;
    dataVect_t cascData;
    streamVect_t streamData;
    TT_DATA* inPtr = inWindow0.data();
    TT_DATA temp;

    constexpr unsigned int kSamplesIn128b = 16 / sizeof(TT_DATA);
    constexpr unsigned int Lsize = (TP_WINDOW_VSIZE) / (2 * kSamplesIn128b);
    // header handling - clone to both outputs
    if
        constexpr(TP_HEADER_BYTES > 0) {
            for (int i = 0; i < TP_HEADER_BYTES / sizeof(TT_DATA); i++) {
                temp = *inPtr++;
                streamData[i % kStreamWidth] = temp; // read input data samplewise
                cascData[i % kCascadeWidth] = temp;
                if (i % kStreamWidth == kStreamWidth - 1) {
                    writeincr(outStream0, streamData);
                }
                if (i % kCascadeWidth == kCascadeWidth - 1) {
                    cascAcc = ::aie::from_vector<cacc64>(cascData);
                    writeincr(outStream1, cascAcc);
                }
            }
        }

    if
        constexpr(TP_PATTERN == 0) {
            // not supported - not needed by FFT
        }
    else if
        constexpr(TP_PATTERN == kSampleIntlv) {
            for (int i = 0; i < TP_WINDOW_VSIZE; i++) {
                if (i % 2 == 0) {                                  // deinterlace
                    streamData[(i / 2) % kStreamWidth] = *inPtr++; // read input data samplewise
                    if ((i / 2) % kStreamWidth == kStreamWidth - 1) {
                        writeincr(outStream0, streamData);
                    }
                } else {
                    cascData[(i / 2) % kCascadeWidth] = *inPtr++;
                    if ((i / 2) % kCascadeWidth == kCascadeWidth - 1) {
                        cascAcc = ::aie::from_vector<cacc64>(cascData);
                        writeincr(outStream1, cascAcc);
                    }
                }
            }
        }
    else if
        constexpr(TP_PATTERN == kSplit) {
            // Not supported - not needed by FFT
        }
};
#endif //__SUPPORTS_ACC64__
}
}
}
}
}
