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

/**
 * @file L2_types.hpp
 * @brief header file for data types used in L2/L3 host code.
 *
 * This file is part of Vitis SPARSE Library.
 */
#ifndef XF_SPARSE_L2_TYPES_HPP
#define XF_SPARSE_L2_TYPES_HPP

#include <algorithm>
#include <cassert>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <vector>

using namespace std;

namespace xf {
namespace sparse {

template <typename t_DataType, typename t_IndexType>
class NnzUnit {
   public:
    NnzUnit() {}
    NnzUnit(t_IndexType p_row, t_IndexType p_col, t_DataType p_val) : m_row(p_row), m_col(p_col), m_val(p_val) {}
    inline t_IndexType& getRow() { return m_row; }
    inline t_IndexType& getCol() { return m_col; }
    inline t_DataType& getVal() { return m_val; }
    void scan(istream& p_is) {
        double l_val;
        p_is >> m_row >> m_col >> l_val;
        m_val = (t_DataType)(l_val);
        if ((m_row <= 0) || (m_col <= 0)) {
            cerr << "Error: invalid MTX file line row=" << m_row << "  col=" << m_col << "  val=" << m_val << endl;
            return;
        }
        // indices start from 1 in .mtx file, 0 locally
        m_row--;
        m_col--;
        return;
    }
    friend bool operator<(NnzUnit<t_DataType, t_IndexType>& a, NnzUnit<t_DataType, t_IndexType>& b) {
        if (a.getCol() < b.getCol()) {
            return true;
        } else if (a.getCol() == b.getCol()) {
            if (a.getRow() <= b.getRow()) {
                return true;
            }
        }
        return false;
    }
    void print(ostream& p_os) {
        p_os << setw(SPARSE_printWidth) << m_row << " " << setw(SPARSE_printWidth) << m_col << "  "
             << setw(SPARSE_printWidth) << m_val;
    }

   private:
    t_IndexType m_row, m_col;
    t_DataType m_val;
};

template <typename t_DataType, typename t_IndexType>
ostream& operator<<(ostream& p_os, NnzUnit<t_DataType, t_IndexType>& p_nnzUnit) {
    p_nnzUnit.print(p_os);
    return (p_os);
}

template <typename t_DataType,
          typename t_IndexType,
          unsigned int t_ParEntries,
          unsigned int t_NnzRowIdxMemBits,
          unsigned int t_ColPtrMemBits>
class CscMat {
   public:
    typedef NnzUnit<t_DataType, t_IndexType> t_NnzUnitType;
    static const unsigned int t_NnzRowIdxMemWords = t_NnzRowIdxMemBits / (8 * sizeof(t_DataType));
    static const unsigned int t_ColPtrMemWords = t_ColPtrMemBits / (8 * sizeof(t_IndexType));

