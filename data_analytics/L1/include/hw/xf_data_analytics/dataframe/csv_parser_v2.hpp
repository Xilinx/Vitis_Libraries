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
 * @file csv_parser_v2.hpp
 * @brief parse one standard CSV file, output each field row by row with obj-stream interface
 * TARGET user: KNN api in geospatial.
 * Handle line index in MergeLineUnitL1 module.
 *
 * This file is part of Vitis Data Analytics Library.
 */

#ifndef XF_DATA_ANALYTICS_L1_DATAFRAME_CSV_PARSER_V2_HPP
#define XF_DATA_ANALYTICS_L1_DATAFRAME_CSV_PARSER_V2_HPP

#include "xf_data_analytics/common/obj_interface.hpp"
#include "xf_data_analytics/dataframe/df_utils.hpp"
#include "xf_data_analytics/dataframe/parser_blocks/read_block.hpp"
#include "xf_data_analytics/dataframe/parser_blocks/csv_parse_block.hpp"

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
template <int PU_NUM>
void readSchema(ap_uint<8>* schema,
                ap_uint<8> type_buff[PU_NUM][256],
                ap_uint<4> type_valid_buff[PU_NUM][256],
                ap_uint<9>& num_of_column) {
    ap_uint<9> valid_column = 0;
    int cnt = 0;
    int addr = 2;
    bool str_start = false;
    bool val_start = false;
    bool mk_start = false;
    ap_uint<4> type_tmp;

    ap_uint<16> nm;
    nm.range(7, 0) = schema[0];
    nm.range(15, 8) = schema[1];
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
        } else if (!str_start && in == ':') {
            val_start = true;
        } else if (val_start) {
            val_start = false;
            type_tmp = in(3, 0);
        } else if (!mk_start && in == ' ') {
            mk_start = true;
        } else if (mk_start) {
            mk_start = false;
            for (int j = 0; j < PU_NUM; ++j) {
#pragma HLS unroll
                if (in == '1') type_valid_buff[j][valid_column] = type_tmp;
                type_buff[j][cnt] = ((ap_uint<3>)0, in[0], type_tmp(3, 0));
            }
            if (in == '1') valid_column++;
        } else { // column name
        }
    }

    num_of_column = valid_column;
}

/**
 * @brief merge two obj-stream by load-balance type, and sink the line break signal
 **/
inline void mergeLineUnitL1(ap_uint<2> pu_grp,
                            hls::stream<Object> i_obj_array_strm[2],
                            hls::stream<bool> i_ln_e_strm[2],

                            hls::stream<Object>& o_obj_strm,
                            ap_uint<32> line_cnt[2]) {
    line_cnt[0] = 0;
    line_cnt[1] = 0;
    ap_uint<1> pu_idx = 0;
    ap_uint<2> e = 0;
    Object obj;

MERGE_CORE_LOOP:
    while (e != (ap_uint<2>)-1) {
#pragma HLS pipeline II = 1
        if (!e[pu_idx] && !i_ln_e_strm[pu_idx].empty()) {
            i_obj_array_strm[pu_idx] >> obj;
            if (obj.get_type() == FEOL || obj.get_type() == FEOF) i_ln_e_strm[pu_idx].read();
            if (obj.get_type() == FEOF) {
                e[pu_idx] = true;
            } else if (obj.get_type() == FEOL) {
                ap_uint<32> tmp;
                tmp.range(31, 30) = pu_grp;
                tmp[29] = pu_idx;
                tmp.range(28, 0) = line_cnt[pu_idx]++;
                // #ifndef __SYNTHESIS__
                //                 std::cout << tmp.range(31, 30) << " " << tmp[29] << " " << pu_idx << " " <<
                //                 tmp.range(28, 0)
                //                           << std::endl;
                // #endif
                obj.set_data(tmp);
                o_obj_strm << obj;
            } else {
                o_obj_strm << obj;
            }
        }

        if (e[pu_idx] || i_ln_e_strm[pu_idx].empty() || (!e[pu_idx] && obj.get_type() == FEOL)) {
            pu_idx = !pu_idx;
        }
    }

    obj.set_type(FEOF); // EOF
    o_obj_strm << obj;
}

/**
 * @brief merge two obj-stream by load-balance type
 **/
