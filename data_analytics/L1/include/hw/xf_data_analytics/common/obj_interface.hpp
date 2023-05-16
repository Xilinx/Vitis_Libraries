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
 * @file obj_interface.hpp
 * @brief the shared interface struct between dataframe and CSV / JSON parser
 *
 * This file is part of Vitis Data Analytics Library.
 */

#ifndef _XF_DATA_ANALYTICS_L1_OBJ_INTERFACE_HPP_
#define _XF_DATA_ANALYTICS_L1_OBJ_INTERFACE_HPP_

#include <ap_int.h>

namespace xf {
namespace data_analytics {
namespace dataframe {

template <int FILE_W, int ARRAY_W, int FIELD_W, int TYPE_W, int VALID_W, int DATA_W>
struct ObjectBase : private ap_uint<FILE_W + ARRAY_W + FIELD_W + TYPE_W + VALID_W + DATA_W> {
    // data: the object stream data
    void set_data(ap_uint<DATA_W> data) { this->range(DATA_W - 1, 0) = data; }

    // valid: 0 for null, non-zero for the number of valid bytes from LSB
    void set_valid(ap_uint<VALID_W> data) { this->range(VALID_W + DATA_W - 1, DATA_W) = data; }

    // type: 0000-boolean, 0001-int64, 0010-float, 0011-double, 0100-date, 0101-string
    // flag: 1101-end of json line, 1110-end of column, 1111-end of file
    void set_type(ap_uint<TYPE_W> data) { this->range(TYPE_W + VALID_W + DATA_W - 1, VALID_W + DATA_W) = data; }

    // filed ID: indicate the col field, maximum supporting 2^FIELD_W fields
    void set_id(ap_uint<FIELD_W> data) {
        this->range(FIELD_W + TYPE_W + VALID_W + DATA_W - 1, TYPE_W + VALID_W + DATA_W) = data;
    }

    // offset of array: indicate the index of the element in each array, -1 stands for non-array value
    // or end of the array, maximum supported length of array is 2^ARRAY_W - 1
    void set_offset(ap_uint<ARRAY_W> data) {
        this->range(ARRAY_W + FIELD_W + TYPE_W + VALID_W + DATA_W - 1, FIELD_W + TYPE_W + VALID_W + DATA_W) = data;
    }

    void set_file(ap_uint<FILE_W> data) {
        this->range(FILE_W + ARRAY_W + FIELD_W + TYPE_W + VALID_W + DATA_W - 1,
                    ARRAY_W + FIELD_W + TYPE_W + VALID_W + DATA_W) = data;
    }

    void set_obj(ap_uint<ARRAY_W + FIELD_W + TYPE_W + VALID_W + DATA_W> data) {
        this->range(ARRAY_W + FIELD_W + TYPE_W + VALID_W + DATA_W - 1, 0) = data;
    }

    void set_all(ap_uint<FILE_W + ARRAY_W + FIELD_W + TYPE_W + VALID_W + DATA_W> data) {
        this->range(FILE_W + ARRAY_W + FIELD_W + TYPE_W + VALID_W + DATA_W - 1, 0) = data;
    }

    ap_uint<DATA_W> get_data() { return this->range(DATA_W - 1, 0); }
    ap_uint<VALID_W> get_valid() { return this->range(VALID_W + DATA_W - 1, DATA_W); }
    ap_uint<TYPE_W> get_type() { return this->range(TYPE_W + VALID_W + DATA_W - 1, VALID_W + DATA_W); }
    ap_uint<FIELD_W> get_id() {
        return this->range(FIELD_W + TYPE_W + VALID_W + DATA_W - 1, TYPE_W + VALID_W + DATA_W);
    }
    ap_uint<ARRAY_W> get_offset() {
        return this->range(ARRAY_W + FIELD_W + TYPE_W + VALID_W + DATA_W - 1, FIELD_W + TYPE_W + VALID_W + DATA_W);
    }
    ap_uint<FILE_W> get_file() {
        return this->range(FILE_W + ARRAY_W + FIELD_W + TYPE_W + VALID_W + DATA_W - 1,
                           ARRAY_W + FIELD_W + TYPE_W + VALID_W + DATA_W);
    }
    ap_uint<ARRAY_W + FIELD_W + TYPE_W + VALID_W + DATA_W> get_all() {
        return this->range(ARRAY_W + FIELD_W + TYPE_W + VALID_W + DATA_W - 1, 0);
    }
};

using Object = ObjectBase<0, 0, 16, 4, 4, 64>;
using ObjectAlter1 = ObjectBase<0, 0, 1, 4, 4, 64>;
using ObjectFile = ObjectBase<4, 0, 1, 4, 4, 64>;
using ObjectEx = ObjectBase<0, 4, 4, 4, 4, 64>;

} // end of dataframe namespace
} // end of data_analytics namespace
} // end of xf namespace

#endif
