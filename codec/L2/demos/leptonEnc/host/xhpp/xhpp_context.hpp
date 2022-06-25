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
/**********
           Copyright (c) 2018, Xilinx, Inc.
           All rights reserved.

           TODO

**********/

#ifndef _XHPP_CONTEXT_
#define _XHPP_CONTEXT_

#include "CL/cl.h"
#include <iostream>
#include <fstream>
#include <string>
#include <cstring>

#include "xhpp_error.hpp"
#include "xhpp_enums.hpp"

// #define _XHPP_DEBUG_
// #define _XHPP_INFO_

namespace xhpp {

// pre-declaration
namespace vbuffer {
template <class T>
class device;

template <class T>
class host;
}
namespace task {
class dev_func;
}

//! context object
class context {
    // friend class
    template <class T>
    friend class xhpp::vbuffer::device;

    template <class T>
    friend class xhpp::vbuffer::host;

    friend class xhpp::task::dev_func;

   private:
    unsigned int xplatformCount = 0; //! number of platforms
    unsigned int xplatformSelect;    //! platform selected
    cl_platform_id xplatform[16] = {0};

    unsigned int xdeviceCount = 0; //! number of devices
    cl_device_id xdeviceSelect;    //! device selected
    cl_device_id xdevice[16];

    // public:
    cl_context xcontext;     //! cl context object
    cl_command_queue xqueue; //! cl queue object
    cl_program xprogram;     //! cl program object

    std::string xclbinname; //! xclbin name
    size_t xclbinsize = 0;  //! xclbin size
    char* xclbin;

    std::string xsaname; //! xsa name

    bool _contextcreated = false;

    //    friend class xhpp::vbuffer::device;

   public:
    Pattern pattern = xhpp::linear;

    //! constructor
    context(const std::string XSA, const std::string xclbinName, Pattern patternin = xhpp::linear) {
        xsaname = XSA;
        xsaname.erase(0, xsaname.find_first_not_of(" ")); // remove space
        xsaname.erase(xsaname.find_last_not_of(" ") + 1);
        xclbinname = xclbinName;
        pattern = patternin;
    };

    //! context create
    int create() {
        createclplatform();
        createcldevice();
        createclcontext();
        createclcmdqueue();
        loadxclbin();
        createclprogram();

#ifdef _XHPP_INFO_
        std::cout << "INFO: Context create.\n";
#endif
        _contextcreated = true;
        return 0;
    };

    //! update pattern
    int pattern_update(Pattern patternin) { pattern = patternin; }

    //! context release
    int release() {
        if (_contextcreated == true) {
            releaseclprogram();
            delete xclbin;
            releaseclcmdqueue();
            releaseclcontext();
            releasecldevice();

#ifdef _XHPP_INFO_
            std::cout << "INFO: Context free.\n";
#endif
            _contextcreated = false;
        };
        return 0;
    };

    //! wait queue
    int wait() {
        clFinish(xqueue);
        return 0;
    };

