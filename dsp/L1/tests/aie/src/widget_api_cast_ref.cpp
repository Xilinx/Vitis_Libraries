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
Widget API Cast reference model
*/

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
                         TP_HEADER_BYTES>::transferData(input_window<TT_DATA>* inWindow0,
                                                        output_window<TT_DATA>* outWindow0) {
    TT_DATA d_in;

    for (unsigned int i = 0; i < TP_WINDOW_VSIZE + TP_HEADER_BYTES / sizeof(TT_DATA); i++) {
        d_in = window_readincr(inWindow0); // read input data
        window_writeincr((output_window<TT_DATA>*)outWindow0, d_in);
    }
};

// Widget API Cast - dual cloned output 'specialization'
template <typename TT_DATA, unsigned int TP_WINDOW_VSIZE, unsigned int TP_PATTERN, unsigned int TP_HEADER_BYTES>
void widget_api_cast_ref<TT_DATA, kWindowAPI, kWindowAPI, 1, TP_WINDOW_VSIZE, 2, TP_PATTERN, TP_HEADER_BYTES>::
    transferData(input_window<TT_DATA>* inWindow0,
                 output_window<TT_DATA>* outWindow0,
                 output_window<TT_DATA>* outWindow1) {
    TT_DATA d_in;

    for (unsigned int i = 0; i < TP_WINDOW_VSIZE + TP_HEADER_BYTES / sizeof(TT_DATA); i++) {
        d_in = window_readincr(inWindow0); // read input data
        window_writeincr((output_window<TT_DATA>*)outWindow0, d_in);
        window_writeincr((output_window<TT_DATA>*)outWindow1, d_in);
    }
};

// Widget API Cast - triple output 'specialization'
template <typename TT_DATA, unsigned int TP_WINDOW_VSIZE, unsigned int TP_PATTERN, unsigned int TP_HEADER_BYTES>
void widget_api_cast_ref<TT_DATA, kWindowAPI, kWindowAPI, 1, TP_WINDOW_VSIZE, 3, TP_PATTERN, TP_HEADER_BYTES>::
    transferData(input_window<TT_DATA>* inWindow0,
                 output_window<TT_DATA>* outWindow0,
                 output_window<TT_DATA>* outWindow1,
                 output_window<TT_DATA>* outWindow2) {
    TT_DATA d_in;

    for (unsigned int i = 0; i < TP_WINDOW_VSIZE + TP_HEADER_BYTES / sizeof(TT_DATA); i++) {
        d_in = window_readincr(inWindow0); // read input data
        window_writeincr((output_window<TT_DATA>*)outWindow0, d_in);
        window_writeincr((output_window<TT_DATA>*)outWindow1, d_in);
        window_writeincr((output_window<TT_DATA>*)outWindow2, d_in);
    }
};

// Widget API Cast - 1 stream input 1 window output
template <typename TT_DATA, unsigned int TP_WINDOW_VSIZE, unsigned int TP_PATTERN, unsigned int TP_HEADER_BYTES>
void widget_api_cast_ref<TT_DATA, kStreamAPI, kWindowAPI, 1, TP_WINDOW_VSIZE, 1, TP_PATTERN, TP_HEADER_BYTES>::
    transferData(input_stream<TT_DATA>* inStream0, output_window<TT_DATA>* outWindow0) {
    TT_DATA d_in;

    for (unsigned int i = 0; i < TP_WINDOW_VSIZE + TP_HEADER_BYTES / sizeof(TT_DATA); i++) {
        d_in = readincr(inStream0); // read input data
        window_writeincr((output_window<TT_DATA>*)outWindow0, d_in);
    }
};

// Widget API Cast - 1 stream input 2 window output
template <typename TT_DATA, unsigned int TP_WINDOW_VSIZE, unsigned int TP_PATTERN, unsigned int TP_HEADER_BYTES>
void widget_api_cast_ref<TT_DATA, kStreamAPI, kWindowAPI, 1, TP_WINDOW_VSIZE, 2, TP_PATTERN, TP_HEADER_BYTES>::
    transferData(input_stream<TT_DATA>* inStream0,
                 output_window<TT_DATA>* outWindow0,
                 output_window<TT_DATA>* outWindow1) {
    TT_DATA d_in;

    for (unsigned int i = 0; i < TP_WINDOW_VSIZE + TP_HEADER_BYTES / sizeof(TT_DATA); i++) {
        d_in = readincr(inStream0); // read input data
        window_writeincr((output_window<TT_DATA>*)outWindow0, d_in);
        window_writeincr((output_window<TT_DATA>*)outWindow1, d_in);
    }
};

