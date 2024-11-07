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
#ifndef _DSPLIB_DYNAMIC_DFT_REF_GRAPH_HPP_
#define _DSPLIB_DYNAMIC_DFT_REF_GRAPH_HPP_

#include <adf.h>
#include <vector>
#include "dft_ref.hpp"
#include "device_defs.h"

using namespace adf;
namespace xf {
namespace dsp {
namespace aie {
namespace fft {
namespace dft {

template <typename TT_DATA,
          typename TT_TWIDDLE,
          unsigned int TP_POINT_SIZE,
          unsigned int TP_FFT_NIFFT,
          unsigned int TP_SHIFT,
          unsigned int TP_CASC_LEN, // necessary to match UUT, but unused by ref model
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_SSR>
class dft_ref_graph : public graph {
   public:
    port<input> in[1];
    port<output> out[1];

    typedef typename std::conditional_t<
        std::is_same<TT_DATA, int16>::value,
        cint16,
        std::conditional_t<std::is_same<TT_DATA, int32>::value,
                           cint32,
                           std::conditional_t<std::is_same<TT_DATA, float>::value, cfloat, TT_DATA> > >
        T_outDataType;

#ifdef __SUPPORTS_ACC64__
    static constexpr int kSamplesInVectData = 8;
#else
    static constexpr int kSamplesInVectData = 256 / 8 / sizeof(T_outDataType);
#endif //__SUPPORTS_ACC64__
    static constexpr int windowSizeIn = TP_POINT_SIZE * TP_NUM_FRAMES;
    // Account for padding in the output required for SSR in UUT
    static constexpr int windowSizeOut = CEIL(TP_POINT_SIZE, (TP_SSR * kSamplesInVectData)) * TP_NUM_FRAMES;
    // FIR Kernel
    kernel m_dftKernel;

    // Constructor
    dft_ref_graph() {
        printf("===================================\n");
        printf("== DFT 1 Channel mono REF \n");
        printf("== Graph window specialization\n");
        printf("===================================\n");

        printf("Point size           = %d \n", TP_POINT_SIZE);
        printf("FFT/nIFFT            = %d \n", TP_FFT_NIFFT);
        printf("Final scaling Shift  = %d \n", TP_SHIFT);
        printf("Number of kernels    = %d \n", TP_CASC_LEN);
        printf("Rounding mode    = %d \n", TP_RND);
        printf("Saturation mode    = %d \n", TP_SAT);
        printf("Window Size In (ref)         = %d \n", windowSizeIn);
        printf("Window Size Out (ref)         = %d \n", windowSizeOut);
        printf("Data type            = ");
        printf(QUOTE(TT_DATA_TYPE));
        printf("\n");
        printf("TWIDDLE type         = ");
        printf(QUOTE(TT_TWIDDLE_TYPE));
        printf("\n");

        // Create DFT class
        m_dftKernel = kernel::create_object<dft_ref<TT_DATA, TT_TWIDDLE, TP_POINT_SIZE, TP_FFT_NIFFT, TP_SHIFT,
                                                    TP_NUM_FRAMES, TP_RND, TP_SAT, TP_SSR> >();

        // Make connections
        // Size of window in Bytes. Dynamic point size adds a 256 bit (32 byte) header. This is larger than required,
        // but keeps 256 bit alignment
        connect(in[0], m_dftKernel.in[0]);
        dimensions(m_dftKernel.in[0]) = {windowSizeIn};
        connect(m_dftKernel.out[0], out[0]);
        dimensions(m_dftKernel.out[0]) = {windowSizeOut};

        // Specify mapping constraints
        runtime<ratio>(m_dftKernel) = 0.8;

        // Source files
        source(m_dftKernel) = "dft_ref.cpp";
        headers(m_dftKernel) = {"dft_ref.hpp"};
        printf("== Graph window specialization exit\n");
    };
};
}
}
}
}
}
#endif // _DSPLIB_DYNAMIC_DFT_REF_GRAPH_HPP_
