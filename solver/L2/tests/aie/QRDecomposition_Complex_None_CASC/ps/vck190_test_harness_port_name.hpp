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

#ifndef _VCK190_TEST_HARNESS_GRAPH_PORT_NAME_HPP_
#define _VCK190_TEST_HARNESS_GRAPH_PORT_NAME_HPP_

#include <vector>
#include <string>

static std::vector<std::string> used_pl_in_name;
static std::vector<std::string> used_pl_out_name;

namespace vck190_test_harness {

static std::vector<std::string> in_names = {
    "Column_12_TO_AIE", "Column_13_TO_AIE", "Column_14_TO_AIE", "Column_15_TO_AIE",
    "Column_16_TO_AIE", "Column_17_TO_AIE", "Column_18_TO_AIE", "Column_19_TO_AIE",
    "Column_20_TO_AIE", "Column_21_TO_AIE", "Column_22_TO_AIE", "Column_23_TO_AIE",
    "Column_24_TO_AIE", "Column_25_TO_AIE", "Column_26_TO_AIE", "Column_27_TO_AIE"};
static std::vector<std::string> out_names = {
    "Column_28_FROM_AIE", "Column_29_FROM_AIE", "Column_30_FROM_AIE", "Column_31_FROM_AIE",
    "Column_32_FROM_AIE", "Column_33_FROM_AIE", "Column_34_FROM_AIE", "Column_35_FROM_AIE",
    "Column_36_FROM_AIE", "Column_37_FROM_AIE", "Column_38_FROM_AIE", "Column_39_FROM_AIE",
    "Column_40_FROM_AIE", "Column_41_FROM_AIE", "Column_42_FROM_AIE", "Column_43_FROM_AIE"};
}
#endif
