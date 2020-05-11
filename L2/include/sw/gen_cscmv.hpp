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
 * @file gen_cscmv.hpp
 * @brief header file for generating data images of cscmv operation.
 *
 * This file is part of Vitis SPARSE Library.
 */
#ifndef XF_SPARSE_GEN_CSCMV_HPP
#define XF_SPARSE_GEN_CSCMV_HPP

#include <ctime>
#include <cstdlib>
#include <vector>
#include "L2_types.hpp"
#include "program.hpp"
#include "mtxFile.hpp"

using namespace std;
namespace xf {
namespace sparse {

template <typename t_DataType,
          typename t_IndexType,
          unsigned int t_ParEntries,
          unsigned int t_ParGroups,
          unsigned int t_NnzRowIdxMemBits,
          unsigned int t_ColPtrMemBits,
          unsigned int t_PageSize = 4096>
class GenCscMat {
   public:
    typedef NnzUnit<t_DataType, t_IndexType> t_NnzUnitType;
    typedef CscMat<t_DataType, t_IndexType, t_ParEntries, t_NnzRowIdxMemBits, t_ColPtrMemBits> t_CscMatType;
    const unsigned int t_NnzRowIdxMemWords = t_NnzRowIdxMemBits / (8 * sizeof(t_DataType));
    const unsigned int t_ColPtrMemWords = t_ColPtrMemBits / (8 * sizeof(t_DataType));

