/*
 * (c) Copyright 2022 Xilinx, Inc. All rights reserved.
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
 *
 */
#ifndef _PAIRHMMMODULES_HPP_
#define _PAIRHMMMODULES_HPP_
#define __constant
#define __kernel
#define __global
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include "hls_stream.h"
#include <ap_int.h>
#include "ap_utils.h"
#include "hls_half.h"
#include <math.h>
#include "ph2pr.hpp"
#include "ph2pr_sub1.hpp"
#include "ph2pr_div3.hpp"
#include "common.hpp"
#define PH2PR_SUB1_10 float(0.9000000059604644775390625)
#define PH2PR_10 float(0.0999999940395355224609375)
#define GMEM_DWIDTH 512
#define READ_INFO_OFFSET 0
#define READ_DATA_OFFSET ((MAX_RSDATA_NUM / READ_BLOCK_SIZE) * 64 / 512)
#define HAP_LEN_OFFSET READ_DATA_OFFSET + (MAX_RSDATA_NUM * MAX_READ_LEN * 64 / 512)
#define HAP_DATA_OFFSET HAP_LEN_OFFSET + (MAX_HAPDATA_NUM * 64 / 512)
#define NUMREADPU_OFFSET HAP_DATA_OFFSET + (16 * MAX_HAPDATA_NUM * MAX_HAP_LEN / HAP_BLOCK_SIZE / 512)
#define ITERNUM_OFFSET NUMREADPU_OFFSET + (16 * 64 / 512)
#define END_OFFSET ITERNUM_OFFSET + (64 * 64 / 512)

