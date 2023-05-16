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

#ifndef XF_DATA_ANALYTICS_L1_DATAFRAME_INTERNAL_PARSE_KEY_HPP
#define XF_DATA_ANALYTICS_L1_DATAFRAME_INTERNAL_PARSE_KEY_HPP
#include "hls_stream.h"
#include "ap_int.h"
#include <stdint.h>

//#define __DEBUG_JSON_PARSE_KEY_IN__
//#define __DEBUG_JSON_PARSE_KEY_OUT__

namespace xf {
namespace data_analytics {
namespace dataframe {
namespace internal {
/**
 *
 * @brief parse the key to find the index for it and filter out the key which is not existed in the schema.
 *
 * @param i_strm input stream for char of key.
 * @param i_idx_strm input key index.
 * @param i_vld_strm valid flag for each char.
 * @param i_e_strm end flag of i_strm.
 * @param i_ln_e_strm line end flag.
 * @param mask_cfg configuration for valid keys from schema.
 * @param o_hash_strm_0 ouput index which corresponds to input key.
 * @param o_ln_e_strm_0 ouput line end flag.
 * @param o_e_strm_0 output end flag of o_hash_strm_0.
 * @param o_mk_strm output mask flag to indicates the missing key for each line.
 * @param o_e_strm output end flag of o_mk_strm;
 * @param key_buff input keys that are described in schema.
 */

template <int COL_NUM>
void parseKey(hls::stream<ap_uint<8> >& i_strm,
              hls::stream<ap_uint<8> >& i_idx_strm,
              hls::stream<bool>& i_vld_strm,
              hls::stream<bool>& i_e_strm,
              hls::stream<bool>& ln_e_strm,
              ap_uint<COL_NUM> mask_cfg,

              hls::stream<ap_uint<COL_NUM + 1> >& o_hash_strm_0,
              hls::stream<bool>& o_ln_e_strm_0,
              hls::stream<bool>& o_e_strm_0,

              // To addNull
              hls::stream<ap_uint<COL_NUM> >& o_mk_strm,
              hls::stream<bool>& o_e_strm,

              ap_uint<8> key_buff[COL_NUM][256]) {
#ifndef __SYNSTHESIS__
#ifdef __DEBUG_JSON_PARSER__
    for (int i = 0; i < COL_NUM; i++) {
        std::cout << "key-" << i << ": ";
        for (int j = 0; j < 10; j++) {
            std::cout << (char)key_buff[i][j];
        }
        std::cout << std::endl;
    }
#endif
#endif
    ap_uint<COL_NUM + 1> hash_val = 0;
    bool flag[COL_NUM];
    // Initialize the flag
    for (int i = 0; i < COL_NUM; ++i) {
#pragma HLS unroll
        flag[i] = true;
    }
    bool e = i_e_strm.read();
    ap_uint<8> idx;
    ap_uint<8> in_byte;
    ap_uint<COL_NUM> record = 0;
    bool i_vld = true;
    bool nb_1 = false;
    bool nb_2 = false;
    bool nb_3 = false;
    bool nb_4 = false;
    bool nb_5 = false;
    bool ln_e = false;
    // write out one more dummy data
    o_e_strm.write(false);
    while (!e) {
#pragma HLS pipeline II = 1
        {
#pragma HLS latency min = 0 max = 0
            // non-blocking read the input stream.
            nb_1 = i_strm.read_nb(in_byte);
            nb_2 = i_vld_strm.read_nb(i_vld);
            nb_3 = i_e_strm.read_nb(e);
            nb_4 = ln_e_strm.read_nb(ln_e);
            nb_5 = i_idx_strm.read_nb(idx);
        }
#ifndef __SYNSTHESIS__
#ifdef __DEBUG_JSON_PARSE_KEY_IN__
        if (nb_1 && nb_2 && nb_3 && nb_4 && nb_5) {
            std::cout << "key_byte = " << (char)in_byte << std::endl;
            std::cout << "key_vld = " << i_vld << std::endl;
            std::cout << "key_e = " << e << std::endl;
            std::cout << "ln_e = " << ln_e << std::endl;
            std::cout << "key_idx = " << idx << std::endl << std::endl;
        }
#endif
#endif
        if (i_vld && nb_1 && nb_2 && nb_3 && nb_4 && nb_5) {
            if (idx == 0 && in_byte == 0) {
                o_hash_strm_0.write(hash_val);
                o_ln_e_strm_0.write(ln_e);
                o_e_strm_0.write(false);
#ifndef __SYNSTHESIS__
#ifdef __DEBUG_JSON_PARSE_KEY_OUT__
                std::cout << "hash_val = " << hash_val << std::endl;
                std::cout << "ln_e = " << ln_e << std::endl;
#endif
#endif
            } else {
                // compare each char
                for (int i = 0; i < COL_NUM; ++i) {
#pragma HLS unroll
                    flag[i] = flag[i] && (in_byte == key_buff[i][idx]);
                }
            }
        } else if (!i_vld && nb_1 && nb_2 && nb_3 && nb_4 && nb_5) {
            hash_val = (1 << COL_NUM);
            // find the matched index of input key
            for (int i = 0; i < COL_NUM; ++i) {
                if (flag[i]) hash_val = i;
                flag[i] = true;
            }
            ap_uint<COL_NUM> ch = 0;
            if (ln_e) {
                // check the missing key.
                ch = record ^ mask_cfg;
                record = 0;
                o_mk_strm.write(ch);
                o_e_strm.write(false);
            } else if (hash_val < COL_NUM) {
                // record the input key
                record[hash_val] = 1;
            }
            // output the index
            o_hash_strm_0.write(hash_val);
            o_ln_e_strm_0.write(ln_e);
            o_e_strm_0.write(false);
#ifndef __SYNSTHESIS__
#ifdef __DEBUG_JSON_PARSE_KEY_OUT__
            std::cout << "hash_val = " << hash_val << std::endl;
            std::cout << "ln_e = " << ln_e << std::endl;
#endif
#endif
        }
    }
    // write-out one more dummay data
    o_hash_strm_0.write(0);
    o_ln_e_strm_0.write(false);
    o_e_strm_0.write(true);

    o_mk_strm.write(0);
    o_e_strm.write(true);
}

/**
 *
 * @brief add index for missing field.
 *
 * @param i_mk_strm input mask flag to indicate the missing field.
 * @param i_e_strm input end flag for i_mk_strm
 * @param o_hash_strm ouput index for missing field.
 * @param o_ln_e_strm output line end flag.
 */
template <int COL_NUM>
void addNull(hls::stream<ap_uint<COL_NUM> >& i_mk_strm,
             hls::stream<bool>& i_e_strm,

             hls::stream<ap_uint<COL_NUM + 1> >& o_hash_strm,
             hls::stream<bool>& o_ln_e_strm) {
    bool e = i_e_strm.read();
    bool ln_e = true;
    ap_uint<COL_NUM> ch = 0;
    while (!e) {
#pragma HLS pipeline II = 2
        if (!ln_e) {
            if (ch == 0) {
                // no missing field for this line.
                ln_e = true;
                o_hash_strm.write(0);
                o_ln_e_strm.write(false);
            } else {
                // add the index for missing field
                ln_e = false;
                ap_uint<COL_NUM> rd = 0;
                for (int i = 0; i < COL_NUM; ++i) {
#pragma HLS unroll
                    if (ch[i]) {
                        rd = i;
                        ch[i] = 0;
                        break;
                    }
                }
                o_hash_strm.write(rd);
                o_ln_e_strm.write(true);
            }
        } else {
            e = i_e_strm.read();
            ch = i_mk_strm.read();
            ln_e = false;
        }
    }
}
}
}
}
}
#endif