   public:
    CscMat()
        : m_rows(0),
          m_cols(0),
          m_nnzs(0),
          m_rowMinIdx(0),
          m_rowMaxIdx(0),
          m_colMinIdx(0),
          m_colMaxIdx(0),
          m_valRowIdxAddr(nullptr),
          m_colPtrAddr(nullptr) {
        assert(sizeof(t_DataType) == sizeof(t_IndexType));
        assert((t_NnzRowIdxMemBits % (8 * sizeof(t_DataType))) == 0);
        assert(t_NnzRowIdxMemWords == (t_ParEntries * 2));
    }
    CscMat(unsigned int p_rows, unsigned int p_cols, unsigned int p_nnzs, void* p_valRowIdxAddr, void* p_colPtrAddr)
        : m_rows(p_rows), m_cols(p_cols), m_nnzs(p_nnzs), m_valRowIdxAddr(p_valRowIdxAddr), m_colPtrAddr(p_colPtrAddr) {
        assert(sizeof(t_DataType) == sizeof(t_IndexType));
        assert((t_NnzRowIdxMemBits % (8 * sizeof(t_DataType))) == 0);
        assert(t_NnzRowIdxMemWords == (t_ParEntries * 2));
        assert(t_ColPtrMemWords % t_ParEntries == 0);
        assert(m_cols % t_ColPtrMemWords == 0);
    }
    inline unsigned int& getRows() { return m_rows; }
    inline unsigned int& getCols() { return m_cols; }
    inline unsigned int& getNnzs() { return m_nnzs; }
    inline unsigned int& getRowMinIdx() { return m_rowMinIdx; }
    inline unsigned int& getRowMaxIdx() { return m_rowMaxIdx; }
    inline unsigned int& getColMinIdx() { return m_colMinIdx; }
    inline unsigned int& getColMaxIdx() { return m_colMaxIdx; }
    inline void setValRowIdxAddr(void* p_valRowIdxAddr) { m_valRowIdxAddr = p_valRowIdxAddr; }
    inline void* getValRowIdxAddr() { return m_valRowIdxAddr; }
    inline void setColPtrAddr(void* p_colPtrAddr) { m_colPtrAddr = p_colPtrAddr; }
    inline void* getColPtrAddr() { return m_colPtrAddr; }
    void storeValRowIdx(vector<t_NnzUnitType>& p_nnzs) {
        if (m_nnzs == 0) {
            m_nnzs = p_nnzs.size();
        } else {
            assert(m_nnzs == p_nnzs.size());
        }
        assert(m_nnzs % t_ParEntries == 0);
        unsigned int l_nnzBlocks = m_nnzs / t_ParEntries;
        for (unsigned int i = 0; i < l_nnzBlocks; ++i) {
            t_IndexType l_parRowIdx[t_ParEntries];
            t_DataType l_parVals[t_ParEntries];
            for (unsigned int j = 0; j < t_ParEntries; ++j) {
                l_parRowIdx[j] = p_nnzs[i * t_ParEntries + j].getRow();
                l_parVals[j] = p_nnzs[i * t_ParEntries + j].getVal();
            }
            uint8_t* l_idxAddr =
                reinterpret_cast<uint8_t*>(m_valRowIdxAddr) + (i * t_NnzRowIdxMemWords) * sizeof(t_DataType);
            uint8_t* l_valAddr = reinterpret_cast<uint8_t*>(m_valRowIdxAddr) +
                                 (i * t_NnzRowIdxMemWords + t_ParEntries) * sizeof(t_DataType);
            unsigned int l_numBytes = t_ParEntries * sizeof(t_DataType);
            memcpy(l_idxAddr, reinterpret_cast<uint8_t*>(&l_parRowIdx[0]), l_numBytes);
            memcpy(l_valAddr, reinterpret_cast<uint8_t*>(&l_parVals[0]), l_numBytes);
        }
    }
    void loadValRowIdx(vector<t_NnzUnitType>& p_nnzs) {
        p_nnzs.resize(m_nnzs);
        assert(m_nnzs % t_ParEntries == 0);
        unsigned int l_nnzBlocks = m_nnzs / t_ParEntries;
        for (unsigned int i = 0; i < l_nnzBlocks; ++i) {
            t_IndexType l_parRowIdx[t_ParEntries];
            t_DataType l_parVals[t_ParEntries];
            uint8_t* l_idxAddr =
                reinterpret_cast<uint8_t*>(m_valRowIdxAddr) + (i * t_NnzRowIdxMemWords) * sizeof(t_DataType);
            uint8_t* l_valAddr = reinterpret_cast<uint8_t*>(m_valRowIdxAddr) +
                                 (i * t_NnzRowIdxMemWords + t_ParEntries) * sizeof(t_DataType);
            unsigned int l_numBytes = t_ParEntries * sizeof(t_DataType);
            memcpy(reinterpret_cast<uint8_t*>(&l_parRowIdx[0]), l_idxAddr, l_numBytes);
            memcpy(reinterpret_cast<uint8_t*>(&l_parVals[0]), l_valAddr, l_numBytes);
            for (unsigned int j = 0; j < t_ParEntries; ++j) {
                p_nnzs[i * t_ParEntries + j].getRow() = l_parRowIdx[j];
                p_nnzs[i * t_ParEntries + j].getVal() = l_parVals[j];
            }
        }
    }
    void storeColPtr(vector<t_IndexType>& p_colPtrs) {
        unsigned int l_numBytes = p_colPtrs.size() * sizeof(t_IndexType);
        memcpy(reinterpret_cast<uint8_t*>(m_colPtrAddr), reinterpret_cast<uint8_t*>(p_colPtrs.data()), l_numBytes);
    }
    void loadColPtr(vector<t_IndexType>& p_colPtrs) {
        p_colPtrs.resize(m_cols);
        unsigned int l_numBytes = m_cols * sizeof(t_IndexType);
        memcpy(reinterpret_cast<uint8_t*>(p_colPtrs.data()), reinterpret_cast<uint8_t*>(m_colPtrAddr), l_numBytes);
    }

