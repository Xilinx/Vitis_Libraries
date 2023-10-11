/*
 * MIT License
 *
 * Copyright (C) 2023 Advanced Micro Devices, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated
 * documentation files (the “Software”), to deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 * WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Except as contained in this notice, the name of Advanced Micro Devices, Inc. shall not be used in advertising or
 * otherwise to promote the sale, use or other dealings in this Software without prior written authorization from
 * Advanced Micro Devices, Inc.
 */

#ifndef _VCK190_TEST_HARNESS_GRAPH_HPP_
#define _VCK190_TEST_HARNESS_GRAPH_HPP_

#include <vector>
#include <string>
#include <adf.h>
#include <adf/stream/types.h>
#include "vck190_test_harness_port_name.hpp"

using namespace adf;
using namespace std;

void dummy_out(output_stream_int32* out);
void dummy_in(input_stream_int32* in);

namespace vck190_test_harness {

template <int used_in_plio, int used_out_plio>
class occupyUnusedPLIO : public graph {
   public:
    kernel k_in[16 - used_in_plio];
    kernel k_out[16 - used_out_plio];
    input_plio pl_in[16 - used_in_plio];
    output_plio pl_out[16 - used_out_plio];

    occupyUnusedPLIO(std::vector<std::string> used_in_plio_names, std::vector<std::string> used_out_plio_names) {
        {
            int k = 0;
            for (int i = 0; i < 16; i++) {
                if (std::count(used_in_plio_names.begin(), used_in_plio_names.end(),
                               vck190_test_harness::in_names[i]) == 0) {
                    pl_in[k] =
                        input_plio::create(vck190_test_harness::in_names[i], adf::plio_128_bits, "dummy.txt", 250);
                    k_in[k] = kernel::create(dummy_in);
                    source(k_in[k]) = "dummy_kernel.cc";
                    runtime<ratio>(k_in[k]) = 0.01;
                    connect<stream> net(pl_in[k].out[0], k_in[k].in[0]);
                    k++;
                }
            }
        }
        {
            int k = 0;
            for (int i = 0; i < 16; i++) {
                if (std::count(used_out_plio_names.begin(), used_out_plio_names.end(),
                               vck190_test_harness::out_names[i]) == 0) {
                    pl_out[k] =
                        output_plio::create(vck190_test_harness::out_names[i], adf::plio_128_bits, "dummy.txt", 250);
                    k_out[k] = kernel::create(dummy_out);
                    source(k_out[k]) = "dummy_kernel.cc";
                    runtime<ratio>(k_out[k]) = 0.01;
                    connect<stream> net(k_out[k].out[0], pl_out[k].in[0]);
                    k++;
                }
            }
        }
    }
};
}

#endif
