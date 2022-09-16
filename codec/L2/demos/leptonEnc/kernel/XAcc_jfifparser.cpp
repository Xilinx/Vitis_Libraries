/*
 * Copyright 2021 Xilinx, Inc.
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
 * @file XAcc_jfifparser.cpp
 * @brief parser_jpg_top template function implementation and kernel_decoder warpper.
 *
 * This file is part of HLS algorithm library.
 */

#include "XAcc_lepjfifparser.hpp"

// ------------------------------------------------------------
#define B_SHORT(v1, v2) ((((int)v1) << 8) + ((int)v2))

namespace xf {
namespace codec {
namespace details {

inline void readBytes(int& j, const int& cnt, int& r, int& c) {
#pragma HLS INLINE
    j += cnt;
    r = j >> 1;
    c = j & 1;
}
inline void oneByte(int& j, int& r, int& c) {
#pragma HLS INLINE
    j += 1;
    r = j >> 1;
    c = j & 1;
}

// ------------------------------------------------------------
void SetOtherQtab(decOutput* plep) {
    // clang-format off
	ap_uint<16> freqmax_[3][64];
	static const unsigned short int freqmax[] =
		        {
		            1024, 931, 985, 968, 1020, 968, 1020, 1020,
		            932, 858, 884, 840, 932, 838, 854, 854,
		            985, 884, 871, 875, 985, 878, 871, 854,
		            967, 841, 876, 844, 967, 886, 870, 837,
		            1020, 932, 985, 967, 1020, 969, 1020, 1020,
		            969, 838, 878, 886, 969, 838, 969, 838,
		            1020, 854, 871, 870, 1010, 969, 1020, 1020,
		            1020, 854, 854, 838, 1020, 838, 1020, 838
		        };
// clang-format on
#pragma HLS ARRAY_PARTITION variable = plep->q_tables complete dim = 2
#pragma HLS ARRAY_PARTITION variable = plep->idct_q_table_x complete dim = 3
#pragma HLS ARRAY_PARTITION variable = plep->idct_q_table_y complete dim = 3
    unsigned short RESIDUAL_NOISE_FLOOR = 7;
    for (int idx_cmp = 0; idx_cmp < plep->axi_num_cmp_mcu; idx_cmp++) {
        uint8_t c = plep->axi_map_row2cmp[idx_cmp];

        for (int i = 0; i < 64; i++) {
#pragma HLS pipeline
            plep->idct_q_table_x[c][i >> 3][i & 7] =
                hls_icos_base_8192_scaled[(i & 7) << 3] * plep->q_tables[c][i & 7][i >> 3];
            plep->idct_q_table_y[c][i >> 3][i & 7] =
                hls_icos_base_8192_scaled[(i & 7) << 3] * plep->q_tables[c][i >> 3][i & 7];
            // plep->idct_q_table_l[c][i>>3][i&7] = hls_icos_idct_linear_8192_scaled[i]   * plep->q_tables[c][0][i&7];
            //}

            // for (int coord = 0; coord < 64; ++coord) {
            freqmax_[c][i] = (freqmax[i] + plep->q_tables[c][i >> 3][i & 7] - 1) / plep->q_tables[c][i >> 3][i & 7];
            // uint8_t max_len = uint16bit_length(freqmax_[c][i]);
            uint8_t max_len = 16 - freqmax_[c][i].countLeadingZeros();
            // bitlen_freqmax_[c][i] = max_len;
            if (max_len > (int)RESIDUAL_NOISE_FLOOR) {
                plep->min_nois_thld_x[c][i] = plep->min_nois_thld_y[c][i] = max_len - RESIDUAL_NOISE_FLOOR;
            } else {
                plep->min_nois_thld_x[c][i] = plep->min_nois_thld_y[c][i] = 0;
            }

        } // end for
    }
}

// ------------------------------------------------------------
void decoder_jpg_top(ap_uint<AXI_WIDTH>* ptr,
                     const int sz,
                     const int c,
                     const uint16_t dht_tbl1[2][2][1 << DHT1],
                     const uint16_t dht_tbl2[2][2][1 << DHT2],
                     ap_uint<12> hls_cmp,

                     // image info
                     const uint8_t hls_mbs[MAX_NUM_COLOR],
                     const img_info img_info,

                     uint32_t& rst_cnt,
                     hls::stream<ap_uint<24> >& block_strm) {
#pragma HLS DATAFLOW
    // clang-format off
    _XF_IMAGE_PRINT(" ************* start decode %d mcus in FPGA  *************\n", (int)img_info.hls_mcuc);
    _XF_IMAGE_PRINT(
    				"  hls_cs_cmpc=%d, hls_mbs[0]=%d, hls_mbs[1]=%d, hls_mbs[2]=%d, \n",
						img_info.hls_cs_cmpc, hls_mbs[0], hls_mbs[1], hls_mbs[2]);


#pragma HLS ARRAY_PARTITION variable = hls_mbs  complete
//#pragma HLS bind_storage        variable = dht_tbl1 type=RAM_2P impl=LUTRAM
//#pragma HLS ARRAY_PARTITION variable = dht_tbl1 complete dim = 0
//#pragma HLS ARRAY_PARTITION variable = dht_tbl2 complete dim = 0
    //#pragma HLS bind_storage variable=dht_tbl1 type=RAM_2P impl=RAM

    hls::stream<CHType> image_strm("input_strm");
    hls::stream<bool>   eof_strm("eof_strm");
#pragma HLS bind_storage variable = image_strm type=FIFO impl=LUTRAM
#pragma HLS STREAM   variable = image_strm depth = 32
#pragma HLS bind_storage variable = eof_strm type=FIFO impl=LUTRAM
#pragma HLS STREAM   variable = eof_strm depth = 32
    // clang-format on

    // xf::common::utils_hw::axi_to_char_stream<BURST_LENTH, AXI_WIDTH, CHType>(ptr, image_strm, eof_strm, sz, c);
    xf::common::utils_hw::axiToCharStream<BURST_LENTH, AXI_WIDTH, CHType>(ptr, image_strm, eof_strm, sz, c);

    mcu_decoder(image_strm, eof_strm, dht_tbl1, dht_tbl2, hls_cmp, img_info.hls_cs_cmpc, hls_mbs, img_info.hls_mcuh,
                img_info.hls_mcuc, rst_cnt, block_strm);
}

// ------------------------------------------------------------

void parser_jpg_top(ap_uint<AXI_WIDTH>* datatoDDR,
                    const int size,
                    int& r,
                    int& c,
                    uint16_t dht_tbl1[2][2][1 << DHT1],
                    uint16_t dht_tbl2[2][2][1 << DHT2],
                    ap_uint<12>& hls_cmp,
                    int& left,

                    // image info
                    img_info& img_info,
                    uint8_t hls_mbs[MAX_NUM_COLOR],
                    hls_compInfo hls_compinfo[MAX_NUM_COLOR],
                    bool& rtn,
                    decOutput* plep) {
    ap_uint<AXI_WIDTH>* segment = datatoDDR;
    int offset = 0;
    uint8_t b1, b2, b3, b4;
    b1 = segment[0](7, 0);
    b2 = segment[0](15, 8);
    if ((size < 127) | (b1 != 0xFF) | (b2 != 0xD8)) {
        _XF_IMAGE_PRINT("Header failed\n");
        rtn = false;
    }
    readBytes(offset, 2, r, c);
    bool scanned = false;

    while (!scanned && rtn) {
        if (segment[r](c * 8 + 7, c * 8) != 0xFF) {
            _XF_IMAGE_PRINT("marker+length detect failed\n");
            rtn = false;
            break;
        }
        oneByte(offset, r, c); // skip marker ff and protect 16 bit length
        b2 = segment[r](c * 8 + 7, c * 8);
        oneByte(offset, r, c);
        uint8_t l1 = segment[r](c * 8 + 7, c * 8);
        oneByte(offset, r, c);
        uint8_t l2 = segment[r](c * 8 + 7, c * 8);
        oneByte(offset, r, c);
        uint16_t len = B_SHORT(l1, l2);
        len -= 2;

        if (((b2 & 0xF0) == 0xE0) || (b2 == 0xDD)) { // all APP

            _XF_IMAGE_PRINT("APP or DRI : OFFSET: %.8x\n", offset - 4);
            readBytes(offset, len, r, c);
            _XF_IMAGE_PRINT("skip %d Bytes of marker \n", len);

        } else if (b2 == 0xDB) {
            _XF_IMAGE_PRINT("DQT 0xDB: OFFSET: %.8x\n", offset - 4);
            // syn_build_DQT(len, offset,r,c, segment, dqt, rtn);
            while (len >= 64 + 1) {
                b1 = segment[r](c * 8 + 7, c * 8);
                oneByte(offset, r, c);
                if (b1 > 3) { // rtn = false;//19 byte marker
                    _XF_IMAGE_PRINT(" ERROR: DQT, ERROR idx \n");
                }
                for (int j = 0; j < 64; ++j) {
//#pragma HLS LOOP_TRIPCOUNT min=1 max=1
#pragma HLS PIPELINE
                    // dqt[b1][hls_jpeg_zigzag_to_raster[j]] = segment[r](c * 8 + 7, c * 8);
                    int jzz_x = hls_jpeg_zigzag_to_raster[j] & 7;
                    int jzz_y = hls_jpeg_zigzag_to_raster[j] >> 3;
                    if (b1 == 0)
                        plep->q_tables[0][jzz_y][jzz_x] = segment[r](c * 8 + 7, c * 8);
                    else {
                        plep->q_tables[1][jzz_y][jzz_x] = segment[r](c * 8 + 7, c * 8);
                        plep->q_tables[2][jzz_y][jzz_x] = segment[r](c * 8 + 7, c * 8);
                    }
                    // unsigned short vv = segment[r](c * 8 + 7, c * 8);
                    // dqt[b1][j] = segment[r](c * 8 + 7, c * 8);
                    oneByte(offset, r, c);
                }
                len -= 65;
            }
            // if(len) {_XF_IMAGE_PRINT("Decode DQT failed\n");}

        } else if (b2 == 0xC0) {
            // sof ffc0//min 17 byte marker
            // 2B B 2B 2B B    B   4   4   B
            // L P Y  X  CMP idx sfh sfv qidx
            _XF_IMAGE_PRINT("SOF 0xC0: OFFSET: %.8x\n", offset - 4);
            // syn_frame_SOF(len, offset,r,c, segment, hls_mbs, rtn, hls_compinfo,
            //	  hls_mcuc,hls_mcuh,hls_mcuv,hls_cs_cmpc);
            b1 = segment[r](c * 8 + 7, c * 8);
            if (b1 != 8) {
                // rtn=false;
                _XF_IMAGE_PRINT(" ERROR: SOF, image precision is not 8bit \n");
            }
            oneByte(offset, r, c);

            b1 = segment[r](c * 8 + 7, c * 8);
            oneByte(offset, r, c);
            b2 = segment[r](c * 8 + 7, c * 8);
            oneByte(offset, r, c);
            int height = B_SHORT(b1, b2);

            b1 = segment[r](c * 8 + 7, c * 8);
            oneByte(offset, r, c);
            b2 = segment[r](c * 8 + 7, c * 8);
            oneByte(offset, r, c);
            int width = B_SHORT(b1, b2);
            _XF_IMAGE_PRINT("height=%d, width=%d, \n", height, width);

            img_info.hls_cs_cmpc = segment[r](c * 8 + 7, c * 8);
            oneByte(offset, r, c);
            if (img_info.hls_cs_cmpc != 3) {
                // rtn= false;
                _XF_IMAGE_PRINT("ERROR: SOF, supports only 3 component color jpeg files\n");
            }

            uint8_t sfhm = 0, sfvm = 0;
            for (int cmp = 0; cmp < 3; ++cmp) {
#pragma HLS PIPELINE
                uint8_t sfv, sfh;
                oneByte(offset, r, c);
                b1 = segment[r](c * 8 + 7, c * 8);
                sfv = b1 >> 4;
                sfh = b1 & 0x0f;

                if ((sfv & (sfv - 1)) || (sfh & (sfh - 1))) {
                    // rtn= false;
                    _XF_IMAGE_PRINT("ERROR: SOF, sfv of sfh \n");
                }

                hls_compinfo[cmp].sfv = sfv;
                hls_compinfo[cmp].sfh = sfh;
                if (hls_compinfo[cmp].sfh > sfhm) sfhm = hls_compinfo[cmp].sfh;
                if (hls_compinfo[cmp].sfv > sfvm) sfvm = hls_compinfo[cmp].sfv;
                hls_mbs[cmp] = hls_compinfo[cmp].sfv * hls_compinfo[cmp].sfh;
                _XF_IMAGE_PRINT("sfv = %d, sfh = %d\n", sfv, sfh);
                // if (cmp == 0) downsample = sfv > 1;
                readBytes(offset, 2, r, c);
            }

            if (hls_mbs[0] == 4) {
                hls_cmp = 0b110000110000;
            } else if (hls_mbs[0] == 2) {
                hls_cmp = 0b110011001100;
            } else if (hls_mbs[0] == 1) {
                hls_cmp = 0b110110110110;
            } else {
                _XF_IMAGE_PRINT("ERROR: hls_cmpnfo[0].mbs is not 4/2/1 \n");
            }

            int sub_o_sfh = (height >> 3) / sfhm;
            int sub_o_sfv = (width >> 3) / sfvm;
            img_info.hls_mcuv = (height - (sub_o_sfh << 3) * sfhm) ? (sub_o_sfh + 1) : sub_o_sfh;
            img_info.hls_mcuh = (width - (sub_o_sfv << 3) * sfvm) ? (sub_o_sfv + 1) : sub_o_sfv;
            // hls_mcuv =  ( int ) ceil( (float) height / (float) ( 8 * sfhm ) );
            // hls_mcuh =  ( int ) ceil( (float) width  / (float) ( 8 * sfvm ) );
            img_info.hls_mcuc = img_info.hls_mcuv * img_info.hls_mcuh;

#ifndef __SYNTHESIS__
            printf("hls_mcuv=%d, hls_mcuh=%d, hls_mcuc=%d, \n", img_info.hls_mcuv, img_info.hls_mcuh,
                   img_info.hls_mcuc);
#endif
            for (int cmp = 0; cmp < 3; cmp++) {
#pragma HLS PIPELINE
                hls_compinfo[cmp].mbs = hls_mbs[cmp];
                hls_compinfo[cmp].bcv = img_info.hls_mcuv * hls_compinfo[cmp].sfh;
                hls_compinfo[cmp].bch = img_info.hls_mcuh * hls_compinfo[cmp].sfv;
                hls_compinfo[cmp].bc = hls_compinfo[cmp].bcv * hls_compinfo[cmp].bch;
            }

        } else if (b2 == 0xC4) {
            // min 19 byte marker
            // 2B  4  4       B
            // L  ac cmp    Vij*256
            // syn_DHT(len, offset,r,c, segment, dht_tbl1, dht_tbl2, rtn);
            _XF_IMAGE_PRINT("DHT 0xC4: OFFSET: %.8x\n", offset - 4);

            uint16_t huff_len = 1, cnt, addr_now, addr_gap;
            const int addr_all = 65536;
            uint16_t huff_cnt[16];
            ////////////////////////////////////////////////
            while (len > 16 + 1) {
                cnt = 0;
                b1 = segment[r](c * 8 + 7, c * 8);
                oneByte(offset, r, c);
                bool ac = b1 & 0x10;
                int cmp_huff = b1 & 0x0f;
                _XF_IMAGE_PRINT(" ac = %d, cmp =%d", ac, cmp_huff);
                if ((b1 & 0xEC) || (b1 & 0x02)) {             // check 0bxxxdxxdd
                    _XF_IMAGE_PRINT(" ERROR: DHT failed \n"); // rtn = false;
                }

                // init huff_cnt
                for (huff_len = 1; huff_len <= 16; ++huff_len) {
#pragma HLS PIPELINE
                    // DHT_segment[ac][cmp_huff].size[huff_len-1] =  segment[r](c*8+7, c*8);
                    huff_cnt[huff_len - 1] = segment[r](c * 8 + 7, c * 8);
                    oneByte(offset, r, c);
                }

                len -= 17;

                // init the val in each address
                for (huff_len = 1; huff_len <= 16; ++huff_len) {
                    // cnt = DHT_segment[ac][cmp_huff].size[huff_len - 1];
                    addr_now = addr_all >> huff_len;

                    _XF_IMAGE_PRINT(" Codes of length %d bits (%.3d total):", huff_len, cnt);
                    for (int j = 0; j < huff_cnt[huff_len - 1]; ++j) {
#pragma HLS PIPELINE
                        b1 = segment[r](c * 8 + 7, c * 8);
                        // DHT_segment[ac][cmp_huff].val[k] =  b1;
                        _XF_IMAGE_PRINT(" %.2x", b1);
                        oneByte(offset, r, c);

                        uint8_t run_vlen = b1;
                        uint8_t val_len = run_vlen & 0x0F;
                        uint8_t run_len = (run_vlen & 0xF0) >> 4;
                        uint8_t total_len = val_len + huff_len;
                        ap_uint<16> data = 0;
                        data(4, 0) = total_len;
                        data(9, 5) = huff_len;
                        data(13, 10) = run_len;
                        data[15] = huff_len > DHT1;
                        //_XF_IMAGE_PRINT(
                        //    " from [%d] to [%d],huff_len=%d, addr_org=%d\n",
                        //    (cnt >> DHT_S), (addr_now + cnt) >> DHT_S, huff_len, addr_now >> DHT_S);

                        for (int k = addr_now; k > 0; k -= huff_len > DHT1 ? 1 : (1 << DHT_S)) {
//#pragma HLS DEPENDENCE array inter false
#pragma HLS LOOP_TRIPCOUNT min = 1 max = 1
#pragma HLS PIPELINE II = 1

                            dht_tbl1[ac][cmp_huff][(cnt >> (DHT_S))] = data;
                            // dht_tbl_align[addr1 + (cnt >> (DHT_S))] = data;

                            if (huff_len > DHT1) {
                                dht_tbl2[ac][cmp_huff][(cnt % (DHT_M))] = data;
                                // dht_tbl_align[addr2  + (cnt % (DHT_M))] = data;
                                //_XF_IMAGE_PRINT(" huff_len > DHT1 :  [%d]  huff_len=%d, total_len=%d, addr_org=%d\n",
                                //                (cnt) % DHT_M, huff_len, total_len, addr_now % DHT_M);
                            }
                            if (huff_len > DHT1)
                                cnt++;
                            else
                                cnt += (1 << DHT_S);
                        } // end one val

                    } // end one huff_len

                    len -= huff_cnt[huff_len - 1];
                    //_XF_IMAGE_PRINT(" \n");

                } // end all huff
                // build_huffman_table3(DHT_segment, ac, cmp_huff, dht_tbl1, dht_tbl2);
            }
        } else if (b2 == 0xDA) {
            // min 12 byte marker
            // 2B  B      B    4    4 x3      B     B   	B
            // L  NS     CSj  Tdj  Taj      ss=0 se=63 ahal=0
            // syn_Scan_decode(size ,len, offset,r,c,  segment, dht_tbl1, dht_tbl2, rtn);
            _XF_IMAGE_PRINT("Scan 0xDA: OFFSET: %.8x\n", offset - 4);
            b1 = segment[r](c * 8 + 7, c * 8);
            if (b1 != 3) {
                // rtn = false;
                _XF_IMAGE_PRINT(" ERROR: SOS, ERROR CMP \n");
            }

            readBytes(offset, 10, r, c);
            // non-interleaving//to be added
            _XF_IMAGE_PRINT("Scan DATA: OFFSET: %.8x\n", offset);

            scanned = true;

        } else {
            _XF_IMAGE_PRINT("Undefined segment  %d\n", b2);
            rtn = false;
        }

    } // end while

    left = size - offset;
    // To full fill the structure plep with hls_compinfo
    // enum COLOR_FORMAT{C400=0, C420, C422, C444};
    // COLOR_FORMAT format;
    if (hls_compinfo[0].sfv == 2 && hls_compinfo[0].sfh == 2)
        plep->format = C420;
    else if (hls_compinfo[0].sfv == 1 && hls_compinfo[0].sfh == 1)
        plep->format = C444;
    else
        plep->format = C422;
    plep->axi_num_cmp = img_info.hls_cs_cmpc;
    plep->axi_map_row2cmp[0] = 2;
    plep->axi_map_row2cmp[1] = 1;
    plep->axi_map_row2cmp[2] = 0;
    plep->axi_map_row2cmp[3] = 0;
    if (plep->format == C400)
        plep->axi_num_cmp_mcu = 1; //? Not very sure
    else if (plep->format == C420)
        plep->axi_num_cmp_mcu = 4;
    else
        plep->axi_num_cmp_mcu = 3;
    plep->axi_width[0] = hls_compinfo[0].bch;
    plep->axi_width[1] =
        (plep->format == C400) ? 0 : (plep->format == C444) ? hls_compinfo[0].bch : (hls_compinfo[0].bch + 1) >> 1;
    plep->axi_width[2] =
        (plep->format == C400) ? 0 : (plep->format == C444) ? hls_compinfo[0].bch : (hls_compinfo[0].bch + 1) >> 1;
    //
    plep->axi_height[0] = hls_compinfo[0].bcv;
    plep->axi_height[1] =
        (plep->format == C400) ? 0 : (plep->format == C420) ? (hls_compinfo[0].bcv + 1) >> 1 : hls_compinfo[0].bcv;
    plep->axi_height[2] =
        (plep->format == C400) ? 0 : (plep->format == C420) ? (hls_compinfo[0].bcv + 1) >> 1 : hls_compinfo[0].bcv;
    plep->axi_mcuv =
        (plep->format == C400) ? 0 : (plep->format == C420) ? (hls_compinfo[0].bcv + 1) >> 1 : hls_compinfo[0].bcv;
    SetOtherQtab(plep);
}

} // namespace details
} // namespace codec
} // namespace xf

