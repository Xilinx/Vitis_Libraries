/*
 * Copyright (C) 2019-2022, Xilinx, Inc.
 * Copyright (C) 2022-2023, Advanced Micro Devices, Inc.
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
 * @file merge.hpp
 * @brief This file contains top function of merge kernel.
 */

#ifndef XF_GRAPH_MERGE_HPP
#define XF_GRAPH_MERGE_HPP

#include "ap_int.h"
#include <hls_stream.h>
#include <hls_burst_maxi.h>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sys/time.h>
#include <vector>
#include <limits>
#include <algorithm>
#include <map>
#include <iterator>
#include "common.hpp"

// float to int
static DF_WI_T ToInt(DF_W_T val, short scl) {
#pragma HLS INLINE
    return (DF_WI_T)val * (1L << scl);
}

// int to float
static DF_W_T ToFloat(DF_WI_T val, short scl) {
#pragma HLS INLINE
    return val / (1L << scl);
}

// val: cid
// add: the index of the separate found
template <class T_V>
struct ValAddr {
    T_V val;
    int addr;
};

// e: real hash key
// w: real value
template <class T_V, class T_W>
struct AggWE {
    T_V e;
    T_W w;
};

namespace xf {
namespace graph {
namespace merge {

// TODO: if merge with louvain, use this for widebus
template <int WA, int WD>
ap_uint<(1 << WD)> AxiRead(ap_uint<(1 << WA)>* axi, int addr) {
#pragma HLS INLINE
    const int B_ADDR = 64;
    const int BITS = 1 << WD; // 1<<5;
    ap_uint<B_ADDR> addr_u = addr;
    ap_uint<1 << WA> tmp = axi[addr_u(B_ADDR - 1, (WA - WD))];
    ap_uint<WA - WD> off = addr_u((WA - WD) - 1, 0);
    return tmp(BITS * off + BITS - 1, BITS * off);
}

// AggRAM used for small Hash aggregation
template <class T_MEM, int W_ADDR, int SIZE>
class AggRAM {
   public:
    T_MEM mem[SIZE];
    // addr and val regisers used for II=1 RAM update
    ap_uint<W_ADDR + 1> addr_cur = 0x7f;
    ap_uint<W_ADDR + 1> addr_pre_1 = 0x7f;
    ap_uint<W_ADDR + 1> addr_pre_2 = 0x7f;
    ap_uint<W_ADDR + 1> addr_pre_3 = 0x7f;
    T_MEM val_cur;
    T_MEM val_pre_1;
    T_MEM val_pre_2;
    T_MEM val_pre_3;
    AggRAM() {
#pragma HLS INLINE
    INITAggRAM:
        for (int i = 0; i < SIZE; i++) {
#pragma HLS PIPELINE
            mem[i] = 0;
        }
    }
    void Reset() {
#pragma HLS INLINE
        addr_cur = 0x7f;
        addr_pre_1 = 0x7f;
        addr_pre_2 = 0x7f;
        addr_pre_3 = 0x7f;
    }
    void Aggregate(ap_uint<W_ADDR> addr, T_MEM w) {
#pragma HLS INLINE
        mem[addr] += w;
    }
    void Aggregate(ap_uint<W_ADDR> addr, T_MEM w, bool isFirst) {
#pragma HLS INLINE
#pragma HLS DEPENDENCE variable = mem inter false
#pragma HLS DEPENDENCE variable = mem intra false
        T_MEM val_new;
        T_MEM val_tmp;
        if (addr == addr_pre_1) {
            val_cur = val_pre_1;
        } else if (addr == addr_pre_2) {
            val_cur = val_pre_2;
        } else if (addr == addr_pre_3) {
            val_cur = val_pre_3;
        } else {
            val_cur = mem[addr];
        }
        val_new = val_cur + w;
        mem[addr] = val_new;
        val_cur = val_new;
        addr_pre_3 = addr_pre_2;
        addr_pre_2 = addr_pre_1;
        addr_pre_1 = addr;
        val_pre_3 = val_pre_2;
        val_pre_2 = val_pre_1;
        val_pre_1 = val_new;
    }
};

// check if hit hash memory and overflow
// if hit: return hit hash id
// if not hit: add number until NUM_SMALL
static ap_uint<NUM_SMALL_LOG> MemAgg_core_key(DF_V_T key,
                                              SMALL_T& num_cid_small,
                                              DF_V_T mem_key[NUM_SMALL],
                                              bool& overflow) {
#pragma HLS INLINE
#pragma HLS ARRAY_PARTITION variable = mem_key complete dim = 1
    ap_uint<NUM_SMALL> Hit = 0;
    int hit_id = 0xffff;
    bool is_hit = false;
    for (int i = 0; i < NUM_SMALL; i++) {
#pragma HLS UNROLL
        if (i >= num_cid_small) {
            mem_key[i] = key;
        } else if (mem_key[i] == key) {
            is_hit = true;
            hit_id = i;
        }
    }

    if (!is_hit && num_cid_small == NUM_SMALL) {
        overflow = true;
    } else {
        overflow = false;
    }
    ap_uint<NUM_SMALL_LOG> ret;
    if (!is_hit) {
        if (num_cid_small < NUM_SMALL) {
            ret = num_cid_small++;
        } else {
            ret = NUM_SMALL;
        }
    } else {
        ret = hit_id;
    }
    return ret;
}

// HashAgg: do big hash aggregation
template <class T_V, class T_W, int W_HASH>
class HashAgg {
   public:
    int count_i;
    int cnt_agg;
    AggWE<T_V, T_W>* hash_we;
    // val: cid
    // add: the index of the separate found
    ap_uint<W_URAM>* valAdd;

    // addr and val regisers used for II=1 RAM update
    // shift used for hash aggregation
    DF_WI_T shift_reg[3];
    int shift_addr[3];
    // count used for get address of hitted hash map
    ap_uint<64> count_reg[3];
    int count_addr[3];

    HashAgg(AggWE<T_V, T_W>* p_hash_we, ap_uint<W_URAM>* p_valAdd) {
        hash_we = p_hash_we;
        valAdd = p_valAdd;
        Init();
    }

    void Init() {
        cnt_agg = 0;
        for (int i = 0; i < 3; i++) {
#pragma HLS unroll
            shift_addr[i] = 0xffffffff;
        }
        for (int i = 0; i < 3; i++) {
#pragma HLS unroll
            count_addr[i] = 0xffffffff;
        }
    }

    // compate hash key
    int myHash(T_V v) {
#pragma HLS INLINE
        ap_uint<32> val = v;
        return val(W_HASH - 1, 0);
    }

