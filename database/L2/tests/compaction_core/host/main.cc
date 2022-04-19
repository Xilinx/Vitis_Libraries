/*
 * Copyright 2022 Xilinx, Inc.
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

#include "xf_database/compaction_core/core.h"
#include "xf_database/compaction_core/config.h"
#include <iostream>
#include <stdlib.h>
#include <vector>
#include <string>
#include <iterator>
#include <algorithm>
#include <ap_int.h>
#include <unistd.h>
#include <fstream>

enum DATASIZE {
    sz_1k = 1024,
    sz_64k = 64 * 1024,
    sz_128k = 128 * 1024,
    sz_1m = 1024 * 1024,
    sz_8m = 8 * 1024 * 1024,
    sz_64m = 64 * 1024 * 1024,
    sz_128m = 128 * 1024 * 1024,
    sz_256m = 256 * 1024 * 1024,
    sz_512m = 512 * 1024 * 1024,
    sz_1g = 1024 * 1024 * 1024,
    sz_test = sz_256m,
    sz_test_ch = sz_test / sz_64m
};

extern "C" void CUCoreLoopTop(unsigned char in_streamID_tag,
                              ap_uint<BufWidth_Set_>* in_dataBuff,
                              ap_uint<32>* in_metaInfo,

                              unsigned char base_streamID_tag,
                              ap_uint<BufWidth_Set_>* base_dataBuff,
                              ap_uint<32>* base_metaInfo,

                              ap_uint<BufWidth_Set_>* out_dataBuff,
                              ap_uint<32>* out_metaInfo,

                              unsigned int in_dataBurst_len,
                              unsigned int in_metaBurst_len,
                              unsigned int base_dataBurst_len,
                              unsigned int base_metaBurst_len);

std::string gen_random(const int len) {
    std::string tmp_s;
    static const char alphanum[] =
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";

    // srand( (unsigned) time(NULL) * getpid());
    // srand(seed);

    tmp_s.reserve(len);

    for (int i = 0; i < len; ++i) tmp_s += alphanum[rand() % (sizeof(alphanum) - 1)];

    return tmp_s;
}

int main(int argc, char* argv[]) {
    static const int datapath_width = BufWidth_Set_;
    static const int datapath_bytes = datapath_width / 8;

    char buf[500];
    std::string prefix_(argv[1]);
    std::vector<std::string> l1;
    std::ifstream if1;
    std::string f1 = prefix_ + "/input1.txt";
    std::cout << f1 << std::endl;
    if1.open(f1.c_str(), std::ifstream::in);
    if (!if1.good()) std::cout << "Fail open input1.txt" << std::endl;
    while (!if1.eof()) {
        if1.getline(buf, 500);
        if (strlen(buf) > 0) l1.emplace_back(std::string(buf));
    }
    if1.close();
    unsigned int byte_cnt1 = 0;
    for (auto it : l1) {
        byte_cnt1 += it.length();
    }

    std::vector<std::string> l2;
    std::ifstream if2;
    std::string f2 = prefix_ + "/input2.txt";
    if2.open(f2.c_str(), std::ifstream::in);
    while (!if2.eof()) {
        if2.getline(buf, 500);
        if (strlen(buf) > 0) l2.emplace_back(std::string(buf));
    }
    if2.close();
    unsigned int byte_cnt2 = 0;
    for (auto it : l2) {
        byte_cnt2 += it.length();
    }

    std::vector<std::string> l3;
    std::ifstream if3;
    std::string f3 = prefix_ + "/input3.txt";
    if3.open(f3.c_str(), std::ifstream::in);
    while (!if3.eof()) {
        if3.getline(buf, 500);
        if (strlen(buf) > 0) l3.emplace_back(std::string(buf));
    }
    if3.close();
    unsigned int byte_cnt3 = 0;
    for (auto it : l3) {
        byte_cnt3 += it.length();
    }

    unsigned int byte_cnt = byte_cnt1;
    if (byte_cnt < byte_cnt2) byte_cnt = byte_cnt2;
    if (byte_cnt < byte_cnt3) byte_cnt = byte_cnt3;
    if ((byte_cnt + datapath_bytes - 1) / datapath_bytes > sz_64k * IncreaseFactor_) {
        printf("Buffer overflow! need %d\n", (byte_cnt + datapath_bytes - 1) / datapath_bytes);
        return 1;
    }

    auto in_keyBuf = new ap_uint<BufWidth_Set_>[ 3 ][sz_64k * IncreaseFactor_];
    auto in_keyLen = new ap_uint<32>[ 3 ][sz_64k * IncreaseFactor_];
    auto out_keyBuf = new ap_uint<BufWidth_Set_>[ 2 ][sz_64k * 2 * IncreaseFactor_];
    auto out_meta = new ap_uint<32>[ 2 ][sz_64k * 2 * IncreaseFactor_];

    in_keyLen[0][0] = l1.size();
    unsigned int word_ptr = 0;
    unsigned int byte_ptr = 0;
    unsigned int meta_ptr = 1;
    for (auto curr_string : l1) {
        // std::cout << curr_string << " " << curr_string.length() << std::endl;

        for (auto curr_char : curr_string) {
            in_keyBuf[0][word_ptr].range(byte_ptr * 8 + 8 - 1, byte_ptr * 8) = curr_char;
            byte_ptr++;
            if (byte_ptr == datapath_bytes) {
                word_ptr++;
                byte_ptr = 0;
            }
        }
        in_keyLen[0][meta_ptr++] = curr_string.length();
    }
    std::cout << "Buf 0 word ptr " << word_ptr << std::endl;

    in_keyLen[1][0] = l2.size();
    word_ptr = 0;
    byte_ptr = 0;
    unsigned int base_meta_ptr = 1;

    for (auto curr_string : l2) {
        // std::cout << curr_string << " " << curr_string.length() << std::endl;
        for (auto curr_char : curr_string) {
            in_keyBuf[1][word_ptr].range(byte_ptr * 8 + 8 - 1, byte_ptr * 8) = curr_char;
            byte_ptr++;
            if (byte_ptr == datapath_bytes) {
                word_ptr++;
                byte_ptr = 0;
            }
        }
        in_keyLen[1][base_meta_ptr++] = curr_string.length();
    }
    std::cout << "word ptr " << word_ptr << std::endl;

    in_keyLen[2][0] = l3.size();
    word_ptr = 0;
    byte_ptr = 0;
    unsigned int l3_meta_ptr = 1;
    for (auto curr_string : l3) {
        for (auto curr_char : curr_string) {
            in_keyBuf[2][word_ptr].range(byte_ptr * 8 + 8 - 1, byte_ptr * 8) = curr_char;
            byte_ptr++;
            if (byte_ptr == datapath_bytes) {
                word_ptr++;
                byte_ptr = 0;
            }
        }
        in_keyLen[2][l3_meta_ptr++] = curr_string.length();
    }
    std::cout << "word_ptr " << word_ptr << std::endl;

    for (int i = 0; i < 2; i++) {
        for (int j = 0; j < datapath_bytes; j++) {
            unsigned char curr = in_keyBuf[0][i].range(j * 8 + 8 - 1, j * 8);
            if (curr != 0) {
                printf("-- %c ", curr);
            } else {
                printf("-- %x", curr);
            }
        }
        printf("\n");
    }

    std::cout << "---------------------------------------------------" << std::endl;

    for (int i = 0; i < 2; i++) {
        for (int j = 0; j < datapath_bytes; j++) {
            unsigned char curr = in_keyBuf[1][i].range(j * 8 + 8 - 1, j * 8);
            if (curr != 0) {
                printf("-- %c ", curr);
            } else {
                printf("-- %x", curr);
            }
        }
        printf("\n");
    }

    std::cout << "Start kernel | address used " << (byte_cnt + datapath_bytes - 1) / datapath_bytes << " " << std::endl;

    // core.CUCoreLoop(0, in_keyBuf[0], in_keyLen[0], 1, in_keyBuf[1], in_keyLen[1], out_keyBuf, out_meta);

    CUCoreLoopTop(0, in_keyBuf[0], in_keyLen[0], 1, in_keyBuf[1], in_keyLen[1], out_keyBuf[0], out_meta[0],
                  (byte_cnt1 + datapath_bytes - 1) / datapath_bytes + 4, meta_ptr + 4,
                  (byte_cnt2 + datapath_bytes - 1) / datapath_bytes + 4, base_meta_ptr + 4);

    std::vector<std::string> lout;
    unsigned int out_keyCnt = (unsigned int)out_meta[0][0];
    ap_uint<32>* out_streamID = (ap_uint<32>*)&out_meta[0][1];
    unsigned int l1_ptr = 0, l2_ptr = 0, l3_ptr = 0;
    for (int i = 0; i < out_keyCnt; i++) {
        // std::cout << "#" << i << " -- " << (unsigned int)(out_streamID[i].range(7,0)) << std::endl ;
    }
    std::cout << std::endl;
    for (int i = 0; i < out_keyCnt; i++) {
        // std::cout << "# " <<  out_keyCnt[i+1] << " " << out_streamID[i] << (unsigned int)(out_streamID[i].range(7,0))
        // << std::endl;
        if ((unsigned int)(out_streamID[i].range(7, 0)) == 0) lout.push_back(l1[l1_ptr++]);
        if ((unsigned int)(out_streamID[i].range(7, 0)) == 1) lout.push_back(l2[l2_ptr++]);
        if ((unsigned int)(out_streamID[i].range(7, 0)) == 2) lout.push_back(l3[l3_ptr++]);
    }

    std::vector<std::string> lcheck = l1;
    copy(l2.begin(), l2.end(), back_inserter(lcheck));
    sort(lcheck.begin(), lcheck.end());

    printf("Check: %s\n", (lout == lcheck) ? "PASS" : "FAIL");
    printf("\nHardware: ");
    unsigned int sample_cnt = 0;
    for (auto it : lout) {
        printf("%s ", it.c_str());
        sample_cnt++;
        if (sample_cnt > 10) break;
    }
    printf("\nSoftware: ");
    sample_cnt = 0;
    for (auto it : lcheck) {
        printf("%s ", it.c_str());
        sample_cnt++;
        if (sample_cnt > 10) break;
    }
    printf("\n");

    CUCoreLoopTop(2, in_keyBuf[2], in_keyLen[2], 0xFF, out_keyBuf[0], out_meta[0], out_keyBuf[1], out_meta[1],
                  (byte_cnt3 + datapath_bytes - 1) / datapath_bytes + 4, l3_meta_ptr + 4,
                  // 30,
                  sz_64k * IncreaseFactor_, meta_ptr + base_meta_ptr + 4);

    lout.clear();
    out_keyCnt = (unsigned int)out_meta[1][0];
    out_streamID = (ap_uint<32>*)&out_meta[1][1];
    for (int i = 0; i < out_keyCnt; i++) {
        // std::cout << "#" << i << "" << out_keyCnt[i+1] << " " ;
    }
    l1_ptr = 0;
    l2_ptr = 0;
    l3_ptr = 0;
    std::cout << std::endl;
    for (int i = 0; i < out_keyCnt; i++) {
        // std::cout << "# " <<  out_keyCnt[i+1] << " " << out_streamID[i] << (unsigned int)(out_streamID[i].range(7,0))
        // << std::endl;
        if ((unsigned int)(out_streamID[i].range(7, 0)) == 0) lout.push_back(l1[l1_ptr++]);
        if ((unsigned int)(out_streamID[i].range(7, 0)) == 1) lout.push_back(l2[l2_ptr++]);
        if ((unsigned int)(out_streamID[i].range(7, 0)) == 2) lout.push_back(l3[l3_ptr++]);
    }

    lcheck.clear();
    copy(l1.begin(), l1.end(), back_inserter(lcheck));
    copy(l2.begin(), l2.end(), back_inserter(lcheck));
    copy(l3.begin(), l3.end(), back_inserter(lcheck));
    sort(lcheck.begin(), lcheck.end());

    printf("Check: %s\n", (lout == lcheck) ? "PASS" : "FAIL");
    printf("\nHardware: ");
    unsigned int sample_cnt = 0;
    for (auto it : lout) {
        printf("%s ", it.c_str());
        sample_cnt++;
        if (sample_cnt > 10) break;
    }
    printf("\nSoftware: ");
    sample_cnt = 0;
    for (auto it : lcheck) {
        printf("%s ", it.c_str());
        sample_cnt++;
        if (sample_cnt > 10) break;
    }
    printf("\n");

    delete[] out_meta;
    delete[] out_keyBuf;
    delete[] in_keyBuf;
    delete[] in_keyLen;
    return 0;
}
