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

using namespace xf::sparse;
using namespace xf::blas;
using namespace std;

int main() {
    const unsigned int t_MaxCols = SPARSE_parEntries * SPARSE_maxColParBlocks;

    hls::stream<ap_uint<32> > l_paramStr;
    hls::stream<ap_uint<SPARSE_dataBits * SPARSE_parEntries> > l_datStr;
    hls::stream<ap_uint<32> > l_paramOutStr[SPARSE_hbmChannels];
    hls::stream<ap_uint<SPARSE_dataBits * SPARSE_parEntries> > l_datOutStr[SPARSE_hbmChannels];

    SPARSE_dataType l_vecStore[t_MaxCols];
    for (unsigned int i = 0; i < t_MaxCols; ++i) {
        l_vecStore[i] = (SPARSE_dataType)i;
    }

    ap_uint<32> l_vecBlocks;
    ap_uint<32> l_chBlocks[SPARSE_hbmChannels];
    ap_uint<32> l_minIdx[SPARSE_hbmChannels];
    ap_uint<32> l_maxIdx[SPARSE_hbmChannels];

    // srand(time(nullptr));
    do {
        l_vecBlocks = (ap_uint<32>)(rand() % SPARSE_maxColParBlocks);
    } while (l_vecBlocks == 0);

    unsigned int l_entries = l_vecBlocks * SPARSE_parEntries;
    for (unsigned int i = 0; i < SPARSE_hbmChannels; ++i) {
        do {
            l_chBlocks[i] = (ap_uint<32>)(rand() % l_vecBlocks);
        } while (l_chBlocks[i] == 0);
        ap_uint<32> l_chEntries = l_chBlocks[i] * SPARSE_parEntries;
        l_minIdx[i] = (ap_uint<32>)((rand() % l_entries));
        ap_uint<32> l_dist2End = (l_entries - l_minIdx[i] - 1);
        ap_uint<32> l_dist = (l_dist2End > l_chEntries) ? l_chEntries : l_dist2End;
        l_maxIdx[i] = l_minIdx[i] + (rand() % l_dist);
        if (l_maxIdx[i] >= t_MaxCols) {
            l_maxIdx[i] = t_MaxCols - 1;
        }
    }

    cout << "Inputs:" << endl;
    cout << "vecBlocks: " << l_vecBlocks << endl;
    for (unsigned int i = 0; i < SPARSE_hbmChannels; ++i) {
        cout << "Channel " << i << endl;
        cout << "    chBlocks:  " << l_chBlocks[i] << endl;
        cout << "    minIdx:    " << l_minIdx[i] << endl;
        cout << "    maxIdx:    " << l_maxIdx[i] << endl;
    }

    // send inputs to the stream
    l_paramStr.write(l_vecBlocks);
    for (unsigned int i = 0; i < SPARSE_hbmChannels; ++i) {
        l_paramStr.write(l_chBlocks[i]);
        l_paramStr.write(l_minIdx[i]);
        l_paramStr.write(l_maxIdx[i]);
    }
    for (unsigned int i = 0; i < l_vecBlocks; ++i) {
        ap_uint<SPARSE_dataBits * SPARSE_parEntries> l_valVecBits;
        WideType<ap_uint<SPARSE_dataBits>, SPARSE_parEntries> l_valVec;
        for (unsigned int j = 0; j < SPARSE_parEntries; ++j) {
            l_valVec[j] = l_vecStore[i * SPARSE_parEntries + j];
        }
        l_valVecBits = l_valVec;
        l_datStr.write(l_valVecBits);
    }

    uut_top(l_paramStr, l_datStr, l_paramOutStr, l_datOutStr);

    unsigned int l_errs = 0;
    for (unsigned int t_chId = 0; t_chId < SPARSE_hbmChannels; ++t_chId) {
        cout << "INFO: channel id = " << t_chId << endl;
        unsigned int l_blocks = l_chBlocks[t_chId];
        unsigned int l_outBlocks = l_paramOutStr[t_chId].read();
        if (l_blocks != l_outBlocks) {
            cout << "ERROR: l_outBlocks " << l_outBlocks << " != " << l_blocks << "(original value)" << endl;
            l_errs++;
        }
        for (unsigned int i = 0; i < l_blocks; ++i) {
            ap_uint<SPARSE_dataBits * SPARSE_parEntries> l_vecOutBits;
            l_vecOutBits = l_datOutStr[t_chId].read();
            WideType<ap_uint<SPARSE_dataBits>, SPARSE_parEntries> l_vecOut(l_vecOutBits);
            for (unsigned int j = 0; j < SPARSE_parEntries; ++j) {
                unsigned int l_idx = l_minIdx[t_chId] + i * SPARSE_parEntries + j;
                if (l_idx <= l_maxIdx[t_chId]) {
                    if (l_vecStore[l_idx] != l_vecOut[j]) {
                        cout << "ERROR: l_valOut at index " << l_idx << endl;
                        cout << "       output is: " << l_vecOut[j] << " original value is: " << l_vecStore[l_idx]
                             << endl;
                        l_errs++;
                    }
                }
            }
        }
    }
    if (l_errs == 0) {
        cout << "TEST PASS!" << endl;
        return 0;
    } else {
        cout << "total errors: " << l_errs << endl;
        return -1;
    }
}
