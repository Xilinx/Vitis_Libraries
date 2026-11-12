/*
 * Copyright (C) 2019-2022, Xilinx, Inc.
 * Copyright (C) 2022-2026, Advanced Micro Devices, Inc.
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

#ifndef _XF_SOLVER_MATRIX_UTILS_HPP_
#define _XF_SOLVER_MATRIX_UTILS_HPP_

#include "ap_fixed.h"
#include "utils/x_hls_utils.h"
#include "hls_x_complex.h"
#include "utils/std_complex_utils.h"
#include "hls_math.h"

#ifndef __SYNTHESIS__
#include <iostream>
#include <iomanip>
#include <string>
#include <sstream>
#include <stdio.h>
#endif

namespace xf {
namespace solver {

// ===================================================================================================================
// Transpose Types & Operation
struct NoTranspose {
    const static int TransposeType = 0;
    template <int RowsA, int ColsA, typename InputType>
    static InputType GetElement(const InputType A[RowsA][ColsA], unsigned Row, unsigned Col) {
        // No transpose, default
        return A[Row][Col];
    }
};

struct Transpose {
    const static int TransposeType = 1;
    template <int RowsA, int ColsA, typename InputType>
    static InputType GetElement(const InputType A[RowsA][ColsA], unsigned Row, unsigned Col) {
        // Transpose, no conjugate
        return A[Col][Row];
    }
};

struct ConjugateTranspose {
    const static int TransposeType = 2;
    template <int RowsA, int ColsA, typename InputType>
    static InputType GetElement(const InputType A[RowsA][ColsA], unsigned Row, unsigned Col) {
        // Complex conjugate transpose.
        // o For non x_complex types this function will return an unaltered value.
        return hls::x_conj(A[Col][Row]);
    }
};

template <class TransposeForm, int RowsA, int ColsA, typename InputType>
InputType GetMatrixElement(const InputType A[RowsA][ColsA], unsigned Row, unsigned Col) {
    // Need to help the compiler identify that the GetElement member function is a template
    return TransposeForm::template GetElement<RowsA, ColsA, InputType>(A, Row, Col);
}

// ===================================================================================================================
// Common math operations and constants. Wrappers/templates to select the correct function based on type.

// sqrt
static half x_sqrt(half x) {
    return hls::half_sqrt(x);
}
static float x_sqrt(float x) {
    return sqrtf(x);
}
static double x_sqrt(double x) {
    return sqrt(x);
}
template <int W1, int I1, ap_q_mode Q1, ap_o_mode O1, int N1>
ap_fixed<W1, I1, Q1, O1, N1> x_sqrt(ap_fixed<W1, I1, Q1, O1, N1> x) {
    return hls::sqrt((double)x);
}

// copysign
static float x_copysign(float a, float b) {
    return copysignf(a, b);
}
static double x_copysign(double a, double b) {
    return copysign(a, b);
}

// sign
static float x_sign(float x) {
    if (x == 0.0f) {
        return 0.0f;
    } else {
        return copysignf(1.0f, x);
    }
}
static double x_sign(double x) {
    if (x == 0.0) {
        return 0.0;
    } else {
        return copysign(1.0, x);
    }
}
template <int W1, int I1, ap_q_mode Q1, ap_o_mode O1, int N1>
ap_fixed<W1, I1, Q1, O1, N1> x_sign(ap_fixed<W1, I1, Q1, O1, N1> x) {
    ap_fixed<W1, I1, Q1, O1, N1> tmp = 0;
    if (x > 0) {
        tmp = 1;
    } else {
        tmp = -1;
    }
    return tmp;
}

// reciprocal sqrt
static half x_rsqrt(half x) {
    return hls::half_rsqrt(x);
}
static float x_rsqrt(float x) {
    return hls::rsqrtf(x);
}
static double x_rsqrt(double x) {
    return hls::rsqrt(x);
}

// Number of bits a type resolves below its leading significant bit: the fractional width for a
// fixed point grid, the significand width for a native floating point format.  Used to size the
// working precision of an operator against its destination.
template <typename T>
struct x_fraction_bits {
    static const int VALUE = 0;
};
template <int W, int I, ap_q_mode Q, ap_o_mode O, int N>
struct x_fraction_bits<ap_fixed<W, I, Q, O, N> > {
    static const int VALUE = W - I;
};
template <int W, int I, ap_q_mode Q, ap_o_mode O, int N>
struct x_fraction_bits<ap_ufixed<W, I, Q, O, N> > {
    static const int VALUE = W - I;
};
// Without these a native floating point destination would fall to the primary template's 0 and
// silently select a working precision narrower than it can hold.
template <>
struct x_fraction_bits<half> {
    static const int VALUE = 11;
};
template <>
struct x_fraction_bits<float> {
    static const int VALUE = 24;
};
template <>
struct x_fraction_bits<double> {
    static const int VALUE = 53;
};

// Working precision available from each native floating point format.
static const int X_FLOAT_SIGNIFICAND_BITS = 24;
static const int X_DOUBLE_SIGNIFICAND_BITS = 53;

// Extra bits of working precision demanded beyond the nominal result LSB.  The native reciprocal
// square roots are not exactly rounded, so at 0 the fixed point back end resolves to about one LSB
// rather than half of one.  Raise this to buy accuracy at the cost of pushing narrow formats onto
// the double precision path.
static const int X_RSQRT_GUARD_BITS = 0;

// Working precision a destination demands when it receives 1/sqrt(x).
// o Fixed point destination: the result sits on a fixed grid, so the significand must carry both
//   the range and the grid.  The input's fractional width bounds 1/sqrt(x) by 2^(F_IN/2), needing
//   ceil(F_IN/2) bits above the binary point, and F_OUT bits below it.
// o Native floating point destination: the exponent carries the range, so only the destination's
//   own significand is demanded.  Without this the range term would be added on top of a 53 bit
//   double and demand a precision no format can supply.
template <typename T_IN, typename T_OUT>
struct x_rsqrt_working_bits {
    static const int VALUE = (x_fraction_bits<T_IN>::VALUE + 1) / 2 + x_fraction_bits<T_OUT>::VALUE;
};
template <typename T_IN>
struct x_rsqrt_working_bits<T_IN, half> {
    static const int VALUE = 11;
};
template <typename T_IN>
struct x_rsqrt_working_bits<T_IN, float> {
    static const int VALUE = 24;
};
template <typename T_IN>
struct x_rsqrt_working_bits<T_IN, double> {
    static const int VALUE = 53;
};

// Evaluation of a fixed point reciprocal square root at a chosen working precision.
template <bool SinglePrecisionIsEnough>
struct x_rsqrt_eval {
    template <typename T_IN, typename T_OUT>
    static void calc(T_IN x, T_OUT& res) {
        res = x_rsqrt((double)x);
    }
};
template <>
struct x_rsqrt_eval<true> {
    template <typename T_IN, typename T_OUT>
    static void calc(T_IN x, T_OUT& res) {
        res = x_rsqrt((float)x);
    }
};

// Fixed point reciprocal square root, resolved to the destination's own quantisation step.
// o There is no fixed point rsqrt operator, so the value is evaluated in native floating point at
//   the narrowest format that still resolves the destination.  This keeps narrow types on the
//   cheap single precision path while wider ones stay accurate.
// o The destination is taken by reference because the working precision depends on it, which the
//   single argument overloads above cannot express.
template <int W, int I, ap_q_mode Q, ap_o_mode O, int N, typename T_OUT>
void x_rsqrt(ap_fixed<W, I, Q, O, N> x, T_OUT& res) {
    const int REQUIRED_BITS = x_rsqrt_working_bits<ap_fixed<W, I, Q, O, N>, T_OUT>::VALUE + X_RSQRT_GUARD_BITS;
    static_assert(REQUIRED_BITS <= X_DOUBLE_SIGNIFICAND_BITS,
                  "x_rsqrt: these formats need more working precision than double provides; "
                  "narrow the destination type.");
    x_rsqrt_eval<(REQUIRED_BITS <= X_FLOAT_SIGNIFICAND_BITS)>::calc(x, res);
}
template <int W, int I, ap_q_mode Q, ap_o_mode O, int N, typename T_OUT>
void x_rsqrt(ap_ufixed<W, I, Q, O, N> x, T_OUT& res) {
    const int REQUIRED_BITS = x_rsqrt_working_bits<ap_ufixed<W, I, Q, O, N>, T_OUT>::VALUE + X_RSQRT_GUARD_BITS;
    static_assert(REQUIRED_BITS <= X_DOUBLE_SIGNIFICAND_BITS,
                  "x_rsqrt: these formats need more working precision than double provides; "
                  "narrow the destination type.");
    x_rsqrt_eval<(REQUIRED_BITS <= X_FLOAT_SIGNIFICAND_BITS)>::calc(x, res);
}

// isneg
static int x_isneg(float x) {
    fp_struct<float> fs = x;
    return fs.__signbit();
}
static int x_isneg(double x) {
    fp_struct<double> fs = x;
    return fs.__signbit();
}

#ifndef __SYNTHESIS__
// Matrix Display
// ============================================================================

// Non-specific data
// -----------------
// This can be used by PoD or any class supporting "<<"
// setprecision may fail on non floating point types
template <typename T>
struct xil_printer {
    static std::string to_s(T x, unsigned prec = 10) {
        std::stringstream ss;
        ss << std::setiosflags(std::ios::fixed) << std::setprecision(prec) << x;

        return ss.str();
    }
};

// Complex data
// ----------------
// This is used by complex data of any type.  A printer is called to print the real and imaginary
// parts, so this just handles the formating of x+jy
template <typename T>
struct xil_printer<hls::x_complex<T> > {
    static std::string to_s(hls::x_complex<T> x, unsigned prec = 10) {
        // Use the basic type printer to print the real and imaginary parts
        typedef xil_printer<T> printer;

        std::stringstream ss;
        // Remember to deal with -0
        bool neg_imag = x.imag() <= -0 ? true : false;

        // Casting to "T" avoids "error: operands to ?: have different types." when using ap_fixed.
        T imag = neg_imag ? (T)-x.imag() : (T)x.imag();

        ss << printer::to_s(x.real(), prec) << (neg_imag ? " - j*" : " + j*") << printer::to_s(imag, prec);
        return ss.str();
    }
};

template <unsigned ROWS, unsigned COLS, typename T, class TransposeForm>
void print_matrix(T a[ROWS][COLS], std::string prefix = "", unsigned prec = 10, unsigned matlab_format = 0) {
    typedef xil_printer<T> printer;

    std::string res[ROWS][COLS];
    unsigned widest_entry = 0;

    // Get the individual fields
    for (int r = 0; r < ROWS; r++) {
        for (int c = 0; c < COLS; c++) {
            T tmp;

            tmp = GetMatrixElement<TransposeForm, ROWS, COLS, T>(a, r, c);

            res[r][c] = printer::to_s(tmp, prec);
            if (res[r][c].length() > widest_entry) {
                widest_entry = res[r][c].length();
            }
        }
    }

    // Print fields.  Each column should be "widest_entry" chars wide
    char col_gap_str[5] = "    ";
    unsigned col_width = widest_entry;

    for (int r = 0; r < ROWS; r++) {
        if (!matlab_format) {
            printf("%s|", prefix.c_str());
        } else {
            if (r == 0) {
                printf("%s[", prefix.c_str());
            } else {
                printf("%s ", prefix.c_str());
            }
        }
        for (int c = 0; c < COLS; c++) {
            unsigned num_spaces_needed = col_width - res[r][c].length();
            for (int x = 0; x < num_spaces_needed; x++) {
                printf(" ");
            }
            if (!matlab_format) {
                printf("(%s)", res[r][c].c_str());
            } else {
                printf("%s", res[r][c].c_str());
            }

            if (c != COLS - 1) {
                printf("%s", col_gap_str);
                if (matlab_format) {
                    printf(",");
                }
            }
        }
        if (!matlab_format) {
            printf(" |\n");
        } else {
            if (r == ROWS - 1) {
                printf(" ];\n");
            } else {
                printf(" ;\n");
            }
        }
    }
}
#endif

} // namespace solver
} // namespace xf

#endif