    // compute address
    // check if the first access of hash key
    // return the address of hash_we
    int GetAddr(T_V cid, bool& isFirst) {
#pragma HLS INLINE
#pragma HLS DEPENDENCE variable = valAdd inter false
#pragma HLS DEPENDENCE variable = valAdd intra false
        int h = myHash(cid);
        ap_uint<64> valAdd_cur;
        if (h == count_addr[0]) {
            valAdd_cur = count_reg[0];
        } else if (h == count_addr[1]) {
            valAdd_cur = count_reg[1];
        } else if (h == count_addr[2]) {
            valAdd_cur = count_reg[2];
        } else {
            valAdd_cur = valAdd[h];
        }
        int valAdd_cid = valAdd_cur(W_URAM - 1, W_URAM - W_V);
        int valAdd_cnt = valAdd_cur(W_AGG - 1, 0);
        bool isUsed = false;
        if (h == 0 && valAdd_cid == 0xffffffff) { // if 0, check 0xf for if used
            isUsed = false;
        } else if (h != 0 && valAdd_cid == 0x0) { // if not 0, check 0x0 for if used
            isUsed = false;
        } else {
            isUsed = true;
        }
        isFirst = !isUsed;

        if (!isUsed) {
            valAdd[h](W_URAM - 1, W_URAM - W_V) = cid;
            valAdd[h](W_AGG - 1, 0) = cnt_agg;
            ap_uint<64> valAdd_tmp;
            valAdd_tmp(W_URAM - 1, W_URAM - W_V) = cid;
            valAdd_tmp(W_AGG - 1, 0) = cnt_agg;
            count_reg[2] = count_reg[1];
            count_reg[1] = count_reg[0];
            count_reg[0] = valAdd_tmp;
            count_addr[2] = count_addr[1];
            count_addr[1] = count_addr[0];
            count_addr[0] = h;
            cnt_agg++;
            return cnt_agg - 1;
        } else {
            int cid_hash = valAdd_cur(W_URAM - 1, W_URAM - W_V);
            int a_hash = valAdd_cur(W_AGG, 0);
            if (cid == cid_hash) {
                return a_hash;
            } else {
                return -1;
            }
        }
    }

    int GetAddr(T_V cid) {
#pragma HLS INLINE
        int h = myHash(cid);
        int ret = -1;
        ap_uint<64> valAdd_cur = valAdd[h];
        int valAdd_cid = valAdd_cur(W_URAM - 1, W_URAM - W_V);
        int valAdd_cnt = valAdd_cur(W_AGG - 1, 0);
        bool isUsed = false;
        if (h == 0 && valAdd_cid == 0xffffffff) { // if 0, check 0xf for if used
            isUsed = false;
        } else if (h != 0 && valAdd_cid == 0x0) { // if not 0, check 0x0 for if used
            isUsed = false;
        } else {
            isUsed = true;
        }

        if (isUsed == false) {
            valAdd[h](W_URAM - 1, W_URAM - W_V) = cid;
            valAdd[h](W_AGG - 1, 0) = cnt_agg;
            ret = cnt_agg;
        } else {
            int cid_hash = valAdd_cur(W_URAM - 1, W_URAM - W_V);
            int a_hash = valAdd_cur(W_AGG, 0);
            if (cid == cid_hash) {
                ret = a_hash;
            } else {
                ret = -1;
            }
        }
        return ret;
    }

    // if hit: do hash aggregation, return true
    // if not hit: init value of hash_we, return true
    // if collision, return false
    bool TryToSet(T_V cid, T_W w, T_V add, bool isFirst) {
#pragma HLS INLINE
#pragma HLS DEPENDENCE variable = hash_we inter false
#pragma HLS DEPENDENCE variable = hash_we intra false
        if (add < 0) {
            return false;
        } else if (isFirst) {
            hash_we[add].w = w;
            hash_we[add].e = cid;
        } else {
            ap_uint<W_AGG_1> tmp;
            ap_uint<W_AGG_1> shift_cur;
            if (add == shift_addr[0]) {
                shift_cur = shift_reg[0];
            } else if (add == shift_addr[1]) {
                shift_cur = shift_reg[1];
            } else if (add == shift_addr[2]) {
                shift_cur = shift_reg[2];
            } else {
                shift_cur = hash_we[add].w;
            }
            tmp = shift_cur + w;
            hash_we[add].w = tmp;

            shift_reg[2] = shift_reg[1];
            shift_reg[1] = shift_reg[0];
            shift_addr[2] = shift_addr[1];
            shift_addr[1] = shift_addr[0];
            shift_addr[0] = add;
            shift_reg[0] = tmp;
        }
        return true;
    }

    bool TryToSet(T_V cid, T_W w, T_V* hbm1, hls::stream<DF_D_T>& stream_index_c) {
#pragma HLS INLINE
        bool ret = false;
        T_V tmp_hash;
        int add = GetAddr(cid);
        if (add < 0) {
            ret = false;
        } else {
            if (cnt_agg == add) {
                tmp_hash = hbm1[cid] + w;
                hash_we[add].w = tmp_hash;
                hash_we[add].e = cid;
                cnt_agg++;
            } else {
                ap_uint<W_AGG_1> tmp;
                tmp_hash = hash_we[add].w + w;
                hash_we[add].w = tmp_hash;
            }
            ret = true;
            stream_index_c.write(tmp_hash - 1);
            count_i++;
        }
        return ret;
    }
    void Output(hls::stream<DF_D_T, 1024>& stream_oute,
                hls::stream<DF_WI_T, 1024>& stream_outw,
                hls::stream<bool, 1024>& stream_endout) {
#pragma HLS DATAFLOW
    HASHAGGOUTPUT:
        for (int i = 0; i < cnt_agg; i++) {
#pragma HLS pipeline
            DF_V_T e = hash_we[i].e;
            stream_oute.write(e);
            stream_outw.write(hash_we[i].w);
            stream_endout.write(false);
            int h = myHash(e);
            if (h == 0) {
                valAdd[h] = 0xffffffffffffffff;
            } else {
                valAdd[h] = 0;
            }
        }
        Init();
    }
    void Output(DF_V_T* hbm1) {
    HASHAGGOUTPUT2:
        for (int i = 0; i < cnt_agg; i++) {
#pragma HLS pipeline
            DF_V_T e = hash_we[i].e;
            hbm1[e] = hash_we[i].w;
            int h = myHash(e);
            if (h == 0) {
                valAdd[h] = 0xffffffffffffffff;
            } else {
                valAdd[h] = 0;
            }
        }
        Init();
    }
    void ResetURAM(T_V e) {
#pragma HLS inline
        int h = myHash(e);
        if (h == 0) {
            valAdd[h] = 0xffffffffffffffff;
        } else {
            valAdd[h] = 0;
        }
    }
};

// aggregate the edges which collision for all cascade hash maps
template <class T_V, class T_W, int SIZE_REM>
class ScanAgg {
   public:
    int cnt_agg;
    AggWE<T_V, T_W>* ScanHash;
#ifdef SHIFT_REG_SCAN
    DF_WI_T scan_reg[4];
    int scan_addr[4];
#endif
    ScanAgg(AggWE<T_V, T_W>* p_mem) {
        ScanHash = p_mem;
        Init();
    }
    void Init() {
        cnt_agg = 0;
#ifdef SHIFT_REG_SCAN
        for (int i = 0; i < 4; i++) {
#pragma HLS unroll
            scan_addr[i] = 0xffffffff;
        }
#endif
    }

    // scan all the element in RAM and aggregate if hit
    // the performance is very bad
    // try to avoid to make data get into this final stage
    void AggWeight(T_V cid, DF_WI_T w) {
#ifdef SHIFT_REG_SCAN
#pragma HLS DEPENDENCE variable = ScanHash->w inter false
#pragma HLS DEPENDENCE variable = ScanHash->w intra false
#endif
        int trip = cnt_agg + 1;
    // ScanHash[i].e 的II也是2，也需要做reg
    AGGSCANF:
        for (int i = 0; i < trip; i++) {
#pragma HLS PIPELINE II = 1
            if (i == cnt_agg) {
                ScanHash[i].e = cid;
                ScanHash[i].w = w;
                cnt_agg++;
            } else if (ScanHash[i].e == cid) {
#ifdef SHIFT_REG_SCAN
                DF_WI_T tmp;
                scan_addr[0] = i;
                if (scan_addr[0] == scan_addr[1]) {
                    scan_reg[0] = scan_reg[1];
                } else if (scan_addr[0] == scan_addr[2]) {
                    scan_reg[0] = scan_reg[2];
                } else if (scan_addr[0] == scan_addr[3]) {
                    scan_reg[0] = scan_reg[3];
                } else {
                    scan_reg[0] = ScanHash[i].w;
                }
                tmp = scan_reg[0] + w;
                ScanHash[i].w = tmp;
                scan_reg[0] = tmp;

                scan_reg[3] = scan_reg[2];
                scan_reg[2] = scan_reg[1];
                scan_reg[1] = scan_reg[0];
                scan_addr[3] = scan_addr[2];
                scan_addr[2] = scan_addr[1];
                scan_addr[1] = scan_addr[0];
#else
                ScanHash[i].w += w;
#endif // SHIFT_REG_SCAN
                break;
            }
        } // for
    }

