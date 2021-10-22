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
 * @file kernel_renumber.cpp
 *
 * @brief This file contains top function of test case.
 */

#ifndef XF_GRAPH_RENUMBER_HPP
#define XF_GRAPH_RENUMBER_HPP

#include "xf_utils_hw/uram_array.hpp"
#include <thread>
#include <ap_fixed.h>
#include <ap_int.h>
#include <hls_math.h>
#include <hls_stream.h>

#define DWIDTHS (32)
#define DSTREAM (2 * DWIDTHS)
#define DSTREAML (3 * DWIDTHS + 1)
#define NMAX (1 << 20) // 64M
#define NPARTS (8)     // 2, 4, 8 only
#define WDATA (64)     // uram width
#define NCACHE (4)
#define NDATA (NMAX / NPARTS) // for every uram group

// It must be larger than latency=32 + 10 > 42
#define SWAITLEN (1 << 14)
#define HWAITLEN (64)

// if you want to debug, you can open this
//#define DEBUG_PRINT

template <typename MType, typename DType>
union f_debug;

template <typename DT>
union f_debug<DT, ap_int<32> > {
    DT f;
    int32_t i;
};

template <typename DT>
union f_debug<DT, ap_int<64> > {
    DT f;
    int64_t i;
};

namespace xf {
namespace graph {
namespace internal {

inline void initRenumber(int32_t NV, ap_int<DWIDTHS>* mapCid0, ap_int<DWIDTHS>* mapCid1) {
#pragma HLS INLINE off
INIT_LOOP:
    for (int32_t i = 0; i < NV; i++) {
#pragma HLS PIPELINE II = 1
        mapCid0[i] = -1;
        mapCid1[i] = -1;
    }
}

inline void readToStrm(int32_t NV, ap_int<DWIDTHS>* oldCids, hls::stream<ap_int<DWIDTHS> >& cidStrm) {
#pragma HLS INLINE off
    ap_int<DWIDTHS> cid;
READ_OLDCID:
    for (int32_t i = 0; i < NV; i++) {
#pragma HLS PIPELINE II = 1
        cid = oldCids[i];
        cidStrm.write(cid);
    }
}

template <int _NPARTS>
void splitCidStream(int32_t NV,
                    hls::stream<ap_int<DWIDTHS> >& cidStrm,
                    hls::stream<ap_int<DWIDTHS * 2> > cidPartStrm[_NPARTS],
                    hls::stream<bool> e_parts[_NPARTS],
                    hls::stream<ap_int<DWIDTHS> >& cidStrmOut) {
#pragma HLS INLINE off
    ap_int<DWIDTHS> cid;
    ap_int<DWIDTHS * 2> vid;
#ifdef DEBUG_PRINT
    f_debug<int32_t, ap_int<DWIDTHS> > vv, cc, cn;
#endif
SPLIT_LOOP:
    for (int32_t i = 0; i < NV; i++) {
#pragma HLS PIPELINE II = 1
        cidStrm.read(cid);
        cidStrmOut.write(cid);
        vid.range(DWIDTHS - 1, 0) = i;
        vid.range(DWIDTHS * 2 - 1, DWIDTHS) = cid;
        if (cid < (NV >> 1)) {
            if (_NPARTS == 2) {
                cidPartStrm[0].write(vid);
                e_parts[0].write(false);
            } else {
                if (cid < (NV >> 2)) {
                    if (_NPARTS == 4) {
                        cidPartStrm[0].write(vid);
                        e_parts[0].write(false);
                    } else {
                        if (cid < (NV >> 3)) {
                            cidPartStrm[0].write(vid);
                            e_parts[0].write(false);
                        } else {
                            cidPartStrm[1].write(vid);
                            e_parts[1].write(false);
                        }
                    }
                } else {
                    if (_NPARTS == 4) {
                        cidPartStrm[1].write(vid);
                        e_parts[1].write(false);
                    } else {
                        if (cid < (3 * NV >> 3)) {
                            cidPartStrm[2].write(vid);
                            e_parts[2].write(false);
                        } else {
                            cidPartStrm[3].write(vid);
                            e_parts[3].write(false);
                        }
                    }
                }
            }
#ifdef DEBUG_PRINT
            cc.i = cid;
            printf("\033[0;33m upper: %d %d\033[0m \n", i, cc.f);
#endif
        } else {
            if (_NPARTS == 2) {
                cidPartStrm[1].write(vid);
                e_parts[1].write(false);
            } else {
                if (cid < (3 * NV >> 2)) {
                    if (_NPARTS == 4) {
                        cidPartStrm[2].write(vid);
                        e_parts[2].write(false);
                    } else {
                        if (cid < (5 * NV >> 3)) {
                            cidPartStrm[4].write(vid);
                            e_parts[4].write(false);
                        } else {
                            cidPartStrm[5].write(vid);
                            e_parts[5].write(false);
                        }
                    }
                } else {
                    if (_NPARTS == 4) {
                        cidPartStrm[3].write(vid);
                        e_parts[3].write(false);
                    } else {
                        if (cid < (7 * NV >> 3)) {
                            cidPartStrm[6].write(vid);
                            e_parts[6].write(false);
                        } else {
                            cidPartStrm[7].write(vid);
                            e_parts[7].write(false);
                        }
                    }
                }
            }
#ifdef DEBUG_PRINT
            cc.i = cid;
            printf("\033[32m under: %d %d \033[0m \n", i, cc.f);
#endif
        }
    }
    for (int i = 0; i < _NPARTS; i++) e_parts[i].write(true);
}

inline void setUram(int32_t offset,
                    hls::stream<ap_int<DWIDTHS * 2> >& cidStrm,
                    hls::stream<bool>& e_str,
                    hls::stream<ap_int<DSTREAML> >& tagStrm) {
#pragma HLS INLINE off
    ap_int<DWIDTHS * 2> cid;
    ap_uint<WDATA> isF;
    ap_int<DSTREAML> tmpWrStr;
#ifdef DEBUG_PRINT
    f_debug<int32_t, ap_int<DWIDTHS> > vv, cc, cn;
#endif
    int32_t cnt = 0;
    bool isDo(false);

    xf::common::utils_hw::UramArray<WDATA, NDATA, NCACHE> isFirst;
    int num = isFirst.memSet(0); // init uram

SET_LOOP:
    while (!isDo) {
#pragma HLS PIPELINE II = 1
#pragma HLS DEPENDENCE variable = isFirst.blocks inter false
        if (!e_str.empty()) {
            e_str.read(isDo);
            if (!isDo) {
                cidStrm.read(cid);
                ap_uint<DWIDTHS> low = cid.range(DWIDTHS - 1, 0);            // vid
                ap_uint<DWIDTHS> high = cid.range(2 * DWIDTHS - 1, DWIDTHS); // cid
                ap_uint<DWIDTHS> base = high - offset;
                ap_uint<6> tmp = base.range(5, 0);
                ap_uint<DWIDTHS - 6> addr = base.range(DWIDTHS - 1, 6);
                isF = isFirst.read(addr);
                if (isF.range(tmp, tmp) == 0) {
                    tmpWrStr.range(DWIDTHS - 1, 0) = low;
                    tmpWrStr.range(2 * DWIDTHS - 1, DWIDTHS) = high;
                    tmpWrStr.range(3 * DWIDTHS - 1, 2 * DWIDTHS) = cnt++;
                    tmpWrStr.range(3 * DWIDTHS, 3 * DWIDTHS) = 1;
                    isF.range(tmp, tmp) = 1;
                    isFirst.write(addr, isF);
                    tagStrm.write(tmpWrStr);
#ifdef DEBUG_PRINT
                    vv.i = low;
                    cc.i = high;
                    printf("\033[32m true: %d %d %d %d \033[0m \n", vv.f, cc.f, cnt - 1, (int32_t)1);
#endif
                } else {
                    tmpWrStr.range(DWIDTHS - 1, 0) = low;
                    tmpWrStr.range(2 * DWIDTHS - 1, DWIDTHS) = high;
                    tmpWrStr.range(3 * DWIDTHS - 1, 2 * DWIDTHS) = cnt;
                    tmpWrStr.range(3 * DWIDTHS, 3 * DWIDTHS) = 0;
                    tagStrm.write(tmpWrStr);
#ifdef DEBUG_PRINT
                    vv.i = low;
                    cc.i = high;
                    printf("\033[35m false: %d %d %d %d \033[0m \n", vv.f, cc.f, cnt - 1, (int32_t)0);
#endif
                }
            }
        }
    }
}

template <int _NPARTS>
void mergeTagStrm(int32_t NV,
                  int32_t& Cnt,
                  hls::stream<ap_int<DWIDTHS> >& cidStrm,
                  hls::stream<ap_int<DSTREAML> > tagPartStrm[_NPARTS],
                  hls::stream<ap_int<DSTREAML> >& tagStrm) {
#pragma HLS INLINE off
    int32_t cnt = 0;
    ap_int<DWIDTHS> cid;
    ap_uint<1> isF;
    ap_int<DSTREAML> upper;
    ap_int<DSTREAML> under;
    ap_int<DSTREAML> tag;
#ifdef DEBUG_PRINT
    f_debug<int32_t, ap_int<DWIDTHS> > vv, cc, cn;
#endif
MERGE_LOOP:
    for (int32_t i = 0; i < NV; i++) {
#pragma HLS PIPELINE II = 1
        cidStrm.read(cid);
        if (cid < (NV / 2)) {
            if (_NPARTS == 2) {
                tagPartStrm[0].read(tag);
            } else { // _NPARTS=4
                if (cid < (NV / 4)) {
                    if (_NPARTS == 4)
                        tagPartStrm[0].read(tag);
                    else {
                        if (cid < (NV >> 3))
                            tagPartStrm[0].read(tag);
                        else
                            tagPartStrm[1].read(tag);
                    }
                } else {
                    if (_NPARTS == 4) {
                        tagPartStrm[1].read(tag);
                    } else {
                        if (cid < (3 * NV >> 3))
                            tagPartStrm[2].read(tag);
                        else
                            tagPartStrm[3].read(tag);
                    }
                }
            }
        } else {
            if (_NPARTS == 2) {
                tagPartStrm[1].read(tag);
            } else { // _NPARTS=4
                if (cid < (3 * NV / 4)) {
                    if (_NPARTS == 4) {
                        tagPartStrm[2].read(tag);
                    } else {
                        if (cid < (5 * NV >> 3))
                            tagPartStrm[4].read(tag);
                        else
                            tagPartStrm[5].read(tag);
                    }
                } else {
                    if (_NPARTS == 4) {
                        tagPartStrm[3].read(tag);
                    } else {
                        if (cid < (7 * NV >> 3))
                            tagPartStrm[6].read(tag);
                        else
                            tagPartStrm[7].read(tag);
                    }
                }
            }
        }
        isF = tag.range(3 * DWIDTHS, 3 * DWIDTHS);
        if (isF) {
            tag.range(3 * DWIDTHS - 1, 2 * DWIDTHS) = cnt++;
#ifdef DEBUG_PRINT
            vv.i = tag.range(DWIDTHS - 1, 0);
            cc.i = tag.range(2 * DWIDTHS - 1, DWIDTHS);
            printf("\033[36mTrue: %d %d %d %d\033[0m\n", vv.f, cc.f, cnt - 1, 1);
#endif
        } else {
            tag.range(3 * DWIDTHS - 1, 2 * DWIDTHS) = cnt;
#ifdef DEBUG_PRINT
            vv.i = tag.range(DWIDTHS - 1, 0);
            cc.i = tag.range(2 * DWIDTHS - 1, DWIDTHS);
            printf("\033[38mFalse: %d %d %d %d\033[0m\n", vv.f, cc.f, cnt - 1, 0);
#endif
        }
        tagStrm.write(tag);
    }
    Cnt = cnt;
}

inline void checkMapHbm(int32_t NV,
                        ap_int<DWIDTHS>* mapCid0,
                        ap_int<DWIDTHS>* mapCid1,
                        hls::stream<ap_int<DSTREAML> >& tagStrm,
                        hls::stream<ap_int<DSTREAML> >& mapStrm,
                        hls::stream<bool>& e_str_map,
                        hls::stream<ap_int<DSTREAML> >& updateStrm,
                        hls::stream<bool>& e_str_update) {
#pragma HLS INLINE off
    ap_int<DSTREAML> waitBuf[1024];
    ap_int<DSTREAML> tmpWrStr;
    ap_int<DSTREAML> tmpWaStr;
    ap_int<DWIDTHS> vid, oldCid, newCid;
    ap_uint<1> isF;
    int pHead = 0, pEnd = 0;
#ifdef DEBUG_PRINT
    f_debug<int32_t, ap_int<DWIDTHS> > vv, cc;
#endif

CHECK_LOOP:
    for (int32_t i = 0; i < NV; i++) {
#pragma HLS PIPELINE = 1
        tagStrm.read(tmpWrStr);
        vid = tmpWrStr.range(DWIDTHS - 1, 0);
        oldCid = tmpWrStr.range(2 * DWIDTHS - 1, DWIDTHS);
        newCid = tmpWrStr.range(3 * DWIDTHS - 1, 2 * DWIDTHS);
        isF = tmpWrStr.range(3 * DWIDTHS, 3 * DWIDTHS);

        if (isF == 1) {
            mapStrm.write(tmpWrStr);
            e_str_map.write(false);
            updateStrm.write(tmpWrStr);
            e_str_update.write(false);
#ifdef DEBUG_PRINT
            vv.i = vid;
            cc.i = newCid;
            printf("Map   : vid=%d cid=%d\n", vv.f, cc.f);
#endif
        } else {
            ap_int<DWIDTHS> newTmp = mapCid0[oldCid];
            if (newTmp != -1) {
                tmpWrStr.range(3 * DWIDTHS - 1, 2 * DWIDTHS) = newTmp;
                updateStrm.write(tmpWrStr);
                e_str_update.write(false);
#ifdef DPRINT
                vv.i = vid;
                cc.i = newTmp;
                printf("Check : vid=%d cid=%d\n", vv.f, cc.f);
#endif
            } else {
                waitBuf[pEnd % HWAITLEN] = tmpWrStr;
                pEnd++;
#ifdef DEBUG_PRINT
                vv.i = vid;
                printf("\033[31mPut   : vid=%d\033[0m\n", vv.i);
#endif
                if ((pEnd - pHead) >= HWAITLEN) {
                    tmpWrStr = waitBuf[pHead % HWAITLEN];
                    ap_int<DWIDTHS> cidTmp = tmpWrStr(2 * DWIDTHS - 1, DWIDTHS);
                    ap_int<DWIDTHS> mapTmp = mapCid1[cidTmp];
                    // if (mapTmp != -1) {
                    tmpWrStr(3 * DWIDTHS - 1, 2 * DWIDTHS) = mapTmp;
                    updateStrm.write(tmpWrStr);
                    e_str_update.write(false);
                    pHead++;
                    // }
                }
            }
        }
    }

    e_str_map.write(true);

WAITTING_LOOP:
    while (pHead < pEnd) {
#pragma HLS PIPELINE
        tmpWaStr = waitBuf[pHead % HWAITLEN];
        ap_int<DWIDTHS> cidTmp = tmpWaStr.range(2 * DWIDTHS - 1, DWIDTHS);
        ap_int<DWIDTHS> mapTmp = mapCid0[cidTmp];
        tmpWaStr.range(3 * DWIDTHS - 1, 2 * DWIDTHS) = mapTmp;
        updateStrm.write(tmpWaStr);
        e_str_update.write(false);
        pHead++;
#ifdef DEBUG_PRINT
        vv.i = tmpWaStr(DWIDTHS - 1, 0);
        printf("\033[32mClear cache: vid=%d\033[0m\n", vv.f);
#endif
    }

    e_str_update.write(true);
}

inline void writeMapHbm(ap_int<DWIDTHS>* mapCid0,
                        ap_int<DWIDTHS>* mapCid1,
                        hls::stream<ap_int<DSTREAML> >& mapStrm,
                        hls::stream<bool>& e_str) {
#pragma HLS INLINE off
    ap_int<DSTREAML> tmpWrStr;
    ap_int<DWIDTHS> vid, oldCid, newCid;
    bool isDo(false);

#ifdef DEBUG_PRINT
    f_debug<int32_t, ap_int<DWIDTHS> > vv, cc;
#endif

WRITE_DIRECT_LOOP:
    while (!isDo) {
#pragma HLS PIPELINE
        if (!e_str.empty()) {
            e_str.read(isDo);
            if (!isDo) {
                mapStrm.read(tmpWrStr);
                oldCid = tmpWrStr.range(2 * DWIDTHS - 1, DWIDTHS);
                newCid = tmpWrStr.range(3 * DWIDTHS - 1, 2 * DWIDTHS);
                mapCid0[oldCid] = newCid;
                mapCid1[oldCid] = newCid;
#ifdef DEBUG_PRINT
                vv.i = oldCid;
                cc.i = newCid;
                printf("Write : vid=%d cid=%d\n", vv.f, cc.f);
#endif
            }
        }
    }
}

inline void updateNewHbm(ap_int<DWIDTHS>* newCids,
                         hls::stream<ap_int<DSTREAML> >& updateStrm,
                         hls::stream<bool>& e_str) {
#pragma HLS INLINE off
    ap_int<DSTREAML> tmpWrStr;
    ap_int<DWIDTHS> vid, oldCid, newCid;
    bool isDo(false);

#ifdef DEBUG_PRINT
    f_debug<int32_t, ap_int<DWIDTHS> > vv, cc;
#endif

UPDATE_LOOP:
    while (!isDo) {
#pragma HLS PIPELINE
        if (!e_str.empty()) {
            e_str.read(isDo);
            if (!isDo) {
                updateStrm.read(tmpWrStr);
                vid = tmpWrStr.range(DWIDTHS - 1, 0);
                newCid = tmpWrStr.range(3 * DWIDTHS - 1, 2 * DWIDTHS);
                newCids[vid] = newCid;
#ifdef DEBUG_PRINT
                vv.i = vid;
                cc.i = newCid;
                printf("Update: vid=%d cid=%d\n", vv.f, cc.f);
#endif
            }
        }
    }
}

inline void checkMapHbm_th(int32_t NV,
                           ap_int<DWIDTHS>* mapCid0,
                           ap_int<DWIDTHS>* mapCid1,
                           hls::stream<ap_int<DSTREAML> >* tagStrm,
                           hls::stream<ap_int<DSTREAML> >* mapStrm,
                           hls::stream<bool>* e_str_map,
                           hls::stream<ap_int<DSTREAML> >* updateStrm,
                           hls::stream<bool>* e_str_update) {
#pragma HLS INLINE off
    ap_int<DSTREAML> tmpWrStr;
    ap_int<DSTREAML> tmpWaStr;
    ap_int<DSTREAML> waitBuf[SWAITLEN];
    ap_int<DWIDTHS> vid, oldCid, newCid;
    ap_uint<1> isF;
    int pHead = 0, pEnd = 0;
#ifdef DEBUG_PRINT
    f_debug<int32_t, ap_int<DWIDTHS> > vv, cc;
#endif

CHECK_LOOP:
    for (int32_t i = 0; i < NV; i++) {
#pragma HLS PIPELINE = 1
        tagStrm->read(tmpWrStr);
        vid = tmpWrStr(DWIDTHS - 1, 0);
        oldCid = tmpWrStr(2 * DWIDTHS - 1, DWIDTHS);
        newCid = tmpWrStr(3 * DWIDTHS - 1, 2 * DWIDTHS);
        isF = tmpWrStr(3 * DWIDTHS, 3 * DWIDTHS);

        if (isF == 1) {
            mapStrm->write(tmpWrStr);
            e_str_map->write(false);
            updateStrm->write(tmpWrStr);
            e_str_update->write(false);
#ifdef DEBUG_PRINT
            vv.i = vid;
            cc.i = newCid;
            printf("Map   : vid=%d cid=%d\n", vv.f, cc.f);
#endif
        } else {
            ap_int<DWIDTHS> newTmp = mapCid0[oldCid];
            if (newTmp != -1) {
                tmpWrStr(3 * DWIDTHS - 1, 2 * DWIDTHS) = newTmp;
                updateStrm->write(tmpWrStr);
                e_str_update->write(false);
#ifdef DEBUG_PRINT
                vv.i = vid;
                cc.i = newTmp;
                printf("Check : vid=%d cid=%d\n", vv.f, cc.f);
#endif
            } else {
                waitBuf[pEnd % SWAITLEN] = tmpWrStr;
                pEnd++;
#ifdef DEBUG_PRINT
                vv.i = vid;
                printf("Put   : vid=%d\n", vv.i);
#endif
                if ((pEnd - pHead) >= SWAITLEN) {
                    tmpWrStr = waitBuf[pHead % SWAITLEN];
                    ap_int<DWIDTHS> cidTmp = tmpWrStr(2 * DWIDTHS - 1, DWIDTHS);
                    ap_int<DWIDTHS> mapTmp = mapCid1[cidTmp];
                    if (mapTmp == -1) printf("\033[34mWarning: buffer size smaller\033[0m\n");
                    tmpWrStr(3 * DWIDTHS - 1, 2 * DWIDTHS) = mapTmp;
                    updateStrm->write(tmpWrStr);
                    e_str_update->write(false);
                    pHead++;
                    // }
                }
            }
        }
    }

    e_str_map->write(true);

WAITTING_LOOP:
    while (pHead < pEnd) {
#pragma HLS PIPELINE = 1
        tmpWaStr = waitBuf[pHead % SWAITLEN];
        ap_int<DWIDTHS> cidTmp = tmpWaStr(2 * DWIDTHS - 1, DWIDTHS);
        ap_int<DWIDTHS> mapTmp = mapCid0[cidTmp];
        tmpWaStr(3 * DWIDTHS - 1, 2 * DWIDTHS) = mapTmp;
        updateStrm->write(tmpWaStr);
        e_str_update->write(false);
        pHead++;
#ifdef DEBUG_PRINT
        vv.i = tmpWaStr(DWIDTHS - 1, 0);
        printf("\033[32mClear cache: vid=%d\033[0m\n", vv.f);
#endif
        if (mapTmp == -1) printf("\033[31mWarning: waiting\033[0m\n");
    }

    e_str_update->write(true);
}

inline void writeMapHbm_th(ap_int<DWIDTHS>* mapCid0,
                           ap_int<DWIDTHS>* mapCid1,
                           hls::stream<ap_int<DSTREAML> >* mapStrm,
                           hls::stream<bool>* e_str) {
#pragma HLS INLINE off
    ap_int<DSTREAML> tmpWrStr;
    ap_int<DWIDTHS> vid, oldCid, newCid;
    bool isDo(false);

#ifdef DEBUG_PRINT
    f_debug<int, ap_int<DWIDTHS> > vv, cc;
#endif

WRITE_MAP_LOOP:
    while (!isDo) {
#pragma HLS PIPELINE
        if (!e_str->empty()) {
            e_str->read(isDo);
            if (!isDo) {
                mapStrm->read(tmpWrStr);
                oldCid = tmpWrStr(2 * DWIDTHS - 1, DWIDTHS);
                newCid = tmpWrStr(3 * DWIDTHS - 1, 2 * DWIDTHS);
                mapCid0[oldCid] = newCid;
                mapCid1[oldCid] = newCid;
#ifdef DEBUG_PRINT
                vv.i = oldCid;
                cc.i = newCid;
                printf("Write : oldCid=%d newCid=%d\n", vv.f, cc.f);
#endif
            }
        }
    }
}

inline void updateNewHbm_th(ap_int<DWIDTHS>* newCids,
                            hls::stream<ap_int<DSTREAML> >* updateStrm,
                            hls::stream<bool>* e_str) {
#pragma HLS INLINE off
    ap_int<DSTREAML> tmpWrStr;
    ap_int<DWIDTHS> vid, oldCid, newCid;
    bool isDo(false);

#ifdef DEBUG_PRINT
    f_debug<int, ap_int<DWIDTHS> > vv, cc;
#endif

UPDATE_LOOP:
    while (!isDo) {
#pragma HLS PIPELINE
        if (!e_str->empty()) {
            e_str->read(isDo);
            if (!isDo) {
                updateStrm->read(tmpWrStr);
                vid = tmpWrStr(DWIDTHS - 1, 0);
                newCid = tmpWrStr(3 * DWIDTHS - 1, 2 * DWIDTHS);
                newCids[vid] = newCid;
#ifdef DEBUG_PRINT
                vv.i = vid;
                cc.i = newCid;
                printf("Update: vid=%ld cid=%ld\n", vv.f, cc.f);
#endif
            }
        }
    }
}

inline void parallelWriteHbm(int32_t NV,
                             ap_int<DWIDTHS>* mapCid0,
                             ap_int<DWIDTHS>* mapCid1,
                             hls::stream<ap_int<DSTREAML> >& tagStrm,
                             ap_int<DWIDTHS>* newCids) {
#pragma HLS INLINE off

    hls::stream<ap_int<DSTREAML> > mapStrm("mapStrm");
#pragma HLS RESOURCE variable = mapStrm core = FIFO_LUTRAM
#pragma HLS STREAM variable = mapStrm depth = 128

    hls::stream<ap_int<DSTREAML> > updateStrm("updateStrm");
#pragma HLS RESOURCE variable = updateStrm core = FIFO_LUTRAM
#pragma HLS STREAM variable = updateStrm depth = 128

    hls::stream<bool> e_str_map("e_str_map");
#pragma HLS RESOURCE variable = e_str_map core = FIFO_LUTRAM
#pragma HLS STREAM variable = e_str_map depth = 128

    hls::stream<bool> e_str_update("e_str_update");
#pragma HLS RESOURCE variable = e_str_update core = FIFO_LUTRAM
#pragma HLS STREAM variable = e_str_update depth = 128

    std::thread ths[3];

    ths[0] =
        std::thread(checkMapHbm_th, NV, mapCid0, mapCid1, &tagStrm, &mapStrm, &e_str_map, &updateStrm, &e_str_update);
    ths[1] = std::thread(writeMapHbm_th, mapCid0, mapCid1, &mapStrm, &e_str_map);
    ths[2] = std::thread(updateNewHbm_th, newCids, &updateStrm, &e_str_update);

    for (auto& t : ths) t.join();
}

} // namespace internal

/**
 * @brief Renumbering recode the categorized graph's table, and it support 64M for input.
 *
 * @param NV the size of vertices.
 * @param numClusters the size of renumbered.
 * @param oldCids the input table for vertices.
 * @param mapCid0 the intermediate memory for new value, and the memory is writen directly after renumbering.
 * @param mapCid1 the duplicate memory for mapCid0
 * @param newCids the output for renumbering, the value is recoded from 0 to numClusters.
 *
 */

inline void renumberCore(int32_t NV,
                         int32_t& numClusters,
                         ap_int<DWIDTHS>* oldCids,
                         ap_int<DWIDTHS>* mapCid0,
                         ap_int<DWIDTHS>* mapCid1,
                         ap_int<DWIDTHS>* newCids) {
#pragma HLS DATAFLOW

    hls::stream<ap_int<DWIDTHS> > cidStrm("cidStrm");
#pragma HLS RESOURCE variable = cidStrm core = FIFO_LUTRAM
#pragma HLS STREAM variable = cidStrm depth = 128

    hls::stream<ap_int<DWIDTHS> > cidStrmOut("cidStrmOut");
#pragma HLS RESOURCE variable = cidStrmOut core = FIFO_LUTRAM
#pragma HLS STREAM variable = cidStrmOut depth = 128

    hls::stream<ap_int<DWIDTHS * 2> > cidPartStrm[NPARTS];
#pragma HLS ARRAY_PARTITION variable = cidPartStrm complete dim = 0
#pragma HLS RESOURCE variable = cidPartStrm core = FIFO_LUTRAM
#pragma HLS STREAM variable = cidPartStrm depth = 128

    hls::stream<ap_int<DSTREAML> > tagStrm("tagStrm");
#pragma HLS RESOURCE variable = tagStrm core = FIFO_LUTRAM
#pragma HLS STREAM variable = tagStrm depth = 128

    hls::stream<ap_int<DSTREAML> > tagPartStrm[NPARTS];
#pragma HLS ARRAY_PARTITION variable = tagPartStrm complete dim = 0
#pragma HLS RESOURCE variable = tagPartStrm core = FIFO_LUTRAM
#pragma HLS STREAM variable = tagPartStrm depth = 128

    hls::stream<ap_int<DSTREAML> > mapStrm("mapStrm");
#pragma HLS RESOURCE variable = mapStrm core = FIFO_LUTRAM
#pragma HLS STREAM variable = mapStrm depth = 128

    hls::stream<ap_int<DSTREAML> > updateStrm("updateStrm");
#pragma HLS RESOURCE variable = updateStrm core = FIFO_LUTRAM
#pragma HLS STREAM variable = updateStrm depth = 128

    hls::stream<bool> e_parts[NPARTS];
#pragma HLS ARRAY_PARTITION variable = e_parts complete dim = 0
#pragma HLS RESOURCE variable = e_parts core = FIFO_LUTRAM
#pragma HLS STREAM variable = e_parts depth = 128

    hls::stream<bool> e_map("e_map");
#pragma HLS RESOURCE variable = e_map core = FIFO_LUTRAM
#pragma HLS STREAM variable = e_map depth = 128

    hls::stream<bool> e_update("e_update");
#pragma HLS RESOURCE variable = e_update core = FIFO_LUTRAM
#pragma HLS STREAM variable = e_update depth = 128

    xf::graph::internal::readToStrm(NV, oldCids, cidStrm);
    xf::graph::internal::splitCidStream<NPARTS>(NV, cidStrm, cidPartStrm, e_parts, cidStrmOut);
    for (int i = 0; i < NPARTS; i++) {
#pragma HLS UNROLL
        xf::graph::internal::setUram((i * NV) / NPARTS, cidPartStrm[i], e_parts[i], tagPartStrm[i]);
    }
    xf::graph::internal::mergeTagStrm<NPARTS>(NV, numClusters, cidStrmOut, tagPartStrm, tagStrm);

#ifndef __SYNTHESIS__
    internal::parallelWriteHbm(NV, mapCid0, mapCid1, tagStrm, newCids);
#else
    xf::graph::internal::checkMapHbm(NV, mapCid0, mapCid1, tagStrm, mapStrm, e_map, updateStrm, e_update);
    xf::graph::internal::writeMapHbm(mapCid0, mapCid1, mapStrm, e_map);
    xf::graph::internal::updateNewHbm(newCids, updateStrm, e_update);
#endif
}

} // namespace graph
} // namespace xf
#endif
