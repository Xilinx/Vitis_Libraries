
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

#ifndef _OCL_HELP_H_
#define _OCL_HELP_H_

#include <CL/cl.h>
#include "vp8_AsyncConfig.h"
#include "xf_utils_sw/logger.hpp"

struct oclHardware {
    cl_platform_id mPlatform;
    cl_context mContext;
    cl_device_id mDevice;
    cl_command_queue mQueue;
    short mMajorVersion;
    short mMinorVersion;
};

struct oclSoftware {
    cl_program mProgram;
    char mCompileOptions[1024];
    char mFileName[1024];
};

struct oclKernelInfo {
    cl_kernel mKernel;
    char mKernelName[128];
    cl_kernel mKernel2;
    char mKernelName2[128];

    cl_kernel* mKernelPred; //[NasyncDepth*Ninstances];
    cl_kernel* mKernelAC;   //[NasyncDepth*Ninstances];
};

extern "C" oclHardware getOclHardware(cl_device_type type, char* target_device);

extern "C" int getOclSoftware(oclSoftware& software, const oclHardware& hardware);

extern "C" void releaseSoftware(oclSoftware& software);

extern "C" void releaseKernel(oclKernelInfo& kernelinfo);

extern "C" void releaseHardware(oclHardware& hardware);

extern "C" const char* oclErrorCode(cl_int code);

#endif
