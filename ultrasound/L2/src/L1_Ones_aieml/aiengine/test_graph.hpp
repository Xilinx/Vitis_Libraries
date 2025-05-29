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
#include "ones/ones.hpp"
#include "common_defines.hpp"

class TestGraph : public adf::graph {
   public:
    adf::input_plio in;
    adf::output_plio out;

    TestGraph() {
#if (defined(__AIESIM__) || defined(__X86SIM__) || defined(__ADF_FRONTEND__))

        out = adf::output_plio::create("data_out", adf::plio_32_bits, "data/output.txt");

        m_ones_kernel = adf::kernel::create(us::L1::Ones<KERNEL_TYPE, LEN, INCREMENT_V, SIMD_DEPTH>);
        adf::connect(m_ones_kernel.out[0], out.in[0]);

        adf::dimensions(m_ones_kernel.out[0]) = {LEN};

        adf::source(m_ones_kernel) = "ones/ones.cpp";

        adf::runtime<adf::ratio>(m_ones_kernel) = RUNTIME_RATIO_V;

#endif
    }

   private:
    adf::kernel m_ones_kernel;
};
