/*
 * Copyright (C) 2019-2022, Xilinx, Inc.
 * Copyright (C) 2022-2023, Advanced Micro Devices, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * @file df_utils.hpp
 * This file is part of Vitis Data Analytics Library.
 */

#ifndef _XF_DATA_ANALYTICS_L1_DATAFRAME_DF_UTILS_HPP_
#define _XF_DATA_ANALYTICS_L1_DATAFRAME_DF_UTILS_HPP_

#include <ap_int.h>

namespace xf {
namespace data_analytics {
namespace dataframe {

// define the data types supported in DataFrame,
// and flags used in Object stream
enum TypeFlags {
    TBoolean = 0,
    TInt64 = 1,
    TFloat32 = 2,
    TDouble = 3,
    TDate = 4,
    TString = 5,
    TNumeric = 6,
    TCount = 7,
    FEOL = 13, // end of json line
    FEOC = 14, // end of column
    FEOF = 15  // end of file
};

struct Node {
    ap_uint<12> next;
    ap_uint<12> base_addr;
};

struct FieldInfo {
    ap_uint<12> head;
    ap_uint<12> tail;
    ap_uint<20> size; // ap_uint<32>
};

} // namespace dataframe
} // namespace data_analytics
} // namespace xf

#endif