    void Output(hls::stream<DF_D_T, 1024>& stream_oute,
                hls::stream<DF_WI_T, 1024>& stream_outw,
                hls::stream<bool, 1024>& stream_endout) {
#pragma HLS DATAFLOW
    AGGSCAN_OUTPUT:
        for (int i = 0; i < cnt_agg; i++) {
#pragma HLS UNROLL
            stream_oute.write(ScanHash[i].e);
            stream_outw.write(ScanHash[i].w);
            stream_endout.write(false);
        }
        Init();
    }

}; // class ScanAgg;

void GetC(int num_v, DF_V_T* c, hls::stream<DF_D_T>& stream_c, bool with_end) {
    printf("start GetC, num_v=%d\n", num_v);
GETC:
    for (int i = 0; i < num_v; i++) {
#pragma HLS pipeline II = 1
        DF_D_T addr = c[i];
        stream_c.write(addr);
    }
    if (with_end) {
        stream_c.write(-1);
    }
}

// random update HBM, use a small RAM to avoid read and write conflict
// and also improve performance
template <class T_VAL, class T_ADD, int ARRAY_SIZE>
class ShiftUpdate {
   public:
    T_ADD shift_addr[ARRAY_SIZE];
    T_VAL shift_val[ARRAY_SIZE];
    ShiftUpdate() {
#pragma HLS INLINE
#pragma HLS array_partition variable = shift_addr complete
    UPDATE_INIT:
        for (int j = 0; j < ARRAY_SIZE; j++) {
#pragma HLS unroll
            shift_addr[j] = -1;
            shift_val[j] = 0;
        }
    }

    bool CheckHit(T_ADD addr, T_VAL& hit_val) {
#pragma HLS inline
        bool hit = false;
        T_ADD hit_addr;
    HIT:
        for (int j = 0; j < ARRAY_SIZE; j++) {
#pragma HLS unroll
            if (addr == shift_addr[j]) {
                hit = true;
                hit_addr = j;
                break;
            }
        }
        if (hit) {
            hit_val = shift_val[hit_addr];
        }
        return hit;
    }

