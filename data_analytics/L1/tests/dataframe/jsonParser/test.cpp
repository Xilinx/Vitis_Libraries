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

#include <assert.h>
#include <math.h>
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

enum { PU_NM = 2, COL_NM = 16, ARRAY_BW = 4, FIELD_BW = 4, TYPE_BW = 4, VALID_BW = 4, DATA_BW = 64 };
enum { MAX_LIST_LEN = 10, IN_BUFF_DEPTH = 50000, SCHEMA_DEPTH = 300, NUM_IN_LINES = 20 };
const bool display_input_jsons = false;
const std::string delimiter = "/"; // XXX: this delimiter should be the same of the one in ./schema.txt

void json_dut(ap_uint<128> json_buf[IN_BUFF_DEPTH],
              ap_uint<8> schema[SCHEMA_DEPTH],
              hls::stream<xf::data_analytics::dataframe::ObjectEx>& o_obj_strm) {
    xf::data_analytics::dataframe::jsonParser<PU_NM, COL_NM, ARRAY_BW, FIELD_BW, TYPE_BW, VALID_BW, DATA_BW>(
        json_buf, schema, o_obj_strm);
}

#ifndef __SYNTHESIS__
std::string gen_space_random() {
    std::string str;
    int rd = rand() % 5;
    for (int i = 0; i < rd; i++) {
        str += " ";
    }
    return str;
}

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

std::string gen_string_list_random() {
    std::string str;
    int list_len = rand() % MAX_LIST_LEN;
    str += "[";
    if (list_len > 0) {
        str += gen_space_random();
    }
    if (list_len > 0) {
        for (int i = 0; i < list_len; i++) {
            int len = rand() % 1024 + 1;

            str.push_back('"');
            for (int i = 0; i < len; i++) {
                str.push_back('n');
            }
            str.push_back('"');
            str += gen_space_random();
            str += ",";
            if (i < list_len - 1) {
                str += gen_space_random();
            }
        }
        str.pop_back();
        str += gen_space_random();
        str += "]";
    } else {
        str.pop_back();
        str += "null";
    }
    return str;
}

