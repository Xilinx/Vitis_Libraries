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

// Widget API Cast - default/base 'specialization'
template <typename TT_DATA,
          unsigned int TP_IN_API,
          unsigned int TP_OUT_API,
          unsigned int TP_NUM_INPUTS,
          unsigned int TP_WINDOW_VSIZE,
          unsigned int TP_NUM_OUTPUT_CLONES>
void widget_api_cast_ref<TT_DATA, TP_IN_API, TP_OUT_API, TP_NUM_INPUTS, TP_WINDOW_VSIZE, TP_NUM_OUTPUT_CLONES>::
    transferData(input_window<TT_DATA>* inWindow0, output_window<TT_DATA>* outWindow0) {
    TT_DATA d_in;

#ifdef _DSPLIB_WIDGET_API_CAST_REF_DEBUG_
    const unsigned int kSamplesInWindow = window_size(inWindow0); // number of samples in window
    if (kSamplesInWindow != TP_WINDOW_VSIZE) {
        printf("Error: mismatch of samples in window versus template parameter");
    }
#endif //_DSPLIB_WIDGET_API_CAST_REF_DEBUG_

    for (unsigned int i = 0; i < TP_WINDOW_VSIZE; i++) {
        d_in = window_readincr(inWindow0); // read input data
        window_writeincr((output_window<TT_DATA>*)outWindow0, d_in);
    }
};

// Widget API Cast - dual output 'specialization'
template <typename TT_DATA, unsigned int TP_WINDOW_VSIZE>
void widget_api_cast_ref<TT_DATA, kWindowAPI, kWindowAPI, 1, TP_WINDOW_VSIZE, 2>::transferData(
    input_window<TT_DATA>* inWindow0, output_window<TT_DATA>* outWindow0, output_window<TT_DATA>* outWindow1) {
    TT_DATA d_in;

#ifdef _DSPLIB_WIDGET_API_CAST_REF_DEBUG_
    const unsigned int kSamplesInWindow = window_size(inWindow0); // number of samples in window
    if (kSamplesInWindow != TP_WINDOW_VSIZE) {
        printf("Error: mismatch of samples in window versus template parameter");
    }
#endif //_DSPLIB_WIDGET_API_CAST_REF_DEBUG_

    for (unsigned int i = 0; i < TP_WINDOW_VSIZE; i++) {
        d_in = window_readincr(inWindow0); // read input data
        window_writeincr((output_window<TT_DATA>*)outWindow0, d_in);
        window_writeincr((output_window<TT_DATA>*)outWindow1, d_in);
    }
};

// Widget API Cast - triple output 'specialization'
template <typename TT_DATA, unsigned int TP_WINDOW_VSIZE>
void widget_api_cast_ref<TT_DATA, kWindowAPI, kWindowAPI, 1, TP_WINDOW_VSIZE, 3>::transferData(
    input_window<TT_DATA>* inWindow0,
    output_window<TT_DATA>* outWindow0,
    output_window<TT_DATA>* outWindow1,
    output_window<TT_DATA>* outWindow2) {
    TT_DATA d_in;

#ifdef _DSPLIB_WIDGET_API_CAST_REF_DEBUG_
    const unsigned int kSamplesInWindow = window_size(inWindow0); // number of samples in window
    if (kSamplesInWindow != TP_WINDOW_VSIZE) {
        printf("Error: mismatch of samples in window versus template parameter");
    }
#endif //_DSPLIB_WIDGET_API_CAST_REF_DEBUG_

    for (unsigned int i = 0; i < TP_WINDOW_VSIZE; i++) {
        d_in = window_readincr(inWindow0); // read input data
        window_writeincr((output_window<TT_DATA>*)outWindow0, d_in);
        window_writeincr((output_window<TT_DATA>*)outWindow1, d_in);
        window_writeincr((output_window<TT_DATA>*)outWindow2, d_in);
    }
};