    void Update(T_ADD addr, T_VAL update_val) {
#pragma HLS inline
    UPDATE_SHIFT:
        for (int j = ARRAY_SIZE - 2; j >= 0; j--) {
#pragma HLS unroll
            shift_addr[j + 1] = shift_addr[j];
            shift_val[j + 1] = shift_val[j];
        }
        shift_addr[0] = addr;
        shift_val[0] = update_val;
    }
};

#ifdef UPDATE_COUNT_HASH
void SetIndexC(int num_v, hls::stream<DF_D_T>& stream_index_c, DF_V_T* index_c) {
    printf("start SetIndexC, num_v=%d\n", num_v);
GETC:
    for (int i = 0; i < num_v; i++) {
#pragma HLS pipeline II = 1
        index_c[i] = stream_index_c.read();
    }
}

void UpdateCount(int num_v,
                 int num_c_out,
                 hls::stream<DF_D_T>& stream_c2,
                 hls::stream<DF_D_T>& stream_index_c,
                 DF_V_T* count_c_single) {
    printf("start UpdateCount, num_v=%d\n", num_v);
#pragma HLS DEPENDENCE variable = count_c_single inter false
#pragma HLS DEPENDENCE variable = count_c_single intra false
    const int HASH_WIDTH = 12;
    AggWE<int, int> hash_we[1 << HASH_WIDTH];
    ap_uint<W_URAM> valAdd[1 << HASH_WIDTH];
#pragma HLS RESOURCE variable = hash_we core = RAM_T2P_URAM
#pragma HLS RESOURCE variable = valAdd core = RAM_T2P_URAM
    HashAgg<int, int, HASH_WIDTH> myHash(hash_we, valAdd);
    myHash.valAdd[0] = 0xffffffffffffffff;
INIT_URAM:
    for (int i = 1; i < (1 << HASH_WIDTH); i++) {
        myHash.valAdd[i] = 0;
    }
    myHash.count_i = 0;
    int i = 0;
    int addr = stream_c2.read();
    bool end = false;
    bool collision = false;
UPDATE_COUNT:
    while (end == false) {
    UPDATE_COUNT_ONE:
        while (collision == false && end == false) {
#pragma HLS pipeline
            if (!myHash.TryToSet(addr, 1, count_c_single, stream_index_c)) {
                collision = true;
            } else {
                addr = stream_c2.read();
                if (addr == -1) {
                    end = true;
                }
            }
        }
        myHash.Output(count_c_single);
        collision = false;
    }
}
#else
void UpdateCount(int num_v, int num_c_out, hls::stream<DF_D_T>& stream_c2, DF_V_T* count_c_single, DF_V_T* index_c) {
    printf("start UpdateCount, num_v=%d\n", num_v);
#pragma HLS DEPENDENCE variable = count_c_single inter false
#pragma HLS DEPENDENCE variable = count_c_single intra false
    ShiftUpdate<int, int, 96> SU;
    int i = 0;
    int addr = stream_c2.read();
UPDATECOUNT:
    while (addr >= 0) {
#pragma HLS pipeline II = 1
        int val;
        bool hit = SU.CheckHit(addr, val);
        if (!hit) {
            val = count_c_single[addr];
        }
        int out = val + 1;
        index_c[i] = val;
        count_c_single[addr] = out;
        SU.Update(addr, out);
        addr = stream_c2.read();
        i++;
    }
}
#endif

// compute the number of each c id
// compute index for current element in each c id
void ComputeCount(int num_v, int num_c_out, DF_V_T* c, DF_V_T* count_c_single, DF_V_T* index_c) {
    printf("start ComputeCount\n");
#pragma HLS inline off
#pragma HLS dataflow
    hls::stream<DF_D_T, 1024> stream_c("c");
    hls::stream<DF_D_T, 1024> stream_index_c("index_c");
    GetC(num_v, c, stream_c, true);
#ifdef UPDATE_COUNT_HASH
    // compute based on HASH map for batch aggregation to reduce the random update
    // but it need some overhead for each batch output
    UpdateCount(num_v, num_c_out, stream_c, stream_index_c, count_c_single);
    SetIndexC(num_v, stream_index_c, index_c);
#else
    // directly do HBM update
    UpdateCount(num_v, num_c_out, stream_c, count_c_single, index_c);
#endif
}

void UpdateAdder(int num_c_out, hls::stream<DF_D_T, 1024>& stream_c_single, hls::stream<DF_D_T, 1024>& stream_c_out) {
    DF_V_T prev = 0;
Adder:
    for (int i = 0; i < num_c_out; i++) {
#pragma HLS pipeline II = 1
        DF_V_T c = stream_c_single.read();
        DF_V_T tmp = c + prev;
        stream_c_out.write(tmp);
        prev = tmp;
    }
}

void WriteAdder(int num_c_out, hls::stream<DF_D_T, 1024>& stream_c_out, DF_V_T* count_c) {
WriteAdder:
    for (int i = 0; i < num_c_out; i++) {
#pragma HLS pipeline II = 1
        count_c[i] = stream_c_out.read();
    }
}

// accumulate c one after another to get the total offset for each single c
void Adder(int num_c_out, DF_V_T* count_c_single, DF_V_T* count_c) {
#pragma HLS inline off
#pragma HLS dataflow
    hls::stream<DF_D_T, 1024> stream_c_single("c_single");
    hls::stream<DF_D_T, 1024> stream_c_out("c_out");
    GetC(num_c_out, count_c_single, stream_c_single, false);
    UpdateAdder(num_c_out, stream_c_single, stream_c_out);
    WriteAdder(num_c_out, stream_c_out, count_c);
}

void ComputSequenceStart(int num_v,
                         hls::stream<DF_D_T, 1024>& stream_c,
                         hls::stream<DF_D_T, 1024>& stream_start,
                         DF_V_T* count_c) {
START:
    for (int i = 0; i < num_v; i++) {
#pragma HLS pipeline II = 1
        DF_V_T addr = stream_c.read();
        int start = 0;
        if (addr != 0) {
            start = count_c[addr - 1];
        }
        stream_start.write(start);
    }
}
#define OUTSTANDING 16
void ComputSequenceJump(int num_v,
                        hls::stream<DF_D_T, 1024>& stream_start,
                        hls::stream<DF_D_T, 1024>& stream_index_c,
                        DF_V_T* jump) {
JUMP:
    for (int i = 0; i < num_v; i++) {
#pragma HLS pipeline II = 1
        DF_V_T start = stream_start.read();
        DF_V_T index = stream_index_c.read();
        int jump_addr = index + start;
        jump[jump_addr] = i;
    }
}

// compute the jump map for V
// jump: split memory to each partition for different c
//       each partation record the all the offset address to the same cid
void ComputeJump(int num_v, DF_V_T* c, DF_V_T* jump, DF_V_T* count_c, DF_V_T* index_c) {
#pragma HLS inline off
#pragma HLS dataflow
    hls::stream<DF_D_T, 1024> stream_c("c");
    hls::stream<DF_D_T, 1024> stream_index_c("index_c");
    hls::stream<DF_D_T, 1024> stream_start("start");
    GetC(num_v, c, stream_c, false);
    GetC(num_v, index_c, stream_index_c, false);
    ComputSequenceStart(num_v, stream_c, stream_start, count_c);
    ComputSequenceJump(num_v, stream_start, stream_index_c, jump);
}

void LoadCountC(int num_c_out, DF_V_T* count_c, hls::stream<DF_V_T>& stream_count) {
    printf("start LoadCountC, num_c_out=%d\n", num_c_out);
LOADCOUNTC:
    for (int i = 0; i < num_c_out; i++) {
#pragma HLS pipeline II = 1
        DF_V_T count = count_c[i];
        stream_count.write(count);
    }
}

void LoadJump(int num_v, DF_V_T* jump, hls::stream<DF_V_T>& stream_jump) {
    printf("start LoadJump, num_v=%d\n", num_v);
LOADJUMP:
    for (int i = 0; i < num_v; i++) {
#pragma HLS pipeline II = 1
        int addr = jump[i];
        stream_jump.write(addr);
    }
}

void GetV(int num_v,
          int num_c_out,
          DF_V_T* offset_in,
          hls::stream<DF_V_T>& stream_count,
          hls::stream<DF_V_T>& stream_jump,
          hls::stream<AP2_V_T>& stream_v,
          hls::stream<bool>& stream_endv) {
    printf("start GetV, num_c_out=%d\n", num_c_out);
    int count_prev = 0;
GETV:
    for (int i = 0; i < num_c_out; i++) {
        DF_V_T count = stream_count.read();
    GETV_COUNT:
        for (int j = count_prev; j < count; j++) {
#pragma HLS pipeline II = 1
            int addr = stream_jump.read();
            DF_V_T adj0 = offset_in[addr];
            DF_V_T adj1 = offset_in[addr + 1];
            AP2_V_T tmp;
            tmp(31, 0) = adj0;
            tmp(63, 32) = adj1;
            stream_v.write(tmp);
            bool endv;
            if (j == count - 1) {
                endv = 1;
            } else {
                endv = 0;
            }
            stream_endv.write(endv);
        }
        count_prev = count;
    }
}

void GetEW(int num_v,
           int num_c_out,
           DF_V_T* c,
           DF_D_T* edges_in,
           DF_W_T* weights_in,
           hls::stream<AP2_V_T>& stream_v,
           hls::stream<bool>& stream_endv,
           hls::stream<DF_D_T>& stream_e,
           hls::stream<DF_WI_T>& stream_w,
           hls::stream<bool>& stream_endew) {
    printf("start GetEW, num_c_out=%d\n", num_c_out);
    int count = 0;
    int count_v = 0;
    DF_D_T adj0 = 0;
    DF_D_T adj1 = 0;
    int degree = 0;
GETEW:
    for (int i = 0; i < num_c_out; i++) {
        int endv = false;
        bool has_self = false;
        bool last_e = true;
        int count_e = 0;
        int index_e = 0;
    GETEW_SINGLE:
        while (!(endv && last_e)) {
#pragma HLS pipeline II = 1
            if (last_e) {
                AP2_V_T tmp = stream_v.read();
                adj0 = tmp(31, 0);
                adj1 = tmp(63, 32);
                endv = stream_endv.read();
                degree = adj1 - adj0;
            }
            if (degree == 0) { // each V include isolate V have edges
                stream_e.write(i);
                stream_w.write(0);
                stream_endew.write(false);
                has_self = true;
                count_e = 0;
                last_e = true;
                count++;
            } else {
                index_e = adj0 + count_e;
                DF_D_T e = c[edges_in[index_e]];
                DF_W_T w = weights_in[index_e];
                stream_e.write(e);
#ifdef FIX_POINT
                DF_WI_T w_i = ToInt(w, 32);
                stream_w.write(w_i);
#else
                stream_w.write(w);
#endif
                if (e == i) has_self = true;
                last_e = (index_e == adj1 - 1);
                stream_endew.write(false);
                if (last_e) {
                    count_e = 0;
                } else {
                    count_e++;
                }
            }
        }
        if (!has_self) { // if no self, add one edge and weight 0
            stream_e.write(i);
            stream_w.write(0);
            stream_endew.write(false);
        }
        stream_e.write(-1);
        stream_w.write(-1);
        stream_endew.write(true);
        count_v++;
    }
}

void HashAggregateSmall(DF_V_T& num_cid_small,
                        DF_V_T mem_key[NUM_SMALL],
                        AggRAM<DF_WI_T, NUM_SMALL_LOG, NUM_SMALL>& mem_agg,

                        hls::stream<DF_D_T, 1024>& stream_e,
                        hls::stream<DF_WI_T>& stream_w,
                        hls::stream<bool, 1024>& stream_endew,

                        hls::stream<DF_D_T, 1024>& stream_overe,
                        hls::stream<DF_WI_T>& stream_overw,
                        hls::stream<bool, 1024>& stream_overendew) {
#pragma HLS inline
    bool endew = false;
    bool has_overflow = false;
    num_cid_small = 0;
SMALLAGG_1:
    while (!endew) {
#pragma HLS pipeline II = 1
        DF_D_T e = stream_e.read();
        DF_WI_T w = stream_w.read();
        endew = stream_endew.read();
        bool isFirst = true;
        if (!endew) {
            bool overflow = false;
            DF_V_T key = e;
            ap_uint<NUM_SMALL_LOG> addr;
            addr = MemAgg_core_key(key, num_cid_small, mem_key, overflow);
            if (overflow) {
                stream_overe.write(e);
                stream_overw.write(w);
                stream_overendew.write(false);
                has_overflow = true;
            } else {
                mem_agg.Aggregate(addr, w, isFirst);
                isFirst = false;
            }
        }
    }
    // write one as end
    mem_agg.Reset();
    stream_overe.write(-1);
    stream_overw.write(-1);
    stream_overendew.write(true);
}

template <int HASH_WIDTH>
void HashAggregateBigAddrGen(int index_v,
                             HashAgg<int, DF_WI_T, HASH_WIDTH>& myHash,
                             hls::stream<DF_D_T, 1024>& stream_e,
                             hls::stream<DF_WI_T>& stream_w,
                             hls::stream<bool, 1024>& stream_endew,

                             hls::stream<DF_D_T, 1024>& stream_bige,
                             hls::stream<DF_WI_T>& stream_bigw,
                             hls::stream<bool, 1024>& stream_bigendew,
                             hls::stream<int, 1024>& stream_bigaddr,
                             hls::stream<bool, 1024>& stream_bigfirst) {
    bool endew = false;
    myHash.valAdd[0] = 0xffffffffffffffff;
HASHBIG_ADDR:
    while (!endew) {
#pragma HLS pipeline II = 1
        DF_D_T e = stream_e.read();
        DF_WI_T w = stream_w.read();
        endew = stream_endew.read();
        if (!endew) {
            bool isFirst = true;
            int add = myHash.GetAddr(e, isFirst);
            stream_bige.write(e);
            stream_bigw.write(w);
            stream_bigendew.write(false);
            stream_bigaddr.write(add);
            stream_bigfirst.write(isFirst);
        }
    }
    stream_bige.write(-1);
    stream_bigw.write(-1);
    stream_bigendew.write(true);
}

template <int HASH_WIDTH>
void HashAggregateBigDataGen(HashAgg<int, DF_WI_T, HASH_WIDTH>& myHash,
                             hls::stream<DF_D_T, 1024>& stream_bige,
                             hls::stream<DF_WI_T>& stream_bigw,
                             hls::stream<bool, 1024>& stream_bigendew,
                             hls::stream<int, 1024>& stream_bigaddr,
                             hls::stream<bool, 1024>& stream_bigfirst,

                             hls::stream<DF_D_T, 1024>& stream_mide,
                             hls::stream<DF_WI_T>& stream_midw,
                             hls::stream<bool, 1024>& stream_midendew) {
    bool endew = false;
HASHBIG_DATA:
    while (!endew) {
#pragma HLS pipeline II = 1
        DF_D_T e = stream_bige.read();
        DF_WI_T w = stream_bigw.read();
        endew = stream_bigendew.read();
        if (!endew) {
            int addr = stream_bigaddr.read();
            bool isFirst = stream_bigfirst.read();
            if (!myHash.TryToSet(e, w, addr, isFirst)) {
                stream_mide.write(e);
                stream_midw.write(w);
                stream_midendew.write(false);
            }
        }
    }
    stream_mide.write(-1);
    stream_midw.write(-1);
    stream_midendew.write(true);
}

void HashAggregateScan(int index_v,
                       ScanAgg<int, DF_WI_T, 0>& myScan,
                       hls::stream<DF_D_T, 1024>& stream_mide,
                       hls::stream<DF_WI_T>& stream_midw,
                       hls::stream<bool, 1024>& stream_midendew) {
    bool midendew = false;
HASHAGG_2:
    while (!midendew) {
#pragma HLS pipeline II = 1
        DF_D_T e = stream_mide.read();
        DF_WI_T w = stream_midw.read();
        midendew = stream_midendew.read();
        if (!midendew) {
            myScan.AggWeight(e, w);
        }
    }
}

template <int HASH_WIDTH, int HASH_WIDTH2, int SCAN_WIDTH>
void HashAggregateBig(int index_v,
                      HashAgg<int, DF_WI_T, HASH_WIDTH>& myHash,
                      HashAgg<int, DF_WI_T, HASH_WIDTH>& myHash_2nd,
                      HashAgg<int, DF_WI_T, HASH_WIDTH>& myHash_3rd,
                      HashAgg<int, DF_WI_T, HASH_WIDTH>& myHash_4th,
                      HashAgg<int, DF_WI_T, HASH_WIDTH2>& myHash_5th,
                      ScanAgg<int, DF_WI_T, 0>& myScan,

                      hls::stream<DF_D_T, 1024>& stream_overe,
                      hls::stream<DF_WI_T, 1024>& stream_overw,
                      hls::stream<bool, 1024>& stream_overendew) {
#pragma HLS inline off

    hls::stream<DF_D_T, 1024> stream_bige("bige");
    hls::stream<DF_WI_T, 1024> stream_bigw("bigw");
    hls::stream<bool, 1024> stream_bigendew("bigendew");
    hls::stream<int, 1024> stream_bigaddr("bigaddr");
    hls::stream<bool, 1024> stream_bigfirst("bigfirst");

    hls::stream<DF_D_T, 1024> stream_overe_2nd("overe_2nd");
    hls::stream<DF_WI_T, 1024> stream_overw_2nd("overw_2nd");
    hls::stream<bool, 1024> stream_overendew_2nd("overendew_2nd");

    hls::stream<DF_D_T, 1024> stream_bige_2nd("bige_2nd");
    hls::stream<DF_WI_T, 1024> stream_bigw_2nd("bigw_2nd");
    hls::stream<bool, 1024> stream_bigendew_2nd("bigendew_2nd");
    hls::stream<int, 1024> stream_bigaddr_2nd("bigaddr_2nd");
    hls::stream<bool, 1024> stream_bigfirst_2nd("bigfirst_2nd");

    hls::stream<DF_D_T, 1024> stream_overe_3rd("overe_3rd");
    hls::stream<DF_WI_T, 1024> stream_overw_3rd("overw_3rd");
    hls::stream<bool, 1024> stream_overendew_3rd("overendew_3rd");

    hls::stream<DF_D_T, 1024> stream_bige_3rd("bige_3rd");
    hls::stream<DF_WI_T, 1024> stream_bigw_3rd("bigw_3rd");
    hls::stream<bool, 1024> stream_bigendew_3rd("bigendew_3rd");
    hls::stream<int, 1024> stream_bigaddr_3rd("bigaddr_3rd");
    hls::stream<bool, 1024> stream_bigfirst_3rd("bigfirst_3rd");

    hls::stream<DF_D_T, 1024> stream_overe_4th("overe_4th");
    hls::stream<DF_WI_T, 1024> stream_overw_4th("overw_4th");
    hls::stream<bool, 1024> stream_overendew_4th("overendew_4th");

    hls::stream<DF_D_T, 1024> stream_bige_4th("bige_4th");
    hls::stream<DF_WI_T, 1024> stream_bigw_4th("bigw_4th");
    hls::stream<bool, 1024> stream_bigendew_4th("bigendew_4th");
    hls::stream<int, 1024> stream_bigaddr_4th("bigaddr_4th");
    hls::stream<bool, 1024> stream_bigfirst_4th("bigfirst_4th");

    hls::stream<DF_D_T, 1024> stream_overe_5th("overe_5th");
    hls::stream<DF_WI_T, 1024> stream_overw_5th("overw_5th");
    hls::stream<bool, 1024> stream_overendew_5th("overendew_5th");

    hls::stream<DF_D_T, 1024> stream_bige_5th("bige_5th");
    hls::stream<DF_WI_T, 1024> stream_bigw_5th("bigw_5th");
    hls::stream<bool, 1024> stream_bigendew_5th("bigendew_5th");
    hls::stream<int, 1024> stream_bigaddr_5th("bigaddr_5th");
    hls::stream<bool, 1024> stream_bigfirst_5th("bigfirst_5th");

    hls::stream<DF_D_T, 1024> stream_mide("mide");
    hls::stream<DF_WI_T, 1024> stream_midw("midw");
    hls::stream<bool, 1024> stream_midendew("midendew");

#pragma HLS dataflow
    HashAggregateBigAddrGen<HASH_WIDTH>(index_v, myHash, stream_overe, stream_overw, stream_overendew, stream_bige,
                                        stream_bigw, stream_bigendew, stream_bigaddr, stream_bigfirst);

    HashAggregateBigDataGen<HASH_WIDTH>(myHash, stream_bige, stream_bigw, stream_bigendew, stream_bigaddr,
                                        stream_bigfirst, stream_overe_2nd, stream_overw_2nd, stream_overendew_2nd);

    HashAggregateBigAddrGen<HASH_WIDTH>(index_v, myHash_2nd, stream_overe_2nd, stream_overw_2nd, stream_overendew_2nd,
                                        stream_bige_2nd, stream_bigw_2nd, stream_bigendew_2nd, stream_bigaddr_2nd,
                                        stream_bigfirst_2nd);

    HashAggregateBigDataGen<HASH_WIDTH>(myHash_2nd, stream_bige_2nd, stream_bigw_2nd, stream_bigendew_2nd,
                                        stream_bigaddr_2nd, stream_bigfirst_2nd, stream_overe_3rd, stream_overw_3rd,
                                        stream_overendew_3rd);

    HashAggregateBigAddrGen<HASH_WIDTH>(index_v, myHash_3rd, stream_overe_3rd, stream_overw_3rd, stream_overendew_3rd,
                                        stream_bige_3rd, stream_bigw_3rd, stream_bigendew_3rd, stream_bigaddr_3rd,
                                        stream_bigfirst_3rd);

    HashAggregateBigDataGen<HASH_WIDTH>(myHash_3rd, stream_bige_3rd, stream_bigw_3rd, stream_bigendew_3rd,
                                        stream_bigaddr_3rd, stream_bigfirst_3rd, stream_overe_4th, stream_overw_4th,
                                        stream_overendew_4th);

    HashAggregateBigAddrGen<HASH_WIDTH>(index_v, myHash_4th, stream_overe_4th, stream_overw_4th, stream_overendew_4th,
                                        stream_bige_4th, stream_bigw_4th, stream_bigendew_4th, stream_bigaddr_4th,
                                        stream_bigfirst_4th);

    HashAggregateBigDataGen<HASH_WIDTH>(myHash_4th, stream_bige_4th, stream_bigw_4th, stream_bigendew_4th,
                                        stream_bigaddr_4th, stream_bigfirst_4th, stream_overe_5th, stream_overw_5th,
                                        stream_overendew_5th);

    HashAggregateBigAddrGen<HASH_WIDTH2>(index_v, myHash_5th, stream_overe_5th, stream_overw_5th, stream_overendew_5th,
                                         stream_bige_5th, stream_bigw_5th, stream_bigendew_5th, stream_bigaddr_5th,
                                         stream_bigfirst_5th);

    HashAggregateBigDataGen<HASH_WIDTH2>(myHash_5th, stream_bige_5th, stream_bigw_5th, stream_bigendew_5th,
                                         stream_bigaddr_5th, stream_bigfirst_5th, stream_mide, stream_midw,
                                         stream_midendew);

    HashAggregateScan(index_v, myScan, stream_mide, stream_midw, stream_midendew);
}

void HashAggregateSmallTop(int num_c_out,
                           hls::stream<DF_D_T, 1024>& stream_e,
                           hls::stream<DF_WI_T, 1024>& stream_w,
                           hls::stream<bool, 1024>& stream_endew,

                           hls::stream<DF_D_T, 1024>& stream_overe,
                           hls::stream<DF_WI_T, 1024>& stream_overw,
                           hls::stream<bool, 1024>& stream_overendew,

                           hls::stream<DF_D_T, 1024>& stream_oute1,
                           hls::stream<DF_WI_T, 1024>& stream_outw1,
                           hls::stream<bool, 1024>& stream_endout1) {
    DF_V_T num_cid_small = 0;
    DF_V_T mem_key[NUM_SMALL];
#pragma HLS ARRAY_PARTITION variable = mem_key complete
    AggRAM<DF_WI_T, NUM_SMALL_LOG, NUM_SMALL> mem_agg;

HASHAGG_SMALL:
    for (int i = 0; i < num_c_out; i++) {
        HashAggregateSmall(num_cid_small, mem_key, mem_agg, stream_e, stream_w, stream_endew, stream_overe,
                           stream_overw, stream_overendew);

    COLLECT_SMALL:
        for (int i = 0; i < num_cid_small; i++) {
#pragma HLS PIPELINE II = 1
            DF_WI_T tmp = mem_agg.mem[i];
            mem_agg.mem[i] = 0;
            DF_V_T addr = mem_key[i];
            stream_oute1.write(addr);
            stream_outw1.write(tmp);
            stream_endout1.write(false);
        }
        stream_oute1.write(num_cid_small);
        stream_outw1.write(-1);
        stream_endout1.write(true);
    }
}

void HashAggregateBigTop(int num_c_out,
                         hls::stream<DF_D_T, 1024>& stream_overe,
                         hls::stream<DF_WI_T, 1024>& stream_overw,
                         hls::stream<bool, 1024>& stream_overendew,

                         hls::stream<DF_D_T, 1024>& stream_oute2,
                         hls::stream<DF_WI_T, 1024>& stream_outw2,
                         hls::stream<bool, 1024>& stream_endout2) {
    const int HASH_WIDTH = 15;
#ifndef __SYNTHESIS__
    AggWE<int, DF_WI_T>* hash_we = (AggWE<int, DF_WI_T>*)malloc(sizeof(AggWE<int, DF_WI_T>) * (1 << HASH_WIDTH));
    ap_uint<W_URAM>* valAdd = (ap_uint<W_URAM>*)malloc(sizeof(ap_uint<W_URAM>) * (1 << HASH_WIDTH));
#else
    AggWE<int, DF_WI_T> hash_we[1 << HASH_WIDTH];
    ap_uint<W_URAM> valAdd[1 << HASH_WIDTH];
#endif
#pragma HLS RESOURCE variable = hash_we core = RAM_T2P_URAM
#pragma HLS RESOURCE variable = valAdd core = RAM_T2P_URAM
    HashAgg<int, DF_WI_T, HASH_WIDTH> myHash(hash_we, valAdd);

#ifndef __SYNTHESIS__
    AggWE<int, DF_WI_T>* hash_we_2nd = (AggWE<int, DF_WI_T>*)malloc(sizeof(AggWE<int, DF_WI_T>) * (1 << HASH_WIDTH));
    ap_uint<W_URAM>* valAdd_2nd = (ap_uint<W_URAM>*)malloc(sizeof(ap_uint<W_URAM>) * (1 << HASH_WIDTH));
#else
    AggWE<int, DF_WI_T> hash_we_2nd[1 << HASH_WIDTH];
    ap_uint<W_URAM> valAdd_2nd[1 << HASH_WIDTH];
#endif
#pragma HLS RESOURCE variable = hash_we_2nd core = RAM_T2P_URAM
#pragma HLS RESOURCE variable = valAdd_2nd core = RAM_T2P_URAM
    HashAgg<int, DF_WI_T, HASH_WIDTH> myHash_2nd(hash_we_2nd, valAdd_2nd);

#ifndef __SYNTHESIS__
    AggWE<int, DF_WI_T>* hash_we_3rd = (AggWE<int, DF_WI_T>*)malloc(sizeof(AggWE<int, DF_WI_T>) * (1 << HASH_WIDTH));
    ap_uint<W_URAM>* valAdd_3rd = (ap_uint<W_URAM>*)malloc(sizeof(ap_uint<W_URAM>) * (1 << HASH_WIDTH));
#else
    AggWE<int, DF_WI_T> hash_we_3rd[1 << HASH_WIDTH];
    ap_uint<W_URAM> valAdd_3rd[1 << HASH_WIDTH];
#endif
#pragma HLS RESOURCE variable = hash_we_3rd core = RAM_T2P_URAM
#pragma HLS RESOURCE variable = valAdd_3rd core = RAM_T2P_URAM
    HashAgg<int, DF_WI_T, HASH_WIDTH> myHash_3rd(hash_we_3rd, valAdd_3rd);

#ifndef __SYNTHESIS__
    AggWE<int, DF_WI_T>* hash_we_4th = (AggWE<int, DF_WI_T>*)malloc(sizeof(AggWE<int, DF_WI_T>) * (1 << HASH_WIDTH));
    ap_uint<W_URAM>* valAdd_4th = (ap_uint<W_URAM>*)malloc(sizeof(ap_uint<W_URAM>) * (1 << HASH_WIDTH));
#else
    AggWE<int, DF_WI_T> hash_we_4th[1 << HASH_WIDTH];
    ap_uint<W_URAM> valAdd_4th[1 << HASH_WIDTH];
#endif
#pragma HLS RESOURCE variable = hash_we_4th core = RAM_T2P_URAM
#pragma HLS RESOURCE variable = valAdd_4th core = RAM_T2P_URAM
    HashAgg<int, DF_WI_T, HASH_WIDTH> myHash_4th(hash_we_4th, valAdd_4th);

    const int HASH_WIDTH2 = 12;
#ifndef __SYNTHESIS__
    AggWE<int, DF_WI_T>* hash_we_5th = (AggWE<int, DF_WI_T>*)malloc(sizeof(AggWE<int, DF_WI_T>) * (1 << HASH_WIDTH2));
    ap_uint<W_URAM>* valAdd_5th = (ap_uint<W_URAM>*)malloc(sizeof(ap_uint<W_URAM>) * (1 << HASH_WIDTH2));
#else
    AggWE<int, DF_WI_T> hash_we_5th[1 << HASH_WIDTH2];
    ap_uint<W_URAM> valAdd_5th[1 << HASH_WIDTH2];
#endif
#pragma HLS RESOURCE variable = hash_we_5th core = RAM_T2P_URAM
#pragma HLS RESOURCE variable = valAdd_5th core = RAM_T2P_URAM
    HashAgg<int, DF_WI_T, HASH_WIDTH2> myHash_5th(hash_we_5th, valAdd_5th);

    const int SCAN_WIDTH = 17;
#ifndef __SYNTHESIS__
    AggWE<int, DF_WI_T>* hash_we_scan = (AggWE<int, DF_WI_T>*)malloc(sizeof(AggWE<int, DF_WI_T>) * (1 << SCAN_WIDTH));
#else
    AggWE<int, DF_WI_T> hash_we_scan[1 << SCAN_WIDTH];
#endif
#pragma HLS RESOURCE variable = hash_we_scan core = RAM_T2P_URAM
    ScanAgg<int, DF_WI_T, 0> myScan(hash_we_scan);

HASHAGG_BIG:
    for (int i = 0; i < num_c_out; i++) {
        HashAggregateBig<HASH_WIDTH, HASH_WIDTH2, SCAN_WIDTH>(i, myHash, myHash_2nd, myHash_3rd, myHash_4th, myHash_5th,
                                                              myScan, stream_overe, stream_overw, stream_overendew);

        DF_V_T overflow_count = myHash.cnt_agg + myScan.cnt_agg + myHash_2nd.cnt_agg + myHash_3rd.cnt_agg +
                                myHash_4th.cnt_agg + myHash_5th.cnt_agg;
        if (myHash.cnt_agg > 0) {
            myHash.Output(stream_oute2, stream_outw2, stream_endout2);
        }
        if (myHash_2nd.cnt_agg > 0) {
            myHash_2nd.Output(stream_oute2, stream_outw2, stream_endout2);
        }
        if (myHash_3rd.cnt_agg > 0) {
            myHash_3rd.Output(stream_oute2, stream_outw2, stream_endout2);
        }
        if (myHash_4th.cnt_agg > 0) {
            myHash_4th.Output(stream_oute2, stream_outw2, stream_endout2);
        }
        if (myHash_5th.cnt_agg > 0) {
            myHash_5th.Output(stream_oute2, stream_outw2, stream_endout2);
        }
        if (myScan.cnt_agg > 0) {
            myScan.Output(stream_oute2, stream_outw2, stream_endout2);
        }
        stream_oute2.write(overflow_count);
        stream_outw2.write(-1);
        stream_endout2.write(true);
    }
}

void HashAggregateCollect(int num_c_out,
                          hls::stream<DF_D_T, 1024>& stream_oute1,
                          hls::stream<DF_WI_T, 1024>& stream_outw1,
                          hls::stream<bool, 1024>& stream_endout1,

                          hls::stream<DF_D_T, 1024>& stream_oute2,
                          hls::stream<DF_WI_T, 1024>& stream_outw2,
                          hls::stream<bool, 1024>& stream_endout2,

                          hls::stream<DF_V_T, 1024>& stream_outv,
                          hls::stream<DF_D_T, 1024>& stream_oute,
                          hls::stream<DF_WI_T, 1024>& stream_outw,
                          hls::stream<bool, 1024>& stream_endout) {
HASHAGG_COLLECT:
    for (int i = 0; i < num_c_out; i++) {
        int num_cid_small = 0;
        bool small_end = false;
    COLLECT_SMALL:
        while (!small_end) {
#pragma HLS PIPELINE II = 1
            DF_D_T tmp_e = stream_oute1.read();
            DF_WI_T tmp_w = stream_outw1.read();
            small_end = stream_endout1.read();
            num_cid_small = tmp_e;
            if (!small_end) {
                stream_oute.write(tmp_e);
                stream_outw.write(tmp_w);
                stream_endout.write(small_end);
            }
        }
        int num_cid_big = 0;
        bool big_end = false;
    COLLECT_BIG:
        while (!big_end) {
#pragma HLS PIPELINE II = 1
            DF_D_T tmp_e = stream_oute2.read();
            DF_WI_T tmp_w = stream_outw2.read();
            big_end = stream_endout2.read();
            num_cid_big = tmp_e;
            if (!big_end) {
                stream_oute.write(tmp_e);
                stream_outw.write(tmp_w);
                stream_endout.write(big_end);
            }
        }
        DF_V_T v = num_cid_small + num_cid_big;
        stream_outv.write(v);
    }
    stream_endout.write(true);
}

// Hash dataflow sequence
// SMALL(64)
// => BIG(32K)
// => BIG(32K)
// => BIG(32K)
// => BIG(32K)
// => BIG(4K)
// => SCAN(128K)
// => Output for each aggregation
void HashAggregateDataflow(int num_c_out,
                           hls::stream<DF_D_T, 1024>& stream_e,
                           hls::stream<DF_WI_T, 1024>& stream_w,
                           hls::stream<bool, 1024>& stream_endew,

                           hls::stream<DF_V_T, 1024>& stream_outv,
                           hls::stream<DF_D_T, 1024>& stream_oute,
                           hls::stream<DF_WI_T, 1024>& stream_outw,
                           hls::stream<bool, 1024>& stream_endout) {
    printf("start HashAggregateDataflow\n");
    hls::stream<DF_D_T, 1024> stream_overe("overe");
    hls::stream<DF_WI_T, 1024> stream_overw("overw");
    hls::stream<bool, 1024> stream_overendew("overendew");

    hls::stream<DF_D_T, 1024> stream_oute1("oute1");
    hls::stream<DF_WI_T, 1024> stream_outw1("outw1");
    hls::stream<bool, 1024> stream_endout1("endout1");

    hls::stream<DF_D_T, 1024> stream_oute2("oute2");
    hls::stream<DF_WI_T, 1024> stream_outw2("outw2");
    hls::stream<bool, 1024> stream_endout2("endout2");

#pragma HLS dataflow
    HashAggregateSmallTop(num_c_out, stream_e, stream_w, stream_endew, stream_overe, stream_overw, stream_overendew,
                          stream_oute1, stream_outw1, stream_endout1);

    HashAggregateBigTop(num_c_out, stream_overe, stream_overw, stream_overendew, stream_oute2, stream_outw2,
                        stream_endout2);

    HashAggregateCollect(num_c_out, stream_oute1, stream_outw1, stream_endout1, stream_oute2, stream_outw2,
                         stream_endout2, stream_outv, stream_oute, stream_outw, stream_endout);
}

void SetV(int num_c_out, hls::stream<DF_V_T>& stream_outv, DF_V_T* offset_out) {
    printf("start SetV, num_c_out=%d\n", num_c_out);
    offset_out[0] = 0;
    int tmp_prev = 0;
SETV:
    for (int i = 0; i < num_c_out; i++) {
#pragma HLS pipeline II = 1
        int tmp = stream_outv.read();
        int tmp_add = tmp + tmp_prev;
        offset_out[i + 1] = tmp_add;
        tmp_prev = tmp_add;
    }
}

void SetEW(int num_c_out,
           int* num_e_out,
           hls::stream<DF_D_T, 1024>& stream_oute,
           hls::stream<DF_WI_T, 1024>& stream_outw,
           hls::stream<bool, 1024>& stream_endout,
           DF_D_T* edges_out,
           DF_W_T* weights_out) {
    printf("start SetEW, num_c_out=%d\n", num_c_out);
    bool end = stream_endout.read();
    int count = 0;
SETEW:
    while (!end) {
#pragma HLS pipeline II = 1
        edges_out[count] = stream_oute.read();
#ifdef FIX_POINT
        DF_WI_T tmp = stream_outw.read();
        weights_out[count] = ToFloat(tmp, SCL);
#else
        DF_W_T tmp = stream_outw.read();
        weights_out[count] = tmp;
#endif
        end = stream_endout.read();
        count++;
    }
    *num_e_out = count;
    printf("SetEW: count = %d\n", count);
}

void DoMergeHLS(int num_v,
                int num_e,
                int num_c_out,
                int* num_e_out,
                DF_V_T* jump,
                DF_V_T* count_c,
                DF_V_T* c,
                DF_D_T* edges_in,
                DF_V_T* offset_in,
                DF_W_T* weights_in,
                DF_V_T* offset_out,
                DF_D_T* edges_out,
                DF_W_T* weights_out) {
#pragma HLS inline off
    printf("start DoMergeHls\n");
    hls::stream<DF_V_T, 1024> stream_count("count");
    hls::stream<DF_V_T, 1024> stream_jump("jump");
    hls::stream<DF_V_T, 1024> stream_first("first");
    hls::stream<AP2_V_T, 1024> stream_v("v");
    hls::stream<bool, 1024> stream_endv("endv");
    hls::stream<DF_D_T, 1024> stream_e("e");
    hls::stream<DF_WI_T, 1024> stream_w("w");
    hls::stream<bool, 1024> stream_endew("endew");
    hls::stream<DF_V_T, 1024> stream_outv("outv");
    hls::stream<DF_D_T, 1024> stream_oute("oute");
    hls::stream<DF_WI_T, 1024> stream_outw("outw");
    hls::stream<bool, 1024> stream_endout("endout");

#pragma HLS DATAFLOW

    LoadCountC(num_c_out, count_c, stream_count);

    LoadJump(num_v, jump, stream_jump);

    GetV(num_v, num_c_out, offset_in, stream_count, stream_jump, stream_v, stream_endv);

    GetEW(num_v, num_c_out, c, edges_in, weights_in, stream_v, stream_endv, stream_e, stream_w, stream_endew);

    HashAggregateDataflow(num_c_out, stream_e, stream_w, stream_endew, stream_outv, stream_oute, stream_outw,
                          stream_endout);

    SetV(num_c_out, stream_outv, offset_out);

    SetEW(num_c_out, num_e_out, stream_oute, stream_outw, stream_endout, edges_out, weights_out);
}

// calculate jump address for the V based on C
void Jump(int num_v, int num_c_out, DF_V_T* c, DF_V_T* count_c_single, DF_V_T* jump, DF_V_T* count_c, DF_V_T* index_c) {
    ComputeCount(num_v, num_c_out, c, count_c_single, index_c);
    Adder(num_c_out, count_c_single, count_c);
    ComputeJump(num_v, c, jump, count_c, index_c);
}
} // namespace merge

/**
 * @brief In graph algorithm, sometimes we need to merge a large graph to a small graph based on specific sequence.
 *
 * @param num_v          input vertex number, read only and the depth is 1
 * @param num_e          input edge number. read only and the depth is 1
 * @param num_c_out      output vertex number, read only and the depth is 1
 * @param num_e_out      output edge number, write only and the depth is 1
 * @param offset_in      input vertex offset, read only and the depth is NV
 * @param edges_in       edges for each input vertex, read only and the depth is NE
 * @param weights_in     weight for each input edge, read only and the depth is NE
 * @param c              map the original vertex id to merged vertex, read only and the depth is NV
 * @param offset_out     output vertex offset, write only and the depth is NV
 * @param edges_out      edges for each output vertex, write only and the depth is NE
 * @param weights_out    weight for each output edge, write only and the depth is NE
 * @param count_c_single get the number of each c, read / write and the depth is NC
 * @param jump           random vertex access sequence, read / write and the depth is NV
 * @param count_c        aggregate offset for the c based on count_c_single, read / write and the depth is NC
 * @param index_c        get the id for the c in same cid, read / write and the depth is NV
 *
 */

void merge_kernel_core(int num_v,
                       int num_e,
                       int num_c_out,
                       int* num_e_out,
                       DF_V_T* offset_in,
                       DF_D_T* edges_in,
                       DF_W_T* weights_in,
                       DF_V_T* c,
                       DF_V_T* offset_out,
                       DF_D_T* edges_out,
                       DF_W_T* weights_out,
                       DF_V_T* count_c_single,
                       DF_V_T* jump,
                       DF_V_T* count_c,
                       DF_V_T* index_c) {
    merge::Jump(num_v, num_c_out, c, count_c_single, jump, count_c, index_c);
    merge::DoMergeHLS(num_v, num_e, num_c_out, num_e_out, jump, count_c, c, edges_in, offset_in, weights_in, offset_out,
                      edges_out, weights_out);
}

} // namespace graph
} // namespace xf
#endif // XF_GRAPH_MERGE_HPP
