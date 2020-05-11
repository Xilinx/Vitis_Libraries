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
#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>
#include <ctime>
#include "uut_top.hpp"
#include "L1_utils.hpp"

#define NnzBlocks 4
using namespace xf::sparse;
using namespace xf::blas;
using namespace std;

int main() {
    const unsigned int t_MaxRows = SPARSE_parEntries * SPARSE_parGroups * SPARSE_maxRowBlocks;
    const unsigned int t_RowBlockSize = SPARSE_parGroups * SPARSE_parEntries;

    hls::stream<ap_uint<SPARSE_dataBits * SPARSE_parEntries> > l_nnzValStr;
    hls::stream<ap_uint<SPARSE_dataBits * SPARSE_parEntries> > l_nnzColValStr;
    hls::stream<ap_uint<SPARSE_indexBits * SPARSE_parEntries> > l_rowIndexStr;
    hls::stream<ap_uint<SPARSE_dataBits * SPARSE_parEntries> > l_rowAggStr;

    WideType<SPARSE_dataType, SPARSE_parEntries> l_nnzVal;
    WideType<SPARSE_dataType, SPARSE_parEntries> l_nnzColVal;
    WideType<SPARSE_indexType, SPARSE_parEntries> l_rowIndex;

    ap_uint<SPARSE_dataBits * SPARSE_parEntries> l_nnzValBits;
    ap_uint<SPARSE_dataBits * SPARSE_parEntries> l_nnzColValBits;
    ap_uint<SPARSE_dataBits * SPARSE_parEntries> l_rowIndexBits;

    SPARSE_dataType l_rowStore[t_MaxRows];
    for (unsigned int i = 0; i < t_MaxRows; ++i) {
        l_rowStore[i] = (SPARSE_dataType)0;
    }

    SPARSE_indexType l_maxRow = 0;
    // srand(time(nullptr));
    cout << "Inputs:" << endl;
    for (unsigned int i = 0; i < NnzBlocks; ++i) {
        for (unsigned int b = 0; b < SPARSE_parEntries; ++b) {
            l_nnzVal[b] = i * SPARSE_parEntries + b;
            l_nnzColVal[b] = l_nnzVal[b] + 1;
            SPARSE_indexType l_rowIdx = rand() % t_MaxRows;
            l_rowIndex[b] = l_rowIdx;
            l_maxRow = (l_maxRow < l_rowIdx) ? l_rowIdx : l_maxRow;
            l_rowStore[l_rowIdx] += l_nnzVal[b] * l_nnzColVal[b];
        }
        l_nnzValBits = l_nnzVal;
        l_nnzColValBits = l_nnzColVal;
        l_rowIndexBits = l_rowIndex;
        l_nnzValStr.write(l_nnzValBits);
        l_nnzColValStr.write(l_nnzColValBits);
        l_rowIndexStr.write(l_rowIndexBits);
        cout << "Nnz Block " << i << endl;
        cout << "    Nnz vals:        " << l_nnzVal << endl;
        cout << "    Nnz colVals:     " << l_nnzColVal << endl;
        cout << "    Nnz Row Indices: " << l_rowIndex << endl;
    }

    unsigned int l_rowBlocks = (l_maxRow + t_RowBlockSize - 1) / t_RowBlockSize;
    uut_top(NnzBlocks, l_rowBlocks, l_nnzValStr, l_nnzColValStr, l_rowIndexStr, l_rowAggStr);

    unsigned int l_errors = 0;

    for (unsigned int i = 0; i < (l_rowBlocks * SPARSE_parGroups); ++i) {
        ap_uint<SPARSE_dataBits * SPARSE_parEntries> l_rowOutBits;
        l_rowOutBits = l_rowAggStr.read();
        WideType<SPARSE_dataType, SPARSE_parEntries> l_rowVal(l_rowOutBits);
        for (unsigned int j = 0; j < SPARSE_parEntries; ++j) {
            unsigned int l_rowIdx = i * SPARSE_parEntries + j;
            SPARSE_dataType l_refVal = l_rowStore[l_rowIdx];
            if (!compare<SPARSE_dataType>(l_refVal, l_rowVal[j])) {
                l_errors++;
                cout << "ERROR: row " << l_rowIdx << " has error! ";
                cout << "      refRow[" << l_rowIdx << "] = " << l_refVal;
                cout << " outVal[" << l_rowIdx << "] = " << l_rowVal[j] << endl;
            }
        }
    }
    if (!l_rowAggStr.empty()) {
        cout << "ERROR: l_rowAggStr not empty" << endl;
    }
    cout << "total row blocks: " << l_rowBlocks << endl;
    cout << "total errors: " << l_errors << endl;
    if (l_errors == 0) {
        return 0;
    } else {
        return -1;
    }
}