// Widget API Cast - 1 stream input 1 window output
template <typename TT_DATA, unsigned int TP_WINDOW_VSIZE>
void widget_api_cast_ref<TT_DATA, kStreamAPI, kWindowAPI, 1, TP_WINDOW_VSIZE, 1>::transferData(
    input_stream<TT_DATA>* inStream0, output_window<TT_DATA>* outWindow0) {
    TT_DATA d_in;

#ifdef _DSPLIB_WIDGET_API_CAST_REF_DEBUG_
    const unsigned int kSamplesInWindow = window_size(outWindow0); // number of samples in window
    if (kSamplesInWindow != TP_WINDOW_VSIZE) {
        printf("Error: mismatch of samples in window versus template parameter");
    }
#endif //_DSPLIB_WIDGET_API_CAST_REF_DEBUG_

    for (unsigned int i = 0; i < TP_WINDOW_VSIZE; i++) {
        d_in = readincr(inStream0); // read input data
        window_writeincr((output_window<TT_DATA>*)outWindow0, d_in);
    }
};

// Widget API Cast - 1 stream input 2 window output
template <typename TT_DATA, unsigned int TP_WINDOW_VSIZE>
void widget_api_cast_ref<TT_DATA, kStreamAPI, kWindowAPI, 1, TP_WINDOW_VSIZE, 2>::transferData(
    input_stream<TT_DATA>* inStream0, output_window<TT_DATA>* outWindow0, output_window<TT_DATA>* outWindow1) {
    TT_DATA d_in;

#ifdef _DSPLIB_WIDGET_API_CAST_REF_DEBUG_
    const unsigned int kSamplesInWindow = window_size(outWindow0); // number of samples in window
    if (kSamplesInWindow != TP_WINDOW_VSIZE) {
        printf("Error: mismatch of samples in window versus template parameter");
    }
#endif //_DSPLIB_WIDGET_API_CAST_REF_DEBUG_

    for (unsigned int i = 0; i < TP_WINDOW_VSIZE; i++) {
        d_in = readincr(inStream0); // read input data
        window_writeincr((output_window<TT_DATA>*)outWindow0, d_in);
        window_writeincr((output_window<TT_DATA>*)outWindow1, d_in);
    }
};

// Widget API Cast - 1 stream input 3 window output
template <typename TT_DATA, unsigned int TP_WINDOW_VSIZE>
void widget_api_cast_ref<TT_DATA, kStreamAPI, kWindowAPI, 1, TP_WINDOW_VSIZE, 3>::transferData(
    input_stream<TT_DATA>* inStream0,
    output_window<TT_DATA>* outWindow0,
    output_window<TT_DATA>* outWindow1,
    output_window<TT_DATA>* outWindow2) {
    TT_DATA d_in;

#ifdef _DSPLIB_WIDGET_API_CAST_REF_DEBUG_
    const unsigned int kSamplesInWindow = window_size(outWindow0); // number of samples in window
    if (kSamplesInWindow != TP_WINDOW_VSIZE) {
        printf("Error: mismatch of samples in window versus template parameter");
    }
#endif //_DSPLIB_WIDGET_API_CAST_REF_DEBUG_

    for (unsigned int i = 0; i < TP_WINDOW_VSIZE; i++) {
        d_in = readincr(inStream0); // read input data
        window_writeincr((output_window<TT_DATA>*)outWindow0, d_in);
        window_writeincr((output_window<TT_DATA>*)outWindow1, d_in);
        window_writeincr((output_window<TT_DATA>*)outWindow2, d_in);
    }
};

// Widget API Cast - 1 stream input 4 window output
template <typename TT_DATA, unsigned int TP_WINDOW_VSIZE>
void widget_api_cast_ref<TT_DATA, kStreamAPI, kWindowAPI, 1, TP_WINDOW_VSIZE, 4>::transferData(
    input_stream<TT_DATA>* inStream0,
    output_window<TT_DATA>* outWindow0,
    output_window<TT_DATA>* outWindow1,
    output_window<TT_DATA>* outWindow2,
    output_window<TT_DATA>* outWindow3) {
    TT_DATA d_in;

#ifdef _DSPLIB_WIDGET_API_CAST_REF_DEBUG_
    const unsigned int kSamplesInWindow = window_size(outWindow0); // number of samples in window
    if (kSamplesInWindow != TP_WINDOW_VSIZE) {
        printf("Error: mismatch of samples in window versus template parameter");
    }
#endif //_DSPLIB_WIDGET_API_CAST_REF_DEBUG_

    for (unsigned int i = 0; i < TP_WINDOW_VSIZE; i++) {
        d_in = readincr(inStream0); // read input data
        window_writeincr((output_window<TT_DATA>*)outWindow0, d_in);
        window_writeincr((output_window<TT_DATA>*)outWindow1, d_in);
        window_writeincr((output_window<TT_DATA>*)outWindow2, d_in);
        window_writeincr((output_window<TT_DATA>*)outWindow3, d_in);
    }
};