// Widget API Cast - 1 stream input 3 window output
template <typename TT_DATA, unsigned int TP_WINDOW_VSIZE, unsigned int TP_PATTERN, unsigned int TP_HEADER_BYTES>
void widget_api_cast_ref<TT_DATA, kStreamAPI, kWindowAPI, 1, TP_WINDOW_VSIZE, 3, TP_PATTERN, TP_HEADER_BYTES>::
    transferData(input_stream<TT_DATA>* inStream0,
                 output_window<TT_DATA>* outWindow0,
                 output_window<TT_DATA>* outWindow1,
                 output_window<TT_DATA>* outWindow2) {
    TT_DATA d_in;

    for (unsigned int i = 0; i < TP_WINDOW_VSIZE + TP_HEADER_BYTES / sizeof(TT_DATA); i++) {
        d_in = readincr(inStream0); // read input data
        window_writeincr((output_window<TT_DATA>*)outWindow0, d_in);
        window_writeincr((output_window<TT_DATA>*)outWindow1, d_in);
        window_writeincr((output_window<TT_DATA>*)outWindow2, d_in);
    }
};

// Widget API Cast - 1 stream input 4 window output
template <typename TT_DATA, unsigned int TP_WINDOW_VSIZE, unsigned int TP_PATTERN, unsigned int TP_HEADER_BYTES>
void widget_api_cast_ref<TT_DATA, kStreamAPI, kWindowAPI, 1, TP_WINDOW_VSIZE, 4, TP_PATTERN, TP_HEADER_BYTES>::
    transferData(input_stream<TT_DATA>* inStream0,
                 output_window<TT_DATA>* outWindow0,
                 output_window<TT_DATA>* outWindow1,
                 output_window<TT_DATA>* outWindow2,
                 output_window<TT_DATA>* outWindow3) {
    TT_DATA d_in;

    for (unsigned int i = 0; i < TP_WINDOW_VSIZE + TP_HEADER_BYTES / sizeof(TT_DATA); i++) {
        d_in = readincr(inStream0); // read input data
        window_writeincr((output_window<TT_DATA>*)outWindow0, d_in);
        window_writeincr((output_window<TT_DATA>*)outWindow1, d_in);
        window_writeincr((output_window<TT_DATA>*)outWindow2, d_in);
        window_writeincr((output_window<TT_DATA>*)outWindow3, d_in);
    }
};

