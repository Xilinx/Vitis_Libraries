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
 * @file shortest_path.hpp
 *
 */

#ifndef __XF_GRAPH_SHORTESTPATH_HPP_
#define __XF_GRAPH_SHORTESTPATH_HPP_

#include "ap_int.h"
#include "hls_stream.h"
#include <stdint.h>

namespace xf {
namespace graph {
namespace internal {
namespace shortest_path {

template <typename MType>
union f_cast;

template <>
union f_cast<ap_uint<32> > {
    uint32_t f;
    uint32_t i;
};

template <>
union f_cast<ap_uint<64> > {
    uint64_t f;
    uint64_t i;
};

template <>
union f_cast<int> {
    int f;
    int i;
};

template <>
union f_cast<unsigned int> {
    unsigned int f;
    unsigned int i;
};

template <>
union f_cast<double> {
    double f;
    uint64_t i;
};

template <>
union f_cast<float> {
    float f;
    uint32_t i;
};

static bool que_valid;
static ap_uint<512> que_reg;
static ap_uint<32> que_addr;

static bool offset_valid;
static ap_uint<512> offset_reg;
static ap_uint<32> offset_addr;

static bool column_valid;
static ap_uint<512> column_reg;
static ap_uint<32> column_addr;

static bool weight_valid;
static ap_uint<512> weight_reg;
static ap_uint<32> weight_addr;

static bool result_valid;
static ap_uint<512> result_reg;
static ap_uint<32> result_addr;

inline void queCtrl(ap_uint<32> qcfg,
                    ap_uint<32> queStatus[7],
                    ap_uint<512>* ddrQue,

                    hls::stream<ap_uint<32> >& fromIDStrm,
                    hls::stream<bool>& ctrlStrm) {
    ap_uint<32> fromID;
    ap_uint<64> que_rd_cnt;
    ap_uint<64> que_wr_cnt;
    ap_uint<32> que_rd_ptr;

    que_rd_cnt = queStatus[1].concat(queStatus[0]);
    que_wr_cnt = queStatus[3].concat(queStatus[2]);
    que_rd_ptr = queStatus[4];

    if (que_rd_cnt != que_wr_cnt) {
        int que_rd_ptrH = que_rd_ptr.range(31, 4);
        int que_rd_ptrL = que_rd_ptr.range(3, 0);
        if (que_valid == 0 || que_rd_ptrH != que_addr) {
            que_reg = ddrQue[que_rd_ptrH];
            que_valid = 1;
            que_addr = que_rd_ptrH;
        }
        fromID = que_reg.range(32 * (que_rd_ptrL + 1) - 1, 32 * que_rd_ptrL);
        que_rd_cnt++;
        if (que_rd_ptr != qcfg - 1)
            que_rd_ptr++;
        else
            que_rd_ptr = 0;
        fromIDStrm.write(fromID);
        ctrlStrm.write(0);
    } else {
        fromIDStrm.write(0);
        ctrlStrm.write(1);
    }

    queStatus[0] = que_rd_cnt.range(31, 0);
    queStatus[1] = que_rd_cnt.range(63, 32);
    queStatus[4] = que_rd_ptr;
}

inline void loadoffsetSendAddr(hls::stream<ap_uint<32> >& fromIDInStrm,
                               hls::stream<bool>& ctrlInStrm,

                               hls::stream<ap_uint<32> >& fromIDOutStrm,
                               hls::stream<ap_uint<32> >& offsetAddrStrm,
                               hls::stream<bool>& ctrlOutStrm) {
    ap_uint<32> fromID;
    bool ctrl;

    fromID = fromIDInStrm.read();
    ctrl = ctrlInStrm.read();

    fromIDOutStrm.write(fromID);

    if (ctrl == 0) {
        offsetAddrStrm.write(fromID);
        ctrlOutStrm.write(0);
        offsetAddrStrm.write(fromID + 1);
        ctrlOutStrm.write(0);
    }
    ctrlOutStrm.write(1);
}

inline void readOffset(hls::stream<ap_uint<32> >& raddrStrm,
                       hls::stream<bool>& eRaddrStrm,
                       hls::stream<ap_uint<32> >& rdataStrm,
                       hls::stream<bool>& eRdataStrm,
                       ap_uint<512>* ddrMem) {
    bool e = eRaddrStrm.read();

    while (!e) {
#pragma HLS pipeline II = 1
        ap_uint<32> raddr = raddrStrm.read();
        int raddrH = raddr.range(31, 4);
        int raddrL = raddr.range(3, 0);
        if (offset_valid == 0 || raddrH != offset_addr) {
            offset_valid = 1;
            offset_addr = raddrH;
            offset_reg = ddrMem[raddrH];
        }
        rdataStrm.write(offset_reg.range(32 * (raddrL + 1) - 1, 32 * raddrL));
        eRdataStrm.write(0);
        e = eRaddrStrm.read();
    }
    eRdataStrm.write(1);
}

inline void loadoffsetCollect(hls::stream<ap_uint<32> >& fromIDInStrm,
                              hls::stream<ap_uint<32> >& offsetDataStrm,
                              hls::stream<bool>& ctrlInStrm,

                              hls::stream<ap_uint<32> >& fromIDOutStrm,
                              hls::stream<ap_uint<32> >& offsetLowStrm,
                              hls::stream<ap_uint<32> >& offsetHighStrm,
                              hls::stream<bool>& ctrlOutStrm) {
    ap_uint<32> fromID;
    bool ctrl;
    fromIDOutStrm.write(fromIDInStrm.read());

    ctrl = ctrlInStrm.read();
    if (ctrl == 0) {
        offsetLowStrm.write(offsetDataStrm.read());
        ctrlInStrm.read();
        offsetHighStrm.write(offsetDataStrm.read());
        ctrlInStrm.read();
        ctrlOutStrm.write(0);
    } else {
        offsetLowStrm.write(0);
        offsetHighStrm.write(0);
        ctrlOutStrm.write(1);
    }
}

inline void loadToIDWeiSendAddr(hls::stream<ap_uint<32> >& fromIDInStrm,
                                hls::stream<ap_uint<32> >& offsetLowStrm,
                                hls::stream<ap_uint<32> >& offsetHighStrm,
                                hls::stream<bool>& ctrlInStrm,

                                hls::stream<ap_uint<32> >& fromIDOutStrm,
                                hls::stream<ap_uint<32> >& columnAddrStrm,
                                hls::stream<bool>& columnCtrlStrm,
                                hls::stream<ap_uint<32> >& weightAddrStrm,
                                hls::stream<bool>& weightCtrlStrm) {
    ap_uint<32> fromID;
    bool ctrl;
    ap_uint<32> offsetLow;
    ap_uint<32> offsetHigh;

    fromID = fromIDInStrm.read();
    offsetLow = offsetLowStrm.read();
    offsetHigh = offsetHighStrm.read();
    ctrl = ctrlInStrm.read();
    fromIDOutStrm.write(fromID);

    if (ctrl == 0) {
        for (ap_uint<32> i = offsetLow; i < offsetHigh; i++) {
#pragma HLS PIPELINE II = 1
            columnAddrStrm.write(i);
            columnCtrlStrm.write(0);
            weightAddrStrm.write(i);
            weightCtrlStrm.write(0);
        }
    }
    columnCtrlStrm.write(1);
    weightCtrlStrm.write(1);
}

inline void loadToIDWeiSendAddr(hls::stream<ap_uint<32> >& fromIDInStrm,
                                hls::stream<ap_uint<32> >& offsetLowStrm,
                                hls::stream<ap_uint<32> >& offsetHighStrm,
                                hls::stream<bool>& ctrlInStrm,

                                hls::stream<ap_uint<32> >& fromIDOutStrm,
                                hls::stream<ap_uint<32> >& columnAddrStrm,
                                hls::stream<bool>& columnCtrlStrm) {
    ap_uint<32> fromID;
    bool ctrl;
    ap_uint<32> offsetLow;
    ap_uint<32> offsetHigh;

    fromID = fromIDInStrm.read();
    offsetLow = offsetLowStrm.read();
    offsetHigh = offsetHighStrm.read();
    ctrl = ctrlInStrm.read();
    fromIDOutStrm.write(fromID);

    if (ctrl == 0) {
        for (ap_uint<32> i = offsetLow; i < offsetHigh; i++) {
#pragma HLS PIPELINE II = 1
            columnAddrStrm.write(i);
            columnCtrlStrm.write(0);
        }
    }
    columnCtrlStrm.write(1);
}

inline void readColumn(hls::stream<ap_uint<32> >& raddrStrm,
                       hls::stream<bool>& eRaddrStrm,
                       hls::stream<ap_uint<32> >& rdataStrm,
                       hls::stream<bool>& eRdataStrm,
                       ap_uint<512>* ddrMem) {
    bool e = eRaddrStrm.read();

    while (!e) {
#pragma HLS pipeline II = 1
        ap_uint<32> raddr = raddrStrm.read();
        int raddrH = raddr.range(31, 4);
        int raddrL = raddr.range(3, 0);
        if (column_valid == 0 || raddrH != column_addr) {
            column_valid = 1;
            column_addr = raddrH;
            column_reg = ddrMem[raddrH];
        }
        rdataStrm.write(column_reg.range(32 * (raddrL + 1) - 1, 32 * raddrL));
        eRdataStrm.write(0);
        e = eRaddrStrm.read();
    }
    eRdataStrm.write(1);
}

template <typename WType>
inline void readWeight(bool enable,
                       hls::stream<ap_uint<32> >& raddrStrm,
                       hls::stream<bool>& eRaddrStrm,
                       hls::stream<WType>& rdataStrm,
                       hls::stream<bool>& eRdataStrm,
                       ap_uint<512>* ddrMem) {
    bool e = eRaddrStrm.read();

    while (!e) {
#pragma HLS pipeline II = 1
        ap_uint<32> raddr = raddrStrm.read();
        if (sizeof(WType) == 4) {
            if (enable) {
                int raddrH = raddr.range(31, 4);
                int raddrL = raddr.range(3, 0);
                if (weight_valid == 0 || raddrH != weight_addr) {
                    weight_valid = 1;
                    weight_addr = raddrH;
                    weight_reg = ddrMem[raddrH];
                }
                f_cast<WType> tmp;
                tmp.i = weight_reg.range(32 * (raddrL + 1) - 1, 32 * raddrL);
                rdataStrm.write(tmp.f);
            } else {
                f_cast<WType> tmp;
                tmp.f = 1.0;
                rdataStrm.write(tmp.f);
            }
        } else if (sizeof(WType) == 8) {
            if (enable) {
                int raddrH = raddr.range(31, 3);
                int raddrL = raddr.range(2, 0);
                if (weight_valid == 0 || raddrH != weight_addr) {
                    weight_valid = 1;
                    weight_addr = raddrH;
                    weight_reg = ddrMem[raddrH];
                }
                f_cast<WType> tmp;
                tmp.i = weight_reg.range(64 * (raddrL + 1) - 1, 64 * raddrL);
                rdataStrm.write(tmp.f);
            } else {
                rdataStrm.write((WType)1.0);
            }
        } else {
        }
        eRdataStrm.write(0);
        e = eRaddrStrm.read();
    }
    eRdataStrm.write(1);
}

template <typename WType>
void loadResSendAddr(hls::stream<ap_uint<32> >& fromIDInStrm,
                     hls::stream<ap_uint<32> >& columnDataStrm,
                     hls::stream<bool>& columnCtrlStrm,
                     hls::stream<WType>& weightDataStrm,
                     hls::stream<bool>& weightCtrlStrm,

                     hls::stream<ap_uint<32> >& resAddrStrm,
                     hls::stream<ap_uint<32> >& toIDOutStrm,
                     hls::stream<WType>& weiOutStrm,
                     hls::stream<bool>& ctrlOutStrm) {
    bool ctrl;

    resAddrStrm.write(fromIDInStrm.read());
    ctrlOutStrm.write(0);

    ctrl = columnCtrlStrm.read();
    weightCtrlStrm.read();

    while (!ctrl) {
        ap_uint<32> toID = columnDataStrm.read();
        resAddrStrm.write(toID);
        toIDOutStrm.write(toID);
        weiOutStrm.write(weightDataStrm.read());
        ctrlOutStrm.write(0);
        ctrl = columnCtrlStrm.read();
        weightCtrlStrm.read();
    }
    ctrlOutStrm.write(1);
}

inline void loadResSendAddr(hls::stream<ap_uint<32> >& fromIDStrm,
                            hls::stream<ap_uint<32> >& toIDStrm,
                            hls::stream<bool>& ctrlInStrm,

                            hls::stream<ap_uint<32> >& resAddrStrm,
                            hls::stream<ap_uint<32> >& toIDOutStrm,
                            hls::stream<bool>& ctrlOutStrm) {
    bool ctrl;

    resAddrStrm.write(fromIDStrm.read());
    ctrlOutStrm.write(0);

    ctrl = ctrlInStrm.read();
    while (!ctrl) {
        ap_uint<32> toID = toIDStrm.read();
        resAddrStrm.write(toID);
        toIDOutStrm.write(toID);
        ctrlOutStrm.write(0);
        ctrl = ctrlInStrm.read();
    }
    ctrlOutStrm.write(1);
}

template <typename WType>
void readResult(hls::stream<ap_uint<32> >& raddrStrm,
                hls::stream<bool>& eRaddrStrm,
                hls::stream<WType>& rdataStrm,
                hls::stream<bool>& eRdataStrm,
                ap_uint<512>* ddrMem) {
    bool e = eRaddrStrm.read();

    while (!e) {
#pragma HLS pipeline II = 1
        ap_uint<32> raddr = raddrStrm.read();
        if (sizeof(WType) == 4) {
            int raddrH = raddr.range(31, 4);
            int raddrL = raddr.range(3, 0);
            if (result_valid == 0 || raddrH != result_addr) {
                result_valid = 1;
                result_addr = raddrH;
                result_reg = ddrMem[raddrH];
            }
            f_cast<WType> tmp;
            tmp.i = result_reg.range(32 * (raddrL + 1) - 1, 32 * raddrL);
            rdataStrm.write(tmp.f);
        } else if (sizeof(WType) == 8) {
            int raddrH = raddr.range(31, 3);
            int raddrL = raddr.range(2, 0);
            if (result_valid == 0 || raddrH != result_addr) {
                result_valid = 1;
                result_addr = raddrH;
                result_reg = ddrMem[raddrH];
            }
            f_cast<WType> tmp;
            tmp.i = result_reg.range(64 * (raddrL + 1) - 1, 64 * raddrL);
            rdataStrm.write(tmp.f);
        } else {
        }
        eRdataStrm.write(0);
        e = eRaddrStrm.read();
    }
    eRdataStrm.write(1);
}

template <typename WType>
void loadResCollectData(hls::stream<ap_uint<32> >& dataStrm,
                        hls::stream<ap_uint<32> >& toIDStrm,
                        hls::stream<WType>& weiStrm,
                        hls::stream<bool>& ctrlInStrm,

                        hls::stream<ap_uint<32> >& toIDOutStrm,
                        hls::stream<WType>& updatestrm,
                        hls::stream<bool>& ctrlOutStrm) {
    bool ctrl;
    f_cast<WType> tmp0;
    tmp0.i = dataStrm.read();
    WType fromDist = tmp0.f;
    ctrl = ctrlInStrm.read();

    ctrl = ctrlInStrm.read();
    while (!ctrl) {
        f_cast<WType> tmp1;
        tmp1.i = dataStrm.read();
        WType toDist = tmp1.f;
        WType curDist = fromDist + weiStrm.read();
        if (fromDist != -1 && curDist < toDist) {
            toIDOutStrm.write(toIDStrm.read());
            updatestrm.write(curDist);
            ctrlOutStrm.write(0);
        } else {
            toIDStrm.read();
        }

        ctrl = ctrlInStrm.read();
    }
    ctrlOutStrm.write(1);
}

template <typename WType>
void loadResCollectData(hls::stream<ap_uint<32> >& dataStrm,
                        hls::stream<ap_uint<32> >& toIDStrm,
                        hls::stream<bool>& ctrlInStrm,

                        hls::stream<ap_uint<32> >& toIDOutStrm,
                        hls::stream<WType>& updatestrm,
                        hls::stream<bool>& ctrlOutStrm) {
    bool ctrl;
    f_cast<WType> tmp0;
    tmp0.i = dataStrm.read();
    WType fromDist = tmp0.f;
    ctrl = ctrlInStrm.read();

    ctrl = ctrlInStrm.read();
    while (!ctrl) {
        f_cast<WType> tmp1;
        tmp1.i = dataStrm.read();
        WType toDist = tmp1.f;
        WType curDist = fromDist + 1;
        if (fromDist != -1 && curDist < toDist) {
            toIDOutStrm.write(toIDStrm.read());
            updatestrm.write(curDist);
            ctrlOutStrm.write(0);
        } else {
            toIDStrm.read();
        }

        ctrl = ctrlInStrm.read();
    }
    ctrlOutStrm.write(1);
}

template <typename WType, int MAXOUTDEGREE>
void storeUpdate(hls::stream<ap_uint<32> >& toIDStrm,
                 hls::stream<WType>& toIDWeiStrm,
                 hls::stream<bool>& ctrlInStrm,

                 ap_uint<32> tableStatus[2],
                 ap_uint<32>* toIDTable,
                 WType* distTable) {
    ap_uint<32> toID;
    WType toIDWei;
    bool ctrl;
    ap_uint<32> cnt = 0;

    ctrl = ctrlInStrm.read();
    while (!ctrl) {
#pragma HLS PIPELINE II = 1
        if (cnt < MAXOUTDEGREE) {
            toIDTable[cnt] = toIDStrm.read();
            distTable[cnt] = toIDWeiStrm.read();
            ctrl = ctrlInStrm.read();
        } else {
            toIDStrm.read();
            toIDWeiStrm.read();
            ctrlInStrm.read();
        }
        cnt++;
    }
    tableStatus[0] = cnt;
}

template <typename WType, int MAXOUTDEGREE>
void shortestPathS1Dataflow(bool enaWei,
                            ap_uint<32> qcfg,
                            ap_uint<32> queStatus[7],
                            ap_uint<512>* ddrQue,
                            ap_uint<512>* offset,
                            ap_uint<512>* column,
                            ap_uint<512>* weight,
                            ap_uint<512>* result512,
                            ap_uint<32> tableStatus[1],
                            ap_uint<32>* toIDTable,
                            WType* distTable) {
#pragma HLS dataflow
    hls::stream<ap_uint<32> > fromIDQueStrm;
#pragma HLS stream variable = fromIDQueStrm depth = 32
#pragma HLS resource variable = fromIDQueStrm core = FIFO_LUTRAM

    hls::stream<bool> ctrlQueStrm;
#pragma HLS stream variable = ctrlQueStrm depth = 32
#pragma HLS resource variable = ctrlQueStrm core = FIFO_LUTRAM

    hls::stream<ap_uint<32> > fromIDStrm2;
#pragma HLS stream variable = fromIDStrm2 depth = 32
#pragma HLS resource variable = fromIDStrm2 core = FIFO_LUTRAM

    hls::stream<ap_uint<32> > offsetLowStrm;
#pragma HLS stream variable = offsetLowStrm depth = 32
#pragma HLS resource variable = offsetLowStrm core = FIFO_LUTRAM

    hls::stream<ap_uint<32> > offsetHighStrm;
#pragma HLS stream variable = offsetHighStrm depth = 32
#pragma HLS resource variable = offsetHighStrm core = FIFO_LUTRAM

    hls::stream<bool> ctrlStrm2;
#pragma HLS stream variable = ctrlStrm2 depth = 32
#pragma HLS resource variable = ctrlStrm2 core = FIFO_LUTRAM

    hls::stream<ap_uint<32> > fromIDStrm3;
#pragma HLS stream variable = fromIDStrm3 depth = 32
#pragma HLS resource variable = fromIDStrm3 core = FIFO_LUTRAM

    hls::stream<ap_uint<32> > toIDStrm1;
#pragma HLS stream variable = toIDStrm1 depth = 1024
#pragma HLS resource variable = toIDStrm1 core = FIFO_BRAM

    hls::stream<WType> weiStrm1;
#pragma HLS stream variable = weiStrm1 depth = 1024
#pragma HLS resource variable = weiStrm1 core = FIFO_BRAM

    hls::stream<bool> ctrlStrm3;
#pragma HLS stream variable = ctrlStrm3 depth = 32
#pragma HLS resource variable = ctrlStrm3 core = FIFO_LUTRAM

    hls::stream<ap_uint<32> > toIDStrm2;
#pragma HLS stream variable = toIDStrm2 depth = 32
#pragma HLS resource variable = toIDStrm2 core = FIFO_LUTRAM

    hls::stream<WType> updatestrm;
#pragma HLS stream variable = updatestrm depth = 32
#pragma HLS resource variable = updatestrm core = FIFO_LUTRAM

    hls::stream<bool> ctrlStrm4;
#pragma HLS stream variable = ctrlStrm4 depth = 32
#pragma HLS resource variable = ctrlStrm4 core = FIFO_LUTRAM

    hls::stream<ap_uint<32> > resAddrStrm;
#pragma HLS stream variable = resAddrStrm depth = 32
#pragma HLS resource variable = resAddrStrm core = FIFO_LUTRAM

    hls::stream<ap_uint<32> > resDataStrm;
#pragma HLS stream variable = resDataStrm depth = 32
#pragma HLS resource variable = resDataStrm core = FIFO_LUTRAM

    hls::stream<ap_uint<32> > toIDStrm3;
#pragma HLS stream variable = toIDStrm3 depth = 1024
#pragma HLS resource variable = toIDStrm3 core = FIFO_BRAM

    hls::stream<WType> weiStrm2;
#pragma HLS stream variable = weiStrm2 depth = 1024
#pragma HLS resource variable = weiStrm2 core = FIFO_BRAM

    hls::stream<bool> ctrlStrm5;
#pragma HLS stream variable = ctrlStrm5 depth = 32
#pragma HLS resource variable = ctrlStrm5 core = FIFO_LUTRAM

    hls::stream<bool> ctrlStrm6;
#pragma HLS stream variable = ctrlStrm6 depth = 32
#pragma HLS resource variable = ctrlStrm6 core = FIFO_LUTRAM

    hls::stream<ap_uint<32> > fromIDoffsetStrm;
#pragma HLS stream variable = fromIDoffsetStrm depth = 32
#pragma HLS resource variable = fromIDoffsetStrm core = FIFO_LUTRAM

    hls::stream<ap_uint<32> > offsetAddrStrm;
#pragma HLS stream variable = offsetAddrStrm depth = 32
#pragma HLS resource variable = offsetAddrStrm core = FIFO_LUTRAM

    hls::stream<bool> offsetAddrCtrlInStrm;
#pragma HLS stream variable = offsetAddrCtrlInStrm depth = 32
#pragma HLS resource variable = offsetAddrCtrlInStrm core = FIFO_LUTRAM

    hls::stream<ap_uint<32> > offsetDataStrm;
#pragma HLS stream variable = offsetDataStrm depth = 32
#pragma HLS resource variable = offsetDataStrm core = FIFO_LUTRAM

    hls::stream<bool> offsetDataCtrlInStrm;
#pragma HLS stream variable = offsetDataCtrlInStrm depth = 32
#pragma HLS resource variable = offsetDataCtrlInStrm core = FIFO_LUTRAM

    hls::stream<ap_uint<32> > fromIDweiStrm;
#pragma HLS stream variable = fromIDweiStrm depth = 32
#pragma HLS resource variable = fromIDweiStrm core = FIFO_LUTRAM

    hls::stream<ap_uint<32> > columnAddrStrm;
#pragma HLS stream variable = columnAddrStrm depth = 32
#pragma HLS resource variable = columnAddrStrm core = FIFO_LUTRAM

    hls::stream<bool> columnCtrlAddrStrm;
#pragma HLS stream variable = columnCtrlAddrStrm depth = 32
#pragma HLS resource variable = columnCtrlAddrStrm core = FIFO_LUTRAM

    hls::stream<ap_uint<32> > columnDataStrm;
#pragma HLS stream variable = columnDataStrm depth = 1024
#pragma HLS resource variable = columnDataStrm core = FIFO_BRAM

    hls::stream<bool> columnCtrlDataStrm;
#pragma HLS stream variable = columnCtrlDataStrm depth = 1024
#pragma HLS resource variable = columnCtrlDataStrm core = FIFO_BRAM

    hls::stream<ap_uint<32> > weightAddrStrm;
#pragma HLS stream variable = weightAddrStrm depth = 32
#pragma HLS resource variable = weightAddrStrm core = FIFO_LUTRAM

    hls::stream<bool> weightCtrlAddrStrm;
#pragma HLS stream variable = weightCtrlAddrStrm depth = 32
#pragma HLS resource variable = weightCtrlAddrStrm core = FIFO_LUTRAM

    hls::stream<WType> weightDataStrm;
#pragma HLS stream variable = weightDataStrm depth = 1024
#pragma HLS resource variable = weightDataStrm core = FIFO_BRAM

    hls::stream<bool> weightCtrlDataStrm;
#pragma HLS stream variable = weightCtrlDataStrm depth = 1024
#pragma HLS resource variable = weightCtrlDataStrm core = FIFO_BRAM

    queCtrl(qcfg, queStatus, ddrQue, fromIDQueStrm, ctrlQueStrm);

    loadoffsetSendAddr(fromIDQueStrm, ctrlQueStrm,

                       fromIDoffsetStrm, offsetAddrStrm, offsetAddrCtrlInStrm);

    readOffset(offsetAddrStrm, offsetAddrCtrlInStrm, offsetDataStrm, offsetDataCtrlInStrm, offset);

    loadoffsetCollect(fromIDoffsetStrm, offsetDataStrm, offsetDataCtrlInStrm,

                      fromIDStrm2, offsetLowStrm, offsetHighStrm, ctrlStrm2);

    loadToIDWeiSendAddr(fromIDStrm2, offsetLowStrm, offsetHighStrm, ctrlStrm2,

                        fromIDweiStrm, columnAddrStrm, columnCtrlAddrStrm, weightAddrStrm, weightCtrlAddrStrm);

    readColumn(columnAddrStrm, columnCtrlAddrStrm, columnDataStrm, columnCtrlDataStrm, column);

    readWeight(enaWei, weightAddrStrm, weightCtrlAddrStrm, weightDataStrm, weightCtrlDataStrm, weight);

    loadResSendAddr(fromIDweiStrm, columnDataStrm, columnCtrlDataStrm, weightDataStrm, weightCtrlDataStrm,

                    resAddrStrm, toIDStrm3, weiStrm2, ctrlStrm5);

    readResult(resAddrStrm, ctrlStrm5, resDataStrm, ctrlStrm6, result512);

    loadResCollectData(resDataStrm, toIDStrm3, weiStrm2, ctrlStrm6,

                       toIDStrm2, updatestrm, ctrlStrm4);

    storeUpdate<WType, MAXOUTDEGREE>(toIDStrm2, updatestrm, ctrlStrm4, tableStatus, toIDTable, distTable);
}

template <typename WType, int MAXOUTDEGREE>
void shortestPathS1Dataflow(ap_uint<32> qcfg,
                            ap_uint<32> queStatus[7],
                            ap_uint<512>* ddrQue,
                            ap_uint<512>* offset,
                            ap_uint<512>* column,
                            ap_uint<512>* result512,
                            ap_uint<32> tableStatus[1],
                            ap_uint<32>* toIDTable,
                            WType* distTable) {
#pragma HLS dataflow
    hls::stream<ap_uint<32> > fromIDQueStrm;
#pragma HLS stream variable = fromIDQueStrm depth = 32
#pragma HLS resource variable = fromIDQueStrm core = FIFO_LUTRAM

    hls::stream<bool> ctrlQueStrm;
#pragma HLS stream variable = ctrlQueStrm depth = 32
#pragma HLS resource variable = ctrlQueStrm core = FIFO_LUTRAM

    hls::stream<ap_uint<32> > fromIDStrm2;
#pragma HLS stream variable = fromIDStrm2 depth = 32
#pragma HLS resource variable = fromIDStrm2 core = FIFO_LUTRAM

    hls::stream<ap_uint<32> > offsetLowStrm;
#pragma HLS stream variable = offsetLowStrm depth = 32
#pragma HLS resource variable = offsetLowStrm core = FIFO_LUTRAM

    hls::stream<ap_uint<32> > offsetHighStrm;
#pragma HLS stream variable = offsetHighStrm depth = 32
#pragma HLS resource variable = offsetHighStrm core = FIFO_LUTRAM

    hls::stream<bool> ctrlStrm2;
#pragma HLS stream variable = ctrlStrm2 depth = 32
#pragma HLS resource variable = ctrlStrm2 core = FIFO_LUTRAM

    hls::stream<ap_uint<32> > toIDStrm2;
#pragma HLS stream variable = toIDStrm2 depth = 32
#pragma HLS resource variable = toIDStrm2 core = FIFO_LUTRAM

    hls::stream<WType> updatestrm;
#pragma HLS stream variable = updatestrm depth = 32
#pragma HLS resource variable = updatestrm core = FIFO_LUTRAM

    hls::stream<bool> ctrlStrm4;
#pragma HLS stream variable = ctrlStrm4 depth = 32
#pragma HLS resource variable = ctrlStrm4 core = FIFO_LUTRAM

    hls::stream<ap_uint<32> > resAddrStrm;
#pragma HLS stream variable = resAddrStrm depth = 32
#pragma HLS resource variable = resAddrStrm core = FIFO_LUTRAM

    hls::stream<ap_uint<32> > resDataStrm;
#pragma HLS stream variable = resDataStrm depth = 32
#pragma HLS resource variable = resDataStrm core = FIFO_LUTRAM

    hls::stream<ap_uint<32> > toIDStrm3;
#pragma HLS stream variable = toIDStrm3 depth = 1024
#pragma HLS resource variable = toIDStrm3 core = FIFO_BRAM

    hls::stream<bool> ctrlStrm5;
#pragma HLS stream variable = ctrlStrm5 depth = 32
#pragma HLS resource variable = ctrlStrm5 core = FIFO_LUTRAM

    hls::stream<bool> ctrlStrm6;
#pragma HLS stream variable = ctrlStrm6 depth = 32
#pragma HLS resource variable = ctrlStrm6 core = FIFO_LUTRAM

    hls::stream<ap_uint<32> > fromIDoffsetStrm;
#pragma HLS stream variable = fromIDoffsetStrm depth = 32
#pragma HLS resource variable = fromIDoffsetStrm core = FIFO_LUTRAM

    hls::stream<ap_uint<32> > offsetAddrStrm;
#pragma HLS stream variable = offsetAddrStrm depth = 32
#pragma HLS resource variable = offsetAddrStrm core = FIFO_LUTRAM

    hls::stream<bool> offsetAddrCtrlInStrm;
#pragma HLS stream variable = offsetAddrCtrlInStrm depth = 32
#pragma HLS resource variable = offsetAddrCtrlInStrm core = FIFO_LUTRAM

    hls::stream<ap_uint<32> > offsetDataStrm;
#pragma HLS stream variable = offsetDataStrm depth = 32
#pragma HLS resource variable = offsetDataStrm core = FIFO_LUTRAM

    hls::stream<bool> offsetDataCtrlInStrm;
#pragma HLS stream variable = offsetDataCtrlInStrm depth = 32
#pragma HLS resource variable = offsetDataCtrlInStrm core = FIFO_LUTRAM

    hls::stream<ap_uint<32> > fromIDweiStrm;
#pragma HLS stream variable = fromIDweiStrm depth = 32
#pragma HLS resource variable = fromIDweiStrm core = FIFO_LUTRAM

    hls::stream<ap_uint<32> > columnAddrStrm;
#pragma HLS stream variable = columnAddrStrm depth = 32
#pragma HLS resource variable = columnAddrStrm core = FIFO_LUTRAM

    hls::stream<bool> columnCtrlAddrStrm;
#pragma HLS stream variable = columnCtrlAddrStrm depth = 32
#pragma HLS resource variable = columnCtrlAddrStrm core = FIFO_LUTRAM

    hls::stream<ap_uint<32> > columnDataStrm;
#pragma HLS stream variable = columnDataStrm depth = 32
#pragma HLS resource variable = columnDataStrm core = FIFO_LUTRAM

    hls::stream<bool> columnCtrlDataStrm;
#pragma HLS stream variable = columnCtrlDataStrm depth = 32
#pragma HLS resource variable = columnCtrlDataStrm core = FIFO_LUTRAM

    queCtrl(qcfg, queStatus, ddrQue, fromIDQueStrm, ctrlQueStrm);

    loadoffsetSendAddr(fromIDQueStrm, ctrlQueStrm,

                       fromIDoffsetStrm, offsetAddrStrm, offsetAddrCtrlInStrm);

    readOffset(offsetAddrStrm, offsetAddrCtrlInStrm, offsetDataStrm, offsetDataCtrlInStrm, offset);

    loadoffsetCollect(fromIDoffsetStrm, offsetDataStrm, offsetDataCtrlInStrm,

                      fromIDStrm2, offsetLowStrm, offsetHighStrm, ctrlStrm2);

    loadToIDWeiSendAddr(fromIDStrm2, offsetLowStrm, offsetHighStrm, ctrlStrm2,

                        fromIDweiStrm, columnAddrStrm, columnCtrlAddrStrm);

    readColumn(columnAddrStrm, columnCtrlAddrStrm, columnDataStrm, columnCtrlDataStrm, column);

    loadResSendAddr(fromIDweiStrm, columnDataStrm, columnCtrlDataStrm,

                    resAddrStrm, toIDStrm3, ctrlStrm5);

    readResult(resAddrStrm, ctrlStrm5, resDataStrm, ctrlStrm6, result512);

    loadResCollectData(resDataStrm, toIDStrm3, ctrlStrm6,

                       toIDStrm2, updatestrm, ctrlStrm4);

    storeUpdate<WType, MAXOUTDEGREE>(toIDStrm2, updatestrm, ctrlStrm4, tableStatus, toIDTable, distTable);
}

template <typename WType>
void updateDispatch(ap_uint<32> tableStatus[2],
                    ap_uint<32>* toIDTable,
                    WType* distTable,

                    hls::stream<ap_uint<32> >& resToIDStrm,
                    hls::stream<WType>& resDistStrm,
                    hls::stream<bool>& resCtrlStrm,
                    hls::stream<ap_uint<32> >& queToIDStrm,
                    hls::stream<bool>& queCtrlStrm) {
    ap_uint<32> tableCnt = tableStatus[0];
    if (tableCnt > 10 * 4096) tableStatus[1] = 1;
    for (int i = 0; i < tableCnt; i++) {
#pragma HLS PIPELINE II = 1
        if (i < 4096 * 10) {
            ap_uint<32> reg_toID = toIDTable[i];
            resToIDStrm.write(reg_toID);
            resDistStrm.write(distTable[i]);
            resCtrlStrm.write(0);
            queToIDStrm.write(reg_toID);
            queCtrlStrm.write(0);
        } else {
            resToIDStrm.write(0);
            resDistStrm.write(0);
            resCtrlStrm.write(0);
            queToIDStrm.write(0);
            queCtrlStrm.write(0);
        }
    }
    resCtrlStrm.write(1);
    queCtrlStrm.write(1);
}

template <typename WType>
void updateRes(hls::stream<ap_uint<32> >& resToIDStrm,
               hls::stream<WType>& resDistStrm,
               hls::stream<bool>& resCtrlStrm,

               WType* result) {
    ap_uint<32> validAddrQ[4] = {-1, -1, -1, -1};
#pragma HLS ARRAY_PARTITION variable = validAddrQ complete dim = 1
    ap_uint<64> validDataQ[4] = {0, 0, 0, 0};
#pragma HLS ARRAY_PARTITION variable = validDataQ complete dim = 1

    bool ctrl = resCtrlStrm.read();
    while (!ctrl) {
#pragma HLS PIPELINE II = 1
        ap_uint<32> toID = resToIDStrm.read();
        ap_uint<32> toID512 = toID.range(31, 4);

        if (toID512 == result_addr) result_valid = 0;

        result[toID] = resDistStrm.read();
        ctrl = resCtrlStrm.read();
    }
}

inline void upadateQue(ap_uint<32> qcfg,
                       hls::stream<ap_uint<32> >& queToIDStrm,
                       hls::stream<bool>& queCtrlStrm,

                       ap_uint<32> queStatus[7],
                       ap_uint<32>* ddrQue) {
    ap_uint<64> que_rd_cnt;
    ap_uint<64> que_wr_cnt;
    ap_uint<32> que_wr_ptr;

    que_rd_cnt = queStatus[1].concat(queStatus[0]);
    que_wr_cnt = queStatus[3].concat(queStatus[2]);
    que_wr_ptr = queStatus[5];

    bool ctrl;
    ctrl = queCtrlStrm.read();
    while (!ctrl) {
#pragma HLS PIPELINE II = 1
        ap_uint<32> toID = queToIDStrm.read();
        ddrQue[que_wr_ptr] = toID;
        int que_wr_ptrH = que_wr_ptr.range(31, 4);
        int que_wr_ptrL = que_wr_ptr.range(3, 0);
        if (que_wr_ptrH == que_addr) {
            que_valid = 0;
        }

        if (que_wr_ptr != qcfg - 1)
            que_wr_ptr++;
        else
            que_wr_ptr = 0;
        que_wr_cnt++;
        if (que_wr_cnt - que_rd_cnt > qcfg - 1) queStatus[6] = 1;
        ctrl = queCtrlStrm.read();
    }

    queStatus[2] = que_wr_cnt.range(31, 0);
    queStatus[3] = que_wr_cnt.range(63, 32);
    queStatus[5] = que_wr_ptr;
}

template <typename WType>
void shortestPathS2Dataflow(ap_uint<32> qcfg,
                            ap_uint<32> tableStatus[1],
                            ap_uint<32>* toIDTable,
                            WType* distTable,

                            WType* result,
                            ap_uint<32> queStatus[7],
                            ap_uint<32>* ddrQue) {
#pragma HLS dataflow
    hls::stream<ap_uint<32> > resToIDStrm;
#pragma HLS stream variable = resToIDStrm depth = 32
#pragma HLS resource variable = resToIDStrm core = FIFO_LUTRAM

    hls::stream<WType> resDistStrm;
#pragma HLS stream variable = resDistStrm depth = 32
#pragma HLS resource variable = resDistStrm core = FIFO_LUTRAM

    hls::stream<bool> resCtrlStrm;
#pragma HLS stream variable = resCtrlStrm depth = 32
#pragma HLS resource variable = resCtrlStrm core = FIFO_LUTRAM

    hls::stream<ap_uint<32> > queToIDStrm;
#pragma HLS stream variable = queToIDStrm depth = 32
#pragma HLS resource variable = queToIDStrm core = FIFO_LUTRAM

    hls::stream<bool> queCtrlStrm;
#pragma HLS stream variable = queCtrlStrm depth = 32
#pragma HLS resource variable = queCtrlStrm core = FIFO_LUTRAM

    updateDispatch(tableStatus, toIDTable, distTable, resToIDStrm, resDistStrm, resCtrlStrm, queToIDStrm, queCtrlStrm);
    updateRes(resToIDStrm, resDistStrm, resCtrlStrm, result);
    upadateQue(qcfg, queToIDStrm, queCtrlStrm, queStatus, ddrQue);
}

template <typename WType>
void initRes(ap_uint<32> numVertex, WType maxValue, ap_uint<32> sourceID, ap_uint<512>* result512) {
    ap_uint<512> max512;
    if (sizeof(WType) == 4) {
        for (int i = 0; i < 16; i++) {
#pragma HLS unroll
            max512.range((i + 1) * 32 - 1, i * 32) = maxValue;
        }

        for (int j = 0; j < numVertex.range(31, 10); j++) {
            for (int i = 0; i < 64; i++) {
#pragma HLS pipeline II = 1
                result512[j * 64 + i] = max512;
            }
        }

        int cnt = 0;
        if (numVertex.get_bit(9) == 1) {
            for (int i = 0; i < 32; i++) {
#pragma HLS pipeline II = 1
                result512[numVertex.range(31, 10) * 64 + cnt] = max512;
                cnt++;
            }
        }

        for (int i = 0; i < numVertex.range(8, 4); i++) {
#pragma HLS pipeline II = 1
            result512[numVertex.range(31, 10) * 64 + cnt] = max512;
            cnt++;
        }

        if (numVertex.range(3, 0) != 0) {
            result512[numVertex.range(31, 10) * 64 + cnt] = max512;
        }

        ap_uint<32> sourceIDH = sourceID(31, 4);
        ap_uint<32> sourceIDL = sourceID(3, 0);
        max512.range((sourceIDL + 1) * 32 - 1, sourceIDL * 32) = 0;
        result512[sourceIDH] = max512;

    } else if (sizeof(WType) == 8) {
        for (int i = 0; i < 8; i++) {
#pragma HLS unroll
            max512.range((i + 1) * 64 - 1, i * 64) = maxValue;
        }

        for (int j = 0; j < numVertex.range(31, 9); j++) {
            for (int i = 0; i < 64; i++) {
#pragma HLS pipeline II = 1
                result512[j * 64 + i] = max512;
            }
        }

        int cnt = 0;
        if (numVertex.get_bit(8) == 1) {
            for (int i = 0; i < 32; i++) {
#pragma HLS pipeline II = 1
                result512[numVertex.range(31, 9) * 64 + cnt] = max512;
                cnt++;
            }
        }

        for (int i = 0; i < numVertex.range(7, 3); i++) {
#pragma HLS pipeline II = 1
            result512[numVertex.range(31, 9) * 64 + cnt] = max512;
            cnt++;
        }

        if (numVertex.range(2, 0) != 0) {
            result512[numVertex.range(31, 9) * 64 + cnt] = max512;
        }

        ap_uint<32> sourceIDH = sourceID(31, 2);
        ap_uint<32> sourceIDL = sourceID(2, 0);
        max512.range((sourceIDL + 1) * 64 - 1, sourceIDL * 64) = 0;
        result512[sourceIDH] = max512;
    }
}

template <typename WType, int MAXOUTDEGREE>
void shortestPathInner(ap_uint<32>* config,
                       ap_uint<512>* offset,
                       ap_uint<512>* column,
                       ap_uint<512>* weight,

                       ap_uint<512>* ddrQue512,
                       ap_uint<32>* ddrQue,

                       ap_uint<512>* result512,
                       WType* result,
                       ap_uint<8>* info) {
    ap_uint<32> tableStatus[2];
#pragma HLS ARRAY_PARTITION variable = tableStatus complete dim = 1

#ifndef __SYNTHESIS__
    ap_uint<32>* toIDTable = (ap_uint<32>*)malloc(MAXOUTDEGREE * sizeof(ap_uint<32>));
    WType* distTable = (WType*)malloc(MAXOUTDEGREE * sizeof(WType));
#else
    ap_uint<32> toIDTable[MAXOUTDEGREE];
#pragma HLS RESOURCE variable = toIDTable core = RAM_S2P_URAM

    WType distTable[MAXOUTDEGREE];
#pragma HLS RESOURCE variable = distTable core = RAM_S2P_URAM
#endif

    offset_valid = 0;
    column_valid = 0;
    weight_valid = 0;
    result_valid = 0;
    que_valid = 0;

    tableStatus[0] = 0;
    tableStatus[1] = 0;

    ap_uint<32> queStatus[7];
#pragma HLS ARRAY_PARTITION variable = queStatus complete dim = 1

    queStatus[0] = 0;
    queStatus[1] = 0;
    queStatus[2] = 1;
    queStatus[3] = 0;
    queStatus[4] = 0;
    queStatus[5] = 1;
    queStatus[6] = 0;

    ap_uint<64> que_rd_cnt;
    ap_uint<64> que_wr_cnt;
    ap_uint<32> que_rd_ptr;

    que_rd_cnt = queStatus[1].concat(queStatus[0]);
    que_wr_cnt = queStatus[3].concat(queStatus[2]);

    ap_uint<32> sourceID = config[0];
    ddrQue[0] = sourceID;
    ap_uint<32> numVertex = config[1];
    WType maxValue = config[2];
    ap_uint<32> qcfg = config[3];
    bool enaWei = config[4];

    initRes(numVertex, maxValue, sourceID, result512);
    while (que_rd_cnt != que_wr_cnt) {
        shortestPathS1Dataflow<WType, MAXOUTDEGREE>(enaWei, qcfg, queStatus, ddrQue512, offset, column, weight,
                                                    result512, tableStatus, toIDTable, distTable);
        shortestPathS2Dataflow(qcfg, tableStatus, toIDTable, distTable, result, queStatus, ddrQue);

        que_rd_cnt = queStatus[1].concat(queStatus[0]);
        que_wr_cnt = queStatus[3].concat(queStatus[2]);
    }

    info[0] = queStatus[6];
    info[1] = tableStatus[1];
}

template <typename WType, int MAXOUTDEGREE>
void shortestPathInner(ap_uint<32>* config,
                       ap_uint<512>* offset,
                       ap_uint<512>* column,

                       ap_uint<512>* ddrQue512,
                       ap_uint<32>* ddrQue,

                       ap_uint<512>* result512,
                       WType* result,
                       ap_uint<8>* info) {
    ap_uint<32> tableStatus[2];
#pragma HLS ARRAY_PARTITION variable = tableStatus complete dim = 1

#ifndef __SYNTHESIS__
    ap_uint<32>* toIDTable = (ap_uint<32>*)malloc(MAXOUTDEGREE * sizeof(ap_uint<32>));
    WType* distTable = (WType*)malloc(MAXOUTDEGREE * sizeof(WType));
#else
    ap_uint<32> toIDTable[MAXOUTDEGREE];
#pragma HLS RESOURCE variable = toIDTable core = RAM_S2P_URAM

    WType distTable[MAXOUTDEGREE];
#pragma HLS RESOURCE variable = distTable core = RAM_S2P_URAM
#endif

    offset_valid = 0;
    column_valid = 0;
    result_valid = 0;
    que_valid = 0;

    tableStatus[0] = 0;
    tableStatus[1] = 0;

    ap_uint<32> queStatus[7];
#pragma HLS ARRAY_PARTITION variable = queStatus complete dim = 1

    queStatus[0] = 0;
    queStatus[1] = 0;
    queStatus[2] = 1;
    queStatus[3] = 0;
    queStatus[4] = 0;
    queStatus[5] = 1;
    queStatus[6] = 0;

    ap_uint<64> que_rd_cnt;
    ap_uint<64> que_wr_cnt;
    ap_uint<32> que_rd_ptr;

    que_rd_cnt = queStatus[1].concat(queStatus[0]);
    que_wr_cnt = queStatus[3].concat(queStatus[2]);

    ap_uint<32> sourceID = config[0];
    ddrQue[0] = sourceID;
    ap_uint<32> numVertex = config[1];
    WType maxValue = config[2];
    ap_uint<32> qcfg = config[3];

    initRes(numVertex, maxValue, sourceID, result512);
    while (que_rd_cnt != que_wr_cnt) {
        shortestPathS1Dataflow<WType, MAXOUTDEGREE>(qcfg, queStatus, ddrQue512, offset, column, result512, tableStatus,
                                                    toIDTable, distTable);
        shortestPathS2Dataflow(qcfg, tableStatus, toIDTable, distTable, result, queStatus, ddrQue);

        que_rd_cnt = queStatus[1].concat(queStatus[0]);
        que_wr_cnt = queStatus[3].concat(queStatus[2]);
    }

    info[0] = queStatus[6];
    info[1] = tableStatus[1];
}
} // namespace shortest_path
} // namespace internal

/**
 * @brief singleSourceShortestPath the single source shortest path algorithm is implemented, the input is the matrix in
 * CSR format.
 *
 * @tparam WTYPE date type of the weight
 * @tparam MAXOUTDEGREE The max out put degree of the input graph supported. Large max out degree value distance32 in
 * more
 * URAM.
 *
 * @param config    The config data. config[0] is sourceID. config[1] is the number of vertices in the graph. config[2]
 * is the max distance value. config[3] is the depth of the queue32.
 * @param offsetCSR    The offsetCSR buffer that stores the offsetCSR data in CSR format
 * @param indexCSR    The indexCSR buffer that stores the indexCSR dada in CSR format
 * @param weightCSR    The weight buffer that stores the weight data in CSR format
 * @param queue512 The shortest path requires a queue. The queue will be stored here in 512bit. Please allocate the
 * same buffer with queue32 and budle to the same gmem port with queue32.
 * @param queue32    The shortest path requires a queue. The queue will be stored here in 32bit. Please allocate the
 * same
 * buffer with queue512 and budle to the same gmem port with queue512.
 * @param distance512 The distance32 data. The width is 512. When allocating buffers, distance512 and distance32 should
 * point to the
 * same buffer. And please bundle distance512 and distance32 to the same gmem port.
 * @param distance32    The distance32 data is stored here. When allocating buffers, distance512 and distance32 should
 * point to the
 * same buffer. And please bundle distance512 and distance32 to the same gmem port.
 * @param info      The debug information. info[0] shows if the queue overflow.
 *
 */
template <typename WTYPE, int MAXOUTDEGREE>
void singleSourceShortestPath(ap_uint<32>* config,
                              ap_uint<512>* offsetCSR,
                              ap_uint<512>* indexCSR,
                              ap_uint<512>* weightCSR,

                              ap_uint<512>* queue512,
                              ap_uint<32>* queue32,

                              ap_uint<512>* distance512,
                              WTYPE* distance32,
                              ap_uint<8>* info) {
    xf::graph::internal::shortest_path::shortestPathInner<WTYPE, MAXOUTDEGREE>(
        config, offsetCSR, indexCSR, weightCSR, queue512, queue32, distance512, distance32, info);
}

/**
 * @brief singleSourceUnWeightedShortestPath the single source shortest path algorithm for unweighted is implemented,
 * the input is the matrix in CSR format.
 *
 * @tparam WTYPE date type of the weight
 * @tparam MAXOUTDEGREE The max out put degree of the input graph supported. Large max out degree value distance32 in
 * more
 * URAM.
 *
 * @param config    The config data. config[0] is sourceID. config[1] is the number of vertices in the graph. config[2]
 * is the max distance value. config[3] is the depth of the queue32.
 * @param offsetCSR    The offsetCSR buffer that stores the offsetCSR data in CSR format
 * @param indexCSR    The indexCSR buffer that stores the indexCSR dada in CSR format
 * @param queue512 The shortest path requires a queue. The queue will be stored here in 512bit. Please allocate the
 * same buffer with queue32 and budle to the same gmem port with queue32.
 * @param queue32    The shortest path requires a queue. The queue will be stored here in 32bit. Please allocate the
 * same
 * buffer with queue512 and budle to the same gmem port with queue512.
 * @param distance512 The distance32 data. The width is 512. When allocating buffers, distance512 and distance32 should
 * point to the
 * same buffer. And please bundle distance512 and distance32 to the same gmem port.
 * @param distance32    The distance32 data is stored here. When allocating buffers, distance512 and distance32 should
 * point to the
 * same buffer. And please bundle distance512 and distance32 to the same gmem port.
 * @param info      The debug information. info[0] shows if the queue overflow.
 *
 */
template <typename WTYPE, int MAXOUTDEGREE>
void singleSourceUnWeightedShortestPath(ap_uint<32>* config,
                                        ap_uint<512>* offsetCSR,
                                        ap_uint<512>* indexCSR,

                                        ap_uint<512>* queue512,
                                        ap_uint<32>* queue32,

                                        ap_uint<512>* distance512,
                                        WTYPE* distance32,
                                        ap_uint<8>* info) {
    xf::graph::internal::shortest_path::shortestPathInner<WTYPE, MAXOUTDEGREE>(config, offsetCSR, indexCSR, queue512,
                                                                               queue32, distance512, distance32, info);
}

} // namespace graph
} // namespace xf
#endif
