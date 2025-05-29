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
#include "l2-libraries_aieml/apodization/apodization.hpp"
#include "common_defines.hpp"

class TestGraphApodization : public adf::graph {
   public:
    adf::input_plio image_points;
    adf::input_plio apodization_reference;
    adf::input_plio apodization_direction;
    adf::input_plio two;
    adf::input_plio apodization_distance;
    adf::input_plio f_number;
    adf::input_plio one;
    adf::input_plio pi;

    adf::output_plio output_apodization;

    TestGraphApodization() {
#if (defined(__AIESIM__) || defined(__X86SIM__) || defined(__ADF_FRONTEND__)) && !TEST_BUFFER

        image_points = adf::input_plio::create("image_points", adf::plio_32_bits, "data/image_points.txt");
        apodization_reference =
            adf::input_plio::create("apodization_reference", adf::plio_32_bits, "data/apodization_reference.txt");
        apodization_direction =
            adf::input_plio::create("apodization_direction", adf::plio_32_bits, "data/apodization_direction.txt");
        two = adf::input_plio::create("two", adf::plio_32_bits, "data/two.txt");
        apodization_distance =
            adf::input_plio::create("apodization_distance", adf::plio_32_bits, "data/apodization_distance.txt");
        f_number = adf::input_plio::create("f_number", adf::plio_32_bits, "data/f_number.txt");
        one = adf::input_plio::create("one", adf::plio_32_bits, "data/one.txt");
        pi = adf::input_plio::create("pi", adf::plio_32_bits, "data/pi.txt");

        output_apodization =
            adf::output_plio::create("output_apodization", adf::plio_32_bits, "data/output_apodization.txt");

        m_apodization_1_kernel = adf::kernel::create(us::L2::Apodization1<KERNEL_TYPE>);
        m_apodization_2_kernel = adf::kernel::create(us::L2::Apodization2<KERNEL_TYPE>);
        m_apodization_output_kernel = adf::kernel::create(us::L2::ApodizationOutput<KERNEL_TYPE>);

        adf::connect(image_points.out[0], m_apodization_1_kernel.in[0]);
        adf::connect(apodization_reference.out[0], m_apodization_1_kernel.in[1]);
        adf::connect(apodization_direction.out[0], m_apodization_1_kernel.in[2]);
        // adf::connect(m_apodization_1_kernel.out[0], output_apodization.in[0]);

        adf::connect(m_apodization_1_kernel.out[0], m_apodization_2_kernel.in[0]);
        adf::connect(two.out[0], m_apodization_2_kernel.in[1]);
        adf::connect(apodization_distance.out[0], m_apodization_2_kernel.in[2]);
        adf::connect(f_number.out[0], m_apodization_2_kernel.in[3]);
        adf::connect(one.out[0], m_apodization_2_kernel.in[4]);
        // adf::connect(m_apodization_2_kernel.out[0], output_apodization.in[0]);

        adf::connect(m_apodization_2_kernel.out[0], m_apodization_output_kernel.in[0]);
        adf::connect(one.out[0], m_apodization_output_kernel.in[1]);
        adf::connect(pi.out[0], m_apodization_output_kernel.in[2]);
        adf::connect(two.out[0], m_apodization_output_kernel.in[3]);

        adf::connect(m_apodization_output_kernel.out[0], output_apodization.in[0]);

        adf::dimensions(m_apodization_1_kernel.in[0]) = {LEN * M_COLUMNS};
        adf::dimensions(m_apodization_1_kernel.in[1]) = {LEN * M_COLUMNS};
        adf::dimensions(m_apodization_1_kernel.in[2]) = {LEN * M_COLUMNS};
        adf::dimensions(m_apodization_1_kernel.out[0]) = {LEN};

        adf::dimensions(m_apodization_2_kernel.in[0]) = {LEN};
        adf::dimensions(m_apodization_2_kernel.in[1]) = {LEN};
        adf::dimensions(m_apodization_2_kernel.in[2]) = {LEN};
        adf::dimensions(m_apodization_2_kernel.in[3]) = {LEN};
        adf::dimensions(m_apodization_2_kernel.in[4]) = {LEN};
        adf::dimensions(m_apodization_2_kernel.out[0]) = {LEN};

        adf::dimensions(m_apodization_output_kernel.in[0]) = {LEN};
        adf::dimensions(m_apodization_output_kernel.in[1]) = {LEN};
        adf::dimensions(m_apodization_output_kernel.in[2]) = {LEN};
        adf::dimensions(m_apodization_output_kernel.in[3]) = {LEN};
        adf::dimensions(m_apodization_output_kernel.out[0]) = {LEN};

        adf::source(m_apodization_1_kernel) = "l2-libraries_aieml/apodization/apodization.cpp";

        adf::runtime<adf::ratio>(m_apodization_1_kernel) = 1;

        adf::source(m_apodization_2_kernel) = "l2-libraries_aieml/apodization/apodization.cpp";

        adf::runtime<adf::ratio>(m_apodization_2_kernel) = 1;

        adf::source(m_apodization_output_kernel) = "l2-libraries_aieml/apodization/apodization.cpp";

        adf::runtime<adf::ratio>(m_apodization_output_kernel) = 1;

#endif

#if (defined(__AIESIM__) || defined(__X86SIM__) || defined(__ADF_FRONTEND__)) && TEST_BUFFER

        image_points = adf::input_plio::create("image_points", adf::plio_32_bits, "data/image_points.txt");
        apodization_reference =
            adf::input_plio::create("apodization_reference", adf::plio_32_bits, "data/apodization_reference.txt");
        apodization_direction =
            adf::input_plio::create("apodization_direction", adf::plio_32_bits, "data/apodization_direction.txt");
        two = adf::input_plio::create("two", adf::plio_32_bits, "data/two.txt");
        apodization_distance =
            adf::input_plio::create("apodization_distance", adf::plio_32_bits, "data/apodization_distance.txt");
        f_number = adf::input_plio::create("f_number", adf::plio_32_bits, "data/f_number.txt");
        one = adf::input_plio::create("one", adf::plio_32_bits, "data/one.txt");
        pi = adf::input_plio::create("pi", adf::plio_32_bits, "data/pi.txt");

        output_apodization =
            adf::output_plio::create("output_apodization", adf::plio_32_bits, "data/output_apodization.txt");

        m_apodization_1_kernel = adf::kernel::create(us::L2::Apodization1<KERNEL_TYPE>);
        m_apodization_2_kernel = adf::kernel::create(us::L2::Apodization2<KERNEL_TYPE>);
        m_apodization_output_kernel = adf::kernel::create(us::L2::ApodizationOutput<KERNEL_TYPE>);

        adf::connect(image_points.out[0], m_apodization_1_kernel.in[0]);
        adf::connect(apodization_reference.out[0], m_apodization_1_kernel.in[1]);
        adf::connect(apodization_direction.out[0], m_apodization_1_kernel.in[2]);
        // adf::connect(m_apodization_1_kernel.out[0], output_apodization.in[0]);

        adf::connect(m_apodization_1_kernel.out[0], m_apodization_2_kernel.in[0]);
        adf::connect(two.out[0], m_apodization_2_kernel.in[1]);
        adf::connect(apodization_distance.out[0], m_apodization_2_kernel.in[2]);
        adf::connect(f_number.out[0], m_apodization_2_kernel.in[3]);
        adf::connect(one.out[0], m_apodization_2_kernel.in[4]);
        // adf::connect(m_apodization_2_kernel.out[0], output_apodization.in[0]);

        adf::connect(m_apodization_2_kernel.out[0], m_apodization_output_kernel.in[0]);
        adf::connect(one.out[0], m_apodization_output_kernel.in[1]);
        adf::connect(pi.out[0], m_apodization_output_kernel.in[2]);
        adf::connect(two.out[0], m_apodization_output_kernel.in[3]);

        adf::connect(m_apodization_output_kernel.out[0], output_apodization.in[0]);

        adf::dimensions(m_apodization_1_kernel.in[0]) = {LEN * M_COLUMNS};
        adf::dimensions(m_apodization_1_kernel.in[1]) = {LEN * M_COLUMNS};
        adf::dimensions(m_apodization_1_kernel.in[2]) = {LEN * M_COLUMNS};
        adf::dimensions(m_apodization_1_kernel.out[0]) = {LEN};

        adf::dimensions(m_apodization_2_kernel.in[0]) = {LEN};
        adf::dimensions(m_apodization_2_kernel.in[1]) = {LEN};
        adf::dimensions(m_apodization_2_kernel.in[2]) = {LEN};
        adf::dimensions(m_apodization_2_kernel.in[3]) = {LEN};
        adf::dimensions(m_apodization_2_kernel.in[4]) = {LEN};
        adf::dimensions(m_apodization_2_kernel.out[0]) = {LEN};

        adf::dimensions(m_apodization_output_kernel.in[0]) = {LEN};
        adf::dimensions(m_apodization_output_kernel.in[1]) = {LEN};
        adf::dimensions(m_apodization_output_kernel.in[2]) = {LEN};
        adf::dimensions(m_apodization_output_kernel.in[3]) = {LEN};
        adf::dimensions(m_apodization_output_kernel.out[0]) = {LEN};

        adf::source(m_apodization_1_kernel) = "l2-libraries_aieml/apodization/apodization.cpp";

        adf::runtime<adf::ratio>(m_apodization_1_kernel) = 1;

        adf::source(m_apodization_2_kernel) = "l2-libraries_aieml/apodization/apodization.cpp";

        adf::runtime<adf::ratio>(m_apodization_2_kernel) = 1;

        adf::source(m_apodization_output_kernel) = "l2-libraries_aieml/apodization/apodization.cpp";

        adf::runtime<adf::ratio>(m_apodization_output_kernel) = 1;

#endif
    }

   private:
    adf::kernel m_apodization_1_kernel;
    adf::kernel m_apodization_2_kernel;
    adf::kernel m_apodization_output_kernel;
};
