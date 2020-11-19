/*
 * Copyright 2019 Xilinx, Inc.
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

#ifndef XF_RTM_ALIGNMEM_HPP
#define XF_RTM_ALIGNMEM_HPP

/**
 * @file alignMem.hpp
 * @brief class alignMem is used for aligned memory management
 * @tparam t_DataType the basic datatype
 * @tparam t_Page is the page size for alignment
 */

#include <cstdlib>
#include <fstream>
using namespace std;
template <typename t_DataType, unsigned int t_Page = 4096>
class AlignMem {
   public:
    AlignMem() : m_ptr(nullptr) { m_size = 0; }
    AlignMem(const int l_size) {
        m_size = l_size;
        m_ptr = (t_DataType*)aligned_alloc(t_Page, sizeof(t_DataType) * m_size);
        for (int i = 0; i < m_size; i++) m_ptr[i] = t_DataType(0);
    }
    AlignMem(const AlignMem& l_mem) = delete;
    AlignMem(AlignMem& l_mem) {
        m_ptr = l_mem.m_ptr;
        l_mem.m_ptr = nullptr;
        m_size = l_mem.m_size;
        l_mem.m_size = 0;
    }

    AlignMem(const string filename) {
        ifstream file(filename, ios::binary);
        file.unsetf(ios::skipws);
        streampos fileSize;
        file.seekg(0, ios::end);
        fileSize = file.tellg();
        file.seekg(0, ios::beg);
        m_size = fileSize / sizeof(t_DataType);
        m_ptr = (t_DataType*)aligned_alloc(t_Page, sizeof(t_DataType) * m_size);

        file.read(reinterpret_cast<char*>(m_ptr), fileSize);
        file.close();
    }

    AlignMem* operator=(AlignMem& l_mem) {
        m_ptr = l_mem.m_ptr;
        l_mem.m_ptr = nullptr;
        m_size = l_mem.m_size;
        l_mem.m_size = 0;
        return this;
    }

    const t_DataType* alloc(const int l_size) {
        if (m_ptr != nullptr) free(m_ptr);
        m_size = l_size;
        m_ptr = (t_DataType*)aligned_alloc(t_Page, m_size * sizeof(t_DataType));
        for (int i = 0; i < m_size; i++) m_ptr[i] = t_DataType(0);
        return m_ptr;
    }

    const int size() { return m_size; }

    void release() {
        if (m_ptr == nullptr) return;
        free(m_ptr);
        m_ptr = nullptr;
        m_size = 0;
    }

    t_DataType* ptr() const { return m_ptr; }
    operator t_DataType*() const { return m_ptr; }
    operator void*() const { return static_cast<void*>(m_ptr); }

    const t_DataType& operator[](const int l_addr) const {
        if (l_addr < m_size) return m_ptr[l_addr];
    }

    t_DataType& operator[](const int l_addr) {
        if (l_addr < m_size) return m_ptr[l_addr];
    }

    ~AlignMem() {
        if (m_ptr != nullptr) free(m_ptr);
        m_size = 0;
    }

   public:
    t_DataType* m_ptr;
    int m_size;
};

#endif