   public:
    GenCscMat() {}
    bool genNnzUnitsFromRnd(unsigned int p_rows,
                            unsigned int p_cols,
                            unsigned int p_nnzs,
                            t_DataType p_maxVal,
                            t_DataType p_valStep,
                            vector<t_NnzUnitType>& p_nnzUnits) {
        srand(time(nullptr));
        unsigned int l_rnd = rand() % p_cols;
        unsigned int l_colStep = (l_rnd == 0) ? 1 : l_rnd;
        unsigned int l_rowStep = p_rows * p_cols / (p_nnzs * l_colStep);
        l_rowStep = (l_rowStep == 0) ? 1 : l_rowStep;
        unsigned int l_row = 0, l_col = 0;
        t_DataType l_val = 0;

        for (unsigned int i = 0; i < p_nnzs; ++i) {
            t_NnzUnitType l_nnzUnit(l_row, l_col, l_val);
            p_nnzUnits.push_back(l_nnzUnit);
            l_row += l_rowStep;
            if (l_row >= p_rows) {
                l_row = 0;
                l_col += l_colStep;
            }
            if ((l_col >= p_cols) && (p_nnzUnits.size() < p_nnzs)) {
                cout << "ERROR: generate rnd cscMat" << endl;
                return false;
            }
            l_val += p_valStep;
            if (l_val > p_maxVal) {
                l_val = 0;
            }
        }
        return true;
    }
    void genColPtrVec(unsigned int p_cols,
                      unsigned int l_rowMinIdx,
                      unsigned int l_colMinIdx,
                      vector<t_NnzUnitType>& p_nnzUnits,
                      vector<t_IndexType>& p_colPtrs) {
        p_colPtrs.resize(p_cols);
        for (unsigned int i = 0; i < p_cols; ++i) {
            p_colPtrs[i] = 0;
        }

        for (unsigned int i = 0; i < p_colPtrs.size(); ++i) {
            p_colPtrs[i] = 0;
        }
        for (unsigned int i = 0; i < p_nnzUnits.size(); ++i) {
            p_nnzUnits[i].getRow() = p_nnzUnits[i].getRow() - l_rowMinIdx;
            p_nnzUnits[i].getCol() = p_nnzUnits[i].getCol() - l_colMinIdx;
            t_IndexType l_col = p_nnzUnits[i].getCol();
            if (l_col < p_cols) {
                p_colPtrs[l_col]++;
            } else {
                cout << "ERROR: col index out of the range" << endl;
            }
        }
        for (unsigned int i = 1; i < p_colPtrs.size(); ++i) {
            p_colPtrs[i] += p_colPtrs[i - 1];
        }
    }
    bool genCscMatFromNnzs(unsigned int p_nnzs,
                           vector<t_NnzUnitType>& p_nnzUnits,
                           Program<t_PageSize>& p_program,
                           t_CscMatType& p_cscMat) {
        if (p_nnzs == 0) {
            return true;
        }
        // assume p_nnzUnits have been sorted along cols
        // assume p_nnzs != 0
        p_cscMat.getNnzs() = p_nnzs;
        unsigned int l_rowMinIdx = (p_nnzUnits.front()).getRow();
        unsigned int l_colMinIdx = (p_nnzUnits.front()).getCol();
        unsigned int l_rowMaxIdx = (p_nnzUnits.back()).getRow();
        unsigned int l_colMaxIdx = (p_nnzUnits.back()).getCol();

        p_cscMat.getRowMinIdx() = l_rowMinIdx;
        p_cscMat.getColMinIdx() = l_colMinIdx;
        p_cscMat.getRowMaxIdx() = l_rowMaxIdx;
        p_cscMat.getColMaxIdx() = l_colMaxIdx;
        p_cscMat.getRows() = l_rowMaxIdx - l_rowMinIdx + 1;
        p_cscMat.getCols() = l_colMaxIdx - l_colMinIdx + 1;
        unsigned int l_nnzs = p_nnzUnits.size();
        while (l_nnzs % (t_NnzRowIdxMemWords / 2) != 0) {
            cout << "INFO: padding NNZ unit (rowMaxIdx,colMaxIdx,0) to CscMat" << endl;
            t_NnzUnitType l_nnzUnit(l_rowMaxIdx, l_colMaxIdx, 0);
            p_nnzUnits.push_back(l_nnzUnit);
            l_nnzs++;
        }
        p_cscMat.getNnzs() = l_nnzs;
        unsigned int l_rows = p_cscMat.getRows();
        unsigned int l_cols = p_cscMat.getCols();
        l_rows =
            (t_ParEntries * t_ParGroups) * ((l_rows + (t_ParEntries * t_ParGroups - 1)) / (t_ParEntries * t_ParGroups));
        l_cols = t_ColPtrMemWords * ((l_cols + t_ColPtrMemWords - 1) / t_ColPtrMemWords);
        p_cscMat.getRows() = l_rows;
        p_cscMat.getCols() = l_cols;
        // sort(p_nnzUnits.begin(), p_nnzUnits.end());
        unsigned long long l_cscMatValRowSz = l_nnzs * 2 * sizeof(t_DataType);
        void* l_valRowIdxAddr = p_program.allocMem(l_cscMatValRowSz);
        unsigned long long l_cscMatColPtrSz = l_cols * sizeof(t_IndexType);
        void* l_colPtrAddr = p_program.allocMem(l_cscMatColPtrSz);
        if (l_valRowIdxAddr == nullptr) {
            return false;
        }
        if (l_colPtrAddr == nullptr) {
            return false;
        }
        vector<t_IndexType> l_colPtrs;
        genColPtrVec(l_cols, l_rowMinIdx, l_colMinIdx, p_nnzUnits, l_colPtrs);
        p_cscMat.setValRowIdxAddr(l_valRowIdxAddr);
        p_cscMat.storeValRowIdx(p_nnzUnits);
        p_cscMat.setColPtrAddr(l_colPtrAddr);
        p_cscMat.storeColPtr(l_colPtrs);
        return true;
    }
    bool genCscMatFromNnzUnits(vector<t_NnzUnitType>& p_nnzUnits,
                               Program<t_PageSize>& p_program,
                               t_CscMatType& p_cscMat) {
        unsigned int l_nnzs = p_nnzUnits.size();
        while (l_nnzs % (t_NnzRowIdxMemWords / 2) != 0) {
            cout << "INFO: padding NNZ unit (0,0,0) to CscMat" << endl;
            t_NnzUnitType l_nnzUnit(0, 0, 0);
            p_nnzUnits.push_back(l_nnzUnit);
            l_nnzs++;
        }
        assert(p_cscMat.getNnzs() == l_nnzs);
        unsigned int l_rows = p_cscMat.getRows();
        unsigned int l_cols = p_cscMat.getCols();
        l_rows =
            (t_ParEntries * t_ParGroups) * ((l_rows + (t_ParEntries * t_ParGroups - 1)) / (t_ParEntries * t_ParGroups));
        l_cols = t_ColPtrMemWords * ((l_cols + t_ColPtrMemWords - 1) / t_ColPtrMemWords);
        p_cscMat.getRows() = l_rows;
        p_cscMat.getCols() = l_cols;
        sort(p_nnzUnits.begin(), p_nnzUnits.end());
        unsigned long long l_cscMatValRowSz = l_nnzs * 2 * sizeof(t_DataType);
        void* l_valRowIdxAddr = p_program.allocMem(l_cscMatValRowSz);
        unsigned long long l_cscMatColPtrSz = l_cols * sizeof(t_IndexType);
        void* l_colPtrAddr = p_program.allocMem(l_cscMatColPtrSz);
        if (l_valRowIdxAddr == nullptr) {
            return false;
        }
        if (l_colPtrAddr == nullptr) {
            return false;
        }
        vector<t_IndexType> l_colPtrs;
        genColPtrVec(l_cols, 0, 0, p_nnzUnits, l_colPtrs);
        p_cscMat.setValRowIdxAddr(l_valRowIdxAddr);
        p_cscMat.storeValRowIdx(p_nnzUnits);
        p_cscMat.setColPtrAddr(l_colPtrAddr);
        p_cscMat.storeColPtr(l_colPtrs);
        return true;
    }
    bool genCscMatFromRnd(unsigned int p_rows,
                          unsigned int p_cols,
                          unsigned int p_nnzs,
                          t_DataType p_maxVal,
                          t_DataType p_valStep,
                          Program<t_PageSize>& p_program,
                          t_CscMatType& p_cscMat) {
        bool l_res = false;
        vector<t_NnzUnitType> l_nnzUnits;
        p_cscMat.getRows() = p_rows;
        p_cscMat.getCols() = p_cols;
        p_cscMat.getNnzs() = p_nnzs;
        if (genNnzUnitsFromRnd(p_rows, p_cols, p_nnzs, p_maxVal, p_valStep, l_nnzUnits)) {
            l_res = genCscMatFromNnzUnits(l_nnzUnits, p_program, p_cscMat);
        } else {
            l_res = false;
        }
        return l_res;
    }
};

template <typename t_DataType, unsigned int t_ParEntries, unsigned int t_MemBits, unsigned int t_PageSize = 4096>
class GenVec {
   public:
    const unsigned int t_MemWords = t_MemBits / (8 * sizeof(t_DataType));

