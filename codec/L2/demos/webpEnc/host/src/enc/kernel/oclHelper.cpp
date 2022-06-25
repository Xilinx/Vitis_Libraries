
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

#include "oclHelper.h"
#include <cstring>
#include <fstream>
#include <iostream>
#include <vector>
#include <stdio.h>

static int loadFile2Memory(const char* filename, char** result) {
    int size = 0;

    std::ifstream stream(filename, std::ifstream::binary);
    if (!stream) {
        return -1;
    }

    stream.seekg(0, stream.end);
    size = stream.tellg();
    stream.seekg(0, stream.beg);

    *result = new char[size + 1];
    stream.read(*result, size);
    if (!stream) {
        return -2;
    }
    stream.close();
    (*result)[size] = 0;
    return size;
}

static void getDeviceVersion(oclHardware& hardware) {
    char versionString[512];
    size_t size = 0;
    cl_int err = clGetDeviceInfo(hardware.mDevice, CL_DEVICE_VERSION, 511, versionString, &size);
    if (err != CL_SUCCESS) {
        std::cout << oclErrorCode(err) << "\n";
        return;
    }
    unsigned major = 0;
    unsigned minor = 0;
    unsigned state = 0;
    for (size_t i = 0; i < size; i++) {
        if (versionString[i] == ' ') {
            state++;
            continue;
        }
        if (versionString[i] == '.') {
            state++;
            continue;
        }
        if (state == 0) {
            continue;
        }
        if (state == 1) {
            major *= 10;
            major += (versionString[i] - '0');
            continue;
        }
        if (state == 2) {
            minor *= 10;
            minor += (versionString[i] - '0');
            continue;
        }
        break;
    }
    hardware.mMajorVersion = major;
    hardware.mMinorVersion = minor;
}

static void getDeviceInfo(oclHardware& hardware) {
    cl_ulong localMemSize;
    cl_ulong allocMemSize;
    cl_ulong globalMemSize;
    size_t groupSize;
    cl_uint unitsSize;
    cl_uint maxDimension;
    size_t itemsize[] = {0, 0, 0};
    size_t size = 0;
    cl_int err =
        clGetDeviceInfo(hardware.mDevice, CL_DEVICE_LOCAL_MEM_SIZE, sizeof(localMemSize), &localMemSize, &size);
    if (err != CL_SUCCESS) {
        std::cout << oclErrorCode(err) << "\n";
        return;
    }
    err = clGetDeviceInfo(hardware.mDevice, CL_DEVICE_MAX_WORK_GROUP_SIZE, sizeof(groupSize), &groupSize, &size);
    if (err != CL_SUCCESS) {
        std::cout << oclErrorCode(err) << "\n";
        return;
    }
    err = clGetDeviceInfo(hardware.mDevice, CL_DEVICE_MAX_COMPUTE_UNITS, sizeof(unitsSize), &unitsSize, &size);
    if (err != CL_SUCCESS) {
        std::cout << oclErrorCode(err) << "\n";
        return;
    }
    err = clGetDeviceInfo(hardware.mDevice, CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS, sizeof(maxDimension), &maxDimension,
                          &size);
    if (err != CL_SUCCESS) {
        std::cout << oclErrorCode(err) << "\n";
        return;
    }
    err = clGetDeviceInfo(hardware.mDevice, CL_DEVICE_MAX_WORK_ITEM_SIZES, sizeof(itemsize), itemsize, &size);
    if (err != CL_SUCCESS) {
        std::cout << oclErrorCode(err) << "\n";
        return;
    }
    err = clGetDeviceInfo(hardware.mDevice, CL_DEVICE_MAX_MEM_ALLOC_SIZE, sizeof(allocMemSize), &allocMemSize, &size);
    if (err != CL_SUCCESS) {
        std::cout << oclErrorCode(err) << "\n";
        return;
    }
    err = clGetDeviceInfo(hardware.mDevice, CL_DEVICE_GLOBAL_MEM_SIZE, sizeof(globalMemSize), &globalMemSize, &size);
    if (err != CL_SUCCESS) {
        std::cout << oclErrorCode(err) << "\n";
        return;
    }
    // fprintf(stderr, "Max compute units size: %d\n", unitsSize);
    // fprintf(stderr, "Max work group size: %d\n", groupSize);
    // fprintf(stderr, "Local memory size: %d\n", localMemSize);
    // fprintf(stderr, "Global memory size: %d\n", globalMemSize);
    // fprintf(stderr, "Max alloc memory size: %d\n", allocMemSize);
    // fprintf(stderr, "Max item dimensions: %d\n", maxDimension);
    // fprintf(stderr, "Max item size: %d %d %d\n", itemsize[0], itemsize[1], itemsize[2]);
}

