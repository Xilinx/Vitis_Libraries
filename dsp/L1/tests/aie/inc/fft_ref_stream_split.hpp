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
#ifndef _DSPLIB_FFT_REF_STREAM_SPLIT_HPP_
#define _DSPLIB_FFT_REF_STREAM_SPLIT_HPP_
/*
FFT Reference-only utilty to combine multiple streams to one window model
*/
#include <adf.h>
#include <limits>
#include "fir_ref_utils.hpp"

#define _DSPLIB_FFT_REF_STREAM_SPLIT_DEBUG_

namespace xf {
namespace dsp {
namespace aie {
namespace fft {
namespace stream_split {

//-----------------------------------------------------------------------------------------------------
template <typename TT_DATA, // type of data input and output
          unsigned int TP_NUM_OUTPUTS,
          unsigned int TP_WINDOW_VSIZE>
class fft_ref_stream_split {
   public:
    // Constructor
    fft_ref_stream_split() {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fft_ref_stream_split::fft_ref_stream_split_main); }
    // FIR
    void fft_ref_stream_split_main(input_window<TT_DATA>* inWindow0,
                                   output_stream<TT_DATA>* outStream0,
                                   output_stream<TT_DATA>* outStream1);
};

template <typename TT_DATA, // type of data input and output
          unsigned int TP_WINDOW_VSIZE>
class fft_ref_stream_split<TT_DATA, 4, TP_WINDOW_VSIZE> {
   public:
    // Constructor
    fft_ref_stream_split() {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fft_ref_stream_split::fft_ref_stream_split_main); }
    // FIR
    void fft_ref_stream_split_main(input_window<TT_DATA>* inWindow0,
                                   output_stream<TT_DATA>* outStream0,
                                   output_stream<TT_DATA>* outStream1,
                                   output_stream<TT_DATA>* outStream2,
                                   output_stream<TT_DATA>* outStream3);
};
template <typename TT_DATA, // type of data input and output
          unsigned int TP_WINDOW_VSIZE>
class fft_ref_stream_split<TT_DATA, 8, TP_WINDOW_VSIZE> {
   public:
    // Constructor
    fft_ref_stream_split() {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fft_ref_stream_split::fft_ref_stream_split_main); }
    // FIR
    void fft_ref_stream_split_main(input_window<TT_DATA>* inWindow0,
                                   output_stream<TT_DATA>* outStream0,
                                   output_stream<TT_DATA>* outStream1,
                                   output_stream<TT_DATA>* outStream2,
                                   output_stream<TT_DATA>* outStream3,
                                   output_stream<TT_DATA>* outStream4,
                                   output_stream<TT_DATA>* outStream5,
                                   output_stream<TT_DATA>* outStream6,
                                   output_stream<TT_DATA>* outStream7);
};
template <typename TT_DATA, // type of data input and output
          unsigned int TP_WINDOW_VSIZE>
class fft_ref_stream_split<TT_DATA, 16, TP_WINDOW_VSIZE> {
   public:
    // Constructor
    fft_ref_stream_split() {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fft_ref_stream_split::fft_ref_stream_split_main); }
    // FIR
    void fft_ref_stream_split_main(input_window<TT_DATA>* inWindow0,
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
                                   output_stream<TT_DATA>* outStream15);
};
}
}
}
}
}

#endif // _DSPLIB_FFT_REF_STREAM_SPLIT_HPP_
