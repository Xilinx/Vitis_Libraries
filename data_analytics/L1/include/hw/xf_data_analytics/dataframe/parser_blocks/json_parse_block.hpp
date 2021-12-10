/*
 * Copyright 2021 Xilinx, Inc.
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
#ifndef XF_DATA_ANALYTICS_L1_DATAFRAME_JSON_PARSE_BLOCK_HPP
#define XF_DATA_ANALYTICS_L1_DATAFRAME_JSON_PARSE_BLOCK_HPP
#include "ap_int.h"
#include "hls_stream.h"
#include <stdint.h>
#include "strtod.hpp"
#include "json_line_parser.hpp"
#include "parse_key.hpp"
#include "parse_value.hpp"
#include "parse_double.hpp"
#include "parse_date.hpp"
#include "utils.hpp"

namespace xf {
namespace data_analytics {
namespace dataframe {
namespace internal {
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
void parseString(hls::stream<ap_uint<8> >& i_strm,
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

/*
 * @brief merge each valid field into a new row by object stream
 *
 **/
static void mergeField(ap_uint<9> num_of_column,
                       // info for each column
                       hls::stream<ap_uint<8> >& i_hash_strm,
                       hls::stream<ap_uint<4> >& i_dt_strm,
                       hls::stream<bool>& i_null_strm,
                       hls::stream<bool>& i_e_strm,
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
                       hls::stream<bool>& i_bool_strm,
                       // Date
                       hls::stream<ap_uint<64> >& i_date_strm,
                       // output
                       hls::stream<bool>& o_ln_e_strm,
                       hls::stream<Object>& o_obj_strm) {
    ap_uint<8> col_cnt = 0;
    ap_uint<4> data_type = i_dt_strm.read();
    ap_uint<8> col_idx = i_hash_strm.read();
    bool is_null = i_null_strm.read();
    bool e = i_e_strm.read();
    while (!e) {
#pragma HLS pipeline II = 2
        ap_uint<64> d;
        ap_uint<4> byte_vld;
        bool e_vld;
        Object obj;

        obj.set_id(col_idx);
        obj.set_type(data_type);
        if (!is_null) {
            switch (data_type) {
                case (TInt64): {
                    e_vld = true;
                    i_int_strm >> d;
                    obj.set_data(d);
                    obj.set_valid(8);
                    break;
                }
                case (TFloat32): {
                    e_vld = true;
                    i_double_strm.read(); // consume the dummy
                    d(31, 0) = i_float_strm.read();
                    obj.set_data(d);
                    obj.set_valid(4);
                    break;
                }
                case (TDouble): {
                    e_vld = true;
                    i_float_strm.read(); // consume the dummy
                    i_double_strm >> d;
                    obj.set_data(d);
                    obj.set_valid(8);
                    break;
                }
                case (TString): {
                    i_str_strm >> d;
                    i_str_vld_strm >> byte_vld;
                    i_str_e_strm >> e_vld;
                    obj.set_data(d);
                    obj.set_valid(byte_vld);
                    break;
                }
                case (TBoolean): {
                    e_vld = true;
                    bool b = i_bool_strm.read();
                    d(7, 0) = b ? 1 : 0;
                    obj.set_data(d);
                    obj.set_valid(1);
                    break;
                }
                case (TDate): {
                    e_vld = true;
                    i_date_strm >> d;
                    obj.set_data(d);
                    obj.set_valid(8);
                    break;
                }
            }
        } else {
            e_vld = true;
            obj.set_data(0);
            obj.set_valid(0); // Null
        }

        // emit the object
        o_obj_strm << obj;

        // update the column index
        if (e_vld) {                            // only variable-length TString/TNumeric will pull down the `e_vld`
            if (col_cnt == num_of_column - 1) { // move to the next line
                Object t;
                t.set_type(FEOL);
                o_obj_strm << t;      // end of line
                o_ln_e_strm << false; // flag of EOL
                col_cnt = 0;
            } else
                col_cnt++;

            i_dt_strm >> data_type;
            i_hash_strm >> col_idx;
            i_null_strm >> is_null;
            i_e_strm >> e;
        }
    }

    // end of part of file
    Object obj;
    obj.set_type(FEOF);
    o_obj_strm << obj;
    o_ln_e_strm << false; // flag of EOF
}