static int compileProgram(const oclHardware& hardware, oclSoftware& software) {
    cl_int err = clBuildProgram(software.mProgram, 1, &hardware.mDevice, software.mCompileOptions, 0, 0);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "%s %d %s\n", __func__, __LINE__, oclErrorCode(err));
        size_t size = 0;
        err = clGetProgramBuildInfo(software.mProgram, hardware.mDevice, CL_PROGRAM_BUILD_LOG, 0, 0, &size);
        if (err != CL_SUCCESS) {
            fprintf(stderr, "%s %d %s\n", __func__, __LINE__, oclErrorCode(err));
            return -1;
        }

        std::vector<char> log(size + 1);
        err = clGetProgramBuildInfo(software.mProgram, hardware.mDevice, CL_PROGRAM_BUILD_LOG, size, &log[0], 0);
        if (err != CL_SUCCESS) {
            fprintf(stderr, "%s %d %s\n", __func__, __LINE__, oclErrorCode(err));
            return -1;
        }

        std::cout << &log[0] << "\n";
        return -1;
    }

    return 0;
}

oclHardware getOclHardware(cl_device_type type, char* target_device) {
    xf::common::utils_sw::Logger logger(std::cerr);
    cl_int err;

    oclHardware hardware = {0, 0, 0, 0, 0, 0};
    cl_platform_id platforms[16] = {0};
    cl_device_id devices[16];
    cl_device_id device_id;
    char platformName[256];
    char deviceName[256];
    cl_uint platformCount = 0;
    err = clGetPlatformIDs(0, 0, &platformCount);
    err = clGetPlatformIDs(16, platforms, &platformCount);
    if (err != CL_SUCCESS) {
        std::cout << oclErrorCode(err) << "\n";
        return hardware;
    }

    fprintf(stderr, "INFO: Number of Platforms: %d\n", platformCount);

    for (cl_uint i = 0; i < platformCount; i++) {
        err = clGetPlatformInfo(platforms[i], CL_PLATFORM_NAME, 256, platformName, 0);
        if (err != CL_SUCCESS) {
            std::cout << oclErrorCode(err) << "\n";
            return hardware;
        }

        if (strcmp(platformName, "Xilinx") != 0) {
            // skip non-Xilinx platform
            continue;
        }

        fprintf(stderr, "INFO: Selected Platform: %s\n", platformName);

        // iterate all devices to find the target device
        cl_uint deviceCount = 0;
        err = clGetDeviceIDs(platforms[i], type, 0, NULL, &deviceCount);
        if ((err != CL_SUCCESS) || (deviceCount == 0)) {
            fprintf(stderr, "ERROR: clGetDeviceIDs count error: %s %d %s\n", __func__, __LINE__, oclErrorCode(err));
            continue;
        };

        fprintf(stderr, "INFO: Number of devices for platform %d: %d\n", i, deviceCount);
        err = clGetDeviceIDs(platforms[i], type, deviceCount, devices, NULL);
        if (err != CL_SUCCESS) {
            fprintf(stderr, "ERROR: clGetDeviceIDs device error: %s %d %s\n", __func__, __LINE__, oclErrorCode(err));
            continue;
        }

        cl_uint idev;
        for (idev = 0; idev < deviceCount; idev++) {
            err = clGetDeviceInfo(devices[idev], CL_DEVICE_NAME, 256, deviceName, 0);
            if (err != CL_SUCCESS) {
                fprintf(stderr, "ERROR: clGetDeviceInfo: %s %d %s\n", __func__, __LINE__, oclErrorCode(err));
                return hardware;
            }

            fprintf(stderr, "INFO: target_device found:   %s\n", deviceName);

            if (strcmp(deviceName, "xilinx_u200_gen3x16_xdma_base_1") == 0 ||
                strcmp(deviceName, "xilinx_u200_gen3x16_xdma_base_2") == 0 ||
                strcmp(deviceName, "xilinx_u200_gen3x16_xdma_1_202110_1") == 0 ||
                strcmp(deviceName, "xilinx_u200_gen3x16_xdma_2_202110_1") == 0) {
                device_id = devices[idev];
                fprintf(stderr, "INFO: target_device chosen:  %s\n", deviceName);
                break;
            }
        }

        if (idev == deviceCount) {
            fprintf(stderr, "ERROR: target device %s not found \n: %s %d %s", target_device, __func__, __LINE__,
                    oclErrorCode(err));
            return hardware;
        }

        cl_context_properties contextData[3] = {CL_CONTEXT_PLATFORM, (cl_context_properties)platforms[i], 0};
        // cl_context context = clCreateContextFromType(contextData, type, 0, 0, &err);
        cl_context context = clCreateContext(NULL, 1, &device_id, NULL, NULL, &err);
        logger.logCreateContext(err);
        if (err != CL_SUCCESS) {
            continue;
        }

        // cl_command_queue queue = clCreateCommandQueue(context, device_id, 0, &err);
        cl_command_queue queue = clCreateCommandQueue(context, device_id, CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE, &err);
        logger.logCreateCommandQueue(err);
        if (err != CL_SUCCESS) {
            std::cout << oclErrorCode(err) << "\n";
            return hardware;
        }

        hardware.mPlatform = platforms[i];
        hardware.mContext = context;
        hardware.mDevice = device_id;
        hardware.mQueue = queue;
        getDeviceVersion(hardware);
        fprintf(stderr, "INFO: OpenCL Version: %d.%d\n", hardware.mMajorVersion, hardware.mMinorVersion);
        return hardware;
    }
    return hardware;
}