// Dual stream in
// Widget API Cast - 2 stream input 1 window output
template <typename TT_DATA, unsigned int TP_WINDOW_VSIZE, unsigned int TP_PATTERN, unsigned int TP_HEADER_BYTES>
void widget_api_cast_ref<TT_DATA, kStreamAPI, kWindowAPI, 2, TP_WINDOW_VSIZE, 1, TP_PATTERN, TP_HEADER_BYTES>::
    transferData(input_stream<TT_DATA>* inStream0,
                 input_stream<TT_DATA>* inStream1,
                 output_window<TT_DATA>* outWindow0) {
    TT_DATA d_in, d_in2;
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
                window_writeincr((output_window<TT_DATA>*)outWindow0, d_in);
            }
        }

    if
        constexpr(TP_PATTERN == 0) {
            for (unsigned int i = 0; i < Lsize; i++) {
                for (int k = 0; k < kStreamReadSize / sizeof(TT_DATA); k++) {
                    d_in = readincr(inStream0); // read input data
                    window_writeincr((output_window<TT_DATA>*)outWindow0, d_in);
                }
                for (int k = 0; k < 128 / 8 / sizeof(TT_DATA); k++) {
                    d_in = readincr(inStream1); // read input data
                    window_writeincr((output_window<TT_DATA>*)outWindow0, d_in);
                }
            }
            if (TP_WINDOW_VSIZE * kSampleSize / (kWriteSize / 2) % 2 == 1) { // odd number of chunks
                for (int k = 0; k < kStreamReadSize / sizeof(TT_DATA); k++) {
                    d_in = readincr(inStream0); // read input data
                    window_writeincr((output_window<TT_DATA>*)outWindow0, d_in);
                }
            }
        }
    else if
        constexpr(TP_PATTERN == kSampleIntlv) {
            for (unsigned int i = 0; i < TP_WINDOW_VSIZE / 2; i++) {
                d_in = readincr(inStream0); // read input data
                window_writeincr((output_window<TT_DATA>*)outWindow0, d_in);
                d_in = readincr(inStream1); // read input data
                window_writeincr((output_window<TT_DATA>*)outWindow0, d_in);
            }
        }
    else if
        constexpr(TP_PATTERN == kSplit) { // merge, in this direction.
            TT_DATA* upperHalfPtr;
            TT_DATA* lowerHalfPtr;
            lowerHalfPtr = (TT_DATA*)outWindow0->ptr;
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
                 output_window<TT_DATA>* outWindow0,
                 output_window<TT_DATA>* outWindow1) {
    TT_DATA d_in;
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
                window_writeincr((output_window<TT_DATA>*)outWindow0, d_in);
                window_writeincr((output_window<TT_DATA>*)outWindow1, d_in);
            }
        }

    if
        constexpr(TP_PATTERN == 0) {
            for (unsigned int i = 0; i < Lsize; i++) {
                for (int k = 0; k < kStreamReadSize / sizeof(TT_DATA); k++) {
                    d_in = readincr(inStream0); // read input data
                    window_writeincr((output_window<TT_DATA>*)outWindow0, d_in);
                    window_writeincr((output_window<TT_DATA>*)outWindow1, d_in);
                }
                for (int k = 0; k < 128 / 8 / sizeof(TT_DATA); k++) {
                    d_in = readincr(inStream1); // read input data
                    window_writeincr((output_window<TT_DATA>*)outWindow0, d_in);
                    window_writeincr((output_window<TT_DATA>*)outWindow1, d_in);
                }
            }
            if (TP_WINDOW_VSIZE * kSampleSize / (kWriteSize / 2) % 2 == 1) { // odd number of chunks
                for (int k = 0; k < kStreamReadSize / sizeof(TT_DATA); k++) {
                    d_in = readincr(inStream0); // read input data
                    window_writeincr((output_window<TT_DATA>*)outWindow0, d_in);
                    window_writeincr((output_window<TT_DATA>*)outWindow1, d_in);
                }
            }
        }
    else if
        constexpr(TP_PATTERN == kSampleIntlv) {
            for (unsigned int i = 0; i < TP_WINDOW_VSIZE / 2; i++) {
                d_in = readincr(inStream0); // read input data
                window_writeincr((output_window<TT_DATA>*)outWindow0, d_in);
                window_writeincr((output_window<TT_DATA>*)outWindow1, d_in);
                d_in = readincr(inStream1); // read input data
                window_writeincr((output_window<TT_DATA>*)outWindow0, d_in);
                window_writeincr((output_window<TT_DATA>*)outWindow1, d_in);
            }
        }
    else if
        constexpr(TP_PATTERN == kSplit) {
            for (unsigned int i = 0; i < TP_WINDOW_VSIZE / 2; i++) {
                d_in = readincr(inStream0); // read input data
                window_writeincr((output_window<TT_DATA>*)outWindow0, d_in);
                window_writeincr((output_window<TT_DATA>*)outWindow1, d_in);
            }
            for (unsigned int i = 0; i < TP_WINDOW_VSIZE / 2; i++) {
                d_in = readincr(inStream1); // read input data
                window_writeincr((output_window<TT_DATA>*)outWindow0, d_in);
                window_writeincr((output_window<TT_DATA>*)outWindow1, d_in);
            }
        }
};

