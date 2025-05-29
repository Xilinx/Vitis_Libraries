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

#include "l1-libraries_aieml/abs_v/abs_v.hpp"
#include "l1-libraries_aieml/diff/diff.hpp"
#include "l1-libraries_aieml/div/div.hpp"
#include "l1-libraries_aieml/mul/mul.hpp"
#include "l1-libraries_aieml/norm_axis_1/norm_axis_1.hpp"
#include "l1-libraries_aieml/sign/sign.hpp"
#include "l1-libraries_aieml/sum/sum.hpp"
#include "l1-libraries_aieml/sum_axis_1/sum_axis_1.hpp"
#include "l1-libraries_aieml/tile_v/tile_v.hpp"

#include "common_defines.hpp"

// template< typename T >
// void Delay(
//
//		adf::input_buffer< T >& image_points_from_pl_1,
//		adf::input_buffer< T >& image_points_from_pl_2,
//		adf::input_buffer< T >& tx_def_ref_point,
//		adf::input_buffer< T >& tx_def_delay_distance_1,
//		adf::input_buffer< T >& tx_def_delay_distance_2,
//		adf::input_buffer< T >& tx_def_focal_point,
//		adf::input_buffer< T >& t_start,
//		adf::input_buffer< T >& direction,
//		adf::input_buffer< T >& speed_of_sound,
//
//		adf::output_buffer< T >& delay_output
//
//);

namespace us {
namespace L2 {

template <typename T>
void Delay1(

    adf::input_buffer<T>& image_points_from_pl_1,
    adf::input_buffer<T>& tx_def_ref_point,
    adf::input_buffer<T>& tx_def_delay_distance_1,
    adf::input_buffer<T>& direction,

    adf::output_buffer<T>& sign_delay_output

    );

template <typename T>
void Delay2(

    adf::input_buffer<T>& sign_delay_input,
    adf::input_buffer<T>& image_points_from_pl_2,
    adf::input_buffer<T>& tx_def_delay_distance_2,
    adf::input_buffer<T>& tx_def_focal_point,
    adf::input_buffer<T>& t_start,
    adf::input_buffer<T>& speed_of_sound,

    adf::output_buffer<T>& delay_output

    );
}
}