inline void mergeLineUnitL2(hls::stream<Object> i_obj_array_strm[2],

                            hls::stream<Object>& o_obj_strm) {
    ap_uint<1> pu_idx = 0;

    ap_uint<2> e = 0;
    Object obj;

MERGE_CORE_LOOP:
    while (e != (ap_uint<2>)-1) {
#pragma HLS pipeline II = 1
        if (!e[pu_idx] && !i_obj_array_strm[pu_idx].empty()) {
            i_obj_array_strm[pu_idx] >> obj;
            if (obj.get_type() == FEOF) {
                e[pu_idx] = true;
            } else {
                o_obj_strm << obj;
            }
        }

        if (e[pu_idx] || (!e[pu_idx] && obj.get_type() == FEOL)) {
            pu_idx = !pu_idx;
        }
    }

    obj.set_type(FEOF); // EOF
    o_obj_strm << obj;
}

/**
 * @brief top function of merging obj-stream from PUs, support 2/4/8-to-1 only
 **/
template <int PU_NUM>
inline void mergeLine(hls::stream<Object> i_obj_array_strm[PU_NUM / 2][2],
                      hls::stream<bool> i_ln_e_strm[PU_NUM / 2][2],

                      ap_uint<32> line_cnt[PU_NUM / 2][2],
                      hls::stream<Object>& o_obj_strm) {
#pragma HLS dataflow

    if (PU_NUM == 2) {
        mergeLineUnitL1(0, i_obj_array_strm[0], i_ln_e_strm[0], o_obj_strm, line_cnt[0]);
    }
    if (PU_NUM == 4) {
        hls::stream<Object> obj_l2_strm[2];
#pragma HLS stream variable = obj_l2_strm depth = 8

        mergeLineUnitL1(0, i_obj_array_strm[0], i_ln_e_strm[0], obj_l2_strm[0], line_cnt[0]);
        mergeLineUnitL1(1, i_obj_array_strm[1], i_ln_e_strm[1], obj_l2_strm[1], line_cnt[1]);

        mergeLineUnitL2(obj_l2_strm, o_obj_strm);
    }
    if (PU_NUM == 8) {
        hls::stream<Object> obj_l1_strm[2][2];
#pragma HLS stream variable = obj_l1_strm depth = 8
        hls::stream<Object> obj_l2_strm[2];
#pragma HLS stream variable = obj_l2_strm depth = 8

        mergeLineUnitL1(0, i_obj_array_strm[0], i_ln_e_strm[0], obj_l1_strm[0][0], line_cnt[0]);
        mergeLineUnitL1(1, i_obj_array_strm[1], i_ln_e_strm[1], obj_l1_strm[0][1], line_cnt[1]);
        mergeLineUnitL1(2, i_obj_array_strm[2], i_ln_e_strm[2], obj_l1_strm[1][0], line_cnt[2]);
        mergeLineUnitL1(3, i_obj_array_strm[3], i_ln_e_strm[3], obj_l1_strm[1][1], line_cnt[3]);

        mergeLineUnitL2(obj_l1_strm[0], obj_l2_strm[0]);
        mergeLineUnitL2(obj_l1_strm[1], obj_l2_strm[1]);

        mergeLineUnitL2(obj_l2_strm, o_obj_strm);
    }
}

/**
 * @brief main function of CSV parser
 **/
