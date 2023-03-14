/*
 * Copyright 2022 Xilinx, Inc.
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

#ifndef XF_MOTORCONTROL_HW_UTILS_H
#define XF_MOTORCONTROL_HW_UTILS_H

#ifndef __SYNTHESIS__
// for assert function.
#include <cassert>
#define XF_MOTORCONTROL_HW_ASSERT(b) assert((b))
#else
#define XF_MOTORCONTROL_HW_ASSERT(b) ((void)0)
#endif

#if __cplusplus >= 201103L
#define XF_MOTORCONTROL_HW_STATIC_ASSERT(b, m) static_assert((b), m)
#else
#define XF_MOTORCONTROL_HW_STATIC_ASSERT(b, m) XF_MOTORCONTROL_HW_ASSERT((b) && (m))
#endif

#define XF_MOTORCONTROL_HW_MACRO_QUOTE(s) #s
#define XF_MOTORCONTROL_HW_MACRO_STR(s) XF_MOTORCONTROL_HW_MACRO_QUOTE(s)
#define XF_MOTORCONTROL_CHECK_RANGE(v, s, b, d) (if (v < s || v > b) v = d)
//--------------------------------------------------------------------------
// Clip functions
//--------------------------------------------------------------------------
/*
* @brief A WIDTH signed integer type.
* @brief Clip the WIDTH-bit argument in the given range, in the form of an inline HLS function.
* @param x Value to be clipped.
* @param x_min Minimum limit for the value.
* @param x_max Maximum limit for the value.
* @return Value #x clipped into the range #x_min ... #x_max.
*/
template <class T>
static T Clip_AP(const T x, const T x_min, const T x_max) {
#pragma HLS INLINE off
    if (x < x_min) {
        return x_min;
    } else if (x > x_max) {
        return x_max;
    } else {
        return x;
    }
}

#endif
