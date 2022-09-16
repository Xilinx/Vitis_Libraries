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
#include "multi_cu.hpp"
#include <iostream>
#include <iomanip>

extern "C" void lepEnc(ap_uint<AXI_WIDTH>* datainDDR, int jpgSize, int* arithInfo, ap_uint<8>* res) {
// clang-format off
        #pragma HLS INTERFACE m_axi offset = slave latency = 125 \
        num_write_outstanding = 1 num_read_outstanding = 2 \
        max_write_burst_length = 2 max_read_burst_length = 32 \
        bundle = gmem_in1 port = datainDDR

        #pragma HLS INTERFACE m_axi offset = slave latency = 125 \
        num_write_outstanding = 2 num_read_outstanding = 2 \
        max_write_burst_length = 32 max_read_burst_length = 2 \
        bundle = gmem_out1 port = res

        #pragma HLS INTERFACE m_axi offset = slave latency = 32 \
        num_write_outstanding = 2 num_read_outstanding = 2 \
        max_write_burst_length = 32 max_read_burst_length = 2 \
        bundle = gmem_out2 port = arithInfo


		#pragma HLS INTERFACE s_axilite port=datainDDR     	bundle=control
		#pragma HLS INTERFACE s_axilite port=res        	bundle=control
		#pragma HLS INTERFACE s_axilite port=jpgSize        bundle=control
		#pragma HLS INTERFACE s_axilite port=arithInfo      bundle=control

		#pragma HLS INTERFACE s_axilite port=return         bundle=control
    // clang-format on

    //    char* tmp = reinterpret_cast<char*>(datainDDR);
    //    std::cout << std::endl;
    //    std::cout << "jpgsize: " << jpgSize << std::endl;
    //    std::cout << std::hex;
    //    for (int i = 0; i < jpgSize; i++) {
    //        if (i % 8 == 0) std::cout << std::endl;
    //        std::cout << std::setfill('0') << std::setw(8) << (int)(tmp[i]) << " ";
    //    }
    //    std::cout << std::dec;
    //    std::cout << std::endl;

    xf::codec::jpegDecLeptonEnc(datainDDR, jpgSize, arithInfo, res);

    //    std::cout << std::endl;
    //    std::cout << "arith:" << std::endl;
    //    for (int i = 0; i < 9; i++) {
    //        std::cout << arithInfo[i] << std::endl;
    //    }
    //    std::cout << std::hex;
    //    std::cout << "res:" << std::endl;
    //    for (int i = 0; i < arithInfo[4]; i++) {
    //        if (i % 8 == 0) std::cout << std::endl;
    //        std::cout << std::setfill('0') << std::setw(8) << (int)res[i] << " ";
    //    }
    //    std::cout << std::dec;
    //    std::cout << std::endl;
}

