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
#ifndef _XFCOMPRESSION_AXI_STREAM_UTILS_HPP_
#define _XFCOMPRESSION_AXI_STREAM_UTILS_HPP_

#include "hls_stream.h"
#include "ap_axi_sdata.h"
#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <ap_int.h>

namespace xf {
namespace compression {

namespace details {

template <int OUT_DWIDTH>
void hlsStream2axiu(hls::stream<ap_uint<OUT_DWIDTH> >& inputStream,
                    hls::stream<bool>& inputStreamEos,
                    hls::stream<ap_axiu<OUT_DWIDTH, 0, 0, 0> >& outAxiStream,
                    hls::stream<ap_axiu<32, 0, 0, 0> >& outAxiSizeStream,
                    hls::stream<uint32_t>& inputTotalCmpSizeStream) {
    for (uint32_t input_size = inputTotalCmpSizeStream.read(); input_size != 0;
         input_size = inputTotalCmpSizeStream.read()) {
        bool flag;
        do {
            ap_uint<OUT_DWIDTH> temp = inputStream.read();
            flag = inputStreamEos.read();
            ap_axiu<OUT_DWIDTH, 0, 0, 0> tOut;
            tOut.data = temp;
            tOut.last = flag;
            tOut.keep = -1;
            outAxiStream << tOut;
        } while (!flag);

        ap_axiu<32, 0, 0, 0> totalSize;
        totalSize.data = input_size;
        outAxiSizeStream << totalSize;
    }
}

template <int OUT_DWIDTH>
void hlsStreamSize2axiu(hls::stream<ap_uint<OUT_DWIDTH> >& inputStream,
                        hls::stream<bool>& inputStreamEos,
                        hls::stream<ap_axiu<OUT_DWIDTH, 0, 0, 0> >& outAxiStream,
                        hls::stream<ap_axiu<32, 0, 0, 0> >& outAxiSizeStream,
                        hls::stream<uint32_t>& inputTotalCmpSizeStream,
                        hls::stream<bool>& inNumBlockStreamEos) {
    for (bool eos = inNumBlockStreamEos.read(); eos == false; eos = inNumBlockStreamEos.read()) {
        bool flag;
        do {
            ap_uint<OUT_DWIDTH> temp = inputStream.read();
            flag = inputStreamEos.read();
            ap_axiu<OUT_DWIDTH, 0, 0, 0> tOut;
            tOut.data = temp;
            tOut.last = flag;
            tOut.keep = -1;
            outAxiStream << tOut;
        } while (!flag);

        ap_axiu<32, 0, 0, 0> totalSize;
        totalSize.data = inputTotalCmpSizeStream.read();
        outAxiSizeStream << totalSize;
    }
}

template <int IN_DWIDTH>
void axiu2hlsStreamSizeEos(hls::stream<ap_axiu<IN_DWIDTH, 0, 0, 0> >& inputAxiStream,
                           hls::stream<ap_uint<IN_DWIDTH> >& outputStream,
                           hls::stream<bool>& outputStreamEos,
                           hls::stream<uint32_t>& outputSizeStream,
                           hls::stream<ap_axiu<32, 0, 0, 0> >& inSizeStream) {
    const int c_parallelByte = IN_DWIDTH / 8;
    for (ap_axiu<32, 0, 0, 0> tempSize = inSizeStream.read(); tempSize.data != 0; tempSize = inSizeStream.read()) {
        uint32_t inputSize = tempSize.data;
        outputSizeStream << inputSize;
        for (uint32_t j = 0; j < inputSize; j += c_parallelByte) {
#pragma HLS PIPELINE II = 1
            ap_axiu<IN_DWIDTH, 0, 0, 0> tempVal = inputAxiStream.read();
            ap_uint<IN_DWIDTH> tmpOut = tempVal.data;
            outputStream << tmpOut;
            outputStreamEos << false;
        }
    }
    outputSizeStream << 0;
    outputStreamEos << true;
}

template <int IN_DWIDTH>
void axiu2hlsStreamSize(hls::stream<ap_axiu<IN_DWIDTH, 0, 0, 0> >& inputAxiStream,
                        hls::stream<ap_uint<IN_DWIDTH> >& outputStream,
                        hls::stream<uint32_t>& outputSizeStream,
                        hls::stream<ap_axiu<32, 0, 0, 0> >& inSizeStream) {
    const int c_parallelByte = IN_DWIDTH / 8;
    for (ap_axiu<32, 0, 0, 0> tempSize = inSizeStream.read(); tempSize.data != 0; tempSize = inSizeStream.read()) {
        ap_uint<32> inputSize = tempSize.data;
        outputSizeStream << inputSize;
        for (uint32_t j = 0; j < inputSize; j += c_parallelByte) {
#pragma HLS PIPELINE II = 1
            ap_axiu<IN_DWIDTH, 0, 0, 0> tempVal = inputAxiStream.read();
            ap_uint<IN_DWIDTH> tmpOut = tempVal.data;
            outputStream << tmpOut;
        }
    }
    outputSizeStream << 0;
}

template <int STREAMDWIDTH = 8>
void axis2hlsStreamFixedSize(hls::stream<qdma_axis<STREAMDWIDTH, 0, 0, 0> >& inputAxiStream,
                             hls::stream<ap_uint<STREAMDWIDTH> >& inputStream,
                             uint32_t inputSize) {
    /**
     * @brief Read data from axi stream and write to internal hls stream.
     *
     * @param inputAxisStream   incoming axi stream
     * @param inputStream       output hls stream
     * @param inputSize         size of the data coming from input axi stream
     *
     */
    for (int i = 0; i < inputSize; i++) {
#pragma HLS PIPELINE II = 1
        qdma_axis<STREAMDWIDTH, 0, 0, 0> t1 = inputAxiStream.read();
        ap_uint<STREAMDWIDTH> tmpOut;
        tmpOut = t1.get_data();
        inputStream << tmpOut;
    }
}

template <int STREAMDWIDTH = 8>
void hlsStream2axis(hls::stream<ap_uint<STREAMDWIDTH> >& outputStream,
                    hls::stream<bool>& outStreamEos,
                    hls::stream<qdma_axis<STREAMDWIDTH, 0, 0, 0> >& outputAxiStream,
                    hls::stream<uint32_t>& outStreamSize,
                    hls::stream<qdma_axis<32, 0, 0, 0> >& outAxiStreamSize) {
    /**
     * @brief Read data from hls stream till the end of stream is indicated and write this data to axi stream.
     *        The total size of data is written 32-bit wide axi stream.
     *
     * @param outputStream      internal output hls stream
     * @param outStreamEos      stream to specify the end of stream
     * @param outputAxisStream  output stream going to axi
     * @param outStreamSize     size of the data coming to output from stream
     * @param outAxiStreamSize  size of the data to go through output axi stream
     *
     */
    ap_uint<STREAMDWIDTH> temp;
    bool flag;
    do {
        temp = outputStream.read();
        flag = outStreamEos.read();
        qdma_axis<STREAMDWIDTH, 0, 0, 0> tOut;
        tOut.set_data(temp);
        tOut.set_last(flag);
        tOut.set_keep(-1);
        outputAxiStream.write(tOut);
    } while (!flag);
    uint32_t outSize = outStreamSize.read();
    qdma_axis<32, 0, 0, 0> tOutSize;
    tOutSize.set_data(outSize);
    tOutSize.set_last(1);
    tOutSize.set_keep(-1);
    outAxiStreamSize.write(tOutSize);
}

template <int STREAMDWIDTH = 8>
void hlsStream2axiStreamFixedSize(hls::stream<ap_uint<STREAMDWIDTH> >& hlsInStream,
                                  hls::stream<qdma_axis<STREAMDWIDTH, 0, 0, 0> >& outputAxiStream,
                                  uint32_t originalSize) {
    /**
     * @brief Read data from internal hls stream and write to output axi stream for the given size.
     *
     * @param hlsInStream       internal hls stream
     * @param outputAxisStream  output axi stream
     * @param originalSize      output data size to be written to output stream
     *
     */
    ap_uint<STREAMDWIDTH> temp;
    for (int i = 0; i < originalSize - 1; i++) {
        temp = hlsInStream.read();
        qdma_axis<STREAMDWIDTH, 0, 0, 0> tOut;
        tOut.set_data(temp);
        tOut.set_keep(-1);
        outputAxiStream.write(tOut);
    }
    // last byte
    temp = hlsInStream.read();
    qdma_axis<STREAMDWIDTH, 0, 0, 0> tOut;
    tOut.set_data(temp);
    tOut.set_last(1);
    tOut.set_keep(-1);
    outputAxiStream.write(tOut);
}

template <uint16_t STREAMDWIDTH>
void axis2hlsStream(hls::stream<qdma_axis<STREAMDWIDTH, 0, 0, 0> >& inAxiStream,
                    hls::stream<ap_uint<STREAMDWIDTH> >& outStream) {
    /**
     * @brief Read N-bit wide data from internal hls streams
     *        and write to output axi stream. N is passed as template parameter.
     *
     * @tparam STREAMDWIDTH stream data width
     *
     * @param inAxiStream   input kernel axi stream
     * @param outStream     output hls stream
     *
     */
    while (true) {
#pragma HLS PIPELINE II = 1
        qdma_axis<STREAMDWIDTH, 0, 0, 0> t1 = inAxiStream.read();
        ap_uint<STREAMDWIDTH> tmp = t1.get_data();
        outStream << tmp;
        if (t1.get_last()) {
            break;
        }
    }
}

template <uint16_t STREAMDWIDTH>
void streamDataDm2k(hls::stream<ap_uint<STREAMDWIDTH> >& in,
                    hls::stream<ap_axiu<STREAMDWIDTH, 0, 0, 0> >& inStream_dm,
                    uint32_t inputSize) {
    /**
     * @brief Write N-bit wide data of given size from hls stream to kernel axi stream.
     *        N is passed as template parameter.
     *
     * @tparam STREAMDWIDTH stream data width
     *
     * @param in            input hls stream
     * @param inStream_dm   output kernel stream
     * @param inputSize     size of data in to be transferred
     *
     */
    // read data from input hls to input stream for decompression kernel
    uint32_t itrLim = 1 + (inputSize - 1) / (STREAMDWIDTH / 8);
    for (uint32_t i = 0; i < itrLim; i++) {
#pragma HLS PIPELINE II = 1
        ap_uint<STREAMDWIDTH> temp = in.read();
        ap_axiu<STREAMDWIDTH, 0, 0, 0> dataIn;
        dataIn.data = temp; // kernel to kernel data transfer
        inStream_dm.write(dataIn);
    }
}

template <uint16_t STREAMDWIDTH>
void streamDm2k(hls::stream<ap_uint<STREAMDWIDTH> >& in,
                uint32_t inputSize,
                hls::stream<ap_axiu<STREAMDWIDTH, 0, 0, 0> >& inStream_dm,
                hls::stream<ap_axiu<32, 0, 0, 0> >& inStreamSize_dm) {
    /**
     * @brief Write N-bit wide data of given size from hls stream to kernel axi stream.
     *        N is passed as template parameter.
     *
     * @tparam STREAMDWIDTH stream data width
     *
     * @param in            input hls stream
     * @param inStream_dm   output kernel stream
     * @param inputSize     size of data in to be transferred
     *
     */
    // read data from input hls to input stream for decompression kernel
    ap_axiu<32, 0, 0, 0> dataInSize;
    dataInSize.data = inputSize;
    dataInSize.last = true;
    inStreamSize_dm.write(dataInSize);

    uint32_t itrLim = 1 + (inputSize - 1) / (STREAMDWIDTH / 8);
    for (uint32_t i = 0; i < itrLim; i++) {
#pragma HLS PIPELINE II = 1
        ap_uint<STREAMDWIDTH> temp = in.read();
        ap_axiu<STREAMDWIDTH, 0, 0, 0> dataIn;
        dataIn.data = temp; // kernel to kernel data transfer
        inStream_dm.write(dataIn);
    }
}

template <int STREAMDWIDTH = 8>
void streamDataK2dm(hls::stream<ap_uint<STREAMDWIDTH> >& out,
                    hls::stream<bool>& bytEos,
                    hls::stream<uint32_t>& dataSize,
                    hls::stream<ap_axiu<STREAMDWIDTH, 0, 0, 0> >& dmOutStream) {
    /**
     * @brief Read data from kernel axi stream byte by byte and write to hls stream and indicate end of stream.
     *
     * @param out           output hls stream
     * @param bytEos        internal stream which indicates end of data stream
     * @param dataSize      size of data in streams
     * @param dmOutStream   input kernel axi stream to be read
     *
     */
    // read data from decompression kernel output to global memory output
    ap_axiu<STREAMDWIDTH, 0, 0, 0> dataout;
    bool last = false;
    uint32_t outSize = 0;
    do {
#pragma HLS PIPELINE II = 1
        dataout = dmOutStream.read();
        last = dataout.last;
        bytEos << last;
        out << dataout.data;
        outSize++;
    } while (!last);
    dataSize << outSize - 1; // read encoded size from decompression kernel
}

template <int STREAMDWIDTH = 8>
void streamDataK2dmFixedSize(hls::stream<ap_uint<STREAMDWIDTH> >& out,
                             hls::stream<ap_axiu<STREAMDWIDTH, 0, 0, 0> >& dmOutStream,
                             uint32_t dataSize) {
    /**
     * @brief Read data from kernel axi stream byte by byte and write to hls stream for given output size.
     *
     * @param out           output hls stream
     * @param dmOutStream   input kernel axi stream to be read
     * @param dataSize      size of data in streams
     *
     */
    // read data from decompression kernel output to global memory output
    ap_axiu<STREAMDWIDTH, 0, 0, 0> dataout;
    for (uint32_t i = 0; i < dataSize; i++) {
#pragma HLS PIPELINE II = 1
        dataout = dmOutStream.read();
        out << dataout.data;
    }
}

template <int PARALLEL_BYTES>
void streamDataK2dmMultiByteSize(hls::stream<ap_uint<PARALLEL_BYTES * 8> >& out,
                                 hls::stream<bool>& outEoS,
                                 hls::stream<uint32_t>& decompressSize,
                                 hls::stream<ap_axiu<PARALLEL_BYTES * 8, 0, 0, 0> >& dmInStream,
                                 hls::stream<ap_axiu<32, 0, 0, 0> >& dmInSizeStream) {
    /**
     * @brief Read data from kernel axi stream byte by byte and write to hls stream for given output size.
     *
     * @param out           output hls stream
     * @param dmOutStream   input kernel axi stream to be read
     * @param dataSize      size of data in streams
     *
     */
    // read data from decompression kernel output to global memory output
    ap_axiu<PARALLEL_BYTES * 8, 0, 0, 0> inValue;
    inValue = dmInStream.read();

    while (inValue.last == false) {
#pragma HLS PIPELINE II = 1
        outEoS << 0;
        out << inValue.data;

        // read nextValue
        inValue = dmInStream.read();
    }

    outEoS << 1;
    out << inValue.data;

    ap_axiu<32, 0, 0, 0> uncompressSize = dmInSizeStream.read();
    decompressSize << uncompressSize.data;
}

} // end details
} // end compression
} // end xf

#endif // _XFCOMPRESSION_AXI_STREAM_UTILS_HPP_
