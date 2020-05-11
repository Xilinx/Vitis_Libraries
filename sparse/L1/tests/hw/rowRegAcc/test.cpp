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

#define NnzGroupEntries 18
#define MaxRowOffsets 2048
#define MaxNumSameRows 10
using namespace xf::sparse;
using namespace std;

template <typename T>
bool isClose(float p_tolRel, float p_tolAbs, T p_vRef, T p_v, bool& p_exactMatch) {
    float l_diffAbs = abs(p_v - p_vRef);
    p_exactMatch = (p_vRef == p_v);
    bool l_status = (l_diffAbs <= (p_tolAbs + p_tolRel * abs(p_vRef)));
    return (l_status);
}
template <typename T>
bool compare(T x, T ref) {
    return x == ref;
}

template <>
bool compare<double>(double x, double ref) {
    bool l_exactMatch;
    return isClose<float>(1e-3, 3e-6, x, ref, l_exactMatch);
}
template <>
bool compare<float>(float x, float ref) {
    bool l_exactMatch;
    return isClose<float>(1e-3, 3e-6, x, ref, l_exactMatch);
}

void uut_top(hls::stream<ap_uint<SPARSE_dataBits + SPARSE_indexBits - SPARSE_logParEntries - SPARSE_logParGroups> >&
                 p_rowEntryStr,
             hls::stream<ap_uint<1> >& p_isEndStr,
             hls::stream<ap_uint<SPARSE_dataBits + SPARSE_indexBits - SPARSE_logParEntries - SPARSE_logParGroups> >&
                 p_rowRegAccStr,
             hls::stream<ap_uint<1> >& p_isEndOutStr) {
    rowRegAcc<SPARSE_logParGroups, SPARSE_dataType, SPARSE_indexType, SPARSE_dataBits,
              SPARSE_indexBits - SPARSE_logParEntries - SPARSE_logParGroups>(p_rowEntryStr, p_isEndStr, p_rowRegAccStr,
                                                                             p_isEndOutStr);
}

int main() {
    static const unsigned int t_RowOffsetBits = SPARSE_indexBits - SPARSE_logParEntries - SPARSE_logParGroups;

    hls::stream<ap_uint<SPARSE_dataBits + t_RowOffsetBits> > l_rowEntryStr;
    hls::stream<ap_uint<1> > l_isEndStr;
    unordered_map<SPARSE_indexType, SPARSE_dataType> l_goldenArr;
    unordered_map<SPARSE_indexType, SPARSE_dataType>::iterator l_goldenArrIt;

    // srand(time(nullptr));
    cout << "Inputs:"
         << "  "
         << "row offset"
         << "val" << endl;
    unsigned int l_nnzRows = 0;
    unsigned int l_nnzGroupEntries = 0;
    unsigned int l_numSameRows = 0;
    unsigned int l_maxNumSameRows = (rand() % MaxNumSameRows) + 1;
    SPARSE_indexType l_row;
    l_row = rand() % MaxRowOffsets;
    while (l_nnzGroupEntries < NnzGroupEntries) {
        RowEntry<SPARSE_dataType, SPARSE_indexType, SPARSE_dataBits, t_RowOffsetBits> l_rowEntry;
        l_rowEntry.getVal() = l_nnzGroupEntries;
        if (l_numSameRows < l_maxNumSameRows) {
            l_rowEntry.getRow() = l_row;
            l_rowEntryStr.write(l_rowEntry.toBits());
            cout << "Nnz " << l_nnzGroupEntries << ": " << l_rowEntry << endl;
            if (l_goldenArr.find(l_rowEntry.getRow()) == l_goldenArr.end()) {
                l_goldenArr[l_rowEntry.getRow()] = l_rowEntry.getVal();
                l_nnzRows++;
            } else {
                l_goldenArr[l_rowEntry.getRow()] += l_rowEntry.getVal();
            }
            l_numSameRows++;
            l_nnzGroupEntries++;
        } else {
            l_numSameRows = 0;
            SPARSE_indexType l_rowTmp = rand() % MaxRowOffsets;
            while (l_rowTmp == l_row) {
                l_rowTmp = rand() % MaxRowOffsets;
            }
            l_row = l_rowTmp;
            l_maxNumSameRows = (rand() % MaxNumSameRows) + 1;
        }
    }
    l_isEndStr.write(1);

    l_goldenArrIt = l_goldenArr.begin();
    unsigned int l_goldenIndex = 0;
    while (l_goldenArrIt != l_goldenArr.end()) {
        cout << "Golden res " << l_goldenIndex << ": " << setw(SPARSE_printWidth) << l_goldenArrIt->first
             << setw(SPARSE_printWidth) << l_goldenArrIt->second << endl;
        l_goldenArrIt++;
        l_goldenIndex++;
    }

    hls::stream<ap_uint<SPARSE_dataBits + t_RowOffsetBits> > l_rowRegAccStr;
    hls::stream<ap_uint<1> > l_isEndOutStr;

    uut_top(l_rowEntryStr, l_isEndStr, l_rowRegAccStr, l_isEndOutStr);

    ap_uint<1> l_isEnd = 0;
    ap_uint<1> l_activity = 1;
    ap_uint<1> l_exit = 0;

    unsigned int l_nnzRowsOut = 0;
    unsigned int l_errors = 0;
    while (!l_exit) {
        if (l_isEnd && !l_activity) {
            l_exit = true;
        }
        l_activity = 0;
        ap_uint<1> l_unused;
        if (l_isEndOutStr.read_nb(l_unused)) {
            l_isEnd = 1;
        }
        RowEntry<SPARSE_dataType, SPARSE_indexType, SPARSE_dataBits, t_RowOffsetBits> l_rowEntry;
        ap_uint<SPARSE_dataBits + t_RowOffsetBits> l_rowEntryBits;
        if (l_rowRegAccStr.read_nb(l_rowEntryBits)) {
            l_rowEntry.toVal(l_rowEntryBits);
            l_activity = 1;
            cout << "Output " << l_nnzRowsOut << ": " << l_rowEntry;
            if ((l_goldenArr.find(l_rowEntry.getRow()) == l_goldenArr.end()) &&
                (!compare<SPARSE_dataType>(l_rowEntry.getVal(), 0))) {
                cout << "ERROR: Row offset " << l_rowEntry.getRow() << "doesn't exist!" << endl;
                l_errors++;
            } else {
                l_goldenArr[l_rowEntry.getRow()] -= l_rowEntry.getVal();
            }
            cout << endl;
            l_nnzRowsOut++;
        }
    }

    l_goldenArrIt = l_goldenArr.begin();
    while (l_goldenArrIt != l_goldenArr.end()) {
        if (!compare<SPARSE_dataType>(l_goldenArrIt->second, 0)) {
            cout << "ERROR: Row offset " << l_goldenArrIt->first << "has accumulation error." << endl;
            l_errors++;
        }
        l_goldenArrIt++;
    }

    cout << "total output nnzRows: " << l_nnzRowsOut << endl;
    cout << "total errors: " << l_errors << endl;
    if (l_errors == 0) {
        return 0;
    } else {
        return -1;
    }
}
