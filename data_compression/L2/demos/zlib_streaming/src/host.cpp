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
#include "zlib_stream.hpp"
#include <fstream>
#include <vector>
#include "cmdlineparser.h"

void xil_decompress_top(std::string& decompress_mod, std::string& decompress_bin) {
    // Xilinx GZIP object
    xfZlibStream xlz;

    // GZIP Compression Binary Name
    std::string binaryFileName = decompress_bin;

    // Create xfZlibStream object
    xlz.init(binaryFileName);
#ifdef VERBOSE
// std::cout<<"\n";
// std::cout<<"LZ77(MBps)\tTREE(MBps)\tHUFFMAN(MBps)\tGZIP_CR\t\tFile Size(MB)\t\tFile Name"<<std::endl;
#if GZIP_FLOW
// std::cout<<"GZIP_KT\t\tFile Size(MB)\t\tFile Name"<<std::endl;
#else
// std::cout<<"ZLIB_KT\t\tFile Size(MB)\t\tFile Name"<<std::endl;
#endif
// std::cout<<"\n";
#endif

    std::ifstream inFile(decompress_mod.c_str(), std::ifstream::binary);
    if (!inFile) {
        std::cout << "Unable to open file";
        exit(1);
    }
    uint32_t input_size = get_file_size(inFile);

    std::string lz_decompress_in = decompress_mod;
    std::string lz_decompress_out = decompress_mod;
    lz_decompress_out = lz_decompress_out + ".raw";

    // Call GZIP compression
    // uint32_t enbytes =
    xlz.decompress_file(lz_decompress_in, lz_decompress_out, input_size);

#ifdef VERBOSE
    std::cout.precision(3);
// std::cout   << "\t\t" << (float) input_size/enbytes
//             << "\t\t" << (float) input_size/1000000
//             << "\t\t\t" << lz_decompress_in << std::endl;
// std::cout << "\n";
// std::cout << "Output Location: " << lz_decompress_out.c_str() << std::endl;
#endif
    xlz.release();
}

int main(int argc, char* argv[]) {
    sda::utils::CmdLineParser parser;
    parser.addSwitch("--decompress_xclbin", "-dx", "Decompress XCLBIN", "decompress");
    parser.addSwitch("--decompress", "-d", "decompress", "");
    parser.parse(argc, argv);

    std::string decompress_mod = parser.value("decompress");
    std::string decompress_bin = parser.value("decompress_xclbin");

    if (!decompress_mod.empty())
        // "-d" - DeCompress Mode
        xil_decompress_top(decompress_mod, decompress_bin);
}
