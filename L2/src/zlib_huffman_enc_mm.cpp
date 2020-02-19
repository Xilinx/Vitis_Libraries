/*
 * (c) Copyright 2019 Xilinx, Inc. All rights reserved.
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
 *
 */
/**
 * @file zlib_huffman_enc_mm.cpp
 * @brief Source for huffman kernel.
 *
 * This file is part of Vitis Data Compression Library.
 */
#include "zlib_huffman_enc_mm.hpp"

// 64bits/8bit = 8 Bytes
typedef ap_uint<16> uintOutV_t;

// 4 Bytes containing LL (1), ML (1), OFset (2)
typedef ap_uint<32> encoded_dt;

// 8 * 4 = 32 Bytes containing LL (1), ML (1), OFset (2)
typedef ap_uint<32> encodedV_dt;

void huffmanCore(hls::stream<xf::compression::uintMemWidth_t>& inStream512,
                 hls::stream<xf::compression::uintMemWidth_t>& outStream512,
                 hls::stream<uint16_t>& outStream512Size,
                 uint32_t input_size,
                 uint32_t core_idx,
                 hls::stream<uint32_t>& inStreamLTreeCode,
                 hls::stream<uint32_t>& inStreamLTreeBlen,
                 hls::stream<uint32_t>& inStreamDTreeCode,
                 hls::stream<uint32_t>& inStreamDTreeBlen,
                 hls::stream<uint32_t>& inStreamBLTreeCode,
                 hls::stream<uint32_t>& inStreamBLTreeBlen,
                 hls::stream<uint32_t>& inStreamMaxCode) {
    const uint32_t c_gmemBSize = 32;

    hls::stream<encodedV_dt> inStream("inStream");
    hls::stream<uintOutV_t> huffOut("huffOut");
    hls::stream<uint16_t> huffOutSize("huffOutSize");
    hls::stream<uint16_t> bitVals("bitVals");
    hls::stream<uint8_t> bitLen("bitLen");
#pragma HLS STREAM variable = inStream depth = c_gmemBSize
#pragma HLS STREAM variable = huffOut depth = c_gmemBSize
#pragma HLS STREAM variable = huffOutSize depth = c_gmemBSize
#pragma HLS STREAM variable = bitVals depth = c_gmemBSize
#pragma HLS STREAM variable = bitLen depth = c_gmemBSize

#pragma HLS RESOURCE variable = inStream core = FIFO_SRL
#pragma HLS RESOURCE variable = huffOut core = FIFO_SRL
#pragma HLS RESOURCE variable = huffOutSize core = FIFO_SRL
#pragma HLS RESOURCE variable = bitVals core = FIFO_SRL
#pragma HLS RESOURCE variable = bitLen core = FIFO_SRL

#pragma HLS dataflow
    // Read data from kernel 1 in stream downsized manner
    // Write the data back to ddr and read it in host
    xf::compression::details::streamDownsizer<uint32_t, GMEM_DWIDTH, 32>(inStream512, inStream, input_size);

    xf::compression::huffmanEncoder(inStream, bitVals, bitLen, input_size, inStreamLTreeCode, inStreamLTreeBlen,
                                    inStreamDTreeCode, inStreamDTreeBlen, inStreamBLTreeCode, inStreamBLTreeBlen,
                                    inStreamMaxCode);

    xf::compression::details::bitPackingSize(bitVals, bitLen, huffOut, huffOutSize);

    xf::compression::details::upsizer_sizestream<uint16_t, 16, GMEM_DWIDTH>(huffOut, huffOutSize, outStream512,
                                                                            outStream512Size);
}

