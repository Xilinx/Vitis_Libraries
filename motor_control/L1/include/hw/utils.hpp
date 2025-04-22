/*
Copyright (C) 2022-2022, Xilinx, Inc.
Copyright (C) 2022-2023, Advanced Micro Devices, Inc.
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
* brief A WIDTH signed integer type.
* brief Clip the WIDTH-bit argument in the given range, in the form of an inline HLS function.
* param x Value to be clipped.
* param x_min Minimum limit for the value.
* param x_max Maximum limit for the value.
* return Value #x clipped into the range #x_min ... #x_max.
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
