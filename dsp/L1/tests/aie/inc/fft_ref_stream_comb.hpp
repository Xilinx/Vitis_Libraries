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
#ifndef _DSPLIB_FFT_REF_STREAM_COMB_HPP_
#define _DSPLIB_FFT_REF_STREAM_COMB_HPP_
/*
FFT Reference-only utilty to combine multiple streams to one window model
*/
#include <adf.h>
#include <limits>
#include "fir_ref_utils.hpp"

#define _DSPLIB_FFT_REF_STREAM_COMB_DEBUG_

namespace xf {
namespace dsp {
namespace aie {
namespace fft {
namespace stream_comb {

//-----------------------------------------------------------------------------------------------------
template <typename TT_DATA, // type of data input and output
          unsigned int TP_NUM_INPUTS,
          unsigned int TP_WINDOW_VSIZE>
class fft_ref_stream_comb {
   public:
    // Constructor
    fft_ref_stream_comb() {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fft_ref_stream_comb::fft_ref_stream_comb_main); }
    // FIR
    void fft_ref_stream_comb_main(input_stream<TT_DATA>* inStream0,
                                  input_stream<TT_DATA>* inStream1,
                                  output_window<TT_DATA>* outWindow0);
};

template <typename TT_DATA, // type of data input and output
          unsigned int TP_WINDOW_VSIZE>
class fft_ref_stream_comb<TT_DATA, 4, TP_WINDOW_VSIZE> {
   public:
    // Constructor
    fft_ref_stream_comb() {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fft_ref_stream_comb::fft_ref_stream_comb_main); }
    // FIR
    void fft_ref_stream_comb_main(input_stream<TT_DATA>* inStream0,
                                  input_stream<TT_DATA>* inStream1,
                                  input_stream<TT_DATA>* inStream2,
                                  input_stream<TT_DATA>* inStream3,
                                  output_window<TT_DATA>* outWindow0);
};
template <typename TT_DATA, // type of data input and output
          unsigned int TP_WINDOW_VSIZE>
class fft_ref_stream_comb<TT_DATA, 8, TP_WINDOW_VSIZE> {
   public:
    // Constructor
    fft_ref_stream_comb() {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fft_ref_stream_comb::fft_ref_stream_comb_main); }
    // FIR
    void fft_ref_stream_comb_main(input_stream<TT_DATA>* inStream0,
                                  input_stream<TT_DATA>* inStream1,
                                  input_stream<TT_DATA>* inStream2,
                                  input_stream<TT_DATA>* inStream3,
                                  input_stream<TT_DATA>* inStream4,
                                  input_stream<TT_DATA>* inStream5,
                                  input_stream<TT_DATA>* inStream6,
                                  input_stream<TT_DATA>* inStream7,
                                  output_window<TT_DATA>* outWindow0);
};
template <typename TT_DATA, // type of data input and output
          unsigned int TP_WINDOW_VSIZE>
class fft_ref_stream_comb<TT_DATA, 16, TP_WINDOW_VSIZE> {
   public:
    // Constructor
    fft_ref_stream_comb() {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fft_ref_stream_comb::fft_ref_stream_comb_main); }
    // FIR
    void fft_ref_stream_comb_main(input_stream<TT_DATA>* inStream0,
                                  input_stream<TT_DATA>* inStream1,
                                  input_stream<TT_DATA>* inStream2,
                                  input_stream<TT_DATA>* inStream3,
                                  input_stream<TT_DATA>* inStream4,
                                  input_stream<TT_DATA>* inStream5,
                                  input_stream<TT_DATA>* inStream6,
                                  input_stream<TT_DATA>* inStream7,
                                  input_stream<TT_DATA>* inStream8,
                                  input_stream<TT_DATA>* inStream9,
                                  input_stream<TT_DATA>* inStream10,
                                  input_stream<TT_DATA>* inStream11,
                                  input_stream<TT_DATA>* inStream12,
                                  input_stream<TT_DATA>* inStream13,
                                  input_stream<TT_DATA>* inStream14,
                                  input_stream<TT_DATA>* inStream15,
                                  output_window<TT_DATA>* outWindow0);
};
}
}
}
}
}

#endif // _DSPLIB_FFT_REF_STREAM_COMB_HPP_
