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
#ifndef _GQE_INPUT_
#define _GQE_INPUT_
// commmon
#include <iostream>
#include <vector>
#include <string>
#include <future>
#include <mutex>
#include <condition_variable>

namespace xf {
namespace database {
namespace gqe {

using namespace std;

class TableSection {
   private:
    vector<string> _col_name;
    vector<vector<char*> > _col_ptr; // [sec][column]
    vector<char*> _val_ptr;
    vector<size_t> _col_type_size;
    vector<size_t> _sec_nrow;
    size_t _valid_col_num;
    size_t _nrow;
    size_t _sec_num;
    string _tab_name;

    bool _rowID_en_flag;
    bool _valid_en_flag;

    string _rowID_col_name;
    string _valid_col_name;

   public:
    size_t getRowNum();
    size_t getSecRowNum(size_t sec_id);
    size_t getColNum();
    size_t getSecNum();
    size_t getColTypeSize(size_t col_id);
    char* getColPointer(size_t sec_id, size_t col_id);
    char* getValColPointer(size_t sec_id);
    vector<string> getColNames();
    string getRowIDColName();
    string getValidColName();
    bool getRowIDEnableFlag();
    bool getValidEnableFlag();
    size_t addSec(vector<char*> sec_ptr, char* val_ptr, size_t sec_rows);
    void info();
    ~TableSection();
    TableSection(string tab_name,
                 vector<string> col_name,
                 vector<size_t> col_type_size,
                 bool enRowID,
                 bool enValid,
                 string rowID_col_name,
                 string valid_col_name);
};

} // gqe
} // database
} // xf
#endif