namespace xf {
namespace genomics {

const int read_fifo_depth = 2 * (1 + MAX_READ_LEN * READ_BLOCK_SIZE);
const int read_block_size = READ_BLOCK_SIZE;
const int hap_width = 4 * HAP_BLOCK_SIZE;

typedef float custom_float;

typedef struct {
    custom_float M;
    custom_float X;
    custom_float Y;
} cellData;

typedef union {
    int hex;
    float val;
} floatUnion;

typedef struct {
    float val;
    int index;
} accData;

typedef struct { accData pack[8]; } accDataBundle;

typedef struct {
    int hapLen;
    float oneDivHapLen;
} HapLenPack;

typedef ap_uint<64> hapLenType;
typedef ap_uint<hap_width> hapType;
typedef ap_uint<64> hapTypeBundle;
typedef ap_uint<512> busType;
typedef ap_uint<64> readType;
typedef ap_uint<64> longlong;

typedef struct { ap_uint<512> reads[MAX_READ_LEN / 8]; } readBundle;

typedef struct {
    ap_uint<8> readLen0;
    ap_uint<8> readLen1;
    ap_uint<21> resultOffset;
    ap_uint<21> iterNum;
    bool oneOrTwo;
} readInfor;

cellData computePipeline(custom_float factor,
                         custom_float p_MM,
                         custom_float p_MX,
                         custom_float p_MY,
                         cellData up,
                         cellData left,
                         cellData diag,
                         float& resultMX,
                         short c,
                         ap_uint<8> r,
                         custom_float Y_firstRow) {
#pragma HLS inline
    cellData ret;
    if (c <= 1) {
        diag.M = 0.0;
        diag.X = 0.0;
        diag.Y = 0.0;
        left.M = 0.0;
        left.Y = 0.0;
    }

    if (r == 1) {
        diag.M = 0.0;
        diag.X = 0.0;
        diag.Y = Y_firstRow;
        up.M = 0.0;
        up.X = 0.0;
    }

    float diag_XY = diag.X + diag.Y;
#pragma HLS BIND_STORAGE variable = diag_XY type = ram_t2p
    //#pragma HLS resource variable=diag_XY core=FAddSub_nodsp

    float pure_M = diag.M * p_MM + diag_XY * PH2PR_SUB1_10;
#pragma HLS BIND_STORAGE variable = pure_M type = ram_t2p
    //#pragma HLS resource variable=pure_M core=FAddSub_nodsp
    ret.M = factor * pure_M;
    ret.X = up.M * p_MX + up.X * PH2PR_10;
    ret.Y = left.M * p_MY + left.Y * PH2PR_10;
    resultMX = (float)(ret.M + ret.X);
#pragma HLS BIND_STORAGE variable = resultMX type = ram_t2p
    //#pragma HLS resource variable=resultMX core=FAddSub_nodsp
    return ret;
}

float computePE(cellData buff_diag[MAX_READ_LEN],
                cellData buff_upleft[MAX_READ_LEN],
                cellData buff_cur[MAX_READ_LEN],
                ap_uint<8> r,
                short c,
                float Y_firstRow,
                float factor,
                float p_MM,
                float p_MX,
                float p_MY,
                cellData& upleft,
                cellData& diag) {
#pragma HLS inline
    cellData data_diag;
    cellData data_up;
    cellData data_left;
    cellData data_cur;

    data_diag = diag;
    data_up = upleft;
    data_left = buff_upleft[r];
    upleft = data_left;
    diag = buff_diag[r];

    float resultMX;

    data_cur = computePipeline((custom_float)factor, (custom_float)p_MM, (custom_float)p_MX, (custom_float)p_MY,
                               data_up, data_left, data_diag, resultMX, c, r, (custom_float)Y_firstRow);

    buff_cur[r] = data_cur;
    return resultMX;
}

void computePEWrap(ap_uint<4> _rs,
                   ap_uint<4> _hap,
                   float p_MM,
                   float ph2pr_div3_val,
                   float ph2pr_sub1_val,
                   float& cur_result,
                   ap_uint<3> dmod3,
                   cellData buff[3][MAX_READ_LEN],
                   ap_uint<8> r,
                   short c,
                   float Y_firstRow,
                   float p_MX,
                   float p_MY,
                   cellData& upleft,
                   cellData& diag) {
#pragma HLS inline
    bool equal = (_rs == _hap || _rs == 4 || _hap == 4);
    float factor = equal ? ph2pr_sub1_val : ph2pr_div3_val;
    float resultMX;
    ap_uint<2> dmod3_n, dmod3_nn;
    if (dmod3 == 0) {
        dmod3_n = 1;
        dmod3_nn = 2;
    } else if (dmod3 == 1) {
        dmod3_n = 2;
        dmod3_nn = 0;
    } else {
        dmod3_n = 0;
        dmod3_nn = 1;
    }
    cellData local_upleft, local_diag;
    local_upleft = upleft;
    local_diag = diag;
    resultMX = computePE(buff[dmod3], buff[dmod3_n], buff[dmod3_nn], r, c, Y_firstRow, factor, p_MM, p_MX, p_MY,
                         local_upleft, local_diag);
    upleft = local_upleft;
    diag = local_diag;

    // if(r == rows - 1){
    cur_result = resultMX;
    //}
}

void computeEngine(short numHap,
                   hls::stream<readType>& readDataCh,
                   hls::stream<hapLenType>& hapLenCh,
                   hls::stream<hapTypeBundle>& haplotypeBasesCh,
                   hls::stream<accDataBundle>& results,
                   hls::stream<short>& max_colsCh,
                   hls::stream<short>& numReadsCh,
                   hls::stream<longlong>& totalIterNumCh) {
#pragma HLS inline off
    const float ph2pr[128] = PH2PR_INIT;
    const float ph2pr_div3[128] = PH2PR_DIV3_INIT;
#pragma HLS BIND_STORAGE variable = ph2pr_div3 type = rom_2p
    const float ph2pr_sub1[128] = PH2PR_SUB1_INIT;
#pragma HLS BIND_STORAGE variable = ph2pr_sub1 type = rom_2p

    ap_uint<16> hapLen[HAP_BLOCK_SIZE][MAX_HAPDATA_NUM / HAP_BLOCK_SIZE];
#pragma HLS ARRAY_PARTITION variable = hapLen complete dim = 1
#pragma HLS BIND_STORAGE variable = hapLen type = ram_1p impl = lutram

    ap_uint<32> Yrow1[HAP_BLOCK_SIZE][MAX_HAPDATA_NUM / HAP_BLOCK_SIZE];
#pragma HLS ARRAY_PARTITION variable = Yrow1 complete dim = 1
#pragma HLS BIND_STORAGE variable = Yrow1 type = ram_1p impl = lutram

    ap_uint<64> haplotypeBases[MAX_HAPDATA_NUM / HAP_BLOCK_SIZE * MAX_HAP_LEN / 4];
#pragma HLS BIND_STORAGE variable = haplotypeBases type = ram_t2p impl = uram
    char kmod = 0;
    int addr = 0;
    int k = 0;
consume_hap_len:
    while (k < numHap) {
        ap_uint<64> hapLen_tmp;
        if (hapLenCh.read_nb(hapLen_tmp)) {
            k++;
            hapLen[kmod][addr] = hapLen_tmp(15, 0);
            Yrow1[kmod][addr] = hapLen_tmp(63, 32);
            if (kmod == HAP_BLOCK_SIZE - 1) {
                addr++;
                kmod = 0;
            } else
                kmod++;
        }
    }
    ap_uint<2> numHap_mod = numHap % HAP_BLOCK_SIZE;
    ap_uint<8> numHap_div = numHap / HAP_BLOCK_SIZE + (numHap_mod != 0);
    k = 0;
consume_hap:
    while (k < numHap_div * MAX_HAP_LEN / 4) {
        if (haplotypeBasesCh.read_nb(haplotypeBases[k])) {
            k++;
        }
    }
    short max_cols = max_colsCh.read();
    short numRead = numReadsCh.read();
    longlong totalIterNum = totalIterNumCh.read();

    cellData buff[READ_BLOCK_SIZE * HAP_BLOCK_SIZE][3][MAX_READ_LEN]; // triple buffering of cellData
//#pragma HLS aggregate variable=buff
#pragma HLS ARRAY_PARTITION variable = buff complete dim = 1
#pragma HLS ARRAY_PARTITION variable = buff complete dim = 2
    ap_uint<8> r = 0;  // row
    ap_uint<11> c = 0; // col
    ap_uint<12> d = 0; // diag
    ap_uint<2> dmod3 = 0;

    ap_uint<12> i = 0;
    ap_uint<8> j = 0;
    ap_uint<8> rows[READ_BLOCK_SIZE];
#pragma HLS ARRAY_PARTITION variable = rows complete
    ap_uint<11> cols[HAP_BLOCK_SIZE];
#pragma HLS ARRAY_PARTITION variable = cols complete
    float Y_firstRow[HAP_BLOCK_SIZE];
#pragma HLS ARRAY_PARTITION variable = Y_firstRow complete
    ap_uint<21> kk = 0;

    ap_uint<8> j_limit;
    if (numHap_mod == 0)
        j_limit = numHap - HAP_BLOCK_SIZE;
    else
        j_limit = numHap - numHap_mod;

    ap_uint<8> j_idx[HAP_BLOCK_SIZE];
#pragma HLS ARRAY_PARTITION variable = j_idx complete

    readInfor readInfoBuf[2];
#pragma HLS ARRAY_PARTITION variable = readInfoBuf complete
    ap_uint<64> readDataBuf[2][READ_BLOCK_SIZE][MAX_READ_LEN];
#pragma HLS ARRAY_PARTITION variable = readDataBuf complete dim = 1
#pragma HLS ARRAY_PARTITION variable = readDataBuf complete dim = 2
    ap_uint<1> produce_idx = 1;
    ap_uint<1> consume_idx = 0;
    if (numRead != 0) {
        ap_int<10> idx = -1;
    consume_read_info:
        while (idx < MAX_READ_LEN * READ_BLOCK_SIZE) {
            readType readTmp = readDataCh.read();
            if (idx == -1) {
                readInfoBuf[0].readLen0 = readTmp(7, 0);
                readInfoBuf[0].readLen1 = readTmp(15, 8);
                readInfoBuf[0].resultOffset = readTmp(36, 16);
                readInfoBuf[0].iterNum = readTmp(57, 37);
                readInfoBuf[0].oneOrTwo = readTmp(58, 58);

            } else {
                bool bank = (idx >= MAX_READ_LEN);
                ap_int<10> offset; // = bank ? kk - 1 - MAX_READ_LEN : kk - 1;
                if (bank)
                    offset = idx - MAX_READ_LEN;
                else
                    offset = idx;
                readDataBuf[0][bank][offset] = readTmp;
            }
            idx++;
        }
        cellData upleft[READ_BLOCK_SIZE * HAP_BLOCK_SIZE];
#pragma HLS ARRAY_PARTITION variable = upleft complete
        cellData diag[READ_BLOCK_SIZE * HAP_BLOCK_SIZE];
#pragma HLS ARRAY_PARTITION variable = diag complete
        ap_uint<12> last_i;
        if (numRead % 2 == 0)
            last_i = numRead - 2;
        else
            last_i = numRead - 1;
        longlong kkk = 0;
    pairhmm_execute:
        for (kkk = 0; kkk < totalIterNum; kkk++) {
#pragma HLS PIPELINE II = 1
#pragma HLS dependence variable = buff inter false
            //#pragma HLS dependence variable=readDataBuf inter false
            ap_uint<64> curRead0, curRead1;
            readType readTmp;
            bool produce_enable = (j == 0 && i != last_i && kk < MAX_READ_LEN * READ_BLOCK_SIZE + 1);
            bool bank = (kk - 1 >= MAX_READ_LEN);
            ap_uint<9> offset; // = bank ? kk - 1 - MAX_READ_LEN : kk - 1;
            if (bank)
                offset = kk - 1 - MAX_READ_LEN;
            else
                offset = kk - 1;
            if (produce_enable) {
                readTmp = readDataCh.read();
                if (kk == 0) {
                    readInfoBuf[produce_idx].readLen0 = readTmp(7, 0);
                    readInfoBuf[produce_idx].readLen1 = readTmp(15, 8);
                    readInfoBuf[produce_idx].resultOffset = readTmp(36, 16);
                    readInfoBuf[produce_idx].iterNum = readTmp(57, 37);
                    readInfoBuf[produce_idx].oneOrTwo = readTmp(58, 58);
                }
            }
            if (produce_idx) {
                if (produce_enable && kk > 0) {
                    readDataBuf[1][bank][offset] = readTmp;
                }
                curRead0 = readDataBuf[0][0][r - 1];
                curRead1 = readDataBuf[0][1][r - 1];
            } else {
                if (produce_enable && kk > 0) {
                    readDataBuf[0][bank][offset] = readTmp;
                }
                curRead0 = readDataBuf[1][0][r - 1];
                curRead1 = readDataBuf[1][1][r - 1];
            }

            bool oneOrTwo = readInfoBuf[consume_idx].oneOrTwo;

            c = d - r;
            rows[0] = readInfoBuf[consume_idx].readLen0 + 1;
            rows[1] = readInfoBuf[consume_idx].readLen1 + 1;
            ap_uint<8> jdiv = j / HAP_BLOCK_SIZE;
            for (int idx = 0; idx < HAP_BLOCK_SIZE; idx++) {
#pragma HLS UNROLL
                j_idx[idx] = j + idx;
                cols[idx] = hapLen[idx][jdiv] + 1;
                floatUnion tmp;
                tmp.hex = Yrow1[idx][jdiv];
                Y_firstRow[idx] = tmp.val;
            }

            ap_uint<7> _i[2], _d[2], _c[2], _q[2];
            ap_uint<4> _rs[2];
            float p_MM[2];
            float ph2pr_sub1_val[2];
            float p_MX[2];
            float p_MY[2];
            float ph2pr_div3_val[2];
            int resultOffset[READ_BLOCK_SIZE];

            _c[0] = curRead0(6, 0);
            _d[0] = curRead0(13, 7);
            _i[0] = curRead0(20, 14);
            _q[0] = curRead0(27, 21);
            _rs[0] = curRead0(31, 28);
            floatUnion union_tmp0;
            union_tmp0.hex = curRead0(63, 32);
            p_MM[0] = union_tmp0.val;
            _c[1] = curRead1(6, 0);
            _d[1] = curRead1(13, 7);
            _i[1] = curRead1(20, 14);
            _q[1] = curRead1(27, 21);
            _rs[1] = curRead1(31, 28);
            floatUnion union_tmp1;
            union_tmp1.hex = curRead1(63, 32);
            p_MM[1] = union_tmp1.val;

            ph2pr_sub1_val[0] = ph2pr_sub1[_q[0]];
            p_MX[0] = ph2pr[_i[0]];
            p_MY[0] = ph2pr[_d[0]];
            ph2pr_div3_val[0] = ph2pr_div3[_q[0]];
            ph2pr_sub1_val[1] = ph2pr_sub1[_q[1]];
            p_MX[1] = ph2pr[_i[1]];
            p_MY[1] = ph2pr[_d[1]];
            ph2pr_div3_val[1] = ph2pr_div3[_q[1]];
            bool read_valid[2];
            ap_uint<8> big_rows = rows[0];
            if (oneOrTwo && big_rows < rows[1]) big_rows = rows[1];
            ap_uint<8> new_rows = big_rows;
            if (new_rows < DEP_DIST) new_rows = DEP_DIST;
            if (oneOrTwo) {
                read_valid[0] = true;
                read_valid[1] = true;
            } else {
                read_valid[0] = true;
                read_valid[1] = false;
            }

            accDataBundle tmp;
            resultOffset[0] = readInfoBuf[consume_idx].resultOffset;
            resultOffset[1] = readInfoBuf[consume_idx].resultOffset + numHap;

            hapType hap_pack;
            hapTypeBundle hap_pack_bundle;

            ap_uint<2> cmod4 = (c - 1) & 0x3;
            ap_uint<11> cdiv4 = (c - 1) / 4;
#ifdef SIM
            if (c >= 1)
#endif
                hap_pack_bundle = haplotypeBases[jdiv * MAX_HAP_LEN / 4 + cdiv4];
            if (cmod4 == 0)
                hap_pack = hap_pack_bundle(15, 0);
            else if (cmod4 == 1)
                hap_pack = hap_pack_bundle(31, 16);
            else if (cmod4 == 2)
                hap_pack = hap_pack_bundle(47, 32);
            else
                hap_pack = hap_pack_bundle(63, 48);

            // accData tmp_data[READ_BLOCK_SIZE][HAP_BLOCK_SIZE];
            ap_uint<4> _hap[4];
            _hap[0] = hap_pack(3, 0);
            _hap[1] = hap_pack(7, 4);
            _hap[2] = hap_pack(11, 8);
            _hap[3] = hap_pack(15, 12);
            bool valid = false;
            for (int index = 0; index < READ_BLOCK_SIZE * HAP_BLOCK_SIZE; index++) {
#pragma HLS UNROLL
                int idx = index % HAP_BLOCK_SIZE;
                int iidx = index / HAP_BLOCK_SIZE;
                ap_uint<12> max_d = cols[idx] + rows[iidx] - 1;
                accData tmp_data;
                computePEWrap(_rs[iidx], _hap[idx], p_MM[iidx], ph2pr_div3_val[iidx], ph2pr_sub1_val[iidx],
                              tmp_data.val, dmod3, buff[index], r, c, Y_firstRow[idx], p_MX[iidx], p_MY[iidx],
                              upleft[index], diag[index]);

                if (r == rows[iidx] - 1 && read_valid[iidx] && j_idx[idx] < numHap && d >= rows[iidx] && d <= max_d) {
                    if (d == max_d) {
                        tmp_data.index = -1;
                    } else {
                        tmp_data.index = resultOffset[iidx] + j_idx[idx];
                    }
                    valid = true;
                } else
                    tmp_data.index = -2;
                tmp.pack[iidx * HAP_BLOCK_SIZE + idx] = tmp_data;
            }
            if (valid) results.write(tmp);

            if (r == new_rows) {
                if (d == big_rows + max_cols - 1) {
                    d = 0;
                    dmod3 = 0;
                } else {
                    d++;
                    if (dmod3 == 2)
                        dmod3 = 0;
                    else
                        dmod3 = dmod3 + 1;
                }
                r = 0;
            } else
                r = r + 1;
            if (kk == readInfoBuf[consume_idx].iterNum) {
                if (j == j_limit) {
                    produce_idx = !produce_idx;
                    consume_idx = !consume_idx;
                    j = 0;
                    i += READ_BLOCK_SIZE;
                } else
                    j += HAP_BLOCK_SIZE;
                kk = 0;
            } else
                kk++;
        }
    }
}

void accEngine(hls::stream<accDataBundle>& results, hls::stream<accData>& output, hls::stream<int>& numResults) {
#pragma HLS inline off
    float acc[READ_BLOCK_SIZE * HAP_BLOCK_SIZE]; // = {0, 0, 0, 0, 0, 0, 0, 0};
#pragma HLS ARRAY_PARTITION variable = acc complete
    int index[READ_BLOCK_SIZE * HAP_BLOCK_SIZE];
#pragma HLS ARRAY_PARTITION variable = index complete
    for (char i = 0; i < READ_BLOCK_SIZE * HAP_BLOCK_SIZE; i++) {
#pragma HLS UNROLL
        acc[i] = 0;
        index[i] = 0;
    }
    int count = 0;
    int expected = numResults.read();
    accDataBundle tmp;
#pragma HLS aggregate variable = tmp
accumulate_result:
    while (count < expected) {
        if (!results.empty()) {
            results.read_nb(tmp);
            for (ap_uint<8> idx = 0; idx < READ_BLOCK_SIZE * HAP_BLOCK_SIZE; idx++) {
                if (tmp.pack[idx].index != -2) {
                    if (tmp.pack[idx].index == -1) {
                        accData tmp_data;
                        tmp_data.val = acc[idx];
                        tmp_data.index = index[idx];
                        output.write(tmp_data);
                        count++;
                        index[idx] = 0;
                        acc[idx] = 0;
                    } else {
                        index[idx] = tmp.pack[idx].index;
                        float tmp_acc = acc[idx] + tmp.pack[idx].val;
#pragma HLS BIND_STORAGE variable = tmp_acc type = ram_t2p
                        //#pragma HLS resource variable=tmp_acc core=FAddSub_nodsp
                        acc[idx] = tmp_acc;
                    }
                }
            }
        }
    }
}

template <unsigned int PU_NUM>
void accBusEngine(int expected, hls::stream<accData> results[PU_NUM], float* output) {
#pragma HLS inline off
    int index = 0;
    accData tmp;
#pragma HLS aggregate variable = tmp
write_result_to_bus:
    while (index < expected) {
        for (char idx = 0; idx < PU_NUM; idx++) {
            if (results[idx].read_nb(tmp)) {
                output[tmp.index] = tmp.val;
                index++;
            }
        }
    }
}

template <unsigned int PU_NUM>
void dispatcher(ap_uint<512>* input_data,
                short numRead,
                short numHap,
                hls::stream<readType> readData[PU_NUM],
                hls::stream<hapLenType> hapLen[PU_NUM],
                hls::stream<hapTypeBundle> haplotypeBases[PU_NUM],
                hls::stream<short> max_cols[PU_NUM],
                hls::stream<short> numReadsCh[PU_NUM],
                hls::stream<int> numResultsCh[PU_NUM],
                hls::stream<longlong> totalIterNumCh[PU_NUM]) {
#pragma HLS inline off
    int readLenPtr = 0;
    int readDataPtr = 0;
    int hapLenPtr = 0;
    int hapDataPtr = 0;
    int i = 0;

    int readDataCount = 0;
    char PU_idx = 0;

    short maxCols = 0;
    int addr = HAP_LEN_OFFSET;
    int iter_num = numHap / 8 + (numHap % 8 != 0);
    ap_uint<512> tmp = input_data[addr];
produce_hap_len:
    for (int k = 0; k < numHap; k++) {
        ap_uint<3> kmod8 = k & 7;
        ap_uint<64> hapLenPack = tmp(63, 0);
        short hapLen_tmp = hapLenPack(15, 0);
        if (hapLen_tmp + 1 > maxCols) maxCols = hapLen_tmp + 1;
        for (int j = 0; j < PU_NUM; j++) {
#pragma HLS UNROLL
            hapLen[j].write(hapLenPack);
        }
        if (kmod8 == 7) {
            addr++;
            tmp = input_data[addr];
        } else
            tmp = (tmp >> 64);
    }

    addr = HAP_DATA_OFFSET;
    int numHapSeg = numHap / HAP_BLOCK_SIZE + (numHap % HAP_BLOCK_SIZE != 0);
    ap_uint<512> hapBurstBuf[MAX_HAPDATA_NUM * MAX_HAP_LEN / HAP_BLOCK_SIZE / 32];
#pragma HLS BIND_STORAGE variable = hapBurstBuf type = ram_t2p impl = uram
    memcpy(hapBurstBuf, &input_data[addr], numHapSeg * MAX_HAP_LEN * 2);
    // hap_burst:
    //  for (int i = 0; i < numHapSeg * MAX_HAP_LEN / 32; i++) {
    //      hapBurstBuf[i] = input_data[addr++];
    //  }
    addr = 0;
    tmp = hapBurstBuf[addr];
produce_hap:
    for (int i = 0; i < numHapSeg * MAX_HAP_LEN / 4; i++) {
        ap_uint<3> imod8 = i & 7;
        hapTypeBundle hap_tmp = tmp(63, 0);
        for (int idx = 0; idx < PU_NUM; idx++) {
#pragma HLS UNROLL
            haplotypeBases[idx].write(hap_tmp);
        }
        if (imod8 == 7) {
            addr++;
            tmp = hapBurstBuf[addr];
        } else {
            tmp = (tmp >> 64);
        }
    }

    for (int idx = 0; idx < PU_NUM; idx++) {
#pragma HLS UNROLL
        max_cols[idx].write_nb(maxCols);
    };

    char kmod = 0;
    busType readDataBuf;
    short PU_id = 0;
    addr = NUMREADPU_OFFSET;
    ap_uint<512> numReadPUBuf = input_data[addr];
boardcast_ctrl_info0:
    for (int i = 0; i < PU_NUM; i++) {
        ap_uint<5> imod32 = i & 31;
        short cur_numRead = numReadPUBuf(15, 0);
        numReadsCh[i].write_nb(cur_numRead);
        int numRes = cur_numRead * numHap;
        numResultsCh[i].write_nb(numRes);
        if (imod32 == 31) {
            addr++;
            numReadPUBuf = input_data[addr];
        } else
            numReadPUBuf = numReadPUBuf >> 16;
    }

    addr = ITERNUM_OFFSET;
    ap_uint<512> totalIterNumBuf = input_data[addr];
boardcast_ctrl_info1:
    for (int i = 0; i < PU_NUM; i++) {
        ap_uint<3> imod8 = i & 7;
        totalIterNumCh[i].write_nb(totalIterNumBuf(63, 0));
        if (imod8 == 7) {
            addr++;
            totalIterNumBuf = input_data[addr];
        } else {
            totalIterNumBuf = totalIterNumBuf >> 64;
        }
    }

    PU_id = 0;
    addr = READ_DATA_OFFSET;
    int info_addr = READ_INFO_OFFSET;

    ap_uint<512> readBurstBuf[PU_NUM * MAX_READ_LEN * READ_BLOCK_SIZE / 8];
#pragma HLS BIND_STORAGE variable = readBurstBuf type = ram_t2p impl = uram
    readType readFeedBuf[PU_NUM][MAX_READ_LEN * READ_BLOCK_SIZE];
#pragma HLS BIND_STORAGE variable = readFeedBuf type = ram_t2p impl = uram
#pragma HLS ARRAY_PARTITION variable = readFeedBuf complete dim = 1
    ap_uint<512> readInfoBurstBuf[MAX_RSDATA_NUM / READ_BLOCK_SIZE / 8];
#pragma HLS BIND_STORAGE variable = readInfoBurstBuf type = ram_t2p impl = uram
    readType readInfoFeedBuf[PU_NUM][MAX_RSDATA_NUM / READ_BLOCK_SIZE / PU_NUM + 1];
#pragma HLS ARRAY_PARTITION variable = readInfoFeedBuf complete dim = 1
#pragma HLS BIND_STORAGE variable = readInfoFeedBuf type = ram_t2p impl = uram

    int info_num = numRead / (READ_BLOCK_SIZE * 8) + (numRead % (READ_BLOCK_SIZE * 8) != 0);
read_info_burst:
    for (int i = 0; i < info_num; i++) {
        readInfoBurstBuf[i] = input_data[info_addr++];
    }
    info_num = numRead / READ_BLOCK_SIZE + (numRead % READ_BLOCK_SIZE != 0);

    int info_offset = 0;
    int batch_count = 0;
    ap_uint<512> infoTmp = readInfoBurstBuf[info_offset];
read_info_distribute:
    for (int i = 0; i < info_num; i++) {
        ap_uint<3> imod8 = i & 7;
        readInfoFeedBuf[PU_id][batch_count] = infoTmp(63, 0);
        if (PU_id == PU_NUM - 1) {
            batch_count++;
            PU_id = 0;
        } else {
            PU_id++;
        }
        if (imod8 == 7) {
            info_offset++;
            infoTmp = readInfoBurstBuf[info_offset];
        } else {
            infoTmp = infoTmp >> 64;
        }
    }

    int read_addr = READ_DATA_OFFSET;
    batch_count = 0;
    for (int i = 0; i < numRead; i += READ_BLOCK_SIZE * PU_NUM) {
        int tile_size = i + READ_BLOCK_SIZE * PU_NUM < numRead ? READ_BLOCK_SIZE * PU_NUM : numRead - i;
        if (tile_size % 2 == 1) tile_size += 1;
        int burst_size = (tile_size * MAX_READ_LEN / 8);

    read_data_burst:
        memcpy(&readBurstBuf[0], &input_data[read_addr], READ_BLOCK_SIZE * PU_NUM * MAX_READ_LEN * 8);
        read_addr += READ_BLOCK_SIZE * PU_NUM * MAX_READ_LEN / 8;
        // for (int j = 0; j < READ_BLOCK_SIZE * PU_NUM * MAX_READ_LEN / 8; j++) {
        //   readBurstBuf[j] = input_data[read_addr++];
        //}
        int pu_id = 0;
        int read_offset = 0;
    read_data_distribute:
        for (int j = 0; j < burst_size; j++) {
            ap_uint<512> readBurstTmp = readBurstBuf[j];
        read_data_distribute_inner:
            for (int k = 0; k < 8; k++) {
#pragma HLS PIPELINE
                readFeedBuf[pu_id][read_offset] = readBurstTmp(63, 0);
                readBurstTmp = readBurstTmp >> 64;
                if (read_offset == MAX_READ_LEN * READ_BLOCK_SIZE - 1) {
                    read_offset = 0;
                    pu_id++;
                } else
                    read_offset++;
            }
        }
        if (read_offset != 0 && burst_size != 0) pu_id++;
    read_data_dispatch:
        for (int j = -1; j < MAX_READ_LEN * READ_BLOCK_SIZE; j++) {
#pragma HLS PIPELINE
            for (int k = 0; k < PU_NUM; k++) {
#pragma HLS UNROLL
                if (k < pu_id) {
                    readType tmp;
                    if (j == -1) {
                        tmp = readInfoFeedBuf[k][batch_count];
                    } else {
                        tmp = readFeedBuf[k][j];
                    }
                    readData[k].write(tmp);
                }
            }
        }
        batch_count++;
    }
    /*
        ap_uint<512> infoBuf = input_data[info_addr];
        for(int i = 0; i < numRead; i += READ_BLOCK_SIZE){
            ap_uint<512> readBuf = input_data[addr];
            ap_uint<3> imod8 = (i / READ_BLOCK_SIZE) & 7;
            for(short j = -1; j < MAX_READ_LEN * READ_BLOCK_SIZE; j++){
                readType readTmp;
                if(j == -1){
                    readTmp = infoBuf(63, 0);
                    if (numRead == 9 && PU_id == 4) {
                        printf("j = %d, readInfo = %lx\n", j, (long)readTmp);
                    }
                }
                else{
                    ap_uint<3> jmod8 = j & 0x7;
                    readTmp = readBuf(63, 0);
                    if(jmod8 == 7){
                        addr++;
                        readBuf = input_data[addr];
                    }
                    else
                        readBuf = readBuf >> 64;
                    if (numRead == 9 && PU_id == 4) {
                        printf("j = %d, readTmp = %lx\n", j, (long)readTmp);
                    }
                }
                readData[PU_id].write(readTmp);
            }
            if(imod8 == 7){
                info_addr++;
                infoBuf = input_data[info_addr];
            }
            else
                infoBuf = infoBuf >> 64;
            PU_id = (PU_id == PU_NUM - 1) ? 0 : PU_id + 1;
        }*/
}

/**
 * @brief PairHMM module to compute the likelihood.
 *
 * @tparam PU_NUM number of process engines in grid
 *
 * @param input_data input read/ref data
 * @param numRead number of read sequence data
 * @param numHap number of hap sequence data
 * @param iterNum number of iterations
 * @param output_data output data
 */
template <unsigned int PU_NUM>
void computePairhmm(ap_uint<512>* input_data, short numRead, short numHap, int iterNum, float* output_data) {
#pragma HLS inline off
#pragma HLS dataflow
    hls::stream<accData> output_fifo[PU_NUM];
#pragma HLS stream variable = output_fifo depth = 8
#pragma HLS aggregate variable = output_fifo
    hls::stream<accDataBundle> acc_fifo[PU_NUM];
#pragma HLS stream variable = acc_fifo depth = 4
    hls::stream<hapLenType> hapLen[PU_NUM];
#pragma HLS stream variable = hapLen depth = 4
#pragma HLS aggregate variable = hapLen
    hls::stream<readType> readData[PU_NUM];
#pragma HLS stream variable = readData depth = read_fifo_depth
    hls::stream<hapTypeBundle> haplotypeBases[PU_NUM];
#pragma HLS stream variable = haplotypeBases depth = 4
    hls::stream<short> max_cols[PU_NUM];
#pragma HLS stream variable = max_cols depth = 2
    hls::stream<short> numReadsCh[PU_NUM];
#pragma HLS stream variable = numReadsCh depth = 2
    hls::stream<int> numResultsCh[PU_NUM];
#pragma HLS stream variable = numResultsCh depth = 2
    hls::stream<longlong> totalIterNumCh[PU_NUM];
#pragma HLS stream variable = totalIterNumCh depth = 2
    dispatcher<PU_NUM>(input_data, numRead, numHap, readData, hapLen, haplotypeBases, max_cols, numReadsCh,
                       numResultsCh, totalIterNumCh);
accEngine:
    for (int idx = 0; idx < PU_NUM; idx++) {
#pragma HLS UNROLL
        computeEngine(numHap, readData[idx], hapLen[idx], haplotypeBases[idx], acc_fifo[idx], max_cols[idx],
                      numReadsCh[idx], totalIterNumCh[idx]);
        accEngine(acc_fifo[idx], output_fifo[idx], numResultsCh[idx]);
    }
    accBusEngine<PU_NUM>(iterNum, output_fifo, output_data);
}

template <unsigned int PU_NUM>
void pmm_core(ap_uint<512>* input_data, int numRead, int numHap, float* output_data) {
#pragma HLS inline off
    int numIters = numRead * numHap;
    // printf("numIters %d, PU_NUM %d \n", numIters, PU_NUM);
    computePairhmm<PU_NUM>(input_data, numRead, numHap, numIters, output_data);
}
}
}
#endif
