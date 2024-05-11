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
#include "fft_ref_stream_split.hpp"
#include "fir_ref_utils.hpp"

namespace xf {
namespace dsp {
namespace aie {
namespace fft {
namespace stream_split {

template <typename TT_DATA, unsigned int TP_NUM_INPUTS, unsigned int TP_WINDOW_VSIZE>
void fft_ref_stream_split<TT_DATA, TP_NUM_INPUTS, TP_WINDOW_VSIZE>::fft_ref_stream_split_main(
    input_window<TT_DATA>* inWindow0, output_stream<TT_DATA>* outStream0, output_stream<TT_DATA>* outStream1) {
    TT_DATA d_in;
    for (unsigned int i = 0; i < TP_WINDOW_VSIZE / TP_NUM_INPUTS; i++) {
        d_in = window_readincr(inWindow0); // read input data
        writeincr(outStream0, d_in);
        d_in = window_readincr(inWindow0); // read input data
        writeincr(outStream1, d_in);
    }
};

template <typename TT_DATA, unsigned int TP_WINDOW_VSIZE>
void fft_ref_stream_split<TT_DATA, 4, TP_WINDOW_VSIZE>::fft_ref_stream_split_main(input_window<TT_DATA>* inWindow0,
                                                                                  output_stream<TT_DATA>* outStream0,
                                                                                  output_stream<TT_DATA>* outStream1,
                                                                                  output_stream<TT_DATA>* outStream2,
                                                                                  output_stream<TT_DATA>* outStream3) {
    TT_DATA d_in;
    for (unsigned int i = 0; i < TP_WINDOW_VSIZE / 4; i++) {
        d_in = window_readincr(inWindow0); // read input data
        writeincr(outStream0, d_in);
        d_in = window_readincr(inWindow0); // read input data
        writeincr(outStream1, d_in);
        d_in = window_readincr(inWindow0); // read input data
        writeincr(outStream2, d_in);
        d_in = window_readincr(inWindow0); // read input data
        writeincr(outStream3, d_in);
    }
};

template <typename TT_DATA, unsigned int TP_WINDOW_VSIZE>
void fft_ref_stream_split<TT_DATA, 8, TP_WINDOW_VSIZE>::fft_ref_stream_split_main(input_window<TT_DATA>* inWindow0,
                                                                                  output_stream<TT_DATA>* outStream0,
                                                                                  output_stream<TT_DATA>* outStream1,
                                                                                  output_stream<TT_DATA>* outStream2,
                                                                                  output_stream<TT_DATA>* outStream3,
                                                                                  output_stream<TT_DATA>* outStream4,
                                                                                  output_stream<TT_DATA>* outStream5,
                                                                                  output_stream<TT_DATA>* outStream6,
                                                                                  output_stream<TT_DATA>* outStream7) {
    TT_DATA d_in;
    for (unsigned int i = 0; i < TP_WINDOW_VSIZE / 8; i++) {
        d_in = window_readincr(inWindow0); // read input data
        writeincr(outStream0, d_in);
        d_in = window_readincr(inWindow0); // read input data
        writeincr(outStream1, d_in);
        d_in = window_readincr(inWindow0); // read input data
        writeincr(outStream2, d_in);
        d_in = window_readincr(inWindow0); // read input data
        writeincr(outStream3, d_in);
        d_in = window_readincr(inWindow0); // read input data
        writeincr(outStream4, d_in);
        d_in = window_readincr(inWindow0); // read input data
        writeincr(outStream5, d_in);
        d_in = window_readincr(inWindow0); // read input data
        writeincr(outStream6, d_in);
        d_in = window_readincr(inWindow0); // read input data
        writeincr(outStream7, d_in);
    }
};

template <typename TT_DATA, unsigned int TP_WINDOW_VSIZE>
void fft_ref_stream_split<TT_DATA, 16, TP_WINDOW_VSIZE>::fft_ref_stream_split_main(
    input_window<TT_DATA>* inWindow0,
    output_stream<TT_DATA>* outStream0,
    output_stream<TT_DATA>* outStream1,
    output_stream<TT_DATA>* outStream2,
    output_stream<TT_DATA>* outStream3,
    output_stream<TT_DATA>* outStream4,
    output_stream<TT_DATA>* outStream5,
    output_stream<TT_DATA>* outStream6,
    output_stream<TT_DATA>* outStream7,
    output_stream<TT_DATA>* outStream8,
    output_stream<TT_DATA>* outStream9,
    output_stream<TT_DATA>* outStream10,
    output_stream<TT_DATA>* outStream11,
    output_stream<TT_DATA>* outStream12,
    output_stream<TT_DATA>* outStream13,
    output_stream<TT_DATA>* outStream14,
    output_stream<TT_DATA>* outStream15) {
    TT_DATA d_in;
    for (unsigned int i = 0; i < TP_WINDOW_VSIZE / 16; i++) {
        d_in = window_readincr(inWindow0); // read input data
        writeincr(outStream0, d_in);
        d_in = window_readincr(inWindow0); // read input data
        writeincr(outStream1, d_in);
        d_in = window_readincr(inWindow0); // read input data
        writeincr(outStream2, d_in);
        d_in = window_readincr(inWindow0); // read input data
        writeincr(outStream3, d_in);
        d_in = window_readincr(inWindow0); // read input data
        writeincr(outStream4, d_in);
        d_in = window_readincr(inWindow0); // read input data
        writeincr(outStream5, d_in);
        d_in = window_readincr(inWindow0); // read input data
        writeincr(outStream6, d_in);
        d_in = window_readincr(inWindow0); // read input data
        writeincr(outStream7, d_in);
        d_in = window_readincr(inWindow0); // read input data
        writeincr(outStream8, d_in);
        d_in = window_readincr(inWindow0); // read input data
        writeincr(outStream9, d_in);
        d_in = window_readincr(inWindow0); // read input data
        writeincr(outStream10, d_in);
        d_in = window_readincr(inWindow0); // read input data
        writeincr(outStream11, d_in);
        d_in = window_readincr(inWindow0); // read input data
        writeincr(outStream12, d_in);
        d_in = window_readincr(inWindow0); // read input data
        writeincr(outStream13, d_in);
        d_in = window_readincr(inWindow0); // read input data
        writeincr(outStream14, d_in);
        d_in = window_readincr(inWindow0); // read input data
        writeincr(outStream15, d_in);
    }
};
}
}
}
}
}
