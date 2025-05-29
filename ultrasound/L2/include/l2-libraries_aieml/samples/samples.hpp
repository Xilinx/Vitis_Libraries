/*
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
#pragma once

#include <adf.h>
#include <aie_api/aie.hpp>
#include <aie_api/utils.hpp>

#include "l1-libraries_aieml/diff/diff.hpp"
#include "l1-libraries_aieml/norm_axis_1/norm_axis_1.hpp"
#include "l1-libraries_aieml/div/div.hpp"
#include "l1-libraries_aieml/sum/sum.hpp"
#include "l1-libraries_aieml/ones/ones.hpp"
#include "l1-libraries_aieml/mul/mul.hpp"

#include "common_defines.hpp"

#define SPEED_OF_SOUND 1540
#define INVERSE_SPEED_OF_SOUND 0.000649350649

template <typename T>
void Samples(adf::input_buffer<T>& image_points,
             adf::input_buffer<T>& delay,
             adf::input_buffer<T>& xdc_def_positions,
             adf::input_buffer<T>& sampling_frequency,
             adf::input_buffer<T>& speed_of_sound,
             adf::output_buffer<T>& output_vector);

// template< typename T >
// void Samples(
//	    adf::input_buffer< T >& image_points, adf::input_buffer< T >& delay, adf::input_buffer< T >&
// xdc_def_positions,/*
//		adf::input_buffer< T >& sampling_frequency,*/ adf::input_buffer< T >& speed_of_sound,
//	    adf::output_buffer< T >& output_vector
//    );