template <int PU_NUM>
void parseCSVCore(ap_uint<128>* csv_buff,
                  ap_uint<8> type_buff[PU_NUM][256],
                  ap_uint<4> type_valid_buff[PU_NUM][256],
                  const ap_uint<9> num_of_column,

                  ap_uint<32> line_cnt[PU_NUM / 2][2],
                  hls::stream<Object>& o_obj_strm) {
#pragma HLS dataflow

    hls::stream<ap_uint<8> > s_w8_strm[PU_NUM];
#pragma HLS stream variable = s_w8_strm depth = 8
#pragma HLS array_partition variable = s_w8_strm dim = 0
    hls::stream<bool> s_e_strm[PU_NUM];
#pragma HLS stream variable = s_e_strm depth = 8
#pragma HLS array_partition variable = s_e_strm dim = 0

    hls::stream<Object> t_obj_array_strm[PU_NUM / 2][2];
#pragma HLS stream variable = t_obj_array_strm depth = 1024
#pragma HLS array_partition variable = t_obj_array_strm dim = 0
#pragma HLS bind_storage variable = t_obj_array_strm type = fifo impl = bram
    hls::stream<bool> o_ln_e_strm[PU_NUM / 2][2];
#pragma HLS stream variable = o_ln_e_strm depth = 64
#pragma HLS array_partition variable = o_ln_e_strm dim = 0
#pragma HLS bind_storage variable = o_ln_e_strm type = fifo impl = lutram

    readCSV<PU_NUM>(csv_buff, s_w8_strm, s_e_strm);

    if (PU_NUM >= 2) {
        parseBlock(num_of_column, type_buff[0], type_valid_buff[0], s_w8_strm[0], s_e_strm[0], o_ln_e_strm[0][0],
                   t_obj_array_strm[0][0]);
        parseBlock(num_of_column, type_buff[1], type_valid_buff[1], s_w8_strm[1], s_e_strm[1], o_ln_e_strm[0][1],
                   t_obj_array_strm[0][1]);
    }
    if (PU_NUM >= 4) {
        parseBlock(num_of_column, type_buff[2], type_valid_buff[2], s_w8_strm[2], s_e_strm[2], o_ln_e_strm[1][0],
                   t_obj_array_strm[1][0]);
        parseBlock(num_of_column, type_buff[3], type_valid_buff[3], s_w8_strm[3], s_e_strm[3], o_ln_e_strm[1][1],
                   t_obj_array_strm[1][1]);
    }
    if (PU_NUM >= 8) {
        parseBlock(num_of_column, type_buff[4], type_valid_buff[4], s_w8_strm[4], s_e_strm[4], o_ln_e_strm[2][0],
                   t_obj_array_strm[2][0]);
        parseBlock(num_of_column, type_buff[5], type_valid_buff[5], s_w8_strm[5], s_e_strm[5], o_ln_e_strm[2][1],
                   t_obj_array_strm[2][1]);
        parseBlock(num_of_column, type_buff[6], type_valid_buff[6], s_w8_strm[6], s_e_strm[6], o_ln_e_strm[3][0],
                   t_obj_array_strm[3][0]);
        parseBlock(num_of_column, type_buff[7], type_valid_buff[7], s_w8_strm[7], s_e_strm[7], o_ln_e_strm[3][1],
                   t_obj_array_strm[3][1]);
    }

    mergeLine<PU_NUM>(t_obj_array_strm, o_ln_e_strm, line_cnt, o_obj_strm);
}

} // namespace internal

/**
 * @brief read one standard CSV file from DDR and parse into object stream with schma defination
 *
 * @tparam PU_NUM number of CSV parse core, only support 2/4/8
 * @param csv_buf buffer of CSV file
 * @param schema name, data type and is_filter flag for each column
 * @param line_cnt_buf output num of csv lines that each pu processes.
 * @param o_obj_strm output object stream for selected columns
 *
 **/
template <int PU_NUM = 8>
void csvParser(ap_uint<128>* csv_buf,
               ap_uint<8>* schema,

               ap_uint<32> line_cnt_buf[PU_NUM / 2][2],
               hls::stream<Object>& o_obj_strm) {
    ap_uint<8> type_buf[PU_NUM][256];
#pragma HLS array_partition variable = type_buf dim = 1
#pragma HLS bind_storage variable = type_buf type = ram_1p impl = lutram
    ap_uint<4> type_valid_buf[PU_NUM][256];
#pragma HLS array_partition variable = type_valid_buf dim = 1
#pragma HLS bind_storage variable = type_valid_buf type = ram_1p impl = lutram

    static_assert((PU_NUM == 2 || PU_NUM == 4 || PU_NUM == 8), "Only support 2/4/8 PU setting");

    ap_uint<9> num_of_column;

    internal::readSchema<PU_NUM>(schema, type_buf, type_valid_buf, num_of_column);

    internal::parseCSVCore<PU_NUM>(csv_buf, type_buf, type_valid_buf, num_of_column, line_cnt_buf, o_obj_strm);
}
} // namespace dataframe
} // namespace data_analytics
} // namespace xf
#endif
