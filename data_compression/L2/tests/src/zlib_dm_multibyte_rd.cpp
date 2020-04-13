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
 * @file zlib_dm_rd.cpp
 * @brief Source for data reader kernel for reading decompressed data from zlib decompression streaming kernel.
 *
 * This file is part of Vitis Data Compression Library.
 */

#include "zlib_dm_multibyte_rd.hpp"

const int kGMemBurstSize = 512;

template <uint16_t STREAMDWIDTH>
void streamDataK2dmSync(hls::stream<ap_uint<STREAMDWIDTH> >& out,
                        hls::stream<bool>& bytEos,
                        uint32_t readBlockSize,
                        hls::stream<uint32_t>& dataSize,
                        hls::stream<ap_axiu<STREAMDWIDTH, 0, 0, 0> >& dmOutStream,
                        hls::stream<ap_axiu<MULTIPLE_BYTES * 8, 0, 0, 0> >& sizestreamk) {
    // read data from decompression kernel output to global memory output
    ap_axiu<STREAMDWIDTH, 0, 0, 0> dataout;
    ap_axiu<MULTIPLE_BYTES * 8, 0, 0, 0> bytesIn512b;
    uint32_t outSize = 0;
    bool last = false;

    uint32_t itrLim = 1 + (readBlockSize - 1) / (STREAMDWIDTH / 8);
streamDataK2dm:
    for (uint32_t i = 0; (i < itrLim) && (!last); ++i) {
#pragma HLS PIPELINE II = 1
        bytesIn512b = sizestreamk.read();
        dataout = dmOutStream.read();
        last = dataout.last;

        bytEos << false;
        out << dataout.data;
        outSize += bytesIn512b.data;
    }
    if (last && (outSize == readBlockSize)) {
        outSize++; // write 1 extra size if last block has the size equal to readBlockSize
        bytEos << 0;
        out << 0;
    }
    bytEos << 1;
    out << 0;
    dataSize << outSize; // read encoded size from decompression kernel
}

void zlib_dm_rd(uintMemWidth_t* out,
                uint32_t* encoded_size,
                uint32_t readBlockSize,
                hls::stream<ap_axiu<MULTIPLE_BYTES * 8, 0, 0, 0> >& outAxiStream,
                hls::stream<ap_axiu<MULTIPLE_BYTES * 8, 0, 0, 0> >& sizestreamk) {
    hls::stream<uintMemWidth_t> outHlsStream("outputStream");
    hls::stream<bool> outHlsStream_eos("outputStreamSize");

#pragma HLS STREAM variable = outHlsStream depth = 512
#pragma HLS STREAM variable = outHlsStream_eos depth = 512

    hls::stream<uint32_t> outsize_val;

#pragma HLS dataflow
    streamDataK2dmSync<MULTIPLE_BYTES * 8>(outHlsStream, outHlsStream_eos, readBlockSize, outsize_val, outAxiStream,
                                           sizestreamk);

    xf::compression::details::s2mmEosSimple<MULTIPLE_BYTES * 8, kGMemBurstSize>(out, outHlsStream, outHlsStream_eos,
                                                                                outsize_val, encoded_size);
}

extern "C" {
void xilZlibDmReader(uintMemWidth_t* out,
                     uint32_t* encoded_size,
                     uint32_t read_block_size,
                     hls::stream<ap_axiu<MULTIPLE_BYTES * 8, 0, 0, 0> >& outstreamk,
                     hls::stream<ap_axiu<MULTIPLE_BYTES * 8, 0, 0, 0> >& sizestreamk) {
#pragma HLS INTERFACE m_axi port = out offset = slave bundle = gmem0 max_read_burst_length = \
    2 max_write_burst_length = 128
#pragma HLS INTERFACE m_axi port = encoded_size offset = slave bundle = gmem0
#pragma HLS interface axis port = outstreamk
#pragma HLS interface axis port = sizestreamk
#pragma HLS INTERFACE s_axilite port = out bundle = control
#pragma HLS INTERFACE s_axilite port = encoded_size bundle = control
#pragma HLS INTERFACE s_axilite port = read_block_size bundle = control
#pragma HLS INTERFACE s_axilite port = return bundle = control

    ////printme("In datamover kernel \n");
    zlib_dm_rd(out, encoded_size, read_block_size, outstreamk, sizestreamk);
}
}
