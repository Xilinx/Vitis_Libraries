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
 * @brief parse the input string stream, generate the number of valid byte information as well.
 *
 * @param i_strm input stream of characters for each value.
 * @param i_e_strm input end flag of i_strm.
 * @param vld_strm input valid flag for each character.
 * @param o_strm output 64-bit width string block stream.
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
#ifndef __SYNTHESIS__
#ifdef __DEBUG_JSON_STRING__
    std::cout << "in = " << (char)in << std::endl;
    std::cout << "vld = " << vld << std::endl;
    std::cout << "e = " << e << std::endl;
#endif
#endif
    bool nb_1 = true;
    bool nb_2 = true;
    bool nb_3 = true;

    while (!e) {
#pragma HLS pipeline II = 1
        if (((len == 8) && nb_1 && nb_2 && nb_3) || (!vld && nb_1 && nb_2 && nb_3)) {
            o_vld_strm.write(!vld); // vld: 0-last byte, 1-continue
            if (len < sizeof(ap_uint<64>)) {
                str = str >> (8 - len) * 8;
            }
            o_strm << str;
            o_byte_valid_strm << len;
#ifndef __SYNTHESIS__
#ifdef __DEBUG_JSON_STRING__
            std::cout << "str = ";
            for (int i = 0; i < 8; i++) {
                std::cout << (char)str.range((7 - i) * 8 + 7, (7 - i) * 8);
            }
            std::cout << std::endl;
            std::cout << "byte_vld = " << len << std::endl;
            std::cout << "o_vld = " << !vld << std::endl;
#endif
#endif
        }

        if (vld && nb_1 && nb_2 && nb_3) {
            // drop the '\"'
            if (in != '\"') {
                str = str >> 8;
                str(63, 56) = in;

                if (len == sizeof(ap_uint<64>))
                    len = 1;
                else
                    len++;
            }
        } else if (!vld)
            len = 0;

        {
#pragma HLS latency min = 0 max = 0
            nb_1 = i_strm.read_nb(in);
            nb_2 = i_e_strm.read_nb(e);
            nb_3 = i_vld_strm.read_nb(vld);
        }
#ifndef __SYNTHESIS__
#ifdef __DEBUG_JSON_STRING__
        std::cout << "in = " << (char)in << std::endl;
        std::cout << "vld = " << vld << std::endl;
        std::cout << "e = " << e << std::endl;
#endif
#endif
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
        {
#pragma HLS latency min = 0 max = 0
            nb_1 = vld_strm.read_nb(vld);
            nb_2 = i_e_strm.read_nb(e);
            nb_3 = i_strm.read_nb(in);
        }
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
 * @brief merge each valid field into a new row to object stream
 *
 **/
template <int ARRAY_BW, int FIELD_BW, int TYPE_BW, int VALID_BW, int DATA_BW>
void mergeField(ap_uint<FIELD_BW> num_of_column,
                // info for each column
                hls::stream<ap_uint<FIELD_BW> >& i_hash_strm,
                hls::stream<ap_uint<TYPE_BW> >& i_dt_strm,
                hls::stream<ap_uint<ARRAY_BW> >& i_offt_strm,
                hls::stream<bool>& i_null_strm,
                hls::stream<bool>& i_e_strm,
                // int64
                hls::stream<ap_uint<DATA_BW> >& i_int_strm,
                // double
                hls::stream<ap_uint<DATA_BW> >& i_double_strm,
                // float
                hls::stream<ap_uint<32> >& i_float_strm,
                // string
                hls::stream<ap_uint<DATA_BW> >& i_str_strm,
                hls::stream<ap_uint<VALID_BW> >& i_str_vld_strm,
                hls::stream<bool>& i_str_e_strm,
                // Boolean
                hls::stream<bool>& i_bool_strm,
                // Date
                hls::stream<ap_uint<DATA_BW> >& i_date_strm,
                // output
                hls::stream<bool>& o_ln_e_strm,
                hls::stream<ObjectEx>& o_obj_strm) {
    ap_uint<FIELD_BW> col_cnt = 0;
    ap_uint<TYPE_BW> data_type = i_dt_strm.read();
    ap_uint<FIELD_BW> col_idx = i_hash_strm.read();
    ap_uint<ARRAY_BW> arr_idx = i_offt_strm.read();
    ObjectEx obj;
    obj.set_all(0);
    bool is_null = i_null_strm.read();
    bool e = i_e_strm.read();
    while (!e) {
#pragma HLS pipeline II = 2
        ap_uint<DATA_BW> d = 0;
        ap_uint<VALID_BW> byte_vld = 0;
        bool e_vld = true;
        ap_uint<DATA_BW> drop;

        {
#pragma HLS latency min = 0 max = 0
            // default as null
            obj.set_id(col_idx);
            obj.set_offset(arr_idx);
            obj.set_type(data_type);
            obj.set_valid(0);
            if (!is_null) {
                switch (data_type) {
                    case (TInt64): {
                        i_int_strm >> d;
                        byte_vld = sizeof(int64_t);
                        break;
                    }
                    case (TFloat32): {
                        drop = i_double_strm.read(); // consume the dummy
                        d(31, 0) = i_float_strm.read();
                        byte_vld = sizeof(float);
                        break;
                    }
                    case (TDouble): {
                        drop.range(31, 0) = i_float_strm.read(); // consume the dummy
                        d = i_double_strm.read();
                        byte_vld = sizeof(double);
                        break;
                    }
                    case (TString): {
                        d = i_str_strm.read();
                        byte_vld = i_str_vld_strm.read();
                        e_vld = i_str_e_strm.read();
                        break;
                    }
                    case (TBoolean): {
                        bool b = i_bool_strm.read();
                        d(7, 0) = b ? 1 : 0;
                        byte_vld = sizeof(int8_t);
                        break;
                    }
                    case (TDate): {
                        d = i_date_strm.read();
                        byte_vld = sizeof(int64_t);
                        break;
                    }
                }
            }
            obj.set_valid(byte_vld);
            obj.set_data(d);
            // emit the object
            o_obj_strm << obj;
        }

        // update the column index
        if (e_vld) { // only variable-length TString pulls down the `e_vld`
            if (col_cnt == num_of_column - 1 && arr_idx == ap_uint<ARRAY_BW>(-1)) { // move to the next line
                ObjectEx t;
                t.set_all(0);
                t.set_type(FEOL);
                o_obj_strm << t;      // end of line
                o_ln_e_strm << false; // flag of EOL
                col_cnt = 0;
            } else if (arr_idx == ap_uint<ARRAY_BW>(-1)) {
                col_cnt++;
            }

            i_dt_strm >> data_type;
            i_hash_strm >> col_idx;
            i_offt_strm >> arr_idx;
            i_null_strm >> is_null;
            i_e_strm >> e;
        }
    }

    // end of part of file
    obj.set_all(0);
    obj.set_type(FEOF);
    o_obj_strm << obj;
    o_ln_e_strm << false; // flag of EOF
}

/**
 *
 * @brief parse the input string and store them to object stream.
 *
 * @param num_of_column number of columns specified in schema.
 * @param mask_cfg valid column specified in schema.
 * @param key_buf input key configuration in schema.
 * @param type_buf input data type configuration in schema.
 * @param i_strm input stream of string.
 * @param i_e_strm input end flag of i_strm.
 * @param o_ln_e_strm ouput line end flag.
 * @param o_obj_strm output parsed object stream.
 *
 */

template <int COL_NUM, int ARRAY_BW, int FIELD_BW, int TYPE_BW, int VALID_BW, int DATA_BW>
void parseBlock(ap_uint<FIELD_BW> num_of_column,
                ap_uint<COL_NUM> mask_cfg,
                ap_uint<8> key_buf[COL_NUM][256],
                ap_uint<4> type_buf[COL_NUM],
                hls::stream<ap_uint<8> >& i_strm,
                hls::stream<bool>& i_e_strm,
                hls::stream<bool>& o_ln_e_strm,
                hls::stream<ObjectEx>& o_obj_strm) {
#pragma HLS dataflow

    hls::stream<ap_uint<8> > k_strm("k_strm");
#pragma HLS stream variable = k_strm depth = 8
    hls::stream<bool> e_k_strm("e_k_strm");
#pragma HLS stream variable = e_k_strm depth = 8
    hls::stream<ap_uint<8> > k_idx_strm("k_idx_strm");
#pragma HLS stream variable = k_idx_strm depth = 8
    hls::stream<bool> k_vld_strm("k_vld_strm");
#pragma HLS stream variable = k_vld_strm depth = 8
    hls::stream<bool> ln_e_strm("ln_e_strm");
#pragma HLS stream variable = ln_e_strm depth = 8

    hls::stream<ap_uint<8> > val_strm("val_strm");
#pragma HLS stream variable = val_strm depth = 8
    hls::stream<bool> e_val_strm("e_val_strm");
#pragma HLS stream variable = e_val_strm depth = 8
    hls::stream<ap_uint<ARRAY_BW> > val_idx_strm("val_idx_strm");
#pragma HLS stream variable = val_idx_strm depth = 8
    hls::stream<bool> val_vld_strm("val_vld_strm");
#pragma HLS stream variable = val_vld_strm depth = 8

    // split the key and value, remove the delimiter
    iterativeParse<ARRAY_BW>(i_strm, i_e_strm, k_strm, e_k_strm, k_idx_strm, k_vld_strm, val_strm, e_val_strm,
                             val_idx_strm, val_vld_strm, ln_e_strm);

    hls::stream<ap_uint<COL_NUM + 1> > hash_strm("hash_strm");
#pragma HLS stream variable = hash_strm depth = 8
    hls::stream<bool> le_strm_0("le_strm_0");
#pragma HLS stream variable = le_strm_0 depth = 8
    hls::stream<bool> e_strm_0_0("e_strm_0_0");
#pragma HLS stream variable = e_strm_0_0 depth = 8
    hls::stream<bool> e_strm_0_1("e_strm_0_1");
#pragma HLS stream variable = e_strm_0_1 depth = 8
    hls::stream<ap_uint<COL_NUM> > mk_strm("mk_strm");
#pragma HLS stream variable = mk_strm depth = 8
    // compare the key and find the index, generate key for paring each element in array
    parseKey<COL_NUM>(k_strm, k_idx_strm, k_vld_strm, e_k_strm, ln_e_strm, mask_cfg, hash_strm, le_strm_0, e_strm_0_1,
                      mk_strm, e_strm_0_0, key_buf);

    hls::stream<ap_uint<COL_NUM + 1> > hash_strm_0_1("hash_strm_0_1");
#pragma HLS stream variable = hash_strm_0_1 depth = 8
    hls::stream<bool> le_strm_1("le_strm_1");
#pragma HLS stream variable = le_strm_1 depth = 8
    // add null index for each JSON line
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
    hls::stream<ap_uint<ARRAY_BW> > val_idx_strm_0("val_idx_strm_0");
#pragma HLS stream variable = val_idx_strm_0 depth = 8
    hls::stream<bool> val_vld_strm_0("val_vld_strm_0");
#pragma HLS stream variable = val_vld_strm_0 depth = 8
    hls::stream<bool> e_val_strm_0("e_val_strm_0");
#pragma HLS stream variable = e_val_strm_0 depth = 8
    hls::stream<ap_uint<FIELD_BW> > hash_strm_0("hash_strm_0");
#pragma HLS stream variable = hash_strm_0 depth = 8
    hls::stream<ap_uint<TYPE_BW> > dt_strm_0("dt_strm_0");
#pragma HLS stream variable = dt_strm_0 depth = 8

    hls::stream<ap_uint<9> > date_strm("date_strm");
#pragma HLS stream variable = date_strm depth = 8
    hls::stream<bool> e_dt_strm("e_dt_strm");
#pragma HLS stream variable = e_dt_strm depth = 8

    hls::stream<ap_uint<FIELD_BW> > hash_strm_1("hash_strm_1");
#pragma HLS stream variable = hash_strm_1 depth = 64
#pragma HLS bind_storage variable = hash_strm_1 type = FIFO impl = LUTRAM
    hls::stream<ap_uint<TYPE_BW> > dt_strm_1("dt_strm_1");
#pragma HLS stream variable = dt_strm_1 depth = 64
#pragma HLS bind_storage variable = dt_strm_1 type = FIFO impl = LUTRAM
    hls::stream<ap_uint<ARRAY_BW> > offt_strm("offt_strm");
#pragma HLS stream variable = offt_strm depth = 64
#pragma HLS bind_storage variable = offt_strm type = FIFO impl = LUTRAM
    hls::stream<bool> null_strm("null_strm");
#pragma HLS stream variable = null_strm depth = 64
#pragma HLS bind_storage variable = null_strm type = FIFO impl = SRL
    hls::stream<bool> e_strm_1("e_strm_1");
#pragma HLS stream variable = e_strm_1 depth = 64
#pragma HLS bind_storage variable = e_strm_1 type = FIFO impl = SRL

    // compose the value with the key index from parseKey (those existed in the current JSON line)
    // and the index generated by addNull (those specified in schema but not found in the current line)
    combineValidAndNull<COL_NUM, ARRAY_BW, FIELD_BW, TYPE_BW>(
        val_strm, val_idx_strm, val_vld_strm, e_val_strm, hash_strm, le_strm_0, e_strm_0_1, hash_strm_0_1, le_strm_1,
        type_buf, val_strm_0, val_idx_strm_0, val_vld_strm_0, e_val_strm_0, hash_strm_0, dt_strm_0);

    // dispatch value (in byte stream form) based on data type, and parse bool
    parseValue<ARRAY_BW, FIELD_BW, TYPE_BW>(val_strm_0, val_idx_strm_0, val_vld_strm_0, e_val_strm_0, hash_strm_0,
                                            dt_strm_0, int_strm, e_i_strm, v_i_strm, db_strm, e_d_strm, v_d_strm,
                                            str_strm, e_s_strm, v_s_strm, bool_strm, date_strm, e_dt_strm, hash_strm_1,
                                            dt_strm_1, offt_strm, null_strm, e_strm_1);

    hls::stream<ap_uint<DATA_BW> > int64_strm("int64_strm");
#pragma HLS stream variable = int64_strm depth = 64
#pragma HLS bind_storage variable = int64_strm type = FIFO impl = LUTRAM
    // parse byte stream to int64_t
    parseInt64(int_strm, e_i_strm, v_i_strm, int64_strm);

    hls::stream<ap_uint<DATA_BW> > db64_strm("db64_strm");
#pragma HLS stream variable = db64_strm depth = 64
#pragma HLS bind_storage variable = db64_strm type = FIFO impl = LUTRAM
    hls::stream<ap_uint<32> > ft32_strm("ft32_strm");
#pragma HLS stream variable = ft32_strm depth = 64
#pragma HLS bind_storage variable = ft32_strm type = FIFO impl = LUTRAM
    // parse byte stream to double or float
    parseDouble(db_strm, e_d_strm, v_d_strm, db64_strm, ft32_strm);

    hls::stream<ap_uint<DATA_BW> > s_w64_strm("s_w64_strm");
#pragma HLS stream variable = s_w64_strm depth = 512
#pragma HLS bind_storage variable = s_w64_strm type = FIFO impl = BRAM
    hls::stream<ap_uint<VALID_BW> > s_byte_valid_strm("s_byte_valid_strm");
#pragma HLS stream variable = s_byte_valid_strm depth = 512
#pragma HLS bind_storage variable = s_byte_valid_strm type = FIFO impl = LUTRAM
    hls::stream<bool> s_vld_strm("s_vld_strm");
#pragma HLS stream variable = s_vld_strm depth = 512
#pragma HLS bind_storage variable = s_vld_strm type = FIFO impl = SRL
    // split byte stream to form up 64-bit data block, remove the \"
    parseString(str_strm, e_s_strm, v_s_strm, s_w64_strm, s_byte_valid_strm, s_vld_strm);

    hls::stream<ap_uint<DATA_BW> > date64_strm("date64_strm");
#pragma HLS stream variable = date64_strm depth = 64
#pragma HLS bind_storage variable = date64_strm type = FIFO impl = LUTRAM
    // parse byte stream to date
    parseDate(date_strm, e_dt_strm, date64_strm);

    // re-order and pack parsed data into ObjectEx stream
    mergeField<ARRAY_BW, FIELD_BW, TYPE_BW, VALID_BW, DATA_BW>(
        num_of_column, hash_strm_1, dt_strm_1, offt_strm, null_strm, e_strm_1, int64_strm, db64_strm, ft32_strm,
        s_w64_strm, s_byte_valid_strm, s_vld_strm, bool_strm, date64_strm, o_ln_e_strm, o_obj_strm);
}
}
}
}
}
#endif
