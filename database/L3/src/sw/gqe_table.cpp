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
#include "xf_database/gqe_table.hpp"
#include <assert.h>

namespace xf {
namespace database {
namespace gqe {

Table::Table() {
    _valid_col_num = 0;
    _nrow = 0;
};

void Table::addCol(std::string _name, TypeEnum type_size, std::vector<std::string> dat_list) {
    _col_name.push_back(_name);
    _col_type_size.push_back((int)type_size);

    std::vector<char*> col_slice_ptrs;
    for (int i = 0; i < (int)dat_list.size(); i++) {
        // alloc pointer
        std::string file_path = dat_list[i];
        FILE* f = fopen(file_path.c_str(), "rb");
        if (!f) {
            std::cerr << "ERROR: " << file_path << " cannot be opened for binary read." << std::endl;
        }
        fseek(f, 0, SEEK_END); // seek to end of file
        size_t n = ftell(f);   // get current file pointer
        fseek(f, 0, SEEK_SET); // seek back to beginning of file
        void* _ptr = NULL;
        if (posix_memalign(&(_ptr), 4096, n)) throw std::bad_alloc();
        char* ptr = reinterpret_cast<char*>(_ptr);
        if ((int)_sec_nrow.size() < (i + 1)) { // the first dat_list info
            _sec_nrow.push_back(n / (int)type_size);
            _nrow += n / (int)type_size;
        } else if (_sec_nrow[i] != (int)(n / (int)type_size)) {
            std::cout << _sec_nrow[i] << "!=" << n / (int)type_size << std::endl;
            std::cout << "Each column should have same sections!" << std::endl;
            exit(1);
        }

        size_t cnt = fread(ptr, sizeof(char), n, f);
        if (cnt != n) {
            std::cerr << "ERROR: " << cnt << " entries read from " << file_path << ", " << n << " entries required."
                      << std::endl;
            exit(1);
        }
        col_slice_ptrs.push_back(ptr);
        fclose(f);
    }
    _col_ptr.push_back(col_slice_ptrs);
    _valid_col_num = _valid_col_num + 1;
};

void Table::addCol(std::string _name, TypeEnum type_size, std::vector<struct ColPtr> ptr_pairs) {
    _col_name.push_back(_name);
    _col_type_size.push_back((int)type_size);

    std::vector<char*> col_slice_ptrs;
    for (size_t i = 0; i < ptr_pairs.size(); i++) {
        int slice_nrow = ptr_pairs[i].len;
        if (_sec_nrow.size() < (i + 1)) {
            _sec_nrow.push_back(slice_nrow);
            _nrow += slice_nrow;
            // std::cout << "slice_nrow debug: " << slice_nrow << std::endl;
        } else if (_sec_nrow[i] != slice_nrow) {
            std::cout << _sec_nrow[i] << "!=" << slice_nrow << std::endl;
            std::cout << "Each column should have same sections!" << std::endl;
            exit(1);
        }
        col_slice_ptrs.push_back((char*)ptr_pairs[i].ptr);
    }

    _col_ptr.push_back(col_slice_ptrs);
    _valid_col_num = _valid_col_num + 1;
}
void Table::addCol(std::string _name, TypeEnum type_size, int row_num) {
    _col_name.push_back(_name);
    _col_type_size.push_back((int)type_size);
    assert(row_num > 0 && "The table must has row_num>0!");
    if (_valid_col_num == 0) {
        _nrow = row_num;
    } else {
        assert(row_num == _nrow && "Please each col has the same row number!");
    }
    int tsize = (int)type_size;
    int bufsize = tsize * row_num;
    void* _ptr = NULL;
    if (posix_memalign(&(_ptr), 4096, bufsize)) throw std::bad_alloc();
    char* ptr = reinterpret_cast<char*>(_ptr);

    _col_ptr.push_back({ptr});
    _valid_col_num = _valid_col_num + 1;
}
void Table::addCol(std::string _name, TypeEnum type_size, void* _ptr, int row_num) {
    _col_name.push_back(_name);
    _col_type_size.push_back((int)type_size);
    if (_valid_col_num == 0) {
        _nrow = row_num;
    } else {
        assert(row_num == _nrow && "Please each col has the same row number!");
    }
    _col_ptr.push_back({(char*)_ptr});
    _valid_col_num = _valid_col_num + 1;
}

void Table::setRowNum(int _num) {
    _nrow = _num;
};

size_t Table::getRowNum() const {
    return _nrow;
};

size_t Table::getColNum() const {
    return _valid_col_num;
};

char* Table::getColPointer(int i, int _slice_num, int j) const {
    if (_slice_num != 0) {
        int slice_nrow = (_nrow + _slice_num - 1) / _slice_num;
        return _col_ptr[i][0] + j * slice_nrow * _col_type_size[i];
    } else {
        return _col_ptr[i][j];
    }
}

char* Table::getColPointer(int i) const {
    return _col_ptr[i][0];
}

void Table::setColNames(std::vector<std::string> col_names) {
    _valid_col_num = col_names.size();
    for (int i = 0; i < _valid_col_num; i++) {
        _col_name[i] = col_names[i];
    }
}

std::vector<std::string> Table::getColNames() {
    std::vector<std::string> col_names;
    for (int i = 0; i < _valid_col_num; i++) {
        col_names.push_back(_col_name[i]);
    }
    return col_names;
}
size_t Table::getColTypeSize(int cid) const {
    return _col_type_size[cid];
}
size_t Table::getSecRowNum(int sid) const {
    return _sec_nrow[sid];
}
size_t Table::getSecNum() const {
    if (_col_ptr[0].size() != _sec_nrow.size()) {
        std::cout << "_col_ptr.size() != _sec_nrow.size()" << std::endl;
        exit(1);
    }
    return _sec_nrow.size();
}
Table::~Table(){};

void Table::info() {
    std::cout << "valid column num: " << _valid_col_num << std::endl;
    for (int c = 0; c < _valid_col_num; c++) {
        std::cout << "column " << c << ": " << _col_name[c] << std::endl;
    }
    std::cout << "each col row num: " << _nrow << std::endl;
}

} // database
} // gqe
} // xf
