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

#ifndef XF_DATA_ANALYTICS_L1_DATAFRAME_INTERNAL_PARSE_VALUE_HPP
#define XF_DATA_ANALYTICS_L1_DATAFRAME_INTERNAL_PARSE_VALUE_HPP
#include "hls_stream.h"
#include "ap_int.h"
#include "xf_data_analytics/dataframe/df_utils.hpp"
#include <stdint.h>

#ifndef __SYNTHESIS__
#include <iostream>
#endif

//#define __DEBUG_JSON_DUPLICATE__

namespace xf {
namespace data_analytics {
namespace dataframe {
namespace internal {
/**
 *
 * @brief parse the value and dispatched it based one data type.
 *
 * @param i_strm input value stream.
 * @param i_idx_strm input value index.
 * @param i_vld_strm input valid flag for each character of value.
 * @param i_e_strm input end flag of i_strm.
 * @param i_hash_strm input index for each value.
 * @param i_type_strm input data type for each value.
 * @param o_int_strm output value with int type.
 * @param e_i_strm output end flag of o_int_strm.
 * @param v_i_strm output valid flag for o_int_strm.
 * @param o_double_strm output value with double type.
 * @param e_d_strm output end flag of o_double_strm.
 * @param v_d_strm output valid flag of o_double_strm.
 * @param o_str_strm output value with string type.
 * @param e_s_strm output end flag of o_str_strm.
 * @param v_s_strm output valid flag of o_str_strm.
 * @param o_bool_strm output value with boolean type.
 * @param o_date_strm output value with date type.
 * @param e_dt_strm output end flag of o_date_strm.
 * @param o_hash_strm output index for each value
 * @param o_dt_strm output data type for each index.
 * @param o_offt_strm output offset of the array for each value (all 1's is considered as end of array or non-array
 * value)
 * @param o_null_strm  null flag for each output value.
 * @param o_e_strm output end flag for o_hash_strm.
 */
template <int ARRAY_BW, int FIELD_BW, int TYPE_BW>
void parseValue(hls::stream<ap_uint<8> >& i_strm,
                hls::stream<ap_uint<ARRAY_BW> >& i_idx_strm,
                hls::stream<bool>& i_vld_strm,
                hls::stream<bool>& i_e_strm,

                hls::stream<ap_uint<FIELD_BW> >& i_hash_strm,
                hls::stream<ap_uint<TYPE_BW> >& i_type_strm,
                // int64
                hls::stream<ap_uint<8> >& o_int_strm,
                hls::stream<bool>& e_i_strm,
                hls::stream<bool>& v_i_strm,
                // double
                hls::stream<ap_uint<8> >& o_double_strm,
                hls::stream<bool>& e_d_strm,
                hls::stream<bool>& v_d_strm,
                // string
                hls::stream<ap_uint<8> >& o_str_strm,
                hls::stream<bool>& e_s_strm,
                hls::stream<bool>& v_s_strm,
                // Boolean
                hls::stream<bool>& o_bool_strm,
                // Date
                hls::stream<ap_uint<9> >& o_date_strm,
                hls::stream<bool>& e_dt_strm,

                hls::stream<ap_uint<FIELD_BW> >& o_hash_strm,
                hls::stream<ap_uint<TYPE_BW> >& o_dt_strm,
                hls::stream<ap_uint<ARRAY_BW> >& o_offt_strm,
                hls::stream<bool>& o_null_strm,
                hls::stream<bool>& o_e_strm) {
    bool first = true;
    bool is_null;
    // write one dummy flag.
    e_dt_strm.write(false);

    bool e = i_e_strm.read();
    while (!e) {
#pragma HLS pipeline II = 1
        ap_uint<8> in = i_strm.read();
        e = i_e_strm.read();
        ap_uint<TYPE_BW> data_type = i_type_strm.read();
        ap_uint<FIELD_BW> key_idx = i_hash_strm.read();
        ap_uint<ARRAY_BW> val_idx = i_idx_strm.read();
        bool i_vld = i_vld_strm.read();
        if (first) {
            is_null = (in == 'n');
            first = false;
        }
        switch (data_type) {
            case (TString): {
                // ignore the null.
                if ((!is_null)) {
                    {
#pragma HLS latency min = 0 max = 0
                        o_str_strm.write(in);
                        e_s_strm.write(false);
                        v_s_strm.write(i_vld);
                    }
                }
                break;
            }
            case (TInt64): {
                // ignore the null
                if (!is_null) {
                    {
#pragma HLS latency min = 0 max = 0
                        o_int_strm.write(in);
                        e_i_strm.write(false);
                        v_i_strm.write(i_vld);
                    }
                }
                break;
            }
            case (TDate): {
                // ignore the null and concat the output stream into one stream.
                ap_uint<9> tmp = 0;
                tmp.range(8, 1) = in;
                if (!is_null) {
                    tmp[0] = i_vld;
                    {
#pragma HLS latency min = 0 max = 0
                        o_date_strm.write(tmp);
                        e_dt_strm.write(false);
                    }
                }
                break;
            }
            case (TDouble):
            case (TFloat32): {
                if (!is_null) {
                    {
#pragma HLS latency min = 0 max = 0
                        o_double_strm.write(in);
                        e_d_strm.write(false);
                        v_d_strm.write(i_vld);
                    }
                }
                break;
            }
            case (TBoolean):
                break;
            default:
#ifndef __SYNTHESIS__
                std::cout << "This data type is not supported.\n";
#endif
                break;
        }
        if (!i_vld) {
            first = true;
            // output index
            o_hash_strm.write(key_idx);
            o_dt_strm.write(data_type);
            o_offt_strm.write(val_idx);
            o_null_strm.write(is_null);
            o_e_strm.write(false);

            // output for boolean data type.
            if (data_type == TBoolean && !is_null) {
                if (in == 'f' || in == 'F')
                    o_bool_strm.write(false);
                else
                    o_bool_strm.write(true);
            }
        }
    }

    // int
    e_i_strm.write(true);
    o_int_strm.write(0);
    v_i_strm.write(false);
    // dobule
    e_d_strm.write(true);
    // Date
    e_dt_strm.write(true);
    // String
    e_s_strm.write(true);
    // end flag
    o_e_strm.write(true);
    o_hash_strm.write(0);
    o_dt_strm.write(0);
    o_offt_strm.write(-1);
    o_null_strm.write(true);
}
/**
 *
 * @brief merge the two input indexes along with the value byte stream
 * and compose a pair of output for each value.
 * One index is from parseKey and the other one is from addNull.
 * The value stream is directly coming from iterativeParse.
 *
 * @param i_strm input value stream.
 * @param i_idx_strm input value index.
 * @param i_vld_strm input valid flag for each character of value.
 * @param i_e_strm input end flag of i_strm.
 * @param i_hash_strm_0 input index 0 from parseKey.
 * @param i_ln_e_strm_0 input line end flag from parseKey.
 * @param i_e_strm_0 input end flag for i_hash_strm_0.
 * @param i_hash_strm_1 input index 1 from addNull.
 * @param i_ln_e_strm_1 input line end flag from addNull.
 * @param type_buff data type buffer from schema.
 * @param o_strm output value stream.
 * @param o_idx_strm output value index.
 * @param o_vld_strm output valid flag for each character,
 * @param o_e_strm output end flag for o_strm.
 * @param o_hash_strm output merged index.
 * @param o_type_strm output data type for input index.
 *
 */

template <int COL_NUM, int ARRAY_BW, int FIELD_BW, int TYPE_BW>
void combineValidAndNull(hls::stream<ap_uint<8> >& i_strm,
                         hls::stream<ap_uint<ARRAY_BW> >& i_idx_strm,
                         hls::stream<bool>& i_vld_strm,
                         hls::stream<bool>& i_e_strm,

                         hls::stream<ap_uint<COL_NUM + 1> >& i_hash_strm_0,
                         hls::stream<bool>& i_ln_e_strm_0,
                         hls::stream<bool>& i_e_strm_0,

                         hls::stream<ap_uint<COL_NUM + 1> >& i_hash_strm_1,
                         hls::stream<bool>& i_ln_e_strm_1,
                         ap_uint<TYPE_BW> type_buff[COL_NUM],

                         hls::stream<ap_uint<8> >& o_strm,
                         hls::stream<ap_uint<ARRAY_BW> >& o_idx_strm,
                         hls::stream<bool>& o_vld_strm,
                         hls::stream<bool>& o_e_strm,
                         hls::stream<ap_uint<FIELD_BW> >& o_hash_strm,
                         hls::stream<ap_uint<TYPE_BW> >& o_type_strm) {
    ap_uint<TYPE_BW> type_buff_local[COL_NUM];
#pragma HLS bind_storage variable = type_buff_local type = RAM_2P impl = LUTRAM

    // intialize the local buffer
    for (int i = 0; i < COL_NUM; ++i) {
#pragma HLS pipeline II = 1
        type_buff_local[i] = type_buff[i];
    }

    ap_uint<COL_NUM + 1> key_idx;
    bool i_vld = false;
    bool ln_e = false;

    bool e1 = false;
    bool e = i_e_strm.read();
    while (!e || !e1) {
#pragma HLS pipeline II = 1
        // read index when value ended
        if (!i_vld) {
            // read from key
            key_idx = i_hash_strm_0.read();
            ln_e = i_ln_e_strm_0.read();
            e1 = i_e_strm_0.read();
            i_vld = true;
#ifndef __SYNTHESIS__
#ifdef __DEBUG_JSON_DUPLICATE__
            std::cout << "key_idx_0 = " << key_idx << std::endl;
            std::cout << "ln_e_0 = " << ln_e << std::endl;
            std::cout << "e1 = " << e1 << std::endl;
#endif
#endif
        } else if (ln_e) {
            // add null for missing field
            key_idx = i_hash_strm_1.read();
            ln_e = i_ln_e_strm_1.read();
            i_vld = ln_e;
#ifndef __SYNTHESIS__
#ifdef __DEBUG_JSON_DUPLICATE__
            std::cout << "key_idx_1 = " << key_idx << std::endl;
            std::cout << "ln_e_1 = " << ln_e << std::endl;
#endif
#endif
            if (ln_e) {
                o_hash_strm.write(key_idx);
                o_type_strm.write(type_buff_local[key_idx]);
                o_strm.write('n');
                o_idx_strm.write(-1);
                o_vld_strm.write(false);
                o_e_strm.write(false);
            }
        } else if (i_vld) {
            // keep read the input value until it ends.
            ap_uint<8> in = i_strm.read();
            e = i_e_strm.read();
            ap_uint<ARRAY_BW> val_idx = i_idx_strm.read();
            i_vld = i_vld_strm.read();
#ifndef __SYNTHESIS__
#ifdef __DEBUG_JSON_DUPLICATE__
            std::cout << "in = " << (char)in << std::endl;
            std::cout << "e = " << e << std::endl;
            std::cout << "val_idx = " << val_idx << std::endl;
            std::cout << "i_vld = " << i_vld << std::endl;
#endif
#endif
            if (key_idx < COL_NUM) {
                o_hash_strm.write(key_idx);
                o_type_strm.write(type_buff_local[key_idx]);
                o_strm.write(in);
                o_idx_strm.write(val_idx);
                o_vld_strm.write(i_vld);
                o_e_strm.write(false);
            }
        }
    }

    o_e_strm.write(true);
}
}
}
}
}
#endif
