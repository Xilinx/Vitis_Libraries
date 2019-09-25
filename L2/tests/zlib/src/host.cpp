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
#include "xil_zlib.hpp"
#include <fstream>
#include <vector>
#include "cmdlineparser.h"
#include "zlib_config.h"

void xilValidate(std::string& file_list, std::string& ext) {
    std::cout << "\n";
    std::cout << "Status\t\tFile Name" << std::endl;
    std::cout << "\n";

    std::ifstream infilelist_val(file_list.c_str());
    std::string line_val;

    while (std::getline(infilelist_val, line_val)) {
        std::string line_in = line_val;
        std::string line_out = line_in + ext;

        int ret = 0;
        // Validate input and output files
        ret = validate(line_in, line_out);
        if (ret == 0) {
            std::cout << (ret ? "FAILED\t" : "PASSED\t") << "\t" << line_in << std::endl;
        } else {
            std::cout << "Validation Failed" << line_out.c_str() << std::endl;
            exit(1);
        }
    }
}

void xilCompressDecompressList(std::string& file_list,
                               std::string& ext1,
                               std::string& ext2,
                               std::string& compress_bin,
                               std::string& decompress_bin,
                               std::string& single_bin) {
    // GZIP/Zlib Compression Binary Name
    std::string binaryFileName;
    if (SINGLE_XCLBIN)
        binaryFileName = single_bin;
    else
        binaryFileName = compress_bin;
    // binaryFileName = "xil_zlib_compress_huffman_" + std::to_string(PARALLEL_BLOCK) + "b";

    // Create xil_zlib object
    xil_zlib xlz;
    xlz.m_bin_flow = 0;
    xlz.init(binaryFileName);

    std::cout << "--------------------------------------------------------------" << std::endl;
#ifdef GZIP_FLOW
    std::cout << "                     Xilinx GZip Compress                          " << std::endl;
#else
    std::cout << "                     Xilinx Zlib Compress                          " << std::endl;
#endif
    std::cout << "--------------------------------------------------------------" << std::endl;

    std::cout << "\n";
    std::cout << "E2E(MBps)\tKT(MBps)\tCR\t\tFile Size(MB)\t\tFile Name" << std::endl;
    std::cout << "\n";

    std::ifstream infilelist(file_list.c_str());
    std::string line;

    // Compress list of files
    // This loop does LZ4 compression on list
    // of files.
    while (std::getline(infilelist, line)) {
        std::ifstream inFile(line.c_str(), std::ifstream::binary);
        if (!inFile) {
            std::cout << "Unable to open file";
            exit(1);
        }

        uint64_t input_size = get_file_size(inFile);
        inFile.close();

        std::string compress_in = line;
        std::string compress_out = line;
        compress_out = compress_out + ext1;

        // Call GZip/Zlib compression
        uint64_t enbytes = xlz.compress_file(compress_in, compress_out, input_size);

        std::cout << "\t\t" << (double)input_size / enbytes << "\t\t" << (double)input_size / 1000000 << "\t\t\t"
                  << compress_in << std::endl;
    }
    if (!SINGLE_XCLBIN) {
        xlz.release();
    }
    // Decompress
    std::string binaryFileName_decompress;
    if (!SINGLE_XCLBIN) binaryFileName_decompress = decompress_bin;
    if (!SINGLE_XCLBIN) {
        xlz.m_bin_flow = 1;
        xlz.init(binaryFileName_decompress);
    }
    std::ifstream infilelist_dec(file_list.c_str());
    std::string line_dec;

    std::cout << "\n";
    std::cout << "--------------------------------------------------------------" << std::endl;
#ifdef GZIP_FLOW
    std::cout << "                     Xilinx GZip DeCompress                       " << std::endl;
#else
    std::cout << "                     Xilinx Zlib DeCompress                       " << std::endl;
#endif
    std::cout << "--------------------------------------------------------------" << std::endl;
    std::cout << "\n";
    std::cout << "KT(MBps)\tFile Size(MB)\t\tFile Name" << std::endl;
    std::cout << "\n";

    // Decompress list of files
    while (std::getline(infilelist_dec, line_dec)) {
        std::string file_line = line_dec;
        file_line = file_line + ext2;

        std::ifstream inFile_dec(file_line.c_str(), std::ifstream::binary);
        if (!inFile_dec) {
            std::cout << "Unable to open file";
            exit(1);
        }

        uint64_t input_size = get_file_size(inFile_dec);
        inFile_dec.close();

        std::string decompress_in = file_line;
        std::string decompress_out = file_line;
        decompress_out = decompress_out + ".orig";

        // Call GZip/Zlib decompression
        xlz.decompress_file(decompress_in, decompress_out, input_size);

        std::cout << "\t\t" << (double)input_size / 1000000 << "\t\t" << decompress_in << std::endl;
    } // While loop ends

    xlz.release();
}