void huffman(const xf::compression::uintMemWidth_t* in,
             xf::compression::uintMemWidth_t* out,
             uint32_t input_idx[PARALLEL_BLOCK],
             uint32_t output_idx[PARALLEL_BLOCK],
             uint32_t input_size[PARALLEL_BLOCK],
             uint32_t output_size[PARALLEL_BLOCK],
             uint32_t* dyn_litmtree_codes,
             uint32_t* dyn_distree_codes,
             uint32_t* dyn_bitlentree_codes,
             uint32_t* dyn_litmtree_blen,
             uint32_t* dyn_dtree_blen,
             uint32_t* dyn_bitlentree_blen,
             uint32_t* dyn_max_codes,
             uint32_t n_blocks) {
    const uint32_t c_gmemBSize = 32;

    hls::stream<xf::compression::uintMemWidth_t> inStream512_0;
    hls::stream<uint16_t> outStream512Size_0;
    hls::stream<xf::compression::uintMemWidth_t> outStream512_0;

    hls::stream<uint32_t> strlitmtree_codes_0;
    hls::stream<uint32_t> strlitmtree_blen_0;

    hls::stream<uint32_t> strdistree_codes_0;
    hls::stream<uint32_t> strdtree_blen_0;

    hls::stream<uint32_t> strbitlentree_codes_0;
    hls::stream<uint32_t> strbitlentree_blen_0;
    hls::stream<uint32_t> strmax_code_0;
#pragma HLS STREAM variable = outStream512Size_0 depth = c_gmemBSize
#pragma HLS STREAM variable = inStream512_0 depth = c_gmemBSize
#pragma HLS STREAM variable = outStream512_0 depth = c_gmemBSize
#pragma HLS STREAM variable = strlitmtree_codes_0 depth = c_gmemBSize
#pragma HLS STREAM variable = strlitmtree_blen_0 depth = c_gmemBSize
#pragma HLS STREAM variable = strdistree_codes_0 depth = c_gmemBSize
#pragma HLS STREAM variable = strdtree_blen_0 depth = c_gmemBSize
#pragma HLS STREAM variable = strbitlentree_codes_0 depth = c_gmemBSize
#pragma HLS STREAM variable = strbitlentree_blen_0 depth = c_gmemBSize
#pragma HLS STREAM variable = strmax_code_0 depth = c_gmemBSize

#pragma HLS RESOURCE variable = outStream512Size_0 core = FIFO_SRL
#pragma HLS RESOURCE variable = inStream512_0 core = FIFO_SRL
#pragma HLS RESOURCE variable = outStream512_0 core = FIFO_SRL

#if PARALLEL_BLOCK > 1
    hls::stream<xf::compression::uintMemWidth_t> inStream512_1("huffManinStream512_1");
    hls::stream<uint16_t> outStream512Size_1("outStream512Size_1");
    hls::stream<xf::compression::uintMemWidth_t> outStream512_1("outStream512_1");
    hls::stream<uint32_t> strlitmtree_codes_1("stream_litmtree_codes_1");
    hls::stream<uint32_t> strlitmtree_blen_1("stream_litmtree_blen_1");

    hls::stream<uint32_t> strdistree_codes_1("stream_distree_codes_1");
    hls::stream<uint32_t> strdtree_blen_1("stream_dtree_blen_1");

    hls::stream<uint32_t> strbitlentree_codes_1("stream_bitlentree_codes_1");
    hls::stream<uint32_t> strbitlentree_blen_1("stream_bitlentree_blen_1");
    hls::stream<uint32_t> strmax_code_1("strmax_code_1");
#pragma HLS STREAM variable = outStream512Size_1 depth = c_gmemBSize
#pragma HLS STREAM variable = inStream512_1 depth = c_gmemBSize
#pragma HLS STREAM variable = outStream512_1 depth = c_gmemBSize
#pragma HLS STREAM variable = strlitmtree_codes_1 depth = c_gmemBSize
#pragma HLS STREAM variable = strlitmtree_blen_1 depth = c_gmemBSize

#pragma HLS STREAM variable = strdistree_codes_1 depth = c_gmemBSize
#pragma HLS STREAM variable = strdtree_blen_1 depth = c_gmemBSize

#pragma HLS STREAM variable = strbitlentree_codes_1 depth = c_gmemBSize
#pragma HLS STREAM variable = strbitlentree_blen_1 depth = c_gmemBSize
#pragma HLS STREAM variable = strmax_code_1 depth = c_gmemBSize

#pragma HLS RESOURCE variable = outStream512Size_1 core = FIFO_SRL
#pragma HLS RESOURCE variable = inStream512_1 core = FIFO_SRL
#pragma HLS RESOURCE variable = outStream512_1 core = FIFO_SRL
#pragma HLS RESOURCE variable = strlitmtree_codes_1 core = FIFO_SRL
#pragma HLS RESOURCE variable = strlitmtree_blen_1 core = FIFO_SRL

#pragma HLS RESOURCE variable = strdistree_codes_1 core = FIFO_SRL
#pragma HLS RESOURCE variable = strdtree_blen_1 core = FIFO_SRL

#pragma HLS RESOURCE variable = strbitlentree_codes_1 core = FIFO_SRL
#pragma HLS RESOURCE variable = strbitlentree_blen_1 core = FIFO_SRL
#pragma HLS RESOURCE variable = strmax_code_1 core = FIFO_SRL
#endif

#if PARALLEL_BLOCK > 2
    hls::stream<xf::compression::uintMemWidth_t> inStream512_2("huffManinStream512_2");
    hls::stream<uint16_t> outStream512Size_2("outStream512Size_2");
    hls::stream<xf::compression::uintMemWidth_t> outStream512_2("outStream512_2");
    hls::stream<uint32_t> strlitmtree_codes_2("stream_litmtree_codes_2");
    hls::stream<uint32_t> strlitmtree_blen_2("stream_litmtree_blen_2");
    hls::stream<uint32_t> strdistree_codes_2("stream_distree_codes_2");
    hls::stream<uint32_t> strdtree_blen_2("stream_dtree_blen_2");
    hls::stream<uint32_t> strbitlentree_codes_2("stream_bitlentree_codes_2");
    hls::stream<uint32_t> strbitlentree_blen_2("stream_bitlentree_blen_2");
    hls::stream<uint32_t> strmax_code_2("strmax_code_2");
#pragma HLS STREAM variable = outStream512Size_2 depth = c_gmemBSize
#pragma HLS STREAM variable = inStream512_2 depth = c_gmemBSize
#pragma HLS STREAM variable = outStream512_2 depth = c_gmemBSize
#pragma HLS STREAM variable = strlitmtree_codes_2 depth = c_gmemBSize
#pragma HLS STREAM variable = strlitmtree_blen_2 depth = c_gmemBSize
#pragma HLS STREAM variable = strdistree_codes_2 depth = c_gmemBSize
#pragma HLS STREAM variable = strdtree_blen_2 depth = c_gmemBSize
#pragma HLS STREAM variable = strbitlentree_codes_2 depth = c_gmemBSize
#pragma HLS STREAM variable = strbitlentree_blen_2 depth = c_gmemBSize
#pragma HLS STREAM variable = strmax_code_2 depth = c_gmemBSize

#pragma HLS RESOURCE variable = outStream512Size_2 core = FIFO_SRL
#pragma HLS RESOURCE variable = inStream512_2 core = FIFO_SRL
#pragma HLS RESOURCE variable = outStream512_2 core = FIFO_SRL
#pragma HLS RESOURCE variable = strlitmtree_codes_2 core = FIFO_SRL
#pragma HLS RESOURCE variable = strlitmtree_blen_2 core = FIFO_SRL
#pragma HLS RESOURCE variable = strdistree_codes_2 core = FIFO_SRL
#pragma HLS RESOURCE variable = strdtree_blen_2 core = FIFO_SRL
#pragma HLS RESOURCE variable = strbitlentree_codes_2 core = FIFO_SRL
#pragma HLS RESOURCE variable = strbitlentree_blen_2 core = FIFO_SRL
#pragma HLS RESOURCE variable = strmax_code_2 core = FIFO_SRL

    hls::stream<xf::compression::uintMemWidth_t> inStream512_3("huffManinStream512_3");
    hls::stream<uint16_t> outStream512Size_3("outStream512Size_3");
    hls::stream<xf::compression::uintMemWidth_t> outStream512_3("outStream512_3");
    hls::stream<uint32_t> strlitmtree_codes_3("stream_litmtree_codes_3");
    hls::stream<uint32_t> strlitmtree_blen_3("stream_litmtree_blen_3");
    hls::stream<uint32_t> strdistree_codes_3("stream_distree_codes_3");
    hls::stream<uint32_t> strdtree_blen_3("stream_dtree_blen_3");
    hls::stream<uint32_t> strbitlentree_codes_3("stream_bitlentree_codes_3");
    hls::stream<uint32_t> strbitlentree_blen_3("stream_bitlentree_blen_3");
    hls::stream<uint32_t> strmax_code_3("strmax_code_3");
#pragma HLS STREAM variable = outStream512Size_3 depth = c_gmemBSize
#pragma HLS STREAM variable = inStream512_3 depth = c_gmemBSize
#pragma HLS STREAM variable = outStream512_3 depth = c_gmemBSize
#pragma HLS STREAM variable = strlitmtree_codes_3 depth = c_gmemBSize
#pragma HLS STREAM variable = strlitmtree_blen_3 depth = c_gmemBSize
#pragma HLS STREAM variable = strdistree_codes_3 depth = c_gmemBSize
#pragma HLS STREAM variable = strdtree_blen_3 depth = c_gmemBSize
#pragma HLS STREAM variable = strbitlentree_codes_3 depth = c_gmemBSize
#pragma HLS STREAM variable = strbitlentree_blen_3 depth = c_gmemBSize
#pragma HLS STREAM variable = strmax_code_3 depth = c_gmemBSize

#pragma HLS RESOURCE variable = outStream512Size_3 core = FIFO_SRL
#pragma HLS RESOURCE variable = inStream512_3 core = FIFO_SRL
#pragma HLS RESOURCE variable = outStream512_3 core = FIFO_SRL
#pragma HLS RESOURCE variable = strlitmtree_codes_3 core = FIFO_SRL
#pragma HLS RESOURCE variable = strlitmtree_blen_3 core = FIFO_SRL
#pragma HLS RESOURCE variable = strdistree_codes_3 core = FIFO_SRL
#pragma HLS RESOURCE variable = strdtree_blen_3 core = FIFO_SRL
#pragma HLS RESOURCE variable = strbitlentree_codes_3 core = FIFO_SRL
#pragma HLS RESOURCE variable = strbitlentree_blen_3 core = FIFO_SRL
#pragma HLS RESOURCE variable = strmax_code_3 core = FIFO_SRL

#endif

#if PARALLEL_BLOCK > 4

    hls::stream<xf::compression::uintMemWidth_t> inStream512_4("huffManinStream512_4");
    hls::stream<uint16_t> outStream512Size_4("outStream512Size_4");
    hls::stream<xf::compression::uintMemWidth_t> outStream512_4("outStream512_4");
    hls::stream<uint32_t> strlitmtree_codes_4("stream_litmtree_codes_4");
    hls::stream<uint32_t> strlitmtree_blen_4("stream_litmtree_blen_4");
    hls::stream<uint32_t> strdistree_codes_4("stream_distree_codes_4");
    hls::stream<uint32_t> strdtree_blen_4("stream_dtree_blen_4");
    hls::stream<uint32_t> strbitlentree_codes_4("stream_bitlentree_codes_4");
    hls::stream<uint32_t> strbitlentree_blen_4("stream_bitlentree_blen_4");
    hls::stream<uint32_t> strmax_code_4("strmax_code_4");
#pragma HLS STREAM variable = outStream512Size_4 depth = c_gmemBSize
#pragma HLS STREAM variable = inStream512_4 depth = c_gmemBSize
#pragma HLS STREAM variable = outStream512_4 depth = c_gmemBSize
#pragma HLS STREAM variable = strlitmtree_codes_4 depth = c_gmemBSize
#pragma HLS STREAM variable = strlitmtree_blen_4 depth = c_gmemBSize
#pragma HLS STREAM variable = strdistree_codes_4 depth = c_gmemBSize
#pragma HLS STREAM variable = strdtree_blen_4 depth = c_gmemBSize
#pragma HLS STREAM variable = strbitlentree_codes_4 depth = c_gmemBSize
#pragma HLS STREAM variable = strbitlentree_blen_4 depth = c_gmemBSize
#pragma HLS STREAM variable = strmax_code_4 depth = c_gmemBSize

#pragma HLS RESOURCE variable = outStream512Size_4 core = FIFO_SRL
#pragma HLS RESOURCE variable = inStream512_4 core = FIFO_SRL
#pragma HLS RESOURCE variable = outStream512_4 core = FIFO_SRL
#pragma HLS RESOURCE variable = strlitmtree_codes_4 core = FIFO_SRL
#pragma HLS RESOURCE variable = strlitmtree_blen_4 core = FIFO_SRL
#pragma HLS RESOURCE variable = strdistree_codes_4 core = FIFO_SRL
#pragma HLS RESOURCE variable = strdtree_blen_4 core = FIFO_SRL
#pragma HLS RESOURCE variable = strbitlentree_codes_4 core = FIFO_SRL
#pragma HLS RESOURCE variable = strbitlentree_blen_4 core = FIFO_SRL
#pragma HLS RESOURCE variable = strmax_code_4 core = FIFO_SRL

    hls::stream<xf::compression::uintMemWidth_t> inStream512_5("huffManinStream512_5");
    hls::stream<uint16_t> outStream512Size_5("outStream512Size_5");
    hls::stream<xf::compression::uintMemWidth_t> outStream512_5("outStream512_5");
    hls::stream<uint32_t> strlitmtree_codes_5("stream_litmtree_codes_5");
    hls::stream<uint32_t> strlitmtree_blen_5("stream_litmtree_blen_5");
    hls::stream<uint32_t> strdistree_codes_5("stream_distree_codes_5");
    hls::stream<uint32_t> strdtree_blen_5("stream_dtree_blen_5");
    hls::stream<uint32_t> strbitlentree_codes_5("stream_bitlentree_codes_5");
    hls::stream<uint32_t> strbitlentree_blen_5("stream_bitlentree_blen_5");
    hls::stream<uint32_t> strmax_code_5("strmax_code_5");
#pragma HLS STREAM variable = outStream512Size_5 depth = c_gmemBSize
#pragma HLS STREAM variable = inStream512_5 depth = c_gmemBSize
#pragma HLS STREAM variable = outStream512_5 depth = c_gmemBSize
#pragma HLS STREAM variable = strlitmtree_codes_5 depth = c_gmemBSize
#pragma HLS STREAM variable = strlitmtree_blen_5 depth = c_gmemBSize
#pragma HLS STREAM variable = strdistree_codes_5 depth = c_gmemBSize
#pragma HLS STREAM variable = strdtree_blen_5 depth = c_gmemBSize
#pragma HLS STREAM variable = strbitlentree_codes_5 depth = c_gmemBSize
#pragma HLS STREAM variable = strbitlentree_blen_5 depth = c_gmemBSize
#pragma HLS STREAM variable = strmax_code_5 depth = c_gmemBSize

#pragma HLS RESOURCE variable = outStream512Size_5 core = FIFO_SRL
#pragma HLS RESOURCE variable = inStream512_5 core = FIFO_SRL
#pragma HLS RESOURCE variable = outStream512_5 core = FIFO_SRL
#pragma HLS RESOURCE variable = strlitmtree_codes_5 core = FIFO_SRL
#pragma HLS RESOURCE variable = strlitmtree_blen_5 core = FIFO_SRL
#pragma HLS RESOURCE variable = strdistree_codes_5 core = FIFO_SRL
#pragma HLS RESOURCE variable = strdtree_blen_5 core = FIFO_SRL
#pragma HLS RESOURCE variable = strbitlentree_codes_5 core = FIFO_SRL
#pragma HLS RESOURCE variable = strbitlentree_blen_5 core = FIFO_SRL
#pragma HLS RESOURCE variable = strmax_code_5 core = FIFO_SRL

    hls::stream<xf::compression::uintMemWidth_t> inStream512_6("huffManinStream512_6");
    hls::stream<uint16_t> outStream512Size_6("outStream512Size_6");
    hls::stream<xf::compression::uintMemWidth_t> outStream512_6("outStream512_6");
    hls::stream<uint32_t> strlitmtree_codes_6("stream_litmtree_codes_6");
    hls::stream<uint32_t> strlitmtree_blen_6("stream_litmtree_blen_6");
    hls::stream<uint32_t> strdistree_codes_6("stream_distree_codes_6");
    hls::stream<uint32_t> strdtree_blen_6("stream_dtree_blen_6");
    hls::stream<uint32_t> strbitlentree_codes_6("stream_bitlentree_codes_6");
    hls::stream<uint32_t> strbitlentree_blen_6("stream_bitlentree_blen_6");
    hls::stream<uint32_t> strmax_code_6("strmax_code_6");
#pragma HLS STREAM variable = outStream512Size_6 depth = c_gmemBSize
#pragma HLS STREAM variable = inStream512_6 depth = c_gmemBSize
#pragma HLS STREAM variable = outStream512_6 depth = c_gmemBSize
#pragma HLS STREAM variable = strlitmtree_codes_6 depth = c_gmemBSize
#pragma HLS STREAM variable = strlitmtree_blen_6 depth = c_gmemBSize
#pragma HLS STREAM variable = strdistree_codes_6 depth = c_gmemBSize
#pragma HLS STREAM variable = strdtree_blen_6 depth = c_gmemBSize
#pragma HLS STREAM variable = strbitlentree_codes_6 depth = c_gmemBSize
#pragma HLS STREAM variable = strbitlentree_blen_6 depth = c_gmemBSize
#pragma HLS STREAM variable = strmax_code_6 depth = c_gmemBSize

#pragma HLS RESOURCE variable = outStream512Size_6 core = FIFO_SRL
#pragma HLS RESOURCE variable = inStream512_6 core = FIFO_SRL
#pragma HLS RESOURCE variable = outStream512_6 core = FIFO_SRL
#pragma HLS RESOURCE variable = strlitmtree_codes_6 core = FIFO_SRL
#pragma HLS RESOURCE variable = strlitmtree_blen_6 core = FIFO_SRL
#pragma HLS RESOURCE variable = strdistree_codes_6 core = FIFO_SRL
#pragma HLS RESOURCE variable = strdtree_blen_6 core = FIFO_SRL
#pragma HLS RESOURCE variable = strbitlentree_codes_6 core = FIFO_SRL
#pragma HLS RESOURCE variable = strbitlentree_blen_6 core = FIFO_SRL
#pragma HLS RESOURCE variable = strmax_code_6 core = FIFO_SRL

    hls::stream<xf::compression::uintMemWidth_t> inStream512_7("huffManinStream512_7");
    hls::stream<uint16_t> outStream512Size_7("outStream512Size_7");
    hls::stream<xf::compression::uintMemWidth_t> outStream512_7("outStream512_7");
    hls::stream<uint32_t> strlitmtree_codes_7("stream_litmtree_codes_7");
    hls::stream<uint32_t> strlitmtree_blen_7("stream_litmtree_blen_7");
    hls::stream<uint32_t> strdistree_codes_7("stream_distree_codes_7");
    hls::stream<uint32_t> strdtree_blen_7("stream_dtree_blen_7");
    hls::stream<uint32_t> strbitlentree_codes_7("stream_bitlentree_codes_7");
    hls::stream<uint32_t> strbitlentree_blen_7("stream_bitlentree_blen_7");
    hls::stream<uint32_t> strmax_code_7("strmax_code_7");
#pragma HLS STREAM variable = outStream512Size_7 depth = c_gmemBSize
#pragma HLS STREAM variable = inStream512_7 depth = c_gmemBSize
#pragma HLS STREAM variable = outStream512_7 depth = c_gmemBSize
#pragma HLS STREAM variable = strlitmtree_codes_7 depth = c_gmemBSize
#pragma HLS STREAM variable = strlitmtree_blen_7 depth = c_gmemBSize
#pragma HLS STREAM variable = strdistree_codes_7 depth = c_gmemBSize
#pragma HLS STREAM variable = strdtree_blen_7 depth = c_gmemBSize
#pragma HLS STREAM variable = strbitlentree_codes_7 depth = c_gmemBSize
#pragma HLS STREAM variable = strbitlentree_blen_7 depth = c_gmemBSize
#pragma HLS STREAM variable = strmax_code_7 depth = c_gmemBSize

#pragma HLS RESOURCE variable = outStream512Size_7 core = FIFO_SRL
#pragma HLS RESOURCE variable = inStream512_7 core = FIFO_SRL
#pragma HLS RESOURCE variable = outStream512_7 core = FIFO_SRL
#pragma HLS RESOURCE variable = strlitmtree_codes_7 core = FIFO_SRL
#pragma HLS RESOURCE variable = strlitmtree_blen_7 core = FIFO_SRL
#pragma HLS RESOURCE variable = strdistree_codes_7 core = FIFO_SRL
#pragma HLS RESOURCE variable = strdtree_blen_7 core = FIFO_SRL
#pragma HLS RESOURCE variable = strbitlentree_codes_7 core = FIFO_SRL
#pragma HLS RESOURCE variable = strbitlentree_blen_7 core = FIFO_SRL
#pragma HLS RESOURCE variable = strmax_code_7 core = FIFO_SRL

#endif

#pragma HLS dataflow
    xf::compression::details::mm2s_freq<GMEM_DWIDTH, GMEM_BURST_SIZE>(
        in, dyn_litmtree_codes, dyn_litmtree_blen, dyn_distree_codes, dyn_dtree_blen, dyn_bitlentree_codes,
        dyn_bitlentree_blen, dyn_max_codes, input_idx, inStream512_0,
#if PARALLEL_BLOCK > 1
        inStream512_1,
#endif
#if PARALLEL_BLOCK > 2
        inStream512_2, inStream512_3,
#endif
#if PARALLEL_BLOCK > 4
        inStream512_4, inStream512_5, inStream512_6, inStream512_7,
#endif
        strlitmtree_codes_0,
#if PARALLEL_BLOCK > 1
        strlitmtree_codes_1,
#endif
#if PARALLEL_BLOCK > 2
        strlitmtree_codes_2, strlitmtree_codes_3,
#endif
#if PARALLEL_BLOCK > 4
        strlitmtree_codes_4, strlitmtree_codes_5, strlitmtree_codes_6, strlitmtree_codes_7,
#endif
        strlitmtree_blen_0,
#if PARALLEL_BLOCK > 1
        strlitmtree_blen_1,
#endif
#if PARALLEL_BLOCK > 2
        strlitmtree_blen_2, strlitmtree_blen_3,
#endif
#if PARALLEL_BLOCK > 4
        strlitmtree_blen_4, strlitmtree_blen_5, strlitmtree_blen_6, strlitmtree_blen_7,
#endif
        strdistree_codes_0,
#if PARALLEL_BLOCK > 1
        strdistree_codes_1,
#endif
#if PARALLEL_BLOCK > 2
        strdistree_codes_2, strdistree_codes_3,
#endif
#if PARALLEL_BLOCK > 4
        strdistree_codes_4, strdistree_codes_5, strdistree_codes_6, strdistree_codes_7,
#endif
        strdtree_blen_0,
#if PARALLEL_BLOCK > 1
        strdtree_blen_1,
#endif
#if PARALLEL_BLOCK > 2
        strdtree_blen_2, strdtree_blen_3,
#endif
#if PARALLEL_BLOCK > 4
        strdtree_blen_4, strdtree_blen_5, strdtree_blen_6, strdtree_blen_7,
#endif
        strbitlentree_codes_0,
#if PARALLEL_BLOCK > 1
        strbitlentree_codes_1,
#endif
#if PARALLEL_BLOCK > 2
        strbitlentree_codes_2, strbitlentree_codes_3,
#endif
#if PARALLEL_BLOCK > 4
        strbitlentree_codes_4, strbitlentree_codes_5, strbitlentree_codes_6, strbitlentree_codes_7,
#endif
        strbitlentree_blen_0,
#if PARALLEL_BLOCK > 1
        strbitlentree_blen_1,
#endif
#if PARALLEL_BLOCK > 2
        strbitlentree_blen_2, strbitlentree_blen_3,
#endif
#if PARALLEL_BLOCK > 4
        strbitlentree_blen_4, strbitlentree_blen_5, strbitlentree_blen_6, strbitlentree_blen_7,
#endif
        strmax_code_0,
#if PARALLEL_BLOCK > 1
        strmax_code_1,
#endif
#if PARALLEL_BLOCK > 2
        strmax_code_2, strmax_code_3,
#endif
#if PARALLEL_BLOCK > 4
        strmax_code_4, strmax_code_5, strmax_code_6, strmax_code_7,
#endif
        n_blocks, input_size);

    huffmanCore(inStream512_0, outStream512_0, outStream512Size_0, input_size[0], 0, strlitmtree_codes_0,
                strlitmtree_blen_0, strdistree_codes_0, strdtree_blen_0, strbitlentree_codes_0, strbitlentree_blen_0,
                strmax_code_0);

#if PARALLEL_BLOCK > 1
    huffmanCore(inStream512_1, outStream512_1, outStream512Size_1, input_size[1], 1, strlitmtree_codes_1,
                strlitmtree_blen_1, strdistree_codes_1, strdtree_blen_1, strbitlentree_codes_1, strbitlentree_blen_1,
                strmax_code_1);
#endif

#if PARALLEL_BLOCK > 2
    huffmanCore(inStream512_2, outStream512_2, outStream512Size_2, input_size[2], 2, strlitmtree_codes_2,
                strlitmtree_blen_2, strdistree_codes_2, strdtree_blen_2, strbitlentree_codes_2, strbitlentree_blen_2,
                strmax_code_2);

    huffmanCore(inStream512_3, outStream512_3, outStream512Size_3, input_size[3], 3, strlitmtree_codes_3,
                strlitmtree_blen_3, strdistree_codes_3, strdtree_blen_3, strbitlentree_codes_3, strbitlentree_blen_3,
                strmax_code_3);
#endif

#if PARALLEL_BLOCK > 4
    huffmanCore(inStream512_4, outStream512_4, outStream512Size_4, input_size[4], 4, strlitmtree_codes_4,
                strlitmtree_blen_4, strdistree_codes_4, strdtree_blen_4, strbitlentree_codes_4, strbitlentree_blen_4,
                strmax_code_4);

    huffmanCore(inStream512_5, outStream512_5, outStream512Size_5, input_size[5], 5, strlitmtree_codes_5,
                strlitmtree_blen_5, strdistree_codes_5, strdtree_blen_5, strbitlentree_codes_5, strbitlentree_blen_5,
                strmax_code_5);

    huffmanCore(inStream512_6, outStream512_6, outStream512Size_6, input_size[6], 6, strlitmtree_codes_6,
                strlitmtree_blen_6, strdistree_codes_6, strdtree_blen_6, strbitlentree_codes_6, strbitlentree_blen_6,
                strmax_code_6);

    huffmanCore(inStream512_7, outStream512_7, outStream512Size_7, input_size[7], 7, strlitmtree_codes_7,
                strlitmtree_blen_7, strdistree_codes_7, strdtree_blen_7, strbitlentree_codes_7, strbitlentree_blen_7,
                strmax_code_7);
#endif

    xf::compression::details::s2mm_compress<uint32_t, GMEM_BURST_SIZE, GMEM_DWIDTH>(
        out, output_idx, outStream512_0,
#if PARALLEL_BLOCK > 1
        outStream512_1,
#endif
#if PARALLEL_BLOCK > 2
        outStream512_2, outStream512_3,
#endif
#if PARALLEL_BLOCK > 4
        outStream512_4, outStream512_5, outStream512_6, outStream512_7,
#endif
        outStream512Size_0,
#if PARALLEL_BLOCK > 1
        outStream512Size_1,
#endif
#if PARALLEL_BLOCK > 2
        outStream512Size_2, outStream512Size_3,
#endif
#if PARALLEL_BLOCK > 4
        outStream512Size_4, outStream512Size_5, outStream512Size_6, outStream512Size_7,
#endif
        output_size);
}

