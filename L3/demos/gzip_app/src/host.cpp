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
#include "zlib_old.hpp"
#include <fstream>
#include <vector>
#include "cmdlineparser.h"

using namespace xf::compression;

void xil_validate(const std::string& file_list, const std::string& ext);

void xil_compress_decompress_list(const std::string& file_list,
                                  const std::string& ext1,
                                  const std::string& ext2,
                                  int cu,
                                  const std::string& single_bin,
                                  uint8_t max_cr,
                                  list_mode mode = COMP_DECOMP,
                                  uint8_t device_id = 0,
                                  design_flow dflow = XILINX_GZIP) {
    // Create xfZlib object
    xfZlib xlz(single_bin, max_cr, BOTH, device_id, 0, FULL, dflow);
    ERROR_STATUS(xlz.error_code());

    if (mode != ONLY_DECOMPRESS) {
        std::cout << "--------------------------------------------------------------" << std::endl;
        if (dflow)
            std::cout << "                     Xilinx Zlib Compress" << std::endl;
        else
            std::cout << "                     Xilinx GZip Compress" << std::endl;
        std::cout << "--------------------------------------------------------------" << std::endl;

        std::ifstream infilelist(file_list);
        std::string line;

        // Compress list of files
        // This loop does LZ4 compression on list
        // of files.
        while (std::getline(infilelist, line)) {
            std::ifstream inFile(line, std::ifstream::binary);
            if (!inFile) {
                std::cerr << "Unable to open file";
                exit(1);
            }

            uint64_t input_size = get_file_size(inFile);
            inFile.close();

            std::string compress_in = line;
            std::string compress_out = line;
            compress_out = compress_out + ext1;
            std::cout << "\n";
            std::cout << "E2E(MBps)\tCR\t\tFile Size(MB)\t\tFile Name" << std::endl;
            std::cout << "\n";

            // Call Zlib compression
            uint64_t enbytes = xlz.compress_file(compress_in, compress_out, input_size);

            std::cout << "\t\t" << (double)input_size / enbytes << "\t\t" << std::fixed << std::setprecision(3)
                      << (double)input_size / 1000000 << "\t\t\t" << compress_in << std::endl;
        }
    }

    // Decompress
    if (mode != ONLY_COMPRESS) {
        std::ifstream infilelist_dec(file_list);
        std::string line_dec;

        std::cout << "\n";
        std::cout << "--------------------------------------------------------------" << std::endl;
        if (dflow)
            std::cout << "                     Xilinx Zlib DeCompress" << std::endl;
        else
            std::cout << "                     Xilinx GZip DeCompress" << std::endl;
        std::cout << "--------------------------------------------------------------" << std::endl;

        // Decompress list of files
        while (std::getline(infilelist_dec, line_dec)) {
            std::string file_line = line_dec;
            file_line = file_line + ext2;

            std::ifstream inFile_dec(file_line, std::ifstream::binary);
            if (!inFile_dec) {
                std::cerr << "Unable to open file";
                exit(1);
            }

            uint64_t input_size = get_file_size(inFile_dec);
            inFile_dec.close();

            std::string decompress_in = file_line;
            std::string decompress_out = file_line;
            decompress_out = decompress_out + ".orig";

            std::cout << "\n";
            std::cout << "E2E(MBps)\tFile Size(MB)\t\tFile Name" << std::endl;
            std::cout << "\n";

            // Call Zlib decompression
            xlz.decompress_file(decompress_in, decompress_out, input_size, cu);

            std::cout << std::fixed << std::setprecision(3) << "\t\t" << (double)input_size / 1000000 << "\t\t"
                      << decompress_in << std::endl;
        } // While loop ends
    }
}

void xil_batch_verify(const std::string& file_list,
                      int cu,
                      list_mode mode,
                      const std::string& single_bin,
                      uint8_t device_id,
                      uint8_t max_cr,
                      design_flow dflow) {
    // Xilinx ZLIB Compression
    std::string ext1, ext2, ext3;

    if (dflow) {
        ext1 = ".xe2xd.xz";
        ext2 = ".xe2xd.xz";
        ext3 = ".xe2xd.xz.orig";
    } else {
        ext1 = ".xe2xd.gz";
        ext2 = ".xe2xd.gz";
        ext3 = ".xe2xd.gz.orig";
    }

    xil_compress_decompress_list(file_list, ext1, ext2, cu, single_bin, max_cr, mode, device_id, dflow);

    // Validate
    std::cout << "\n";
    std::cout << "----------------------------------------------------------------------------------------"
              << std::endl;
    if (dflow)
        std::cout << "                       Validate: Xilinx Zlib Compress vs Xilinx Zlib Decompress " << std::endl;
    else
        std::cout << "                       Validate: Xilinx GZip Compress vs Xilinx GZip Decompress " << std::endl;

    std::cout << "----------------------------------------------------------------------------------------"
              << std::endl;

    xil_validate(file_list, ext3);
}

