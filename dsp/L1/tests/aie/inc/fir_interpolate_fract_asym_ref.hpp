#ifndef _DSPLIB_FIR_INTERPOLATE_FRACT_ASYM_REF_HPP_
#define _DSPLIB_FIR_INTERPOLATE_FRACT_ASYM_REF_HPP_

/*
fir_interpolate_fract asym filter reference model
*/

#include <adf.h>
#include <limits>
#include "fir_ref_utils.hpp"

namespace xf {
namespace dsp {
namespace aie {
namespace fir {
namespace interpolate_fract_asym {

//-----------------------------------------------------------------------------------------------------
// Interpolate Fract Asym Reference Model Class - static coefficients
template <typename TT_DATA,  // type of data input and output
          typename TT_COEFF, // type of coefficients           (e.g. int16, cint32)
          unsigned int TP_FIR_LEN,
          unsigned int TP_INTERPOLATE_FACTOR,
          unsigned int TP_DECIMATE_FACTOR,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_USE_COEFF_RELOAD = 0, // 1 = use coeff reload, 0 = don't use coeff reload
          unsigned int TP_NUM_OUTPUTS = 1>
class fir_interpolate_fract_asym_ref {
   public:
    // Constructor
    fir_interpolate_fract_asym_ref(const TT_COEFF (&coefficients)[TP_FIR_LEN]) {
        // This reference model uses taps directly. It does not need to pad the taps array
        // to the column width because the concept of columns does not apply to the ref model.
        for (int i = 0; i < FIR_LEN; ++i) {
            // We don't need any reversal
            m_internalTapsRef[i] = coefficients[i];
        }
    }
    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fir_interpolate_fract_asym_ref::filter); }
    // FIR
    void filter(input_window<TT_DATA>* inWindow, output_window<TT_DATA>* outWindow);

   private:
    TT_COEFF chess_storage(% chess_alignof(v8cint16)) m_internalTapsRef[TP_FIR_LEN];
};

// specialization for static coefficients, dual output
template <typename TT_DATA,  // type of data input and output
          typename TT_COEFF, // type of coefficients           (e.g. int16, cint32)
          unsigned int TP_FIR_LEN,
          unsigned int TP_INTERPOLATE_FACTOR,
          unsigned int TP_DECIMATE_FACTOR,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE>
class fir_interpolate_fract_asym_ref<TT_DATA,
                                     TT_COEFF,
                                     TP_FIR_LEN,
                                     TP_INTERPOLATE_FACTOR,
                                     TP_DECIMATE_FACTOR,
                                     TP_SHIFT,
                                     TP_RND,
                                     TP_INPUT_WINDOW_VSIZE,
                                     USE_COEFF_RELOAD_FALSE,
                                     2> {
   public:
    // Constructor
    fir_interpolate_fract_asym_ref(const TT_COEFF (&coefficients)[TP_FIR_LEN]) {
        // This reference model uses taps directly. It does not need to pad the taps array
        // to the column width because the concept of columns does not apply to the ref model.
        for (int i = 0; i < FIR_LEN; ++i) {
            // We don't need any reversal
            m_internalTapsRef[i] = coefficients[i];
        }
    }
    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fir_interpolate_fract_asym_ref::filter); }
    // FIR
    void filter(input_window<TT_DATA>* inWindow, output_window<TT_DATA>* outWindow, output_window<TT_DATA>* outWindow2);

   private:
    TT_COEFF chess_storage(% chess_alignof(v8cint16)) m_internalTapsRef[TP_FIR_LEN];
};

//-----------------------------------------------------------------------------------------------------
// specialization for reloadable coefficients, single output
template <typename TT_DATA,  // type of data input and output
          typename TT_COEFF, // type of coefficients           (e.g. int16, cint32)
          unsigned int TP_FIR_LEN,
          unsigned int TP_INTERPOLATE_FACTOR,
          unsigned int TP_DECIMATE_FACTOR,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE>
class fir_interpolate_fract_asym_ref<TT_DATA,
                                     TT_COEFF,
                                     TP_FIR_LEN,
                                     TP_INTERPOLATE_FACTOR,
                                     TP_DECIMATE_FACTOR,
                                     TP_SHIFT,
                                     TP_RND,
                                     TP_INPUT_WINDOW_VSIZE,
                                     USE_COEFF_RELOAD_TRUE,
                                     1> {
   public:
    // Constructor
    fir_interpolate_fract_asym_ref() {}
    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fir_interpolate_fract_asym_ref::filter); }
    // FIR
    void filter(input_window<TT_DATA>* inWindow,
                output_window<TT_DATA>* outWindow,
                const TT_COEFF (&inTaps)[TP_FIR_LEN]);

   private:
    TT_COEFF chess_storage(% chess_alignof(v8cint16)) m_internalTapsRef[TP_FIR_LEN];
};

// specialization for reloadable coefficients, dual output
template <typename TT_DATA,  // type of data input and output
          typename TT_COEFF, // type of coefficients           (e.g. int16, cint32)
          unsigned int TP_FIR_LEN,
          unsigned int TP_INTERPOLATE_FACTOR,
          unsigned int TP_DECIMATE_FACTOR,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE>
class fir_interpolate_fract_asym_ref<TT_DATA,
                                     TT_COEFF,
                                     TP_FIR_LEN,
                                     TP_INTERPOLATE_FACTOR,
                                     TP_DECIMATE_FACTOR,
                                     TP_SHIFT,
                                     TP_RND,
                                     TP_INPUT_WINDOW_VSIZE,
                                     USE_COEFF_RELOAD_TRUE,
                                     2> {
   public:
    // Constructor
    fir_interpolate_fract_asym_ref() {}
    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fir_interpolate_fract_asym_ref::filter); }
    // FIR
    void filter(input_window<TT_DATA>* inWindow,
                output_window<TT_DATA>* outWindow,
                output_window<TT_DATA>* outWindow2,
                const TT_COEFF (&inTaps)[TP_FIR_LEN]);

   private:
    TT_COEFF chess_storage(% chess_alignof(v8cint16)) m_internalTapsRef[TP_FIR_LEN];
};
}
}
}
}
}

#endif // _DSPLIB_FIR_INTERPOLATE_FRACT_ASYM_REF_HPP_

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
