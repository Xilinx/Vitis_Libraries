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
#ifndef _GQE_TABLE_L3_
#define _GQE_TABLE_L3_
// commmon
#include <iostream>
#include <vector>
#include <algorithm>

namespace xf {
namespace database {
namespace gqe {

enum ErrCode { SUCCESS = 0, PARAM_ERROR = 1, MEM_ERROR = 2, DEV_ERROR = 3 };
enum class TypeEnum { TypeInt32 = sizeof(int32_t), TypeInt64 = sizeof(int64_t), TypeBool };

struct ColPtr {
    void* ptr;
    int len;
};

/**
 * @brief column wrapper, column name and data pointer
 *
 */

class Table {
   private:
    std::vector<std::string> _col_name;
    std::vector<std::vector<char*> > _col_ptr;
    std::vector<int> _col_type_size;
    std::vector<int> _sec_nrow;
    int _valid_col_num;
    int _nrow;

   public:
    /**
     * @brief construct of Table.
     *
     */
    Table();

    /**
     * @brief provide pointer by user add it as one column into Table.
     *
     * usage:
     * tab.addCol("o_orderkey", TypeEnum::TypeInt32, tab_o_col0, 10000);
     *
     * @param _name column name
     * @param type_size column element type size
     * @param _ptr column buffer pointer
     * @param row_num row number
     *
     */
    void addCol(std::string _name, TypeEnum type_size, void* _ptr, int row_num);
    /**
     * @brief create pointer for one column and add it into Table.
     *
     * usage:
     * tab.addCol("o_orderkey", TypeEnum::TypeInt32, 10000);
     *
     * @param _name column name
     * @param type_size column element type size
     * @param row_num row number
     *
     */
    void addCol(std::string _name, TypeEnum type_size, int row_num);
    /**
     * @brief create one column with several sections by loading from data file.
     *
     * usage:
     * tab.addCol("o_orderkey", TypeEnum::TypeInt32, {file1.dat,file2.dat});
     *
     * @param _name column name
     * @param type_size column element type size
     * @param dat_list data file list
     *
     */
    void addCol(std::string _name, TypeEnum type_size, std::vector<std::string> dat_list);
    /**
     * @brief create one column with several sections by user's provided pointers
     *
     * usage:
     * tab.addCol("o_orderkey", TypeEnum::TypeInt32, {{ptr1,10000},{ptr2,20000}});
     *
     * @param _name column name
     * @param type_size column element type size
     * @param ptr_pairs vector of (ptr,row_num) pairs
     *
     */
    void addCol(std::string _name, TypeEnum type_size, std::vector<struct ColPtr> ptr_pairs);

    /**
     * @brief set row number.
     *
     * @param _num row number
     *
     */
    void setRowNum(int _num);
    /**
     * @brief set section row number for one column.
     *
     * @param _num section row number
     *
     */
    void setSecRowNum(int _num);

    /**
     * @brief get row number.
     * @return row number
     */
    size_t getRowNum() const;
    /**
     * @brief get section row number.
     * @return section row number of input setction id
     */
    size_t getSecRowNum(int sid) const;

    /**
     * @brief get column num.
     * @return column num.
     */
    size_t getColNum() const;
    /**
     * @brief get section num.
     * @return section num.
     */
    size_t getSecNum() const;

    /**
     * @brief get column data type size.
     * @return date type size of input column id.
     */
    size_t getColTypeSize(int cid) const;

    /**
     * @brief get buffer pointer.
     *
     * @param i column id
     * @param _slice_num divide column i into _slice_num parts
     * @param j get the j'th part pointer after dividing
     *
     * @return column buffer pointer
     *
     * when getColPointer(2,4,1), it means column 2 was divied into 4 sections,
     * return the second part pointer
     *
     */
    char* getColPointer(int i, int _slice_num, int j = 0) const;

    /**
     * @brief get column pointer.
     *
     * @param i column id
     * @return column pointer
     *
     */
    char* getColPointer(int i) const;

    /**
     * @brief set col_names
     *
     * @param col_name column name vector
     *
     */
    void setColNames(std::vector<std::string> col_names);

    /**
     * @brief get col_names
     * @return vector of column names
     */
    std::vector<std::string> getColNames();

    /**
     * @brief deconstruct of Table.
     *
     */
    ~Table();

    /**
     * @brief print info for tab
     */
    void info();
};
}
}
}
#endif