void xil_decompress_top(
    const std::string& decompress_mod, int cu, const std::string& single_bin, uint8_t device_id, uint8_t max_cr) {
    // Xilinx ZLIB object
    xfZlib xlz(single_bin, max_cr, DECOMP_ONLY, device_id, 0, FULL);
    ERROR_STATUS(xlz.error_code());

    std::ifstream inFile(decompress_mod, std::ifstream::binary);
    if (!inFile) {
        std::cerr << "Unable to open file";
        exit(1);
    }
    uint64_t input_size = get_file_size(inFile);

    std::string lz_decompress_in = decompress_mod;
    std::string lz_decompress_out = decompress_mod;
    lz_decompress_out = lz_decompress_out + ".raw";

    std::cout << std::fixed << std::setprecision(2) << "E2E(MBps)\t\t:";

    // Call ZLIB compression
    // uint32_t enbytes =
    xlz.decompress_file(lz_decompress_in, lz_decompress_out, input_size, cu);
    std::cout << std::fixed << std::setprecision(3) << std::endl
              << "File Size(MB)\t\t:" << (double)input_size / 1000000 << std::endl
              << "File Name\t\t:" << lz_decompress_in << std::endl;
}

void xil_compress_top(const std::string& compress_mod,
                      const std::string& single_bin,
                      uint8_t device_id,
                      uint8_t max_cr,
                      design_flow dflow) {
    // Xilinx ZLIB object
    xfZlib xlz(single_bin, max_cr, COMP_ONLY, device_id, 0, FULL, dflow);
    ERROR_STATUS(xlz.error_code());

    std::ifstream inFile(compress_mod, std::ifstream::binary);
    if (!inFile) {
        std::cerr << "Unable to open file";
        exit(1);
    }
    uint64_t input_size = get_file_size(inFile);

    std::string lz_compress_in = compress_mod;
    std::string lz_compress_out = compress_mod;
    if (dflow)
        lz_compress_out = lz_compress_out + ".xz";
    else
        lz_compress_out = lz_compress_out + ".gz";

    std::cout << std::fixed << std::setprecision(2) << "E2E(MBps)\t\t:";

    // Call ZLIB compression
    uint64_t enbytes = xlz.compress_file(lz_compress_in, lz_compress_out, input_size);

    std::cout.precision(3);
    std::cout << std::fixed << std::setprecision(2) << std::endl
              << "ZLIB_CR\t\t\t:" << (double)input_size / enbytes << std::endl
              << std::fixed << std::setprecision(3) << "File Size(MB)\t\t:" << (double)input_size / 1000000 << std::endl
              << "File Name\t\t:" << lz_compress_in << std::endl;
    std::cout << "\n";
    std::cout << "Output Location: " << lz_compress_out << std::endl;
}

void xil_validate(const std::string& file_list, const std::string& ext) {
    std::cout << "\n";
    std::cout << "Status\t\tFile Name" << std::endl;
    std::cout << "\n";

    std::ifstream infilelist_val(file_list);
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
            std::cerr << "Validation Failed" << line_out << std::endl;
            exit(1);
        }
    }
}

