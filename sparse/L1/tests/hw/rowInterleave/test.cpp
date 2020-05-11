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
#include <unordered_map>
#include "xf_sparse.hpp"

#define NnzBankId 0
#define NnzBankEntries 18
#define MaxRows 2048
using namespace xf::sparse;
using namespace std;

void uut_top(hls::stream<ap_uint<SPARSE_dataBits + SPARSE_indexBits> >& p_rowEntryStr,
             hls::stream<ap_uint<1> >& p_isEndStr,
             hls::stream<ap_uint<SPARSE_dataBits + SPARSE_indexBits - SPARSE_logParEntries - SPARSE_logParGroups> >
                 p_rowInterleaveStr[SPARSE_parGroups],
             hls::stream<ap_uint<1> > p_isEndOutStr[SPARSE_parGroups]) {
    rowInterleave<SPARSE_logParEntries, SPARSE_logParGroups, SPARSE_dataType, SPARSE_indexType, SPARSE_dataBits,
                  SPARSE_indexBits>(p_rowEntryStr, p_isEndStr, p_rowInterleaveStr, p_isEndOutStr);
}

int main() {
    static const unsigned int RowOffsetBits = SPARSE_indexBits - SPARSE_logParEntries - SPARSE_logParGroups;

    hls::stream<ap_uint<SPARSE_dataBits + SPARSE_indexBits> > l_rowEntryStr;
    hls::stream<ap_uint<1> > l_isEndStr;
    unordered_map<SPARSE_dataType, SPARSE_indexType> l_inputArr;

    // srand(time(nullptr));
    cout << "Inputs:"
         << "  "
         << "row Index "
         << "val    "
         << "group" << endl;
    for (unsigned int i = 0; i < NnzBankEntries; ++i) {
        RowEntry<SPARSE_dataType, SPARSE_indexType> l_rowEntry;
        l_rowEntry.getVal() = i;
        SPARSE_indexType l_row;
        do {
            l_row = rand() % MaxRows;
        } while ((l_row % SPARSE_parEntries) != NnzBankId);
        l_rowEntry.getRow() = l_row;
        l_rowEntryStr.write(l_rowEntry.toBits());
        cout << "Nnz " << i << ": " << l_rowEntry << setw(SPARSE_printWidth)
             << (l_rowEntry.getRow() / SPARSE_parEntries) % SPARSE_parGroups << endl;
        l_inputArr[l_rowEntry.getVal()] = l_rowEntry.getRow();
    }
    l_isEndStr.write(1);

    hls::stream<ap_uint<SPARSE_dataBits + RowOffsetBits> > l_rowInterleaveStr[SPARSE_parGroups];
    hls::stream<ap_uint<1> > l_isEndOutStr[SPARSE_parGroups];

    uut_top(l_rowEntryStr, l_isEndStr, l_rowInterleaveStr, l_isEndOutStr);

    ap_uint<SPARSE_parGroups> l_isEnd = 0;
    ap_uint<SPARSE_parGroups> l_activity = 0;
    l_activity.b_not();
    ap_uint<1> l_exit = 0;

    unsigned int l_nnzs = 0;
    unsigned int l_errors = 0;
    while (!l_exit) {
        if (l_isEnd.and_reduce() && !l_activity.or_reduce()) {
            l_exit = true;
        }
        l_activity = 0;
        for (unsigned int g = 0; g < SPARSE_parGroups; ++g) {
            ap_uint<1> l_unused;
            if (l_isEndOutStr[g].read_nb(l_unused)) {
                l_isEnd[g] = 1;
            }
        }
        for (unsigned int g = 0; g < SPARSE_parGroups; ++g) {
            RowEntry<SPARSE_dataType, SPARSE_indexType, SPARSE_dataBits, RowOffsetBits> l_rowEntry;
            ap_uint<SPARSE_dataBits + RowOffsetBits> l_rowEntryBits;
            if (l_rowInterleaveStr[g].read_nb(l_rowEntryBits)) {
                l_rowEntry.toVal(l_rowEntryBits);
                l_activity[g] = 1;
                SPARSE_indexType l_row =
                    l_rowEntry.getRow() * (SPARSE_parEntries * SPARSE_parGroups) + g * SPARSE_parEntries + NnzBankId;

                cout << "Group " << g << ": " << l_rowEntry << setw(SPARSE_printWidth) << l_row;
                if (l_inputArr.find(l_rowEntry.getVal()) == l_inputArr.end()) {
                    cout << "ERROR: Val " << l_rowEntry.getVal() << "doesn't exist!" << endl;
                    l_errors++;
                } else if (l_inputArr[l_rowEntry.getVal()] != l_row) {
                    cout << " ERROR: "
                         << "row index = " << setw(SPARSE_printWidth) << l_row << endl;
                    l_errors++;
                }
                cout << endl;
                l_nnzs++;
            }
        }
    }

    cout << "totoal nnzs: " << l_nnzs << endl;
    cout << "totoal errors: " << l_errors << endl;
    if (l_errors == 0) {
        return 0;
    } else {
        return -1;
    }
}
