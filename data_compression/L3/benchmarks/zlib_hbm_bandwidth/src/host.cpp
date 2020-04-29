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
#include "zlib.hpp"
#include <fstream>
#include <vector>
#include "cmdlineparser.h"
#include <sys/wait.h>
#include <unistd.h>

constexpr auto M_COMP_UNITS = 1;
constexpr auto M_DECOMP_UNITS = 1;
constexpr auto NUM_ITER = 1;

using namespace xf::compression;

// Bandwidth measurement API
uint32_t xil_compress_bandwidth(
    const std::string& single_bin, uint8_t* in, uint8_t* out, uint32_t input_size, uint32_t cu, uint8_t max_cr) {
    uint32_t enbytes = 0;
    uint32_t num_iter = NUM_ITER;
    xfZlib xlz(single_bin, max_cr, COMP_ONLY);
    std::chrono::duration<double, std::nano> compress_API_time_ns_1(0);
    std::chrono::duration<double, std::milli> compress_API_time_ms_1(0);
    auto compress_API_start = std::chrono::high_resolution_clock::now();

    for (uint32_t i = 0; i < num_iter; i++) enbytes = xlz.compress_buffer(in, out, input_size);

    auto compress_API_end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration<double, std::nano>(compress_API_end - compress_API_start);
    auto duration_ms = std::chrono::duration<double, std::milli>(compress_API_end - compress_API_start);

    compress_API_time_ns_1 = duration / num_iter;
    compress_API_time_ms_1 = duration_ms / num_iter;

    float throughput_in_mbps_1 = (float)input_size * 1000 / compress_API_time_ns_1.count();
    std::cout << "Input Size: " << input_size / 1024 << "KB ";
    std::cout << "Compressed Size: " << enbytes / 1024 << "KB ";
    std::cout << "CU: " << cu;
    std::cout << " API: " << std::fixed << std::setprecision(2) << throughput_in_mbps_1 << "MB/s";
    std::cout << " Time: " << std::fixed << std::setprecision(2) << compress_API_time_ms_1.count() << std::endl;
    return enbytes;
}

// Bandwidth measurement API
uint32_t xil_decompress_bandwidth(
    const std::string& single_bin, uint8_t* in, uint8_t* out, uint32_t input_size, uint32_t cu, uint8_t max_cr) {
    uint32_t debytes = 0;
    uint32_t num_iter = NUM_ITER;
    xfZlib xlz(single_bin, max_cr, DECOMP_ONLY);
    std::chrono::duration<double, std::nano> decompress_API_time_ns_1(0);
    std::chrono::duration<double, std::milli> decompress_API_time_ms_1(0);
    auto decompress_API_start = std::chrono::high_resolution_clock::now();

    for (uint32_t i = 0; i < num_iter; i++) debytes = xlz.decompress(in, out, input_size, cu);

    auto decompress_API_end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration<double, std::nano>(decompress_API_end - decompress_API_start);
    auto duration_ms = std::chrono::duration<double, std::milli>(decompress_API_end - decompress_API_start);

    decompress_API_time_ns_1 = duration / num_iter;
    decompress_API_time_ms_1 = duration_ms / num_iter;
    float throughput_in_mbps_1 = (float)debytes * 1000 / decompress_API_time_ns_1.count();
    std::cout << "Input Size: " << input_size / 1024 << "KB ";
    std::cout << "Compressed Size: " << debytes / 1024 << "KB ";
    std::cout << "CU: " << cu;
    std::cout << " API: " << std::fixed << std::setprecision(2) << throughput_in_mbps_1 << "MB/s";
    std::cout << " Time: " << std::fixed << std::setprecision(2) << decompress_API_time_ms_1.count() << std::endl;
    return debytes;
}

int main(int argc, char* argv[]) {
    sda::utils::CmdLineParser parser;
    parser.addSwitch("--compress", "-c", "Compress", "");
    parser.addSwitch("--decompress", "-d", "DeCompress", "");
    parser.addSwitch("--single_xclbin", "-sx", "Single XCLBIN", "single");
    parser.addSwitch("--max_cr", "-mcr", "Maximum CR", "10");

    parser.parse(argc, argv);

    std::string compress_mod = parser.value("compress");
    std::string decompress_mod = parser.value("decompress");
    std::string single_bin = parser.value("single_xclbin");
    std::string mcr = parser.value("max_cr");

    uint8_t max_cr_val = 0;
    if (!(mcr.empty())) {
        max_cr_val = atoi(mcr.c_str());
    } else {
        // Default block size
        max_cr_val = MAX_CR;
    }

    if (!compress_mod.empty()) {
        // "-c" - Compress Mode
        std::string inFile_name = compress_mod;
        std::string outFile_name = inFile_name + ".zlib";
        std::ifstream inFile(inFile_name.c_str(), std::ifstream::binary);
        std::ofstream outFile(outFile_name.c_str(), std::ofstream::binary);
        inFile.seekg(0, inFile.end);
        uint32_t input_size = inFile.tellg();
        inFile.seekg(0, inFile.beg);

        std::vector<uint8_t, zlib_aligned_allocator<uint8_t> > in(input_size);
        std::vector<uint8_t, zlib_aligned_allocator<uint8_t> > out(input_size * 2);
        uint32_t ret_debytes[M_COMP_UNITS];

        std::cout << "\n";
        // Read input file
        inFile.read((char*)in.data(), input_size);

        for (int i = 0; i < M_COMP_UNITS; i++)
            ret_debytes[i] = xil_compress_bandwidth(single_bin, in.data(), out.data(), input_size, i, max_cr_val);

        outFile.write((char*)out.data(), ret_debytes[0]);

        inFile.close();
        outFile.close();
    } else if (!decompress_mod.empty()) {
        // "-d" - DeCompress Mode
        std::string inFile_name = decompress_mod;
        std::string outFile_name = inFile_name + ".raw";
        std::ifstream inFile(inFile_name.c_str(), std::ifstream::binary);
        std::ofstream outFile(outFile_name.c_str(), std::ofstream::binary);
        inFile.seekg(0, inFile.end);
        uint32_t input_size = inFile.tellg();
        inFile.seekg(0, inFile.beg);

        std::vector<uint8_t, zlib_aligned_allocator<uint8_t> > in[M_DECOMP_UNITS];
        std::vector<uint8_t, zlib_aligned_allocator<uint8_t> > out[M_DECOMP_UNITS];
        uint32_t ret_debytes[M_DECOMP_UNITS];

        std::cout << "\n";
        // Allocate buffers for input and output
        for (int i = 0; i < M_DECOMP_UNITS; i++) {
            in[i].resize(input_size);
            out[i].resize(input_size * 10);
        }

        // Read input file
        inFile.read((char*)in[0].data(), input_size);

        // Copy input data into multiple buffers
        for (int i = 1; i < M_DECOMP_UNITS; i++) {
            std::memcpy(in[i].data(), in[0].data(), input_size);
        }

        for (int i = 0; i < M_DECOMP_UNITS; i++)
            ret_debytes[i] =
                xil_decompress_bandwidth(single_bin, in[i].data(), out[i].data(), input_size, i, max_cr_val);

        outFile.write((char*)out[0].data(), ret_debytes[0]);

        inFile.close();
        outFile.close();
    }
}