//----------------------------------------------------------
/*extern "C" {
void jpegDecLeptonEncKernel_0(ap_uint<AXI_WIDTH>* datainDDR, int jpgSize, int* arithInfo, uint8_t* res) {
    // clang-format off
                uint64_t max_pix = MAX_NUM_PIX;
                #pragma HLS INTERFACE m_axi port = datainDDR  depth = max_pix offset = slave \
                bundle = gmem_in1 latency = 125  max_read_burst_length = 32
                #pragma HLS INTERFACE m_axi port = res       depth = max_pix offset = slave   \
                bundle = gmem_out1 latency = 125 max_read_burst_length = 32

                #pragma HLS INTERFACE m_axi port = arith_info depth = 7      offset=slave \
                bundle = gmem_out2

                #pragma HLS INTERFACE s_axilite port=datainDDR     	bundle=control
                #pragma HLS INTERFACE s_axilite port=res        	bundle=control
                #pragma HLS INTERFACE s_axilite port=jpgSize        bundle=control
                #pragma HLS INTERFACE s_axilite port=arith_info     bundle=control

                #pragma HLS INTERFACE s_axilite port=return         bundle=control
    // clang-format on

    xf::codec::jpegDecLeptonEnc(datainDDR, jpgSize, arithInfo, res);
}

//----------------------------------------------------------
void jpegDecLeptonEncKernel_1(ap_uint<AXI_WIDTH>* datainDDR, int jpgSize, int* arithInfo, uint8_t* res) {
    // clang-format off
                uint64_t max_pix = MAX_NUM_PIX;
                #pragma HLS INTERFACE m_axi port = datainDDR  depth = max_pix offset = slave \
                bundle = gmem_in1 latency = 125  max_read_burst_length = 32
                #pragma HLS INTERFACE m_axi port = res       depth = max_pix offset = slave   \
                bundle = gmem_out1 latency = 125 max_read_burst_length = 32

                #pragma HLS INTERFACE m_axi port = arith_info depth = 7      offset=slave \
                bundle = gmem_out2

                #pragma HLS INTERFACE s_axilite port=datainDDR     	bundle=control
                #pragma HLS INTERFACE s_axilite port=res        	bundle=control
                #pragma HLS INTERFACE s_axilite port=jpgSize        bundle=control
                #pragma HLS INTERFACE s_axilite port=arith_info     bundle=control

                #pragma HLS INTERFACE s_axilite port=return         bundle=control
    // clang-format on

    xf::codec::jpegDecLeptonEnc(datainDDR, jpgSize, arithInfo, res);
}

//----------------------------------------------------------
void jpegDecLeptonEncKernel_2(ap_uint<AXI_WIDTH>* datainDDR, int jpgSize, int* arithInfo, uint8_t* res) {
    // clang-format off
                uint64_t max_pix = MAX_NUM_PIX;
                #pragma HLS INTERFACE m_axi port = datainDDR  depth = max_pix offset = slave \
                bundle = gmem_in1 latency = 125  max_read_burst_length = 32
                #pragma HLS INTERFACE m_axi port = res       depth = max_pix offset = slave   \
                bundle = gmem_out1 latency = 125 max_read_burst_length = 32

                #pragma HLS INTERFACE m_axi port = arith_info depth = 7      offset=slave \
                bundle = gmem_out2

                #pragma HLS INTERFACE s_axilite port=datainDDR     	bundle=control
                #pragma HLS INTERFACE s_axilite port=res        	bundle=control
                #pragma HLS INTERFACE s_axilite port=jpgSize        bundle=control
                #pragma HLS INTERFACE s_axilite port=arith_info     bundle=control

                #pragma HLS INTERFACE s_axilite port=return         bundle=control
    // clang-format on

    xf::codec::jpegDecLeptonEnc(datainDDR, jpgSize, arithInfo, res);
}

//----------------------------------------------------------
void jpegDecLeptonEncKernel_3(ap_uint<AXI_WIDTH>* datainDDR, int jpgSize, int* arithInfo, uint8_t* res) {
    // clang-format off
                uint64_t max_pix = MAX_NUM_PIX;
                #pragma HLS INTERFACE m_axi port = datainDDR  depth = max_pix offset = slave \
                bundle = gmem_in1 latency = 125  max_read_burst_length = 32
                #pragma HLS INTERFACE m_axi port = res       depth = max_pix offset = slave   \
                bundle = gmem_out1 latency = 125 max_read_burst_length = 32

                #pragma HLS INTERFACE m_axi port = arith_info depth = 7      offset=slave \
                bundle = gmem_out2

                #pragma HLS INTERFACE s_axilite port=datainDDR     	bundle=control
                #pragma HLS INTERFACE s_axilite port=res        	bundle=control
                #pragma HLS INTERFACE s_axilite port=jpgSize        bundle=control
                #pragma HLS INTERFACE s_axilite port=arith_info     bundle=control

                #pragma HLS INTERFACE s_axilite port=return         bundle=control
    // clang-format on

    xf::codec::jpegDecLeptonEnc(datainDDR, jpgSize, arithInfo, res);
}

//----------------------------------------------------------
void jpegDecLeptonEncKernel_4(ap_uint<AXI_WIDTH>* datainDDR, int jpgSize, int* arithInfo, uint8_t* res) {
    // clang-format off
                uint64_t max_pix = MAX_NUM_PIX;
                #pragma HLS INTERFACE m_axi port = datainDDR  depth = max_pix offset = slave \
                bundle = gmem_in1 latency = 125  max_read_burst_length = 32
                #pragma HLS INTERFACE m_axi port = res       depth = max_pix offset = slave   \
                bundle = gmem_out1 latency = 125 max_read_burst_length = 32

                #pragma HLS INTERFACE m_axi port = arith_info depth = 7      offset=slave \
                bundle = gmem_out2

                #pragma HLS INTERFACE s_axilite port=datainDDR     	bundle=control
                #pragma HLS INTERFACE s_axilite port=res        	bundle=control
                #pragma HLS INTERFACE s_axilite port=jpgSize        bundle=control
                #pragma HLS INTERFACE s_axilite port=arith_info     bundle=control

                #pragma HLS INTERFACE s_axilite port=return         bundle=control
    // clang-format on

    xf::codec::jpegDecLeptonEnc(datainDDR, jpgSize, arithInfo, res);
}

//----------------------------------------------------------
void jpegDecLeptonEncKernel_5(ap_uint<AXI_WIDTH>* datainDDR, int jpgSize, int* arithInfo, uint8_t* res) {
    // clang-format off
                uint64_t max_pix = MAX_NUM_PIX;
                #pragma HLS INTERFACE m_axi port = datainDDR  depth = max_pix offset = slave \
                bundle = gmem_in1 latency = 125  max_read_burst_length = 32
                #pragma HLS INTERFACE m_axi port = res       depth = max_pix offset = slave   \
                bundle = gmem_out1 latency = 125 max_read_burst_length = 32

                #pragma HLS INTERFACE m_axi port = arith_info depth = 7      offset=slave \
                bundle = gmem_out2

                #pragma HLS INTERFACE s_axilite port=datainDDR     	bundle=control
                #pragma HLS INTERFACE s_axilite port=res        	bundle=control
                #pragma HLS INTERFACE s_axilite port=jpgSize        bundle=control
                #pragma HLS INTERFACE s_axilite port=arith_info     bundle=control

                #pragma HLS INTERFACE s_axilite port=return         bundle=control
    // clang-format on

    xf::codec::jpegDecLeptonEnc(datainDDR, jpgSize, arithInfo, res);
}

//----------------------------------------------------------
void jpegDecLeptonEncKernel_6(ap_uint<AXI_WIDTH>* datainDDR, int jpgSize, int* arithInfo, uint8_t* res) {
    // clang-format off
                uint64_t max_pix = MAX_NUM_PIX;
                #pragma HLS INTERFACE m_axi port = datainDDR  depth = max_pix offset = slave \
                bundle = gmem_in1 latency = 125  max_read_burst_length = 32
                #pragma HLS INTERFACE m_axi port = res       depth = max_pix offset = slave   \
                bundle = gmem_out1 latency = 125 max_read_burst_length = 32

                #pragma HLS INTERFACE m_axi port = arith_info depth = 7      offset=slave \
                bundle = gmem_out2

                #pragma HLS INTERFACE s_axilite port=datainDDR     	bundle=control
                #pragma HLS INTERFACE s_axilite port=res        	bundle=control
                #pragma HLS INTERFACE s_axilite port=jpgSize        bundle=control
                #pragma HLS INTERFACE s_axilite port=arith_info     bundle=control

                #pragma HLS INTERFACE s_axilite port=return         bundle=control
    // clang-format on

    xf::codec::jpegDecLeptonEnc(datainDDR, jpgSize, arithInfo, res);
}

//----------------------------------------------------------
void jpegDecLeptonEncKernel_7(ap_uint<AXI_WIDTH>* datainDDR, int jpgSize, int* arithInfo, uint8_t* res) {
    // clang-format off
                uint64_t max_pix = MAX_NUM_PIX;
                #pragma HLS INTERFACE m_axi port = datainDDR  depth = max_pix offset = slave \
                bundle = gmem_in1 latency = 125  max_read_burst_length = 32
                #pragma HLS INTERFACE m_axi port = res       depth = max_pix offset = slave   \
                bundle = gmem_out1 latency = 125 max_read_burst_length = 32

                #pragma HLS INTERFACE m_axi port = arith_info depth = 7      offset=slave \
                bundle = gmem_out2

                #pragma HLS INTERFACE s_axilite port=datainDDR     	bundle=control
                #pragma HLS INTERFACE s_axilite port=res        	bundle=control
                #pragma HLS INTERFACE s_axilite port=jpgSize        bundle=control
                #pragma HLS INTERFACE s_axilite port=arith_info     bundle=control

                #pragma HLS INTERFACE s_axilite port=return         bundle=control
    // clang-format on

    xf::codec::jpegDecLeptonEnc(datainDDR, jpgSize, arithInfo, res);
}

//----------------------------------------------------------
void jpegDecLeptonEncKernel_8(ap_uint<AXI_WIDTH>* datainDDR, int jpgSize, int* arithInfo, uint8_t* res) {
    // clang-format off
                uint64_t max_pix = MAX_NUM_PIX;
                #pragma HLS INTERFACE m_axi port = datainDDR  depth = max_pix offset = slave \
                bundle = gmem_in1 latency = 125  max_read_burst_length = 32
                #pragma HLS INTERFACE m_axi port = res       depth = max_pix offset = slave   \
                bundle = gmem_out1 latency = 125 max_read_burst_length = 32

                #pragma HLS INTERFACE m_axi port = arith_info depth = 7      offset=slave \
                bundle = gmem_out2

                #pragma HLS INTERFACE s_axilite port=datainDDR     	bundle=control
                #pragma HLS INTERFACE s_axilite port=res        	bundle=control
                #pragma HLS INTERFACE s_axilite port=jpgSize        bundle=control
                #pragma HLS INTERFACE s_axilite port=arith_info     bundle=control

                #pragma HLS INTERFACE s_axilite port=return         bundle=control
    // clang-format on

    xf::codec::jpegDecLeptonEnc(datainDDR, jpgSize, arithInfo, res);
}

//----------------------------------------------------------
void jpegDecLeptonEncKernel_9(ap_uint<AXI_WIDTH>* datainDDR, int jpgSize, int* arithInfo, uint8_t* res) {
    // clang-format off
                uint64_t max_pix = MAX_NUM_PIX;
                #pragma HLS INTERFACE m_axi port = datainDDR  depth = max_pix offset = slave \
                bundle = gmem_in1 latency = 125  max_read_burst_length = 32
                #pragma HLS INTERFACE m_axi port = res       depth = max_pix offset = slave   \
                bundle = gmem_out1 latency = 125 max_read_burst_length = 32

                #pragma HLS INTERFACE m_axi port = arith_info depth = 7      offset=slave \
                bundle = gmem_out2

                #pragma HLS INTERFACE s_axilite port=datainDDR     	bundle=control
                #pragma HLS INTERFACE s_axilite port=res        	bundle=control
                #pragma HLS INTERFACE s_axilite port=jpgSize        bundle=control
                #pragma HLS INTERFACE s_axilite port=arith_info     bundle=control

                #pragma HLS INTERFACE s_axilite port=return         bundle=control
    // clang-format on

    xf::codec::jpegDecLeptonEnc(datainDDR, jpgSize, arithInfo, res);
}

//----------------------------------------------------------
void jpegDecLeptonEncKernel_10(ap_uint<AXI_WIDTH>* datainDDR, int jpgSize, int* arithInfo, uint8_t* res) {
    // clang-format off
                uint64_t max_pix = MAX_NUM_PIX;
                #pragma HLS INTERFACE m_axi port = datainDDR  depth = max_pix offset = slave \
                bundle = gmem_in1 latency = 125  max_read_burst_length = 32
                #pragma HLS INTERFACE m_axi port = res       depth = max_pix offset = slave   \
                bundle = gmem_out1 latency = 125 max_read_burst_length = 32

                #pragma HLS INTERFACE m_axi port = arith_info depth = 7      offset=slave \
                bundle = gmem_out2

                #pragma HLS INTERFACE s_axilite port=datainDDR     	bundle=control
                #pragma HLS INTERFACE s_axilite port=res        	bundle=control
                #pragma HLS INTERFACE s_axilite port=jpgSize        bundle=control
                #pragma HLS INTERFACE s_axilite port=arith_info     bundle=control

                #pragma HLS INTERFACE s_axilite port=return         bundle=control
    // clang-format on

    xf::codec::jpegDecLeptonEnc(datainDDR, jpgSize, arithInfo, res);
}

//----------------------------------------------------------
void jpegDecLeptonEncKernel_11(ap_uint<AXI_WIDTH>* datainDDR, int jpgSize, int* arithInfo, uint8_t* res) {
    // clang-format off
                uint64_t max_pix = MAX_NUM_PIX;
                #pragma HLS INTERFACE m_axi port = datainDDR  depth = max_pix offset = slave \
                bundle = gmem_in1 latency = 125  max_read_burst_length = 32
                #pragma HLS INTERFACE m_axi port = res       depth = max_pix offset = slave   \
                bundle = gmem_out1 latency = 125 max_read_burst_length = 32

                #pragma HLS INTERFACE m_axi port = arith_info depth = 7      offset=slave \
                bundle = gmem_out2

                #pragma HLS INTERFACE s_axilite port=datainDDR     	bundle=control
                #pragma HLS INTERFACE s_axilite port=res        	bundle=control
                #pragma HLS INTERFACE s_axilite port=jpgSize        bundle=control
                #pragma HLS INTERFACE s_axilite port=arith_info     bundle=control

                #pragma HLS INTERFACE s_axilite port=return         bundle=control
    // clang-format on

    xf::codec::jpegDecLeptonEnc(datainDDR, jpgSize, arithInfo, res);
}

} // extern C*/
