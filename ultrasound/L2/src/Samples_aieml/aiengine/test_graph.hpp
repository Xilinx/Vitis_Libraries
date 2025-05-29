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
#include "l2-libraries_aieml/samples/samples.hpp"
#include "common_defines.hpp"

class TestGraphSamples : public adf::graph {
   public:
    adf::input_plio image_points;
    adf::input_plio delay;
    adf::input_plio xdc_def_positions;
    adf::input_plio sampling_frequency;
    adf::input_plio speed_of_sound;
    adf::output_plio output_vector;

    TestGraphSamples() {
#if (defined(__AIESIM__) || defined(__X86SIM__) || defined(__ADF_FRONTEND__))

        image_points = adf::input_plio::create("image_points", adf::plio_32_bits, "data/image_points.txt");
        delay = adf::input_plio::create("delay", adf::plio_32_bits, "data/delay.txt");
        xdc_def_positions =
            adf::input_plio::create("xdc_def_positions", adf::plio_32_bits, "data/xdc_def_positions.txt");
        sampling_frequency =
            adf::input_plio::create("sampling_frequency", adf::plio_32_bits, "data/sampling_frequency.txt");
        speed_of_sound = adf::input_plio::create("speed_of_sound", adf::plio_32_bits, "data/speed_of_sound.txt");
        output_vector = adf::output_plio::create("output_vector", adf::plio_32_bits, "data/output_vector.txt");

        m_samples_kernel = adf::kernel::create(Samples<KERNEL_TYPE>);

        adf::connect(image_points.out[0], m_samples_kernel.in[0]);
        adf::connect(delay.out[0], m_samples_kernel.in[1]);
        adf::connect(xdc_def_positions.out[0], m_samples_kernel.in[2]);
        adf::connect(sampling_frequency.out[0], m_samples_kernel.in[3]);
        adf::connect(speed_of_sound.out[0], m_samples_kernel.in[4]);
        adf::connect(m_samples_kernel.out[0], output_vector.in[0]);

        adf::dimensions(m_samples_kernel.in[0]) = {LEN * M_COLUMNS};
        adf::dimensions(m_samples_kernel.in[1]) = {LEN};
        adf::dimensions(m_samples_kernel.in[2]) = {LEN * M_COLUMNS};
        adf::dimensions(m_samples_kernel.in[3]) = {LEN};
        adf::dimensions(m_samples_kernel.in[4]) = {LEN};
        adf::dimensions(m_samples_kernel.out[0]) = {LEN};

        adf::source(m_samples_kernel) = "l2-libraries_aieml/samples/samples.cpp";

        adf::runtime<adf::ratio>(m_samples_kernel) = 0.5;

#endif
    }

   private:
    adf::kernel m_samples_kernel;
};
