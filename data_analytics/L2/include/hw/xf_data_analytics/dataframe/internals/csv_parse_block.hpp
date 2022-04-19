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

#ifndef _XF_GZIP_HPP_
#define _XF_GZIP_HPP_

#include <ap_int.h>
#include <hls_stream.h>
#include "inflate.hpp"
#include "xf_data_analytics/common/obj_interface.hpp"
#include "xf_data_analytics/dataframe/df_utils.hpp"
#include "xf_data_analytics/dataframe/parser_blocks/csv_parse_block_v1.hpp"

namespace xf {
namespace data_analytics {
namespace dataframe {

// Find the position of '\n'
template <int NM, int W = 64>
void preRoundRobin(hls::stream<ap_uint<W + W / 8> >& s,
                   hls::stream<ap_uint<W + W / 8> >& dataStrm,
                   hls::stream<ap_uint<4> >& lenStrm) {
    const int BN = W / 8;
    ap_uint<W + BN> in_data = s.read();
    ap_uint<BN> strb = in_data(BN - 1, 0);
    ap_uint<BN> vb = (32 - __builtin_clz(strb));
    if (strb == 0) vb = 0;
    in_data(BN - 1, 0) = vb;
    dataStrm.write(in_data);
    while (vb) {
#pragma HLS PIPELINE II = 1
        ap_uint<W> dw = in_data(W + BN - 1, BN);
        ap_uint<4> data_id = 0;
        for (int i = 0; i < BN; i++) {
            if (i < vb) {
                char d8 = dw((i + 1) * 8 - 1, i * 8);
                if (d8 == '\n') {
                    data_id = i + 1;
                    break;
                }
            }
        }
        lenStrm.write(data_id);
        in_data = s.read();
        strb = in_data(BN - 1, 0);
        vb = (32 - __builtin_clz(strb));
        if (strb == 0) vb = 0;
        in_data(BN - 1, 0) = vb;
        dataStrm.write(in_data);
    }
}

// round-robin split data to corresponding stream
// NM channel number
// BS Single data volume (unit: Byte)
template <int NM, int W = 64, int BS = 512>
void roundRobin(hls::stream<ap_uint<W + W / 8> >& s, // csv from decompression
                hls::stream<ap_uint<4> >& idStrm,
                hls::stream<ap_uint<W> > o_strm[NM],
                hls::stream<ap_uint<4> > l_strm[NM]) {
    const int BN = W / 8; // byte number
    ap_uint<W + BN> in_data = s.read();
    ap_uint<29> blk_offset[NM];
#pragma HLS array_partition variable = blk_offset dim = 0
    for (int i = 0; i < NM; i++) blk_offset[i] = 0;
    ap_uint<29> blk_id = 0;
    ap_uint<BN> vb = in_data(BN - 1, 0); // valid byte
    ap_uint<NM> data_offset = 0;
    bool s_flag = 0;
    ap_uint<4> nm = 0;
loop_roundRobin:
    while (vb) {
#pragma HLS pipeline II = 1
        // std::cout << "nm=" << nm << ", vb=" << vb << ", data_offset=" << data_offset << ", s_flag=" << s_flag
        //          << ", blk_id=" << blk_id << std::endl;
        ap_uint<W> dw;
        if (s_flag) {
            ap_uint<NM> ds = vb - data_offset;
            blk_id = ds + blk_offset[nm];
            if (ds > 0) {
                dw = in_data(W + BN - 1, BN + data_offset * 8);
                l_strm[nm].write(ds);
                o_strm[nm].write(dw);
            }
            in_data = s.read();
            vb = in_data(BN - 1, 0);
            s_flag = 0;
        } else {
            data_offset = idStrm.read();
            dw = in_data(W + BN - 1, BN);
            if (blk_id + vb > BS && data_offset > 0) {
                blk_offset[nm] = blk_id + vb - BS;
                l_strm[nm].write(data_offset);
                o_strm[nm].write(dw);
                s_flag = 1;
                nm = (nm + 1) % NM;
            } else {
                blk_id += vb;
                l_strm[nm].write(vb);
                o_strm[nm].write(dw);
                in_data = s.read();
                vb = in_data(BN - 1, 0);
            }
        }
    }
    // write end flag: 0
    for (int i = 0; i < NM; i++) {
        l_strm[i].write(0);
    }
}

template <int W>
void splitByte(hls::stream<ap_uint<W> >& i_strm, hls::stream<ap_uint<4> >& i_e_strm, hls::stream<ap_uint<9> >& o_strm) {
    ap_uint<4> len = i_e_strm.read();
    ap_uint<4> cnt = 0;
    ap_uint<W> data_in;
    ap_uint<9> data_out;
splitByte_loop:
    while (len) {
#pragma HLS pipeline II = 1
        // std::cout << "cnt=" << cnt << ", len=" << len << std::endl;
        if (cnt == 0) {
            data_in = i_strm.read();
            // std::cout << "len = " << len << ", data_in = " << std::hex << data_in << std::endl;
        }
        data_out(7, 0) = data_in(7, 0);
        data_out[8] = 0;
        o_strm.write(data_out);
        data_in(W - 9, 0) = data_in(W - 1, 8);
        cnt++;
        if (cnt == len) {
            cnt = 0;
            len = i_e_strm.read();
        }
    }
    data_out[8] = 1;
    o_strm.write(data_out);
}

template <int NM, int W>
void splitByteWrap(hls::stream<ap_uint<W> > i_strm[NM],
                   hls::stream<ap_uint<4> > i_e_strm[NM],
                   hls::stream<ap_uint<9> > o_strm[NM]) {
#pragma HLS dataflow
    for (int i = 0; i < NM; i++) {
#pragma HLS unroll
        splitByte(i_strm[i], i_e_strm[i], o_strm[i]);
    }
}

template <int NM, int UL = 1024, int W = 64>
void csv_parser(hls::stream<ap_uint<W + W / 8> >& in_strm,
                ap_uint<8> type_buf[NM][16],
                ap_uint<4> type_valid_buf[16],
                ap_uint<3> type_num[7],
                ap_uint<9>& num_of_column,
                hls::stream<ObjectAlter1>& o_obj_strm) {
#pragma HLS dataflow
    hls::stream<ap_uint<W + W / 8> > in_strm2("in2Stream");
    hls::stream<ap_uint<4> > id_strm("idStream");
#pragma HLS stream variable = in_strm2 depth = 16
#pragma HLS stream variable = id_strm depth = 16
    preRoundRobin<NM, W>(in_strm, in_strm2, id_strm);

    hls::stream<ap_uint<W> > o_strm[NM];
    hls::stream<ap_uint<4> > l_strm[NM];
#pragma HLS array_partition variable = o_strm dim = 0
#pragma HLS array_partition variable = l_strm dim = 0
#pragma HLS stream variable = o_strm depth = 512
#pragma HLS stream variable = l_strm depth = 512
#pragma HLS bind_storage variable = o_strm type = fifo impl = bram
#pragma HLS bind_storage variable = l_strm type = fifo impl = bram
    roundRobin<NM, W>(in_strm2, id_strm, o_strm, l_strm);

    hls::stream<ap_uint<9> > split_byte_strm[NM];
#pragma HLS stream variable = split_byte_strm depth = 16
    splitByteWrap<NM, W>(o_strm, l_strm, split_byte_strm);
#ifndef __SYNTHESIS__
    std::cout << "splitByteWrap End!\n";
#endif
    internal::parseBlock<NM>(type_buf, type_num, type_valid_buf, split_byte_strm, o_obj_strm);
#ifndef __SYNTHESIS__
    std::cout << "parseBlock End!\n";
#endif
}

} // end namespace dataframe
} // end namespace data_analytics
} // end namespace xf

#endif
