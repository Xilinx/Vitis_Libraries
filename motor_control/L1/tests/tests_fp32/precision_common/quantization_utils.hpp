/*
Copyright (C) 2025-2025, Advanced Micro Devices, Inc.
SPDX-License-Identifier: X11

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
X CONSORTIUM BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
IN THE SOFTWARE.

Except as contained in this notice, the name of Advanced Micro Devices
shall not be used in advertising or otherwise to promote the sale,
use or other dealings in this Software without prior written authorization
from Advanced Micro Devices, Inc.
*/

#pragma once

#include "ap_fixed.h"
#include <cmath>
#include <limits>

namespace xf {
namespace motorcontrol {
namespace test {

/**
 * @brief Quantization utilities for Phase 1 precision validation
 *
 * This module provides conversion utilities between float32 and ap_fixed formats,
 * following Vitis HLS ap_fixed default rounding and truncation behavior.
 *
 * Key characteristics:
 * - ap_fixed uses AP_RND (rounding to plus infinity) and AP_SAT (saturation) by default
 * - These utilities replicate the exact quantization behavior for golden model validation
 * - Algorithm-agnostic and reusable across all motor control IP blocks
 */

/**
 * @brief Convert float to ap_fixed with explicit Q-format
 *
 * Replicates Vitis HLS ap_fixed quantization behavior:
 * - Rounding mode: AP_RND (round to plus infinity, i.e., round half up)
 * - Overflow mode: AP_SAT (saturation at boundaries)
 *
 * @tparam W Total bit width (including sign bit)
 * @tparam I Integer bits (bits to the left of binary point, including sign bit)
 * @param value Input float32 value
 * @return ap_fixed<W, I> Quantized fixed-point value
 *
 * Example:
 *   float_to_fixed<16, 8>(1.234f)  // Q8.7 format (16 bits, 8 integer bits)
 */
template <int W, int I>
ap_fixed<W, I> float_to_fixed(float value) {
    // ap_fixed constructor handles quantization with default AP_RND and AP_SAT modes
    ap_fixed<W, I> result = value;
    return result;
}

/**
 * @brief Convert ap_fixed to float
 *
 * Performs lossless conversion from ap_fixed to float32.
 * Since float32 has 24-bit mantissa precision, this is exact for most
 * motor control fixed-point formats (typically ≤24 bits total width).
 *
 * @tparam W Total bit width
 * @tparam I Integer bits
 * @param value Input ap_fixed value
 * @return float Converted float32 value
 *
 * Example:
 *   ap_fixed<16, 8> fixed_val = 1.5;
 *   float result = fixed_to_float(fixed_val);  // result = 1.5f
 */
template <int W, int I>
float fixed_to_float(const ap_fixed<W, I>& value) {
    // ap_fixed's to_double() provides exact conversion
    return static_cast<float>(value.to_double());
}

/**
 * @brief Simulate quantization: float -> ap_fixed -> float
 *
 * Useful for understanding quantization error in golden model.
 * Applies the same quantization that DUT uses, returns result as float.
 *
 * @tparam W Total bit width
 * @tparam I Integer bits
 * @param value Input float32 value
 * @return float Quantized value converted back to float
 *
 * Example:
 *   float original = 1.2345f;
 *   float quantized = quantize_float<16, 8>(original);
 *   float error = original - quantized;  // quantization error
 */
template <int W, int I>
float quantize_float(float value) {
    ap_fixed<W, I> fixed_val = float_to_fixed<W, I>(value);
    return fixed_to_float(fixed_val);
}

/**
 * @brief Get the quantization step size (LSB value) for a given Q-format
 *
 * @tparam W Total bit width
 * @tparam I Integer bits
 * @return float The value of the least significant bit (2^(-F) where F = W - I)
 *
 * Example:
 *   get_lsb<16, 8>()  // Returns 2^(-8) = 0.00390625 for Q8.7 format
 */
template <int W, int I>
float get_lsb() {
    constexpr int F = W - I; // Fractional bits
    return std::pow(2.0f, -static_cast<float>(F));
}

/**
 * @brief Get the maximum representable value for a given Q-format
 *
 * @tparam W Total bit width
 * @tparam I Integer bits
 * @return float Maximum representable value
 *
 * For signed ap_fixed:
 *   max = 2^(I-1) - 2^(-F) where F = W - I
 *
 * Example:
 *   get_max_value<16, 8>()  // Returns ~127.996 for Q8.7 format
 */
template <int W, int I>
float get_max_value() {
    ap_fixed<W, I> max_val;
    // Set to maximum positive value
    for (int i = 0; i < W - 1; i++) {
        max_val[i] = 1;
    }
    max_val[W - 1] = 0; // Sign bit = 0 for positive
    return fixed_to_float(max_val);
}

/**
 * @brief Get the minimum representable value for a given Q-format
 *
 * @tparam W Total bit width
 * @tparam I Integer bits
 * @return float Minimum representable value
 *
 * For signed ap_fixed:
 *   min = -2^(I-1)
 *
 * Example:
 *   get_min_value<16, 8>()  // Returns -128.0 for Q8.7 format
 */
template <int W, int I>
float get_min_value() {
    ap_fixed<W, I> min_val;
    // Set to minimum negative value
    for (int i = 0; i < W - 1; i++) {
        min_val[i] = 0;
    }
    min_val[W - 1] = 1; // Sign bit = 1 for negative
    return fixed_to_float(min_val);
}

/**
 * @brief Check if a float value will saturate when converted to ap_fixed
 *
 * @tparam W Total bit width
 * @tparam I Integer bits
 * @param value Input float value to check
 * @return bool True if value exceeds representable range
 *
 * Example:
 *   will_saturate<16, 8>(150.0f)  // Returns true (max ~128)
 *   will_saturate<16, 8>(100.0f)  // Returns false
 */
template <int W, int I>
bool will_saturate(float value) {
    float max_val = get_max_value<W, I>();
    float min_val = get_min_value<W, I>();
    return (value > max_val) || (value < min_val);
}

/**
 * @brief Calculate quantization error for a float value
 *
 * @tparam W Total bit width
 * @tparam I Integer bits
 * @param value Input float value
 * @return float Absolute quantization error (original - quantized)
 *
 * Example:
 *   float error = get_quantization_error<16, 8>(1.2345f);
 */
template <int W, int I>
float get_quantization_error(float value) {
    float quantized = quantize_float<W, I>(value);
    return std::abs(value - quantized);
}

/**
 * @brief Format information structure for runtime Q-format queries
 *
 * Useful for debugging and reporting
 */
struct FixedPointInfo {
    int total_bits;      // W
    int integer_bits;    // I
    int fractional_bits; // F = W - I
    float lsb;           // Quantization step
    float max_value;     // Maximum representable value
    float min_value;     // Minimum representable value

    // Constructor for compile-time format
    template <int W, int I>
    static FixedPointInfo create() {
        FixedPointInfo info;
        info.total_bits = W;
        info.integer_bits = I;
        info.fractional_bits = W - I;
        info.lsb = get_lsb<W, I>();
        info.max_value = get_max_value<W, I>();
        info.min_value = get_min_value<W, I>();
        return info;
    }
};

} // namespace test
} // namespace motorcontrol
} // namespace xf