    //! hw info
    int hwinfo() {
        unsigned int platformCount = 0;
        unsigned int platformSelect;
        cl_platform_id platform[16] = {0};
        unsigned int deviceCount = 0;
        cl_device_id deviceSelect;
        cl_device_id device[16];

        cl_int err;

        // platform and device
        err = clGetPlatformIDs(0, 0, &platformCount);
        if (err != CL_SUCCESS) {
            throw xhpp::error(err, "ERROR: xhpp::context::hwinfo(), clGetPlatformIDs count error.\n");
        };
        err = clGetPlatformIDs(16, platform, &platformCount);
        if (err != CL_SUCCESS) {
            throw xhpp::error(err, "ERROR: xhpp::context::hwinfo(), clGetPlatformIDs platform error.\n");
        };

        std::cout << "INFO: Number of platforms: " << platformCount << std::endl;

        char platformName[256];
        for (unsigned int i = 0; i < platformCount; i++) {
            // platform info
            err = clGetPlatformInfo(platform[i], CL_PLATFORM_NAME, 256, platformName, 0);

            err = clGetDeviceIDs(platform[i], CL_DEVICE_TYPE_ACCELERATOR, 0, NULL, &deviceCount);
            if ((err != CL_SUCCESS) || (deviceCount == 0)) {
                throw xhpp::error(err, "ERROR: xhpp::context::hwinfo(), clGetDeviceIDs count error.\n");
            };

            std::cout << "      - Platform " << i << ": " << platformName << ", number of devices: " << deviceCount
                      << std::endl;

            err = clGetDeviceIDs(platform[i], CL_DEVICE_TYPE_ACCELERATOR, deviceCount, device, NULL);
            if (err != CL_SUCCESS) {
                throw xhpp::error(err, "ERROR: xhpp::context::hwinfo(), clGetDeviceIDs device error.\n");
            };

            // devices in platform info
            for (unsigned int idev = 0; idev < deviceCount; idev++) {
                char namestr[256];
                err = clGetDeviceInfo(device[idev], CL_DEVICE_NAME, 256, namestr, 0);
                if (err != CL_SUCCESS) {
                    throw xhpp::error(err, "ERROR: xhpp::context::hwinfo(), clGetDeviceInfo error.\n");
                };
                std::cout << "        * Device " << idev << ": " << namestr << std::endl;
            };
        };
        return 0;
    };

   private:
    //! cl platform create
    int createclplatform() {
        // number of platforms
        cl_int err;
        err = clGetPlatformIDs(0, 0, &xplatformCount);
        if ((err != CL_SUCCESS) || (xplatformCount == 0)) {
            throw xhpp::error(err, "ERROR: xhpp::context::createclplatform(), clGetPlatformIDs count error.\n");
        };

        // get platforms
        err = clGetPlatformIDs(16, xplatform, &xplatformCount);
        if (err != CL_SUCCESS) {
            throw xhpp::error(err, "ERROR: xhpp::context::createclplatform(), clGetPlatformIDs platform error.\n");
        };

        // select platform
        unsigned int iplat = 0;
        for (unsigned int i = 0; i < xplatformCount; i++) {
            char platformName[256];
            cl_int err = clGetPlatformInfo(xplatform[i], CL_PLATFORM_NAME, 256, platformName, 0);
            if (strcmp(platformName, "Xilinx") != 0) {
                iplat++;
                continue; // skip non-Xilinx platform
            } else {
                xplatformSelect = i;
#ifdef _XHPP_INFO_
                printf("INFO: Selected Platform: %s\n", platformName);
#endif
                break;
            };

            if (iplat == xplatformCount) {
                throw xhpp::error("ERROR: No Xilinx platform found.\n");
            };
        };
        return 0;
    };

    //! cl device create
    int createcldevice() {
        cl_int err;
        err = clGetDeviceIDs(xplatform[xplatformSelect], CL_DEVICE_TYPE_ACCELERATOR, 0, NULL, &xdeviceCount);
        if ((err != CL_SUCCESS) || (xdeviceCount == 0)) {
            throw xhpp::error(err, "ERROR: xhpp::context::createcldevice(), clGetDeviceIDs count error.\n");
        };

        err = clGetDeviceIDs(xplatform[xplatformSelect], CL_DEVICE_TYPE_ACCELERATOR, xdeviceCount, xdevice, NULL);
        if (err != CL_SUCCESS) {
            throw xhpp::error(err, "ERROR: xhpp::context::createcldevice(), clGetDeviceIDs device error.\n");
        };

        unsigned int idev = 0;
        for (unsigned int i = 0; i < xdeviceCount; i++) {
            char namechar[256];
            err = clGetDeviceInfo(xdevice[idev], CL_DEVICE_NAME, 256, namechar, 0);
            if (err != CL_SUCCESS) {
                throw xhpp::error(err, "ERROR: xhpp::context::createcldevice(), clGetDeviceInfo error.\n");
            };

            std::string namestr(namechar);
            namestr.erase(0, namestr.find_first_not_of(" ")); // remove white space in string
            namestr.erase(namestr.find_last_not_of(" ") + 1);

            // check xsa
            if (xsaname == namestr) {
                xdeviceSelect = xdevice[idev];
                break;
            } else {
                idev++;
                continue;
            };
        };

        // check required xsa found or not
        if (idev == xdeviceCount) {
            throw xhpp::error(err, "ERROR: xhpp::context::createcldevice(), required XSA not found.\n");
        };
        return 0;
    };