   public:
    GenVec() {}
    void genEntVecFromRnd(unsigned int p_entries,
                          t_DataType p_maxVal,
                          t_DataType p_valStep,
                          vector<t_DataType>& p_entryVec) {
        t_DataType l_val = 0;
        for (unsigned int i = 0; i < p_entries; ++i) {
            p_entryVec.push_back(l_val);
            l_val += p_valStep;
            if (l_val > p_maxVal) {
                l_val = 0;
            }
        }
    }
    bool genColVecFromEnt(vector<t_DataType>& p_entryVec,
                          Program<t_PageSize>& p_program,
                          ColVec<t_DataType, t_MemBits>& p_colVec) {
        unsigned int l_entries = p_entryVec.size();
        while (l_entries % t_MemWords != 0) {
            cout << "INFO: padding col vector with 0 entry" << endl;
            p_entryVec.push_back(0);
            l_entries++;
        }
        unsigned long long l_vecSz = l_entries * sizeof(t_DataType);
        void* l_valAddr = p_program.allocMem(l_vecSz);
        if (l_valAddr == nullptr) {
            return false;
        }
        p_colVec.getEntries() = l_entries;
        p_colVec.setValAddr(reinterpret_cast<uint8_t*>(l_valAddr));
        p_colVec.storeVal(p_entryVec);
        return true;
    }
    bool genColVecFromRnd(unsigned int p_entries,
                          t_DataType p_maxVal,
                          t_DataType p_valStep,
                          Program<t_PageSize>& p_program,
                          ColVec<t_DataType, t_MemBits>& p_colVec) {
        vector<t_DataType> l_entryVec;
        p_colVec.getEntries() = p_entries;
        genEntVecFromRnd(p_entries, p_maxVal, p_valStep, l_entryVec);
        bool l_res = false;
        l_res = genColVecFromEnt(l_entryVec, p_program, p_colVec);
        return l_res;
    }

