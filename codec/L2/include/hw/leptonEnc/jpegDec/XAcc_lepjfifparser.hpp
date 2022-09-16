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
 * @file XAcc_jfifparser.h
 * @brief parser_jpg_top template function API.
 *
 * This file is part of HLS algorithm library.
 */

#ifndef _XACC_JFIFPARSER_HPP_
#define _XACC_JFIFPARSER_HPP_

#ifndef __cplusplus
#error "XF Image Library only works with C++."
#endif

#include "XAcc_lepjpegdecoder.hpp"

namespace xf {
namespace codec {
namespace details {
// ------------------------------------------------------------
/**
 * @brief parser the jfif register for the jepg decoder
 *
 * @tparam CH_W size of data path in dataflow region, in bit.
 *         when CH_W is 16, the decoder could decode one symbol per cycle in about 99% cases.
 *         when CH_W is 8 , the decoder could decode one symbol per cycle in about 80% cases, but use less resource.
 *
 * @param datatoDDR the pointer to DDR.
 * @param size the total bytes to be read from DDR.
 * @param r the index of vector to be read from AXI in all cases
 * @param c the column to be read from AXI in the case when AXI_WIDTH > 8*sizeof(char)
 * @param dht_tbl1/dht_tbl2 the segment data of Define huffman table marker.
 * @param hls_cmp the shift register organized by the index of each color component.
 * @param hls_mbs the number of blocks in mcu for each component.
 * @param left the number of bytes to be read from DDR after parser.
 * @param image info include hls_cs_cmpc/hls_mbs/hls_mcuh/hls_mcuc is just for csim tests.
 * @param hls_compinfo image information may be used by lepton.
 * @param rtn return flag.
 * @param plep information used by lepton.
 */
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
                    decOutput* plep);

// ------------------------------------------------------------
/**
 * @brief Level 1 : decode all mcu
 *
 * @tparam CH_W size of data path in dataflow region, in bit.
 *         when CH_W is 16, the decoder could decode one symbol per cycle in about 99% cases.
 *         when CH_W is 8 , the decoder could decode one symbol per cycle in about 80% cases, but use less resource.
 *
 * @param ptr the pointer to DDR.
 * @param sz the total bytes to be read from DDR.
 * @param c the column to be read from AXI in the case when AXI_WIDTH > 8*sizeof(char)
 * @param dht_tbl1/dht_tbl2 the segment data of Define huffman table marker.
 * @param hls_cmp the shift register organized by the index of each color component.
 * @param hls_mbs the number of blocks in mcu for each component.
 * @param image info include hls_cs_cmpc/hls_mbs/hls_mcuh/hls_mcuc is just for csim tests.
 * @param block_strm the stream of coefficients in block,23:is_rst, 22:is_endblock,21~16:bpos,15~0:block val
 */
void decoder_jpg_top(ap_uint<AXI_WIDTH>* ptr,
                     const int sz,
                     const int c,
                     const uint16_t dht_tbl1[2][2][1 << DHT1],
                     const uint16_t dht_tbl2[2][2][1 << DHT2],
                     ap_uint<12> hls_cmp,
                     const uint8_t hls_mbs[MAX_NUM_COLOR],
                     const img_info img_info,

                     uint32_t& rst_cnt,
                     hls::stream<ap_uint<24> >& block_strm);

} // namespace details
} // namespace codec
} // namespace xf

namespace xf {
namespace codec {
// ------------------------------------------------------------
/**
* @brief Level 2 : kernel for jfif parser + huffman decoder
*
* @tparam CH_W size of data path in dataflow region, in bit.
*         when CH_W is 16, the decoder could decode one symbol per cycle in about 99% cases.
*         when CH_W is 8 , the decoder could decode one symbol per cycle in about 80% cases, but use less resource.
*
* @param datatoDDR the pointer to DDR.
* @param size the total bytes to be read from DDR.
* @param hls_mcuc total mcu.
* @param hls_cmpnfo the component info used by lepton.
* @param block_strm the stream of coefficients in block,23:is_rst, 22:is_endblock,21~16:bpos,15~0:block val
* @param rtn the flag of the decode succeed
*/
void kernel_parser_decoder(ap_uint<AXI_WIDTH>* datatoDDR,
                           const int size,
                           img_info& img_info,
                           hls_compInfo hls_cmpnfo[MAX_NUM_COLOR],
                           hls::stream<ap_uint<24> >& block_strm,
                           bool& rtn,
                           decOutput* plep);

} // namespace codec
} // namespace xf

#endif
