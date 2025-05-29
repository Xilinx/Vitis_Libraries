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
#include "l2-libraries_aieml/image_points/image_points.hpp"
#include "common_defines.hpp"

class TestGraphImagePoints : public adf::graph {
   public:
    adf::input_plio start_position;
    adf::input_plio directions;
    adf::input_plio samples_arange;
    adf::output_plio output_matrix;

    TestGraphImagePoints() {
#if (defined(__AIESIM__) || defined(__X86SIM__) || defined(__ADF_FRONTEND__))

#if (defined(TYPE_IS_FLOAT))
        start_position = adf::input_plio::create("start_position", adf::plio_32_bits, "data/start_position.txt");
        directions = adf::input_plio::create("directions", adf::plio_32_bits, "data/directions.txt");
#else
        start_position = adf::input_plio::create("start_position", adf::plio_32_bits, "data/start_position_int32.txt");
        directions = adf::input_plio::create("directions", adf::plio_32_bits, "data/directions_int32.txt");
#endif
        samples_arange = adf::input_plio::create("samples_arange", adf::plio_32_bits, "data/samples_arange.txt");
        output_matrix = adf::output_plio::create("output_matrix", adf::plio_32_bits, "data/output_matrix.txt");

        m_image_points_kernel = adf::kernel::create(ImagePoints<KERNEL_TYPE>);
        adf::connect(start_position.out[0], m_image_points_kernel.in[0]);
        adf::connect(directions.out[0], m_image_points_kernel.in[1]);
        adf::connect(samples_arange.out[0], m_image_points_kernel.in[2]);
        adf::connect(m_image_points_kernel.out[0], output_matrix.in[0]);

        adf::dimensions(m_image_points_kernel.in[0]) = {LEN};
        adf::dimensions(m_image_points_kernel.in[1]) = {LEN};
        adf::dimensions(m_image_points_kernel.in[2]) = {LEN};
        adf::dimensions(m_image_points_kernel.out[0]) = {LEN * M_COLUMNS};

        adf::source(m_image_points_kernel) = "l2-libraries_aieml/image_points/image_points.cpp";

        adf::runtime<adf::ratio>(m_image_points_kernel) = 0.6;

#endif
    }

   private:
    adf::kernel m_image_points_kernel;
};
