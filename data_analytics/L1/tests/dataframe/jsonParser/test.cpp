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

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <cstring>
#include <algorithm>
#include <ap_int.h>
#include <hls_stream.h>
#include "xf_data_analytics/common/obj_interface.hpp"
#include "xf_data_analytics/dataframe/df_utils.hpp"
#include "xf_data_analytics/dataframe/json_parser.hpp"

enum { PU_NM = 2 };

std::string gen_int_random() {
    std::string str;
    int rd = rand();
    int sg = rand();
    if (sg % 2 == 0)
        str = std::to_string(-rd);
    else
        str = std::to_string(rd);
    return str;
}

std::string gen_float_random() {
    std::string str;
    int base = rand() % 1000;
    double db = base + (double)(rand()) / RAND_MAX;
    str = std::to_string(db);
    return str;
}

std::string gen_string_random() {
    std::string str;
    int len = rand() % 1024 + 1;

    str.push_back('"');
    for (int i = 0; i < len; i++) {
        str.push_back('a');
    }
    str.push_back('"');
    return str;
}

std::string gen_bool_random() {
    std::string str;
    int rd = rand();
    if (rd % 2 == 1)
        str = "false";
    else
        str = "true";
    return str;
}

std::string gen_date_random() {
    int rd = rand();
    std::string test_array[10] = {"\"1970-01-01 00:00:01\"", // time just 1 sec after epoch
                                  "\"2018-11-13 17:11:10\"", // casual time after epoch
                                  "\"1969-12-31 23:59:59\"", // time just 1 sec before epoch
                                  "\"1969-01-01 00:00:00\"", // casual time before epoch
                                  "\"1970-01-01\"",          // date without time
                                  "\"0000-01-01 00:00:00\"", // the starting point of time
                                  "\"9999-12-31 23:59:59\"", // the stopping point of time
                                  "\"1970-01-01 01\"",       // date with hour
                                  "\"1970-01-01 01:01\"",    // date with hour & min
                                  "\"\""};                   // not supported
    return test_array[rd % 7];
}

void check_unit(const int valid_field_nm, hls::stream<xf::data_analytics::dataframe::Object>& o_obj_strm, int info[2]) {
    int nerror = 0;
    int cnt_hw_lines = 0, cnt_hw_fields = 0;
    ap_uint<16> prev_id = 0xFFFF;

    xf::data_analytics::dataframe::Object t = o_obj_strm.read();
    while (t.get_type() != 15) {
#pragma HLS pipeline
        if (t.get_type() == 13) {
            if (cnt_hw_fields != valid_field_nm && valid_field_nm > 0) {
                std::cout << "Error! " << cnt_hw_fields << ", at line " << cnt_hw_lines << std::endl;
                nerror++;
            }
            cnt_hw_lines++;
            cnt_hw_fields = 0;
        }
        if (t.get_id() != prev_id && t.get_type() != 13) cnt_hw_fields++;
        if (t.get_type() != 13)
            prev_id = t.get_id();
        else
            prev_id = 0xFFFF;
        // std::cout << std::hex << t.get_type() << " " << t.get_valid() << " " << t.get_id() << " " << t.get_data()
        //          << std::dec << std::endl;

        o_obj_strm >> t;
    }

    info[0] = nerror;
    info[1] = cnt_hw_lines;
}

void json_dut(const int valid_field_nm, ap_uint<128> json_buf[50000], ap_uint<8> schema[300], int return_flag[2]) {
#pragma HLS dataflow
    hls::stream<xf::data_analytics::dataframe::Object> o_obj_strm;
#pragma HLS stream variable = o_obj_strm depth = 32
    xf::data_analytics::dataframe::jsonParser<PU_NM, 16>(json_buf, schema, o_obj_strm);

    check_unit(valid_field_nm, o_obj_strm, return_flag);
}

