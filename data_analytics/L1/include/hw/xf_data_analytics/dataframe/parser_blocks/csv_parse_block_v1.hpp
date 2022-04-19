/*
 * Copyright 2022 Xilinx, Inc.
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

#ifndef _CSV_PARSE_BLOCK_HPP_
#define _CSV_PARSE_BLOCK_HPP_
#include "xf_data_analytics/dataframe/parser_blocks/csv_line_parser_v1.hpp"
#include "xf_data_analytics/dataframe/parser_blocks/parse_numeric.hpp"
#include "xf_data_analytics/dataframe/df_utils.hpp"
#include "xf_data_analytics/common/obj_interface.hpp"
#include <hls_stream.h>
#include <ap_int.h>
#include <stdint.h>
#include "strtod.hpp"

#ifndef __SYNTHESIS__
#include <iostream>
#endif

//#define DOUBLE_TYPE_SUPPORT

namespace xf {
namespace data_analytics {
namespace dataframe {
namespace internal {

static void fieldFilter(ap_uint<8>* type_buff,
                        hls::stream<ap_uint<11> >& i_data_strm,

                        // int64
                        hls::stream<ap_uint<9> >& o_int_strm,
                        // string
                        hls::stream<ap_uint<9> >& o_str_strm,
                        // Boolean
                        hls::stream<bool>& o_bool_strm,
                        // Date
                        hls::stream<ap_uint<9> >& o_date_strm,
                        // Numeric
                        hls::stream<ap_uint<9> >& o_numeric_strm,
                        // end of line
                        hls::stream<bool>& o_ln_e_strm) {
    ap_uint<4> col_idx = 0;
    ap_uint<8> data_type = type_buff[0];
    int row_cnt = 0;
    ap_uint<11> in = i_data_strm.read();
    bool e = in[10];
    bool nb = true;
    while (!e) {
#pragma HLS pipeline II = 1
        if (nb) {
            ap_uint<9> c;
            c(7, 0) = in(7, 0);
            bool vld = in[8];
            c[8] = !vld;
            // std::cout << "c=" << c[8] << ", " << c(7, 0) << std::endl;
            bool e_ln = in[9];
#ifndef __SYNTHESIS__
//        std::cout << "if:" << std::hex << c << ", data_type=" << data_type << std::dec << ", col_idx=" << col_idx
//                  << ", row_cnt=" << row_cnt << ", e_ln=" << e_ln << ", e=" << e << std::endl;
#endif

            // dispatch selected fields
            if (data_type[4]) {
                switch (data_type(3, 0)) {
                    case (TInt64): {
#ifndef __SYNTHESIS__
// std::cout << "int64c:" << std::hex << c << std::dec << " " << !vld << std::endl;
#endif
                        o_int_strm << c;
                        break;
                    }
                    case (TFloat32):
                    case (TDouble): {
                        break;
                    }
                    case (TString): {
                        o_str_strm << c;
#ifndef __SYNTHESIS__
//                    std::cout << "strc:" << std::hex << c << std::dec << " " << !vld << std::endl;
#endif
                        break;
                    }
                    case (TBoolean): {
                        if (c == 'f' || c == 'F') {
                            o_bool_strm << false;
                        } else if (c == 't' || c == 'T') {
                            o_bool_strm << true;
                        }
                        break;
                    }
                    case (TDate): {
                        o_date_strm << c;
#ifndef __SYNTHESIS__
//                    std::cout << "datec:" << std::hex << c << std::dec << " " << !vld << std::endl;
#endif
                        break;
                    }
                    case (TNumeric): {
#ifndef __SYNTHESIS__
// std::cout << "num9c:" << std::hex << c << std::dec << std::endl;
#endif
                        o_numeric_strm << c;
                        break;
                    }
                }
            }

            if (e_ln) { // end of line
                o_ln_e_strm << false;
                data_type = type_buff[0];
                col_idx = 0;
                row_cnt++;
            } else if (vld) { // move to next field
                data_type = type_buff[col_idx + 1];
                col_idx++;
            }
        }
        nb = i_data_strm.read_nb(in);
        if (nb) e = in[10];
    }
    o_ln_e_strm << true;
}

/**
 *
 * @brief parse the input string stream, generate the offset.
 *
 * @param i_strm input stream of characters for each value.
 * @param vld_strm input valid flag for each character.
 * @param i_e_strm input end flag of i_hash_strm.
 * @param o_strm output formated 64-bits width stream for string.
 * @param o_vld_strm last frame flag for each string.
 *
 */