// Dual stream in
// Widget API Cast - 2 stream input 1 window output
template <typename TT_DATA, unsigned int TP_WINDOW_VSIZE>
void widget_api_cast_ref<TT_DATA, kStreamAPI, kWindowAPI, 2, TP_WINDOW_VSIZE, 1>::transferData(
    input_stream<TT_DATA>* inStream0, input_stream<TT_DATA>* inStream1, output_window<TT_DATA>* outWindow0) {
    TT_DATA d_in;
    constexpr unsigned int kWriteSize = 256 / 8; // in bytes
    constexpr unsigned int kStreamReadSize = 128 / 8;
    constexpr unsigned int kNumStreams = 2;
    constexpr unsigned int kSampleSize = sizeof(TT_DATA);
    constexpr unsigned int Lsize = TP_WINDOW_VSIZE * kSampleSize / (kWriteSize);
#ifdef _DSPLIB_WIDGET_API_CAST_REF_DEBUG_
    const unsigned int kSamplesInWindow = window_size(outWindow0); // number of samples in window
    if (kSamplesInWindow != TP_WINDOW_VSIZE) {
        printf("Error: mismatch of samples in window versus template parameter");
    }
#endif //_DSPLIB_WIDGET_API_CAST_REF_DEBUG_

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
};

// Widget API Cast - 2 stream input 2 window output
template <typename TT_DATA, unsigned int TP_WINDOW_VSIZE>
void widget_api_cast_ref<TT_DATA, kStreamAPI, kWindowAPI, 2, TP_WINDOW_VSIZE, 2>::transferData(
    input_stream<TT_DATA>* inStream0,
    input_stream<TT_DATA>* inStream1,
    output_window<TT_DATA>* outWindow0,
    output_window<TT_DATA>* outWindow1) {
    TT_DATA d_in;
    constexpr unsigned int kWriteSize = 256 / 8; // in bytes
    constexpr unsigned int kStreamReadSize = 128 / 8;
    constexpr unsigned int kNumStreams = 2;
    constexpr unsigned int kSampleSize = sizeof(TT_DATA);
    constexpr unsigned int Lsize = TP_WINDOW_VSIZE * kSampleSize / (kWriteSize);

#ifdef _DSPLIB_WIDGET_API_CAST_REF_DEBUG_
    const unsigned int kSamplesInWindow = window_size(outWindow0); // number of samples in window
    if (kSamplesInWindow != TP_WINDOW_VSIZE) {
        printf("Error: mismatch of samples in window versus template parameter");
    }
#endif //_DSPLIB_WIDGET_API_CAST_REF_DEBUG_

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
};

// Widget API Cast - 2 stream input 3 window output
template <typename TT_DATA, unsigned int TP_WINDOW_VSIZE>
void widget_api_cast_ref<TT_DATA, kStreamAPI, kWindowAPI, 2, TP_WINDOW_VSIZE, 3>::transferData(
    input_stream<TT_DATA>* inStream0,
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

#ifdef _DSPLIB_WIDGET_API_CAST_REF_DEBUG_
    const unsigned int kSamplesInWindow = window_size(outWindow0); // number of samples in window
    if (kSamplesInWindow != TP_WINDOW_VSIZE) {
        printf("Error: mismatch of samples in window versus template parameter");
    }
#endif //_DSPLIB_WIDGET_API_CAST_REF_DEBUG_

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
};

// Widget API Cast - 2 stream input 4 window output
template <typename TT_DATA, unsigned int TP_WINDOW_VSIZE>
void widget_api_cast_ref<TT_DATA, kStreamAPI, kWindowAPI, 2, TP_WINDOW_VSIZE, 4>::transferData(
    input_stream<TT_DATA>* inStream0,
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

#ifdef _DSPLIB_WIDGET_API_CAST_REF_DEBUG_
    const unsigned int kSamplesInWindow = window_size(outWindow0); // number of samples in window
    if (kSamplesInWindow != TP_WINDOW_VSIZE) {
        printf("Error: mismatch of samples in window versus template parameter");
    }
#endif //_DSPLIB_WIDGET_API_CAST_REF_DEBUG_

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
};