extern "C" {
void xilHuffmanKernel(xf::compression::uintMemWidth_t* in,
                      xf::compression::uintMemWidth_t* out,
                      uint32_t* in_block_size,
                      uint32_t* compressd_size,
                      uint32_t* dyn_litmtree_codes,
                      uint32_t* dyn_distree_codes,
                      uint32_t* dyn_bitlentree_codes,
                      uint32_t* dyn_litmtree_blen,
                      uint32_t* dyn_dtree_blen,
                      uint32_t* dyn_bitlentree_blen,
                      uint32_t* dyn_max_codes,
                      uint32_t block_size_in_kb,
                      uint32_t input_size) {
#pragma HLS INTERFACE m_axi port = in offset = slave bundle = gmem0
#pragma HLS INTERFACE m_axi port = out offset = slave bundle = gmem0
#pragma HLS INTERFACE m_axi port = compressd_size offset = slave bundle = gmem1
#pragma HLS INTERFACE m_axi port = in_block_size offset = slave bundle = gmem1

#pragma HLS INTERFACE m_axi port = dyn_litmtree_codes offset = slave bundle = gmem1
#pragma HLS INTERFACE m_axi port = dyn_distree_codes offset = slave bundle = gmem1
#pragma HLS INTERFACE m_axi port = dyn_bitlentree_codes offset = slave bundle = gmem1
#pragma HLS INTERFACE m_axi port = dyn_litmtree_blen offset = slave bundle = gmem1
#pragma HLS INTERFACE m_axi port = dyn_dtree_blen offset = slave bundle = gmem1
#pragma HLS INTERFACE m_axi port = dyn_bitlentree_blen offset = slave bundle = gmem1
#pragma HLS INTERFACE m_axi port = dyn_max_codes offset = slave bundle = gmem1

#pragma HLS INTERFACE s_axilite port = in bundle = control
#pragma HLS INTERFACE s_axilite port = out bundle = control
#pragma HLS INTERFACE s_axilite port = compressd_size bundle = control
#pragma HLS INTERFACE s_axilite port = in_block_size bundle = control
#pragma HLS INTERFACE s_axilite port = dyn_litmtree_codes bundle = control
#pragma HLS INTERFACE s_axilite port = dyn_distree_codes bundle = control
#pragma HLS INTERFACE s_axilite port = dyn_bitlentree_codes bundle = control
#pragma HLS INTERFACE s_axilite port = dyn_litmtree_blen bundle = control
#pragma HLS INTERFACE s_axilite port = dyn_dtree_blen bundle = control
#pragma HLS INTERFACE s_axilite port = dyn_bitlentree_blen bundle = control
#pragma HLS INTERFACE s_axilite port = dyn_max_codes bundle = control
#pragma HLS INTERFACE s_axilite port = block_size_in_kb bundle = control
#pragma HLS INTERFACE s_axilite port = input_size bundle = control
#pragma HLS INTERFACE s_axilite port = return bundle = control

    int block_idx = 0;
    int block_length = block_size_in_kb * 1024;
    int no_blocks = (input_size - 1) / block_length + 1;
    uint32_t max_block_size = block_size_in_kb * 1024;

    bool small_block[PARALLEL_BLOCK];
    uint32_t input_block_size[PARALLEL_BLOCK];
    uint32_t input_idx[PARALLEL_BLOCK];
    uint32_t output_idx[PARALLEL_BLOCK];
    uint32_t output_block_size[PARALLEL_BLOCK];
    uint32_t small_block_inSize[PARALLEL_BLOCK];
    uint32_t max_lit_limit[PARALLEL_BLOCK];
#pragma HLS ARRAY_PARTITION variable = input_block_size dim = 0 complete
#pragma HLS ARRAY_PARTITION variable = input_idx dim = 0 complete
#pragma HLS ARRAY_PARTITION variable = output_idx dim = 0 complete
#pragma HLS ARRAY_PARTITION variable = output_block_size dim = 0 complete
#pragma HLS ARRAY_PARTITION variable = max_lit_limit dim = 0 complete

    // Figure out total blocks and block sizes
    for (int i = 0; i < no_blocks; i += PARALLEL_BLOCK) {
        int n_blocks = PARALLEL_BLOCK;
        if ((i + PARALLEL_BLOCK) > no_blocks) n_blocks = no_blocks - i;

        for (int j = 0; j < PARALLEL_BLOCK; j++) {
            if (j < n_blocks) {
                uint32_t inBlockSize = in_block_size[i + j];

                if (inBlockSize < MIN_BLOCK_SIZE) {
                    small_block[j] = 1;
                    small_block_inSize[j] = inBlockSize;
                    input_block_size[j] = 0;
                    input_idx[j] = 0;
                } else {
                    small_block[j] = 0;
                    input_block_size[j] = inBlockSize;
                    input_idx[j] = (i + j) * max_block_size * 4;
                    output_idx[j] = (i + j) * max_block_size;
                }
            } else {
                input_block_size[j] = 0;
                input_idx[j] = 0;
            }
            output_block_size[j] = 0;
            max_lit_limit[j] = 0;
        }

        huffman(in, out, input_idx, output_idx, input_block_size, output_block_size, dyn_litmtree_codes,
                dyn_distree_codes, dyn_bitlentree_codes, dyn_litmtree_blen, dyn_dtree_blen, dyn_bitlentree_blen,
                dyn_max_codes, n_blocks);

        for (int k = 0; k < n_blocks; k++) {
            if (max_lit_limit[k]) {
                in_block_size[block_idx] = input_block_size[k];
            } else {
                in_block_size[block_idx] = output_block_size[k];
            }

            if (small_block[k] == 1) in_block_size[block_idx] = small_block_inSize[k];

            block_idx++;
        }
    } // Main loop ends here
}
}