// Widget API Cast - 2 stream input 3 window output
template <typename TT_DATA, unsigned int TP_WINDOW_VSIZE, unsigned int TP_PATTERN, unsigned int TP_HEADER_BYTES>
void widget_api_cast_ref<TT_DATA, kStreamAPI, kWindowAPI, 2, TP_WINDOW_VSIZE, 3, TP_PATTERN, TP_HEADER_BYTES>::
    transferData(input_stream<TT_DATA>* inStream0,
                 input_stream<TT_DATA>* inStream1,
                 output_window<TT_DATA>* outWindow0,
                 output_window<TT_DATA>* outWindow1,
                 output_window<TT_DATA>* outWindow2) {
    TT_DATA d_in;
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
                window_writeincr((output_window<TT_DATA>*)outWindow0, d_in);
                window_writeincr((output_window<TT_DATA>*)outWindow1, d_in);
                window_writeincr((output_window<TT_DATA>*)outWindow2, d_in);
            }
        }

    if
        constexpr(TP_PATTERN == 0) {
            for (unsigned int i = 0; i < Lsize; i++) {
                for (int k = 0; k < kStreamReadSize / sizeof(TT_DATA); k++) {
                    d_in = readincr(inStream0); // read input data
                    window_writeincr((output_window<TT_DATA>*)outWindow0, d_in);
                    window_writeincr((output_window<TT_DATA>*)outWindow1, d_in);
                    window_writeincr((output_window<TT_DATA>*)outWindow2, d_in);
                }
                for (int k = 0; k < 128 / 8 / sizeof(TT_DATA); k++) {
                    d_in = readincr(inStream1); // read input data
                    window_writeincr((output_window<TT_DATA>*)outWindow0, d_in);
                    window_writeincr((output_window<TT_DATA>*)outWindow1, d_in);
                    window_writeincr((output_window<TT_DATA>*)outWindow2, d_in);
                }
            }
            if (TP_WINDOW_VSIZE * kSampleSize / (kWriteSize / 2) % 2 == 1) { // odd number of chunks
                for (int k = 0; k < kStreamReadSize / sizeof(TT_DATA); k++) {
                    d_in = readincr(inStream0); // read input data
                    window_writeincr((output_window<TT_DATA>*)outWindow0, d_in);
                    window_writeincr((output_window<TT_DATA>*)outWindow1, d_in);
                    window_writeincr((output_window<TT_DATA>*)outWindow2, d_in);
                }
            }
        }
    else if
        constexpr(TP_PATTERN == kSampleIntlv) {
            for (unsigned int i = 0; i < TP_WINDOW_VSIZE / 2; i++) {
                d_in = readincr(inStream0); // read input data
                window_writeincr((output_window<TT_DATA>*)outWindow0, d_in);
                window_writeincr((output_window<TT_DATA>*)outWindow1, d_in);
                window_writeincr((output_window<TT_DATA>*)outWindow2, d_in);
                d_in = readincr(inStream1); // read input data
                window_writeincr((output_window<TT_DATA>*)outWindow0, d_in);
                window_writeincr((output_window<TT_DATA>*)outWindow1, d_in);
                window_writeincr((output_window<TT_DATA>*)outWindow2, d_in);
            }
        }
    else if
        constexpr(TP_PATTERN == kSplit) {
            for (unsigned int i = 0; i < TP_WINDOW_VSIZE / 2; i++) {
                d_in = readincr(inStream0); // read input data
                window_writeincr((output_window<TT_DATA>*)outWindow0, d_in);
                window_writeincr((output_window<TT_DATA>*)outWindow1, d_in);
                window_writeincr((output_window<TT_DATA>*)outWindow2, d_in);
            }
            for (unsigned int i = 0; i < TP_WINDOW_VSIZE / 2; i++) {
                d_in = readincr(inStream1); // read input data
                window_writeincr((output_window<TT_DATA>*)outWindow0, d_in);
                window_writeincr((output_window<TT_DATA>*)outWindow1, d_in);
                window_writeincr((output_window<TT_DATA>*)outWindow2, d_in);
            }
        }
};