extern "C" int getOclSoftware(oclSoftware& soft, const oclHardware& hardware) {
    xf::common::utils_sw::Logger logger(std::cerr);
    cl_int err;

    cl_device_type deviceType = CL_DEVICE_TYPE_DEFAULT;
    err = clGetDeviceInfo(hardware.mDevice, CL_DEVICE_TYPE, sizeof(deviceType), &deviceType, 0);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "%s %d %s\n", __func__, __LINE__, oclErrorCode(err));
        return -1;
    }

    unsigned char* kernelCode = 0;
    fprintf(stderr, "INFO: Loading %s\n", soft.mFileName);

    int size = loadFile2Memory(soft.mFileName, (char**)&kernelCode);
    if (size < 0) {
        fprintf(stderr, "Failed to load kernel\n");
        return -2;
    }

    fprintf(stderr, "INFO: Loading %s Finished\n", soft.mFileName);

    if (deviceType == CL_DEVICE_TYPE_ACCELERATOR) {
        size_t n = size;
        soft.mProgram = clCreateProgramWithBinary(hardware.mContext, 1, &hardware.mDevice, &n,
                                                  (const unsigned char**)&kernelCode, 0, &err);
        logger.logCreateProgram(err);
    } else {
        soft.mProgram = clCreateProgramWithSource(hardware.mContext, 1, (const char**)&kernelCode, 0, &err);
        logger.logCreateProgram(err);
    }
    if (!soft.mProgram || (err != CL_SUCCESS)) {
        fprintf(stderr, "%s %d %s\n", __func__, __LINE__, oclErrorCode(err));
        delete[] kernelCode;
        return -3;
    }

    err = compileProgram(hardware, soft);
    delete[] kernelCode;
    return err;
}

extern "C" void releaseSoftware(oclSoftware& software) {
    clReleaseProgram(software.mProgram);
}

extern "C" void releaseKernel(oclKernelInfo& kernelinfo) {
    clReleaseKernel(kernelinfo.mKernel);
}

extern "C" void releaseHardware(oclHardware& hardware) {
    clReleaseCommandQueue(hardware.mQueue);
    clReleaseContext(hardware.mContext);
    if ((hardware.mMajorVersion >= 1) && (hardware.mMinorVersion > 1)) {
        // Only available in OpenCL >= 1.2
        clReleaseDevice(hardware.mDevice);
    }
}