static void parseString(hls::stream<ap_uint<10> >& i_strm, hls::stream<ap_uint<64 + 4 + 1> >& o_strm) {
    ap_uint<72> str;
    ap_uint<4> len = 0;
    ap_uint<89> out;

    bool nb = true;
    bool nb_2 = true;
    bool nb_3 = true;

    ap_uint<10> in = i_strm.read();
    ap_uint<8> d = in(7, 0);
    bool vld = in[8];
    bool e = in[9];
    while (!e) {
#pragma HLS pipeline II = 1

        if (nb) {
            str(63, 0) = str(71, 8);
            str(71, 64) = d;
            if (len == 8 || !vld) {
                out[68] = !vld;
                if (len == 8) {
                    out(63, 0) = str(63, 0);
                    out(67, 64) = 8;
                } else if (len > 0) {
                    ap_uint<64> t;
                    t(8 * len - 1, 0) = str(63, 8 * (8 - len));
                    out(63, 0) = t;
                    out(67, 64) = len;
                }
                o_strm.write(out);
            }

            if (!vld)
                len = 0;
            else if (len == 8)
                len = 1;
            else
                len++;
        }
        nb = i_strm.read_nb(in);
        if (nb) {
            d = in(7, 0);
            vld = in[8];
            e = in[9];
        }
    }
}

