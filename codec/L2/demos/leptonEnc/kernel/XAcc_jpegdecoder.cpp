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
 * @file XAcc_jpegdecoder.cpp
 * @brief mcu_decoder template function implementation.
 *
 * This file is part of HLS algorithm library.
 */

#include "XAcc_lepjpegdecoder.hpp"

#define DEVLI(s, n) ((s) == 0 ? (n) : (((n) >= (1 << ((s)-1))) ? (n) : (n) + 1 - (1 << (s))))

//**************************************

namespace xf {
namespace codec {
namespace details {

// ------------------------------------------------------------
void p1_cache_mcuLine(ap_uint<64> hls_block[8][MAX_NUM_BLOCK88_W * 8],
                      COLOR_FORMAT fmt,
                      hls::stream<ap_uint<24> >& block_strm,
                      int16_t width_cmp0,
                      int16_t width_cmp1) {
    int16_t end_blk_line = (fmt == C400)
                               ? width_cmp0
                               : (fmt == C420) ? (width_cmp0 << 1) + (width_cmp1 << 1) : width_cmp0 + (width_cmp1 << 1);
    int i_blk = 0;
    int cmp2 = 0;
    int32_t dpos2[3] = {0, 0, 0}; // Y,U,V
#pragma HLS ARRAY_PARTITION variable = dpos2 complete
    ap_uint<22> addr[4];
#pragma HLS ARRAY_PARTITION variable = addr complete dim = 1
    ap_uint<64> data[4];
#pragma HLS ARRAY_PARTITION variable = data complete dim = 1

    for (int i = 0; i < width_cmp0 * 8; i++) {
#pragma HLS pipeline
        for (int j = 0; j < 8; j++) {
            hls_block[j][i] = 0;
        }
    }

    for (int j = 0; j < 4; j++) {
#pragma HLS UNROLL
        addr[j] = 0;
        data[j] = 0;
    }

    while (i_blk < end_blk_line) {
#pragma HLS PIPELINE II = 1
#pragma HLS DEPENDENCE variable = hls_block inter true RAW distance = 4
        ap_uint<24> block_coeff = block_strm.read();
        bool is_endblock = block_coeff[22];
        ap_uint<8> bpos = block_coeff(21, 16);
        int16_t block = block_coeff(15, 0);

        ap_uint<14> addr2;
        if (fmt == C420 && cmp2 == 0) {
            //////////////////////////
            ap_uint<12> addr_blk;
            addr_blk(9, 1) = dpos2[0] >> 2;
            ;
            addr_blk(0, 0) = dpos2[0] & 1;
            addr_blk[10] = 0;
            addr_blk[11] = 0;
            addr2 = (addr_blk << 3) + (bpos >> 3);
        } else {
            addr2 = (dpos2[cmp2] << 3) + (bpos >> 3);
        }

        ap_uint<64> ramdata;
        ap_uint<22> rd_addr = (bpos & 7, addr2);
        if (rd_addr == addr[3])
            ramdata = data[3];
        else if (rd_addr == addr[2])
            ramdata = data[2];
        else if (rd_addr == addr[1])
            ramdata = data[1];
        else if (rd_addr == addr[0])
            ramdata = data[0];
        else
            ramdata = hls_block[bpos & 7][addr2];

        if (fmt == C420 && cmp2 == 0 && dpos2[0] & 2) {
            ramdata.range(63, 48) = block;
        } else if (fmt == C420 && cmp2 == 0) {
            ramdata.range(47, 32) = block;
        } else {
            ramdata.range(16 * (3 - cmp2) - 1, 16 * (2 - cmp2)) = block;
        }

        hls_block[bpos & 7][addr2] = ramdata;

        addr[0] = addr[1];
        addr[1] = addr[2];
        addr[2] = addr[3];
        addr[3] = (bpos & 7, addr2);
        data[0] = data[1];
        data[1] = data[2];
        data[2] = data[3];
        data[3] = ramdata;

        // hls_block2[addr2] = block;
        // std::cout<<addr2<<':'<<block<<' ';
        // if(is_endblock)std::cout<<std::endl<<"------Above cmp is:"<<cmp2<<"---------"<<std::endl;
        if (is_endblock) {
            if (fmt == C444) {
                dpos2[cmp2]++;
                cmp2 = (cmp2 == 2) ? 0 : cmp2 + 1;
            } else if (fmt == C422) {
                if (cmp2 == 0) {
                    if ((dpos2[0] & 1) == 1) {
                        cmp2 = 1;
                    }
                    dpos2[0]++;
                } else if (cmp2 == 1) {
                    cmp2 = 2;
                    dpos2[1]++;
                } else { // cmp==2
                    cmp2 = 0;
                    dpos2[2]++;
                }
            } else if (fmt == C420) {
                if (cmp2 == 0) {
                    if ((dpos2[0] & 3) == 3) {
                        cmp2 = 1;
                    }
                    dpos2[0]++;
                } else if (cmp2 == 1) {
                    cmp2 = 2;
                    dpos2[1]++;
                } else { // cmp==2
                    cmp2 = 0;
                    dpos2[2]++;
                }
            } else { // C400
                cmp2 = 0;
                dpos2[0]++; // II=2
            }

            i_blk++;
        } // end one block
    }     // end while
}

void hls_next_mcupos_strm(hls::stream<ap_uint<24> >& block_strm,
                          const decOutput plep,
                          // int16_t* hls_block2,
                          hls::stream<ap_int<11> > strm_coef[8],
                          uint16_t axi_width[MAX_NUM_COLOR],
                          uint16_t axi_height[MAX_NUM_COLOR]) {
    // int sta = 0; // status
    int test = 0;

    ///////////////////////
    COLOR_FORMAT fmt = plep.format;
    uint8_t axi_num_cmp_mcu = (fmt == C400) ? 1 : (fmt == C420) ? 4 : 3;
    // uint8_t axi_num_cmp_mcu    = (fmt==C400) ? 1 :  3;
    uint8_t axi_map_row2cmp[4] = {2, 1, 0, 0};
    // uint16_t end_mcu_v = fmt == C400 ? plep.axi_height[0] : plep.axi_height[1];
    // uint16_t plep_axi_width[3];
    // uint16_t block_width_0 = plep.axi_width[0];
    // uint16_t block_width_1 = plep.axi_width[1];
    // plep_axi_width[0] = plep.axi_width[0];
    // plep_axi_width[1] = plep.axi_width[1];
    // plep_axi_width[2] = plep.axi_width[2];
    uint16_t end_mcu_v = fmt == C400 ? axi_height[0] : axi_height[1];
    uint16_t plep_axi_width[3];
    uint16_t block_width_0 = axi_width[0];
    uint16_t block_width_1 = axi_width[1];
    plep_axi_width[0] = axi_width[0];
    plep_axi_width[1] = axi_width[1];
    plep_axi_width[2] = axi_width[2];

    // clang-format off
    ap_uint<64>                      hls_block [8][MAX_NUM_BLOCK88_W*8];
#pragma HLS bind_storage        variable=hls_block type=RAM_2P impl=URAM
#pragma HLS ARRAY_PARTITION variable=hls_block complete dim=1
// clang-format on

#ifndef __SYNTHESIS__
    fprintf(stderr, "hls_next_mcupos_strm start\n");
#endif

    for (int i_mucv = 0; i_mucv < end_mcu_v; i_mucv++) {
#pragma HLS dataflow
        p1_cache_mcuLine(hls_block,
                         fmt,           // COLOR_FORMAT fmt,
                         block_strm,    // hls::stream<ap_uint<24> >& block_strm,
                         block_width_0, // int16_t width_cmp0,
                         block_width_1  // int16_t width_cmp1,
                         );

        for (int idx_cmp = 0; idx_cmp < axi_num_cmp_mcu; idx_cmp++) {
            uint8_t id_cmp = axi_map_row2cmp[idx_cmp];
            uint16_t block_width = plep_axi_width[id_cmp];

            for (int jpeg_x = 0; jpeg_x + 0 < block_width; jpeg_x++) {
                int16_t coef_here[8][8];
                for (int i = 0; i < 8; i++) {
#pragma HLS pipeline
                    for (int j = 0; j < 8; j++) {
#pragma HLS unroll
                        strm_coef[j].write(hls_block[j][jpeg_x * 8 + i].range(16 * (idx_cmp + 1) - 1, 16 * idx_cmp));
                        // std::cout<<coef_here[i][j]<<' ';
                    }
                } // end pipeline

            } // end jpeg_x
        }     // end idx_cmp

    } // end sort
}

// ------------------------------------------------------------
void pick_huff_data(hls::stream<CHType>& image_strm,
                    hls::stream<bool>& eof_strm,
                    uint32_t& cnt_rst,
                    hls::stream<sos_data>& huff_sos_strm) {
#pragma HLS INLINE off

    int test = 0; // for debug

    uint8_t EOI_marker = 0xD9;
    uint8_t RST_filter = 0xD0;
    sos_data huff_sos;
    uint8_t bytes[16];
    uint8_t tmp[8];
#pragma HLS ARRAY_PARTITION variable = bytes complete
#pragma HLS ARRAY_PARTITION variable = tmp complete
    bool is_ff = false;
    bool entropy_end = false; // other marker:d8~dd. is marker ff00 ffff
    int rst_cnt = 0;

    bool eof = eof_strm.read();
    if (!eof) {
        ap_uint<CH_W> image = image_strm.read();
        eof = eof_strm.read();
        for (int i = 0; i < CH_W / 8; i++) {
#pragma HLS UNROLL
            bytes[i] = image(8 * i + 7, 8 * i);
        }
    }
    bool eof_reg = eof;

PICK_HUFF_LOOP:
    while (!eof_reg) { // eof_reg
#pragma HLS LOOP_TRIPCOUNT min = 5000 max = 5000
#pragma HLS PIPELINE II = 1
        eof_reg = eof; // read one more time to loop the shift buffer
        if (!eof) {
            ap_uint<CH_W> image = image_strm.read();
            eof = eof_strm.read();
            for (int i = 0; i < CH_W / 8; i++) {
#pragma HLS UNROLL
                bytes[CH_W / 8 + i] = image(8 * i + 7, 8 * i);
                tmp[i] = 0;
            }
        }
        if (eof_reg || eof) {
            // printf("test");
        }
        int idx = 0;
        int garbage_bytes = 16;
        bool rst_flag = false;
        for (int i = 0; i < CH_W / 8; i++) { // check the ff
#pragma HLS UNROLL
            bool Redu_data = (bytes[i] == 0xFF) && ((bytes[i + 1] == 0x00));
            if (!Redu_data || entropy_end) {
                tmp[idx] = (is_ff | entropy_end) ? 0xFF : bytes[i];
                idx++; // when !Redu_data or marker entropy_end
            }

            if (is_ff && ((bytes[i] & RST_filter) == 0xD0)) { // ff dn or ff ff
                garbage_bytes = idx;
            }

            if (is_ff && (bytes[i] == 0xD0 + (rst_cnt & 7))) {
                rst_flag |= true;
            }

            entropy_end = entropy_end | (is_ff && (bytes[i] == EOI_marker));
            if (bytes[i] == 0xFF) {
                is_ff = true;
            } else {
                is_ff = false;
            }
        }

        if (rst_flag) {
            rst_cnt++;
        }

        for (int i = 0; i < CH_W / 8; i++) {
#pragma HLS UNROLL
            if (i >= idx) tmp[i] = 0;
        }

        huff_sos.bits = idx * 8;
#if (CH_W == 32)
        huff_sos.data = tmp[0] << 24 | tmp[1] << 16 | tmp[2] << 8 | tmp[3];
#else
        huff_sos.data = tmp[0] << 8 | tmp[1];
#endif
        huff_sos.garbage_bits = garbage_bytes * 8;
        huff_sos.end_sos = false; // 7fff + 95 + false

        huff_sos_strm.write(huff_sos);

        // printf("\n  %.2x  %.2x  %.2x  %.2x",tmp[0],tmp[1],tmp[2],tmp[3]);
        for (int i = 0; i < CH_W / 8; i++) {
#pragma HLS UNROLL
            bytes[i] = bytes[i + CH_W / 8];
        }

#ifndef __SYNTHESIS__
        test++;
#endif
    } // endwhile

    cnt_rst = rst_cnt; //
// printf("cnt_rst = %d", cnt_rst);

#if (CH_W == 32)
    huff_sos.bits = CH_W;
    huff_sos.data = 0xffffffff;
    huff_sos.garbage_bits = CH_W - 16;
    huff_sos.end_sos = true;
    huff_sos_strm.write(huff_sos);
#else
    huff_sos.bits = CH_W;
    huff_sos.data = 0xffff;
    huff_sos.garbage_bits = CH_W - 16;
    huff_sos.end_sos = true;
    huff_sos_strm.write(huff_sos);
#endif
}

// ------------------------------------------------------------
void Huffman_decoder2(
    // input
    hls::stream<sos_data>& huff_sos_strm,
    const uint16_t dht_tbl1[2][2][1 << DHT1],
    const uint16_t dht_tbl2[2][2][1 << DHT2],
    const ap_uint<12> cyc_cmp,
// regs
#ifndef __SYNTHESIS__
    const uint8_t hls_cs_cmpc,
    const uint8_t hls_mbs[MAX_NUM_COLOR],
    const uint16_t hls_mcuh,
    const uint32_t hls_mcuc,
#endif
    // output
    hls::stream<ap_uint<24> >& block_strm) {

#pragma HLS INLINE off

    ap_uint<12> hls_cmp = cyc_cmp;
#pragma HLS bind_storage variable = hls_cmp type = FIFO impl = SRL
    int16_t lastDC[4] = {0, 0, 0, 0};
#pragma HLS ARRAY_PARTITION variable = lastDC complete

    // major parameter
    uint8_t huff_len = 0;    // the length of bits for huffman codes, eq with idx+1 1~16
    uint8_t run_len = 0;     // the number of zero before the non-zero ac coefficient 0~15
    uint8_t val_len = 0;     // the length of bits for value, 0~11
    uint8_t total_len = 0;   // huff_len + val_len 1~27
    uint8_t dec_len = 0;     // the length of tbl_data bits in buff 1~27
    ap_uint<24> block_coeff; // 23:is_rst, 22:is_endblock,21~16:bpos,15~0:block val
#ifndef __SYNTHESIS__
    uint8_t n_last = 0;
    uint8_t garbage_bits = 0;
    int cpmall = 0;
    int test = 0;
    int test_in8 = 0;
    int test_in16 = 0;
    int test_ov16 = 0;
    int cmp = 0;
    int mbs = 0;
    int n_mcu = 0;
#else
    ap_uint<6> n_last = 0;
    ap_uint<6> garbage_bits = 0;
#endif
    bool empty_n = false;
    bool e = false; // data end
    bool e_reg2 = false;
    bool e_reg1 = false;

    // tmp parameter
    ap_uint<16> input;
    ap_uint<8> input8;
    uint8_t bpos = 0;
    sos_data huff_sos;
    uint16_t val_i;
    int16_t block, block_tmp;
    bool ac = false;

    // major buffer
    ap_uint<2 * CH_W> buff_huff = 0; // the shift buffer
    ap_uint<2 * CH_W> buff_tail = 0; // the shift buffer
    ap_uint<3 * CH_W> buf_reg = 0;   // reg
    ap_uint<3 * CH_W> buf_reg0 = 0;  // reg
    ap_uint<3 * CH_W> buf_sft = 0;   // the shift buffer
    // accurate shift control group, to adjust circuit timing
    ap_uint<2 * CH_W> buf_dat0 = 0;
    ap_uint<CH_W> buf_dat1 = 0;
    ap_uint<CH_W> buf_dat2 = 0;
    ap_uint<CH_W> buf_dat3 = 0;
    ap_uint<2 * CH_W> buf1 = 0;
    ap_uint<2 * CH_W> buf2 = 0;

    // major flag to control state machine
    bool lookup_tbl2 = false;
    bool is_rst = false; // todo may be used in hls_next_mcupos2
    bool val_loop = false;
    bool next_block_reg = false;
    bool is_garbage = false;

    // tmp
    uint16_t tbl1 = 0;
    uint16_t tbl2 = 0;
    uint16_t tbl_data = 0;
    int tmp_bits = 0;

DECODE_LOOP:
    while (!e_reg2) {
#pragma HLS LOOP_TRIPCOUNT min = 10000 max = 10000
#pragma HLS DEPENDENCE array inter false
#pragma HLS PIPELINE II = 1

        //----------
        // 1. shift all buffers and read huff_sos
        n_last = n_last - dec_len;
        garbage_bits = garbage_bits - dec_len;
        buff_huff <<= dec_len;
#if 0
	    buf_sft = buf_reg;
	    buf_sft <<= dec_len;
	    buf_reg = buf_sft;
#else
        buf2(2 * CH_W - 1, CH_W) = buf_dat1;
        buf2(1 * CH_W - 1, 0) = buf_dat2;
        buf1(2 * CH_W - 1, CH_W) = buf_dat2;
        buf1(1 * CH_W - 1, 0) = buf_dat3;
        buf1 <<= dec_len;
        buf2 <<= dec_len;
        buf_dat1 = (CHType)(buf2(2 * CH_W - 1, CH_W));
        buf_dat2 = (CHType)(buf1(2 * CH_W - 1, CH_W));
        buf_dat3 = buf1(CH_W - 1, 0);
#endif

        if (garbage_bits == 0) {
            e_reg2 = e_reg1; // end all blocks
            is_garbage = false;
        }
        if ((n_last < CH_W) && (empty_n)) { // prepare data
#if 0
    	buff_tail = buf_reg(3*CH_W-1, 1*CH_W);
#else
            buff_tail(2 * CH_W - 1, CH_W) = (CHType)(buf2(2 * CH_W - 1, CH_W));
            buff_tail(1 * CH_W - 1, 0) = (CHType)(buf1(2 * CH_W - 1, CH_W));
#endif
            buff_huff |= buff_tail;
            tmp_bits = huff_sos.garbage_bits;
            if (tmp_bits <= 2 * CH_W) {
                garbage_bits = n_last + huff_sos.garbage_bits;
                is_garbage = true;
            }
            n_last += huff_sos.bits;
            e_reg1 = e;

            empty_n = false;
        }
        if ((empty_n == false) && (!e)) { // read data

            huff_sos = huff_sos_strm.read();
            e = huff_sos.end_sos;
#if 0
        buf_reg0(3*CH_W-1, 2*CH_W) = huff_sos.data ;
	    buf_reg = buf_reg0 ;
	    buf_reg>>=n_last;
#else
            buf_dat0 = huff_sos.data;
            buf_dat1 = buf_dat0 >> n_last;
            buf_dat2 = (buf_dat0 << CH_W) >> n_last;
            // buf_dat3 = (buf_dat0 << (63 - n_last))<<1 ;//CH_W==32
            buf_dat3 = (buf_dat0 << (31 - n_last)) << 1; // CH_W==16
#endif
            empty_n = true;
        }

        // decode one huffman code from 32b
        bool freeze_out = (n_last < CH_W) && (!e_reg1); // unfreeze
        input = buff_huff(2 * CH_W - 1, 2 * CH_W - 16);
        input8 = buff_huff(2 * CH_W - 1, 2 * CH_W - 8);
        is_rst = false;
#if (CH_W == 16)
        //----------
        // 2. look up the table
        ap_uint<DHT1> addr1 = input(15, 16 - DHT1);
        ap_uint<DHT2> addr2 = input(DHT2 - 1, 0);

        tbl1 = dht_tbl1[ac][hls_cmp[0]][addr1];
        tbl2 = dht_tbl2[ac][hls_cmp[0]][addr2];

        dec_len = 0;

        if (!val_loop) {
            tbl_data = (tbl1 >> 15) ? tbl2 : tbl1;

            total_len = tbl_data & 0x1F;
            huff_len = (tbl_data >> 5) & 0x1F;
            run_len = (tbl_data >> 10) & 0x0F;
            val_len = total_len - huff_len;

            if (!freeze_out) { // if reset, false valid
                if (input == 0xFFFF) {
                    // if((garbage_bits<24 && (input8 == 0xFF) &&(!e_reg2) )  ){

                    if (garbage_bits <= 16) {
                        dec_len = garbage_bits;
                    } else {
                        dec_len = garbage_bits - 16;
                    }

                    ac = false;
                    freeze_out = true;
                    val_loop = false;
                    lastDC[0] = 0;
                    lastDC[1] = 0;
                    lastDC[2] = 0;
                } else {
                    if (total_len <= 15) {
                        dec_len = total_len;
                        freeze_out = false;
                        val_loop = false;
                    } else {
                        dec_len = huff_len;
                        freeze_out = true;
                        val_loop = true;
                    }
                }
            }
        } else {
            if (!freeze_out) { // wait until there is enough data

                huff_len = 0;
                total_len = val_len;
                dec_len = total_len;
                val_loop = false;
            }
        }

#elif (CH_W == 32)
        //----------
        // 2. look up the table anyway
        if (!lookup_tbl2) {
            ap_uint<DHT1> addr1 = input(15, 16 - DHT1);
            tbl1 = dht_tbl1[ac][hls_cmp[0]][addr1];
            lookup_tbl2 = (tbl1 >> 15);
            if (!lookup_tbl2) {
                tbl_data = tbl1;
                total_len = tbl_data & 0x1F;
                huff_len = (tbl_data >> 5) & 0x1F;
                run_len = (tbl_data >> 10) & 0x0F;
                val_len = total_len - huff_len;
            } else {
                total_len = 0;
                lookup_tbl2 = true;
            }

        } else {
            ap_uint<DHT2> addr2 = input(DHT2 - 1, 0);
            tbl2 = dht_tbl2[ac][hls_cmp[0]][addr2];
            tbl_data = tbl2;
            total_len = tbl_data & 0x1F;
            huff_len = (tbl_data >> 5) & 0x1F;
            run_len = (tbl_data >> 10) & 0x0F;
            val_len = total_len - huff_len;
            lookup_tbl2 = false;
        }

        //----------
        // 3. update dec_len and pos in the block
        dec_len = 0;

        if (!freeze_out) { // if reset, false valid
            if ((input) == 0xFFFF) {
                dec_len = garbage_bits;
                ac = false;
                freeze_out = true;
                lookup_tbl2 = false;
                lastDC[0] = 0;
                lastDC[1] = 0;
                lastDC[2] = 0;
            } else {
                dec_len = total_len;
                freeze_out = (lookup_tbl2) ? true : false;
            }
        }

#endif

        //----------
        // 3. get the value
        if (!freeze_out) {
#ifndef __SYNTHESIS__
            if (run_len > 0) //&&(test>=187270)
                _XF_IMAGE_PRINT(" run_len = %d \n", (int)run_len);
#endif

            if (val_len) {
                val_i = buff_huff(2 * CH_W - 1 - huff_len, 2 * CH_W - total_len);
            } else {
                val_i = 0;
            }
            block_tmp = DEVLI(val_len, val_i);
        }

        bool eob = !freeze_out && ac && ((run_len | val_len) == 0);

        if (!freeze_out) {
            if (ac) {
                bpos = bpos + 1 + run_len;
                block = block_tmp;
                // if(test>=187270)
                _XF_IMAGE_PRINT("AC: huff_len = %d , block[%d] = %d\n", (int)huff_len, bpos, (int)block);
            } else {
                ac = true;
                bpos = 0;
                block = lastDC[0] + block_tmp;
                lastDC[0] = block;
                _XF_IMAGE_PRINT("\nDC: huff_len = %d , dc_val_i = %d\n", (int)huff_len, (int)block_tmp);
            }
        }

        //----------
        // 4. write out
        if (!freeze_out) {
            if (!eob) {
                block_coeff[23] = is_rst && (bpos == 63);
                block_coeff[22] = (bpos == 63);
                block_coeff(21, 16) = (uint8_t)bpos;
                block_coeff(15, 0) = block;
                block_strm.write(block_coeff);

            } else { // is eob W [63]=0

                block_coeff[23] = is_rst;
                block_coeff[22] = 1;
                block_coeff(21, 16) = (uint8_t)(63);
                block_coeff(15, 0) = 0;
                block_strm.write(block_coeff);
                _XF_IMAGE_PRINT(" ================ eob [%d,63] \n", bpos);
            }
#ifndef __SYNTHESIS__
            if (total_len <= 8) {
                test_in8++;
            } else if (total_len <= 16) {
                test_in16++;
            } else {
                test_ov16++;
            }
#endif
        }

        //----------
        // 5. next_block update and shift sampling cmp
        bool next_block = (eob || (!freeze_out && (bpos == 63)));
        // next_block_reg = freeze_out? next_block_reg : next_block;
        if (next_block) {
            ac = false;
            ap_uint<1> tmp_sft = hls_cmp[0];
            if (hls_cmp[0] | hls_cmp[1]) {
                int16_t tmpDC = lastDC[0];

                lastDC[0] = lastDC[1];
                lastDC[1] = lastDC[2];
                lastDC[2] = tmpDC;
            }
            hls_cmp >>= 1;
            hls_cmp[11] = tmp_sft;

#ifndef __SYNTHESIS__

            if (cmp < hls_cs_cmpc - 1) {
                if (mbs < hls_mbs[cmp] - 1) {
                    mbs++;
                } else {
                    mbs = 0;
                    cmp++;
                }
            } else {
                cmp = 0;
                n_mcu++;
            }

            cpmall = hls_mbs[0] + hls_mbs[1] + hls_mbs[2];

            // clang-format off
            _XF_IMAGE_PRINT(" block decode %d  times !! mcu [%d, %d][%d] block \n", test, (test / (cpmall)) % hls_mcuh,
                            test / ((cpmall)*hls_mcuh), test % (cpmall)); // test 420
            _XF_IMAGE_PRINT(" lft_in_buff = %d  n_mcu=%d *****\n\n *********************\n", (int)(n_last - dec_len),n_mcu);
            if (((test / (cpmall)) % hls_mcuh == 27) && (test / ((cpmall)*hls_mcuh) == 58) && (test % (cpmall) == 0)) {
                cpmall = hls_mbs[0] + hls_mbs[1] + hls_mbs[2];
            }
            // clang-format on
            test++;
#endif

        } // end new block

    } // end decode one block and loop all mcu/cmp/mbs

#ifndef __SYNTHESIS__
    // clang-format off
    if (test != hls_mcuc * cpmall) {
        std::cout << "ERROR : there is error blocks!" << std::endl;
    }
    std::cout << "run :" << test << " times !!!!!" << std::endl;
#if (CH_W == 32)
    std::cout << "test_in8 :" << test_in8 << ", test_in16:" << test_in16 << ", test_ov16:" << test_ov16 << std::endl;
    int allblock = test_in8 + test_in16 + test_ov16;
    std::cout << "test_in8 :" << (float)test_in8 / allblock << ", test_in16:" << (float)test_in16 / allblock
              << ", test_ov16:" << (float)test_ov16 / allblock << std::endl;
#endif
// clang-format on
#endif
}

// ------------------------------------------------------------
void mcu_decoder(
    // input
    hls::stream<CHType>& image_strm,
    hls::stream<bool>& eof_strm,
    const uint16_t dht_tbl1[2][2][1 << DHT1],
    const uint16_t dht_tbl2[2][2][1 << DHT2],
    ap_uint<12> hls_cmp,

    // image info
    const uint8_t hls_cs_cmpc, // component count in current scan
    const uint8_t hls_mbs[MAX_NUM_COLOR],
    const uint16_t hls_mcuh, // the horizontal mcu
    const uint32_t hls_mcuc, // the total mcu

    // output
    uint32_t& rst_cnt,
    hls::stream<ap_uint<24> >& block_strm) {
#pragma HLS DATAFLOW

    // clang-format off
    hls::stream<sos_data> huff_sos_strm;
#pragma HLS DATA_PACK variable = huff_sos_strm
#pragma HLS bind_storage  variable = huff_sos_strm type=FIFO impl=LUTRAM
#pragma HLS STREAM    variable = huff_sos_strm depth = 32
    // clang-format on

    pick_huff_data(image_strm, eof_strm, rst_cnt, huff_sos_strm);

    Huffman_decoder2(huff_sos_strm, dht_tbl1, dht_tbl2, hls_cmp,
#ifndef __SYNTHESIS__
                     hls_cs_cmpc, hls_mbs, hls_mcuh, hls_mcuc,
#endif
                     block_strm);
}

// ------------------------------------------------------------
// for JPEG-D
void hls_next_mcupos2(hls::stream<ap_uint<24> >& block_strm,
                      int16_t hls_block[MAX_NUM_COLOR * MAXCMP_BC * 64],
                      int hls_sfv[4],
                      int hls_sfh[4],
                      const uint8_t hls_mbs[4],
                      int hls_bch,
                      int hls_bc,
                      int32_t hls_mcuc,
                      uint8_t hls_cs_cmpc,
                      int& sta) {
    // int sta = 0; // status
    int test = 0;

    int n_mcu = 0;
    int cmp = 0;
    int mbs = 0;
    ap_uint<24> block_coeff;
    bool is_endblock;
    uint8_t bpos = 0;
    int16_t block;
    //  int lastdc[4] = {0, 0, 0, 0}; // last dc for each component
    int dpos[MAX_NUM_COLOR] = {0};
//#pragma HLS ARRAY_PARTITION variable = lastdc complete
#pragma HLS ARRAY_PARTITION variable = dpos complete

    while (!sta) {
#pragma HLS PIPELINE II = 1
        block_coeff = block_strm.read();
        is_endblock = block_coeff[22];
        bpos = block_coeff(21, 16);
        block = block_coeff(15, 0);

        hls_block[(cmp)*hls_bc * 64 + (dpos[cmp]) * 64 + bpos] = block;

        if (is_endblock) {
            unsigned int sfh = hls_sfh[cmp]; // 2   1    1
            unsigned int sfv = hls_sfv[cmp]; // 2   2    1
            if (sfh > 1) {                   // 420 cmp=0
                if (cmp != 0) {
                    _XF_IMAGE_PRINT("ERROR: next_mcu 420 case, cmp!=0");
                    sta = 2;
                }
                if (mbs == 0) {
                    dpos[cmp]++;
                } else if (mbs == 1) {
                    dpos[cmp] += hls_bch - 1;
                } else if (mbs == 2) {
                    dpos[cmp]++;
                } else {
                    if (dpos[cmp] % (2 * hls_bch) == 2 * hls_bch - 1) {
                        dpos[cmp]++;
                    } else {
                        dpos[cmp] -= hls_bch - 1;
                    }
                }
            } else if (sfv > 1) { // 422 cmp=0
                if (cmp != 0) {
                    _XF_IMAGE_PRINT("ERROR: next_mcu 422 case, cmp!=0");
                    sta = 2;
                }
                dpos[cmp]++;
            } else { // 420 cmp=1/2 422 cmp=1/2 444 cmp=0/1/2
                dpos[cmp]++;
            }
            if (n_mcu < hls_mcuc) {
                if (cmp < hls_cs_cmpc - 1) {
                    if (mbs < hls_mbs[cmp] - 1) { // 420:4/422:2/444:1
                        mbs++;
                    } else {
                        mbs = 0;
                        cmp++;
                    }
                } else {
                    cmp = 0;
                    n_mcu++;
                    if (n_mcu == hls_mcuc) {
                        sta = 2;
                    }
                }
            }

            test++;
        } // end one block

    } // end while
}

} // namespace details
} // namespace codec
} // namespace xf
