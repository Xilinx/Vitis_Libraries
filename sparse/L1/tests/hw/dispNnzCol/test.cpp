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

    ap_uint<32> l_chBlocks[SPARSE_hbmChannels];

    // srand(time(nullptr));
    for (unsigned int i = 0; i < SPARSE_hbmChannels; ++i) {
        do {
            l_chBlocks[i] = (ap_uint<32>)(rand() % SPARSE_maxColParBlocks);
        } while (l_chBlocks[i] == 0);
    }

    cout << "Inputs:" << endl;
    for (unsigned int i = 0; i < SPARSE_hbmChannels; ++i) {
        cout << "Channel " << i << endl;
        cout << "    chBlocks:  " << l_chBlocks[i] << endl;
    }

    // send inputs to the stream
    for (unsigned int i = 0; i < SPARSE_hbmChannels; ++i) {
        l_paramStr.write(l_chBlocks[i]);
    }
    for (unsigned int ch = 0; ch < SPARSE_hbmChannels; ++ch) {
        unsigned int l_blocks = l_chBlocks[ch];
        for (unsigned int i = 0; i < l_blocks; ++i) {
            ap_uint<SPARSE_dataBits * SPARSE_parEntries> l_valVecBits;
            WideType<ap_uint<SPARSE_dataBits>, SPARSE_parEntries> l_valVec;
            for (unsigned int j = 0; j < SPARSE_parEntries; ++j) {
                l_valVec[j] = (ap_uint<SPARSE_dataBits>)(ch + i * SPARSE_parEntries + j);
            }
            l_valVecBits = l_valVec;
            l_datStr.write(l_valVecBits);
        }
    }

    uut_top(l_paramStr, l_datStr, l_paramOutStr, l_datOutStr);

    unsigned int l_errs = 0;
    for (unsigned int t_chId = 0; t_chId < SPARSE_hbmChannels; ++t_chId) {
        cout << "INFO: channel id = " << t_chId << endl;
        ap_uint<32> l_outBlocks = l_paramOutStr[t_chId].read();
        if (l_outBlocks != l_chBlocks[t_chId]) {
            cout << "ERROR: output blocks " << l_outBlocks;
            cout << " != original blocks" << l_chBlocks[t_chId] << endl;
            l_errs++;
        }
        for (unsigned int i = 0; i < l_chBlocks[t_chId]; ++i) {
            ap_uint<SPARSE_dataBits * SPARSE_parEntries> l_vecOutBits;
            l_vecOutBits = l_datOutStr[t_chId].read();
            WideType<ap_uint<SPARSE_dataBits>, SPARSE_parEntries> l_vecOut(l_vecOutBits);
            for (unsigned int j = 0; j < SPARSE_parEntries; ++j) {
                unsigned int l_idx = t_chId + i * SPARSE_parEntries + j;
                ap_uint<SPARSE_dataBits> l_valRef = l_idx;
                if (l_valRef != l_vecOut[j]) {
                    cout << "ERROR: l_valOut at index " << l_idx << endl;
                    cout << "       output is: " << l_vecOut[j] << " original value is: " << l_valRef << endl;
                    l_errs++;
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
