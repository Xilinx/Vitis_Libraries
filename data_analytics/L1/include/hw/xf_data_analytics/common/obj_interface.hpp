/*
 * Copyright 2019-2022 Xilinx, Inc.
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

struct Object : private ap_uint<88> {
    // data: ap_uint<64>, the object stream data, maximum 64bits
    void set_data(ap_uint<64> data) { ap_uint<88>::range(63, 0) = data; }

    // filed ID: ap_uint<16>, indicate the col field, maximum supporting 256 fields
    void set_id(ap_uint<16> data) { ap_uint<88>::range(79, 64) = data; }

    // valid: ap_uint<4>, 0 for null, non-zero for the number of valid byte from LSB
    void set_valid(ap_uint<4> data) { ap_uint<88>::range(83, 80) = data; }

    // type: ap_uint<4>, 0000-boolean, 0001-int64, 0010-float, 0011-double, 0100-date, 0101-string
    // flag: 1101-end of json line, 1110-end of column, 1111-end of file
    void set_type(ap_uint<4> data) { ap_uint<88>::range(87, 84) = data; }

    ap_uint<64> get_data() { return ap_uint<88>::range(63, 0); }
    ap_uint<16> get_id() { return ap_uint<88>::range(79, 64); }
    ap_uint<4> get_valid() { return ap_uint<88>::range(83, 80); }
    ap_uint<4> get_type() { return ap_uint<88>::range(87, 84); }
    ap_uint<88> get_all() { return ap_uint<88>::range(87, 0); }
};

#define OBJ_W 73

struct ObjectAlter1 : private ap_uint<OBJ_W> {
    // data: ap_uint<64>, the object stream data, maximum 64bits
    void set_data(ap_uint<64> data) { ap_uint<OBJ_W>::range(63, 0) = data; }

    // filed ID: ap_uint<16>, indicate the col field, maximum supporting 256 fields
    void set_id(ap_uint<1> data) { ap_uint<OBJ_W>::range(64, 64) = data; }

    // valid: ap_uint<4>, 0 for null, non-zero for the number of valid byte from LSB
    void set_valid(ap_uint<4> data) { ap_uint<OBJ_W>::range(68, 65) = data; }

    // type: ap_uint<4>, 0000-boolean, 0001-int64, 0010-float, 0011-double, 0100-date, 0101-string
    // flag: 1101-end of json line, 1110-end of column, 1111-end of file
    void set_type(ap_uint<4> data) { ap_uint<OBJ_W>::range(72, 69) = data; }
    void set_all(ap_uint<OBJ_W> data) { ap_uint<OBJ_W>::range(72, 0) = data; }

    ap_uint<64> get_data() { return ap_uint<OBJ_W>::range(63, 0); }
    ap_uint<1> get_id() { return ap_uint<OBJ_W>::range(64, 64); }
    ap_uint<4> get_valid() { return ap_uint<OBJ_W>::range(68, 65); }
    ap_uint<4> get_type() { return ap_uint<OBJ_W>::range(72, 69); }
    ap_uint<OBJ_W> get_all() { return ap_uint<OBJ_W>::range(72, 0); }
};

//#undef OBJ_W

#define OBJ_WF 77

struct ObjectFile : private ap_uint<OBJ_WF> {
    // data: ap_uint<64>, the object stream data, maximum 64bits
    void set_data(ap_uint<64> data) { ap_uint<OBJ_WF>::range(63, 0) = data; }

    // filed ID: ap_uint<16>, indicate the col field, maximum supporting 256 fields
    void set_id(ap_uint<1> data) { ap_uint<OBJ_WF>::range(64, 64) = data; }

    // valid: ap_uint<4>, 0 for null, non-zero for the number of valid byte from LSB
    void set_valid(ap_uint<4> data) { ap_uint<OBJ_WF>::range(68, 65) = data; }

    // type: ap_uint<4>, 0000-boolean, 0001-int64, 0010-float, 0011-double, 0100-date, 0101-string
    // flag: 1101-end of json line, 1110-end of column, 1111-end of file
    void set_type(ap_uint<4> data) { ap_uint<OBJ_WF>::range(72, 69) = data; }
    void set_file(ap_uint<4> data) { ap_uint<OBJ_WF>::range(76, 73) = data; }
    void set_obj(ap_uint<OBJ_W> data) { ap_uint<OBJ_WF>::range(OBJ_W - 1, 0) = data; }
    void set_all(ap_uint<OBJ_WF> data) { ap_uint<OBJ_WF>::range(OBJ_WF - 1, 0) = data; }

    ap_uint<64> get_data() { return ap_uint<OBJ_WF>::range(63, 0); }
    ap_uint<1> get_id() { return ap_uint<OBJ_WF>::range(64, 64); }
    ap_uint<4> get_valid() { return ap_uint<OBJ_WF>::range(68, 65); }
    ap_uint<4> get_type() { return ap_uint<OBJ_WF>::range(72, 69); }
    ap_uint<4> get_file() { return ap_uint<OBJ_WF>::range(76, 73); }
    ap_uint<OBJ_WF> get_all() { return ap_uint<OBJ_WF>::range(OBJ_WF - 1, 0); }
};

} // end of dataframe namespace
} // end of data_analytics namespace
} // end of xf namespace

#endif