void xilCompressDecompressTop(const std::string& compress_decompress_mod,
                              const std::string& single_bin,
                              uint8_t device_id,
                              uint8_t max_cr_val,
                              design_flow dflow) {
    // Create xfZlib object
    xfZlib xlz(single_bin, max_cr_val, BOTH, device_id, 0, FULL, dflow);
    ERROR_STATUS(xlz.error_code());

    std::cout << "--------------------------------------------------------------" << std::endl;
    if (dflow)
        std::cout << "                     Xilinx Zlib Compress" << std::endl;
    else
        std::cout << "                     Xilinx GZip Compress" << std::endl;
    std::cout << "--------------------------------------------------------------" << std::endl;
    std::cout << "\n";

    std::ifstream inFile(compress_decompress_mod, std::ifstream::binary);
    if (!inFile) {
        std::cerr << "Unable to open file";
        exit(1);
    }

    uint64_t input_size = get_file_size(inFile);
    inFile.close();

    std::string compress_in = compress_decompress_mod;
    std::string compress_out = compress_decompress_mod;
    if (dflow)
        compress_out = compress_out + ".xz";
    else
        compress_out = compress_out + ".gz";

    std::cout << std::fixed << std::setprecision(2) << "E2E(MBps)\t\t:";

    // Call Zlib compression
    uint64_t enbytes = xlz.compress_file(compress_in, compress_out, input_size);
    std::cout << std::fixed << std::setprecision(2) << std::endl
              << "CR\t\t\t:" << (double)input_size / enbytes << std::endl
              << std::fixed << std::setprecision(3) << "File Size(MB)\t\t:" << (double)input_size / 1000000 << std::endl
              << "File Name\t\t:" << compress_in << std::endl;

    // Decompress

    std::cout << "\n";
    std::cout << "--------------------------------------------------------------" << std::endl;
    if (dflow)
        std::cout << "                     Xilinx Zlib DeCompress" << std::endl;
    else
        std::cout << "                     Xilinx GZip DeCompress" << std::endl;
    std::cout << "--------------------------------------------------------------" << std::endl;
    std::cout << "\n";

    // Decompress list of files
    std::string lz_decompress_in, lz_decompress_out;
    if (dflow)
        lz_decompress_in = compress_decompress_mod + ".xz";
    else
        lz_decompress_in = compress_decompress_mod + ".gz";
    lz_decompress_out = lz_decompress_in + ".orig";

    std::cout << std::fixed << std::setprecision(2) << "E2E(MBps)\t\t:";

    std::ifstream inFile_dec(lz_decompress_in, std::ifstream::binary);
    if (!inFile_dec) {
        std::cerr << "Unable to open file" << std::endl;
        exit(1);
    }

    input_size = get_file_size(inFile_dec);
    inFile_dec.close();

    // Call Zlib decompression
    xlz.decompress_file(lz_decompress_in, lz_decompress_out, input_size, 0);

    std::cout << std::fixed << std::setprecision(2) << std::endl
              << std::fixed << std::setprecision(3) << "File Size(MB)\t\t:" << (double)input_size / 1000000 << std::endl
              << "File Name\t\t:" << lz_decompress_in << std::endl;

    // Validate
    std::cout << "\n";

    std::string outputFile = compress_decompress_mod;
    std::string inputFile = compress_decompress_mod;
    if (dflow)
        outputFile = outputFile + ".xz" + ".orig";
    else
        outputFile = outputFile + ".gz" + ".orig";

    int ret = validate(inputFile, outputFile);
    if (ret == 0) {
        std::cout << (ret ? "FAILED\t" : "PASSED\t") << "\t" << inputFile << std::endl;
    } else {
        std::cout << "Validation Failed" << outputFile << std::endl;
    }
}

int main(int argc, char* argv[]) {
    int cu_run;
    sda::utils::CmdLineParser parser;
    parser.addSwitch("--compress", "-c", "Compress", "");
    parser.addSwitch("--decompress", "-d", "DeCompress", "");
    parser.addSwitch("--single_xclbin", "-sx", "Single XCLBIN", "single");
    parser.addSwitch("--compress_decompress", "-v", "Compress Decompress", "");
    parser.addSwitch("--zlib", "-zlib", "[0:GZIP, 1:ZLIB]", "0");

    parser.addSwitch("--file_list", "-l", "List of Input Files", "");
    parser.addSwitch("--cu", "-k", "CU", "0");
    parser.addSwitch("--id", "-id", "Device ID", "0");
    parser.addSwitch("--max_cr", "-mcr", "Maximum CR", "10");
    parser.parse(argc, argv);

    std::string compress_mod = parser.value("compress");
    std::string filelist = parser.value("file_list");
    std::string decompress_mod = parser.value("decompress");
    std::string single_bin = parser.value("single_xclbin");
    std::string compress_decompress_mod = parser.value("compress_decompress");
    std::string cu = parser.value("cu");
    std::string device_ids = parser.value("id");
    std::string is_Zlib = parser.value("zlib");
    std::string mcr = parser.value("max_cr");

    uint8_t max_cr_val = 0;
    if (!mcr.empty()) {
        max_cr_val = std::stoi(mcr);
    } else {
        // Default block size
        max_cr_val = MAX_CR;
    }

    uint8_t device_id = 0;
    design_flow dflow = std::stoi(is_Zlib) ? XILINX_ZLIB : XILINX_GZIP;

    if (!device_ids.empty()) device_id = std::stoi(device_ids);

    if (cu.empty()) {
        std::cerr << "please provide -k option for cu" << std::endl;
        exit(0);
    } else {
        cu_run = std::stoi(cu);
    }

    if (!compress_decompress_mod.empty())
        xilCompressDecompressTop(compress_decompress_mod, single_bin, device_id, max_cr_val, dflow);

    if (!filelist.empty()) {
        list_mode lMode;
        // "-l" - List of files
        if (!compress_mod.empty()) {
            lMode = ONLY_COMPRESS;
        } else if (!decompress_mod.empty()) {
            lMode = ONLY_DECOMPRESS;
        } else {
            lMode = COMP_DECOMP;
        }
        xil_batch_verify(filelist, cu_run, lMode, single_bin, device_id, max_cr_val, dflow);
    } else if (!compress_mod.empty()) {
        // "-c" - Compress Mode
        xil_compress_top(compress_mod, single_bin, device_id, max_cr_val, dflow);
    } else if (!decompress_mod.empty())
        // "-d" - DeCompress Mode
        xil_decompress_top(decompress_mod, cu_run, single_bin, device_id, max_cr_val);
}
