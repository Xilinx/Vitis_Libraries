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
#include "l2-libraries_aieml/focusing/focusing.hpp"
#include "common_defines.hpp"

class TestGraphFocusing : public adf::graph {
   public:
    adf::input_plio apo_ref_0;
    adf::input_plio xdc_def_0;
    adf::input_plio apo_ref_1;
    adf::input_plio xdc_def_1;
    adf::output_plio out;

    TestGraphFocusing() {
#if (defined(__AIESIM__) || defined(__X86SIM__) || defined(__ADF_FRONTEND__))

        apo_ref_0 = adf::input_plio::create("apo_ref_0", adf::plio_32_bits, "data/apo_ref_0.txt");
        xdc_def_0 = adf::input_plio::create("xdc_def_0", adf::plio_32_bits, "data/xdc_def_0.txt");
        apo_ref_1 = adf::input_plio::create("apo_ref_1", adf::plio_32_bits, "data/apo_ref_1.txt");
        xdc_def_1 = adf::input_plio::create("xdc_def_1", adf::plio_32_bits, "data/xdc_def_1.txt");
        out = adf::output_plio::create("focusing_output", adf::plio_32_bits, "data/focusing_output.txt");

        m_focusing_kernel = adf::kernel::create(us::L2::Focusing<KERNEL_TYPE>);
        adf::connect(apo_ref_0.out[0], m_focusing_kernel.in[0]);
        adf::connect(xdc_def_0.out[0], m_focusing_kernel.in[1]);
        adf::connect(apo_ref_1.out[0], m_focusing_kernel.in[2]);
        adf::connect(xdc_def_1.out[0], m_focusing_kernel.in[3]);
        adf::connect(m_focusing_kernel.out[0], out.in[0]);

        adf::dimensions(m_focusing_kernel.in[0]) = {LEN};
        adf::dimensions(m_focusing_kernel.in[1]) = {LEN};
        adf::dimensions(m_focusing_kernel.in[2]) = {LEN};
        adf::dimensions(m_focusing_kernel.in[3]) = {LEN};
        adf::dimensions(m_focusing_kernel.out[0]) = {LEN};

        adf::source(m_focusing_kernel) = "l2-libraries_aieml/focusing/focusing.cpp";

        adf::runtime<adf::ratio>(m_focusing_kernel) = 1;

#endif
    }

   private:
    adf::kernel m_focusing_kernel;
};
