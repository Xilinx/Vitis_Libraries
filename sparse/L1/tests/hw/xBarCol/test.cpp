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
#include <cstdlib>
#include <vector>
#include <ctime>
#include "xf_sparse.hpp"

#define NnzBlocks 16
#define ColBlocks 4

using namespace xf::sparse;
using namespace std;

void uut_top(unsigned int p_colPtrBlocks,
             unsigned int p_nnzBlocks,
             hls::stream<ap_uint<(SPARSE_indexBits * (1 << SPARSE_logParEntries))> >& p_colPtrStr,
             hls::stream<ap_uint<(SPARSE_dataBits * (1 << SPARSE_logParEntries))> >& p_colValStr,
             hls::stream<ap_uint<(SPARSE_dataBits * (1 << SPARSE_logParEntries))> >& p_nnzColValStr) {
    xBarCol<SPARSE_logParEntries, SPARSE_dataType, SPARSE_indexType, SPARSE_dataBits, SPARSE_indexBits>(
        p_colPtrBlocks, p_nnzBlocks, p_colPtrStr, p_colValStr, p_nnzColValStr);
}

int main() {
    hls::stream<ap_uint<SPARSE_indexBits * SPARSE_parEntries> > l_colPtrStr;
    hls::stream<ap_uint<SPARSE_dataBits * SPARSE_parEntries> > l_colValStr;
    hls::stream<ap_uint<SPARSE_dataBits * SPARSE_parEntries> > l_nnzColValStr;
#pragma DATA_PACK variable = l_colPtrStr
#pragma DATA_PACK variable = l_colSelStr

    WideType<SPARSE_indexType, SPARSE_parEntries> l_colPtr;
    WideType<SPARSE_dataType, SPARSE_parEntries> l_colVal;
    ap_uint<SPARSE_indexBits * SPARSE_parEntries> l_colPtrBits;
    ap_uint<SPARSE_dataBits * SPARSE_parEntries> l_colValBits;

    vector<SPARSE_dataType> l_nnzColValGolden;

    unsigned int l_nnzInputs = 0;
    cout << "Input:" << endl;
    // srand(time(nullptr));
    for (unsigned int i = 0; i < ColBlocks; ++i) {
        for (unsigned int j = 0; j < SPARSE_parEntries; ++j) {
            l_colVal[j] = (SPARSE_dataType)(i * SPARSE_parEntries + j);
            SPARSE_indexType l_nnzsInCol = ((NnzBlocks * SPARSE_parEntries - l_nnzInputs) == 0)
                                               ? 0
                                               : rand() % (NnzBlocks * SPARSE_parEntries - l_nnzInputs);
            l_colPtr[j] =
                ((i == ColBlocks - 1) && (j == SPARSE_parEntries - 1) && (l_nnzInputs < NnzBlocks * SPARSE_parEntries))
                    ? NnzBlocks * SPARSE_parEntries
                    : l_nnzInputs;
            if ((i == ColBlocks - 1) && (j == SPARSE_parEntries - 1) && (l_nnzInputs < NnzBlocks * SPARSE_parEntries)) {
                l_colPtr[j] = NnzBlocks * SPARSE_parEntries;
                l_nnzsInCol = NnzBlocks * SPARSE_parEntries - l_nnzInputs;
            } else {
                l_colPtr[j] = l_nnzInputs + l_nnzsInCol;
            }
            l_nnzInputs += l_nnzsInCol;
            for (unsigned int k = 0; k < l_nnzsInCol; ++k) {
                l_nnzColValGolden.push_back(l_colVal[j]);
            }
        }
        l_colPtrBits = l_colPtr;
        l_colValBits = l_colVal;
        l_colPtrStr.write(l_colPtrBits);
        l_colValStr.write(l_colValBits);
        cout << "colPtr " << i << ": " << l_colPtr << endl;
    }

    assert(l_nnzColValGolden.size() == NnzBlocks * SPARSE_parEntries);
    cout << "Golden reference:";
    for (unsigned int i = 0; i < l_nnzColValGolden.size(); ++i) {
        if ((i % SPARSE_parEntries) == 0) {
            cout << endl;
        }
        cout << setw(SPARSE_printWidth) << l_nnzColValGolden[i];
    }
    cout << endl;

    uut_top(ColBlocks, NnzBlocks, l_colPtrStr, l_colValStr, l_nnzColValStr);

    unsigned int l_errors = 0;
    for (unsigned int b = 0; b < NnzBlocks; ++b) {
        ap_uint<SPARSE_dataBits * SPARSE_parEntries> l_nnzColValBits;
        l_nnzColValBits = l_nnzColValStr.read();
        WideType<SPARSE_dataType, SPARSE_parEntries> l_nnzColVal(l_nnzColValBits);
        cout << "Output " << b << " : " << l_nnzColVal << endl;
        for (unsigned int i = 0; i < SPARSE_parEntries; ++i) {
            if (l_nnzColVal[i] != l_nnzColValGolden[b * SPARSE_parEntries + i]) {
                cout << "ERROR: "
                     << "block=" << b << " id=" << i;
                cout << " refVal=" << l_nnzColValGolden[b * SPARSE_parEntries + i];
                cout << " outVal=" << l_nnzColVal[i] << endl;
                l_errors++;
            }
        }
    }
    if (l_errors != 0) {
        return -1;
    } else {
        return 0;
    }
}
