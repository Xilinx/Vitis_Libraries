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
#include "jpeg_dec_lepton_enc.hpp"

// ------------------------------------------------------------
namespace xf {
namespace codec {
namespace details {

void init_parser(ap_uint<16>* datatoDDR,
                 int size,
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
    //#pragma HLS DATAFLOW
    // Functions to init
    //----------------------------------------------------------
    init_hlsmodel();

    // Functions to parser the header before the data burst load from DDR
    //----------------------------------------------------------
    xf::codec::details::parser_jpg_top(datatoDDR, size, r, c, dht_tbl1, dht_tbl2, hls_cmp, left, img_info, hls_mbs,
                                       hls_compinfo, rtn, plep);
}

// ------------------------------------------------------------

void jpegD_leptonE_union(ap_uint<AXI_WIDTH>* ptr,
                         const int sz,
                         const int c,
                         const uint16_t dht_tbl1[2][2][1 << DHT1],
                         const uint16_t dht_tbl2[2][2][1 << DHT2],
                         ap_uint<12> hls_cmp,

                         // image info
                         const uint8_t hls_mbs[MAX_NUM_COLOR],
                         const img_info img_info,
                         const int left,
                         decOutput& lepp,

                         // output
                         uint32_t& rst_cnt,
                         ap_uint<8>* axi_res, //[MAX_NUM_PIX]
                         struct_arith& axi_arith,
                         uint16_t axi_width[MAX_NUM_COLOR],
                         uint16_t axi_height[MAX_NUM_COLOR],
                         uint8_t axi_map_row2cmp[4]) {
#pragma HLS DATAFLOW

    // clang-format off
	hls::stream<ap_uint<24> >   block_strm;
#pragma HLS bind_storage variable = block_strm type=FIFO impl=LUTRAM
#pragma HLS STREAM variable   = block_strm depth = 32
    // clang-format on
    // Functions to decode the huffman code to non(Inverse quantization+IDCT) block coefficient
    //----------------------------------------------------------
    xf::codec::details::decoder_jpg_top(ptr, left, (ap_uint<1>)c, dht_tbl1, dht_tbl2, hls_cmp, hls_mbs, img_info,
                                        rst_cnt, block_strm);

    // clang-format off
	hls::stream<ap_int<11> >         str_coef[8];
#pragma HLS bind_storage        variable = str_coef   type=FIFO impl=LUTRAM
//#pragma HLS ARRAY_PARTITION variable = str_coef   complete  dim=1
#pragma HLS STREAM          variable = str_coef   depth = 1024 dim=1
    // clang-format on
    // Copy to aligned block
    //-----------------------------------------------------------
    hls_next_mcupos_strm(block_strm, lepp, str_coef, axi_width, axi_height);

    // clang-format off
 	hls::stream< bool >          strm_pos_o_e;
 	hls::stream< ap_uint<8> > strm_pos_o_byte;
 #pragma HLS stream depth=256 variable = strm_pos_o_e
 #pragma HLS bind_storage         variable = strm_pos_o_e    type=FIFO impl=LUTRAM
 #pragma HLS stream depth=256 variable = strm_pos_o_byte
 #pragma HLS bind_storage         variable = strm_pos_o_byte type=FIFO impl=LUTRAM
    // clang-format on
    // leptonE kernel
    //-----------------------------------------------------------
    kernel_LeptonE_strmIn_engine(
        // input
        str_coef,

        axi_width, // colldata->block_width(i);
        // lepp.axi_height,//colldata->block_width(i);
        axi_map_row2cmp, //     AXI                   2,1,0,0 2,1,0
        lepp.min_nois_thld_x, lepp.min_nois_thld_y,
        lepp.q_tables, //[64],
        lepp.idct_q_table_x, lepp.idct_q_table_y,
        // lepp.idct_q_table_l,
        lepp.axi_mcuv, lepp.axi_num_cmp_mcu,
        // lepp.axi_num_cmp,
        // output
        axi_arith, strm_pos_o_e, strm_pos_o_byte);

// clang-format off
	#ifndef __SYNTHESIS__
		fprintf(stderr,"stream out start\n");
	#endif
    // clang-format on
    // stream out
    //-----------------------------------------------------------
    // xf::common::utils_hw::streamToAxi<32, uint8_t, unsigned char>(strm_pos_o_byte, strm_pos_o_e, axi_res);
    // template <int _BurstLen, int _WAxi, int _WStrm>
    // void streamToAxi(ap_uint<_WAxi>* wbuf, hls::stream<ap_uint<_WStrm> >& istrm, hls::stream<bool>& e_istrm) {
    xf::common::utils_hw::streamToAxi<32, 8, 8>(axi_res, strm_pos_o_byte, strm_pos_o_e);
}

} // namespace details
} // namespace codec
} // namespace xf

// ------------------------------------------------------------