// Widget API Cast - window to stream, 1 to 1
template <typename TT_DATA, unsigned int TP_WINDOW_VSIZE>
void widget_api_cast_ref<TT_DATA, kWindowAPI, kStreamAPI, 1, TP_WINDOW_VSIZE, 1>::transferData(
    input_window<TT_DATA>* inWindow0, output_stream<TT_DATA>* outStream0) {
    TT_DATA d_in;

#ifdef _DSPLIB_WIDGET_API_CAST_REF_DEBUG_
    const unsigned int kSamplesInWindow = window_size(inWindow0); // number of samples in window
    if (kSamplesInWindow != TP_WINDOW_VSIZE) {
        printf("Error: mismatch of samples in window versus template parameter");
    }
#endif //_DSPLIB_WIDGET_API_CAST_REF_DEBUG_

    for (unsigned int i = 0; i < TP_WINDOW_VSIZE; i++) {
        d_in = window_readincr(inWindow0); // read input data
        writeincr(outStream0, d_in);
    }
};

// Widget API Cast - window to stream, 1 to 2
template <typename TT_DATA, unsigned int TP_WINDOW_VSIZE>
void widget_api_cast_ref<TT_DATA, kWindowAPI, kStreamAPI, 1, TP_WINDOW_VSIZE, 2>::transferData(
    input_window<TT_DATA>* inWindow0, output_stream<TT_DATA>* outStream0, output_stream<TT_DATA>* outStream1) {
    TT_DATA d_in;
    constexpr unsigned int kSamplesIn128b = 16 / sizeof(TT_DATA);
    constexpr unsigned int Lsize = TP_WINDOW_VSIZE / (2 * kSamplesIn128b);
#ifdef _DSPLIB_WIDGET_API_CAST_REF_DEBUG_
    const unsigned int kSamplesInWindow = window_size(inWindow0); // number of samples in window
    if (kSamplesInWindow != TP_WINDOW_VSIZE) {
        printf("Error: mismatch of samples in window versus template parameter");
    }
#endif //_DSPLIB_WIDGET_API_CAST_REF_DEBUG_

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
};
}
}
}
}
}

/*  (c) Copyright 2020 Xilinx, Inc. All rights reserved.

    This file contains confidential and proprietary information
    of Xilinx, Inc. and is protected under U.S. and
    international copyright and other intellectual property
    laws.

    DISCLAIMER
    This disclaimer is not a license and does not grant any
    rights to the materials distributed herewith. Except as
    otherwise provided in a valid license issued to you by
    Xilinx, and to the maximum extent permitted by applicable
    law: (1) THESE MATERIALS ARE MADE AVAILABLE "AS IS" AND
    WITH ALL FAULTS, AND XILINX HEREBY DISCLAIMS ALL WARRANTIES
    AND CONDITIONS, EXPRESS, IMPLIED, OR STATUTORY, INCLUDING
    BUT NOT LIMITED TO WARRANTIES OF MERCHANTABILITY, NON-
    INFRINGEMENT, OR FITNESS FOR ANY PARTICULAR PURPOSE; and
    (2) Xilinx shall not be liable (whether in contract or tort,
    including negligence, or under any other theory of
    liability) for any loss or damage of any kind or nature
    related to, arising under or in connection with these
    materials, including for any direct, or any indirect,
    special, incidental, or consequential loss or damage
    (including loss of data, profits, goodwill, or any type of
    loss or damage suffered as a result of any action brought
    by a third party) even if such damage or loss was
    reasonably foreseeable or Xilinx had been advised of the
    possibility of the same.

    CRITICAL APPLICATIONS
    Xilinx products are not designed or intended to be fail-
    safe, or for use in any application requiring fail-safe
    performance, such as life-support or safety devices or
    systems, Class III medical devices, nuclear facilities,
    applications related to the deployment of airbags, or any
    other applications that could lead to death, personal
    injury, or severe property or environmental damage
    (individually and collectively, "Critical
    Applications"). Customer assumes the sole risk and
    liability of any use of Xilinx products in Critical
    Applications, subject only to applicable laws and
    regulations governing limitations on product liability.

    THIS COPYRIGHT NOTICE AND DISCLAIMER MUST BE RETAINED AS
    PART OF THIS FILE AT ALL TIMES.                       */