// Widget API Cast - 2 stream input 4 window output
template <typename TT_DATA, unsigned int TP_WINDOW_VSIZE, unsigned int TP_PATTERN, unsigned int TP_HEADER_BYTES>
void widget_api_cast_ref<TT_DATA, kStreamAPI, kWindowAPI, 2, TP_WINDOW_VSIZE, 4, TP_PATTERN, TP_HEADER_BYTES>::
    transferData(input_stream<TT_DATA>* inStream0,
                 input_stream<TT_DATA>* inStream1,
                 output_window<TT_DATA>* outWindow0,
                 output_window<TT_DATA>* outWindow1,
                 output_window<TT_DATA>* outWindow2,
                 output_window<TT_DATA>* outWindow3) {
    TT_DATA d_in;
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
                window_writeincr((output_window<TT_DATA>*)outWindow0, d_in);
                window_writeincr((output_window<TT_DATA>*)outWindow1, d_in);
                window_writeincr((output_window<TT_DATA>*)outWindow2, d_in);
                window_writeincr((output_window<TT_DATA>*)outWindow3, d_in);
            }
        }

    if
        constexpr(TP_PATTERN == 0) {
            for (unsigned int i = 0; i < Lsize; i++) {
                for (int k = 0; k < kStreamReadSize / sizeof(TT_DATA); k++) {
                    d_in = readincr(inStream0); // read input data
                    window_writeincr((output_window<TT_DATA>*)outWindow0, d_in);
                    window_writeincr((output_window<TT_DATA>*)outWindow1, d_in);
                    window_writeincr((output_window<TT_DATA>*)outWindow2, d_in);
                    window_writeincr((output_window<TT_DATA>*)outWindow3, d_in);
                }
                for (int k = 0; k < 128 / 8 / sizeof(TT_DATA); k++) {
                    d_in = readincr(inStream1); // read input data
                    window_writeincr((output_window<TT_DATA>*)outWindow0, d_in);
                    window_writeincr((output_window<TT_DATA>*)outWindow1, d_in);
                    window_writeincr((output_window<TT_DATA>*)outWindow2, d_in);
                    window_writeincr((output_window<TT_DATA>*)outWindow3, d_in);
                }
            }
            if (TP_WINDOW_VSIZE * kSampleSize / (kWriteSize / 2) % 2 == 1) { // odd number of chunks
                for (int k = 0; k < kStreamReadSize / sizeof(TT_DATA); k++) {
                    d_in = readincr(inStream0); // read input data
                    window_writeincr((output_window<TT_DATA>*)outWindow0, d_in);
                    window_writeincr((output_window<TT_DATA>*)outWindow1, d_in);
                    window_writeincr((output_window<TT_DATA>*)outWindow2, d_in);
                    window_writeincr((output_window<TT_DATA>*)outWindow3, d_in);
                }
            }
        }
    else if
        constexpr(TP_PATTERN == kSampleIntlv) {
            for (unsigned int i = 0; i < TP_WINDOW_VSIZE / 2; i++) {
                d_in = readincr(inStream0); // read input data
                window_writeincr((output_window<TT_DATA>*)outWindow0, d_in);
                window_writeincr((output_window<TT_DATA>*)outWindow1, d_in);
                window_writeincr((output_window<TT_DATA>*)outWindow2, d_in);
                window_writeincr((output_window<TT_DATA>*)outWindow3, d_in);
                d_in = readincr(inStream1); // read input data
                window_writeincr((output_window<TT_DATA>*)outWindow0, d_in);
                window_writeincr((output_window<TT_DATA>*)outWindow1, d_in);
                window_writeincr((output_window<TT_DATA>*)outWindow2, d_in);
                window_writeincr((output_window<TT_DATA>*)outWindow3, d_in);
            }
        }
    else if
        constexpr(TP_PATTERN == kSplit) {
            for (unsigned int i = 0; i < TP_WINDOW_VSIZE / 2; i++) {
                d_in = readincr(inStream0); // read input data
                window_writeincr((output_window<TT_DATA>*)outWindow0, d_in);
                window_writeincr((output_window<TT_DATA>*)outWindow1, d_in);
                window_writeincr((output_window<TT_DATA>*)outWindow2, d_in);
                window_writeincr((output_window<TT_DATA>*)outWindow3, d_in);
            }
            for (unsigned int i = 0; i < TP_WINDOW_VSIZE / 2; i++) {
                d_in = readincr(inStream1); // read input data
                window_writeincr((output_window<TT_DATA>*)outWindow0, d_in);
                window_writeincr((output_window<TT_DATA>*)outWindow1, d_in);
                window_writeincr((output_window<TT_DATA>*)outWindow2, d_in);
                window_writeincr((output_window<TT_DATA>*)outWindow3, d_in);
            }
        }
};

