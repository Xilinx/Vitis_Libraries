/*
 * Copyright (C) 2019-2022, Xilinx, Inc.
 * Copyright (C) 2022-2023, Advanced Micro Devices, Inc.
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
 *
 * @file converter.hpp
 * @brief header file for conversion from double-precision floating-point
 * to single-precision floating-point.
 *
 * All transformation comply with IEEE 754-2008.
 *
 * @detail If you use float x = (float)(double a) to convert a from double to float,
 * HLS will automatically call Xilinx IP to implement the cast process, but it
 * sets all subnormals of the single-precision floating-point number to zero.
 * This will generate minor error and fail the test of the implementation.
 * This module intends to manually cast the double to float,
 * do the same operations as 'static_cast' does. Thus, co-sim results will
 * be exctly the same as csim's.
 *
 */

#ifndef _XF_DATABASE_CONVERTER_HPP_
#define _XF_DATABASE_CONVERTER_HPP_

#include <ap_int.h>
#include <hls_stream.h>
#include <stdint.h>

// for debug
#ifndef __SYNTHESIS__
#include <iostream>
#endif

namespace xf {
namespace dataframe {

// converter for double -> ap_uint<64>
union unionDouble {
    double in;
    uint64_t out;
};

// converter for float -> ap_uint<32>
union unionFloat {
    uint32_t in;
    float out;
};

/**
 *
 * @brief Impletmentation for rounding the normal double-precision
 * floating-point number to single-precision floating-point number.
 *
 * The implementation is designed for better performance.
 *
 * @param sign Sign of double-precision floating-point number.
 * @param exp Exponent transformation from double-precision floating-point to single-precision.
 * @param sig Fraction transformation from double-precision floating-point to single-precision.
 * @return The code of the signle-precision floating-point number truncated from the input.
 *
 */

static ap_uint<32> roundPackToFloat(ap_uint<1> sign, ap_int<16> exp, ap_uint<32> sig) {
#pragma HLS inline
    ap_uint<8> roundIncrement = 0x40;
    ap_uint<8> roundBits = sig & 0x7f;

    // result register
    ap_uint<32> out = 0;

    // normal value of float
    if (0xfd <= (unsigned int)exp) {
        // subnormal of float
        if (exp < 0) {
            if (-exp < 31) {
                sig = sig >> -exp | ((sig << (exp & 31)) != 0);
            } else {
                sig = sig != 0;
            }
            exp = 0;
            roundBits = sig & 0x7f;
            // round to near even for float
            sig = (sig + roundIncrement) >> 7;
            sig &= ~(ap_uint<32>)(!(roundBits ^ 0x40));
            if (!sig) {
                exp = 0;
            }

            out = ((ap_uint<32>)sign << 31) + ((ap_uint<32>)exp << 23) + sig;
            // infinite of float
        } else if ((0xfd < exp) || (0x80000000 <= sig + roundIncrement)) {
            out = ((ap_uint<32>)sign << 31) + ((ap_uint<32>)0xff << 23);
            // normal number of float
        } else {
            // round to near even for float
            sig = (sig + roundIncrement) >> 7;
            sig &= ~(ap_uint<32>)(!(roundBits ^ 0x40));
            if (!sig) {
                exp = 0;
            }

            out = ((ap_uint<32>)sign << 31) + ((ap_uint<32>)exp << 23) + sig;
        }
        // overflows
    } else {
        // round to near even for float
        sig = (sig + roundIncrement) >> 7;
        sig &= ~(ap_uint<32>)(!(roundBits ^ 0x40));
        if (!sig) {
            exp = 0;
        }

        out = ((ap_uint<32>)sign << 31) + ((ap_uint<32>)exp << 23) + sig;
    }
    return out;

} // end roundPackToFloat

/**
 *
 * @brief Impletmentation of double to float conversion.
 *
 * The implementation is designed for better performance.
 *
 * @param x Input double-precision floating-point number.
 * @return The signle-precision floating-point number converted from the input.
 *
 */

static float doubleToFloat(double x) {
    // double -> ap_uint<64>
    unionDouble ud;
    ud.in = x;
    ap_uint<64> in = ud.out;

    // intermediate registers
    ap_uint<64> a;
    ap_uint<32> frac32;

    // split floating-point number to sign, exp, and frac
    ap_uint<1> sign = in.range(63, 63);
    ap_uint<11> exp = in.range(62, 52);
    ap_uint<52> frac = in.range(51, 0);

    // result register
    ap_uint<32> out;

    // exponent decoding for double
    if (exp == 0x7ff) {
        // NaNs
        if (frac) {
            ap_uint<64> signNaN = in >> 63;
            ap_uint<64> fracNaN = in << 12;
            out = (ap_uint<32>)signNaN << 31 | 0x7FC00000 | fracNaN >> 41;
            // infinite
        } else {
            out = (ap_uint<32>)sign << 31 | 0x7f800000;
        }
    } else {
        a = frac;
        frac32 = a >> 22 | ((a & 0x3fffff) != 0);
        // zero
        if (!(exp | frac32)) {
            out = (ap_uint<32>)sign << 31;
            // ordinary numbers & subnormals of double
        } else {
            out = roundPackToFloat(sign, exp - 0x381, frac32 | 0x40000000);
        }
    }

    // ap_uint<32> -> float
    unionFloat uf;
    uf.in = out;
    float y = uf.out;

    return y;

} // end doubleToFloat

} // namespace database
} // namespace xf

#endif