    //! release cl device
    int releasecldevice() {
        for (unsigned int idev = 0; idev < xdeviceCount; idev++) {
            clReleaseDevice(xdevice[idev]);
        };
        return 0;
    };

    //! cl context create
    int createclcontext() {
        cl_int err;
        xcontext = clCreateContext(NULL, 1, &xdeviceSelect, NULL, NULL, &err);
        if (err != CL_SUCCESS) {
            throw xhpp::error(err, "ERROR: xhpp::context::createclcontext(), clCreateContext error.\n");
        };
#ifdef _XHPP_INFO_
        printf("INFO: cl Context created \n");
#endif
        return 0;
    };

    //! release cl context
    int releaseclcontext() {
        cl_int err = clReleaseContext(xcontext);
        if (err != CL_SUCCESS) {
            throw xhpp::error(err, "ERROR: xhpp::context::releaseclcontext(), clReleaseContext error.\n");
        };
        return 0;
    };

    //! cl command queue create
    int createclcmdqueue() {
        cl_int err;
        xqueue = clCreateCommandQueue(xcontext, xdeviceSelect, CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE, &err);
        if (err != CL_SUCCESS) {
            throw xhpp::error(err, "ERROR: xhpp::context::createclcmdqueue(), clCreateCommandQueue error.\n");
        };
#ifdef _XHPP_INFO_
        printf("INFO: cl Command Queue created \n");
#endif
        return 0;
    };

    //! release cl command queue
    int releaseclcmdqueue() {
        cl_int err = clReleaseCommandQueue(xqueue);
        if (err != CL_SUCCESS) {
            throw xhpp::error(err, "ERROR: xhpp::context::releaseclcmdqueue(), clReleaseCommandQueue error.\n");
        };
        return 0;
    };

    //! cl program create
    int createclprogram() {
        cl_int err;
        xprogram = clCreateProgramWithBinary(xcontext, 1, &xdeviceSelect, &xclbinsize, (const unsigned char**)&xclbin,
                                             0, &err);
        if (err != CL_SUCCESS) {
            throw xhpp::error(err, "ERROR: xhpp::context::createclprogram(), clCreateProgramWithBinary error.\n");
        };
#ifdef _XHPP_INFO_
        printf("INFO: Program created \n");
#endif
        return 0;
    };

    //! release cl program
    int releaseclprogram() {
        int err = clReleaseProgram(xprogram);
        if (err != CL_SUCCESS) {
            throw xhpp::error(err, "ERROR: xhpp::context::releasecldevice(), clReleaseProgram error.\n");
        };
        return 0;
    };

    //! load xclbin
    int loadxclbin() {
        std::ifstream stream(xclbinname, std::ifstream::binary);
        if (!stream) {
            throw xhpp::error("ERROR: xhpp::context::loadxclbin(), find xclbin error.\n");
        };

        stream.seekg(0, stream.end);
        xclbinsize = stream.tellg();
        stream.seekg(0, stream.beg);

        xclbin = new char[xclbinsize + 1];
        stream.read(xclbin, xclbinsize);
        if (!stream) {
            throw xhpp::error("ERROR: xhpp::context::loadxclbin(), read xclbin error.\n");
        };
        stream.close();
        xclbin[xclbinsize] = 0;

#ifdef _XHPP_INFO_
        printf("INFO: xclbin load \n");
#endif
        return 0;
    };

}; // end of class context
}; // end of namespace

#endif
