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
#include "l2-libraries_aieml/delay/delay.hpp"
#include "common_defines.hpp"

class TestGraphDelay : public adf::graph {
   public:
    adf::input_plio image_points_from_pl_1;
    adf::input_plio image_points_from_pl_2;
    adf::input_plio tx_def_ref_point;
    adf::input_plio tx_def_delay_distance_1;
    adf::input_plio tx_def_delay_distance_2;
    adf::input_plio tx_def_focal_point;
    adf::input_plio t_start;
    adf::input_plio direction;
    adf::input_plio speed_of_sound;

    adf::input_plio debug_after_virtual_source;

    adf::output_plio delay_output;

    TestGraphDelay() {
#if (defined(__AIESIM__) || defined(__X86SIM__) || defined(__ADF_FRONTEND__)) && !TEST_BUFFER

#if (defined(TYPE_IS_FLOAT))
        image_points_from_pl_1 = adf::input_plio::create("image_points_from_pl_1", adf::plio_32_bits,
                                                         "data/image_points_from_pl_1_float.txt");
        image_points_from_pl_2 = adf::input_plio::create("image_points_from_pl_2", adf::plio_32_bits,
                                                         "data/image_points_from_pl_2_float.txt");
        tx_def_ref_point =
            adf::input_plio::create("tx_def_ref_point", adf::plio_32_bits, "data/tx_def_ref_point_float.txt");
        tx_def_delay_distance_1 = adf::input_plio::create("tx_def_delay_distance_1", adf::plio_32_bits,
                                                          "data/tx_def_delay_distance_1_float.txt");
        tx_def_delay_distance_2 = adf::input_plio::create("tx_def_delay_distance_2", adf::plio_32_bits,
                                                          "data/tx_def_delay_distance_2_float.txt");
        tx_def_focal_point =
            adf::input_plio::create("tx_def_focal_point", adf::plio_32_bits, "data/tx_def_focal_point_float.txt");
        t_start = adf::input_plio::create("t_start", adf::plio_32_bits, "data/t_start_float.txt");
        direction = adf::input_plio::create("direction", adf::plio_32_bits, "data/direction_float.txt");
        speed_of_sound = adf::input_plio::create("speed_of_sound", adf::plio_32_bits, "data/speed_of_sound_float.txt");
        delay_output = adf::output_plio::create("delay_output", adf::plio_32_bits, "data/delay_output_float.txt");
#else
        image_points_from_pl_1 =
            adf::input_plio::create("image_points_from_pl_1", adf::plio_32_bits, "data/image_points_from_pl_1.txt");
        image_points_from_pl_2 =
            adf::input_plio::create("image_points_from_pl_2", adf::plio_32_bits, "data/image_points_from_pl_2.txt");
        tx_def_ref_point = adf::input_plio::create("tx_def_ref_point", adf::plio_32_bits, "data/tx_def_ref_point.txt");
        tx_def_delay_distance_1 =
            adf::input_plio::create("tx_def_delay_distance_1", adf::plio_32_bits, "data/tx_def_delay_distance_1.txt");
        tx_def_delay_distance_2 =
            adf::input_plio::create("tx_def_delay_distance_2", adf::plio_32_bits, "data/tx_def_delay_distance_2.txt");
        tx_def_focal_point =
            adf::input_plio::create("tx_def_focal_point", adf::plio_32_bits, "data/tx_def_focal_point.txt");
        t_start = adf::input_plio::create("t_start", adf::plio_32_bits, "data/t_start.txt");
        direction = adf::input_plio::create("direction", adf::plio_32_bits, "data/direction.txt");
        speed_of_sound = adf::input_plio::create("speed_of_sound", adf::plio_32_bits, "data/speed_of_sound.txt");
        delay_output = adf::output_plio::create("delay_output", adf::plio_32_bits, "data/delay_output.txt");
#endif

        m_delay_1_kernel = adf::kernel::create(us::L2::Delay1<KERNEL_TYPE>);
        m_delay_2_kernel = adf::kernel::create(us::L2::Delay2<KERNEL_TYPE>);

        adf::connect(image_points_from_pl_1.out[0], m_delay_1_kernel.in[0]);
        adf::connect(tx_def_ref_point.out[0], m_delay_1_kernel.in[1]);
        adf::connect(tx_def_delay_distance_1.out[0], m_delay_1_kernel.in[2]);
        adf::connect(direction.out[0], m_delay_1_kernel.in[3]);

        adf::connect(m_delay_1_kernel.out[0], m_delay_2_kernel.in[0]);
        //			adf::connect(debug_after_virtual_source.out[0], m_delay_2_kernel.in[0]);

        adf::connect(image_points_from_pl_2.out[0], m_delay_2_kernel.in[1]);
        adf::connect(tx_def_delay_distance_2.out[0], m_delay_2_kernel.in[2]);
        adf::connect(tx_def_focal_point.out[0], m_delay_2_kernel.in[3]);
        adf::connect(t_start.out[0], m_delay_2_kernel.in[4]);
        adf::connect(speed_of_sound.out[0], m_delay_2_kernel.in[5]);
        adf::connect(m_delay_2_kernel.out[0], delay_output.in[0]);

        adf::dimensions(m_delay_1_kernel.in[0]) = {LEN * M_COLUMNS};
        adf::dimensions(m_delay_1_kernel.in[1]) = {LEN * M_COLUMNS};
        adf::dimensions(m_delay_1_kernel.in[2]) = {LEN};
        adf::dimensions(m_delay_1_kernel.in[3]) = {SIMD_DEPTH};
        adf::dimensions(m_delay_1_kernel.out[0]) = {LEN};

        adf::dimensions(m_delay_2_kernel.in[0]) = {LEN};
        adf::dimensions(m_delay_2_kernel.in[1]) = {LEN * M_COLUMNS};
        adf::dimensions(m_delay_2_kernel.in[2]) = {LEN};
        adf::dimensions(m_delay_2_kernel.in[3]) = {LEN * M_COLUMNS};
        adf::dimensions(m_delay_2_kernel.in[4]) = {LEN};
        adf::dimensions(m_delay_2_kernel.in[5]) = {LEN};
        adf::dimensions(m_delay_2_kernel.out[0]) = {LEN};

        adf::source(m_delay_1_kernel) = "l2-libraries_aieml/delay/delay.cpp";

        adf::runtime<adf::ratio>(m_delay_1_kernel) = 1;

        adf::source(m_delay_2_kernel) = "l2-libraries_aieml/delay/delay.cpp";

        adf::runtime<adf::ratio>(m_delay_2_kernel) = 1;

#endif

#if (defined(__AIESIM__) || defined(__X86SIM__) || defined(__ADF_FRONTEND__)) && TEST_BUFFER

        image_points_from_pl_1 =
            adf::input_plio::create("image_points_from_pl_1", adf::plio_32_bits, "data/image_points_from_pl_1.txt");
        image_points_from_pl_2 =
            adf::input_plio::create("image_points_from_pl_2", adf::plio_32_bits, "data/image_points_from_pl_2.txt");
        tx_def_ref_point = adf::input_plio::create("tx_def_ref_point", adf::plio_32_bits, "data/tx_def_ref_point.txt");
        tx_def_delay_distance_1 =
            adf::input_plio::create("tx_def_delay_distance_1", adf::plio_32_bits, "data/tx_def_delay_distance_1.txt");
        tx_def_delay_distance_2 =
            adf::input_plio::create("tx_def_delay_distance_2", adf::plio_32_bits, "data/tx_def_delay_distance_2.txt");
        tx_def_focal_point =
            adf::input_plio::create("tx_def_focal_point", adf::plio_32_bits, "data/tx_def_focal_point.txt");
        t_start = adf::input_plio::create("t_start", adf::plio_32_bits, "data/t_start.txt");
        direction = adf::input_plio::create("direction", adf::plio_32_bits, "data/direction.txt");
        speed_of_sound = adf::input_plio::create("speed_of_sound", adf::plio_32_bits, "data/speed_of_sound.txt");
        delay_output = adf::output_plio::create("delay_output", adf::plio_32_bits, "data/delay_output.txt");

        m_delay_1_kernel = adf::kernel::create(us::L2::Delay1<KERNEL_TYPE>);
        m_delay_2_kernel = adf::kernel::create(us::L2::Delay2<KERNEL_TYPE>);

        adf::connect(image_points_from_pl_1.out[0], m_delay_1_kernel.in[0]);
        adf::connect(tx_def_ref_point.out[0], m_delay_1_kernel.in[1]);
        adf::connect(tx_def_delay_distance_1.out[0], m_delay_1_kernel.in[2]);
        adf::connect(direction.out[0], m_delay_1_kernel.in[3]);

        adf::connect(m_delay_1_kernel.out[0], m_delay_2_kernel.in[0]);
        //			adf::connect(debug_after_virtual_source.out[0], m_delay_2_kernel.in[0]);

        adf::connect(image_points_from_pl_2.out[0], m_delay_2_kernel.in[1]);
        adf::connect(tx_def_delay_distance_2.out[0], m_delay_2_kernel.in[2]);
        adf::connect(tx_def_focal_point.out[0], m_delay_2_kernel.in[3]);
        adf::connect(t_start.out[0], m_delay_2_kernel.in[4]);
        adf::connect(speed_of_sound.out[0], m_delay_2_kernel.in[5]);
        adf::connect(m_delay_2_kernel.out[0], delay_output.in[0]);

        adf::dimensions(m_delay_1_kernel.in[0]) = {LEN * M_COLUMNS};
        adf::dimensions(m_delay_1_kernel.in[1]) = {LEN * M_COLUMNS};
        adf::dimensions(m_delay_1_kernel.in[2]) = {LEN};
        adf::dimensions(m_delay_1_kernel.in[3]) = {SIMD_DEPTH};
        adf::dimensions(m_delay_1_kernel.out[0]) = {LEN};

        adf::dimensions(m_delay_2_kernel.in[0]) = {LEN};
        adf::dimensions(m_delay_2_kernel.in[1]) = {LEN * M_COLUMNS};
        adf::dimensions(m_delay_2_kernel.in[2]) = {LEN};
        adf::dimensions(m_delay_2_kernel.in[3]) = {LEN * M_COLUMNS};
        adf::dimensions(m_delay_2_kernel.in[4]) = {LEN};
        adf::dimensions(m_delay_2_kernel.in[5]) = {LEN};
        adf::dimensions(m_delay_2_kernel.out[0]) = {LEN};

        adf::source(m_delay_1_kernel) = "l2-libraries_aieml/delay/delay.cpp";

        adf::runtime<adf::ratio>(m_delay_1_kernel) = 1;

        adf::source(m_delay_2_kernel) = "l2-libraries_aieml/delay/delay.cpp";

        adf::runtime<adf::ratio>(m_delay_2_kernel) = 1;

#endif
    }

   private:
    adf::kernel m_delay_1_kernel;
    adf::kernel m_delay_2_kernel;
};
