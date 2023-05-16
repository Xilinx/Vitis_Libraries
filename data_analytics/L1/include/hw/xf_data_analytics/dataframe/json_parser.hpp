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
 * @file json_parser.hpp
 * @brief parse one standard pre-flatten JSON file, output each field row by row with obj-stream interface
 *
 * This file is part of Vitis Data Analytics Library.
 */

#ifndef XF_DATA_ANALYTICS_L1_DATAFRAME_JSON_PARSER_HPP
#define XF_DATA_ANALYTICS_L1_DATAFRAME_JSON_PARSER_HPP

#include "xf_data_analytics/common/obj_interface.hpp"
#include "xf_data_analytics/dataframe/df_utils.hpp"
#include "xf_data_analytics/dataframe/parser_blocks/read_block.hpp"
#include "xf_data_analytics/dataframe/parser_blocks/json_parse_block.hpp"

#ifndef __SYNTHESIS__
#include <iostream>
#endif

namespace xf {
namespace data_analytics {
namespace dataframe {

namespace internal {

/**
 * @brief read the schema and cache the type of each enable column
 **/
template <int PU_NUM, int COL_NUM, int FIELD_BW, int TYPE_BW>
void readSchema(ap_uint<8>* schema,

                ap_uint<FIELD_BW>& num_of_column,
                ap_uint<COL_NUM>& mask_cfg,
                ap_uint<8> key_buff[PU_NUM][COL_NUM][256],
                ap_uint<TYPE_BW> type_buff[PU_NUM][COL_NUM]) {
    int cnt = 0;
    int addr = 4;
    bool str_start = false;
    bool val_start = false;
    ap_uint<8> str_len = 0;
    ap_uint<FIELD_BW> column_nm = 0;

    ap_uint<16> nm;
    mask_cfg.range(7, 0) = schema[0];
    mask_cfg.range(15, 8) = schema[1];
    nm.range(7, 0) = schema[2];
    nm.range(15, 8) = schema[3];
    // init key buffer
    for (int i = 0; i < 256; ++i) {
        for (int j = 0; j < COL_NUM; ++j) {
#pragma HLS pipeline II = 1
            for (int k = 0; k < PU_NUM; ++k) {
                key_buff[k][j][i] = '\0';
            }
        }
    }

READ_SCHEMA_CORE_LOOP:
    for (int i = 0; i < nm; ++i) {
#pragma HLS pipeline II = 1
        ap_uint<8> in = schema[addr + i];
        if (in == '\n')
            cnt++;
        else if (!str_start && in == '"') {
            str_start = true;
        } else if (str_start && in == '"') {
            str_start = false;
            str_len = 0;
        } else if (!str_start && in == ':') {
            val_start = true;
        } else if (val_start) {
            val_start = false;
            for (int j = 0; j < PU_NUM; ++j) {
#pragma HLS unroll
                type_buff[j][cnt] = in - '0';
            }
            column_nm++;
        } else {
            for (int j = 0; j < PU_NUM; ++j) {
#pragma HLS unroll
                if (in != '/') {
                    key_buff[j][cnt][str_len] = in;
                } else {
                    key_buff[j][cnt][str_len] = 0;
                }
            }
            str_len++;
        }
    }

    num_of_column = column_nm;
}

/**
 * @brief merge two obj-stream by load-balance type, and sink the line break signal
 **/
inline void mergeLineUnitL1(hls::stream<ObjectEx> i_obj_array_strm[2],
                            hls::stream<bool> i_ln_e_strm[2],

                            hls::stream<ObjectEx>& o_obj_strm) {
    ap_uint<1> pu_idx = 0;

    ap_uint<2> e = 0;
    ObjectEx obj;

MERGE_CORE_LOOP:
    while (e != (ap_uint<2>)-1) {
#pragma HLS pipeline II = 1
        if (!e[pu_idx] && !i_ln_e_strm[pu_idx].empty()) {
            obj = i_obj_array_strm[pu_idx].read();
            if (obj.get_type() == FEOL || obj.get_type() == FEOF) {
                bool drop = i_ln_e_strm[pu_idx].read();
            }
            if (obj.get_type() == FEOF) {
                e[pu_idx] = true;
            } else {
                o_obj_strm.write(obj);
            }
        }

        if (e[pu_idx] || i_ln_e_strm[pu_idx].empty() || (!e[pu_idx] && obj.get_type() == FEOL)) {
            pu_idx = !pu_idx;
        }
    }

    obj.set_type(FEOF); // EOF
    o_obj_strm.write(obj);
}

/**
 * @brief merge two obj-stream by load-balance type
 **/
inline void mergeLineUnitL2(hls::stream<ObjectEx> i_obj_array_strm[2],

                            hls::stream<ObjectEx>& o_obj_strm) {
    ap_uint<1> pu_idx = 0;

    ap_uint<2> e = 0;
    ObjectEx obj;

MERGE_CORE_LOOP:
    while (e != (ap_uint<2>)-1) {
#pragma HLS pipeline II = 1
        if (!e[pu_idx]) {
            obj = i_obj_array_strm[pu_idx].read();
            if (obj.get_type() == FEOF) {
                e[pu_idx] = true;
            } else {
                o_obj_strm.write(obj);
            }
        }

        if (e[pu_idx] || (!e[pu_idx] && obj.get_type() == FEOL)) {
            pu_idx = !pu_idx;
        }
    }

    obj.set_type(FEOF); // EOF
    o_obj_strm.write(obj);
}

/**
 * @brief top function of merging obj-stream from PUs, support 2/4/8-to-1 only
 **/
template <int PU_NUM>
inline void mergeLine(hls::stream<ObjectEx> i_obj_array_strm[PU_NUM / 2][2],
                      hls::stream<bool> i_ln_e_strm[PU_NUM / 2][2],
                      hls::stream<ObjectEx>& o_obj_strm) {
#pragma HLS dataflow

    if (PU_NUM == 2) {
        mergeLineUnitL1(i_obj_array_strm[0], i_ln_e_strm[0], o_obj_strm);
    }
    if (PU_NUM == 4) {
        hls::stream<ObjectEx> obj_l2_strm[2];
#pragma HLS stream variable = obj_l2_strm depth = 8

        mergeLineUnitL1(i_obj_array_strm[0], i_ln_e_strm[0], obj_l2_strm[0]);
        mergeLineUnitL1(i_obj_array_strm[1], i_ln_e_strm[1], obj_l2_strm[1]);

        mergeLineUnitL2(obj_l2_strm, o_obj_strm);
    }
    if (PU_NUM == 8) {
        hls::stream<ObjectEx> obj_l1_strm[2][2];
#pragma HLS stream variable = obj_l1_strm depth = 8
        hls::stream<ObjectEx> obj_l2_strm[2];
#pragma HLS stream variable = obj_l2_strm depth = 8

        mergeLineUnitL1(i_obj_array_strm[0], i_ln_e_strm[0], obj_l1_strm[0][0]);
        mergeLineUnitL1(i_obj_array_strm[1], i_ln_e_strm[1], obj_l1_strm[0][1]);
        mergeLineUnitL1(i_obj_array_strm[2], i_ln_e_strm[2], obj_l1_strm[1][0]);
        mergeLineUnitL1(i_obj_array_strm[3], i_ln_e_strm[3], obj_l1_strm[1][1]);

        mergeLineUnitL2(obj_l1_strm[0], obj_l2_strm[0]);
        mergeLineUnitL2(obj_l1_strm[1], obj_l2_strm[1]);

        mergeLineUnitL2(obj_l2_strm, o_obj_strm);
    }
}

/**
 * @brief main function of JSON parser
 **/
template <int PU_NUM, int COL_NUM, int ARRAY_BW, int FIELD_BW, int TYPE_BW, int VALID_BW, int DATA_BW>
void parseJSONCore(ap_uint<128>* json_buff,
                   ap_uint<FIELD_BW> num_of_column,
                   ap_uint<COL_NUM> mask_cfg,
                   ap_uint<8> key_buff[PU_NUM][COL_NUM][256],
                   ap_uint<TYPE_BW> type_buff[PU_NUM][COL_NUM],
                   hls::stream<ObjectEx>& o_obj_strm) {
#pragma HLS dataflow

    hls::stream<ap_uint<8> > s_w8_strm[PU_NUM];
#pragma HLS stream variable = s_w8_strm depth = 8
    hls::stream<bool> s_e_strm[PU_NUM];
#pragma HLS stream variable = s_e_strm depth = 8

    hls::stream<ObjectEx> t_obj_array_strm[PU_NUM / 2][2];
#pragma HLS stream variable = t_obj_array_strm depth = 1024
#pragma HLS bind_storage variable = t_obj_array_strm type = fifo impl = bram
    hls::stream<bool> o_ln_e_strm[PU_NUM / 2][2];
#pragma HLS stream variable = o_ln_e_strm depth = 64
#pragma HLS bind_storage variable = o_ln_e_strm type = fifo impl = lutram

    readJSON<PU_NUM>(json_buff, s_w8_strm, s_e_strm);

    if (PU_NUM >= 2) {
        parseBlock<COL_NUM, ARRAY_BW, FIELD_BW, TYPE_BW, VALID_BW, DATA_BW>(num_of_column, mask_cfg, key_buff[0],
                                                                            type_buff[0], s_w8_strm[0], s_e_strm[0],
                                                                            o_ln_e_strm[0][0], t_obj_array_strm[0][0]);
        parseBlock<COL_NUM, ARRAY_BW, FIELD_BW, TYPE_BW, VALID_BW, DATA_BW>(num_of_column, mask_cfg, key_buff[1],
                                                                            type_buff[1], s_w8_strm[1], s_e_strm[1],
                                                                            o_ln_e_strm[0][1], t_obj_array_strm[0][1]);
    }
    if (PU_NUM >= 4) {
        parseBlock<COL_NUM, ARRAY_BW, FIELD_BW, TYPE_BW, VALID_BW, DATA_BW>(num_of_column, mask_cfg, key_buff[2],
                                                                            type_buff[2], s_w8_strm[2], s_e_strm[2],
                                                                            o_ln_e_strm[1][0], t_obj_array_strm[1][0]);
        parseBlock<COL_NUM, ARRAY_BW, FIELD_BW, TYPE_BW, VALID_BW, DATA_BW>(num_of_column, mask_cfg, key_buff[3],
                                                                            type_buff[3], s_w8_strm[3], s_e_strm[3],
                                                                            o_ln_e_strm[1][1], t_obj_array_strm[1][1]);
    }
    if (PU_NUM >= 8) {
        parseBlock<COL_NUM, ARRAY_BW, FIELD_BW, TYPE_BW, VALID_BW, DATA_BW>(num_of_column, mask_cfg, key_buff[4],
                                                                            type_buff[4], s_w8_strm[4], s_e_strm[4],
                                                                            o_ln_e_strm[2][0], t_obj_array_strm[2][0]);
        parseBlock<COL_NUM, ARRAY_BW, FIELD_BW, TYPE_BW, VALID_BW, DATA_BW>(num_of_column, mask_cfg, key_buff[5],
                                                                            type_buff[5], s_w8_strm[5], s_e_strm[5],
                                                                            o_ln_e_strm[2][1], t_obj_array_strm[2][1]);
        parseBlock<COL_NUM, ARRAY_BW, FIELD_BW, TYPE_BW, VALID_BW, DATA_BW>(num_of_column, mask_cfg, key_buff[6],
                                                                            type_buff[6], s_w8_strm[6], s_e_strm[6],
                                                                            o_ln_e_strm[3][0], t_obj_array_strm[3][0]);
        parseBlock<COL_NUM, ARRAY_BW, FIELD_BW, TYPE_BW, VALID_BW, DATA_BW>(num_of_column, mask_cfg, key_buff[7],
                                                                            type_buff[7], s_w8_strm[7], s_e_strm[7],
                                                                            o_ln_e_strm[3][1], t_obj_array_strm[3][1]);
    }

    mergeLine<PU_NUM>(t_obj_array_strm, o_ln_e_strm, o_obj_strm);
}

/**
 * @brief self-defined power function for integer type, to be used in assertions
 *
 **/
constexpr int my_pow(int base, int exp) {
    int result = 1;
    for (int i = 0; i < exp; i++) {
        result *= base;
    }
    return result;
}

} // namespace internal

/**
 * @brief read one standard JSON file from DDR and parse into object stream according to given schema
 *
 * @tparam PU_NUM number of paralleled JSON parse core, only support 2/4/8
 * @tparam COL_NUM maximum supported number of columns, should be power of 2 and less than or equal to 2^FIELD_BW
 * @tparam ARRAY_BW bit width of index for each element in array (all 1's represent for the last element of the array or
 * non-array value)
 * @tparam FIELD_BW bit width of number of fields (columns)
 * @tparam TYPE_BW bit width of data type (including line separator / end of file etc.)
 * @tparam VALID_BW bit width of number of valid bytes in data (from LSB)
 * @tparam DATA_BW bit width of data
 *
 * @param json_buf buffer of JSON file to be parsed
 * @param schema key name, nested status, and data type of value for each column
 * @param o_obj_strm output object stream for selected columns
 *
 **/
template <int PU_NUM = 4,
          int COL_NUM = 16,
          int ARRAY_BW = 4,
          int FIELD_BW = 4,
          int TYPE_BW = 4,
          int VALID_BW = 4,
          int DATA_BW = 64>
void jsonParser(ap_uint<128>* json_buf, ap_uint<8>* schema, hls::stream<ObjectEx>& o_obj_strm) {
    ap_uint<8> key_buf[PU_NUM][COL_NUM][256];
#pragma HLS array_partition variable = key_buf dim = 1
#pragma HLS array_partition variable = key_buf dim = 2
#pragma HLS bind_storage variable = key_buf type = ram_1p impl = lutram
    ap_uint<TYPE_BW> type_buf[PU_NUM][COL_NUM];
#pragma HLS array_partition variable = type_buf dim = 1
#pragma HLS bind_storage variable = type_buf type = ram_1p impl = lutram

    static_assert((PU_NUM == 2 || PU_NUM == 4 || PU_NUM == 8), "ERROR: only support 2/4/8 PU settings\n");
    static_assert(
        internal::my_pow(2, FIELD_BW) >= COL_NUM,
        "ERROR: insufficient bit width of FIELD_BW for maximum number of fields that is required by COL_NUM\n");

    ap_uint<FIELD_BW> num_of_column;
    ap_uint<COL_NUM> mask_cfg;

    internal::readSchema<PU_NUM, COL_NUM, FIELD_BW, TYPE_BW>(schema, num_of_column, mask_cfg, key_buf, type_buf);

    internal::parseJSONCore<PU_NUM, COL_NUM, ARRAY_BW, FIELD_BW, TYPE_BW, VALID_BW, DATA_BW>(
        json_buf, num_of_column, mask_cfg, key_buf, type_buf, o_obj_strm);
}
} // namespace dataframe
} // namespace data_analytics
} // namespace xf
#endif