namespace xf {
namespace codec {

/**
 * @brief IMGAE Jpeg Decoder Lepton Encoder Kernel
 * \rst
 * For detailed document, see :ref:`JpegDecoderLeptonEncoderKernel`.
 * \endrst
 * @param datainDDR input image buffer.
 * @param jpgSize size of input image buffer.
 * @param arithInfo meta information of output buffer.
 * @param res output lepton format data buffer.
 */
void jpegDecLeptonEnc(ap_uint<AXI_WIDTH>* datainDDR, int jpgSize, int arithInfo[9], ap_uint<8>* res) {
    //        #pragma HLS INTERFACE m_axi offset = slave latency = 125 \
//        num_write_outstanding = 1 num_read_outstanding = 2 \
//        max_write_burst_length = 2 max_read_burst_length = 32 \
//        bundle = gmem_in1 port = datainDDR
    //
    //        #pragma HLS INTERFACE m_axi offset = slave latency = 125 \
//        num_write_outstanding = 2 num_read_outstanding = 2 \
//        max_write_burst_length = 32 max_read_burst_length = 2 \
//        bundle = gmem_out1 port = res
    //
    //        #pragma HLS INTERFACE m_axi offset = slave latency = 32 \
//        num_write_outstanding = 2 num_read_outstanding = 2 \
//        max_write_burst_length = 32 max_read_burst_length = 2 \
//        bundle = gmem_out2 port = arithInfo
    //
    //
    //		#pragma HLS INTERFACE s_axilite port=datainDDR     	bundle=control
    //		#pragma HLS INTERFACE s_axilite port=res        	bundle=control
    //		#pragma HLS INTERFACE s_axilite port=jpgSize        bundle=control
    //		#pragma HLS INTERFACE s_axilite port=arithInfo      bundle=control
    //
    //		#pragma HLS INTERFACE s_axilite port=return         bundle=control
    // image infos
    bool rtn = true;
    decOutput lepp;
    struct_arith arith;
    // clang-format off
//		#pragma HLS ARRAY_PARTITION variable=lepp.axi_map_row2cmp complete dim=0
//		#pragma HLS ARRAY_PARTITION variable=lepp.axi_width       complete dim=0
    // clang-format on
    int r = 0, c = 0; // for offset = r*scale_char + c
    ap_uint<12> hls_cmp;
    xf::codec::img_info img_info; // may have some redundant data
    xf::codec::hls_compInfo hls_cmpnfo[MAX_NUM_COLOR];
    uint8_t hls_mbs[MAX_NUM_COLOR];
    int left = 0;
    // tables
    uint16_t dqt[2][64];
    uint16_t dht_tbl1[2][2][1 << DHT1];
    uint16_t dht_tbl2[2][2][1 << DHT2];
#pragma HLS bind_storage variable = dht_tbl1 type = RAM_2P impl = LUTRAM
#pragma HLS bind_storage variable = dht_tbl2 type = RAM_2P impl = LUTRAM

#ifndef __SYNTHESIS__
    fprintf(stderr, "kernel start!\n");
#endif
    // Functions to parser the header and init rams before the data burst load from DDR
    //----------------------------------------------------------
    xf::codec::details::init_parser(datainDDR, jpgSize, r, c, dht_tbl1, dht_tbl2, hls_cmp, left, img_info, hls_mbs,
                                    hls_cmpnfo, rtn, &lepp);

    decOutput lepp2 = lepp;
    uint16_t axi_width[MAX_NUM_COLOR];
    uint16_t axi_height[MAX_NUM_COLOR];
    uint8_t axi_map_row2cmp[4];
    uint8_t min_nois_thld_x[MAX_NUM_COLOR][64];
    uint8_t min_nois_thld_y[MAX_NUM_COLOR][64];
    uint8_t q_tables[MAX_NUM_COLOR][8][8];
    int32_t idct_q_table_x[MAX_NUM_COLOR][8][8];
    int32_t idct_q_table_y[MAX_NUM_COLOR][8][8];
    int32_t idct_q_table_l[MAX_NUM_COLOR][8][8];
    for (int i = 0; i < MAX_NUM_COLOR; i++) {
        axi_width[i] = lepp.axi_width[i];
        axi_height[i] = lepp.axi_height[i];
    }
    for (int i = 0; i < 4; i++) {
        axi_map_row2cmp[i] = lepp.axi_map_row2cmp[i];
    }
    for (int i = 0; i < MAX_NUM_COLOR; i++) {
        for (int j = 0; j < 64; j++) {
            min_nois_thld_x[i][j] = lepp.min_nois_thld_x[i][j];
            min_nois_thld_y[i][j] = lepp.min_nois_thld_y[i][j];
        }
    }
    for (int i = 0; i < MAX_NUM_COLOR; i++) {
        for (int j = 0; j < 8; j++) {
            for (int k = 0; k < 8; k++) {
                q_tables[i][j][k] = lepp.q_tables[i][j][k];
                idct_q_table_x[i][j][k] = lepp.idct_q_table_x[i][j][k];
                idct_q_table_y[i][j][k] = lepp.idct_q_table_y[i][j][k];
                idct_q_table_l[i][j][k] = lepp.idct_q_table_l[i][j][k];
            }
        }
    }

    uint32_t rst_cnt;
    // Functions to burst load from DDR forms the dataflow region
    //----------------------------------------------------------
    ap_uint<AXI_WIDTH>* ptr = (ap_uint<AXI_WIDTH>*)datainDDR + r;
    xf::codec::details::jpegD_leptonE_union(ptr, left, c, dht_tbl1, dht_tbl2, hls_cmp, hls_mbs, img_info, left, lepp2,
                                            rst_cnt, res, arith, axi_width, axi_height, axi_map_row2cmp);

    // Tails
    //----------------------------------------------------------
    res[arith.pos++] = arith.pre_byte;
    for (int run = arith.run; run > 0; run--) res[arith.pos++] = 0xff;

    // clang-format off
    	arithInfo[0] = arith.count;
    	arithInfo[1] = arith.value;
    	arithInfo[2] = arith.pre_byte;
    	arithInfo[3] = arith.run ;
    	arithInfo[4] = arith.pos ;
    	arithInfo[5] = arith.range ;
    	arithInfo[6] = arith.isFirst ;
    	arithInfo[7] = left;
    	arithInfo[8] = rst_cnt;
    // clang-format on
} // extern "C"
} // namespace codec
} // namespace xf