int main() {
    const int input_lines = 1000;
    std::string schemafile = "./schema.txt";
    std::ifstream sfile(schemafile.c_str(), std::ios::in);
    if (!sfile.is_open()) {
        std::cout << "Failed to open file " << schemafile << std::endl;
        return -1;
    }

    std::vector<std::string> field_name;
    char ty_arr[256];
    int field_nm = 0;
    ap_uint<16> valid_field = 0;
    char line[256];
    std::string schema_lines;
    while (sfile.getline(line, sizeof(line))) {
        std::stringstream tmp(line);
        schema_lines.append(tmp.str());
        schema_lines.push_back('\n');

        std::size_t del_pos = tmp.str().find(':');
        field_name.push_back(tmp.str().substr(0, del_pos));
        valid_field[field_nm] = 1;
        ty_arr[field_nm++] = tmp.str()[del_pos + 1];
    }

    std::string json_lines;
    for (int i = 0; i < input_lines; i++) {
        json_lines += "{";
        for (int j = 0; j < field_nm; j++) {
            int null_col_idx_0 = rand() % field_nm;
            int null_col_idx_1 = rand() % field_nm;
            if (i % 2 == 1 && j == null_col_idx_0) {
                // insert missing column
            } else {
                json_lines += field_name[j];
                json_lines += ":";
                if (i % 2 == 0 && j == null_col_idx_1)
                    json_lines.append("null");
                else if (ty_arr[j] == '0')
                    json_lines.append(gen_bool_random());
                else if (ty_arr[j] == '1')
                    json_lines.append(gen_int_random());
                else if (ty_arr[j] == '2')
                    json_lines.append(gen_float_random());
                else if (ty_arr[j] == '3')
                    json_lines.append(gen_float_random());
                else if (ty_arr[j] == '4')
                    json_lines.append(gen_date_random());
                else if (ty_arr[j] == '5')
                    json_lines.append(gen_string_random());
                else {
                    std::cout << "Invalid type for column setting" << std::endl;
                    return -1;
                }
                if (j < field_nm - 1) json_lines.push_back(',');
            }
            if (i % 2 == 1 && j == field_nm - 1 && null_col_idx_0 == field_nm - 1) json_lines.pop_back();
        }
        json_lines += "}\n";
    }
    // std::cout << json_lines << std::endl;
    std::cout << "Size in bytes: " << json_lines.length() << std::endl;

    int input_size = json_lines.size();
    int dep = 50000;
    ap_uint<128>* ddr_in = (ap_uint<128>*)malloc(sizeof(ap_uint<128>) * dep);
    ap_uint<128> d_128;
    ap_uint<4> vec_len = 0;
    int rnm = 1;
    for (int i = 0; i < input_size; i++) {
        int8_t c = json_lines[i];
        d_128((vec_len + 1) * 8 - 1, vec_len * 8) = c;
        vec_len++;
        if (vec_len == 0) {
            ddr_in[rnm] = d_128;
            rnm++;
        }
    }
    if (vec_len != 0) {
        ap_uint<128> t = 0;
        t(vec_len * 8 - 1, 0) = d_128(vec_len * 8 - 1, 0);
        ddr_in[rnm++] = t;
    }

    const int NM = PU_NM;
    int avg = (rnm - 1) / NM;
    int left = rnm - 1 - avg * NM;
    int last = avg + left;
    ap_uint<128> head = 0;
    head.range(31, 0) = avg;
    head.range(63, 32) = last;
    ddr_in[0] = head;

    int dep2 = 300;
    ap_uint<8>* schema_in = (ap_uint<8>*)malloc(dep2);
    schema_in[0] = valid_field(7, 0);
    schema_in[1] = valid_field(15, 8);
    schema_in[2] = schema_lines.size() % 256;
    schema_in[3] = schema_lines.size() / 256;
    for (int i = 0; i < schema_lines.size(); i++) {
        int8_t c = schema_lines[i];
        schema_in[i + 4] = c;
    }

    int return_flag[2];
    json_dut(field_nm, ddr_in, schema_in, return_flag);

    int nerror = return_flag[0];
    int cnt_hw_lines = return_flag[1];
    std::cout << "Total lines: " << cnt_hw_lines << std::endl;
    if (nerror == 0 && cnt_hw_lines == input_lines)
        std::cout << "TEST PASSED!!" << std::endl;
    else
        std::cout << "TEST FAILED!!" << std::endl;

    free(ddr_in);
    free(schema_in);
    sfile.close();

    return 0;
}