    bool genEmptyColVec(unsigned int p_entries,
                        Program<t_PageSize>& p_program,
                        ColVec<t_DataType, t_MemBits>& p_colVec) {
        unsigned long long l_vecSz = p_entries * sizeof(t_DataType);
        void* l_valAddr = p_program.allocMem(l_vecSz);
        if (l_valAddr == nullptr) {
            return false;
        }
        p_colVec.getEntries() = p_entries;
        p_colVec.setValAddr(reinterpret_cast<uint8_t*>(l_valAddr));
        return true;
    }
};

template <typename t_DataType,
          typename t_IndexType,
          unsigned int t_ParEntries,
          unsigned int t_ParGroups,
          unsigned int t_NnzRowIdxMemBits,
          unsigned int t_ColVecMemBits,
          unsigned int t_MaxColMemBlocks,
          unsigned int t_MaxRowBlocks,
          unsigned int t_HbmChannels,
          unsigned int t_HbmChannelMegaBytes,
          unsigned int t_PageSize>
class GenCscPartition {
   public:
    typedef Program<t_PageSize> t_ProgramType;
    typedef NnzUnit<t_DataType, t_IndexType> t_NnzUnitType;
    typedef MtxFile<t_DataType, t_IndexType> t_MtxFileType;
    typedef ColVec<t_DataType, t_ColVecMemBits> t_ColVecType;
    typedef CscMat<t_DataType, t_IndexType, t_ParEntries, t_NnzRowIdxMemBits, t_ColVecMemBits> t_CscMatType;

    typedef CscPartition<t_DataType, t_IndexType, t_ParEntries, t_NnzRowIdxMemBits, t_ColVecMemBits, t_HbmChannels>
        t_CscPartitionType;

    typedef GenCscMat<t_DataType,
                      t_IndexType,
                      t_ParEntries,
                      t_ParGroups,
                      t_NnzRowIdxMemBits,
                      t_ColVecMemBits,
                      t_PageSize>
        t_GenCscMatType;

    typedef GenVec<t_DataType, t_ParEntries, t_ColVecMemBits, t_PageSize> t_GenVecType;

    const unsigned int t_HbmChannelBytes = t_HbmChannelMegaBytes * 1024 * 1024;
    const unsigned int t_MaxRowsPerKernel = t_MaxRowBlocks * t_ParEntries * t_ParGroups;
    const unsigned int t_colVecWords = t_ColVecMemBits / (8 * sizeof(t_DataType));
    const unsigned int t_MaxColsPerKernel = t_MaxColMemBlocks * t_colVecWords;
    const unsigned int t_MaxNnzsPerKernel = t_HbmChannelBytes / sizeof(t_DataType) / 2;

   public:
    GenCscPartition() {}

