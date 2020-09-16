/*
 * Copyright 2020 Xilinx, Inc.
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
#ifndef _XFCOMPRESSION_CHECKSUM_HPP_
#define _XFCOMPRESSION_CHECKSUM_HPP_

/**
 * @file checksum_wrapper.hpp
 * @brief Header for modules used in ZLIB/GZIP compression kernel.
 *
 * This file is part of Vitis Data Compression Library.
 */

#include "hls_stream.h"
#include <ap_int.h>
#include <iostream>
#include <stdlib.h>
#include <assert.h>
#include <stdint.h>

#include "xf_security/adler32.hpp"
#include "xf_security/crc32.hpp"
#include "stream_downsizer.hpp"

namespace xf {
namespace compression {
namespace details {

template <int PARALLEL_BYTE>
void mm2s32(const ap_uint<PARALLEL_BYTE * 8>* in,
            const ap_uint<32>* inChecksumData,
            hls::stream<ap_uint<PARALLEL_BYTE * 8> >& outStream,
            hls::stream<ap_uint<32> >& outChecksumStream,
            hls::stream<ap_uint<32> >& outLenStream,
            hls::stream<bool>& outLenStreamEos,
            ap_uint<32> inputSize) {
    uint32_t inSize_gmemwidth = (inputSize - 1) / PARALLEL_BYTE + 1;
    uint32_t readSize = 0;

    outLenStream << inputSize;
    outChecksumStream << inChecksumData[0];
    outLenStreamEos << false;
    outLenStreamEos << true;

mm2s_simple:
    for (uint32_t i = 0; i < inSize_gmemwidth; i++) {
#pragma HLS PIPELINE II = 1
        ap_uint<PARALLEL_BYTE* 8> temp = in[i];
        outStream << temp;
    }
}

void s2mm32(hls::stream<ap_uint<32> >& inStream, hls::stream<bool>& inStreamEos, ap_uint<32>* outChecksumData) {
    bool eos = inStreamEos.read();
    if (!eos) outChecksumData[0] = inStream.read();
    inStreamEos.read();
}

} // End namespace details

template <int PARALLEL_BYTE>
void adler32_mm(const ap_uint<PARALLEL_BYTE * 8>* in, ap_uint<32>* adlerData, ap_uint<32> inputSize) {
#pragma HLS dataflow
    hls::stream<ap_uint<32> > inAdlerStream;
    hls::stream<ap_uint<PARALLEL_BYTE * 8> > inStream;
    hls::stream<ap_uint<32> > inLenStream;
    hls::stream<bool> inLenStreamEos;
    hls::stream<ap_uint<32> > outStream;
    hls::stream<bool> outStreamEos;

#pragma HLS STREAM variable = inStream depth = 32

    // mm2s
    details::mm2s32<PARALLEL_BYTE>(in, adlerData, inStream, inAdlerStream, inLenStream, inLenStreamEos, inputSize);

    // Adler-32
    xf::security::adler32<PARALLEL_BYTE>(inAdlerStream, inStream, inLenStream, inLenStreamEos, outStream, outStreamEos);

    // s2mm
    details::s2mm32(outStream, outStreamEos, adlerData);
}

template <int PARALLEL_BYTE>
void crc32_mm(const ap_uint<PARALLEL_BYTE * 8>* in, ap_uint<32>* crcData, ap_uint<32> inputSize) {
#pragma HLS dataflow
    hls::stream<ap_uint<32> > inCrcStream;
    hls::stream<ap_uint<PARALLEL_BYTE * 8> > inStream;
    hls::stream<ap_uint<32> > inLenStream;
    hls::stream<bool> inLenStreamEos;
    hls::stream<ap_uint<32> > outStream;
    hls::stream<bool> outStreamEos;

#pragma HLS STREAM variable = inStream depth = 32

    // mm2s
    details::mm2s32<PARALLEL_BYTE>(in, crcData, inStream, inCrcStream, inLenStream, inLenStreamEos, inputSize);

    // CRC-32
    xf::security::crc32<PARALLEL_BYTE>(inCrcStream, inStream, inLenStream, inLenStreamEos, outStream, outStreamEos);

    // s2mm
    details::s2mm32(outStream, outStreamEos, crcData);
}

template <int W>
void checksum32(hls::stream<ap_uint<32> >& checksumInitStrm,
                hls::stream<ap_uint<8 * W> >& inStrm,
                hls::stream<ap_uint<32> >& inLenStrm,
                hls::stream<bool>& endInStrm,
                hls::stream<ap_uint<32> >& outStrm,
                hls::stream<bool>& endOutStrm,
                bool checksumType) {
#pragma HLS array_partition variable = details::table dim = 1 block factor = W
    bool e = endInStrm.read();
    while (!e) {
        ap_uint<32> checksum = checksumInitStrm.read();
        ap_uint<32> len = inLenStrm.read();
        e = endInStrm.read();
        ap_uint<8 * W> inData;

        // CRC
        if (checksumType == true) {
            for (uint64_t i = 0; i < len / W; i++) {
#pragma HLS PIPELINE II = 1
#pragma HLS loop_tripcount max = 100 min = 100
                inData = inStrm.read();
                const int NUM = W / 4;
                ap_uint<32> inTmp[NUM];
                for (int j = 0; j < NUM; j++) {
                    if (j == 0)
                        inTmp[j] = inData(31 + 32 * j, 32 * j) ^ checksum;
                    else
                        inTmp[j] = inData(31 + 32 * j, 32 * j);
                }
                checksum = xf::security::internal::table[0][inTmp[W / 4 - 1](31, 24)];
                for (int j = 1; j < W; j++) {
                    int index = j / 4;
                    int offset = j % 4;
                    checksum ^=
                        xf::security::internal::table[j][inTmp[W / 4 - 1 - index](31 - offset * 8, 24 - offset * 8)];
                }
            }
            for (uint64_t i = 0; i < len - (len / W) * W; i++) {
#pragma HLS PIPELINE II = 1
#pragma HLS loop_tripcount max = W min = W
                if (i == 0) inData = inStrm.read();
                checksum =
                    (checksum >> 8) ^ xf::security::internal::table[0][checksum(7, 0) ^ inData.range(7 + 8 * i, 8 * i)];
            }
        }
        // ADLER
        else {
            ap_uint<32> s1 = checksum & 0xffff;
            ap_uint<32> s2 = ((checksum >> 16) & 0xffff);
            for (ap_uint<32> i = 0; i < len / W; i++) {
#pragma HLS PIPELINE II = 1
#pragma HLS loop_tripcount max = 100 min = 100
                inData = inStrm.read();
                ap_uint<12> sTmp[W];
#pragma HLS array_partition variable = sTmp dim = 1
                for (int i = 0; i < W; i++) {
#pragma HLS unroll
                    sTmp[i] = 0;
                    for (int j = 0; j <= i; j++) {
                        sTmp[i] += inData(j * 8 + 7, j * 8);
                    }
                }
                s2 += s1 * W;
                if (W == 16) {
                    s2 += xf::security::internal::treeAdd<12, 4>::f(sTmp);
                } else if (W == 8) {
                    s2 += xf::security::internal::treeAdd<12, 3>::f(sTmp);
                } else if (W == 4) {
                    s2 += xf::security::internal::treeAdd<12, 2>::f(sTmp);
                } else if (W == 2) {
                    s2 += xf::security::internal::treeAdd<12, 1>::f(sTmp);
                } else if (W == 1) {
                    s2 += xf::security::internal::treeAdd<12, 0>::f(sTmp);
                }

                for (int j = 0; j < W; j++) {
                    if (s2 > xf::security::internal::BASE[W - 1 - j]) {
                        s2 -= xf::security::internal::BASE[W - 1 - j];
                        break;
                    }
                }
                s1 += sTmp[W - 1];
                if (s1 > xf::security::internal::BASE[0]) s1 -= xf::security::internal::BASE[0];
            }
            for (int j = 0; j < len - (len / W) * W; j++) {
#pragma HLS PIPELINE II = 1
#pragma HLS loop_tripcount max = W min = W
                if (j == 0) inData = inStrm.read();
                s1 += inData(j * 8 + 7, j * 8);
                if (s1 > xf::security::internal::BASE[0]) s1 -= xf::security::internal::BASE[0];
                s2 += s1;
                if (s2 > xf::security::internal::BASE[0]) s2 -= xf::security::internal::BASE[0];
            }
            checksum = (s2 << 16) + s1;
        }

        outStrm.write(checksum);
        endOutStrm.write(false);
    }
    endOutStrm.write(true);
}

template <int PARALLEL_BYTE>
void checksum32_mm(const ap_uint<PARALLEL_BYTE * 8>* in,
                   ap_uint<32>* checksumData,
                   ap_uint<32> inputSize,
                   const bool checksumType) {
#pragma HLS dataflow

    hls::stream<ap_uint<PARALLEL_BYTE * 8> > inStream;
    hls::stream<ap_uint<32> > inChecksumStream;
    hls::stream<ap_uint<32> > inLenStream;
    hls::stream<ap_uint<32> > outStream;
    hls::stream<bool> inLenStreamEos;
    hls::stream<bool> outStreamEos;

#pragma HLS STREAM variable = inStream depth = 32

    // mm2s
    details::mm2s32<PARALLEL_BYTE>(in, checksumData, inStream, inChecksumStream, inLenStream, inLenStreamEos,
                                   inputSize);

    // Checksum32
    checksum32<PARALLEL_BYTE>(inChecksumStream, inStream, inLenStream, inLenStreamEos, outStream, outStreamEos,
                              checksumType);

    // s2mm
    details::s2mm32(outStream, outStreamEos, checksumData);
}

} // End Namespace compression
} // End Namespace xf

#endif // _XF_COMPRESSION_CHECKSUM_HPP_