// Widget API Cast - window to stream, 1 to 1
template <typename TT_DATA, unsigned int TP_WINDOW_VSIZE, unsigned int TP_PATTERN, unsigned int TP_HEADER_BYTES>
void widget_api_cast_ref<TT_DATA, kWindowAPI, kStreamAPI, 1, TP_WINDOW_VSIZE, 1, TP_PATTERN, TP_HEADER_BYTES>::
    transferData(input_window<TT_DATA>* inWindow0, output_stream<TT_DATA>* outStream0) {
    TT_DATA d_in;

    for (unsigned int i = 0; i < TP_WINDOW_VSIZE + TP_HEADER_BYTES / sizeof(TT_DATA); i++) {
        d_in = window_readincr(inWindow0); // read input data
        writeincr(outStream0, d_in);
    }
};

// Widget API Cast - window to stream, 1 to 2
template <typename TT_DATA, unsigned int TP_WINDOW_VSIZE, unsigned int TP_PATTERN, unsigned int TP_HEADER_BYTES>
void widget_api_cast_ref<TT_DATA, kWindowAPI, kStreamAPI, 1, TP_WINDOW_VSIZE, 2, TP_PATTERN, TP_HEADER_BYTES>::
    transferData(input_window<TT_DATA>* inWindow0,
                 output_stream<TT_DATA>* outStream0,
                 output_stream<TT_DATA>* outStream1) {
    TT_DATA d_in;
    constexpr unsigned int kSamplesIn128b = 16 / sizeof(TT_DATA);
    constexpr unsigned int Lsize = (TP_WINDOW_VSIZE) / (2 * kSamplesIn128b);

    // header handling - clone to both outputs
    if
        constexpr(TP_HEADER_BYTES > 0) {
            for (int i = 0; i < TP_HEADER_BYTES / sizeof(TT_DATA); i++) {
                d_in = window_readincr(inWindow0); // read input data samplewise
                writeincr(outStream0, d_in);
                writeincr(outStream1, d_in);
            }
        }

    if
        constexpr(TP_PATTERN == 0) {
            for (unsigned int i = 0; i < Lsize; i++) {
                for (int i = 0; i < kSamplesIn128b; i++) {
                    d_in = window_readincr(inWindow0); // read input data samplewise
                    writeincr(outStream0, d_in);
                }
                for (int i = 0; i < kSamplesIn128b; i++) {
                    d_in = window_readincr(inWindow0); // read input data
                    writeincr(outStream1, d_in);
                }
            }
            if (TP_WINDOW_VSIZE / kSamplesIn128b % 2 == 1) {
                d_in = window_readincr(inWindow0); // read input data samplewise
                writeincr(outStream0, d_in);
            }
        }
    else if
        constexpr(TP_PATTERN == kSampleIntlv) {
            for (int i = 0; i < TP_WINDOW_VSIZE / 2; i++) {
                d_in = window_readincr(inWindow0); // read input data samplewise
                writeincr(outStream0, d_in);
                d_in = window_readincr(inWindow0); // read input data samplewise
                writeincr(outStream1, d_in);
            }
        }
    else if
        constexpr(TP_PATTERN == kSplit) {
            for (int i = 0; i < TP_WINDOW_VSIZE / 2; i++) {
                d_in = window_readincr(inWindow0); // read input data samplewise
                writeincr(outStream0, d_in);
            }
            for (int i = 0; i < TP_WINDOW_VSIZE / 2; i++) {
                d_in = window_readincr(inWindow0); // read input data samplewise
                writeincr(outStream1, d_in);
            }
        }
};
}
}
}
}
}
