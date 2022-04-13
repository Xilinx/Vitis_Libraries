/*
 * Copyright 2021 Xilinx, Inc.
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

/**
 * @file svd.hpp
 * @brief This file contains implentation of SVD functions
 *   -svd                 : Entry point function
 *   -svdTop              : Top level function that selects implementation architecture and internal types based on
 * traits class
 *   -svdBasic           : Default architecture
 *   -svdPairs           : Alternative architecture
 */

#ifndef _XF_SOLVER_SVD_HPP_
#define _XF_SOLVER_SVD_HPP_

#include "ap_fixed.h"
#include "hls_x_complex.h"
#include "utils/x_matrix_utils.hpp"
#include "hls_stream.h"
#include "utils/std_complex_utils.h"
#include <complex>

#include <assert.h>

namespace xf {
namespace solver {

// ===================================================================================================================
// Default traits struct
template <int RowsA, int ColsA, typename InputType, typename OutputType>
struct svdTraits {
    typedef OutputType SIntType;
    typedef OutputType UIntType;
    typedef OutputType VIntType;
    typedef OutputType CSIntType;
    static const int NUM_SWEEPS = 10; // Literature typically suggestions 6 to 10 iterations to successfully converge
    static const int MIN_DIM = (RowsA < ColsA ? RowsA : ColsA);
    static const int ARCH = 1;        // Select implementation. 0 = Basic loop engine. 1 = Pairs based engine
    static const int OFF_DIAG_II = 8; // Specify the pipelining target for the off diagonal loop. Upto 4 memory accesses
                                      // on single array in one iteration, use mulitple
    static const int DIAG_II = 8; // Specify the pipelining target for the diagonal loop. >1 enables reuse of operators.
    // IMPLEMENTATION TIP: Potential additional configuration parameters to fully unroll the "Pairs" based engine
    // static const int UNROLL_FACTOR      = (MIN_DIM + 1) / 2; // Specify off-diagonal loop unrolling factor
    // static const int DIAG_UNROLL_FACTOR = (MIN_DIM + 1) / 2; // Specify diagonal loop unrolling factor
};

// ===================================================================================================================
// Helper functions

// Compare 2 values relative magnitude
// - Replaces a test using EPS as a scaling factor:
//   abs(b) <= (e*abs(a)) where e = hls::numeric_limits<CSType>::epsilon()/2;
// - b is within/just outside the representable precision of a
template <typename T>
bool within_precision(T a, T b) {
    fp_struct<T> fs_a = a;
    fp_struct<T> fs_b = b;
    if ((fs_b.exp + fs_a.SIG_BITS + 1) < fs_a.exp || fs_b.exp == 0) {
        return false;
    } else {
        return true;
    }
}

// 2x1 vector dot product
// o Used throughout the code for non-vector operations to ease the application of resource sharing directives
// o When complex data type is used multiple version of this function will be created with a mix of complex and real
//   argument types.
#ifdef HLS_SVD_SKIP_COMMON_VM2X1
template <typename AType, typename BType, typename CType>
void vm2x1(AType a1, BType b1, AType a2, BType b2, CType& c) {
    c = a1 * b1 + a2 * b2;
}
#endif

// Define additional overloaded forms of vm2x1
// o Simplifies controlling resource utilization for complex data types implementations
// o Maps all forms of complex vm2x1 to a single complex only input
#ifndef HLS_SVD_SKIP_COMMON_VM2X1
template <typename AType, typename BType, typename CType>
void vm2x1_base(AType a1, BType b1, AType a2, BType b2, CType& c) {
    // Disable the inlining of the base vm2x1 function and limit instances using the ALLOCATION directive
    // #pragma HLS inline off
    c = a1 * b1 + a2 * b2;
}
template <typename T>
void vm2x1(T a1, hls::x_complex<T> b1, T a2, hls::x_complex<T> b2, hls::x_complex<T>& c) {
    hls::x_complex<T> c_a1, c_a2;
    c_a1 = a1;
    c_a2 = a2;
    vm2x1_base(c_a1, b1, c_a2, b2, c);
}
template <typename T>
void vm2x1(hls::x_complex<T> a1, T b1, hls::x_complex<T> a2, T b2, hls::x_complex<T>& c) {
    hls::x_complex<T> c_b1, c_b2;
    c_b1 = b1;
    c_b2 = b2;
    vm2x1_base(a1, c_b1, a2, c_b2, c);
}
template <typename T>
void vm2x1(T a1, std::complex<T> b1, T a2, std::complex<T> b2, std::complex<T>& c) {
    std::complex<T> c_a1, c_a2;
    c_a1 = a1;
    c_a2 = a2;
    vm2x1_base(c_a1, b1, c_a2, b2, c);
}
template <typename T>
void vm2x1(std::complex<T> a1, T b1, std::complex<T> a2, T b2, std::complex<T>& c) {
    std::complex<T> c_b1, c_b2;
    c_b1 = b1;
    c_b2 = b2;
    vm2x1_base(a1, c_b1, a2, c_b2, c);
}
template <typename T>
void vm2x1(T a1, T b1, T a2, T b2, T& c) {
    vm2x1_base(a1, b1, a2, b2, c);
}
#endif

// 2x2 matrix multiply
template <typename AType, typename BType, typename CType>
void mm2x2(AType a1,
           AType a2,
           AType a3,
           AType a4,
           BType b1,
           BType b2,
           BType b3,
           BType b4,
           CType& c1,
           CType& c2,
           CType& c3,
           CType& c4) {
    vm2x1(a1, b1, a2, b3, c1);
    vm2x1(a1, b2, a2, b4, c2);
    vm2x1(a3, b1, a4, b3, c3);
    vm2x1(a3, b2, a4, b4, c4);
}

// Calculate the sin and cos of the complex input's phase angle and phase angle divided by 2
template <typename InType, typename CSType>
void calc_angle(hls::x_complex<InType> A,
                CSType& cosThetaA,
                CSType& sinThetaA,
                CSType& cosThetaAdiv2,
                CSType& sinThetaAdiv2,
                bool& is_pos_real,
                bool& is_imag) {
    const InType inZERO = 0;
    const CSType csZERO = 0;
    const CSType csONE = 1;
    // NOTE: Hard single precision floating point value
    const float ONE_OVER_ROOT2 = 1.0f / sqrtf(2.0f);

    CSType tanThetaA, cosThetaA_int, sinThetaA_int, tanThetaAdiv2, cosThetaAdiv2_int;

    InType re = A.real();
    InType im = A.imag();

    // Helpers to avoid testing the sin and cos outputs for particular characteristics.
    is_pos_real = false;
    is_imag = false;

    // Check for when effectively real only or imag only
    if (!within_precision(re, im)) {
        if (x_isneg(re)) {
            // 180 degs (half is 90!)
            cosThetaA = -csONE;
            sinThetaA = csZERO;
            cosThetaAdiv2 = csZERO;
            sinThetaAdiv2 = csONE;
        } else {
            // 0 degs
            cosThetaA = csONE;
            sinThetaA = csZERO;
            cosThetaAdiv2 = csONE;
            sinThetaAdiv2 = csZERO;
            is_pos_real = true;
        }
    } else if (!within_precision(im, re)) {
        is_imag = true;
        if (x_isneg(im)) {
            // 270 deg
            cosThetaA = csZERO;
            sinThetaA = -csONE;
            cosThetaAdiv2 = -ONE_OVER_ROOT2;
            sinThetaAdiv2 = ONE_OVER_ROOT2;
        } else {
            // 90 deg
            cosThetaA = csZERO;
            sinThetaA = csONE;
            cosThetaAdiv2 = ONE_OVER_ROOT2;
            sinThetaAdiv2 = ONE_OVER_ROOT2;
        }
    } else {
        // Basic
        // Full angle values
        tanThetaA = im / re;
        cosThetaA_int = x_copysign(csONE, re) * x_rsqrt(csONE + tanThetaA * tanThetaA);
        cosThetaA = cosThetaA_int;
        sinThetaA_int = cosThetaA_int * tanThetaA;
        sinThetaA = sinThetaA_int;

        // Half angle values
        // o Select the correct expression to minimize error in tan(thetaA/2)
        //   - Avoid creating near eps values
        if (x_isneg(cosThetaA_int)) {
            tanThetaAdiv2 = (csONE - cosThetaA_int) / sinThetaA_int;
        } else {
            tanThetaAdiv2 = sinThetaA_int / (csONE + cosThetaA_int);
        }
        cosThetaAdiv2_int = x_rsqrt(csONE + tanThetaAdiv2 * tanThetaAdiv2);

        cosThetaAdiv2 = cosThetaAdiv2_int;
        sinThetaAdiv2 = cosThetaAdiv2_int * tanThetaAdiv2;
    }
}
// Calculate the sin and cos of the complex input's phase angle and phase angle divided by 2
template <typename InType, typename CSType>
void calc_angle(std::complex<InType> A,
                CSType& cosThetaA,
                CSType& sinThetaA,
                CSType& cosThetaAdiv2,
                CSType& sinThetaAdiv2,
                bool& is_pos_real,
                bool& is_imag) {
    const InType inZERO = 0;
    const CSType csZERO = 0;
    const CSType csONE = 1;
    // NOTE: Hard single precision floating point value
    const float ONE_OVER_ROOT2 = 1.0f / sqrtf(2.0f);

    CSType tanThetaA, cosThetaA_int, sinThetaA_int, tanThetaAdiv2, cosThetaAdiv2_int;

    InType re = A.real();
    InType im = A.imag();

    // Helpers to avoid testing the sin and cos outputs for particular characteristics.
    is_pos_real = false;
    is_imag = false;

    // Check for when effectively real only or imag only
    if (!within_precision(re, im)) {
        if (x_isneg(re)) {
            // 180 degs (half is 90!)
            cosThetaA = -csONE;
            sinThetaA = csZERO;
            cosThetaAdiv2 = csZERO;
            sinThetaAdiv2 = csONE;
        } else {
            // 0 degs
            cosThetaA = csONE;
            sinThetaA = csZERO;
            cosThetaAdiv2 = csONE;
            sinThetaAdiv2 = csZERO;
            is_pos_real = true;
        }
    } else if (!within_precision(im, re)) {
        is_imag = true;
        if (x_isneg(im)) {
            // 270 deg
            cosThetaA = csZERO;
            sinThetaA = -csONE;
            cosThetaAdiv2 = -ONE_OVER_ROOT2;
            sinThetaAdiv2 = ONE_OVER_ROOT2;
        } else {
            // 90 deg
            cosThetaA = csZERO;
            sinThetaA = csONE;
            cosThetaAdiv2 = ONE_OVER_ROOT2;
            sinThetaAdiv2 = ONE_OVER_ROOT2;
        }
    } else {
        // Basic
        // Full angle values
        tanThetaA = im / re;
        cosThetaA_int = x_copysign(csONE, re) * x_rsqrt(csONE + tanThetaA * tanThetaA);
        cosThetaA = cosThetaA_int;
        sinThetaA_int = cosThetaA_int * tanThetaA;
        sinThetaA = sinThetaA_int;

        // Half angle values
        // o Select the correct expression to minimize error in tan(thetaA/2)
        //   - Avoid creating near eps values
        if (x_isneg(cosThetaA_int)) {
            tanThetaAdiv2 = (csONE - cosThetaA_int) / sinThetaA_int;
        } else {
            tanThetaAdiv2 = sinThetaA_int / (csONE + cosThetaA_int);
        }
        cosThetaAdiv2_int = x_rsqrt(csONE + tanThetaAdiv2 * tanThetaAdiv2);

        cosThetaAdiv2 = cosThetaAdiv2_int;
        sinThetaAdiv2 = cosThetaAdiv2_int * tanThetaAdiv2;
    }
}

// ===================================================================================================================
// Diagonal processing functions

// Real 2x2 SVD function
template <typename AInType, typename CSType, typename AOutType>
void svd2x2(AInType w_in,
            AInType x_in,
            AInType y_in,
            AInType z_in,
            CSType& uw_out,
            CSType& ux_out,
            CSType& uy_out,
            CSType& uz_out,
            CSType& vw_out,
            CSType& vx_out,
            CSType& vy_out,
            CSType& vz_out,
            AOutType& w_out,
            AOutType& x_out,
            AOutType& y_out,
            AOutType& z_out) {
Function_svd2x2_real:;
// Inline to bring common lower level functions to this level of hierarchy to simplify the application
// of resource sharing directives.
#pragma HLS inline

    const AOutType outZERO = 0;
    CSType s1, c1, s2, c2;
    AInType u1, u2;
    // hls::x_complex<AInType> A, B;
    std::complex<AInType> A, B;
    CSType cosA_full, sinA_full, cosA_half, sinA_half;
    CSType cosB_full, sinB_full, cosB_half, sinB_half;
    bool A_is_pos_real, A_is_imag;
    bool B_is_pos_real, B_is_imag;
    CSType uw_int, ux_int, uy_int, uz_int;
    CSType vw_int, vx_int, vy_int, vz_int;
    AOutType w_out1, w_out2, z_out1, z_out2, w_out_int, z_out_int;

    // Determine first half angle required to zero off-diagonal values
    u1 = z_in - w_in;
    u2 = y_in + x_in;
    A.imag(u2);
    A.real(u1);
    calc_angle(A, cosA_full, sinA_full, cosA_half, sinA_half, A_is_pos_real, A_is_imag);

    // Determine second half angle
    u1 = z_in + w_in;
    u2 = y_in - x_in;
    B.imag(u2);
    B.real(u1);
    calc_angle(B, cosB_full, sinB_full, cosB_half, sinB_half, B_is_pos_real, B_is_imag);

    // Combine half angles to produce left and right rotations
    // IMPLEMENTATION TIP: There are common products in the following calculations. For parallel implementations these
    // should be shared.
    // Consider in-lining these function calls.
    vm2x1(cosA_half, cosB_half, sinA_half, sinB_half, c1);
    vm2x1(sinA_half, cosB_half, -cosA_half, sinB_half, s1);
    vm2x1(cosA_half, cosB_half, -sinA_half, sinB_half, c2);
    vm2x1(sinA_half, cosB_half, cosA_half, sinB_half, s2);

    // Build full U and V matrix
    uw_int = c1;
    ux_int = s1;
    uy_int = -s1;
    uz_int = c1;

    vw_int = c2;
    vx_int = s2;
    vy_int = -s2;
    vz_int = c2;

    // Apply rotation
    // - Uses the transpose version of U
    // w_out
    vm2x1(w_in, vw_int, x_in, vy_int, w_out1);
    vm2x1(y_in, vw_int, z_in, vy_int, w_out2);
    vm2x1(uw_int, w_out1, uy_int, w_out2, w_out_int);
    // z_out
    vm2x1(w_in, vx_int, x_in, vz_int, z_out1);
    vm2x1(y_in, vx_int, z_in, vz_int, z_out2);
    vm2x1(ux_int, z_out1, uz_int, z_out2, z_out_int);
    x_out = outZERO;
    y_out = outZERO;

    // Ensure singular values are positive
    if (x_isneg(w_out_int)) {
        w_out = -w_out_int;
        vw_int = -c2;
        vy_int = s2;
    } else {
        w_out = w_out_int;
    }
    if (x_isneg(z_out_int)) {
        z_out = -z_out_int;
        vx_int = -s2;
        vz_int = -c2;
    } else {
        z_out = z_out_int;
    }

    // Assign outputs
    uw_out = uw_int;
    ux_out = ux_int;
    uy_out = uy_int;
    uz_out = uz_int;
    vw_out = vw_int;
    vx_out = vx_int;
    vy_out = vy_int;
    vz_out = vz_int;
}

// Complex 2x2 SVD function
// o Calculates several additional rotations to convert the w,x,y & z values to real only before calling the real 2x2
// svd
//   function
template <typename AInType, typename CSType, typename AOutType>
void svd2x2(hls::x_complex<AInType> w_in,
            hls::x_complex<AInType> x_in,
            hls::x_complex<AInType> y_in,
            hls::x_complex<AInType> z_in,
            hls::x_complex<CSType>& uw_out,
            hls::x_complex<CSType>& ux_out,
            hls::x_complex<CSType>& uy_out,
            hls::x_complex<CSType>& uz_out,
            hls::x_complex<CSType>& vw_out,
            hls::x_complex<CSType>& vx_out,
            hls::x_complex<CSType>& vy_out,
            hls::x_complex<CSType>& vz_out,
            hls::x_complex<AOutType>& w_out,
            hls::x_complex<AOutType>& x_out,
            hls::x_complex<AOutType>& y_out,
            hls::x_complex<AOutType>& z_out) {
Function_svd2x2_complex:;
// Inline to bring common lower level functions to this level of hierarchy to simplify the application
// of resource sharing directives.
#pragma HLS inline

    const hls::x_complex<AInType> CMPLX_ZERO = 0;
    const AInType REAL_ZERO = 0;
    const CSType csZERO = 0;

    AInType wMag, xMag, yMag, zMag;
    CSType cosThetaW, sinThetaW, cosThetaWdiv2, sinThetaWdiv2, cosThetaX, sinThetaX, cosThetaXdiv2, sinThetaXdiv2,
        cosThetaY, sinThetaY, cosThetaYdiv2, sinThetaYdiv2, cosThetaZ, sinThetaZ, cosThetaZdiv2, sinThetaZdiv2,
        RotL1_w_re, RotL1_w_im, RotR1_w_re, RotR1_w_im, Rot2_C, Rot2_S, Rot2_Cdiv2, Rot2_Sdiv2, RotL2_w, RotL2_x,
        RotL2_y, RotL2_z, RotL3_w_re, RotL3_w_im, RotR3_w_im, RotL4_w, RotL4_x, RotL4_y, RotL4_z, RotR4_w, RotR4_x,
        RotR4_y, RotR4_z;

    hls::x_complex<CSType> RotL1_w, RotL1_x, RotL1_y, RotL1_z, RotR1_w, RotR1_x, RotR1_y, RotR1_z, RotL12_w, RotL12_x,
        RotL12_y, RotL12_z, RotL3_w, RotL3_x, RotL3_y, RotL3_z, RotR3_w, RotR3_x, RotR3_y, RotR3_z, Rot2_cmplx,
        RotL123_w, RotL123_x, RotL123_y, RotL123_z, RotR13_w, RotR13_x, RotR13_y, RotR13_z, uw_int, ux_int, uy_int,
        uz_int;

    hls::x_complex<AInType> w_int, x_int, x_int1, y_int, z_int, z_int1;
    AInType w_out_re, x_out_re, y_out_re, z_out_re, w_int_re;

    bool w_is_pos_real, w_is_imag, x_is_pos_real, x_is_imag, y_is_pos_real, y_is_imag, z_is_pos_real, z_is_imag,
        tmp_is_pos_real, tmp_is_imag;

    // Determine sin and cos values of W and Y phase angles, ThetaW and ThetaY
    calc_angle(w_in, cosThetaW, sinThetaW, cosThetaWdiv2, sinThetaWdiv2, w_is_pos_real, w_is_imag);
    calc_angle(y_in, cosThetaY, sinThetaY, cosThetaYdiv2, sinThetaYdiv2, y_is_pos_real, y_is_imag);

    // Rotation 1
    // o 2-sided Unitary Complex rotation to make W and Y real
    //        RotL1 = | exp(j*(ThetaY-ThetaW)/2)  0                         |
    //                | 0                         exp(-j*(ThetaY-ThetaW)/2) |
    //        RotR1 = | exp(-j*(ThetaY+ThetaW)/2) 0                         |
    //                |  0                        exp(-j*(ThetaY+ThetaW)/2) |
    // o So
    //   exp(j*(ThetaY-ThetaW)/2)  = cos((ThetaY-ThetaW)/2) + j sin((ThetaY-ThetaW)/2)
    //                             = cos(ThetaY/2)cos(ThetaW/2) + sin(ThetaY/2)*sin(ThetaW/2) + j (
    //                             sin(ThetaY/2)cos(ThetaW/2) - cos(ThetaY/2)sin(ThetaW/2) )
    //   exp(-j*(ThetaY+ThetaW)/2) = cos((ThetaY+ThetaW)/2) - j sin((ThetaY+ThetaW)/2)
    //                             = cos(ThetaY/2)cos(ThetaW/2) - sin(ThetaY/2)*sin(ThetaW/2) - j (
    //                             sin(ThetaY/2)cos(ThetaW/2) + cos(ThetaY/2)sin(ThetaW/2) )
    vm2x1(cosThetaYdiv2, cosThetaWdiv2, sinThetaYdiv2, sinThetaWdiv2, RotL1_w_re);
    vm2x1(sinThetaYdiv2, cosThetaWdiv2, -cosThetaYdiv2, sinThetaWdiv2, RotL1_w_im);
    RotL1_w.real(RotL1_w_re);
    RotL1_w.imag(RotL1_w_im);
    RotL1_x = 0; // Unused
    RotL1_y = 0; // Unused
    RotL1_z = hls::x_conj(RotL1_w);

    // IMPLEMENTATION TIP: The following calls duplicate the multiplies also implemented above. For parallel
    // implementations
    // these should be optimized/inlined.
    vm2x1(cosThetaYdiv2, cosThetaWdiv2, -sinThetaYdiv2, sinThetaWdiv2, RotR1_w_re);
    vm2x1(-sinThetaYdiv2, cosThetaWdiv2, -cosThetaYdiv2, sinThetaWdiv2, RotR1_w_im);
    RotR1_w.real(RotR1_w_re);
    RotR1_w.imag(RotR1_w_im);
    RotR1_x = 0; // Unused
    RotR1_y = 0; // Unused
    RotR1_z = RotR1_w;

    // Rotation 2
    // o 1-sided real Givens rotation to zero Y
    // o Use the magnitudes of W and Y and calculate the sin and the cos of the rotation required to zero Y
    vm2x1(w_in.real(), cosThetaW, w_in.imag(), sinThetaW, wMag);
    vm2x1(y_in.real(), cosThetaY, y_in.imag(), sinThetaY, yMag);

    Rot2_cmplx.real(wMag);
    Rot2_cmplx.imag(yMag);
    calc_angle(Rot2_cmplx, Rot2_C, Rot2_S, Rot2_Cdiv2, Rot2_Sdiv2, tmp_is_pos_real, tmp_is_imag);
    RotL2_w = Rot2_C;
    RotL2_x = Rot2_S;
    RotL2_y = -Rot2_S;
    RotL2_z = Rot2_C;

    // Combine left hand rotations 1 & 2
    // o Using the constant value CMPLX_ZERO to obtain some optimization when the implementation allows.
    // o Note that rotation 2 contains real only values
    mm2x2(RotL2_w, RotL2_x, RotL2_y, RotL2_z, RotL1_w, CMPLX_ZERO, CMPLX_ZERO, RotL1_z, RotL12_w, RotL12_x, RotL12_y,
          RotL12_z);

    // Update w,x,y & z values to reflect rotations
    w_int.imag(0);
    vm2x1(wMag, Rot2_C, yMag, Rot2_S, w_int_re);
    w_int.real(w_int_re);
    y_int = 0;
    vm2x1(x_in, RotL12_w, z_in, RotL12_x, x_int1);
    x_int = x_int1 * RotR1_z;
    vm2x1(x_in, RotL12_y, z_in, RotL12_z, z_int1);
    z_int = z_int1 * RotR1_z;

    // Determine sin and cos values of the updated X and Z phase angles, ThetaX and ThetaZ
    calc_angle(x_int, cosThetaX, sinThetaX, cosThetaXdiv2, sinThetaXdiv2, x_is_pos_real, x_is_imag);
    calc_angle(z_int, cosThetaZ, sinThetaZ, cosThetaZdiv2, sinThetaZdiv2, z_is_pos_real, z_is_imag);

    // Rotation 3
    // o 2 additional 2-sided complex rotations to turn the updated X and Z into real only values.
    // o The 2 rotations are combined into a single 2-sided complex rotation.
    // o The first rotation rotates W and X - Rotation 3a:
    //        RotL3a = | exp(-j*(ThetaX+ThetaW)/2)  0                         |
    //                 | 0                          exp(-j*(ThetaX+ThetaW)/2) |
    //        RotR3a = | exp(j*(ThetaX-ThetaW)/2)   0                         |
    //                 | 0                          exp(-j*(ThetaX-ThetaW)/2) |
    //   - Note ThetaW already equals 0 so the above simplifies to only use ThetaX
    // o The second rotation rotates X and Z - Rotation 3b:
    //        RotL3b = | exp(j*(ThetaZ-ThetaX)/2)   0                         |
    //                 | 0                          exp(-j*(ThetaZ-ThetaX)/2) |
    //        RotR3b = | exp(-j*(ThetaZ+ThetaX)/2)  0                         |
    //                 | 0                          exp(-j*(ThetaZ+ThetaX)/2) |
    //   - Note Following rotation 3a ThetaX equals 0 so this rotation simplifies to use only ThetaZ
    // o Finally we can then combine these 2 rotations into single left and right unitary matrix giving the final
    // rotation we'll use:
    //        RotL3 =  | exp(j*(ThetaZ/2 - ThetaX))  0                  |
    //                 | 0                           exp(j*-(ThetaZ/2)) |
    //        RotR3 =  | exp(j*(ThetaX - ThetaZ/2))  0                  |
    //                 | 0                           exp(j*-(ThetaZ/2)) |
    // o So
    //   exp(j*(ThetaZ/2 - ThetaX)) = cos(ThetaZ/2 - ThetaX) + j sin(ThetaZ/2 - ThetaX)
    //                              = cos(ThetaZ/2)cos(ThetaX) + sin(ThetaZ/2)*sin(ThetaX) + j (
    //                              sin(ThetaZ/2)cos(ThetaX) - cos(ThetaZ/2)sin(ThetaX) )
    //   exp(j*-(ThetaZ/2))         = cos(ThetaZ/2) - j sin(ThetaZ/2)
    //   exp(j*(ThetaX - ThetaZ/2)) = cos(ThetaX - ThetaZ/2) + j sin(ThetaX - ThetaZ/2)
    //                              = cos(ThetaX)cos(ThetaZ/2) + sin(ThetaX)*sin(ThetaZ/2) + j (
    //                              sin(ThetaX)cos(ThetaZ/2) - cos(ThetaX)sin(ThetaZ/2) )
    vm2x1(cosThetaZdiv2, cosThetaX, sinThetaZdiv2, sinThetaX, RotL3_w_re);
    vm2x1(sinThetaZdiv2, cosThetaX, -cosThetaZdiv2, sinThetaX, RotL3_w_im);
    RotL3_w.real(RotL3_w_re);
    RotL3_w.imag(RotL3_w_im);
    RotL3_x = 0; // Unused
    RotL3_y = 0; // Unused
    RotL3_z.real(cosThetaZdiv2);
    RotL3_z.imag(-sinThetaZdiv2);

    RotR3_w.real(RotL3_w.real());
    // IMPLEMENTATION TIP: Below duplicates the multiplies implemented above. For parallel implementations these should
    // be inlined
    vm2x1(cosThetaZdiv2, sinThetaX, -sinThetaZdiv2, cosThetaX, RotR3_w_im);
    RotR3_w.imag(RotR3_w_im);
    RotR3_x = 0; // Unused
    RotR3_y = 0; // Unused
    RotR3_z = RotL3_z;

    // Combine rotation 3 with 2 & 1
    mm2x2(RotL3_w, CMPLX_ZERO, CMPLX_ZERO, RotL3_z, RotL12_w, RotL12_x, RotL12_y, RotL12_z, RotL123_w, RotL123_x,
          RotL123_y, RotL123_z);
    mm2x2(RotR1_w, CMPLX_ZERO, CMPLX_ZERO, RotR1_z, RotR3_w, CMPLX_ZERO, CMPLX_ZERO, RotR3_z, RotR13_w, RotR13_x,
          RotR13_y, RotR13_z);

    // Calculate the magnitudes of X and Z for use in real SVD calculation
    vm2x1(x_int.real(), cosThetaX, x_int.imag(), sinThetaX, xMag);
    vm2x1(z_int.real(), cosThetaZ, z_int.imag(), sinThetaZ, zMag);

    // Call real SVD function
    svd2x2(w_int.real(), xMag, REAL_ZERO, zMag, RotL4_w, RotL4_x, RotL4_y, RotL4_z, RotR4_w, RotR4_x, RotR4_y, RotR4_z,
           w_out_re, x_out_re, y_out_re, z_out_re);

    // Generate and assign outputs
    // o The combined U values (Left rotations) must be Hermitian Transposed.
    // o Note the rotation values output by the SVD function are real only
    w_out = w_out_re;
    x_out = x_out_re; // Zero'ed by the SVD function
    y_out = y_out_re; // Zero'ed by the SVD function
    z_out = z_out_re;

    mm2x2(RotL4_w, RotL4_y, RotL4_x, RotL4_z, RotL123_w, RotL123_x, RotL123_y, RotL123_z, uw_int, ux_int, uy_int,
          uz_int);
    uw_out = hls::x_conj(uw_int);
    ux_out = hls::x_conj(uy_int); // Transposed
    uy_out = hls::x_conj(ux_int); // Transposed
    uz_out = hls::x_conj(uz_int);

    mm2x2(RotR13_w, RotR13_x, RotR13_y, RotR13_z, RotR4_w, RotR4_x, RotR4_y, RotR4_z, vw_out, vx_out, vy_out, vz_out);
}

// Complex 2x2 SVD function
// o Calculates several additional rotations to convert the w,x,y & z values to real only before calling the real 2x2
// svd
//   function
template <typename AInType, typename CSType, typename AOutType>
void svd2x2(std::complex<AInType> w_in,
            std::complex<AInType> x_in,
            std::complex<AInType> y_in,
            std::complex<AInType> z_in,
            std::complex<CSType>& uw_out,
            std::complex<CSType>& ux_out,
            std::complex<CSType>& uy_out,
            std::complex<CSType>& uz_out,
            std::complex<CSType>& vw_out,
            std::complex<CSType>& vx_out,
            std::complex<CSType>& vy_out,
            std::complex<CSType>& vz_out,
            std::complex<AOutType>& w_out,
            std::complex<AOutType>& x_out,
            std::complex<AOutType>& y_out,
            std::complex<AOutType>& z_out) {
Function_svd2x2_complex:;
// Inline to bring common lower level functions to this level of hierarchy to simplify the application
// of resource sharing directives.
#pragma HLS inline

    const std::complex<AInType> CMPLX_ZERO = 0;
    const AInType REAL_ZERO = 0;
    const CSType csZERO = 0;

    AInType wMag, xMag, yMag, zMag;
    CSType cosThetaW, sinThetaW, cosThetaWdiv2, sinThetaWdiv2, cosThetaX, sinThetaX, cosThetaXdiv2, sinThetaXdiv2,
        cosThetaY, sinThetaY, cosThetaYdiv2, sinThetaYdiv2, cosThetaZ, sinThetaZ, cosThetaZdiv2, sinThetaZdiv2,
        RotL1_w_re, RotL1_w_im, RotR1_w_re, RotR1_w_im, Rot2_C, Rot2_S, Rot2_Cdiv2, Rot2_Sdiv2, RotL2_w, RotL2_x,
        RotL2_y, RotL2_z, RotL3_w_re, RotL3_w_im, RotR3_w_im, RotL4_w, RotL4_x, RotL4_y, RotL4_z, RotR4_w, RotR4_x,
        RotR4_y, RotR4_z;

    std::complex<CSType> RotL1_w, RotL1_x, RotL1_y, RotL1_z, RotR1_w, RotR1_x, RotR1_y, RotR1_z, RotL12_w, RotL12_x,
        RotL12_y, RotL12_z, RotL3_w, RotL3_x, RotL3_y, RotL3_z, RotR3_w, RotR3_x, RotR3_y, RotR3_z, Rot2_cmplx,
        RotL123_w, RotL123_x, RotL123_y, RotL123_z, RotR13_w, RotR13_x, RotR13_y, RotR13_z, uw_int, ux_int, uy_int,
        uz_int;

    std::complex<AInType> w_int, x_int, x_int1, y_int, z_int, z_int1;
    AInType w_out_re, x_out_re, y_out_re, z_out_re, w_int_re;

    bool w_is_pos_real, w_is_imag, x_is_pos_real, x_is_imag, y_is_pos_real, y_is_imag, z_is_pos_real, z_is_imag,
        tmp_is_pos_real, tmp_is_imag;

    // Determine sin and cos values of W and Y phase angles, ThetaW and ThetaY
    calc_angle(w_in, cosThetaW, sinThetaW, cosThetaWdiv2, sinThetaWdiv2, w_is_pos_real, w_is_imag);
    calc_angle(y_in, cosThetaY, sinThetaY, cosThetaYdiv2, sinThetaYdiv2, y_is_pos_real, y_is_imag);

    // Rotation 1
    // o 2-sided Unitary Complex rotation to make W and Y real
    //        RotL1 = | exp(j*(ThetaY-ThetaW)/2)  0                         |
    //                | 0                         exp(-j*(ThetaY-ThetaW)/2) |
    //        RotR1 = | exp(-j*(ThetaY+ThetaW)/2) 0                         |
    //                |  0                        exp(-j*(ThetaY+ThetaW)/2) |
    // o So
    //   exp(j*(ThetaY-ThetaW)/2)  = cos((ThetaY-ThetaW)/2) + j sin((ThetaY-ThetaW)/2)
    //                             = cos(ThetaY/2)cos(ThetaW/2) + sin(ThetaY/2)*sin(ThetaW/2) + j (
    //                             sin(ThetaY/2)cos(ThetaW/2) - cos(ThetaY/2)sin(ThetaW/2) )
    //   exp(-j*(ThetaY+ThetaW)/2) = cos((ThetaY+ThetaW)/2) - j sin((ThetaY+ThetaW)/2)
    //                             = cos(ThetaY/2)cos(ThetaW/2) - sin(ThetaY/2)*sin(ThetaW/2) - j (
    //                             sin(ThetaY/2)cos(ThetaW/2) + cos(ThetaY/2)sin(ThetaW/2) )
    vm2x1(cosThetaYdiv2, cosThetaWdiv2, sinThetaYdiv2, sinThetaWdiv2, RotL1_w_re);
    vm2x1(sinThetaYdiv2, cosThetaWdiv2, -cosThetaYdiv2, sinThetaWdiv2, RotL1_w_im);
    RotL1_w.real(RotL1_w_re);
    RotL1_w.imag(RotL1_w_im);
    RotL1_x = 0; // Unused
    RotL1_y = 0; // Unused
    RotL1_z = hls::x_conj(RotL1_w);

    // IMPLEMENTATION TIP: The following calls duplicate the multiplies also implemented above. For parallel
    // implementations
    // these should be optimized/inlined.
    vm2x1(cosThetaYdiv2, cosThetaWdiv2, -sinThetaYdiv2, sinThetaWdiv2, RotR1_w_re);
    vm2x1(-sinThetaYdiv2, cosThetaWdiv2, -cosThetaYdiv2, sinThetaWdiv2, RotR1_w_im);
    RotR1_w.real(RotR1_w_re);
    RotR1_w.imag(RotR1_w_im);
    RotR1_x = 0; // Unused
    RotR1_y = 0; // Unused
    RotR1_z = RotR1_w;

    // Rotation 2
    // o 1-sided real Givens rotation to zero Y
    // o Use the magnitudes of W and Y and calculate the sin and the cos of the rotation required to zero Y
    vm2x1(w_in.real(), cosThetaW, w_in.imag(), sinThetaW, wMag);
    vm2x1(y_in.real(), cosThetaY, y_in.imag(), sinThetaY, yMag);

    Rot2_cmplx.real(wMag);
    Rot2_cmplx.imag(yMag);
    calc_angle(Rot2_cmplx, Rot2_C, Rot2_S, Rot2_Cdiv2, Rot2_Sdiv2, tmp_is_pos_real, tmp_is_imag);
    RotL2_w = Rot2_C;
    RotL2_x = Rot2_S;
    RotL2_y = -Rot2_S;
    RotL2_z = Rot2_C;

    // Combine left hand rotations 1 & 2
    // o Using the constant value CMPLX_ZERO to obtain some optimization when the implementation allows.
    // o Note that rotation 2 contains real only values
    mm2x2(RotL2_w, RotL2_x, RotL2_y, RotL2_z, RotL1_w, CMPLX_ZERO, CMPLX_ZERO, RotL1_z, RotL12_w, RotL12_x, RotL12_y,
          RotL12_z);

    // Update w,x,y & z values to reflect rotations
    w_int.imag(0);
    vm2x1(wMag, Rot2_C, yMag, Rot2_S, w_int_re);
    w_int.real(w_int_re);
    y_int = 0;
    vm2x1(x_in, RotL12_w, z_in, RotL12_x, x_int1);
    x_int = x_int1 * RotR1_z;
    vm2x1(x_in, RotL12_y, z_in, RotL12_z, z_int1);
    z_int = z_int1 * RotR1_z;

    // Determine sin and cos values of the updated X and Z phase angles, ThetaX and ThetaZ
    calc_angle(x_int, cosThetaX, sinThetaX, cosThetaXdiv2, sinThetaXdiv2, x_is_pos_real, x_is_imag);
    calc_angle(z_int, cosThetaZ, sinThetaZ, cosThetaZdiv2, sinThetaZdiv2, z_is_pos_real, z_is_imag);

    // Rotation 3
    // o 2 additional 2-sided complex rotations to turn the updated X and Z into real only values.
    // o The 2 rotations are combined into a single 2-sided complex rotation.
    // o The first rotation rotates W and X - Rotation 3a:
    //        RotL3a = | exp(-j*(ThetaX+ThetaW)/2)  0                         |
    //                 | 0                          exp(-j*(ThetaX+ThetaW)/2) |
    //        RotR3a = | exp(j*(ThetaX-ThetaW)/2)   0                         |
    //                 | 0                          exp(-j*(ThetaX-ThetaW)/2) |
    //   - Note ThetaW already equals 0 so the above simplifies to only use ThetaX
    // o The second rotation rotates X and Z - Rotation 3b:
    //        RotL3b = | exp(j*(ThetaZ-ThetaX)/2)   0                         |
    //                 | 0                          exp(-j*(ThetaZ-ThetaX)/2) |
    //        RotR3b = | exp(-j*(ThetaZ+ThetaX)/2)  0                         |
    //                 | 0                          exp(-j*(ThetaZ+ThetaX)/2) |
    //   - Note Following rotation 3a ThetaX equals 0 so this rotation simplifies to use only ThetaZ
    // o Finally we can then combine these 2 rotations into single left and right unitary matrix giving the final
    // rotation we'll use:
    //        RotL3 =  | exp(j*(ThetaZ/2 - ThetaX))  0                  |
    //                 | 0                           exp(j*-(ThetaZ/2)) |
    //        RotR3 =  | exp(j*(ThetaX - ThetaZ/2))  0                  |
    //                 | 0                           exp(j*-(ThetaZ/2)) |
    // o So
    //   exp(j*(ThetaZ/2 - ThetaX)) = cos(ThetaZ/2 - ThetaX) + j sin(ThetaZ/2 - ThetaX)
    //                              = cos(ThetaZ/2)cos(ThetaX) + sin(ThetaZ/2)*sin(ThetaX) + j (
    //                              sin(ThetaZ/2)cos(ThetaX) - cos(ThetaZ/2)sin(ThetaX) )
    //   exp(j*-(ThetaZ/2))         = cos(ThetaZ/2) - j sin(ThetaZ/2)
    //   exp(j*(ThetaX - ThetaZ/2)) = cos(ThetaX - ThetaZ/2) + j sin(ThetaX - ThetaZ/2)
    //                              = cos(ThetaX)cos(ThetaZ/2) + sin(ThetaX)*sin(ThetaZ/2) + j (
    //                              sin(ThetaX)cos(ThetaZ/2) - cos(ThetaX)sin(ThetaZ/2) )
    vm2x1(cosThetaZdiv2, cosThetaX, sinThetaZdiv2, sinThetaX, RotL3_w_re);
    vm2x1(sinThetaZdiv2, cosThetaX, -cosThetaZdiv2, sinThetaX, RotL3_w_im);
    RotL3_w.real(RotL3_w_re);
    RotL3_w.imag(RotL3_w_im);
    RotL3_x = 0; // Unused
    RotL3_y = 0; // Unused
    RotL3_z.real(cosThetaZdiv2);
    RotL3_z.imag(-sinThetaZdiv2);

    RotR3_w.real(RotL3_w.real());
    // IMPLEMENTATION TIP: Below duplicates the multiplies implemented above. For parallel implementations these should
    // be inlined
    vm2x1(cosThetaZdiv2, sinThetaX, -sinThetaZdiv2, cosThetaX, RotR3_w_im);
    RotR3_w.imag(RotR3_w_im);
    RotR3_x = 0; // Unused
    RotR3_y = 0; // Unused
    RotR3_z = RotL3_z;

    // Combine rotation 3 with 2 & 1
    mm2x2(RotL3_w, CMPLX_ZERO, CMPLX_ZERO, RotL3_z, RotL12_w, RotL12_x, RotL12_y, RotL12_z, RotL123_w, RotL123_x,
          RotL123_y, RotL123_z);
    mm2x2(RotR1_w, CMPLX_ZERO, CMPLX_ZERO, RotR1_z, RotR3_w, CMPLX_ZERO, CMPLX_ZERO, RotR3_z, RotR13_w, RotR13_x,
          RotR13_y, RotR13_z);

    // Calculate the magnitudes of X and Z for use in real SVD calculation
    vm2x1(x_int.real(), cosThetaX, x_int.imag(), sinThetaX, xMag);
    vm2x1(z_int.real(), cosThetaZ, z_int.imag(), sinThetaZ, zMag);

    // Call real SVD function
    svd2x2(w_int.real(), xMag, REAL_ZERO, zMag, RotL4_w, RotL4_x, RotL4_y, RotL4_z, RotR4_w, RotR4_x, RotR4_y, RotR4_z,
           w_out_re, x_out_re, y_out_re, z_out_re);

    // Generate and assign outputs
    // o The combined U values (Left rotations) must be Hermitian Transposed.
    // o Note the rotation values output by the SVD function are real only
    w_out = w_out_re;
    x_out = x_out_re; // Zero'ed by the SVD function
    y_out = y_out_re; // Zero'ed by the SVD function
    z_out = z_out_re;

    mm2x2(RotL4_w, RotL4_y, RotL4_x, RotL4_z, RotL123_w, RotL123_x, RotL123_y, RotL123_z, uw_int, ux_int, uy_int,
          uz_int);
    uw_out = hls::x_conj(uw_int);
    ux_out = hls::x_conj(uy_int); // Transposed
    uy_out = hls::x_conj(ux_int); // Transposed
    uz_out = hls::x_conj(uz_int);

    mm2x2(RotR13_w, RotR13_x, RotR13_y, RotR13_z, RotR4_w, RotR4_x, RotR4_y, RotR4_z, vw_out, vx_out, vy_out, vz_out);
}

// ===================================================================================================================
// SVD_BASIC: Top level function taking a SVDTraits template parameter to defines internal types
template <int RowsA, int ColsA, class SVDTraits, typename InputType, typename OutputType>
void svdBasic(const InputType A[RowsA][ColsA],
              OutputType S[RowsA][ColsA],
              OutputType U[RowsA][RowsA],
              OutputType V[ColsA][ColsA]) {
    // Initially only supporting square matrix
    assert(RowsA == ColsA);

    // Internal memories for partial results
    typename SVDTraits::SIntType s_in[RowsA][ColsA];
    typename SVDTraits::UIntType u_in[RowsA][ColsA];
    typename SVDTraits::VIntType v_in[RowsA][ColsA];

    // Current S,U,V values being worked on
    typename SVDTraits::SIntType w_in, x_in, y_in, z_in;
    typename SVDTraits::SIntType w_out, x_out, y_out, z_out;
    typename SVDTraits::UIntType uw_in, ux_in, uy_in, uz_in;
    typename SVDTraits::UIntType uw_out, ux_out, uy_out, uz_out;
    typename SVDTraits::VIntType vw_in, vx_in, vy_in, vz_in;
    typename SVDTraits::VIntType vw_out, vx_out, vy_out, vz_out;

    // 2x2 Rotation values
    typename SVDTraits::CSIntType uw_new, ux_new, uy_new, uz_new;
    typename SVDTraits::CSIntType vw_new, vx_new, vy_new, vz_new;

sweep_loop:
    for (int sweepnum = 0; sweepnum < SVDTraits::NUM_SWEEPS; sweepnum++) {
    // NOTE: Using the minimum dimension. i.e. will process a square matrix
    row_loop:
        for (int top_left = 0; top_left < SVDTraits::MIN_DIM; top_left++) {
        col_loop:
            for (int bottom_right = top_left + 1; bottom_right < SVDTraits::MIN_DIM; bottom_right++) {
                // Fetch w,x,y,z values
                if (sweepnum == 0 && top_left == 0) {
                    if (bottom_right == 1) {
                        w_in = A[top_left][top_left];
                        x_in = A[top_left][bottom_right];
                        y_in = A[bottom_right][top_left];
                    } else {
                        // Now revist values already updated in first diagonal pass
                        w_in = s_in[top_left][top_left];
                        x_in = s_in[top_left][bottom_right];
                        y_in = s_in[bottom_right][top_left];
                    }
                    z_in = A[bottom_right][bottom_right];
                } else {
                    w_in = s_in[top_left][top_left];
                    x_in = s_in[top_left][bottom_right];
                    y_in = s_in[bottom_right][top_left];
                    z_in = s_in[bottom_right][bottom_right];
                }

                // Diagonal
                svd2x2(w_in, x_in, y_in, z_in, uw_new, ux_new, uy_new, uz_new, vw_new, vx_new, vy_new, vz_new, w_out,
                       x_out, y_out, z_out);

                // Update S on diagonal
                s_in[top_left][top_left] = w_out;
                s_in[top_left][bottom_right] = x_out;
                s_in[bottom_right][top_left] = y_out;
                s_in[bottom_right][bottom_right] = z_out;
                if (sweepnum == SVDTraits::NUM_SWEEPS - 1) {
                    S[top_left][top_left] = w_out;
                    S[top_left][bottom_right] = x_out;
                    S[bottom_right][top_left] = y_out;
                    S[bottom_right][bottom_right] = z_out;
                }

                // Update U & V
                // o On the diagonal use a 2x2 as per the sigma
                // o Need to create the indentity in U & V at the start
                if (sweepnum == 0 && top_left == 0) {
                    if (bottom_right == 1) {
                        uw_in = 1;
                        vw_in = 1;
                    } else {
                        // Now re-visiting diagonal values where I has been set
                        uw_in = u_in[top_left][top_left];
                        vw_in = v_in[top_left][top_left];
                    }

                    ux_in = 0;
                    uy_in = 0;
                    uz_in = 1;

                    vx_in = 0;
                    vy_in = 0;
                    vz_in = 1;
                } else {
                    uw_in = u_in[top_left][top_left];
                    ux_in = u_in[top_left][bottom_right];
                    uy_in = u_in[bottom_right][top_left];
                    uz_in = u_in[bottom_right][bottom_right];
                    vw_in = v_in[top_left][top_left];
                    vx_in = v_in[top_left][bottom_right];
                    vy_in = v_in[bottom_right][top_left];
                    vz_in = v_in[bottom_right][bottom_right];
                }

                mm2x2(uw_in, ux_in, uy_in, uz_in, uw_new, ux_new, uy_new, uz_new, uw_out, ux_out, uy_out, uz_out);
                mm2x2(vw_in, vx_in, vy_in, vz_in, vw_new, vx_new, vy_new, vz_new, vw_out, vx_out, vy_out, vz_out);

                u_in[top_left][top_left] = uw_out;
                u_in[top_left][bottom_right] = ux_out;
                u_in[bottom_right][top_left] = uy_out;
                u_in[bottom_right][bottom_right] = uz_out;
                v_in[top_left][top_left] = vw_out;
                v_in[top_left][bottom_right] = vx_out;
                v_in[bottom_right][top_left] = vy_out;
                v_in[bottom_right][bottom_right] = vz_out;
                if (sweepnum == SVDTraits::NUM_SWEEPS - 1) {
                    U[top_left][top_left] = uw_out;
                    U[top_left][bottom_right] = ux_out;
                    U[bottom_right][top_left] = uy_out;
                    U[bottom_right][bottom_right] = uz_out;
                    V[top_left][top_left] = vw_out;
                    V[top_left][bottom_right] = vx_out;
                    V[bottom_right][top_left] = vy_out;
                    V[bottom_right][bottom_right] = vz_out;
                }

            // Off-diagonal
            // Col updates
            off_col:
                for (int off_col = 0; off_col < SVDTraits::MIN_DIM; off_col++) {
#pragma HLS PIPELINE II = SVDTraits::OFF_DIAG_II
                    if (off_col != bottom_right && off_col != top_left) {
                        if (sweepnum == 0 && top_left == 0 && bottom_right == 1) {
                            w_in = A[top_left][off_col];
                        } else {
                            w_in = s_in[top_left][off_col];
                        }
                        if (sweepnum == 0 && top_left == 0 && off_col > bottom_right) {
                            y_in = A[bottom_right][off_col];
                        } else {
                            y_in = s_in[bottom_right][off_col];
                        }

                        // U must be Hermitian transposed before it is applied to A
                        vm2x1(hls::x_conj(uw_new), w_in, hls::x_conj(uy_new), y_in, w_out);
                        vm2x1(hls::x_conj(ux_new), w_in, hls::x_conj(uz_new), y_in, y_out);

                        // Store off-diagonal updates
                        s_in[top_left][off_col] = w_out;
                        s_in[bottom_right][off_col] = y_out;
                        if (sweepnum == SVDTraits::NUM_SWEEPS - 1) {
                            S[top_left][off_col] = w_out;
                            S[bottom_right][off_col] = y_out;
                        }
                    }
                }
            // Row update
            off_row:
                for (int off_row = 0; off_row < SVDTraits::MIN_DIM; off_row++) {
#pragma HLS PIPELINE II = SVDTraits::OFF_DIAG_II
                    if (off_row != bottom_right && off_row != top_left) {
                        if (sweepnum == 0 && top_left == 0 && bottom_right == 1) {
                            w_in = A[off_row][top_left];
                            vw_in = 0;
                            uw_in = 0;
                        } else {
                            w_in = s_in[off_row][top_left];
                            vw_in = v_in[off_row][top_left];
                            uw_in = u_in[off_row][top_left];
                        }
                        if (sweepnum == 0 && top_left == 0 && off_row > bottom_right) {
                            x_in = A[off_row][bottom_right];
                        } else {
                            x_in = s_in[off_row][bottom_right];
                        }
                        if (sweepnum == 0 && top_left == 0) {
                            vx_in = 0;
                            ux_in = 0;
                        } else {
                            vx_in = v_in[off_row][bottom_right];
                            ux_in = u_in[off_row][bottom_right];
                        }

                        vm2x1(w_in, vw_new, x_in, vy_new, w_out);
                        vm2x1(w_in, vx_new, x_in, vz_new, x_out);

                        vm2x1(vw_in, vw_new, vx_in, vy_new, vw_out);
                        vm2x1(vw_in, vx_new, vx_in, vz_new, vx_out);

                        vm2x1(uw_in, uw_new, ux_in, uy_new, uw_out);
                        vm2x1(uw_in, ux_new, ux_in, uz_new, ux_out);

                        // Store off-diagonal updates
                        s_in[off_row][top_left] = w_out;
                        s_in[off_row][bottom_right] = x_out;
                        v_in[off_row][top_left] = vw_out;
                        v_in[off_row][bottom_right] = vx_out;
                        u_in[off_row][top_left] = uw_out;
                        u_in[off_row][bottom_right] = ux_out;
                        if (sweepnum == SVDTraits::NUM_SWEEPS - 1) {
                            S[off_row][top_left] = w_out;
                            S[off_row][bottom_right] = x_out;
                            V[off_row][top_left] = vw_out;
                            V[off_row][bottom_right] = vx_out;
                            U[off_row][top_left] = uw_out;
                            U[off_row][bottom_right] = ux_out;
                        }
                    }
                }
            }
        }
    }
}

// ===================================================================================================================
// SVD_PAIRS: Alternative architecture with improved parallelization/pipelining at the expense of higher resource
// requirements.
// - Processes the input matrix by pairs of columns.
// - IMPLEMENTATION TIP: If a fully unrolled implementation is required partion the function's input/output argument
// arrays on the column
//   dimension.
template <int RowsA, int ColsA, class SVDTraits, typename InputType, typename OutputType>
void svdPairs(const InputType A[RowsA][ColsA],
              OutputType S[RowsA][ColsA],
              OutputType U[RowsA][RowsA],
              OutputType V[ColsA][ColsA]) {
    // Initially only supporting square matrix
    assert(RowsA == ColsA);

    const int ODD_DIM = (SVDTraits::MIN_DIM % 2 == 1 ? 1 : 0);

    // The number of diagonal/column processors used in one step. In a single step all rows/columns of the input array
    // will be processed.
    const int NUM_PROCESSORS = (SVDTraits::MIN_DIM + 1) / 2;

    // The number of steps required to process the whole input matrix for one sweep iteration.
    const int NUM_STEPS = SVDTraits::MIN_DIM + ODD_DIM - 1;

    // Defines the number of banks (or pages) of the internal memory.
    // - The step_loop reads from one page and writes to the other,  ping pongs between two pages for each
    // iteration/step.
    const int BANKS = 2;

    // Stores the current indexes being process by each columns/diagonal processor. The "pairs".
    int diag1_i[BANKS][NUM_PROCESSORS];
    int diag2_i[BANKS][NUM_PROCESSORS];

    int d1_i, d2_i, c1_i, c2_i;
    bool col_swap = false;

    // Internal memories for partial results
    typename SVDTraits::SIntType s_col1[NUM_PROCESSORS][BANKS][RowsA + ODD_DIM];
    typename SVDTraits::SIntType s_col2[NUM_PROCESSORS][BANKS][RowsA + ODD_DIM];
    typename SVDTraits::UIntType u_col1[NUM_PROCESSORS][BANKS][RowsA + ODD_DIM];
    typename SVDTraits::UIntType u_col2[NUM_PROCESSORS][BANKS][RowsA + ODD_DIM];
    typename SVDTraits::UIntType v_col1[NUM_PROCESSORS][BANKS][RowsA + ODD_DIM];
    typename SVDTraits::UIntType v_col2[NUM_PROCESSORS][BANKS][RowsA + ODD_DIM];

    // Current S,U,V values being worked on, used in each column processor
    typename SVDTraits::SIntType w_in, x_in, y_in, z_in;
    typename SVDTraits::SIntType w_int1, x_int1, y_int1, z_int1;
    typename SVDTraits::SIntType w_int2, x_int2, y_int2, z_int2;
    typename SVDTraits::SIntType w_out, x_out, y_out, z_out;
    typename SVDTraits::UIntType uw_in, ux_in, uy_in, uz_in;
    typename SVDTraits::UIntType uw_out, ux_out, uy_out, uz_out;
    typename SVDTraits::VIntType vw_in, vx_in, vy_in, vz_in;
    typename SVDTraits::VIntType vw_out, vx_out, vy_out, vz_out;

    typename SVDTraits::SIntType w_out_sel, x_out_sel, y_out_sel, z_out_sel;
    typename SVDTraits::UIntType uw_out_sel, ux_out_sel, uy_out_sel, uz_out_sel;
    typename SVDTraits::VIntType vw_out_sel, vx_out_sel, vy_out_sel, vz_out_sel;

    // Diagonal processor results - 2x2 Rotation values (U &V) and S
    typename SVDTraits::SIntType diag_w_out[NUM_PROCESSORS];
    typename SVDTraits::SIntType diag_x_out[NUM_PROCESSORS];
    typename SVDTraits::SIntType diag_y_out[NUM_PROCESSORS];
    typename SVDTraits::SIntType diag_z_out[NUM_PROCESSORS];
    typename SVDTraits::CSIntType uw_new[NUM_PROCESSORS];
    typename SVDTraits::CSIntType ux_new[NUM_PROCESSORS];
    typename SVDTraits::CSIntType uy_new[NUM_PROCESSORS];
    typename SVDTraits::CSIntType uz_new[NUM_PROCESSORS];
    typename SVDTraits::CSIntType vw_new[NUM_PROCESSORS];
    typename SVDTraits::CSIntType vx_new[NUM_PROCESSORS];
    typename SVDTraits::CSIntType vy_new[NUM_PROCESSORS];
    typename SVDTraits::CSIntType vz_new[NUM_PROCESSORS];
    // Local copies to avoid multiple reads
    typename SVDTraits::CSIntType uw_new_px, ux_new_px, uy_new_px, uz_new_px;
    typename SVDTraits::CSIntType vw_new_px, vx_new_px, vy_new_px, vz_new_px;

// IMPLEMENTATION TIP: Additional directives for fully unrolling
// #pragma HLS ARRAY_PARTITION variable=diag1_i cyclic dim=2 factor=SVDTraits::UNROLL_FACTOR
// #pragma HLS ARRAY_PARTITION variable=diag2_i cyclic dim=2 factor=SVDTraits::UNROLL_FACTOR
// #pragma HLS ARRAY_PARTITION variable=s_col1 cyclic dim=1 factor=SVDTraits::UNROLL_FACTOR
// #pragma HLS ARRAY_PARTITION variable=s_col2 cyclic dim=1 factor=SVDTraits::UNROLL_FACTOR
// #pragma HLS ARRAY_PARTITION variable=u_col1 cyclic dim=1 factor=SVDTraits::UNROLL_FACTOR
// #pragma HLS ARRAY_PARTITION variable=u_col2 cyclic dim=1 factor=SVDTraits::UNROLL_FACTOR
// #pragma HLS ARRAY_PARTITION variable=v_col1 cyclic dim=1 factor=SVDTraits::UNROLL_FACTOR
// #pragma HLS ARRAY_PARTITION variable=v_col2 cyclic dim=1 factor=SVDTraits::UNROLL_FACTOR
// #pragma HLS ARRAY_PARTITION variable=diag_w_out cyclic dim=1 factor=SVDTraits::UNROLL_FACTOR
// #pragma HLS ARRAY_PARTITION variable=diag_x_out cyclic dim=1 factor=SVDTraits::UNROLL_FACTOR
// #pragma HLS ARRAY_PARTITION variable=diag_y_out cyclic dim=1 factor=SVDTraits::UNROLL_FACTOR
// #pragma HLS ARRAY_PARTITION variable=diag_z_out cyclic dim=1 factor=SVDTraits::UNROLL_FACTOR
// #pragma HLS ARRAY_PARTITION variable=uw_new cyclic dim=1 factor=SVDTraits::UNROLL_FACTOR
// #pragma HLS ARRAY_PARTITION variable=ux_new cyclic dim=1 factor=SVDTraits::UNROLL_FACTOR
// #pragma HLS ARRAY_PARTITION variable=uy_new cyclic dim=1 factor=SVDTraits::UNROLL_FACTOR
// #pragma HLS ARRAY_PARTITION variable=uz_new cyclic dim=1 factor=SVDTraits::UNROLL_FACTOR
// #pragma HLS ARRAY_PARTITION variable=vw_new cyclic dim=1 factor=SVDTraits::UNROLL_FACTOR
// #pragma HLS ARRAY_PARTITION variable=vx_new cyclic dim=1 factor=SVDTraits::UNROLL_FACTOR
// #pragma HLS ARRAY_PARTITION variable=vy_new cyclic dim=1 factor=SVDTraits::UNROLL_FACTOR
// #pragma HLS ARRAY_PARTITION variable=vz_new cyclic dim=1 factor=SVDTraits::UNROLL_FACTOR

// Load column memories
// - Up-front transfer but can be pipeled/data flow
col_load:
    for (int col = 0; col < NUM_PROCESSORS; col++) {
        diag1_i[0][col] = 2 * col;
        diag2_i[0][col] = 2 * col + 1;
    row_load:
        for (int row = 0; row < RowsA + ODD_DIM; row++) {
#pragma HLS PIPELINE
            if (ODD_DIM) {
                // When odd dimensions the first column of the first processor is padded with 0's
                if (row == 0) {
                    s_col1[col][0][row] = 0;
                    s_col2[col][0][row] = 0;
                } else {
                    if (col == 0) {
                        s_col1[col][0][row] = 0;
                    } else {
                        s_col1[col][0][row] = A[row - 1][2 * col - 1];
                    }
                    s_col2[col][0][row] = A[row - 1][2 * col];
                }
            } else {
                s_col1[col][0][row] = A[row][2 * col];
                s_col2[col][0][row] = A[row][2 * col + 1];
            }
        }
    }

sweep_loop:
    for (int sweepnum = 0; sweepnum < SVDTraits::NUM_SWEEPS; sweepnum++) {
    step_loop:
        for (int step = 0; step < NUM_STEPS; step++) {
            const int INPUT_BANK = ((sweepnum * NUM_STEPS) + step) % BANKS;
            const int OUTPUT_BANK = ((sweepnum * NUM_STEPS) + step + 1) % BANKS;

        // Indepent loop to remove any dependency on the processor loops
        diag_index_update:
            for (int px = 0; px < NUM_PROCESSORS; px++) {
                // #pragma HLS UNROLL FACTOR = SVDTraits::UNROLL_FACTOR
                if (px == 0) {
                    diag1_i[OUTPUT_BANK][px] = diag1_i[INPUT_BANK][px]; // Unchanged
                    diag1_i[OUTPUT_BANK][px + 1] = diag2_i[INPUT_BANK][px];
                } else if (px == NUM_PROCESSORS - 1) {
                    diag2_i[OUTPUT_BANK][px] = diag1_i[INPUT_BANK][px];
                    diag2_i[OUTPUT_BANK][px - 1] = diag2_i[INPUT_BANK][px];
                } else {
                    diag1_i[OUTPUT_BANK][px + 1] = diag1_i[INPUT_BANK][px];
                    diag2_i[OUTPUT_BANK][px - 1] = diag2_i[INPUT_BANK][px];
                }
            }

        // Diagonal processor loop
        // - In a separeate loop as we need the rotations from each diagonal processor before updating the
        //   off diagonal values
        diag_px:
            for (int px = 0; px < NUM_PROCESSORS; px++) {
// IMPLEMENTATION TIP: Additional directives for fully unrolling
// #pragma HLS UNROLL FACTOR = SVDTraits::DIAG_UNROLL_FACTOR
#pragma HLS PIPELINE II = SVDTraits::DIAG_II
                if (diag1_i[INPUT_BANK][px] < diag2_i[INPUT_BANK][px]) {
                    d1_i = diag1_i[INPUT_BANK][px];
                    d2_i = diag2_i[INPUT_BANK][px];
                    col_swap = false;
                } else {
                    d2_i = diag1_i[INPUT_BANK][px];
                    d1_i = diag2_i[INPUT_BANK][px];
                    col_swap = true;
                }
                // Fetch diagonal inputs; w, x, y, z
                if (!col_swap) {
                    w_in = s_col1[px][INPUT_BANK][d1_i];
                    x_in = s_col2[px][INPUT_BANK][d1_i];
                    y_in = s_col1[px][INPUT_BANK][d2_i];
                    z_in = s_col2[px][INPUT_BANK][d2_i];
                } else {
                    w_in = s_col2[px][INPUT_BANK][d1_i];
                    x_in = s_col1[px][INPUT_BANK][d1_i];
                    y_in = s_col2[px][INPUT_BANK][d2_i];
                    z_in = s_col1[px][INPUT_BANK][d2_i];
                }
                // Diagonal processor
                if (ODD_DIM && px == 0) {
                    // Column 0 is padded with zeros. w and y for this diagonal group will be 0 so pass through
                    diag_w_out[px] = w_in;
                    diag_x_out[px] = x_in;
                    diag_y_out[px] = y_in;
                    diag_z_out[px] = z_in;
                    uw_new[px] = 1;
                    ux_new[px] = 0;
                    uy_new[px] = 0;
                    uz_new[px] = 1;
                    vw_new[px] = 1;
                    vx_new[px] = 0;
                    vy_new[px] = 0;
                    vz_new[px] = 1;
                } else {
                    svd2x2(w_in, x_in, y_in, z_in, uw_new[px], ux_new[px], uy_new[px], uz_new[px], vw_new[px],
                           vx_new[px], vy_new[px], vz_new[px], diag_w_out[px], diag_x_out[px], diag_y_out[px],
                           diag_z_out[px]);
                }
            }

        // Off-diagonal processors
        off_diag_px:
            for (int px = 0; px < NUM_PROCESSORS; px++) {
                // IMPLEMENTATION TIP: Additional directives for fully unrolling
                // #pragma HLS UNROLL FACTOR = SVDTraits::UNROLL_FACTOR
                if (diag1_i[INPUT_BANK][px] < diag2_i[INPUT_BANK][px]) {
                    c1_i = diag1_i[INPUT_BANK][px];
                    c2_i = diag2_i[INPUT_BANK][px];
                    col_swap = false;
                } else {
                    c2_i = diag1_i[INPUT_BANK][px];
                    c1_i = diag2_i[INPUT_BANK][px];
                    col_swap = true;
                }
                // Fetch new U rotations for this column to avoid multiple access to the uX_new arrays
                uw_new_px = uw_new[px];
                ux_new_px = ux_new[px];
                uy_new_px = uy_new[px];
                uz_new_px = uz_new[px];
            off_diag_loop:
                for (int off_px = 0; off_px < NUM_PROCESSORS; off_px++) {
#pragma HLS PIPELINE II = SVDTraits::OFF_DIAG_II
// Inline mm2x2 function to enable resource sharing
#pragma HLS INLINE recursive
                    // Additional pragmas sometime required when HLS is unable to identifiy that there are no inter loop
                    // dependencies.
                    // o HLS can identify false dependencies on the column memories between loops. All the outputs in
                    // the
                    //   loop are written to a different memory "bank"/region using the INPUT_BANK and OUTPUT_BANK
                    //   indices
                    // #pragma HLS dependence variable=s_col1 inter false
                    // #pragma HLS dependence variable=s_col2 inter false
                    // #pragma HLS dependence variable=u_col1 inter false
                    // #pragma HLS dependence variable=u_col2 inter false
                    // #pragma HLS dependence variable=v_col1 inter false
                    // #pragma HLS dependence variable=v_col2 inter false

                    // Determine index to use
                    if (diag1_i[INPUT_BANK][off_px] < diag2_i[INPUT_BANK][off_px]) {
                        d1_i = diag1_i[INPUT_BANK][off_px];
                        d2_i = diag2_i[INPUT_BANK][off_px];
                    } else {
                        d2_i = diag1_i[INPUT_BANK][off_px];
                        d1_i = diag2_i[INPUT_BANK][off_px];
                    }
                    // Fetch stored U and V values
                    if (sweepnum == 0 && step == 0) {
                        // First sweep construct identity
                        if (px == off_px) {
                            uw_in = 1;
                            ux_in = 0;
                            uy_in = 0;
                            uz_in = 1;
                            vw_in = 1;
                            vx_in = 0;
                            vy_in = 0;
                            vz_in = 1;
                        } else {
                            uw_in = 0;
                            ux_in = 0;
                            uy_in = 0;
                            uz_in = 0;
                            vw_in = 0;
                            vx_in = 0;
                            vy_in = 0;
                            vz_in = 0;
                        }
                    } else {
                        // Using the diagonal indexes of the other column processors
                        if (!col_swap) {
                            uw_in = u_col1[px][INPUT_BANK][d1_i];
                            ux_in = u_col2[px][INPUT_BANK][d1_i];
                            uy_in = u_col1[px][INPUT_BANK][d2_i];
                            uz_in = u_col2[px][INPUT_BANK][d2_i];
                            vw_in = v_col1[px][INPUT_BANK][d1_i];
                            vx_in = v_col2[px][INPUT_BANK][d1_i];
                            vy_in = v_col1[px][INPUT_BANK][d2_i];
                            vz_in = v_col2[px][INPUT_BANK][d2_i];
                        } else {
                            uw_in = u_col2[px][INPUT_BANK][d1_i];
                            ux_in = u_col1[px][INPUT_BANK][d1_i];
                            uy_in = u_col2[px][INPUT_BANK][d2_i];
                            uz_in = u_col1[px][INPUT_BANK][d2_i];
                            vw_in = v_col2[px][INPUT_BANK][d1_i];
                            vx_in = v_col1[px][INPUT_BANK][d1_i];
                            vy_in = v_col2[px][INPUT_BANK][d2_i];
                            vz_in = v_col1[px][INPUT_BANK][d2_i];
                        }
                    }
                    // Fetch S values when not overlapping with this columns diagonal
                    if (off_px != px) {
                        if (!col_swap) {
                            w_in = s_col1[px][INPUT_BANK][d1_i];
                            x_in = s_col2[px][INPUT_BANK][d1_i];
                            y_in = s_col1[px][INPUT_BANK][d2_i];
                            z_in = s_col2[px][INPUT_BANK][d2_i];
                        } else {
                            w_in = s_col2[px][INPUT_BANK][d1_i];
                            x_in = s_col1[px][INPUT_BANK][d1_i];
                            y_in = s_col2[px][INPUT_BANK][d2_i];
                            z_in = s_col1[px][INPUT_BANK][d2_i];
                        }
                    } else {
                        // Otherwise use values output by diagonal processor
                        w_in = diag_w_out[px];
                        x_in = diag_x_out[px];
                        y_in = diag_y_out[px];
                        z_in = diag_z_out[px];
                    }

                    // Update S values
                    if (off_px < px) {
                        // S values must be pre-multiplied with earlier columns new U rotation values before
                        // the V rotations due to this columns processor are applied

                        // U must be Hermitian transposed before it is applied to S
                        mm2x2(hls::x_conj(uw_new[off_px]), hls::x_conj(uy_new[off_px]), hls::x_conj(ux_new[off_px]),
                              hls::x_conj(uz_new[off_px]), w_in, x_in, y_in, z_in, w_int1, x_int1, y_int1, z_int1);
                    } else {
                        w_int1 = w_in;
                        x_int1 = x_in;
                        y_int1 = y_in;
                        z_int1 = z_in;
                    }

                    if (off_px != px) {
                        // Non-diagonal S values must be post-multiplied with V rotations
                        mm2x2(w_int1, x_int1, y_int1, z_int1, vw_new[px], vx_new[px], vy_new[px], vz_new[px], w_int2,
                              x_int2, y_int2, z_int2);
                    } else {
                        w_int2 = w_int1;
                        x_int2 = x_int1;
                        y_int2 = y_int1;
                        z_int2 = z_int1;
                    }

                    if (off_px > px) {
                        // S values must be post-multiplied with subsequent columns new U rotation values after
                        // the V rotations due to this columns processor are applied

                        // U must be Hermitian transposed before it is applied to S
                        mm2x2(hls::x_conj(uw_new[off_px]), hls::x_conj(uy_new[off_px]), hls::x_conj(ux_new[off_px]),
                              hls::x_conj(uz_new[off_px]), w_int2, x_int2, y_int2, z_int2, w_out, x_out, y_out, z_out);
                    } else {
                        w_out = w_int2;
                        x_out = x_int2;
                        y_out = y_int2;
                        z_out = z_int2;
                    }

                    // Update U and V values
                    mm2x2(uw_in, ux_in, uy_in, uz_in, uw_new_px, ux_new_px, uy_new_px, uz_new_px, uw_out, ux_out,
                          uy_out, uz_out);
                    mm2x2(vw_in, vx_in, vy_in, vz_in, vw_new[px], vx_new[px], vy_new[px], vz_new[px], vw_out, vx_out,
                          vy_out, vz_out);

                    // If the current indices for this column have required the column sources to be swapped we need to
                    // do the
                    // same on the output
                    if (!col_swap) {
                        w_out_sel = w_out;
                        x_out_sel = x_out;
                        y_out_sel = y_out;
                        z_out_sel = z_out;

                        uw_out_sel = uw_out;
                        ux_out_sel = ux_out;
                        uy_out_sel = uy_out;
                        uz_out_sel = uz_out;

                        vw_out_sel = vw_out;
                        vx_out_sel = vx_out;
                        vy_out_sel = vy_out;
                        vz_out_sel = vz_out;
                    } else {
                        w_out_sel = x_out;
                        x_out_sel = w_out;
                        y_out_sel = z_out;
                        z_out_sel = y_out;

                        uw_out_sel = ux_out;
                        ux_out_sel = uw_out;
                        uy_out_sel = uz_out;
                        uz_out_sel = uy_out;

                        vw_out_sel = vx_out;
                        vx_out_sel = vw_out;
                        vy_out_sel = vz_out;
                        vz_out_sel = vy_out;
                    }

                    // Store updated values in the correct column memory
                    if (px == 0) {
                        s_col1[px][OUTPUT_BANK][d1_i] = w_out_sel;
                        s_col1[px + 1][OUTPUT_BANK][d1_i] = x_out_sel;
                        s_col1[px][OUTPUT_BANK][d2_i] = y_out_sel;
                        s_col1[px + 1][OUTPUT_BANK][d2_i] = z_out_sel;

                        u_col1[px][OUTPUT_BANK][d1_i] = uw_out_sel;
                        u_col1[px + 1][OUTPUT_BANK][d1_i] = ux_out_sel;
                        u_col1[px][OUTPUT_BANK][d2_i] = uy_out_sel;
                        u_col1[px + 1][OUTPUT_BANK][d2_i] = uz_out_sel;

                        v_col1[px][OUTPUT_BANK][d1_i] = vw_out_sel;
                        v_col1[px + 1][OUTPUT_BANK][d1_i] = vx_out_sel;
                        v_col1[px][OUTPUT_BANK][d2_i] = vy_out_sel;
                        v_col1[px + 1][OUTPUT_BANK][d2_i] = vz_out_sel;
                    } else if (px == NUM_PROCESSORS - 1) {
                        s_col2[px][OUTPUT_BANK][d1_i] = w_out_sel;
                        s_col2[px - 1][OUTPUT_BANK][d1_i] = x_out_sel;
                        s_col2[px][OUTPUT_BANK][d2_i] = y_out_sel;
                        s_col2[px - 1][OUTPUT_BANK][d2_i] = z_out_sel;

                        u_col2[px][OUTPUT_BANK][d1_i] = uw_out_sel;
                        u_col2[px - 1][OUTPUT_BANK][d1_i] = ux_out_sel;
                        u_col2[px][OUTPUT_BANK][d2_i] = uy_out_sel;
                        u_col2[px - 1][OUTPUT_BANK][d2_i] = uz_out_sel;

                        v_col2[px][OUTPUT_BANK][d1_i] = vw_out_sel;
                        v_col2[px - 1][OUTPUT_BANK][d1_i] = vx_out_sel;
                        v_col2[px][OUTPUT_BANK][d2_i] = vy_out_sel;
                        v_col2[px - 1][OUTPUT_BANK][d2_i] = vz_out_sel;
                    } else {
                        s_col1[px + 1][OUTPUT_BANK][d1_i] = w_out_sel;
                        s_col2[px - 1][OUTPUT_BANK][d1_i] = x_out_sel;
                        s_col1[px + 1][OUTPUT_BANK][d2_i] = y_out_sel;
                        s_col2[px - 1][OUTPUT_BANK][d2_i] = z_out_sel;

                        u_col1[px + 1][OUTPUT_BANK][d1_i] = uw_out_sel;
                        u_col2[px - 1][OUTPUT_BANK][d1_i] = ux_out_sel;
                        u_col1[px + 1][OUTPUT_BANK][d2_i] = uy_out_sel;
                        u_col2[px - 1][OUTPUT_BANK][d2_i] = uz_out_sel;

                        v_col1[px + 1][OUTPUT_BANK][d1_i] = vw_out_sel;
                        v_col2[px - 1][OUTPUT_BANK][d1_i] = vx_out_sel;
                        v_col1[px + 1][OUTPUT_BANK][d2_i] = vy_out_sel;
                        v_col2[px - 1][OUTPUT_BANK][d2_i] = vz_out_sel;
                    }
                }
            }
        }
    }
// Transfer results
col_store:
    for (int col = 0; col < NUM_PROCESSORS; col++) {
    row_store:
        for (int row = 0; row < RowsA; row++) {
#pragma HLS PIPELINE
            if (ODD_DIM) {
                if (col != 0) {
                    S[row][2 * col - 1] = s_col1[col][(SVDTraits::NUM_SWEEPS * (NUM_STEPS)) % BANKS][row + 1];
                    U[row][2 * col - 1] = u_col1[col][(SVDTraits::NUM_SWEEPS * (NUM_STEPS)) % BANKS][row + 1];
                    V[row][2 * col - 1] = v_col1[col][(SVDTraits::NUM_SWEEPS * (NUM_STEPS)) % BANKS][row + 1];
                }
                S[row][2 * col] = s_col2[col][(SVDTraits::NUM_SWEEPS * (NUM_STEPS)) % BANKS][row + 1];
                U[row][2 * col] = u_col2[col][(SVDTraits::NUM_SWEEPS * (NUM_STEPS)) % BANKS][row + 1];
                V[row][2 * col] = v_col2[col][(SVDTraits::NUM_SWEEPS * (NUM_STEPS)) % BANKS][row + 1];
            } else {
                S[row][2 * col] = s_col1[col][(SVDTraits::NUM_SWEEPS * (NUM_STEPS)) % BANKS][row];
                S[row][2 * col + 1] = s_col2[col][(SVDTraits::NUM_SWEEPS * (NUM_STEPS)) % BANKS][row];
                U[row][2 * col] = u_col1[col][(SVDTraits::NUM_SWEEPS * (NUM_STEPS)) % BANKS][row];
                U[row][2 * col + 1] = u_col2[col][(SVDTraits::NUM_SWEEPS * (NUM_STEPS)) % BANKS][row];
                V[row][2 * col] = v_col1[col][(SVDTraits::NUM_SWEEPS * (NUM_STEPS)) % BANKS][row];
                V[row][2 * col + 1] = v_col2[col][(SVDTraits::NUM_SWEEPS * (NUM_STEPS)) % BANKS][row];
            }
        }
    }
}

// ===================================================================================================================
// SVD_TOP: Top level function that selects implementation architecture and internal types based on the
// traits class provided via the SVDTraits template parameter.
// o Call this function directly if you wish to override the default architecture choice or internal types
template <int RowsA, int ColsA, class SVDTraits, typename InputType, typename OutputType>
void svdTop(const InputType A[RowsA][ColsA],
            OutputType S[RowsA][ColsA],
            OutputType U[RowsA][RowsA],
            OutputType V[ColsA][ColsA]) {
    switch (SVDTraits::ARCH) {
        case 0:
            svdBasic<RowsA, ColsA, SVDTraits, InputType, OutputType>(A, S, U, V);
            break;
        case 1:
            svdPairs<RowsA, ColsA, SVDTraits, InputType, OutputType>(A, S, U, V);
            break;
        default:
            svdBasic<RowsA, ColsA, SVDTraits, InputType, OutputType>(A, S, U, V);
            break;
    }
}

/**
 * @brief SVD the entry point function
 *
 * @tparam RowsA                 Row dimension
 * @tparam ColsA                 Column dimension
 * @tparam InputType             Input data type
 * @tparam OutputType            Output data type
 *
 * @param  matrixAStrm           Stream of input matrix
 * @param  matrixSStrm           Stream of singular values output matrix
 * @param  matrixUStrm           Stream of left singular vectors output matrix
 * @param  matrixVStrm           Stream of right singular vectors output matrix
 */
template <int RowsA,
          int ColsA,
          typename InputType,
          typename OutputType,
          typename SVDTraits = svdTraits<RowsA, ColsA, InputType, OutputType> >
void svd(hls::stream<InputType>& matrixAStrm,
         hls::stream<OutputType>& matrixSStrm,
         hls::stream<OutputType>& matrixUStrm,
         hls::stream<OutputType>& matrixVStrm) {
    InputType A[RowsA][ColsA];
    OutputType S[RowsA][ColsA];
    OutputType U[RowsA][RowsA];
    OutputType V[ColsA][ColsA];

    for (int r = 0; r < RowsA; r++) {
#pragma HLS PIPELINE
        for (int c = 0; c < ColsA; c++) {
            matrixAStrm.read(A[r][c]);
        }
    }

    svdTop<RowsA, ColsA, SVDTraits, InputType, OutputType>(A, S, U, V);
    for (int r = 0; r < RowsA; r++) {
#pragma HLS PIPELINE
        for (int c = 0; c < ColsA; c++) {
            matrixSStrm.write(S[r][c]);
        }
    }
    for (int r = 0; r < RowsA; r++) {
#pragma HLS PIPELINE
        for (int c = 0; c < RowsA; c++) {
            matrixUStrm.write(U[r][c]);
        }
    }
    for (int r = 0; r < ColsA; r++) {
#pragma HLS PIPELINE
        for (int c = 0; c < ColsA; c++) {
            matrixVStrm.write(V[r][c]);
        }
    }
}
} // end namespace solver
} // end namespace xf
#endif
