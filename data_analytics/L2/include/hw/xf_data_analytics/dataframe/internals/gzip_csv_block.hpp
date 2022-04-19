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
#ifndef _GZIP_CSV_BLOCK_HPP_
#define _GZIP_CSV_BLOCK_HPP_

#include <ap_int.h>
#include <hls_stream.h>
#include <hls_math.h>
#include "csv_parse_block.hpp"
#include "filter.hpp"

#if !defined(__SYNTHESIS__)
#include <iostream>
#endif

namespace xf {
namespace data_analytics {
namespace dataframe {
namespace internals {

inline void crc32c(hls::stream<ap_uint<10> >& i_strm, hls::stream<ap_uint<32> >& o_strm) {
    const ap_uint<32> crc32c_table[256] = {
        0x0,        0xf26b8303, 0xe13b70f7, 0x1350f3f4, 0xc79a971f, 0x35f1141c, 0x26a1e7e8, 0xd4ca64eb, 0x8ad958cf,
        0x78b2dbcc, 0x6be22838, 0x9989ab3b, 0x4d43cfd0, 0xbf284cd3, 0xac78bf27, 0x5e133c24, 0x105ec76f, 0xe235446c,
        0xf165b798, 0x30e349b,  0xd7c45070, 0x25afd373, 0x36ff2087, 0xc494a384, 0x9a879fa0, 0x68ec1ca3, 0x7bbcef57,
        0x89d76c54, 0x5d1d08bf, 0xaf768bbc, 0xbc267848, 0x4e4dfb4b, 0x20bd8ede, 0xd2d60ddd, 0xc186fe29, 0x33ed7d2a,
        0xe72719c1, 0x154c9ac2, 0x61c6936,  0xf477ea35, 0xaa64d611, 0x580f5512, 0x4b5fa6e6, 0xb93425e5, 0x6dfe410e,
        0x9f95c20d, 0x8cc531f9, 0x7eaeb2fa, 0x30e349b1, 0xc288cab2, 0xd1d83946, 0x23b3ba45, 0xf779deae, 0x5125dad,
        0x1642ae59, 0xe4292d5a, 0xba3a117e, 0x4851927d, 0x5b016189, 0xa96ae28a, 0x7da08661, 0x8fcb0562, 0x9c9bf696,
        0x6ef07595, 0x417b1dbc, 0xb3109ebf, 0xa0406d4b, 0x522bee48, 0x86e18aa3, 0x748a09a0, 0x67dafa54, 0x95b17957,
        0xcba24573, 0x39c9c670, 0x2a993584, 0xd8f2b687, 0xc38d26c,  0xfe53516f, 0xed03a29b, 0x1f682198, 0x5125dad3,
        0xa34e59d0, 0xb01eaa24, 0x42752927, 0x96bf4dcc, 0x64d4cecf, 0x77843d3b, 0x85efbe38, 0xdbfc821c, 0x2997011f,
        0x3ac7f2eb, 0xc8ac71e8, 0x1c661503, 0xee0d9600, 0xfd5d65f4, 0xf36e6f7,  0x61c69362, 0x93ad1061, 0x80fde395,
        0x72966096, 0xa65c047d, 0x5437877e, 0x4767748a, 0xb50cf789, 0xeb1fcbad, 0x197448ae, 0xa24bb5a,  0xf84f3859,
        0x2c855cb2, 0xdeeedfb1, 0xcdbe2c45, 0x3fd5af46, 0x7198540d, 0x83f3d70e, 0x90a324fa, 0x62c8a7f9, 0xb602c312,
        0x44694011, 0x5739b3e5, 0xa55230e6, 0xfb410cc2, 0x92a8fc1,  0x1a7a7c35, 0xe811ff36, 0x3cdb9bdd, 0xceb018de,
        0xdde0eb2a, 0x2f8b6829, 0x82f63b78, 0x709db87b, 0x63cd4b8f, 0x91a6c88c, 0x456cac67, 0xb7072f64, 0xa457dc90,
        0x563c5f93, 0x82f63b7,  0xfa44e0b4, 0xe9141340, 0x1b7f9043, 0xcfb5f4a8, 0x3dde77ab, 0x2e8e845f, 0xdce5075c,
        0x92a8fc17, 0x60c37f14, 0x73938ce0, 0x81f80fe3, 0x55326b08, 0xa759e80b, 0xb4091bff, 0x466298fc, 0x1871a4d8,
        0xea1a27db, 0xf94ad42f, 0xb21572c,  0xdfeb33c7, 0x2d80b0c4, 0x3ed04330, 0xccbbc033, 0xa24bb5a6, 0x502036a5,
        0x4370c551, 0xb11b4652, 0x65d122b9, 0x97baa1ba, 0x84ea524e, 0x7681d14d, 0x2892ed69, 0xdaf96e6a, 0xc9a99d9e,
        0x3bc21e9d, 0xef087a76, 0x1d63f975, 0xe330a81,  0xfc588982, 0xb21572c9, 0x407ef1ca, 0x532e023e, 0xa145813d,
        0x758fe5d6, 0x87e466d5, 0x94b49521, 0x66df1622, 0x38cc2a06, 0xcaa7a905, 0xd9f75af1, 0x2b9cd9f2, 0xff56bd19,
        0xd3d3e1a,  0x1e6dcdee, 0xec064eed, 0xc38d26c4, 0x31e6a5c7, 0x22b65633, 0xd0ddd530, 0x417b1db,  0xf67c32d8,
        0xe52cc12c, 0x1747422f, 0x49547e0b, 0xbb3ffd08, 0xa86f0efc, 0x5a048dff, 0x8ecee914, 0x7ca56a17, 0x6ff599e3,
        0x9d9e1ae0, 0xd3d3e1ab, 0x21b862a8, 0x32e8915c, 0xc083125f, 0x144976b4, 0xe622f5b7, 0xf5720643, 0x7198540,
        0x590ab964, 0xab613a67, 0xb831c993, 0x4a5a4a90, 0x9e902e7b, 0x6cfbad78, 0x7fab5e8c, 0x8dc0dd8f, 0xe330a81a,
        0x115b2b19, 0x20bd8ed,  0xf0605bee, 0x24aa3f05, 0xd6c1bc06, 0xc5914ff2, 0x37faccf1, 0x69e9f0d5, 0x9b8273d6,
        0x88d28022, 0x7ab90321, 0xae7367ca, 0x5c18e4c9, 0x4f48173d, 0xbd23943e, 0xf36e6f75, 0x105ec76,  0x12551f82,
        0xe03e9c81, 0x34f4f86a, 0xc69f7b69, 0xd5cf889d, 0x27a40b9e, 0x79b737ba, 0x8bdcb4b9, 0x988c474d, 0x6ae7c44e,
        0xbe2da0a5, 0x4c4623a6, 0x5f16d052, 0xad7d5351};
#pragma HLS bind_storage variable = crc32c_table type = ram_1p impl = bram
    ap_uint<10> in = i_strm.read();
    bool e = in[9];
    ap_uint<32> crc = ~0;
loop_crc32c:
    while (!e) {
#pragma HLS PIPELINE II = 1
        ap_uint<8> in_data = in(7, 0);
        //#ifndef __SYNTHESIS__
        //        std::cout << "in_data=" << std::hex << in_data << std::endl;
        //#endif
        crc = (crc >> 8) ^ crc32c_table[crc(7, 0) ^ in_data];
        bool line_flag = in[8];
        if (line_flag) {
            //#ifndef __SYNTHESIS__
            //            std::cout << "crc=" << std::hex << ~crc << std::endl;
            //#endif
            o_strm.write(~crc);
            crc = ~0;
        }
        in = i_strm.read();
        e = in[9];
    }
}

// roundrobin file
template <int W, int BN>
void men2Strm(ap_uint<32>* begin_buff,
              ap_uint<32>* size_buff,
              ap_uint<W>* csvBuf,
              hls::stream<ap_uint<W> > csvStrm[BN]) {
    const int BS = 1024; // burst size
    int loop_nm = 0;
    // calculate most size as loop condition
    for (int i = 0; i < BN; ++i) {
#pragma HLS pipeline II = 1
        int tmp = (size_buff[i] + BS - 1) / BS;
        if (tmp > loop_nm) loop_nm = tmp;
    }

    // round-robin read file buffer to the corresponding stream
    for (int l = 0; l < loop_nm; ++l) {
        for (int i = 0; i < BN; ++i) {
            ap_uint<32> offset = size_buff[i];
            ap_uint<32> base_addr = begin_buff[i];
            ap_uint<16> byte_cnt = BS;
            if (byte_cnt > offset) byte_cnt = offset;
            size_buff[i] -= byte_cnt;
            begin_buff[i] += byte_cnt;
            for (int j = 0; j < byte_cnt; j++) {
#pragma HLS pipeline II = 1
                csvStrm[i].write(csvBuf[base_addr + j]);
            }
        }
    }
}

// split bitwidth to gzip decompression
template <int IW, int OW>
void streamSplit(int size,
                 hls::stream<ap_uint<IW> >& inHlsStream,
                 hls::stream<ap_uint<OW> >& outdownstream,
                 hls::stream<bool>& outEndstream) {
    int factor = IW / OW;
    ap_uint<IW> inBuffer = 0;
    for (int i = 0; i < (size * 8 + OW - 1) / OW; i++) {
#pragma HLS pipeline II = 1
        if (i % factor == 0) inBuffer = inHlsStream.read();
        ap_uint<OW> tmpValue = inBuffer;
        inBuffer >>= OW;
        outdownstream.write(tmpValue);
        outEndstream.write(0);
    }
    outdownstream.write(0);
    outEndstream.write(1);
}

template <int IW, int OW>
void gzipDecomp(int size, hls::stream<ap_uint<IW> >& inStrm, hls::stream<ap_uint<OW + OW / 8> >& outStrm) {
#pragma HLS dataflow
    hls::stream<ap_uint<16> > outdownstream("outdownStream");
#pragma HLS STREAM variable = outdownstream depth = 8
#pragma HLS bind_storage variable = outdownstream type = fifo impl = srl
    hls::stream<bool> outEndstream("outEndStream");
#pragma HLS STREAM variable = outEndstream depth = 8
#pragma HLS bind_storage variable = outEndstream type = fifo impl = srl
    streamSplit<IW, 16>(size, inStrm, outdownstream, outEndstream);
    xf::compression::details::inflateMultiByteCore<2, OW / 8, 0, false, 32768>(outdownstream, outEndstream, outStrm);
}

// if not gzip decompression
template <int IW, int OW>
void bypassUnGzip(int size, hls::stream<ap_uint<IW> >& inStrm, hls::stream<ap_uint<OW + OW / 8> >& outStrm) {
#pragma HLS inline off
    ap_uint<OW + OW / 8> temp = ap_uint<OW / 8>(-1);
    const int nm = IW / OW; // IW>OW
    ap_uint<IW> in_data;
    int end = size / (OW / 8);
    for (int i = 0; i < end; i++) {
#pragma HLS pipeline ii = 1
        if (i % nm == 0) in_data = inStrm.read();
        temp(OW + OW / 8 - 1, OW / 8) = in_data(OW - 1, 0);
        outStrm.write(temp);
        in_data = in_data(IW - 1, OW);
    }
    temp = 0;
    for (int i = 0; i < size - end * (OW / 8); i++) {
        temp[i] = 1;
        if (i == 0 && end % nm == 0) in_data = inStrm.read();
        if (i == size - end * (OW / 8) - 1) {
            temp(OW + OW / 8 - 1, OW / 8) = in_data(OW - 1, 0);
            outStrm.write(temp);
        }
    }
    outStrm.write(0);
}

template <int IW, int OW>
void switchGzipDecomp(bool gzipFlag,
                      int size,
                      hls::stream<ap_uint<IW> >& inStrm,
                      hls::stream<ap_uint<OW + OW / 8> >& outStrm) {
    // if (gzipFlag)
    gzipDecomp<IW, OW>(size, inStrm, outStrm);
    // else
    //    bypassUnGzip<IW, OW>(size, inStrm, outStrm);
}

typedef xf::data_analytics::dataframe::ObjectAlter1 OBJ;
typedef xf::data_analytics::dataframe::ObjectFile OBJF;

// add file id into obj
inline void objConvert(int id, hls::stream<OBJ>& i_obj_strm, hls::stream<OBJF>& o_objf_strm) {
    OBJ obj = i_obj_strm.read();
    OBJF objf;
    bool suc_flag = true;
    while (obj.get_type() != 15) {
        if (suc_flag) {
            if (obj.get_type() < 14) {
                objf.set_obj(obj.get_all());
                objf.set_file(id);
                o_objf_strm.write(objf);
            }
        }
        suc_flag = i_obj_strm.read_nb(obj);
    }
    objf.set_obj(obj.get_all());
    objf.set_file(id);
    o_objf_strm.write(objf);
}

template <int ON>
void mergeOBJF(hls::stream<OBJF> i_objf_strm[ON], hls::stream<OBJF>& o_objf_strm) {
    ap_uint<ON> e = 0;
    ap_uint<4> pu_idx = 0;
    OBJF objf;
    ap_uint<1> line_flag = 0;
    ap_uint<4> fld_cnt = 0;
loop_mergeOBJF:
    while (e != (ap_uint<ON>)-1) {
#pragma HLS pipeline ii = 1
        pu_idx = pu_idx % ON;
        if (!e[pu_idx]) {
            bool suc_flag = i_objf_strm[pu_idx].read_nb(objf);
            // objf = i_objf_strm[pu_idx].read();
            if (suc_flag) {
                if (objf.get_type() == 15) {
                    e[pu_idx] = 1;
                } else {
                    o_objf_strm.write(objf);
                }
                if (objf.get_type() == 13) line_flag = 1;
                fld_cnt++;
            }
        }
        if (e[pu_idx] || line_flag || (fld_cnt == 0)) {
            pu_idx++;
            line_flag = 0;
            fld_cnt = 0;
        }
    }
    o_objf_strm.write(objf);
}

// get crc32 field
template <int ON>
void split2Crc32c(ap_uint<4> id,
                  ap_uint<32> crc_check_field,
                  hls::stream<OBJF>& i_objf_strm,
                  hls::stream<OBJF>& o_objf_strm,
                  // 63~0: data, 67~64: len, 68: line end flag, 69: end flag
                  hls::stream<ap_uint<70> > o_crc_strm[ON]) {
    OBJF objf = i_objf_strm.read();
    ap_uint<64> tmp = objf.get_data();
    ap_uint<64> tmp2;
    // csv parse date structure - > output date structure
    if (objf.get_type() == 4) {
        tmp2(31, 0) = tmp(63, 32);  // year
        tmp2(47, 32) = tmp(31, 16); // month
        tmp2(63, 48) = tmp(15, 0);  // day
        objf.set_data(tmp2);
    }
    o_objf_strm.write(objf);
    ap_uint<4> bgn = 0;
    ap_uint<128> buf;
    ap_uint<70> out_data;
    ap_uint<4> fld_id = 0;
    ap_uint<1> fld_flag_last = 0;
loop_split2Crc32c:
    while (objf.get_type() != 15) {
#pragma HLS pipeline ii = 1
        ap_uint<4> fid = objf.get_file() - id;
        if (objf.get_type() < 13) {
            if (fld_flag_last != objf.get_id()) {
                fld_id++;
            }
            // std::cout << std::hex << "fld_id=" << fld_id << ", crc_check_field=" << crc_check_field << std::endl;
            if (crc_check_field[fld_id]) {
                ap_uint<4> sz = objf.get_valid();
                ap_uint<5> end = bgn + sz;
                ap_uint<64> data = objf.get_data();
                // std::cout << "bgn=" << bgn << ", end=" << end << ", objf.get_id()=" << objf.get_id() << std::endl;
                buf(end * 8 - 1, bgn * 8) = data(sz * 8 - 1, 0);
                if (end > 8) {
                    out_data(63, 0) = buf(63, 0);
                    buf(63, 0) = buf(127, 64);
                    out_data(67, 64) = 8;
                    out_data[68] = 0;
                    out_data[69] = 0;
                    o_crc_strm[fid % ON].write(out_data);
                    bgn = end - 8;
                } else {
                    bgn = end;
                }
            }
            fld_flag_last = objf.get_id();
        } else {
            out_data(63, 0) = buf(63, 0);
            out_data(67, 64) = bgn;
            out_data[68] = 1;
            out_data[69] = 0;
            if (bgn > 0) o_crc_strm[fid % ON].write(out_data);
            bgn = 0;
            fld_id = 0;
            fld_flag_last = 0;
        }
        // std::cout << "bgn=" << bgn << std::endl;
        // std::cout << "chan id=" << objf.get_file() << std::endl;
        objf = i_objf_strm.read();
        tmp = objf.get_data();
        if (objf.get_type() == 4) {
            tmp2(31, 0) = tmp(63, 32);
            tmp2(47, 32) = tmp(31, 16);
            tmp2(63, 48) = tmp(15, 0);
            objf.set_data(tmp2);
        }
        o_objf_strm.write(objf);
    }
    for (int i = 0; i < ON; i++) {
        out_data[69] = 1;
        o_crc_strm[i].write(out_data);
    }
}

inline void preCrc32c(hls::stream<ap_uint<70> >& i_strm, hls::stream<ap_uint<10> >& o_strm) {
    ap_uint<70> in = i_strm.read();
    bool e = in[69];
    bool le = in[68];
    ap_uint<4> len = in(67, 64);
    ap_uint<4> cnt = 0;
    ap_uint<10> out;
loop_preCrc32c:
    while (!e) {
        out(7, 0) = in(cnt * 8 + 7, cnt * 8);
        if (len == cnt + 1 && le)
            out[8] = 1;
        else
            out[8] = 0;
        out[9] = 0;
        o_strm.write(out);
        if (len == cnt + 1) {
            in = i_strm.read();
            e = in[69];
            le = in[68];
            len = in(67, 64);
            cnt = 0;
        } else {
            cnt += 1;
        }
    }
    out[9] = 1;
    o_strm.write(out);
}

template <int ON>
void compactLine(ap_uint<4> id,
                 hls::stream<OBJF>& i_objf_strm,
                 // 63~0: data, 67~64: len, 68: line end flag, 69: end flag
                 hls::stream<ap_uint<70> > o_crc_strm[ON]) {
    OBJF objf = i_objf_strm.read();
    ap_uint<4> bgn = 0;
    ap_uint<128> buf;
    ap_uint<70> out_data;
loop_split2Crc32c:
    while (objf.get_type() != 15) {
#pragma HLS pipeline ii = 1
        ap_uint<4> fid = objf.get_file() - id;
        if (objf.get_type() < 13) {
            // std::cout << std::hex << "fld_id=" << fld_id << ", crc_check_field=" << crc_check_field << std::endl;
            ap_uint<4> sz = objf.get_valid();
            ap_uint<5> end = bgn + sz;
            ap_uint<64> data = objf.get_data();
            // std::cout << "bgn=" << bgn << ", end=" << end << ", objf.get_id()=" << objf.get_id() << std::endl;
            buf(end * 8 - 1, bgn * 8) = data(sz * 8 - 1, 0);
            if (end > 8) {
                out_data(63, 0) = buf(63, 0);
                buf(63, 0) = buf(127, 64);
                out_data(67, 64) = 8;
                out_data[68] = 0;
                out_data[69] = 0;
                o_crc_strm[fid % ON].write(out_data);
                bgn = end - 8;
            } else {
                bgn = end;
            }
        } else {
            out_data(63, 0) = buf(63, 0);
            out_data(67, 64) = bgn;
            out_data[68] = 1;
            out_data[69] = 0;
            o_crc_strm[fid % ON].write(out_data);
            bgn = 0;
        }
        // std::cout << "bgn=" << bgn << std::endl;
        // std::cout << "chan id=" << objf.get_file() << std::endl;
        objf = i_objf_strm.read();
    }
    for (int i = 0; i < ON; i++) {
        out_data[69] = 1;
        o_crc_strm[i].write(out_data);
    }
}

// merge crc32 value to strm
inline void compactCrc32(ap_uint<32> crc_flag,
                         hls::stream<ap_uint<70> >& i_strm,
                         hls::stream<ap_uint<32> >& i_crc_strm,
                         hls::stream<ap_uint<64> >& o_strm,
                         hls::stream<bool>& o_e_strm) {
    ap_uint<4> bgn = 0;
    ap_uint<32> total_len = 0;
    ap_uint<128> buf;
    ap_uint<70> in = i_strm.read();
loop_compactCrc32:
    while (!in[69]) {
#pragma HLS pipeline ii = 1
        bool le = in[68];
        ap_uint<4> sz = in(67, 64);
        total_len += sz;
        ap_uint<4> end = bgn + sz;
        ap_uint<64> data = in(63, 0);
        buf(end * 8 - 1, bgn * 8) = data(sz * 8 - 1, 0);
        if (end >= 8) {
            o_strm.write(buf(63, 0));
            o_e_strm.write(false);
            buf(63, 0) = buf(127, 64);
            bgn = end - 8;
        } else {
            bgn = end;
        }
        if (in[68] && (crc_flag != 0)) { // line end flag, add crc32
            in[68] = 0;
            in(67, 64) = 4;
            in(31, 0) = i_crc_strm.read();
        } else {
            in = i_strm.read();
        }
    }
    if (bgn > 0) {
        o_strm.write(buf(63, 0));
        o_e_strm.write(false);
    }
    o_strm.write(total_len);
    o_e_strm.write(true);
}

// convert IW -> OW
template <int IW, int OW, int BL = 256>
void compactUnit2(hls::stream<ap_uint<IW> >& i_strm,
                  hls::stream<bool>& i_e_strm,
                  hls::stream<ap_uint<OW> >& o_strm,
                  hls::stream<ap_uint<10> >& o_cnt_strm) {
    int N = OW / IW; // IW<=OW
    int cnt = 0;
    int bl_cnt = 0;
    ap_uint<OW> buf = 0;
    while (!i_e_strm.read()) {
        buf((cnt % N) * IW + IW - 1, (cnt % N) * IW) = i_strm.read();
        cnt++;
        if (cnt % N == 0) {
            o_strm.write(buf);
            // std::cout << "compactUnit2: buf=" << std::hex << buf << std::endl;
            buf = 0;
            if (bl_cnt == BL - 1) {
                o_cnt_strm.write(BL);
                bl_cnt = 0;
            } else {
                bl_cnt++;
            }
        }
    }
    if (cnt % N) {
        o_strm.write(buf);
        bl_cnt++;
    }
    // std::cout << "compactUnit2: cnt=" << cnt << ", o_strm size=" << o_strm.size()
    //          << ", o_cnt_strm size=" << o_cnt_strm.size() << std::endl;
    if (bl_cnt > 0) o_cnt_strm.write(bl_cnt);
    o_strm.write(i_strm.read());
    o_cnt_strm.write(0); // for end
}

// round-robin ouput with empty judgment
template <int BN, int W>
void writeOut2Mem(hls::stream<ap_uint<W> > i_strm[BN],
                  hls::stream<ap_uint<10> > i_cnt_strm[BN],
                  ap_uint<32> wr_addr[BN],
                  ap_uint<W>* fir_value) {
    ap_uint<BN> end = 0;
    ap_uint<10> out_cnt;

    ap_uint<4> ch = 0;
    bool success_flag = 0;
loop_writeOut2Mem:
    while (end != ap_uint<BN>(-1)) {
#pragma HLS loop_tripcount max = 1000 min = 1000
        success_flag = i_cnt_strm[ch].read_nb(out_cnt);
        if (success_flag) {
            // std::cout << "writeOut2Mem: out_cnt=" << out_cnt << std::endl;
            if (out_cnt == 0) {
                end[ch] = 1;
            } else {
                ap_uint<32> begin = wr_addr[ch];
            loop_writeOut2Mem2:
                for (int i = 0; i < out_cnt; i++) {
#pragma HLS pipeline ii = 1
#pragma HLS loop_tripcount max = 1000 min = 1000
                    fir_value[begin + i] = i_strm[ch].read();
                }
                wr_addr[ch] = begin + out_cnt;
            }
        }
        ch = (ch + 1) % BN;
    }
    for (int i = 0; i < BN; i++) {
        wr_addr[i] = i_strm[i].read();
        // std::cout << "wr_addr[" << i << "]=" << wr_addr[i] << std::endl;
    }
}

// compact output field
template <int BN, int FN, int OW>
void compactOutput(ap_uint<32> crc_check_field,
                   hls::stream<OBJF> i_obj_strm[BN],
                   ap_uint<32> wr_addr[BN],
                   ap_uint<OW>* fir_value) {
#pragma HLS dataflow
    // std::cout << "crc_check_field=" << crc_check_field << std::endl;
    const int obj_cnt = (BN + FN - 1) / FN;
    hls::stream<ap_uint<64> > comp_strm[BN];
#pragma HLS stream variable = comp_strm depth = 32
    hls::stream<bool> comp_e_strm[BN];
#pragma HLS stream variable = comp_e_strm depth = 32
    hls::stream<OBJF> obj_strm[BN];
#pragma HLS stream variable = obj_strm depth = 32
    hls::stream<ap_uint<70> > crc_field_strm[BN];
#pragma HLS stream variable = crc_field_strm depth = 32
    hls::stream<ap_uint<70> > comp_line_strm[BN];
#pragma HLS stream variable = comp_line_strm depth = 32
    hls::stream<ap_uint<10> > crc_data_strm[BN];
#pragma HLS stream variable = crc_data_strm depth = 32
    for (int i = 0; i < FN - 1; i++) {
#pragma HLS unroll
        split2Crc32c<obj_cnt>(i * obj_cnt, crc_check_field, i_obj_strm[i], obj_strm[i], &crc_field_strm[i * obj_cnt]);
    }
    split2Crc32c<BN - (FN - 1) * obj_cnt>((FN - 1) * obj_cnt, crc_check_field, i_obj_strm[FN - 1], obj_strm[FN - 1],
                                          &crc_field_strm[(FN - 1) * obj_cnt]);
    for (int i = 0; i < FN - 1; i++) {
#pragma HLS unroll
        compactLine<obj_cnt>(i * obj_cnt, obj_strm[i], &comp_line_strm[i * obj_cnt]);
    }
    compactLine<BN - (FN - 1) * obj_cnt>((FN - 1) * obj_cnt, obj_strm[FN - 1], &comp_line_strm[(FN - 1) * obj_cnt]);

    hls::stream<ap_uint<32> > crc_strm[BN];
#pragma HLS stream variable = crc_strm depth = 2
    for (int i = 0; i < BN; i++) {
#pragma HLS unroll
        preCrc32c(crc_field_strm[i], crc_data_strm[i]);
        internals::crc32c(crc_data_strm[i], crc_strm[i]);
    }

    // std::cout << "crc_field_strm size=" << crc_field_strm[0].size() << std::endl;
    hls::stream<ap_uint<OW> > comp2_strm[BN];
    const int OBL = 512;
#pragma HLS stream variable = comp2_strm depth = OBL * 2
#pragma HLS bind_storage variable = comp2_strm type = fifo impl = bram
    hls::stream<ap_uint<10> > comp2_cnt_strm[BN];
#pragma HLS stream variable = comp2_cnt_strm depth = 2
    for (int i = 0; i < BN; i++) {
#pragma HLS unroll
        compactCrc32(crc_check_field, comp_line_strm[i], crc_strm[i], comp_strm[i], comp_e_strm[i]);
        compactUnit2<64, OW, OBL>(comp_strm[i], comp_e_strm[i], comp2_strm[i], comp2_cnt_strm[i]);
    }
    writeOut2Mem<BN, OW>(comp2_strm, comp2_cnt_strm, wr_addr, fir_value);
}

// split column as condition column and output column
template <int FW>
inline void splitFilter(ap_uint<32> cond_field_flag,
                        ap_uint<32> out_field_flag,
                        hls::stream<OBJF>& i_objf_strm,
                        hls::stream<ap_uint<64> >& o_cond_strm,
                        hls::stream<bool>& o_e_cond_strm,
                        hls::stream<OBJF>& o_objf_strm) {
    OBJF objf = i_objf_strm.read();
    ap_uint<4> fld_id = 0;
    ap_uint<4> last_fld_id = 0;
    ap_uint<1> fld_flag = 0;
    ap_uint<1> first_flag = 0;
    ap_uint<1> fld_out_flag = 0;
loop_SplitFilter:
    while (objf.get_type() != 15) {
#pragma HLS pipeline ii = 1
        if (objf.get_type() < 13) {
            if (fld_flag != objf.get_id()) {
                fld_id++;
            }
            if (cond_field_flag[fld_id]) {
                o_cond_strm.write(objf.get_data());
                o_e_cond_strm.write(false);
            }
            if (out_field_flag[fld_id]) {
                OBJF objf_out = objf;
                if (first_flag && (last_fld_id != fld_id)) fld_out_flag = !fld_out_flag;
                objf_out.set_id(fld_out_flag);
                o_objf_strm.write(objf_out);
                last_fld_id = fld_id;
                first_flag = 1;
            }
            // std::cout << "fld_id=" << fld_id << ", cond_field_flag=" << cond_field_flag[fld_id]
            //          << ", out_field_flag=" << out_field_flag[fld_id] << std::endl;
            fld_flag = objf.get_id();
        } else {
            OBJF objf_out = objf;
            objf_out.set_id(!fld_out_flag);
            o_objf_strm.write(objf_out);
            fld_id = 0;
            fld_flag = 0;
            first_flag = 0;
            fld_out_flag = 0;
        }
        objf = i_objf_strm.read();
    }
    o_objf_strm.write(objf);
    o_e_cond_strm.write(true);
}

// filter row based on filter conditon
inline void filterRow(ap_uint<32> cond_field_flag,
                      hls::stream<bool>& filter_flag_strm,
                      hls::stream<OBJF>& i_objf_strm,
                      hls::stream<OBJF>& o_objf_strm) {
    OBJF objf = i_objf_strm.read();
    bool flag = filter_flag_strm.read();
loop_filterRow:
    while (objf.get_type() != 15) {
#pragma HLS pipeline ii = 1
        if (flag) o_objf_strm.write(objf);
        if (cond_field_flag != 0 && objf.get_type() == 13) {
            flag = filter_flag_strm.read();
        }
        objf = i_objf_strm.read();
    }
    o_objf_strm.write(objf);
}

// calculate length of string type
inline void calcuStrLen(hls::stream<OBJF>& i_objf_strm, hls::stream<OBJF>& o_objf_strm, hls::stream<int>& o_len_strm) {
    OBJF objf = i_objf_strm.read();
    int len = 0;
    bool last_flag = 0;
    ap_uint<4> last_type = 15;
loop_calcuStrLen:
    while (objf.get_type() != 15) {
#pragma HLS pipeline ii = 1
        // std::cout << "objf.get_id()=" << objf.get_id() << ", objf.get_type()=" << objf.get_type()
        //          << ", objf.get_valid()=" << objf.get_valid() << ", o_len_strm size=" << o_len_strm.size()
        //          << std::endl;
        o_objf_strm.write(objf);
        if (objf.get_type() == 5) {
            len += objf.get_valid();
        }
        last_type = objf.get_type();
        last_flag = objf.get_id();
        objf = i_objf_strm.read();
        if (objf.get_id() != last_flag) {
            if (last_type == 5) {
                // std::cout << "len=" << len << std::endl;
                o_len_strm.write(len);
            }
            len = 0;
        }
        if (objf.get_type() == 13) len = 0;
    }
    o_objf_strm.write(objf);
    // std::cout << "calcuStrLen: o_objf_strm size=" << o_objf_strm.size() << std::endl;
    // std::cout << "o_len_strm size=" << o_len_strm.size() << std::endl;
}

// insert length of string into data stream
inline void insertStrLen(hls::stream<OBJF>& i_objf_strm, hls::stream<int>& i_len_strm, hls::stream<OBJF>& o_objf_strm) {
    OBJF objf = i_objf_strm.read();
    bool last_flag = 1;
loop_insertStrLen:
    while (objf.get_type() != 15) {
#pragma HLS pipeline ii = 1
        if (objf.get_id() != last_flag && objf.get_type() == 5) {
            OBJF objf_t = objf;
            objf_t.set_data(i_len_strm.read());
            objf_t.set_valid(4);
            o_objf_strm.write(objf_t);
            // std::cout << "objf_t=" << objf_t.get_data() << std::endl;
            last_flag = objf.get_id();
        } else {
            o_objf_strm.write(objf);
            // std::cout << "objf_t=" << objf.get_data() << std::endl;
            last_flag = objf.get_id();
            objf = i_objf_strm.read();
        }
    }
    o_objf_strm.write(objf);
    // std::cout << "insertStrLen: o_objf_strm size=" << o_objf_strm.size() << std::endl;
}

// filter row based on filter conditon and select output columns
template <int BN, int FN, int FW>
void dynamicFilterWrap(ap_uint<32> cond_field_flag,
                       ap_uint<32> out_field_flag,
                       hls::stream<ap_uint<64> >& firCfgStrm,
                       hls::stream<OBJF> i_objf_strm[BN],
                       hls::stream<OBJF> o_objf_strm[FN]) {
#pragma HLS dataflow
    const int obj_cnt = (BN + FN - 1) / FN;
    hls::stream<ap_uint<64> > cond_strm[FN];
#pragma HLS stream variable = cond_strm depth = 32
#pragma HLS bind_storage variable = cond_strm type = fifo impl = srl
    hls::stream<bool> e_cond_strm[FN];
#pragma HLS stream variable = e_cond_strm depth = 32
#pragma HLS bind_storage variable = e_cond_strm type = fifo impl = srl
    hls::stream<OBJF> merge_objf_strm[FN];
#pragma HLS stream variable = merge_objf_strm depth = 32
#pragma HLS bind_storage variable = merge_objf_strm type = fifo impl = srl
    hls::stream<OBJF> split_objf_strm[FN];
#pragma HLS stream variable = split_objf_strm depth = 32
#pragma HLS bind_storage variable = split_objf_strm type = fifo impl = srl
    hls::stream<OBJF> str_objf_strm[FN];
#pragma HLS stream variable = str_objf_strm depth = 512
#pragma HLS bind_storage variable = str_objf_strm type = fifo impl = bram
    hls::stream<OBJF> insert_objf_strm[FN];
#pragma HLS stream variable = insert_objf_strm depth = 32
#pragma HLS bind_storage variable = insert_objf_strm type = fifo impl = srl
    hls::stream<int> str_len_strm[FN];
#pragma HLS stream variable = str_len_strm depth = 32
#pragma HLS bind_storage variable = str_len_strm type = fifo impl = srl
    ap_uint<128 + 9> cmpvc_cfg[FN][4];
#pragma HLS array_partition variable = cmpvc_cfg dim = 1 complete
    parse_filter_config<FN, 64>(firCfgStrm, cmpvc_cfg);
    hls::stream<bool> filter_flag_strm[FN];
#pragma HLS stream variable = filter_flag_strm depth = 32
#pragma HLS bind_storage variable = filter_flag_strm type = fifo impl = srl
    for (int i = 0; i < FN - 1; i++) {
#pragma HLS unroll
        mergeOBJF<obj_cnt>(&i_objf_strm[i * obj_cnt], merge_objf_strm[i]);
    }
    mergeOBJF<BN - obj_cnt*(FN - 1)>(&i_objf_strm[(FN - 1) * obj_cnt], merge_objf_strm[FN - 1]);
    for (int i = 0; i < FN; i++) {
#pragma HLS unroll
        splitFilter<FW>(cond_field_flag, out_field_flag, merge_objf_strm[i], cond_strm[i], e_cond_strm[i],
                        split_objf_strm[i]);
        calcuFilter<64>(cond_strm[i], e_cond_strm[i], cmpvc_cfg[i], filter_flag_strm[i]);
        calcuStrLen(split_objf_strm[i], str_objf_strm[i], str_len_strm[i]);
        insertStrLen(str_objf_strm[i], str_len_strm[i], insert_objf_strm[i]);
        filterRow(cond_field_flag, filter_flag_strm[i], insert_objf_strm[i], o_objf_strm[i]);
    }
}

template <int IW, int OW, int BN, int NM, int FN, int FW>
void gzipCsvBlockWrap(bool gzipFlag,
                      ap_uint<32>* size_buf,
                      ap_uint<8> type_buf[BN][NM][16],
                      ap_uint<4> type_valid_buf[BN][16],
                      ap_uint<3> type_num[BN][7],
                      ap_uint<9>& num_of_column,
                      ap_uint<32> cond_field_flag,
                      ap_uint<32> out_field_flag,
                      hls::stream<ap_uint<64> >& firCfgStrm,
                      hls::stream<ap_uint<IW> > i_strm[BN],
                      hls::stream<OBJF> o_objf_strm[BN]) {
#pragma HLS dataflow
    hls::stream<ap_uint<OW + OW / 8> > inflateStrm[BN];
#pragma HLS stream variable = inflateStrm depth = 8
#pragma HLS bind_storage variable = inflateStrm type = fifo impl = srl
    hls::stream<OBJ> obj_strm[BN];
#pragma HLS stream variable = obj_strm depth = 8
    hls::stream<OBJF> objf_strm[BN];
#pragma HLS stream variable = objf_strm depth = 32
    for (int i = 0; i < BN; i++) {
#pragma HLS unroll
        switchGzipDecomp<IW, OW>(gzipFlag, size_buf[i], i_strm[i], inflateStrm[i]);
        csv_parser<NM>(inflateStrm[i], type_buf[i], type_valid_buf[i], type_num[i], num_of_column, obj_strm[i]);
        objConvert(i, obj_strm[i], objf_strm[i]);
    }
    dynamicFilterWrap<BN, FN, FW>(cond_field_flag, out_field_flag, firCfgStrm, objf_strm, o_objf_strm);
}

/**
 * @brief computation core
 *
 * @param gzipFlag gzip or not
 * @param begin_buf start position of each file in csvBuf
 * @param size_buf file size in FW/8 bytes
 * @param size_buf2 file size in bytes
 * @param cond_field_flag indicate which column is needed to be filtered
 * @param out_field_flag which column to be output
 * @param crc_check_field which column to perform CRC
 * @param csvBuf input CSV files
 * @param type_buf data type of each column for CSV parser
 * @param type_valid_buf drop or pass on after CSV parser
 * @param type_num number of columns of each data type
 * @param num_of_column number of columns (post filtration)
 * @param firCfgStrm filter configurations
 * @param wrAddr start position of each file in result buffer (firValue)
 * @param firValue result buffer
 *
 */
template <int BN, int NM, int FW, int FN, int OUTW>
inline void coreWrapp(bool gzipFlag,
                      ap_uint<32>* begin_buf,
                      ap_uint<32>* size_buf,
                      ap_uint<32>* size_buf2,
                      ap_uint<32> cond_field_flag,
                      ap_uint<32> out_field_flag,
                      ap_uint<32> crc_check_field,
                      ap_uint<FW>* csvBuf,
                      ap_uint<8> type_buf[BN][NM][16],
                      ap_uint<4> type_valid_buf[BN][16],
                      ap_uint<3> type_num[BN][7],
                      ap_uint<9>& num_of_column,
                      hls::stream<ap_uint<64> >& firCfgStrm,
                      ap_uint<32>* wrAddr,
                      ap_uint<OUTW>* firValue) {
#pragma HLS dataflow
    hls::stream<ap_uint<FW> > csvStrm[BN];
#pragma HLS STREAM variable = csvStrm depth = 4096
#pragma HLS bind_storage variable = csvStrm type = fifo impl = uram
    men2Strm<FW, BN>(begin_buf, size_buf, csvBuf, csvStrm);
#ifndef __SYNTHESIS__
    std::cout << "gzipCsvBlockWrap...\n";
#endif
    hls::stream<OBJF> obj_strm[BN];
    gzipCsvBlockWrap<FW, 64, BN, NM, FN, 32>(gzipFlag, size_buf2, type_buf, type_valid_buf, type_num, num_of_column,
                                             cond_field_flag, out_field_flag, firCfgStrm, csvStrm, obj_strm);

    compactOutput<BN, FN, OUTW>(crc_check_field, obj_strm, wrAddr, firValue);
}

/**
 * @brief read file size and config
 *
 * @param sz_buf input size buffer
 * @param cfg_buf input configurations
 * @param size_buf file size in FW/8 bytes
 * @param size_buf2 file size in bytes
 * @param begin_buf start position of each file in csvBuf
 * @param type_buf data type of each column for CSV parser
 * @param type_valid_buf drop or pass on after CSV parser
 * @param type_num number of columns of each data type
 * @param num_of_column number of columns (post filtration)
 * @param cond_field_flag indicate which column is needed to be filtered
 * @param out_field_flag which column to be output
 * @param crc_check_field which column to perform CRC
 * @param fir_cfg_strm filter configurations
 */
template <int BN, int NM, int FW>
inline void read_cfg(ap_uint<64>* sz_buf,
                     ap_uint<64>* cfg_buf,
                     ap_uint<32>* begin_buf,
                     ap_uint<32>* size_buf,
                     ap_uint<32>* size_buf2,
                     ap_uint<32>* begin_addr,
                     ap_uint<8> type_buf[BN][NM][16],
                     ap_uint<4> type_valid_buf[BN][16],
                     ap_uint<3> type_num[BN][7],
                     ap_uint<9>& num_of_column,
                     ap_uint<32>& cond_field_flag,
                     ap_uint<32>& out_field_flag,
                     ap_uint<32>& crc_check_field,
                     hls::stream<ap_uint<64> >& fir_cfg_strm) {
    for (ap_uint<5> i = 0; i < BN * 2; i++) {
        ap_uint<64> sz_value = sz_buf[i + 1];
        if (i < BN) {
            // file begin address
            begin_buf[i] = sz_value(31, 0);
            ap_uint<32> size = sz_value(63, 32);
            // file size
            size_buf[i] = (size + FW / 8 - 1) / (FW / 8); // for memory read
            size_buf2[i] = size;                          // for split bitwidth before gzip
        } else {
            // output begin address
            begin_addr[i - BN] = sz_value(31, 0);
        }
    }

    // csv column enable flag, up to 16 columns
    ap_uint<16> en_flag = cfg_buf[1](31, 16);
    for (ap_uint<8> i = 0; i < cfg_buf[1](7, 0); i++) {
        for (ap_uint<3> j = 0; j < NM; j++) {
            for (ap_uint<4> k = 0; k < BN; k++) {
                // csv column enable flag + data type
                type_buf[k][j][i] = ((ap_uint<3>)0, en_flag[i], cfg_buf[2](i * 4 + 3, i * 4));
                if (en_flag[i]) {
                    // select data type of enable column
                    type_valid_buf[k][num_of_column] = cfg_buf[2](i * 4 + 3, i * 4);
                }
            }
        }
        // calculate number of enable column
        if (en_flag[i]) num_of_column++;
    }
    // number of data type of enable column
    for (ap_uint<8> i = 0; i < 7; i++) {
        for (ap_uint<4> k = 0; k < BN; k++) {
            type_num[k][i] = cfg_buf[3](i * 4 + 2, i * 4);
        }
    }
    // end flag for csv parse
    for (ap_uint<4> k = 0; k < BN; k++) {
        type_valid_buf[k][num_of_column] = 13;
    }

    // configure of dynamic filter
    for (ap_uint<4> i = 0; i < 13; ++i) {
#pragma HLS pipeline II = 1
        fir_cfg_strm.write(cfg_buf[i + 4]);
    }
    // condition field of dynamic filter
    cond_field_flag = cfg_buf[17];
    // output field of dynamic filter
    out_field_flag = cfg_buf[18];
    // crc check field
    crc_check_field = cfg_buf[19];
}

} // end namespace internals

} // end namespace dataframe
} // end namespace data_analytics
} // end namespace xf

#endif //_GZIP_CSV_BLOCK_HPP_