namespace xf {
namespace codec {
// ------------------------------------------------------------

// @brief Level 2 : kernel for jfif parser + huffman decoder

void kernel_parser_decoder(ap_uint<AXI_WIDTH>* datatoDDR,
                           const int size,

                           img_info& img_info,
                           hls_compInfo hls_cmpnfo[MAX_NUM_COLOR],
                           hls::stream<ap_uint<24> >& block_strm,
                           bool& rtn,
                           decOutput* plep) {
    // clang-format off
	//uint64_t max_pix = MAX_NUM_PIX;//for 8K*8K
	uint64_t max_pix = MAX_DEC_PIX;//for 800*800
#pragma HLS INTERFACE m_axi port = datatoDDR depth = max_pix offset = slave bundle = gmem_in2 \
    latency = 125 max_read_burst_length = 128
#pragma HLS INTERFACE s_axilite port=datatoDDR      bundle=control
#pragma HLS INTERFACE s_axilite port=return         bundle=control
    // clang-format on

    // for offset = r*scale_char + c
    int r = 0, c = 0;
    int left = 0;
    ap_uint<12> hls_cmp;
    uint32_t rst_cnt;
    uint8_t hls_mbs[MAX_NUM_COLOR];

    // clang-format off
    uint16_t 					dht_tbl1[2][2][1 << DHT1];
    uint16_t 					dht_tbl2[2][2][1 << DHT2];
#pragma HLS bind_storage variable = dht_tbl1 type=RAM_2P impl=LUTRAM
#pragma HLS bind_storage variable = dht_tbl2 type=RAM_2P impl=LUTRAM
    // clang-format on

    // Functions to parser the header before the data burst load from DDR
    //----------------------------------------------------------
    details::parser_jpg_top(datatoDDR, size, r, c, dht_tbl1, dht_tbl2, hls_cmp, left, img_info, hls_mbs, hls_cmpnfo,
                            rtn, plep);

    ap_uint<AXI_WIDTH>* ptr = datatoDDR + r;

    // Functions to decode the huffman code to non(Inverse quantization+IDCT) block coefficient
    //----------------------------------------------------------
    details::decoder_jpg_top(ptr, left, c, dht_tbl1, dht_tbl2, hls_cmp, hls_mbs, img_info, rst_cnt, block_strm);

#ifndef __SYNTHESIS__
    if (!rtn) {
        fprintf(stderr, "Error: parser the input file! \n");
    }
#endif
}

} // namespace codec
} // namespace xf