std::string gen_bool_list_random() {
    std::string str;
    int list_len = rand() % MAX_LIST_LEN;
    str += "[";
    if (list_len > 0) {
        str += gen_space_random();
    }
    if (list_len > 0) {
        for (int i = 0; i < list_len; i++) {
            int rd = rand();
            if (rd % 3 == 0)
                str += "false";
            else if (rd % 3 == 1)
                str += "true";
            else
                str += "null";
            str += gen_space_random();
            str += ",";
            if (i < list_len - 1) {
                str += gen_space_random();
            }
        }
        str.pop_back();
        str += gen_space_random();
        str += "]";
    } else {
        str.pop_back();
        str += "null";
    }
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

int gen_json_lines(std::string& json_lines,
                   const std::vector<std::string> field_name,
                   const char* ty_arr,
                   const int field_nm,
                   const int input_lines,
                   const std::string delimiter,
                   const bool display_input_jsons,
                   const int schema_idx) {
    if (schema_idx == 0 || schema_idx == 1) {
        for (int i = 0; i < input_lines; i++) {
            std::string json_line;
            json_line += gen_space_random();
            json_line += "{";
            json_line += gen_space_random();
            for (int j = 0; j < field_nm; j++) {
                int null_col_idx_0 = rand() % field_nm;
                int null_col_idx_1 = rand() % field_nm;
                if (i % 2 == 1 && j == null_col_idx_0) {
                    // insert missing column
                } else {
                    if (field_name[j].find(delimiter) == std::string::npos) { // non-nested field
                        json_line += field_name[j];
                        json_line += gen_space_random();
                        json_line += ":";
                        json_line += gen_space_random();
                        if (i % 2 == 0 && j == null_col_idx_1)
                            json_line.append("null");
                        else if (ty_arr[j] == '0')
                            json_line.append(gen_bool_list_random());
                        else if (ty_arr[j] == '1')
                            json_line.append(gen_int_random());
                        else if (ty_arr[j] == '2')
                            json_line.append(gen_float_random());
                        else if (ty_arr[j] == '3')
                            json_line.append(gen_float_random());
                        else if (ty_arr[j] == '4')
                            json_line.append(gen_date_random());
                        else if (ty_arr[j] == '5')
                            json_line.append(gen_string_list_random());
                        else {
                            std::cout << "ERROR: invalid type for column setting" << std::endl;
                            return -1;
                        }
                        json_line += gen_space_random();
                    } else { // nested field
                        int nest_num = 0;
                        std::size_t pos_s = 0;
                        std::size_t pos_e = 0;
                        std::string full_field = field_name[j];
                        std::string sub_field;
                        // remove the first "
                        full_field.erase(full_field.begin());
                        // remove the last "
                        full_field.pop_back();
                        while ((pos_e = full_field.find(delimiter, pos_s)) != std::string::npos) {
                            sub_field = full_field.substr(pos_s, pos_e - pos_s);
                            pos_s = pos_e + delimiter.length();
                            nest_num++;
                            json_line += "\"";
                            json_line += sub_field;
                            json_line += "\"";
                            json_line += gen_space_random();
                            json_line += ":";
                            json_line += gen_space_random();
                            json_line += "{";
                            json_line += gen_space_random();
                        }
                        sub_field = full_field.substr(pos_s);
                        json_line += "\"";
                        json_line += sub_field;
                        json_line += "\"";
                        json_line += gen_space_random();
                        json_line += ":";
                        json_line += gen_space_random();
                        if (i % 2 == 0 && j == null_col_idx_1)
                            json_line.append("null");
                        else if (ty_arr[j] == '0')
                            json_line.append(gen_bool_list_random());
                        else if (ty_arr[j] == '1')
                            json_line.append(gen_int_random());
                        else if (ty_arr[j] == '2')
                            json_line.append(gen_float_random());
                        else if (ty_arr[j] == '3')
                            json_line.append(gen_float_random());
                        else if (ty_arr[j] == '4')
                            json_line.append(gen_date_random());
                        else if (ty_arr[j] == '5')
                            json_line.append(gen_string_list_random());
                        else {
                            std::cout << "ERROR: invalid type for column setting" << std::endl;
                            return -1;
                        }
                        json_line += gen_space_random();
                        for (int n = 0; n < nest_num; n++) {
                            json_line += "}";
                            json_line += gen_space_random();
                        }
                    }
                    if (j < field_nm - 1) json_line.push_back(',');
                }
                if (i % 2 == 1 && j == field_nm - 1 && null_col_idx_0 == field_nm - 1) json_line.pop_back();
            }
            json_line += "}";
            json_line += gen_space_random();
            json_line += "\n";
            json_lines += json_line;
        }
    } else if (schema_idx == 2) {
        for (int i = 0; i < input_lines; i++) {
            std::string json_line;
            json_line += gen_space_random();
            json_line += "{";
            json_line += gen_space_random();
            for (int j = 0; j < field_nm; j++) {
                int null_col_idx_0 = rand() % field_nm;
                int null_col_idx_1 = rand() % field_nm;
                if (i % 2 == 1 && j == null_col_idx_0) {
                    // insert missing column
                    if (j == 0) {
                        j++;
                        int nest_num = 0;
                        std::size_t pos_s = 0;
                        std::size_t pos_e = 0;
                        std::string full_field = field_name[j];
                        std::string sub_field;
                        // remove the first "
                        full_field.erase(full_field.begin());
                        // remove the last "
                        full_field.pop_back();
                        while ((pos_e = full_field.find(delimiter, pos_s)) != std::string::npos) {
                            sub_field = full_field.substr(pos_s, pos_e - pos_s);
                            pos_s = pos_e + delimiter.length();
                            nest_num++;
                            json_line += "\"";
                            json_line += sub_field;
                            json_line += "\"";
                            json_line += gen_space_random();
                            json_line += ":";
                            json_line += gen_space_random();
                            json_line += "{";
                            json_line += gen_space_random();
                        }
                        sub_field = full_field.substr(pos_s);
                        json_line += "\"";
                        json_line += sub_field;
                        json_line += "\"";
                        json_line += gen_space_random();
                        json_line += ":";
                        json_line += gen_space_random();
                        if (i % 2 == 0 && j == null_col_idx_1)
                            json_line.append("null");
                        else if (ty_arr[j] == '0')
                            json_line.append(gen_bool_list_random());
                        else if (ty_arr[j] == '1')
                            json_line.append(gen_int_random());
                        else if (ty_arr[j] == '2')
                            json_line.append(gen_float_random());
                        else if (ty_arr[j] == '3')
                            json_line.append(gen_float_random());
                        else if (ty_arr[j] == '4')
                            json_line.append(gen_date_random());
                        else if (ty_arr[j] == '5')
                            json_line.append(gen_string_list_random());
                        else {
                            std::cout << "Invalid type for column setting" << std::endl;
                            return -1;
                        }
                        json_line += gen_space_random();
                        for (int n = 0; n < nest_num; n++) {
                            json_line += "}";
                            json_line += gen_space_random();
                        }
                        json_line.push_back(',');
                    } else if (j == 1) {
                        json_line.pop_back();
                        int nest_num = 0;
                        std::size_t pos_s = 0;
                        std::size_t pos_e = 0;
                        std::string full_field = field_name[j - 1];
                        std::string sub_field;
                        // remove the first "
                        full_field.erase(full_field.begin());
                        // remove the last "
                        full_field.pop_back();
                        while ((pos_e = full_field.find(delimiter, pos_s)) != std::string::npos) {
                            sub_field = full_field.substr(pos_s, pos_e - pos_s);
                            pos_s = pos_e + delimiter.length();
                            nest_num++;
                        }
                        for (int n = 0; n < nest_num; n++) {
                            json_line += "}";
                            json_line += gen_space_random();
                        }
                        json_line.push_back(',');
                    }
                } else {
                    if (field_name[j].find(delimiter) == std::string::npos) { // non-nested field
                        json_line += field_name[j];
                        json_line += gen_space_random();
                        json_line += ":";
                        json_line += gen_space_random();
                        if (i % 2 == 0 && j == null_col_idx_1)
                            json_line.append("null");
                        else if (ty_arr[j] == '0')
                            json_line.append(gen_bool_list_random());
                        else if (ty_arr[j] == '1')
                            json_line.append(gen_int_random());
                        else if (ty_arr[j] == '2')
                            json_line.append(gen_float_random());
                        else if (ty_arr[j] == '3')
                            json_line.append(gen_float_random());
                        else if (ty_arr[j] == '4')
                            json_line.append(gen_date_random());
                        else if (ty_arr[j] == '5')
                            json_line.append(gen_string_list_random());
                        else {
                            std::cout << "Invalid type for column setting" << std::endl;
                            return -1;
                        }
                        json_line += gen_space_random();
                    } else { // nested field
                        int nest_num = 0;
                        std::size_t pos_s = 0;
                        std::size_t pos_e = 0;
                        std::string full_field = field_name[j];
                        std::string sub_field;
                        // remove the first "
                        full_field.erase(full_field.begin());
                        // remove the last "
                        full_field.pop_back();
                        while ((pos_e = full_field.find(delimiter, pos_s)) != std::string::npos) {
                            sub_field = full_field.substr(pos_s, pos_e - pos_s);
                            pos_s = pos_e + delimiter.length();
                            nest_num++;
                            if (j == 0) {
                                json_line += "\"";
                                json_line += sub_field;
                                json_line += "\"";
                                json_line += gen_space_random();
                                json_line += ":";
                                json_line += gen_space_random();
                                json_line += "{";
                                json_line += gen_space_random();
                            }
                        }
                        sub_field = full_field.substr(pos_s);
                        json_line += "\"";
                        json_line += sub_field;
                        json_line += "\"";
                        json_line += gen_space_random();
                        json_line += ":";
                        json_line += gen_space_random();
                        if (i % 2 == 0 && j == null_col_idx_1)
                            json_line.append("null");
                        else if (ty_arr[j] == '0')
                            json_line.append(gen_bool_list_random());
                        else if (ty_arr[j] == '1')
                            json_line.append(gen_int_random());
                        else if (ty_arr[j] == '2')
                            json_line.append(gen_float_random());
                        else if (ty_arr[j] == '3')
                            json_line.append(gen_float_random());
                        else if (ty_arr[j] == '4')
                            json_line.append(gen_date_random());
                        else if (ty_arr[j] == '5')
                            json_line.append(gen_string_list_random());
                        else {
                            std::cout << "Invalid type for column setting" << std::endl;
                            return -1;
                        }
                        json_line += gen_space_random();
                        if (j == 1) {
                            for (int n = 0; n < nest_num; n++) {
                                json_line += "}";
                                json_line += gen_space_random();
                            }
                        }
                    }
                    if (j < field_nm - 1) json_line.push_back(',');
                }
                if (i % 2 == 1 && j == field_nm - 1 && null_col_idx_0 == field_nm - 1) json_line.pop_back();
            }
            json_line += "}";
            json_line += gen_space_random();
            json_line += "\n";
            json_lines += json_line;
        }
    } else if (schema_idx == 3) {
        for (int i = 0; i < input_lines; i++) {
            std::string json_line;
            json_line += gen_space_random();
            json_line += "{";
            json_line += gen_space_random();
            for (int j = 0; j < field_nm; j++) {
                int null_col_idx_0 = rand() % field_nm;
                int null_col_idx_1 = rand() % field_nm;
                if (i % 2 == 1 && j == null_col_idx_0) {
                    // insert missing column
                    if (j == 0) {
                        j++;
                        int nest_num = 0;
                        std::size_t pos_s = 0;
                        std::size_t pos_e = 0;
                        std::string full_field = field_name[j];
                        std::string sub_field;
                        // remove the first "
                        full_field.erase(full_field.begin());
                        // remove the last "
                        full_field.pop_back();
                        while ((pos_e = full_field.find(delimiter, pos_s)) != std::string::npos) {
                            sub_field = full_field.substr(pos_s, pos_e - pos_s);
                            pos_s = pos_e + delimiter.length();
                            nest_num++;
                            json_line += "\"";
                            json_line += sub_field;
                            json_line += "\"";
                            json_line += gen_space_random();
                            json_line += ":";
                            json_line += gen_space_random();
                            json_line += "{";
                            json_line += gen_space_random();
                        }
                        sub_field = full_field.substr(pos_s);
                        json_line += "\"";
                        json_line += sub_field;
                        json_line += "\"";
                        json_line += gen_space_random();
                        json_line += ":";
                        json_line += gen_space_random();
                        if (i % 2 == 0 && j == null_col_idx_1)
                            json_line.append("null");
                        else if (ty_arr[j] == '0')
                            json_line.append(gen_bool_list_random());
                        else if (ty_arr[j] == '1')
                            json_line.append(gen_int_random());
                        else if (ty_arr[j] == '2')
                            json_line.append(gen_float_random());
                        else if (ty_arr[j] == '3')
                            json_line.append(gen_float_random());
                        else if (ty_arr[j] == '4')
                            json_line.append(gen_date_random());
                        else if (ty_arr[j] == '5')
                            json_line.append(gen_string_list_random());
                        else {
                            std::cout << "Invalid type for column setting" << std::endl;
                            return -1;
                        }
                        json_line += gen_space_random();
                        for (int n = 0; n < nest_num; n++) {
                            json_line += "}";
                            json_line += gen_space_random();
                        }
                        json_line.push_back(',');
                    } else if (j == 1) {
                        int nest_num = 0;
                        std::size_t pos_s = 0;
                        std::size_t pos_e = 0;
                        std::string full_field = field_name[j - 1];
                        std::string sub_field;
                        // remove the first "
                        full_field.erase(full_field.begin());
                        // remove the last "
                        full_field.pop_back();
                        while ((pos_e = full_field.find(delimiter, pos_s)) != std::string::npos) {
                            sub_field = full_field.substr(pos_s, pos_e - pos_s);
                            pos_s = pos_e + delimiter.length();
                            nest_num++;
                        }
                        for (int n = 0; n < nest_num; n++) {
                            json_line += "}";
                            json_line += gen_space_random();
                        }
                        json_line.push_back(',');
                    }
                } else {
                    if (field_name[j].find(delimiter) == std::string::npos) { // non-nested field
                        json_line += field_name[j];
                        json_line += gen_space_random();
                        json_line += ":";
                        json_line += gen_space_random();
                        if (i % 2 == 0 && j == null_col_idx_1)
                            json_line.append("null");
                        else if (ty_arr[j] == '0')
                            json_line.append(gen_bool_list_random());
                        else if (ty_arr[j] == '1')
                            json_line.append(gen_int_random());
                        else if (ty_arr[j] == '2')
                            json_line.append(gen_float_random());
                        else if (ty_arr[j] == '3')
                            json_line.append(gen_float_random());
                        else if (ty_arr[j] == '4')
                            json_line.append(gen_date_random());
                        else if (ty_arr[j] == '5')
                            json_line.append(gen_string_list_random());
                        else {
                            std::cout << "Invalid type for column setting" << std::endl;
                            return -1;
                        }
                        json_line += gen_space_random();
                    } else { // nested field
                        int nest_num = 0;
                        std::size_t pos_s = 0;
                        std::size_t pos_e = 0;
                        std::string full_field = field_name[j];
                        std::string sub_field;
                        // remove the first "
                        full_field.erase(full_field.begin());
                        // remove the last "
                        full_field.pop_back();
                        while ((pos_e = full_field.find(delimiter, pos_s)) != std::string::npos) {
                            sub_field = full_field.substr(pos_s, pos_e - pos_s);
                            pos_s = pos_e + delimiter.length();
                            nest_num++;
                            if (j == 0) {
                                json_line += "\"";
                                json_line += sub_field;
                                json_line += "\"";
                                json_line += gen_space_random();
                                json_line += ":";
                                json_line += gen_space_random();
                                json_line += "{";
                                json_line += gen_space_random();
                            }
                        }
                        if (j == 1) {
                            json_line += "}";
                            json_line += gen_space_random();
                            json_line += ",";
                        }
                        sub_field = full_field.substr(pos_s);
                        json_line += "\"";
                        json_line += sub_field;
                        json_line += "\"";
                        json_line += gen_space_random();
                        json_line += ":";
                        json_line += gen_space_random();
                        if (i % 2 == 0 && j == null_col_idx_1)
                            json_line.append("null");
                        else if (ty_arr[j] == '0')
                            json_line.append(gen_bool_list_random());
                        else if (ty_arr[j] == '1')
                            json_line.append(gen_int_random());
                        else if (ty_arr[j] == '2')
                            json_line.append(gen_float_random());
                        else if (ty_arr[j] == '3')
                            json_line.append(gen_float_random());
                        else if (ty_arr[j] == '4')
                            json_line.append(gen_date_random());
                        else if (ty_arr[j] == '5')
                            json_line.append(gen_string_list_random());
                        else {
                            std::cout << "Invalid type for column setting" << std::endl;
                            return -1;
                        }
                        json_line += gen_space_random();
                        if (j == 1) {
                            json_line += "}";
                            json_line += gen_space_random();
                            json_line += ",";
                        }
                    }
                    if (j < field_nm - 1 && j > 1) json_line.push_back(',');
                }
                if (i % 2 == 1 && j == field_nm - 1 && null_col_idx_0 == field_nm - 1) json_line.pop_back();
            }
            json_line += "}";
            json_line += gen_space_random();
            json_line += "\n";
            json_lines += json_line;
        }
    } else {
        std::cout << "ERROR: only 4 schemas under test.\n";
    }
    if (display_input_jsons) {
        std::cout << json_lines;
    }
    std::cout << "JSON lines size in bytes: " << json_lines.length() << " bytes\n";
    return 0;
}

void check_unit(const int valid_field_nm,
                hls::stream<xf::data_analytics::dataframe::ObjectEx>& o_obj_strm,
                int info[2]) {
    int nerror = 0;
    int cnt_hw_lines = 0, cnt_hw_fields = 0;
    ap_uint<FIELD_BW> prev_id = ap_uint<FIELD_BW>(-1);
    ap_uint<ARRAY_BW> prev_offt = 0;

    xf::data_analytics::dataframe::ObjectEx t = o_obj_strm.read();
    while (t.get_type() != 15) {
        if (t.get_type() == 13) {
            if (cnt_hw_fields != valid_field_nm && valid_field_nm > 0) {
                std::cout << "ERROR: only " << cnt_hw_fields << " in " << valid_field_nm << " valid fields, at line "
                          << cnt_hw_lines << std::endl;
                nerror++;
            }
            cnt_hw_lines++;
            cnt_hw_fields = 0;
        }
        if (t.get_id() == prev_id && t.get_type() != 13) {
            if (prev_offt > t.get_offset()) {
                std::cout << "ERROR: invalid offset detected at line " << cnt_hw_lines << std::endl;
                nerror++;
            }
        }
        if (t.get_id() != prev_id && t.get_type() != 13) cnt_hw_fields++;
        if (t.get_type() != 13) {
            prev_id = t.get_id();
            prev_offt = t.get_offset();
        } else {
            prev_id = ap_uint<FIELD_BW>(-1);
            prev_offt = 0;
        }
        // std::cout << std::hex << "obj.data = " << t.get_data() << "  obj.type = " << t.get_type()
        //          << "  obj.id = " << t.get_id() << "  obj.offt = " << t.get_offset() << "  obj.vld = " <<
        //          t.get_valid()
        //          << std::dec << std::endl;

        o_obj_strm >> t;
    }

    info[0] = nerror;
    info[1] = cnt_hw_lines;
}

int main() {
    int nerrors = 0;
    for (int rnd = 0; rnd < 4; rnd++) {
        std::cout << "Testing schema-" << rnd << "...\n";
        std::string schemafile = "./schema-" + std::to_string(rnd) + ".txt";
        std::ifstream sfile(schemafile.c_str(), std::ios::in);
        if (!sfile.is_open()) {
            std::cout << "Failed to open file " << schemafile << std::endl;
            return 1;
        }

        std::vector<std::string> field_name;
        char ty_arr[256];
        int field_nm = 0;
        ap_uint<COL_NM> valid_field = 0;
        char line[256];
        std::string schema_lines;
        while (sfile.getline(line, sizeof(line))) {
            std::stringstream tmp(line);
            schema_lines.append(tmp.str());
            schema_lines.push_back('\n');
            // std::cout << schema_lines << std::endl;

            std::size_t del_pos = tmp.str().find(':');
            field_name.push_back(tmp.str().substr(0, del_pos));
            valid_field[field_nm] = 1;
            ty_arr[field_nm++] = tmp.str()[del_pos + 1];
        }
        sfile.close();
        if (SCHEMA_DEPTH < (schema_lines.size() + 4)) {
            std::cout << "ERROR: insufficient schema_in buffer size, please consider increasing the SCHEMA_DEPTH or "
                         "decresing the NUM_IN_LINES enumeration\n";
            return 1;
        }

        std::string json_lines;
        if (gen_json_lines(json_lines, field_name, ty_arr, field_nm, NUM_IN_LINES, delimiter, display_input_jsons,
                           rnd)) {
            std::cout << "ERROR: failed to generate json line inputs for FPGA.\n";
            return 1;
        }

        int input_size = json_lines.size();
        if ((sizeof(ap_uint<128>) * IN_BUFF_DEPTH) < (sizeof(ap_uint<128>) + input_size)) {
            std::cout << "ERROR: insufficient ddr_in buffer size, please consider increasing the IN_BUFF_DEPTH or "
                         "decresing the NUM_IN_LINES enumeration\n";
            return 1;
        }
        ap_uint<128>* ddr_in = (ap_uint<128>*)malloc(sizeof(ap_uint<128>) * IN_BUFF_DEPTH);
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

        ap_uint<8>* schema_in = (ap_uint<8>*)malloc(SCHEMA_DEPTH);
        schema_in[0] = valid_field(7, 0);
        schema_in[1] = valid_field(15, 8);
        schema_in[2] = schema_lines.size() % 256;
        schema_in[3] = schema_lines.size() / 256;
        for (int i = 0; i < schema_lines.size(); i++) {
            int8_t c = schema_lines[i];
            schema_in[i + 4] = c;
        }

        hls::stream<xf::data_analytics::dataframe::ObjectEx> o_obj_strm;
        // call FPGA
        json_dut(ddr_in, schema_in, o_obj_strm);

        int return_flag[2];
        check_unit(field_nm, o_obj_strm, return_flag);

        int nerror = return_flag[0];
        nerrors += nerror;
        int cnt_hw_lines = return_flag[1];
        std::cout << "Total lines: " << cnt_hw_lines << std::endl;
        if (nerror == 0 && cnt_hw_lines == NUM_IN_LINES) {
            std::cout << "TEST-" << rnd << " PASSED!!" << std::endl;
        } else {
            std::cout << "TEST-" << rnd << " FAILED!!" << std::endl;
            std::cout << "nerror = " << nerror << std::endl;
            std::cout << "cnt_hw_lines = " << cnt_hw_lines << std::endl;
        }

        free(ddr_in);
        free(schema_in);
    }

    return nerrors;
}
#endif
