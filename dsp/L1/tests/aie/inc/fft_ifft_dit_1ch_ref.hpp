#ifndef _DSPLIB_FFT_IFFT_DIT_1CH_REF_HPP_
#define _DSPLIB_FFT_IFFT_DIT_1CH_REF_HPP_

/*
FFT/iFFT DIT single channel reference model
*/

//#define _DSPLIB_FFT_IFFT_DIT_1CH_REF_DEBUG_

#include <adf.h>
#include <limits>
namespace xf {
namespace dsp {
namespace aie {
namespace fft {
namespace dit_1ch {

//---------------------------------
// Templatized types
template <typename T_D>
struct T_int_data {};
template <>
struct T_int_data<int16> {
    int32 real;
    int32 imag;
};
template <>
struct T_int_data<cint16> {
    int32 real;
    int32 imag;
};
template <>
struct T_int_data<int32> {
    int32 real;
    int32 imag;
};
template <>
struct T_int_data<cint32> {
    int32 real;
    int32 imag;
};
template <>
struct T_int_data<float> {
    float real;
    float imag;
};
template <>
struct T_int_data<cfloat> {
    float real;
    float imag;
};

template <typename T_D>
struct T_accfftRef {};
template <>
struct T_accfftRef<int16> {
    int32 real;
    int32 imag;
};
template <>
struct T_accfftRef<cint16> {
    int32 real;
    int32 imag;
};
template <>
struct T_accfftRef<int32> {
    int32 real;
    int32 imag;
};
template <>
struct T_accfftRef<cint32> {
    int32 real;
    int32 imag;
};
template <>
struct T_accfftRef<float> {
    float real;
    float imag;
};
template <>
struct T_accfftRef<cfloat> {
    float real;
    float imag;
};

// Fn to perform log2 on TP_POINT_SIZE to get #ranks
template <unsigned int TP_POINT_SIZE>
inline constexpr unsigned int fnGetPointSizePower() {
    return TP_POINT_SIZE == 16
               ? 4
               : TP_POINT_SIZE == 32
                     ? 5
                     : TP_POINT_SIZE == 64
                           ? 6
                           : TP_POINT_SIZE == 128
                                 ? 7
                                 : TP_POINT_SIZE == 256
                                       ? 8
                                       : TP_POINT_SIZE == 512
                                             ? 9
                                             : TP_POINT_SIZE == 1024
                                                   ? 10
                                                   : TP_POINT_SIZE == 2048 ? 11 : TP_POINT_SIZE == 4096 ? 12 : 0;
}

//-----------------------------------------------------------------------------------------------------
// FFT/iFFT DIT single channel reference model class
template <typename TT_DATA,    // type of data input and output
          typename TT_TWIDDLE, // type of twiddle factor
          unsigned int TP_POINT_SIZE,
          unsigned int TP_FFT_NIFFT,
          unsigned int TP_SHIFT,
          unsigned int TP_DYN_PT_SIZE,
          unsigned int TP_WINDOW_VSIZE = TP_POINT_SIZE>
class fft_ifft_dit_1ch_ref {
   private:
    TT_TWIDDLE twiddles[TP_POINT_SIZE];
    static constexpr unsigned int kMaxLogPtSize = 12;
    static constexpr unsigned int kRanks =
        fnGetPointSizePower<TP_POINT_SIZE>(); // while each rank is radix2 this is true
    void r2StageInt(
        T_int_data<TT_DATA>* samplesA, T_int_data<TT_DATA>* samplesB, TT_TWIDDLE* twiddles, int pptSize, bool inv);
    void r2StageFloat(T_int_data<TT_DATA>* samplesA,
                      T_int_data<TT_DATA>* samplesB,
                      TT_TWIDDLE* twiddles,
                      unsigned int rank,
                      int pptSize,
                      bool inv);
    void r4StageInt(T_int_data<TT_DATA>* samplesIn,
                    TT_TWIDDLE* twiddles1,
                    TT_TWIDDLE* twiddles2,
                    unsigned int n,
                    unsigned int r,
                    unsigned int shift,
                    unsigned int rank,
                    T_int_data<TT_DATA>* samplesOut,
                    int pptSize,
                    bool inv);

   public:
    // Constructor
    fft_ifft_dit_1ch_ref() {}
    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fft_ifft_dit_1ch_ref::fft); }
    // FFT
    void fft(input_window<TT_DATA>* inWindow, output_window<TT_DATA>* outWindow);
    void nonBitAccfft(input_window<TT_DATA>* inWindow, output_window<TT_DATA>* outWindow);
};
}
}
}
}
} // namespace closing braces

#endif // _DSPLIB_FFT_IFFT_DIT_1CH_REF_HPP_

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
