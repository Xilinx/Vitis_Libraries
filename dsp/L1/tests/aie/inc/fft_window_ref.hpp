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
#ifndef _DSPLIB_FFT_WINDOW_REF_HPP_
#define _DSPLIB_FFT_WINDOW_REF_HPP_

/*
FFT Window reference model
*/

#include <adf.h>
#include <limits>
#include <array>
#include "device_defs.h"
#include "fft_ref_utils.hpp"

using namespace adf;

//#define _DSPLIB_FFT_WINDOW_REF_DEBUG_

namespace xf {
namespace dsp {
namespace aie {
namespace fft {
namespace windowfn {

template <int T_SSR>
unsigned int fnLogSSRref() {
    return -1;
}; // inline? workaround for double definition.
template <>
inline unsigned int fnLogSSRref<1>() {
    return 0;
};
template <>
inline unsigned int fnLogSSRref<2>() {
    return 1;
};
template <>
inline unsigned int fnLogSSRref<4>() {
    return 2;
};
template <>
inline unsigned int fnLogSSRref<8>() {
    return 3;
};
template <>
inline unsigned int fnLogSSRref<16>() {
    return 4;
};
template <>
inline unsigned int fnLogSSRref<32>() {
    return 5;
};

template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_POINT_SIZE,
          unsigned int TP_WINDOW_VSIZE,
          unsigned int TP_SHIFT,
          unsigned int TP_API,
          unsigned int TP_DYN_PT_SIZE,
          unsigned int TP_RND,
          unsigned int TP_SAT>
class fft_window_ref_base { // base class with all features except interface which is left to inherited
   private:
   public:
    static constexpr int kMaxGraphPointSize = 65536;
    static constexpr int kMinGraphPointSize = 16;
    static constexpr int klogMaxGraphPointSize = 16;
    static constexpr int klogMinGraphPointSize = 4;
    TT_COEFF weights[TP_POINT_SIZE * (1 + TP_DYN_PT_SIZE)];
    int tableStarts[klogMaxGraphPointSize - klogMinGraphPointSize];
    // Constructor
    fft_window_ref_base(const std::array<TT_COEFF, TP_POINT_SIZE*(1 + TP_DYN_PT_SIZE)>& kernel_weights) {
        for (int i = 0; i < TP_POINT_SIZE * (1 + TP_DYN_PT_SIZE); i++) {
            weights[i] = kernel_weights[i];
        }

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
    }
};

//-----------------------------------------------------------------------------------------------------
// FFT window- default/base 'specialization'
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
class fft_window_ref : public fft_window_ref_base<TT_DATA,
                                                  TT_COEFF,
                                                  TP_POINT_SIZE,
                                                  TP_WINDOW_VSIZE,
                                                  TP_SHIFT,
                                                  TP_API,
                                                  TP_DYN_PT_SIZE,
                                                  TP_RND,
                                                  TP_SAT> {
   private:
   public:
    using baseKernel = fft_window_ref_base<TT_DATA,
                                           TT_COEFF,
                                           TP_POINT_SIZE,
                                           TP_WINDOW_VSIZE,
                                           TP_SHIFT,
                                           TP_API,
                                           TP_DYN_PT_SIZE,
                                           TP_RND,
                                           TP_SAT>;
    unsigned int kLogSSR = fnLogSSRref<TP_SSR>();
    // Constructor
    fft_window_ref(const std::array<TT_COEFF, TP_POINT_SIZE*(1 + TP_DYN_PT_SIZE)>& kernel_weights)
        : baseKernel(kernel_weights) {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fft_window_ref::fft_window_main); }
    // FIR
    void fft_window_main(input_buffer<TT_DATA>& inWindow0, output_buffer<TT_DATA>& outWindow0);
};

template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_POINT_SIZE,
          unsigned int TP_WINDOW_VSIZE,
          unsigned int TP_SHIFT,
          unsigned int TP_SSR,
          unsigned int TP_DYN_PT_SIZE,
          unsigned int TP_RND,
          unsigned int TP_SAT>
class fft_window_ref<TT_DATA,
                     TT_COEFF,
                     TP_POINT_SIZE,
                     TP_WINDOW_VSIZE,
                     TP_SHIFT,
                     1,
                     TP_SSR,
                     TP_DYN_PT_SIZE,
                     TP_RND,
                     TP_SAT> : public fft_window_ref_base<TT_DATA,
                                                          TT_COEFF,
                                                          TP_POINT_SIZE,
                                                          TP_WINDOW_VSIZE,
                                                          TP_SHIFT,
                                                          1,
                                                          TP_DYN_PT_SIZE,
                                                          TP_RND,
                                                          TP_SAT> {
   private:
   public:
    // Constructor
    using baseKernel = fft_window_ref_base<TT_DATA,
                                           TT_COEFF,
                                           TP_POINT_SIZE,
                                           TP_WINDOW_VSIZE,
                                           TP_SHIFT,
                                           1,
                                           TP_DYN_PT_SIZE,
                                           TP_RND,
                                           TP_SAT>;
    unsigned int kLogSSR = fnLogSSRref<TP_SSR>();
    fft_window_ref(const std::array<TT_COEFF, TP_POINT_SIZE*(1 + TP_DYN_PT_SIZE)>& kernel_weights)
        : baseKernel(kernel_weights) {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fft_window_ref::fft_window_main); }
// kernel main entry function
#if __STREAMS_PER_TILE__ == 2
    void fft_window_main(input_stream<TT_DATA>* inStream0,
                         input_stream<TT_DATA>* inStream1,
                         output_stream<TT_DATA>* outStream0,
                         output_stream<TT_DATA>* outStream1);
#else
    void fft_window_main(input_stream<TT_DATA>* inStream0, output_stream<TT_DATA>* outStream0);
#endif // __STREAMS_PER_TILE__ == 2
};
}
}
}
}
}

#endif // _DSPLIB_FFT_WINDOW_REF_HPP_
