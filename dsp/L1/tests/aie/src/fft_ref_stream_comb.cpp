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
#include "fft_ref_stream_comb.hpp"
#include "fir_ref_utils.hpp"

namespace xf {
namespace dsp {
namespace aie {
namespace fft {
namespace stream_comb {

template <typename TT_DATA, unsigned int TP_NUM_INPUTS, unsigned int TP_WINDOW_VSIZE>
void fft_ref_stream_comb<TT_DATA, TP_NUM_INPUTS, TP_WINDOW_VSIZE>::fft_ref_stream_comb_main(
    input_stream<TT_DATA>* inWindow0, input_stream<TT_DATA>* inWindow1, output_stream<TT_DATA>* outStream0) {
    TT_DATA d_in;
    for (unsigned int i = 0; i < TP_WINDOW_VSIZE / TP_NUM_INPUTS; i++) {
        d_in = readincr(inStream0); // read input data
        window_writeincr((output_window<TT_DATA>*)outWindow0, d_in);
        d_in = readincr(inStream1); // read input data
        window_writeincr((output_window<TT_DATA>*)outWindow0, d_in);
    }
};

template <typename TT_DATA, unsigned int TP_WINDOW_VSIZE>
void fft_ref_stream_comb<TT_DATA, 4, TP_WINDOW_VSIZE>::fft_ref_stream_comb_main(input_stream<TT_DATA>* inWindow0,
                                                                                input_stream<TT_DATA>* inWindow1,
                                                                                input_stream<TT_DATA>* inWindow2,
                                                                                input_stream<TT_DATA>* inWindow3,
                                                                                output_stream<TT_DATA>* outStream0) {
    TT_DATA d_in;
    for (unsigned int i = 0; i < TP_WINDOW_VSIZE / 4; i++) {
        d_in = readincr(inStream0); // read input data
        window_writeincr((output_window<TT_DATA>*)outWindow0, d_in);
        d_in = readincr(inStream1); // read input data
        window_writeincr((output_window<TT_DATA>*)outWindow0, d_in);
        d_in = readincr(inStream2); // read input data
        window_writeincr((output_window<TT_DATA>*)outWindow0, d_in);
        d_in = readincr(inStream3); // read input data
        window_writeincr((output_window<TT_DATA>*)outWindow0, d_in);
    }
};

template <typename TT_DATA, unsigned int TP_WINDOW_VSIZE>
void fft_ref_stream_comb<TT_DATA, 8, TP_WINDOW_VSIZE>::fft_ref_stream_comb_main(input_stream<TT_DATA>* inWindow0,
                                                                                input_stream<TT_DATA>* inWindow1,
                                                                                input_stream<TT_DATA>* inWindow2,
                                                                                input_stream<TT_DATA>* inWindow3,
                                                                                input_stream<TT_DATA>* inWindow4,
                                                                                input_stream<TT_DATA>* inWindow5,
                                                                                input_stream<TT_DATA>* inWindow6,
                                                                                input_stream<TT_DATA>* inWindow7,
                                                                                output_stream<TT_DATA>* outStream0) {
    TT_DATA d_in;
    for (unsigned int i = 0; i < TP_WINDOW_VSIZE / 8; i++) {
        d_in = readincr(inStream0); // read input data
        window_writeincr((output_window<TT_DATA>*)outWindow0, d_in);
        d_in = readincr(inStream1); // read input data
        window_writeincr((output_window<TT_DATA>*)outWindow0, d_in);
        d_in = readincr(inStream2); // read input data
        window_writeincr((output_window<TT_DATA>*)outWindow0, d_in);
        d_in = readincr(inStream3); // read input data
        window_writeincr((output_window<TT_DATA>*)outWindow0, d_in);
        d_in = readincr(inStream4); // read input data
        window_writeincr((output_window<TT_DATA>*)outWindow0, d_in);
        d_in = readincr(inStream5); // read input data
        window_writeincr((output_window<TT_DATA>*)outWindow0, d_in);
        d_in = readincr(inStream6); // read input data
        window_writeincr((output_window<TT_DATA>*)outWindow0, d_in);
        d_in = readincr(inStream7); // read input data
        window_writeincr((output_window<TT_DATA>*)outWindow0, d_in);
    }
};

template <typename TT_DATA, unsigned int TP_WINDOW_VSIZE>
void fft_ref_stream_comb<TT_DATA, 16, TP_WINDOW_VSIZE>::fft_ref_stream_comb_main(input_stream<TT_DATA>* inWindow0,
                                                                                 input_stream<TT_DATA>* inWindow1,
                                                                                 input_stream<TT_DATA>* inWindow2,
                                                                                 input_stream<TT_DATA>* inWindow3,
                                                                                 input_stream<TT_DATA>* inWindow4,
                                                                                 input_stream<TT_DATA>* inWindow5,
                                                                                 input_stream<TT_DATA>* inWindow6,
                                                                                 input_stream<TT_DATA>* inWindow7,
                                                                                 input_stream<TT_DATA>* inWindow8,
                                                                                 input_stream<TT_DATA>* inWindow9,
                                                                                 input_stream<TT_DATA>* inWindow10,
                                                                                 input_stream<TT_DATA>* inWindow11,
                                                                                 input_stream<TT_DATA>* inWindow12,
                                                                                 input_stream<TT_DATA>* inWindow13,
                                                                                 input_stream<TT_DATA>* inWindow14,
                                                                                 input_stream<TT_DATA>* inWindow15,
                                                                                 output_stream<TT_DATA>* outStream0) {
    TT_DATA d_in;
    for (unsigned int i = 0; i < TP_WINDOW_VSIZE / 16; i++) {
        d_in = readincr(inStream0); // read input data
        window_writeincr((output_window<TT_DATA>*)outWindow0, d_in);
        d_in = readincr(inStream1); // read input data
        window_writeincr((output_window<TT_DATA>*)outWindow0, d_in);
        d_in = readincr(inStream2); // read input data
        window_writeincr((output_window<TT_DATA>*)outWindow0, d_in);
        d_in = readincr(inStream3); // read input data
        window_writeincr((output_window<TT_DATA>*)outWindow0, d_in);
        d_in = readincr(inStream4); // read input data
        window_writeincr((output_window<TT_DATA>*)outWindow0, d_in);
        d_in = readincr(inStream5); // read input data
        window_writeincr((output_window<TT_DATA>*)outWindow0, d_in);
        d_in = readincr(inStream6); // read input data
        window_writeincr((output_window<TT_DATA>*)outWindow0, d_in);
        d_in = readincr(inStream7); // read input data
        window_writeincr((output_window<TT_DATA>*)outWindow0, d_in);
        d_in = readincr(inStream8); // read input data
        window_writeincr((output_window<TT_DATA>*)outWindow0, d_in);
        d_in = readincr(inStream9); // read input data
        window_writeincr((output_window<TT_DATA>*)outWindow0, d_in);
        d_in = readincr(inStream10); // read input data
        window_writeincr((output_window<TT_DATA>*)outWindow0, d_in);
        d_in = readincr(inStream11); // read input data
        window_writeincr((output_window<TT_DATA>*)outWindow0, d_in);
        d_in = readincr(inStream12); // read input data
        window_writeincr((output_window<TT_DATA>*)outWindow0, d_in);
        d_in = readincr(inStream13); // read input data
        window_writeincr((output_window<TT_DATA>*)outWindow0, d_in);
        d_in = readincr(inStream14); // read input data
        window_writeincr((output_window<TT_DATA>*)outWindow0, d_in);
        d_in = readincr(inStream15); // read input data
        window_writeincr((output_window<TT_DATA>*)outWindow0, d_in);
    }
};
}
}
}
}
}
