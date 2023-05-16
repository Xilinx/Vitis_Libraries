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
 * This file is part of Vitis Data Analytics Library.
 */
#ifndef XF_DATA_ANALYTICS_L1_DATAFRAME_CSV_PARSE_BLOCK_HPP
#define XF_DATA_ANALYTICS_L1_DATAFRAME_CSV_PARSE_BLOCK_HPP
#include "xf_data_analytics/dataframe/parser_blocks/csv_line_parser.hpp"
#include "xf_data_analytics/dataframe/parser_blocks/parse_double.hpp"
#include "xf_data_analytics/dataframe/parser_blocks/parse_date.hpp"
#include "xf_data_analytics/dataframe/df_utils.hpp"
#include "xf_data_analytics/common/obj_interface.hpp"
#include <hls_stream.h>
#include <ap_int.h>
#include <stdint.h>
#include "strtod.hpp"

namespace xf {
namespace data_analytics {
namespace dataframe {
namespace internal {

/**
 * @brief filter and dispatch the column to dedicated unit parser by user defination
 *
 **/
static void fieldFilter(const ap_uint<9> num_of_column,
                        ap_uint<8> type_buff[256],
                        hls::stream<ap_uint<8> >& i_data_strm,
                        hls::stream<bool>& i_e_strm,
                        hls::stream<bool>& i_vld_strm,
                        hls::stream<bool>& i_ln_e_strm,

                        // int64
                        hls::stream<ap_uint<8> >& o_int_strm,
                        hls::stream<bool>& e_int_strm,
                        hls::stream<bool>& v_int_strm,
                        // double
                        hls::stream<ap_uint<8> >& o_double_strm,
                        hls::stream<bool>& e_double_strm,
                        hls::stream<bool>& v_double_strm,
                        // string
                        hls::stream<ap_uint<8> >& o_str_strm,
                        hls::stream<bool>& e_str_strm,
                        hls::stream<bool>& v_str_strm,
                        // Boolean
                        hls::stream<bool>& o_bool_strm,
                        hls::stream<bool>& e_bool_strm,
                        // Date
                        hls::stream<ap_uint<8> >& o_date_strm,
                        hls::stream<bool>& e_date_strm,
                        hls::stream<bool>& v_date_strm,
                        // end of line
                        hls::stream<bool>& empty_ln_strm,
                        hls::stream<bool>& o_ln_e_strm) {
    ap_uint<4> col_idx = 0;
    ap_uint<8> data_type = type_buff[0];
    bool e = i_e_strm.read();
    empty_ln_strm.write(!e);
    while (!e) {
#pragma HLS pipeline II = 1
        ap_uint<8> c = i_data_strm.read();
        bool vld = i_vld_strm.read();
        bool e_ln = i_ln_e_strm.read();
        e = i_e_strm.read();

        // dispatch selected fields
        if (data_type[4] && num_of_column > 0) {
            switch (data_type(3, 0)) {
                case (TInt64): {
                    o_int_strm << c;
                    e_int_strm << false;
                    v_int_strm << (!vld);
                    break;
                }
                case (TFloat32):
                case (TDouble): {
                    o_double_strm << c;
                    e_double_strm << false;
                    v_double_strm << (!vld);
                    break;
                }
                case (TString): {
                    o_str_strm << c;
                    e_str_strm << false;
                    v_str_strm << (!vld);
                    break;
                }
                case (TBoolean): {
                    if (c == 'f' || c == 'F') {
                        o_bool_strm << false;
                        e_bool_strm << false;
                    } else if (c == 't' || c == 'T') {
                        o_bool_strm << true;
                        e_bool_strm << false;
                    }
                    break;
                }
                case (TDate): {
                    o_date_strm << c;
                    e_date_strm << false;
                    v_date_strm << (!vld);
                    break;
                }
            }
        }

        if (e_ln && num_of_column > 0) { // end of line
            o_ln_e_strm << e;
            data_type = type_buff[0];
            col_idx = 0;
        } else if (vld) { // move to next field
            data_type = type_buff[col_idx + 1];
            col_idx++;
        }
    }

    // int
    o_int_strm << 0;
    e_int_strm << true;
    v_int_strm << false;
    // double
    // o_double_strm << 0;
    e_double_strm << true;
    // v_double_strm << false;
    // string
    o_str_strm << 0;
    e_str_strm << true;
    v_str_strm << false;
    // boolean
    e_bool_strm << true;
    // date
    e_date_strm << true;
}

/**
 *
 * @brief parse the input string stream, generate the offset.
 *
 * @param i_strm input stream of characters for each value.
 * @param i_e_strm input end flag of i_hash_strm.
 * @param vld_strm input valid flag for each character.
 * @param o_strm output formated 64-bits width stream for string.
 * @param o_byte_valid_strm byte valid for 64-bit o_strm
 * @param o_vld_strm last frame flag for each string.
 *
 */
static void parseString(hls::stream<ap_uint<8> >& i_strm,
                        hls::stream<bool>& i_e_strm,
                        hls::stream<bool>& i_vld_strm,

                        hls::stream<ap_uint<64> >& o_strm,
                        hls::stream<ap_uint<4> >& o_byte_valid_strm,
                        hls::stream<bool>& o_vld_strm) {
    ap_uint<64> str;
    ap_uint<4> len = 0;

    bool e = i_e_strm.read();
    bool vld = i_vld_strm.read();
    ap_uint<8> in = i_strm.read();
    bool nb_1 = true;
    bool nb_2 = true;
    bool nb_3 = true;

    while (!e) {
#pragma HLS pipeline II = 1
        if (len == 8 || (!vld && nb_1 && nb_2 && nb_3)) {
            o_vld_strm.write(!vld); // 1-last frame, 0-continue
            if (len == 8) {
                o_strm << str(63, 0);
                o_byte_valid_strm << 0x8;
            } else if (len > 0) {
                ap_uint<64> t;
                t(8 * len - 1, 0) = str(63, 8 * (8 - len));
                o_strm << t;
                o_byte_valid_strm << len;
            }
        }

        if (vld && nb_1 && nb_2 && nb_3) {
            str(55, 0) = str(63, 8);
            str(63, 56) = in;

            if (len == 8)
                len = 1;
            else
                len++;
        } else if (!vld)
            len = 0;

        nb_1 = i_strm.read_nb(in);
        nb_2 = i_e_strm.read_nb(e);
        nb_3 = i_vld_strm.read_nb(vld);
    }
}

/**
 *
 * @brief parse the input string stream and convert it to int64.
 *
 * @param i_strm input stream of characters for each value.
 * @param i_e_strm input end flag of i_strm.
 * @param vld_strm input valid flag for each character.
 * @param o_strm, output interger.
 *
 */
static void parseInt64(hls::stream<ap_uint<8> >& i_strm,
                       hls::stream<bool>& i_e_strm,
                       hls::stream<bool>& vld_strm,
                       hls::stream<ap_uint<64> >& o_strm) {
    ap_uint<64> value = 0;
    bool e = false;
    bool vld = true;
    bool is_neg = false;
    bool nb_1 = false;
    bool nb_2 = false;
    bool nb_3 = false;
    while (!e) {
#pragma HLS pipeline II = 1
        if (!vld && nb_1 && nb_2 && nb_3) {
            int64_t out = 0;
            if (is_neg)
                out = -value;
            else
                out = value;
            o_strm.write(out);
            value = 0;
            is_neg = false;
        }
        // non-blocking read input
        ap_uint<8> in;
        nb_1 = vld_strm.read_nb(vld);
        nb_2 = i_e_strm.read_nb(e);
        nb_3 = i_strm.read_nb(in);
        if (vld && nb_1 && nb_2 && nb_3) {
            if (in == '-')
                is_neg = true;
            else if (in >= '0' && in <= '9') {
                value = value * 10 + (in - '0');
            }
        }
    }
}

/**
 * @brief parse the input stream and convert it to 64bit.
 *
 * @param i_strm input stream of characters for each value.
 * @param i_e_strm input end flag of i_strm.
 * @param o_strm, output boolean
 *
 */
static void parseBoolean(hls::stream<bool>& i_strm, hls::stream<bool>& i_e_strm, hls::stream<ap_uint<64> >& o_strm) {
    bool nb_0 = false;
    bool nb_1 = false;
    bool e = false;
    while (!e) {
#pragma HLS pipeline II = 1
        bool d;
        nb_0 = i_strm.read_nb(d);
        nb_1 = i_e_strm.read_nb(e);
        if (!e && nb_0 && nb_1) {
            ap_uint<64> t = d ? 1 : 0;
            o_strm << d;
        }
    }
}

/*
 * @brief merge each valid field into a new row by object stream
 *
 **/
static void mergeField(const ap_uint<9> num_of_column,
                       ap_uint<4> type_valid_buff[256], // for all valid fields
                       // int64
                       hls::stream<ap_uint<64> >& i_int_strm,
                       // double
                       hls::stream<ap_uint<64> >& i_double_strm,
                       // float
                       hls::stream<ap_uint<32> >& i_float_strm,
                       // string
                       hls::stream<ap_uint<64> >& i_str_strm,
                       hls::stream<ap_uint<4> >& i_str_vld_strm,
                       hls::stream<bool>& i_str_e_strm,
                       // Boolean
                       hls::stream<ap_uint<64> >& i_bool_strm,
                       // Date
                       hls::stream<ap_uint<64> >& i_date_strm,
                       // EOF
                       hls::stream<bool>& i_empty_ln_strm,
                       hls::stream<bool>& i_ln_e_strm,

                       hls::stream<bool>& o_ln_e_strm,
                       hls::stream<Object>& o_obj_strm) {
    ap_uint<8> col_idx = 0;
    bool e0 = i_empty_ln_strm.read();
    bool e = false;
    while (!e && e0 && num_of_column > 0) {
#pragma HLS pipeline II = 2 style = flp
        ap_uint<64> d;
        ap_uint<4> byte_vld;
        bool e_vld;
        Object obj;
        obj.set_id(col_idx);

        ap_uint<4> data_type = type_valid_buff[col_idx];
        switch (data_type) {
            case (TInt64): {
                e_vld = true;
                i_int_strm >> d;
                obj.set_data(d);
                obj.set_valid(8);
                obj.set_type(TInt64);
                break;
            }
            case (TFloat32): {
                e_vld = true;
                i_double_strm.read(); // consume the dummy
                d(31, 0) = i_float_strm.read();
                obj.set_data(d);
                obj.set_valid(4);
                obj.set_type(TFloat32);
                break;
            }
            case (TDouble): {
                e_vld = true;
                i_float_strm.read(); // consume the dummy
                i_double_strm >> d;
                obj.set_data(d);
                obj.set_valid(8);
                obj.set_type(TDouble);
                break;
            }
            case (TString): {
                i_str_strm >> d;
                i_str_vld_strm >> byte_vld;
                i_str_e_strm >> e_vld;
                obj.set_data(d);
                obj.set_valid(byte_vld);
                obj.set_type(TString); // mask it when e_vld = 0
                break;
            }
            case (TBoolean): {
                e_vld = true;
                i_bool_strm >> d;
                obj.set_data(d);
                obj.set_valid(1);
                obj.set_type(TBoolean);
                break;
            }
            case (TDate): {
                e_vld = true;
                i_date_strm >> d;
                obj.set_data(d);
                obj.set_valid(8);
                obj.set_type(TDate);
                break;
            }
        }

        // emit the object
        o_obj_strm << obj;

        // update the column index
        if (e_vld) {                            // only variable-length TString/TNumeric will pull down the `e_vld`
            if (col_idx == num_of_column - 1) { // move to the next line
                Object t;
                t.set_type(FEOL);
                o_obj_strm << t;      // end of line
                o_ln_e_strm << false; // flag of EOL
                i_ln_e_strm >> e;
                col_idx = 0;
            } else
                col_idx++;
        }
    }

    // end of part of file
    Object obj;
    obj.set_type(FEOF);
    o_obj_strm << obj;
    o_ln_e_strm << false; // flag of EOF
}

/**
 * @brief parse the input string line and convert into object stream
 *
 * @param num_of_column number of selected column
 * @param type_buff user-defined data type for each column
 * @param type_valid_buff data type for user-selected column
 * @param i_strm input stream of string.
 * @param i_e_strm input end flag of i_strm.
 * @param o_ln_e_strm end of each line
 * @param o_object_strm output object stream
 *
 */
static void parseBlock(const ap_uint<9> num_of_column,
                       ap_uint<8> type_buff[256],
                       ap_uint<4> type_valid_buff[256],
                       hls::stream<ap_uint<8> >& i_strm,
                       hls::stream<bool>& i_e_strm,

                       hls::stream<bool>& o_ln_e_strm,
                       hls::stream<Object>& o_object_strm) {
#pragma HLS dataflow

    hls::stream<ap_uint<8> > iter_data_strm;
#pragma HLS stream variable = iter_data_strm depth = 8
    hls::stream<bool> iter_e_strm;
#pragma HLS stream variable = iter_e_strm depth = 8
    hls::stream<bool> iter_vld_strm;
#pragma HLS stream variable = iter_vld_strm depth = 8
    hls::stream<bool> iter_ln_e_strm;
#pragma HLS stream variable = iter_ln_e_strm depth = 8

    // split the fields, reserve the delimiter
    iterativeCSVParse(i_strm, i_e_strm, iter_data_strm, iter_e_strm, iter_vld_strm, iter_ln_e_strm);

    hls::stream<ap_uint<8> > filter_int_strm("filter_int_strm");
#pragma HLS stream variable = filter_int_strm depth = 8
    hls::stream<bool> filter_int_e_strm("filter_int_e_strm");
#pragma HLS stream variable = filter_int_e_strm depth = 8
    hls::stream<bool> filter_int_v_strm("filter_int_v_strm");
#pragma HLS stream variable = filter_int_v_strm depth = 8
    hls::stream<ap_uint<8> > filter_double_strm("filter_double_strm");
#pragma HLS stream variable = filter_double_strm depth = 8
    hls::stream<bool> filter_double_e_strm("filter_double_e_strm");
#pragma HLS stream variable = filter_double_e_strm depth = 8
    hls::stream<bool> filter_double_v_strm("filter_double_v_strm");
#pragma HLS stream variable = filter_double_v_strm depth = 8
    hls::stream<ap_uint<8> > filter_str_strm("filter_str_strm");
#pragma HLS stream variable = filter_str_strm depth = 8
    hls::stream<bool> filter_str_e_strm("filter_str_e_strm");
#pragma HLS stream variable = filter_str_e_strm depth = 8
    hls::stream<bool> filter_str_v_strm("filter_str_v_strm");
#pragma HLS stream variable = filter_str_v_strm depth = 8
    hls::stream<bool> filter_bool_strm("filter_bool_strm");
#pragma HLS stream variable = filter_bool_strm depth = 8
    hls::stream<bool> filter_bool_e_strm("filter_bool_e_strm");
#pragma HLS stream variable = filter_bool_e_strm depth = 8
    hls::stream<ap_uint<8> > filter_date_strm("filter_date_strm");
#pragma HLS stream variable = filter_date_strm depth = 8
    hls::stream<bool> filter_date_e_strm("filter_date_e_strm");
#pragma HLS stream variable = filter_date_e_strm depth = 8
    hls::stream<bool> filter_date_v_strm("filter_date_v_strm");
#pragma HLS stream variable = filter_date_v_strm depth = 8
    hls::stream<bool> filter_o_empty_ln_strm("filter_o_empty_ln_strm");
#pragma HLS stream variable = filter_o_empty_ln_strm depth = 2
    hls::stream<bool> filter_o_ln_e_strm("filter_o_ln_e_strm");
#pragma HLS stream variable = filter_o_ln_e_strm depth = 64

    // filter out the selected columns
    fieldFilter(num_of_column, type_buff, iter_data_strm, iter_e_strm, iter_vld_strm, iter_ln_e_strm, filter_int_strm,
                filter_int_e_strm, filter_int_v_strm, filter_double_strm, filter_double_e_strm, filter_double_v_strm,
                filter_str_strm, filter_str_e_strm, filter_str_v_strm, filter_bool_strm, filter_bool_e_strm,
                filter_date_strm, filter_date_e_strm, filter_date_v_strm, filter_o_empty_ln_strm, filter_o_ln_e_strm);

    hls::stream<ap_uint<64> > int64_strm;
#pragma HLS stream variable = int64_strm depth = 8
    parseInt64(filter_int_strm, filter_int_e_strm, filter_int_v_strm, int64_strm);

    hls::stream<ap_uint<64> > double64_strm("double64_strm");
#pragma HLS stream variable = double64_strm depth = 8
    hls::stream<ap_uint<32> > float32_strm("float32_strm");
#pragma HLS stream variable = float32_strm depth = 8
    parseDouble(filter_double_strm, filter_double_e_strm, filter_double_v_strm, double64_strm, float32_strm);

    hls::stream<ap_uint<64> > string64_strm;
#pragma HLS stream variable = string64_strm depth = 8
    hls::stream<ap_uint<4> > string_o_byte_valid_strm;
#pragma HLS stream variable = string_o_byte_valid_strm depth = 8
    hls::stream<bool> string_o_vld_strm;
#pragma HLS stream variable = string_o_vld_strm depth = 8
    // combine char to form 64-bit data
    parseString(filter_str_strm, filter_str_e_strm, filter_str_v_strm, string64_strm, string_o_byte_valid_strm,
                string_o_vld_strm);

    hls::stream<ap_uint<64> > bool_strm;
#pragma HLS stream variable = bool_strm depth = 8
    parseBoolean(filter_bool_strm, filter_bool_e_strm, bool_strm);

    hls::stream<ap_uint<64> > date64_strm;
#pragma HLS stream variable = date64_strm depth = 8
    parseDate(filter_date_strm, filter_date_e_strm, filter_date_v_strm, date64_strm);

    // collect each valid field to form one line
    mergeField(num_of_column, type_valid_buff, int64_strm, double64_strm, float32_strm, string64_strm,
               string_o_byte_valid_strm, string_o_vld_strm, bool_strm, date64_strm, filter_o_empty_ln_strm,
               filter_o_ln_e_strm, o_ln_e_strm, o_object_strm);
}
}
}
}
}
#endif
