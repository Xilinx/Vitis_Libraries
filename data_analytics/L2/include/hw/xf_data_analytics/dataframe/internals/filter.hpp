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

/**
 * @file dynamic_filter.hpp
 * @brief This file contains run-time-configurable filter primitive.
 *
 * This file is part of Vitis Database Library.
 */

#ifndef _FILTER_H_
#define _FILTER_H_

#include "ap_int.h"
#include "hls_stream.h"
#include <cstddef>

namespace xf {
namespace data_analytics {
namespace dataframe {

/**
 *  * @brief Comparison operator of filter.
 *   */
enum FilterOp {
    FOP_DC = 0, ///< don't care, always true.
    FOP_EQ,     ///< equal
    FOP_NE,     ///< not equal
    FOP_GT,     ///< greater than, signed.
    FOP_LT,     ///< less than, signed.
    FOP_GE,     ///< greater than or equal, signed.
    FOP_LE,     ///< less than or equal, signed.
    //    FOP_GTU,    ///< greater than, unsigned.
    //    FOP_LTU,    ///< less than, unsigned.
    //    FOP_GEU,    ///< greater than or equal, unsigned.
    //    FOP_LEU     ///< less than or equal, unsigned.
};

/// @brief width of comparison operator in bits.
enum { FilterOpWidth = 4 };

// compare: a*10^exp and b. exp>=0
inline void numericCoreComp(ap_int<56> a, ap_int<56> b, ap_uint<8> exp, bool flag, ap_uint<7>& res) {
    //#pragma HLS inline off
    static const ap_uint<56> POW10[17] = {1e+0, 1e+1,  1e+2,  1e+3,  1e+4,  1e+5,  1e+6,  1e+7, 1e+8,
                                          1e+9, 1e+10, 1e+11, 1e+12, 1e+13, 1e+14, 1e+15, 1e+16};
    ap_int<111> d = a * POW10[exp];
#ifndef __SYNTHESIS__
// std::cout << std::setprecision(15) << "GT: a=" << a << ", b=" << b << ", exp=" << exp << ", d=" << d << std::endl;
#endif
    if (exp > 16) { // overflow
        if ((a > 0) == flag)
            res = 0b0101101;
        else
            res = 0b1010101;
    } else {
        if (d == b)
            res = 0b1100011;
        else if ((d > b) == flag)
            res = 0b0101101;
        else
            res = 0b1010101;
    }
}

// comp <=> FilterOp
inline void numericComp(ap_uint<64> a, ap_uint<64> b, ap_uint<7>& comp) {
#pragma HLS pipeline ii = 1
    ap_int<8> a_exp = a(63, 56);
    ap_int<8> b_exp = b(63, 56);
    ap_int<56> d1, d2;
    ap_uint<8> exp;
    bool flag;
    if (a_exp > b_exp) {
        d1 = a(55, 0);
        d2 = b(55, 0);
        exp = a_exp - b_exp;
        flag = true;
    } else {
        d1 = b(55, 0);
        d2 = a(55, 0);
        exp = b_exp - a_exp;
        flag = false;
    }
    numericCoreComp(d1, d2, exp, flag, comp);
}

/// @brief Static Information about true table module.
template <int NCOL>
struct true_table_info {
    /// type of config stream.
    typedef ap_uint<64> cfg_type;
    /// width of address.
    static const size_t addr_width = NCOL; //+ NCOL * (NCOL - 1) / 2;
    /// number of dwords to read for complete table.
    static const size_t dwords_num = addr_width > 6 ? (1ul << (addr_width - 6)) : 1ul;
};

// ------------------------------------------------------------

template <int W>
struct var_const_cmp_info {
    typedef struct {
        ap_uint<FilterOpWidth> lop;
        ap_uint<FilterOpWidth> rop;
        ap_uint<W> l;
        ap_uint<W> r;
        ap_uint<1> en;
    } cfg_type;
};

inline void numericEQ(ap_uint<64> d) {}

template <int W>
bool var_const_cmp(ap_uint<137> cfg, ap_uint<W> x) {
    ap_int<W> l;
    l.range(W - 1, 0) = cfg(W - 1, 0);
    ap_int<W> r;
    r.range(W - 1, 0) = cfg(2 * W - 1, W);
    ap_uint<4> rop = cfg(3 + 2 * W, 2 * W);
    ap_uint<4> lop = cfg(7 + 2 * W, 4 + 2 * W);

    bool bl = false, br = false;
    ap_uint<7> comp_l, comp_r;
    numericComp(x, l, comp_l);
    numericComp(x, r, comp_r);
    bl = comp_l[lop];
    br = comp_r[rop];
    bool ret = bl && br;
    return ret;
}

template <int W>
bool var_const_cmp(typename var_const_cmp_info<W>::cfg_type cfg, ap_uint<W> xu) {
#pragma HLS aggregate variable = cfg
    ap_int<W> x;
    x.range(W - 1, 0) = xu.range(W - 1, 0);
    ap_int<W> l;
    l.range(W - 1, 0) = cfg.l;
    // ap_uint<W> lu;
    // lu.range(W - 1, 0) = cfg.l;
    ap_int<W> r;
    r.range(W - 1, 0) = cfg.r;
    // ap_uint<W> ru;
    // ru.range(W - 1, 0) = cfg.r;

    bool bl = false, br = false;
    ap_uint<7> comp_l, comp_r;
    numericComp(x, l, comp_l);
    numericComp(x, r, comp_r);
    bl = comp_l[cfg.lop];
    br = comp_r[cfg.rop];
    // std::cout << "comp_l=" << comp_l << ", comp_r=" << comp_r << ",
    // one adder should be enough with a bit extra logic,
    // let Vivado do the magic.
    //    if (cfg.lop == FOP_DC) {
    //        bl = true;
    //    } else if (cfg.lop == FOP_EQ) {
    //        bl = (x == l);
    //    } else if (cfg.lop == FOP_NE) {
    //        bl = (x != l);
    //    } else if (cfg.lop == FOP_GT) {
    //        bl = (x > l);
    //    } else if (cfg.lop == FOP_GE) {
    //        bl = (x >= l);
    //        //} else if (cfg.lop == FOP_GTU) {
    //        //    bl = (xu > lu);
    //        //} else if (cfg.lop == FOP_GEU) {
    //        //    bl = (xu >= lu);
    //    } else {
    //#ifndef __SYNTHESIS__
    //        printf("Unsupported FOP in var_const_cmp left\n");
    //#endif
    //    }
    //
    //    if (cfg.rop == FOP_DC) {
    //        br = true;
    //    } else if (cfg.rop == FOP_EQ) {
    //        br = (x == r);
    //    } else if (cfg.rop == FOP_NE) {
    //        br = (x != r);
    //    } else if (cfg.rop == FOP_LT) {
    //        br = (x < r);
    //    } else if (cfg.rop == FOP_LE) {
    //        br = (x <= r);
    //        //} else if (cfg.rop == FOP_LTU) {
    //        //    br = (xu < ru);
    //        //} else if (cfg.rop == FOP_LEU) {
    //        //    br = (xu <= ru);
    //    } else {
    //#ifndef __SYNTHESIS__
    //        printf("Unsupported FOP in var_const_cmp right\n");
    //#endif
    //    }

    bool ret = bl && br;
    return ret;
}

// ------------------------------------------------------------

template <int W>
void compare_ops(hls::stream<typename var_const_cmp_info<W>::cfg_type>& cmpv0c_cfg_strm,
                 hls::stream<typename var_const_cmp_info<W>::cfg_type>& cmpv1c_cfg_strm,
                 hls::stream<typename var_const_cmp_info<W>::cfg_type>& cmpv2c_cfg_strm,
                 hls::stream<typename var_const_cmp_info<W>::cfg_type>& cmpv3c_cfg_strm,
                 //
                 hls::stream<ap_uint<W> >& v0_strm,
                 hls::stream<ap_uint<W> >& v1_strm,
                 hls::stream<ap_uint<W> >& v2_strm,
                 hls::stream<ap_uint<W> >& v3_strm,
                 hls::stream<bool>& e_v_strm,
                 //
                 hls::stream<ap_uint<4> >& addr_strm,
                 hls::stream<bool>& e_addr_strm) {
#pragma HLS aggregate variable = cmpv0c_cfg_strm
#pragma HLS aggregate variable = cmpv1c_cfg_strm
#pragma HLS aggregate variable = cmpv2c_cfg_strm
#pragma HLS aggregate variable = cmpv3c_cfg_strm

    typename var_const_cmp_info<W>::cfg_type cmpv0c = cmpv0c_cfg_strm.read();
    typename var_const_cmp_info<W>::cfg_type cmpv1c = cmpv1c_cfg_strm.read();
    typename var_const_cmp_info<W>::cfg_type cmpv2c = cmpv2c_cfg_strm.read();
    typename var_const_cmp_info<W>::cfg_type cmpv3c = cmpv3c_cfg_strm.read();

#pragma HLS aggregate variable = cmpv0c
#pragma HLS aggregate variable = cmpv1c
#pragma HLS aggregate variable = cmpv2c
#pragma HLS aggregate variable = cmpv3c

#if !defined(__SYNTHESIS__) && _XFDB_DYN_FILTER_DEBUG == 1
    int cnt = 0;
#endif

    bool e = false; // e_v_strm.read();
FILTER_MAIN_LOOP:
    do {
#pragma HLS pipeline II = 1
#pragma HLS pipeline style = flp
#pragma HLS loop_tripcount max = 100000 min = 100000
        e = e_v_strm.read();
        ap_uint<W> v0 = v0_strm.read();
        ap_uint<W> v1 = v1_strm.read();
        ap_uint<W> v2 = v2_strm.read();
        ap_uint<W> v3 = v3_strm.read();

        if (!e) {
            bool bv0c = var_const_cmp(cmpv0c, v0);
            bool bv1c = var_const_cmp(cmpv1c, v1);
            bool bv2c = var_const_cmp(cmpv2c, v2);
            bool bv3c = var_const_cmp(cmpv3c, v3);

            ap_uint<4> bvec;
            bvec[0] = bv0c;
            bvec[1] = bv1c;
            bvec[2] = bv2c;
            bvec[3] = bv3c;

            addr_strm.write(bvec);
            e_addr_strm.write(false);
#if !defined(__SYNTHESIS__) && _XFDB_DYN_FILTER_DEBUG == 1
            // std::cout << "bvec " << cnt << ": " << bvec.range(3, 0).to_string(2) <<
            // std::endl;
            ++cnt;
#endif
        }
    } while (!e);
    addr_strm.write(0);
    e_addr_strm.write(true);
#if !defined(__SYNTHESIS__) && _XFDB_DYN_FILTER_DEBUG == 1
    std::cout << "compare_ops has generated " << cnt << " addresses.\n";
#endif
}

// ------------------------------------------------------------

template <typename T, typename T2>
inline void _write_array(T* t, int i, T2 v) {
    t[i] = v;
}

template <typename T, typename T2>
inline void _read_array(const T* t, int i, T2* v) {
    *v = t[i];
}

template <int NCOL>
void true_table(hls::stream<ap_uint<64> >& cfg_strm,
                hls::stream<ap_uint<true_table_info<NCOL>::addr_width> >& addr_strm,
                hls::stream<bool>& e_addr_strm,
                hls::stream<bool>& b_strm,
                hls::stream<bool>& e_b_strm) {
    //
    const size_t addr_width = (size_t)true_table_info<NCOL>::addr_width;
    bool truetable[(1 << addr_width)];
    // to be care for the depth of truetable less than 64
    const size_t dwords_sz = (1 << addr_width) < 64 ? (1 << addr_width) : 64;
    // XXX break config into multiple 64-bit words, to avoid too wide stream.
    for (unsigned i = 0; i < true_table_info<NCOL>::dwords_num; ++i) {
        ap_uint<64> dw = cfg_strm.read();
    TRUE_TABLE_INIT32:
        for (int j = 0; j < dwords_sz; ++j) {
#pragma HLS pipeline II = 1
            //_write_array(truetable, (i * 32 + j), dw[j]);
            truetable[(i << 6) + j] = dw[j];
        }
    }
//
#if !defined(__SYNTHESIS__) && _XFDB_DYN_FILTER_DEBUG == 1
    std::cout << "true_table has finished configuration.\n";
    int cnt = 0;
#endif
    bool e = false; // e_addr_strm.read();
TRUE_TABLE_READ:
    do {
#pragma HLS pipeline II = 1
#pragma HLS pipeline style = flp
#pragma HLS loop_tripcount max = 100000 min = 100000
        e = e_addr_strm.read();
        ap_uint<addr_width> addr = addr_strm.read();
        if (!e) {
            bool b;
            //_read_array(truetable, addr, &b);
            b = truetable[addr];
            b_strm.write(b);
            e_b_strm.write(false);
#if !defined(__SYNTHESIS__) && _XFDB_DYN_FILTER_DEBUG == 1
            ++cnt;
#endif
        }
    } while (!e);
    b_strm.write(0);
    e_b_strm.write(true);
#if !defined(__SYNTHESIS__) && _XFDB_DYN_FILTER_DEBUG == 1
    std::cout << "true_table has done " << cnt << " lookups.\n";
#endif
}

// ------------------------------------------------------------

template <int WP>
void pred_pass(hls::stream<ap_uint<32> >& cfg_strm,
               hls::stream<ap_uint<WP> >& p_strm,
               hls::stream<bool>& b_strm,
               hls::stream<bool>& e_b_strm,
               //
               hls::stream<ap_uint<WP> >& pld_out_strm,
               hls::stream<bool>& e_out_strm) {
#if !defined(__SYNTHESIS__)
    int keep = 0, drop = 0;
#endif

    ap_uint<32> cfg = cfg_strm.read();
    bool eb = false;
loop_pred_pass:
    do {
#pragma HLS pipeline II = 2
//#pragma HLS pipeline style = flp
#pragma HLS loop_tripcount max = 100000 min = 100000
        eb = e_b_strm.read();
        bool b = b_strm.read();
        if (!eb) {
            ap_uint<WP> p0 = p_strm.read();
            ap_uint<WP> p1 = p_strm.read();
            if (b) {
                pld_out_strm.write(p0);
                e_out_strm.write(false);
                pld_out_strm.write(p1);
                e_out_strm.write(false);
#if !defined(__SYNTHESIS__)
                ++keep;
#endif
            } else {
                ;
#if !defined(__SYNTHESIS__)
                ++drop;
#endif
            }
        }
    } while (!eb);
    pld_out_strm.write(0);
    e_out_strm.write(true);
#if !defined(__SYNTHESIS__)
    std::cout << "pred_pass has kept " << keep << " rows, and dropped " << drop << " rows.\n";
#endif
}

// ------------------------------------------------------------
/**
 * @brief parse the 64 bit config stream into each block's config.
 *
 * @tparam NCOL number of variable columns.
 * @tparam W the width of data.
 */
template <int NCOL, int W>
void parse_filter_config(hls::stream<ap_uint<64> >& filter_cfg_strm, ap_uint<128 + 9> cmpvc_cfg[NCOL][4]) {
    // shuffle for condition column
    ap_uint<64> shuf_cfg = filter_cfg_strm.read();
    /* config variable-in-range comparison
     *
     * | 0 | immediate left  |
     * | 0 | immediate right |
     * | 0 . . . | lop | rop |
     *      ... x NCOL
     */
    {
    loop_parse_filter_config:
        for (int i = 0; i < 4; ++i) {
#pragma HLS pipeline off
            ap_uint<128 + 9> cfg;
            cfg(63, 0) = filter_cfg_strm.read();
            cfg(127, 64) = filter_cfg_strm.read();

            ap_uint<64> dw = filter_cfg_strm.read();
            cfg(136, 128) = dw.range(FilterOpWidth * 2, 0);
            for (int j = 0; j < NCOL; ++j) {
                cmpvc_cfg[j][i] = cfg;
            }
        }
    }
}

template <int NCOL, int W>
void parse_filter_config(hls::stream<ap_uint<64> >& filter_cfg_strm,
                         hls::stream<ap_uint<32> >& shuffle_cfg_strm_0,
                         hls::stream<ap_uint<32> >& shuffle_cfg_strm_1,
                         //
                         hls::stream<typename var_const_cmp_info<W>::cfg_type> cmpvc_cfg_strms[NCOL],
                         hls::stream<typename true_table_info<NCOL>::cfg_type>& tt_cfg_strm) {
    // shuffle for condition column
    ap_uint<64> shuf_cfg = filter_cfg_strm.read();
    shuffle_cfg_strm_0.write(shuf_cfg.range(31, 0));
    // shuffle for pldload out column
    shuffle_cfg_strm_1.write(shuf_cfg.range(63, 32));
    /* config variable-in-range comparison
     *
     * | 0 | immediate left  |
     * | 0 | immediate right |
     * | 0 . . . | lop | rop |
     *      ... x NCOL
     */
    {
    loop_parse_filter_config:
        for (int i = 0; i < NCOL; ++i) {
#pragma HLS pipeline off
            typename var_const_cmp_info<W>::cfg_type cfg;
            cfg.l.range(63, 0) = filter_cfg_strm.read();
            cfg.r.range(63, 0) = filter_cfg_strm.read();

            ap_uint<64> dw = filter_cfg_strm.read();
            cfg.lop = dw.range(FilterOpWidth * 2 - 1, FilterOpWidth);
            cfg.rop = dw.range(FilterOpWidth - 1, 0);
            cmpvc_cfg_strms[i].write(cfg);
        }
    }

    /* config truetable
     *
     * | . . dword . . . |
     *  ... x dwords_num
     */
    {
        const int NTTW = true_table_info<NCOL>::dwords_num;

        for (int i = 0; i < NTTW; ++i) {
            ap_uint<64> dw = filter_cfg_strm.read();
            tt_cfg_strm.write(dw);
        }
    }
}

template <int W>
void shuffle_in(hls::stream<ap_uint<32> >& cfg_strm,
                hls::stream<ap_uint<4 * W> >& pld_in_strm,
                hls::stream<bool>& e_in_strm,
                hls::stream<ap_uint<W> > v_strm[4],
                hls::stream<bool>& e_v_strm,
                hls::stream<ap_uint<W * 4> >& pld_out_strm) {
    // hls::stream<bool>& e_pld_strm) {

    // choose which column is set as condition
    ap_uint<32> cfg = cfg_strm.read();
    bool e = false;
    ap_uint<W> pld_reg[8];
#pragma HLS array_partition variable = pld_reg dim = 0
    ap_uint<1> idx = 0;
    do {
#pragma HLS pipeline II = 1
#pragma HLS pipeline style = flp
#pragma HLS loop_tripcount max = 100000 min = 100000
        e = e_in_strm.read();
        ap_uint<4 * W> pld = pld_in_strm.read();
        // e_pld_strm.write(false);
        if (!e) {
            pld_out_strm.write(pld);
            for (int i = 0; i < 4; ++i) {
                pld_reg[idx * 4 + i] = pld.range(W * (i + 1) - 1, W * i);
            }
            idx++;
            if (idx == 0) {
                for (int i = 0; i < 4; ++i) {
                    v_strm[i].write(pld_reg[cfg.range(4 * (i + 1) - 1, 4 * i)]);
                }
                e_v_strm.write(false);
            }
        }
    } while (!e);
    for (int i = 0; i < 4; ++i) {
#pragma HLS unroll
        v_strm[i].write(0);
    }
    e_v_strm.write(true);
    // pld_out_strm.write(0);
    // e_pld_strm.write(true);
}

template <int W>
void calcuFilter2(hls::stream<ap_uint<W> >& i_cond_strm,
                  hls::stream<bool>& i_e_cond_strm,
                  ap_uint<128 + 9> cmpvc_cfg[4],
                  hls::stream<bool>& o_fit_flag_strm) {
    bool e = i_e_cond_strm.read();
loop_calcuFilter:
    while (!e) {
        bool flag = true;
        for (int i = 0; i < 4; i++) {
#pragma HLS pipeline ii = 1
            ap_uint<W> in;
            if (cmpvc_cfg[i][136]) {
                in = i_cond_strm.read();
                e = i_e_cond_strm.read();
            }
            flag &= var_const_cmp(cmpvc_cfg[i], in);
        }
        o_fit_flag_strm.write(flag);
    }
    o_fit_flag_strm.write(0);
}

template <int W>
void calcuFilter(hls::stream<ap_uint<W> >& i_cond_strm,
                 hls::stream<bool>& i_e_cond_strm,
                 ap_uint<128 + 9> cmpvc_cfg[4],
                 hls::stream<bool>& o_fit_flag_strm) {
    bool e = i_e_cond_strm.read();
    ap_uint<2> cnt = 0;
    bool flag = true;
    ap_uint<W> in;
loop_calcuFilter:
    while (!e || cnt) {
#pragma HLS pipeline ii = 1
        if (cnt == 0) flag = true;
        if (cmpvc_cfg[cnt][136]) {
            in = i_cond_strm.read();
            e = i_e_cond_strm.read();
        }
        flag &= var_const_cmp(cmpvc_cfg[cnt], in);
        if (cnt == 3) {
            o_fit_flag_strm.write(flag);
            // std::cout << "calcuFilter flag=" << flag << ", cnt=" << cnt << std::endl;
        }
        cnt++;
    }
    o_fit_flag_strm.write(1);
}

template <int W, int WP>
void dynamicFilter(hls::stream<ap_uint<64> >& filter_cfg_strm,
                   //
                   hls::stream<ap_uint<WP> >& pld_in_strm,
                   hls::stream<bool>& e_in_strm,
                   //
                   hls::stream<ap_uint<WP> >& pld_out_strm,
                   hls::stream<bool>& e_pld_out_strm) {
#pragma HLS dataflow

    // split end signal for value and pldload.

    hls::stream<ap_uint<32> > shuffle_cfg_strm_0("shuffle_cfg_strm_0");
#pragma HLS stream variable = shuffle_cfg_strm_0 depth = 1
    hls::stream<ap_uint<32> > shuffle_cfg_strm_1("shuffle_cfg_strm_1");
#pragma HLS stream variable = shuffle_cfg_strm_1 depth = 1
    hls::stream<typename var_const_cmp_info<W>::cfg_type> cmpvc_cfg_strms[4];
#pragma HLS aggregate variable = cmpvc_cfg_strms
#pragma HLS array_partition variable = cmpvc_cfg_strms

    hls::stream<typename true_table_info<4>::cfg_type> tt_cfg_strm;
#pragma HLS aggregate variable = tt_cfg_strm
    // parse dynamic config.
    parse_filter_config<4, W>(filter_cfg_strm, // 32b
                              shuffle_cfg_strm_0, shuffle_cfg_strm_1,
                              //
                              cmpvc_cfg_strms, // ((32b im) * 2 + (4b op) * 2 = 96b) * 4
                              tt_cfg_strm);    // 10b addr: 2^10 = 32b * 32

    hls::stream<ap_uint<W> > v_strm[4];
#pragma HLS stream variable = v_strm depth = 8

    hls::stream<bool> e_v_strm("e_v_strm");
#pragma HLS stream variable = e_v_strm depth = 8

    hls::stream<ap_uint<WP> > p_strm("p_strm");
#pragma HLS stream variable = p_strm depth = 32

    shuffle_in(shuffle_cfg_strm_0, pld_in_strm, e_in_strm, v_strm, e_v_strm, p_strm);

    hls::stream<ap_uint<true_table_info<4>::addr_width> > addr_strm("addr_strm");
#pragma HLS stream variable = addr_strm depth = 8
    hls::stream<bool> e_addr_strm("e_addr_strm");
#pragma HLS stream variable = e_addr_strm depth = 8
    compare_ops(cmpvc_cfg_strms[0], cmpvc_cfg_strms[1], cmpvc_cfg_strms[2],
                cmpvc_cfg_strms[3], //
                //
                v_strm[0], v_strm[1], v_strm[2], v_strm[3], e_v_strm,
                //
                addr_strm, e_addr_strm);

    hls::stream<bool> b_strm;
#pragma HLS stream variable = b_strm depth = 8
    hls::stream<bool> e_b_strm;
#pragma HLS stream variable = e_b_strm depth = 8

    true_table<4>(tt_cfg_strm,
                  //
                  addr_strm, e_addr_strm,
                  //
                  b_strm, e_b_strm);

    pred_pass(shuffle_cfg_strm_1, p_strm, b_strm, e_b_strm,
              //
              pld_out_strm, e_pld_out_strm);
}

} // end namespace dataframe
} // end namespace data_analytics
} // end namespace xf

#endif
