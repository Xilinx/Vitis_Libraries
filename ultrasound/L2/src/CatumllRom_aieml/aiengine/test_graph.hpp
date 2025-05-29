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
#include "l2-libraries_aieml/catmull_rom/catmull_rom.hpp"
#include "common_defines.hpp"

class TestGraphCatmullRom : public adf::graph {
   public:
    adf::input_plio point_0;
    adf::input_plio point_1;
    adf::input_plio point_2;
    adf::input_plio point_3;
    adf::input_plio t1t;
    adf::input_plio t2t;
    adf::input_plio t3t;
    adf::input_plio tt0;
    adf::input_plio tt1;
    adf::input_plio tt2;

    adf::output_plio output_catmull_rom;

    TestGraphCatmullRom() {
#if (defined(__AIESIM__) || defined(__X86SIM__) || defined(__ADF_FRONTEND__)) && !TEST_BUFFER

        point_0 = adf::input_plio::create("point_0", adf::plio_32_bits, "data/point_0.txt");
        point_1 = adf::input_plio::create("point_1", adf::plio_32_bits, "data/point_1.txt");
        point_2 = adf::input_plio::create("point_2", adf::plio_32_bits, "data/point_2.txt");
        point_3 = adf::input_plio::create("point_3", adf::plio_32_bits, "data/point_3.txt");
        t1t = adf::input_plio::create("t1t", adf::plio_32_bits, "data/t1t.txt");
        t2t = adf::input_plio::create("t2t", adf::plio_32_bits, "data/t2t.txt");
        t3t = adf::input_plio::create("t3t", adf::plio_32_bits, "data/t3t.txt");
        tt0 = adf::input_plio::create("tt0", adf::plio_32_bits, "data/tt0.txt");
        tt1 = adf::input_plio::create("tt1", adf::plio_32_bits, "data/tt1.txt");
        tt2 = adf::input_plio::create("tt2", adf::plio_32_bits, "data/tt2.txt");

        output_catmull_rom = adf::output_plio::create("output_c", adf::plio_32_bits, "data/output_catmull_rom.txt");

        m_catmull_rom_a1_a2_kernel = adf::kernel::create(us::L2::CatmullRomA1A2<KERNEL_TYPE>);
        m_catmull_rom_a3_b1_kernel = adf::kernel::create(us::L2::CatmullRomA3B1<KERNEL_TYPE>);
        m_catmull_rom_b2_c_output_kernel = adf::kernel::create(us::L2::CatmullRomB2C<KERNEL_TYPE>);

        // A1 - A2 stage
        adf::connect(point_0.out[0], m_catmull_rom_a1_a2_kernel.in[0]);
        adf::connect(point_1.out[0], m_catmull_rom_a1_a2_kernel.in[1]);
        adf::connect(point_2.out[0], m_catmull_rom_a1_a2_kernel.in[2]);
        adf::connect(t1t.out[0], m_catmull_rom_a1_a2_kernel.in[3]);

        adf::connect(tt0.out[0], m_catmull_rom_a1_a2_kernel.in[4]);
        adf::connect(t2t.out[0], m_catmull_rom_a1_a2_kernel.in[5]);
        adf::connect(tt1.out[0], m_catmull_rom_a1_a2_kernel.in[6]);

        // A3 - B1 stage
        adf::connect(m_catmull_rom_a1_a2_kernel.out[0], m_catmull_rom_a3_b1_kernel.in[0]);
        adf::connect(m_catmull_rom_a1_a2_kernel.out[1], m_catmull_rom_a3_b1_kernel.in[1]);
        adf::connect(point_2.out[0], m_catmull_rom_a3_b1_kernel.in[2]);
        adf::connect(point_3.out[0], m_catmull_rom_a3_b1_kernel.in[3]);

        adf::connect(t3t.out[0], m_catmull_rom_a3_b1_kernel.in[4]);
        adf::connect(tt2.out[0], m_catmull_rom_a3_b1_kernel.in[5]);
        adf::connect(t2t.out[0], m_catmull_rom_a3_b1_kernel.in[6]);
        adf::connect(tt0.out[0], m_catmull_rom_a3_b1_kernel.in[7]);

        // B2 - C stage
        adf::connect(m_catmull_rom_a1_a2_kernel.out[1], m_catmull_rom_b2_c_output_kernel.in[0]);
        adf::connect(m_catmull_rom_a3_b1_kernel.out[0], m_catmull_rom_b2_c_output_kernel.in[1]);
        adf::connect(m_catmull_rom_a3_b1_kernel.out[1], m_catmull_rom_b2_c_output_kernel.in[2]);

        adf::connect(t3t.out[0], m_catmull_rom_b2_c_output_kernel.in[3]);
        adf::connect(tt1.out[0], m_catmull_rom_b2_c_output_kernel.in[4]);
        adf::connect(t2t.out[0], m_catmull_rom_b2_c_output_kernel.in[5]);

        adf::connect(m_catmull_rom_b2_c_output_kernel.out[0], output_catmull_rom.in[0]);

        // A1 - A2 stage
        adf::dimensions(m_catmull_rom_a1_a2_kernel.in[0]) = {LEN};
        adf::dimensions(m_catmull_rom_a1_a2_kernel.in[1]) = {LEN};
        adf::dimensions(m_catmull_rom_a1_a2_kernel.in[2]) = {LEN};
        adf::dimensions(m_catmull_rom_a1_a2_kernel.in[3]) = {LEN};
        adf::dimensions(m_catmull_rom_a1_a2_kernel.in[4]) = {LEN};
        adf::dimensions(m_catmull_rom_a1_a2_kernel.in[5]) = {LEN};
        adf::dimensions(m_catmull_rom_a1_a2_kernel.in[6]) = {LEN};
        adf::dimensions(m_catmull_rom_a1_a2_kernel.out[0]) = {LEN};
        adf::dimensions(m_catmull_rom_a1_a2_kernel.out[1]) = {LEN};
        // A3 - B1 stage
        adf::dimensions(m_catmull_rom_a3_b1_kernel.in[0]) = {LEN};
        adf::dimensions(m_catmull_rom_a3_b1_kernel.in[1]) = {LEN};
        adf::dimensions(m_catmull_rom_a3_b1_kernel.in[2]) = {LEN};
        adf::dimensions(m_catmull_rom_a3_b1_kernel.in[3]) = {LEN};
        adf::dimensions(m_catmull_rom_a3_b1_kernel.in[4]) = {LEN};
        adf::dimensions(m_catmull_rom_a3_b1_kernel.in[5]) = {LEN};
        adf::dimensions(m_catmull_rom_a3_b1_kernel.in[6]) = {LEN};
        adf::dimensions(m_catmull_rom_a3_b1_kernel.in[7]) = {LEN};
        adf::dimensions(m_catmull_rom_a3_b1_kernel.out[0]) = {LEN};
        adf::dimensions(m_catmull_rom_a3_b1_kernel.out[1]) = {LEN};
        // B2 - C stage
        adf::dimensions(m_catmull_rom_b2_c_output_kernel.in[0]) = {LEN};
        adf::dimensions(m_catmull_rom_b2_c_output_kernel.in[1]) = {LEN};
        adf::dimensions(m_catmull_rom_b2_c_output_kernel.in[2]) = {LEN};
        adf::dimensions(m_catmull_rom_b2_c_output_kernel.in[3]) = {LEN};
        adf::dimensions(m_catmull_rom_b2_c_output_kernel.in[4]) = {LEN};
        adf::dimensions(m_catmull_rom_b2_c_output_kernel.in[5]) = {LEN};
        adf::dimensions(m_catmull_rom_b2_c_output_kernel.out[0]) = {LEN};

        adf::source(m_catmull_rom_a1_a2_kernel) = "l2-libraries_aieml/catmull_rom/catmull_rom.cpp";
        adf::source(m_catmull_rom_a3_b1_kernel) = "l2-libraries_aieml/catmull_rom/catmull_rom.cpp";
        adf::source(m_catmull_rom_b2_c_output_kernel) = "l2-libraries_aieml/catmull_rom/catmull_rom.cpp";

        adf::runtime<adf::ratio>(m_catmull_rom_a1_a2_kernel) = 0.5;
        adf::runtime<adf::ratio>(m_catmull_rom_a3_b1_kernel) = 0.5;
        adf::runtime<adf::ratio>(m_catmull_rom_b2_c_output_kernel) = 0.5;

#endif

#if (defined(__AIESIM__) || defined(__X86SIM__) || defined(__ADF_FRONTEND__)) && TEST_BUFFER

        point_0 = adf::input_plio::create("point_0", adf::plio_32_bits, "data/point_0.txt");
        point_1 = adf::input_plio::create("point_1", adf::plio_32_bits, "data/point_1.txt");
        point_2 = adf::input_plio::create("point_2", adf::plio_32_bits, "data/point_2.txt");
        point_3 = adf::input_plio::create("point_3", adf::plio_32_bits, "data/point_3.txt");
        t1t = adf::input_plio::create("t1t", adf::plio_32_bits, "data/t1t.txt");
        t2t = adf::input_plio::create("t2t", adf::plio_32_bits, "data/t2t.txt");
        t3t = adf::input_plio::create("t3t", adf::plio_32_bits, "data/t3t.txt");
        tt0 = adf::input_plio::create("tt0", adf::plio_32_bits, "data/tt0.txt");
        tt1 = adf::input_plio::create("tt1", adf::plio_32_bits, "data/tt1.txt");
        tt2 = adf::input_plio::create("tt2", adf::plio_32_bits, "data/tt2.txt");

        output_catmull_rom = adf::output_plio::create("output_c", adf::plio_32_bits, "data/output_catmull_rom.txt");

        m_catmull_rom_a1_a2_kernel = adf::kernel::create(us::L2::CatmullRomA1A2<KERNEL_TYPE>);
        m_catmull_rom_a3_b1_kernel = adf::kernel::create(us::L2::CatmullRomA3B1<KERNEL_TYPE>);
        m_catmull_rom_b2_c_output_kernel = adf::kernel::create(us::L2::CatmullRomB2C<KERNEL_TYPE>);

        // A1 - A2 stage
        adf::connect(point_0.out[0], m_catmull_rom_a1_a2_kernel.in[0]);
        adf::connect(point_1.out[0], m_catmull_rom_a1_a2_kernel.in[1]);
        adf::connect(point_2.out[0], m_catmull_rom_a1_a2_kernel.in[2]);
        adf::connect(t1t.out[0], m_catmull_rom_a1_a2_kernel.in[3]);

        adf::connect(tt0.out[0], m_catmull_rom_a1_a2_kernel.in[4]);
        adf::connect(t2t.out[0], m_catmull_rom_a1_a2_kernel.in[5]);
        adf::connect(tt1.out[0], m_catmull_rom_a1_a2_kernel.in[6]);

        // A3 - B1 stage
        adf::connect(m_catmull_rom_a1_a2_kernel.out[0], m_catmull_rom_a3_b1_kernel.in[0]);
        adf::connect(m_catmull_rom_a1_a2_kernel.out[1], m_catmull_rom_a3_b1_kernel.in[1]);
        adf::connect(point_2.out[0], m_catmull_rom_a3_b1_kernel.in[2]);
        adf::connect(point_3.out[0], m_catmull_rom_a3_b1_kernel.in[3]);

        adf::connect(t3t.out[0], m_catmull_rom_a3_b1_kernel.in[4]);
        adf::connect(tt2.out[0], m_catmull_rom_a3_b1_kernel.in[5]);
        adf::connect(t2t.out[0], m_catmull_rom_a3_b1_kernel.in[6]);
        adf::connect(tt0.out[0], m_catmull_rom_a3_b1_kernel.in[7]);

        // B2 - C stage
        adf::connect(m_catmull_rom_a1_a2_kernel.out[1], m_catmull_rom_b2_c_output_kernel.in[0]);
        adf::connect(m_catmull_rom_a3_b1_kernel.out[0], m_catmull_rom_b2_c_output_kernel.in[1]);
        adf::connect(m_catmull_rom_a3_b1_kernel.out[1], m_catmull_rom_b2_c_output_kernel.in[2]);

        adf::connect(t3t.out[0], m_catmull_rom_b2_c_output_kernel.in[3]);
        adf::connect(tt1.out[0], m_catmull_rom_b2_c_output_kernel.in[4]);
        adf::connect(t2t.out[0], m_catmull_rom_b2_c_output_kernel.in[5]);

        adf::connect(m_catmull_rom_b2_c_output_kernel.out[0], output_catmull_rom.in[0]);

        // A1 - A2 stage
        adf::dimensions(m_catmull_rom_a1_a2_kernel.in[0]) = {LEN};
        adf::dimensions(m_catmull_rom_a1_a2_kernel.in[1]) = {LEN};
        adf::dimensions(m_catmull_rom_a1_a2_kernel.in[2]) = {LEN};
        adf::dimensions(m_catmull_rom_a1_a2_kernel.in[3]) = {LEN};
        adf::dimensions(m_catmull_rom_a1_a2_kernel.in[4]) = {LEN};
        adf::dimensions(m_catmull_rom_a1_a2_kernel.in[5]) = {LEN};
        adf::dimensions(m_catmull_rom_a1_a2_kernel.in[6]) = {LEN};
        adf::dimensions(m_catmull_rom_a1_a2_kernel.out[0]) = {LEN};
        adf::dimensions(m_catmull_rom_a1_a2_kernel.out[1]) = {LEN};
        // A3 - B1 stage
        adf::dimensions(m_catmull_rom_a3_b1_kernel.in[0]) = {LEN};
        adf::dimensions(m_catmull_rom_a3_b1_kernel.in[1]) = {LEN};
        adf::dimensions(m_catmull_rom_a3_b1_kernel.in[2]) = {LEN};
        adf::dimensions(m_catmull_rom_a3_b1_kernel.in[3]) = {LEN};
        adf::dimensions(m_catmull_rom_a3_b1_kernel.in[4]) = {LEN};
        adf::dimensions(m_catmull_rom_a3_b1_kernel.in[5]) = {LEN};
        adf::dimensions(m_catmull_rom_a3_b1_kernel.in[6]) = {LEN};
        adf::dimensions(m_catmull_rom_a3_b1_kernel.in[7]) = {LEN};
        adf::dimensions(m_catmull_rom_a3_b1_kernel.out[0]) = {LEN};
        adf::dimensions(m_catmull_rom_a3_b1_kernel.out[1]) = {LEN};
        // B2 - C stage
        adf::dimensions(m_catmull_rom_b2_c_output_kernel.in[0]) = {LEN};
        adf::dimensions(m_catmull_rom_b2_c_output_kernel.in[1]) = {LEN};
        adf::dimensions(m_catmull_rom_b2_c_output_kernel.in[2]) = {LEN};
        adf::dimensions(m_catmull_rom_b2_c_output_kernel.in[3]) = {LEN};
        adf::dimensions(m_catmull_rom_b2_c_output_kernel.in[4]) = {LEN};
        adf::dimensions(m_catmull_rom_b2_c_output_kernel.in[5]) = {LEN};
        adf::dimensions(m_catmull_rom_b2_c_output_kernel.out[0]) = {LEN};

        adf::source(m_catmull_rom_a1_a2_kernel) = "l2-libraries_aieml/catmull_rom/catmull_rom.cpp";
        adf::source(m_catmull_rom_a3_b1_kernel) = "l2-libraries_aieml/catmull_rom/catmull_rom.cpp";
        adf::source(m_catmull_rom_b2_c_output_kernel) = "l2-libraries_aieml/catmull_rom/catmull_rom.cpp";

        adf::runtime<adf::ratio>(m_catmull_rom_a1_a2_kernel) = 0.5;
        adf::runtime<adf::ratio>(m_catmull_rom_a3_b1_kernel) = 0.5;
        adf::runtime<adf::ratio>(m_catmull_rom_b2_c_output_kernel) = 0.5;

#endif
    }

   private:
    adf::kernel m_catmull_rom_a1_a2_kernel;
    adf::kernel m_catmull_rom_a3_b1_kernel;
    adf::kernel m_catmull_rom_b2_c_output_kernel;
};