static void parseDate(hls::stream<ap_uint<10> >& inStrm, hls::stream<ap_uint<64> >& outStrm) {
    ap_uint<16> year = 0;
    ap_uint<8> month = 0;
    ap_uint<8> day = 0;
    ap_uint<3> currPtr = 0;

    ap_uint<10> in10 = inStrm.read();
    char in = in10(7, 0);
    bool vld = in10[8];
    bool end = in10[9];
    bool nb = true;
    ap_uint<64> out = 0;
    while (!end) {
#pragma HLS pipeline II = 1
        if (nb) {
            if (vld) {
                // start or end, do nothing
                if (in == '"') {
                    // hit the separator, move to next section
                } else if (in == '-' || in == ' ' || in == ':') {
                    currPtr++;
                    // string to corresponding integer
                } else {
                    if (currPtr == 0) {
                        year = year * 10 + (in - '0');
                    } else if (currPtr == 1) {
                        month = month * 10 + (in - '0');
                    } else if (currPtr == 2) {
                        day = day * 10 + (in - '0');
                    }
                }
            } else {
                // emit outputs
                out(63, 32) = year;
                out(23, 16) = month;
                out(7, 0) = day;
                outStrm.write(out);
                year = 0;
                month = 0;
                day = 0;
                currPtr = 0;
            }
        }
        nb = inStrm.read_nb(in10);
        if (nb) {
            in = in10(7, 0);
            vld = in10[8];
            end = in10[9];
        }
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
static void parseInt64(hls::stream<ap_uint<10> >& i_strm, hls::stream<ap_uint<64> >& o_strm) {
    ap_uint<64> value = 0;
    ap_uint<10> in10 = i_strm.read();
    ap_uint<8> in = in10(7, 0);
    bool vld = in10[8];
    bool e = in10[9];

    bool is_neg = false;
    while (!e) {
#pragma HLS pipeline II = 1
        if (!vld) {
            int64_t out = 0;
            if (is_neg)
                out = -value;
            else
                out = value;
            o_strm.write(out);
            // std::cout << "parseInt data:" << value << std::endl;
            value = 0;
            is_neg = false;
        } else {
            if (in == '-')
                is_neg = true;
            else if (in >= '0' && in <= '9') {
                value = value * 10 + (in - '0');
            }
        }
        in10 = i_strm.read();
        in = in10(7, 0);
        vld = in10[8];
        e = in10[9];
    }
}

static void parseBoolean(hls::stream<ap_uint<2> >& i_strm, hls::stream<ap_uint<1> >& o_strm) {
    ap_uint<2> in = i_strm.read();
    while (!in[1]) {
#pragma HLS pipeline II = 1
        bool d = in[0];
        in = i_strm.read();
        o_strm << d;
    }
};

// merge each valid field into a new row by object stream
static void mergeField(ap_uint<4>* type_valid_buff, // for all valid fields
                                                    // int64
                       hls::stream<ap_uint<64> >& i_int_strm,
                       // string
                       hls::stream<ap_uint<64 + 4 + 1> >& i_str_strm,
                       // Boolean
                       hls::stream<ap_uint<1> >& i_bool_strm,
                       // Date
                       hls::stream<ap_uint<64> >& i_date_strm,
                       // Numeric
                       hls::stream<ap_uint<64> >& i_numeric_strm,
                       // EOF
                       hls::stream<bool>& i_ln_e_strm,

                       hls::stream<ObjectAlter1>& o_obj_strm) {
    ap_uint<8> col_idx = 0;
    bool e = i_ln_e_strm.read();
loop_mergeField:
    while (!e) {
#pragma HLS pipeline II = 1
        if (!(col_idx == 0 && i_ln_e_strm.empty())) {
            ap_uint<64> d;
            ap_uint<4> byte_vld;
            bool e_vld;
            ObjectAlter1 obj;
            obj.set_id(col_idx);
            bool suc_flag = false;

            ap_uint<4> data_type = type_valid_buff[col_idx];
            // std::cout << "e=" << e << ", col_idx=" << col_idx << ", data_type=" << data_type << std::endl;
            switch (data_type) {
                case (TInt64): {
                    e_vld = true;
                    suc_flag = i_int_strm.read_nb(d);
#ifndef __SYNTHESIS__
// std::cout << "int64:" << std::hex << d << std::dec << std::endl;
#endif
                    if (suc_flag) {
                        obj.set_data(d);
                        obj.set_valid(8);
                        obj.set_type(TInt64);
                    }
                    break;
                }
                case (TFloat32): {
                    break;
                }
                case (TDouble): {
                    break;
                }
                case (TString): {
                    ap_uint<64 + 4 + 1> str_d;
                    suc_flag = i_str_strm.read_nb(str_d);
                    if (suc_flag) {
                        e_vld = str_d[68];
                        obj.set_data(str_d(63, 0));
                        obj.set_valid(str_d(67, 64));
                        obj.set_type(TString); // mask it when e_vld = 0
                    }
                    break;
                }
                case (TBoolean): {
                    ap_uint<1> db;
                    suc_flag = i_bool_strm.read_nb(db);
                    if (suc_flag) {
                        e_vld = true;
                        obj.set_data(db);
                        obj.set_valid(1);
                        obj.set_type(TBoolean);
                    }
                    break;
                }
                case (TDate): {
                    suc_flag = i_date_strm.read_nb(d);
                    if (suc_flag) {
                        e_vld = true;
                        obj.set_data(d);
                        obj.set_valid(8);
                        obj.set_type(TDate);
                    }
                    break;
                }
                case (TNumeric): {
                    suc_flag = i_numeric_strm.read_nb(d);
                    if (suc_flag) {
                        e_vld = true;
                        obj.set_data(d);
                        obj.set_valid(8);
                        obj.set_type(TNumeric); // mask it to distinguish the neighbour string field when e_vld = 0
                    }
                    break;
                }
                case (FEOL): {
                    suc_flag = i_ln_e_strm.read_nb(e);
                    if (suc_flag) {
                        e_vld = false;
                        obj.set_type(FEOL);
                        col_idx = 0;
                    }
                    break;
                }
            }

            // emit the object
            if (suc_flag) {
                o_obj_strm << obj;

                // update the column index
                if (e_vld) { // only variable-length TString/TNumeric will pull down the `e_vld`
                    col_idx++;
                }
            }
        }
    }

    // end of part of file
    ObjectAlter1 obj;
    obj.set_type(FEOF);
    o_obj_strm << obj;
}

template <int N, int DTN>
void mergeLineEndChan(hls::stream<bool> i_strm[N],
                      hls::stream<bool>& o_strm,
                      hls::stream<ap_uint<3> > ch_ctrl_strm[DTN]) {
    ap_uint<N> end = 0;
    ap_uint<N> ch = 0;
    bool e;
loop_mergeLineEndChan:
    while (end != ap_uint<N>(-1)) {
#pragma HLS pipeline ii = 1
        if (!end[ch]) {
            bool nb_1 = i_strm[ch].read_nb(e);
            if (nb_1) {
                end[ch] = e;
                if (!e) {
                    o_strm.write(e);
                    for (int i = 0; i < DTN; i++) {
                        ch_ctrl_strm[i].write(ch);
                    }
                }
            }
            ch = (ch + 1) % N;
        } else {
            ch = (ch + 1) % N;
        }
    }
    o_strm.write(e);
    for (int i = 0; i < DTN; i++) {
        ch_ctrl_strm[i].write(-1);
    }
}

template <int N>
void mergeBoolChan(ap_uint<N> num,
                   hls::stream<ap_uint<3> >& ch_ctrl_strm,
                   hls::stream<bool> i_strm[N],
                   hls::stream<ap_uint<2> >& o_strm) {
    ap_uint<3> ch = ch_ctrl_strm.read();
    ap_uint<N> cnt = 0;
    ap_uint<2> out;
    bool in;
    bool nb_ch = true;
    while (ch != ap_uint<3>(-1)) {
#pragma HLS pipeline ii = 1
        if (num > 0 && nb_ch) {
            bool nb = i_strm[ch].read_nb(in);
            if (nb) {
                out[0] = in;
                out[1] = 0;
                o_strm.write(out);
                if (cnt + 1 == num) {
                    cnt = 0;
                    nb_ch = ch_ctrl_strm.read_nb(ch);
                } else {
                    cnt += 1;
                }
            }
        } else {
            nb_ch = ch_ctrl_strm.read_nb(ch);
        }
    }
    out[1] = 1;
    o_strm.write(out);
}

template <int N, int DT = 0>
void mergeChan(const ap_uint<N> num,
               hls::stream<ap_uint<3> >& ch_ctrl_strm,
               hls::stream<ap_uint<9> > i_strm[N],
               // 0~7: data, 8: v, 9: e
               hls::stream<ap_uint<10> >& o_strm) {
    ap_uint<N> cnt = 0;
    ap_uint<3> ch = ch_ctrl_strm.read();
    ap_uint<9> in;
    ap_uint<10> out;
    bool nb_ch = true;
loop_mergeChan:
    while (ch != ap_uint<3>(-1)) {
#pragma HLS pipeline ii = 1
        if (num > 0 && nb_ch) {
            bool nb = i_strm[ch(1, 0)].read_nb(in);
            // std::cout << "nb=" << nb << std::endl;
            if (nb) {
                out(8, 0) = in;
                out[9] = 0;
                o_strm.write(out);
                // std::cout << "in[" << cnt << "][" << ch << "]=" << in[8] << ", " << (char)in(7, 0) << std::endl;
                if (!in[8]) {
                    if (cnt + 1 == num) {
                        cnt = 0;
                        nb_ch = ch_ctrl_strm.read_nb(ch);
                    } else {
                        cnt += 1;
                    }
                }
            }
        } else {
            nb_ch = ch_ctrl_strm.read_nb(ch);
        }
    }
    out[9] = 1;
    o_strm.write(out);
}
/**
 *
 * @brief parse the input string and store them in buffer.
 *
 * @param i_strm input stream of string.
 * @param i_e_strm input end flag of i_strm.
 * @param type_buff input data type configuration.
 *
 */
template <int N, int CN = 16>
void parseBlock(ap_uint<8> type_buff[N][CN],
                ap_uint<3>* type_num,
                ap_uint<4>* type_valid_buff,
                hls::stream<ap_uint<9> > i_strm[N],
                hls::stream<ObjectAlter1>& o_object_strm) {
#pragma HLS dataflow

    hls::stream<ap_uint<11> > iter_data_strm[N];
#pragma HLS stream variable = iter_data_strm depth = 8
    // split the fields, reserve the delimiter
    for (int i = 0; i < N; i++) {
#pragma HLS unroll
        iterativeCSVParse(i_strm[i], iter_data_strm[i]);
    }
#ifndef __SYNTHESIS__
// std::cout << "iterativeParse ln_e_strm.size(): " << iter_ln_e_strm[0].size() << ", N=" << N << std::endl;
#endif

    hls::stream<ap_uint<9> > filter_int_strm[N];
#pragma HLS stream variable = filter_int_strm depth = 8
    hls::stream<ap_uint<9> > filter_str_strm[N];
#pragma HLS stream variable = filter_str_strm depth = 64
    hls::stream<bool> filter_bool_strm[N];
#pragma HLS stream variable = filter_bool_strm depth = 8
    hls::stream<ap_uint<9> > filter_date_strm[N];
#pragma HLS stream variable = filter_date_strm depth = 32
    hls::stream<ap_uint<9> > filter_numeric_strm[N];
#pragma HLS stream variable = filter_numeric_strm depth = 64
    hls::stream<bool> filter_o_ln_e_strm[N];
#pragma HLS stream variable = filter_o_ln_e_strm depth = 64

    // filter out the selected columns
    for (int i = 0; i < N; i++) {
#pragma HLS unroll
        fieldFilter(type_buff[i], iter_data_strm[i], filter_int_strm[i], filter_str_strm[i], filter_bool_strm[i],
                    filter_date_strm[i], filter_numeric_strm[i], filter_o_ln_e_strm[i]);
    }

    hls::stream<ap_uint<10> > merge_int_strm;
#pragma HLS stream variable = merge_int_strm depth = 8
    hls::stream<ap_uint<10> > merge_str_strm;
#pragma HLS stream variable = merge_str_strm depth = 64
    hls::stream<ap_uint<2> > merge_bool_strm;
#pragma HLS stream variable = merge_bool_strm depth = 8
    hls::stream<ap_uint<10> > merge_date_strm;
#pragma HLS stream variable = merge_date_strm depth = 32
    hls::stream<ap_uint<10> > merge_numeric_strm;
#pragma HLS stream variable = merge_numeric_strm depth = 64
    hls::stream<bool> merge_o_ln_e_strm;
#pragma HLS stream variable = merge_o_ln_e_strm depth = 64

    const int DTN = 5;                          // data type number
    hls::stream<ap_uint<3> > ch_ctrl_strm[DTN]; // for sync up to solve hang
#pragma HLS stream variable = ch_ctrl_strm depth = 64
    // merge line end for line order
    mergeLineEndChan<N, 5>(filter_o_ln_e_strm, merge_o_ln_e_strm, ch_ctrl_strm);
    // merge stream  based on ch_ctrl_strm for line order
    mergeChan<N, 1>(type_num[TDate], ch_ctrl_strm[0], filter_date_strm, merge_date_strm);
    mergeBoolChan<N>(type_num[TBoolean], ch_ctrl_strm[1], filter_bool_strm, merge_bool_strm);
    mergeChan<N>(type_num[TString], ch_ctrl_strm[2], filter_str_strm, merge_str_strm);
    mergeChan<N>(type_num[TInt64], ch_ctrl_strm[3], filter_int_strm, merge_int_strm);
    mergeChan<N>(type_num[TNumeric], ch_ctrl_strm[4], filter_numeric_strm, merge_numeric_strm);

    hls::stream<ap_uint<64> > int64_strm;
#pragma HLS stream variable = int64_strm depth = 32

    hls::stream<ap_uint<64 + 4 + 1> > string64_strm;
#pragma HLS stream variable = string64_strm depth = 32
    hls::stream<ap_uint<4> > string_o_byte_valid_strm;
#pragma HLS stream variable = string_o_byte_valid_strm depth = 32
    hls::stream<bool> string_o_vld_strm;
#pragma HLS stream variable = string_o_vld_strm depth = 32

    hls::stream<ap_uint<1> > bool_strm;
#pragma HLS stream variable = bool_strm depth = 32

    hls::stream<ap_uint<64> > date64_strm;
#pragma HLS stream variable = date64_strm depth = 32

    hls::stream<ap_uint<64> > numeric64_strm;
#pragma HLS stream variable = numeric64_strm depth = 32

    parseDate(merge_date_strm, date64_strm);
    parseBoolean(merge_bool_strm, bool_strm);
    // combine char to form 64-bit data
    parseString(merge_str_strm, string64_strm);
    parseInt64(merge_int_strm, int64_strm);
    parseNumeric(merge_numeric_strm, numeric64_strm);
#ifndef __SYNTHESIS__
    std::cout << "date size=" << date64_strm.size() << ", bool size=" << bool_strm.size()
              << ", str size=" << string64_strm.size() << ", int64 size=" << int64_strm.size()
              << ", numeric size=" << numeric64_strm.size() << ", line end size=" << merge_o_ln_e_strm.size()
              << std::endl;
#endif

    // collect each valid field to form one line
    mergeField(type_valid_buff, int64_strm, string64_strm, bool_strm, date64_strm, numeric64_strm, merge_o_ln_e_strm,
               o_object_strm);
}
} // namespace internal
} // namespace dataframe
} // namespace data_analytics
} // namespace xf
#endif
