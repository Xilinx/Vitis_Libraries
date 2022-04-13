/*
 * (c) Copyright 2019-2021 Xilinx, Inc. All rights reserved.
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
#include "xcl2.hpp"
#include <algorithm>
#include <vector>
#include <time.h>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <cassert>

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cout << "Usage: " << argv[0] << " <XCLBIN File> <INPUT File>" << std::endl;
        return EXIT_FAILURE;
    }

    // Read input file
    std::ifstream ifs;
    std::string binaryFile = argv[1];

    std::string inFile_name = argv[2];
    ifs.open(argv[2], std::ofstream::binary | std::ofstream::in);
    if (!ifs.is_open()) {
        std::cout << "Cannot open the input file!!" << std::endl;
        exit(0);
    }

    ifs.seekg(0, std::ios::end);
    uint32_t size = ifs.tellg();
    ifs.seekg(0, std::ios::beg);

    auto blockSize = BLOCK_SIZE_IN_KB * 1024; // can be changed for custom testing of block sizes upto 4KB.
    auto blckNum = (size - 1) / blockSize + 1;
    auto output_size = 10 * blckNum * BLOCK_SIZE_IN_KB * 1024;
    auto num_itr = xcl::is_emulation() ? 4 : 400000;

    std::vector<uint8_t, aligned_allocator<uint8_t> > h_input_buffer(size);
    std::vector<uint8_t, aligned_allocator<uint8_t> > h_output_buffer(output_size);
    std::vector<uint32_t, aligned_allocator<uint32_t> > h_compressSize(2 * blckNum);
    ifs.read(reinterpret_cast<char*>(h_input_buffer.data()), size);

    // OPENCL HOST CODE AREA START
    cl_int err;
    cl::Context context;
    cl::Kernel krnl_dm;
    cl::CommandQueue q;
    std::chrono::duration<double, std::nano> kernel_time_ns_1(0);

    // get_xil_devices() is a utility API which will find the xilinx
    // platforms and will return list of devices connected to Xilinx platform
    auto devices = xcl::get_xil_devices();
    // read_binary_file() is a utility API which will load the binaryFile
    // and will return the pointer to file buffer.
    auto fileBuf = xcl::read_binary_file(binaryFile);
    cl::Program::Binaries bins{{fileBuf.data(), fileBuf.size()}};
    bool valid_device = false;
    for (unsigned int i = 0; i < devices.size(); i++) {
        auto device = devices[i];
        // Creating Context and Command Queue for selected Device
        OCL_CHECK(err, context = cl::Context(device, nullptr, nullptr, nullptr, &err));
        OCL_CHECK(err, q = cl::CommandQueue(context, device, CL_QUEUE_PROFILING_ENABLE, &err));
        std::cout << "Trying to program device[" << i << "]: " << device.getInfo<CL_DEVICE_NAME>() << std::endl;
        cl::Program program(context, {device}, bins, nullptr, &err);
        if (err != CL_SUCCESS) {
            std::cout << "Failed to program device[" << i << "] with xclbin file!\n";
        } else {
            std::cout << "Device[" << i << "]: program successful!\n";
            OCL_CHECK(err, krnl_dm = cl::Kernel(program, "xilDataMover", &err));
            valid_device = true;
            break; // we break because we found a valid device
        }
    }
    if (!valid_device) {
        std::cout << "Failed to program any device found, exit!\n";
        exit(EXIT_FAILURE);
    }

    // Device buffers
    OCL_CHECK(err, cl::Buffer buffer_input(context, CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR, sizeof(uint8_t) * size,
                                           h_input_buffer.data(), &err));
    OCL_CHECK(err, cl::Buffer buffer_output(context, CL_MEM_READ_WRITE | CL_MEM_USE_HOST_PTR,
                                            sizeof(uint8_t) * output_size, h_output_buffer.data(), &err));
    OCL_CHECK(err, cl::Buffer buffer_cSize(context, CL_MEM_READ_WRITE | CL_MEM_USE_HOST_PTR,
                                           sizeof(uint32_t) * blckNum * 2, h_compressSize.data(), &err));
    auto narg = 0;
    krnl_dm.setArg(narg++, buffer_input);
    krnl_dm.setArg(narg++, buffer_output);
    krnl_dm.setArg(narg++, size);
    krnl_dm.setArg(narg++, num_itr);
    krnl_dm.setArg(narg++, blockSize);
    krnl_dm.setArg(narg++, buffer_cSize);

    // Copy input data to device global memory
    OCL_CHECK(err,
              err = q.enqueueMigrateMemObjects({buffer_input, buffer_output, buffer_cSize}, 0 /* 0 means from host*/));

    // Launch the Kernel
    // For HLS kernels global and local size is always (1,1,1). So, it is
    // recommended
    // to always use enqueueTask() for invoking HLS kernel
    auto kernel_start = std::chrono::high_resolution_clock::now();
    OCL_CHECK(err, err = q.enqueueTask(krnl_dm));
    q.finish();
    auto kernel_stop = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration<double, std::nano>(kernel_stop - kernel_start);
    kernel_time_ns_1 += duration;

    // Copy Result from Device Global Memory to Host Local Memory
    OCL_CHECK(err, err = q.enqueueMigrateMemObjects({buffer_output, buffer_cSize}, CL_MIGRATE_MEM_OBJECT_HOST));
    q.finish();
    std::string outFile_name = inFile_name + ".orig";
    std::ofstream outFile(outFile_name.c_str(), std::ofstream::binary);
    uint32_t uncompressedSize = h_compressSize[0];
    outFile.write((char*)h_output_buffer.data(), uncompressedSize);

    // OPENCL HOST CODE AREA END
    std::cout << "**********************************************************************\n";
    std::cout << "\t\t\tXilinx Decompress\n";
    std::cout << "**********************************************************************\n";
    std::cout << "File Name \t\t:" << argv[2] << "\n";
    float throughput_in_mbps_1 = (float)blockSize * 1000 * num_itr / kernel_time_ns_1.count();
    std::cout << "Throughput \t\t:" << std::fixed << std::setprecision(2) << throughput_in_mbps_1 << "MB/s\n";
    std::cout << "**********************************************************************\n";
}
