#ifndef _DSPLIB_MATRIX_MULT_REF_HPP_
#define _DSPLIB_MATRIX_MULT_REF_HPP_

// This file holds the definition of the Asymmetric Interpolation FIR reference model kernel class.
// Design Notes
// Note that the AIE intrinsics operate on increasing indices, but in a conventional FIR there is a convolution of data
// and coefficients.
// So as to achieve the impulse response from the filter which matches the coefficeint set, the coefficient array has to
// be reversed
// to compensate for the action of the intrinsics. This reversal is performed in the constructor where possible, or in
// the reference
// model's run-time filter function.

#include <adf.h>
#include <limits>

namespace xf {
namespace dsp {
namespace aie {
namespace blas {
namespace matrix_mult {

//#ifndef GET_TT_OUT
//#define GET_TT_OUT(A,B) std::conditional_t<(sizeof(B) > sizeof(A)),B, A>
//#endif //GET_TT_OUT
#ifndef ROW_MAJOR
#define ROW_MAJOR 0
#endif // ROW_MAJOR
#ifndef COL_MAJOR
#define COL_MAJOR 1
#endif // COL_MAJOR

template <typename T_A, typename T_B>
struct outType {
    using type = cint16;
};
template <>
struct outType<int16, int16> {
    using type = int16;
};
template <>
struct outType<int16, cint16> {
    using type = cint16;
};
template <>
struct outType<int16, cint32> {
    using type = cint32;
};
template <>
struct outType<int16, int32> {
    using type = int32;
};
template <>
struct outType<cint16, int16> {
    using type = cint16;
};
template <>
struct outType<cint16, cint16> {
    using type = cint16;
};
template <>
struct outType<cint16, int32> {
    using type = cint32;
};
template <>
struct outType<cint16, cint32> {
    using type = cint32;
};
template <>
struct outType<int32, int16> {
    using type = int32;
};
template <>
struct outType<int32, cint16> {
    using type = cint32;
};
template <>
struct outType<int32, int32> {
    using type = int32;
};
template <>
struct outType<int32, cint32> {
    using type = cint32;
};
template <>
struct outType<cint32, int16> {
    using type = cint32;
};
template <>
struct outType<cint32, cint16> {
    using type = cint32;
};
template <>
struct outType<cint32, int32> {
    using type = cint32;
};
template <>
struct outType<cint32, cint32> {
    using type = cint32;
};
template <>
struct outType<float, float> {
    using type = float;
};
template <>
struct outType<cfloat, float> {
    using type = cfloat;
};
template <>
struct outType<float, cfloat> {
    using type = cfloat;
};
template <>
struct outType<cfloat, cfloat> {
    using type = cfloat;
};
template <typename T_D_A, typename T_D_B>
using outType_t = typename outType<T_D_A, T_D_B>::type;

//-----------------------------------------------------------------------------------------------------
// Single Rate class
template <typename TT_DATA_A,
          typename TT_DATA_B,
          size_t TP_DIM_A,
          size_t TP_DIM_AB,
          size_t TP_DIM_B,
          size_t TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_DIM_A_LEADING = ROW_MAJOR,
          unsigned int TP_DIM_B_LEADING = COL_MAJOR,
          unsigned int TP_DIM_OUT_LEADING = ROW_MAJOR,
          unsigned int TP_INPUT_WINDOW_VSIZE_A = TP_DIM_A* TP_DIM_AB,
          unsigned int TP_INPUT_WINDOW_VSIZE_B = TP_DIM_B* TP_DIM_AB>
class matrix_mult_ref {
   public:
    // Constructor
    matrix_mult_ref() {
        // something?
    }
    // Register Kernel Class
    static void registerKernelClass() {
        REGISTER_FUNCTION(matrix_mult_ref::mmult);
        // REGISTER_PARAMETER(m_internalTapsRef);
    }
    // TT_OUT will need to be a formula.
    // If A or B is cplx, then output is cplx, else real.
    // If A or B is float then output is float, else int.
    // if A or B is *int32, then output is int32, else int16.
    //    - This can be user decision and simply changes SRS.
    void mmult(input_window<TT_DATA_A>* inWindowA,
               input_window<TT_DATA_B>* inWindowB,
               output_window<outType_t<TT_DATA_A, TT_DATA_B> >* outWindow);

   private:
};
}
}
}
}
}

#endif // matrix_mult_ref

/*  (c) Copyright 2019 Xilinx, Inc. All rights reserved.

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
