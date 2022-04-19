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

#ifndef _BLOCK_SORT_HPP_
#define _BLOCK_SORT_HPP_

#include "xf_database/compound_sort.hpp"

namespace xf {
namespace data_analytics {
namespace geospatial {
namespace internal {

template <typename KT, typename VT, int ISN, int N>
void channel1toNCore(hls::stream<KT>& inKeyStrm,
                     hls::stream<VT>& inDataStrm,
                     hls::stream<bool>& inEndStrm,
                     hls::stream<KT> outKeyStrm[N],
                     hls::stream<VT> outDataStrm[N],
                     hls::stream<bool> outEndBlockStrm[N],
                     hls::stream<bool> outEndStrm[N]) {
    int flag = 0;
    bool blockFlag = false;
    int index = 0;
    bool endFlag = inEndStrm.read();
    if (endFlag) flag = -1;
loop_channel1toNCore:
    while (!endFlag) {
#pragma HLS loop_tripcount max = 100000 min = 100000
#pragma HLS pipeline ii = 1
        if ((index % ISN == 0) && blockFlag) {
            outEndBlockStrm[flag].write(true);
            blockFlag = false;
            flag = (flag + 1) % N;
        } else {
            blockFlag = true;
            KT key = inKeyStrm.read();
            VT data = inDataStrm.read();
            outKeyStrm[flag].write(key);
            outDataStrm[flag].write(data);
            outEndBlockStrm[flag].write(false);
            if (index % ISN == 0) {
                outEndStrm[flag].write(false);
            }
            index += 1;
            endFlag = inEndStrm.read();
        }
    }
    if (flag >= 0) outEndBlockStrm[flag].write(true);
    for (int i = 0; i < N; i++) {
#pragma HLS pipeline ii = 2
        if (i > flag) {
            outEndBlockStrm[i].write(true);
            outEndStrm[i].write(false);
        }
        outEndStrm[i].write(true);
    }
}

template <typename KT, typename VT>
void mergeSortWrapper(hls::stream<KT>& inKeyStrm0,
                      hls::stream<VT>& inDataStrm0,
                      hls::stream<bool>& inEndBlockStrm0,
                      hls::stream<bool>& inEndStrm0,
                      hls::stream<KT>& inKeyStrm1,
                      hls::stream<VT>& inDataStrm1,
                      hls::stream<bool>& inEndBlockStrm1,
                      hls::stream<bool>& inEndStrm1,
                      hls::stream<KT>& outKeyStrm,
                      hls::stream<VT>& outDataStrm,
                      hls::stream<bool>& outEndBlockStrm,
                      hls::stream<bool>& outEndStrm,
                      bool order) {
    while (!inEndStrm0.read()) {
#pragma HLS loop_tripcount max = 100 min = 100
        inEndStrm1.read();
        outEndStrm.write(false);
        xf::database::mergeSort<VT, KT>(inDataStrm0, inKeyStrm0, inEndBlockStrm0, inDataStrm1, inKeyStrm1,
                                        inEndBlockStrm1, outDataStrm, outKeyStrm, outEndBlockStrm, order);
    }
    inEndStrm1.read();
    outEndStrm.write(true);
}

template <typename KT, typename VT>
void flatenStrm(hls::stream<KT>& KeyStrm,
                hls::stream<VT>& DataStrm,
                hls::stream<bool>& endBlockStrm,
                hls::stream<bool>& endStrm,
                hls::stream<KT>& outKeyStrm,
                hls::stream<VT>& outDataStrm,
                hls::stream<bool>& outEndStrm) {
    while (!endStrm.read()) {
#pragma HLS loop_tripcount max = 100 min = 100
        while (!endBlockStrm.read()) {
#pragma HLS loop_tripcount max = 1000 min = 1000
            KT key = KeyStrm.read();
            VT val = DataStrm.read();
            outKeyStrm.write(key);
            outDataStrm.write(val);
            outEndStrm.write(0);
        }
    }
    outEndStrm.write(1);
}

template <typename KT, typename VT, int ISN>
void mergeSort4to1(hls::stream<KT>& inKeyStrm,
                   hls::stream<VT>& inDataStrm,
                   hls::stream<bool>& inEndStrm,
                   bool order,
                   hls::stream<KT>& outKeyStrm,
                   hls::stream<VT>& outDataStrm,
                   hls::stream<bool>& outEndStrm) {
#pragma HLS dataflow
    hls::stream<KT, 2048> keyStrm1[4];
    hls::stream<VT, 2048> dataStrm1[4];
    hls::stream<bool, 2048> endBlockStrm1[4];
    hls::stream<bool, 32> endStrm1[4];
    hls::stream<KT, 32> keyStrm2[2];
    hls::stream<VT, 32> dataStrm2[2];
    hls::stream<bool, 32> endBlockStrm2[2];
    hls::stream<bool, 32> endStrm2[2];
    channel1toNCore<KT, VT, ISN, 4>(inKeyStrm, inDataStrm, inEndStrm, keyStrm1, dataStrm1, endBlockStrm1, endStrm1);
    for (int i = 0; i < 2; i++) {
#pragma HLS unroll
        mergeSortWrapper<KT, VT>(keyStrm1[i * 2], dataStrm1[i * 2], endBlockStrm1[i * 2], endStrm1[i * 2],
                                 keyStrm1[i * 2 + 1], dataStrm1[i * 2 + 1], endBlockStrm1[i * 2 + 1],
                                 endStrm1[i * 2 + 1], keyStrm2[i], dataStrm2[i], endBlockStrm2[i], endStrm2[i], order);
    }

    hls::stream<KT, 32> outKeyStrm2;
    hls::stream<VT, 32> outDataStrm2;
    hls::stream<bool, 32> outEndBlkStrm2;
    hls::stream<bool, 32> outEndStrm2;
    mergeSortWrapper<KT, VT>(keyStrm2[0], dataStrm2[0], endBlockStrm2[0], endStrm2[0], keyStrm2[1], dataStrm2[1],
                             endBlockStrm2[1], endStrm2[1], outKeyStrm2, outDataStrm2, outEndBlkStrm2, outEndStrm2,
                             order);
    flatenStrm<KT, VT>(outKeyStrm2, outDataStrm2, outEndBlkStrm2, outEndStrm2, outKeyStrm, outDataStrm, outEndStrm);
}

template <typename KT, typename VT, typename MT, int LEN>
void push(hls::stream<KT>& i_key_strm,
          hls::stream<VT>& i_val_strm,
          hls::stream<bool>& i_end_strm,
          int& blk_sz,
          MT* o_mem_buf) {
    MT tmp = 0;
    for (int i = 0; i < LEN; i++) {
#pragma HLS loop_tripcount max = 1000 min = 1000
#pragma HLS pipeline ii = 1
        if (!i_end_strm.read()) {
            tmp(sizeof(KT) * 8 - 1, 0) = i_key_strm.read();
            tmp((sizeof(KT) + sizeof(VT)) * 8 - 1, sizeof(KT) * 8) = i_val_strm.read();
            o_mem_buf[i] = tmp;
        } else {
            blk_sz = i + 1;
            break;
        }
    }
}

template <typename KT, typename VT, typename MT, int LEN>
void strm2Mem(bool& flag,
              hls::stream<KT>& KeyStrm,
              hls::stream<VT>& DataStrm,
              hls::stream<bool>& EndStrm,
              MT* value,
              hls::stream<int>& blkSzStrm,
              hls::stream<bool>& bpCtrlStrm) {
    MT tmp = 0;
    int sz = LEN;
    bpCtrlStrm.write(0);
    for (int i = 0; i < LEN; i++) {
#pragma HLS loop_tripcount max = 1000 min = 1000
        if (!EndStrm.read()) {
            tmp(sizeof(KT) * 8 - 1, 0) = KeyStrm.read();
            tmp((sizeof(KT) + sizeof(VT)) * 8 - 1, sizeof(KT) * 8) = DataStrm.read();
            value[i] = tmp;
        } else {
            sz = i;
            flag = 1;
            break;
        }
        // std::cout << "In Point: i=" << i << ", x=" << tmp(31, 0) << ", key=" << tmp(63, 32) << ", id=" << tmp(95, 64)
        //          << std::endl;
    }
    blkSzStrm.write(sz);
}

template <typename KT, typename VT, typename MT, int BSN>
void pushMem(hls::stream<KT>& i_key_strm,
             hls::stream<VT>& i_val_strm,
             hls::stream<bool>& i_end_strm,
             MT* uram_buf1,
             MT* uram_buf2,
             hls::stream<int>& o_blk_sz_strm,
             hls::stream<bool>& o_bp_ctrl_strm,
             hls::stream<bool>& o_blk_end_strm) {
#pragma HLS allocation function instances = strm2Mem < KT, VT, MT, BSN > limit = 1
    bool end_flag = 0;
    int cnt = 0;
    while (!end_flag) {
#pragma HLS loop_tripcount max = 1000 min = 1000
        o_blk_end_strm.write(0);
#ifndef __SYNTHESIS__
        strm2Mem<KT, VT, MT, BSN>(end_flag, i_key_strm, i_val_strm, i_end_strm, uram_buf1 + cnt * BSN, o_blk_sz_strm,
                                  o_bp_ctrl_strm);
#else
        if (cnt % 2 == 0)
            strm2Mem<KT, VT, MT, BSN>(end_flag, i_key_strm, i_val_strm, i_end_strm, uram_buf1, o_blk_sz_strm,
                                      o_bp_ctrl_strm);
        else
            strm2Mem<KT, VT, MT, BSN>(end_flag, i_key_strm, i_val_strm, i_end_strm, uram_buf2, o_blk_sz_strm,
                                      o_bp_ctrl_strm);
#endif
        cnt++;
    }
    o_blk_end_strm.write(1);
}

template <typename KT, typename VT>
void outStrm(hls::stream<KT>& i_key_strm,
             hls::stream<VT>& i_val_strm,
             hls::stream<bool>& i_end_strm,
             hls::stream<KT>& o_key_strm,
             hls::stream<VT>& o_val_strm,
             hls::stream<bool>& o_end_strm) {
    while (!i_end_strm.read()) {
#pragma HLS pipeline ii = 1
#pragma HLS loop_tripcount max = 100000 min = 100000
        o_key_strm.write(i_key_strm.read());
        o_val_strm.write(i_val_strm.read());
        o_end_strm.write(0);
    }
}

template <typename KT, typename VT, typename MT>
void readArrayCore(int keyLength,
                   MT* value,
                   int begin_cnt,
                   int block_cnt,
                   hls::stream<KT>& keyStrm,
                   hls::stream<VT>& dataStrm,
                   hls::stream<bool>& endStrm) {
    MT tmp = 0;
loop_read:
    for (int k = 0; k < block_cnt; k++) {
#pragma HLS pipeline ii = 1
#pragma HLS loop_tripcount max = 1000 min = 1000
        if (begin_cnt + k >= keyLength) break;
        tmp = value[begin_cnt + k];
        // std::cout << "readArrayCore: i=" << k + begin_cnt << ", x=" << tmp(31, 0) << ", y=" << tmp(63, 32)
        //          << ", id=" << tmp(95, 64) << std::endl;
        keyStrm.write(tmp(sizeof(KT) * 8 - 1, 0));
        dataStrm.write(tmp((sizeof(KT) + sizeof(VT)) * 8 - 1, sizeof(KT) * 8));
        endStrm.write(false);
    }
    endStrm.write(true);
}

template <typename KT, typename VT, typename MT, int K, int BLOCK_LEN>
void readArray2NStream(hls::stream<int>& keyLenStrm,
                       hls::stream<bool>& bpCtrlStrm,
                       MT* value,
                       hls::stream<KT> keyStrm[K],
                       hls::stream<VT> dataStrm[K],
                       hls::stream<bool> endStrm[K]) {
#pragma HLS dataflow
    int keyLength = keyLenStrm.read();
    // std::cout << "keyLength=" << keyLength << std::endl;
    for (int i = 0; i < K; i++) {
#pragma HLS unroll
        readArrayCore<KT, VT, MT>(keyLength, value, i * BLOCK_LEN, BLOCK_LEN, keyStrm[i], dataStrm[i], endStrm[i]);
    }
    bpCtrlStrm.read();
}

template <typename KT, typename VT, typename MT, int ISN, int BSN, int K>
void readArray2NStream(int cnt,
                       hls::stream<int>& i_blk_sz_strm,
                       hls::stream<bool>& i_bp_ctrl_strm,
                       MT* uram_buf1,
                       MT* uram_buf2,
                       hls::stream<KT> keyStrm[K],
                       hls::stream<VT> dataStrm[K],
                       hls::stream<bool> endStrm[K]) {
#pragma HLS inline off
#ifndef __SYNTHESIS__
    readArray2NStream<KT, VT, MT, K, ISN * 4>(i_blk_sz_strm, i_bp_ctrl_strm, uram_buf1 + cnt * BSN, keyStrm, dataStrm,
                                              endStrm);
#else
    if (cnt % 2 == 0)
        readArray2NStream<KT, VT, MT, K, ISN * 4>(i_blk_sz_strm, i_bp_ctrl_strm, uram_buf1, keyStrm, dataStrm, endStrm);
    else
        readArray2NStream<KT, VT, MT, K, ISN * 4>(i_blk_sz_strm, i_bp_ctrl_strm, uram_buf2, keyStrm, dataStrm, endStrm);
#endif
}

template <typename KT, typename VT, int N, int BEGIN_CNT>
void mergeSortN(bool i_order,
                hls::stream<KT>* key_strm,
                hls::stream<VT>* val_strm,
                hls::stream<bool>* end_blk_strm,
                hls::stream<bool>* end_strm) {
#pragma HLS dataflow
    for (int i = 0; i < N / 2; i++) {
#pragma HLS unroll
        mergeSortWrapper<KT, VT>(
            key_strm[i * 2 + BEGIN_CNT], val_strm[i * 2 + BEGIN_CNT], end_blk_strm[i * 2 + BEGIN_CNT],
            end_strm[i * 2 + BEGIN_CNT], key_strm[i * 2 + 1 + BEGIN_CNT], val_strm[i * 2 + 1 + BEGIN_CNT],
            end_blk_strm[i * 2 + 1 + BEGIN_CNT], end_strm[i * 2 + 1 + BEGIN_CNT], key_strm[N + BEGIN_CNT + i],
            val_strm[N + BEGIN_CNT + i], end_blk_strm[N + BEGIN_CNT + i], end_strm[N + BEGIN_CNT + i], i_order);
    }
}

template <typename KT, typename VT, int N, int BEGIN_CNT>
void mergeSortN(bool order, hls::stream<KT>* keyStrm, hls::stream<VT>* dataStrm, hls::stream<bool>* endStrm) {
#pragma HLS dataflow
    for (int i = 0; i < N / 2; i++) {
#pragma HLS unroll
        xf::database::mergeSort<VT, KT>(
            dataStrm[i * 2 + BEGIN_CNT], keyStrm[i * 2 + BEGIN_CNT], endStrm[i * 2 + BEGIN_CNT],
            dataStrm[i * 2 + 1 + BEGIN_CNT], keyStrm[i * 2 + 1 + BEGIN_CNT], endStrm[i * 2 + 1 + BEGIN_CNT],
            dataStrm[N + i + BEGIN_CNT], keyStrm[N + i + BEGIN_CNT], endStrm[N + i + BEGIN_CNT], order);
    }
}

template <typename KT, typename VT, typename MT, int ISN, int BSN, int K>
void pullMemWrap(MT* uram_buf1,
                 MT* uram_buf2,
                 int cnt,
                 // hls::stream<KT> keyStrm[2 * K - 1],
                 // hls::stream<VT> dataStrm[2 * K - 1],
                 // hls::stream<bool> endStrm[2 * K - 1],
                 bool i_order,
                 hls::stream<int>& i_blk_sz_strm,
                 hls::stream<bool>& i_bp_ctrl_strm,
                 hls::stream<KT>& o_key_strm,
                 hls::stream<VT>& o_val_strm,
                 hls::stream<bool>& o_end_strm) {
#pragma HLS dataflow
    hls::stream<KT> keyStrm[2 * K - 1];
    hls::stream<VT> dataStrm[2 * K - 1];
    hls::stream<bool> endStrm[2 * K - 1];
#pragma HLS stream variable = keyStrm depth = 16
#pragma HLS stream variable = dataStrm depth = 16
#pragma HLS stream variable = endStrm depth = 16
    readArray2NStream<KT, VT, MT, ISN, BSN, K>(cnt, i_blk_sz_strm, i_bp_ctrl_strm, uram_buf1, uram_buf2, keyStrm,
                                               dataStrm, endStrm);
    mergeSortN<KT, VT, K, 0>(i_order, keyStrm, dataStrm, endStrm);
    if (K > 2) mergeSortN<KT, VT, K / 2, K>(i_order, keyStrm, dataStrm, endStrm);
    if (K > 4) mergeSortN<KT, VT, K / 4, K + K / 2>(i_order, keyStrm, dataStrm, endStrm);
    if (K > 8) mergeSortN<KT, VT, K / 8, 2 * K - K / 4>(i_order, keyStrm, dataStrm, endStrm);
    if (K > 16) mergeSortN<KT, VT, K / 16, 2 * K - K / 8>(i_order, keyStrm, dataStrm, endStrm);
    if (K > 32) mergeSortN<KT, VT, K / 32, 2 * K - K / 16>(i_order, keyStrm, dataStrm, endStrm);
    outStrm<KT, VT>(keyStrm[2 * K - 2], dataStrm[2 * K - 2], endStrm[2 * K - 2], o_key_strm, o_val_strm, o_end_strm);
}

template <typename KT, typename VT, typename MT, int ISN, int BSN>
void pullMem(MT* uram_buf1,
             MT* uram_buf2,
             bool i_order,
             hls::stream<int>& i_blk_sz_strm,
             hls::stream<bool>& i_bp_ctrl_strm,
             hls::stream<bool>& i_blk_end_strm,
             hls::stream<KT>& o_key_strm,
             hls::stream<VT>& o_val_strm,
             hls::stream<bool>& o_end_strm) {
    int cnt = 0;
    const int K = BSN / (ISN * 4);
    while (!i_blk_end_strm.read()) {
#pragma HLS loop_tripcount max = 10 min = 10
        pullMemWrap<KT, VT, MT, ISN, BSN, K>(uram_buf1, uram_buf2, cnt, /*keyStrm, dataStrm, endStrm,*/ i_order,
                                             i_blk_sz_strm, i_bp_ctrl_strm, o_key_strm, o_val_strm, o_end_strm);
        cnt++;
    }
    o_end_strm.write(1);
}

template <typename KT, typename VT, typename MT, int ISN, int OSN>
void pull(int i_blk_sz,
          MT* i_mem_buf,
          hls::stream<KT> o_key_strm[OSN / ISN],
          hls::stream<VT> o_val_strm[OSN / ISN],
          hls::stream<bool> o_end_strm[OSN / ISN]) {
#pragma HLS dataflow
    for (int i = 0; i < OSN / ISN; i++) {
#pragma HLS unroll
        readArrayCore<KT, VT, MT>(i_blk_sz, i_mem_buf, i * ISN, ISN, o_key_strm[i], o_val_strm[i], o_end_strm[i]);
    }
}

template <typename KT, typename VT, int N>
void mergeTreeLayer(bool i_order,
                    hls::stream<KT> i_key_strm[N],
                    hls::stream<VT> i_val_strm[N],
                    hls::stream<bool> i_end_blk_strm[N],
                    hls::stream<bool> i_end_strm[N],
                    hls::stream<KT> o_key_strm[N / 2],
                    hls::stream<VT> o_val_strm[N / 2],
                    hls::stream<bool> o_end_blk_strm[N / 2],
                    hls::stream<bool> o_end_strm[N / 2]) {
#pragma HLS dataflow
    for (int i = 0; i < N / 2; i++) {
#pragma HLS unroll
        mergeSortWrapper<KT, VT>(i_key_strm[i * 2], i_val_strm[i * 2], i_end_blk_strm[i * 2], i_end_strm[i * 2],
                                 i_key_strm[i * 2 + 1], i_val_strm[i * 2 + 1], i_end_blk_strm[i * 2 + 1],
                                 i_end_strm[i * 2 + 1], o_key_strm[i], o_val_strm[i], o_end_blk_strm[i], o_end_strm[i],
                                 i_order);
    }
}

template <typename KT, typename VT, int N>
void mergeSortTree(bool i_order,
                   hls::stream<KT> i_key_strm[N],
                   hls::stream<VT> i_val_strm[N],
                   hls::stream<bool> i_end_blk_strm[N],
                   hls::stream<bool> i_end_strm[N],
                   hls::stream<KT>& o_key_strm,
                   hls::stream<VT>& o_val_strm,
                   hls::stream<bool>& o_end_strm) {
#pragma HLS dataflow
    hls::stream<KT, 32> key_strm[N - 1];
    hls::stream<VT, 32> val_strm[N - 1];
    hls::stream<bool, 32> end_blk_strm[N - 1];
    hls::stream<bool, 32> end_strm[N - 1];
    mergeTreeLayer<KT, VT, N>(i_order, i_key_strm, i_val_strm, i_end_blk_strm, i_end_strm, key_strm, val_strm,
                              end_blk_strm, end_strm);
    if (N > 2) mergeSortN<KT, VT, N / 2, 0>(i_order, key_strm, val_strm, end_blk_strm, end_strm);
    if (N > 4) mergeSortN<KT, VT, N / 4, N - N / 2>(i_order, key_strm, val_strm, end_blk_strm, end_strm);
    if (N > 8) mergeSortN<KT, VT, N / 8, N - N / 4>(i_order, key_strm, val_strm, end_blk_strm, end_strm);
    if (N > 16) mergeSortN<KT, VT, N / 16, N - N / 8>(i_order, key_strm, val_strm, end_blk_strm, end_strm);
    if (N > 32) mergeSortN<KT, VT, N / 32, N - N / 16>(i_order, key_strm, val_strm, end_blk_strm, end_strm);
    flatenStrm<KT, VT>(key_strm[N - 2], val_strm[N - 2], end_blk_strm[N - 2], end_strm[N - 2], o_key_strm, o_val_strm,
                       o_end_strm);
}

template <typename KT, typename VT, typename MT, int ISN, int OSN>
void manualPingPong(hls::stream<KT>& i_key_strm,
                    hls::stream<VT>& i_val_strm,
                    hls::stream<bool>& i_end_strm,
                    MT* uram_buf1,
                    MT* uram_buf2,
                    int& o_blk_sz,
                    hls::stream<KT> o_key_strm[OSN / ISN],
                    hls::stream<VT> o_val_strm[OSN / ISN],
                    hls::stream<bool> o_end_strm[OSN / ISN]) {
#pragma HLS dataflow
    push<KT, VT, MT, OSN>(i_key_strm, i_val_strm, i_end_strm, o_blk_sz, uram_buf2);
    pull<KT, VT, MT, ISN, OSN>(OSN, uram_buf1, o_key_strm, o_val_strm, o_end_strm);
}

template <typename KT, typename VT, typename MT, int ISN, int OSN>
void manualPingPongWrap(hls::stream<KT>& i_key_strm,
                        hls::stream<VT>& i_val_strm,
                        hls::stream<bool>& i_end_strm,
                        MT* uram_buf1,
                        MT* uram_buf2,
                        hls::stream<KT> o_key_strm[OSN / ISN],
                        hls::stream<VT> o_val_strm[OSN / ISN],
                        hls::stream<bool> o_end_blk_strm[OSN / ISN],
                        hls::stream<bool> o_end_strm[OSN / ISN]) {
    int blk_sz = 0;
    push<KT, VT, MT, OSN>(i_key_strm, i_val_strm, i_end_strm, blk_sz, uram_buf1);
    int cnt = 0;
    for (int i = 0; i < OSN / ISN; i++) {
        o_end_strm[i].write(0);
    }
    while (blk_sz == 0) {
#pragma HLS loop_tripcount max = 100 min = 100
        if (cnt % 2 == 0)
            manualPingPong<KT, VT, MT, ISN, OSN>(i_key_strm, i_val_strm, i_end_strm, uram_buf1, uram_buf2, blk_sz,
                                                 o_key_strm, o_val_strm, o_end_blk_strm);
        else
            manualPingPong<KT, VT, MT, ISN, OSN>(i_key_strm, i_val_strm, i_end_strm, uram_buf2, uram_buf1, blk_sz,
                                                 o_key_strm, o_val_strm, o_end_blk_strm);
        for (int i = 0; i < OSN / ISN; i++) {
            o_end_strm[i].write(0);
        }
        cnt++;
    }
    if (cnt % 2 == 0)
        pull<KT, VT, MT, ISN, OSN>(blk_sz - 1, uram_buf1, o_key_strm, o_val_strm, o_end_blk_strm);
    else
        pull<KT, VT, MT, ISN, OSN>(blk_sz - 1, uram_buf2, o_key_strm, o_val_strm, o_end_blk_strm);
    for (int i = 0; i < OSN / ISN; i++) {
        o_end_strm[i].write(1);
    }
    cnt++;
}

template <typename KT, typename VT, typename MT, int ISN, int BSN, int MSN>
void blockSort(bool i_order,
               hls::stream<KT>& i_key_strm,
               hls::stream<VT>& i_val_strm,
               hls::stream<bool>& i_end_strm,
               hls::stream<KT>& o_key_strm,
               hls::stream<VT>& o_val_strm,
               hls::stream<bool>& o_end_strm) {
#pragma HLS dataflow

    hls::stream<KT, 32> insert_key_strm("insert_key_strm");
    hls::stream<VT, 32> insert_val_strm("insert_val_strm");
    hls::stream<bool, 32> insert_end_strm;
    xf::database::insertSort<KT, VT, ISN>(i_val_strm, i_key_strm, i_end_strm, insert_val_strm, insert_key_strm,
                                          insert_end_strm, i_order);

    hls::stream<KT, 32> merge4_key_strm("merge4_key_strm");
    hls::stream<VT, 32> merge4_val_strm("merge4_val_strm");
    hls::stream<bool, 32> merge4_end_strm("merge4_end_strm");
    mergeSort4to1<KT, VT, ISN>(insert_key_strm, insert_val_strm, insert_end_strm, i_order, merge4_key_strm,
                               merge4_val_strm, merge4_end_strm);
#ifdef __SYNTHESIS__
    MT uram_buf1[BSN];
    MT uram_buf2[BSN];
    const int AP = BSN / ISN / 4;
#pragma HLS bind_storage variable = uram_buf1 type = RAM_T2P impl = uram
#pragma HLS array_partition variable = uram_buf1 dim = 1 block factor = AP
#pragma HLS bind_storage variable = uram_buf2 type = RAM_T2P impl = uram
#pragma HLS array_partition variable = uram_buf2 dim = 1 block factor = AP
#else
    MT* uram_buf1;
    MT* uram_buf2;
    uram_buf1 = new MT[BSN * 10];
    uram_buf2 = new MT[BSN];
#endif
    const int K = BSN / ISN / 4;
    hls::stream<KT, 32> key_strm[K];
    hls::stream<VT, 32> val_strm[K];
    hls::stream<bool, 32> end_blk_strm[K];
    hls::stream<bool, 32> end_strm[K];
    manualPingPongWrap<KT, VT, MT, ISN * 4, BSN>(merge4_key_strm, merge4_val_strm, merge4_end_strm, uram_buf1,
                                                 uram_buf2, key_strm, val_strm, end_blk_strm, end_strm);
    mergeSortTree<KT, VT, K>(i_order, key_strm, val_strm, end_blk_strm, end_strm, o_key_strm, o_val_strm, o_end_strm);
}

} // internal
} // geospatial
} // data_analytics
} // xf
#endif // _BLOCK_SORT_HPP_
