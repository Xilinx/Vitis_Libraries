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
 * @file kernelJpegDecoder.cpp
 * @brief kernelJpegDecoder template function implementation and kernel_decoder warpper.
 *
 * This file is part of HLS algorithm library.
 */

#include "kernelJpegDecoder.hpp"
// ------------------------------------------------------------
// @brief Level 2 : kernel for jfif parser + huffman decoder + iQ_iDCT
// a.input the jpg 420/422/444 baseline file
// b.output the as the 8x8 's Column scan order YUV (0~255), like [Y*allpixels,U*0.5*allpixels, V*0.5*allpixels], and
// image infos
// c.Fault tolerance: If the picture's format is incorrect, error codes will directly end the kernel
// and wait for the input of the next image. Error codes cloud help to position at which decoding stage does the error
// occur
// d.performance: input throughput: 150MB/s~300MB/s(1symbol/clk), output 1~1.6GB/s (max 8B/clk),
// frequency 250MHz for kernel, for only huffman core 286MHz by vivado 2018.3

extern "C" void kernelJpegDecoder(ap_uint<AXI_WIDTH>* jpeg_pointer,
                                  const int size,
                                  ap_uint<64>* yuv_mcu_pointer,
                                  ap_uint<32>* infos) {
    // clang-format off
	//const uint64_t max_pix = MAX_NUM_PIX;//for 8K*8K
	const uint64_t max_pix = MAX_DEC_PIX;//for 800*800
	const uint64_t max_yuv = MAXCMP_BC * 8;//blocknum * 8 rows
	const uint64_t burst_lenth = BURST_LENTH;
#pragma HLS INTERFACE m_axi port = jpeg_pointer     depth = 65000 offset = slave bundle = gmem_in0 \
					  latency = 64 num_read_outstanding = 32 max_read_burst_length = 32
#pragma HLS INTERFACE m_axi port = yuv_mcu_pointer 	depth = 230400 offset = slave bundle = gmem_in1 \
					  latency = 64 num_write_outstanding = 32 max_write_burst_length = 32
#pragma HLS INTERFACE m_axi port = infos 			depth = 1024   offset = slave bundle = gmem_in2 \
					  latency = 64 num_write_outstanding = 32 max_write_burst_length = 32
	#pragma HLS INTERFACE s_axilite port=jpeg_pointer      	bundle=control
	#pragma HLS INTERFACE s_axilite port=yuv_mcu_pointer    bundle=control
	#pragma HLS INTERFACE s_axilite port=size      			bundle=control
	#pragma HLS INTERFACE s_axilite port=infos    		    bundle=control
	#pragma HLS INTERFACE s_axilite port=return         	bundle=control

	//for jfif parser
    int r = 0, c = 0;
    int left = 0;
    ap_uint<12> hls_cmp;
    uint8_t hls_mbs[MAX_NUM_COLOR] = {0};
#pragma HLS ARRAY_PARTITION variable = hls_mbs  complete

    //for reset of the decoder
    uint32_t rst_cnt;
    int rtn;
    int rtn2;

    //tables
	uint8_t 						   q_tables[2][8][8];
#pragma HLS ARRAY_PARTITION variable = q_tables  dim = 3
    uint16_t 					       dht_tbl1[2][2][1 << DHT1];
#pragma HLS ARRAY_PARTITION variable = dht_tbl1 complete dim = 1
#pragma HLS ARRAY_PARTITION variable = dht_tbl1 complete dim = 2

	uint8_t ac_value_buckets  [2][ 165 ];//every val relative with the huffman codes
	HCODE_T ac_huff_start_code[2][AC_N]; // the huff_start_code<65535
	int16_t ac_huff_start_addr[2][16];   // the addr of the val huff_start_addr<256
	uint8_t dc_value_buckets  [2][ 12 ]; //every val relative with the huffman codes
	HCODE_T dc_huff_start_code[2][DC_N]; // the huff_start_code<65535
	int16_t dc_huff_start_addr[2][16];   // the addr of the val huff_start_addr<256
//#pragma HLS RESOURCE 		variable = ac_huff_start_code core = RAM_2P_LUTRAM
#pragma HLS ARRAY_PARTITION variable = ac_huff_start_code complete dim = 2
//#pragma HLS RESOURCE 		variable = dc_huff_start_code core = RAM_2P_LUTRAM
#pragma HLS ARRAY_PARTITION variable = dc_huff_start_code complete dim = 2

#pragma HLS RESOURCE variable = ac_huff_start_addr  core = RAM_2P_LUTRAM
#pragma HLS RESOURCE variable = ac_value_buckets 	core = RAM_2P_LUTRAM
#pragma HLS RESOURCE variable = dc_huff_start_addr  core = RAM_2P_LUTRAM
#pragma HLS RESOURCE variable = dc_value_buckets 	core = RAM_2P_LUTRAM

	// image infos for multi-applications
    xf::image::img_info img_info;
    xf::image::cmp_info cmp_info[MAX_NUM_COLOR];
    xf::image::bas_info bas_info;
    img_info.hls_cs_cmpc = 0;//init

    // Functions to parser the header before the data burst load from DDR
    //----------------------------------------------------------
    xf::image::details::parser_jpg_top(jpeg_pointer, size, r, c, dht_tbl1,
    								   ac_value_buckets, ac_huff_start_code, ac_huff_start_addr,
                                       dc_value_buckets, dc_huff_start_code, dc_huff_start_addr,
									   hls_cmp, left,
									   hls_mbs, q_tables, rtn,
									   img_info, cmp_info, bas_info);

    ap_uint<AXI_WIDTH>* ptr = jpeg_pointer + r;

    // Functions to decode the huffman code to non(Inverse quantization+IDCT) block coefficient
    //----------------------------------------------------------
    xf::image::details::decoder_jpg_full_top(ptr, left, c, dht_tbl1,
											 ac_value_buckets, ac_huff_start_code, ac_huff_start_addr,
											 dc_value_buckets, dc_huff_start_code, dc_huff_start_addr,
											 hls_cmp, hls_mbs, q_tables, img_info, bas_info,
											 rtn2, rst_cnt, yuv_mcu_pointer);
											 //strm_iDCT_x8);//idx_coef,

    // Functions to copy image infos from struct to pointer for axi master
    //----------------------------------------------------------
    xf::image::details::copyInfo(img_info, cmp_info, bas_info, rtn, rtn2, infos);

// clang-format on
#ifndef __SYNTHESIS__
    if (rtn || (rtn2)) {
        fprintf(stderr, "Warning: parser the bad case input file! \n");
    }
#endif
}