/**
 *
 * @brief parse the input string and store them in buffer.
 *
 * @param mask_cfg input valid flag of i_strm.
 * @param key_buf input key configuration.
 * @param type_buf input data type configuration.
 * @param i_strm input stream of string.
 * @param i_e_strm input end flag of i_strm.
 * @param o_ln_e_strm ouput line end flag
 *
 */

template <int COL_NUM>
void parseBlock(ap_uint<9> num_of_column,
                ap_uint<COL_NUM> mask_cfg,
                ap_uint<8> key_buf[COL_NUM][256],
                ap_uint<4> type_buf[COL_NUM],
                hls::stream<ap_uint<8> >& i_strm,
                hls::stream<bool>& i_e_strm,
                hls::stream<bool>& o_ln_e_strm,
                hls::stream<Object>& o_obj_strm) {
#pragma HLS dataflow
#ifndef __SYNTHESIS__
    static int PU_ID = 0;
#endif
    hls::stream<ap_uint<8> > k_strm("k_strm");
#pragma HLS stream variable = k_strm depth = 8
    hls::stream<bool> e_k_strm("e_k_strm");
#pragma HLS stream variable = e_k_strm depth = 8
    hls::stream<bool> k_vld_strm("k_vld_strm");
#pragma HLS stream variable = k_vld_strm depth = 8

    hls::stream<ap_uint<8> > val_strm("val_strm");
#pragma HLS stream variable = val_strm depth = 8
    hls::stream<bool> e_val_strm("e_val_strm");
#pragma HLS stream variable = e_val_strm depth = 8
    hls::stream<bool> val_vld_strm("val_vld_strm");
#pragma HLS stream variable = val_vld_strm depth = 8

    hls::stream<bool> ln_e_strm("ln_e_strm");
#pragma HLS stream variable = ln_e_strm depth = 8

    // split the key and value, remove the delimiter
    iterativeParse(i_strm, i_e_strm, k_strm, e_k_strm, k_vld_strm, val_strm, e_val_strm, val_vld_strm, ln_e_strm);

    hls::stream<ap_uint<9> > hash_strm("hash_strm");
#pragma HLS stream variable = hash_strm depth = 8
    hls::stream<bool> le_strm_0("le_strm_0");
#pragma HLS stream variable = le_strm_0 depth = 8
    hls::stream<bool> e_strm_0_0("e_strm_0_0");
#pragma HLS stream variable = e_strm_0_0 depth = 8
    hls::stream<bool> e_strm_0_1("e_strm_0_1");
#pragma HLS stream variable = e_strm_0_1 depth = 8
    hls::stream<ap_uint<COL_NUM> > mk_strm("mk_strm");
#pragma HLS stream variable = mk_strm depth = 8
    // compare the key and find the index
    parseKey<COL_NUM>(k_strm, k_vld_strm, e_k_strm, ln_e_strm, mask_cfg, hash_strm, le_strm_0, e_strm_0_1, mk_strm,
                      e_strm_0_0, key_buf);

    hls::stream<ap_uint<9> > hash_strm_0_1("hash_strm_0_1");
#pragma HLS stream variable = hash_strm_0_1 depth = 8
    hls::stream<bool> le_strm_1("le_strm_1");
#pragma HLS stream variable = le_strm_1 depth = 8
    addNull<COL_NUM>(mk_strm, e_strm_0_0, hash_strm_0_1, le_strm_1);

    hls::stream<ap_uint<8> > int_strm("int_strm");
#pragma HLS stream variable = int_strm depth = 8

    hls::stream<bool> e_i_strm("e_i_strm");
#pragma HLS stream variable = e_i_strm depth = 8
    hls::stream<bool> v_i_strm("v_i_strm");
#pragma HLS stream variable = v_i_strm depth = 8

    hls::stream<ap_uint<8> > db_strm("db_strm");
#pragma HLS stream variable = db_strm depth = 8
    hls::stream<bool> e_d_strm("e_d_strm");
#pragma HLS stream variable = e_d_strm depth = 8
    hls::stream<bool> v_d_strm("v_d_strm");
#pragma HLS stream variable = v_d_strm depth = 8

    hls::stream<ap_uint<8> > str_strm("str_strm");
#pragma HLS stream variable = str_strm depth = 8
    hls::stream<bool> e_s_strm("e_s_strm");
#pragma HLS stream variable = e_s_strm depth = 8
    hls::stream<bool> v_s_strm("v_s_strm");
#pragma HLS stream variable = v_s_strm depth = 8

    hls::stream<bool> bool_strm("bool_strm");
#pragma HLS stream variable = bool_strm depth = 8

    hls::stream<ap_uint<8> > val_strm_0("val_strm_0");
#pragma HLS stream variable = val_strm_0 depth = 8
    hls::stream<bool> val_vld_strm_0("val_vld_strm_0");
#pragma HLS stream variable = val_vld_strm_0 depth = 8
    hls::stream<bool> e_val_strm_0("e_val_strm_0");
#pragma HLS stream variable = e_val_strm_0 depth = 8

    hls::stream<ap_uint<8> > hash_strm_0("hash_strm_0");
#pragma HLS stream variable = hash_strm_0 depth = 8
    hls::stream<ap_uint<4> > dt_strm_0("dt_strm_0");
#pragma HLS stream variable = dt_strm_0 depth = 8

    hls::stream<ap_uint<9> > date_strm("date_strm");
#pragma HLS stream variable = date_strm depth = 8
    hls::stream<bool> e_dt_strm("e_dt_strm");
#pragma HLS stream variable = e_dt_strm depth = 8

    hls::stream<ap_uint<8> > hash_strm_1("hash_strm_1");
#pragma HLS stream variable = hash_strm_1 depth = 64
    hls::stream<ap_uint<4> > dt_strm_1("dt_strm_1");
#pragma HLS stream variable = dt_strm_1 depth = 64
    hls::stream<bool> null_strm("null_strm");
#pragma HLS stream variable = null_strm depth = 64
    hls::stream<bool> e_strm_1("e_strm_1");
#pragma HLS stream variable = e_strm_1 depth = 64

    // merge the columns with null fields
    duplicate<COL_NUM>(val_strm, val_vld_strm, e_val_strm, hash_strm, le_strm_0, e_strm_0_1, hash_strm_0_1, le_strm_1,
                       type_buf, val_strm_0, val_vld_strm_0, e_val_strm_0, hash_strm_0, dt_strm_0);

    // dispatch based on data type
    parseValue<COL_NUM>(val_strm_0, val_vld_strm_0, e_val_strm_0, hash_strm_0, dt_strm_0, int_strm, e_i_strm, v_i_strm,
                        db_strm, e_d_strm, v_d_strm, str_strm, e_s_strm, v_s_strm, bool_strm, date_strm, e_dt_strm,
                        hash_strm_1, dt_strm_1, null_strm, e_strm_1);

    hls::stream<ap_uint<64> > int64_strm("int64_strm");
#pragma HLS stream variable = int64_strm depth = 8
    parseInt64(int_strm, e_i_strm, v_i_strm, int64_strm);

    hls::stream<ap_uint<64> > db64_strm("db64_strm");
#pragma HLS stream variable = db64_strm depth = 8
    hls::stream<ap_uint<32> > ft32_strm("ft32_strm");
#pragma HLS stream variable = ft32_strm depth = 8
    parseDouble(db_strm, e_d_strm, v_d_strm, db64_strm, ft32_strm);

    hls::stream<ap_uint<64> > s_w64_strm("s_w64_strm");
#pragma HLS stream variable = s_w64_strm depth = 8
    hls::stream<ap_uint<4> > s_byte_valid_strm("s_byte_valid_strm");
#pragma HLS stream variable = s_byte_valid_strm depth = 8
    hls::stream<bool> s_vld_strm("s_vld_strm");
#pragma HLS stream variable = s_vld_strm depth = 8
    // combine char to form 64-bit data
    parseString(str_strm, e_s_strm, v_s_strm, s_w64_strm, s_byte_valid_strm, s_vld_strm);

    hls::stream<ap_uint<64> > date64_strm("date64_strm");
#pragma HLS stream variable = date64_strm depth = 8
    parseDate(date_strm, e_dt_strm, date64_strm);

    // re-order and packet each into Object stream
    mergeField(num_of_column, hash_strm_1, dt_strm_1, null_strm, e_strm_1, int64_strm, db64_strm, ft32_strm, s_w64_strm,
               s_byte_valid_strm, s_vld_strm, bool_strm, date64_strm, o_ln_e_strm, o_obj_strm);
#ifndef __SYNTHESIS__
    PU_ID++;
#endif
}
}
}
}
}
#endif
