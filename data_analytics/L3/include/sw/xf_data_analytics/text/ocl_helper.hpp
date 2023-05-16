/*
 * Copyright (C) 2019-2022, Xilinx, Inc.
 * Copyright (C) 2022-2023, Advanced Micro Devices, Inc.
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
#ifndef XF_TEXT_OCL_HELPER_HPP
#define XF_TEXT_OCL_HELPER_HPP

// Include xilinx header first, to define CL_USE_DEPRECATED_OPENCL_1_2_APIS
#include <CL/cl_ext_xilinx.h>
#include <CL/cl.h>

#include <cstddef>
#include <cstdlib>
#include <new>

#include <sys/time.h>

#include <algorithm>
#include <string>
#include <vector>

#include <sys/types.h>
#include <sys/stat.h>
#include "xf_utils_sw/logger.hpp"

namespace xf {
namespace data_analytics {
namespace text {

// helper functions for internal use.
namespace details {

inline unsigned long read_binary_file(const char* fname, void** buffer) {
    unsigned long size = 0;
    FILE* fp = fopen(fname, "rb");
    if (!fp) {
        fprintf(stderr, "File %s cannot be opened for read.\n", fname);
    }
    fseek(fp, 0, SEEK_END);
    size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    *buffer = (void*)malloc(size);
    fread(*buffer, 1, size, fp);
    fclose(fp);
    return size;
}

inline cl_int init_hardware(cl_context* context,
                            cl_device_id* device_id,
                            cl_command_queue* cmd_queue,
                            cl_command_queue_properties queue_props,
                            const int dev_index) {
    cl_int err;
    xf::common::utils_sw::Logger logger(std::cout, std::cerr);

    cl_uint platform_count = 0;
    err = clGetPlatformIDs(0, NULL, &platform_count);
    platform_count = platform_count > 16 ? 16 : platform_count;
    cl_platform_id platforms[16] = {0};
    err = clGetPlatformIDs(platform_count, platforms, NULL);

    cl_uint pid;
    char platform_name[256];
    for (pid = 0; pid < platform_count; ++pid) {
        err = clGetPlatformInfo(platforms[pid], CL_PLATFORM_NAME, 256, platform_name, 0);
        if (strcmp(platform_name, "Xilinx")) continue;
        // platform found.
        const cl_device_type device_type = CL_DEVICE_TYPE_ACCELERATOR;
        cl_uint device_count;
        err = clGetDeviceIDs(platforms[pid], device_type, 0, NULL, &device_count);
        cl_device_id devices[16] = {0};
        device_count = device_count > 16 ? 16 : device_count;
        err = clGetDeviceIDs(platforms[pid], device_type, device_count, devices, NULL);
        char device_name[256];
        err = clGetDeviceInfo(devices[dev_index], CL_DEVICE_NAME, 256, device_name, 0);
        printf("INFO: selected device %s\n", device_name);
        *device_id = devices[dev_index];
        // device found.
        cl_context_properties ctx_prop[3] = {CL_CONTEXT_PLATFORM, (cl_context_properties)platforms[pid], 0};
        cl_context ctx = clCreateContext(ctx_prop, 1, device_id, NULL, NULL, &err);
        logger.logCreateContext(err);
        if (err != CL_SUCCESS) {
            return err;
        }
        printf("INFO: initilized context.\n");
        // context ready.
        cl_command_queue q = clCreateCommandQueue(ctx, devices[dev_index], queue_props, &err);
        logger.logCreateCommandQueue(err);
        if (err != CL_SUCCESS) {
            fprintf(stderr, "ERROR: Failed to create command queue.\n");
            return err;
        }
        printf("INFO: initilized command queue.\n");
        // queue ready.
        *context = ctx;
        *cmd_queue = q;
        break;
    }
    if (pid == platform_count) {
        fprintf(stderr, "ERROR: Xilinx platform not found.\n");
        return CL_INVALID_PLATFORM;
    }
    return err;
}

static cl_int load_binary(cl_program* program, cl_context context, cl_device_id device_id, const char* xclbin) {
    cl_int err;
    xf::common::utils_sw::Logger logger(std::cout, std::cerr);

    void* kernel_image = NULL;
    unsigned long size = read_binary_file(xclbin, &kernel_image);
    cl_program prog =
        clCreateProgramWithBinary(context, 1, &device_id, &size, (const unsigned char**)&kernel_image, NULL, &err);
    logger.logCreateProgram(err);
    if (err != CL_SUCCESS) {
        return err;
    }
    printf("INFO: created program with binary %s\n", xclbin);

    err = clBuildProgram(prog,       // program
                         1,          // number of devices
                         &device_id, // devices list
                         NULL,       // compile options
                         NULL,       // pfn_notify
                         NULL);      // user_data
    if (err != CL_SUCCESS) {
        free(kernel_image);
        return err;
    }
    printf("INFO: built program.\n");
    *program = prog;
    free(kernel_image);
    return err;
}

} /* details */
} // namespace text
} // namespace data_analytics
} // namespace xf

#endif