    void loadNnzUnits(vector<t_NnzUnitType>& p_nnzUnits) {
        if (m_nnzs > 0) {
            vector<t_IndexType> l_colPtrs;
            loadValRowIdx(p_nnzUnits);
            loadColPtr(l_colPtrs);
            t_IndexType l_colPtrPre = 0;
            unsigned int l_nnzIdx = 0;
            for (unsigned int i = 0; i < m_cols; ++i) {
                t_IndexType l_colPtrCur = l_colPtrs[i] - l_colPtrPre;
                l_colPtrPre = l_colPtrs[i];
                for (unsigned int j = 0; j < l_colPtrCur; ++j) {
                    p_nnzUnits[l_nnzIdx].getCol() = i + m_colMinIdx;
                    p_nnzUnits[l_nnzIdx].getRow() = p_nnzUnits[l_nnzIdx].getRow() + m_rowMinIdx;
                    l_nnzIdx++;
                }
            }
        }
    }

   private:
    unsigned int m_rows, m_cols, m_nnzs;
    unsigned int m_rowMinIdx, m_rowMaxIdx;
    unsigned int m_colMinIdx, m_colMaxIdx;
    void* m_valRowIdxAddr;
    void* m_colPtrAddr;
};

template <typename t_DataType, unsigned int t_MemBits>
class ColVec {
   public:
    static const unsigned int t_MemWords = t_MemBits / (8 * sizeof(t_DataType));

   public:
    ColVec() : m_entries(0), m_valAddr(nullptr) {}
    ColVec(unsigned int p_entries) : m_entries(p_entries), m_valAddr(nullptr) {}
    inline unsigned int& getEntries() { return m_entries; }
    inline void setValAddr(void* p_valAddr) { m_valAddr = p_valAddr; }
    inline void* getValAddr() { return m_valAddr; }
    void storeVal(vector<t_DataType>& p_colVec) {
        if (m_entries == 0) {
            m_entries = p_colVec.size();
        } else {
            assert(m_entries == p_colVec.size());
        }
        assert(m_entries % t_MemWords == 0);
        unsigned int l_colBlocks = m_entries / t_MemWords;
        for (unsigned int i = 0; i < l_colBlocks; ++i) {
            t_DataType l_colWord[t_MemWords];
            for (unsigned int j = 0; j < t_MemWords; ++j) {
                l_colWord[j] = p_colVec[i * t_MemWords + j];
            }
            unsigned int l_bytes = t_MemWords * sizeof(t_DataType);
            uint8_t* l_valAddr = reinterpret_cast<uint8_t*>(m_valAddr) + i * t_MemWords * sizeof(t_DataType);
            memcpy(l_valAddr, reinterpret_cast<uint8_t*>(&l_colWord[0]), l_bytes);
        }
    }
    void loadVal(vector<t_DataType>& p_colVec) {
        assert(m_entries % t_MemWords == 0);
        p_colVec.resize(m_entries);
        unsigned int l_colBlocks = m_entries / t_MemWords;

        for (unsigned int i = 0; i < l_colBlocks; ++i) {
            t_DataType l_colWord[t_MemWords];
            unsigned int l_bytes = t_MemWords * sizeof(t_DataType);
            uint8_t* l_valAddr = reinterpret_cast<uint8_t*>(m_valAddr) + i * t_MemWords * sizeof(t_DataType);
            memcpy(reinterpret_cast<uint8_t*>(&l_colWord[0]), l_valAddr, l_bytes);
            for (unsigned int j = 0; j < t_MemWords; ++j) {
                p_colVec[i * t_MemWords + j] = l_colWord[j];
            }
        }
    }

   private:
    unsigned int m_entries;
    void* m_valAddr;
};

template <typename t_DataType,
          typename t_IndexType,
          unsigned int t_ParEntries,
          unsigned int t_NnzRowIdxMemBits,
          unsigned int t_ColVecMemBits,
          unsigned int t_HbmChannels>
class CscPartition {
   public:
    typedef CscMat<t_DataType, t_IndexType, t_ParEntries, t_NnzRowIdxMemBits, t_ColVecMemBits> t_CscMatType;
    typedef ColVec<t_DataType, t_ColVecMemBits> t_ColVecType;

   public:
    CscPartition() {}
    t_CscMatType& getMat(unsigned int p_id) { return m_cscMats[p_id]; }
    t_ColVecType& getVec() { return m_colVec; }

   private:
    t_CscMatType m_cscMats[t_HbmChannels]; // partitions are column-major stored
    t_ColVecType m_colVec;
};

} // end namespace sparse
} // end namespace xf
#endif
