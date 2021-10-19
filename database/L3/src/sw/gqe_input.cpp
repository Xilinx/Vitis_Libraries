/*
 * Copyright 2020 Xilinx, Inc.
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
#include "xf_database/gqe_input.hpp"

namespace xf {
namespace database {
namespace gqe {

using namespace std;

size_t TableSection::getRowNum() {
    return _nrow;
}

size_t TableSection::getSecRowNum(size_t sec_id) {
    return _sec_nrow[sec_id];
}

size_t TableSection::getColNum() {
    return _col_type_size.size();
}

size_t TableSection::getSecNum() {
    return _col_ptr.size();
}

size_t TableSection::getColTypeSize(size_t col_id) {
    return _col_type_size[col_id];
}

char* TableSection::getColPointer(size_t sec_id, size_t col_id) {
    return _col_ptr[sec_id][col_id];
}

char* TableSection::getValColPointer(size_t sec_id) {
    return _val_ptr[sec_id];
}

vector<string> TableSection::getColNames() {
    return _col_name;
}

string TableSection::getRowIDColName() {
    return _rowID_col_name;
}

string TableSection::getValidColName() {
    return _valid_col_name;
}

bool TableSection::getRowIDEnableFlag() {
    return _rowID_en_flag;
}

bool TableSection::getValidEnableFlag() {
    return _valid_en_flag;
}

TableSection::~TableSection(){};

TableSection::TableSection(string tab_name,
                           vector<string> col_name,
                           vector<size_t> col_type_size,
                           bool enRowID,
                           bool enValid,
                           string rowID_col_name,
                           string valid_col_name)
    : _tab_name(tab_name),
      _col_name(col_name),
      _col_type_size(col_type_size),
      _valid_col_num(_col_name.size()),
      _rowID_en_flag(enRowID),
      _valid_en_flag(enValid),
      _rowID_col_name(rowID_col_name),
      _valid_col_name(valid_col_name),
      _nrow(0),
      _sec_num(0) {}

size_t TableSection::addSec(vector<char*> sec_ptr, char* val_ptr, size_t sec_rows) {
    _col_ptr.push_back(sec_ptr);
    _val_ptr.push_back(val_ptr);
    _sec_nrow.push_back(sec_rows);
    _nrow += sec_rows;
    _sec_num++;
    return sec_rows;
}

void TableSection::info() {
    for (int i = 0; i < _col_name.size(); i++) {
        std::cout << "_col_name: " << _col_name[i] << std::endl;
    }

    std::cout << "_col_ptr.size() = " << _col_ptr.size() << std::endl;
    std::cout << "_val_ptr.size() = " << _val_ptr.size() << std::endl;

    for (int i = 0; i < _col_type_size.size(); i++) {
        std::cout << "_col_type_size: " << _col_type_size[i] << std::endl;
    }

    for (int i = 0; i < _sec_nrow.size(); i++) {
        std::cout << "_sec_nrow: " << _sec_nrow[i] << std::endl;
    }

    std::cout << "_vali_col_num = " << _valid_col_num << std::endl;
    std::cout << "_nrow = " << _nrow << std::endl;
    std::cout << "_sec_num = " << _sec_num << std::endl;
    std::cout << "_tab_name = " << _tab_name << std::endl;

    std::cout << "_rowID_en_flag = " << _rowID_en_flag << std::endl;
    std::cout << "_valid_en_flag = " << _valid_en_flag << std::endl;

    std::cout << "_rowID_col_name = " << _rowID_col_name << std::endl;
    std::cout << "_valid_col_name = " << _valid_col_name << std::endl;
}

} // gqe
} // database
} // xf