void xilBatchVerify(std::string& file_list,
                    std::string& compress_bin,
                    std::string& decompress_bin,
                    std::string& single_bin) {
    std::string ext1;
    std::string ext2;

#ifdef GZIP_FLOW
    // Xilinx GZip Compression
    ext1 = ".xe2xd.gz";
    ext2 = ".xe2xd.gz";

    xilCompressDecompressList(file_list, ext1, ext2, compress_bin, decompress_bin, single_bin);

    // Validate
    std::cout << "\n";
    std::cout << "----------------------------------------------------------------------------------------"
              << std::endl;
    std::cout << "                       Validate: Xilinx GZip Compress vs Xilinx GZip Decompress           "
              << std::endl;
    std::cout << "----------------------------------------------------------------------------------------"
              << std::endl;
    std::string ext3 = ".xe2xd.gz.orig";
    xilValidate(file_list, ext3);

#else
    // Xilinx GZip Compression
    ext1 = ".xe2xd.zlib";
    ext2 = ".xe2xd.zlib";

    xilCompressDecompressList(file_list, ext1, ext2, compress_bin, decompress_bin, single_bin);

    // Validate
    std::cout << "\n";
    std::cout << "----------------------------------------------------------------------------------------"
              << std::endl;
    std::cout << "                       Validate: Xilinx Zlib Compress vs Xilinx Zlib Decompress           "
              << std::endl;
    std::cout << "----------------------------------------------------------------------------------------"
              << std::endl;
    std::string ext3 = ".xe2xd.zlib.orig";
    xilValidate(file_list, ext3);

#endif
}

void xilDecompressTop(std::string& decompress_mod, std::string& decompress_bin, std::string& single_bin) {
    // Xilinx GZIP object
    xil_zlib xlz;

    // GZIP Compression Binary Name
    std::string binaryFileName;
    if (SINGLE_XCLBIN)
        binaryFileName = single_bin;
    else
        binaryFileName = decompress_bin;
    // binaryFileName = "xil_zlib_compress_huffman_" + std::to_string(PARALLEL_BLOCK) + "b";

    // Create xil_zlib object
    xlz.init(binaryFileName);
#ifdef VERBOSE
// std::cout<<"\n";
// std::cout<<"LZ77(MBps)\tTREE(MBps)\tHUFFMAN(MBps)\tGZIP_CR\t\tFile Size(MB)\t\tFile Name"<<std::endl;
#if GZIP_FLOW
    std::cout << "GZIP_KT\t\tFile Size(MB)\t\tFile Name" << std::endl;
#else
    std::cout << "ZLIB_KT\t\tFile Size(MB)\t\tFile Name" << std::endl;
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
    uint32_t enbytes = xlz.decompress_file(lz_decompress_in, lz_decompress_out, input_size);

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

void xilCompressTop(std::string& compress_mod, std::string& compress_bin, std::string& single_bin) {
    // Xilinx GZIP object
    xil_zlib xlz;

    // GZIP Compression Binary Name
    std::string binaryFileName;
    if (SINGLE_XCLBIN)
        binaryFileName = single_bin;
    else
        binaryFileName = compress_bin;
    // binaryFileName = "xil_zlib_compress_huffman_" + std::to_string(PARALLEL_BLOCK) + "b";

    // Create xil_zlib object
    xlz.init(binaryFileName);
#ifdef VERBOSE
    std::cout << "\n";
    // std::cout<<"LZ77(MBps)\tTREE(MBps)\tHUFFMAN(MBps)\tGZIP_CR\t\tFile Size(MB)\t\tFile Name"<<std::endl;
    std::cout << "E2E\t\tKT\t\tGZIP_CR\t\tFile Size(MB)\t\tFile Name" << std::endl;
    std::cout << "\n";
#endif

    std::ifstream inFile(compress_mod.c_str(), std::ifstream::binary);
    if (!inFile) {
        std::cout << "Unable to open file";
        exit(1);
    }
    uint32_t input_size = get_file_size(inFile);

    std::string lz_compress_in = compress_mod;
    std::string lz_compress_out = compress_mod;
    lz_compress_out = lz_compress_out + ".gz";

    // Call GZIP compression
    uint32_t enbytes = xlz.compress_file(lz_compress_in, lz_compress_out, input_size);

#ifdef VERBOSE
    std::cout.precision(3);
    std::cout << "\t\t" << (float)input_size / enbytes << "\t\t" << (float)input_size / 1000000 << "\t\t\t"
              << lz_compress_in << std::endl;
    std::cout << "\n";
    std::cout << "Output Location: " << lz_compress_out.c_str() << std::endl;
#endif
    xlz.release();
}

int main(int argc, char* argv[]) {
    sda::utils::CmdLineParser parser;
    parser.addSwitch("--compress", "-c", "Compress", "");
    parser.addSwitch("--decompress", "-d", "DeCompress", "");
    parser.addSwitch("--compress_xclbin", "-cx", "Compress XCLBIN", "compress");
    parser.addSwitch("--decompress_xclbin", "-dx", "Decompress XCLBIN", "decompress");
    parser.addSwitch("--single_xclbin", "-sx", "Single XCLBIN", "single");
    parser.addSwitch("--file_list", "-l", "List of Input Files", "");
    parser.parse(argc, argv);

    std::string single_bin = parser.value("single_xclbin");
    std::string compress_bin = parser.value("compress_xclbin");
    std::string decompress_bin = parser.value("decompress_xclbin");
    std::string compress_mod = parser.value("compress");
    std::string filelist = parser.value("file_list");
    std::string decompress_mod = parser.value("decompress");

    // "-c" - Compress Mode
    if (!compress_mod.empty()) xilCompressTop(compress_mod, compress_bin, single_bin);

    // "-d" - DeCompress Mode
    if (!decompress_mod.empty()) xilDecompressTop(decompress_mod, decompress_bin, single_bin);

    // "-l" - List of files
    if (!filelist.empty()) {
        xilBatchVerify(filelist, compress_bin, decompress_bin, single_bin);
    }
}