    bool genPartition(unsigned int p_rows, // total rows
                      unsigned int p_cols, // rotal cols
                      unsigned int p_rowsPerKernel[t_HbmChannels],
                      unsigned int p_rowIdxBase[t_HbmChannels],
                      unsigned int& p_colsPerKernel,
                      unsigned int& p_colIdxBase,
                      vector<t_NnzUnitType>& p_nnzUnits,
                      vector<t_DataType>& p_colVecEnt,
                      t_ProgramType& p_program,
                      t_CscPartitionType& p_partition) {
        // assume p_nnzUnits are sorted along columns already
        bool l_res = true;
        vector<t_NnzUnitType> l_nnzSets[t_HbmChannels];
        // partition cols
        unsigned int l_cols = 0;
        unsigned int l_lastCols = p_colIdxBase + p_colsPerKernel;
        if ((p_cols > l_lastCols) && (l_lastCols != 0)) {
            l_cols = p_cols - l_lastCols;
            p_colIdxBase += p_colsPerKernel;
            if (l_cols > t_MaxColsPerKernel) {
                l_cols = t_MaxColsPerKernel;
            }
            p_colsPerKernel = l_cols;
        } else {
            p_colIdxBase = 0;
            p_colsPerKernel = (p_cols < t_MaxColsPerKernel) ? p_cols : t_MaxColsPerKernel;
        }
        // partition rows
        if (p_colIdxBase == 0) {
            unsigned int l_lastRows = p_rowIdxBase[0] + p_rowsPerKernel[0];
            for (unsigned int i = 1; i < t_HbmChannels; ++i) {
                assert(l_lastRows == p_rowIdxBase[i]);
                l_lastRows += p_rowsPerKernel[i];
            }
            if (p_rows > l_lastRows) {
                unsigned int l_restRows = p_rows - l_lastRows;
                unsigned int l_rows = l_restRows / t_HbmChannels;
                for (unsigned int i = 0; i < t_HbmChannels; ++i) {
                    if (l_rows > t_MaxRowsPerKernel) {
                        p_rowIdxBase[i] = l_lastRows;
                        p_rowsPerKernel[i] = t_MaxRowsPerKernel;
                        l_lastRows += t_MaxRowsPerKernel;
                    } else {
                        p_rowIdxBase[i] = l_lastRows;
                        p_rowsPerKernel[i] = (l_rows != 0) ? l_rows : l_restRows;
                        l_lastRows += p_rowsPerKernel[i];
                        l_restRows -= p_rowsPerKernel[i];
                    }
                }
            } else {
                l_res = false;
            }
        }

        // partition NnzUnits
        for (unsigned int i = 0; i < t_HbmChannels; ++i) {
            typename vector<t_NnzUnitType>::iterator l_nnzIt;
            l_nnzIt = p_nnzUnits.begin();
            while (l_nnzIt != p_nnzUnits.end()) {
                unsigned int l_row = (*l_nnzIt).getRow();
                unsigned int l_col = (*l_nnzIt).getCol();
                bool l_isValid = (l_col >= p_colIdxBase) && (l_col < p_colIdxBase + p_colsPerKernel) &&
                                 (l_row >= p_rowIdxBase[i]) && (l_row < p_rowIdxBase[i] + p_rowsPerKernel[i]);
                if (l_isValid && l_nnzSets[i].size() < t_MaxNnzsPerKernel) {
                    l_nnzSets[i].push_back(*l_nnzIt);
                    l_nnzIt = p_nnzUnits.erase(l_nnzIt);
                } else {
                    ++l_nnzIt;
                }
            }
        }
        vector<t_DataType> l_colVecEnt;
        for (unsigned int i = 0; i < p_colsPerKernel; ++i) {
            l_colVecEnt.push_back(p_colVecEnt[p_colIdxBase + i]);
        }
        // allocate memory
        t_GenCscMatType l_genCscMat;
        t_GenVecType l_genColVec;
        for (unsigned int i = 0; i < t_HbmChannels; ++i) {
            bool l_genCscRes;
            l_genCscRes =
                l_genCscMat.genCscMatFromNnzs(l_nnzSets[i].size(), l_nnzSets[i], p_program, p_partition.getMat(i));
            l_res = l_res && l_genCscRes;
        }
        l_genColVec.genColVecFromEnt(l_colVecEnt, p_program, p_partition.getVec());
        return l_res;
    }
};

} // end namespace sparse
} // end namespace xf
#endif
