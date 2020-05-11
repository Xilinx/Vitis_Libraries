/*
 * Copyright 2019 Xilinx, Inc.
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
 * @file xcl2.hpp
 * @brief C++ wrappers for XRT OpenCL runtime routines.
 *
 * This file is part of Vitis SPARSE Library.
 */

#pragma once

#define CL_HPP_CL_1_2_DEFAULT_BUILD
#define CL_HPP_TARGET_OPENCL_VERSION 120
#define CL_HPP_MINIMUM_OPENCL_VERSION 120
#define CL_HPP_ENABLE_PROGRAM_CONSTRUCTION_FROM_ARRAY_COMPATIBILITY 1
#define CL_USE_DEPRECATED_OPENCL_1_2_APIS

// OCL_CHECK doesn't work if call has templatized function call
#define OCL_CHECK(error, call)                                                                   \
    call;                                                                                        \
    if (error != CL_SUCCESS) {                                                                   \
        printf("%s:%d Error calling " #call ", error code is: %d\n", __FILE__, __LINE__, error); \
        exit(EXIT_FAILURE);                                                                      \
    }

#include <CL/cl2.hpp>
#include <iostream>
#include <fstream>
#include <CL/cl_ext_xilinx.h>

namespace xcl {
std::vector<cl::Device> get_xil_devices();
std::vector<cl::Device> get_devices(const std::string& vendor_name);
std::vector<unsigned char> read_binary_file(const std::string& xclbin_file_name);
bool is_emulation();
bool is_hw_emulation();
bool is_xpr_device(const char* device_name);
class Stream {
   public:
    static decltype(&clCreateStream) createStream;
    static decltype(&clReleaseStream) releaseStream;
    static decltype(&clReadStream) readStream;
    static decltype(&clWriteStream) writeStream;
    static decltype(&clPollStreams) pollStreams;
    static void init(const cl_platform_id& platform) {
        void* bar = clGetExtensionFunctionAddressForPlatform(platform, "clCreateStream");
        createStream = (decltype(&clCreateStream))bar;
        bar = clGetExtensionFunctionAddressForPlatform(platform, "clReleaseStream");
        releaseStream = (decltype(&clReleaseStream))bar;
        bar = clGetExtensionFunctionAddressForPlatform(platform, "clReadStream");
        readStream = (decltype(&clReadStream))bar;
        bar = clGetExtensionFunctionAddressForPlatform(platform, "clWriteStream");
        writeStream = (decltype(&clWriteStream))bar;
        bar = clGetExtensionFunctionAddressForPlatform(platform, "clPollStreams");
        pollStreams = (decltype(&clPollStreams))bar;
    }
};
}
