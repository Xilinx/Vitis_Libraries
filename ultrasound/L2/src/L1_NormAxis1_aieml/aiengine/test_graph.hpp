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
#include "norm_axis_1/norm_axis_1.hpp"
#include "common_defines.hpp"

class TestGraph : public adf::graph {
   public:
    adf::input_plio in;
    adf::output_plio out;

    TestGraph() {
#if (defined(__AIESIM__) || defined(__X86SIM__) || defined(__ADF_FRONTEND__)) && !TEST_BUFFER

        in = adf::input_plio::create("data_in", adf::plio_32_bits, "data/input.txt");
        out = adf::output_plio::create("data_out", adf::plio_32_bits, "data/output.txt");

        m_norm_axis_1_kernel = adf::kernel::create(us::L1::NormAxis1<KERNEL_TYPE, LEN, 1, M_COLUMNS>);
        adf::connect(in.out[0], m_norm_axis_1_kernel.in[0]);
        adf::connect(m_norm_axis_1_kernel.out[0], out.in[0]);

        adf::dimensions(m_norm_axis_1_kernel.in[0]) = {LEN * M_COLUMNS};
        adf::dimensions(m_norm_axis_1_kernel.out[0]) = {LEN};

        adf::source(m_norm_axis_1_kernel) = "norm_axis_1/norm_axis_1.cpp";

        adf::runtime<adf::ratio>(m_norm_axis_1_kernel) = RUNTIME_RATIO_M;

#endif

#if (defined(__AIESIM__) || defined(__X86SIM__) || defined(__ADF_FRONTEND__)) && TEST_BUFFER

        in = adf::input_plio::create("data_in", adf::plio_32_bits, "data/input.txt");
        out = adf::output_plio::create("data_out", adf::plio_32_bits, "data/output.txt");

        m_norm_axis_1_kernel = adf::kernel::create(us::L1::NormAxis1InternalBuffer<KERNEL_TYPE, LEN, 1, M_COLUMNS>);
        adf::connect(in.out[0], m_norm_axis_1_kernel.in[0]);
        adf::connect(m_norm_axis_1_kernel.out[0], out.in[0]);

        adf::dimensions(m_norm_axis_1_kernel.in[0]) = {LEN * M_COLUMNS};
        adf::dimensions(m_norm_axis_1_kernel.out[0]) = {LEN};

        adf::source(m_norm_axis_1_kernel) = "norm_axis_1/norm_axis_1.cpp";

        adf::runtime<adf::ratio>(m_norm_axis_1_kernel) = RUNTIME_RATIO_M;

#endif
    }

   private:
    adf::kernel m_norm_axis_1_kernel;
};
